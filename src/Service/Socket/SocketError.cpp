#include "SocketError.h"

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <errno.h>
#endif

#include "Socket.h"

#ifndef WSAETIMEDOUT
#define WSAETIMEDOUT ETIMEDOUT
#endif

#ifndef WSAEADDRINUSE
#define WSAEADDRINUSE EADDRINUSE
#endif

#ifndef WSAEWOULDBLOCK
#define WSAEWOULDBLOCK EWOULDBLOCK
#endif

#ifndef WSAEALREADY
#define WSAEALREADY EALREADY
#endif

#ifndef WSAEISCONN
#define WSAEISCONN EISCONN
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
        case SocketError::SOCKET_E_SUCCESS:
            return "Socket Error Success";
        case SocketError::SOCKET_E_TIMEOUT:
            return "Socket Timed Out";
        case SocketError::SOCKET_E_BIND:
            return "Error Binding To Port";
        case SocketError::SOCKET_E_CREATION:
            return "Error Creating Socket";
        case SocketError::SOCKET_E_WOULD_BLOCK:
            return "Error Socket Would Block";
        case SocketError::SOCKET_E_OP_IN_PROGRESS:
            return "Error Operation Already In Progress";
        case SocketError::SOCKET_E_ADDR_IN_USE:
            return "Error Address Already In Use";
        case SocketError::SOCKET_E_CONN_REFUSED:
            return "Error Connection Refused";
        case SocketError::SOCKET_E_BROKEN_PIPE:
            return "Error Socket Has Broken Pipe";
        case SocketError::SOCKET_E_ALREADY_CONN:
            return "Error Socket Already Connected";
        case SocketError::SOCKET_E_NOT_CONNECTED:
            return "Error Socket Not Connected";
        case SocketError::SOCKET_E_INITIALIZATION:
            return "Error Initializing Socket Subsystem";
        case SocketError::SOCKET_E_NOT_INITIALIZED:
            return "Error Socket Subsystem Not Initialized";
        case SocketError::SOCKET_E_INVALID_PARAM:
            return "Error Invalid Paramater was passed";
        case SocketError::SOCKET_E_INVALID_PROTO:
            return "Error Invalid Protocol given for socket";
        case SocketError::SOCKET_E_NETWORK:
            return "Error No Network for Socket";
        case SocketError::SOCKET_E_MUST_BE_BOUND:
            return "Error Listen Called On Unboud Socket";
        case SocketError::SOCKET_E_NOT_LISTENING:
            return "Error Accept Called On Socket When Socket isn't Listening for Connections";
        case SocketError::SOCKET_E_OS_ERROR:
            return "OS Error";
        case SocketError::SOCKET_E_NOT_IMPLEMENTED:
            return "Error Not Implemented";
        case SocketError::SOCKET_E_INVALID_PACKET:
            return "Error Invalid Packet Received";
        case SocketError::SOCKET_E_UNKOWN:
            return "Unkown Error";

    }

    return "Invalid SocketError";
}

const std::string OSErrorToString(NativeError os_err)
{
#ifdef _WIN32
    return std::to_string(os_err);
#else
    return strerror(os_err);
#endif
}

SocketError OSErrorToSocketError(NativeError os_err)
{
    switch(os_err)
    {
    case WSAEADDRINUSE:
        return SocketError::SOCKET_E_ADDR_IN_USE;
    case WSAETIMEDOUT:
        return SocketError::SOCKET_E_TIMEOUT;
    case WSAENETDOWN:
    case WSAENETUNREACH:
        return SocketError::SOCKET_E_NETWORK;
    case WSAECONNABORTED:
    case WSAECONNRESET:
        return SocketError::SOCKET_E_BROKEN_PIPE;
    case WSAEWOULDBLOCK:
        return SocketError::SOCKET_E_WOULD_BLOCK;
    case WSAEALREADY:
        return SocketError::SOCKET_E_OP_IN_PROGRESS;
    case WSAEISCONN:
        return SocketError::SOCKET_E_ALREADY_CONN;
    case WSAECONNREFUSED:
        return SocketError::SOCKET_E_CONN_REFUSED;
    case WSAENOTCONN:
        return SocketError::SOCKET_E_NOT_CONNECTED;
    case WSAESHUTDOWN:
#ifdef _WIN32
    case WSANOTINITIALISED:
        return SocketError::SOCKET_E_NOT_INITIALIZED;
#endif
    default:
        return SocketError::SOCKET_E_OS_ERROR;
    }
}

std::string FormatedStringWithSocketAndSocketError(const std::shared_ptr<Socket>& socket, SocketError error)
{
    return FormatedStringWithSocketAndSocketError(socket.get(), error);
}

std::string FormatedStringWithSocketAndSocketError(const std::unique_ptr<Socket>& socket, SocketError error)
{
    return FormatedStringWithSocketAndSocketError(socket.get(), error);
}

std::string FormatedStringWithSocketAndSocketError(const Socket* socket, SocketError error)
{
   std::string info = SockErrorToString(error);

    if (error == SocketError::SOCKET_E_OS_ERROR)
    {
        info += " With Internal Error:  " + OSErrorToString(socket->lastOSErr);
    }

    return info;
}
