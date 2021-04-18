#ifndef CC_GUI_INTERFACE_H
#define CC_GUI_INTERFACE_H

#include <memory>
#include <string>
#include <vector>
#include <thread>

class Socket;
class IGuiServiceInterface;
class CCGuiService
{
private:
	std::unique_ptr<Socket> _serverSocket;

	IGuiServiceInterface*	_delegate;

	bool					_shouldRunServer;

	int						_guiServerPort;
	std::string				_guiServerAddress;
private:
	void SocketAcceptThread();

public:
	CCGuiService(IGuiServiceInterface* _delegate, int guiPort = 10049, std::string address = "127.0.0.1");

	bool StartGUIServer();
	bool StopGUIServer();

	bool StartGuiClient()const;
};

#endif