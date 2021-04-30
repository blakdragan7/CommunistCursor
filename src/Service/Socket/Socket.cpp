#include "Socket.h"

#include "SocketException.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#define GETSOCKOPT(a,b,c,d,e) getsockopt((SOCKET)a,b,c,(char*)d, e)

#else

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/errno.h>

#define ioctlsocket ioctl
#define GETSOCKOPT(a,b,c,d,e) getsockopt((SOCKET)a,b,c,(void*)d, (socklen_t*)e)

#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifdef _WIN32
#define RECVBUFF_TYPE char*
#endif

#ifndef _WIN32
#define SOCKET NativeSocketHandle
#define closesocket close
#define RECVBUFF_TYPE void*
#endif

#ifndef SD_SEND
#define SD_SEND SHUT_WR
#endif

#ifndef SD_RECEIVE
#define SD_RECEIVE SHUT_RD
#endif

#ifndef SD_BOTH
#define SD_BOTH SHUT_RDWR
#endif

#ifndef NO_ERROR
#define NO_ERROR 0
#endif

#undef SOCK_ERR
#define SOCK_ERR ConvertOSError

bool Socket::hasBeenInitialized = false;

Socket::~Socket()
{
    if(_isConnected)
    {
        Disconnect();
    }

    if(_internalSockInfo)
        freeaddrinfo((addrinfo*)_internalSockInfo);
    // free socket
    if(_sfd != (NativeSocketHandle)INVALID_SOCKET)
        Close();
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
    _sfd = (NativeSocketHandle)INVALID_SOCKET;

    SocketError e = MakeInternalSocketInfo();
    if(e != SocketError::SOCKET_E_SUCCESS)
    {
        return e;
    }

    struct addrinfo* result = static_cast<addrinfo*>(_internalSockInfo);

    _sfd = (NativeSocketHandle)socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(_sfd == (NativeSocketHandle)INVALID_SOCKET)
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

    switch(_protocol)
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

    if (_address == SOCKET_ANY_ADDRESS)
    {
        address_c = NULL;
        hints.ai_addr = (struct sockaddr*)INADDR_ANY;
    }
    else
    {
       address_c = _address.size() > 0 ? _address.c_str() : NULL;
    }

    int iresult = getaddrinfo(address_c, std::to_string(_port).c_str(), &hints, &result);
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

Socket::Socket(SocketProtocol protocol, NativeSocketHandle _sfd) : _isBlocking(true), _isBroadcast(false), _isListening(false), _sfd(_sfd), 
_internalSockInfo(0), _useIPV6(false), _isBindable(false), lastOSErr(0), _port(0), _isBound(false), _isConnected(false), _protocol(protocol)
{
#ifdef _WIN32
    if(hasBeenInitialized == false)
    {
        throw std::exception("Must Call OSSocketStartup before creating a socket !");
    }
#endif
}

Socket::Socket(Socket&& socket)noexcept : _isBlocking(socket._isBlocking), _isBroadcast(socket._isBroadcast), _isListening(socket._isListening), _address(socket._address), _internalSockInfo(socket._internalSockInfo), 
_useIPV6(socket._useIPV6), _isBindable(socket._isBindable), _protocol(socket._protocol), lastOSErr(socket.lastOSErr), 
_port(socket._port), _sfd(socket._sfd), _isBound(socket._isBound), _isConnected(socket._isConnected)
{
    memset(&socket, 0, sizeof(Socket));
    socket._sfd = (NativeSocketHandle)INVALID_SOCKET;
}

Socket::Socket(const std::string& address, int port, bool useIPV6, SocketProtocol protocol) : _isBlocking(true), _isListening(false), _address(address), _isBroadcast(false),
_internalSockInfo(0), _useIPV6(useIPV6), _isBindable(false), _protocol(protocol), lastOSErr(0), _port(port), _sfd(0), _isBound(false), _isConnected(false)
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

Socket::Socket(const std::string& address, int port, bool useIPV6, bool isBroadcast, SocketProtocol protocol) : \
_isListening(false), _address(address), _isBroadcast(false),_internalSockInfo(0), _useIPV6(useIPV6), \
_isBindable(false), _protocol(protocol), lastOSErr(0), _port(port), _sfd(0), _isBound(false),\
_isConnected(false), _isBlocking(true)
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
        iResult = connect((SOCKET)_sfd, addrInfo->ai_addr, (int)addrInfo->ai_addrlen);

        if(iResult != SOCKET_ERROR)
            break;

        addrInfo = addrInfo->ai_next;
    }

    if (iResult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        SocketError err = SOCK_ERR(lastOSErr);
        if (err == SocketError::SOCKET_E_WOULD_BLOCK || err == SocketError::SOCKET_E_OP_IN_PROGRESS)
        {
            fd_set read, write, uer;
            struct timeval timeout;

            FD_ZERO(&read);
            FD_ZERO(&write);
            FD_ZERO(&uer);

            FD_SET((SOCKET)_sfd, &read);
            FD_SET((SOCKET)_sfd, &write);
            FD_SET((SOCKET)_sfd, &uer);

            timeout.tv_sec = 2;
            timeout.tv_usec = 0;

            int oer = select(0, &read, &write, &uer, &timeout);
            if (oer == SOCKET_ERROR)
            {
                lastOSErr = OSGetLastError();
                return SOCK_ERR(lastOSErr);
            }
            else if (oer == 0)
            {
                return SocketError::SOCKET_E_TIMEOUT;
            }
            else
            {
                if (FD_ISSET((SOCKET)_sfd, &uer))
                {
                    int len = sizeof(lastOSErr);
                    if (GETSOCKOPT(_sfd, SOL_SOCKET, SO_ERROR, &lastOSErr, &len))
                    {
                        lastOSErr = OSGetLastError();
                    }
                    return SOCK_ERR(lastOSErr);
                }

                if (FD_ISSET((SOCKET)_sfd, &read) && FD_ISSET((SOCKET)_sfd, &write))
                {
                    addrInfo = (struct addrinfo*)_internalSockInfo;
                }
            }
        }
        else return err;
    }

    _internalSockInfo = addrInfo;
    _isConnected = true;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Connect(int port)
{
    if (port != -1 && _port != port)
    {
        _port = port;

        SocketError e = MakeInternalSocketInfo();
        if (e != SocketError::SOCKET_E_SUCCESS)
        {
            return e;
        }
    }

    return Connect();
}

SocketError Socket::Connect(const std::string& address, int port)
{
    if(address.size() == 0)
        return SocketError::SOCKET_E_INVALID_PARAM;

    if (_address != address || (port != -1 && _port != port))
    {
        _address = address;

        if (port != -1)
            _port = port;

        SocketError e = MakeInternalSocketInfo();
        if (e != SocketError::SOCKET_E_SUCCESS)
        {
            return e;
        }
    }

    return Connect();
}

SocketError Socket::SendTo(const void* bytes, size_t length)
{
    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);

    struct sockaddr_in send_addr;

    send_addr.sin_family = result->ai_family;
    send_addr.sin_port = htons(_port);
    int res = inet_pton(send_addr.sin_family, _address.c_str(), &send_addr.sin_addr.s_addr);

    if (res == 0)
    {
        return SocketError::SOCKET_E_INVALID_PARAM;
    }

    if (res == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    res = sendto((SOCKET)_sfd, (const char*)bytes, (int)length, 0, (sockaddr*)&send_addr, (int)sizeof(send_addr));
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

    res = sendto((SOCKET)_sfd, (const char*)bytes, (int)length, 0, (sockaddr*)&send_addr, (int)sizeof(send_addr));
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

    if(_isConnected == false && _protocol != SocketProtocol::SOCKET_P_UDP)
        return SocketError::SOCKET_E_NOT_CONNECTED;

    int iResult = send((SOCKET)_sfd, (const char*)bytes, (int)length, 0);
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

    if(_isConnected == false && _protocol != SocketProtocol::SOCKET_P_UDP)
        return SocketError::SOCKET_E_NOT_CONNECTED;

    int received = recv((SOCKET)_sfd, buff, (int)buffLength, 0);
    
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

    if (_protocol != SocketProtocol::SOCKET_P_UDP)
        return SocketError::SOCKET_E_INVALID_PROTO;

    if (address == SOCKET_ANY_ADDRESS)
    {
        address = "0.0.0.0";
    }

    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);

    struct sockaddr_in recv_addr;
    socklen_t recvAddrSize = sizeof(recv_addr);

    recv_addr.sin_family = result->ai_family;
    recv_addr.sin_port = htons(port);
    int res = inet_pton(recv_addr.sin_family, address.c_str(), &recv_addr.sin_addr.s_addr);
    if (res != 1)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }
//ssize_t recvfrom(int, void *, size_t, int, struct sockaddr * __restrict, socklen_t * __restrict) __DARWIN_ALIAS_C(recvfrom);
    int received = recvfrom((SOCKET)_sfd, (RECVBUFF_TYPE)buff, buffLength, 0, (sockaddr*)&recv_addr, &recvAddrSize);

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
    return RecvFrom(_address, _port, buff, buffLength, receivedLength);
}

SocketError Socket::WaitForServer()
{
    if(_isConnected == false && _protocol != SocketProtocol::SOCKET_P_UDP)
        return SocketError::SOCKET_E_NOT_CONNECTED;

    // some dumb buff because we dont actually care about the value
    char buff[8];
    int received = 0;
    do
    {
        received = recv((SOCKET)_sfd, buff, (int)sizeof(buff), 0);
    
        if(received == SOCKET_ERROR)
        {
            int OSErr = OSGetLastError();
            SocketError er = SOCK_ERR(OSErr);
            if (er == SocketError::SOCKET_E_WOULD_BLOCK || er == SocketError::SOCKET_E_OP_IN_PROGRESS)
                continue;
            lastOSErr = OSErr;
            return er;
        }
    } while (received > 0);
    
    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Close(bool reCreate)
{
    int iResult = closesocket((SOCKET)_sfd);
    if (iResult == SOCKET_ERROR) {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    _sfd = (NativeSocketHandle)INVALID_SOCKET;
    
    if (reCreate)
    {
        SocketError err = MakeSocket();
        if (err != SocketError::SOCKET_E_SUCCESS)
        {
            return err;
        }

        return SetIsBroadcastable(_isBroadcast);
    }

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::SetIsBroadcastable(bool _isBroadcastable)
{
    // early out if nothing to do
    if (_isBroadcast == _isBroadcastable)
        return SocketError::SOCKET_E_SUCCESS;
#ifdef _WIN32
    char b = _isBroadcastable ? '1' : '0';

    int res = setsockopt((SOCKET)_sfd, SOL_SOCKET, SO_BROADCAST, &b, sizeof(b));
#else
    int b = _isBroadcastable ? '1' : '0';

    int res = setsockopt((SOCKET)_sfd, SOL_SOCKET, SO_BROADCAST, &b, sizeof(b));
#endif
    if (res < 0)
    {
#ifndef _WIN32
        lastOSErr = errno;
#else
        lastOSErr = res;
#endif
        return SOCK_ERR(res);
    }

    _isBroadcast = _isBroadcastable;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::SetIsBlocking(bool isBlocking)
{
    u_long iMode = isBlocking ? 0 : 1;
    int iResult = ioctlsocket((SOCKET)_sfd, FIONBIO, &iMode);
    if (iResult != NO_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }
    _isBlocking = isBlocking;
    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Disconnect(SocketDisconectType sdt)
{
    if(_isConnected == false)
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

    int iResult = shutdown((SOCKET)_sfd, dt);
    if (iResult == SOCKET_ERROR) {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Listen()
{
    if ( listen((SOCKET)_sfd, SOMAXCONN ) == SOCKET_ERROR ) 
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);        
    }

    _isListening = true;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Listen(int maxAwaitingConnections)
{
    if ( listen((SOCKET)_sfd, maxAwaitingConnections ) == SOCKET_ERROR ) 
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);        
    }

    _isListening = true;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Bind()
{
    int iresult = 0;
    struct sockaddr_in addr_in;

    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);

    addr_in.sin_family = result->ai_family;
    addr_in.sin_port = htons(_port);
    if (_address == SOCKET_ANY_ADDRESS)
    {
        addr_in.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        iresult = inet_pton(result->ai_family, _address.c_str(), &addr_in.sin_addr);
        if (iresult == SOCKET_ERROR)
        {
            lastOSErr = OSGetLastError();
            return SOCK_ERR(lastOSErr);
        }
    }

    iresult = bind((SOCKET)_sfd, (sockaddr*)&addr_in, sizeof(addr_in));
    if(iresult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    _isBound = true;
    
    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Bind(int _port)
{
    if(_port != -1)
        _port = _port;

    SocketError e = MakeInternalSocketInfo();
    if(e != SocketError::SOCKET_E_SUCCESS)
    {
        return e;
    }

    struct addrinfo* addrInfo = static_cast<addrinfo*>(_internalSockInfo);

    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);
    int iresult = bind((SOCKET)_sfd, result->ai_addr, (int)result->ai_addrlen);

    if(iresult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    _isBound = true;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Bind(const std::string& address, int _port)
{
    if(_address.size() == 0)
        return SocketError::SOCKET_E_INVALID_PARAM;

    if(_port != -1)
        _port = _port;

    _address = address;

    SocketError e = MakeInternalSocketInfo();
    if(e != SocketError::SOCKET_E_SUCCESS)
    {
        return e;
    }

    struct addrinfo* addrInfo = static_cast<addrinfo*>(_internalSockInfo);

    struct addrinfo* result = static_cast<struct addrinfo*>(this->_internalSockInfo);
    int iresult = bind((SOCKET)_sfd, result->ai_addr, (int)result->ai_addrlen);

    if(iresult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    _isBound = true;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Accept(NativeSocketHandle* acceptedSocket)
{
    *acceptedSocket = (NativeSocketHandle)INVALID_SOCKET;
    *acceptedSocket = (NativeSocketHandle)accept((SOCKET)_sfd, NULL, NULL);

    if(*acceptedSocket == (NativeSocketHandle)INVALID_SOCKET)
    {
        lastOSErr = OSGetLastError();
        SocketError e = SOCK_ERR(lastOSErr);
        if (e == SocketError::SOCKET_E_WOULD_BLOCK || e == SocketError::SOCKET_E_OP_IN_PROGRESS)
        {
            bool hasInput = false;
            e = HasReadInput(hasInput, std::chrono::seconds(1));
            if(e != SocketError::SOCKET_E_SUCCESS)
            {
                return e;
            }

            *acceptedSocket = (NativeSocketHandle)accept((SOCKET)_sfd, NULL, NULL);
        }
        else return e;
    }

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Accept(NativeSocketHandle* acceptedSocket, size_t timeout)
{
    *acceptedSocket = (NativeSocketHandle)INVALID_SOCKET;
    *acceptedSocket = (NativeSocketHandle)accept((SOCKET)_sfd, NULL, NULL);

    if (*acceptedSocket == (NativeSocketHandle)INVALID_SOCKET)
    {
        lastOSErr = OSGetLastError();
        SocketError e = SOCK_ERR(lastOSErr);
        if (e == SocketError::SOCKET_E_WOULD_BLOCK || e == SocketError::SOCKET_E_OP_IN_PROGRESS)
        {
            bool hasInput = false;
            e = HasReadInput(hasInput, std::chrono::milliseconds(timeout));
            if (e != SocketError::SOCKET_E_SUCCESS)
            {
                return e;
            }

            *acceptedSocket = (NativeSocketHandle)accept((SOCKET)_sfd, NULL, NULL);
        }
        else return e;
    }

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::ConvertOSError(NativeError error)
{
    SocketError ret = OSErrorToSocketError(error);

    return ret;
}

SocketError Socket::Accept(Socket** acceptedSocket)
{
    if(_isListening == false)
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

    Socket* newSocket = new Socket(this->_protocol, newSFD);

    char newAddress[256] = {0};

    inet_ntop(clientInfo.sin_family, (const void*)&clientInfo.sin_addr, newAddress, sizeof(newAddress));

    newSocket->_isConnected = true;
    newSocket->_isListening = false;
    newSocket->_isBound = false;
    newSocket->_port = clientInfo.sin_port;
    newSocket->_address = newAddress;

    newSocket->_internalSockInfo = 0;
    newSocket->_isBindable = false;
    newSocket->_useIPV6 = this->_useIPV6;

    *acceptedSocket = newSocket;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::Accept(Socket** acceptedSocket, size_t timeout)
{
    if (_isListening == false)
        return SocketError::SOCKET_E_NOT_LISTENING;

    NativeSocketHandle newSFD = 0;
    SocketError e = Accept(&newSFD, timeout);

    if (e != SocketError::SOCKET_E_SUCCESS)
    {
        return e;
    }

    struct sockaddr_in clientInfo = { 0 };
    socklen_t length = sizeof(clientInfo);
    int iresult = getpeername((SOCKET)newSFD, (struct sockaddr*)&clientInfo, &length);
    if (iresult == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        closesocket((SOCKET)newSFD);
        return SOCK_ERR(lastOSErr);
    }

    Socket* newSocket = new Socket(this->_protocol, newSFD);

    char newAddress[256] = { 0 };

    inet_ntop(clientInfo.sin_family, (const void*)&clientInfo.sin_addr, newAddress, sizeof(newAddress));

    newSocket->_isConnected = true;
    newSocket->_isListening = false;
    newSocket->_isBound = false;
    newSocket->_port = clientInfo.sin_port;
    newSocket->_address = newAddress;

    newSocket->_internalSockInfo = 0;
    newSocket->_isBindable = false;
    newSocket->_useIPV6 = this->_useIPV6;

    *acceptedSocket = newSocket;

    return SocketError::SOCKET_E_SUCCESS;
}

SocketError Socket::HasReadInput(bool& outHasReadInput)
{
    fd_set set, err;

    FD_ZERO(&set);
    FD_ZERO(&err);
    FD_SET((SOCKET)_sfd, &set);
    FD_SET((SOCKET)_sfd, &set);
    
    int ret = select(NULL, &set, NULL, &err, NULL);
    if (ret == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }
    if (ret == 1)
    {
        if (FD_ISSET((SOCKET)_sfd, &err))
        {
            int len = sizeof(lastOSErr);
            if (GETSOCKOPT(_sfd, SOL_SOCKET, SO_ERROR, &lastOSErr, &len))
            {
                lastOSErr = OSGetLastError();
            }
            return SOCK_ERR(lastOSErr);
        }

        outHasReadInput = FD_ISSET((SOCKET)_sfd, &set);
        return SocketError::SOCKET_E_SUCCESS;
    }
    if (ret == 0)
    {
        return SocketError::SOCKET_E_TIMEOUT;
    }

    return SocketError::SOCKET_E_UNKOWN;
}

SocketError Socket::HasReadInput(bool& outHasReadInput, WaitDuration  duration)
{
    struct timeval waitTime = {0};
    auto miliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    waitTime.tv_sec = (long)(miliseconds.count() / 1000);
    waitTime.tv_usec = (long)((miliseconds.count() % 1000) * 1000);

    fd_set set, err;

    FD_ZERO(&set);
    FD_ZERO(&err);
    FD_SET((SOCKET)_sfd, &set);
    FD_SET((SOCKET)_sfd, &set);

    int ret = select(NULL, &set, NULL, &err, &waitTime);
    if (ret == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }
    if (ret == 1)
    {
        if (FD_ISSET((SOCKET)_sfd, &err))
        {
            int len = sizeof(lastOSErr);
            if (GETSOCKOPT(_sfd, SOL_SOCKET, SO_ERROR, &lastOSErr, &len))
            {
                lastOSErr = OSGetLastError();
            }
            return SOCK_ERR(lastOSErr);
        }

        outHasReadInput = FD_ISSET((SOCKET)_sfd, &set);
        return SocketError::SOCKET_E_SUCCESS;
    }
    if (ret == 0)
    {
        return SocketError::SOCKET_E_TIMEOUT;
    }

    return SocketError::SOCKET_E_UNKOWN;
}

SocketError Socket::HasWriteOutput(bool& outHasWriteOutput)
{
    fd_set set, err;

    FD_ZERO(&set);
    FD_ZERO(&err);
    FD_SET((SOCKET)_sfd, &set);
    FD_SET((SOCKET)_sfd, &set);

    int ret = select(NULL, NULL, &set, &err, NULL);
    if (ret == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }
    if (ret == 1)
    {
        if (FD_ISSET((SOCKET)_sfd, &err))
        {
            int len = sizeof(lastOSErr);
            if (GETSOCKOPT(_sfd, SOL_SOCKET, SO_ERROR, &lastOSErr, &len))
            {
                lastOSErr = OSGetLastError();
            }
            return SOCK_ERR(lastOSErr);
        }

        outHasWriteOutput = FD_ISSET((SOCKET)_sfd, &set);
        return SocketError::SOCKET_E_SUCCESS;
    }
    if (ret == 0)
        return SocketError::SOCKET_E_WOULD_BLOCK;

    return SocketError::SOCKET_E_UNKOWN;
}

SocketError Socket::HasWriteOutput(bool& outHasWriteOutput, WaitDuration  duration)
{
    struct timeval waitTime = { 0 };
    auto miliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    waitTime.tv_sec = (long)(miliseconds.count() / 1000);
    waitTime.tv_usec = (long)((miliseconds.count() % 1000) * 1000);

    fd_set set, err;

    FD_ZERO(&set);
    FD_ZERO(&err);
    FD_SET((SOCKET)_sfd, &set);
    FD_SET((SOCKET)_sfd, &set);

    int ret = select(NULL, NULL, &set, &err, &waitTime);
    if (ret == SOCKET_ERROR)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }
    if (ret == 1)
    {
        outHasWriteOutput = FD_ISSET((SOCKET)_sfd, &set);
        return SocketError::SOCKET_E_SUCCESS;
    }
    if (ret == 0)
        return SocketError::SOCKET_E_TIMEOUT;

    return SocketError::SOCKET_E_UNKOWN;
}
