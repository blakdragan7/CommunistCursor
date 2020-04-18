#ifdef _WIN32
#include "../OSInterface.h"

#include <windows.h>
#include <functional>

OSInterface* osi = 0;
HHOOK mouseHook=0, keyboardHook=0;

static int VScreenXMin = 0;
static int VScreenXMax = 0;
static int VScreenYMin = 0;
static int VScreenYMax = 0;

LRESULT CALLBACK LowLevelMouseProc(_In_ int nCode,_In_ WPARAM wParam,_In_ LPARAM lParam)
{
    if (nCode == HC_ACTION)
	{
        OSEvent event;
        event.eventType = OS_EVENT_MOUSE;

        MSLLHOOKSTRUCT* msHook = (MSLLHOOKSTRUCT*)lParam;

        event.posX = msHook->pt.x;
        event.posY = msHook->pt.y;

        event.minX = VScreenXMin;
        event.minY = VScreenYMin;
        event.maxX = VScreenXMax;
        event.maxY = VScreenYMax;

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

extern int SendMouseEvent(const OSEvent mouseEvent)
{
    INPUT newInput = {0};
    int eventType = 0;

    newInput.type = INPUT_MOUSE;

    newInput.mi.dx = mouseEvent.posX;
    newInput.mi.dy = mouseEvent.posY;

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

    newInput.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK | eventType;

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
    static const int OS_SIZE_X = 65535;
    static const int OS_SIZE_Y = 65535;

    outEvent = OSEvent(inEvent);

    outEvent.posX = (int)((float)(inEvent.posX - inEvent.minX) / \
                    (float)(inEvent.maxX - inEvent.minX)) * OS_SIZE_X;
    outEvent.posY = (int)((float)(inEvent.posY - inEvent.minY) / \
                    (float)(inEvent.maxY - inEvent.minY)) * OS_SIZE_Y;

    outEvent.minX = 0;
    outEvent.minY = 0;
    outEvent.maxX = OS_SIZE_X;
    outEvent.maxY = OS_SIZE_Y;

    return 0;
}

int StoreScreenSize()
{
    VScreenXMin = GetSystemMetrics(SM_XVIRTUALSCREEN);
    VScreenYMin = GetSystemMetrics(SM_YVIRTUALSCREEN);
    VScreenXMax = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    VScreenYMax = GetSystemMetrics(SM_CYVIRTUALSCREEN);

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