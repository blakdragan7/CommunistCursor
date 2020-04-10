#include "OSInterface.h"
#include <map>
#include <mutex>
#include <iostream>

bool hasHookedEvents = false;
int HookLowLevelMouseEvents();

#define SLEEPM(a) std::this_thread::sleep_for(std::chrono::milliseconds(a));

static std::map<void*, MouseEventCallback> MouseEventRegisterdCallbacks;
typedef std::pair<void*, MouseEventCallback> MouseEventEntry;
static std::mutex MapAccessMutex;

OSInterfaceError OSIetMousePosition(int x, int y)
{
    return OS_E_SUCCESS;
}

OSInterfaceError OSIGetMousePosition(int *x, int *y)
{
    return OS_E_SUCCESS;
}

OSInterfaceError OSIRegisterForMouseEvents(MouseEventCallback callback, void* userInfo)
{
    if(hasHookedEvents == false)
    {
        hasHookedEvents = true;
        int res = HookLowLevelMouseEvents();
        if(res != 0)
        {
            std::cout << "Error hooking events with " << res << std::endl;
        }
            
    }

    while(MapAccessMutex.try_lock() == false)SLEEPM(1);

    if(MouseEventRegisterdCallbacks.count(userInfo) > 0)
    {
        return OS_E_ALREADY_REGISTERED;
    }
    else
    {
        MouseEventRegisterdCallbacks.insert(MouseEventEntry(userInfo, callback));
        return OS_E_SUCCESS;
    }

    MapAccessMutex.unlock();
}

OSInterfaceError OSIUnRegisterForMouseEvents(void* userInfo)
{
    while(MapAccessMutex.try_lock() == false)SLEEPM(1);

    if(MouseEventRegisterdCallbacks.count(userInfo) > 0)
    {
        MouseEventRegisterdCallbacks.erase(userInfo);
        return OS_E_SUCCESS;
    }
    else
    {
        return OS_E_NOT_REGISTERED;
    }

    MapAccessMutex.unlock();
}

void event_dispatch_thread(MouseEvent event)
{
    while(MapAccessMutex.try_lock() == false)SLEEPM(1);

    for(MouseEventEntry entry : MouseEventRegisterdCallbacks)
    {
        entry.second(event, entry.first);
    }

    MapAccessMutex.unlock();
}

#ifdef _WIN32
#include <windows.h>

HWND hiddenWindow;

void WindowsEventThread()
{
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INPUT)
	{
		MSLLHOOKSTRUCT* hk = (MSLLHOOKSTRUCT*)lParam;

		MouseEvent event;

		event.posX = hk->pt.x;
		event.posY = hk->pt.y;

		switch (wParam) {
		case WM_LBUTTONDOWN:
			event.eventType = MOUSE_ET_DOWN;
            event.eventButton = MOUSE_BUTTON_LEFT;
			break;
		case WM_LBUTTONUP:
            event.eventType = MOUSE_ET_UP;
            event.eventButton = MOUSE_BUTTON_LEFT;
			break;
		case WM_MOUSEMOVE:
            event.eventType = MOUSE_ET_MOVE;
			break;
		case WM_RBUTTONDOWN:
            event.eventType = MOUSE_ET_DOWN;
            event.eventButton = MOUSE_BUTTON_RIGHT;
			break;
		case WM_RBUTTONUP:
			event.eventType = MOUSE_ET_UP;
            event.eventButton = MOUSE_BUTTON_RIGHT;
			break;
        case WM_MOUSEWHEEL:
            event.eventType = MOUSE_ET_SCROLL;
            event.eventButton = MOUSE_BUTTON_MIDDLE;
            event.extendButtonInfo = hk->mouseData;
		}
	
        std::thread dispatch(event_dispatch_thread, event);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int HookLowLevelMouseEvents()
{
    WNDCLASS wc = { };

    HINSTANCE hInstance = GetModuleHandle(NULL);

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = "HiddenEventWindowClass";

    RegisterClass(&wc);

    hiddenWindow = CreateWindowEx(0, "HiddenEventWindowClass", "Hidden Event Window",WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL,NULL,hInstance,NULL);

    std::thread windowThread(WindowsEventThread);

    return GetLastError();
}
#endif