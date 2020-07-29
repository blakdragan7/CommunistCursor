#include "PacketTypes.h"

OSInputEventPacket::OSInputEventPacket() : data1(0), data2(0), data3(0), data4(0), nativeScreenID(-1), eventType(EventPacketType::INVALID)
{

}

OSInputEventPacket::OSInputEventPacket(const OSEvent& event) : data1(0), data2(0), data3(0), data4(0), nativeScreenID(event.nativeScreenID)
{
	switch (event.eventType)
	{
	case OS_EVENT_KEY:
		eventType = EventPacketType::Key;
		scancode = event.scanCode;
		isDown = event.keyEvent == KEY_EVENT_DOWN;
		break;
	case OS_EVENT_MOUSE:
		switch (event.mouseEvent)
		{
		case MOUSE_EVENT_DOWN:
		case MOUSE_EVENT_UP:
			mouseButton = event.mouseButton;
			isDown = event.mouseEvent == MOUSE_EVENT_DOWN;
			eventType = EventPacketType::MouseButton;
			break;
		case MOUSE_EVENT_MOVE:
			eventType = EventPacketType::MouseMove;
			deltaX = event.deltaX;
			deltaY = event.deltaY;
			posX = event.x;
			posY = event.y;
			break;
		case MOUSE_EVENT_SCROLL:
			eventType = EventPacketType::MouseWheel;
			wheelData = event.extendButtonInfo;
        case MOUSE_EVENT_INVALID:
                break;
		}
		break;
	case OS_EVENT_HID:
	default:
		break;
	}
}

OSEvent OSInputEventPacket::AsOSEvent() const
{
	OSEvent ret;

	switch (eventType)
	{
	case EventPacketType::Key:
		ret.eventType = OS_EVENT_KEY;
		ret.keyEvent = isDown ? KEY_EVENT_DOWN : KEY_EVENT_UP;
		ret.scanCode = scancode;
		break;
	case EventPacketType::MouseWheel:
		ret.eventType = OS_EVENT_MOUSE;
		ret.mouseEvent = MOUSE_EVENT_SCROLL;
		ret.extendButtonInfo = wheelData;
		break;
	case EventPacketType::MouseButton:
		ret.eventType = OS_EVENT_MOUSE;
		ret.mouseEvent = isDown ? MOUSE_EVENT_DOWN : MOUSE_EVENT_UP;
		ret.mouseButton = (MouseButton)mouseButton;
		break;
	case EventPacketType::MouseMove:
		ret.eventType = OS_EVENT_MOUSE;
		ret.mouseEvent = MOUSE_EVENT_MOVE;
		ret.deltaX = deltaX;
		ret.deltaY = deltaY;
		ret.x = posX;
		ret.y = posY;
		break;
    case EventPacketType::INVALID:
        break;
	}

	return ret;
}

OSClipboardDataPacketHeader::OSClipboardDataPacketHeader() : dataSize(0), dataType(0)
{
}

OSClipboardDataPacketHeader::OSClipboardDataPacketHeader(const ClipboardData& data) : dataSize((int)data.stringData.size()), dataType((int)data.type)
{
}
