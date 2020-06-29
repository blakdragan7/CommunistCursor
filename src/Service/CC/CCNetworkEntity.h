#ifndef NETWORK_ENTITY_H
#define NETWORK_ENTITY_H

#include "BasicTypes.h"
#include "../Socket/SocketError.h"

#include <string>
#include <vector>
#include <memory>
#include <thread>

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

enum class RPCType : int
{
    RPC_SetMousePosition,
    RPC_HideMouse,
    RPC_UnhideMouse
};

class CCNetworkEntity
{
private:
    std::unique_ptr<Socket> _udpCommSocket;
    std::unique_ptr<Socket> _tcpCommSocket; // is a server on local entities and a client for remote
    std::vector<std::shared_ptr<CCDisplay>> _displays;
    std::string _entityID;
    bool _isLocalEntity;

    // only used client side
    std::thread _tcpCommThread;
    bool _shouldBeRunningCommThread;

    // only used server side
    
    Point _offsets;
    Rect  _totalBounds;

    std::vector<CCNetworkEntity*> _topEntites;
    std::vector<CCNetworkEntity*> _bottomEntites;
    std::vector<CCNetworkEntity*> _leftEntites;
    std::vector<CCNetworkEntity*> _rightEntites;

    static int _jumpBuffer;

private:
    // Some Helper Functions
    bool ShouldRetryRPC(SocketError error)const;
    SocketError SendKeyEventPacket(const OSEvent& event)const;
    SocketError SendMouseEventPacket(const OSEvent& event)const;
    SocketError SendHIDEventPacket(const OSEvent& event)const;
    SocketError SendRPCOfType(RPCType rpcType, void* data = 0, size_t dataSize = 0)const;
    SocketError SendRPCAwk(Socket* socket)const;

public:
    CCNetworkEntity(std::string entityID);
    CCNetworkEntity(std::string entityID, Socket* socket);

    // passes buff and size to internal socket. returns a SocketError enum
    SocketError Send(const char* buff, const size_t size)const;
    // passes ToSend to the internal socket. returns a SocketError enum
    SocketError Send(const std::string toSend)const;
    // converts event into the appropriate packet and sends it over with a header
    SocketError SendOSEvent(const OSEvent& event)const;

    // This will add the display to the internal displays vector
    void AddDisplay(std::shared_ptr<CCDisplay> display);
    // this will remove and the display passed to this function from internal list of displays
    // Entity, if not it does nothing 
    void RemoveDisplay(std::shared_ptr<CCDisplay> display);

    // Sets the offsets for all displays in this entity
    void SetDisplayOffsets(Point offsets);

    // this returns the display that this point coincides or NULL if there are none
    const std::shared_ptr <CCDisplay> DisplayForPoint(const Point& point)const;
    // this returns wether or not {p} is within the bounds of any of it's displays
    bool PointIntersectsEntity(const Point& p)const;
    // Adds this entity as a connected entity if close enough
    void AddEntityIfInProximity(CCNetworkEntity* entity);
    // clears out all connected entities
    void ClearAllEntities();

    void LoadFrom(const CCConfigurationManager& manager);
    void SaveTo(CCConfigurationManager& manager)const;

    // RPC Functions

    void RPC_SetMousePosition(float xPercent,float yPercent);
    void RPC_HideMouse();
    void RPC_UnhideMouse();

    // Client Functions

    void TCPCommThread();

    // return a list of all displays accosiated with this entity
    // This is currently just used for hardcoding coords for testing

    inline std::vector<std::shared_ptr<CCDisplay>> GetAllDisplays()const { return _displays; }

    // This tests point {p} against the edges of this entities displays and connected entities
    // if a collision if found that collided entity is returned
    // {p} is modified by reference to account for "jump" zones 
    CCNetworkEntity* GetEntityForPointInJumpZone(Point& p)const;

    inline const std::string& GetID()const { return _entityID; }
    inline bool GetIsLocal()const {return _isLocalEntity;};
    inline const Point& GetOffsets()const { return _offsets; }
    inline const Rect& GetBounds()const { return _totalBounds; }
};

#endif