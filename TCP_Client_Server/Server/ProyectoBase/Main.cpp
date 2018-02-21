#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>

std::list<sf::TcpSocket*> myClients; //Lista con todos los clientes conectados.
void SocketSelector() {
	bool end = false;
	sf::TcpListener listener;
	sf::Socket::Status status = listener.listen(50000);
	if (status != sf::Socket::Done) {
		std::cout << "ERROR: Can't open listener";
	}

	sf::SocketSelector mySocketSelector;
	mySocketSelector.add(listener);
	
	//Socket selector tiene el listener + clients.
	//Primero comprueba si el listener recibe alguna connexion nueva.
	//Si no se hace el manage de los clientes.
	std::string string= "Conectado al server";
	while (!end) {
		if (mySocketSelector.wait()){
			if (mySocketSelector.isReady(listener)) {
				sf::TcpSocket* newClient = new sf::TcpSocket;
				if (listener.accept(*newClient) == sf::Socket::Done) {
					myClients.push_back(newClient);	
					std::cout << "INFO: Client added -> " << newClient->getRemoteAddress() << std::endl;
					mySocketSelector.add(*newClient);
				} else {
					std::cout << "ERROR: Can't set connection" << std::endl;
					delete newClient;
				}
			} else {
				for (std::list<sf::TcpSocket*>::iterator it = myClients.begin(); it != myClients.end(); it++){
					//To Do Mange clients
					//Check receives
					//Send received mesages
					
				}
			}
		}
	}
}

int main()
{
	SocketSelector();
	system("pause");	
	return 0;
}