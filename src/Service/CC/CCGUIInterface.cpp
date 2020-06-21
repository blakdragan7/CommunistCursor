#include "CCGUIInterface.h"

#include "../Socket/Socket.h"

#include "CCDisplay.h"
#include "CCNetworkEntity.h"

#include "../OSInterface/OSInterface.h"

#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <future>

#include <algorithm>

#ifdef _WIN32

#define GUI_PROGRAM_PATH "GUI/Main.exe"
#define GUI_PROGRAM_ARGS "D:/Python37/python.exe D:\\git\\CommunistCursor\\src\\GUI\\Main.py"

#endif

CCGuiInterface::CCGuiInterface(int guiPort, std::string address) : _serverSocket(new Socket(address, guiPort, false, SocketProtocol::SOCKET_P_TCP))
{
}

bool CCGuiInterface::StartGUI(const std::vector<std::shared_ptr<CCNetworkEntity>>& entitesToConfigure, std::vector<int> globalBounds)
{
	auto guiThreadRes = std::async([&]() {
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

		Socket* acceptedSocket = 0;
		error = _serverSocket->Accept(&acceptedSocket);
		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			std::cout << "Error accepting Socket listen on GUI Port " << _serverSocket->GetPort() << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
			return false;
		}

		_clientSocket.reset(acceptedSocket);

		_serverSocket->Disconnect();

		return true;
	});

	ProccessInfo info = {0};
	OSInterfaceError oError = OSInterface::SharedInterface().StartProcessAsDesktopUser(GUI_PROGRAM_PATH, GUI_PROGRAM_ARGS, "", true, &info);

	if (oError != OSInterfaceError::OS_E_SUCCESS)
	{
		std::cout << "Error Starting GUI Process " << OSInterfaceErrorToString(oError) << std::endl;
		return false;
	}

	guiThreadRes.wait();
	
	if (guiThreadRes.get() == false)
		return false;

	using namespace nlohmann;

	json entites;
	std::vector<json> entityJsons;

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

	SocketError error = _clientSocket->Send(entites.dump());
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		std::cout << "Error sending json packet " << SOCK_ERR_STR(_serverSocket.get(), error) << std::endl;
		return false;
	}

	char buff[1024] = { 0 };
	size_t received = 0;
	error = _clientSocket->Recv(buff, 1024, &received);
	if (error != SocketError::SOCKET_E_SUCCESS )
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

	return true;
}

bool CCGuiInterface::WaitForConfigurationToFinish()
{
	return false;
}
