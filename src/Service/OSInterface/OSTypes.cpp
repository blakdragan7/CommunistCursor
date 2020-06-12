#include "OSTypes.h"
#define DEFINE_ENUMCLASS_BITWISE_OPERATORS(EnumType)\
EnumType operator |(const EnumType& lhs, const EnumType& rhs)\
{\
    return static_cast<EnumType> (\
        static_cast<std::underlying_type<EnumType>::type>(lhs) |\
        static_cast<std::underlying_type<EnumType>::type>(rhs));\
}\
EnumType operator |=(EnumType& lhs, const EnumType& rhs)\
{\
    lhs = static_cast<EnumType> (\
        static_cast<std::underlying_type<EnumType>::type>(lhs) |\
        static_cast<std::underlying_type<EnumType>::type>(rhs));\
    return lhs;\
}\
EnumType operator &(const EnumType& lhs, const EnumType& rhs)\
{\
    return static_cast<EnumType> (\
        static_cast<std::underlying_type<EnumType>::type>(lhs) &\
        static_cast<std::underlying_type<EnumType>::type>(rhs));\
}\
EnumType operator &=(EnumType& lhs, const EnumType& rhs)\
{\
    lhs = static_cast<EnumType> (\
        static_cast<std::underlying_type<EnumType>::type>(lhs) &\
        static_cast<std::underlying_type<EnumType>::type>(rhs));\
    return lhs;\
}\
EnumType operator ^(const EnumType& lhs, const EnumType& rhs)\
{\
    return static_cast<EnumType> (\
        static_cast<std::underlying_type<EnumType>::type>(lhs) ^\
        static_cast<std::underlying_type<EnumType>::type>(rhs));\
}\
EnumType operator ^=(EnumType& lhs, const EnumType& rhs)\
{\
    lhs = static_cast<EnumType> (\
        static_cast<std::underlying_type<EnumType>::type>(lhs)^\
        static_cast<std::underlying_type<EnumType>::type>(rhs));\
    return lhs;\
}\
EnumType operator ~(const EnumType& rhs)\
{return static_cast<EnumType> (~static_cast<std::underlying_type<EnumType>::type>(rhs));}

DEFINE_ENUMCLASS_BITWISE_OPERATORS(IPAddressFamilly);
DEFINE_ENUMCLASS_BITWISE_OPERATORS(IPAddressType);
