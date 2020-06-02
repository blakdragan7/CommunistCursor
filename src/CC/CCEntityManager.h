#ifndef CC_DISPLAY_MANAGER_H
#define CC_DISPLAY_MANAGER_H
#include "BasicTypes.h"

#include <vector>

class CCDisplay;
class CCNetworkEntity;
class CCEntityManager
{
private:
    std::vector<CCNetworkEntity*> entities;
    
public:
    CCEntityManager();

    void DicoveredNewEntity(CCNetworkEntity* entity);
    void LostEntity(CCNetworkEntity* entity);

    CCNetworkEntity* EntityForGlobalPosition(const Point& p);
};

#endif