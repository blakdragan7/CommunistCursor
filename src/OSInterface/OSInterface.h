#ifndef MOUSE_INTERFACE_H
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
        int scanCode;
    }eventButton;

    int extendButtonInfo;
    int posX,posY;
    int maxX, maxY;
    int minX, minY;

    OSEvent() : eventType(OS_EVENT_INVALID), extendButtonInfo(0), \
                                posX(0), posY(0), maxX(0), maxY(0), minX(0), minY(0)
    {subEvent.keyEvent = KEY_EVENT_INVALID;eventButton.scanCode = -1;}
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

#endif
