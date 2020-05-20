/* Class that handles createing and maintaining a socket from ths OS */

#ifndef SOCKET_H
#define SOCKET_H

#include <string>

#include "SocketError.h"

#ifdef _WIN32
typedef void* NativeSocketHandle;
#else
typedef int NativeSocketHandle; 
#endif

enum SocketProtocol
{
    SOCKET_P_UDP,
    SOCKET_P_TCP
};

enum SocketDisconectType
{
    SDT_SEND,
    SDT_RECEIVE,
    SDT_ALL
};

class Socket
{
private:
    static bool hasBeenInitialized; // used for checking if OSSocketStartup Was Called

    bool _isBindable; // used internally for socket options
    bool _useIPV6; // wether or not to attempt to use an IPV6 address

    void* _internalSockInfo; // used to add a little bit of efficiency 

    int port; // the port this socket is connected to
    NativeSocketHandle sfd; // changes what it represents based on OS
    bool isBound; // wether or not this socket is bound to a port
    bool isConnected; // essentially used with tcp connections
    bool isListening; // wether or not Listen has been called succesfully

    std::string address; // the address this socket either connects to or binds to

    SocketProtocol protocol; // the current protocol to use

private:
    /* Creates a socket and stores the pointer to sfd using the class members*/
    SocketError MakeSocket();
    /* Used to make socket info for connecting, creating and binding the socket */
    SocketError MakeInternalSocketInfo();
    /* Used internally to create client sockets from Accept */
    Socket(NativeSocketHandle _sfd); 
    /* Used internally */
    SocketError Accept(NativeSocketHandle* acceptedSocket);

public:
    int lastOSErr; // The last error returned by the OS that was not succesful

public:
    Socket(const std::string& address, int port, bool useIPV6, SocketProtocol protocol); 
    ~Socket();

    // uses address passed in constructor
    SocketError Connect();
    // can be used to change the port to connect to w/o creating a new socket
    SocketError Connect(int port);
    // can be used to change address / port to connect to w/o creating a new socket
    SocketError Connect(const std::string& address, int port = -1);

    SocketError Send(const void* bytes, size_t length);
    SocketError Send(const std::string& toSend);

    SocketError Recv(char* buff, size_t buffLength, size_t* receivedLength);

    SocketError Bind();
    // can be used to change port to bind to w/o creating a new socket
    SocketError Bind(int port);
    // can be used to change address / port to bind to w/o creating a new socket
    SocketError Bind(const std::string& address, int port = -1);

    SocketError Disconnect(SocketDisconectType sdt = SDT_ALL);
    // allows for maximum backlog of connections as defined by the OS
    SocketError Listen();
    // any clients waiting to connect passed the {maxAwaitingConnections} will be given an error and not connected
    SocketError Listen(int maxAwaitingConnections);
    // you must delete {acceptSocket} when finished
    SocketError Accept(Socket** acceptedSocket);
    // wait for server to close socket
    SocketError WaitForServer();

     /* this must be called before any sockets are created */
    static void OSSocketStartup();
    /* this must be called when there is no longer a need for sockets */
    static void OSSocketTeardown();
};

#endif
