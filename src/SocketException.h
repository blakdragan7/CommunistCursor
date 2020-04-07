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
    SocketException(SocketError error);
    SocketException(SocketError error, int osError);
    SocketException(const std::string info, SocketError error = SOCKET_E_OS_ERROR);
    const char * what () const throw ();
};

#endif
