#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>
#include <random>

//numero de jugadores de la partida
#define MAX_PLAYERS 3
//ratio de perdida de paquetes
#define PERCENT_LOSS 0.1
sf::UdpSocket socket;

struct Movment
{
	float movX = 0, movY = 0;
	int IDMove;
};
struct PacketCritic
{
	sf::Packet packet;
	int ID = 0;
}; 
struct Player
{
	sf::IpAddress IP;
	unsigned short port;
	bool connected = true;
	int ID;
	int puntos;
	bool caco = false;
	float posX, posY;
	std::string name;
	sf::Clock timePing;
	std::map<int, Movment> movment;
	std::vector<Movment>interpolation;
	std::map<int, PacketCritic> mapPacketCritic;
};

std::map<int, Player> Players;
//paredes
float maxY = 470;
float minY = 20.f;
float maxX = 470.f;
float minX = 20.f;

int packID = 1;
int counter = 1;
sf::Clock clockTime;
sf::Clock clockReset;
bool reset = false;

//funcion para calcular el random para la perdida de paquetes
static float GerRandomFloat() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> dis(0.f, 1.f);
	return dis(gen);
}
//funcion para reenviar los paquetes criticos del newPlayer
void Resend() {
	sf::Clock clockResend;
	clockResend.restart();
	while (Players.size() < MAX_PLAYERS)
	{
		//std::cout << "hola";
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			while (it->second.mapPacketCritic.size() > 0) {
				if (clockResend.getElapsedTime().asMilliseconds() > 300) {
					std::cout << "IDPACKETE a enviar " << it->second.mapPacketCritic.find(Players.size() - 1)->first << " para " << it->second.name << std::endl;
					if (it->second.mapPacketCritic.find(Players.size() - 1) != it->second.mapPacketCritic.end()) {
						if (socket.send(it->second.mapPacketCritic.find(Players.size() - 1)->second.packet, it->second.IP, it->second.port) != sf::Socket::Done) {
							std::cout << "Error al enviar nueva conexion" << std::endl;
						}
					}
					clockResend.restart();
				}
			}
		}
	}
	
}
//funcion para comprobar si algun cliente envia el ACK de newPlayer
void CheckNewPlayer() {

		sf::IpAddress IP;
		unsigned short port;
		sf::Packet packetLog;
		std::string str_CON;
		for (int i = 0; i < Players.size(); i++) {
			while (Players.find(i)->second.mapPacketCritic.size() > 0) { 
				if (socket.receive(packetLog, IP, port) != sf::Socket::Done) {
					std::cout << "Error al recivir" << std::endl;
				}
				packetLog >> str_CON;
				if (str_CON == "CMD_ACK_NEW") {
					int idPacket, id;
					packetLog >> idPacket >> id;
					if (Players.find(i)->first == id) {
						if (idPacket == Players.find(i)->second.mapPacketCritic.find(idPacket)->first && Players.find(i)->second.mapPacketCritic.find(idPacket) != Players.find(i)->second.mapPacketCritic.end()) {
							std::cout << "Recivo ACK " << Players.find(i)->second.name << " " << Players.find(i)->second.mapPacketCritic.find(idPacket)->first << std::endl;
							Players.find(i)->second.mapPacketCritic.erase(idPacket);
							std::cout << "Tamano " << Players.find(id)->second.mapPacketCritic.size() << " de" << Players.find(id)->second.name << std::endl;
						}
					}
				}
			}
		}

			//tr.join();
}
//funcion que añade el paquete critico y lo envia la primera vez
void NewPlayer(Player player) {
	PacketCritic packCritic;
	if (Players.size() > 1) {
		packCritic.ID = player.ID;
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			std::string cmd = "CMD_NEW_PLAYER";
			if (it->first != player.ID) {
				packCritic.packet << cmd << packID << player.name << player.ID << player.posX << player.posY << player.caco;
				it->second.mapPacketCritic.insert(std::pair <int, PacketCritic>(packID, packCritic));
				if (socket.send(packCritic.packet, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al enviar nueva conexion" << std::endl;
				}

						std::cout << it->second.name << it->second.mapPacketCritic.find(Players.size() - 1)->first << std::endl;
			}
		}	
		packID++;
	}		
}
//funcion para enviar la desconexion
void sendAllPlayers(std::string cmd, int id) {
	sf::Packet packDes;
	if (cmd == "CMD_DESC") {
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			packDes << cmd << id;
			if (it->first != id) {
				if (socket.send(packDes, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al enviar nueva conexion" << std::endl;
				}
			}
			packDes.clear();
		}
	}
}
//funcion para resetear la ronda
void GameInfo() {
	//std::cout << random;
	sf::Packet packInfo;
	packInfo << "CMD_RESET";
	srand(time(NULL));
	int random = rand() % MAX_PLAYERS;
	for (int i = 0; i < Players.size(); i++) {
		Players.find(i)->second.caco = false;
		packInfo << Players.find(i)->first << Players.find(i)->second.puntos;
		if (Players.find(i)->first == random) {
			Players.find(i)->second.caco = true;
			std::cout << Players.find(i)->second.name;
		}
		packInfo  << Players.find(i)->second.caco;
	}
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		if (socket.send(packInfo, it->second.IP, it->second.port) != sf::Socket::Done) {
			std::cout << "Error al enviar mov" << std::endl;
		}
	}
	counter = 1;
	
}
//funcion para comprobar si se ha llegado al limite de puntos y acabar
void CheckScore(int id) {
	sf::Packet packEnd;
	packEnd << "CMD_END" << id;
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		if (socket.send(packEnd, it->second.IP, it->second.port) != sf::Socket::Done) {
			std::cout << "Error al enviar mov" << std::endl;
		}
	}
	socket.unbind();
	system(0);
}
//gestiona el final de cada ronda y los puntos
void TimeGame() {
	sf::Packet packPoints;
	if (clockTime.getElapsedTime().asSeconds() > 15 && counter < MAX_PLAYERS) {
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			if (!it->second.caco) {	
				it->second.puntos++;
			}
			if (it->second.puntos == 5) {
				CheckScore(it->first);
			}
		}
		GameInfo();
		clockTime.restart();
	}
	else if (counter == MAX_PLAYERS) {
		GameInfo();

	}
}

//Para que se conecten todos los jugadores
void Connection() {
	srand(time(NULL));
	sf::Packet packetLog;
	sf::Packet newPlayerPack;
	std::string str_CON;
	int i = 0;
	sf::Clock clockResend;
	clockResend.restart();
	int newPlayer = 0;
	socket.bind(50000);
	Player player;
	std::thread tr(&Resend);
	while(Players.size() < MAX_PLAYERS) {
		sf::IpAddress IP;
		unsigned short port;
		if (socket.receive(packetLog, IP, port) != sf::Socket::Done) {
			std::cout << "Error al recivir del cliente" << std::endl;
		}
		packetLog >> str_CON;
		if (str_CON == "HELLO")
		{
			packetLog >> player.name;
			player.IP = IP;
			player.port = port;
			player.posX = rand() % 470;
			player.posY = rand() % 470;
			player.ID = i;
			if (Players.size() == 0) {// si es el primer jugador sera caco
				player.caco = true;
			}
			else { player.caco = false; }

			Players.insert(std::pair<int, Player>(player.ID, player));

		
			packetLog.clear();
			packetLog << (int)Players.size();
			for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
				std::string cmd = "CMD_WELCOME";
				packetLog << cmd << it->second.name << it->first << it->second.posX << it->second.posY << it->second.caco;
				std::cout << "Se ha conectado : " << it->second.name << cmd << it->first << it->second.posX << it->second.posY << " " << it->second.caco << std::endl;
			}
			if (socket.send(packetLog, IP, port) != sf::Socket::Done) {
				std::cout << "Error al enviar el WELCOME al cliente";
			}
			NewPlayer(player);
			packetLog.clear();
			i++;
		}
			CheckNewPlayer();
	}	
}

//bucle principal
void Game() {
	socket.setBlocking(false);
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		it->second.timePing.restart(); //ponemos todos los clocks internos a 0
	}
	sf::IpAddress IP;
	unsigned short port;
	sf::Packet packR, packPingS;
	std::string ping;
	int newPlayer = 0;
	sf::Packet packM;
	sf::Clock clockSend, clockP, clockAcum;
	clockSend.restart();
	clockAcum.restart();
	clockP.restart();
	int id;
	std::string cmd;
	while (true) {
		float rndPacketLoss = GerRandomFloat();

		if (socket.receive(packR, IP, port) != sf::Socket::Done) {
		}
		//perdida de paquetes
		//if (rndPacketLoss < PERCENT_LOSS) {
			//packR.clear();
		//}
		else {
			packR >> cmd;
			if (cmd == "CMD_ACK") {
				packR >> id;
				Players.find(id)->second.timePing.restart();
			}
				if (cmd == "CMD_MOV") {
					int idMove, id;
					float deltaX, deltaY;
					packR >> idMove >> id >> deltaX >> deltaY;
					std::string a = "CMD_OK_MOVE";
					packM << a;
					for (std::map<int, Player>::iterator it2 = Players.begin(); it2 != Players.end(); ++it2) {
						if (it2->first == id) {
							Movment movAux;
							movAux.IDMove = idMove;
							movAux.movX += deltaX;
							movAux.movY += deltaY;
							//voy añadiendo al map mientras no se compla el tiempo
							it2->second.movment.insert((std::pair<int, Movment>(idMove, movAux)));
							if (clockAcum.getElapsedTime().asMilliseconds() > 100) {
								//compruebo con las paredes
								if ((it2->second.posX += it2->second.movment[it2->second.movment.size() - 1].movX) > maxX) {
									it2->second.posX = maxX;
									packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
								}
								else if ((it2->second.posX += it2->second.movment[it2->second.movment.size() - 1].movX) < minX) {
									it2->second.posX = minX;
									packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
								}
								else if ((it2->second.posY += it2->second.movment[it2->second.movment.size() - 1].movY) > maxY) {
									it2->second.posY = maxY;
									packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
								}
								else if ((it2->second.posY += it2->second.movment[it2->second.movment.size() - 1].movY) < minY) {
									it2->second.posY = minY;
									packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
								}
								else {
									//siempre comprueba la ultima posicion del map para asi validar las anteriores	
									it2->second.posX += it2->second.movment[it2->second.movment.size() - 1].movX;
									it2->second.posY += it2->second.movment[it2->second.movment.size() - 1].movY;
									packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
								}
								clockAcum.restart();
							}
						}

					}
				}
				if (cmd == "CMD_CACO") {
					sf::Packet packACKC;
					int idP1;
					packR >> idP1;
					for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
						if (it->first == idP1) {
							if (!it->second.caco) {
								it->second.caco = true;
								counter++;
								packACKC << "CMD_ACK_CACO" << it->first << it->second.caco;
							}
						}
					}
					for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
						if (socket.send(packACKC, it->second.IP, it->second.port) != sf::Socket::Done) {
							std::cout << "Error al enviar mov" << std::endl;
						}
					}
				}	
		}
		//GameManager
		TimeGame();
		//send PING
		if (clockP.getElapsedTime().asMilliseconds() > 150) {
			for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
				ping = "CMD_PING";
				packPingS << ping;
				if (socket.send(packPingS, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al enviar el ping" << std::endl;
				}
				else {

					packPingS.clear();
					clockP.restart();
				}
			}

		}
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			if (it->second.timePing.getElapsedTime().asSeconds() >= 5) {
				sendAllPlayers("CMD_DESC", it->first);
				it->second.connected = false;
			}
		}
		//comprobar si tengo que eliminar a alguien
		for (int i = 0; i < Players.size(); i++) {
			if (Players.find(i) != Players.end() && !Players[i].connected) {
				std::cout << Players[i].ID << " disconnected." << std::endl;
				Players.erase(Players[i].ID);
			}
		}
		//send mov
		if (clockSend.getElapsedTime().asMilliseconds() > 50) {
			for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
				if (socket.send(packM, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al enviar mov" << std::endl;
				}
			}
			packM.clear();
			clockSend.restart();
		}

		packR.clear();
	}
}

int main()
{
	Connection();
	
	do {
		Game();
	} while (Players.size() >= 0);
	return 0;
}