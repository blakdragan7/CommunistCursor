#include "IOSEventReceiver.h"
#include "NativeInterface.h"
#include "PacketTypes.h"
#include "OSInterface.h"

#include <iostream>
#include <thread>

#define SLEEPM(a) std::this_thread::sleep_for(std::chrono::milliseconds(a));

OSInterface OSInterface::_sharedInterface;

OSInterface& OSInterface::SharedInterface()
{
    return _sharedInterface;
}

OSInterface::OSInterface() :_shouldRunMainloop(true), _hasHookedEvents(false)
{
    int ret = StartupOSConnection();
    if (ret != 0)
    {
        std::string errorString = "Could no Start Connection To OS " + std::to_string(ret);
        throw std::runtime_error(errorString);
    }
}

OSInterface::~OSInterface()
{
    int ret = ShutdownOSConnection();
    if (ret != 0)
    {
        std::cout << "Error Shuting down OS Connection error:" << ret << std::endl;
    }
}

OSInterfaceError OSInterface::ConvertEventToNativeCoords(const OSEvent inEvent, OSEvent& outEvent)
{
    int OSError = ::ConvertEventCoordsToNative(inEvent, outEvent);
    if(OSError != 0)
        return OSErrorToOSInterfaceError(OSError);
    return OSInterfaceError::OS_E_SUCCESS;
}

OSInterfaceError OSInterface::SetMouseHidden(bool hideMouse)
{
    int OSError = ::SetMouseHidden(hideMouse);
    if (OSError != 0)
        return OSErrorToOSInterfaceError(OSError);
    return OSInterfaceError::OS_E_SUCCESS;
}

OSInterfaceError OSInterface::SetMousePosition(int posX, int posY)
{
    int osError = ::SetMousePosition(posX, posY);
    if (osError != 0)
        return OSErrorToOSInterfaceError(osError);

    return OSInterfaceError::OS_E_SUCCESS;
}

OSInterfaceError OSInterface::SendOSEvent(const OSEvent osEvent)
{
    switch (osEvent.eventType)
    {
    case OS_EVENT_MOUSE:
        return SendMouseEvent(osEvent);
    case OS_EVENT_KEY:
        return SendKeyEvent(osEvent);
    case OS_EVENT_HID:
        return OSInterfaceError::OS_E_NOT_IMPLEMENTED;
    default:
        return OSInterfaceError::OS_E_INVALID_PARAM;
    }
}

OSInterfaceError OSInterface::SendMouseEvent(OSEvent mouseEvent)
{
    if(mouseEvent.eventType != OS_EVENT_MOUSE)
        return OSInterfaceError::OS_E_INVALID_PARAM;

    int OSError = ::SendMouseEvent(mouseEvent);

    if(OSError == 0)
        return OSInterfaceError::OS_E_SUCCESS;
    else
        return OSErrorToOSInterfaceError(OSError);
}

OSInterfaceError OSInterface::SendKeyEvent(OSEvent keyEvent)
{
    if(keyEvent.eventType != OS_EVENT_KEY)
        return OSInterfaceError::OS_E_INVALID_PARAM;

    int OSError = ::SendKeyEvent(keyEvent);

    if(OSError == 0)
        return OSInterfaceError::OS_E_SUCCESS;
    else
        return OSErrorToOSInterfaceError(OSError);
}

OSInterfaceError OSInterface::RegisterForOSEvents(IOSEventReceiver* newReceiver)
{
    if(_hasHookedEvents == false)
    {
        int res = NativeRegisterForOSEvents(this);
        if(res != 0)
        {
            std::cout << "Error hooking events with " << res << std::endl;
            return OSErrorToOSInterfaceError(res);
        }
            
        _hasHookedEvents = true;
    }

    std::lock_guard<std::mutex> lock(_mapAccessMutex);
    
    _eventReceivers.push_back(newReceiver);

    return OSInterfaceError::OS_E_SUCCESS;
}

OSInterfaceError OSInterface::UnRegisterForOSEvents(IOSEventReceiver* toRemove)
{
    std::lock_guard<std::mutex> lock(_mapAccessMutex);

    auto itr = std::find(_eventReceivers.begin(), _eventReceivers.end(), toRemove);

    if (itr != _eventReceivers.end())
    {
        _eventReceivers.erase(itr);
        return OSInterfaceError::OS_E_SUCCESS;
    }
    else
    {
        return OSInterfaceError::OS_E_NOT_REGISTERED;
    }
}

OSInterfaceError OSInterface::GetNativeDisplayList(std::vector<NativeDisplay>& displayList)
{
    int OSError = GetAllDisplays(displayList);
    if (OSError == 0)
        return OSInterfaceError::OS_E_SUCCESS;
    else
        return OSErrorToOSInterfaceError(OSError);
}

OSInterfaceError OSInterface::GetIPAddressList(std::vector<IPAdressInfo>& outAddresses, const IPAdressInfoHints hints)
{
    int OSError = ::GetIPAddressList(outAddresses, hints);
    if (OSError == 0)
        return OSInterfaceError::OS_E_SUCCESS;
    return OSErrorToOSInterfaceError(OSError);
}

OSInterfaceError OSInterface::GetClipboardData(ClipboardData& outData)
{
    int OSError = ::GetClipBoard(outData);
    if (OSError == 0)
        return OSInterfaceError::OS_E_SUCCESS;
    return OSErrorToOSInterfaceError(OSError);
}

OSInterfaceError OSInterface::SetClipboardData(const ClipboardData& inData)
{
    if (inData.type != ClipboardDataType::Text)
        return OSInterfaceError::OS_E_INVALID_PARAM;

    int OSError = ::SetClipBoard(inData);
    if (OSError == 0)
        return OSInterfaceError::OS_E_SUCCESS;
    return OSErrorToOSInterfaceError(OSError);
}

OSInterfaceError OSInterface::GetMousePosition(int& xPos, int& yPos)
{
    int ret = ::GetMousePosition(xPos, yPos);
    if(ret != 0)
        return OSErrorToOSInterfaceError(ret);

    return OSInterfaceError::OS_E_SUCCESS;
}

OSInterfaceError OSInterface::GetProcessExitCode(int processID, unsigned long* exitCode)
{
    int ret = ::GetProcessExitCode(processID, exitCode);
    if (ret != 0)
        return OSErrorToOSInterfaceError(ret);

    return OSInterfaceError::OS_E_SUCCESS;
}

OSInterfaceError OSInterface::GetIsProcessActive(int processID, bool* isActive)
{
    int ret = ::GetIsProcessActive(processID, isActive);
    if (ret != 0)
        return OSErrorToOSInterfaceError(ret);

    return OSInterfaceError::OS_E_SUCCESS;
}

OSInterfaceError OSInterface::StartProcessAsDesktopUser(std::string process, std::string args, std::string workingDir, bool isVisible, ProccessInfo* processInfo)
{
    int ret = ::StartProcessAsDesktopUser(process, args, workingDir, isVisible, processInfo);
    if (ret != 0)
        return OSErrorToOSInterfaceError(ret);

    return OSInterfaceError::OS_E_SUCCESS;
}

OSInterfaceError OSInterface::GetLocalHostName(std::string& hostName)
{
    int ret = GetHostName(hostName);
    if (ret != 0)
        return OSErrorToOSInterfaceError(ret);
    
    return OSInterfaceError::OS_E_SUCCESS;
}

void OSInterface::OSMainLoop()
{
    ::OSMainLoop(_shouldRunMainloop);
}

void OSInterface::StopMainLoop()
{
    // probably wrap this in mutex
    _shouldRunMainloop = false;
}

bool OSInterface::ConsumeInputEvent(OSEvent event)
{
    std::lock_guard<std::mutex> lock(_mapAccessMutex);

    bool shouldConsumeEvent = false;

    // If any of the receivers request it we consume the event (generally speaking there will only ever be one)

    for(IOSEventReceiver* receiver : _eventReceivers)
    {
        if (receiver->ReceivedNewInputEvent(event))
        {
            shouldConsumeEvent = true;
        }
    }

    return shouldConsumeEvent;
}

std::ostream& operator<<(std::ostream& os, const NativeDisplay& display)
{
    return os << "NativeID:"<<display.nativeScreenID << " pos{" << display.posX << "," << display.posY \
    << "} size{" << display.width << "," << display.height << "}";
}

std::ostream& operator<<(std::ostream& os, const OSEvent& event)
{
    switch(event.eventType)
    {
    case OS_EVENT_MOUSE:
        if (event.mouseEvent == MOUSE_EVENT_MOVE)
        {
            return os << "{ type: " << "[Mouse Move Event] " << "pos [" << event.x << "," << event.y << "]" << " delta [" << event.deltaX << "," << event.deltaY << "] }";
        }
        else if (event.mouseEvent == MOUSE_EVENT_DOWN || event.mouseEvent == MOUSE_EVENT_UP)
        {
            return os << "{ type: " << "[Mouse Button Event] isDown: [" << \
                ((event.mouseEvent == MOUSE_EVENT_DOWN) ? " True] " : "False] ") << "button: [" \
                << MouseButtonToString(event.mouseButton) << "] }";
        }
        else if (event.mouseEvent == MOUSE_EVENT_SCROLL)
        {
            return os << "{ type: " << "[Mouse Wheel Event] { wheelData: [" << event.extendButtonInfo << "] }";
        }
    case OS_EVENT_KEY:
        return os << "{" << "type:" << "[Key Event]" << " subType:" \
        << KeyEventTypeToString(event.keyEvent) << " scaneCode:" << event.scanCode; 
    case OS_EVENT_HID:
    case OS_EVENT_INVALID:
    default:
        return os << "Invalid Event";
    }
}

std::string OSEventTypeToString(OSEventType type)
{
    switch(type)
    {
    case OS_EVENT_MOUSE:
        return "OS_EVENT_MOUSE";
        break;
    case OS_EVENT_KEY:
        return "OS_EVENT_KEY";
        break;
    case OS_EVENT_HID:
        return "OS_EVENT_HID";
        break;
    case OS_EVENT_INVALID:
    default:
        return "OS_EVENT_INVALID";
        break;
    }
}
            
std::string MouseEventTypeToString(MouseEventType type)
{
    switch(type)
    {
    case MOUSE_EVENT_MOVE:
        return "MOUSE_EVENT_MOVE";
        break;
    case MOUSE_EVENT_DOWN:
        return "MOUSE_EVENT_DOWN";
        break;
    case MOUSE_EVENT_UP:
        return "MOUSE_EVENT_UP";
        break;
    case MOUSE_EVENT_SCROLL:
        return "MOUSE_EVENT_MOVE";
        break;
    case MOUSE_EVENT_INVALID:
    default:
        return "MOUSE_EVENT_INVALID";
        break;
    }
}
            
std::string KeyEventTypeToString(KeyEventType type)
{
    switch(type)
    {
    case KEY_EVENT_DOWN:
        return "KEY_EVENT_DOWN";
        break;
    case KEY_EVENT_UP:
        return "KEY_EVENT_UP";
        break;
    case KEY_EVENT_INVALID:
    default:
        return "KEY_EVENT_INVALID";
        break;
    }
}

std::string MouseButtonToString(MouseButton button)
{
    switch(button)
    {
    case MOUSE_BUTTON_LEFT:
        return "MOUSE_BUTTON_LEFT";
        break;
    case MOUSE_BUTTON_RIGHT:
        return "MOUSE_BUTTON_RIGHT";
        break;
    case MOUSE_BUTTON_MIDDLE:
        return "MOUSE_BUTTON_MIDDLE";
        break;
    case MOUSE_BUTTON_EXTENDED:
        return "MOUSE_BUTTON_EXTENDED";
        break;
    case MOUSE_BUTTON_INVALID:
    default:
        return "MOUSE_BUTTON_INVALID";
        break;
    }
}
