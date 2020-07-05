#ifndef PACKET_TYPES_H
#define PACKET_TYPES_H
#include "OSTypes.h"

enum class EventPacketType : unsigned char
{
    MouseMove = 0,
    MouseButton = 1,
    MouseWheel = 2,
    Key = 3,
    INVALID = 255
};

/* This file includes all type definitions for Packets that are sent accross the network */

struct OSInputEventPacket
{
    union {
        int16_t posX;
        int16_t wheelData;

        uint16_t mouseButton;
        uint16_t scancode;
        uint16_t data1;
    };
    union {
        int16_t posY;
        uint16_t isDown;
        uint16_t data2;
    };
    union {
        int16_t deltaX;
        int16_t data3;
    };
    union {
        int16_t deltaY;
        int16_t data4;
    };

    uint16_t nativeScreenID;
    EventPacketType eventType;

    OSInputEventPacket();
    OSInputEventPacket(const OSEvent& event);

    OSEvent AsOSEvent()const;
};

#endif
