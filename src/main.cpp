#include <iostream>
#include "Socket.h"
#include "SocketException.h"
#include "OSInterface.h"

void Callback(OSEvent event, void* info);

int MouseTest(int argc, char* argv[]);
int SocketTest(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    return MouseTest(argc,argv);
}

int MouseTest(int argc, char* argv[])
{
    OSInterface& osi = OSInterface::SharedInterface();

    void* someData = (void*)10;
    osi.RegisterForOSEvents(Callback, someData);
    osi.OSMainLoop();
    osi.UnRegisterForOSEvents(someData);

    return 0;
}

int SocketTest(int argc, char* argv[])
{
    Socket::OSSocketStartup();

    std::string toSend = "Hello !";

    char type = 'c';

    if(argc > 1)
        type = argv[1][0];

    Socket socket("0.0.0.0", 6555, false, SOCKET_P_TCP);

    switch(type)
    {
    case 'c':
    {
        SocketError e = socket.Connect("127.0.0.1");
        if(e != SOCKET_E_SUCCESS)
        {
            SocketException ex(e, socket.lastOSErr);
            std::cout << "Error connecting to server " << ex.what();
            break;
        }

        e = socket.Send(toSend);
        if(e != SOCKET_E_SUCCESS)
        {
            SocketException ex(e, socket.lastOSErr);
            std::cout << "Error sending from client " << ex.what();
            break;
        }

        e = socket.WaitForServer();
        if(e != SOCKET_E_SUCCESS)
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
            if(e != SOCKET_E_SUCCESS)
            {
                SocketException ex(e, socket.lastOSErr);
                std::cout << "Error binding socket " << ex.what();
                break;
            }
            e = socket.Listen();
            if(e != SOCKET_E_SUCCESS)
            {
                SocketException ex(e, socket.lastOSErr);
                std::cout << "Error listening on socket " << ex.what();
                break;
            }
            Socket* client = 0;
            e = socket.Accept(&client);
            if(e != SOCKET_E_SUCCESS)
            {
                SocketException ex(e, socket.lastOSErr);
                std::cout << "Error accepting client " << ex.what();
                break;
            }
            char buf[256] = {0};
            size_t received = 0;
            e = client->Recv(buf,sizeof(buf), &received);
            if(e != SOCKET_E_SUCCESS)
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

    Socket::OSSocketTeardown();

    return 0;
}

void Callback(OSEvent event, void* info)
{
    std::cout << "New Event {" << event.posX << "," << event.posY << "} " << event.extendButtonInfo  << std::endl;
}