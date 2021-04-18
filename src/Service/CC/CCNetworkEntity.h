#ifndef NETWORK_ENTITY_H
#define NETWORK_ENTITY_H

#include "BasicTypes.h"
#include "../Socket/SocketError.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

/*
*
*   A CCNetworkEntity represent a computer connected to this Session. It contains the monitor information
*   about this computer as well as a way to communicate with this computer.
*
*   Note: If the socket becomes invalid all "Send" functions will fail
*
*/

struct OSEvent;

class Socket;
class CCDisplay;
class CCConfigurationManager;
class INetworkEntityDelegate;

enum class TCPPacketType : int
{
    // RPC Types
    RPC_SetMousePosition    = 0,
    RPC_HideMouse           = 1,
    RPC_UnhideMouse         = 2,

    // Monitoring Types
    Heartbeat               = 3,

    // Events
    OSEventHeader           = 4,
    OSEventCLipboard        = 5
};

enum class JumpDirection : int
{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class CCNetworkEntity
{
private:
    std::unique_ptr<Socket> _udpCommSocket;
    std::unique_ptr<Socket> _tcpCommSocket; // is a server on local entities and a client for remote
    std::vector<std::shared_ptr<CCDisplay>> _displays;
    std::string _entityID;
    std::string _entityName;
    bool _isLocalEntity;
    bool _wasGivenOffset;
    std::mutex      _tcpMutex;

    // only used client side
    unsigned           _tcpCommQueue;
    unsigned           _udpCommQueue;
    std::atomic<bool>  _shouldBeRunningCommThread;

    // only used server side
    
    Point _offsets;
    Rect  _totalBounds;

    INetworkEntityDelegate* _delegate;

    std::vector<CCNetworkEntity*> _topEntites;
    std::vector<CCNetworkEntity*> _bottomEntites;
    std::vector<CCNetworkEntity*> _leftEntites;
    std::vector<CCNetworkEntity*> _rightEntites;

    static int _jumpBuffer;
    static int _collisionBuffer;

private:
    // Some Helper Functions
    bool ShouldRetryRPC(SocketError error)const;
    SocketError SendRPCOfType(TCPPacketType rpcType, void* data = 0, size_t dataSize = 0);
    SocketError ReceiveOSEvent(std::unique_ptr<Socket>& socket, OSEvent& newEvent);
    SocketError ReceiveOSEvent(Socket* socket, OSEvent& newEvent);
    SocketError SendAwk(std::unique_ptr<Socket>& socket);
    SocketError SendAwk(Socket* socket);
    SocketError WaitForAwk(Socket* socket);
    SocketError HandleServerTCPComm(Socket* server);
    void HandleServerTCPCommJob(Socket* server);

public:
    CCNetworkEntity(std::string entityID, std::string entityName, bool isServer = false);
    CCNetworkEntity(std::string entityID, std::string entityName, Socket* socket);
    ~CCNetworkEntity();
    // converts event into the appropriate packet and sends it over with a header
    void SendOSEvent(const OSEvent& event);
    // sends the clipboard data to the remote computer if this is not a local entity
    // if this is a local entity this just does nothing
    // also if there is no clipboard data to send this does nothing
    void SendLocalClipBoardData();

    // This will add the display to the internal displays vector
    void AddDisplay(std::shared_ptr<CCDisplay> display);
    // this will remove and the display passed to this function from internal list of displays
    // Entity, if not it does nothing 
    void RemoveDisplay(std::shared_ptr<CCDisplay> display);

    // Sets the offsets for all displays in this entity
    void SetDisplayOffsets(Point offsets);

    // this returns the display that this point coincides or NULL if there are none
    const std::shared_ptr<CCDisplay> DisplayForPoint(const Point& point)const;
    // this returns wether or not {p} is within the bounds of any of it's displays
    bool PointIntersectsEntity(const Point& p)const;
    // Adds this entity as a connected entity if close enough
    void AddEntityIfInProximity(CCNetworkEntity* entity);
    // clears out all connected entities
    void ClearAllEntities();

    bool LoadFrom(const CCConfigurationManager& manager);
    void SaveTo(CCConfigurationManager& manager)const;

    void ShutdownThreads();

    // RPC Functions

    void RPC_SetMousePosition(float xPercent,float yPercent);
    void RPC_HideMouse();
    void RPC_UnhideMouse();

    // Client Functions
    void TCPCommThread();
    void UDPCommThread();

    // Server Functions
    // this is a bad way to do this because it's a thread per NetworkEntity but for now, it will work
    void HeartbeatThread();

    // return a list of all displays accosiated with this entity
    // This is currently just used for hardcoding coords for testing

    inline std::vector<std::shared_ptr<CCDisplay>> GetAllDisplays()const { return _displays; }

    // This tests point {p} against the edges of this entities displays and connected entities
    // if a collision if found that collided entity is stored in *{jumpEntity} and direction is set
    // {p} is modified by reference to account for "jump" zones 
    // returns true on found jump zone
    bool GetEntityForPointInJumpZone(Point& p, CCNetworkEntity** jumpEntity, JumpDirection &direction)const;

    // gettters

    inline const std::string& GetID()const { return _entityID; }
    inline const std::string& GetName()const { return _entityName; }
    inline bool GetIsLocal()const {return _isLocalEntity;};
    inline const Point& GetOffsets()const { return _offsets; }
    inline const Rect& GetBounds()const { return _totalBounds; }
    inline const Socket* GetUDPSocket()const { return _udpCommSocket.get(); }
    inline const bool GetWasGivenOffset()const { return _wasGivenOffset; }

    // setters

    inline void SetDelegate(INetworkEntityDelegate* newDelegate) { _delegate = newDelegate; }

    // operators

    bool operator==(const CCNetworkEntity& other)const
    {
        return _entityID == other._entityID;
    }
};

#endif