#include "../OSInterface/NativeInterface.h"

#include <ApplicationServices/ApplicationServices.h>

#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hid/IOHIDManager.h>

CGEventRef
callback(CGEventTapProxy proxy, CGEventType type,
                  CGEventRef event, void *refcon)
{
    OSEvent osEvent;
    OSInterface* osi = static_cast<OSInterface*>(refcon);
    
    switch(type)
    {
        case kCGEventLeftMouseDown:
            osEvent.eventType = OS_EVENT_MOUSE;
            osEvent.subEvent.mouseEvent = MOUSE_EVENT_DOWN;
            osEvent.eventButton.mouseButton = MOUSE_BUTTON_LEFT;
            break;
        case kCGEventLeftMouseUp:
            osEvent.eventType = OS_EVENT_MOUSE;
            osEvent.subEvent.mouseEvent = MOUSE_EVENT_UP;
            osEvent.eventButton.mouseButton = MOUSE_BUTTON_LEFT;
            break;
        case kCGEventRightMouseDown:
            osEvent.eventType = OS_EVENT_MOUSE;
            osEvent.subEvent.mouseEvent = MOUSE_EVENT_DOWN;
            osEvent.eventButton.mouseButton = MOUSE_BUTTON_RIGHT;
        break;
        case kCGEventRightMouseUp:
            osEvent.eventType = OS_EVENT_MOUSE;
            osEvent.subEvent.mouseEvent = MOUSE_EVENT_UP;
            osEvent.eventButton.mouseButton = MOUSE_BUTTON_RIGHT;
        break;
        case kCGEventMouseMoved:
            osEvent.eventType = OS_EVENT_MOUSE;
            osEvent.subEvent.mouseEvent = MOUSE_EVENT_MOVE;
        break;
        case kCGEventKeyDown:
            osEvent.eventType = OS_EVENT_KEY;
            osEvent.subEvent.keyEvent = KEY_EVENT_DOWN;
            osEvent.eventButton.scanCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            break;
        case kCGEventKeyUp:
            osEvent.eventType = OS_EVENT_KEY;
            osEvent.subEvent.keyEvent = KEY_EVENT_DOWN;
            osEvent.eventButton.scanCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            break;
        case kCGEventScrollWheel:
            osEvent.eventType = OS_EVENT_MOUSE;
            osEvent.subEvent.mouseEvent = MOUSE_EVENT_SCROLL;
            osEvent.eventButton.mouseButton = MOUSE_BUTTON_MIDDLE;
        case kCGEventFlagsChanged:
            osEvent.eventType = OS_EVENT_KEY;
            osEvent.subEvent.keyEvent = KEY_EVENT_DOWN;
            osEvent.eventButton.scanCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        break;
        default:
            return event;
    }
    
    osi->UpdateThread(osEvent);
    
    return event;
}

int NativeRegisterForOSEvents(OSInterface* osi)
{

    CGEventMask eventMask =
        CGEventMaskBit(kCGEventLeftMouseDown) |
        CGEventMaskBit(kCGEventLeftMouseUp) |
        CGEventMaskBit(kCGEventScrollWheel) |
        CGEventMaskBit(kCGEventRightMouseDown) |
        CGEventMaskBit(kCGEventRightMouseUp) |
        CGEventMaskBit(kCGEventMouseMoved) |
        CGEventMaskBit(kCGEventKeyDown) |
        CGEventMaskBit(kCGEventFlagsChanged) |
        CGEventMaskBit(kCGEventKeyUp);

    CFMachPortRef eventTap = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, eventMask, callback, (void*)osi);

    if(!eventTap)
        return -1;

    // Create a run loop source.
    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(
                        kCFAllocatorDefault, eventTap, 0);


    // Add to the current run loop.
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource,
                       kCFRunLoopCommonModes);


    // Enable the event tap.
    CGEventTapEnable(eventTap, true);

    return 0;
}

void OSMainLoop(bool& stopSwitch)
{
    while(stopSwitch == true)
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 5, true);
}

void NativeUnhookAllEvents()
{
    
}

int SendMouseEvent(const OSEvent mouseEvent)
{
    return 0;
}

int SendKeyEvent(const OSEvent keyEvent)
{
    return 0;
}

int StoreScreenSize()
{
    return 0;
}

int ConvertEventCoordsToNative(const OSEvent inEvent, OSEvent& outEvent)
{
    return 0;
}

OSInterfaceError OSErrorToOSInterfaceError(int OSError)
{
    return OS_E_SUCCESS;
}
