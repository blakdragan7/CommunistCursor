#include "OSInterfaceError.h"

std::string OSInterfaceErrorToString(OSInterfaceError err)
{
    switch(err)
    {
        case OSInterfaceError::OS_E_SUCCESS:
            return "Error Success";
        case OSInterfaceError::OS_E_ALREADY_REGISTERED:
            return "Error Already Registered";
        case OSInterfaceError::OS_E_INVALID_PARAM:
            return "Error Invalid Paramater Passed To Function";
        case OSInterfaceError::OS_E_NOT_REGISTERED:
            return "Error Not Registered";
        case OSInterfaceError::OS_E_NOT_AUTHERIZED:
            return "Error Not Autherized";
        case OSInterfaceError::OS_E_COMMUNICATION_ERROR:
            return "Error Internal Communication";
        case OSInterfaceError::OS_E_NOT_IMPLEMENTED:
            return "Error Not Implemented";
        case OSInterfaceError::OS_E_UNKOWN:
            return "Error Unkown Error";
        default:
            return "Invalid Error";
    }
}
