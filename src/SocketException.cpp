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

SocketException::SocketException(SocketError error) : std::exception()
{
    this->info = SockErrorToString(error);
}

SocketException::SocketException(const std::string info, SocketError error) : std::exception()
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

const char* SocketException::what () const throw ()
{
    return info.c_str();
}
