#include "SocketException.h"
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

SocketException::~SocketException() _NOEXCEPT
{

}

SocketException::SocketException(SocketError error) _NOEXCEPT : std::exception()
{
    this->info = SockErrorToString(error);
}

SocketException::SocketException(SocketError error, int osError) _NOEXCEPT : std::exception()
{
    this->info = SockErrorToString(error);

    if(error == SOCKET_E_OS_ERROR)
    {
        this->info += " With Internal Error:  " + std::to_string(OSGetLastError());
    }
}

SocketException::SocketException(const std::string info, SocketError error) _NOEXCEPT : std::exception()
{
    if(error == SOCKET_E_OS_ERROR)
    {
        this->info += " With Internal Error:  " + std::to_string(OSGetLastError());
    }
    else
    {
        this->info += " Error:  " + SockErrorToString(error) + " and Internal Error " + std::to_string(OSGetLastError());;
    }
}

const char* SocketException::what () const _NOEXCEPT
{
    return info.c_str();
}
