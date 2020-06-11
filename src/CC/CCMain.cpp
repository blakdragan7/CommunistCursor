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
#include <sstream>

#include <chrono>
#include <thread>

std::string BroadcastAddressFromIPAndSubnetMask(std::string IPv4, std::string subnet);

// Info uses tcp 6555
// Discovery uses udp 1046
// OSEvents use udo 1265

CCMain::CCMain() : server(new CCServer(6555, SOCKET_ANY_ADDRESS, this)), client(new CCClient(1047)), 
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
	serverShouldRun = true;
	server->StartServer();

	std::vector<IPAdressInfo> ipAddress;

	OSInterfaceError error = OSInterface::SharedInterface().GetIPAddressList(ipAddress, {IPAddressType::UNICAST, IPAddressFamilly::IPv4});

	if (error != OSInterfaceError::OS_E_SUCCESS)
	{
		std::string exceptionString = "Error Getting IP Address " + OSInterfaceErrorToString(error);
		// there is nothing we can do here if we can't even get our address so exception time
		throw std::exception(exceptionString.c_str());
	}

	if (ipAddress.size() < 1)
	{
		throw std::exception("Could not start server because IP address could not be found");
	}

	// Remove all Addresses that don't have a broadcastable address
	// this can probably be optimized later but not high priority
	for (int i = 0; i < ipAddress.size(); i++)
	{
		if (ipAddress[i].subnetMask.find(".0") == std::string::npos)
		{
			ipAddress.erase(ipAddress.begin() + i);
		}
	}

	std::vector<CCBroadcastManager> broadcasters;
	broadcasters.reserve(ipAddress.size());

	for (auto address : ipAddress)
	{
		std::string broadcastAddress = BroadcastAddressFromIPAndSubnetMask(address.address, address.subnetMask);

		broadcasters.push_back(CCBroadcastManager(broadcastAddress, 1046));
	}

	OSInterfaceError err = OSInterface::SharedInterface().GetMousePosition(currentMousePosition.x, currentMousePosition.y);
	if (err != OSInterfaceError::OS_E_SUCCESS)
	{
		std::string exceptionString = "Could not get mouse position from OS with error: ";

		exceptionString += OSInterfaceErrorToString(err);

		throw std::exception(exceptionString.c_str());
	}

	while (serverShouldRun)
	{
		// broadcase to every possible network we can.
		// this will be configurable later
		for (int i = 0; i < broadcasters.size(); i++)
		{
			// If we fail to broadcase we remove it from the list of broadcasters and
			// break here to wait to start the loop again so we don't cause any
			// weird issues with looping through a vector while modifying it
			if (broadcasters[i].BroadcastNow(ipAddress[i].address, 6555) == false)
			{
				broadcasters.erase(broadcasters.begin() + i);
				ipAddress.erase(ipAddress.begin() + i);
				break;
			}
		}
	
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
	OSInterface::SharedInterface().RegisterForOSEvents(this);
}

void CCMain::StartClientMain()
{
	CCBroadcastManager broadcastReceiver(SOCKET_ANY_ADDRESS, 1046);

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

// move these somewhere else later

std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const std::string& s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

std::string BroadcastAddressFromIPAndSubnetMask(std::string IPv4, std::string subnet)
{
	auto parts = split(subnet, '.');
	size_t numZs = std::count(parts.begin(), parts.end(), "0");

	auto ipParts = split(IPv4, '.');
	size_t ipPartsSize = ipParts.size();
	for (int i = 0; i < numZs; i++)
	{
		ipParts[ipPartsSize - 1 - i] = "255";
	}

	std::string broadcast;

	for (auto part : ipParts)
	{
		broadcast += part;
		broadcast += ".";
	}

	broadcast.pop_back();

	return broadcast;
}