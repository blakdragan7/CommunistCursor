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

#ifndef _WIN32
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

#ifndef _WIN32
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
        throw SocketException(SocketError::SOCKET_E_INITIALIZATION);
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
    if(e != SocketError::SOCKET_E_SUCCESS)
    {
        return e;
    }

    struct addrinfo* result = static_cast<addrinfo*>(_internalSockInfo);

    sfd = (NativeSocketHandle)socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(sfd == (NativeSocketHandle)INVALID_SOCKET)
    {
        freeaddrinfo(result);
        lastOSErr = OSGetLastError();
        return SocketError::SOCKET_E_CREATION;
    }

    return SocketError::SOCKET_E_SUCCESS;
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

    hints.ai_family = _useIPV6 ? AF_INET6 : AF_INET;

    switch(protocol)
    {
        case SocketProtocol::SOCKET_P_TCP:
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_socktype = SOCK_STREAM;
            break;
        case SocketProtocol::SOCKET_P_UDP:
            hints.ai_protocol = IPPROTO_UDP;
            hints.ai_socktype = SOCK_DGRAM;
            break;
        default:
            return SocketError::SOCKET_E_INVALID_PROTO;
    }
    addrinfo* result = NULL;
    const char* address_c = NULL;

    if (address == SOCKET_ANY_ADDRESS)
    {
        address_c = NULL;
        hints.ai_addr = INADDR_ANY;
    }
    else
    {
       address_c = address.size() > 0 ? address.c_str() : NULL;
    }

    int iresult = getaddrinfo(address_c, std::to_string(port).c_str(), &hints, &result);
    if(iresult != 0)
    {
        lastOSErr = iresult;
        return SOCK_ERR(iresult);
    }

    if(_internalSockInfo)
        freeaddrinfo((struct addrinfo*)_internalSockInfo);

    _internalSockInfo = result;

    return SocketError::SOCKET_E_SUCCESS;
}

Socket::Socket(SocketProtocol protocol, NativeSocketHandle _sfd) : isBroadcast(false), isListening(false), sfd(_sfd), 
_internalSockInfo(0), _useIPV6(false), _isBindable(false), lastOSErr(0), port(0), isBound(false), isConnected(false), protocol(protocol)
{
#ifdef _WIN32
    if(hasBeenInitialized == false)
    {
        throw std::exception("Must Call OSSocketStartup before creating a socket !");
    }
#endif
}

Socket::Socket(Socket&& socket)noexcept : isBroadcast(socket.isBroadcast), isListening(socket.isListening), address(socket.address), _internalSockInfo(socket._internalSockInfo), 
_useIPV6(socket._useIPV6), _isBindable(socket._isBindable), protocol(socket.protocol), lastOSErr(socket.lastOSErr), 
port(socket.port), sfd(socket.sfd), isBound(socket.isBound), isConnected(socket.isConnected)
{
    memset(&socket, 0, sizeof(Socket));
    socket.sfd = (NativeSocketHandle)INVALID_SOCKET;
}

Socket::Socket(const std::string& address, int port, bool useIPV6, SocketProtocol protocol) : isListening(false), address(address), isBroadcast(false),
_internalSockInfo(0), _useIPV6(useIPV6), _isBindable(false), protocol(protocol), lastOSErr(0), port(port), sfd(0), isBound(false), isConnected(false)
{
#ifdef _WIN32
    if(hasBeenInitialized == false)
    {
        throw std::exception("Must Call OSSocketStartup before creating a socket !");
    }
#endif

    SocketError err = MakeSocket();
    if(err != SocketError::SOCKET_E_SUCCESS)
    {
        throw SocketException(err, lastOSErr);
    }
}

Socket::Socket(const std::string& address, int port, bool useIPV6, bool isBroadcast, SocketProtocol protocol) : isListening(false), 
address(address), isBroadcast(false),_internalSockInfo(0), _useIPV6(useIPV6), _isBindable(false), protocol(protocol), lastOSErr(0), 
port(port), sfd(0), isBound(false), isConnected(false)
{
#ifdef _WIN32
    if (hasBeenInitialized == false)
    {
        throw std::exception("Must Call OSSocketStartup before creating a socket !");
    }
#endif

    SocketError err = MakeSocket();
    if (err != SocketError::SOCKET_E_SUCCESS)
    {
        throw SocketException(err, lastOSErr);
    }

    err = SetIsBroadcastable(isBroadcast);
    if (err != SocketError::SOCKET_E_SUCCESS)
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

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Connect(int _port)
{
    if(_port != -1)
        port = _port;

    SocketError e = MakeInternalSocketInfo();
    if(e != SocketError::SOCKET_E_SUCCESS)
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

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Connect(const std::string& _address, int _port)
{
    if(address.size() == 0)
        return SocketError::SOCKET_E_INVALID_PARAM;

    if(_port != -1)
        port = _port;

    address = _address;

    SocketError e = MakeInternalSocketInfo();
    if(e != SocketError::SOCKET_E_SUCCESS)
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

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::SendTo(const void* bytes, size_t length)
{
    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);

    struct sockaddr_in send_addr;

    send_addr.sin_family = result->ai_family;
    send_addr.sin_port = htons(port);
    int res = inet_pton(send_addr.sin_family, address.c_str(), &send_addr.sin_addr.s_addr);

    if (res == 0)
    {
        return SocketError::SOCKET_E_INVALID_PARAM;
    }

    if (res == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    res = sendto((SOCKET)sfd, (const char*)bytes, (int)length, 0, (sockaddr*)&send_addr, (int)sizeof(send_addr));
    if (res == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::SendTo(std::string address, int port, const void* bytes, size_t length)
{
    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);

    struct sockaddr_in send_addr;

    send_addr.sin_family = result->ai_family;
    send_addr.sin_port = htons(port);
    int res = inet_pton(send_addr.sin_family, address.c_str(), &send_addr.sin_addr.s_addr);

    if (res == 0)
    {
        return SocketError::SOCKET_E_INVALID_PARAM;
    }

    if (res == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    res = sendto((SOCKET)sfd, (const char*)bytes, (int)length, 0, (sockaddr*)&send_addr, (int)sizeof(send_addr));
    if (res == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::SendTo(std::string address, int port, const std::string& toSend)
{
    return SendTo(address, port, toSend.c_str(), toSend.length());
}

SocketError Socket::Send(const void* bytes, size_t length)
{
    if(bytes == NULL || length == 0)
        return SocketError::SOCKET_E_INVALID_PARAM;

    if(isConnected == false && protocol != SocketProtocol::SOCKET_P_UDP)
        return SocketError::SOCKET_E_NOT_CONNECTED;

    int iResult = send((SOCKET)sfd, (const char*)bytes, (int)length, 0);
    if (iResult == SOCKET_ERROR || iResult != length) 
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    return SocketError::SOCKET_E_SUCCESS;
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
        return SocketError::SOCKET_E_INVALID_PARAM;

    if(isConnected == false && protocol != SocketProtocol::SOCKET_P_UDP)
        return SocketError::SOCKET_E_NOT_CONNECTED;

    int received = recv((SOCKET)sfd, buff, (int)buffLength, 0);
    
    if(received == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    *receivedLength = received;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::RecvFrom(std::string address, int port, char* buff, size_t buffLength, size_t* receivedLength)
{
    if (buff == 0 || buffLength == 0 || receivedLength == 0)
        return SocketError::SOCKET_E_INVALID_PARAM;

    if (protocol != SocketProtocol::SOCKET_P_UDP)
        return SocketError::SOCKET_E_INVALID_PROTO;

    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);

    struct sockaddr_in recv_addr;
    int recvAddrSize = sizeof(recv_addr);

    recv_addr.sin_family = result->ai_family;
    recv_addr.sin_port = htons(port);
    int res = inet_pton(recv_addr.sin_family, address.c_str(), &recv_addr.sin_addr.s_addr);
    if (res != 1)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    int received = recvfrom((SOCKET)sfd, buff, (int)buffLength, 0, (sockaddr*)&recv_addr, &recvAddrSize);

    if (received == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    *receivedLength = received;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::RecvFrom(char* buff, size_t buffLength, size_t* receivedLength)
{
    return RecvFrom(address, port, buff, buffLength, receivedLength);
}

SocketError Socket::WaitForServer()
{
    if(isConnected == false && protocol != SocketProtocol::SOCKET_P_UDP)
        return SocketError::SOCKET_E_NOT_CONNECTED;

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
    
    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Close(bool reCreate)
{
    int iResult = closesocket((SOCKET)sfd);
    if (iResult == SOCKET_ERROR) {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    sfd = (NativeSocketHandle)INVALID_SOCKET;
    
    if (reCreate)
    {
        SocketError err = MakeSocket();
        if (err != SocketError::SOCKET_E_SUCCESS)
        {
            return err;
        }

        return SetIsBroadcastable(isBroadcast);
    }

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::SetIsBroadcastable(bool _isBroadcastable)
{
    // early out if nothing to do
    if (isBroadcast == _isBroadcastable)
        return SocketError::SOCKET_E_SUCCESS;

    char b = _isBroadcastable ? '1' : '0';

    char broadcast = '1';
    int res = setsockopt((SOCKET)sfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    if (res < 0)
    {
        lastOSErr = res;
        return SOCK_ERR(res);
    }

    isBroadcast = _isBroadcastable;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Disconnect(SocketDisconectType sdt)
{
    if(isConnected == false)
        return SocketError::SOCKET_E_NOT_CONNECTED;

    int dt;

    switch(sdt)
    {
    case SocketDisconectType::SDT_SEND:
        dt = SD_SEND;
        break;
    case SocketDisconectType::SDT_RECEIVE:
        dt = SD_RECEIVE;
        break;
    case SocketDisconectType::SDT_ALL:
        dt = SD_BOTH;
        break;
    default:
        return SocketError::SOCKET_E_INVALID_PARAM;
    }

    int iResult = shutdown((SOCKET)sfd, dt);
    if (iResult == SOCKET_ERROR) {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Listen()
{
    if ( listen((SOCKET)sfd, SOMAXCONN ) == SOCKET_ERROR ) 
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);        
    }

    isListening = true;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Listen(int maxAwaitingConnections)
{
    if ( listen((SOCKET)sfd, maxAwaitingConnections ) == SOCKET_ERROR ) 
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);        
    }

    isListening = true;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Bind()
{
    int iresult = 0;
    struct sockaddr_in addr_in;

    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);

    addr_in.sin_family = result->ai_family;
    addr_in.sin_port = htons(port);
    if (address == SOCKET_ANY_ADDRESS)
    {
        addr_in.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        iresult = inet_pton(result->ai_family, address.c_str(), &addr_in.sin_addr);
        if (iresult == SOCKET_ERROR)
        {
            lastOSErr = OSGetLastError();
            return SOCK_ERR(lastOSErr);
        }
    }

    iresult = bind((SOCKET)sfd, (sockaddr*)&addr_in, sizeof(addr_in));
    if(iresult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    isBound = true;
    
    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Bind(int _port)
{
    if(_port != -1)
        port = _port;

    SocketError e = MakeInternalSocketInfo();
    if(e != SocketError::SOCKET_E_SUCCESS)
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

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Bind(const std::string& _address, int _port)
{
    if(address.size() == 0)
        return SocketError::SOCKET_E_INVALID_PARAM;

    if(_port != -1)
        port = _port;

    address = _address;

    SocketError e = MakeInternalSocketInfo();
    if(e != SocketError::SOCKET_E_SUCCESS)
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

    return SocketError::SOCKET_E_SUCCESS;
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

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Accept(NativeSocketHandle* acceptedSocket, size_t timeout)
{
    *acceptedSocket = (NativeSocketHandle)INVALID_SOCKET;
    *acceptedSocket = (NativeSocketHandle)accept((SOCKET)sfd, NULL, NULL);

    if (*acceptedSocket == (NativeSocketHandle)INVALID_SOCKET)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Accept(Socket** acceptedSocket)
{
    if(isListening == false)
        return SocketError::SOCKET_E_NOT_LISTENING;

    NativeSocketHandle newSFD = 0;
    SocketError e = Accept(&newSFD);

    if(e != SocketError::SOCKET_E_SUCCESS)
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

    Socket* newSocket = new Socket(this->protocol, newSFD);

    char newAddress[256] = {0};

    inet_ntop(clientInfo.sin_family, (const void*)&clientInfo.sin_addr, newAddress, sizeof(newAddress));

    newSocket->isConnected = true;
    newSocket->isListening = false;
    newSocket->isBound = false;
    newSocket->port = clientInfo.sin_port;
    newSocket->address = newAddress;

    newSocket->_internalSockInfo = 0;
    newSocket->_isBindable = false;
    newSocket->_useIPV6 = this->_useIPV6;

    *acceptedSocket = newSocket;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Accept(Socket** acceptedSocket, size_t timeout)
{
    return SocketError::SOCKET_E_NOT_IMPLEMENTED;
}