#ifndef CC_MAIN_H
#define CC_MAIN_H

#include <vector>
#include <memory>
#include <mutex>

#include "../OSInterface/IOSEventReceiver.h"
#include "../OSInterface/OSTypes.h"

#include "INetworkEntityDiscovery.h"
#include "INetworkEntityDelegate.h"
#include "CCConfigurationManager.h"
#include "IGuiServiceInterface.h"
#include "CCBroadcastManager.h"
#include "CCGUIService.h"

#include "BasicTypes.h"

class CCServer;
class CCClient;
class CCNetworkEntity;
class CCMain : public INetworkEntityDiscovery, public IOSEventReceiver, public IGuiServiceInterface, public INetworkEntityDelegate
{
private:
	std::vector<CCBroadcastManager>					_broadcasters;
	std::vector<std::string>						_ipAddress;

	std::vector<std::shared_ptr<CCNetworkEntity>>	_entites;
	std::vector<CCNetworkEntity*>					_lostEntites;
	std::mutex										_entitesAccessMutex;


	std::shared_ptr<CCNetworkEntity> _localEntity;
	CCNetworkEntity*				 _currentEntity;

	unsigned						_serverBroadcastQueue;
	unsigned						_inputQueue;

	std::unique_ptr<CCServer>	_server;
	std::unique_ptr<CCClient>	_client;

	CCGuiService				_guiService;

	std::vector<int>			_globalBounds;
	
	Point						_currentMousePosition;

	bool						_serverShouldRun;
	bool						_clientShouldRun;
	bool						_ignoreInputEvent;

	std::string					_configFile;
	std::string					_localID;
	CCConfigurationManager		_configManager;

private:
	void CheckJumpZones();
	void SetupEntityConnections();
	void RemoveLostEntites();

	void BroadcastAll();

	bool ProcessInputEvent(OSEvent osEvent);
	void SetupLocalEntity(bool isServer);

public:
	CCMain();
	~CCMain();

	void StartServerMain();
	void StartClientMain();

	void StopServer();
	void StopClient();
	void InstallService();

	void LoadAll(std::string path = "");
	void SaveAll(std::string path = "");

	// Sets up Global Position for network entites
	// Global positions refer to the global mouse space
	// so Event can be dispatched to the correct location
	// This will eventually be a GUI interface somehow
	void SetupGlobalPositions();

	// INetworkDiscoery Implementation

	virtual void NewEntityDiscovered(std::shared_ptr<CCNetworkEntity> entity)override;
	// End INetworkDiscoery


	// INetworkEntityDelegate Implmentation

	virtual void EntityCursorPositionUpdate(CCNetworkEntity* entity, int x, int y);
	virtual void EntityLost(CCNetworkEntity* entity)override;
	virtual void LostServer()override;

	// End INetworkEntityDelegate

	// IGuiServiceInterface Implementation

	virtual const std::vector<std::shared_ptr<CCNetworkEntity>>& GetEntitiesToConfigure()const override;
	virtual const std::vector<int>& GetGlobalBounds()const override;
	virtual void EntitiesFinishedConfiguration()override;

	// End IGuiServiceInterface

	// IOSEventReceiver Implementation

	virtual bool ReceivedNewInputEvent(OSEvent event)override;

	// END IOSEventReceiver 
};

#endif

