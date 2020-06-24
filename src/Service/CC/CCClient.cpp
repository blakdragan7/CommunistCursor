#include "CCClient.h"
#include "../Socket/Socket.h"
#include "../OSInterface/OSInterface.h"
#include "../OSInterface/PacketTypes.h"

#include "CCPacketTypes.h"

#include <iostream>

CCClient::CCClient(int listenPort) : _serverAddress("0.0.0.0"), _listenPort(listenPort)
{
	auto error = OSInterface::SharedInterface().GetNativeDisplayList(_displayList);
	if (error != OSInterfaceError::OS_E_SUCCESS)
	{
		std::string errorMessage = "Error Getting Display List " + OSInterfaceErrorToString(error);
		throw std::exception(errorMessage.c_str());
	}
}

void CCClient::ConnectToServer(std::string address, int port)
{
	Socket servSocket(address, port, false, SocketProtocol::SOCKET_P_TCP);

	SocketError error = servSocket.Connect();
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		std::cout << "Error Trying To Connect To Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
		return;
	}

	DisplayListHeaderPacket listHeader;
	listHeader.NumberOfDisplays = (int)_displayList.size();

	std::string hostName;
	OSInterfaceError osError = OSInterface::SharedInterface().GetLocalHostName(hostName);
	if (osError != OSInterfaceError::OS_E_SUCCESS)
	{
		std::cout << "Error Trying to get Local Host Name\n";
		return;
	}

	EntityIDPacket idPacket(hostName);

	error = servSocket.Send((char*)&idPacket, sizeof(EntityIDPacket));
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		std::cout << "Error Trying To Send ID Packet To Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
		return;
	}

	AddressPacket addPacket;
	memcpy(addPacket.Address, INVALID_PACKET_ADDRESS, INVALID_PACKET_ADDRESS_SIZE);
	addPacket.Address[8] = 0;
	// this port will eventually be configurable but for now, random numbers
	addPacket.Port = _listenPort;

	error = servSocket.Send((char*)&addPacket, sizeof(AddressPacket));
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		std::cout << "Error Trying To Send Address Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
		return;
	}

	error = servSocket.Send((char*)&listHeader, sizeof(DisplayListHeaderPacket));
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		std::cout << "Error Trying To Send List Header To Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
		return;
	}

	for (auto display : _displayList)
	{
		DisplayListDisplayPacket displayPacket;

		displayPacket.Top = display.posY;
		displayPacket.Left = display.posX;
		displayPacket.Width = display.width;
		displayPacket.Height = display.height;
		displayPacket.NativeDisplayID = display.nativeScreenID;

		error = servSocket.Send((char*)&displayPacket, sizeof(DisplayListDisplayPacket));
		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			std::cout << "Error Trying To Send Display Info To Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
			return;
		}
	}

	// server will close socket on it's end when it receives everything
	// we wait for it here
	error = servSocket.WaitForServer();
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		std::cout << "Error Trying To Wait For Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
		return;
	}

	_serverAddress = address;
}

bool CCClient::ListenForOSEvent(OSEvent& newEvent)
{
	if (_internalSocket.get() == NULL)
	{
		_internalSocket.reset(new Socket(_serverAddress, _listenPort, false, SocketProtocol::SOCKET_P_UDP));
		SocketError error = _internalSocket->Connect();
		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			std::cout << "Error Trying Receive Event Header From Server !: " << SOCK_ERR_STR(_internalSocket.get(), error) << std::endl;
			_internalSocket.reset();
			return false;
		}
	}

	EventPacketHeader eventHeader;
	size_t received = 0;
	SocketError error = _internalSocket->Recv((char*)&eventHeader, sizeof(EventPacketHeader), &received);

	if (error != SocketError::SOCKET_E_SUCCESS || received != sizeof(EventPacketHeader))
	{
		std::cout << "Error Trying Receive Event Header From Server !: " << SOCK_ERR_STR(_internalSocket.get(), error) << std::endl;
		return false;
	}

	if (eventHeader.incomming_event_type == EVENT_PACKET_MM)
	{
		MouseMoveEventPacket moveEvent;

		SocketError error = _internalSocket->RecvFrom((char*)&eventHeader, sizeof(EventPacketHeader), &received);

		if (error != SocketError::SOCKET_E_SUCCESS || received != sizeof(MouseMoveEventPacket))
		{
			std::cout << "Error Trying To Receive Mouse Move Event From Server: " << SOCK_ERR_STR(_internalSocket.get(), error) << std::endl;
			return false;
		}

		memset(&newEvent, 0, sizeof(OSEvent));
		newEvent.eventType = OS_EVENT_MOUSE;
		newEvent.subEvent.mouseEvent = MOUSE_EVENT_MOVE;
		newEvent.deltaX = moveEvent.posX;
		newEvent.deltaY = moveEvent.posY;
		newEvent.nativeScreenID = moveEvent.nativeScreenID;

		return true;
	}
	else if (eventHeader.incomming_event_type == EVENT_PACKET_MB)
	{
		MouseButtonEventPacket mouseButtonEvent;

		SocketError error = _internalSocket->RecvFrom((char*)&mouseButtonEvent, sizeof(MouseButtonEventPacket), &received);

		if (error != SocketError::SOCKET_E_SUCCESS || received != sizeof(MouseButtonEventPacket))
		{
			std::cout << "Error Trying To Receive Mouse Button Event From Server: " << SOCK_ERR_STR(_internalSocket.get(), error) << std::endl;
			return false;
		}

		memset(&newEvent, 0, sizeof(OSEvent));
		newEvent.eventType = OS_EVENT_MOUSE;
		newEvent.subEvent.mouseEvent = mouseButtonEvent.isDown ? MOUSE_EVENT_DOWN : MOUSE_EVENT_UP;
		newEvent.eventButton.mouseButton = (MouseButton)mouseButtonEvent.mouseButton;

		return true;
	}
	else if (eventHeader.incomming_event_type == EVENT_PACKET_MW)
	{
		MouseWheelEventPacket mouseWheelEvent;

		SocketError error = _internalSocket->RecvFrom((char*)&mouseWheelEvent, sizeof(MouseWheelEventPacket), &received);

		if (error != SocketError::SOCKET_E_SUCCESS || received != sizeof(MouseWheelEventPacket))
		{
			std::cout << "Error Trying To Receive Mouse Wheel Event From Server: " << SOCK_ERR_STR(_internalSocket.get(), error) << std::endl;
			return false;
		}

		memset(&newEvent, 0, sizeof(OSEvent));
		newEvent.eventType = OS_EVENT_MOUSE;
		newEvent.subEvent.mouseEvent = MOUSE_EVENT_SCROLL;
		newEvent.eventButton.mouseButton = MOUSE_BUTTON_MIDDLE;

		return true;
	}
	else if (eventHeader.incomming_event_type == EVENT_PACKET_K)
	{
		KeyEventPacket keyEvent;

		SocketError error = _internalSocket->RecvFrom((char*)&keyEvent, sizeof(KeyEventPacket), &received);

		if (error != SocketError::SOCKET_E_SUCCESS || received != sizeof(KeyEventPacket))
		{
			std::cout << "Error Trying To Receive Mouse Wheel Event From Server: " << SOCK_ERR_STR(_internalSocket.get(), error) << std::endl;
			return false;
		}

		newEvent.eventType = OS_EVENT_KEY;
		newEvent.subEvent.keyEvent = (KeyEventType)keyEvent.keyEvent;
		newEvent.eventButton.scanCode = keyEvent.scancode;

		return false;
	}
	else
	{
		std::cout << "Received Invalid Event type from Header Packet" << std::endl;
		return false;
	}

	return false;
}

void CCClient::StopClientSocket()
{
	if (_internalSocket.get())
		_internalSocket->Close();
}
