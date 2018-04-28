#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>
#include <random>

#define MAX_PLAYERS 4
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
	sf::IpAddress IP;
	unsigned short port;
	int ID;
}; 
struct Player
{
	sf::IpAddress IP;
	unsigned short port;
	bool connected = true;
	int ID;
	bool resend = false;
	float posX, posY;
	std::string name;
	sf::Clock timePing;
	std::map<int, Movment> movment;
	std::vector<Movment>interpolation;
	std::map<int, PacketCritic> mapPacketCritic;
};

int ID;

std::map<int, Player> Players;
float maxY = 480;
float minY = 1.f;
float maxX = 480.f;
float minX = 1.f;
int packID = 1;
//StateModes --> chat_mode - countdown_mode - bet_money_mode - bet_number_mode - simulate_game_mode - bet_processor_mode
static float GerRandomFloat() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> dis(0.f, 1.f);
	return dis(gen);
}

void A() {
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
						std::cout << "OK" << Players.find(id)->second.mapPacketCritic.find(idPacket)->first << Players.find(id)->first << std::endl;
						Players.find(id)->second.mapPacketCritic.erase(idPacket);
						//newPlayer++;
					}
				}
			}
		}
}
void NewPlayer(Player player) {

	PacketCritic packCritic;

	if (Players.size() > 1) {
		packCritic.ID = player.ID;
		//std::cout << "newPlayer  " << packID << " " << packCritic.ID << std::endl;
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			std::string cmd = "CMD_NEW_PLAYER";
			if (it->first != player.ID) {
				packCritic.packet << cmd << packID << player.name << player.ID << player.posX << player.posY;
				if (socket.send(packCritic.packet, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al enviar nueva conexion" << std::endl;
				}
				it->second.mapPacketCritic.insert(std::pair <int, PacketCritic>(packID, packCritic));
			}
		}
		
		packID++;
	}		

	//packCritic.packet.clear();
}

void Resend(sf::Packet pack) {
	std::cout << "NOOK";
	for (int i = 0; i < Players.size(); i++) {
		//if()
		if (socket.send(pack, Players.find(i)->second.IP, Players.find(i)->second.port) != sf::Socket::Done) {
			std::cout << "Error al enviar nueva conexion" << std::endl;
		}
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
void Connection() {
	srand(time(NULL));
	sf::Packet packetLog;
	bool resend = false;
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
		/*if (str_CON == "CMD_ACK_NEW") {
			int idPacket, id;
			packetLog >> idPacket >> id;
			if (idPacket == Players.find(id)->second.mapPacketCritic.find(idPacket)->first && Players.find(id)->second.mapPacketCritic.find(idPacket) != Players.find(id)->second.mapPacketCritic.end()) {
				std::cout << "OK" << Players.find(id)->second.mapPacketCritic.find(idPacket)->first << Players.find(id)->first;
				Players.find(id)->second.mapPacketCritic.erase(idPacket);

			}
		}*/
		if (str_CON == "HELLO")
		{
			packetLog >> player.name;
			player.IP = IP;
			player.port = port;
			player.posX = rand() % 480;
			player.posY = rand() % 480;
			player.ID = i;

			Players.insert(std::pair<int, Player>(player.ID, player));

		
			packetLog.clear();
			packetLog << (int)Players.size();
			for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
				std::string cmd = "CMD_WELCOME";
				packetLog << cmd << it->second.name << it->first << it->second.posX << it->second.posY;
				std::cout << "Se ha conectado : " << it->second.name << cmd << it->first << it->second.posX << it->second.posY << std::endl;
			}
			if (socket.send(packetLog, IP, port) != sf::Socket::Done) {
				std::cout << "error";
			}
			NewPlayer(player);
			packetLog.clear();
			i++;
		}
		A();
		/*if (clockResend.getElapsedTime().asMilliseconds() > 300) {
			for (int j = 1; j <= mapPacketCritic.size(); j++) {
				Resend(mapPacketCritic.find(j)->second.packet);
			}
			clockResend.restart();
		}*/
		
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
	sf::Clock clockSend;
	clockSend.restart();
	int id;
	std::string cmd;
	while (true) {

		float rndPacketLoss = GerRandomFloat();

		if (socket.receive(packR, IP, port) != sf::Socket::Done) {
		}
		/*if (rndPacketLoss < PERCENT_LOSS) {
			packR.clear();
			std::cout << rndPacketLoss << std::endl;
		}*/
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
		}
	/*	if (mapPacketCritic.size() > 0) {
			for (int j = 1; j <= mapPacketCritic.size(); j++) {
				Resend(mapPacketCritic.find(j)->second.packet);
			}

		}*/
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
		for (std::map<int, Player>::iterator it2 = Players.begin(); it2 != Players.end(); ++it2) {
			it2->second.movment.erase(it2->first);
		}
	}
}

int main()
{

	Connection();

//	A();
	do {
	//	A();
		//
		Game();
	} while (Players.size() >= 0);
	//tr.join();
	return 0;
}