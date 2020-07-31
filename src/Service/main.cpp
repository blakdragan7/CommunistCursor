#include <iostream>
#include <args.hxx>

#include <string>

#include "Socket/Socket.h"
#include "Socket/SocketException.h"
#include "OSInterface/OSInterface.h"
#include "OSInterface/NativeInterface.h"
#include "OSInterface/IOSEventReceiver.h"
#include "OSInterface/OSTypes.h"

#include "CC/CCLogger.h"

#include "CC/CCMain.h"
#include "CC/CCDisplay.h"
#include "CC/CCNetworkEntity.h"
#include "CC/CCConfigurationManager.h"
#include "CC/CCLogger.h"

#include "Dispatcher/DispatchManager.h"

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

class Printer : public IOSEventReceiver
{
    bool ReceivedNewInputEvent(OSEvent event)override
    {
        if(event.eventType == OS_EVENT_KEY && event.keyEvent == KEY_EVENT_DOWN)
            std::cout << event.scanCode << std::endl;

        return event.eventType == OS_EVENT_KEY;
    }
};

int main(int argc, char* argv[])
{
    Socket::OSSocketStartup();

    int res = ParaseArguments(argc,argv);

    if(res < 0)
    {
        LOG_INFO << "Starting Communist Cursor\n";

        if (res == -1)
        {
            LOG_INFO << "Starting Server\n";
            CCMain main;
            main.LoadAll();
            main.StartServerMain();
        }
        else if (res == -2)
        {
            LOG_INFO << "Starting Client\n";
            CCMain main;
            main.LoadAll();
            main.StartClientMain();
        }
        else if (res == -3)
        {
            LOG_INFO << "Starting in Print Mode" << std::endl;
 
            Printer printer;

            OSInterface::SharedInterface().RegisterForOSEvents(&printer);

            OSInterface::SharedInterface().OSMainLoop();
        }
        else
        {
            LOG_ERROR << "Invalid return or error res: " << res << std::endl;
            shouldPause = true;
        }
    }

    if(shouldPause)
    {
        LOG_INFO << "waiting for input:";
        std::string val;
        std::cin >> val;
    }

    Socket::OSSocketTeardown();

    SHUTDOWN_DISPATCHER;

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
    args::Command runServer(commandGroup, "runServer", "Run in server mode.");
    args::Command runClient(commandGroup, "runClient", "Run in client mode.");
    args::Command printKeyCodes(commandGroup, "printKeyCodes", "Hook keyboard events and print virtual keys generated from those events");
    //args::Command installService(commandGroup, "installService", "Install as a service");
    args::Group arguments(parser, "arguments", args::Group::Validators::DontCare, args::Options::Global);
    args::ValueFlag<std::string> logLevel(arguments, "logLevel", "Sets the log level for the program valid options (VERBOSE, DEBUG, INFO, WARN, ERROR, NONE)", { 'l' });
    args::ValueFlag<unsigned int> numThreads(arguments, "threads", "Sets the number of threads in the pool. Must be > 0", {'t'});
    args::Flag isServer(arguments, "isServer", "Set to Server mode for socket test", { 's', "server" });
    args::Flag shouldPause(arguments, "shouldPause", "Pauses at tend of execution", {'p', "pause"});

    try
    {
        parser.ParseCLI(argc,argv);

        ::shouldPause = shouldPause;

        if (numThreads)
        {
            INIT_DISPATCHER_WITH_THREAD_COUNT(args::get(numThreads));
        }
        else
        {
            INIT_DISPATCHER;
        }

        if (logLevel)
        {
            LOG_INFO << "Setting Log Level To " << args::get(logLevel) << std::endl;
            CCLogger::logger.SetLogLevel(args::get(logLevel));
        }
        else
        {
            LOG_INFO << "Setting Log Level To " << CCLogger::logger.GetLogLevelAsString() << std::endl;
        }

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
        /*else if(installService)
        {
            // install service
            return 0;
        }*/
        else if(runServer)
        {
            return -1;
        }
        else if (runClient)
        {
            return -2;
        }
        else if (printKeyCodes)
        {
            return -3;
        }
    }
    catch (args::Help)
    {
        LOG_INFO << parser;
        return 0;
    }
    catch (args::ParseError e)
    {
        ::shouldPause = true;
        LOG_ERROR << e.what() << std::endl;
        LOG_ERROR << parser;
        return 1;
    }
    catch (args::ValidationError e)
    {
        ::shouldPause = true;
        LOG_ERROR << e.what() << std::endl;
        LOG_ERROR << parser;
        return 1;
    }

    return 0;
    
}

int MouseMoveTest()
{
    LOG_INFO << "MouseMoveTest" << std::endl;
    
    OSInterface& osi = OSInterface::SharedInterface();
    OSEvent event;

    event.eventType = OS_EVENT_MOUSE;
    event.mouseEvent = MOUSE_EVENT_MOVE;

    event.x = 20;
    event.y = 20;

    auto error = osi.SendMouseEvent(event);
    if(error != OSInterfaceError::OS_E_SUCCESS)
    {
        LOG_ERROR << "Error Sending event " << event << " with error:" << OSInterfaceErrorToString(error);
        return 0;
    }

    return 0;
}

int KeyTest()
{
    LOG_INFO << "KeyTest" << std::endl;
    
    OSInterface& osi = OSInterface::SharedInterface();
    
    OSEvent event;
    event.eventType = OS_EVENT_KEY;
    event.keyEvent = KEY_EVENT_DOWN;

    event.scanCode = 20;

    //std::cin.get();

    auto error = osi.SendKeyEvent(event);
    if(error != OSInterfaceError::OS_E_SUCCESS)
    {
        LOG_ERROR << "Error Sending event " << event << " with error:" << OSInterfaceErrorToString(error);
        return 0;
    }

   // std::cin.get();

    event.keyEvent = KEY_EVENT_UP;
    error = osi.SendKeyEvent(event);
    if(error != OSInterfaceError::OS_E_SUCCESS)
    {
        LOG_ERROR << "Error Sending event " << event << " with error:" << OSInterfaceErrorToString(error);
    }

    return 0;
}

int EventTest()
{
    LOG_INFO << "EventTest" << std::endl;
    
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
    LOG_INFO << "SocketTest" << std::endl;
    

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
            LOG_ERROR << "Error connecting to server " << SOCK_ERR_STR(&socket, e);
            break;
        }

        e = socket.Send(toSend);
        if(e != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error sending from client " << SOCK_ERR_STR(&socket, e);
            break;
        }

        e = socket.WaitForServer();
        if(e != SocketError::SOCKET_E_SUCCESS)
        {
            LOG_ERROR << "Error sending from client " << SOCK_ERR_STR(&socket, e);
            break;
        }
    }
        break;
    case 's':
    {
            SocketError e = socket.Bind("127.0.0.1");
            if(e != SocketError::SOCKET_E_SUCCESS)
            {
                LOG_ERROR << "Error binding socket " << SOCK_ERR_STR(&socket, e);
                break;
            }
            e = socket.Listen();
            if(e != SocketError::SOCKET_E_SUCCESS)
            {
                LOG_ERROR << "Error listening on socket " << SOCK_ERR_STR(&socket, e);
                break;
            }
            Socket* client = 0;
            e = socket.Accept(&client);
            if(e != SocketError::SOCKET_E_SUCCESS)
            {
                LOG_ERROR << "Error accepting client " << SOCK_ERR_STR(client, e);
                break;
            }
            char buf[256] = {0};
            size_t received = 0;
            e = client->Recv(buf,sizeof(buf), &received);
            if(e != SocketError::SOCKET_E_SUCCESS)
            {
                LOG_ERROR << "Error recieving from client " << SOCK_ERR_STR(client, e);
            }
            else
            {
                buf[received] = 0;
                LOG_INFO << "Server Received {" << buf << "} From Client !" << std::endl;
            }
            
            delete client;
        }
        break;
    }


    return 0;
}
