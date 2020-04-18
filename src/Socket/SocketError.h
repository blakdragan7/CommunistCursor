#ifndef SOCKET_ERROR_H
#define SOCKET_ERROR_H

#include <string>

enum SocketError
{
    SOCKET_E_SUCCESS,
    SOCKET_E_NOT_INITIALIZED,
    SOCKET_E_INITIALIZATION,
    SOCKET_E_CREATION,
    SOCKET_E_BROKEN_PIPE,
    SOCKET_E_ADDR_IN_USE,
    SOCKET_E_CONN_REFUSED,
    SOCKET_E_NOT_CONNECTED,
    SOCKET_E_INVALID_PARAM,
    SOCKET_E_INVALID_PROTO,
    SOCKET_E_MUST_BE_BOUND,
    SOCKET_E_NOT_LISTENING,
    SOCKET_E_BIND,
    SOCKET_E_NETWORK,
    SOCKET_E_OS_ERROR,
    SOCKET_E_UNKOWN
};

extern int OSGetLastError();
extern const std::string SockErrorToString(SocketError err);
extern const std::string OSErrorToString(int os_err);
extern SocketError OSErrorToSocketError(int os_err);

#define OS_OR_SOCK_ERR OSErrorToSocketError(OSGetLastError())
#define SOCK_ERR(X) OSErrorToSocketError(X)

#endif
