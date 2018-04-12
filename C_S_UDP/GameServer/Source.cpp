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
		// Bucle que informa a los anteriores del nuevo jugador
		/*for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		//std::cout << it->second.name << " --> New player connected:  " << player.name << " " << player.posX << player.posY << std::endl;
		packetLog << player.name << player.posX << player.posY;
		if (socket.send(packetLog, IP, port) != sf::Socket::Done) {
		std::cout << "error";
		}
		}
		packetLog.clear();*/
		Players.insert(std::pair<int, Player>(i, player));
		packetLog.clear();
		ID = i;
		packetLog << (int)Players.size();
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			std::string welcome = " Bienvenido, te has conectado con el servidor, tu ID es :";
			packetLog << it->second.name << welcome << it->first << it->second.posX << it->second.posY;
			std::cout << "Se ha conectado : " << it->second.name << welcome << it->first << it->second.posX << it->second.posY << std::endl;
		}
		//Recorrer();
		if (socket.send(packetLog, IP, port) != sf::Socket::Done) {
			std::cout << "error";
		}
		packetLog.clear();
	}
	
}
void sendAllPlayers(std::string msg, int id) {
	sf::Packet packSendAll;
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		packSendAll << msg <<id;
		if (socket.send(packSendAll, it->second.IP, it->second.port) != sf::Socket::Done) {
			std::cout << "Error al enviar la desconexion" << std::endl;
		}
	}
}

void Ping() {
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		it->second.timePing.restart();
	}
	socket.setBlocking(false);
	sf::IpAddress IP;
	unsigned short port;
	bool send = false;
	sf::Packet packPingR, packPingS;
	sf::Clock clockP;
	clockP.restart();
	std::string ping;
	int id;
	while (true) {
		if (clockP.getElapsedTime().asMilliseconds() > 1000) {
			for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
				ping = "PING";
				packPingS << ping;
				if (socket.send(packPingS, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al enviar el ping" << std::endl;
				}
				else {
					clockP.restart();
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
		if (socket.receive(packPingR, IP, port) != sf::Socket::Done) {
			//std::cout << "Error al recivir pingafasfasfa" << std::endl; //esto salta todo el rato hasta q no le llega el mensaje, pero no siginifa q no llegue sino q tarda
		}
		else {
			packPingR >> id >> ping;
			//std::cout << id << ping;
			Players.find(id)->second.timePing.restart();
		}
		packPingR.clear();
		packPingS.clear();
	}

}

int main()
{
	//SocketSelector();
	Connection();
	do {
		Ping();
	} while (Players.size() >= 0);
	// TODO gestion de desconexion y PING
	return 0;
}