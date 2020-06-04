#include "CCMain.h"

#include "CCServer.h"
#include "CCClient.h"
#include "CCBroadcastManager.h"

#include "CCNetworkEntity.h"

#include "../Socket/Socket.h"

#include <iostream>

// Use tcp 6555
// Use udp 1046

CCMain::CCMain() : server(new CCServer(6555, "0.0.0.0", this)), client(new CCClient()),
broadcaster(new CCBroadcastManager("192.168.0.255", 1046)), broadcastReceiver(new CCBroadcastManager(SOCKET_ANY_ADDRESS, 1046))
{
}

void CCMain::StartServerMain()
{
	server->StartServer();

	// hard coded for now but will be configurable / dynamic later
	std::string serverAddress = "192.168.0.3";
	// blocks but will not block later
	broadcaster->StartBraodcasting(serverAddress, 6555);
}

void CCMain::StartClientMain()
{
	BServerAddress address;
	// intentionally blocks because we need to know the server address before we can do anything else
	while (broadcastReceiver->ListenForBroadcasts(&address) == false);

	std::cout << "Address: " << address.first << " Port: " << address.second << std::endl;
}

void CCMain::StopRunning()
{
	server->StopServer();
}

void CCMain::InstallService()
{
	// does nothing for now
}

void CCMain::NewEntityDiscovered(CCNetworkEntity* entity)
{
	entites.push_back(std::shared_ptr<CCNetworkEntity>(entity));
}

void CCMain::EntityLost(CCNetworkEntity* entity, NELostReason lostReason)
{
	auto itr = std::find_if(entites.begin(), entites.end(), 
		[&](std::shared_ptr<CCNetworkEntity> const& p) {return p.get() == entity; });

	if (itr != entites.end())
		entites.erase(itr);
}
