#ifndef MOUSE_INTERFACE_H
#define MOUSE_INTERFACE_H

#include <functional>
#include <mutex>
#include <map>

#include "OSInterfaceError.h"
#include "OSTypes.h"

typedef std::function<void(OSEvent, void* userInfo)> OSEventCallback;
typedef std::pair<void*, OSEventCallback> OSEventEntry;

class OSInterface
{
private:
    bool shouldRunMainloop;
    bool hasHookedEvents;
    std::map<void*, OSEventCallback> OSEventRegisterdCallbacks;
    std::mutex MapAccessMutex;

    OSInterface();

    static OSInterface sharedInterface;
public:
    
    static OSInterface& SharedInterface();

    OSInterfaceError ConvertEventToNativeCoords(const OSEvent inEvent, OSEvent& outEvent);
    OSInterfaceError SendMouseEvent(const OSEvent mouseEvent);
    OSInterfaceError SendKeyEvent(const OSEvent keyEvent);
    OSInterfaceError RegisterForOSEvents(OSEventCallback callback, void* userInfo);
    OSInterfaceError UnRegisterForOSEvents(void* userInfo);
    
    void OSMainLoop();
    void UpdateThread(OSEvent event);
};

extern std::string OSEventTypeToString(OSEventType);
extern std::string MouseEventTypeToString(MouseEventType);
extern std::string KeyEventTypeToString(KeyEventType);
extern std::string MouseButtonToString(MouseButton);

#endif
