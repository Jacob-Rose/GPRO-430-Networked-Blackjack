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
	RakNet::RakPeerInterface* peer;
	std::list<ChatMessage> unprintedMessageCache;
	std::list<ChatMessage> messageCache; //more optimized here to use a linked list
	std::vector<ChatMessage> unhandeledClientMessages;
	std::vector<ChatMessage> unhandeledRemoteMessages;

	//Server info
	RakNet::SystemAddress m_ServerAddress;
	RakNet::SystemAddress m_MyAddress;

	std::map<RakNet::SystemAddress, std::string> m_DisplayNames;
	std::string m_LocalDisplayName;


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
		std::string text = getUserInput();
		ChatMessage msg = {
			RakNet::GetTime(),
			gs->m_MyAddress,
			text.c_str()
		};

		//Add new msg to unhandled Clieny messages
		gs->unhandeledClientMessages.push_back(msg);
	}

}

void handleInputRemote(GameState* gs)
{
	//receive packets
	for (RakNet::Packet* packet = gs->peer->Receive(); packet; gs->peer->DeallocatePacket(packet), packet = gs->peer->Receive())
	{
		RakNet::MessageID msg;
		RakNet::BitStream bsIn(packet->data, packet->length, false);
		RakNet::BitStream bsOut;
		bsIn.Read(msg);


		RakNet::Time timestamp = RakNet::GetTime(); //as a safe backup, just in case its not set
		if (msg == ID_TIMESTAMP)
		{
			//todo handle time
			bsIn.Read(timestamp);
			bsIn.Read(msg);//now we update to show the real message
		}

		//used in switch
		RakNet::SystemAddress address;
		RakNet::RakString msgStr;

		switch (msg)
		{
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
		case ID_REMOTE_CONNECTION_LOST:
			
			bsOut.Read(address);

			printf((gs->m_DisplayNames.find(address)->second + " has disconnected.").c_str());

			gs->m_DisplayNames.erase(gs->m_DisplayNames.find(address));
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION:
			//we dont do anything cause their name will send automatically and be handeled in ID_DISPLAY_NAME_UPDATED event
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
		{
			printf("Our connection request has been accepted.\n");
			//RakNet::BitStream bsOut; moved to the top of this loop

			//we are officially connected, save stuff here
			gs->m_MyAddress = gs->peer->GetInternalID();
			gs->m_DisplayNames[gs->m_MyAddress] = gs->m_LocalDisplayName;
			gs->m_ServerAddress = packet->systemAddress;

			//This needs to be handled better this was just for debug purposes
			bsOut.Write((RakNet::MessageID)ID_TIMESTAMP);
			bsOut.Write(RakNet::GetTime());
			bsOut.Write((RakNet::MessageID)ID_DISPLAY_NAME_UPDATED);
			bsOut.Write(gs->m_MyAddress);
			bsOut.Write(RakNet::RakString(gs->m_DisplayNames.find(gs->m_MyAddress)->second.c_str()));
			gs->peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
			break;
		}
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			printf("The server is full, cannot join.\n");
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			printf("We have disconnected.\n");
			break;
		case ID_CONNECTION_LOST:
			printf("We lost connection.\n");	
			break;
		case ID_CHAT_MESSAGE:
		{
			bsIn.Read(address);
			bsIn.Read(msgStr);
			ChatMessage msg = {
				timestamp,
				address,
				msgStr.C_String(),
			};
			gs->unhandeledRemoteMessages.push_back(msg);
			break;
		}

		case ID_DISPLAY_NAME_UPDATED:
		{
			RakNet::SystemAddress sender;
			RakNet::RakString displayName;

			bsIn.Read(sender);
			bsIn.Read(displayName);

			gs->m_DisplayNames[sender] = displayName.C_String();
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
	gs->unprintedMessageCache.clear();
	for (int i = 0; i < gs->unhandeledRemoteMessages.size(); i++)
	{
		//add to message cache
		gs->unprintedMessageCache.push_back(gs->unhandeledRemoteMessages[i]);
	}
	gs->unhandeledRemoteMessages.clear();


	for (int i = 0; i < gs->unhandeledClientMessages.size(); i++) 
	{
		gs->unprintedMessageCache.push_back(gs->unhandeledClientMessages[i]);
	}
	//we dont delete from unhandledClientMessages as that is used in the remote sending
}

//Note: we dont use const here as we move the message from unhandeled to handled.
void handleOutputRemote(GameState* gs)
{
	//send all input messages from player
	for (int i = 0; i < gs->unhandeledClientMessages.size(); i++)
	{
		RakNet::BitStream bsOut;

		//Timestamp Message
		bsOut.Write((RakNet::MessageID)ID_TIMESTAMP);
		bsOut.Write(gs->unhandeledClientMessages[i].time);


		bsOut.Write((RakNet::MessageID)ID_CHAT_MESSAGE);
		bsOut.Write(RakNet::RakString(gs->unhandeledClientMessages[i].msg.c_str()));

		
		gs->peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, gs->m_ServerAddress, false);//gs->m_ServerAddress, false); //For some reason this is not correct so as a fix to just test my server changes im broadcasting to all
	}
	gs->unhandeledClientMessages.clear();

}

void handleOutputLocal(const GameState* gs)
{
	//output all messages
	//reprint all messages from message cache

	for (std::list<ChatMessage>::const_iterator it = (gs->unprintedMessageCache.begin()); it != gs->unprintedMessageCache.end(); ++it)
	{
		std::map<RakNet::SystemAddress, std::string>::const_iterator displayNameIt = gs->m_DisplayNames.find(it->sender);
		std::string name;
		if (displayNameIt != gs->m_DisplayNames.end())
		{
			name = displayNameIt->second;
		}
		else
		{
			name = it->sender.ToString();
		}
		std::cout << name << ": " << it->msg << std::endl;
	}
}

int main(void)
{
	 //for us



	GameState gs[1] = { 0 };

	const unsigned short SERVER_PORT = 7777;
	const char* SERVER_IP = "172.16.2.60"; //update every time

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
