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
#include <random>
#include <math.h>
#include <queue>

#define PERCENT_LOSS 0.7

#define MAX_PLAYERS 2
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

char tablero[SIZE_TABLERO];
struct Movment
{
	float movX, movY;
	int IDMove;
};
struct Player
{
	int ID;
	int puntos = 0;
	float posX, posY;
	std::string name;
	bool caco = false;
	std::vector<Movment>listMovments;
	std::queue<float>interpolX;
	std::queue<float>interpolY;
};


sf::UdpSocket socket;
Player player;
std::map<int, Player>Players;
int ID;
Movment movActual;



static float GerRandomFloat() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> dis(0.f, 1.f);
	return dis(gen);
}
void resetMov(Movment* mov) {
	mov->movX = 0;
	mov->movY = 0;
}
float Distance(float x, float y, float a, float b) {
	return sqrt(pow(x - a, 2) + pow(y - b, 2));
}
void CheckCollisionPlayers() {
	sf::Packet packCaco;
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		if (Players.find(player.ID)->first != it->first) {
			if (Distance(Players.find(player.ID)->second.posX, Players.find(player.ID)->second.posY, it->second.posX, it->second.posY) <= RADIO_AVATAR*2) {
				if (!Players.find(player.ID)->second.caco && it->second.caco)
					packCaco << "CMD_CACO" << Players.find(player.ID)->first;
					if (socket.send(packCaco, "localhost", 50000) != sf::Socket::Done) {
						std::cout << "Error al enviar" << std::endl;
					}
			}
		}
		packCaco.clear();
	}

}

void Connection() {
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
				std::cout << "Error al contactar con el server" << std::endl;
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
		std::cout << "No recives del server, cierra y vuelve a abrir";
	}
	std::string welcome = "";
	int size = 0;
	packR >> size;
	for (int i = 0; i < size; i++) {
		packR >> welcome >> player.name >> player.ID >> player.posX >> player.posY >> player.caco;
		if (welcome == "CMD_WELCOME") {
			Players.insert(std::pair<int, Player>(player.ID, player));
			std::cout << Players[i].name << " ID:" << Players[i].ID << " POS: " << Players[i].posX << " " << Players[i].posY << "C3" << Players[i].caco << std::endl;
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
		float rndPacketLoss = GerRandomFloat();

		if (socket.receive(pack, _IP, _port) != sf::Socket::Done) {
		}
		
		if (rndPacketLoss < PERCENT_LOSS) {
			pack.clear();
		}
		else {
			pack >> cmd;

			if (cmd == "CMD_NEW_PLAYER") {
				std::cout << "Recivo el new" << std::endl;
				int packID = 0;
				sf::Packet packACKNEW;
				Player newPlayer;
				
				pack >> packID >> newPlayer.name >> newPlayer.ID >> newPlayer.posX >> newPlayer.posY >> newPlayer.caco;
				std::cout << " > " << cmd << " ID: " << newPlayer.ID << " POS: " << newPlayer.posX << newPlayer.posY << "C3" << newPlayer.caco << std::endl;
				std::cout << packID;
				Players.insert(std::pair<int, Player>(newPlayer.ID, newPlayer));
				packACKNEW << "CMD_ACK_NEW" << packID << player.ID;

				if (socket.send(packACKNEW, "localhost", 50000) != sf::Socket::Done) {
					std::cout << "Error al enviar" << std::endl;
				}
				//std::cout << "Envio el new"  << std::endl;
				//packACKNEW.clear();
			}
			if (cmd == "CMD_PING") {
				sf::Clock clock;
				clock.restart();
				packS << "CMD_ACK" << player.ID;
				if (socket.send(packS, "localhost", 50000) != sf::Socket::Done) {
					std::cout << "Error al enviar" << std::endl;
				}
				clock.restart();
				//packS.clear();
			}
			if (cmd == "CMD_DESC") {
				std::string a;
				int idAux;
				pack >> idAux;
				if (Players.find(idAux) != Players.end()) {
					Players.erase(idAux);
				}
			}
			if (cmd == "CMD_OK_MOVE") {
				pack >> idAux2 >> idMoveAux;
				if (Players.find(player.ID)->first == idAux2) {
					for (int i = 0; i < Players.find(player.ID)->second.listMovments.size(); i++) {
						if (Players.find(player.ID)->second.listMovments[i].IDMove == idMoveAux) {
							float auxX, auxY;
							pack >> auxX >> auxY;
							if ((auxX <= (Players.find(player.ID)->second.posX - 5) || auxX >= (Players.find(player.ID)->second.posX + 5)) && (auxY <= (Players.find(player.ID)->second.posY - 5) || auxY >= (Players.find(player.ID)->second.posY + 5))) {
								Players.find(player.ID)->second.listMovments.erase(Players.find(player.ID)->second.listMovments.begin(), Players.find(player.ID)->second.listMovments.begin() + i);
								//std::cout << "no corrijo nada" <<  std::endl;
							}
							else if (auxX != Players.find(player.ID)->second.posX || auxY != Players.find(player.ID)->second.posY) {
								Players.find(player.ID)->second.listMovments.erase(Players.find(player.ID)->second.listMovments.begin(), Players.find(player.ID)->second.listMovments.end());
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
						if (it->first == idAux2 && player.ID != it->first) {
							float posX, posY, interX, interY;
							pack >> posX >> posY;

							interX = (posX - it->second.posX) / 5;
							interY = (posY - it->second.posY) / 5;
							//std::cout << interX << " " << interY << std::endl;
							for (int i = 0; i < 5; i++) {
								it->second.interpolX.push(interX);
								it->second.interpolY.push(interY);
								//std::cout << interpolX.front() << " " << interpolY.front() << std::endl;

							}
							
						}

					}
				}
			}
			if (cmd == "CMD_ACK_CACO") {
				int idAux;
				bool aux;
				pack >> idAux >> aux;
				for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
					if (it->first == idAux) {
						it->second.caco = aux;
					}
				}
			}	
			if (cmd == "CMD_RESET") {
				int idAux, puntos;
				bool cacoAux;			
				for (int i = 0; i < Players.size(); i++) {
					pack >> idAux >> puntos;
					if (Players.find(player.ID)->first == idAux) {
						std::cout << "Se ha acabado la partida, en 3 segundos comienza la siguiente " << std::endl;
						Players.find(player.ID)->second.puntos = puntos;
						std::cout << "Tienes :" << Players.find(player.ID)->second.puntos << " puntos" << std::endl;
						pack >> cacoAux;
						Players.find(player.ID)->second.caco = cacoAux;
					}
					else {
						pack >> cacoAux;
						Players.find(i)->second.caco = cacoAux;
					}
				}
				std::cout << "Se ha resetado la partida" << std::endl;
			}
		/*	if (cmd == "CMD_INFO") {
				int idAux, puntos;
				for (int i = 0; i < Players.size(); i++) {
					pack >> idAux >> puntos;
					if (Players.find(player.ID)->first == idAux) {
						std::cout << "Se ha acabado la partida, en 3 segundos comienza la siguiente " << std::endl;
						Players.find(player.ID)->second.puntos = puntos;
						std::cout << "Tienes :" << Players.find(player.ID)->second.puntos << " puntos" << std::endl;
					}
				}
			}*/
			if (cmd == "CMD_END") {
				int idAux;
				pack >> idAux;
				std::cout << "Se ha acabado la partida" << std::endl;
				for (int i = 0; i < Players.size(); i++) {
					if (Players.find(i)->first == idAux) {
						std::cout << "El ganador ha sido " << Players.find(i)->second.name << std::endl;
						socket.unbind();
						//system("exit");
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
					Players.find(player.ID)->second.listMovments.push_back(movActual);

					Players.find(player.ID)->second.posX++;
				}
				if (event.key.code == sf::Keyboard::Left) {
					movActual.movX--;
					Players.find(player.ID)->second.listMovments.push_back(movActual);

					Players.find(player.ID)->second.posX--;
				}
				if (event.key.code == sf::Keyboard::Up) {
					movActual.movY--;
					Players.find(player.ID)->second.listMovments.push_back(movActual);

					Players.find(player.ID)->second.posY--;
				}
				if (event.key.code == sf::Keyboard::Down) {
					movActual.movY++;
					Players.find(player.ID)->second.listMovments.push_back(movActual);

					Players.find(player.ID)->second.posY++;
				}	
				//interpol.push_back(movActual);
				break;
			default:
				break;

			}
		}
		//send mov
		if (clockMov.getElapsedTime().asMilliseconds() > 50 && (movActual.movX != 0 || movActual.movY != 0)) {
			movActual.IDMove++;
			Players.find(player.ID)->second.listMovments.push_back(movActual);

			packMov << "CMD_MOV" << movActual.IDMove << player.ID << movActual.movX << movActual.movY;

			if (socket.send(packMov, "localhost", 50000) != sf::Socket::Done) {
				std::cout << "Error al enviar la posicion" << std::endl;
			}
			else {
				clockMov.restart();
				resetMov(&movActual);

			}
		}

		window.clear();	
		for (int i = 0; i<SIZE_FILA_TABLERO; i++)
		{
			for (int j = 0; j < SIZE_FILA_TABLERO; j++)
			{
				sf::RectangleShape rectBlanco(sf::Vector2f(LADO_CASILLA, LADO_CASILLA));
				rectBlanco.setFillColor(sf::Color::White);
				if (j == 0) {
					sf::RectangleShape rectBlu(sf::Vector2f(LADO_CASILLA, LADO_CASILLA));
					rectBlu.setFillColor(sf::Color::Magenta);
					rectBlu.setPosition(sf::Vector2f(i*LADO_CASILLA, 0));
					window.draw(rectBlu);
				}
				if (j == SIZE_FILA_TABLERO - 1) {
					sf::RectangleShape rectBlu(sf::Vector2f(LADO_CASILLA, LADO_CASILLA));
					rectBlu.setFillColor(sf::Color::Magenta);
					rectBlu.setPosition(sf::Vector2f(i*LADO_CASILLA, (SIZE_FILA_TABLERO - 1)*LADO_CASILLA));
					window.draw(rectBlu);
				}
				if (i == 0) {
					sf::RectangleShape rectBlu(sf::Vector2f(LADO_CASILLA, LADO_CASILLA));
					rectBlu.setFillColor(sf::Color::Magenta);
					rectBlu.setPosition(sf::Vector2f(0, j *LADO_CASILLA));
					window.draw(rectBlu);
				}
				if (i == SIZE_FILA_TABLERO - 1) {
					sf::RectangleShape rectBlu(sf::Vector2f(LADO_CASILLA, LADO_CASILLA));
					rectBlu.setFillColor(sf::Color::Magenta);
					rectBlu.setPosition(sf::Vector2f((SIZE_FILA_TABLERO - 1)*LADO_CASILLA, j *LADO_CASILLA));
					window.draw(rectBlu);
				}
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

		//TODO: Para pintar el circulito del ratón
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			sf::CircleShape shapeRaton(RADIO_AVATAR);
			if (it->second.caco) {
				shapeRaton.setFillColor(sf::Color::Red);
			}
			else {
				shapeRaton.setFillColor(sf::Color::Blue);
			}
			if (it->second.interpolX.size() > 1 && it->second.interpolY.size() > 1) {
				it->second.posX += it->second.interpolX.front();
				it->second.posY += it->second.interpolY.front();
				it->second.interpolX.pop();
				it->second.interpolY.pop();

			}sf::Vector2f positionGato1(it->second.posX, it->second.posY);
			shapeRaton.setPosition(positionGato1);

			window.draw(shapeRaton);
		}
		CheckCollisionPlayers();
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