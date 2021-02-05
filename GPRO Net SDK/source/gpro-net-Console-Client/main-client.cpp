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
#include <future>
#include <limits>
#include <iostream>
#include "RakNet/RakPeerInterface.h"
#include "RakNet/MessageIdentifiers.h"
#include <RakNet/RakNetTypes.h>
#include "RakNet/BitStream.h"
#include "RakNet/RakNetTypes.h"  // MessageID
#include <RakNet/GetTime.h>
#include <RakNet/StringCompressor.h>

enum GameMessages
{
	ID_GAME_MESSAGE_1 = ID_USER_PACKET_ENUM + 1
};

#pragma pack (push)
#pragma pack (1)

struct ChatMessage
{
	char timeID; //ID_TIMESTAMP
	RakNet::Time time; //RakNet::GetTime()
	char iD; //ID_GAME_MESSAGE_1
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
	//get text
	//https://stackoverflow.com/questions/6171132/non-blocking-console-input-c

	std::chrono::seconds timeout(5);
	std::future<std::string> future = std::async(getUserInput);
	if (future.wait_for(timeout) == std::future_status::ready)
	{
		std::string input = future.get();
	}
	else
	{
		//save cin and load it back after output
		//rn we just erase it
		//std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}


	/*
	//0x01 is because this is bitwise operations and the return value of getAsyncKeyState is in the same format
	if(GetAsyncKeyState(VK_RETURN) & 0x01)
	{
		//printf("Enter key pressed \n"); //debug

		char text[512];
		//std::basic_istream::getline(std::cin, text); //Code does not like basic_istream will need to find something that works 
		ChatMessage msg = {
			//ID_TIMESTAMP,
			//RakNet::GetTime(),
			(char)ID_GAME_MESSAGE_1,
			*text
		};

		//Add new msg to unhandled Clieny messages
		gs->unhandeledClientMessages.push_back(msg);
	}

	//Right click
	if (GetAsyncKeyState(VK_RBUTTON) & 0x01) 
	{
		for (unsigned int i = 0; i < gs->unhandeledClientMessages.size(); i++) 
		{
			//Print out stored messages
			printf(gs->unhandeledClientMessages[i].msg);
		}
	}
	



	ChatMessage msg = {
		ID_TIMESTAMP,
		RakNet::GetTime(),
		(char)ID_GAME_MESSAGE_1,
		strtmp.c_str()
	};
	*/

}

void handleInputRemote(GameState* gs)
{
	RakNet::Packet* packet;
	//receive packets
	for (packet = gs->peer->Receive(); packet; gs->peer->DeallocatePacket(packet), packet = gs->peer->Receive())
	{
		RakNet::Time timestamp;
		RakNet::MessageID msg;
		RakNet::BitStream bsIn(packet->data, packet->length, false);
		bsIn.Read(msg);
		if (msg == ID_TIMESTAMP)
		{
			//todo handle time
			bsIn.Read(timestamp);
			bsIn.Read(msg);//now we update to show the real message
		}


		switch (msg)
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
			RakNet::BitStream bsOut;
			bsOut.Write((RakNet::MessageID)ID_TIMESTAMP);
			bsOut.Write(RakNet::GetTime());
			bsOut.Write((RakNet::MessageID)ID_GAME_MESSAGE_1);
			bsOut.Write(RakNet::RakString( "Hello, I have Joined!"));
			gs->peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);

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
