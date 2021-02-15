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
	bs->Write(m_MoveIndex);
	return true;
}

bool PlayerMoveMessage::ReadPacketBitstream(RakNet::BitStream* bs)
{
	bs->Read(m_Player);
	bs->Read(m_MoveIndex);
	return true;
}

void NetworkMessage::DecypherPacket(RakNet::BitStream* bs, std::vector<NetworkMessage*>& msgQueue)
{
	RakNet::MessageID id;
	bs->Read(id); //now for each message constructor, THE MESSAGE IS TRIMMED OFF THE FRONT, SO THE READ DOES NOT NEED TO ADDRESS THIS MESSAGE ID WE ALREADY KNOW
	//possibly read in id count
	int idCount;
	if (id == ID_PACKAGED_PACKET)
	{
		bs->Read(idCount);
	}
	for (int i = 0; i < idCount; i++)
	{
		switch (id) {
			case ID_TIMESTAMP:
			{
				TimestampMessage* msg = new TimestampMessage();
				msg->ReadPacketBitstream(bs);
				msgQueue.push_back(msg);
				break;
			}
			case ID_DISPLAY_NAME_UPDATED:
			{
				DisplayNameChangeMessage* msg = new DisplayNameChangeMessage();
				msg->ReadPacketBitstream(bs);
				msgQueue.push_back(msg);
				break;
			}
			case ID_PLAYER_MOVE:
			{
				PlayerMoveMessage* msg = new PlayerMoveMessage();
				msg->ReadPacketBitstream(bs);
				msgQueue.push_back(msg);
				break;
			}
			default:
			{
				NotificationMessage* msg = new NotificationMessage(id);
				msgQueue.push_back(msg);
				break;
			}
		}
	}
}

void NetworkMessage::CreatePacketHeader(RakNet::BitStream* bs, int msgCount)
{
	bs->Write((RakNet::MessageID)ID_PACKAGED_PACKET);
	bs->Write(msgCount);
}

bool TimestampMessage::WritePacketBitstream(RakNet::BitStream* bs)
{
	bs->Write(m_MessageID);
	bs->Write(m_Time);
	return true;
}

bool TimestampMessage::ReadPacketBitstream(RakNet::BitStream* bs)
{
	bs->Read(m_Time);
	return true;
}
