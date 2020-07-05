#include "CCBroadcastManager.h"

#include "CCLogger.h"

#include "../Socket/SocketError.h"
#include "../Socket/Socket.h"

#include "CCPacketTypes.h"

CCBroadcastManager::CCBroadcastManager() : _shouldBroadcast(false)
{
}

CCBroadcastManager::CCBroadcastManager(std::string broadcastAddress, int broadcastPort) :
_internalSocket(new Socket(broadcastAddress, broadcastPort, false,  SocketProtocol::SOCKET_P_UDP)), _shouldBroadcast(false)
{
}

bool CCBroadcastManager::BroadcastNow(std::string serverAddress, int serverPort)
{
	_internalSocket->SetIsBroadcastable(true);

	AddressPacket addressPacket;

	memcpy(addressPacket.Address, serverAddress.c_str(), serverAddress.length());
	addressPacket.Address[serverAddress.length()] = 0;
	addressPacket.Port = serverPort;
	
	//std::cout << "Broadcasting Address As {" << addressPacket.Address << "," << addressPacket.Port << "}" << std::endl;
	SocketError error = _internalSocket->SendTo(&addressPacket, sizeof(addressPacket));

	if (error != SocketError::SOCKET_E_SUCCESS)
	{
		LOG_ERROR << "Error trying to send Broadcast " << SockErrorToString(error) << std::endl;
		return false;
	}

	return true;
}

void CCBroadcastManager::StopBroadcasting()
{
	_shouldBroadcast = false;
}

bool CCBroadcastManager::ListenForBroadcasts(BServerAddress* foundAddress)
{
	if (_internalSocket->GetIsBound() == false)
	{
		SocketError error = _internalSocket->Bind();

		if (error != SocketError::SOCKET_E_SUCCESS)
		{
			LOG_ERROR << "Error Bind Client Broadcast Socket: " << SOCK_ERR_STR(_internalSocket.get(), error) << std::endl;
			return false;
		}
	}

	size_t received = 0;
	AddressPacket addressPacket;
	SocketError err = _internalSocket->Recv((char*)&addressPacket, sizeof(AddressPacket), &received);

	if (err == SocketError::SOCKET_E_SUCCESS && received == sizeof(AddressPacket))
	{
		if (addressPacket.MagicNumber != P_MAGIC_NUMBER)
		{
			return false;
		}

		(*foundAddress).first = addressPacket.Address;
		(*foundAddress).second = addressPacket.Port;

		return true;
	}
	else
	{
		// there was some error and any reasonable person would do something about that
	}

	return false;
}
