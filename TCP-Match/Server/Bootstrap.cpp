﻿#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <string>

#define MAX_PLAYERS 2

//StateModes --> chat_mode - countdown_mode - bet_money_mode - bet_number_mode - simulate_game_mode - bet_processor_mode

std::list<sf::TcpSocket*> myClients; //Lista con todos los clientes conectados.
std::string currentState = "chat_mode";
int counterForChat = 0;
sf::Packet packLogin, packCreate, packSend, packRecv;
sf::SocketSelector mySocketSelector, socketSelectorGame;
sf::Socket::Status status;
sf::TcpListener listener;
bool end = false;
int clientsConnectedCounter = 0;
bool gameIsReady = false;
int IDGame = 0;
std::vector<std::thread> threads;
void do_join(std::thread& tr) {
	tr.join();
}
void all_join(std::vector<std::thread>& v) {
	std::for_each(v.begin(), v.end(), do_join);
}

class Player {
public:
	sf::TcpSocket* sock;
	std::string nickname, correo, pasword;
	int money, bet, betMoney;
	bool isReady;
	int IDGame;
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
bool ArePlayersReady(int IDG) {
	for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
		Player& player = **it;
		if (player.isReady == false) {//coimprobar los de la misma partida no todos los jugadores guardados
			return false;
		}
	}
	return true;
}
void GameLoop(int IDG, int maxPlayers, int maxMoney, Player* player, std::string nameGame) {
	std::string comand = " Has creado una partida, espera a que se conecten todos";
	packSend.clear();
	packSend << comand;
	player->sock->send(packSend);
	int IDAux;
	while (true) {
		if (currentState == "chat_mode" && !gameIsReady) {
			packRecv.clear();
			if (mySocketSelector.wait()) {
				std::cout << "estoy esperando" << std::endl;
				for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
					Player& iplayer = **it;
					if (mySocketSelector.isReady(*iplayer.sock) && iplayer.IDGame == IDG) {
						//std::cout << "Crep un THREAD" << std::endl;
						//Server recibe mensajes
						std::string mesage;
						std::cout << "Antes recv GAME" << std::endl;
						status = iplayer.sock->receive(packRecv);
						std::cout << "Despues recv GAME" << std::endl;

						if (status == sf::Socket::Done) {
							packRecv >> IDAux >> mesage;
							//enviar la info a los otros jugadores
							for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
								Player& player = **it;
								if (player.IDGame == IDAux && player.nickname != iplayer.nickname) {//comprobar que no sea uno mismo!
									packSend.clear();
									packSend << mesage;

									player.sock->send(packSend);

								}
							}
							//mesage = buffer;
							std::cout << "Client with port: [" << iplayer.sock->getRemotePort() << "] SEND: " << mesage << std::endl;
							if (mesage == "ready") {
								iplayer.isReady = true;
							}
						}

						//Cliente desconectado
						else if (status == sf::Socket::Disconnected) {
							mySocketSelector.remove(*iplayer.sock);

							//eliminar el socket de la lista
							std::cout << "Client with port: [" << iplayer.sock->getRemotePort() << "] DISCONECTED " << std::endl;
							clientsConnectedCounter--;

							std::list<Player*> auxPlayers;

							for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
								Player& bPlayer = **it;

								if (bPlayer.sock != iplayer.sock) {
									auxPlayers.push_back(&bPlayer);
								}
								packSend.clear();
								std::string connectedMesage = "GAME INFO: -- " + player->nickname + " left the game -- Wait until a new player connects";
								packSend << connectedMesage;
								if (bPlayer.IDGame == IDG && iplayer.IDGame == IDG) { //informo a los de la misma partida
									bPlayer.sock->send(packSend);
									bPlayer.isReady = false;
								}
							}
							gameIsReady = false;
							aPlayers = auxPlayers;
							//do_join(threads[0]);
						}

						if (ArePlayersReady(IDG)) {
							std::cout << "entro";
							currentState = "countdown_mode";
							packSend.clear();
							for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
								Player& player = **it;
								std::string mesage = "GAME INFO: -- Chatting time has finished -- Press Enter to continue ...";
								packSend << mesage;
								if (player.IDGame == IDG) { //comrpueba q sea la misma partidaa
									player.sock->send(packSend);
								}
							}
						}
					}
					//}
				//}
				}
			}
			else if (currentState == "countdown_mode") {

				//SAFE
				packRecv.clear();
				for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
					Player& iPlayer2 = **it;
					if (socketSelectorGame.isReady(*iPlayer2.sock) && iPlayer2.isReady) {

						std::string mesage;
						//char buffer[2000];
						//std::size_t received;
						status = iPlayer2.sock->receive(packRecv);
						if (status == sf::Socket::Done) {
							//desempaquetar ID
							std::cout << "Client with port: [" << iPlayer2.sock->getRemotePort() << "] OK " << std::endl;
						}

						if (ArePlayersReady(IDG)) {
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
					if (player.IDGame == IDG) { //envio a los de la misma partida
						player.sock->send(packSend);
						player.isReady = false;
					}
				}

			}
			else if (currentState == "bet_money_mode") {
				packRecv.clear();
				for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
					Player& iPlayer = **it;

				if (socketSelectorGame.isReady(*iPlayer.sock) && ! iPlayer.isReady) {

					//Server recibe apuestas
					std::string mesage;

					//char buffer[2000];
					//std::size_t received;
					status = iPlayer.sock->receive(packRecv);
					packRecv >> IDAux >> mesage;
					std::cout << " P " << maxMoney << " J " << mesage << "  " << iPlayer.money << std::endl;
					if (status == sf::Socket::Done) {
						//mesage = buffer;
						int temp = atoi(mesage.c_str());
						if (iPlayer.money - temp >= 0 && temp <= maxMoney && iPlayer.IDGame == IDAux) { //comrpueba q no apuesta mas del maximo de la partida
							iPlayer.betMoney = temp;
							iPlayer.money -= temp;
							std::cout << "Client with port: [" << iPlayer.sock->getRemotePort() << "] BET MONEY: " << iPlayer.betMoney << std::endl;
							iPlayer.isReady = true;
						}
						else {
							packSend.clear();
							std::string mesage = "GAME INFO: -- No tienes dinero suficiento o pasas del limite de la partida, vuelve a probar";
							packSend << mesage;
							iPlayer.sock->send(packSend);
						}
					}
				}
				if (ArePlayersReady(IDG)) {
					currentState = "bet_number_mode";
					packSend.clear();
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& player = **it;
						std::string mesage = "GAME INFO: -- Enter your bet number: <0-36> Numbers";
						if (player.IDGame == IDG) { //enviamos solo a los de la partida
							packSend << mesage;
							player.sock->send(packSend);
							packSend.clear();
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
			}
			else if (currentState == "bet_number_mode") {
				packRecv.clear();
				for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
					Player& iPlayer = **it;
					if (socketSelectorGame.isReady(*iPlayer.sock) && !iPlayer.isReady) {

						//Server recibe apuestas
						std::string mesage;
						status = iPlayer.sock->receive(packRecv);
						packRecv >> IDAux >> mesage;
						if (status == sf::Socket::Done) {
							//mesage = buffer;
							int temp = stoi(mesage);//atoi

							if (temp >= 0 && temp <= 48) {
								iPlayer.bet = temp;
								std::cout << "Client with port: [" << iPlayer.sock->getRemotePort() << "] BET: " << iPlayer.bet << std::endl;
								iPlayer.isReady = true;
							}
							else {
								packSend.clear();
								std::string mesage = "GAME INFO: -- This number doesn't exist, please enter a valid number";
								packSend << mesage;
								if (iPlayer.IDGame == IDG) {
									iPlayer.sock->send(packSend);
								}
							}
						}
					}
					if (ArePlayersReady(IDG)) {
						currentState = "chat_mode";
						packSend.clear();
						for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
							Player& player = **it;
							std::string mesage = "GAME INFO: -- You bet [" + std::to_string(player.betMoney) + "]$ on number [" + std::to_string(player.bet) + "]  -- You have a total credit of [" + std::to_string(player.money) + "]$";
							//player.sock->send(mesage.c_str(), mesage.length() + 1);
							//mesage = "GAME INFO: -- You have a total credit of [" + std::to_string(player.money) + "]$";
							//packSend << mesage;
							if (player.IDGame == IDG) {//hago todo el proceso si es de l partida
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

								int rojos[] = { 1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36 }; //18 -->37
								int negros[] = { 2,4,6,8,10,11,13,15,17,20,22,24,26,28,29,31,33,35 }; //18 -->38
								int par[] = { 0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34 }; //19 -->39
								int impar[] = { 1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33 }; //18 -->40
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
								mesage = "GAME INFO: -- You win [" + std::to_string(apuestaGanancia) + "]$ your current creadit is [" + std::to_string(player.money) + "]$";
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
		//}
	}
}
class GamesManager
{
public:
	int sizeMax = 0;
	int maxPlayers;
	int maxMoney;
	int IDG;
	void CreateGame(Player* player, std::string name, int _maxPlayers, int _maxMoney, int _ID) {
		IDG = _ID;
		maxMoney = _maxMoney;
		maxPlayers = _maxPlayers;
		//std::thread tr(&GameLoop, player->IDGame, maxPlayers, maxMoney, player, name); //inicialiar un thread como puntero
		threads.push_back(std::thread (&GameLoop, player->IDGame, maxPlayers, maxMoney, player, name));
		//tr.join();

	}

	void JoinGame() {

	}
	void DeleteGame(std::string name);
	void ShowGames() {

	}
};

std::map<std::string, GamesManager*> gameManager;

void InfoNewPlayer(Player* player) {
	for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
		Player& iPlayer = **it;
		if (player->IDGame == iPlayer.IDGame && iPlayer.nickname != player->nickname) {
			std::string connectedMesage = " GAME INFO: -- " + player->nickname + " se ha conectado";
			packLogin << connectedMesage;
			iPlayer.sock->send(packLogin);
		}
	}
}
bool CheckGame(std::string name, Player* player) { //comprueba que se peudan unir a esa partida
	gameManager.find(name)->second->sizeMax++;
	if (name == gameManager.find(name)->first && gameManager.find(name)->second->sizeMax < gameManager.find(name)->second->maxPlayers && player->money <= gameManager.find(name)->second->maxMoney) {
		return true;
	}
	else {
		return false;
	}
}


void ListAvailableGames(Player* actualPlayer) {

	sf::TcpSocket* newClient = new sf::TcpSocket;

	Player* newPlayer_1 = new Player(newClient, "");
	Player* newPlayer_2 = new Player(newClient, "");
	Player* newPlayer_3 = new Player(newClient, "");;



	GamesManager *gameManagerAux = new GamesManager();
	gameManagerAux->CreateGame(newPlayer_1, "Test_Game_Non_Functional_1", 2, 100, IDGame);
	IDGame++;
	gameManager.insert(std::pair<std::string, GamesManager*>("Test_Game_Non_Functional_1", gameManagerAux));

	gameManagerAux->CreateGame(newPlayer_2, "Test_Game_Non_Functional_2", 2, 100, IDGame);
	IDGame++;
	gameManager.insert(std::pair<std::string, GamesManager*>("Test_Game_Non_Functional_2", gameManagerAux));

	gameManagerAux->CreateGame(newPlayer_3, "Test_Game_Non_Functional_3", 2, 100, IDGame);
	IDGame++;
	gameManager.insert(std::pair<std::string, GamesManager*>("Test_Game_Non_Functional_3", gameManagerAux));

	packCreate << "LISTA DE PARTIDAS DISPONIBLES: \n";
	actualPlayer->sock->send(packCreate);
	packCreate.clear();


	for (std::map<std::string, GamesManager*>::iterator it = gameManager.begin(); it != gameManager.end(); it++) {
		std::cout << "Nombre partida: " << it->first << std::endl;

		std::string text = "NOMBRE: " + it->first;
		packCreate << text;
	}
}

void CrearUnir(Player* newPlayer, std::string name) {

	

	std::string modo;
	/*packSend.clear();
	packSend << newPlayer->money;
	newPlayer->sock->send(packSend);*/
	std::string confirmText = "Bienvenido [ " + name + " ], Si quieres crear una partida aprieta 1, si quieres unirte a una ya creado apreta 2 ";
	packCreate << confirmText;
	newPlayer->sock->send(packCreate);
	packCreate.clear();
	std::cout << "antes rcv unirse" << std::endl;
	newPlayer->sock->receive(packCreate);
	std::cout << "despues rcv unirse" << std::endl;
	packCreate >> modo;
	packCreate.clear();
	if (stoi(modo) == 1) { // crear una partida
		std::string maxPlayers, maxMoney, name;
		GamesManager *gameManagerAux = new GamesManager();
		std::string specs = " Introduce el nombre de la partida, el numero max de jugadores y dinero";
		packCreate.clear();
		packCreate << specs;
		newPlayer->sock->send(packCreate);
		packCreate.clear();
		newPlayer->sock->receive(packCreate);
		packCreate >> name;
		packCreate.clear();
		newPlayer->sock->receive(packCreate);
		packCreate >> maxPlayers;
		packCreate.clear();
		newPlayer->sock->receive(packCreate);
		packCreate >> maxMoney;
		packCreate.clear();
		std::string comand = "CMD_WELCOME";
		packSend.clear();
		packSend << comand << IDGame;
		newPlayer->sock->send(packSend);
		newPlayer->IDGame = IDGame;
		//mySocketSelector.remove(*newPlayer->sock);
		//socketSelectorGame.add(*newPlayer->sock);
		gameManagerAux->CreateGame(newPlayer, name, stoi(maxPlayers), stoi(maxMoney), IDGame);
		IDGame++;
		gameManager.insert(std::pair<std::string, GamesManager*>(name, gameManagerAux));
		std::cout << "creo y aumento" << std::endl;
	}
	else if (stoi(modo) == 2) {
		std::string mesage = " Introduce el nombre de la partida ";
		packSend.clear();
		packSend << mesage;
		newPlayer->sock->send(packSend);
		newPlayer->sock->receive(packRecv);
		packRecv >> mesage;
		if (CheckGame(mesage, newPlayer)) {
			//mySocketSelector.remove(*newPlayer->sock);
			//socketSelectorGame.add(*newPlayer->sock);
			//elimino otro
			std::string welcome = "Te has conectado a la partida: " + mesage;
			newPlayer->IDGame = gameManager.find(mesage)->second->IDG;
			packSend.clear();
			packSend << welcome;
			newPlayer->sock->send(packSend);
			std::string comand = "CMD_WELCOME";
			packSend.clear();
			packSend << comand << gameManager.find(mesage)->second->IDG;
			newPlayer->sock->send(packSend);
			packSend.clear();
			InfoNewPlayer(newPlayer);
		}
		else {
			std::string a = " Esa partida no existe o no puedes conectarte por los requisitos ";
			packSend.clear();
			packSend << a;
			newPlayer->sock->send(packSend);
		}
	}
	else if (stoi(modo) == 3) {
		ListAvailableGames(newPlayer);
	}
}


void NewConnection() {
	std::string modo;
	if (mySocketSelector.wait()) {
		if (mySocketSelector.isReady(listener)) { //gamerady
			sf::TcpSocket* newClient = new sf::TcpSocket;
			Player* newPlayer = new Player(newClient, "");

			std::size_t received;
			if (listener.accept(*newPlayer->sock) == sf::Socket::Done) {
				//Bucle para todos los clientes -> Nuevo cliente conectado.
				//Antes de añadir el nuevo cliente para no tener que comparalos.

				mySocketSelector.add(*newPlayer->sock);
				packLogin.clear();
				std::string login = "Si tienes una cuenta aprieta 1, si tienes que registrarte aprieta 2 ";
				packCreate << login;
				newPlayer->sock->send(packCreate);
				packCreate.clear();
				std::cout << "antes rcv hello" << std::endl;
				newPlayer->sock->receive(packCreate);
				std::cout << "despues rcv hello" << std::endl;
				packCreate >> modo;
				packCreate.clear();
				if (stoi(modo) == 1) {
					std::string nameAux, paswordAux;
					login = "Introduce tu usuario y contraseña en orden ";
					packSend.clear();
					packSend << login;
					newPlayer->sock->send(packSend);
					packRecv.clear();
					newPlayer->sock->receive(packRecv);
					packRecv >> nameAux;
					packRecv.clear();
					newPlayer->sock->receive(packRecv);
					packRecv >> paswordAux;
					for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
						Player& iPlayer = **it;

						if (iPlayer.nickname == nameAux && iPlayer.pasword == paswordAux) {
							//
							std::cout << "conexion de " << iPlayer.nickname << "  " << iPlayer.pasword << std::endl;
							login = "CMD_WB";
							packSend.clear();
							packSend << login << iPlayer.nickname;
							newPlayer->sock->send(packSend);
							CrearUnir(newPlayer, iPlayer.nickname);
						}
						else {
							login = "El usuario o constraseña no existe, cierra y vuelve a intentarlo";
							packSend.clear();
							packSend << login;
							newPlayer->sock->send(packSend);
							std::cout << "no existe";
						}
					}		
				}
				if (stoi(modo) == 2) {
					std::string money;
					std::string login = "Introduce por orden tu usuario, contrareña y dinero ";
					packSend.clear();
					packSend << login;
					newPlayer->sock->send(packSend);
					packRecv.clear();
					newPlayer->sock->receive(packRecv);
					packRecv >> newPlayer->nickname;
					packRecv.clear();
					newPlayer->sock->receive(packRecv);
					packRecv >> newPlayer->pasword;
					packRecv.clear();
					newPlayer->sock->receive(packRecv);
					packRecv >> money;
					newPlayer->money = stoi(money);
					aPlayers.push_back(newPlayer);
					std::cout << "Se ha logeado [" << newPlayer->nickname << "] con pasword [" << newPlayer->pasword << "] y dinero [" << newPlayer->money << "]"  << std::endl;
					login = "CMD_LOGED";
					packSend.clear();
					packSend << login << newPlayer->nickname;
					newPlayer->sock->send(packSend);
					CrearUnir(newPlayer, newPlayer->nickname);
				}
			}
			else {
				std::cout << "ERROR: Can't set connection" << std::endl;
				delete newClient;
			}
		}


	}
}

void SocketSelector() {
	
	std::string modo;
	char name[200];
	char money[200];
	// -- Abrir listener

	status = listener.listen(50000);
	if (status != sf::Socket::Done) {
		std::cout << "ERROR: Can't open listener";
		exit(0);
	}

	mySocketSelector.add(listener);

	//Socket selector tiene el listener + clients.
	//Primero comprueba si el listener recibe alguna connexion nueva.
	//Si no se hace el manage de los clientes.

	while (true) {
		NewConnection();	
	}
	listener.close();
	mySocketSelector.clear();
		
}

int main()
{
	SocketSelector();

	system("pause");
	return 0;

	/*else if (gameIsReady) {
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
	}*/
}