#include "OSInterface.h"
#include <iostream>

#define SLEEPM(a) std::this_thread::sleep_for(std::chrono::milliseconds(a));

extern int NativeRegisterForOSEvents(OSInterface* osi);
extern void OSMainLoop(bool& stopSwitch);
extern void NativeUnhookAllEvents();

OSInterface OSInterface::sharedInterface;

OSInterface& OSInterface::SharedInterface()
{
    return sharedInterface;
}

OSInterfaceError OSInterface::SetMousePosition(int x, int y)
{
    return OS_E_SUCCESS;
}

OSInterfaceError OSInterface::GetMousePosition(int *x, int *y)
{
    return OS_E_SUCCESS;
}

OSInterfaceError OSInterface::RegisterForOSEvents(OSEventCallback callback, void* userInfo)
{
    if(hasHookedEvents == false)
    {
        int res = NativeRegisterForOSEvents(this);
        if(res != 0)
        {
            std::cout << "Error hooking events with " << res << std::endl;
            return OS_E_NOT_REGISTERED;
        }
            
        hasHookedEvents = true;
    }

    while(MapAccessMutex.try_lock() == false)SLEEPM(1);

    if(OSEventRegisterdCallbacks.count(userInfo) > 0)
    {
        MapAccessMutex.unlock();
        return OS_E_ALREADY_REGISTERED;
    }
    else
    {
        OSEventRegisterdCallbacks.insert(OSEventEntry(userInfo, callback));
        MapAccessMutex.unlock();
        return OS_E_SUCCESS;
    }
}

OSInterfaceError OSInterface::UnRegisterForOSEvents(void* userInfo)
{
    while(MapAccessMutex.try_lock() == false)SLEEPM(1);

    if(OSEventRegisterdCallbacks.count(userInfo) > 0)
    {
        OSEventRegisterdCallbacks.erase(userInfo);
        if(OSEventRegisterdCallbacks.empty())
        {
            NativeUnhookAllEvents();
            hasHookedEvents = false;
        }
        MapAccessMutex.unlock();
        return OS_E_SUCCESS;
    }
    else
    {
        MapAccessMutex.unlock();
        return OS_E_NOT_REGISTERED;
    }
}

void OSInterface::OSMainLoop()
{
    ::OSMainLoop(shouldRunMainloop);
}

void OSInterface::UpdateThread(OSEvent event)
{
    while(MapAccessMutex.try_lock() == false)SLEEPM(1);

    for(OSEventEntry entry : OSEventRegisterdCallbacks)
    {
        entry.second(event, entry.first);
    }

    MapAccessMutex.unlock();
}