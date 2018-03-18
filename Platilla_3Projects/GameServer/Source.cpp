#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>

#define MAX_PLAYERS 1

//StateModes --> chat_mode - countdown_mode - bet_money_mode - bet_number_mode - simulate_game_mode - bet_processor_mode

std::list<sf::TcpSocket*> myClients; //Lista con todos los clientes conectados.
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
						std::string connectedMesage = "GAME INFO: -- " + newPlayer->nickname + " has been connected";
						iPlayer.sock->send(connectedMesage.c_str(), connectedMesage.length() + 1);
					}

					clientsConnectedCounter++;

					if (clientsConnectedCounter == MAX_PLAYERS) {
						gameIsReady = true;
						std::cout << "All players are connected ... Starting Game ..." << std::endl;

						for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
							Player& iPlayer = **it;
							std::string connectedMesage = "GAME INFO: -- All players are connected ... Starting Game ...";
							iPlayer.sock->send(connectedMesage.c_str(), connectedMesage.length() + 1);
						}
					}
				}
				else {
					std::cout << "ERROR: Can't set connection" << std::endl;
					delete newClient;
				}
			}
			else if (gameIsReady) {
				std::cout << "GAME STATE: " << currentState << std::endl;
				if (currentState == "chat_mode") {
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;
						if (mySocketSelector.isReady(*iPlayer.sock) && !iPlayer.isReady) {


							//Server recibe mensajes
							std::string mesage;
							char buffer[2000];
							std::size_t received;
							status = iPlayer.sock->receive(buffer, sizeof(buffer), received);
							if (status == sf::Socket::Done) {
								mesage = buffer;
								std::cout << "Client with port: [" << iPlayer.sock->getRemotePort() << "] SEND: " << mesage << std::endl;
								if (mesage == "ready") {
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
									std::string connectedMesage = "GAME INFO: -- " + iPlayer.nickname + " left the game ---";
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
								currentState = "countdown_mode";
								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& player = **it;
									std::string mesage = "GAME INFO: -- Chatting time has finished -- Press Enter to continue ...";
									player.sock->send(mesage.c_str(), mesage.length() + 1);
								}
							}
						}
					}
				}
				else if (currentState == "countdown_mode") {

					//SAFE
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;
						if (mySocketSelector.isReady(*iPlayer.sock) && iPlayer.isReady) {
							
							std::string mesage;
							char buffer[2000];
							std::size_t received;
							status = iPlayer.sock->receive(buffer, sizeof(buffer), received);
							if (status == sf::Socket::Done) {
								std::cout << "Client with port: [" << iPlayer.sock->getRemotePort() << "] OK " << std::endl;
							}

							if (ArePlayersReady()) {
								currentState = "countdown_mode";
								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& player = **it;
								}
							}
						}
					}
					//ENDSAFE

					Countdown();
					currentState = "bet_money_mode";
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& player = **it;
						std::string mesage = "GAME INFO: -- Enter your bet money";
						player.sock->send(mesage.c_str(), mesage.length() + 1);
						player.isReady = false;
					}
					
				}
				else if (currentState == "bet_money_mode") {
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;
						
						if (mySocketSelector.isReady(*iPlayer.sock) && !iPlayer.isReady) {
							
							//Server recibe apuestas
							std::string mesage;
							char buffer[2000];
							std::size_t received;
							status = iPlayer.sock->receive(buffer, sizeof(buffer), received);
							if (status == sf::Socket::Done) {
								mesage = buffer;
								int temp = atoi(mesage.c_str());
								if (iPlayer.money - temp >= 0) {
									iPlayer.betMoney = temp;
									iPlayer.money -= temp;
									std::cout << "Client with port: [" << iPlayer.sock->getRemotePort() << "] BET MONEY: " << iPlayer.betMoney << std::endl;
									iPlayer.isReady = true;
								}
								else {
									std::string mesage = "GAME INFO: -- You don't have all that money, please enter a valid amount";
									iPlayer.sock->send(mesage.c_str(), mesage.length() + 1);
								}
							}
						}
						if (ArePlayersReady()) {
							currentState = "bet_number_mode";
							for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
								Player& player = **it;
								std::string mesage = "GAME INFO: -- Enter your bet number\n             <0-34> Numbers";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
								player.isReady = false;
							}
						}
					}
				}
				else if (currentState == "bet_number_mode") {
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;
						if (mySocketSelector.isReady(*iPlayer.sock) && !iPlayer.isReady) {

							//Server recibe apuestas
							std::string mesage;
							char buffer[2000];
							std::size_t received;
							status = iPlayer.sock->receive(buffer, sizeof(buffer), received);
							if (status == sf::Socket::Done) {
								mesage = buffer;
								int temp = atoi(buffer);

								if (temp >= 0 && temp <= 34) {
									iPlayer.bet = temp;
									std::cout << "Client with port: [" << iPlayer.sock->getRemotePort() << "] BET: " << iPlayer.bet<< std::endl;
									iPlayer.isReady = true;
								}
								else {
									std::string mesage = "GAME INFO: -- This number doesn't exist, please enter a valid number";
									iPlayer.sock->send(mesage.c_str(), mesage.length() + 1);
								}
							}
						}
						if (ArePlayersReady()) {
							currentState = "chat_mode";
							for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
								Player& player = **it;
								std::string mesage = "GAME INFO: -- You bet [" + std::to_string(player.betMoney) + "]$ on number [" + std::to_string(player.bet) +"]";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
								mesage = "GAME INFO: -- You have a total credit of [" + std::to_string(player.money) + "]$";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
								player.isReady = false;
								srand(time(NULL));
								//int randomNumber = rand() % 35;
								int randomNumber = 5;
								std::cout << randomNumber << std::endl;
								mesage = "GAME INFO: -- Winner number: " + std::to_string(randomNumber) + " !!!!!";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
								int apuestaGanancia = 0;
								if (player.bet == randomNumber) {
									apuestaGanancia = player.betMoney * 36;
								}
								player.money += apuestaGanancia;
								mesage = "GAME INFO: -- You win [" + std::to_string(apuestaGanancia) + "]$ your current creadit is [" + std::to_string(player.money) +"]$";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
								mesage = "GAME INFO: -- Chatting time has started -- Enter 'ready' to start a new game";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
							}
						}
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

	system("pause");
	return 0;
}