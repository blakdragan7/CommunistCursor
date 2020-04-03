/* Class that handles createing and maintaining a socket from ths OS */

#ifndef SOCKET_H
#define SOCKET_H

#include <string>

#include "SocketError.h"

typedef void* NativeSocketHandle;

enum SocketProtocol
{
    SOCKET_P_UDP,
    SOCKET_P_TCP
};

class Socket
{
private:
    static bool hasBeenInitialized; // used for checking if OSSocketStartup Was Called

    int port; // the port this socket is connected to
    NativeSocketHandle sfd; // changes what it represents based on OS
    bool isBound; // wether or not this socket is bound to a port
    bool isConnected; // essentially used with tcp connections

    SocketProtocol protocol; // the current protocol to use

    int lastOSErr; // The last error returned by the OS that was not succesful

private:
    /* this must be called before any sockets are created */
    static void OSSocketStartup();
    /* this must be called when there is no longer a need for sockets */
    static void OSSocketTeardown();
    /* Creates a socket and stores the pointer to sfd using the class members*/
    SocketError MakeSocket();

public:
    Socket(SocketProtocol protocol); 
    /* Allows ommiting the port param for member functions which is defaulted to {port} */
    Socket(int port, SocketProtocol protocol); 
    ~Socket();

    SocketError Connect(const std::string& address, int port = -1);

    SocketError Send(void* bytes, size_t length, int port = -1);
    SocketError Send(const std::string& toSend, int port = -1);

    SocketError Recv(char* buff, size_t buffLength, size_t* receivedLength);

    SocketError Listen(int port = -1);
    SocketError Listen(const std::string& address, int port = -1);

    SocketError Accept(NativeSocketHandle* acceptedSocket);
};

extern const std::string SockErrorToString(SocketError err);

#endif
