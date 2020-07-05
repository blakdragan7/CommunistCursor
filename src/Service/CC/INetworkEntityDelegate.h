#ifndef INETWORK_DELEGATE_H
#define INETWORK_DELEGATE_H

class CCNetworkEntity;
class INetworkEntityDelegate
{
public:
	virtual void EntityLost(CCNetworkEntity* entity) = 0;
	virtual void LostServer() = 0;
};

#endif