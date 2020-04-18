#ifdef _WIN32
#include "../OSInterface.h"

#include <windows.h>
#include <functional>

OSInterface* osi = 0;
HHOOK mouseHook=0, keyboardHook=0;

LRESULT CALLBACK LowLevelMouseProc(_In_ int nCode,_In_ WPARAM wParam,_In_ LPARAM lParam)
{
    if (nCode == HC_ACTION)
	{
        OSEvent event;
        event.eventType = OS_EVENT_MOUSE;

        MSLLHOOKSTRUCT* msHook = (MSLLHOOKSTRUCT*)lParam;

        event.posX = msHook->pt.x;
        event.posY = msHook->pt.y;

        switch (wParam) {
		case WM_LBUTTONDOWN:
			event.eventButton.mouseButton = MOUSE_BUTTON_LEFT;
            event.subEvent.mouseEvent = MOUSE_EVENT_DOWN;
            break;
		case WM_LBUTTONUP:
            event.eventButton.mouseButton = MOUSE_BUTTON_LEFT;
            event.subEvent.mouseEvent = MOUSE_EVENT_UP;
			break;
		case WM_MOUSEMOVE:
            event.subEvent.mouseEvent = MOUSE_EVENT_MOVE;
			break;
		case WM_RBUTTONDOWN:
            event.eventButton.mouseButton = MOUSE_BUTTON_RIGHT;
            event.subEvent.mouseEvent = MOUSE_EVENT_DOWN;
			break;
		case WM_RBUTTONUP:
            event.eventButton.mouseButton = MOUSE_BUTTON_RIGHT;
            event.subEvent.mouseEvent = MOUSE_EVENT_UP;
			break;
        case WM_MOUSEWHEEL:
            event.subEvent.mouseEvent = MOUSE_EVENT_SCROLL;
            event.eventButton.mouseButton = MOUSE_BUTTON_MIDDLE;
            event.extendButtonInfo = GET_WHEEL_DELTA_WPARAM(msHook->mouseData) / WHEEL_DELTA;
            break;
        case WM_MBUTTONDOWN:
            event.subEvent.mouseEvent = MOUSE_EVENT_DOWN;
            event.eventButton.mouseButton = MOUSE_BUTTON_MIDDLE;
            break;
        case WM_MBUTTONUP:
            event.subEvent.mouseEvent = MOUSE_EVENT_UP;
            event.eventButton.mouseButton = MOUSE_BUTTON_MIDDLE;
            break;
        }
        osi->UpdateThread(event);
    }
    return CallNextHookEx(0, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(_In_ int nCode,_In_ WPARAM wParam,_In_ LPARAM lParam)
{
    if (nCode == HC_ACTION)
	{
        OSEvent event;
        event.eventType = OS_EVENT_KEY;

        KBDLLHOOKSTRUCT* msHook = (KBDLLHOOKSTRUCT*)lParam;
        event.eventButton.scanCode = msHook->scanCode;

        switch(wParam)
        {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            event.subEvent.keyEvent = KEY_EVENT_DOWN;
            break;
            case WM_KEYUP:
            case WM_SYSKEYUP:
            event.subEvent.keyEvent = KEY_EVENT_UP;
            break;
        }

        osi->UpdateThread(event);
    }
    
    return CallNextHookEx(0, nCode, wParam, lParam);
}

void OSMainLoop(bool& stopSwitch)
{
    MSG msg = { };
    while (GetMessage(&msg, 0, 0, 0) && stopSwitch == false)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int NativeRegisterForOSEvents(OSInterface* _osi)
{
    osi = _osi;

    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, 0, 0);
    if (mouseHook == 0)
    {
        return GetLastError();
    }

    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
    if(keyboardHook == 0)
    {
        UnhookWindowsHookEx(mouseHook);
        return GetLastError();
    }

    return GetLastError();
}

void NativeUnhookAllEvents()
{
    if(mouseHook)UnhookWindowsHookEx(mouseHook);
    if(keyboardHook)UnhookWindowsHookEx(keyboardHook);

    mouseHook = 0;
    keyboardHook = 0;
}

#endif