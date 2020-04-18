#include "OSInterfaceError.h"

std::string OSInterfaceErrorToString(OSInterfaceError err)
{
    switch(err)
    {
        case OS_E_SUCCESS:
            return "Error Success";
        case OS_E_ALREADY_REGISTERED:
            return "Error Already Registered";
        case OS_E_INVALID_PARAM:
            return "Error Invalid Paramater Passed To Function";
        case OS_E_NOT_REGISTERED:
            return "Error Not Registered";
        case OS_E_NOT_AUTHERIZED:
            return "Error Not Autherized";
        case OS_E_UNKOWN:
            return "Error Unkown Error";
        default:
            return "Invalid Error";
    }
}