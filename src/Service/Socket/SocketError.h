#ifndef SOCKET_ERROR_H
#define SOCKET_ERROR_H

#include <string>
#include <memory>

class Socket;

enum class SocketError
{
    SOCKET_E_SUCCESS,
    SOCKET_E_TIMEOUT,
    SOCKET_E_NOT_INITIALIZED,
    SOCKET_E_INITIALIZATION,
    SOCKET_E_CREATION,
    SOCKET_E_BROKEN_PIPE,
    SOCKET_E_WOULD_BLOCK,
    SOCKET_E_OP_IN_PROGRESS,
    SOCKET_E_ADDR_IN_USE,
    SOCKET_E_ALREADY_CONN,
    SOCKET_E_CONN_REFUSED,
    SOCKET_E_NOT_CONNECTED,
    SOCKET_E_INVALID_PARAM,
    SOCKET_E_INVALID_PROTO,
    SOCKET_E_MUST_BE_BOUND,
    SOCKET_E_NOT_LISTENING,
    SOCKET_E_BIND,
    SOCKET_E_NETWORK,
    SOCKET_E_OS_ERROR,
    SOCKET_E_NOT_IMPLEMENTED,
    SOCKET_E_INVALID_PACKET,
    SOCKET_E_UNKOWN
};

typedef int NativeError;

extern int OSGetLastError();
extern const std::string SockErrorToString(SocketError err);
extern const std::string OSErrorToString(NativeError os_err);
extern SocketError OSErrorToSocketError(NativeError os_err);
extern std::string FormatedStringWithSocketAndSocketError(const std::shared_ptr<Socket>&, SocketError error);
extern std::string FormatedStringWithSocketAndSocketError(const std::unique_ptr<Socket>&, SocketError error);
extern std::string FormatedStringWithSocketAndSocketError(const Socket* socket, SocketError error);
extern std::string FormatedStringWithSocketAndSocketError(const Socket *socket, SocketError error);

#define OS_OR_SOCK_ERR OSErrorToSocketError(OSGetLastError())
#define SOCK_ERR(X) OSErrorToSocketError(X)
#define SOCK_ERR_STR(a,b) FormatedStringWithSocketAndSocketError(a, b)

#endif
