#ifndef CC_SERVER_H
#define CC_SERVER_H

#include <memory>
#include <string>
#include <thread>
#include <vector>

class Socket;
class INetworkEntityDiscovery;
class CCServer
{
private:
    std::unique_ptr<Socket>                         _internalSocket;
    INetworkEntityDiscovery*                        _discoverer;
    std::thread                                     _accpetThread;
    std::thread                                     _heartbeatThread;

    bool                                            _isRunning;

public:
    CCServer(int port, std::string listenAddress = "127.0.0.1", INetworkEntityDiscovery* discoverer = 0);
    void SetDiscoverer(INetworkEntityDiscovery* discoverer);

    void StartServer();
    void StopServer();

    bool GetServerIsRunning();

    void ServerAcceptThread();
};

#endif