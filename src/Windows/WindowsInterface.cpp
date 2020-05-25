#ifdef _WIN32
#include "../OSInterface/OSInterface.h"

#include <windows.h>
#include <functional>
#include <vector>

OSInterface* osi = 0;
HHOOK mouseHook=0, keyboardHook=0;

POINT lastMousePoint = {0};

LRESULT CALLBACK LowLevelMouseProc(_In_ int nCode,_In_ WPARAM wParam,_In_ LPARAM lParam)
{
    if (nCode == HC_ACTION)
	{
        OSEvent event;
        event.eventType = OS_EVENT_MOUSE;

        MSLLHOOKSTRUCT* msHook = (MSLLHOOKSTRUCT*)lParam;

        event.deltaX = msHook->pt.x - lastMousePoint.x;
        event.deltaY = msHook->pt.y - lastMousePoint.y;

        lastMousePoint = msHook->pt;

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

void OSMainLoop(bool& shouldRun)
{
    MSG msg = { };
    while (GetMessage(&msg, 0, 0, 0) && shouldRun)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

BOOL Monitorenumproc(HMONITOR Arg1,HDC Arg2,LPRECT Arg3,LPARAM Arg4)
{
    std::vector<NativeDisplay>* outDisplays = static_cast<std::vector<NativeDisplay>*>((void*)Arg4);

    NativeDisplay display;

    display.posX = Arg3->left; 
    display.posY = Arg3->top;

    display.width = Arg3->right - Arg3->left;
    display.height = Arg3->bottom - Arg3->top; 

    display.nativeScreenID = -1;

    outDisplays->push_back(display);

    return TRUE;
}

int GetAllDisplays(std::vector<NativeDisplay>& outDisplays)
{
    if(EnumDisplayMonitors(GetDC(NULL), NULL, Monitorenumproc, (LPARAM)&outDisplays) == 0)
    {
        return GetLastError();
    }

    return 0;
}

int NativeRegisterForOSEvents(OSInterface* _osi)
{
    osi = _osi;

    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, 0, 0);
    if (mouseHook == 0)
    {
        return GetLastError();
    }

    if(GetCursorPos(&lastMousePoint) == FALSE)
    {
        DWORD error = GetLastError();
        UnhookWindowsHookEx(mouseHook);
        return error;
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

extern int SendMouseEvent(const OSEvent mouseEvent)
{
    INPUT newInput = {0};
    int eventType = 0;

    newInput.type = INPUT_MOUSE;

    newInput.mi.dx = mouseEvent.deltaX;
    newInput.mi.dy = mouseEvent.deltaY;

    switch(mouseEvent.subEvent.mouseEvent)
    {
    case MOUSE_EVENT_SCROLL:
        newInput.mi.mouseData = mouseEvent.extendButtonInfo * WHEEL_DELTA;
        eventType = MOUSEEVENTF_WHEEL;
        break;
    case MOUSE_EVENT_MOVE:
        eventType = MOUSEEVENTF_MOVE;
        break;
    case MOUSE_EVENT_DOWN:
        if(mouseEvent.eventButton.mouseButton == MOUSE_BUTTON_LEFT)
            eventType = MOUSEEVENTF_LEFTDOWN;
        else if(mouseEvent.eventButton.mouseButton == MOUSE_BUTTON_RIGHT)
            eventType = MOUSEEVENTF_RIGHTDOWN;
        else if(mouseEvent.eventButton.mouseButton == MOUSE_BUTTON_MIDDLE)
            eventType = MOUSEEVENTF_MIDDLEDOWN;
        break;
    case MOUSE_EVENT_UP:
        if(mouseEvent.eventButton.mouseButton == MOUSE_BUTTON_LEFT)
            eventType = MOUSEEVENTF_LEFTUP;
        else if(mouseEvent.eventButton.mouseButton == MOUSE_BUTTON_RIGHT)
            eventType = MOUSEEVENTF_RIGHTUP;
        else if(mouseEvent.eventButton.mouseButton == MOUSE_BUTTON_MIDDLE)
            eventType = MOUSEEVENTF_MIDDLEUP;
        break;
    }

    newInput.mi.dwFlags = eventType;

    if(SendInput(1, &newInput, sizeof(INPUT)) != 1)
        return GetLastError();
    
    return 0;
}

extern int SendKeyEvent(const OSEvent keyEvent)
{
    INPUT newInput = {0};
    int eventType = 0;

    newInput.type = INPUT_KEYBOARD;

    newInput.ki.wScan = keyEvent.eventButton.scanCode;

    switch(keyEvent.subEvent.keyEvent)
    {
    case KEY_EVENT_UP:
        eventType = KEYEVENTF_KEYUP;
        break;
    }

    newInput.mi.dwFlags = KEYEVENTF_SCANCODE | eventType;

    if(SendInput(1, &newInput, sizeof(INPUT)) != 1)
        return GetLastError();
    
    return 0;
}

int ConvertEventCoordsToNative(const OSEvent inEvent, OSEvent& outEvent)
{
    // no need on windows
    outEvent = inEvent;
    return 0;
}

OSInterfaceError OSErrorToOSInterfaceError(int OSError)
{
    switch(OSError)
    {
        case 0:
            return OS_E_SUCCESS;
        default:
            return OS_E_UNKOWN;
    }
}

#endif