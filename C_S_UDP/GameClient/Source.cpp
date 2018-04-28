#include <stdlib.h>
//#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
//#include <wait.h>
#include <thread>
#include <stdio.h>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML\Network.hpp>

#define MAX_PLAYERS 4
#define MAX 100
#define SIZE_TABLERO 64
#define SIZE_FILA_TABLERO 25
#define LADO_CASILLA 20
#define RADIO_AVATAR 10.f
#define OFFSET_AVATAR 1

#define SIZE_TABLERO 64
#define LADO_CASILLA 20
#define RADIO_AVATAR 10.f
#define OFFSET_AVATAR 1

enum TipoProceso { RATON, GATO, PADRE };
char tablero[SIZE_TABLERO];
struct Player
{
	int ID;
	float posX, posY;
	std::string name;
};
struct Movment
{
	float movX, movY;
	int IDMove;
};
sf::Vector2f TransformaCoordenadaACasilla(int _x, int _y)
{
	float xCasilla = _x / LADO_CASILLA;
	float yCasilla = _y / LADO_CASILLA;
	sf::Vector2f casilla(xCasilla, yCasilla);
	return casilla;
}

sf::Vector2f BoardToWindows(sf::Vector2f _position)
{
	return sf::Vector2f(_position.x*LADO_CASILLA, _position.y*LADO_CASILLA);
}

sf::UdpSocket socket;
Player player;
std::map<int, Player>Players;
int ID;
Movment movActual;
std::vector<Movment>listMovments;
std::vector<Movment>interpol;

void resetMov(Movment* mov) {
	mov->movX = 0;
	mov->movY = 0;
}
void Connection(){
	//socket.setBlocking(true);
	bool send = false;
	sf::Packet packetLog;
	std::string name;
	std::cout << "Introduce tu nombre" << std::endl;
	std::cin >> name;
	packetLog << "HELLO" << name;
	sf::Clock c;
	c.restart();
	while (!send) {
		if (c.getElapsedTime().asMilliseconds() >= 500) {
			if (socket.send(packetLog, "localhost", 50000) != sf::Socket::Done) {
				std::cout << "Error al enviar" << std::endl;
			}
			c.restart();
			send = true;
		}
	}
	packetLog.clear();
	sf::IpAddress IP;
	unsigned short port;
	sf::Packet packR;

	if (socket.receive(packR, IP, port) != sf::Socket::Done) {
		std::cout << "Error al recivir";
	}
	std::string welcome = "";
	int size = 0;
	packR >> size;
	for (int i = 0; i < size; i++) {
		packR >> welcome >> player.name >> player.ID >> player.posX >> player.posY;
		if (welcome == "CMD_WELCOME") {
			Players.insert(std::pair<int, Player>(player.ID, player));
			std::cout << Players[i].name << " ID:" << Players[i].ID << " POS: " << Players[i].posX << " " << Players[i].posY << std::endl;
		}
	}
	packR.clear();
}

void Gameplay()
{
	socket.setBlocking(false);
	sf::Packet packS;
	int idAux2 = 0;
	int idMoveAux = 0;

	sf::Vector2f casillaOrigen, casillaDestino;
	bool casillaMarcada = false;
	sf::Clock clockMov;
	clockMov.restart();
	sf::RenderWindow window(sf::VideoMode(500, 500), "El Gato y el Raton");
	while (window.isOpen())
	{
		int ultimID;
		sf::Event event;
		sf::Packet packMov;
		sf::Packet pack;
		std::string cmd;
		sf::IpAddress _IP;
		unsigned short _port;

		if (socket.receive(pack, _IP, _port) != sf::Socket::Done) {
			//std::cout << "Error al recivir";
		}
		pack >> cmd; 

		if (cmd == "CMD_NEW_PLAYER") {
			int packID = 0;
			sf::Packet packACKNEW;
			Player newPlayer;
			pack >> packID >> newPlayer.name >> newPlayer.ID >> newPlayer.posX >> newPlayer.posY;
			std::cout << " > " << cmd << " ID: " << newPlayer.ID << " POS: " << newPlayer.posX << newPlayer.posY << std::endl;
			Players.insert(std::pair<int, Player>(newPlayer.ID, newPlayer));
			packACKNEW << "CMD_ACK_NEW" << packID << player.ID;

			if (socket.send(packACKNEW, "localhost", 50000) != sf::Socket::Done) {
				std::cout << "Error al enviar" << std::endl;
			}
			std::cout << packID << std::endl;
			//packACKNEW.clear();
		}
		else if (cmd == "CMD_PING") {
			sf::Clock clock;
			clock.restart();
			packS << "CMD_ACK" << player.ID;
				if (socket.send(packS, "localhost", 50000) != sf::Socket::Done) {
					std::cout << "Error al enviar" << std::endl;
				}
				clock.restart();
				packS.clear();
		}
		else if (cmd == "CMD_DESC") {
			std::string a;
			int idAux;
			pack >> idAux;
			if (Players.find(idAux) != Players.end()) {
				Players.erase(idAux);
			}
		}
		else if (cmd == "CMD_OK_MOVE") {
			pack >> idAux2 >> idMoveAux;
			if (Players.find(player.ID)->first == idAux2) {
				for (int i = 0; i < listMovments.size(); i++) {
					if (listMovments[i].IDMove == idMoveAux) {
						float auxX, auxY;
						pack >> auxX >> auxY;
						if ((auxX <= (Players.find(player.ID)->second.posX-5) || auxX >= (Players.find(player.ID)->second.posX + 5)) && (auxY <= (Players.find(player.ID)->second.posY - 5) || auxY >= (Players.find(player.ID)->second.posY + 5))) {
							listMovments.erase(listMovments.begin(), listMovments.begin() + i); 
							//std::cout << "no corrijo nada" <<  std::endl;
						}
						else if (auxX != Players.find(player.ID)->second.posX || auxY != Players.find(player.ID)->second.posY) {
							listMovments.erase(listMovments.begin(), listMovments.end());
							Players.find(player.ID)->second.posX = auxX;
							Players.find(player.ID)->second.posY = auxY;
							//std::cout << "corrijo pos de " << Players.find(player.ID)->second.name << std::endl;
						}
					}
				}
			}
			else {
				for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
					//	std::cout << " X " << it->second.posX << " Y " << it->second.posY << std::endl;
					if (it->first == idAux2 && player.ID != idAux2) {
						float auxX, auxY;
						pack >> auxX >> auxY;
						it->second.posX = auxX;
						it->second.posY = auxY;
						/*for (int i = 0; i < listMovments.size(); i++) {
							if (listMovments[i].IDMove == idMoveAux) {
								float auxX, auxY;
								pack >> auxX >> auxY;
								std::cout << auxX << " " << auxY << " " << it->second.posX << " " << it->second.posY << std::endl;
								if (auxX == it->second.posX && auxY == it->second.posY) {
									//listMovments.erase(listMovments.begin(), listMovments.begin() + i);
									std::cout << "no corrijo nada" << it->second.name << std::endl;
								}
								else if (auxX != it->second.posX || auxY != it->second.posY) {

									std::cout << "corrijo pos de " << it->second.name << std::endl;
								}
							}
						}*/
					}

				}
			}
		}
		pack.clear();
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed:
				window.close();
				socket.unbind();
				break;
			case sf::Event::KeyPressed:
				if (event.key.code == sf::Keyboard::Right) {
					movActual.movX++;
					Players.find(player.ID)->second.posX++;
				}
				if (event.key.code == sf::Keyboard::Left) {
					movActual.movX--;
					Players.find(player.ID)->second.posX--;
				}
				if (event.key.code == sf::Keyboard::Up) {
					movActual.movY--;
					Players.find(player.ID)->second.posY--;
				}
				if (event.key.code == sf::Keyboard::Down) {
					movActual.movY++;
					Players.find(player.ID)->second.posY++;
				}	
				interpol.push_back(movActual);
				break;
			default:
				break;

			}
		}
		if (clockMov.getElapsedTime().asMilliseconds() > 50 && (movActual.movX != 0 || movActual.movY != 0)) {
			movActual.IDMove++;
			listMovments.push_back(movActual);
			packMov << "CMD_MOV" << movActual.IDMove << player.ID << movActual.movX << movActual.movY;

			if (socket.send(packMov, "localhost", 50000) != sf::Socket::Done) {
				std::cout << "Error al enviar la posicion" << std::endl;
			}
			else {
				clockMov.restart();
				//std::cout <<  " IDM " << movActual.IDMove  << std::endl;
				//ultimID = movActual.IDMove;
				resetMov(&movActual);

			}
		}
		window.clear();	
		for (int i = 0; i<SIZE_FILA_TABLERO; i++)
		{
			for (int j = 0; j<SIZE_FILA_TABLERO; j++)
			{
				sf::RectangleShape rectBlanco(sf::Vector2f(LADO_CASILLA, LADO_CASILLA));
				rectBlanco.setFillColor(sf::Color::White);
				if (i % 2 == 0)
				{
					//Empieza por el blanco
					if (j % 2 == 0)
					{
						rectBlanco.setPosition(sf::Vector2f(i*LADO_CASILLA, j*LADO_CASILLA));
						window.draw(rectBlanco);
					}
				}
				else
				{
					//Empieza por el negro
					if (j % 2 == 1)
					{
						rectBlanco.setPosition(sf::Vector2f(i*LADO_CASILLA, j*LADO_CASILLA));
						window.draw(rectBlanco);
					}
				}
			}
		}

		//TODO: Para pintar el circulito del rat�n
		/*sf::CircleShape shapeRaton(RADIO_AVATAR);
		shapeRaton.setFillColor(sf::Color::Blue);
		sf::Vector2f posicionRaton(player.posX, player.posY);
		posicionRaton = BoardToWindows(posicionRaton);
		shapeRaton.setPosition(posicionRaton);
		window.draw(shapeRaton);*/

		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			sf::CircleShape shapeRaton(RADIO_AVATAR);
			if (it->first == player.ID) {
				shapeRaton.setFillColor(sf::Color::Blue);
			}
			else {
				shapeRaton.setFillColor(sf::Color::Red);
			}
			sf::Vector2f positionGato1(it->second.posX, it->second.posY);
			//positionGato1 = BoardToWindows(positionGato1);
			shapeRaton.setPosition(positionGato1);

			window.draw(shapeRaton);
		}
	
	/*	//Pintamos los cuatro circulitos del gato
		sf::CircleShape shapeGato(RADIO_AVATAR);
		shapeGato.setFillColor(sf::Color::Red);

		sf::Vector2f positionGato1(1.f, 0.f);
		positionGato1 = BoardToWindows(positionGato1);
		shapeGato.setPosition(positionGato1);

		window.draw(shapeGato);

		sf::Vector2f positionGato2(3.f, 0.f);
		positionGato2 = BoardToWindows(positionGato2);
		shapeGato.setPosition(positionGato2);

		window.draw(shapeGato);

		sf::Vector2f positionGato3(5.f, 0.f);
		positionGato3 = BoardToWindows(positionGato3);
		shapeGato.setPosition(positionGato3);

		window.draw(shapeGato);*/

		window.display();
	}
}

int main()
{
	bool done = false;
	Connection();
	Gameplay();
	return 0;
}