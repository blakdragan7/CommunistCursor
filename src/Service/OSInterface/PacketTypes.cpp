#include "PacketTypes.h"

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#endif
OSInputEventPacket::OSInputEventPacket() : data1(0), data2(0), data3(0), data4(0), nativeScreenID(-1), eventType(EventPacketType::INVALID)
{

}

OSInputEventPacket::OSInputEventPacket(const OSEvent& event) : data1(0), data2(0), data3(0), data4(0), nativeScreenID(htons(event.nativeScreenID))
{
	switch (event.eventType)
	{
	case OS_EVENT_KEY:
		eventType = (EventPacketType)htons((short)EventPacketType::Key);
		scancode = htons(event.scanCode);
		isDown = event.keyEvent == KEY_EVENT_DOWN;
		break;
	case OS_EVENT_MOUSE:
		switch (event.mouseEvent)
		{
		case MOUSE_EVENT_DOWN:
		case MOUSE_EVENT_UP:
			mouseButton = (MouseButton)htons((short)event.mouseButton);
			isDown = event.mouseEvent == MOUSE_EVENT_DOWN;
			eventType = (EventPacketType)htons((short)EventPacketType::MouseButton);
			break;
		case MOUSE_EVENT_MOVE:
			eventType = (EventPacketType)htons((short)EventPacketType::MouseMove);
			deltaX = htons(event.deltaX);
			deltaY = htons(event.deltaY);
			posX = htons(event.x);
			posY = htons(event.y);
			break;
		case MOUSE_EVENT_SCROLL:
			eventType = (EventPacketType)htons((short)EventPacketType::MouseWheel);
			wheelData = (int16_t)htonl(event.extendButtonInfo);
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
		ret.scanCode = ntohs(scancode);
		break;
	case EventPacketType::MouseWheel:
		ret.eventType = OS_EVENT_MOUSE;
		ret.mouseEvent = MOUSE_EVENT_SCROLL;
		ret.extendButtonInfo = ntohl(wheelData);
		break;
	case EventPacketType::MouseButton:
		ret.eventType = OS_EVENT_MOUSE;
		ret.mouseEvent = isDown ? MOUSE_EVENT_DOWN : MOUSE_EVENT_UP;
		ret.mouseButton = (MouseButton)ntohs(mouseButton);
		break;
	case EventPacketType::MouseMove:
		ret.eventType = OS_EVENT_MOUSE;
		ret.mouseEvent = MOUSE_EVENT_MOVE;
		ret.deltaX = ntohs(deltaX);
		ret.deltaY = ntohs(deltaY);
		ret.x =  ntohs(posX);
		ret.y =  ntohs(posY);
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
