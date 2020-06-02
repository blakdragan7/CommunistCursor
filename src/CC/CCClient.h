#ifndef CC_CLIENT_H
#define CC_CLIENT_H

class Socket;
class CCClient
{
private:
    Socket* _internalSocket;

public:
    CCClient();
};

#endif