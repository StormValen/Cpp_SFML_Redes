#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>

#define MAX_PLAYERS 2
sf::UdpSocket socket;

struct Player
{
	sf::IpAddress IP;
	unsigned short port;
	float posX, posY;
	std::string name;
	sf::Clock timePing;
};

int ID;
std::map<int, Player> Players;

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



/*std::list<sf::TcpSocket*> myClients; //Lista con todos los clientes conectados.
std::string currentState = "chat_mode";
int counterForChat = 0;

class Player {
public:
	sf::TcpSocket* sock;
	std::string nickname;
	int money, bet, betMoney;
	bool isReady;

	Player() {

	}

	Player(sf::TcpSocket* _sock, std::string _nickname) {
		sock = _sock;
		nickname = "";
		money = 100;
		bet = betMoney = -1;
		isReady = false;
	}
};

std::list<Player*> aPlayers; // Contenedor de jugadores
int clientsConnectedCounter = 0;
bool gameIsReady = false;

void Countdown() {
	bool end = false;
	while (!end) {
		if (counterForChat == 10000) {
			end = true;
			std::cout << counterForChat << std::endl;
			counterForChat = 0;
		}
		else {
			std::cout << counterForChat << '\r';
		}
		
		counterForChat++;
	}
}

bool ArePlayersReady() {
	for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
		Player& player = **it;
		if (player.isReady == false) {
			return false;
		}
	}
	return true;
}*/
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
		player.posX = rand() % 8;
		player.posY = rand() % 8;
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
			//std::cout << (int)Players.size();
			std::cout << it->second.name << it->first << it->second.posX << it->second.posY;
		}
		//Recorrer();
		if (socket.send(packetLog, IP, port) != sf::Socket::Done) {
			std::cout << "error";
		}
		packetLog.clear();
	}
	
}
void Ping() {
	bool send = false;
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		sf::Clock clockP;
		clockP.restart();
		sf::Packet packPing;
		std::string ping = "PING";
		packPing << ping;
		//while (!send) {
			//if (clockP.getElapsedTime().asSeconds() > 1) {
				if (socket.send(packPing, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al enviar el ping" << std::endl;
				}
				std::cout << "Send" << it->second.name;
				send = true;
				packPing.clear();
				clockP.restart();
			//}
		//}
				if (socket.receive(packPing, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al recivir ping" << std::endl;
				}
				packPing >> ping;
				std::cout << ping;
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