#ifndef NATIVE_INTERFACE_H
#define NATIVE_INTERFACE_H
#include "OSInterface.h"
#include <vector>
/*
    Registers OSInterface osi with key and mouse events on the OS using native methods
    this calles UpdateThread of the osi passed with a generate OSEvent from the native events
    received from the OS

    this will return 0 if completed succesfully or a native error if it failes 
*/
extern int NativeRegisterForOSEvents(OSInterface* osi);
/*
    Runs the main loop based on OS, i.e. Windows runs a Message loop
*/
extern void OSMainLoop(bool& stopSwitch);
/*
    Release all event hooks using native OS calls
*/
extern void NativeUnhookAllEvents();
/*
    Injects a Mouse Event using Native OS APIs

    this will return 0 if completed succesfully or a native error if it failes 
*/
extern int SendMouseEvent(const OSEvent mouseEvent);
/*
    This injects a keyboard event using the Native OS API

    this will return 0 if completed succesfully or a native error if it failes 
*/
extern int SendKeyEvent(const OSEvent keyEvent);
/* 
    Retreives all active Displays connected to the main virtual monitor of this computer
 
    the contents of outDisplays is filled with the display information
 
    Note. outDisplays is only appended to, so it's previous contents will be left alone.
 
    returns an OS defined error code if fails or 0 if succesful
*/
extern int GetAllDisplays(std::vector<NativeDisplay>& outDisplays);
/*
    Converts inEvent posX,posY,minX,minY,maxX and maxY to a native counterpart so that
    SendMouseEvent will send the event to the correct position

    this will always return 0 
*/
extern int ConvertEventCoordsToNative(const OSEvent inEvent, OSEvent& outEvent);
/*
    this tries to convert OSError to an OSInterfaceError.
    
    if there is no equilvelent OSInterfaceError OS_E_UNKOWN is returned 
*/
extern OSInterfaceError OSErrorToOSInterfaceError(int OSError);
#endif
