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

#include "deck.cpp"

//STD
#include <stdio.h>
#include <string.h>
#include <vector>
#include <future>
#include <limits>
#include <iostream>
#include <list>
#include <map>
#include <numeric>

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

//Global BlackjackState
BlackjackState bjs[1] = { };

struct GameState
{
	RakNet::RakPeerInterface* m_Peer;

	std::vector<NetworkMessage*> m_ServerMessageQueue; //we directly read from the bitstreams and add to this. Note, delete event object with delete when done
	std::vector<NetworkMessage*> m_LocalMessageQueue; //we directly read from the bitstreams and add to this. Note, delete event object with delete when done

	std::vector<RakNet::SystemAddress> m_PlayerOrder; //stores the player order for the game

	std::map<RakNet::SystemAddress, std::string> m_DisplayNames; //all display names
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
	if (GetAsyncKeyState(VK_LCONTROL)) //good way to async open, uses backslash to start!
	{
		//printf("Enter key pressed \n"); //debug
		std::string text = getUserInput();
		//todo
	}

}

void handleInputRemote(GameState* gs)
{
	//receive packets
	for (RakNet::Packet* packet = gs->m_Peer->Receive(); packet; gs->m_Peer->DeallocatePacket(packet), packet = gs->m_Peer->Receive())
	{
		RakNet::BitStream bsIn(packet->data, packet->length, false);

		NetworkMessage::DecypherPacket(&bsIn, packet->systemAddress, gs->m_ServerMessageQueue);
		//yup, thats it in the input step
	}
}

void handleUpdate(GameState* gs)
{
	//Recieve PlayerCardDrawnMessage
	//AddCardToPlayer(playerAddress, cardDrawn);
	//DisplayHands(gs)

	//Recieve message to get the client know they can act
	//PlayerTurn();

	//Recieve message to let people know the game is over
	//GetResults()

	//Recieve PlayerActiveOrderMessage
	//AskIfPlayerOrSpectator()
}

//Note: we dont use const here as we move the message from unhandeled to handled.
void handleOutputRemote(GameState* gs)
{


}

void handleOutputLocal(const GameState* gs)
{

}

//Ask the client if they wish to play again
void AskIfPlayerOrSpectator()
{

}

//Gets the totals of hands and see who won, lost, and tied
void GetResults()
{
	//Get total of dealer hand
	int totalOfDealer = std::accumulate(bjs->m_DealerCards.begin(), bjs->m_DealerCards.end(), decltype(bjs->m_DealerCards)::value_type(0));


	//Loop over all players and get total
	//Decide who wins

	for (int i = 0; i < bjs->m_ActivePlayers.size(); i++)
	{
		int total = std::accumulate(bjs->m_ActivePlayers[i].m_Cards.begin(), bjs->m_ActivePlayers[i].m_Cards.end(), decltype(bjs->m_ActivePlayers[i].m_Cards)::value_type(0));

		//Decide victory
		//Above 21 means lose
		//Who ever is higher wins
		//If same then tie


		//Based on what the outcome is, construct a string to cout
		if ((totalOfDealer > 21 && total > 21) || totalOfDealer == total)
		{
			//If both bust or totals are equal
			//It is a tie
		}
		else if (totalOfDealer > 21 && total < 21)
		{
			//if dealer busts and player did not
			//Player Wins
		}
		else if (total > 21 && totalOfDealer < 21)
		{
			//If player busts and dealer did not
			//Dealer wins
		}
		else if (total > totalOfDealer)
		{
			//If player total is higher than dealer
			//Player wins
		}
		else if (totalOfDealer > total)
		{
			//If Dealer total is higher than dealer
			//Dealer wins
		}
		else
		{
			//If it reaches here, something went wrong
		}
	}
}

//What happens on the players turn
void PlayerTurn()
{
	//Ask the user if hit or stand
	//AskUserForMove()
}

//This will ask the user for a move
void AskUserForMove()
{
	bool validChoice = false;
	//Will ask either Hit or Stand
	//Sends a PlayerMoveMessage with 0 or 1 depending on the move
	while (!validChoice)
	{
		std::cout << "Would you like to (H)it or (S)tand? " << '\n';
		std::string input = getUserInput();

		if (input == "h")
		{
			//Player wants to Hit
			//Send a PlayerMoveMessage with 0
		}
		else if (input == "s")
		{
			//Player wants to Stand
			//Send a PlayerMoveMessage with 1
		}
		else
		{
			std::cout << "Invalid Entry, please try again" << '\n';
		}
	}

	return;
}

//Shows all of the hands to the client
void DisplayHands(const GameState* gs)
{

	int i = 0, j = 0;
	std::string output;

	output = "Dealer: ";
	//Show dealers cards first so they are always at the top
	for (i = 0; i < bjs->m_DealerCards.size(); i++)
	{
		output += std::to_string(bjs->m_DealerCards[i]) + " ";
	}
	std::cout << output << '\n';

	for (i = 0; i < bjs->m_ActivePlayers.size(); i++)
	{
		//Show hands coupled with nickname via m_DisplayNames

		//TODO
		//Getting the data from the map is not cooperating
		output = "";
		//RakNet::SystemAddress ad = bjs->m_ActivePlayers[i].m_Address;
		//output = gs->m_DisplayNames[ad] + ": ";

		for (int j = 0; j < bjs->m_ActivePlayers[i].m_Cards.size(); j++)
		{
			output += std::to_string(bjs->m_ActivePlayers[i].m_Cards[i]) + " ";
		}
	}
}

//Adds a card to a given player
void AddCardToPlayer(RakNet::SystemAddress playerAddress, int cardDrawn)
{
	//TODO
	//Currently the dealer uses a different vector
	//Either way we need to check if this player is the dealer

	//Iterate thorough m_ActivePlayers and check if m_Address exists
	for (int i = 0; i < bjs->m_ActivePlayers.size(); ++i)
	{
		//if exists, add card to vector
		if (bjs->m_ActivePlayers[i].m_Address == playerAddress)
		{
			bjs->m_ActivePlayers[i].m_Cards.push_back(cardDrawn);
			return;
		}
	}
	//if not exists, create new PlayerStruct and add card to vector
	BlackjackState::PlayerState ps;
	ps.m_Address = playerAddress;
	ps.m_Cards.push_back(cardDrawn);

	bjs->m_ActivePlayers.push_back(ps);
}

void DisplayHands(std::vector<int> p1, std::vector<int> dealer)
{
	std::cout << "The Dealer has: ";

	for (auto i : dealer)
	{
		std::cout << i << " ";
	}

	std::cout << "\n\n";
	std::cout << "Player 1 has: ";

	for (auto i : p1)
	{
		std::cout << i << " ";
	}

	std::cout << "\n\n";
}

int getSum(std::vector<int> hand)
{
	int sum = 0;
	for (auto i : hand)
	{
		sum += i;
	}
	return sum;
}

bool CheckHand(std::vector<int> hand)
{
	return getSum(hand) > 21;
}

void ClearBlackjackState(BlackjackState& bjs)
{
	bjs.m_ActivePlayers.clear();
	bjs.m_DealerCards.clear();
	bjs.m_SpectatingPlayers.clear();

}

int main(void)
{
	//for us


	GameState gs[1] = { 0 };

	const unsigned short SERVER_PORT = 7777;
	const char* SERVER_IP = "172.16.2.57"; //update every time

	gs->m_Peer = RakNet::RakPeerInterface::GetInstance(); //set up peer

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
	gs->m_Peer->Startup(1, &sd, 1);
	gs->m_Peer->Connect(SERVER_IP, SERVER_PORT, 0, 0);


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


	RakNet::RakPeerInterface::DestroyInstance(gs->m_Peer);


	//Starting the game
	//Clear BlackjackState
	ClearBlackjackState(*bjs);

	//We can fill out m_ActivePlayers as we recieve cards from players

	bool turnOver = false;
	bool isRunning = true;
	char in;
	Deck deck;
	deck.initDeck();
	deck.shuffleDeck();

	std::vector<int> p1Hand;
	std::vector<int> dealerHand;


	while (isRunning)
	{
		//Starting the game, we deal a hand to p1 and dealer

		p1Hand.push_back(deck.getNextCard());
		p1Hand.push_back(deck.getNextCard());
		dealerHand.push_back(deck.getNextCard());
		dealerHand.push_back(deck.getNextCard());

		DisplayHands(p1Hand, dealerHand);

		//p1 turn
		while (!turnOver)
		{
			std::cout << "Would you like to (h)it or (s)tand?" << "\n\n";
			std::cin >> in;

			if (in == 'h')
			{
				p1Hand.push_back(deck.getNextCard());
				turnOver = CheckHand(p1Hand);
			}
			else
			{
				turnOver = true;
			}
			//Checks if player has busted
			DisplayHands(p1Hand, dealerHand);

		}

		turnOver = false;

		//Dealer Turn
		while (!turnOver)
		{
			if (getSum(dealerHand) >= 18) //stopping or bust
			{
				turnOver = true;
			}
			else
			{
				dealerHand.push_back(deck.getNextCard());
			}
		}

		//Decide victory
		//Above 21 means lose
		//Who ever is higher wins
		//If same then tie

		DisplayHands(p1Hand, dealerHand);

		int p1Sum = getSum(p1Hand);
		int dealerSum = getSum(dealerHand);

		if ((dealerSum > 21 && p1Sum > 21) || dealerSum == p1Sum)
		{
			std::cout << "It's a tie" << '\n';
		}
		else if (dealerSum > 21 && p1Sum <= 21)
		{
			std::cout << "Player 1 Wins" << '\n';
		}
		else if (p1Sum > 21 && dealerSum <= 21)
		{
			std::cout << "Dealer Wins" << '\n';
		}
		else if (p1Sum > dealerSum)
		{
			std::cout << "Player 1 Wins" << '\n';
		}
		else if (dealerSum < p1Sum)
		{
			std::cout << "Dealer Wins" << '\n';
		}
		else
		{
			std::cout << "Something went wrong: " << '\n';
		}

		std::cout << "Would you like to play again? (Y)es or (N)o?" << '\n';
		char in;
		std::cin >> in;

		if (in == 'y')
		{
			//Reset Values
			deck.initDeck();
			deck.shuffleDeck();
			p1Hand.clear();
			dealerHand.clear();
			turnOver = false;

		}
		else
		{
			isRunning = false;
		}
	}

	return 0;
}
