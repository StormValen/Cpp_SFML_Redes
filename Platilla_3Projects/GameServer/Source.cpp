#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>

#define MAX_PLAYERS 4

std::list<sf::TcpSocket*> myClients; //Lista con todos los clientes conectados.

class Player {
public:
	sf::TcpSocket* sock;
	std::string nickname;
	int money, bet, betMoney;

	Player() {

	}

	Player(sf::TcpSocket* _sock, std::string _nickname) {
		sock = _sock;
		nickname = "";
		money = 100;
		bet = betMoney = 0;
	}
};

std::vector<Player> aPlayers; // Contenedor de jugadores
int clientsConnectedCounter = 0;

void SocketSelector() {
	bool end = false;
	bool gameIsReady = false;
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

				std::size_t received;
				if (listener.accept(*newClient) == sf::Socket::Done) {
					//Bucle para todos los clientes -> Nuevo cliente conectado.
					//Antes de añadir el nuevo cliente para no tener que comparalos.

					/*for (std::list<sf::TcpSocket*>::iterator it = myClients.begin(); it != myClients.end(); it++) {
						std::string connectedMesage = "--- A new client has been connected ---";
						sf::TcpSocket& clientSocket = **it;
						clientSocket.send(connectedMesage.c_str(), connectedMesage.length() + 1);
					}*/

					myClients.push_back(newClient);
					newClient->receive(name, sizeof(name), received);

					std::cout << "Client with port: [" << newClient->getRemotePort() << "] and name [" << name << "] CONNECTED" << std::endl;
					mySocketSelector.add(*newClient);

					Player newPlayer(newClient,name);
					aPlayers.push_back(newPlayer);

					std::string confirmText = "INFO: You have entered the game, please wait until all players are connected ...";
					size_t bytesSent;
					newClient->send(confirmText.c_str(), confirmText.length());

					clientsConnectedCounter++;

					if (clientsConnectedCounter == 4) {
						gameIsReady = true;
						std::cout << "All 4 player are connected ... Starting Game ..." << std::endl;
						for (std::list<sf::TcpSocket*>::iterator it = myClients.begin(); it != myClients.end(); it++) {
							std::string connectedMesage = "All 4 player are connected ... Starting Game ...";
							sf::TcpSocket& clientSocket = **it;
							clientSocket.send(connectedMesage.c_str(), connectedMesage.length() + 1);
						}
					}
				}
				else {
					std::cout << "ERROR: Can't set connection" << std::endl;
					delete newClient;
				}
			}
			else if (gameIsReady) {
				for (std::list<sf::TcpSocket*>::iterator it = myClients.begin(); it != myClients.end(); it++) {
					sf::TcpSocket& clientSocketReceive = **it;
					if (mySocketSelector.isReady(clientSocketReceive)) {
						

							//Server recibe mensajes
							std::string mesage;
							char buffer[2000];
							std::size_t received;
							status = clientSocketReceive.receive(buffer, sizeof(buffer), received);
							if (status == sf::Socket::Done) {
								mesage = buffer;
								std::cout << "Client with port: [" << clientSocketReceive.getRemotePort() << "] SEND: " << mesage << std::endl;
							}

							//Cliente desconectado
							else if (status == sf::Socket::Disconnected) {
								mySocketSelector.remove(clientSocketReceive);
								//eliminar el socket de la lista
								std::cout << "Client with port: [" << clientSocketReceive.getRemotePort() << "] DISCONECTED " << std::endl;

								for (std::list<sf::TcpSocket*>::iterator it = myClients.begin(); it != myClients.end(); it++) {
									std::string connectedMesage = "--- A client has been disconected ---";
									sf::TcpSocket& clientSocket = **it;
									clientSocket.send(connectedMesage.c_str(), connectedMesage.length() + 1);
								}
							}

							for (std::list<sf::TcpSocket*>::iterator it = myClients.begin(); it != myClients.end(); it++) {
								sf::TcpSocket& clientSocket = **it;
								if (clientSocket.getRemotePort() != clientSocketReceive.getRemotePort()) {
									clientSocket.send(mesage.c_str(), mesage.length() + 1);
								}
							}
						}
						else {
							for (std::list<sf::TcpSocket*>::iterator it = myClients.begin(); it != myClients.end(); it++) {
								std::string connectedMesage = "Wait until a new player connects";
								sf::TcpSocket& clientSocket = **it;
								clientSocket.send(connectedMesage.c_str(), connectedMesage.length() + 1);
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