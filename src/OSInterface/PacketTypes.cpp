#include "PacketTypes.h"

EventPacketHeader::EventPacketHeader(EventPacketType type) :incomming_event_type(type) 
{}

MouseMoveEventPacket::MouseMoveEventPacket(const OSEvent& event) : posX(event.deltaX), posY(event.deltaY), nativeScreenID(event.nativeScreenID)
{}

MouseButtonEventPacket::MouseButtonEventPacket(const OSEvent& event) : \
mouseButton(event.eventButton.mouseButton), isDown(event.subEvent.mouseEvent == MOUSE_EVENT_DOWN)
{}

MouseWheelEventPacket::MouseWheelEventPacket(const OSEvent& event) : wheel_data(event.extendButtonInfo)
{}

KeyEventPacket::KeyEventPacket(const OSEvent& event) : \
keyEvent(event.subEvent.raw), scancode(event.eventButton.scanCode)
{}