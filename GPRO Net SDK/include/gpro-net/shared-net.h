#ifndef _SHARED_NET_H
#define _SHARED_NET_H

#include <RakNet/RakPeerInterface.h>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakNetTypes.h>

#include <string>

enum GameMessages
{
	ID_CHAT_MESSAGE = ID_USER_PACKET_ENUM + 1,
	ID_DISPLAY_NAME_UPDATED
};


//Note, i refuse to not use bitstreams as I prefer maintaining cross-platform and native support!
//Thus, chatmessage is used on the client only, and is not inherintly network safe to send as a packet
struct ChatMessage
{
	RakNet::Time time; //RakNet::GetTime()
	RakNet::SystemAddress sender;
	std::string msg;

	/*
	RakNet::BitStream getBitstream()
	{

	}
	*/
};



#endif