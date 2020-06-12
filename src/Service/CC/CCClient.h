#ifndef CC_CLIENT_H
#define CC_CLIENT_H

#include <memory>
#include <string>
#include <vector>

#include "../OSInterface/OSTypes.h"

class Socket;
class CCClient
{
private:
    std::unique_ptr<Socket> _internalSocket;
    std::vector<NativeDisplay> _displayList;

    std::string serverAddress;
    int listenPort;

public:
    CCClient(int listenPort);
    // connects to and performs handshake with server.
    // gives list of native display and local address to server
    void ConnectToServer(std::string address, int port);
    // Waits for a single OS event from the server on {port}
    // {port} is cached in the internal socket and for a new value to be used ResetSocket
    // must be called first

    // {newEvent} is a reconstructed OSEvent from the packet information
    // if return value is false then newEvent is undefined
    // returns True upen success and false otherwise
    bool ListenForOSEvent(OSEvent& newEvent);
    // Stops the client from listening for events
    // ensures the client is no longer receiving or binding port: listenPort
    void StopClientSocket();
};

#endif