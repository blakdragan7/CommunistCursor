#ifndef PACKET_TYPES_H
#define PACKET_TYPES_H
#include "OSTypes.h"

/* This file includes all type definitions for Packets that are sent accross the network */

enum EventPacketType
{
    EVENT_PACKET_MM = 0,
    EVENT_PACKET_MB = 1,
    EVENT_PACKET_MW = 2,
    EVENT_PACKET_K = 3,
    EVENT_PACKET_INVALID
};

struct EventPacketHeader
{
    uint8_t incomming_event_type; 
    EventPacketHeader();
    EventPacketHeader(EventPacketType type);
};

struct MouseMoveEventPacket
{
    uint16_t posX;
    uint16_t posY;
    uint16_t nativeScreenID;

    MouseMoveEventPacket();
    MouseMoveEventPacket(const OSEvent& event);
};

struct MouseButtonEventPacket
{
    uint8_t mouseButton;
    uint8_t isDown;
    MouseButtonEventPacket();
    MouseButtonEventPacket(const OSEvent& event);
};

struct MouseWheelEventPacket
{
    int8_t wheel_data;
    MouseWheelEventPacket();
    MouseWheelEventPacket(const OSEvent& event);
};

struct KeyEventPacket
{
    // only down or up
    uint8_t keyEvent;
    uint16_t scancode;
    KeyEventPacket();
    KeyEventPacket(const OSEvent& event);
};

#endif
