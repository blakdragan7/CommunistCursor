#ifndef NETWORK_ENTITY_H
#define NETWORK_ENTITY_H

#include "BasicTypes.h"

#include <string>
#include <map>

/*

    A NetworkEntity represent a computer connected to this Session. It contains the monitor information
    about this computer as well as a way to communicate with this computer.

    If the socket becomes invalid all "Send" functions will fail

*/
class Socket;
struct OSEvent;
struct NativeDisplay;

typedef std::pair<NativeDisplay*, Rect> DisplayEntry;

class NetworkEntity
{
private:
    Socket* _internalSocket;
    std::map<NativeDisplay*, Rect> displays;

private:
    // Some Helper Functions
    int SendKeyEventPacket(const OSEvent& event)const;
    int SendMouseEventPacket(const OSEvent& event)const;
    int SendHIDEventPacket(const OSEvent& event)const;

public:
    NetworkEntity(Socket* socket);

    // passes buff and size to internal socket. returns a SocketError enum
    int Send(const char* buff, const size_t size)const;
    // passes ToSend to the internal socket. returns a SocketError enum
    int Send(const std::string toSend)const;
    // converts event into the appropriate packet and sends it over with a header
    int SendOSEvent(const OSEvent& event)const;

    // This will add the display to the internal displays vector
    // Note, this takes ownership of that display so it must not be deleted outside this
    // class. bounds is a global virtual space bounds of this display
    void AddDisplay(NativeDisplay* display, Rect bounds);

    // this will remove and delete the display passed to this function if it is owned by this
    // Entity, if not it does nothing 
    void RemoveDisplay(NativeDisplay* display);

    // this returns the display that this point coincides or NULL if there are none
    const NativeDisplay* PointIsInEntitiesDisplay(const Point& point)const;
};

#endif