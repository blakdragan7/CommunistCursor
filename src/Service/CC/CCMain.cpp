#include "CCMain.h"

#include "CCLogger.h"
#include "CCServer.h"
#include "CCClient.h"
#include "CCDisplay.h"
#include "CCNetworkEntity.h"
#include "CCBroadcastManager.h"

#include "../Socket/Socket.h"
#include "../Socket/SocketException.h"
#include "../OSInterface/OSInterface.h"

#include <algorithm>
#include <sstream>
#include <chrono>
#include <thread>

#define REGISTER_OS_EVENTS 0

#define DELTA_X_MAX 200
#define DELTA_Y_MAX 200

template<typename t>
bool operator==(std::shared_ptr<t> lh, t* rh)
{
	return lh.get() == rh;
}

template<typename t>
bool operator!=(std::shared_ptr<t> lh, t* rh)
{
	return lh.get() != rh;
}

std::ostream& operator <<(std::ostream& os, const Point& p)
{
	return os << "{" << p.x << "," << p.y << "}";
}

std::string BroadcastAddressFromIPAndSubnetMask(std::string IPv4, std::string subnet);

// Info uses tcp 6555
// gui uses tcp 1049
// Discovery uses udp 1046
// OSEvents use udo 1265

void CCMain::SetupEntityConnections()
{
	// clear everybody before reassigning

	for (auto entity : _entites)
	{
		entity->ClearAllEntities();
	}

	for (int i = 0; i < _entites.size(); i++)
	{
		auto entity = _entites[i];

		for (int l = i + 1; l < _entites.size(); l++)
		{
			entity->AddEntityIfInProximity(_entites[l].get());
		}
	}
}

void CCMain::RemoveLostEntites()
{
	// only lock out things that are cirtical
	{
		std::lock_guard<std::mutex> lock(_entitesAccessMutex);
		for (auto entity : _lostEntites)
		{
			auto itr = std::find(_entites.begin(), _entites.end(), entity);
			if (itr != _entites.end())
				_entites.erase(itr);
		}
		_lostEntites.clear();
	}

	SetupEntityConnections();
}

CCMain::CCMain() : _server(new CCServer(6555, SOCKET_ANY_ADDRESS, this)), _client(new CCClient(1047)),
_clientShouldRun(false), _serverShouldRun(false), _globalBounds({0,0,0,0}), _guiService(this), \
_configFile("cc.json"), _ignoreInputEvent(false)
{
	auto displayList = _client->GetDisplayList();

	std::string hostName;
	OSInterfaceError error = OSInterface::SharedInterface().GetLocalHostName(hostName);
	if (error != OSInterfaceError::OS_E_SUCCESS)
	{
		LOG_ERROR << "Error Getting Host Name. Using Default Instead\n";
		hostName = "Server";
	}

	// setup this computers entity

	_localEntity = std::make_shared<CCNetworkEntity>(hostName);
	_currentEntity = _localEntity.get();

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

	LOG_INFO << "Starting Gui Server\n";

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

		LOG_INFO << "New Broadcase Address: " << broadcastAddress << " on port: " << 1046 << std::endl;
		broadcasters.push_back(CCBroadcastManager(broadcastAddress, 1046));
	}

	OSInterfaceError err = OSInterface::SharedInterface().GetMousePosition(_currentMousePosition.x, _currentMousePosition.y);
	if (err != OSInterfaceError::OS_E_SUCCESS)
	{
		std::string exceptionString = "Could not get mouse position from OS with error: ";

		exceptionString += OSInterfaceErrorToString(err);

		throw std::exception(exceptionString.c_str());
	}
#if REGISTER_OS_EVENTS
	OSInterface::SharedInterface().RegisterForOSEvents(this);
#endif
	_serverBroadcastThread = std::thread([&broadcasters, &ipAddress, this]() {
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
					break;
				}
			}

			// remove all queued lost entites
			RemoveLostEntites();

			std::this_thread::sleep_for(std::chrono::seconds(5));
		}
	});

	OSInterface::SharedInterface().OSMainLoop();

#if REGISTER_OS_EVENTS
	OSInterface::SharedInterface().UnRegisterForOSEvents(this);
#endif
}

void CCMain::StartClientMain()
{
	CCBroadcastManager broadcastReceiver(SOCKET_ANY_ADDRESS, 1046);

	_clientShouldRun = true;
	BServerAddress address;
	// intentionally blocks because we need to know the server address before we can do anything else
	while (broadcastReceiver.ListenForBroadcasts(&address) == false);

	LOG_INFO << "Address: " << address.first << " Port: " << address.second << std::endl;

	// This connects to the server and then tells the server everything it needs to know about us
	// this connection will be disconnected and the server will re-connect via the remote CCNetworkEntity
	_client->ConnectToServer(address.first, address.second);

	OSInterface::SharedInterface().OSMainLoop();
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

void CCMain::LoadAll(std::string path)
{
	if (path == "")
	{
		path = _configFile;
	}
	else
	{
		_configFile = path;
	}

	if (_configManager.LoadFromFile(path))
	{
		for (auto entity : _entites)
		{
			entity->LoadFrom(_configManager);
		}

		_currentMouseOffsets = _localEntity->GetBounds().topLeft;
	}
	else
	{
		LOG_ERROR << "Error loading from file " << path << std::endl;
	}
}

void CCMain::SaveAll(std::string path)
{
	if (path == "")
	{
		path = _configFile;
	}

	for (auto entity : _entites)
	{
		entity->SaveTo(_configManager);
	}

	if (!_configManager.SaveToFile(path))
	{
		LOG_ERROR << "Error Saving to file " << path << std::endl;
	}
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
	LOG_INFO << "New Entity Discovered {" << entity->GetID() << "}" << std::endl;

	entity->LoadFrom(_configManager);
	entity->SetDelegate(this);
	{
		std::lock_guard<std::mutex> lock(_entitesAccessMutex);
		_entites.push_back(entity);
	}
	
	SetupGlobalPositions();
	SetupEntityConnections();
}

void CCMain::EntityLost(CCNetworkEntity* entity)
{
	entity->ClearAllEntities();

	std::lock_guard<std::mutex> lock(_entitesAccessMutex);

	LOG_INFO << "Lost Entity " << entity->GetID() << std::endl;
	_lostEntites.push_back(entity);

	if (entity == _currentEntity)
	{
		_currentEntity = _localEntity.get();
		_currentEntity->RPC_UnhideMouse();
	}
}

void CCMain::LostServer()
{
	// force client to re-listen for server
	_client->StopClientSocket();
	_client->SetNeedsNewServer();
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

	SetupEntityConnections();

	SaveAll();
}

bool CCMain::ReceivedNewInputEvent(OSEvent event)
{
	bool isMove = false;
	Point OffsetPos = _currentMousePosition + _currentMouseOffsets;

	// check if we should skep or if the mouse moved more then we think it should
	if (_ignoreInputEvent || abs(event.deltaX) > DELTA_X_MAX || abs(event.deltaY) > DELTA_Y_MAX)
	{
		LOG_INFO << "Skipping event " << event << std::endl;
		_ignoreInputEvent = false;
		return false;
	}

	if (event.eventType == OS_EVENT_MOUSE)
	{
		if (event.mouseEvent == MOUSE_EVENT_MOVE)
		{
			_currentMousePosition.x += event.deltaX;
			_currentMousePosition.y += event.deltaY;

			OffsetPos = _currentMousePosition + _currentMouseOffsets;
			if (_localEntity != _currentEntity)
			{
				Rect bounds = _currentEntity->GetBounds();
				Point offsets = _currentEntity->GetOffsets();

				int x = event.x - _currentMouseOffsets.x;
				int y = event.y - _currentMouseOffsets.y;

				if (abs(x - bounds.topLeft.x) < 20 || abs(x - bounds.bottomRight.x) < 20 || \
					abs(y - bounds.topLeft.y) < 20 || abs(y - bounds.bottomRight.y) < 20)
				{
					_localEntity->RPC_SetMousePosition(0.5f, 0.5f);
					_ignoreInputEvent = true;
				}

				event.x = ((OffsetPos.x - bounds.topLeft.x) + offsets.x) - ;
				event.y = ((OffsetPos.y - bounds.topLeft.y) + offsets.y);
			}

			isMove = true;
		}
	}
	else // always set offset pos
		OffsetPos = _currentMousePosition + _currentMouseOffsets;
	

	if (event.eventType == OS_EVENT_KEY && event.scanCode == 69 /* PAUSE/BREAK button */)
	{
		_currentEntity = _localEntity.get();
		_currentEntity->RPC_UnhideMouse();
		_currentEntity->RPC_SetMousePosition(0.5, 0.5);
		Rect bounds = _currentEntity->GetBounds();
		_currentMousePosition = bounds.topLeft + ((bounds.bottomRight - bounds.topLeft) / 2);
		return false;
	}
	

	JumpDirection direction;
	CCNetworkEntity* nextEntity = 0;
	if (_currentEntity->GetEntityForPointInJumpZone(OffsetPos, &nextEntity, direction))
	{
		_currentMousePosition = OffsetPos - _currentMouseOffsets;

		// we have a jump zone
		LOG_INFO << "Jump To " << nextEntity->GetID() << std::endl;
				
		// hide mouse
		LOG_INFO << "Hide Mouse Current" << std::endl;
		_currentEntity->RPC_HideMouse();
					
		// Force mouse to be in center of screen
		LOG_INFO << "Warp Current Mouse To Center" << std::endl;
		_currentEntity->RPC_SetMousePosition(0.5f,0.5f);
		if (_currentEntity->GetIsLocal())
			_ignoreInputEvent = true;

		// unhide mouse of last entity
		LOG_INFO << "Unhide Mouse Next" << std::endl;
		nextEntity->RPC_UnhideMouse();

		_currentEntity = nextEntity;
	}

	if (_currentEntity->GetIsLocal()) return false;

	LOG_INFO << "Sending Event " << event << std::endl;

	SocketError error = _currentEntity->SendOSEvent(event);
	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		LOG_ERROR << "Error Sending Event To " << _currentEntity->GetID() << " Error: " << SOCK_ERR_STR(_currentEntity->GetUDPSocket(), error) << std::endl;
	}

	return true && !isMove;
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