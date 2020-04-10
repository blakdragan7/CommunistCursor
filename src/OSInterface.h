#ifndef MOUSEINTERFACE_H
#define MOUSE_INTERFACE_H

#include <functional>

#include "OSInterfaceError.h"

enum MouseEventType
{
    MOUSE_ET_MOVE,
    MOUSE_ET_DOWN,
    MOUSE_ET_UP,
    MOUSE_ET_SCROLL,
    MOUSE_ET_INVALID
};

enum MouseButton
{
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_EXTENDED,
    MOUSE_BUTTON_INVALID
};

struct MouseEvent
{
    MouseEventType eventType;
    MouseButton eventButton;
    int extendButtonInfo;
    int posX,posY;

    MouseEvent::MouseEvent() : eventType(MOUSE_ET_INVALID), eventButton(MOUSE_BUTTON_INVALID), extendButtonInfo(0), \
                                posX(0), posY(0) {}
};

typedef std::function<void(MouseEvent, void* userInfo)> MouseEventCallback;

extern OSInterfaceError OSISetMousePosition(int x, int y);
extern OSInterfaceError OSIGetMousePosition(int *x, int *y);
extern OSInterfaceError OSIRegisterForMouseEvents(MouseEventCallback callback, void* userInfo);
extern OSInterfaceError OSIUnRegisterForMouseEvents(void* userInfo);
#endif