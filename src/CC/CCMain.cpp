#include "CCMain.h"

#include "CCServer.h"
#include "CCClient.h"
#include "CCBroadcastManager.h"

#include "CCDisplay.h"

#include "CCNetworkEntity.h"

#include "../Socket/Socket.h"
#include "../Socket/SocketException.h"
#include "../OSInterface/OSInterface.h"

#include <iostream>

// Info uses tcp 6555
// Discovery uses udp 1046
// OSEvents use udo 1265

CCMain::CCMain() : server(new CCServer(6555, "0.0.0.0", this)), client(new CCClient(1047)), 
clientShouldRun(false), serverShouldRun(false)
{
}

CCMain::~CCMain()
{
	if(serverShouldRun)
		StopServer();
	if (clientShouldRun)
		StopClient();
}

void CCMain::StartServerMain()
{
	std::vector<IPAdressInfo> ipAddress;

	OSInterfaceError error = OSInterface::SharedInterface().GetIPAddressList(ipAddress);

	if (error != OSInterfaceError::OS_E_SUCCESS)
	{
		throw std::exception(OSInterfaceErrorToString(error).c_str());
	}

	if (ipAddress.size() < 1)
	{
		throw std::exception("Could not start server because IP address could not be found");
	}

	CCBroadcastManager broadcaster(ipAddress[0].address, 1046);

	serverShouldRun = true;
	server->StartServer();

	OSInterfaceError err = OSInterface::SharedInterface().GetMousePosition(currentMousePosition.x, currentMousePosition.y);
	if (err != OSInterfaceError::OS_E_SUCCESS)
	{
		std::string exceptionString = "Could not get mouse position from OS with error: ";

		exceptionString += OSInterfaceErrorToString(err);

		throw std::exception(exceptionString.c_str());
	}

	// hard coded for now but will be configurable / dynamic later
	std::string serverAddress = "192.168.0.3";
	// blocks but will not block later
	broadcaster.StartBraodcasting(serverAddress, 6555);

	OSInterface::SharedInterface().RegisterForOSEvents(this);
}

void CCMain::StartClientMain()
{
	CCBroadcastManager broadcastReceiver;

	clientShouldRun = true;
	BServerAddress address;
	// intentionally blocks because we need to know the server address before we can do anything else
	while (broadcastReceiver.ListenForBroadcasts(&address) == false);

	std::cout << "Address: " << address.first << " Port: " << address.second << std::endl;

	client->ConnectToServer(address.first, address.second);

	// receive events from server until told not to
	OSEvent newEvent;
	while (clientShouldRun)
	{
		if (client->ListenForOSEvent(newEvent))
		{
			if (newEvent.eventType == OS_EVENT_KEY)
				OSInterface::SharedInterface().SendKeyEvent(newEvent);
			else if (newEvent.eventType == OS_EVENT_MOUSE)
				OSInterface::SharedInterface().SendMouseEvent(newEvent);
			else
				std::cout << "Received Invalid Event From Server: " << newEvent << std::endl;
		}

		// loop until told not to
	}
}

void CCMain::StopServer()
{
	if (serverShouldRun)
		server->StopServer();

	serverShouldRun = false;

	OSInterface::SharedInterface().UnRegisterForOSEvents(this);
}

void CCMain::StopClient()
{
	if (clientShouldRun)
		client->StopClientSocket();

	clientShouldRun = false;
}

void CCMain::InstallService()
{
	// does nothing for now
	// eventually will install this as a platform dependent service
}

void CCMain::SetupGlobalPositions()
{
	//hardcoded for now

	auto displayList = entites[0]->GetAllDisplays();

	displayList[0]->SetBounds(Point(2160, 0));
}

void CCMain::NewEntityDiscovered(std::shared_ptr<CCNetworkEntity> entity)
{
	std::cout << "New Entity Discovered !\n";

	entites.push_back(entity);

	SetupGlobalPositions();
}

void CCMain::EntityLost(std::shared_ptr<CCNetworkEntity> entity, NELostReason lostReason)
{
	auto itr = std::find(entites.begin(), entites.end(), entity);
	if (itr != entites.end())
		entites.erase(itr);
}

bool CCMain::ReceivedNewInputEvent(const OSEvent event)
{
	if (event.eventType == OS_EVENT_MOUSE)
	{
		if (event.subEvent.mouseEvent == MOUSE_EVENT_MOVE)
		{
			currentMousePosition.x += event.deltaX;
			currentMousePosition.y += event.deltaY;

			for (auto entity : entites)
			{
				if (entity->PointIntersectsEntity(currentMousePosition))
				{
					entity->SendOSEvent(event);
					return true;
				}
			}
		}
	}

	return false;
}
