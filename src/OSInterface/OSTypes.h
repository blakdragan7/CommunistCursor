#ifndef OS_TYPES_H
#define OS_TYPES_H

#include <iostream>

enum OSEventType
{
    OS_EVENT_MOUSE = 0,
    OS_EVENT_KEY = 1,
    OS_EVENT_HID = 2,
    OS_EVENT_INVALID = -1
};

enum MouseEventType
{
    MOUSE_EVENT_MOVE = 0,
    MOUSE_EVENT_DOWN = 1,
    MOUSE_EVENT_UP = 2,
    MOUSE_EVENT_SCROLL = 3,
    MOUSE_EVENT_INVALID = -1
};

enum MouseButton
{
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT = 1,
    MOUSE_BUTTON_MIDDLE = 2,
    MOUSE_BUTTON_EXTENDED = 3,
    MOUSE_BUTTON_INVALID = -1
};

enum KeyEventType
{
    KEY_EVENT_DOWN = 0,
    KEY_EVENT_UP = 1,
    KEY_EVENT_INVALID = -1
};

struct NativeDisplay
{
    int nativeScreenID;
    int posX,posY;
    int width,height;
    NativeDisplay() : nativeScreenID(-1), posX(-1), posY(-1), width(-1), height(-1)
    {}
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
    int deltaX, deltaY;
    int nativeScreenID;

    OSEvent() : eventType(OS_EVENT_INVALID), extendButtonInfo(0), \
                                deltaX(0), deltaY(0), nativeScreenID(-1)
    {subEvent.keyEvent = KEY_EVENT_INVALID;eventButton.scanCode = -1;}
};

extern std::ostream& operator<<(std::ostream& os, const OSEvent& event);
extern std::ostream& operator<<(std::ostream& os, const NativeDisplay& event);

#endif