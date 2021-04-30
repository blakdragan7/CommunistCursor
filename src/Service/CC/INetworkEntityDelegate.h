#ifndef INETWORK_DELEGATE_H
#define INETWORK_DELEGATE_H

class CCNetworkEntity;
class INetworkEntityDelegate
{
public:
	virtual void EntityCursorPositionUpdate(CCNetworkEntity* entity, int x, int y) = 0;
	virtual void EntityLost(CCNetworkEntity* entity) = 0;
	virtual void LostServer() = 0;
};

#endif