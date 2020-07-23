#ifdef _WIN32
#include "../OSInterface/OSInterface.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <mutex>

#include <WtsApi32.h>

#include <windows.h>
#include <functional>
#include <vector>

#define DEFAULT_ADAPTOR_ADDRESS_SIZE 15000;
#define MAX_ADAPTOR_FETCH_TRIES 3

#define WINDOWS_NORM_X_MAX 65535
#define WINDOWS_NORM_Y_MAX 65535

#define WINDOW_CLASS_NAME "CC Window Class"

OSInterface* osi = 0;
HHOOK mouseHook=0, keyboardHook=0;


std::mutex inputMutex;

POINT lastMousePoint = {0};

HWND windowHandle = (HWND)INVALID_HANDLE_VALUE;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int StartupOSConnection()
{
    if (windowHandle != (HWND)INVALID_HANDLE_VALUE)
        return 0;

    WNDCLASSEX cls = {0};
    cls.cbSize = sizeof(cls);
    cls.hInstance = GetModuleHandle(NULL);
    cls.lpszClassName = WINDOW_CLASS_NAME;
    cls.lpfnWndProc = MainWndProc;

    if (RegisterClassEx(&cls) == false)
        return GetLastError();

    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (width == 0 || height == 0)
        return -1;

    windowHandle = CreateWindow(WINDOW_CLASS_NAME, "CC Window", WS_OVERLAPPEDWINDOW, 0, \
        0, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (windowHandle == (HWND)INVALID_HANDLE_VALUE)
        return GetLastError();
    
    UpdateWindow(windowHandle);

    return 0;
}

int ShutdownOSConnection()
{
    if (windowHandle == (HWND)INVALID_HANDLE_VALUE)
        return 0;

    if (UnregisterClass(WINDOW_CLASS_NAME, GetModuleHandle(NULL)) == FALSE)
        return GetLastError();

    if (CloseWindow(windowHandle) == false)
        return GetLastError();

    return 0;
}

LRESULT CALLBACK LowLevelMouseProc(_In_ int nCode,_In_ WPARAM wParam,_In_ LPARAM lParam)
{
    if (nCode == HC_ACTION && osi)
	{
        OSEvent event;
        event.eventType = OS_EVENT_MOUSE;

        MSLLHOOKSTRUCT* msHook = (MSLLHOOKSTRUCT*)lParam;

        event.deltaX = msHook->pt.x - lastMousePoint.x;
        event.deltaY = msHook->pt.y - lastMousePoint.y;
        event.x = msHook->pt.x;
        event.y = msHook->pt.y;

        lastMousePoint = msHook->pt;

        switch (wParam) {
		case WM_LBUTTONDOWN:
			event.mouseButton = MOUSE_BUTTON_LEFT;
            event.mouseEvent = MOUSE_EVENT_DOWN;
            break;
		case WM_LBUTTONUP:
            event.mouseButton = MOUSE_BUTTON_LEFT;
            event.mouseEvent = MOUSE_EVENT_UP;
			break;
		case WM_MOUSEMOVE:
            event.mouseEvent = MOUSE_EVENT_MOVE;
			break;
		case WM_RBUTTONDOWN:
            event.mouseButton = MOUSE_BUTTON_RIGHT;
            event.mouseEvent = MOUSE_EVENT_DOWN;
			break;
		case WM_RBUTTONUP:
            event.mouseButton = MOUSE_BUTTON_RIGHT;
            event.mouseEvent = MOUSE_EVENT_UP;
			break;
        case WM_MOUSEWHEEL:
            event.mouseEvent = MOUSE_EVENT_SCROLL;
            event.mouseButton = MOUSE_BUTTON_MIDDLE;
            event.extendButtonInfo = GET_WHEEL_DELTA_WPARAM(msHook->mouseData) / WHEEL_DELTA;
            break;
        case WM_MBUTTONDOWN:
            event.mouseEvent = MOUSE_EVENT_DOWN;
            event.mouseButton = MOUSE_BUTTON_MIDDLE;
            break;
        case WM_MBUTTONUP:
            event.mouseEvent = MOUSE_EVENT_UP;
            event.mouseButton = MOUSE_BUTTON_MIDDLE;
            break;
        default:
            return CallNextHookEx(0, nCode, wParam, lParam);
        }

        // if OSI requests it we consume the event and stop it here

        if (osi->ConsumeInputEvent(event))
        {
            return 1;
        }
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
        event.scanCode = msHook->scanCode;

        switch(wParam)
        {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            event.keyEvent = KEY_EVENT_DOWN;
            break;
            case WM_KEYUP:
            case WM_SYSKEYUP:
            event.keyEvent = KEY_EVENT_UP;
            break;
        }

        // if OSI requests it we consume the event and stop it here

        if (osi->ConsumeInputEvent(event))
        {
            return 1;
        }
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

int GetIPAddressList(std::vector<IPAdressInfo>& outAddresses, const IPAdressInfoHints& hints)
{
    ULONG family = 0;
    ULONG flags = GAA_FLAG_SKIP_DNS_SERVER;

    switch (hints.familly)
    {
    case IPAddressFamilly::IPv4:
        family = AF_INET;
        break;
    case IPAddressFamilly::IPv6:
        family = AF_INET6;
        break;
    case IPAddressFamilly::ALL:
    default:
        family = AF_UNSPEC;
    }

    if ((hints.type & IPAddressType::ANYCAST) == IPAddressType::NONE)
    {
        flags |= GAA_FLAG_SKIP_ANYCAST;
    }
    if ((hints.type & IPAddressType::MULTICAST) == IPAddressType::NONE)
    {
        flags |= GAA_FLAG_SKIP_MULTICAST;
    }
    if ((hints.type & IPAddressType::UNICAST) == IPAddressType::NONE)
    {
        flags |= GAA_FLAG_SKIP_UNICAST;
    }

    ULONG adaptorAddressSize = DEFAULT_ADAPTOR_ADDRESS_SIZE;
    IP_ADAPTER_ADDRESSES* addresses = 0;

    int iterations = 0;
    ULONG ret = 0;
    do
    {
        addresses = (IP_ADAPTER_ADDRESSES*)malloc(adaptorAddressSize);

        ret = GetAdaptersAddresses(family, flags, NULL, addresses, &adaptorAddressSize);
        if (ret == ERROR_BUFFER_OVERFLOW)
        {
            free(addresses);
            addresses = NULL;
            iterations++;
        }
        else break;

    } while (ret == ERROR_BUFFER_OVERFLOW && (iterations < MAX_ADAPTOR_FETCH_TRIES));

    if (ret != NO_ERROR)
    {
        return ret;
    }

    IP_ADAPTER_ADDRESSES* currentAddress = addresses;

    while (currentAddress)
    {
        if(currentAddress->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
        {
            currentAddress = currentAddress->Next;
            continue;
        }

        IP_ADAPTER_UNICAST_ADDRESS* currentUnicast = currentAddress->FirstUnicastAddress;
        while (currentUnicast)
        {
            IPAdressInfo info;
            info.adaptorName = currentAddress->AdapterName;
            info.addressType = IPAddressType::UNICAST;
            info.addressFamilly = (currentUnicast->Address.iSockaddrLength == sizeof(sockaddr_in)) ? IPAddressFamilly::IPv4 : IPAddressFamilly::IPv6;
            
            char buff[43] = { 0 };
            DWORD buffSize = sizeof(buff);
            ret = WSAAddressToString(currentUnicast->Address.lpSockaddr, currentUnicast->Address.iSockaddrLength, NULL,
                buff, &buffSize);

            if (ret == SOCKET_ERROR)
            {
                std::cout << "Error Converting Address To String " << WSAGetLastError();
                continue;
            }

            info.address = buff;

            ULONG MaskInt = 0;
            ret = ConvertLengthToIpv4Mask(currentUnicast->OnLinkPrefixLength, &MaskInt);

            if (ret == NO_ERROR)
            {
                char buff[16] = { 0 };
                inet_ntop(AF_INET, &MaskInt, buff, 16);

                info.subnetMask = buff;
            }
            else
            {
                // we failed to get subnet but that isn't critical so we just print an error and continue anyway

                std::cout << "Error Getting SubnetMask from OnLinkPrefixLength: " << ret << std::endl;
            }

            outAddresses.push_back(info);

            currentUnicast = currentUnicast->Next;
        }
        IP_ADAPTER_ANYCAST_ADDRESS* currentAnycast = currentAddress->FirstAnycastAddress;
        while (currentAnycast)
        {
            IPAdressInfo info;
            info.adaptorName = currentAddress->AdapterName;
            info.addressType = IPAddressType::ANYCAST;
            info.addressFamilly = (currentAnycast->Address.iSockaddrLength == sizeof(sockaddr_in)) ? IPAddressFamilly::IPv4 : IPAddressFamilly::IPv6;

            char buff[43] = { 0 };
            DWORD buffSize = sizeof(buff);
            ret = WSAAddressToString(currentAnycast->Address.lpSockaddr, currentAnycast->Address.iSockaddrLength, NULL,
                buff, &buffSize);

            if (ret == SOCKET_ERROR)
            {
                std::cout << "Error Converting Address To String " << WSAGetLastError();
                continue;
            }

            info.address = buff;

            outAddresses.push_back(info);

            currentAnycast = currentAnycast->Next;
        }

        IP_ADAPTER_MULTICAST_ADDRESS* currentMulticast = currentAddress->FirstMulticastAddress;
        while (currentMulticast)
        {
            IPAdressInfo info;
            info.adaptorName = currentAddress->AdapterName;
            info.addressType = IPAddressType::MULTICAST;
            info.addressFamilly = (currentMulticast->Address.iSockaddrLength == sizeof(sockaddr_in)) ? IPAddressFamilly::IPv4 : IPAddressFamilly::IPv6;

            char buff[43] = { 0 };
            DWORD buffSize = sizeof(buff);
            ret = WSAAddressToString(currentMulticast->Address.lpSockaddr, currentMulticast->Address.iSockaddrLength, NULL,
                buff, &buffSize);

            if (ret == SOCKET_ERROR)
            {
                std::cout << "Error Converting Address To String " << WSAGetLastError();
                continue;
            }

            info.address = buff;

            outAddresses.push_back(info);

            currentMulticast = currentMulticast->Next;
        }

        IP_ADAPTER_DNS_SERVER_ADDRESS* currentDNSServer = currentAddress->FirstDnsServerAddress;
        while (currentDNSServer)
        {
            IPAdressInfo info;
            info.adaptorName = currentAddress->AdapterName;
            info.addressType = IPAddressType::MULTICAST;
            info.addressFamilly = (currentDNSServer->Address.iSockaddrLength == sizeof(sockaddr_in)) ? IPAddressFamilly::IPv4 : IPAddressFamilly::IPv6;

            char buff[43] = { 0 };
            DWORD buffSize = sizeof(buff);
            ret = WSAAddressToString(currentDNSServer->Address.lpSockaddr, currentDNSServer->Address.iSockaddrLength, NULL,
                buff, &buffSize);

            if (ret == SOCKET_ERROR)
            {
                std::cout << "Error Converting Address To String " << WSAGetLastError();
                continue;
            }

            info.address = buff;

            outAddresses.push_back(info);

            currentDNSServer = currentDNSServer->Next;
        }

        currentAddress = currentAddress->Next;
    }

    if (addresses)free(addresses);

    return 0;
}

int GetClipBoard(ClipboardData& outData)
{
    if (windowHandle == INVALID_HANDLE_VALUE)
        return -1;

    size_t tries = 3;
    while (OpenClipboard(windowHandle) == FALSE)
    {
        if (--tries <= 0)
        {
            return GetLastError();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    HANDLE cHandle = GetClipboardData(CF_TEXT);

    if (cHandle == NULL)
    {
        CloseClipboard();
        return GetLastError();
    }

    LPVOID data = GlobalLock(cHandle);

    outData.stringData = (char*)data;
    outData.type = ClipboardDataType::Text;

    GlobalUnlock(cHandle);

    CloseClipboard();

    return 0;
}

int SetClipBoard(const ClipboardData& data)
{
    if (windowHandle == INVALID_HANDLE_VALUE)
        return -1;

    size_t tries = 3;
    while (OpenClipboard(windowHandle) == FALSE)
    {
        if (--tries <= 0)
        {
            return GetLastError();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    EmptyClipboard();

    size_t cSize = data.stringData.size();
    HGLOBAL cData = GlobalAlloc(GMEM_MOVEABLE, cSize + 1);

    if (cData == NULL)
    {
        CloseClipboard();
        return GetLastError();
    }

    LPVOID gdata = GlobalLock(cData);
    memcpy(gdata, data.stringData.c_str(), cSize);

    ((char*)gdata)[cSize] = 0;

    GlobalUnlock(cData);

    if (SetClipboardData(CF_TEXT, cData) == NULL)
    {
        CloseClipboard();
        return GetLastError();
    }

    CloseClipboard();

    return 0;
}

int GetProcessExitCode(int processID, unsigned long* exitCode)
{
    HANDLE processHandle = OpenProcess(SYNCHRONIZE, FALSE, processID);

    if (processHandle == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }

    if (GetExitCodeProcess(processHandle, (LPDWORD)exitCode) == false)
    {
        CloseHandle(processHandle);
        return GetLastError();
    }

    CloseHandle(processHandle);
    return 0;
}

int GetIsProcessActive(int processID, bool* isActive)
{
    HANDLE processHandle = OpenProcess(SYNCHRONIZE, FALSE, processID);

    if (processHandle == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }

    DWORD exitCode = 0;
    if (GetExitCodeProcess(processHandle, &exitCode) == false)
    {
        CloseHandle(processHandle);
        return GetLastError();
    }

    CloseHandle(processHandle);

    *isActive = (exitCode == STILL_ACTIVE);

    return 0;
}

int StartProcessAsDesktopUser(std::string process, std::string args, std::string workingDir, bool isVisible, ProccessInfo* processInfo)
{
    HANDLE dTop = GetDesktopWindow();

    char* cmd = NULL;
    if (args.size() > 0)
    {
        cmd = new char[args.size() + 10];
        memset(cmd, 0, args.size() + 10);
        strcpy(cmd, args.c_str());
    }
    
    STARTUPINFO info;
    memset(&info, 0, sizeof(info));
    info.lpDesktop = "winsta0\\default";
    info.wShowWindow = (short)(isVisible ? SW_SHOW : SW_HIDE);
    info.cb = sizeof(info);

    PROCESS_INFORMATION pInfo = { 0 };

    unsigned int flags = CREATE_UNICODE_ENVIRONMENT | (unsigned int)(isVisible ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW);;

    const char* processStr = process.size() > 0 ? process.c_str() : NULL;
    const char* wrkStr = workingDir.size() > 0 ? workingDir.c_str() : NULL;
    // we are a desktop user

    if(dTop)
    {
        if(CreateProcess(processStr, cmd, NULL, NULL, false, flags, NULL, wrkStr, &info, &pInfo) == false)
            return GetLastError();
    }
    else // we are not a desktop user
    {
        HANDLE desktopUser = INVALID_HANDLE_VALUE;
        if (WTSQueryUserToken(WTSGetActiveConsoleSessionId(), &desktopUser) == false)
        {
            return GetLastError();
        }

        if (CreateProcessAsUser(desktopUser, process.size() > 0 ? process.c_str() : NULL, cmd, NULL, NULL, false, flags, NULL, workingDir.c_str(), &info, &pInfo) == false)
            return GetLastError();

        CloseHandle(desktopUser);
    }

    CloseHandle(pInfo.hThread);

    (*processInfo).processID = pInfo.dwProcessId;
    (*processInfo).nativeHandle = pInfo.hProcess;
    (*processInfo).processName = process;

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

int GetHostName(std::string& hostName)
{
    WSADATA wsaData;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != ERROR_SUCCESS)
    {
        return ret;
    }

    char buff[256] = {0};
    int len = sizeof(buff);

    ret = gethostname(buff, len);

    if (ret != ERROR_SUCCESS)
    {
        return ret;
    }

    WSACleanup();

    hostName = buff;

    return 0;
}

int SetMousePosition(int x, int y)
{
    INPUT newInput;
    memset(&newInput, 0, sizeof(INPUT));
    int eventType = 0;

    newInput.type = INPUT_MOUSE;

    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    newInput.mi.dx = (LONG)(((float)x / (float)width) * 65535);
    newInput.mi.dy = (LONG)(((float)y / (float)height) * 65535);

    newInput.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;

    {
        std::lock_guard<std::mutex> lock(inputMutex);
        if (SendInput(1, &newInput, sizeof(INPUT)) != 1)
            return GetLastError();
    }
    return 0;
}

int GetMousePosition(int& xPos, int& yPos)
{
    POINT p;
    if (GetCursorPos(&p))
    {
        xPos = p.x;
        yPos = p.y;

        return 0;
    }
    else
        return GetLastError();
}

void NativeUnhookAllEvents()
{
    if(mouseHook)UnhookWindowsHookEx(mouseHook);
    if(keyboardHook)UnhookWindowsHookEx(keyboardHook);

    mouseHook = 0;
    keyboardHook = 0;
}

int SetMouseHidden(bool isHidden)
{
    // right now does nothing
    return 0;
}

int SendMouseEvent(const OSEvent mouseEvent)
{
    if (mouseEvent.mouseEvent == MOUSE_EVENT_MOVE)
    {
        return SetMousePosition(mouseEvent.x, mouseEvent.y);
    }

    INPUT newInput;
    memset(&newInput, 0, sizeof(INPUT));

    int eventType = 0;

    newInput.type = INPUT_MOUSE;

    switch(mouseEvent.mouseEvent)
    {
    case MOUSE_EVENT_SCROLL:
        newInput.mi.mouseData = mouseEvent.extendButtonInfo * WHEEL_DELTA;
        eventType = MOUSEEVENTF_WHEEL;
        break;
    case MOUSE_EVENT_DOWN:
        if(mouseEvent.mouseButton == MOUSE_BUTTON_LEFT)
            eventType = MOUSEEVENTF_LEFTDOWN;
        else if(mouseEvent.mouseButton == MOUSE_BUTTON_RIGHT)
            eventType = MOUSEEVENTF_RIGHTDOWN;
        else if(mouseEvent.mouseButton == MOUSE_BUTTON_MIDDLE)
            eventType = MOUSEEVENTF_MIDDLEDOWN;
        break;
    case MOUSE_EVENT_UP:
        if(mouseEvent.mouseButton == MOUSE_BUTTON_LEFT)
            eventType = MOUSEEVENTF_LEFTUP;
        else if(mouseEvent.mouseButton == MOUSE_BUTTON_RIGHT)
            eventType = MOUSEEVENTF_RIGHTUP;
        else if(mouseEvent.mouseButton == MOUSE_BUTTON_MIDDLE)
            eventType = MOUSEEVENTF_MIDDLEUP;
        break;
    }

    newInput.mi.dwFlags = eventType;

    {
        std::lock_guard<std::mutex> lock(inputMutex);
        if (SendInput(1, &newInput, sizeof(INPUT)) != 1)
            return GetLastError();
    }
    return 0;
}

int SendKeyEvent(const OSEvent keyEvent)
{
    INPUT newInput;
    memset(&newInput, 0, sizeof(INPUT));

    newInput.type = INPUT_KEYBOARD;
    newInput.ki.wScan = keyEvent.scanCode;

    switch(keyEvent.keyEvent)
    {
    case KEY_EVENT_UP:
        newInput.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        break;
    case KEY_EVENT_DOWN:
        newInput.ki.dwFlags = KEYEVENTF_SCANCODE;
        break;
    }

    {
        std::lock_guard<std::mutex> lock(inputMutex);
        if (SendInput(1, &newInput, sizeof(INPUT)) != 1)
            return GetLastError();
    }
    
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
            return OSInterfaceError::OS_E_SUCCESS;
        default:
            return OSInterfaceError::OS_E_UNKOWN;
    }
}

#endif