#ifndef NETWORK_ENTITY_H
#define NETWORK_ENTITY_H

#include "BasicTypes.h"

#include <string>
#include <vector>

/*

    A CCNetworkEntity represent a computer connected to this Session. It contains the monitor information
    about this computer as well as a way to communicate with this computer.

    If the socket becomes invalid all "Send" functions will fail

*/
class Socket;
struct OSEvent;
class CCDisplay;

class CCNetworkEntity
{
private:
    Socket* _internalSocket;
    std::vector<CCDisplay*> displays;

private:
    // Some Helper Functions
    int SendKeyEventPacket(const OSEvent& event)const;
    int SendMouseEventPacket(const OSEvent& event)const;
    int SendHIDEventPacket(const OSEvent& event)const;

public:
    CCNetworkEntity(Socket* socket);

    // passes buff and size to internal socket. returns a SocketError enum
    int Send(const char* buff, const size_t size)const;
    // passes ToSend to the internal socket. returns a SocketError enum
    int Send(const std::string toSend)const;
    // converts event into the appropriate packet and sends it over with a header
    int SendOSEvent(const OSEvent& event)const;

    // This will add the display to the internal displays vector
    // Note, this takes ownership of that display so it must not be deleted outside this
    // class. bounds is a global virtual space bounds of this display
    void AddDisplay(CCDisplay* display);

    // this will remove and delete the display passed to this function if it is owned by this
    // Entity, if not it does nothing 
    void RemoveDisplay(CCDisplay* display);

    // this returns the display that this point coincides or NULL if there are none
    const CCDisplay* DisplayForPoint(const Point& point)const;

    inline bool operator==(const Point& p)const
    {
        return DisplayForPoint(p) != NULL;
    }
};

#endif