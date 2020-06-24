#include "CCNetworkEntity.h"

#include <algorithm>

#include "../Socket/Socket.h"
#include "../OSInterface/OSTypes.h"
#include "../OSInterface/PacketTypes.h"
#include "CCDisplay.h"

int CCNetworkEntity::_jumpBuffer = 20;

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

CCNetworkEntity::CCNetworkEntity(std::string entityID) : _entityID(entityID), _internalSocket(NULL), _isLocalEntity(true)
{
}

CCNetworkEntity::CCNetworkEntity(std::string entityID, Socket* socket) : _entityID(entityID), _internalSocket(socket), _isLocalEntity(false)
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
    if (_isLocalEntity)
    {
        std::cout << "Trying to send Event to Local Entity !" << std::endl;
        return SocketError::SOCKET_E_SUCCESS;
    }

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
    _displays.push_back(display);

    auto displayBounds = display->GetCollision();

    _totalBounds.topLeft.x      = std::min(displayBounds.topLeft.x, _totalBounds.topLeft.x);
    _totalBounds.topLeft.y      = std::min(displayBounds.topLeft.y, _totalBounds.topLeft.y);
    _totalBounds.bottomRight.x  = std::max(displayBounds.bottomRight.x, _totalBounds.bottomRight.x);
    _totalBounds.bottomRight.y  = std::max(displayBounds.bottomRight.y, _totalBounds.bottomRight.y);
}

void CCNetworkEntity::RemoveDisplay(std::shared_ptr<CCDisplay> display)
{
    auto iter = std::find(_displays.begin(), _displays.end(),display);
    if(iter != _displays.end())
    {
        _displays.erase(iter);
    }
}

void CCNetworkEntity::SetDisplayOffsets(Point offsets)
{
    _offsets = offsets;

    _totalBounds.topLeft.x      =  400000;
    _totalBounds.topLeft.y      =  400000;
    _totalBounds.bottomRight.x  = -400000;
    _totalBounds.bottomRight.y  = -400000;

    for (auto display : _displays)
    {
        display->SetOffsets(_offsets.x, _offsets.y);

        auto displayBounds = display->GetCollision();

        _totalBounds.topLeft.x = std::min(displayBounds.topLeft.x, _totalBounds.topLeft.x);
        _totalBounds.topLeft.y = std::min(displayBounds.topLeft.y, _totalBounds.topLeft.y);
        _totalBounds.bottomRight.x = std::max(displayBounds.bottomRight.x, _totalBounds.bottomRight.x);
        _totalBounds.bottomRight.y = std::max(displayBounds.bottomRight.y, _totalBounds.bottomRight.y);
    }
}

const std::shared_ptr<CCDisplay> CCNetworkEntity::DisplayForPoint(const Point& point)const
{
    for(auto display : _displays)
    {
        if(display->PointIsInBounds(point))
            return display;
    }

    return NULL;
}

bool CCNetworkEntity::PointIntersectsEntity(const Point& p) const
{
    for (auto display : _displays)
    {
        if (display->PointIsInBounds(p))
            return true;
    }

    return false;
}

void CCNetworkEntity::AddEntityIfInProximity(CCNetworkEntity* entity)
{
    Rect collision = entity->_totalBounds;

    // create test collision Rects, possibly cache these

    Rect top = { {_totalBounds.topLeft.x, _totalBounds.topLeft.y + _jumpBuffer}, {_totalBounds.bottomRight.x, _totalBounds.topLeft.y} };
    Rect bottom = { {_totalBounds.topLeft.x, _totalBounds.bottomRight.y}, {_totalBounds.bottomRight.x, _totalBounds.bottomRight.y - _jumpBuffer} };
    Rect left   = { {_totalBounds.topLeft.x - _jumpBuffer, _totalBounds.topLeft.y}, {_totalBounds.topLeft.x, _totalBounds.bottomRight.y} };
    Rect right  = { {_totalBounds.bottomRight.x, _totalBounds.topLeft.y}, {_totalBounds.bottomRight.x + _jumpBuffer, _totalBounds.bottomRight.y} };

    if (collision.IntersectsRect(top))
    {
        _topEntites.push_back(entity);
        entity->_bottomEntites.push_back(this);
    }
    if (collision.IntersectsRect(bottom))
    {
        _bottomEntites.push_back(entity);
        entity->_topEntites.push_back(this);
    }
    if (collision.IntersectsRect(left))
    {
        _leftEntites.push_back(entity);
        entity->_rightEntites.push_back(this);
    }
    if (collision.IntersectsRect(right))
    {
        _rightEntites.push_back(entity);
        entity->_leftEntites.push_back(this);
    }
}

CCNetworkEntity* CCNetworkEntity::GetEntityForPointInJumpZone(Point p) const
{
    if (p.y < (_totalBounds.topLeft.y + _jumpBuffer))
    {
        for (auto entity : _topEntites)
        {
            if (entity->PointIntersectsEntity({ p.x, p.y + _jumpBuffer }))
                return entity;
        }
    }
    if (p.y > (_totalBounds.bottomRight.y - _jumpBuffer))
    {
        for (auto entity : _bottomEntites)
        {
            if (entity->PointIntersectsEntity({ p.x, p.y - _jumpBuffer }))
                return entity;
        }
    }
    if (p.x < (_totalBounds.topLeft.x + _jumpBuffer))
    {
        for (auto entity : _rightEntites)
        {
            if (entity->PointIntersectsEntity({ p.x + _jumpBuffer, p.y }))
                return entity;
        }
    }
    if (p.x < (_totalBounds.bottomRight.x - _jumpBuffer))
    {
        for (auto entity : _leftEntites)
        {
            if (entity->PointIntersectsEntity({ p.x - _jumpBuffer, p.y }))
                return entity;
        }
    }

    // no jump zone
    return nullptr;
}
