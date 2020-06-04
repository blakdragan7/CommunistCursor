#include "CCServer.h"

#include "../Socket/Socket.h"
#include "INetworkEntityDiscovery.h"
#include "CCNetworkEntity.h"
#include "CCDisplay.h"

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
	_internalSocket->Bind();
	_internalSocket->Listen();

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
		Socket* acceptedSocket = 0;
		server->_internalSocket->Accept(&acceptedSocket);

		CCNetworkEntity* entity = new CCNetworkEntity(acceptedSocket);
		server->discoverer->NewEntityDiscovered(entity);
	}
}