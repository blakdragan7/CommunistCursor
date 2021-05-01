#include "../OSInterface/NativeInterface.h"
#include "../OSInterface/KeyCodes.h"
#include "../CC/CCLogger.h"

#include <ApplicationServices/ApplicationServices.h>

#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hid/IOHIDManager.h>

#include <iostream>
#include <string>

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#import <Carbon/Carbon.h>

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

#define USE_RELATIVE_MOUSE_INPUT 1

int ConverKeyCodeToVirtualKey(KeyCode keyCode)
{
    switch (keyCode) {
        case KeyCode::KC_A:
            return kVK_ANSI_A;
            break;
        case KeyCode::KC_B:
            return kVK_ANSI_B;
        break;
        case KeyCode::KC_C:
            return kVK_ANSI_C;
        break;
        case KeyCode::KC_D:
            return kVK_ANSI_D;
        break;
        case KeyCode::KC_E:
            return kVK_ANSI_E;
        break;
        case KeyCode::KC_F:
            return kVK_ANSI_F;
        break;
        case KeyCode::KC_G:
            return kVK_ANSI_G;
        break;
        case KeyCode::KC_H:
            return kVK_ANSI_H;
        break;
        case KeyCode::KC_I:
            return kVK_ANSI_I;
        break;
        case KeyCode::KC_J:
            return kVK_ANSI_J;
        break;
        case KeyCode::KC_K:
            return kVK_ANSI_K;
        break;
        case KeyCode::KC_L:
            return kVK_ANSI_L;
        break;
        case KeyCode::KC_M:
            return kVK_ANSI_M;
        break;
        case KeyCode::KC_N:
            return kVK_ANSI_N;
        break;
        case KeyCode::KC_O:
            return kVK_ANSI_O;
        break;
        case KeyCode::KC_P:
            return kVK_ANSI_P;
        break;
        case KeyCode::KC_Q:
            return kVK_ANSI_Q;
        break;
        case KeyCode::KC_R:
            return kVK_ANSI_R;
        break;
        case KeyCode::KC_S:
            return kVK_ANSI_S;
        break;
        case KeyCode::KC_T:
            return kVK_ANSI_T;
        break;
        case KeyCode::KC_U:
            return kVK_ANSI_U;
        break;
        case KeyCode::KC_V:
            return kVK_ANSI_V;
        break;
        case KeyCode::KC_W:
            return kVK_ANSI_W;
        break;
        case KeyCode::KC_X:
            return kVK_ANSI_X;
        break;
        case KeyCode::KC_Y:
            return kVK_ANSI_Y;
        break;
        case KeyCode::KC_Z:
            return kVK_ANSI_Z;
        break;
        case KeyCode::KC_0:
            return kVK_ANSI_0;
        break;
        case KeyCode::KC_1:
            return kVK_ANSI_1;
        break;
        case KeyCode::KC_2:
            return kVK_ANSI_2;
        break;
        case KeyCode::KC_3:
            return kVK_ANSI_3;
        break;
        case KeyCode::KC_4:
            return kVK_ANSI_4;
        break;
        case KeyCode::KC_5:
            return kVK_ANSI_5;
        break;
        case KeyCode::KC_6:
            return kVK_ANSI_6;
        break;
        case KeyCode::KC_7:
            return kVK_ANSI_7;
        break;
        case KeyCode::KC_8:
            return kVK_ANSI_8;
        break;
        case KeyCode::KC_9:
            return kVK_ANSI_9;
        break;
        case KeyCode::KC_SPACE:
            return kVK_Space;
        break;
        case KeyCode::KC_MINUS:
            return kVK_ANSI_Minus;
        break;
        case KeyCode::KC_EQUALS:
            return kVK_ANSI_Equal;
        break;
        case KeyCode::KC_BACKSPACE:
            return kVK_Delete;
        break;
        case KeyCode::KC_NUM_PAD_0:
            return kVK_ANSI_Keypad0;
        break;
        case KeyCode::KC_NUM_PAD_1:
            return kVK_ANSI_Keypad1;
        break;
        case KeyCode::KC_NUM_PAD_2:
            return kVK_ANSI_Keypad2;
        break;
        case KeyCode::KC_NUM_PAD_3:
            return kVK_ANSI_Keypad3;
        break;
        case KeyCode::KC_NUM_PAD_4:
            return kVK_ANSI_Keypad4;
        break;
        case KeyCode::KC_NUM_PAD_5:
            return kVK_ANSI_Keypad5;
        break;
        case KeyCode::KC_NUM_PAD_6:
            return kVK_ANSI_Keypad6;
        break;
        case KeyCode::KC_NUM_PAD_7:
            return kVK_ANSI_Keypad7;
        break;
        case KeyCode::KC_NUM_PAD_8:
            return kVK_ANSI_Keypad8;
        break;
        case KeyCode::KC_NUM_PAD_9:
            return kVK_ANSI_Keypad9;
        break;
        case KeyCode::KC_NUM_PAD_SLASH:
            return kVK_ANSI_Slash;
        break;
        case KeyCode::KC_NUM_PAD_STAR:
            return kVK_ANSI_KeypadMultiply;
        break;
        case KeyCode::KC_NUM_PAD_MINUS:
            return kVK_ANSI_KeypadMinus;
        break;
        case KeyCode::KC_NUM_PAD_PLUS:
            return kVK_ANSI_KeypadPlus;
        break;
        case KeyCode::KC_NUM_PAD_NUMLOCK:
            return kVK_ANSI_KeypadClear;
        break;
        case KeyCode::KC_NUM_PAD_DECIMAL:
            return kVK_ANSI_KeypadDecimal;
        break;
        case KeyCode::KC_LEFT_ALT:
            return kVK_Option;
        break;
        case KeyCode::KC_LEFT_CNTRL:
            return kVK_Control;
        break;
        case KeyCode::KC_LEFT_SHIT:
            return kVK_Shift;
        break;
        case KeyCode::KC_CAPS_LOCK:
            return kVK_CapsLock;
        break;
        case KeyCode::KC_TAB:
            return kVK_Tab;
        break;
        case KeyCode::KC_GRAVE:
            return kVK_ANSI_Grave;
        break;
        case KeyCode::KC_LEFT_WINDOWS_KEY:
            return kVK_Command;
        break;
        case KeyCode::KC_LEFT_BRACE:
            return kVK_ANSI_LeftBracket;
        break;
        case KeyCode::KC_RIGHT_BRACE:
            return kVK_ANSI_RightBracket;
        break;
        case KeyCode::KC_BACK_SLASH:
            return kVK_ANSI_Backslash;
        break;
        case KeyCode::KC_ENTER:
            return kVK_ANSI_KeypadEnter;
        break;
        case KeyCode::KC_RIGHT_SHIFT:
            return kVK_RightShift;
        break;
        case KeyCode::KC_RIGHT_ALT:
            return kVK_RightOption;
        break;
        case KeyCode::KC_RIGHT_CTRL:
            return kVK_RightControl;
        break;
        case KeyCode::KC_RIGHT_WINDOWS_KEY:
            return kVK_RightCommand;
        break;
        case KeyCode::KC_SEMI_COLON:
            return kVK_ANSI_Semicolon;
        break;
        case KeyCode::KC_QUOTE:
            return kVK_ANSI_Quote;
        break;
        case KeyCode::KC_LESS_THEN:
            return kVK_ANSI_Comma;
        break;
        case KeyCode::KC_GREATER_THEN:
            return kVK_ANSI_Period;
        break;
        case KeyCode::KC_QUESTION_MARK:
            return kVK_ANSI_Slash;
        break;
        case KeyCode::KC_APPS_KEY:
            return (int)keyCode;
        break;
        case KeyCode::KC_INSERT:
            return kVK_Help;
        break;
        case KeyCode::KC_HOME:
            return kVK_Home;
        break;
        case KeyCode::KC_PAGE_UP:
            return kVK_PageUp;
        break;
        case KeyCode::KC_PAGE_DOWN:
            return kVK_PageDown;
        break;
        case KeyCode::KC_END:
            return kVK_End;
        break;
        case KeyCode::KC_DELETE:
            return kVK_ForwardDelete;
        break;
        case KeyCode::KC_PRINT_SCREEN:
            return (int)keyCode;
        break;
        case KeyCode::KC_SCROLL_LOCK:
            return (int)keyCode;
        break;
        case KeyCode::KC_PAUSE_BREAK:
            return (int)keyCode;
        break;
        case KeyCode::KC_F1:
            return kVK_F1;
        break;
        case KeyCode::KC_F2:
            return kVK_F2;
        break;
        case KeyCode::KC_F3:
            return kVK_F3;
        break;
        case KeyCode::KC_F4:
            return kVK_F4;
        break;
        case KeyCode::KC_F5:
            return kVK_F5;
        break;
        case KeyCode::KC_F6:
            return kVK_F6;
        break;
        case KeyCode::KC_F7:
            return kVK_F7;
        break;
        case KeyCode::KC_F8:
            return kVK_F8;
        break;
        case KeyCode::KC_F9:
            return kVK_F9;
        break;
        case KeyCode::KC_F10:
            return kVK_F10;
        break;
        case KeyCode::KC_F11:
            return kVK_F11;
        break;
        case KeyCode::KC_F12:
            return kVK_F12;
        break;
        case KeyCode::KC_ESC:
            return kVK_Escape;
        break;
        case KeyCode::KC_LEFT_ARROW:
            return kVK_LeftArrow;
        break;
        case KeyCode::KC_RIGHT_ARROW:
            return kVK_RightArrow;
        break;
        case KeyCode::KC_UP_ARROW:
            return kVK_UpArrow;
        break;
        case KeyCode::KC_DOWN_ARROW:
            return kVK_DownArrow;
        break;
        }
    
    return (int)keyCode;
}

KeyCode ConvertUsageToKeyCode(int usage)
{
    switch (usage)
    {
        case kHIDUsage_KeyboardA:                   return KeyCode::KC_A;
        case kHIDUsage_KeyboardB:                   return KeyCode::KC_B;
        case kHIDUsage_KeyboardC:                   return KeyCode::KC_C;
        case kHIDUsage_KeyboardD:                   return KeyCode::KC_D;
        case kHIDUsage_KeyboardE:                   return KeyCode::KC_E;
        case kHIDUsage_KeyboardF:                   return KeyCode::KC_F;
        case kHIDUsage_KeyboardG:                   return KeyCode::KC_G;
        case kHIDUsage_KeyboardH:                   return KeyCode::KC_H;
        case kHIDUsage_KeyboardI:                   return KeyCode::KC_I;
        case kHIDUsage_KeyboardJ:                   return KeyCode::KC_J;
        case kHIDUsage_KeyboardK:                   return KeyCode::KC_K;
        case kHIDUsage_KeyboardL:                   return KeyCode::KC_L;
        case kHIDUsage_KeyboardM:                   return KeyCode::KC_M;
        case kHIDUsage_KeyboardN:                   return KeyCode::KC_N;
        case kHIDUsage_KeyboardO:                   return KeyCode::KC_O;
        case kHIDUsage_KeyboardP:                   return KeyCode::KC_P;
        case kHIDUsage_KeyboardQ:                   return KeyCode::KC_Q;
        case kHIDUsage_KeyboardR:                   return KeyCode::KC_R;
        case kHIDUsage_KeyboardS:                   return KeyCode::KC_S;
        case kHIDUsage_KeyboardT:                   return KeyCode::KC_T;
        case kHIDUsage_KeyboardU:                   return KeyCode::KC_U;
        case kHIDUsage_KeyboardV:                   return KeyCode::KC_V;
        case kHIDUsage_KeyboardW:                   return KeyCode::KC_W;
        case kHIDUsage_KeyboardX:                   return KeyCode::KC_X;
        case kHIDUsage_KeyboardY:                   return KeyCode::KC_Y;
        case kHIDUsage_KeyboardZ:                   return KeyCode::KC_Z;

        case kHIDUsage_Keyboard1:                   return KeyCode::KC_1;
        case kHIDUsage_Keyboard2:                   return KeyCode::KC_2;
        case kHIDUsage_Keyboard3:                   return KeyCode::KC_3;
        case kHIDUsage_Keyboard4:                   return KeyCode::KC_4;
        case kHIDUsage_Keyboard5:                   return KeyCode::KC_5;
        case kHIDUsage_Keyboard6:                   return KeyCode::KC_6;
        case kHIDUsage_Keyboard7:                   return KeyCode::KC_7;
        case kHIDUsage_Keyboard8:                   return KeyCode::KC_8;
        case kHIDUsage_Keyboard9:                   return KeyCode::KC_9;
        case kHIDUsage_Keyboard0:                   return KeyCode::KC_0;

        case kHIDUsage_KeyboardReturnOrEnter:       return KeyCode::KC_ENTER;
        case kHIDUsage_KeyboardEscape:              return KeyCode::KC_ESC;
        case kHIDUsage_KeyboardDeleteOrBackspace:   return KeyCode::KC_BACKSPACE;
        case kHIDUsage_KeyboardTab:                 return KeyCode::KC_TAB;
        case kHIDUsage_KeyboardSpacebar:            return KeyCode::KC_SPACE;
        case kHIDUsage_KeyboardHyphen:              return KeyCode::KC_MINUS;
        case kHIDUsage_KeyboardEqualSign:           return KeyCode::KC_EQUALS;
        case kHIDUsage_KeyboardOpenBracket:         return KeyCode::KC_LEFT_BRACE;
        case kHIDUsage_KeyboardCloseBracket:        return KeyCode::KC_RIGHT_BRACE;
        case kHIDUsage_KeyboardBackslash:           return KeyCode::KC_BACK_SLASH;
        case kHIDUsage_KeyboardSemicolon:           return KeyCode::KC_SEMI_COLON;
        case kHIDUsage_KeyboardQuote:               return KeyCode::KC_QUOTE;
        case kHIDUsage_KeyboardGraveAccentAndTilde: return KeyCode::KC_GRAVE;
        case kHIDUsage_KeyboardComma:               return KeyCode::KC_LESS_THEN;
        case kHIDUsage_KeyboardPeriod:              return KeyCode::KC_GREATER_THEN;
        case kHIDUsage_KeyboardSlash:               return KeyCode::KC_NUM_PAD_SLASH;
        case kHIDUsage_KeyboardCapsLock:            return KeyCode::KC_CAPS_LOCK;

        case kHIDUsage_KeyboardF1:                  return KeyCode::KC_F1;
        case kHIDUsage_KeyboardF2:                  return KeyCode::KC_F2;
        case kHIDUsage_KeyboardF3:                  return KeyCode::KC_F3;
        case kHIDUsage_KeyboardF4:                  return KeyCode::KC_F4;
        case kHIDUsage_KeyboardF5:                  return KeyCode::KC_F5;
        case kHIDUsage_KeyboardF6:                  return KeyCode::KC_F6;
        case kHIDUsage_KeyboardF7:                  return KeyCode::KC_F7;
        case kHIDUsage_KeyboardF8:                  return KeyCode::KC_F8;
        case kHIDUsage_KeyboardF9:                  return KeyCode::KC_F9;
        case kHIDUsage_KeyboardF10:                 return KeyCode::KC_F10;
        case kHIDUsage_KeyboardF11:                 return KeyCode::KC_F11;
        case kHIDUsage_KeyboardF12:                 return KeyCode::KC_F12;

        case kHIDUsage_KeyboardPrintScreen:         return KeyCode::KC_PRINT_SCREEN;
        case kHIDUsage_KeyboardScrollLock:          return KeyCode::KC_SCROLL_LOCK;
        case kHIDUsage_KeyboardPause:               return KeyCode::KC_PAUSE_BREAK;
        case kHIDUsage_KeyboardInsert:              return KeyCode::KC_INSERT;
        case kHIDUsage_KeyboardHome:                return KeyCode::KC_HOME;
        case kHIDUsage_KeyboardPageUp:              return KeyCode::KC_PAGE_UP;
        case kHIDUsage_KeyboardDeleteForward:       return KeyCode::KC_DELETE;
        case kHIDUsage_KeyboardEnd:                 return KeyCode::KC_END;
        case kHIDUsage_KeyboardPageDown:            return KeyCode::KC_PAGE_DOWN;

        case kHIDUsage_KeyboardRightArrow:          return KeyCode::KC_RIGHT_ARROW;
        case kHIDUsage_KeyboardLeftArrow:           return KeyCode::KC_LEFT_ARROW;
        case kHIDUsage_KeyboardDownArrow:           return KeyCode::KC_DOWN_ARROW;
        case kHIDUsage_KeyboardUpArrow:             return KeyCode::KC_UP_ARROW;

        case kHIDUsage_KeypadNumLock:               return KeyCode::KC_NUM_PAD_NUMLOCK;
        case kHIDUsage_KeypadSlash:                 return KeyCode::KC_QUESTION_MARK;
        case kHIDUsage_KeypadAsterisk:              return KeyCode::KC_NUM_PAD_STAR;
        case kHIDUsage_KeypadHyphen:                return KeyCode::KC_MINUS;
        case kHIDUsage_KeypadPlus:                  return KeyCode::KC_NUM_PAD_PLUS;
        case kHIDUsage_KeypadEnter:                 return KeyCode::KC_ENTER;

        case kHIDUsage_Keypad1:                     return KeyCode::KC_NUM_PAD_1;
        case kHIDUsage_Keypad2:                     return KeyCode::KC_NUM_PAD_2;
        case kHIDUsage_Keypad3:                     return KeyCode::KC_NUM_PAD_3;
        case kHIDUsage_Keypad4:                     return KeyCode::KC_NUM_PAD_4;
        case kHIDUsage_Keypad5:                     return KeyCode::KC_NUM_PAD_5;
        case kHIDUsage_Keypad6:                     return KeyCode::KC_NUM_PAD_6;
        case kHIDUsage_Keypad7:                     return KeyCode::KC_NUM_PAD_7;
        case kHIDUsage_Keypad8:                     return KeyCode::KC_NUM_PAD_8;
        case kHIDUsage_Keypad9:                     return KeyCode::KC_NUM_PAD_9;
        case kHIDUsage_Keypad0:                     return KeyCode::KC_NUM_PAD_0;

        case kHIDUsage_KeypadPeriod:                return KeyCode::KC_NUM_PAD_DECIMAL;
        case kHIDUsage_KeypadEqualSign:             return KeyCode::KC_EQUALS;

        /*
        case kHIDUsage_KeyboardNonUSBackslash:      return KeyCode::KC_;
        case kHIDUsage_KeyboardApplication:         return KeyCode::KC_;
        case kHIDUsage_KeyboardPower:               return KeyCode::KC_;
        case kHIDUsage_KeyboardF13:                 return 0x69;
        case kHIDUsage_KeyboardF14:                 return 0x6b;
        case kHIDUsage_KeyboardF15:                 return 0x71;
        case kHIDUsage_KeyboardF16:                 return 0xff;
        case kHIDUsage_KeyboardF17:                 return 0xff;
        case kHIDUsage_KeyboardF18:                 return 0xff;
        case kHIDUsage_KeyboardF19:                 return 0xff;
        case kHIDUsage_KeyboardF20:                 return 0xff;
        case kHIDUsage_KeyboardF21:                 return 0xff;
        case kHIDUsage_KeyboardF22:                 return 0xff;
        case kHIDUsage_KeyboardF23:                 return 0xff;
        case kHIDUsage_KeyboardF24:                 return 0xff;

        case kHIDUsage_KeyboardExecute:             return 0xff;
        case kHIDUsage_KeyboardHelp:                return 0xff;
        case kHIDUsage_KeyboardMenu:                return 0x7F;
        case kHIDUsage_KeyboardSelect:              return 0x4c;
        case kHIDUsage_KeyboardStop:                return 0xff;
        case kHIDUsage_KeyboardAgain:               return 0xff;
        case kHIDUsage_KeyboardUndo:                return 0xff;
        case kHIDUsage_KeyboardCut:                 return 0xff;
        case kHIDUsage_KeyboardCopy:                return 0xff;
        case kHIDUsage_KeyboardPaste:               return 0xff;
        case kHIDUsage_KeyboardFind:                return 0xff;

        case kHIDUsage_KeyboardMute:                return 0xff;
        case kHIDUsage_KeyboardVolumeUp:            return 0xff;
        case kHIDUsage_KeyboardVolumeDown:          return 0xff;

        case kHIDUsage_KeyboardLockingCapsLock:     return 0xff;
        case kHIDUsage_KeyboardLockingNumLock:      return 0xff;
        case kHIDUsage_KeyboardLockingScrollLock:   return 0xff;

        case kHIDUsage_KeypadComma:                 return 0xff;
        case kHIDUsage_KeypadEqualSignAS400:        return 0xff;
        case kHIDUsage_KeyboardInternational1:      return 0xff;
        case kHIDUsage_KeyboardInternational2:      return 0xff;
        case kHIDUsage_KeyboardInternational3:      return 0xff;
        case kHIDUsage_KeyboardInternational4:      return 0xff;
        case kHIDUsage_KeyboardInternational5:      return 0xff;
        case kHIDUsage_KeyboardInternational6:      return 0xff;
        case kHIDUsage_KeyboardInternational7:      return 0xff;
        case kHIDUsage_KeyboardInternational8:      return 0xff;
        case kHIDUsage_KeyboardInternational9:      return 0xff;

        case kHIDUsage_KeyboardLANG1:               return 0xff;
        case kHIDUsage_KeyboardLANG2:               return 0xff;
        case kHIDUsage_KeyboardLANG3:               return 0xff;
        case kHIDUsage_KeyboardLANG4:               return 0xff;
        case kHIDUsage_KeyboardLANG5:               return 0xff;
        case kHIDUsage_KeyboardLANG6:               return 0xff;
        case kHIDUsage_KeyboardLANG7:               return 0xff;
        case kHIDUsage_KeyboardLANG8:               return 0xff;
        case kHIDUsage_KeyboardLANG9:               return 0xff;

        case kHIDUsage_KeyboardAlternateErase:      return 0xff;
        case kHIDUsage_KeyboardSysReqOrAttention:   return 0xff;
        case kHIDUsage_KeyboardCancel:              return 0xff;
        case kHIDUsage_KeyboardClear:               return 0xff;
        case kHIDUsage_KeyboardPrior:               return 0xff;
        case kHIDUsage_KeyboardReturn:              return 0xff;
        case kHIDUsage_KeyboardSeparator:           return 0xff;
        case kHIDUsage_KeyboardOut:                 return 0xff;
        case kHIDUsage_KeyboardOper:                return 0xff;
        case kHIDUsage_KeyboardClearOrAgain:        return 0xff;
        case kHIDUsage_KeyboardCrSelOrProps:        return 0xff;
        case kHIDUsage_KeyboardExSel:               return 0xff;*/

            /* 0xa5-0xdf Reserved */

        /*case kHIDUsage_KeyboardLeftControl:         return 0x3b;
        case kHIDUsage_KeyboardLeftShift:           return 0x38;
        case kHIDUsage_KeyboardLeftAlt:             return 0x3a;
        case kHIDUsage_KeyboardLeftGUI:             return 0x37;
        case kHIDUsage_KeyboardRightControl:        return 0x3e;
        case kHIDUsage_KeyboardRightShift:          return 0x3c;
        case kHIDUsage_KeyboardRightAlt:            return 0x3d;
        case kHIDUsage_KeyboardRightGUI:            return 0x36;*/

            /* 0xe8-0xffff Reserved */

        //case kHIDUsage_Keyboard_Reserved:           return 0xff;
        //default:                                    return 0xff;
    }
}

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
            
            event.scanCode = (int)ConvertUsageToKeyCode(scancode);
            
            event.eventType = OS_EVENT_KEY;
            event.keyEvent = pressed == 1 ? KEY_EVENT_DOWN : KEY_EVENT_UP;
        }
            break;
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
            break;
        }
    
        case kHIDPage_GenericDesktop:
        {
            CGEventRef dummyEvent = CGEventCreate(NULL);
            CGPoint mouseLocation = CGEventGetLocation(dummyEvent);
            CFRelease(dummyEvent);
            
            event.eventType = OS_EVENT_MOUSE;
            event.mouseEvent = MOUSE_EVENT_MOVE;
            event.x = mouseLocation.x;
            event.y = mouseLocation.y;
            
            switch (usage) {
                case kHIDUsage_GD_X:
                    event.deltaX = integerValue;
                    break;
                case kHIDUsage_GD_Y:
                    event.deltaY = integerValue;
                    break;
                case kHIDUsage_GD_Wheel:
                    // skip wheel events where the wheel didnt move
                    if(integerValue == 0)
                        return;
                    
                    event.mouseEvent = MOUSE_EVENT_SCROLL;
                    event.mouseButton = MOUSE_BUTTON_MIDDLE;
                    event.extendButtonInfo = integerValue;
                    break;
                default:
                    break;
            }
        }
            break;
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
    CGPoint point;
    CGEventRef event = CGEventCreate(NULL);
    point = CGEventGetLocation(event);
    CFRelease(event);
    xPos = point.x;
    yPos = point.y;
    
    return 0;
}

int GetNormalMousePosition(float xPos, float yPos)
{
    CGPoint point;
    CGEventRef event = CGEventCreate(NULL);
    point = CGEventGetLocation(event);
    CFRelease(event);
    
    
    
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
    
    return 0;
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
#if USE_RELATIVE_MOUSE_INPUT
    int eventPosX;
    int eventPosY;
    
    GetMousePosition(eventPosX, eventPosY);
#else
    int eventPosX = lastEventPosX;
    int eventPosY = lastEventPosY;
#endif
    bool isScrollEvent = false;
    
    switch (mouseEvent.mouseEvent)
    {
    case MOUSE_EVENT_MOVE:
#if USE_RELATIVE_MOUSE_INPUT
        eventPosX += mouseEvent.deltaX;
        eventPosY += mouseEvent.deltaY;
#else
        eventPosX = mouseEvent.x;
        eventPosY = mouseEvent.y;
#endif
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
    
    LOG_INFO << "Mouse X " << eventPosX << " Y " << eventPosY << std::endl;
    
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
    code = ConverKeyCodeToVirtualKey((KeyCode)keyEvent.scanCode);
    
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

int GetPointIsAtEdgeOfGlobalScreen(int x, int y, int xLimit, int yLimit, bool& result)
{
    int tlX = 0;
    int tlY = 0;
    int brX = 0;
    int brY = 0;
    
    bool first = true;
    
    NSArray<NSScreen*>* screens = [NSScreen screens];
    
    if(screens == nil || screens.count < 1)
        return -1;
    
    for (NSScreen* screen in screens) {
        int stlX = screen.frame.origin.x;
        int stlY = screen.frame.origin.y;
        int sbrX = stlX + screen.frame.size.width;
        int sbrY = stlY + screen.frame.size.height;
        
        if(first)
        {
            first = false;
            
            tlX = stlX;
            tlY = stlY;
            brX = sbrX;
            brY = sbrY;
        }
        else
        {
            tlX = MIN(tlX, stlX);
            tlY = MIN(tlY, stlY);
            brX = MAX(brX, sbrX);
            brY = MAX(brY, sbrY);
        }
    }
    
    result = x < (tlX + xLimit) || x > (brX - xLimit) || y < (tlY + yLimit) || y > (brY - yLimit);

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
