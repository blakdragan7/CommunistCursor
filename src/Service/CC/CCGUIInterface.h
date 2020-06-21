#ifndef CC_GUI_INTERFACE_H
#define CC_GUI_INTERFACE_H

#include <memory>
#include <string>
#include <vector>

class Socket;
class CCNetworkEntity;
class CCGuiInterface
{
private:
	std::unique_ptr<Socket> _serverSocket;
	std::unique_ptr<Socket> _clientSocket;

public:
	CCGuiInterface(int guiPort, std::string address = "127.0.0.1");

	bool StartGUI(const std::vector<std::shared_ptr<CCNetworkEntity>>& entitesToConfigure, std::vector<int> globalBounds);
	bool WaitForConfigurationToFinish();
};

#endif