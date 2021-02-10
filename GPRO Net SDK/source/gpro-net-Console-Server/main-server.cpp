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

struct ServerState 
{
	// not much need for anything else rn
	RakNet::RakPeerInterface* peer;
	std::vector<ChatMessage> messageCache;


	std::vector<ChatMessage> unsentMessages;
	std::vector<ChatMessage> unhandledBroadcastMessages;

	std::map<RakNet::SystemAddress, std::string> m_DisplayNames;
	std::string saveFilePath = "ServerMessageCache.txt"; //this creates a file on the VDI which gets wiped but for testing purposes this works
	std::ofstream msgSaver;
};

void handleInput(ServerState* ss) 
{
	RakNet::Packet* packet;
	for (packet = ss->peer->Receive(); packet; ss->peer->DeallocatePacket(packet), packet = ss->peer->Receive())
	{
		RakNet::MessageID msg;
		RakNet::BitStream bsIn(packet->data, packet->length, false);
		RakNet::BitStream bsOut;
		bsIn.Read(msg);

		RakNet::Time timestamp = NULL;
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
		{
			printf("A client has connected.\n");
			//bsOut.Write((RakNet::MessageID)ID_TIMESTAMP);
			//bsOut.Write(RakNet::GetTime());

			//Get our display name and create a chatmessage
			RakNet::RakString msgStr = ss->m_DisplayNames[packet->systemAddress].c_str();

			ChatMessage msg{
				timestamp,
				packet->systemAddress,
				msgStr.C_String()
			};

			//write message to bit stream let somewhere else handle this
			//bsOut.Write((RakNet::MessageID)ID_CHAT_MESSAGE);
			//bsOut.Write(RakNet::RakString(msgStr));

			ss->unhandledBroadcastMessages.push_back(msg);
			//Todo, send them all the display names currently active
			break;
		}
		case ID_NEW_INCOMING_CONNECTION:
			printf("A connection is incoming.\n");
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			printf("The server is full.\n");
			break;
		case ID_DISCONNECTION_NOTIFICATION: 
		{

			printf("A client has disconnected.\n");
			//Create new time stamp for bit stream to be sent
			bsOut.Write((RakNet::MessageID)ID_TIMESTAMP);
			bsOut.Write(RakNet::GetTime());

			//Get our display name and create a chatmessage
			RakNet::RakString msgStr = ss->m_DisplayNames[packet->systemAddress].c_str();

			ChatMessage msg{
				timestamp,
				packet->systemAddress,
				msgStr.C_String()
			};

			//write message to bit stream
			bsOut.Write((RakNet::MessageID)ID_CHAT_MESSAGE);
			bsOut.Write(RakNet::RakString(msgStr));


			//Erase our display name from the map
			ss->m_DisplayNames[packet->systemAddress].erase();//probably not a great solution but it will clear the name

			//Send the data
			ss->unhandledBroadcastMessages.push_back(msg);
			//todo remove display name and relay to clients
			break;
		}
		case ID_CONNECTION_LOST:
		{
			printf("A client lost the connection.\n");

			// read in orignal sender address
			//RakNet::SystemAddress RnAddress;		
			//bsIn.Read(RnAddress);
			
			//Create new time stamp for bit stream to be sent
			bsOut.Write((RakNet::MessageID)ID_TIMESTAMP);
			bsOut.Write(RakNet::GetTime());
		
			//Get our display name and create a chatmessage
			RakNet::RakString msgStr = ss->m_DisplayNames[packet->systemAddress].c_str();

			ChatMessage msg{
				timestamp,
				packet->systemAddress,
				msgStr.C_String()
			};

			//write message to bit stream
			bsOut.Write((RakNet::MessageID)ID_CHAT_MESSAGE);
			bsOut.Write(RakNet::RakString(msgStr));	

			
			//Erase our display name from the map
			ss->m_DisplayNames[packet->systemAddress].erase();//probably not a great solution but it will clear the name

			//Send the data
			ss->unhandledBroadcastMessages.push_back(msg);
			
			//todo remove display name and relay to clients
			break;
		}
		case ID_CHAT_MESSAGE:
		{
			RakNet::RakString msgStr;
			bsIn.Read(msgStr);
			ChatMessage msg{
				timestamp,
				packet->systemAddress,
				msgStr.C_String()
			};
			//Save to file set in the server State struct
			 
			ss->msgSaver << msgStr;
			ss->msgSaver << std::endl; //each message on its own line			

			//Broadcast Message To Everyone (except one who sent it)
			RakNet::BitStream untamperedBS(packet->data, packet->length, false); //so we send the whole message
			ss->peer->Send(&untamperedBS, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
		}
		break;
		case ID_DISPLAY_NAME_UPDATED:
		{
			RakNet::SystemAddress sender;
			RakNet::RakString displayName;

			bsIn.Read(sender);
			bsIn.Read(displayName);

			if (sender == packet->systemAddress) //make sure the client is changing their name only
			{
				ss->m_DisplayNames[packet->systemAddress] = displayName.C_String();
			}
			
			//Broadcast Message To Everyone (except one who sent it)
			RakNet::BitStream untamperedBS(packet->data, packet->length, false); //so we send the whole message
			ss->peer->Send(&untamperedBS, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
			
		}
		break;
		default:
			printf("Message with identifier %i has arrived.\n", packet->data[0]);
			break;
		}
	}
}

void handleUpdate(ServerState* ss)
{
	//Same format as client messages
	for (int i = 0; i < ss->unhandledBroadcastMessages.size(); i++)
	{
		//add to message cache
		ss->unsentMessages.push_back(ss->unhandledBroadcastMessages[i]);
	}
	ss->unhandledBroadcastMessages.clear();
	
}

void handleOutput(ServerState* ss)
{
	for (int i = 0; i < ss->unsentMessages.size(); i++)
	{
		RakNet::BitStream bsOut;

		//Timestamp Message
		bsOut.Write((RakNet::MessageID)ID_TIMESTAMP);
		bsOut.Write(ss->unsentMessages[i].time);


		bsOut.Write((RakNet::MessageID)ID_CHAT_MESSAGE);
		bsOut.Write(RakNet::RakString(ss->unsentMessages[i].msg.c_str()));


		ss->peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);//gs->m_ServerAddress, false); // For now I want it to send messages I have tried geting the correct address but no luck
	}
	ss->unsentMessages.clear();
}

int main(void)
{
	const unsigned short SERVER_PORT = 7777;
	const unsigned short MAX_CLIENTS = 10;

	ServerState ss[1] = { 0 };

	ss->peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::SocketDescriptor sd(SERVER_PORT, 0);
	ss->peer->Startup(MAX_CLIENTS, &sd, 1);
	ss->peer->SetMaximumIncomingConnections(MAX_CLIENTS);
	printf("Starting the server.\n");

	//test load IMPORTANT NOTE This creates a txt file on the VDI which gets wiped on startup
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
	// We need to let the server accept incoming connections from the clients
	ss->msgSaver = std::ofstream(ss->saveFilePath); // this is probably not the best way to handle this but it function

	while (1)
	{
		
		handleInput(ss);

		handleUpdate(ss);

		handleOutput(ss);
	}

	ss->msgSaver.close();
	RakNet::RakPeerInterface::DestroyInstance(ss->peer);

	return 0;
}
