#ifndef OS_TYPES_H
#define OS_TYPES_H

#include <iostream>
#include <bitset>

#define ADD_ENUMCLASS_BITWISE_OPERATORS(EnumType) \
extern EnumType operator |(const EnumType& lh, const EnumType& rh);\
extern EnumType operator |=(EnumType& lh, const EnumType& rh);\
extern EnumType operator &(const EnumType& lh, const EnumType& rh);\
extern EnumType operator &=(EnumType& lh, const EnumType& rh);\
extern EnumType operator ^(const EnumType& lh, const EnumType& rh);\
extern EnumType operator ^=(EnumType& lh, const EnumType& rh);\
extern EnumType operator ~(const EnumType& rh);

// IP Types

enum class IPAddressFamilly
{
    NONE = 0,
    IPv4 = 1 << 0,
    IPv6 = 1 << 1,
    ALL  = 0x255
};

ADD_ENUMCLASS_BITWISE_OPERATORS(IPAddressFamilly)

enum class IPAddressType
{
    NONE        = 0,
    UNICAST     = 1 << 0,
    MULTICAST   = 1 << 1,
    ANYCAST     = 1 << 2,
    DNSSERVER   = 1 << 3,
    ALL         = 0x255
};

ADD_ENUMCLASS_BITWISE_OPERATORS(IPAddressType)

struct IPAdressInfo
{
    std::string		    address;
    std::string		    subnetMask;
    std::string		    adaptorName;

    IPAddressFamilly    addressFamilly;
    IPAddressType	    addressType;
};

struct IPAdressInfoHints
{
    IPAddressType       type;
    IPAddressFamilly    familly;

    IPAdressInfoHints() :type(IPAddressType::ALL), familly(IPAddressFamilly::ALL) {}
    IPAdressInfoHints(IPAddressType type, IPAddressFamilly familly):type(type), familly(familly) {}
};

// Event Types

enum OSEventType
{
    OS_EVENT_MOUSE = 0,
    OS_EVENT_KEY = 1,
    OS_EVENT_HID = 2,
    OS_EVENT_INVALID = -1
};

enum MouseEventType
{
    MOUSE_EVENT_MOVE = 0,
    MOUSE_EVENT_DOWN = 1,
    MOUSE_EVENT_UP = 2,
    MOUSE_EVENT_SCROLL = 3,
    MOUSE_EVENT_INVALID = -1
};

enum MouseButton
{
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT = 1,
    MOUSE_BUTTON_MIDDLE = 2,
    MOUSE_BUTTON_EXTENDED = 3,
    MOUSE_BUTTON_INVALID = -1
};

enum KeyEventType
{
    KEY_EVENT_DOWN = 0,
    KEY_EVENT_UP = 1,
    KEY_EVENT_INVALID = -1
};

struct NativeDisplay
{
    int nativeScreenID;
    int posX,posY;
    int width,height;
    NativeDisplay() : nativeScreenID(-1), posX(-1), posY(-1), width(-1), height(-1)
    {}
};

struct OSEvent
{
    OSEventType eventType;

    union
    {
        MouseEventType mouseEvent;
        KeyEventType keyEvent;
    };
    
    union
    {
        MouseButton mouseButton;
        int scanCode;
    };

    int extendButtonInfo;

    int deltaX;
    int x;

    int deltaY;
    int y;

    int nativeScreenID;

    OSEvent() : eventType(OS_EVENT_INVALID), extendButtonInfo(0), \
                                deltaX(0), x(0), deltaY(0), y(0), nativeScreenID(-1)
    {keyEvent = KEY_EVENT_INVALID;scanCode = -1;}
};

// Misc Types

struct ProccessInfo
{
    int processID;              // the ID of the process
    void* nativeHandle;         // mainly used in windows
    std::string processName;    // the name of the process
};

extern std::ostream& operator<<(std::ostream& os, const OSEvent& event);
extern std::ostream& operator<<(std::ostream& os, const NativeDisplay& event);

#endif