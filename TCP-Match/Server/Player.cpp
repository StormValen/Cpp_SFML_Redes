#pragma once

#include <string>
#include <iostream>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <vector>

#define INITIAL_MONEY 200
#define ROWS_BOOK 4
#define COLUMNS_BOOK 2
#define BINGO_90 89

class Player {
private:
	int numberPlayer;
	int book[8][2];
	bool bingo;
	bool line;
	int money;

public:
	
	Player(int _numberPlayer) {
		numberPlayer = _numberPlayer;
		
		//construir la cartilla amb numeros sense repeticio
		srand(time(NULL));
		std::vector<int> alreadyInsideBook;

		for (int i = 0; i <= ROWS_BOOK; i++) {
			for (int j = 0; j <= COLUMNS_BOOK; j++) {
				
				int randomNumber = rand() % BINGO_90 + 1;
				while (CheckWithoutRepetition(alreadyInsideBook, randomNumber)) {
					//find another random number, already inside
				}
					
				alreadyInsideBook.push_back(randomNumber);
				book[i][j] = randomNumber;
			}
		}
		
		bingo = false;
		line = false;
		money = INITIAL_MONEY;
	}

	bool CheckWithoutRepetition(std::vector<int> _myvector, int _randomNumber) {

		bool inside = false;
		for (std::vector<int>::iterator it = _myvector.begin(); it != _myvector.end(); ++it)
		{
			if (*it == _randomNumber) {
				inside = true;
			}
		}
		return inside;
	}

	bool CheckBingo() {
		//recorre tota la matriu comprovant que tots els numeros son negatius
	}
	bool CheckLine() {
		//recorre una fila i mirar si tots els numero son negatius

	}
	bool CheckNumber(int _numberToCheck) {
		//comprobar si a la cartilla hi ha el mateix numero
		//si hi és, actualitzem la cartilla posant el numero en negatiu
	}

	void InitialBet(int _initalBet) {
		//restar dels diners del jugador

	}

	int getNumberPlayer() {
		//retornar el numero del jugador
	}

	std::string bookReadyToSend() {
		//BOOK_
		//convertir la cartilla en un string per ja poder-la enviar
	}




};