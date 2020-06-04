#ifndef CC_SERVER_H
#define CC_SERVER_H

#include <memory>
#include <string>
#include <thread>

class Socket;
class INetworkEntityDiscovery;
class CCServer
{
private:
    std::unique_ptr<Socket>     _internalSocket;
    INetworkEntityDiscovery*    discoverer;
    std::thread                 accpetThread;

    bool                        isRunning;

public:
    CCServer(int port, std::string listenAddress = "127.0.0.1", INetworkEntityDiscovery* discoverer = 0);

    void SetDiscoverer(INetworkEntityDiscovery* discoverer);

    void StartServer();
    void StopServer();

    bool GetServerIsRunning();

    friend void ServerAcceptThread(CCServer*);
};

#endif