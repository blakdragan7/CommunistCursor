#ifndef CC_PACKET_TYPES_H
#define CC_PACKET_TYPES_H

#define P_MAGIC_NUMBER 48884003

#define INVALID_PACKET_ADDRESS "DONT USE"
#define INVALID_PACKET_ADDRESS_SIZE 8

// Magic Numbers are used to ensure entegrety of Packets

/*
 * AddressPacket simply is a serialzed version of some address and port.
 * TODO Address should be the 16 bit version of address that sockets use instead for efficiency
 */

struct AddressPacket
{
	unsigned int MagicNumber;
	char Address[16];
	int Port;

	AddressPacket() : MagicNumber(P_MAGIC_NUMBER), Address("invalid"), Port(0)
	{}
};

/*
 * EntityIDPacket is a packet to receive the ID of the computer connecting to the server
 * The EntityID is generally the host name of the computer
 * Note: if the EntityID is greater then 256 bytes it will be truncated 
 */

struct EntityIDPacket
{
	unsigned int MagicNumber;
	size_t IDLength;
	char EntityID[256];
	EntityIDPacket() : MagicNumber(P_MAGIC_NUMBER), IDLength(0) {}
	EntityIDPacket(std::string id) : MagicNumber(P_MAGIC_NUMBER), IDLength(id.size()) { memset(EntityID,0,256); strcpy_s(EntityID, id.c_str()); }
};

/*
 * DisplayListHeaderPacket is an informative packet to let the server know how many
 * DisplayListDisplayPackets are comming next
 */

struct DisplayListHeaderPacket
{
	unsigned int MagicNumber;
	int NumberOfDisplays;
	DisplayListHeaderPacket() : MagicNumber(P_MAGIC_NUMBER), NumberOfDisplays(0) {}
};

/*
 * DisplayListDisplayPacket represents a display on a client machine
 * This packet will continue to grow in size as more info is needed about displays
 * Note: NativeDisplayID may not be valid as the OS from the client might not assign unique IDs to displays
 * or at least not make them visible to an end user
 */

struct DisplayListDisplayPacket
{
	unsigned int MagicNumber;
	int NativeDisplayID; // equivelent to NativeScreenID from OSInterface/PacketTypes.h
	int Width, Height;
	int Top, Left;
	DisplayListDisplayPacket() : MagicNumber(P_MAGIC_NUMBER), NativeDisplayID(-1), Width(-1), Height(-1),
		Top(-1), Left(-1)
	{}
};

// RPC Packets

struct NETCPPacketHeader
{
	unsigned int MagicNumber;
	unsigned char Type;

	NETCPPacketHeader() : MagicNumber(P_MAGIC_NUMBER), Type(0) {}
	NETCPPacketHeader(unsigned char Type) : MagicNumber(P_MAGIC_NUMBER), Type(Type) {}
};

struct NERPCSetMouseData
{
	float x;
	float y;
	unsigned int MagicNumber;
	NERPCSetMouseData() : MagicNumber(P_MAGIC_NUMBER), x(0), y(0) {}
	NERPCSetMouseData(float x, float y) : MagicNumber(P_MAGIC_NUMBER), x(x), y(y) {}
};

struct NETCPPacketAwk
{
	unsigned int MagicNumber;
	NETCPPacketAwk() : MagicNumber(P_MAGIC_NUMBER) {}
};

#endif