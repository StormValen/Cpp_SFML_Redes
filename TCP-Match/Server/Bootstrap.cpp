#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn\driver.h>
#include <cppconn\resultset.h>
#include <cppconn\statement.h>
#include <cppconn\exception.h>

#define MAX_PLAYERS 2
#define HOST "tcp://192.168.1.40:3306"
#define user "root"
#define PASWORD "123456"
#define schema "RuletaBD"

std::string execute;
class DBManager
{
private:
	sql::Driver* driver;
	sql::Connection* con;
	sql::Statement* stmt;
public:
	int IDSesion, IDAc;
	DBManager() {
		driver = get_driver_instance();
		con = driver->connect(HOST, user, PASWORD);
		stmt = con->createStatement();
		stmt->execute("USE RuletaBD");
		//con->setSchema(schema);
	}
	bool Registrar(std::string name, std::string pasword, std::string money) {
		sql::ResultSet* rs;
		std::string comand = "SELECT COUNT(*) FROM UserAcount WHERE NameUser=";
		comand += "'"+name+"'";
		rs = stmt->executeQuery(comand.c_str()); //quiero comrpoar si hay alguien con ese nombre

		rs->next(); //retorna un bool
		std::cout << rs << std::endl;
			int num = rs->getInt(1);// poner el numero de la columna que queramos saber
			delete rs;
			if (num == 0) {//se puede hacer el insert
				execute = "INSERT INTO UserAcount(NameUser, Pasword, Money) VALUES(";
				execute += "'" + name + "'," + "'" + pasword + "'," + "'" + money + "') ";
				//std::cout << execute.c_str();
				stmt->execute(execute.c_str());//creo q tmb habra q meter el id
				return true;
			}
		
		else {
			std::cout << "NOO";
			return false;
		}
	}
	bool Login(std::string name, std::string pasword) {
		sql::ResultSet *rs;
		std::string command = "SELECT COUNT(*) FROM UserAcount WHERE NameUser=";
		command += "'" + name + "' and Pasword=" + "'" + pasword + "'";
		 rs = stmt->executeQuery(command.c_str());
		 rs->next();
		 std::cout << rs << std::endl;
			int adAc = rs->getInt(1); //numero columna
			if (adAc == 1) {
				delete rs;
				return true;
			}
		
		else {	
			delete rs;
			std::cout << "NOOOO";
			return false;
		}
	}
	~DBManager() {
		delete &driver;
		delete &stmt;
		delete &con;
	}

};
DBManager *dbManager = new DBManager();

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
	int money, bet, betMoney, IDAc;
	bool isReady, game;
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
		if (counterForChat == 500) {
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
		std::cout << player.nickname << " " << player.IDGame << std::endl;
		if (player.isReady == false) {
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
		packRecv.clear();
		for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); ++it) {
			Player& iplayer = **it;
 				if (mySocketSelector.wait() ) {
					std::cout << "Hola";
					if (mySocketSelector.isReady(*iplayer.sock) && iplayer.game == true && iplayer.IDGame == IDG) {
						std::cout << iplayer.nickname << std::endl;
						if (currentState == "chat_mode" && !gameIsReady) {
							std::string mesage;
							packRecv.clear();
							status = iplayer.sock->receive(packRecv);

							if (status == sf::Socket::Done) {
								packRecv >> IDAux >> mesage;
								//enviar la info a los otros jugadores
								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& player = **it;
									if (player.sock->getRemotePort() != iplayer.sock->getRemotePort() && player.IDGame == IDG && player.nickname != iplayer.nickname) {//comprobar que no sea uno mismo!
										packSend.clear();
										std::string auxNameMesage = "[ " + iplayer.nickname + " ]> " + mesage;
										packSend << auxNameMesage;

										player.sock->send(packSend);
									}
								}
								std::cout << "Client [" << iplayer.nickname << "] SEND: " << mesage << std::endl;
								if (mesage == "ready" && iplayer.IDGame == IDG && IDAux == IDG) {
									std::cout << "Client [" << iplayer.nickname << "] is ready: " << mesage << std::endl;
									iplayer.isReady = true;
								}
								if (mesage == "back") {

									std::cout << "QUIERT IR ATRASSS" << std::endl;
								}
							}

							//Cliente desconectado
							else if (status == sf::Socket::Disconnected) {
								std::cout << "Client with port: [" << iplayer.nickname << "] DISCONECTED " << std::endl;
								std::list<Player*> auxPlayers;

								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& bPlayer = **it;

									if (bPlayer.sock != iplayer.sock) {
										auxPlayers.push_back(&bPlayer);
									}
									packSend.clear();
									std::string connectedMesage = "GAME INFO: -- " + iplayer.nickname + " left the game -- Wait until a new player connects";
									packSend << connectedMesage;
									if (bPlayer.IDGame == IDG && bPlayer.nickname != iplayer.nickname) { //informo a los de la misma partida
										bPlayer.sock->send(packSend);
										bPlayer.isReady = false;
									}
								}
								//aPlayers.remove(&iplayer);
								gameIsReady = false;
								aPlayers = auxPlayers;
								mySocketSelector.remove(*iplayer.sock);
							}
							if (ArePlayersReady(IDG)) {
								currentState = "countdown_mode";
								packSend.clear();
								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& player = **it;
									std::string mesage = "GAME INFO: -- Chatting time has finished -- Press Enter to continue ...";
									packSend << mesage;
									if (player.IDGame == IDG && IDAux == IDG) { //comrpueba q sea la misma partidaa
										status = player.sock->send(packSend);
										if (status != sf::Socket::Done) {
											std::cout << "no se envia bien" << std::endl;
										}
									}
								}
							}
						}
						else if (currentState == "countdown_mode" && iplayer.IDGame == IDG) {
							//SAFE
							int idAux;
							packRecv.clear();
							std::string mesage;
							status = iplayer.sock->receive(packRecv);
							if (status == sf::Socket::Done) {
								packRecv >> idAux;
								if (idAux == IDG && iplayer.IDGame == IDG) {
									std::cout << "Client  [" << iplayer.nickname << "] OK " << std::endl;
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
							}
							else if (status == sf::Socket::Disconnected) {
								mySocketSelector.remove(*iplayer.sock);

								//eliminar el socket de la lista
								std::cout << "Client  [" << iplayer.nickname << "] DISCONECTED " << std::endl;

								std::list<Player*> auxPlayers;

								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& bPlayer = **it;

									if (bPlayer.sock != iplayer.sock) {
										auxPlayers.push_back(&bPlayer);
									}
									packSend.clear();
									std::string connectedMesage = "GAME INFO: -- " + iplayer.nickname + " left the game -- Wait until a new player connects";
									packSend << connectedMesage;
									if (bPlayer.IDGame == IDG && bPlayer.nickname != iplayer.nickname) { //informo a los de la misma partida
										bPlayer.sock->send(packSend);
										bPlayer.isReady = false;
									}
								}
								//aPlayers.remove(&iplayer);
								gameIsReady = false;
								aPlayers = auxPlayers;
							}
							

						}
						else if (currentState == "bet_money_mode" && iplayer.IDGame == IDG) {
							packRecv.clear();
							std::string mesage;
							status = iplayer.sock->receive(packRecv);
							packRecv >> IDAux >> mesage;
							if (IDG == IDAux && iplayer.IDGame == IDG) {
								std::cout << " P " << maxMoney << " J " << mesage << "  " << iplayer.money << std::endl;
								if (status == sf::Socket::Done) {
									//mesage = buffer;
									int temp = atoi(mesage.c_str());
									if (iplayer.money - temp >= 0 && temp <= maxMoney && iplayer.IDGame == IDG && IDAux == IDG) { //comrpueba q no apuesta mas del maximo de la partida
										iplayer.betMoney = temp;
										iplayer.money -= temp;
										std::cout << "Client  [" << iplayer.nickname << "] BET MONEY: " << iplayer.betMoney << std::endl;
										iplayer.isReady = true;
									}
									else {
										packSend.clear();
										std::string mesage = "GAME INFO: -- No tienes dinero suficiento o pasas del limite de la partida, vuelve a probar";
										packSend << mesage;
										iplayer.sock->send(packSend);
									}
								}
									else if (status == sf::Socket::Disconnected) {
								mySocketSelector.remove(*iplayer.sock);

								//eliminar el socket de la lista
								std::cout << "Client  [" << iplayer.nickname << "] DISCONECTED " << std::endl;
								clientsConnectedCounter--;

								std::list<Player*> auxPlayers;

								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& bPlayer = **it;

									if (bPlayer.sock != iplayer.sock) {
										auxPlayers.push_back(&bPlayer);
									}
									packSend.clear();
									std::string connectedMesage = "GAME INFO: -- " + iplayer.nickname + " left the game -- Wait until a new player connects";
									packSend << connectedMesage;
									if (bPlayer.IDGame == IDG && bPlayer.nickname != iplayer.nickname) { //informo a los de la misma partida
										bPlayer.sock->send(packSend);
										bPlayer.isReady = false;
									}
								}
								//aPlayers.remove(&iplayer);
								gameIsReady = false;
								aPlayers = auxPlayers;
							}
							}
						
							if (ArePlayersReady(IDG)) {
								currentState = "bet_number_mode";
								packSend.clear();
								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& player = **it;
									std::string mesage = "GAME INFO: -- Enter your bet number: <0-36> Numbers";
									if (player.IDGame == IDG && IDAux == IDG) { //enviamos solo a los de la partida
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
						else if (currentState == "bet_number_mode" && iplayer.IDGame == IDG) {
							packRecv.clear();

							//Server recibe apuestas
							std::string mesage;
							status = iplayer.sock->receive(packRecv);
							packRecv >> IDAux >> mesage;
							if (status == sf::Socket::Done) {
								if (IDAux == IDG && iplayer.IDGame == IDG) {
									//mesage = buffer;
									int temp = stoi(mesage);//atoi

									if (temp >= 0 && temp <= 48) {
										iplayer.bet = temp;
										std::cout << "Client [" << iplayer.nickname << "] BET: " << iplayer.bet << std::endl;
										iplayer.isReady = true;
									}
									else {
										packSend.clear();
										std::string mesage = "GAME INFO: -- This number doesn't exist, please enter a valid number";
										packSend << mesage;
										if (iplayer.IDGame == IDG) {
											iplayer.sock->send(packSend);
										}
									}
								}
								
							}
							else if (status == sf::Socket::Disconnected) {
								mySocketSelector.remove(*iplayer.sock);

								//eliminar el socket de la lista
								std::cout << "Client [" << iplayer.nickname << "] DISCONECTED " << std::endl;
								clientsConnectedCounter--;

								std::list<Player*> auxPlayers;

								for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
									Player& bPlayer = **it;

									if (bPlayer.sock != iplayer.sock) {
										auxPlayers.push_back(&bPlayer);
									}
									packSend.clear();
									std::string connectedMesage = "GAME INFO: -- " + iplayer.nickname + " left the game -- Wait until a new player connects";
									packSend << connectedMesage;
									if (bPlayer.IDGame == IDG && bPlayer.nickname != iplayer.nickname) { //informo a los de la misma partida
										bPlayer.sock->send(packSend);
										bPlayer.isReady = false;
									}
								}
								//aPlayers.remove(&iplayer);
								gameIsReady = false;
								aPlayers = auxPlayers;
							}
							//}
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
										mesage = "GAME INFO: -- Chatting time has started -- Enter 'ready' to start a new game, enter 'back' to go to the Lobby";
										packSend << mesage;
										player.sock->send(packSend);
										packSend.clear();
									}
								}

							}
							//}
						}
					}
				}
		}
	}
}
class GamesManager
{
public:
	int sizeMax = 1;
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
};

std::map<std::string, GamesManager*> gameManager;

void InfoNewPlayer(Player* player) {
	for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
		Player& iPlayer = **it;
		if (player->IDGame == iPlayer.IDGame && iPlayer.nickname != player->nickname) {
			std::string connectedMesage = " GAME INFO: -- " + player->nickname + " se ha conectado";
			packLogin << connectedMesage;
			status = iPlayer.sock->send(packLogin);
			if (status == sf::Socket::Done) {
			}
			else if (status == sf::Socket::Disconnected) {
				std::cout << "Ha habido una desconexion al informar New Player" << std::endl;
			}
		}
	}
}
bool CheckGame(std::string name, Player* player) { //comprueba que se peudan unir a esa partida
	gameManager.find(name)->second->sizeMax++;
	if (name == gameManager.find(name)->first && gameManager.find(name)->second->sizeMax <= gameManager.find(name)->second->maxPlayers && player->money <= gameManager.find(name)->second->maxMoney) {
		return true;
	}
	else {
		return false;
	}
}

void SelectGame(Player* newPlayer) {
	sf::Socket::Status statusRecv;
	std::string mesage = " Introduce el nombre de la partida ";
	packSend.clear();
	packSend << mesage;
	status = newPlayer->sock->send(packSend);
	if (status == sf::Socket::Done) {
		if (!newPlayer->game) {
			statusRecv = newPlayer->sock->receive(packRecv);
			if (statusRecv == sf::Socket::Done) {
				packRecv >> mesage;
				if (CheckGame(mesage, newPlayer)) {
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
					newPlayer->game = true;
					std::cout << newPlayer->nickname << "  " << newPlayer->IDGame << std::endl;
				}
				else {
					std::string a = " Esa partida no existe o no puedes conectarte por los requisitos ";
					packSend.clear();
					packSend << a;
					newPlayer->sock->send(packSend);
				}
			}
			else if (status == sf::Socket::Disconnected) {
				std::cout << "Se ha descoenctado alguien al seleccionar la partida" << std::endl; //no envio nada a los jugadores pq aun no esta en nignuna partidaa
			}
		}
	}
	else if (status == sf::Socket::Disconnected) {
		std::cout << "Se ha descoenctado alguien al seleccionar la partida" << std::endl; //no envio nada a los jugadores pq aun no esta en nignuna partidaa
	}
	
}
void ListAvailableGames(Player* actualPlayer) {
	std::string filtro;
	packCreate << "Si quieres filtrar por partidas que puedes entrar aprieta S, sino aprieta otra N: \n";
	actualPlayer->sock->send(packCreate);
	packCreate.clear();
	
	packRecv.clear();
	actualPlayer->sock->receive(packRecv);
	packRecv >> filtro;
	if (filtro == "S") {
		for (std::map<std::string, GamesManager*>::iterator it = gameManager.begin(); it != gameManager.end(); it++) {
			//std::cout << "Nombre partida: " << it->first << std::endl;
			if (it->second->sizeMax < it->second->maxPlayers) {

				std::string text = "NOMBRE: " + it->first;
				packCreate << text;
				status = actualPlayer->sock->send(packCreate);
				if (status == sf::Socket::Done) {
					packCreate.clear();

				}
				else if (status == sf::Socket::Disconnected) {
					std::cout << "Ha habido una desconexion al mostrar partidas" << std::endl; //no se envia aun
				}
			}
		}
	}
	if (filtro == "N") {
		for (std::map<std::string, GamesManager*>::iterator it = gameManager.begin(); it != gameManager.end(); it++) {
			std::cout << "Nombre partida: " << it->first << std::endl;
			std::string text = "NOMBRE: " + it->first;
			packCreate << text;
			status = actualPlayer->sock->send(packCreate);
			if (status == sf::Socket::Done) {
				packCreate.clear();

			}
			else if (status == sf::Socket::Disconnected) {
				std::cout << "Ha habido una desconexion al mostrar partidas" << std::endl; //no se envia aun
			}
		}
		
	}
	SelectGame(actualPlayer);
}

void CrearUnir(Player* newPlayer, std::string name) {
	sf::Socket::Status statusRecv;
	std::string modo;
	std::string confirmText = "Bienvenido [ " + name + " ], Si quieres crear una partida aprieta C, si quieres ver las partidas aprieta V ";
	packCreate << confirmText;
	status = newPlayer->sock->send(packCreate);
	if (status == sf::Socket::Done) {
		packCreate.clear();
		if (!newPlayer->game) {
			//std::cout << "antes rcv unirse" << std::endl;
			statusRecv = newPlayer->sock->receive(packCreate);
			if (statusRecv == sf::Socket::Done) {
				//std::cout << "despues rcv unirse" << std::endl;
				packCreate >> modo;
				packCreate.clear();
				if (modo == "C") { // crear una partida
					std::string maxPlayers, maxMoney, name;
					GamesManager *gameManagerAux = new GamesManager();
					std::string specs = " Introduce el nombre de la partida, el numero max de jugadores y dinero maximo que se podra apostar";
					packCreate.clear();
					packCreate << specs;
					newPlayer->sock->send(packCreate);
					packCreate.clear();
					statusRecv = newPlayer->sock->receive(packCreate);
					if (statusRecv == sf::Socket::Done) {
						packCreate >> name;
						packCreate.clear();
						statusRecv = newPlayer->sock->receive(packCreate);
						if (statusRecv == sf::Socket::Done) {
							packCreate >> maxPlayers;
							packCreate.clear();
							statusRecv = newPlayer->sock->receive(packCreate);
							if (statusRecv == sf::Socket::Done) {
								packCreate >> maxMoney;
								packCreate.clear();
								std::string comand = "CMD_WELCOME";
								packSend.clear();
								packSend << comand << IDGame;
								status = newPlayer->sock->send(packSend);
								if (status == sf::Socket::Done) {
									newPlayer->IDGame = IDGame;
									//mySocketSelector.remove(*newPlayer->sock);
									//socketSelectorGame.add(*newPlayer->sock);
									gameManagerAux->CreateGame(newPlayer, name, stoi(maxPlayers), stoi(maxMoney), IDGame);
									newPlayer->game = true;
									IDGame++;
									gameManager.insert(std::pair<std::string, GamesManager*>(name, gameManagerAux));
									std::cout << "creo y aumento" << std::endl;
								}		
							}
						}
					}
				}
				if (modo == "V") {
					ListAvailableGames(newPlayer);
				}
			}
			else if (statusRecv == sf::Socket::Disconnected) {
				std::cout << "Se ha producido una desconexion" << std::endl;
			}
			
		}
	}
	else if (status == sf::Socket::Disconnected) {
		std::cout << "Se ha producido una desconexion" << std::endl;
	}
	
}

void NewConnection() {
	std::string modo;
	sf::Socket::Status statusRcv;
	if (mySocketSelector.wait()) {
		if (mySocketSelector.isReady(listener)) { //gamerady
			sf::TcpSocket* newClient = new sf::TcpSocket;
			Player* newPlayer = new Player(newClient, "");
			if (listener.accept(*newPlayer->sock) == sf::Socket::Done) {
				//Bucle para todos los clientes -> Nuevo cliente conectado.
				//Antes de añadir el nuevo cliente para no tener que comparalos.

				mySocketSelector.add(*newPlayer->sock);
				if (!newPlayer->game) {
					packLogin.clear();
					std::string login = "Si tienes una cuenta aprieta L, si tienes que registrarte aprieta R ";
					packCreate << login;
					status = newPlayer->sock->send(packCreate);
					if (status == sf::Socket::Done) {
						packCreate.clear();
						//std::cout << "antes rcv hello" << std::endl;
						statusRcv = newPlayer->sock->receive(packCreate);
						if (statusRcv == sf::Socket::Done) {
							//std::cout << "despues rcv hello" << std::endl;
							packCreate >> modo;
							packCreate.clear();
							if (modo == "L") {
								std::string nameAux, paswordAux;
								login = "Introduce tu usuario y contraseña en orden ";
								packSend.clear();
								packSend << login;
								status = newPlayer->sock->send(packSend);
								if (status == sf::Socket::Done) {
									packRecv.clear();
									statusRcv = newPlayer->sock->receive(packRecv);
									if (statusRcv == sf::Socket::Done) {
										packRecv >> nameAux;
										packRecv.clear();
										statusRcv = newPlayer->sock->receive(packRecv);
										if (statusRcv == sf::Socket::Done) {
											packRecv >> paswordAux;
											bool log = dbManager->Login(nameAux, paswordAux);
											if (log) {
												newPlayer->nickname = nameAux;
												newPlayer->pasword = paswordAux;
												//puschPALYER??
												login = "CMD_LOGED";
												packSend.clear();
												packSend << login << newPlayer->nickname;
												status = newPlayer->sock->send(packSend);
												if (status == sf::Socket::Done) {
													aPlayers.push_back(newPlayer);
													CrearUnir(newPlayer, newPlayer->nickname);
												}
											}
											if (!log) {
												std::string mesage = "Usuario o pasword incorrecta";
												packSend.clear();
												packSend << mesage;
												status = newPlayer->sock->send(packSend);
											}
											/*for (std::list<Player*>::iterator it = aPlayers.begin(); it != aPlayers.end(); it++) {
												Player& iPlayer = **it;

												if (iPlayer.nickname == nameAux && iPlayer.pasword == paswordAux) {
													std::cout << "conexion de " << iPlayer.nickname << "  " << iPlayer.pasword << std::endl;
													login = "CMD_WB";
													packSend.clear();
													packSend << login << iPlayer.nickname;
													status = newPlayer->sock->send(packSend);
													if (status == sf::Socket::Done) {
														CrearUnir(newPlayer, iPlayer.nickname);
													}
													else {
														std::cout << "No puede crear/unir" << std::endl;
													}
												}
												else {
													login = "El usuario o constraseña no existe, cierra y vuelve a intentarlo";
													packSend.clear();
													packSend << login;
													status = newPlayer->sock->send(packSend);
													if (status == sf::Socket::Done) {
														std::cout << "no existe";
													}
												}
											}*/
										}
										
									}
								}
							}
							if (modo == "R") {
								std::string money;
								std::string login = "Introduce por orden tu usuario, contrareña y dinero ";
								packSend.clear();
								packSend << login;
								status = newPlayer->sock->send(packSend);
								if (status == sf::Socket::Done) {
									packRecv.clear();
									statusRcv = newPlayer->sock->receive(packRecv);
									if (statusRcv == sf::Socket::Done) {
										packRecv >> newPlayer->nickname;
										packRecv.clear();
										statusRcv = newPlayer->sock->receive(packRecv);
										if (statusRcv == sf::Socket::Done) {
											packRecv >> newPlayer->pasword;
											packRecv.clear();
											statusRcv = newPlayer->sock->receive(packRecv);
											if (statusRcv == sf::Socket::Done) {
												packRecv >> money;
												newPlayer->money = stoi(money);
												bool reg = dbManager->Registrar(newPlayer->nickname, newPlayer->pasword, std::to_string(newPlayer->money));
												if (reg) {
													//std::cout << "hola";
													aPlayers.push_back(newPlayer);
													std::cout << "Se ha logeado [" << newPlayer->nickname << "] con pasword [" << newPlayer->pasword << "] y dinero [" << newPlayer->money << "]" << std::endl;
													login = "CMD_LOGED";
													packSend.clear();
													packSend << login << newPlayer->nickname;
													status = newPlayer->sock->send(packSend);
													if (status == sf::Socket::Done) {
														CrearUnir(newPlayer, newPlayer->nickname);
													}
												}
												if (!reg) {
													std::string mesage = "Ese usuario esta ocupado, cierra y escribe otro";
													packSend.clear();
													packSend << mesage;
													status = newPlayer->sock->send(packSend);
												}
											}							
										}									
									}									
								}
							}
						}		
					}
					if (status == sf::Socket::Disconnected) {
						std::cout << "Desconexion al hacer conexion" << std::endl;
					}
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
	/*sf::TcpSocket* newSock = new sf::TcpSocket;
	Player *PlayerAux = new Player(newSock, "");
	PlayerAux->money = 100;
	PlayerAux->nickname = "nick";
	PlayerAux->pasword = 123456;
	aPlayers.push_back(PlayerAux);*/

	SocketSelector();

	system("pause");
	return 0;
}