#include "SocketError.h"

#ifdef _WIN32
#include <winsock.h>
#endif
#ifdef __linux__
#include <sys/socket.h>
#endif
#ifdef __unix__
#include <sys/socket.h>
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
            return "Error Invalid Paramater was passed"
        case SOCKET_E_NETWORK:
            return "Error No Network for Socket";
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
    case WSANOTINITIALISED:
        return SOCKET_E_NOT_INITIALIZED;
    default:
        return SOCKET_E_OS_ERROR;
    }
}
