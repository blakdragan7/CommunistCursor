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
		eventType = (EventPacketType)htonl((u_long)EventPacketType::Key);
		scancode = htonl(event.scanCode);
		isDown = htonl(event.keyEvent == KEY_EVENT_DOWN);
		break;
	case OS_EVENT_MOUSE:
	{
		switch (event.mouseEvent)
		{
		case MOUSE_EVENT_DOWN:
		case MOUSE_EVENT_UP:
			mouseButton = htonl(event.mouseButton);
			isDown = htonl(event.mouseEvent == MOUSE_EVENT_DOWN);
			eventType = (EventPacketType)htonl((u_long)EventPacketType::MouseButton);
			break;
		case MOUSE_EVENT_MOVE:
			eventType = (EventPacketType)htonl((u_long)EventPacketType::MouseMove);
			deltaX = htonl(event.deltaX);
			deltaY = htonl(event.deltaY);
			posX = htonl(event.x);
			posY = htonl(event.y);
			break;
		case MOUSE_EVENT_SCROLL:
			eventType = (EventPacketType)htonl((u_long)EventPacketType::MouseWheel);
			wheelData = htonl(event.extendButtonInfo);
		case MOUSE_EVENT_INVALID:
			break;
		}
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

	EventPacketType leventType = (EventPacketType)ntohl((u_long)eventType);

	switch (leventType)
	{
	case EventPacketType::Key:
		ret.eventType = OS_EVENT_KEY;
		ret.keyEvent = ntohl(isDown) ? KEY_EVENT_DOWN : KEY_EVENT_UP;
		ret.scanCode = ntohl(scancode);
		break;
	case EventPacketType::MouseWheel:
		ret.eventType = OS_EVENT_MOUSE;
		ret.mouseEvent = MOUSE_EVENT_SCROLL;
		ret.extendButtonInfo = ntohl(wheelData);
		break;
	case EventPacketType::MouseButton:
		ret.eventType = OS_EVENT_MOUSE;
		ret.mouseEvent = ntohl(isDown) ? MOUSE_EVENT_DOWN : MOUSE_EVENT_UP;
		ret.mouseButton = (MouseButton)ntohl(mouseButton);
		break;
	case EventPacketType::MouseMove:
		ret.eventType = OS_EVENT_MOUSE;
		ret.mouseEvent = MOUSE_EVENT_MOVE;
		ret.deltaX = ntohl(deltaX);
		ret.deltaY = ntohl(deltaY);
		ret.x =  ntohl(posX);
		ret.y =  ntohl(posY);
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
