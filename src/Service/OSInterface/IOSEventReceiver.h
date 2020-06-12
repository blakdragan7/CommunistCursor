#ifndef OS_EVENT_RECEIVER_H
#define OS_EVENT_RECEIVER_H

#include "OSTypes.h"

class IOSEventReceiver
{
public:

	/*
	 * Called when a new input event is received from the OS
	 *
	 * Return value determines if the event should be passed back to OS or blocked
	 * true means block false means let it pass through
	 */

	virtual bool ReceivedNewInputEvent(const OSEvent event) = 0;
	//virtual void RecveivedNewClipboardEvent(/* will be determined later */) = 0;
};

#endif