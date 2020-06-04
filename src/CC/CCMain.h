#ifndef CC_MAIN_H
#define CC_MAIN_H

#include <vector>
#include <memory>

#include "INetworkEntityDiscovery.h"
#include "CCBroadcastManager.h"
#include "CCServer.h"
#include "CCClient.h"

class CCMain : public INetworkEntityDiscovery
{
private:
	std::vector<std::shared_ptr<CCNetworkEntity>> entites;

	std::unique_ptr<CCServer> server;
	std::unique_ptr<CCClient> client;
	std::unique_ptr<CCBroadcastManager> broadcaster;
	std::unique_ptr<CCBroadcastManager> broadcastReceiver;

public:
	CCMain();

	void StartServerMain();
	void StartClientMain();

	void StopRunning();
	void InstallService();

	// INetworkDiscoery Implementation

	virtual void NewEntityDiscovered(CCNetworkEntity* entity)override;
	virtual void EntityLost(CCNetworkEntity* entity, NELostReason lostReason)override;

	// End INetworkDiscoery
};

#endif

