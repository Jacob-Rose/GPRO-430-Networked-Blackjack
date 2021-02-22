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
	RakNet::RakPeerInterface* m_Peer;
	std::vector<NetworkMessage*> m_InputEventCache;


	//std::vector<ChatMessage> unsentMessages;
	//std::vector<ChatMessage> unhandledBroadcastMessages;

	std::map<RakNet::SystemAddress, std::string> m_ActivePlayers;
	std::map<RakNet::SystemAddress, std::string> m_SpectatingPlayers;
	std::string saveFilePath = "ServerMessageCache.txt"; //this creates a file on the VDI which gets wiped but for testing purposes this works
	std::ofstream msgSaver;
};

void handleInput(ServerState* ss) 
{
	RakNet::Packet* packet;
	for (packet = ss->m_Peer->Receive(); packet; ss->m_Peer->DeallocatePacket(packet), packet = ss->m_Peer->Receive())
	{
		RakNet::BitStream bsIn(packet->data, packet->length, false);
		NetworkMessage::DecypherPacket(&bsIn, ss->m_InputEventCache);
		//yup, thats it in the input step
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
	RakNet::SocketDescriptor sd(SERVER_PORT, 0);
	ss->peer->Startup(MAX_CLIENTS, &sd, 1);
	ss->peer->SetMaximumIncomingConnections(MAX_CLIENTS);
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
	RakNet::RakPeerInterface::DestroyInstance(ss->peer);

	return 0;
}
