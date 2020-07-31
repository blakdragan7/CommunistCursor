#ifndef MOUSE_INTERFACE_ERROR_H
#define MOUSE_INTERFACE_ERROR_H

#include <string>

enum class OSInterfaceError
{
    OS_E_SUCCESS,
    OS_E_INVALID_PARAM,
    OS_E_ALREADY_REGISTERED,
    OS_E_NOT_REGISTERED,
    OS_E_NOT_AUTHERIZED,
    OS_E_COMMUNICATION_ERROR,
    OS_E_NOT_IMPLEMENTED,
    OS_E_UNKOWN
};

extern std::string OSInterfaceErrorToString(OSInterfaceError err);

#endif
