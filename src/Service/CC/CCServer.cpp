#include "CCServer.h"

#include "../Socket/Socket.h"
#include "../Socket/SocketException.h"

#include "../OSInterface/OSTypes.h"
#include "../OSInterface/OSInterface.h"

#include "INetworkEntityDiscovery.h"
#include "CCNetworkEntity.h"
#include "CCPacketTypes.h"
#include "CCDisplay.h"

#include <iostream>

// simply used to accept more sockets
void ServerAcceptThread(CCServer* server);

CCServer::CCServer(int port, std::string listenAddress, INetworkEntityDiscovery* discoverer) : discoverer(discoverer), isRunning(false)
{
	_internalSocket = std::make_unique<Socket>(listenAddress, port, false, SocketProtocol::SOCKET_P_TCP);
}

void CCServer::SetDiscoverer(INetworkEntityDiscovery* discoverer)
{
	this->discoverer = discoverer;
}

void CCServer::StartServer()
{
	isRunning = true;

	SocketError error = _internalSocket->Bind();
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		throw SocketException(error, _internalSocket->lastOSErr);
	}

	error = _internalSocket->Listen();
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		throw SocketException(error, _internalSocket->lastOSErr);
	}

	accpetThread = std::thread(ServerAcceptThread, this);
}

void CCServer::StopServer()
{
	isRunning = false;
	_internalSocket->Disconnect();

	accpetThread.join();

	
}

bool CCServer::GetServerIsRunning()
{
	return isRunning && _internalSocket->GetIsListening();
}

void ServerAcceptThread(CCServer* server)
{
	while (server->isRunning)
	{
		Socket* newSocket = 0;
		SocketError error = server->_internalSocket->Accept(&newSocket);

		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			std::cout << "Error accepting new client Socket " << SOCK_ERR_STR(server->_internalSocket.get(), error) << std::endl;
			continue;
		}

		// me being lazy about memory management
		std::unique_ptr<Socket> acceptedSocket(newSocket);

		// AddressPacket is currently just used here to get the desired port
		// but we may use it for the actuall conection address later

		AddressPacket addPacket;
		EntityIDPacket idPacket;
		DisplayListHeaderPacket listHeaderPacket;

		size_t received = 0;
		error = acceptedSocket->Recv((char*)&idPacket, sizeof(EntityIDPacket), &received);
		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			std::cout << "Error Receiving EntityIDPacket " << SOCK_ERR_STR(acceptedSocket.get(), error) << std::endl;
			continue;
		}

		if (received != sizeof(EntityIDPacket) || idPacket.MagicNumber != P_MAGIC_NUMBER)
		{
			std::cout << "Invalid EntityIDPacket Received " << SOCK_ERR_STR(acceptedSocket.get(), error) << std::endl;
			continue;
		}
		
		error = acceptedSocket->Recv((char*)&addPacket, sizeof(AddressPacket), &received);

		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			std::cout << "Error Receiving AddressPacket " << SOCK_ERR_STR(acceptedSocket.get(), error) << std::endl;
			continue;
		}

		if (received != sizeof(AddressPacket) || addPacket.MagicNumber != P_MAGIC_NUMBER)
		{
			std::cout << "Invalid AddressPacket Received " << SOCK_ERR_STR(acceptedSocket.get(), error) << std::endl;
			continue;
		}

		// socket is owned by entity as a uniqe_ptr so no delete needed
		Socket* udpRemoteClientSocket = new Socket(acceptedSocket->GetAddress(), addPacket.Port, acceptedSocket->GetCanUseIPV6(), SocketProtocol::SOCKET_P_UDP);
		std::shared_ptr<CCNetworkEntity> entity(new CCNetworkEntity(idPacket.EntityID, udpRemoteClientSocket));

		received = 0;
		error = acceptedSocket->Recv((char*)&listHeaderPacket, sizeof(DisplayListHeaderPacket), &received);

		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			std::cout << "Error Receiving DisplayListHeaderPacket " << SOCK_ERR_STR(acceptedSocket.get(), error) << std::endl;
			continue;
		}

		if (received != sizeof(DisplayListHeaderPacket) || listHeaderPacket.MagicNumber != P_MAGIC_NUMBER)
		{
			std::cout << "Invalid DisplayListHeaderPacket Received " << SOCK_ERR_STR(acceptedSocket.get(), error) << std::endl;
			continue;
		}

		// we now know that there will be {listHeaderPacket.NumberOfDisplays} number of displays about to be sent over
		// tcp is garunteed delivery so we don't have to be so picky about handshakes / acks ourselves

		bool failed = false;

		for (int i = 0; i < listHeaderPacket.NumberOfDisplays; i++)
		{
			DisplayListDisplayPacket displayPacket;
			NativeDisplay nativeDisplay;

			received = 0;
			error = acceptedSocket->Recv((char*)&displayPacket, sizeof(DisplayListDisplayPacket), &received);

			if (error != SocketError::SOCKET_E_SUCCESS)
			{
				std::cout << "Error Receiving DisplayListPacket " << SOCK_ERR_STR(acceptedSocket.get(), error) << std::endl;
				failed = true;
				break;
			}

			if (received != sizeof(DisplayListDisplayPacket) || displayPacket.MagicNumber != P_MAGIC_NUMBER)
			{
				std::cout << "Invalid DisplayListDisplayPacket Received " << SOCK_ERR_STR(acceptedSocket.get(), error) << std::endl;
				failed = true;
				break;
			}

			nativeDisplay.height = displayPacket.Height;
			nativeDisplay.width = displayPacket.Width;
			nativeDisplay.nativeScreenID = displayPacket.NativeDisplayID;
			nativeDisplay.posX = displayPacket.Left;
			nativeDisplay.posY = displayPacket.Top;

			std::shared_ptr<CCDisplay> newDisplay(new CCDisplay(nativeDisplay));

			entity->AddDisplay(newDisplay);
		}

		// if we failed getting the display list, give up on this client / let them retry with a new connection

		if (failed)
			continue;

		server->discoverer->NewEntityDiscovered(entity);

		error = acceptedSocket->Close();
		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			// very strange if we hit here.
			std::cout << "Error Closing Accepted Socket: " << SOCK_ERR_STR(acceptedSocket.get(), error) << std::endl;
		}
	}
}