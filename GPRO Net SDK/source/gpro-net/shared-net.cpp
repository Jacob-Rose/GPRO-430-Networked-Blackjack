#include "gpro-net/shared-net.h"



bool DisplayNameChangeMessage::WritePacketBitstream(RakNet::BitStream* bs)
{
	bs->Write(m_MessageID);
	bs->Write(m_Sender);
	bs->Write(m_UpdatedDisplayName);
	return true;
}

bool DisplayNameChangeMessage::ReadPacketBitstream(RakNet::BitStream* bs)
{
	bs->Read(m_Sender);
	bs->Read(m_UpdatedDisplayName);
	return true;
}

bool PlayerMoveMessage::WritePacketBitstream(RakNet::BitStream* bs)
{
	bs->Write(m_MessageID);
	bs->Write(m_Player);
	bs->Write(m_PlayerMove);
	return true;
}

bool PlayerMoveMessage::ReadPacketBitstream(RakNet::BitStream* bs)
{
	bs->Read(m_Player);
	bs->Read(m_PlayerMove);
	return true;
}

void NetworkMessage::DecypherPacket(RakNet::BitStream* bs, RakNet::SystemAddress sender, std::vector<NetworkMessage*>& msgQueue)
{
	RakNet::MessageID id;
	bs->Read(id); //now for each message constructor, THE MESSAGE IS TRIMMED OFF THE FRONT, SO THE READ DOES NOT NEED TO ADDRESS THIS MESSAGE ID WE ALREADY KNOW
	//possibly read in id count
	int idCount = 0;
	if (id == ID_PACKAGED_PACKET)
	{
		bs->Read(idCount);
		bs->Read(id);
	}
	for (int i = 0; i < idCount; i++)
	{
		NetworkMessage* msg;
		switch (id) {
			case ID_TIMESTAMP:
			{
				msg = new TimestampMessage();
				break;
			}
			case ID_DISPLAY_NAME_UPDATED:
			{
				msg = new DisplayNameChangeMessage();
				break;
			}
			case ID_PLAYER_MOVE:
			{
				msg = new PlayerMoveMessage();
				break;
			}
			case ID_PLAYER_CARD_DRAWN:
			{
				msg = new PlayerCardDrawnMessage();
				break;
			}
			case ID_PLAYER_CHAT:
			{
				msg = new PlayerChatMessage();
				break;
			}
			case ID_ACTIVE_PLAYER_ORDER:
			{
				msg = new PlayerActiveOrderMessage();
				break;
			}
			default:
			{
				msg = new NotificationMessage(id);
				break;
			}
		}
		msg->m_Sender = sender;
		msg->ReadPacketBitstream(bs);
		msgQueue.push_back(msg);
	}
}

void NetworkMessage::CreatePacketHeader(RakNet::BitStream* bs, int msgCount)
{
	bs->Write((RakNet::MessageID)ID_PACKAGED_PACKET);
	bs->Write(msgCount);
}

bool TimestampMessage::WritePacketBitstream(RakNet::BitStream* bs)
{
	bs->Write(m_MessageID); //ID_TIMESTAMP
	bs->Write(m_Time);
	return true;
}

bool TimestampMessage::ReadPacketBitstream(RakNet::BitStream* bs)
{
	bs->Read(m_Time);
	return true;
}

bool PlayerChatMessage::WritePacketBitstream(RakNet::BitStream* bs)
{
	bs->Write(m_MessageID);
	bs->Write(m_Sender);
	bs->Write(m_Receiver);
	bs->Write(RakNet::RakString(m_Message.c_str()));
	return true;
}

bool PlayerChatMessage::ReadPacketBitstream(RakNet::BitStream* bs)
{
	bs->Read(m_Sender);
	bs->Read(m_Receiver);
	RakNet::RakString rakString;
	bs->Read(rakString);
	m_Message = rakString.C_String();
	return true;
}

bool PlayerSpectatorChoiceMessage::WritePacketBitstream(RakNet::BitStream* bs)
{
	return false;
}

bool PlayerSpectatorChoiceMessage::ReadPacketBitstream(RakNet::BitStream* bs)
{
	return false;
}

bool PlayerActiveOrderMessage::WritePacketBitstream(RakNet::BitStream* bs)
{
	bs->Write(m_MessageID);
	bs->Write(m_ActivePlayers.size());
	for (int i = 0; i < m_ActivePlayers.size(); i++)
	{
		bs->Write(m_ActivePlayers[i]);
	}
	return true;
}

bool PlayerActiveOrderMessage::ReadPacketBitstream(RakNet::BitStream* bs)
{
	int playerCount;
	bs->Read(playerCount);
	for (int i = 0; i < playerCount; i++)
	{
		RakNet::SystemAddress add;
		bs->Read(add);
		m_ActivePlayers.push_back(add);
	}
	return true;
}

bool PlayerCardDrawnMessage::ReadPacketBitstream(RakNet::BitStream* bs)
{
	bs->Read(m_Player);
	bs->Read(m_CardDrawn);
	return true;
}

bool PlayerCardDrawnMessage::WritePacketBitstream(RakNet::BitStream* bs)
{
	bs->Write(m_MessageID);
	bs->Write(m_Player);
	bs->Write(m_CardDrawn);
	return true;
}

bool PlayerJoinGameRequestMessage::WritePacketBitstream(RakNet::BitStream* bs)
{
	bs->Write(m_MessageID);
	bs->Write(m_GameIndex);
	return true;
}

bool PlayerJoinGameRequestMessage::ReadPacketBitstream(RakNet::BitStream* bs)
{
	bs->Read(m_GameIndex);
	return true;
}
