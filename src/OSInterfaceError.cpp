#include "OSInterfaceError.h"

std::string MouseInterfaceErrorToString(OSInterfaceError err)
{
    switch(err)
    {
        case OS_E_SUCCESS:
            return "Error Success";
        case OS_E_ALREADY_REGISTERED:
            return "Error Already Registered";
        case OS_E_NOT_REGISTERED:
            return "Error Not Registered";
        default:
            return "Invalid Error";
    }
}