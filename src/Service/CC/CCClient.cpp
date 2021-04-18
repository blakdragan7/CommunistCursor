#include "CCClient.h"
#include "../Socket/Socket.h"
#include "../OSInterface/OSInterface.h"
#include "../OSInterface/PacketTypes.h"

#include "CCPacketTypes.h"
#include "CCLogger.h"
#include <exception>

CCClient::CCClient(int listenPort) : _serverAddress("0.0.0.0"), _listenPort(listenPort), _needsNewServer(true)
{
	auto error = OSInterface::SharedInterface().GetNativeDisplayList(_displayList);
	if (error != OSInterfaceError::OS_E_SUCCESS)
	{
		std::string errorMessage = "Error Getting Display List " + OSInterfaceErrorToString(error);
		throw std::runtime_error(errorMessage);
	}
}

void CCClient::ConnectToServer(std::string address, int port)
{
	Socket servSocket(address, port, false, SocketProtocol::SOCKET_P_TCP);

	SocketError error = servSocket.Connect();
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		LOG_ERROR << "Error Trying To Connect To Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
		return;
	}

	DisplayListHeaderPacket listHeader;
	listHeader.NumberOfDisplays = (int)_displayList.size();

	std::string hostName;
	OSInterfaceError osError = OSInterface::SharedInterface().GetLocalHostName(hostName);
	if (osError != OSInterfaceError::OS_E_SUCCESS)
	{
		LOG_ERROR << "Error Trying to get Local Host Name " << OSInterfaceErrorToString(osError) << std::endl;
		return;
	}

	std::string uniqueID;
	osError = OSInterface::SharedInterface().GetUUID(uniqueID, 32);
	if (osError != OSInterfaceError::OS_E_SUCCESS)
	{
		LOG_ERROR << "Error Trying to get UUID " << OSInterfaceErrorToString(osError) << std::endl;
		return;
	}

	EntityIDPacket idPacket(hostName, uniqueID);

	error = servSocket.Send((char*)&idPacket, sizeof(EntityIDPacket));
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		LOG_ERROR << "Error Trying To Send ID Packet To Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
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
		LOG_ERROR << "Error Trying To Send Address Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
		return;
	}

	error = servSocket.Send((char*)&listHeader, sizeof(DisplayListHeaderPacket));
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		LOG_ERROR << "Error Trying To Send List Header To Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
		return;
	}

	for (auto& display : _displayList)
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
			LOG_ERROR << "Error Trying To Send Display Info To Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
			return;
		}
	}

	// server will close socket on it's end when it receives everything
	// we wait for it here
	error = servSocket.WaitForServer();
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		LOG_ERROR << "Error Trying To Wait For Server: " << SOCK_ERR_STR(&servSocket, error) << std::endl;
		return;
	}

	_serverAddress = address;
	_needsNewServer = false;
}

void CCClient::StopClientSocket()
{
	if (_internalSocket.get())
	{
		_internalSocket->Close();
		_internalSocket.reset();
	}
}
