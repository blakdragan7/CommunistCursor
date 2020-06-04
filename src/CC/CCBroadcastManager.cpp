#include "CCBroadcastManager.h"

#include <chrono>
#include <thread>
#include <iostream>

#include "../Socket/SocketError.h"
#include "../Socket/Socket.h"

#define B_MAGIC_NUMBER 123456789

struct AddressBroadcastStruct
{
	int MagicNumber;
	char servAddress[16];
	int servPort;

	AddressBroadcastStruct() : MagicNumber(B_MAGIC_NUMBER), servAddress("invalid"), servPort(0)
	{}
};

CCBroadcastManager::CCBroadcastManager(std::string broadcastAddress, int broadcastPort) : 
_internalSocket(new Socket(broadcastAddress, broadcastPort, false,  SocketProtocol::SOCKET_P_UDP)), shouldBroadcast(false)
{
}

void CCBroadcastManager::StartBraodcasting(std::string serverAddress, int serverPort)
{
	_internalSocket->SetIsBroadcastable(true);

	shouldBroadcast = true;

	AddressBroadcastStruct bStruct;

	memcpy(bStruct.servAddress, serverAddress.c_str(), serverAddress.length());
	bStruct.servAddress[serverAddress.length()] = 0;
	bStruct.servPort = serverPort;
	
	while (shouldBroadcast)
	{
		SocketError error = _internalSocket->SendTo(&bStruct, sizeof(AddressBroadcastStruct));

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
		std::cout << "Error Bind Client Broadcast Socket: " << SockErrorToString(error) << std::endl;
		return false;
	}

	size_t received = 0;
	AddressBroadcastStruct bStruct;
	SocketError err = _internalSocket->Recv((char*)&bStruct, sizeof(AddressBroadcastStruct), &received);

	if (err == SocketError::SOCKET_E_SUCCESS && received == sizeof(AddressBroadcastStruct))
	{
		if (bStruct.MagicNumber != B_MAGIC_NUMBER)
		{
			return false;
		}

		(*foundAddress).first = bStruct.servAddress;
		(*foundAddress).second = bStruct.servPort;

		return true;
	}
	else
	{
		// there was some error and any reasonable person would do something about that
	}

	return false;
}
