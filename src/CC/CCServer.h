#ifndef CC_SERVER_H
#define CC_SERVER_H

class Socket;
class CCServer
{
private:
    Socket* _internalSocket;

public:
    CCServer();
};

#endif