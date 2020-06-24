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

#include <algorithm>

std::string BroadcastAddressFromIPAndSubnetMask(std::string IPv4, std::string subnet);

// Info uses tcp 6555
// gui uses tcp 1049
// Discovery uses udp 1046
// OSEvents use udo 1265

CCMain::CCMain() : _server(new CCServer(6555, SOCKET_ANY_ADDRESS, this)), _client(new CCClient(1047)), 
_clientShouldRun(false), _serverShouldRun(false), _globalBounds({0,0,0,0}), _guiService(this)
{
	auto displayList = _client->GetDisplayList();

	std::string hostName;
	OSInterfaceError error = OSInterface::SharedInterface().GetLocalHostName(hostName);
	if (error != OSInterfaceError::OS_E_SUCCESS)
	{
		std::cout << "Error Getting Host Name. Using Default Instead\n";
		hostName = "Server";
	}

	// setup this computers entity

	_localEntity = std::make_shared<CCNetworkEntity>(hostName);

	for (auto display : displayList)
	{
		_localEntity->AddDisplay(std::make_shared<CCDisplay>(display));
	}

	this->NewEntityDiscovered(_localEntity);
}

CCMain::~CCMain()
{
	if(_serverShouldRun)
		StopServer();
	if (_clientShouldRun)
		StopClient();
}

void CCMain::StartServerMain()
{
	_serverShouldRun = true;
	_server->StartServer();

	std::cout << "Starting Gui Server\n";

	if (_guiService.StartGUIServer() == false)
	{
		throw std::exception("Could Not Start Gui Socket Server !");
	}

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

		std::cout << "New Broadcase Address: " << broadcastAddress << " on port: " << 1046 << std::endl;
		broadcasters.push_back(CCBroadcastManager(broadcastAddress, 1046));
	}

	OSInterfaceError err = OSInterface::SharedInterface().GetMousePosition(_currentMousePosition.x, _currentMousePosition.y);
	if (err != OSInterfaceError::OS_E_SUCCESS)
	{
		std::string exceptionString = "Could not get mouse position from OS with error: ";

		exceptionString += OSInterfaceErrorToString(err);

		throw std::exception(exceptionString.c_str());
	}

	OSInterface::SharedInterface().RegisterForOSEvents(this);
	_serverBroadcastThread = std::thread([&]() {
		while (_serverShouldRun)
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
	});

	OSInterface::SharedInterface().OSMainLoop();
}

void CCMain::StartClientMain()
{
	CCBroadcastManager broadcastReceiver(SOCKET_ANY_ADDRESS, 1046);

	_clientShouldRun = true;
	BServerAddress address;
	// intentionally blocks because we need to know the server address before we can do anything else
	while (broadcastReceiver.ListenForBroadcasts(&address) == false);

	std::cout << "Address: " << address.first << " Port: " << address.second << std::endl;

	_client->ConnectToServer(address.first, address.second);

	// receive events from server until told not to
	OSEvent newEvent;
	while (_clientShouldRun)
	{
		if (_client->ListenForOSEvent(newEvent))
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
	if (_serverShouldRun)
		_server->StopServer();

	_serverShouldRun = false;

	OSInterface::SharedInterface().UnRegisterForOSEvents(this);

	_serverBroadcastThread.join();
}

void CCMain::StopClient()
{
	if (_clientShouldRun)
		_client->StopClientSocket();

	_clientShouldRun = false;
}

void CCMain::InstallService()
{
	// does nothing for now
	// eventually will install this as a platform dependent service
}

void CCMain::SetupGlobalPositions()
{
	//hardcoded for now

	auto displayList = _entites[0]->GetAllDisplays();

	int minX = 4000;
	int minY = 4000;
	int maxX = 0;
	int maxY = 0;

	int Buffer = (int)displayList.size() * 1500;

	for (auto display : displayList)
	{
		Rect collision = display->GetCollision();
		minX = std::min(collision.topLeft.x, minX);
		minY = std::min(collision.topLeft.y, minY);
		maxX = std::max(collision.bottomRight.x, maxX);
		maxY = std::max(collision.bottomRight.y, maxY);
	}

	_globalBounds[0] = minX - Buffer;
	_globalBounds[1] = minY - Buffer;
	_globalBounds[2] = maxX + Buffer;
	_globalBounds[3] = maxY + Buffer;
}

void CCMain::NewEntityDiscovered(std::shared_ptr<CCNetworkEntity> entity)
{
	std::cout << "New Entity Discovered {" << entity->GetID() << "}" << std::endl;

	_entites.push_back(entity);

	SetupGlobalPositions();
}

void CCMain::EntityLost(std::shared_ptr<CCNetworkEntity> entity, NELostReason lostReason)
{
	auto itr = std::find(_entites.begin(), _entites.end(), entity);
	if (itr != _entites.end())
		_entites.erase(itr);
}

const std::vector<std::shared_ptr<CCNetworkEntity>>& CCMain::GetEntitiesToConfigure() const
{
	return _entites;
}

const std::vector<int>& CCMain::GetGlobalBounds() const
{
	return _globalBounds;
}

void CCMain::EntitiesFinishedConfiguration()
{
	_currentMouseOffsets = _localEntity->GetOffsets();

	for (int i = 0; i < _entites.size(); i++)
	{
		auto entity = _entites[i];

		for (int l = i + 1; l < _entites.size(); l++)
		{
			entity->AddEntityIfInProximity(_entites[l].get());
		}
	}
}

bool CCMain::ReceivedNewInputEvent(const OSEvent event)
{
	if (event.eventType == OS_EVENT_MOUSE)
	{
		if (event.subEvent.mouseEvent == MOUSE_EVENT_MOVE)
		{
			_currentMousePosition.x += event.deltaX;
			_currentMousePosition.y += event.deltaY;

			//std::cout << "New Mouse Pos {" << _currentMousePosition.x << "," << _currentMousePosition.y << "}\n";
		}
	}

	for (auto entity : _entites)
	{
		Point OffsetPos;
		
		OffsetPos.x = _currentMousePosition.x + _currentMouseOffsets.x;
		OffsetPos.y = _currentMousePosition.y + _currentMouseOffsets.y;

		if (entity->PointIntersectsEntity(OffsetPos))
		{
			auto nextEntity = entity->GetEntityForPointInJumpZone(OffsetPos);
			if (nextEntity)
			{
				// we have a jump zone
				std::cout << "Jump !!\n";
			}

			if (entity->GetIsLocal()) return false;

			entity->SendOSEvent(event);
			return true;
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