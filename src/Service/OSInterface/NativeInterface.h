#ifndef NATIVE_INTERFACE_H
#define NATIVE_INTERFACE_H
#include "OSInterface.h"
#include "OSTypes.h"
#include <list>
/*
    So whatever this OS requires for starting up and being able to control the mouse.
    i.e. on windows it creates an invisble window giving us access to setcursorpos etc...

    returns on Success or OS specific error on failure
*/
extern int StartupOSConnection();
/*
    Essentially undoes everthing StartupOSConnection does.

    returns on Success or OS specific error on failure 
*/
extern int ShutdownOSConnection();
/*
    returns 0 on success and sets the exit code of the process with id {processID} into
    {*exitCode}

    returns on Success or OS specific error on failure    
*/
extern int GetProcessExitCode(int processID, unsigned long* exitCode);
/*
    Checks if process with ID {processID} is running and sets {*isActive} to true if it is or false otherwise

    returns 0 on success or an OS specific error on failure
*/
extern int GetIsProcessActive(int processID, bool* isActive);
/*
    Starts a process as a user with access to the desktop
    This is usually the actively logged in user.

    {process} is the name of the process (the path to the executable)
    {args} is the cmd arguments passed to the program
    {workingDir} is the current working dir for the spawned process
    {isVisible} is used for flags on wether or not to show the window of the program. This may be ignored by the OS
    {processInfo} is information about the spawned process on success or undefined on failure

    Returns 0 on Soccess or a OS specific error on failure
*/
extern int StartProcessAsDesktopUser(std::string process, std::string args, std::string workingDir,bool isVisible, ProccessInfo* processInfo);
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
extern void OSMainLoop(bool& shouldRun);
/*
    Release all event hooks using native OS calls
*/
extern void NativeUnhookAllEvents();
/*
    Hides the mouse if {isHidden} is true of unhides the mouse otherwise

    return 0 on success or native error if fails
*/
extern int SetMouseHidden(bool isHidden);
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
    Retreives all valid IP address that are not the  loop back from the OS.
    the contents of outAddresses is filled with the address but it is not cleared
    so new addresses are just appended to the list

    returns a SocketError
 */
extern int GetIPAddressList(std::vector<IPAdressInfo>& outAddresses, const IPAdressInfoHints& hints);
/*
    Converts inEvent posX,posY,minX,minY,maxX and maxY to a native counterpart so that
    SendMouseEvent will send the event to the correct position

    this will always return 0 
*/
extern int ConvertEventCoordsToNative(const OSEvent inEvent, OSEvent& outEvent);
/*
    Sets the mouse position to {x,y} in desktop coords 
    (in other words two monitor setup coords would be both monitors together)

    return 0 on success or an OS specific error on failure
*/
extern int SetMousePosition(int x,int y);
/*
    Returns an OS error or 0 on success

    Gets the mouse position and stores it in {xPos} and {yPos}
*/
extern int GetMousePosition(int& xPos, int& yPos);
/*
    Returns an OS error or 0 on success

    Gets the mouse position normalized to the virtual screen and stores it in {xPos} and {yPos}
*/
extern int GetNormalMousePosition(float xPos, float yPos);

/*
    Checks if the point {x,y} is  within +/- {xLimit} of the global screen coords or
    within +/- {yLimit} of the global screen coords. For example, if {xLimit} is 10, then
    if {x} is within 10 of either the far left or far right of the screen it will set reult to true
    
    {x} x location in global screen coords
    {y} y location in global screen coords
    
    {xLimit} x limit in global screen coords to be considered the "Edge" of the screen.
    {yLimit} x limit in global screen coords to be considered the "Edge" of the screen.
    
    {result} set to true if either {x} or {y} is within the {xLimit} or {yLimit} of the screen.

    return 0 on success or an OS error on failure
*/
extern int GetPointIsAtEdgeOfGlobalScreen(int x, int y, int xLimit, int yLimit, bool& result);
/*
    Gets the local computers Clipboard data.

    return 0 on success or OS Specific error on failure
*/
extern int GetClipBoard(ClipboardData& outData);
/*
    Set local Clipboard Data

    returns 0 on success or OS Specific error on failure
*/
extern int SetClipBoard(const ClipboardData& data);
/*
    Gets the local computer host name and stores it in {hostName}

    Returns OS error or 0 on success
*/
extern int GetHostName(std::string& hostName);
/*
    this tries to convert OSError to an OSInterfaceError.
    
    if there is no equilvelent OSInterfaceError OS_E_UNKOWN is returned 
*/
extern OSInterfaceError OSErrorToOSInterfaceError(int OSError);
#endif
