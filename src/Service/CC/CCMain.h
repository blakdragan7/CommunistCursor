#ifndef CC_MAIN_H
#define CC_MAIN_H

#include <vector>
#include <memory>

#include "../OSInterface/IOSEventReceiver.h"
#include "../OSInterface/OSTypes.h"

#include "INetworkEntityDiscovery.h"
#include "CCBroadcastManager.h"
#include "CCServer.h"
#include "CCClient.h"

#include "BasicTypes.h"

class CCMain : public INetworkEntityDiscovery, public IOSEventReceiver
{
private:
	std::vector<std::shared_ptr<CCNetworkEntity>> entites;

	std::unique_ptr<CCServer> server;
	std::unique_ptr<CCClient> client;

	bool serverShouldRun;
	bool clientShouldRun;

	Point currentMousePosition;

public:
	CCMain();
	~CCMain();

	void StartServerMain();
	void StartClientMain();

	void StopServer();
	void StopClient();
	void InstallService();

	// Sets up Global Position for network entites
	// Global positions refer to the global mouse space
	// so Event can be dispatched to the correct location
	// This will eventually be a GUI interface somehow
	void SetupGlobalPositions();

	// INetworkDiscoery Implementation

	virtual void NewEntityDiscovered(std::shared_ptr<CCNetworkEntity> entity)override;
	virtual void EntityLost(std::shared_ptr<CCNetworkEntity> entity, NELostReason lostReason)override;

	// End INetworkDiscoery

	// IOSEventReceiver Implementation

	virtual bool ReceivedNewInputEvent(const OSEvent event)override;

	// END IOSEventReceiver 
};

#endif

