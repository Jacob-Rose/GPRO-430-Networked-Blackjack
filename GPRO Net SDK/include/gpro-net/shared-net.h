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
	ID_PLAYER_CARD_DRAWN,
	ID_PLAYER_JOIN_GAME_REQUEST,
	ID_PLAYER_CHAT,
	ID_PLAYER_SPECTATOR_CHOICE,
	ID_GAME_START,
	ID_ACTIVE_PLAYER_ORDER
};

class TimestampMessage;
class PlayerMoveMessage;
class DisplayNameChangeMessage;


struct BlackjackState
{
	struct PlayerState
	{
		RakNet::SystemAddress m_Address;
		std::vector<int> m_Cards;
	};
	std::vector<PlayerState> m_ActivePlayers;
	std::vector<int> m_DealerCards;
	std::vector<RakNet::SystemAddress> m_SpectatingPlayers; //no cards

	short m_CurrentPlayerTurn = 0;
};


class NetworkMessage
{
protected:
	NetworkMessage(RakNet::MessageID id) : m_MessageID(id) {}
public:
	const RakNet::MessageID m_MessageID;
	RakNet::SystemAddress m_Sender;


public:
	virtual bool WritePacketBitstream(RakNet::BitStream* bs) = 0; 
	virtual bool ReadPacketBitstream(RakNet::BitStream* bs) = 0; //all messages should assume the messageid has already been read in and the read index is moved past it


	//We push back each message onto the msgQueue
	//NOTE! All messages will be dynamically allocated
	static void DecypherPacket(RakNet::BitStream* bs, RakNet::SystemAddress sender, std::vector<NetworkMessage*>& msgQueue);
	static void CreatePacketHeader(RakNet::BitStream* bs, int msgCount);
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
public:
	RakNet::Time m_Time;

public:
	TimestampMessage() : NetworkMessage((RakNet::MessageID)ID_TIMESTAMP), m_Time(RakNet::GetTime()) { }

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};



//Display Name Changed
class DisplayNameChangeMessage : public NetworkMessage
{
public:
	RakNet::RakString m_UpdatedDisplayName;

public:
	DisplayNameChangeMessage() : NetworkMessage((RakNet::MessageID)ID_DISPLAY_NAME_UPDATED) { }

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};



class PlayerSpectatorChoiceMessage : public NetworkMessage
{
public:
	bool m_Spectating;
public:
	PlayerSpectatorChoiceMessage() : NetworkMessage((RakNet::MessageID)ID_PLAYER_SPECTATOR_CHOICE), m_Spectating(false) {}

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};


//Holds all information about the player move
class PlayerMoveMessage : public NetworkMessage
{
public:
	RakNet::SystemAddress m_Player;
	short m_PlayerMove; //0 -> Stay | 1-> Hit

public:
	PlayerMoveMessage() : NetworkMessage((RakNet::MessageID)ID_PLAYER_MOVE), m_PlayerMove(0) { }

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};

//mostly sent to players
class PlayerCardDrawnMessage : public NetworkMessage 
{
public:
	RakNet::SystemAddress m_Player; //UNASSINGED_SYSTEM_ADDRESS = SERVER
	short m_CardDrawn; 
public:
	PlayerCardDrawnMessage() : NetworkMessage((RakNet::MessageID)ID_PLAYER_CARD_DRAWN), m_CardDrawn(0) {}

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};



class PlayerChatMessage : public NetworkMessage
{
public:
	RakNet::SystemAddress m_Receiver;
	std::string m_Message; //converts to RakNet::RakString and back
public:
	PlayerChatMessage() : NetworkMessage((RakNet::MessageID)ID_PLAYER_CHAT) { }

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};

class PlayerJoinGameRequestMessage : public NetworkMessage
{
public:
	short m_GameIndex; //-1 = quit to lobby
public:
	PlayerJoinGameRequestMessage() : NetworkMessage((RakNet::MessageID)ID_PLAYER_JOIN_GAME_REQUEST), m_GameIndex(-1) { }

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};


//Send at start each time
class PlayerActiveOrderMessage : public NetworkMessage
{
public:
	std::vector<RakNet::SystemAddress> m_ActivePlayers;
	std::vector<RakNet::SystemAddress> m_SpectatingPlayers;
public:
	PlayerActiveOrderMessage() : NetworkMessage((RakNet::MessageID)ID_ACTIVE_PLAYER_ORDER) { }

	bool WritePacketBitstream(RakNet::BitStream* bs) override;
	bool ReadPacketBitstream(RakNet::BitStream* bs) override;
};



#endif