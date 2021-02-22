#ifndef _SHARED_NET_H
#define _SHARED_NET_H

#include <RakNet/RakPeerInterface.h>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakNetTypes.h>


#include <RakNet/RakString.h>
#include <RakNet/BitStream.h>
#include <RakNet/GetTime.h>

#include <memory>
#include <vector>
#include <string>

#include "gpro-net-gamestate.h"

enum GameMessageID
{
	ID_PACKAGED_PACKET = ID_USER_PACKET_ENUM + 1,
	ID_DISPLAY_NAME_UPDATED,
	ID_PLAYER_MOVE,
	ID_PLAYER_CHAT,
	ID_GAME_START,
	ID_ACTIVE_PLAYER_ORDER
};

class TimestampMessage;
class PlayerMoveMessage;
class DisplayNameChangeMessage;


class NetworkMessage
{
protected:
	const RakNet::MessageID m_MessageID;

	NetworkMessage(RakNet::MessageID id) : m_MessageID(id) {}
public:
	virtual bool WritePacketBitstream(RakNet::BitStream* bs) = 0; 
	virtual bool ReadPacketBitstream(RakNet::BitStream* bs) = 0; //all messages should assume the messageid has already been read in and the read index is moved past it


	//We push back each message onto the msgQueue
	//NOTE! All messages will be dynamically allocated
	static void DecypherPacket(RakNet::BitStream* bs, std::vector<NetworkMessage*>& msgQueue);
	static void CreatePacketHeader(RakNet::BitStream* bs, int msgCount);


	//friend RakNet::BitStream& operator<<(RakNet::BitStream& bsp, NetworkMessage& msg);
};

//Used for messages that are more events that have no information tied to them, connecting, disconnect, all that jazz
class NotificationMessage : public NetworkMessage
{
public:
	NotificationMessage(RakNet::MessageID id) : NetworkMessage(id) { }

	//these do nothing as the notification message stores the messageID and nothing more
	bool WritePacketBitstream(RakNet::BitStream* bs) override { bs->Write(m_MessageID); return true; }
	bool ReadPacketBitstream(RakNet::BitStream* bs) override { return true; }
};


//Holds the timestamp, may need to be moved to the packet header
class TimestampMessage : public NetworkMessage
{
	RakNet::Time m_Time;

public:
	TimestampMessage() : NetworkMessage((RakNet::MessageID)ID_TIMESTAMP), m_Time(RakNet::GetTime()) { }

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};



//Display Name Changed
class DisplayNameChangeMessage : public NetworkMessage
{
	RakNet::SystemAddress m_Sender;
	RakNet::RakString m_UpdatedDisplayName;

public:
	DisplayNameChangeMessage() : NetworkMessage((RakNet::MessageID)ID_DISPLAY_NAME_UPDATED) { }

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};


//Holds all information about the player move
class PlayerMoveMessage : public NetworkMessage
{
	RakNet::SystemAddress m_Player;
	int m_CardValue;

public:
	PlayerMoveMessage() : NetworkMessage((RakNet::MessageID)ID_PLAYER_MOVE), m_CardValue(0) { }

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};

class PlayerChatMessage : public NetworkMessage
{

	RakNet::SystemAddress m_Sender;
	RakNet::SystemAddress m_Receiver;
	std::string m_Message; //converts to RakNet::RakString and back
public:
	PlayerChatMessage() : NetworkMessage((RakNet::MessageID)ID_PLAYER_CHAT) { }

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};

class PlayerActiveOrderMessage : public NetworkMessage
{
	std::vector<RakNet::SystemAddress> m_ActivePlayers;
public:
	PlayerActiveOrderMessage() : NetworkMessage((RakNet::MessageID)ID_ACTIVE_PLAYER_ORDER) { }

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};



#endif