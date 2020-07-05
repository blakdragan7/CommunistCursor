#ifndef CC_BROADCASTER_H
#define CC_BROADCASTER_H

#include <memory>
#include <string>
/*
*  CCBroadcaster 
*
*  Used to broadcast the server address and port for server and receive the broadcast for server and port if client
*  Essentially hanlds all UDP messages related to discovery
*/

typedef std::pair<std::string, int> BServerAddress;

class Socket;
class CCNetworkEntity;
class CCBroadcastManager
{
private:
	std::unique_ptr<Socket>							_internalSocket;
	bool											_shouldBroadcast;

public:
	CCBroadcastManager();
	CCBroadcastManager(std::string broadcastAddress, int broadcastPort);

	// Broadcasts the {serverAddress} and {serverPort} to broadcastAddress on broadcastPort
	// returns false on failure to braodcast
	bool BroadcastNow(std::string serverAddress, int serverPort);
	// Stops broadscasting once refresh hits
	void StopBroadcasting();
	// blocks until broadcast of server address is received.
	// return value pair of address (string) and port (int) of server
	bool ListenForBroadcasts(BServerAddress* foundAddress);
	
	inline bool GetIsBroadcasting()const { return _shouldBroadcast; }
};

#endif