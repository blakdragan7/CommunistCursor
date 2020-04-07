#include "Socket.h"

#include "SocketException.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET
#define SOCKET NativeSocketHandle
#endif

#ifndef SD_SEND
#define SD_SEND SDT_SEND
#endif

#ifndef SD_RECEIVE
#define SD_RECEIVE SDT_RECEIVE
#endif

#ifndef SD_BOTH
#define SD_BOTH SDT_ALL
#endif

#ifndef closesocket
#define closesocket close
#endif

bool Socket::hasBeenInitialized = false;

Socket::~Socket()
{
    if(isConnected)
    {
        Disconnect();
    }

    if(_internalSockInfo)
        freeaddrinfo((addrinfo*)_internalSockInfo);
    // free socket
    if(sfd != (NativeSocketHandle)INVALID_SOCKET)
        closesocket((SOCKET)sfd);
}

void Socket::OSSocketStartup()
{
#ifdef _WIN32

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if(iResult != 0)
    {
        throw SocketException(SOCKET_E_INITIALIZATION);
    }

#endif

    hasBeenInitialized = true;
}

void Socket::OSSocketTeardown()
{
    #ifdef _WIN32

    int iResult = WSACleanup();
    if(iResult != 0)
    {
        throw SocketException((std::string("Could not Teadown WinSock Subsystem with Error ") + std::to_string(iResult)));
    }

    #endif

    hasBeenInitialized = false;
}

SocketError Socket::MakeSocket()
{
    sfd = (NativeSocketHandle)INVALID_SOCKET;

    SocketError e = MakeInternalSocketInfo();
    if(e != SOCKET_E_SUCCESS)
    {
        return e;
    }

    struct addrinfo* result = static_cast<addrinfo*>(_internalSockInfo);

    sfd = (NativeSocketHandle)socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(sfd == (NativeSocketHandle)INVALID_SOCKET)
    {
        freeaddrinfo(result);
        lastOSErr = OSGetLastError();
        return SOCKET_E_CREATION;
    }

    return SOCKET_E_SUCCESS;
}

SocketError Socket::MakeInternalSocketInfo()
{
    struct addrinfo hints;

#ifdef _WIN32
    ZeroMemory(&hints, sizeof(addrinfo));
#else
    memset(&hints, 0, sizeof(addrinfo));
#endif

    hints.ai_flags = _isBindable ? AI_PASSIVE : 0;
    hints.ai_family = AF_UNSPEC;

    if(_isBindable)
        hints.ai_family = _useIPV6 ? AF_INET6 : AF_INET;

    switch(protocol)
    {
        case SOCKET_P_TCP:
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_socktype = SOCK_STREAM;
            break;
        case SOCKET_P_UDP:
            hints.ai_protocol = IPPROTO_UDP;
            hints.ai_socktype = SOCK_DGRAM;
            break;
        default:
            return SOCKET_E_INVALID_PROTO;
    }

    addrinfo* result = NULL;

    const char* address_c = address.size() > 0 ? address.c_str() : NULL;

    int iresult = getaddrinfo(address_c, std::to_string(port).c_str(), &hints, &result);
    if(iresult != 0)
    {
        lastOSErr = iresult;
        return SOCK_ERR(iresult);
    }

    if(_internalSockInfo)
        freeaddrinfo((struct addrinfo*)_internalSockInfo);

    _internalSockInfo = result;

    return SOCKET_E_SUCCESS;
}

Socket::Socket(NativeSocketHandle _sfd) : isListening(false), sfd(_sfd), _internalSockInfo(0), _useIPV6(false), _isBindable(false), lastOSErr(0), port(0), isBound(false), isConnected(false)
{
#ifdef _WIN32
    if(hasBeenInitialized == false)
    {
        throw std::exception("Must Call OSSocketStartup before creating a socket !");
    }
#endif
}

Socket::Socket(const std::string& address, int port, bool useIPV6, SocketProtocol protocol) : isListening(false), address(address), _internalSockInfo(0), _useIPV6(useIPV6), _isBindable(false), protocol(protocol), lastOSErr(0), port(port), sfd(0), isBound(false), isConnected(false)
{
#ifdef _WIN32
    if(hasBeenInitialized == false)
    {
        throw std::exception("Must Call OSSocketStartup before creating a socket !");
    }
#endif

    SocketError err = MakeSocket();
    if(err != SOCKET_E_SUCCESS)
    {
        throw SocketException(err, lastOSErr);
    }
}

SocketError Socket::Connect()
{
    // attempt to connect with all results

    struct addrinfo* addrInfo = static_cast<struct addrinfo*>(_internalSockInfo);
    int iResult = 0;

    while(addrInfo)
    {
        iResult = connect((SOCKET)sfd, addrInfo->ai_addr, (int)addrInfo->ai_addrlen);

        if(iResult != SOCKET_ERROR)
            break;

        addrInfo = addrInfo->ai_next;
    }

    if(iResult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    isConnected = true;

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Connect(int _port)
{
    if(_port != -1)
        port = _port;

    SocketError e = MakeInternalSocketInfo();
    if(e != SOCKET_E_SUCCESS)
    {
        return e;
    }

    struct addrinfo* addrInfo = static_cast<addrinfo*>(_internalSockInfo);
    int iResult = 0;
    // attempt to connect with all results

    while(addrInfo)
    {
        iResult = connect((SOCKET)sfd, addrInfo->ai_addr, (int)addrInfo->ai_addrlen);

        if(iResult != SOCKET_ERROR)
            break;

        addrInfo = addrInfo->ai_next;
    }

    if(iResult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    isConnected = true;

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Connect(const std::string& _address, int _port)
{
    if(address.size() == 0)
        return SOCKET_E_INVALID_PARAM;

    if(_port != -1)
        port = _port;

    address = _address;

    SocketError e = MakeInternalSocketInfo();
    if(e != SOCKET_E_SUCCESS)
    {
        return e;
    }

    struct addrinfo* addrInfo = static_cast<addrinfo*>(_internalSockInfo);
    int iResult = 0;
    // attempt to connect with all results

    while(addrInfo)
    {
        iResult = connect((SOCKET)sfd, addrInfo->ai_addr, (int)addrInfo->ai_addrlen);

        if(iResult != SOCKET_ERROR)
            break;

        addrInfo = addrInfo->ai_next;
    }

    _internalSockInfo = addrInfo;

    if(iResult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    isConnected = true;

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Send(void* bytes, size_t length)
{
    if(bytes == NULL || length == 0)
        return SOCKET_E_INVALID_PARAM;

    if(isConnected == false && protocol != SOCKET_P_UDP)
        return SOCKET_E_NOT_CONNECTED;

    int iResult = send((SOCKET)sfd, (const char*)bytes, (int)length, 0);
    if (iResult == SOCKET_ERROR || iResult != length) 
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Send(const std::string& toSend)
{
    void* data = (void*)toSend.c_str();
    size_t length = toSend.length();

    return Send(data, length);
}

SocketError Socket::Recv(char* buff, size_t buffLength, size_t* receivedLength)
{
    if(buff == 0 || buffLength == 0 || receivedLength == 0)
        return SOCKET_E_INVALID_PARAM;

    if(isConnected == false && protocol != SOCKET_P_UDP)
        return SOCKET_E_NOT_CONNECTED;

    int received = recv((SOCKET)sfd, buff, (int)buffLength, 0);
    
    if(received == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    *receivedLength = received;

    return SOCKET_E_SUCCESS;
}

SocketError Socket::WaitForServer()
{
    if(isConnected == false && protocol != SOCKET_P_UDP)
        return SOCKET_E_NOT_CONNECTED;

    // some dumb buff because we dont actually care about the value
    char buff[8];
    int received = 0;
    do
    {
        received = recv((SOCKET)sfd, buff, (int)sizeof(buff), 0);
    
        if(received == SOCKET_ERROR)
        {
            lastOSErr = OSGetLastError();
            return SOCK_ERR(lastOSErr);
        }
    } while (received > 0);
    
    return SOCKET_E_SUCCESS;
}

SocketError Socket::Disconnect(SocketDisconectType sdt)
{
    if(isConnected == false)
        return SOCKET_E_NOT_CONNECTED;

    int dt;

    switch(sdt)
    {
    case SDT_SEND:
        dt = SD_SEND;
        break;
    case SDT_RECEIVE:
        dt = SD_RECEIVE;
        break;
    case SDT_ALL:
        dt = SD_BOTH;
        break;
    default:
        return SOCKET_E_INVALID_PARAM;
    }

    int iResult = shutdown((SOCKET)sfd, dt);
    if (iResult == SOCKET_ERROR) {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Listen()
{
    if ( listen((SOCKET)sfd, SOMAXCONN ) == SOCKET_ERROR ) 
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);        
    }

    isListening = true;

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Listen(int maxAwaitingConnections)
{
    if ( listen((SOCKET)sfd, maxAwaitingConnections ) == SOCKET_ERROR ) 
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);        
    }

    isListening = true;

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Bind()
{
    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);
    int iresult = bind((SOCKET)sfd, result->ai_addr, (int)result->ai_addrlen);

    if(iresult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    isBound = true;
    
    return SOCKET_E_SUCCESS;
}

SocketError Socket::Bind(int _port)
{
    if(_port != -1)
        port = _port;

    SocketError e = MakeInternalSocketInfo();
    if(e != SOCKET_E_SUCCESS)
    {
        return e;
    }

    struct addrinfo* addrInfo = static_cast<addrinfo*>(_internalSockInfo);

    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);
    int iresult = bind((SOCKET)sfd, result->ai_addr, (int)result->ai_addrlen);

    if(iresult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    isBound = true;

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Bind(const std::string& _address, int _port)
{
    if(address.size() == 0)
        return SOCKET_E_INVALID_PARAM;

    if(_port != -1)
        port = _port;

    address = _address;

    SocketError e = MakeInternalSocketInfo();
    if(e != SOCKET_E_SUCCESS)
    {
        return e;
    }

    struct addrinfo* addrInfo = static_cast<addrinfo*>(_internalSockInfo);

    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);
    int iresult = bind((SOCKET)sfd, result->ai_addr, (int)result->ai_addrlen);

    if(iresult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    isBound = true;

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Accept(NativeSocketHandle* acceptedSocket)
{
    *acceptedSocket = (NativeSocketHandle)INVALID_SOCKET;
    *acceptedSocket = (NativeSocketHandle)accept((SOCKET)sfd, NULL, NULL);

    if(*acceptedSocket == (NativeSocketHandle)INVALID_SOCKET)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Accept(Socket** acceptedSocket)
{
    if(isListening == false)
        return SOCKET_E_NOT_LISTENING;

    NativeSocketHandle newSFD = 0;
    SocketError e = Accept(&newSFD);

    if(e != SOCKET_E_SUCCESS)
    {
        return e;
    }

    struct sockaddr_in clientInfo = {0};
    socklen_t length = sizeof(clientInfo);
    int iresult = getpeername((SOCKET)newSFD, (struct sockaddr*)&clientInfo, &length);
    if(iresult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        closesocket((SOCKET)newSFD);
        return SOCK_ERR(lastOSErr);
    }

    Socket* newSocket = new Socket(newSFD);

    newSocket->isConnected = true;
    newSocket->isListening = false;
    newSocket->isBound = false;
    newSocket->protocol = this->protocol;
    newSocket->port = clientInfo.sin_port;
    newSocket->address = inet_ntoa(clientInfo.sin_addr);

    newSocket->_internalSockInfo = 0;
    newSocket->_isBindable = false;
    newSocket->_useIPV6 = this->_useIPV6;

    *acceptedSocket = newSocket;

    return SOCKET_E_SUCCESS;
}