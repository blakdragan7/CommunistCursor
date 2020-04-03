#include "Socket.h"

int main(int argc, char* argv[])
{
    Socket socket(SOCKET_P_UDP);

    socket.Send(std::string("stuff"));
    socket.Send((void*)"stuff", 6);

    return 0;
}