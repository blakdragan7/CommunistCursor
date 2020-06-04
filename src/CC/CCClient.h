#ifndef CC_CLIENT_H
#define CC_CLIENT_H

#include <memory>

class Socket;
class CCClient
{
private:
    std::unique_ptr<Socket> _internalSocket;

public:
    CCClient();
};

#endif