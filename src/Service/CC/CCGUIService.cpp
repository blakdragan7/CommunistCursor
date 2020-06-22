#include "CCGUIService.h"

#include "../Socket/Socket.h"

#include "CCDisplay.h"
#include "CCNetworkEntity.h"

#include "IGuiServiceInterface.h"

#include "../OSInterface/OSInterface.h"

#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <future>

#include <algorithm>

#ifdef _WIN32

#define GUI_PROGRAM_PATH "bin/CCGui.exe"

#endif

void CCGuiService::SocketAcceptThread()
{
	while (_shouldRunServer)
	{
		Socket* acceptedSocket = 0;
		SocketError error = _serverSocket->Accept(&acceptedSocket);
		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			std::cout << "Error accepting Socket listen on GUI Port " << _serverSocket->GetPort() << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
			if (error == SocketError::SOCKET_E_BROKEN_PIPE)continue;
			else break;
		}
	
		std::async([acceptedSocket, this]() {
			using namespace nlohmann;

			json entites;
			std::vector<json> entityJsons;

			const std::vector<std::shared_ptr<CCNetworkEntity>>& entitesToConfigure = _delegate->GetEntitiesToConfigure();
			const std::vector<int>& globalBounds = _delegate->GetGlobalBounds();

			for (auto entity : entitesToConfigure)
			{
				json entityJson;

				entityJson["id"] = entity->GetID();

				std::vector<json> displays;

				for (auto display : entity->GetAllDisplays())
				{
					json jDisplay;
					const Rect& dBounds = display->GetCollision();
					const NativeDisplay& nDisplay = display->GetNativeDisplay();

					jDisplay["bounds"] = { dBounds.topLeft.x, dBounds.topLeft.y, nDisplay.width, nDisplay.height };
					jDisplay["id"] = display->GetAssignedID();

					displays.push_back(jDisplay);
				}

				entityJson["displays"] = displays;
				entityJsons.push_back(entityJson);
			}

			entites["entites"] = entityJsons;
			entites["globalBounds"] = globalBounds;

			SocketError error = acceptedSocket->Send(entites.dump());
			if (error != SocketError::SOCKET_E_SUCCESS)
			{
				std::cout << "Error sending json packet " << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
				return false;
			}

			char buff[1024] = { 0 };
			size_t received = 0;
			error = acceptedSocket->Recv(buff, 1024, &received);
			if (error != SocketError::SOCKET_E_SUCCESS)
			{
				std::cout << "Error receiving json packet " << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
				return false;
			}

			// app was closed before we finished
			if (received == 0)
			{
				// clearly we don't want to confiure to we just return success
				return true;
			}

			json offsets = json::parse(buff);

			for (auto entity : entitesToConfigure)
			{
				auto itr = offsets.find(entity->GetID());
				if (itr != offsets.end())
				{
					auto offset = itr.value();
					auto displays = entity->GetAllDisplays();

					for (int i = 0; i < displays.size(); i++)
					{
						auto display = displays[i];
						display->SetOffsets(offset[0], offset[1]);
					}
				}
			}

			delete acceptedSocket;
		});
	}
}

CCGuiService::CCGuiService(IGuiServiceInterface* _delegate, int guiPort, std::string address) : _shouldRunServer(false), _delegate(_delegate), \
_serverSocket(new Socket(address, guiPort, false, SocketProtocol::SOCKET_P_TCP)), _guiServerPort(guiPort), _guiServerAddress(address)
{
}

bool CCGuiService::StartGUIServer()
{
	SocketError error = _serverSocket->Bind();

	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		std::cout << "Error trying to bind to GUI Port " << _serverSocket->GetPort() << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
		return false;
	}

	error = _serverSocket->Listen();
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		std::cout << "Error trying listen on GUI Port " << _serverSocket->GetPort() << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
		return false;
	}

	_serverSocketThread = std::thread(&CCGuiService::SocketAcceptThread, this);

	return true;
}

bool CCGuiService::StopGUIServer()
{
	_serverSocket->Disconnect();
	_shouldRunServer = false;
	return false;
}

bool CCGuiService::StartGuiClient() const
{
	ProccessInfo info = { 0 };
	std::string programArgs = GUI_PROGRAM_PATH + _guiServerAddress + " " + std::to_string(_guiServerPort);

	OSInterfaceError oError = OSInterface::SharedInterface().StartProcessAsDesktopUser(GUI_PROGRAM_PATH, programArgs.c_str(), "", true, &info);

	if (oError != OSInterfaceError::OS_E_SUCCESS)
	{
		std::cout << "Error Starting GUI Process " << OSInterfaceErrorToString(oError) << std::endl;
		return false;
	}

	return true;
}
