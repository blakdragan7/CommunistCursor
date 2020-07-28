#include "CCNetworkEntity.h"

#include <algorithm>

#include "../Socket/Socket.h"
#include "../OSInterface/OSTypes.h"
#include "../OSInterface/PacketTypes.h"
#include "../OSInterface/OSInterface.h"

#include "../Dispatcher/DispatchManager.h"

#include "CCPacketTypes.h"
#include "CCDisplay.h"
#include "CCLogger.h"

#include "INetworkEntityDelegate.h"
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

int CCNetworkEntity::_jumpBuffer = 5;
int CCNetworkEntity::_collisionBuffer = 100;

SocketError CCNetworkEntity::SendRPCOfType(TCPPacketType rpcType, void* data, size_t dataSize)
{
    if (_isLocalEntity)
        return SocketError::SOCKET_E_UNKOWN;

    std::unique_lock<std::mutex> lock(_tcpMutex);

    NETCPPacketHeader packet((unsigned char)rpcType);

    SocketError error = _tcpCommSocket->Send(&packet, sizeof(packet));
    if (error != SocketError::SOCKET_E_SUCCESS)
    {
        if (ShouldRetryRPC(error))
        {
            lock.unlock();
            return SendRPCOfType(rpcType, data, dataSize);
        }
        else return error;
    }

    error = WaitForAwk(_tcpCommSocket.get());
    if (error != SocketError::SOCKET_E_SUCCESS)
    {
        if (ShouldRetryRPC(error))
        {
            lock.unlock();
            return SendRPCOfType(rpcType, data, dataSize);
        }
        else return error;
    }

    if (data && dataSize != 0)
    {
        error = _tcpCommSocket->Send(data, dataSize);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            if (ShouldRetryRPC(error))
            {
                lock.unlock();
                return SendRPCOfType(rpcType, data, dataSize);
            }
            else return error;
        }

        error = WaitForAwk(_tcpCommSocket.get());
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            if (ShouldRetryRPC(error))
            {
                lock.unlock();
                return SendRPCOfType(rpcType, data, dataSize);
            }
            else return error;
        }
    }

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError CCNetworkEntity::ReceiveOSEvent(std::unique_ptr<Socket>& socket, OSEvent& newEvent)
{
    return ReceiveOSEvent(socket.get(), newEvent);
}

bool CCNetworkEntity::ShouldRetryRPC(SocketError error) const
{
    if (error == SocketError::SOCKET_E_NOT_CONNECTED)
    {
        SocketError error = _tcpCommSocket->Connect(); 
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            return false; 
        }
        return true;
    }
    else if (error == SocketError::SOCKET_E_BROKEN_PIPE)\
    {
        _tcpCommSocket->Close(true); 
        return true; 
    }
    else return false;
}

SocketError CCNetworkEntity::SendAwk(std::unique_ptr<Socket>& socket)
{
    std::lock_guard<std::mutex> lock(_tcpMutex);

    NETCPPacketAwk awk;
    return socket->Send(&awk, sizeof(awk));
}

SocketError CCNetworkEntity::SendAwk(Socket* socket)
{
    std::lock_guard<std::mutex> lock(_tcpMutex);

    NETCPPacketAwk awk;
    return socket->Send(&awk, sizeof(awk));
}

SocketError CCNetworkEntity::WaitForAwk(Socket* socket)
{
    NETCPPacketAwk awk;
    size_t received;
    SocketError error = _tcpCommSocket->Recv((char*)&awk, sizeof(awk), &received);

    if (received != sizeof(awk) || awk.MagicNumber != P_MAGIC_NUMBER)
    {
        LOG_ERROR << "Error Receiving Awk, Invalid Packet received" << std::endl;
        return SocketError::SOCKET_E_INVALID_PACKET;
    }

    return error;
}

SocketError CCNetworkEntity::HandleServerTCPComm(Socket* server)
{
    SocketError error = SocketError::SOCKET_E_SUCCESS;
    if(server->IsConnected() && _shouldBeRunningCommThread)
    {
        LOG_INFO << "while (server->IsConnected() && _shouldBeRunningCommThread) loop start\n";

        NETCPPacketHeader packet;
        size_t received = 0;
        error = server->Recv((char*)&packet, sizeof(packet), &received);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error receiving NETCPPacketHeader from Server {" << server->Address() << "} " << SOCK_ERR_STR(server, error) << std::endl;
            return error;
        }

        if (received == sizeof(packet) && packet.MagicNumber == P_MAGIC_NUMBER)
        {
            error = SendAwk(server);

            if (error != SocketError::SOCKET_E_SUCCESS)
            {
                LOG_ERROR << "Error Sending Awk to Server {" << server->Address() << "} " << SOCK_ERR_STR(server, error) << std::endl;
                return error;
            }

            LOG_INFO << "Received RPC ";
            switch ((TCPPacketType)packet.Type)
            {
            case TCPPacketType::RPC_SetMousePosition:
                LOG_ERROR << "RPC_SetMousePosition" << std::endl;
                {
                    NERPCSetMouseData data;
                    error = server->Recv((char*)&data, sizeof(data), &received);
                    if (error != SocketError::SOCKET_E_SUCCESS)
                    {
                        LOG_ERROR << "Error receiving NetworkEntityRPCSetMouseData Packet from Server {" << server->Address() << "} " << SOCK_ERR_STR(server, error) << std::endl;
                        return error;
                    }
                    if (received != sizeof(data))
                    {
                        LOG_ERROR << "Error Received invalid NetworkEntityRPCSetMouseData from Server {" << server->Address() << "} " << SOCK_ERR_STR(server, error) << std::endl;
                        return error;
                    }

                    error = SendAwk(server);
                    if (error != SocketError::SOCKET_E_SUCCESS)
                    {
                        LOG_ERROR << "Error Sending Awk to Server {" << server->Address() << "} " << SOCK_ERR_STR(server, error) << std::endl;
                        return error;
                    }

                    LOG_INFO << "NetworkEntityRPCSetMouseData {" << data.x << "," << data.y << "}" << std::endl;
                    RPC_SetMousePosition(data.x, data.y);
                }
                break;
            case TCPPacketType::RPC_HideMouse:
                LOG_INFO << "RPC_HideMouse" << std::endl;
                RPC_HideMouse();
                break;
            case TCPPacketType::RPC_UnhideMouse:
                LOG_INFO << "RPC_UnhideMouse" << std::endl;
                RPC_UnhideMouse();
                break;
            case TCPPacketType::Heartbeat:
                LOG_INFO << "Received Heartbeat from server !" << std::endl;
                break;
            case TCPPacketType::OSEventHeader:
                LOG_INFO << "Received OS Event Header from server !" << std::endl;
                {
                    OSEvent osEvent;

                    error = ReceiveOSEvent(server, osEvent);

                    if (error != SocketError::SOCKET_E_SUCCESS)
                    {
                        LOG_ERROR << "Error receiving OSEvent Data Packet from Server {" << server->Address() << "} " << SOCK_ERR_STR(server, error) << std::endl;
                        return error;
                    }

                    error = SendAwk(server);
                    if (error != SocketError::SOCKET_E_SUCCESS)
                    {
                        LOG_ERROR << "Error Sending Awk to Server {" << server->Address() << "} " << SOCK_ERR_STR(server, error) << std::endl;
                        return error;
                    }

                    LOG_INFO << osEvent;

                    auto osError = OSInterface::SharedInterface().SendOSEvent(osEvent);
                    if (osError != OSInterfaceError::OS_E_SUCCESS)
                    {
                        LOG_ERROR << "Error Trying To Inject OS Event" << osEvent << " with error " << OSInterfaceErrorToString(osError) << std::endl;
                        return error;
                    }

                    LOG_INFO << "Sent Event";
                }
                break;
            case TCPPacketType::OSEventCLipboard:
            {
                LOG_INFO << "Received Clipbaord Packet" << std::endl;
                OSClipboardDataPacketHeader cHeader;
                rsize_t received = 0;
                error = server->Recv((char*)&cHeader, sizeof(OSClipboardDataPacketHeader), &received);
                if (error != SocketError::SOCKET_E_SUCCESS)
                {
                    LOG_ERROR << "Error Receiving OSClipboardDataPacketHeader " << SOCK_ERR_STR(server, error) << std::endl;
                    return error;
                }
                if (received != sizeof(OSClipboardDataPacketHeader))
                {
                    LOG_ERROR << "Error Receiving OSClipboardDataPacketHeader " << SOCK_ERR_STR(server, error) << std::endl;
                    return SocketError::SOCKET_E_INVALID_PACKET;
                }

                error = SendAwk(server);
                if (error != SocketError::SOCKET_E_SUCCESS)
                {
                    LOG_ERROR << "Error Sending Awk For OSClipboardDataPacketHeader " << SOCK_ERR_STR(server, error) << std::endl;
                    return error;
                }

                char* osData = new char[((size_t)cHeader.dataSize + 1)];
                memset(osData, 0, ((size_t)cHeader.dataSize+1));

                error = server->Recv(osData, cHeader.dataSize, &received);
                if (error != SocketError::SOCKET_E_SUCCESS)
                {
                    LOG_ERROR << "Error Receiving OS Clipboard Data " << SOCK_ERR_STR(server, error) << std::endl;
                    return error;
                }
                if (received != cHeader.dataSize)
                {
                    LOG_ERROR << "Error Receiving OS Clipboard Data " << SOCK_ERR_STR(server, error) << std::endl;
                    return SocketError::SOCKET_E_INVALID_PACKET;
                }

                error = SendAwk(server);
                if (error != SocketError::SOCKET_E_SUCCESS)
                {
                    LOG_ERROR << "Error Sending Awk For OS Clipboard Data " << SOCK_ERR_STR(server, error) << std::endl;
                    return error;
                }

                ClipboardData data(osData, (ClipboardDataType)cHeader.dataType);
                OSInterfaceError oError = OSInterface::SharedInterface().SetClipboardData(data);
                if (oError != OSInterfaceError::OS_E_SUCCESS)
                {
                    LOG_ERROR << "Error Setting Clipboard Data " << OSInterfaceErrorToString(oError) << std::endl;
                }
            }
                break;
            }
        }
        else
        {
            LOG_ERROR << "Received Invalid TCP Packet from Server {" << server->Address() << "} " << SOCK_ERR_STR(server, error) << std::endl;
        }
    }
    else return SocketError::SOCKET_E_BROKEN_PIPE;

    return error;
}

void CCNetworkEntity::HandleServerTCPCommJob(Socket* server)
{
    if (HandleServerTCPComm(server) != SocketError::SOCKET_E_SUCCESS)
    {
        delete server;
        if (_delegate)
            _delegate->LostServer();
        if (_shouldBeRunningCommThread)
        {
            DISPATCH_ASYNC_SERIAL(_tcpCommQueue, std::bind(&CCNetworkEntity::TCPCommThread, this));
        }
    }

    if (_shouldBeRunningCommThread)
    {
        DISPATCH_ASYNC_SERIAL(_tcpCommQueue, std::bind(&CCNetworkEntity::HandleServerTCPCommJob, this, server));
    }
    else
    {
        delete server;
    }
}

CCNetworkEntity::CCNetworkEntity(std::string entityID, bool isServer) : _tcpCommQueue(0), _entityID(entityID), _isLocalEntity(true), \
_shouldBeRunningCommThread(true), _delegate(0)
{
    // this is local so we make the server here
    int port = 1045; // this should be configured somehow at some point
    if (isServer == false)
    {
        _tcpCommSocket = std::make_unique<Socket>(SOCKET_ANY_ADDRESS, port, false, SocketProtocol::SOCKET_P_TCP);
        _udpCommSocket = std::make_unique<Socket>(SOCKET_ANY_ADDRESS, 1047, false, SocketProtocol::SOCKET_P_UDP);
        _tcpCommQueue = CREATE_SERIAL_QUEUE("CCNetworkEntity TCP Comm Thread");
        _udpCommQueue = CREATE_SERIAL_QUEUE("CCNetworkEntity UDP Comm Thread");
        DISPATCH_ASYNC_SERIAL(_tcpCommQueue, std::bind(&CCNetworkEntity::TCPCommThread, this));
        DISPATCH_ASYNC_SERIAL(_udpCommQueue, std::bind(&CCNetworkEntity::UDPCommThread, this));
    }
}

CCNetworkEntity::CCNetworkEntity(std::string entityID, Socket* socket) : _tcpCommQueue(0), _entityID(entityID), _udpCommSocket(socket),\
_isLocalEntity(false), _shouldBeRunningCommThread(true), _delegate(0)
{
    // this is a remote entity so we create a tcp client here
    std::string address = socket->Address();
    int port = 1045; // this should be configured somehow at some point

    // this is our comm socket, we don't need to do anything else at this point with it
    _tcpCommSocket = std::make_unique<Socket>(address, port, false, SocketProtocol::SOCKET_P_TCP);
    _tcpCommQueue = CREATE_SERIAL_QUEUE("CCNetworkEntity Heartbeat Queue");

    DISPATCH_ASYNC_SERIAL(_tcpCommQueue, std::bind(&CCNetworkEntity::HeartbeatThread, this));
}

CCNetworkEntity::~CCNetworkEntity()
{
    ShutdownThreads();
}

void CCNetworkEntity::SendOSEvent(const OSEvent& event)
{
    if (_isLocalEntity)
    {
        LOG_ERROR << "Trying to send Event to Local Entity !" << std::endl;
        return;
    }

    if (event.eventType == OS_EVENT_MOUSE && event.mouseEvent == MOUSE_EVENT_MOVE)
    {
        OSInputEventPacket inputPacket(event);
        SocketError error = _udpCommSocket->SendTo(&inputPacket, sizeof(inputPacket));
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error Sending UDP OS Event " << SOCK_ERR_STR(_udpCommSocket, error) << std::endl;
        }

        return;
    }

    std::lock_guard<std::mutex> lock(_tcpMutex);

    if (_tcpCommSocket->IsConnected() == false)
    {
        SocketError error = _tcpCommSocket->Connect();
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error Connecting tcp socket Error: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
            return;
        }
    }

    NETCPPacketHeader header((char)TCPPacketType::OSEventHeader);
    SocketError error = _tcpCommSocket->Send(&header, sizeof(header));
    if (error != SocketError::SOCKET_E_SUCCESS)
    {
        LOG_ERROR << "Error sending NETCPPacketHeader Error: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
        return;
    }

    error = WaitForAwk(_tcpCommSocket.get());
    if (error != SocketError::SOCKET_E_SUCCESS)
    {
        LOG_ERROR << "Error waiting for client Awk Error: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
        return;
    }

    OSInputEventPacket packet(event);

    error = _tcpCommSocket->Send(&packet, sizeof(packet));
    if (error != SocketError::SOCKET_E_SUCCESS)
    {
        LOG_ERROR << "Error sending OSInputEventPacket Error: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
        return;
    }

    error = WaitForAwk(_tcpCommSocket.get());
    if(error !=SocketError::SOCKET_E_SUCCESS)
    {
        LOG_ERROR << "Error waiting for client Awk Error: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
    }
}

SocketError CCNetworkEntity::ReceiveOSEvent(Socket* socket, OSEvent& newEvent)
{
    OSInputEventPacket packet;
    size_t received = 0;
    SocketError error = socket->Recv((char*)&packet, sizeof(packet), &received);

    if (error != SocketError::SOCKET_E_SUCCESS || received != sizeof(packet))
    {
        LOG_ERROR << "Error Trying Receive OS Event Packet From Server !: " << SOCK_ERR_STR(socket, error) << std::endl;
        return error;
    }

    newEvent = packet.AsOSEvent();

    return error;
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
    Rect collision = _totalBounds;

    return collision.topLeft.x <= p.x && collision.topLeft.y <= p.y \
        && collision.bottomRight.x >= p.x && collision.bottomRight.y >= p.y;;
}

void CCNetworkEntity::AddEntityIfInProximity(CCNetworkEntity* entity)
{
    Rect collision = _totalBounds;
    Rect otherCollision = entity->_totalBounds;

    collision.topLeft = collision.topLeft + _offsets;
    collision.bottomRight = collision.bottomRight + _offsets;

    otherCollision.topLeft = otherCollision.topLeft + entity->_offsets;
    otherCollision.bottomRight = otherCollision.bottomRight + entity->_offsets;

    // create test collision Rects, possibly cache these

    Rect top = { {collision.topLeft.x, collision.topLeft.y + _collisionBuffer}, {collision.bottomRight.x, collision.topLeft.y} };
    Rect bottom = { {collision.topLeft.x, collision.bottomRight.y}, {collision.bottomRight.x, collision.bottomRight.y - _collisionBuffer} };
    Rect left   = { {collision.topLeft.x - _collisionBuffer, collision.topLeft.y}, {collision.topLeft.x, collision.bottomRight.y} };
    Rect right  = { {collision.bottomRight.x, collision.topLeft.y}, {collision.bottomRight.x + _collisionBuffer, collision.bottomRight.y} };

    if (otherCollision.IntersectsRect(top))
    {
        _topEntites.push_back(entity);
        entity->_bottomEntites.push_back(this);
    }
    if (otherCollision.IntersectsRect(bottom))
    {
        _bottomEntites.push_back(entity);
        entity->_topEntites.push_back(this);
    }
    if (otherCollision.IntersectsRect(left))
    {
        _leftEntites.push_back(entity);
        entity->_rightEntites.push_back(this);
    }
    if (otherCollision.IntersectsRect(right))
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

void CCNetworkEntity::ShutdownThreads()
{
    _shouldBeRunningCommThread = false;

    if (_udpCommSocket.get())
        _udpCommSocket->Close();

    if (_tcpCommSocket.get())
        _tcpCommSocket->Close();
}

void CCNetworkEntity::RPC_SetMousePosition(float xPercent, float yPercent)
{
    if (_isLocalEntity)
    {
        // warp mouse
        int x = _totalBounds.topLeft.x + (int)((_totalBounds.bottomRight.x - _totalBounds.topLeft.x) * xPercent);
        int y = _totalBounds.topLeft.y + (int)((_totalBounds.bottomRight.y - _totalBounds.topLeft.y) * yPercent);
        LOG_ERROR << "RPC_SetMousePosition {" << x << "," << y << "}" << std::endl;

        // spawn thread for to send input on because we don't want a dead lock with messages
        // this is a windows issue and could be solved in OSInterface probably but for now
        // this will work

        DISPATCH_ASYNC(([x,y]() {OSInterface::SharedInterface().SetMousePosition(x, y);}));
    }
    else
    {
        NERPCSetMouseData data(xPercent, yPercent);
        SocketError error = SendRPCOfType(TCPPacketType::RPC_SetMousePosition, &data, sizeof(data));
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Could not perform RPC_StartWarpingMouse!: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
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
        SocketError error = SendRPCOfType(TCPPacketType::RPC_HideMouse);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Could not perform RPC_HideMouse!: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
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
        SocketError error = SendRPCOfType(TCPPacketType::RPC_UnhideMouse);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Could not perform RPC!: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
        }
    }
}

void CCNetworkEntity::SendLocalClipBoardData()
{
    if (!_isLocalEntity)
    {
        ClipboardData data;
        OSInterfaceError oerror = OSInterface::SharedInterface().GetClipboardData(data);
        if (oerror != OSInterfaceError::OS_E_SUCCESS)
        {
            LOG_ERROR << "Could not Get Clipboard Data " << OSInterfaceErrorToString(oerror) << std::endl;
            return;
        }

        // is there anything to send ?
        if (data.stringData.size() == 0)
            return;
    
        NETCPPacketHeader nheader((char)TCPPacketType::OSEventCLipboard);
        SocketError error = _tcpCommSocket->Send(&nheader, sizeof(nheader));
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error Sending NETCPPacketHeader: " << SOCK_ERR_STR(_tcpCommSocket, error) << std::endl;
            return;
        }

        error = WaitForAwk(_tcpCommSocket.get());
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error Waiting For Awk after Sending NETCPPacketHeader: " << SOCK_ERR_STR(_tcpCommSocket, error) << std::endl;
            return;
        }

        OSClipboardDataPacketHeader header(data);
        error = _tcpCommSocket->Send((void*)&header, sizeof(header));
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error Sending OSClipboardDataPacketHeader: " << SOCK_ERR_STR(_tcpCommSocket, error) << std::endl;
            return;
        }

        error = WaitForAwk(_tcpCommSocket.get());
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error Waiting For Awk after Sending OSClipboardDataPacketHeader: " << SOCK_ERR_STR(_tcpCommSocket, error) << std::endl;
            return;
        }

        error = _tcpCommSocket->Send(data.stringData);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error Sending Clipboard Data: " << SOCK_ERR_STR(_tcpCommSocket, error) << std::endl;
            return;
        }

        error = WaitForAwk(_tcpCommSocket.get());
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error Waiting For Awk After Sending Clipboard Data: " << SOCK_ERR_STR(_tcpCommSocket, error) << std::endl;
            return;
        }
    }
}

void CCNetworkEntity::TCPCommThread()
{
    SocketError error;
    if (_tcpCommSocket->IsBound() == false)
    {
        error = _tcpCommSocket->Bind();
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error Binding Network Entity TCP Comm Socket: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
        }
    }

    if (_tcpCommSocket->IsListening() == false)
    {
        error = _tcpCommSocket->Listen();
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error Listening From Network Entity TCP Comm Socket: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
        }
    }

    Socket* pserver = NULL;
    error = _tcpCommSocket->Accept(&pserver);

    if (error != SocketError::SOCKET_E_SUCCESS)
    {
        LOG_ERROR << "Error accepting server connection: " << SOCK_ERR_STR(_tcpCommSocket.get(), error) << std::endl;
    }

    if(_shouldBeRunningCommThread)
        DISPATCH_ASYNC_SERIAL(_tcpCommQueue, std::bind(&CCNetworkEntity::HandleServerTCPCommJob, this, pserver));
}

void CCNetworkEntity::UDPCommThread()
{
    SocketError error = SocketError::SOCKET_E_SUCCESS;
    if (_udpCommSocket->IsBound() == false)
    {
        error = _udpCommSocket->Bind();
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error Binding Network Entity UDP Comm Socket: " << SOCK_ERR_STR(_udpCommSocket, error) << std::endl;
        }
    }

    OSEvent event;
    error = ReceiveOSEvent(_udpCommSocket, event);
    if (error != SocketError::SOCKET_E_SUCCESS)
    {
        LOG_ERROR << "Error Receiving OSEvent UDP Comm Socket: " << SOCK_ERR_STR(_udpCommSocket, error) << std::endl;
    }

    LOG_DEBUG << "Received " << event << " From UDP Comm" << std::endl;

    if(_shouldBeRunningCommThread)
        DISPATCH_ASYNC_SERIAL(_udpCommQueue, std::bind(&CCNetworkEntity::UDPCommThread, this));

    OSInterface::SharedInterface().SendOSEvent(event);
}

void CCNetworkEntity::HeartbeatThread()
{
    NETCPPacketHeader hearbeat((char)TCPPacketType::Heartbeat);

    if (_tcpCommSocket->IsConnected() == false)
    {
        std::lock_guard<std::mutex> lock(_tcpMutex);
        SocketError error = _tcpCommSocket->Connect();
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            if (_delegate)
            {
                _shouldBeRunningCommThread = false;
                _delegate->EntityLost(this);
                return;
            }
        }
    }


    auto nextHeartbeat = std::chrono::system_clock::now() + std::chrono::seconds(5);
    {
        std::lock_guard<std::mutex> lock(_tcpMutex);
        SocketError error = _tcpCommSocket->Send((void*)&hearbeat, sizeof(hearbeat));

        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            if (_delegate)
            {
                _shouldBeRunningCommThread = false;
                _delegate->EntityLost(this);
                return;
            }
        }

        NETCPPacketAwk awk;
        size_t received = 0;
        error = _tcpCommSocket->Recv((char*)&awk, sizeof(awk), &received);
        if (error != SocketError::SOCKET_E_SUCCESS)
        {
            if (_delegate)
            {
                _shouldBeRunningCommThread = false;
                _delegate->EntityLost(this);
                return;
            }
        }
        if (received != sizeof(awk) || awk.MagicNumber != P_MAGIC_NUMBER)
        {
            if (_delegate)
            {
                _shouldBeRunningCommThread = false;
                _delegate->EntityLost(this);
                return;
            }
        }
    }

    if (_shouldBeRunningCommThread)
    {
        if (_tcpCommSocket->IsConnected() == false)
        {
            _delegate->EntityLost(this);
            _shouldBeRunningCommThread = false;
            return;
        }

        DISPATCH_AFTER_SERIAL(nextHeartbeat - TIME_NOW,_tcpCommQueue, std::bind(&CCNetworkEntity::HeartbeatThread, this))
    }
}

bool CCNetworkEntity::GetEntityForPointInJumpZone(Point& p, CCNetworkEntity** jumpEntity, JumpDirection& direction)const
{
    Rect collision = _totalBounds;

    if (p.y < (collision.topLeft.y + _jumpBuffer))
    {
        for (auto entity : _topEntites)
        {
            p.x = entity->_totalBounds.topLeft.x + p.x - collision.topLeft.x + \
                _offsets.x - entity->_offsets.x;
            p.y = entity->_totalBounds.bottomRight.y - _jumpBuffer;

           // LOG_DEBUG << "Checking Point " << p << std::endl;

            if (entity->PointIntersectsEntity({ p.x, p.y - _jumpBuffer }))
            {
                *jumpEntity = entity;
                direction = JumpDirection::UP;
                return true;
            }
        }
    }
    if (p.y > (collision.bottomRight.y - _jumpBuffer))
    {
        for (auto entity : _bottomEntites)
        {
            p.x = entity->_totalBounds.topLeft.x + p.x - collision.topLeft.x + \
                _offsets.x - entity->_offsets.x;
            p.y = entity->_totalBounds.bottomRight.y + _jumpBuffer;

            //LOG_DEBUG << "Checking Point " << p << std::endl;
            if (entity->PointIntersectsEntity({ p.x, p.y + _jumpBuffer }))
            {
                *jumpEntity = entity;
                direction = JumpDirection::DOWN;
                return true;
            }
        }
    }
    if (p.x < (collision.topLeft.x + _jumpBuffer))
    {
        for (auto entity : _leftEntites)
        {
            p.x = entity->_totalBounds.bottomRight.x - _jumpBuffer;
            p.y = entity->_totalBounds.topLeft.y + p.y - collision.topLeft.y + \
                _offsets.y - entity->_offsets.y;
           //LOG_DEBUG << "Checking Point " << p << std::endl;
            if (entity->PointIntersectsEntity({ p.x - _jumpBuffer, p.y }))
            {
                *jumpEntity = entity;
                direction = JumpDirection::LEFT;
                return true;
            }
        }
    }
    if (p.x > (collision.bottomRight.x - _jumpBuffer))
    {
        for (auto entity : _rightEntites)
        {
            p.x = entity->_totalBounds.topLeft.x + _jumpBuffer;
            p.y = entity->_totalBounds.topLeft.y + p.y - collision.topLeft.y + \
                _offsets.y - entity->_offsets.y;
            //LOG_DEBUG << "Checking Point " << p << std::endl;
            if (entity->PointIntersectsEntity({ p.x + _jumpBuffer, p.y }))
            {
                *jumpEntity = entity;
                direction = JumpDirection::RIGHT;
                return true;
            }
        }
    }

    // no jump zone
    return false;
}
