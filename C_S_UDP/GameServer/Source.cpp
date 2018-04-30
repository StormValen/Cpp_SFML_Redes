#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>
#include <random>

#define MAX_PLAYERS 3
#define PERCENT_LOSS 0.05
sf::UdpSocket socket;

struct Movment
{
	float movX = 0, movY = 0;
	int IDMove;
};
struct PacketCritic
{
	sf::Packet packet;
	sf::IpAddress IP;
	unsigned short port;
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

int ID;

std::map<int, Player> Players;
float maxY = 470;
float minY = 20.f;
float maxX = 470.f;
float minX = 20.f;
int packID = 1;
int coco = 0;
int counter = 1;
sf::Clock clockTime;

//StateModes --> chat_mode - countdown_mode - bet_money_mode - bet_number_mode - simulate_game_mode - bet_processor_mode
static float GerRandomFloat() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> dis(0.f, 1.f);
	return dis(gen);
}
void Resend() {
	sf::Clock clockResend;
	clockResend.restart();
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		while (it->second.mapPacketCritic.size() > 0) {
			if (clockResend.getElapsedTime().asMilliseconds() > 300) {
				if (it != Players.end()) {
					if (it->second.mapPacketCritic.size() > 0) {
						//std::cout << "Este tiene " << Players.find(i)->second.name << std::endl;
						for (int j = 1; j <= it->second.mapPacketCritic.size(); ++j) {
							std::cout << "IDPACKETE a enviar " << it->second.mapPacketCritic.find(j)->first << std::endl;
							if (it->second.mapPacketCritic.find(j) != it->second.mapPacketCritic.end()) {
								//std::cout << "Siguiente 2" << std::endl;
								if (socket.send(it->second.mapPacketCritic.find(j)->second.packet, it->second.IP, it->second.port) != sf::Socket::Done) {
									std::cout << "Error al enviar nueva conexion" << std::endl;
								}
								std::cout << it->second.name << std::endl;
							}
						}
					}
					clockResend.restart();
				}
			}
		}
	}
}
void CheckNewPlayer() {
	std::thread tr(&Resend);
		sf::IpAddress IP;
		unsigned short port;
		sf::Packet packetLog;
		std::string str_CON;
		for (int i = 0; i < Players.size(); i++) {
			while (Players.find(i)->second.mapPacketCritic.size() > 0) {
				//std::cout << "hol222a";
				if (socket.receive(packetLog, IP, port) != sf::Socket::Done) {
					std::cout << "Error al recivir" << std::endl;
				}
				packetLog >> str_CON;
				//std::cout << str_CON;
				if (str_CON == "CMD_ACK_NEW") {
					int idPacket, id;
					packetLog >> idPacket >> id;
					//std::cout << idPacket << " " << id << std::endl;
					if (idPacket == Players.find(id)->second.mapPacketCritic.find(idPacket)->first && Players.find(id)->second.mapPacketCritic.find(idPacket) != Players.find(id)->second.mapPacketCritic.end()) {
						//std::cout << "OK" << Players.find(id)->second.mapPacketCritic.find(idPacket)->first << Players.find(id)->first << std::endl;
						//std::cout << "Elimino el packet " << Players.find(id)->second.mapPacketCritic.find(idPacket)->first << " de " << Players.find(id)->second.name << std::endl;
						Players.find(id)->second.mapPacketCritic.erase(idPacket);
					}
				}
			}
		}
		tr.join();
}
void NewPlayer(Player player) {
	PacketCritic packCritic;

	if (Players.size() > 1) {
		packCritic.ID = player.ID;
		//std::cout << "newPlayer  " << packID << " " << packCritic.ID << std::endl;
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			std::string cmd = "CMD_NEW_PLAYER";
			if (it->first != player.ID) {
				packCritic.packet << cmd << packID << player.name << player.ID << player.posX << player.posY << player.caco;
				if (socket.send(packCritic.packet, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al enviar nueva conexion" << std::endl;
				}
				it->second.mapPacketCritic.insert(std::pair <int, PacketCritic>(packID, packCritic));
			}
		}	
		packID++;
	}		
}
void sendAllPlayers(std::string cmd, int id) {
	sf::Packet packDes;
	if (cmd == "CMD_DESC") {
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			//std::string cmd = "CMD_DESC";
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
void Reset() {
	srand(time(NULL));
	counter = 1;
	clockTime.restart();
	sf::Packet packetReset;
	//std::cout << "RESET";
	packetReset << "CMD_RESET";
	int random = rand() % 3;
	for (int i = 0; i < Players.size(); i++) {
		packetReset << Players.find(i)->first << Players.find(i)->second.puntos;
		Players.find(i)->second.caco = false;
		if (Players.find(i)->first == random) {
			Players.find(i)->second.caco = true;
			std::cout << random  << " " << Players.find(i)->second.name << std::endl;
		}
		packetReset << Players.find(i)->second.caco;
	}
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		if (socket.send(packetReset, it->second.IP, it->second.port) != sf::Socket::Done) {
			std::cout << "Error al enviar mov" << std::endl;
		}
	}
}
void TimeGame() {
	sf::Packet packPoints;
	if (clockTime.getElapsedTime().asSeconds() > 40 && counter < MAX_PLAYERS) {
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			if (!it->second.caco) {	
				it->second.puntos++;
			}
		}
		/*for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			packPoints << "CMD_PUNTOS" << it->first << it->second.puntos;
			if (socket.send(packPoints, it->second.IP, it->second.port) != sf::Socket::Done) {
				std::cout << "Error al enviar mov" << std::endl;
			}
			std::cout << it->second.name << it->second.puntos << std::endl;
		}*/
		Reset();
		//packPoints.clear();
		clockTime.restart();
	}
	else if (counter == MAX_PLAYERS) {
		/*for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			packPoints << "CMD_PUNTOS" << it->first << it->second.puntos;
			if (socket.send(packPoints, it->second.IP, it->second.port) != sf::Socket::Done) {
				std::cout << "Error al enviar mov" << std::endl;
			}
			std::cout << it->second.name << it->second.puntos << std::endl;
		}*/
		Reset();
	}
}

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

	while(Players.size() < MAX_PLAYERS) {
		sf::IpAddress IP;
		unsigned short port;
		if (socket.receive(packetLog, IP, port) != sf::Socket::Done) {
			std::cout << "Error al recivir" << std::endl;
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
			if (Players.size() == 0) {
				//coco++;
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
				std::cout << "error";
			}
			NewPlayer(player);
			packetLog.clear();
			i++;
		}
		CheckNewPlayer();
	
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
	int newPlayer = 0;
	sf::Packet packM;
	sf::Clock clockMov, clockAcum;
	clockMov.restart();
	clockAcum.restart();
	clockTime.restart();
	sf::Clock clockSend;
	clockSend.restart();
	int id;
	std::string cmd;
	while (true) {

		float rndPacketLoss = GerRandomFloat();

		if (socket.receive(packR, IP, port) != sf::Socket::Done) {
		}
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
						it2->second.movment.insert((std::pair<int, Movment>(idMove, movAux)));
						if (clockAcum.getElapsedTime().asMilliseconds() > 100) {

							if ((it2->second.posX += deltaX) > maxX) {
								it2->second.posX = maxX;
								packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
							}
							else if ((it2->second.posX += deltaX) < minX) {
								it2->second.posX = minX;
								packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
							}
							else if ((it2->second.posY += deltaY) > maxY) {
								it2->second.posY = maxY;
								packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
							}
							else if ((it2->second.posY += deltaY) < minY) {
								it2->second.posY = minY;
								packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
							}
							else {
								//acmular aqui		
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
							//std::cout << counter << std::endl;
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
		if (clockP.getElapsedTime().asMilliseconds() > 1000) {
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
		//for (std::map<int, Player>::iterator it2 = Players.begin(); it2 != Players.end(); ++it2) {
			//it2->second.movment.erase(it2->first);
		//}
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