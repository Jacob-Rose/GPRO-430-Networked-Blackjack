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

	main-server.c/.cpp
	Main source for console server application.
*/

#include <stdio.h>
#include <string.h>
#include <vector>
#include <future>
#include <limits>
#include <iostream>
#include <fstream>
#include <list>
#include <map>

//RakNet
#include <RakNet/RakPeerInterface.h>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakNetTypes.h>
#include <RakNet/GetTime.h>
#include <RakNet/BitStream.h>
#include <RakNet/RakNetTypes.h>  // MessageID

#include "gpro-net/shared-net.h"
#include "Deck.h"


struct ServerBlackjackState
{
	//Blackjack data only the server should access
	Deck deck;
	int currentPlayerTurn = 0; 
};

struct ServerState 
{
	// not much need for anything else rn
	RakNet::RakPeerInterface* m_Peer;
	std::vector<NetworkMessage*> m_InputEventCache; //filed in input
	std::vector<NetworkMessage*> m_OutputEventCache; //filled in update
	std::vector<BlackjackState> m_ActiveGames;

	std::vector<RakNet::SystemAddress> m_LobbyPlayers; //players who havent joined a game yet

	std::map<RakNet::SystemAddress, std::string> m_DisplayNames; //we store everyones (even outside our current game) to ensure its constant after we leave


	std::string saveFilePath = "ServerMessageCache.txt"; //this creates a file on the VDI which gets wiped but for testing purposes this works
	std::ofstream msgSaver;
};


void handleInput(ServerState* ss) 
{
	RakNet::Packet* packet;
	for (packet = ss->m_Peer->Receive(); packet; ss->m_Peer->DeallocatePacket(packet), packet = ss->m_Peer->Receive())
	{
		RakNet::BitStream bsIn(packet->data, packet->length, false);
		NetworkMessage::DecypherPacket(&bsIn, packet->systemAddress, ss->m_InputEventCache);
		//yup, thats it in the input step
	}
}

int findGamePlayerIsIn(RakNet::SystemAddress address, ServerState* ss)
{
	short activeGame = -1;
	for (int i = 0; i < ss->m_ActiveGames.size(); i++)
	{
		for (int j = 0; j < ss->m_ActiveGames[i].m_ActivePlayers.size(); j++)
		{
			if (ss->m_ActiveGames[i].m_ActivePlayers[j].m_Address == address)
			{
				activeGame = i;
				goto foundPlayersGame; //we cant use break here, this feels gross though
			}
		}
	}
foundPlayersGame:
	return activeGame;
}

void handleUpdate(ServerState* ss)
{
	for (int i = 0; i < ss->m_InputEventCache.size(); i++)
	{
		if (DisplayNameChangeMessage* msg = dynamic_cast<DisplayNameChangeMessage*>(ss->m_InputEventCache[i]))
		{
			ss->m_DisplayNames[msg->m_Sender] = msg->m_UpdatedDisplayName;
			ss->m_OutputEventCache.push_back(msg);
		}
		else if (PlayerChatMessage* msg = dynamic_cast<PlayerChatMessage*>(ss->m_InputEventCache[i]))
		{

		}
		else if (PlayerMoveMessage* msg = dynamic_cast<PlayerMoveMessage*>(ss->m_InputEventCache[i]))
		{

		}
		else if (NotificationMessage* msg = dynamic_cast<NotificationMessage*>(ss->m_InputEventCache[i]))
		{
			switch (msg->m_MessageID)
			{
				case ID_NEW_INCOMING_CONNECTION:
				case ID_REMOTE_NEW_INCOMING_CONNECTION:
				{
					//todo player joined, make them join lobby
					ss->m_LobbyPlayers.push_back(msg->m_Sender);

					//Once they join a lobby send PlayerActiveOrderMessage to check if player or spectator
					break;
				}
				case ID_CONNECTION_LOST:
				case ID_DISCONNECTION_NOTIFICATION:
				case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				case ID_REMOTE_CONNECTION_LOST:
				{
					//todo player disconnected
					break;
				}
			}
		}
	}
	
}

void handleOutput(ServerState* ss)
{
	for (int i = 0; i < ss->m_OutputEventCache.size(); i++)
	{
		if (DisplayNameChangeMessage* msg = dynamic_cast<DisplayNameChangeMessage*>(ss->m_OutputEventCache[i]))
		{
			//GLOBALLY SET
			RakNet::BitStream bs;
			msg->WritePacketBitstream(&bs);
			ss->m_Peer->Send(&bs, PacketPriority::HIGH_PRIORITY, PacketReliability::RELIABLE_ORDERED, 0, msg->m_Sender, true); //broadcast it to everyone
		}
		else if (PlayerChatMessage* msg = dynamic_cast<PlayerChatMessage*>(ss->m_OutputEventCache[i]))
		{
			//PER GAME OR IN LOBBY
			int gameIndex = findGamePlayerIsIn(msg->m_Sender, ss);
			//SENT PER ROOM
			RakNet::BitStream bs;
			msg->WritePacketBitstream(&bs);

			//send to everyone in that game specifically
			if (gameIndex != -1)
			{
				for (int j = 0; j < ss->m_ActiveGames[gameIndex].m_ActivePlayers.size(); j++)
				{
					ss->m_Peer->Send(&bs, PacketPriority::HIGH_PRIORITY, PacketReliability::RELIABLE_ORDERED, 0, ss->m_ActiveGames[gameIndex].m_ActivePlayers[j].m_Address, false);
				}
			}
			else
			{
				for (int j = 0; j < ss->m_LobbyPlayers.size(); j++)
				{
					ss->m_Peer->Send(&bs, PacketPriority::HIGH_PRIORITY, PacketReliability::RELIABLE_ORDERED, 0, ss->m_LobbyPlayers[j], false);
				}
			}


			
		}
		else if (PlayerMoveMessage* msg = dynamic_cast<PlayerMoveMessage*>(ss->m_OutputEventCache[i]))
		{
			//PER GAME OR IN LOBBY
			int gameIndex = findGamePlayerIsIn(msg->m_Sender, ss);
		}
	}
}

int main(void)
{
	const unsigned short SERVER_PORT = 7777;
	const unsigned short MAX_CLIENTS = 10;

	ServerState ss[1] = { 0 };

	ss->m_Peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::SocketDescriptor sd(SERVER_PORT, 0);
	ss->m_Peer->Startup(MAX_CLIENTS, &sd, 1);
	ss->m_Peer->SetMaximumIncomingConnections(MAX_CLIENTS);
	printf("Starting the server.\n");

	//test load IMPORTANT NOTE This creates a txt file on the VDI which gets wiped on startup
	/*
	std::ifstream msgLoader(ss->saveFilePath);
	if (msgLoader) 
	{
		int counter = 0;
		std::string loadString;
		while (std::getline(msgLoader, loadString))
		{
			printf(loadString.c_str()); //Change this if we want to store the message and do more with it
			printf("\n");
		}		
	}
	msgLoader.close();
	*/
	// We need to let the server accept incoming connections from the clients
	ss->msgSaver = std::ofstream(ss->saveFilePath); // this is probably not the best way to handle this but it function

	while (1)
	{
		
		handleInput(ss);

		handleUpdate(ss);

		handleOutput(ss);
	}

	ss->msgSaver.close();
	RakNet::RakPeerInterface::DestroyInstance(ss->m_Peer);

	return 0;
}
