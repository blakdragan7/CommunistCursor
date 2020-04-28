#include "../OSInterface/NativeInterface.h"

#include <ApplicationServices/ApplicationServices.h>

#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hid/IOHIDManager.h>

#include <iostream>
#include <string>

#define S(a) std::to_string(a)

IOHIDManagerRef hidManager = NULL;

void myHIDKeyboardCallback( void* context,  IOReturn result,  void* sender,  IOHIDValueRef value )
{
    // a non zero result means there was an error we can't do anything about
    if(result != 0)return;
    
    IOHIDElementRef elem = IOHIDValueGetElement( value );
    UInt32 usagePage = IOHIDElementGetUsagePage(elem);
    uint32_t usage = IOHIDElementGetUsage( elem );
    long integerValue = IOHIDValueGetIntegerValue( value );
    
    OSInterface* osi = static_cast<OSInterface*>(context);
    OSEvent event;
    
    switch(usagePage)
    {
        case kHIDPage_KeyboardOrKeypad:
        {
            uint32_t scancode = IOHIDElementGetUsage( elem );
            long pressed = IOHIDValueGetIntegerValue( value );
        
            if(scancode <= kHIDUsage_KeyboardErrorUndefined || scancode > kHIDUsage_KeyboardRightGUI)
                return;
            
            event.eventButton.scanCode = scancode;
            
            event.eventType = OS_EVENT_KEY;
            event.subEvent.keyEvent = pressed == 1 ? KEY_EVENT_DOWN : KEY_EVENT_UP;
        }
        case kHIDPage_Button:
        {
            event.eventType = OS_EVENT_MOUSE;
            switch (usage) {
                    // left button
                case kHIDUsage_Button_1:
                    event.eventButton.mouseButton = MOUSE_BUTTON_LEFT;
                    break;
                    // right button
                case kHIDUsage_Button_2:
                    event.eventButton.mouseButton = MOUSE_BUTTON_RIGHT;
                    break;
                    // middle button probably
                case kHIDUsage_Button_3:
                    event.eventButton.mouseButton = MOUSE_BUTTON_MIDDLE;
                    break;
                default:
                    break;
            }
            
            event.subEvent.mouseEvent = integerValue ? MOUSE_EVENT_DOWN : MOUSE_EVENT_UP;
        }
    
        case kHIDPage_GenericDesktop:
        {
            event.eventType = OS_EVENT_MOUSE;
            switch (usage) {
                case kHIDUsage_GD_X:
                    event.posX = integerValue;
                    break;
                case kHIDUsage_GD_Y:
                    event.posY = integerValue;
                    break;
                case kHIDUsage_GD_Wheel:
                    event.eventButton.mouseButton = MOUSE_BUTTON_MIDDLE;
                    event.extendButtonInfo = integerValue;
                    break;
                default:
                    break;
            }
        }
    }
    
    osi->UpdateThread(event);
}

CFMutableDictionaryRef CreateDeviceMatchingDictionary(UInt32 usagePage, UInt32 usage)
{
    CFMutableDictionaryRef dict = CFDictionaryCreateMutable(
                                                            kCFAllocatorDefault, 0
                                                        , & kCFTypeDictionaryKeyCallBacks
                                                        , & kCFTypeDictionaryValueCallBacks );
    if ( ! dict )
        return NULL;
    
    CFNumberRef pageNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &usagePage);
    
    if ( ! pageNumberRef ) {
        CFRelease( dict );
        return NULL;
    }

    CFDictionarySetValue( dict, CFSTR(kIOHIDDeviceUsagePageKey), pageNumberRef);
    CFRelease( pageNumberRef );

    CFNumberRef usageNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &usage );

    if ( ! usageNumberRef ) {
        CFRelease( dict );
        return NULL;
    }

    CFDictionarySetValue( dict, CFSTR(kIOHIDDeviceUsageKey), usageNumberRef );
    CFRelease( usageNumberRef );

    return dict;
}


int NativeRegisterForOSEvents(OSInterface* osi)
{
    hidManager = IOHIDManagerCreate( kCFAllocatorDefault, kIOHIDOptionsTypeNone );
    
    CFMutableDictionaryRef keyboard = CreateDeviceMatchingDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
    CFMutableDictionaryRef keypad = CreateDeviceMatchingDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Keypad);
    CFMutableDictionaryRef mouse = CreateDeviceMatchingDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse);
    CFMutableDictionaryRef pointer = CreateDeviceMatchingDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Pointer);
    
    CFMutableDictionaryRef matches_dict[] = {keyboard, keypad, mouse, pointer};
    
    CFArrayRef matches = CFArrayCreate(kCFAllocatorDefault, (const void**)matches_dict, 4, NULL);
    
    IOHIDManagerSetDeviceMatchingMultiple( hidManager, matches );

    IOHIDManagerRegisterInputValueCallback( hidManager, myHIDKeyboardCallback, (void*)osi );

    IOHIDManagerScheduleWithRunLoop( hidManager, CFRunLoopGetMain(), kCFRunLoopDefaultMode );

    IOReturn ret = IOHIDManagerOpen( hidManager, kIOHIDOptionsTypeNone );
    
    CFRelease(matches);
    CFRelease(pointer);
    CFRelease(mouse);
    CFRelease(keypad);
    CFRelease(keyboard);
    
    return 0;
}

void OSMainLoop(bool& stopSwitch)
{
    while(stopSwitch == true)
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 5, true);
}

void NativeUnhookAllEvents()
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
    IOHIDManagerRegisterInputValueCallback(hidManager, nullptr, nullptr);
#pragma clang diagnostic pop
    
    IOHIDManagerUnscheduleFromRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDManagerClose(hidManager, kIOHIDOptionsTypeNone);
    CFRelease(hidManager);
    
    hidManager = NULL;
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
