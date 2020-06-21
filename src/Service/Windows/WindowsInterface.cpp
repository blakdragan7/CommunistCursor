#ifdef _WIN32
#include "../OSInterface/OSInterface.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <windows.h>
#include <functional>
#include <vector>

#define DEFAULT_ADAPTOR_ADDRESS_SIZE 15000;
#define MAX_ADAPTOR_FETCH_TRIES 3

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
            return OSInterfaceError::OS_E_SUCCESS;
        default:
            return OSInterfaceError::OS_E_UNKOWN;
    }
}

#endif