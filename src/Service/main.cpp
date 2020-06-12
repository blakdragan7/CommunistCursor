#include <iostream>
#include <args.hxx>

#include <string>

#include "Socket/Socket.h"
#include "Socket/SocketException.h"
#include "OSInterface/OSInterface.h"
#include "OSInterface/NativeInterface.h"
#include "OSInterface/IOSEventReceiver.h"

#include "OSInterface/OSTypes.h"

#include "CC/CCMain.h"

class TestEventReceiver : public IOSEventReceiver
{
    virtual bool ReceivedNewInputEvent(const OSEvent event)override
    {
        std::cout << event << std::endl;
        return false;
    }
};

int EventTest();
int SocketTest(bool isServer);
int KeyTest();
int MouseMoveTest();
int ParaseArguments(int argc, char* argv[]);

bool shouldPause = false;

int main(int argc, char* argv[])
{
    Socket::OSSocketStartup();

    int res = ParaseArguments(argc,argv);

    if(res < 0)
    {
        std::cout << "Starting Communist Cursor\n";

        CCMain main;

        if (res == -1)
        {
            std::cout << "Starting Server\n";
            main.StartServerMain();
        }
        else if (res == -2)
        {
            std::cout << "Starting Client\n";
            main.StartClientMain();
        }
        else
        {
            std::cout << "Invalid return or error res: " << res << std::endl;
            shouldPause = true;
        }
    }

    if(shouldPause)
    {
        std::cout << "waiting for input:";
        std::string val;
        std::cin >> val;
    }

    Socket::OSSocketTeardown();

    return 0;
}

int ParaseArguments(int argc, char* argv[])
{
    args::ArgumentParser parser("CommunistCursor shares the Mouse and Keyboard input between multiple computers", "");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::Group commandGroup(parser, "commands");
    args::Command testSocket(commandGroup, "test-socket", "perform a simple socket test, if -s is specified it will run as a server");
    args::Command testEvent(commandGroup, "test-event", "perform event hooking tests, outputs all events found to stdout");
    args::Command testKey(commandGroup, "test-key", "perform key injection test, will inject scan code 20 into the OS");
    args::Command testMouseMove(commandGroup, "test-mousemove", "Perform mouse injection tests, will move mouse to random location on screen");
    args::Command run(commandGroup, "run", "Run in standard mode.");
    args::Command iservice(commandGroup, "service", "Install as a service");
    args::Group arguments(parser, "arguments", args::Group::Validators::DontCare, args::Options::Global);
    args::Flag isServer(arguments, "isServer", "forces program to run in server mode, can me used in {run} and {teset-socket} commands", {'s', "server"});
    args::Flag shouldPause(arguments, "shouldPause", "Pauses at tend of execution", {'p', "pause"});

    try
    {
        parser.ParseCLI(argc,argv);

        ::shouldPause = shouldPause;

        if(testSocket)
        {
            return SocketTest(isServer);
        }
        else if(testKey)
        {
            return KeyTest();
        }
        else if(testMouseMove)
        {
            return MouseMoveTest();
        }
        else if(testEvent)
        {
            return EventTest();
        }
        else if(iservice)
        {
            // install service
            return 0;
        }
        else if(run)
        {
            // perform standard operation
            return isServer ? -1 : -2;
        }
    }
    catch (args::Help)
    {
        std::cout << parser;
        return 0;
    }
    catch (args::ParseError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    catch (args::ValidationError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    return 0;
    
}

int MouseMoveTest()
{
    std::cout << "MouseMoveTest" << std::endl;
    
    OSInterface& osi = OSInterface::SharedInterface();
    OSEvent event;

    event.eventType = OS_EVENT_MOUSE;
    event.subEvent.mouseEvent = MOUSE_EVENT_MOVE;

    event.deltaX = 20;
    event.deltaY = 20;

    auto error = osi.SendMouseEvent(event);
    if(error != OSInterfaceError::OS_E_SUCCESS)
    {
        std::cout << "Error Sending event " << event << " with error:" << OSInterfaceErrorToString(error);
        return 0;
    }

    return 0;
}

int KeyTest()
{
    std::cout << "KeyTest" << std::endl;
    
    OSInterface& osi = OSInterface::SharedInterface();
    
    OSEvent event;
    event.eventType = OS_EVENT_KEY;
    event.subEvent.keyEvent = KEY_EVENT_DOWN;

    event.eventButton.scanCode = 20;

    //std::cin.get();

    auto error = osi.SendKeyEvent(event);
    if(error != OSInterfaceError::OS_E_SUCCESS)
    {
        std::cout << "Error Sending event " << event << " with error:" << OSInterfaceErrorToString(error);
        return 0;
    }

   // std::cin.get();

    event.subEvent.keyEvent = KEY_EVENT_UP;
    error = osi.SendKeyEvent(event);
    if(error != OSInterfaceError::OS_E_SUCCESS)
    {
        std::cout << "Error Sending event " << event << " with error:" << OSInterfaceErrorToString(error);
    }

    return 0;
}

int EventTest()
{
    std::cout << "EventTest" << std::endl;
    
    OSInterface& osi = OSInterface::SharedInterface();
    TestEventReceiver receiver;

    void* someData = (void*)10;
    osi.RegisterForOSEvents(&receiver);
    osi.OSMainLoop();
    osi.UnRegisterForOSEvents(&receiver);

    return 0;
}

int SocketTest(bool isServer)
{
    std::cout << "SocketTest" << std::endl;
    

    std::string toSend = "Hello !";

    char type = isServer ? 's' : 'c';

    Socket socket("0.0.0.0", 6555, false, SocketProtocol::SOCKET_P_TCP);

    switch(type)
    {
    case 'c':
    {
        SocketError e = socket.Connect("127.0.0.1");
        if(e != SocketError::SOCKET_E_SUCCESS)
        {
            SocketException ex(e, socket.lastOSErr);
            std::cout << "Error connecting to server " << ex.what();
            break;
        }

        e = socket.Send(toSend);
        if(e != SocketError::SOCKET_E_SUCCESS)
        {
            SocketException ex(e, socket.lastOSErr);
            std::cout << "Error sending from client " << ex.what();
            break;
        }

        e = socket.WaitForServer();
        if(e != SocketError::SOCKET_E_SUCCESS)
        {
            SocketException ex(e, socket.lastOSErr);
            std::cout << "Error sending from client " << ex.what();
            break;
        }
    }
        break;
    case 's':
    {
            SocketError e = socket.Bind("127.0.0.1");
            if(e != SocketError::SOCKET_E_SUCCESS)
            {
                SocketException ex(e, socket.lastOSErr);
                std::cout << "Error binding socket " << ex.what();
                break;
            }
            e = socket.Listen();
            if(e != SocketError::SOCKET_E_SUCCESS)
            {
                SocketException ex(e, socket.lastOSErr);
                std::cout << "Error listening on socket " << ex.what();
                break;
            }
            Socket* client = 0;
            e = socket.Accept(&client);
            if(e != SocketError::SOCKET_E_SUCCESS)
            {
                SocketException ex(e, socket.lastOSErr);
                std::cout << "Error accepting client " << ex.what();
                break;
            }
            char buf[256] = {0};
            size_t received = 0;
            e = client->Recv(buf,sizeof(buf), &received);
            if(e != SocketError::SOCKET_E_SUCCESS)
            {
                SocketException ex(e, client->lastOSErr);
                std::cout << "Error recieving from client " << ex.what();
            }
            else
            {
                buf[received] = 0;
                std::cout << "Server Received {" << buf << "} From Client !" << std::endl;
            }
            
            delete client;
        }
        break;
    }


    return 0;
}
