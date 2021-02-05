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

//STD
#include <stdio.h>
#include <string.h>
#include <vector>
#include <future>
#include <limits>
#include <iostream>

//RakNet
#include <RakNet/RakPeerInterface.h>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakNetTypes.h>
#include <RakNet/BitStream.h>
#include <RakNet/RakNetTypes.h>  // MessageID
#include <RakNet/GetTime.h>
#include <RakNet/StringCompressor.h>

enum GameMessages
{
	ID_GAME_MESSAGE_1 = ID_USER_PACKET_ENUM + 1
};

#pragma pack (push)
#pragma pack (1)

//Note, i refuse to not use bitstreams as I prefer maintaining cross-platform and native support!
//Thus, chatmessage is used on the client only, and is not inherintly network safe to send as a packet
struct ChatMessage
{
	RakNet::Time time; //RakNet::GetTime()
	RakNet::SystemAddress sender;
	//RakNet::SystemAddress target; //use RakNet::UNASSIGNED_SYSTEM_ADDRESS to make it public
	std::string msg;
};

#pragma pack (pop)

struct GameState
{
	RakNet::RakPeerInterface* peer;
	std::vector<ChatMessage> messagesHandled;
	std::vector<ChatMessage> unhandeledClientMessages;
	std::vector<ChatMessage> unhandeledRemoteMessages;
};

static std::string getUserInput()
{
	std::string input;
	std::cin >> input;
	return input;
}


void handleInputLocal(GameState* gs)
{
	//0x01 is because this is bitwise operations and the return value of getAsyncKeyState is in the same format
	if(GetAsyncKeyState(VK_OEM_102) & 0x01) //good way to async open, uses backslash to start!
	{
		//printf("Enter key pressed \n"); //debug
		std::string text = getUserInput();
		ChatMessage msg = {
			RakNet::GetTime(),
			gs->peer->GetSystemAddressFromGuid(gs->peer->GetMyGUID()),
			//RakNet::UNASSIGNED_SYSTEM_ADDRESS,
			text.c_str()
		};

		//Add new msg to unhandled Clieny messages
		gs->unhandeledClientMessages.push_back(msg);
	}

}

void handleInputRemote(GameState* gs)
{
	RakNet::Packet* packet;
	//receive packets
	for (packet = gs->peer->Receive(); packet; gs->peer->DeallocatePacket(packet), packet = gs->peer->Receive())
	{
		RakNet::MessageID msg;
		RakNet::BitStream bsIn(packet->data, packet->length, false);
		bsIn.Read(msg);


		RakNet::Time timestamp = RakNet::GetTime(); //as a safe backup, just in case its not set
		if (msg == ID_TIMESTAMP)
		{
			//todo handle time
			bsIn.Read(timestamp);
			bsIn.Read(msg);//now we update to show the real message
		}


		switch (msg)
		{
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			printf("Our client disconnected from the server.\n");
			break;
		case ID_REMOTE_CONNECTION_LOST:
			printf("Our client connection to the server has been lost.\n");
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION:
			printf("Another client has connected.\n");
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
		{
			printf("Our connection request has been accepted.\n");
			ChatMessage msg = {
				timestamp,
				gs->peer->GetSystemAddressFromGuid(gs->peer->GetMyGUID()),
				"I have joined!"
			};
			gs->unhandeledClientMessages.push_back(msg); //we need to send this out now!
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
			bsIn.Read(rs);
			ChatMessage msg = {
				timestamp,
				packet->systemAddress,
				(std::string)rs
			};
			gs->unhandeledRemoteMessages.push_back(msg);
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

//Note: we dont use const here as we move the message from unhandeled to handled.
void handleOutputRemote(const GameState* gs)
{
	//send all input messages from player
	for (int i = 0; i < gs->unhandeledClientMessages.size(); i++)
	{
		RakNet::BitStream bsOut;
		bsOut.Write((RakNet::MessageID)ID_TIMESTAMP);
		bsOut.Write(gs->unhandeledClientMessages[i].time);
		bsOut.Write((RakNet::MessageID)ID_GAME_MESSAGE_1);
		bsOut.Write(RakNet::RakString(gs->unhandeledClientMessages[i].msg.c_str()));
		gs->peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, false);
		gs->messagesHandled.push_back(gs->unhandeledClientMessages[i]);
	}
	
	gs->unhandeledClientMessages.clear();


}

void handleOutputLocal(const GameState* gs)
{
	//output all messages
	system('clr'); //first clear then output all messages
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
