#include "PacketTypes.h"

EventPacketHeader::EventPacketHeader() : incomming_event_type(0)
{
}

EventPacketHeader::EventPacketHeader(EventPacketType type) :incomming_event_type(type)
{}

MouseMoveEventPacket::MouseMoveEventPacket() : posX(0), posY(0), nativeScreenID(-1)
{
}

MouseMoveEventPacket::MouseMoveEventPacket(const OSEvent& event) : posX(event.deltaX), posY(event.deltaY), nativeScreenID(event.nativeScreenID)
{}

MouseButtonEventPacket::MouseButtonEventPacket() : mouseButton(MOUSE_BUTTON_INVALID), isDown(false)
{
}

MouseButtonEventPacket::MouseButtonEventPacket(const OSEvent& event) : \
mouseButton(event.eventButton.mouseButton), isDown(event.subEvent.mouseEvent == MOUSE_EVENT_DOWN)
{}

MouseWheelEventPacket::MouseWheelEventPacket() : wheel_data(0)
{
}

MouseWheelEventPacket::MouseWheelEventPacket(const OSEvent& event) : wheel_data(event.extendButtonInfo)
{}

KeyEventPacket::KeyEventPacket() : keyEvent(KEY_EVENT_INVALID), scancode(-1)
{
}

KeyEventPacket::KeyEventPacket(const OSEvent& event) : \
keyEvent(event.subEvent.raw), scancode(event.eventButton.scanCode)
{}