#include "CCNetworkEntity.h"

#include <algorithm>

#include "../Socket/Socket.h"
#include "../OSInterface/OSTypes.h"
#include "../OSInterface/PacketTypes.h"
#include "../OSInterface/OSInterface.h"
#include "CCPacketTypes.h"
#include "CCDisplay.h"

#include "CCConfigurationManager.h"

using nlohmann::json;

void to_json(json& j, const Point& p) 
{
    j = json{ {"x", p.x}, {"y", p.y} };
}

void from_json(const json& j, Point& p) 
{
    j.at("x").get_to(p.x);
    j.at("y").get_to(p.y);
}

void to_json(json& j, const Rect& p) 
{
    j = json{ {"topLeft", p.topLeft}, {"bottomRight", p.bottomRight} };
}

void from_json(const json& j, Rect& p) 
{
    j.at("topLeft").get_to(p.topLeft);
    j.at("bottomRight").get_to(p.bottomRight);
}

int CCNetworkEntity::_jumpBuffer = 20;

SocketError CCNetworkEntity::SendKeyEventPacket(const OSEvent& event)const
{
    KeyEventPacket packet(event);
    EventPacketHeader header(EVENT_PACKET_K);

    SocketError ret = _udpCommSocket->Send(&header, sizeof(header));
    if(ret != SocketError::SOCKET_E_SUCCESS)
        return ret;
    return _udpCommSocket->Send(&packet, sizeof(packet));
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

                SocketError ret = _udpCommSocket->Send(&header, sizeof(header));
                if(ret != SocketError::SOCKET_E_SUCCESS)
                    return ret;
                return _udpCommSocket->Send(&packet, sizeof(packet));
            }
            break;
        case MOUSE_EVENT_SCROLL:
            {
                EventPacketHeader header(EVENT_PACKET_MW);
                MouseWheelEventPacket packet(event);

                SocketError ret = _udpCommSocket->Send(&header, sizeof(header));
                if(ret != SocketError::SOCKET_E_SUCCESS)
                    return ret;
                return _udpCommSocket->Send(&packet, sizeof(packet));
            }
            break;
        case MOUSE_EVENT_MOVE:
            {
                EventPacketHeader header(EVENT_PACKET_MM);
                MouseMoveEventPacket packet(event);

                SocketError ret = _udpCommSocket->Send(&header, sizeof(header));
                if(ret != SocketError::SOCKET_E_SUCCESS)
                    return ret;
                return _udpCommSocket->Send(&packet, sizeof(packet));
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

SocketError CCNetworkEntity::SendRCPOfType(RPCType rpcType) const
{
    if (_isLocalEntity)
        return SocketError::SOCKET_E_UNKOWN;

    NetworkEntityRPCPacket packet((unsigned char)rpcType);

    SocketError error = _tcpCommSocket->Send(&packet, sizeof(packet));
    if (error != SocketError::SOCKET_E_SUCCESS)
    {
        if (error == SocketError::SOCKET_E_NOT_CONNECTED)
        {
            SocketError error = _tcpCommSocket->Connect();
            if (error != SocketError::SOCKET_E_SUCCESS)
            {
                return error;
            } 

            return SendRCPOfType(rpcType);
        }
        else if (error == SocketError::SOCKET_E_BROKEN_PIPE)
        {
            _tcpCommSocket->Close(true);
            return SendRCPOfType(rpcType);
        }
        else return error;
    }

    return SocketError::SOCKET_E_SUCCESS;
}

CCNetworkEntity::CCNetworkEntity(std::string entityID) : _entityID(entityID), _isLocalEntity(true), _shouldBeRunningCommThread(true)
{
    // this is local so we make the server here
    int port = 1045; // this should be configured somehow at some point

    _tcpCommSocket = std::make_unique<Socket>(SOCKET_ANY_ADDRESS, port, false, SocketProtocol::SOCKET_P_TCP);
    _tcpCommThread = std::thread(&CCNetworkEntity::TCPCommThread, this);
}

CCNetworkEntity::CCNetworkEntity(std::string entityID, Socket* socket) : _entityID(entityID), _udpCommSocket(socket),\
_isLocalEntity(false), _shouldBeRunningCommThread(false)
{
    // this is a remote entity so we create a tcp client here
    std::string address = socket->GetAddress();
    int port = 1045; // this should be configured somehow at some point

    // this is our comm socket, we don't need to do anything else at this point with it
    _tcpCommSocket = std::make_unique<Socket>(address, port, false, SocketProtocol::SOCKET_P_TCP);
}

SocketError CCNetworkEntity::Send(const char* buff, const size_t size)const
{
    return _udpCommSocket->Send(buff, size);
}

SocketError CCNetworkEntity::Send(const std::string toSend)const
{
    return _udpCommSocket->Send(toSend);
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

    collision.topLeft = collision.topLeft + _offsets;
    collision.bottomRight = collision.bottomRight + _offsets;

    // create test collision Rects, possibly cache these

    Rect top = { {collision.topLeft.x, collision.topLeft.y + _jumpBuffer}, {collision.bottomRight.x, collision.topLeft.y} };
    Rect bottom = { {collision.topLeft.x, collision.bottomRight.y}, {collision.bottomRight.x, collision.bottomRight.y - _jumpBuffer} };
    Rect left   = { {collision.topLeft.x - _jumpBuffer, collision.topLeft.y}, {collision.topLeft.x, collision.bottomRight.y} };
    Rect right  = { {collision.bottomRight.x, collision.topLeft.y}, {collision.bottomRight.x + _jumpBuffer, collision.bottomRight.y} };

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

void CCNetworkEntity::ClearAllEntities()
{
    _leftEntites.clear();
    _topEntites.clear();
    _bottomEntites.clear();
    _rightEntites.clear();
}

void CCNetworkEntity::LoadFrom(const CCConfigurationManager& manager)
{
    manager.GetValue({ "Entities", _entityID }, _offsets);
    SetDisplayOffsets(_offsets);
}

void CCNetworkEntity::SaveTo(CCConfigurationManager& manager) const
{
    manager.SetValue({ "Entities", _entityID }, _offsets);
}

void CCNetworkEntity::RPC_StartWarpingMouse()
{
    if (_isLocalEntity)
    {
        // warp mouse
        OSInterface::SharedInterface().SetMousePosition(400, 600);
    }
    else
    {
        SocketError error = SendRCPOfType(RPCType::RPC_StartWarpingMouse);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            std::cout << "Could not perform RPC_StartWarpingMouse!: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
        }
    }
}

void CCNetworkEntity::RPC_StopWarpingMouse()
{
    if (_isLocalEntity)
    {
        // stop warp mouse
        OSInterface::SharedInterface().SetMousePosition(400,600);
    }
    else
    {
        SocketError error = SendRCPOfType(RPCType::RPC_StopWarpingMouse);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            std::cout << "Could not perform RPC_StopWarpingMouse!: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
        }
    }
}

void CCNetworkEntity::RPC_HideMouse()
{
    if (_isLocalEntity)
    {
        // hide mouse
        OSInterface::SharedInterface().SetMouseHidden(true);
    }
    else
    {
        SocketError error = SendRCPOfType(RPCType::RPC_HideMouse);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            std::cout << "Could not perform RPC_HideMouse!: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
        }
    }
}

void CCNetworkEntity::RPC_UnhideMouse()
{
    if (_isLocalEntity)
    {
        // stop hide mouse
        OSInterface::SharedInterface().SetMouseHidden(false);
    }
    else
    {
        SocketError error = SendRCPOfType(RPCType::RPC_UnhideMouse);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            std::cout << "Could not perform RPC!: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
        }
    }
}

void CCNetworkEntity::TCPCommThread()
{
    SocketError error = _tcpCommSocket->Bind();
    if (error != SocketError::SOCKET_E_SUCCESS)
    {
        std::cout << "Error Binding Network Entity TCP Comm Socket: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
    }

    error = _tcpCommSocket->Listen();
    if (error != SocketError::SOCKET_E_SUCCESS)
    {
        std::cout << "Error Listening From Network Entity TCP Comm Socket: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
    }

    while (_shouldBeRunningCommThread)
    {
        Socket* server = NULL;
        error = _tcpCommSocket->Accept(&server);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            std::cout << "Error accepting server connection: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
            continue;
        }

        NetworkEntityRPCPacket packet;
        size_t received = 0;
        error = server->Recv((char*)&packet, sizeof(packet), &received);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            std::cout << "Error receiving RPC Packet from Server {" << server->GetAddress() << "} " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
        }
        else
        {
            if (received == sizeof(packet) && packet.MagicNumber == P_MAGIC_NUMBER)
            {
                std::cout << "Received RPC ";
                switch ((RPCType)packet.RPCType)
                {
                case RPCType::RPC_StartWarpingMouse:
                    std::cout << "RPC_StartWarpingMouse" << std::endl;
                    RPC_StartWarpingMouse();
                    break;
                case RPCType::RPC_StopWarpingMouse:
                    std::cout << "RPC_StopWarpingMouse" << std::endl;
                    RPC_StopWarpingMouse();
                    break;
                case RPCType::RPC_HideMouse:
                    std::cout << "RPC_HideMouse" << std::endl;
                    RPC_HideMouse();
                    break;
                case RPCType::RPC_UnhideMouse:
                    std::cout << "RPC_UnhideMouse" << std::endl;
                    RPC_UnhideMouse();
                    break;
                }
            }
            else
            {
                std::cout << "Received Invalid RPC Packet from Server {" << server->GetAddress() << "} " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
            }
        }

        delete server;
    }
}

CCNetworkEntity* CCNetworkEntity::GetEntityForPointInJumpZone(Point& p) const
{
    Rect collision = _totalBounds;

    collision.topLeft = collision.topLeft + _offsets;
    collision.bottomRight = collision.bottomRight + _offsets;

    if (p.y < (collision.topLeft.y + _jumpBuffer))
    {
        for (auto entity : _topEntites)
        {
            if (entity->PointIntersectsEntity({ p.x, p.y - _jumpBuffer }))
            {
                p.y -= _jumpBuffer;
                return entity;
            }
        }
    }
    if (p.y > (collision.bottomRight.y - _jumpBuffer))
    {
        for (auto entity : _bottomEntites)
        {
            if (entity->PointIntersectsEntity({ p.x, p.y + _jumpBuffer }))
            {
                p.y += _jumpBuffer;
                return entity;
            }
        }
    }
    if (p.x < (collision.topLeft.x + _jumpBuffer))
    {
        for (auto entity : _leftEntites)
        {
            if (entity->PointIntersectsEntity({ p.x - _jumpBuffer, p.y }))
            {
                p.x -= _jumpBuffer;
                return entity;
            }
        }
    }
    if (p.x > (collision.bottomRight.x - _jumpBuffer))
    {
        for (auto entity : _rightEntites)
        {
            if (entity->PointIntersectsEntity({ p.x + _jumpBuffer, p.y }))
            {
                p.y = +_jumpBuffer;
                return entity;
            }
        }
    }

    // no jump zone
    return nullptr;
}
