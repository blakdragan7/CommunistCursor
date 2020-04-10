#ifndef MOUSE_INTERFACE_ERROR_H
#define MOUSE_INTERFACE_ERROR_H

#include <string>

enum OSInterfaceError
{
    OS_E_SUCCESS,
    OS_E_ALREADY_REGISTERED,
    OS_E_NOT_REGISTERED,
};

extern std::string OSInterfaceErrorToString(OSInterfaceError err);

#endif