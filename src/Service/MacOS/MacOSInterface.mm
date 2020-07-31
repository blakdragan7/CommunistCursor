#include "../OSInterface/NativeInterface.h"

#include <ApplicationServices/ApplicationServices.h>

#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hid/IOHIDManager.h>

#include <iostream>
#include <string>

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#import <sys/types.h>
#import <net/if.h>
#import <net/if_var.h>
#import <ifaddrs.h>
#import <sys/socket.h>
#import <sys/types.h>
#import <net/ethernet.h>
#import <arpa/inet.h>
#import <netinet/in.h>

#define S(a) std::to_string(a)

#define INET_ADDRSLEN 16
#define INET6_ADDRSLEN 46

// change this latter to something that won't overlap any other error codes
#define ERROR_NO_IMP -1
#define ERROR_INVALID_PARAM -2
#define ERROR_COMM -3
#define ERROR_UNKOWN -4

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
            
            event.scanCode = scancode;
            
            event.eventType = OS_EVENT_KEY;
            event.keyEvent = pressed == 1 ? KEY_EVENT_DOWN : KEY_EVENT_UP;
        }
        case kHIDPage_Button:
        {
            event.eventType = OS_EVENT_MOUSE;
            switch (usage) {
                    // left button
                case kHIDUsage_Button_1:
                    event.mouseButton = MOUSE_BUTTON_LEFT;
                    break;
                    // right button
                case kHIDUsage_Button_2:
                    event.mouseButton = MOUSE_BUTTON_RIGHT;
                    break;
                    // middle button probably
                case kHIDUsage_Button_3:
                    event.mouseButton = MOUSE_BUTTON_MIDDLE;
                    break;
                default:
                    break;
            }
            
            event.mouseEvent = integerValue ? MOUSE_EVENT_DOWN : MOUSE_EVENT_UP;
        }
    
        case kHIDPage_GenericDesktop:
        {
            event.eventType = OS_EVENT_MOUSE;
            event.mouseEvent = MOUSE_EVENT_MOVE;
            switch (usage) {
                case kHIDUsage_GD_X:
                    event.deltaX = integerValue;
                    break;
                case kHIDUsage_GD_Y:
                    event.deltaY = integerValue;
                    break;
                case kHIDUsage_GD_Wheel:
                    event.mouseEvent = MOUSE_EVENT_SCROLL;
                    event.mouseButton = MOUSE_BUTTON_MIDDLE;
                    event.extendButtonInfo = integerValue;
                    break;
                default:
                    break;
            }
        }
    }
    
    if(osi->ConsumeInputEvent(event))
    {
        // somehow stop event from moving forward
    }
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

int StartupOSConnection()
{
    return 0;
}

int ShutdownOSConnection()
{
    return 0;
}

int SetMousePosition(int x,int y)
{
    return (int)CGDisplayMoveCursorToPoint(CGMainDisplayID(), CGPointMake(x, y));
}

int GetMousePosition(int& xPos, int& yPos)
{
    NSPoint point;
    point = [NSEvent mouseLocation];
    xPos = point.x;
    yPos = point.y;
    
    return 0;
}

int SetMouseHidden(bool isHidden)
{
    if(isHidden)
        [NSCursor hide];
    else
        [NSCursor unhide];
}

int GetProcessExitCode(int processID, unsigned long* exitCode)
{
    return ERROR_NO_IMP;
}

int GetIsProcessActive(int processID, bool* isActive)
{
    return ERROR_NO_IMP;
}

int StartProcessAsDesktopUser(std::string process, std::string args, std::string workingDir,bool isVisible, ProccessInfo* processInfo)
{
    return ERROR_NO_IMP;
}

int GetClipBoard(ClipboardData& outData)
{
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        NSString* string = [pb stringForType:NSPasteboardTypeString];
        if(string == nil)
            return 0; // we don't have clipboard data or something wen't wrong sadly we don't know which one
        outData.stringData = string.UTF8String;
        outData.type = ClipboardDataType::Text;
    }
    
    return 0;
}

int SetClipBoard(const ClipboardData& data)
{
    if(data.type != ClipboardDataType::Text)
        return ERROR_INVALID_PARAM;
    
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        NSString* pData = [NSString stringWithUTF8String:data.stringData.c_str()];
        try {
            if([pb declareTypes:@[NSPasteboardTypeString] owner:nil] == NO)
            {
                return ERROR_UNKOWN;
            }
            
            if([pb setString:pData forType:NSPasteboardTypeString] == NO)
            {
                return ERROR_UNKOWN;
            }
        } catch (NSException* e) {
            if([e.name isEqualToString:NSPasteboardCommunicationException])
            {
                return ERROR_COMM;
            }
            
            return ERROR_UNKOWN;
        }
        
    }
    return 0;
}

int GetAllDisplays(std::vector<NativeDisplay>& outDisplays)
{
    NSArray<NSScreen*>* screens = [NSScreen screens];

    for(NSScreen* screen in screens)
    {
        NativeDisplay display;
        id screenNumber = [screen.deviceDescription objectForKey:@"NSSScreenNumber"];
        display.nativeScreenID = [screenNumber integerValue];
        display.posX = screen.frame.origin.x;
        display.posY = screen.frame.origin.y;
        display.height = screen.frame.size.height;
        display.width = screen.frame.size.width;
        
        outDisplays.push_back(display);
    }
    
    return 0;
}

int GetIPAddressList(std::vector<IPAdressInfo>& outAddresses, const IPAdressInfoHints& hints)
{
    if((bool)(hints.type & IPAddressType::MULTICAST) || (bool)(hints.type & IPAddressType::ANYCAST) || (bool)(hints.type & IPAddressType::DNSSERVER))
    {
        return ERROR_NO_IMP;
        // not compatable
    }
    
    struct ifaddrs *addrs, *tmp;
    
    if(getifaddrs(&addrs) == -1)
        return errno;
    
    tmp = addrs;
    
    while(tmp)
    {
        if(tmp->ifa_addr && !(tmp->ifa_flags & IFF_LOOPBACK))
        {
            if(tmp->ifa_addr->sa_family == AF_INET && ((bool)(hints.familly & IPAddressFamilly::IPv4) && (bool)(hints.type & IPAddressType::UNICAST)))
            {
                IPAdressInfo info;
                info.adaptorName = tmp->ifa_name;
                
                char address[INET_ADDRSLEN] = {0};
                char mask[INET_ADDRSLEN] = {0};
                
                struct sockaddr_in* inAd= (sockaddr_in*)tmp->ifa_addr;
                
                inet_ntop(inAd->sin_family, (void*)&inAd->sin_addr, address, INET_ADDRSLEN);
                
                if(tmp->ifa_netmask)
                {
                    inAd = (sockaddr_in*)tmp->ifa_netmask;
                    inet_ntop(inAd->sin_family, (void*)&inAd->sin_addr, mask, INET_ADDRSLEN);
                }
                
                info.address = address;
                info.subnetMask = mask;
                info.addressFamilly = IPAddressFamilly::IPv4;
                info.addressType = IPAddressType::UNICAST;
                
                outAddresses.push_back(info);
            }
            
            if(tmp->ifa_addr->sa_family == AF_INET6 && ((bool)(hints.familly & IPAddressFamilly::IPv6) && (bool)(hints.type & IPAddressType::UNICAST)))
            {
                IPAdressInfo info;
                info.adaptorName = tmp->ifa_name;
                
                char address[INET6_ADDRSLEN] = {0};
                char mask[INET6_ADDRSLEN] = {0};
                
                struct sockaddr_in* inAd= (sockaddr_in*)tmp->ifa_addr;
                
                inet_ntop(inAd->sin_family, (void*)&inAd->sin_addr, address, INET6_ADDRSLEN);
                
                if(tmp->ifa_netmask)
                {
                    inAd = (sockaddr_in*)tmp->ifa_netmask;
                    inet_ntop(inAd->sin_family, (void*)&inAd->sin_addr, mask, INET6_ADDRSLEN);
                }
                
                info.address = address;
                info.subnetMask = mask;
                info.addressFamilly = IPAddressFamilly::IPv6;
                info.addressType = IPAddressType::UNICAST;
                
                outAddresses.push_back(info);
            }
        }
        tmp = tmp->ifa_next;
    }
    
    freeifaddrs(addrs);
    
    return 0;
}

int GetHostName(std::string& hostName)
{
    @autoreleasepool {
        NSString* hName = [[NSHost currentHost] name];
        
        if(hName.length <= 0)
            return ERROR_UNKOWN;
        
        hostName = [hName UTF8String];
    }
    
    return 0;
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
    
    if(ret != kIOReturnSuccess)
    {
        NativeUnhookAllEvents();
    }
    
    CFRelease(matches);
    CFRelease(pointer);
    CFRelease(mouse);
    CFRelease(keypad);
    CFRelease(keyboard);
    
    return ret;
}

void OSMainLoop(bool& shouldRun)
{
    while(shouldRun)
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
    CGEventType eventType = kCGEventMouseMoved;
    CGMouseButton mouseButton = kCGMouseButtonLeft;
    CGEventRef event;
    
    static bool leftMouseDown = false;
    static bool rightMouseDown = false;
    static bool centerMouseDown = false;
    
    static int lastEventPosX = 0;
    static int lastEventPosY = 0;
    
    int eventPosX = lastEventPosX;
    int eventPosY = lastEventPosY;
    
    bool isScrollEvent = false;
    
    switch (mouseEvent.mouseEvent)
    {
    case MOUSE_EVENT_MOVE:
        eventPosX = mouseEvent.x;
        eventPosY = mouseEvent.y;
        if(leftMouseDown)
        {
            mouseButton = kCGMouseButtonLeft;
            eventType = kCGEventLeftMouseDragged;
        }
        else if(rightMouseDown)
        {
            mouseButton = kCGMouseButtonRight;
            eventType = kCGEventRightMouseDragged;
        }
        else if(centerMouseDown)
        {
            mouseButton = kCGMouseButtonCenter;
            eventType = kCGEventOtherMouseDragged;
        }
        else
        {
            eventType = kCGEventMouseMoved;
        }
        break;
    case MOUSE_EVENT_DOWN:
            switch (mouseEvent.mouseButton)
            {
                case MOUSE_BUTTON_LEFT:
                    eventType = kCGEventLeftMouseDown;
                    mouseButton = kCGMouseButtonLeft;
                    leftMouseDown = true;
                    break;
                case MOUSE_BUTTON_RIGHT:
                    eventType = kCGEventRightMouseDown;
                    mouseButton = kCGMouseButtonRight;
                    rightMouseDown = true;
                    break;
                case MOUSE_BUTTON_MIDDLE:
                    eventType = kCGEventOtherMouseDown;
                    mouseButton = kCGMouseButtonCenter;
                    centerMouseDown = true;
                    break;
                default:
                    break;
            }
        
        break;
    case MOUSE_EVENT_UP:
        switch (mouseEvent.mouseButton)
        {
            case MOUSE_BUTTON_LEFT:
                eventType = kCGEventLeftMouseUp;
                mouseButton = kCGMouseButtonLeft;
                leftMouseDown = false;
                break;
            case MOUSE_BUTTON_RIGHT:
                eventType = kCGEventRightMouseUp;
                mouseButton = kCGMouseButtonRight;
                rightMouseDown = false;
                break;
            case MOUSE_BUTTON_MIDDLE:
                eventType = kCGEventOtherMouseUp;
                mouseButton = kCGMouseButtonCenter;
                centerMouseDown = false;
                break;
            default:
                break;
        }
        break;
    case MOUSE_EVENT_SCROLL:
        isScrollEvent = true;
        break;
    case MOUSE_EVENT_INVALID:
    default:
        return ERROR_INVALID_PARAM;
    }
    
    if(isScrollEvent)
        event = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitPixel, 1, mouseEvent.extendButtonInfo*32);
    else
        event = CGEventCreateMouseEvent(NULL, eventType, CGPointMake(eventPosX, eventPosY), mouseButton);
    
    CGEventPost(kCGHIDEventTap, event);
    
    CFRelease(event);
    
    lastEventPosX = eventPosX;
    lastEventPosY = eventPosY;
    
    return 0;
}

int GetAllDisplays(std::list<NativeDisplay>& outDisplays)
{
    CGDirectDisplayID ids[16] = {0};
    uint32_t numDisplays = 0;
    
    CGError error = CGGetActiveDisplayList(16, ids, &numDisplays);
    
    if(error != kCGErrorSuccess)
    {
        return error;
    }
    
    for(int i=0; i<numDisplays; i++)
    {
        NativeDisplay display;
        display.nativeScreenID = ids[i];
        
        CGRect bounds = CGDisplayBounds(display.nativeScreenID);
        
        display.posX = bounds.origin.x;
        display.posY = bounds.origin.y;
        
        display.width = bounds.size.width;
        display.height = bounds.size.height;
        
        outDisplays.push_back(display);
    }
}

int SendKeyEvent(const OSEvent keyEvent)
{
    CGKeyCode code;
    bool keydown;
    
    keydown = keyEvent.keyEvent == KEY_EVENT_DOWN;
    code = keyEvent.scanCode;
    
    CGEventRef event = CGEventCreateKeyboardEvent(NULL, code, keydown);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
    
    return 0;
}

int ConvertEventCoordsToNative(const OSEvent inEvent, OSEvent& outEvent)
{
    outEvent = inEvent;
    // no need on mac
    return 0;
}

OSInterfaceError OSErrorToOSInterfaceError(int OSError)
{
    switch (OSError) {
        case ERROR_NO_IMP:
            return OSInterfaceError::OS_E_NOT_IMPLEMENTED;
            break;
        case ERROR_INVALID_PARAM:
            return OSInterfaceError::OS_E_INVALID_PARAM;
            break;
        case ERROR_COMM:
            return OSInterfaceError::OS_E_COMMUNICATION_ERROR;
            break;
        case ERROR_UNKOWN:
        default:
            return OSInterfaceError::OS_E_SUCCESS;
            break;
    }
}
