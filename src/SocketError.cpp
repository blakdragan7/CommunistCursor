#include "SocketError.h"

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <errno.h>
#endif

#ifndef WSAEADDRINUSE
#define WSAEADDRINUSE EADDRINUSE
#endif

#ifndef WSAENETDOWN
#define WSAENETDOWN ENETDOWN
#endif
#ifndef WSAENETUNREACH
#define WSAENETUNREACH ENETUNREACH
#endif
#ifndef WSAECONNABORTED
#define WSAECONNABORTED ECONNABORTED
#endif
#ifndef WSAECONNRESET
#define WSAECONNRESET ECONNRESET
#endif
#ifndef WSAECONNREFUSED
#define WSAECONNREFUSED ECONNREFUSED
#endif
#ifndef WSAENOTCONN
#define WSAENOTCONN ENOTCONN
#endif
#ifndef WSAESHUTDOWN
#define WSAESHUTDOWN ESHUTDOWN
#endif

int OSGetLastError()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

const std::string SockErrorToString(SocketError err)
{
    switch(err)
    {
        case SOCKET_E_SUCCESS:
            return "Socket Error Success";
        case SOCKET_E_BIND:
            return "Error Binding To Port";
        case SOCKET_E_CREATION:
            return "Error Creating Socket";
        case SOCKET_E_ADDR_IN_USE:
            return "Error Address Already In Use";
        case SOCKET_E_CONN_REFUSED:
            return "Error Connection Refused";
        case SOCKET_E_BROKEN_PIPE:
            return "Error Socket Has Broken Pipe";
        case SOCKET_E_NOT_CONNECTED:
            return "Error Socket Not Connected";
        case SOCKET_E_INITIALIZATION:
            return "Error Initializing Socket Subsystem";
        case SOCKET_E_NOT_INITIALIZED:
            return "Error Socket Subsystem Not Initialized";
        case SOCKET_E_INVALID_PARAM:
            return "Error Invalid Paramater was passed";
        case SOCKET_E_INVALID_PROTO:
            return "Error Invalid Protocol given for socket";
        case SOCKET_E_NETWORK:
            return "Error No Network for Socket";
        case SOCKET_E_MUST_BE_BOUND:
            return "Error Listen Called On Unboud Socket";
        case SOCKET_E_NOT_LISTENING:
            return "Error Accept Called On Socket When Socket isn't Listening for Connections";
        case SOCKET_E_OS_ERROR:
            return "OS Error";
        case SOCKET_E_UNKOWN:
            return "Unkown Error";

    }

    return "Invalid SocketError";
}

SocketError OSErrorToSocketError(int os_err)
{
    switch(os_err)
    {
    case WSAEADDRINUSE:
        return SOCKET_E_ADDR_IN_USE;
    case WSAENETDOWN:
    case WSAENETUNREACH:
        return SOCKET_E_NETWORK;
    case WSAECONNABORTED:
    case WSAECONNRESET:
        return SOCKET_E_BROKEN_PIPE;
    case WSAECONNREFUSED:
        return SOCKET_E_CONN_REFUSED;
    case WSAENOTCONN:
        return SOCKET_E_NOT_CONNECTED;
    case WSAESHUTDOWN:
#ifdef _WIN32
    case WSANOTINITIALISED:
#endif
    return SOCKET_E_NOT_INITIALIZED;
    default:
        return SOCKET_E_OS_ERROR;
    }
}
