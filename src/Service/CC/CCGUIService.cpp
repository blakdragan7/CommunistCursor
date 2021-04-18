#include "CCGUIService.h"

#include "../Socket/Socket.h"

#include "CCLogger.h"
#include "CCDisplay.h"
#include "CCNetworkEntity.h"

#include "IGuiServiceInterface.h"

#include "../Dispatcher/DispatchManager.h"

#include "../OSInterface/OSInterface.h"


#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include <algorithm>

#ifdef _WIN32

#define GUI_PROGRAM_PATH "bin/CCGui.exe"

#else

#define GUI_PROGRAM_PATH "bin/CCGui.exe"

#endif

void CCGuiService::SocketAcceptThread()
{
	Socket* acceptedSocket = 0;
	SocketError error = _serverSocket->Accept(&acceptedSocket);
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		LOG_ERROR << "Error accepting Socket listen on GUI Port " << _serverSocket->Port() << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
		if (error == SocketError::SOCKET_E_BROKEN_PIPE)
			return;
	}
	
	//DISPATCH_ASYNC(([acceptedSocket, this]() {
		using namespace nlohmann;

		json entites;
		std::vector<json> entityJsons;

		LOG_DEBUG << "GUI Socket Accepted" << std::endl;

		const std::vector<std::shared_ptr<CCNetworkEntity>>& entitesToConfigure = _delegate->GetEntitiesToConfigure();
		const std::vector<int>& globalBounds = _delegate->GetGlobalBounds();

		for (auto& entity : entitesToConfigure)
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

		/*SocketError*/ error = acceptedSocket->Send(entites.dump());
		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			LOG_ERROR << "Error sending json packet " << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
			return;
		}

		char buff[1024] = { 0 };
		size_t received = 0;
		error = acceptedSocket->Recv(buff, 1024, &received);
		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			LOG_ERROR << "Error receiving json packet " << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
			return;
		}

		// app was closed before we finished
		if (received == 0)
		{
			LOG_DEBUG << "No GUI Config Received" << std::endl;
			// clearly we don't want to confiure to we just return success
			return;
		}

		LOG_DEBUG << "GUI Service Received " << buff << std::endl;

		json offsets = json::parse(buff);

		for (auto& entity : entitesToConfigure)
		{
			auto itr = offsets.find(entity->GetID());
			if (itr != offsets.end())
			{
				auto offset = itr.value();
				entity->SetDisplayOffsets({ offset[0],offset[1] });
			}
		}

		delete acceptedSocket;

		_delegate->EntitiesFinishedConfiguration();
	//}));
	
	if (_shouldRunServer)
		DISPATCH_ASYNC(std::bind(&CCGuiService::SocketAcceptThread, this));
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
		LOG_ERROR << "Error trying to bind to GUI Port " << _serverSocket->Port() << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
		return false;
	}

	error = _serverSocket->Listen();
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		LOG_ERROR << "Error trying listen on GUI Port " << _serverSocket->Port() << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
		return false;
	}

	_shouldRunServer = true;

	DISPATCH_ASYNC(std::bind(&CCGuiService::SocketAcceptThread, this));

	return true;
}

bool CCGuiService::StopGUIServer()
{
	_shouldRunServer = false;
	_serverSocket->Disconnect();
	return false;
}

bool CCGuiService::StartGuiClient() const
{
	ProccessInfo info = { 0 };
	std::string programArgs = GUI_PROGRAM_PATH + _guiServerAddress + " " + std::to_string(_guiServerPort);

	OSInterfaceError oError = OSInterface::SharedInterface().StartProcessAsDesktopUser(GUI_PROGRAM_PATH, programArgs.c_str(), "", true, &info);

	if (oError != OSInterfaceError::OS_E_SUCCESS)
	{
		LOG_ERROR << "Error Starting GUI Process " << OSInterfaceErrorToString(oError) << std::endl;
		return false;
	}

	return true;
}
