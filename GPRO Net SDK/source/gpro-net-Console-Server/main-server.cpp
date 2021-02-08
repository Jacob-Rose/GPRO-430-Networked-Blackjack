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
#include <list>

//RakNet
#include <RakNet/RakPeerInterface.h>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakNetTypes.h>
#include <RakNet/GetTime.h>
#include <RakNet/BitStream.h>
#include <RakNet/RakNetTypes.h>  // MessageID

enum GameMessages
{
	ID_GAME_MESSAGE_1 = ID_USER_PACKET_ENUM + 1
};

struct ServerState 
{
	// not much need for anything else rn
	RakNet::RakPeerInterface* peer;
	//std::vector<RakNet::BitStream> serverMessages; 
};

void handleInput(ServerState* ss) 
{
	RakNet::Packet* packet;
	for (packet = ss->peer->Receive(); packet; ss->peer->DeallocatePacket(packet), packet = ss->peer->Receive())
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
			printf("Another client has disconnected.\n");
			break;
		case ID_REMOTE_CONNECTION_LOST:
			printf("Another client has lost the connection.\n");
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION:
			printf("A client has connected.\n");

			break;
		case ID_NEW_INCOMING_CONNECTION:
			printf("A connection is incoming.\n");
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			printf("The server is full.\n");
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			printf("A client has disconnected.\n");
			break;
		case ID_CONNECTION_LOST:
			printf("A client lost the connection.\n");
			break;
		case ID_GAME_MESSAGE_1:
		{
			RakNet::BitStream untamperedBS(packet->data, packet->length, false);
			ss->peer->Send(&untamperedBS, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, false);
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
	
}

void handleOutput(ServerState* ss)
{
	
}



int main(void)
{
	const unsigned short SERVER_PORT = 7777;
	const unsigned short MAX_CLIENTS = 10;

	ServerState ss[1] = { 0 };

	ss->peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::Packet* packet;
	RakNet::SocketDescriptor sd(SERVER_PORT, 0);
	ss->peer->Startup(MAX_CLIENTS, &sd, 1);
	ss->peer->SetMaximumIncomingConnections(MAX_CLIENTS);
	printf("Starting the server.\n");
	// We need to let the server accept incoming connections from the clients


	while (1)
	{
		
		handleInput(ss);

		//I dont think these are needed for the current assignment but could be useful for the future
		//handleUpdate(ss);

		//handleOutput(ss);
	}


	RakNet::RakPeerInterface::DestroyInstance(ss->peer);

	return 0;
}
