#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>

#define MAX_PLAYERS 2

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
	sf::Packet packLogin, packSend, packRecv;
	char name[200];
	char money[200];
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

					newPlayer->sock->receive(packLogin);
					packLogin >> newPlayer->nickname >> newPlayer->money;

					std::cout << "Client with port: [" << newPlayer->sock->getRemotePort() << "] and name [" << newPlayer->nickname << "] and money [" << newPlayer->money << "]  CONNECTED" << std::endl;
					mySocketSelector.add(*newPlayer->sock);
					//newPlayer->nickname = name;
					aPlayers.push_back(newPlayer);
					packLogin.clear();
					std::string confirmText = "INFO: You have entered the game, please wait until all players are connected ...";
					size_t bytesSent;
					packLogin << confirmText;
					newPlayer->sock->send(packLogin);
					packLogin.clear();
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;
						std::string connectedMesage = "GAME INFO: -- " + newPlayer->nickname + " has been connected";
						packLogin << connectedMesage;
						iPlayer.sock->send(packLogin);
					}

					clientsConnectedCounter++;

					if (clientsConnectedCounter == MAX_PLAYERS) {
						gameIsReady = true;
						std::cout << "All players are connected ... Starting Game ..." << std::endl;
						packLogin.clear();
						for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
							Player& iPlayer = **it;
							std::string connectedMesage = "GAME INFO: -- All players are connected ... Starting Game ...";
							packLogin << connectedMesage;
							iPlayer.sock->send(packLogin);
						}
					}
				}
				else {
					std::cout << "ERROR: Can't set connection" << std::endl;
					delete newClient;
				}
			}
			else if (gameIsReady) {
				if (currentState == "chat_mode") {
					packRecv.clear();
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;
						if (mySocketSelector.isReady(*iPlayer.sock) && !iPlayer.isReady) {


							//Server recibe mensajes
							std::string mesage;
							//char buffer[2000];
							//std::size_t received;
							status = iPlayer.sock->receive(packRecv);
							packRecv >> mesage;
							if (status == sf::Socket::Done) {
								//mesage = buffer;
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
								clientsConnectedCounter--;

								std::list<Player*> auxPlayers;

								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& bPlayer = **it;

									if (bPlayer.sock != iPlayer.sock) {
										auxPlayers.push_back(&bPlayer);
									}
									packSend.clear();
									std::string connectedMesage = "GAME INFO: -- " + iPlayer.nickname + " left the game -- Wait until a new player connects";
									//packSend << connectedMesage;
									//bPlayer.sock->send(connectedMesage.c_str(), connectedMesage.length() + 1);
									//connectedMesage = "GAME INFO: -- Wait until a new player connects";
									packSend << connectedMesage;
									bPlayer.sock->send(packSend);
									bPlayer.isReady = false;
								}
								gameIsReady = false;
								aPlayers = auxPlayers;
							}

							//Server reenvia mensajes
							packSend.clear();
							for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
								Player& player = **it;
								if (player.sock->getRemotePort() != iPlayer.sock->getRemotePort()) {
									std::string auxNameMesage = "[ " + iPlayer.nickname + " ]> " + mesage;
									packSend << auxNameMesage;
									player.sock->send(packSend);
								}
							}

							if (ArePlayersReady()) {
								currentState = "countdown_mode";
								packSend.clear();
								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& player = **it;
									std::string mesage = "GAME INFO: -- Chatting time has finished -- Press Enter to continue ...";
									packSend << mesage;
									player.sock->send(packSend);
								}
							}
						}
					}
				}
				else if (currentState == "countdown_mode") {

					//SAFE
					packRecv.clear();
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;
						if (mySocketSelector.isReady(*iPlayer.sock) && iPlayer.isReady) {

							std::string mesage;
							//char buffer[2000];
							//std::size_t received;
							status = iPlayer.sock->receive(packRecv);
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
					packSend.clear();
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& player = **it;
						std::string mesage = "GAME INFO: -- Enter your bet money";
						packSend << mesage;
						player.sock->send(packSend);
						player.isReady = false;
					}
					
				}
				else if (currentState == "bet_money_mode") {
					packRecv.clear();
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;
						
						if (mySocketSelector.isReady(*iPlayer.sock) && !iPlayer.isReady) {
							
							//Server recibe apuestas
							std::string mesage;

							//char buffer[2000];
							//std::size_t received;
							status = iPlayer.sock->receive(packRecv);
							packRecv >> mesage;
							if (status == sf::Socket::Done) {
								//mesage = buffer;
								int temp = atoi(mesage.c_str());
								if (iPlayer.money - temp >= 0) {
									iPlayer.betMoney = temp;
									iPlayer.money -= temp;
									std::cout << "Client with port: [" << iPlayer.sock->getRemotePort() << "] BET MONEY: " << iPlayer.betMoney << std::endl;
									iPlayer.isReady = true;
								}
								else {
									packSend.clear();
									std::string mesage = "GAME INFO: -- You don't have all that money, please enter a valid amount";
									packSend << mesage;
									iPlayer.sock->send(packSend);
								}
							}
						}
						if (ArePlayersReady()) {
							currentState = "bet_number_mode";
							packSend.clear();
							for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
								Player& player = **it;
								std::string mesage = "GAME INFO: -- Enter your bet number: <0-36> Numbers";
								packSend << mesage;
								mesage = "-- <37-38>Rojos / Negros";
								packSend << mesage;
								player.sock->send(packSend);
								packSend.clear();
								mesage = "-- <39-40>Pares / Impares";
								packSend << mesage;
								player.sock->send(packSend);
								packSend.clear();
								mesage = "-- <41-42-43>Filas";
								packSend << mesage;
								player.sock->send(packSend);
								packSend.clear();
								mesage = "-- <44-45-46>Docenas";
								packSend << mesage;
								player.sock->send(packSend);
								packSend.clear();
								mesage = "-- <47-48> 0-18 / 19-36";
								packSend << mesage;
								player.sock->send(packSend);
								packSend.clear();
								player.isReady = false;
							}
						}
					}
				}
				else if (currentState == "bet_number_mode") {
					packRecv.clear();
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;
						if (mySocketSelector.isReady(*iPlayer.sock) && !iPlayer.isReady) {

							//Server recibe apuestas

							std::string mesage;
						//	char buffer[2000];
							//std::size_t received;
							status = iPlayer.sock->receive(packRecv);
							packRecv >> mesage;
							if (status == sf::Socket::Done) {
								//mesage = buffer;
								int temp = stoi(mesage);//atoi

								if (temp >= 0 && temp <= 48) {
									iPlayer.bet = temp;
									std::cout << "Client with port: [" << iPlayer.sock->getRemotePort() << "] BET: " << iPlayer.bet<< std::endl;
									iPlayer.isReady = true;
								}
								else {
									packSend.clear();
									std::string mesage = "GAME INFO: -- This number doesn't exist, please enter a valid number";
									packSend << mesage;
									iPlayer.sock->send(packSend);
								}
							}
						}
						if (ArePlayersReady()) {
							currentState = "chat_mode";
							packSend.clear();
							for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
								Player& player = **it;
								std::string mesage = "GAME INFO: -- You bet [" + std::to_string(player.betMoney) + "]$ on number [" + std::to_string(player.bet) +"]  -- You have a total credit of [" + std::to_string(player.money) + "]$";
								//player.sock->send(mesage.c_str(), mesage.length() + 1);
								//mesage = "GAME INFO: -- You have a total credit of [" + std::to_string(player.money) + "]$";
								//packSend << mesage;
								player.sock->send(packSend);
								packSend.clear();
								player.isReady = false;
								srand(time(NULL));
								int randomNumber = rand() % 37;
								std::cout << randomNumber << std::endl;


								mesage = "GAME INFO: -- Winner number: " + std::to_string(randomNumber) + " !!!!!";
								packSend << mesage;
								player.sock->send(packSend);
								packSend.clear();

								int rojos[] = {1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36 }; //18 -->37
								int negros[] = { 2,4,6,8,10,11,13,15,17,20,22,24,26,28,29,31,33,35 }; //18 -->38
								int par[] = { 0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34 }; //19 -->39
								int impar[] = { 1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33}; //18 -->40
								int filaA[] = { 3,6,9,12,15,18,21,24,27,30,33,36 }; //12 -->41
								int filaB[] = { 2,5,8,11,14,17,20,23,26,29,32,35 }; //12 -->42
								int filaC[] = { 1,4,7,10,13,16,19,22,25,28,31,34 }; //12 -->43
								int docenaA[] = { 1,2,3,4,5,6,7,8,9,10,11,12 }; //12 -->44
								int docenaB[] = { 13,14,15,16,17,18,19,20,21,22,23,24 }; //12 -->45
								int docenaC[] = { 25,26,27,28,29,30,31,32,33,34,35,36 }; //12 -->46
								int mA[] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 }; //18 -->47
								int mB[] = { 19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36 }; //18 -->48


								int apuestaGanancia = 0;
								if (player.bet == 37) { //apuesta a rojos
									for (int i = 0; i < 18; i++) {
										if (randomNumber == rojos[i]) {
											apuestaGanancia = player.betMoney * 2;
										}
									}
								}

								if (player.bet == 38) { //apuesta a negros
									for (int i = 0; i < 18; i++) {
										if (randomNumber == negros[i]) {
											apuestaGanancia = player.betMoney * 2;
										}
									}
								}

								if (player.bet == 39) { //apuesta a pares
									for (int i = 0; i < 19; i++) {
										if (randomNumber == par[i]) {
											apuestaGanancia = player.betMoney * 2;
										}
									}
								}

								if (player.bet == 40) { //apuesta a impares
									for (int i = 0; i < 19; i++) {
										if (randomNumber == impar[i]) {
											apuestaGanancia = player.betMoney * 2;
										}
									}
								}

								if (player.bet == 41) { //apuesta a fila 1
									for (int i = 0; i < 12; i++) {
										if (randomNumber == filaA[i]) {
											apuestaGanancia = player.betMoney * 3;
										}
									}
								}
								if (player.bet == 42) { //apuesta a fila 2
									for (int i = 0; i < 12; i++) {
										if (randomNumber == filaB[i]) {
											apuestaGanancia = player.betMoney * 3;
										}
									}
								}
								if (player.bet == 43) { //apuesta a fila 3
									for (int i = 0; i < 12; i++) {
										if (randomNumber == filaC[i]) {
											apuestaGanancia = player.betMoney * 3;
										}
									}
								}

								if (player.bet == 44) { //apuesta a docena 1
									for (int i = 0; i < 12; i++) {
										if (randomNumber == docenaA[i]) {
											apuestaGanancia = player.betMoney * 3;
										}
									}
								}

								if (player.bet == 45) { //apuesta a docena 2
									for (int i = 0; i < 12; i++) {
										if (randomNumber == docenaB[i]) {
											apuestaGanancia = player.betMoney * 3;
										}
									}
								}
								if (player.bet == 46) { //apuesta a docena 3
									for (int i = 0; i < 12; i++) {
										if (randomNumber == docenaC[i]) {
											apuestaGanancia = player.betMoney * 3;
										}
									}
								}

								if (player.bet == 47) { //apuesta a 0-18
									for (int i = 0; i < 12; i++) {
										if (randomNumber == mA[i]) {
											apuestaGanancia = player.betMoney * 2;
										}
									}
								}

								if (player.bet == 48) { //apuesta a 18-0
									for (int i = 0; i < 12; i++) {
										if (randomNumber == mB[i]) {
											apuestaGanancia = player.betMoney * 2;
										}
									}
								}
								if (player.bet == randomNumber) { //acierta numero exacto
									apuestaGanancia = player.betMoney * 36;
								}

								player.money += apuestaGanancia;
								mesage = "GAME INFO: -- You win [" + std::to_string(apuestaGanancia) + "]$ your current creadit is [" + std::to_string(player.money) +"]$";
								packSend << mesage;
								player.sock->send(packSend);
								packSend.clear();
								mesage = "GAME INFO: -- Chatting time has started -- Enter 'ready' to start a new game";
								packSend << mesage;
								player.sock->send(packSend);
								packSend.clear();
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