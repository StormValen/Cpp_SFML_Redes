#pragma once

#include <string>
#include <iostream>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <vector>

#include "Player.cpp"

class Game {

	std::vector<Player> players;
	std::vector<int> alreadyPickUp;
	int bote;

public:
	Game() {
		bote = 0;
	}

	void addNewPlayerToList(Player _player) {
		//posar a dintre el vector players el nou jugador
	}

	void deletePlayerList(Player _player) {
		//agafar el numero del jugador
		//eliminar de la llista segons el numero
		//(deixar pel final, quan el joc funcioni)
	}

	int RandomWithoutRepetiton() {
		//creació del numero que es jugara ara i retornar-lo
	}

	int CalculatePot() {
		//per cada jugador treure la aposta inicial
		//actualitzar bote
	}
};