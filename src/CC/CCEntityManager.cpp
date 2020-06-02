#include "CCEntityManager.h"

#include "CCNetworkEntity.h"
#include "CCDisplay.h"

bool operator==(CCNetworkEntity* entity, const Point& p)
{
    return *entity == p;
} 

CCEntityManager::CCEntityManager()
{

}

void CCEntityManager::DicoveredNewEntity(CCNetworkEntity* entity)
{
    entities.push_back(entity);
}

void CCEntityManager::LostEntity(CCNetworkEntity* entity)
{
    auto itr = std::find(entities.begin(), entities.end(), entity);
    if(itr != entities.end())
        entities.erase(itr);
}

CCNetworkEntity* CCEntityManager::EntityForGlobalPosition(const Point& p)
{
    auto itr = std::find(entities.begin(), entities.end(), p);

    if(itr != entities.end())
        return *itr;

    return NULL;
}