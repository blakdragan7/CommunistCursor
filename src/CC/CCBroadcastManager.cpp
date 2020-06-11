#include "CCBroadcastManager.h"

#include <chrono>
#include <thread>
#include <iostream>

#include "../Socket/SocketError.h"
#include "../Socket/Socket.h"

#include "CCPacketTypes.h"


CCBroadcastManager::CCBroadcastManager() : shouldBroadcast(false)
{
}

CCBroadcastManager::CCBroadcastManager(std::string broadcastAddress, int broadcastPort) :
_internalSocket(new Socket(broadcastAddress, broadcastPort, false,  SocketProtocol::SOCKET_P_UDP)), shouldBroadcast(false)
{
}

void CCBroadcastManager::StartBraodcasting(std::string serverAddress, int serverPort)
{
	_internalSocket->SetIsBroadcastable(true);

	shouldBroadcast = true;

	AddressPacket addressPacket;

	memcpy(addressPacket.Address, serverAddress.c_str(), serverAddress.length());
	addressPacket.Address[serverAddress.length()] = 0;
	addressPacket.Port = serverPort;
	
	while (shouldBroadcast)
	{
		std::cout << "Broadcasting Address As {" << addressPacket.Address << "," << addressPacket.Port << "}" << std::endl;
		SocketError error = _internalSocket->SendTo(&addressPacket, sizeof(addressPacket));

		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			std::cout << "Error trying to send Broadcast " << SockErrorToString(error) << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

void CCBroadcastManager::StopBroadcasting()
{
	shouldBroadcast = false;
}

bool CCBroadcastManager::ListenForBroadcasts(BServerAddress* foundAddress)
{
	SocketError error = _internalSocket->Bind();

	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		std::cout << "Error Bind Client Broadcast Socket: " << SOCK_ERR_STR(_internalSocket.get(), error) << std::endl;
		return false;
	}

	size_t received = 0;
	AddressPacket addressPacket;
	SocketError err = _internalSocket->Recv((char*)&addressPacket, sizeof(AddressPacket), &received);

	if (err == SocketError::SOCKET_E_SUCCESS && received == sizeof(AddressPacket))
	{
		if (addressPacket.MagicNumber != P_MAGIC_NUMBER)
		{
			return false;
		}

		(*foundAddress).first = addressPacket.Address;
		(*foundAddress).second = addressPacket.Port;

		return true;
	}
	else
	{
		// there was some error and any reasonable person would do something about that
	}

	return false;
}
