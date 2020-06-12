#ifndef INETWORK_DISCOVERY_H
#define INETWORK_DISCOVERY_H

#include <memory>

enum class NELostReason
{
    NEGracefuleDisconnect,
    NETimeout,
    NEError
};

/*
*   Interface for receiving information about discovering network entites
*/

class CCNetworkEntity;
class INetworkEntityDiscovery
{
public:
    virtual void NewEntityDiscovered(std::shared_ptr<CCNetworkEntity> entity) = 0;
    virtual void EntityLost(std::shared_ptr<CCNetworkEntity> entity, NELostReason lostReason) = 0;
};

#endif