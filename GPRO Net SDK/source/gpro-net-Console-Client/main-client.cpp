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
#include <list>
#include <map>

//RakNet
#include <RakNet/RakPeerInterface.h>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakNetTypes.h>
#include <RakNet/BitStream.h>
#include <RakNet/RakNetTypes.h>  // MessageID
#include <RakNet/GetTime.h>
#include <RakNet/StringCompressor.h>

#include "gpro-net/shared-net.h"

#define MAX_MESSAGES_TO_STORE 10




struct GameState
{
	RakNet::RakPeerInterface* m_Peer;

	std::vector<NetworkMessage*> m_ServerMessageQueue; //we directly read from the bitstreams and add to this. Note, delete event object with delete when done

	std::map<RakNet::SystemAddress, std::string> m_DisplayNames;
	std::string m_LocalDisplayName; //saved and added to m_DisplayName on ID_CONNECTION_REQUEST_ACCEPTED
	const bool m_Debug = true;
};

static std::string getUserInput()
{
	std::string input;
	std::getline(std::cin, input);
	return input;
}


void handleInputLocal(GameState* gs)
{
	//0x01 is because this is bitwise operations and the return value of getAsyncKeyState is in the same format
	if(GetAsyncKeyState(VK_LCONTROL)) //good way to async open, uses backslash to start!
	{
		//printf("Enter key pressed \n"); //debug
		//std::string text = getUserInput();
	}

}

void handleInputRemote(GameState* gs)
{
	//receive packets
	for (RakNet::Packet* packet = gs->m_Peer->Receive(); packet; gs->m_Peer->DeallocatePacket(packet), packet = gs->m_Peer->Receive())
	{
		RakNet::MessageID msg;
		RakNet::BitStream bsIn(packet->data, packet->length, false);

		NetworkMessage::DecypherPacket(&bsIn, gs->m_ServerMessageQueue);
		//now msgqueue up to date
	}
}

void handleUpdate(GameState* gs)
{

}

//Note: we dont use const here as we move the message from unhandeled to handled.
void handleOutputRemote(GameState* gs)
{


}

void handleOutputLocal(const GameState* gs)
{

}

int main(void)
{
	 //for us



	GameState gs[1] = { 0 };

	const unsigned short SERVER_PORT = 7777;
	const char* SERVER_IP = "172.16.2.57"; //update every time

	gs->peer = RakNet::RakPeerInterface::GetInstance(); //set up peer

	std::string displayName;
	std::string serverIp;
	if (!gs->m_Debug)
	{
		
		printf("Enter Display Name for server: ");
		gs->m_LocalDisplayName = getUserInput();

		
		printf("Enter IP Address for server: ");
		serverIp = getUserInput();
	}
	else
	{
		serverIp = SERVER_IP;
		gs->m_LocalDisplayName = "D Client";
	}
	
	RakNet::SocketDescriptor sd;
	gs->peer->Startup(1, &sd, 1);
	gs->peer->Connect(SERVER_IP, SERVER_PORT, 0, 0);


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
