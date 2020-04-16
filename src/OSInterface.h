#ifndef MOUSEINTERFACE_H
#define MOUSE_INTERFACE_H

#include <functional>
#include <mutex>
#include <map>

#include "OSInterfaceError.h"

enum OSEventType
{
    OS_EVENT_MOUSE,
    OS_EVENT_KEY,
    OS_EVENT_HID,
    OS_EVENT_INVALID = -1
};

enum MouseEventType
{
    MOUSE_EVENT_MOVE,
    MOUSE_EVENT_DOWN,
    MOUSE_EVENT_UP,
    MOUSE_EVENT_SCROLL,
    MOUSE_EVENT_INVALID = -1
};

enum MouseButton
{
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_EXTENDED,
    MOUSE_BUTTON_INVALID = -1
};

enum KeyEventType
{
    KEY_EVENT_DOWN,
    KEY_EVENT_UP,
    KEY_EVENT_INVALID = -1
};

struct OSEvent
{
    OSEventType eventType;
    union
    {
        MouseEventType mouseEvent;
        KeyEventType keyEvent;
        int raw;
    } subEvent;
    
    union
    {
        MouseButton mouseButton;
        int key;
    }eventButton;

    int extendButtonInfo;
    int posX,posY;

    OSEvent::OSEvent() : eventType(OS_EVENT_INVALID), extendButtonInfo(0), \
                                posX(0), posY(0) {subEvent.keyEvent = KEY_EVENT_INVALID;eventButton.key = -1;}
};

extern std::ostream& operator<<(std::ostream& os, const OSEvent& event);

typedef std::function<void(OSEvent, void* userInfo)> OSEventCallback;
typedef std::pair<void*, OSEventCallback> OSEventEntry;

class OSInterface
{
private:
    bool shouldRunMainloop;
    bool hasHookedEvents;
    std::map<void*, OSEventCallback> OSEventRegisterdCallbacks;
    std::mutex MapAccessMutex;

    OSInterface() :shouldRunMainloop(true), hasHookedEvents(false){}

    static OSInterface sharedInterface;
public:
    
    static OSInterface& SharedInterface();

    OSInterfaceError SetMousePosition(int x, int y);
    OSInterfaceError GetMousePosition(int *x, int *y);
    OSInterfaceError RegisterForOSEvents(OSEventCallback callback, void* userInfo);
    OSInterfaceError UnRegisterForOSEvents(void* userInfo);
    
    void OSMainLoop();
    void UpdateThread(OSEvent event);
};

#endif