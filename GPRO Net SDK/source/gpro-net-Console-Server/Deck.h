#ifndef DECK_H
#define DECK_H

#include <vector>
#include <stdlib.h>
#include <time.h>


class Deck
{
public:
	Deck() {};
	void initDeck()
	{
		//Kings, Queens, and Jacks are all worth 10 points
		deck = { 1,2,3,4,5,6,7,8,9,10,10,10,10,
				 1,2,3,4,5,6,7,8,9,10,10,10,10,
				 1,2,3,4,5,6,7,8,9,10,10,10,10,
				 1,2,3,4,5,6,7,8,9,10,10,10,10 };
	};

	void shuffleDeck()
	{
		srand((unsigned)time(0));
		for (int i = 0; i < 52; i++)
		{
			std::swap(deck[i], deck[rand() % 52]);
		}
	};

	int getNextCard()
	{
		int nextCard = deck.front();
		deck.erase(deck.begin());
		return nextCard;
	};

private:
	std::vector<int> deck;
};

#endif // DECK_H

