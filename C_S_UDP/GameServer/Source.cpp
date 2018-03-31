#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>

#define MAX_PLAYERS 4

//StateModes --> chat_mode - countdown_mode - bet_money_mode - bet_number_mode - simulate_game_mode - bet_processor_mode

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
	sf::UdpSocket socket;
	sf::Packet packetLog;
	socket.bind(50000);
	sf::IpAddress IP;
	unsigned short port;
	if (socket.receive(packetLog, IP, port) != sf::Socket::Done) {
		std::cout << "Error al recivir" << std::endl;
	}
	std::string name;
	packetLog >> name;
	std::cout << name << std::endl;
	packetLog.clear();
	int ID = 1;
	float posX = 0.0f;
	float posY = 3.0f;
	packetLog << ID << posX << posY;
	if (socket.send(packetLog, IP, port) != sf::Socket::Done) {
		std::cout << "error";
	}
}

/*void SocketSelector() {
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
								clientsConnectedCounter--;

								std::list<Player*> auxPlayers;

								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& bPlayer = **it;

									if (bPlayer.sock != iPlayer.sock) {
										auxPlayers.push_back(&bPlayer);
									}

									std::string connectedMesage = "GAME INFO: -- " + iPlayer.nickname + " left the game ---";
									bPlayer.sock->send(connectedMesage.c_str(), connectedMesage.length() + 1);
									connectedMesage = "GAME INFO: -- Wait until anew player connects";
									bPlayer.sock->send(connectedMesage.c_str(), connectedMesage.length() + 1);
									bPlayer.isReady = false;
								}
								gameIsReady = false;
								aPlayers = auxPlayers;
							}

							//Server reenvia mensajes
							for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
								Player& player = **it;
								if (player.sock->getRemotePort() != iPlayer.sock->getRemotePort()) {
									std::string auxNameMesage = "[ " + iPlayer.nickname + " ]> " + mesage;
									player.sock->send(auxNameMesage.c_str(), auxNameMesage.length() + 1);
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
								std::string mesage = "GAME INFO: -- Enter your bet number:";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
								mesage = "           -- <0-36> Numbers";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
								mesage = "           -- <37-38>Rojos / Negros";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
								mesage = "           -- <39-40>Pares / Impares";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
								mesage = "           -- <41-42-43>Filas";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
								mesage = "           -- <44-45-46>Docenas";
								player.sock->send(mesage.c_str(), mesage.length() + 1);
								mesage = "           -- <47-48> 0-18 / 19-36";
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

								if (temp >= 0 && temp <= 48) {
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
								int randomNumber = rand() % 37;
								std::cout << randomNumber << std::endl;


								mesage = "GAME INFO: -- Winner number: " + std::to_string(randomNumber) + " !!!!!";
								player.sock->send(mesage.c_str(), mesage.length() + 1);

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
	}*/

int main()
{
	//SocketSelector();
	Connection();
	system("pause");
	return 0;
}