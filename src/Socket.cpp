#include "Socket.h"

#include "SocketException.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#ifdef __linux__
#include <sys/socket.h>
#endif
#ifdef __unix__
#include <sys/socket.h>
#endif

bool Socket::hasBeenInitialized = false;

Socket::~Socket()
{
    if(isConnected)
    {
        // disconnect
    }

    // free socket
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
    sfd = (void*)INVALID_SOCKET;

    switch(protocol)
    {
        case SOCKET_P_TCP:
            sfd = (NativeSocketHandle)socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
            if(sfd == (NativeSocketHandle)INVALID_SOCKET)
            {
                return SOCKET_E_CREATION;
            }
            break;
        case SOCKET_P_UDP:
            sfd = (NativeSocketHandle)socket(AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP);
            if(sfd == (NativeSocketHandle)INVALID_SOCKET)
            {
                return SOCKET_E_CREATION;
            }
            break;
    }

    return SOCKET_E_SUCCESS;
}

Socket::Socket(SocketProtocol protocol) : protocol(protocol), lastOSErr(0), sfd(0), isBound(false), isConnected(false), port(-1)
{
#ifdef _WIN32
    if(hasBeenInitialized == false)
    {
        throw SocketException(SOCKET_E_NOT_INITIALIZED);
    }
#endif

    if(MakeSocket() != SOCKET_E_SUCCESS)
    {
        throw SocketException(std::string("Socket Creation"));
    }

    SocketError err = MakeSocket();
    if(err != SOCKET_E_SUCCESS)
    {
        throw SocketException(err);
    }
}

Socket::Socket(int port, SocketProtocol protocol) : protocol(protocol), lastOSErr(0), port(port), sfd(0), isBound(false), isConnected(false)
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
        throw SocketException(err);
    }
}

SocketError Socket::Connect(const std::string& address, int port)
{
    if(address.size() == 0)
        return SOCKET_E_INVALID_PARAM;

    if(port == -1)
    {
        if(this->port == -1)
            return SOCKET_E_INVALID_PARAM;
        port = this->port;
    }

    addrinfo* addrInfo;
    addrinfo hints;

    int iResult = getaddrinfo(address.c_str(), itoa(port,NULL, 0), &hints, &addrInfo);

    if(iResult == SOCKET_ERROR)
    {
        closesocket((SOCKET)sfd);
        return SOCK_ERR(iResult);
    }

    // attempt to connect with all results

    while(addrInfo)
    {
        iResult = connect((SOCKET)sfd, addrInfo->ai_addr, addrInfo->ai_addrlen);

        if(iResult != SOCKET_ERROR)
            break;

        addrInfo = addrInfo->ai_next;
    }

    freeaddrinfo(addrInfo);

    if(iResult == SOCKET_ERROR)
    {
        closesocket((SOCKET)sfd);
        return OS_OR_SOCK_ERR;
    }

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Send(void* bytes, size_t length, int port)
{
    if(bytes == NULL || length == 0)
        return SOCKET_E_INVALID_PARAM;
    
    if(port == -1)
    {
        if(this->port == -1)
            return SOCKET_E_INVALID_PARAM;
        port = this->port;
    }

    int iResult = send((SOCKET)sfd, (const char*)bytes, length, 0);
    if (iResult == SOCKET_ERROR || iResult != length) 
    {
        closesocket((SOCKET)sfd);
        return OS_OR_SOCK_ERR;
    }

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Send(const std::string& toSend, int port)
{
    void* data = (void*)toSend.c_str();
    size_t length = toSend.length();

    return Send(data, length, port);
}

SocketError Socket::Recv(char* buff, size_t buffLength, size_t* receivedLength)
{
    if(buff == 0 || buffLength == 0 || receivedLength == 0)
        return SOCKET_E_INVALID_PARAM;

    *receivedLength = recv((SOCKET)sfd, buff, buffLength, 0);
    
    if(*receivedLength < 0)
    {
        return OS_OR_SOCK_ERR;
    }

    return SOCKET_E_SUCCESS;
}

SocketError Socket::Listen(int port)
{
    return SOCKET_E_SUCCESS;
}

SocketError Socket::Listen(const std::string& address, int port)
{
    return SOCKET_E_SUCCESS;
}

SocketError Socket::Accept(NativeSocketHandle* acceptedSocket)
{
    *acceptedSocket = (NativeSocketHandle)INVALID_SOCKET;
    *acceptedSocket = (NativeSocketHandle)accept((SOCKET)sfd, NULL, NULL);

    if(*acceptedSocket == (void*)INVALID_SOCKET)
    {
        lastOSErr = OSGetLastError();
        return SOCK_ERR(lastOSErr);
    }

    return SOCKET_E_SUCCESS;
}