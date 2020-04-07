#ifndef SOCKET_EXCEPTION_H
#define SOCKET_EXCEPTION_H

#include <exception>
#include <string>

#include "SocketError.h"

class SocketException : public std::exception
{
private:
    std::string info;
    
public:
    SocketException(SocketError error) _NOEXCEPT;
    SocketException(SocketError error, int osError) _NOEXCEPT; 
    SocketException(const std::string info, SocketError error = SOCKET_E_OS_ERROR) _NOEXCEPT;
    const char * what () const _NOEXCEPT override;

    virtual ~SocketException() _NOEXCEPT;
};

#endif
