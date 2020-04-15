#ifdef _WIN32
#include "../OSInterface.h"

#include <windows.h>
#include <functional>

OSInterface* osi = 0;
HHOOK mouseHook=0, keyboardHook=0;

LRESULT CALLBACK LowLevelMouseProc(_In_ int nCode,_In_ WPARAM wParam,_In_ LPARAM lParam)
{
	OSEvent event;
    event.eventType = OS_EVENT_MOUSE;

    osi->UpdateThread(event);
    
    return CallNextHookEx(0, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(_In_ int nCode,_In_ WPARAM wParam,_In_ LPARAM lParam)
{
	OSEvent event;
    event.eventType = OS_EVENT_KEY;

    osi->UpdateThread(event);
    
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