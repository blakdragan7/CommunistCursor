#ifndef IGUI_SERVICE_INTERFACE_H
#define IGUI_SERVICE_INTERFACE_H

#include <vector>
#include <memory>

class CCNetworkEntity;
class IGuiServiceInterface
{
public:
	virtual const std::vector<std::shared_ptr<CCNetworkEntity>>& GetEntitiesToConfigure()const = 0;
	virtual const std::vector<int>& GetGlobalBounds()const = 0;
};

#endif