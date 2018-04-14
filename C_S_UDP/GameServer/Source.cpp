#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>

#define MAX_PLAYERS 2
sf::UdpSocket socket;

struct Movment
{
	float movX, movY;
	int IDMove;
};
struct Player
{
	sf::IpAddress IP;
	unsigned short port;
	float posX, posY;
	std::string name;
	sf::Clock timePing;
	Movment movment;
};

int ID;
std::map<int, Player> Players;
int maxY = 499;
int minY = 1;
int maxX = 499;
int minX = 1;
//StateModes --> chat_mode - countdown_mode - bet_money_mode - bet_number_mode - simulate_game_mode - bet_processor_mode

void Recorrer() {
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		for (std::map<int, Player>::iterator it2 = Players.begin(); it2 != Players.end(); ++it2) {
			sf::Packet pack;
			pack << it2->first << it2->second.posX << it2->second.posY;
			if (socket.send(pack, it->second.IP, it->second.port) != sf::Socket::Done) {
				std::cout << "error";
			}
		}
	}

}

void Connection() {
	srand(time(NULL));
	sf::Packet packetLog;
	sf::Packet newPlayerPack;

	socket.bind(50000);
	Player player;
	for (int i = 0; i < MAX_PLAYERS; i++) {
		sf::IpAddress IP;
		unsigned short port;
		if (socket.receive(packetLog, IP, port) != sf::Socket::Done) {
			std::cout << "Error al recivir" << std::endl;
		}
		packetLog >> player.name;
		player.IP = IP;
		player.port = port;
		player.posX = rand() % 499;
		player.posY = rand() % 499;

		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			std::string cmd = "CMD_NEW_PLAYER";
			newPlayerPack << cmd << player.name  << i << player.posX << player.posY;
			if (socket.send(newPlayerPack, it->second.IP, it->second.port) != sf::Socket::Done) {
				std::cout << "Error al enviar nueva conexion" << std::endl;
			}
			newPlayerPack.clear();
		}

		Players.insert(std::pair<int, Player>(i, player));

		packetLog.clear();
		ID = i;
		packetLog << (int)Players.size();

		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			std::string cmd = "CMD_WELCOME";
			packetLog << cmd << it->second.name << it->first << it->second.posX << it->second.posY;
			std::cout << "Se ha conectado : " << it->second.name << cmd << it->first << it->second.posX << it->second.posY << std::endl;
		}
		if (socket.send(packetLog, IP, port) != sf::Socket::Done) {
			std::cout << "error";
		}
		packetLog.clear();
	}
	
}
void sendAllPlayers(std::string msg, int id) {
	sf::Packet packDes;
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		std::string cmd = "CMD_DESC";
		packDes << cmd << msg << id;
		if (socket.send(packDes, it->second.IP, it->second.port) != sf::Socket::Done) {
			std::cout << "Error al enviar nueva conexion" << std::endl;
		}
		packDes.clear();
	}
}

void Game() {
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		it->second.timePing.restart();
	}
	socket.setBlocking(false);
	sf::IpAddress IP;
	unsigned short port;
	bool send = false;
	sf::Packet packR, packPingS;
	sf::Clock clockP;
	clockP.restart();
	std::string ping;
	int id;
	std::string cmd;
	while (true) {
		if (clockP.getElapsedTime().asMilliseconds() > 1000) {
			for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
				ping = "CMD_PING";
				packPingS << ping;
				if (socket.send(packPingS, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al enviar el ping" << std::endl;
				}
				else {
					clockP.restart();
					packPingS.clear();
				}
			}
		}
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			if (it->second.timePing.getElapsedTime().asSeconds() >= 5) {
				sendAllPlayers("Desconectado con la ID: ", it->first);
				std::cout << "Desconexion con la ID: " << it->first << std::endl;
				Players.erase(it->first);
			}
		}
		if (socket.receive(packR, IP, port) != sf::Socket::Done) {
		}
		else {
			packR >> cmd;
			if (cmd == "CMD_ACK") {
				packR >> id;
				//std::cout << id << ping;
				Players.find(id)->second.timePing.restart();
			}
			if (cmd == "CMD_MOV") {
				int idMove, id;
				float deltaX, deltaY;
				packR >> idMove >> id >> deltaX >> deltaY;
				std::string a = "CMD_OK_MOVE";
				sf::Packet packM;
				packM << a;
				for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
					if (it->first == id) {
						it->second.movment.IDMove = idMove;
						it->second.movment.movX = deltaX;
						it->second.movment.movY = deltaY;
						it->second.posX += it->second.movment.movX;
						it->second.posY += it->second.movment.movY;
						packM << it->first << it->second.movment.IDMove << it->second.posX << it->second.posY;
						std::cout << " ID" << it->first << " IDM " << it->second.movment.IDMove << " X " << deltaX << " Y " << deltaY << " posX " << it->second.posX << " posY " << it->second.posY;

					}	
					socket.send(packM, it->second.IP, it->second.port);
					packM.clear();
				}
				
			}
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
	// TODO gestion de desconexion y PING
	return 0;
}