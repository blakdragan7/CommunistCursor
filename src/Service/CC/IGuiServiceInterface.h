#ifndef IGUI_SERVICE_INTERFACE_H
#define IGUI_SERVICE_INTERFACE_H

#include <vector>
#include <memory>

class CCNetworkEntity;
class IGuiServiceInterface
{
public:
	// Used to retreive list of entities to send to GUI
	virtual const std::vector<std::shared_ptr<CCNetworkEntity>>& GetEntitiesToConfigure()const = 0;
	// Used to retrieve global bounds GUI uses for entity configuration
	virtual const std::vector<int>& GetGlobalBounds()const = 0;
	// Called when Gui Sends Offsets for Configuration and Entites have had them applied
	// Note: this is generally called on a background thread. So watchout for thread safety
	virtual void EntitiesFinishedConfiguration() = 0;
};

#endif