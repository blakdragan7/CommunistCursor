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
        int32_t posX;
        int32_t wheelData;

        int32_t mouseButton;
        int32_t scancode;
        int32_t data1;
    };
    union {
        int32_t posY;
        int32_t isDown;
        int32_t data2;
    };
    union {
        int32_t deltaX;
        int32_t data3;
    };
    union {
        int32_t deltaY;
        int32_t data4;
    };

    uint16_t nativeScreenID;
    EventPacketType eventType;

    OSInputEventPacket();
    OSInputEventPacket(const OSEvent& event);

    OSEvent AsOSEvent()const;
};

// this is the first message in a Cliboard packet message. The next message
// will be just the raw data of the clipboard itself

struct OSClipboardDataPacketHeader
{
    unsigned int dataSize;
    uint8_t dataType;

    OSClipboardDataPacketHeader();
    OSClipboardDataPacketHeader(const ClipboardData& data);
};

#endif
