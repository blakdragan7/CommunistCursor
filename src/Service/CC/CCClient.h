#ifndef CC_CLIENT_H
#define CC_CLIENT_H

#include <memory>
#include <string>
#include <vector>

#include "../OSInterface/OSTypes.h"

class Socket;
class CCNetworkEntity;
class CCClient
{
private:
    std::unique_ptr<Socket> _internalSocket;
    std::vector<NativeDisplay> _displayList;

    std::string _serverAddress;
    int _listenPort;

    bool _needsNewServer;

public:
    CCClient(int listenPort);
    // connects to and performs handshake with server.
    // gives list of native display and local address to server
    void ConnectToServer(std::shared_ptr<CCNetworkEntity> localEntity, std::string address, int port);
    // Waits for a single OS event from the server on {port}
    // {port} is cached in the internal socket and for a new value to be used ResetSocket
    // must be called first

    // Stops the client from listening for events
    // ensures the client is no longer receiving or binding port: listenPort
    void StopClientSocket();

    // let CCMain be able to know that we lost the server
    void SetNeedsNewServer() { _needsNewServer = true; }

    // Getters
    inline std::vector<NativeDisplay> GetDisplayList()const { return _displayList; };
    inline bool GetNeedsNewServer()const {return _needsNewServer;}
};

#endif