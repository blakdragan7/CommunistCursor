#include "CCNetworkEntity.h"

#include "../Socket/Socket.h"
#include "../OSInterface/OSTypes.h"
#include "../OSInterface/PacketTypes.h"
#include "CCDisplay.h"

SocketError CCNetworkEntity::SendKeyEventPacket(const OSEvent& event)const
{
    KeyEventPacket packet(event);
    EventPacketHeader header(EVENT_PACKET_K);

    SocketError ret = _internalSocket->Send(&header, sizeof(header));
    if(ret != SocketError::SOCKET_E_SUCCESS)
        return ret;
    return _internalSocket->Send(&packet, sizeof(packet));
}

SocketError CCNetworkEntity::SendMouseEventPacket(const OSEvent& event)const
{
    switch(event.subEvent.mouseEvent)
    {
        case MOUSE_EVENT_DOWN:
        case MOUSE_EVENT_UP:
            {
                EventPacketHeader header(EVENT_PACKET_MB);
                MouseButtonEventPacket packet(event);

                SocketError ret = _internalSocket->Send(&header, sizeof(header));
                if(ret != SocketError::SOCKET_E_SUCCESS)
                    return ret;
                return _internalSocket->Send(&packet, sizeof(packet));
            }
            break;
        case MOUSE_EVENT_SCROLL:
            {
                EventPacketHeader header(EVENT_PACKET_MW);
                MouseWheelEventPacket packet(event);

                SocketError ret = _internalSocket->Send(&header, sizeof(header));
                if(ret != SocketError::SOCKET_E_SUCCESS)
                    return ret;
                return _internalSocket->Send(&packet, sizeof(packet));
            }
            break;
        case MOUSE_EVENT_MOVE:
            {
                EventPacketHeader header(EVENT_PACKET_MM);
                MouseMoveEventPacket packet(event);

                SocketError ret = _internalSocket->Send(&header, sizeof(header));
                if(ret != SocketError::SOCKET_E_SUCCESS)
                    return ret;
                return _internalSocket->Send(&packet, sizeof(packet));
            }
            break;
        default:
            return SocketError::SOCKET_E_INVALID_PARAM;
    }
}

SocketError CCNetworkEntity::SendHIDEventPacket(const OSEvent& event)const
{
    // not implemented yet
    return SocketError::SOCKET_E_NOT_IMPLEMENTED;
}

CCNetworkEntity::CCNetworkEntity(Socket* socket) : _internalSocket(socket)
{}

SocketError CCNetworkEntity::Send(const char* buff, const size_t size)const
{
    return _internalSocket->Send(buff, size);
}

SocketError CCNetworkEntity::Send(const std::string toSend)const
{
    return _internalSocket->Send(toSend);
}

SocketError CCNetworkEntity::SendOSEvent(const OSEvent& event)const
{
    switch(event.eventType)
    {
        case OS_EVENT_KEY:
            return SendKeyEventPacket(event);
            break;
        case OS_EVENT_MOUSE:
            return SendMouseEventPacket(event);
            break;
        case OS_EVENT_HID:
            return SendHIDEventPacket(event);
            break;
        default:
            return SocketError::SOCKET_E_INVALID_PARAM;
    }
}

void CCNetworkEntity::AddDisplay(std::shared_ptr<CCDisplay> display)
{
    displays.push_back(display);
}

void CCNetworkEntity::RemoveDisplay(std::shared_ptr<CCDisplay> display)
{
    auto iter = std::find(displays.begin(), displays.end(),display);
    if(iter != displays.end())
    {
        displays.erase(iter);
    }
}

const std::shared_ptr<CCDisplay> CCNetworkEntity::DisplayForPoint(const Point& point)const
{
    for(auto display : displays)
    {
        if(display->PointIsInBounds(point))
            return display;
    }

    return NULL;
}

bool CCNetworkEntity::PointIntersectsEntity(const Point& p) const
{
    for (auto display : displays)
    {
        if (display->PointIsInBounds(p))
            return true;
    }

    return false;
}
