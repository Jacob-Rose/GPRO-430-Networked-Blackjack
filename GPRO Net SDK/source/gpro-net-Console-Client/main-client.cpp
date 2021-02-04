/*
   Copyright 2021 Daniel S. Buckstein

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

/*
	GPRO Net SDK: Networking framework.
	By Daniel S. Buckstein

	main-client.c/.cpp
	Main source for console client application.
*/

#include "gpro-net/gpro-net.h"


#include <stdio.h>
#include <string.h>
#include <vector>
#include "RakNet/RakPeerInterface.h"
#include "RakNet/MessageIdentifiers.h"
#include <RakNet/RakNetTypes.h>
#include "RakNet/BitStream.h"
#include "RakNet/RakNetTypes.h"  // MessageID

enum GameMessages
{
	ID_GAME_MESSAGE_1 = ID_USER_PACKET_ENUM + 1
};

#pragma pack (push)
#pragma pack (1)

struct ChatMessage
{
	//char timeID; //ID_TIMESTAMP
	//RakNet::Time time; //RakNet::GetTime()
	char iD; //ID_GAME_MESSAGE_1
	char msg[512];
};

#pragma pack (pop)

struct GameState
{
	RakNet::RakPeerInterface* peer;
	std::vector<ChatMessage> messagesHandled;
	std::vector<ChatMessage> unhandeledClientMessages;
	std::vector<ChatMessage> unhandeledRemoteMessages;
};



void handleInputLocal(GameState* gs)
{
	//get text
}

void handleInputRemote(GameState* gs)
{
	RakNet::Packet* packet;
	//receive packets
	for (packet = gs->peer->Receive(); packet; gs->peer->DeallocatePacket(packet), packet = gs->peer->Receive())
	{
		if (packet->data[0] == ID_TIMESTAMP)
		{
			//todo handle time
		}
		switch (packet->data[0])
		{
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			printf("Another client has disconnected.\n");
			break;
		case ID_REMOTE_CONNECTION_LOST:
			printf("Another client has lost the connection.\n");
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION:
			printf("Another client has connected.\n");
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
		{
			printf("Our connection request has been accepted.\n");

			// Use a BitStream to write a custom user message
			// Bitstreams are easier to use than sending casted structures, and handle endian swapping automatically
			/*RakNet::BitStream bsOut;
			bsOut.Write((RakNet::MessageID)ID_GAME_MESSAGE_1);
			bsOut.Write("Hello world");
			gs->peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
			*/
			ChatMessage msg = {
				//ID_TIMESTAMP,
				//RakNet::GetTime(),
				(char)ID_GAME_MESSAGE_1,
				"Hello World"
			};
			gs->peer->Send((char*)&msg, sizeof(msg), HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);

		}
		case ID_NEW_INCOMING_CONNECTION:
			printf("A connection is incoming.\n");
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			printf("The server is full.\n");
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			printf("We have been disconnected.\n");
			break;
		case ID_CONNECTION_LOST:
			printf("Connection lost.\n");
			break;
		case ID_GAME_MESSAGE_1:
		{
			RakNet::RakString rs;
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
			bsIn.Read(rs);
			printf("%s\n", rs.C_String());
		}
		break;
		default:
			printf("Message with identifier %i has arrived.\n", packet->data[0]);
			break;
		}
	}
}

void handleUpdate(GameState* gs)
{
	//update state of everything
	
}

void handleOutputRemote(const GameState* gs)
{
	//send all input messages from player
}

void handleOutputLocal(const GameState* gs)
{
	//output all messages
}

int main(void)
{
	const unsigned short SERVER_PORT = 7777;
	const char SERVER_IP[] = "172.16.2.57";

	GameState gs[1] = { 0 };

	gs->peer = RakNet::RakPeerInterface::GetInstance();
	

	RakNet::SocketDescriptor sd;
	gs->peer->Startup(1, &sd, 1);
	gs->peer->Connect(SERVER_IP, SERVER_PORT, 0, 0);
	printf("Starting the client");

	while (1)
	{
		//input
		handleInputLocal(gs);
		//receive and merge
		handleInputRemote(gs);
		//update
		handleUpdate(gs);
		//package & send
		handleOutputRemote(gs);
		//output
		handleOutputLocal(gs);
	}


	RakNet::RakPeerInterface::DestroyInstance(gs->peer);

	return 0;
}
