#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>

#define MAX_PLAYERS 4

std::list<sf::TcpSocket*> myClients; //Lista con todos los clientes conectados.
std::string currentState = "chat";
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
		bet = betMoney = 0;
		isReady = false;
	}
};

std::list<Player*> aPlayers; // Contenedor de jugadores
int clientsConnectedCounter = 0;
bool gameIsReady = false;

void thread_game() {
	bool tBucle = true;
	while (tBucle) {
		counterForChat++;
		if (counterForChat == 5000) {
			tBucle = false;
			currentState = "notChat";
			std::cout << "Chat finished" << std::endl;
		}
	}
}

bool ArePlayersReady() {
	for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
		Player& player = **it;
		std::cout << player.nickname << player.isReady << std::endl;
		if (player.isReady == false) {
			return false;
		}
	}
	return true;
}

void SocketSelector() {
	bool end = false;
	
	char name[200];

	// -- Abrir listener
	sf::TcpListener listener;
	sf::Socket::Status status = listener.listen(50000);
	if (status != sf::Socket::Done) {
		std::cout << "ERROR: Can't open listener";
		exit(0);
	}

	sf::SocketSelector mySocketSelector;
	mySocketSelector.add(listener);

	//Socket selector tiene el listener + clients.
	//Primero comprueba si el listener recibe alguna connexion nueva.
	//Si no se hace el manage de los clientes.
	 
	while (!end) {
		if (mySocketSelector.wait()) {
			if (mySocketSelector.isReady(listener) && !gameIsReady) {
				sf::TcpSocket* newClient = new sf::TcpSocket;
				Player* newPlayer = new Player(newClient,"");

				std::size_t received;
				if (listener.accept(*newPlayer->sock) == sf::Socket::Done) {
					//Bucle para todos los clientes -> Nuevo cliente conectado.
					//Antes de añadir el nuevo cliente para no tener que comparalos.

					newPlayer->sock->receive(name, sizeof(name), received);

					std::cout << "Client with port: [" << newPlayer->sock->getRemotePort() << "] and name [" << name << "] CONNECTED" << std::endl;
					mySocketSelector.add(*newPlayer->sock);
					newPlayer->nickname = name;

					aPlayers.push_back(newPlayer);

					std::string confirmText = "INFO: You have entered the game, please wait until all players are connected ...";
					size_t bytesSent;
					newPlayer->sock->send(confirmText.c_str(), confirmText.length());

					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;
						std::string connectedMesage = "GAME INFO: " + newPlayer->nickname + " has been connected ---";
						iPlayer.sock->send(connectedMesage.c_str(), connectedMesage.length() + 1);
					}

					clientsConnectedCounter++;

					if (clientsConnectedCounter == 4) {
						gameIsReady = true;
						std::cout << "All 4 player are connected ... Starting Game ..." << std::endl;

						for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
							Player& iPlayer = **it;
							std::string connectedMesage = "GAME INFO: All 4 player are connected ... Starting Game ...";
							iPlayer.sock->send(connectedMesage.c_str(), connectedMesage.length() + 1);
						}

						//std::thread tr(&thread_game);
						//tr.join();
					}
				}
				else {
					std::cout << "ERROR: Can't set connection" << std::endl;
					delete newClient;
				}
			}
			else if (gameIsReady) {
				if (currentState == "chat") {
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;
						if (mySocketSelector.isReady(*iPlayer.sock)) {


							//Server recibe mensajes
							std::string mesage;
							char buffer[2000];
							std::size_t received;
							status = iPlayer.sock->receive(buffer, sizeof(buffer), received);
							if (status == sf::Socket::Done) {
								mesage = buffer;
								std::cout << "Client with port: [" << iPlayer.sock->getRemotePort() << "] SEND: " << mesage << std::endl;
								if (mesage == ">ready") {
									iPlayer.isReady = true;
								}
							}

							//Cliente desconectado
							else if (status == sf::Socket::Disconnected) {
								mySocketSelector.remove(*iPlayer.sock);
								//eliminar el socket de la lista
								std::cout << "Client with port: [" << iPlayer.sock->getRemotePort() << "] DISCONECTED " << std::endl;

								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& bPlayer = **it;
									std::string connectedMesage = "GAME INFO: " + iPlayer.nickname + " left the game ---";
									bPlayer.sock->send(connectedMesage.c_str(), connectedMesage.length() + 1);
								}

							}

							//Server reenvia mensajes
							for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
								Player& player = **it;
								if (player.sock->getRemotePort() != iPlayer.sock->getRemotePort()) {
									player.sock->send(mesage.c_str(), mesage.length() + 1);
								}
							}

							if (ArePlayersReady()) {
								currentState = "notChat";
							}
							
						}
					}
				}
				else if (currentState == "notChat") {
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& player = **it;
						std::string mesage = "Chatting time has finished";
						player.sock->send(mesage.c_str(), mesage.length() + 1);
						
					}
				}
			}
		}
	}
	listener.close();
	mySocketSelector.clear();
	}

int main()
{
	SocketSelector();
	
	std::thread counter(&thread_game);
	counter.join();
	

	


	system("pause");
	return 0;
}