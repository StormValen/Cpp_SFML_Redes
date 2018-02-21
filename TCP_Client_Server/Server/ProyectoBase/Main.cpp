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

					//Bucle para todos los clientes -> Nuevo cliente conectado.
					//Antes de añadir el nuevo cliente para no tener que comparalos.
					for (std::list<sf::TcpSocket*>::iterator it = myClients.begin(); it != myClients.end(); it++) {
						std::string connectedMesage = "--- A new client has been connected ---";
						sf::TcpSocket& clientSocket = **it;
						clientSocket.send(connectedMesage.c_str(), connectedMesage.length() + 1);
					}

					myClients.push_back(newClient);	
					std::cout << "Client with port: [" << newClient->getRemotePort() << "] CONNECTED" <<std::endl;
					mySocketSelector.add(*newClient);

					std::string confirmText = "Conected to server";
					size_t bytesSent;
					newClient->send(confirmText.c_str(), confirmText.length());
					
				} else {
					std::cout << "ERROR: Can't set connection" << std::endl;
					delete newClient;
				}
			} else {
				for (std::list<sf::TcpSocket*>::iterator it = myClients.begin(); it != myClients.end(); it++){
					sf::TcpSocket& clientSocketReceive = **it;
					if (mySocketSelector.isReady(clientSocketReceive)) {
						std::string mesage;
						char buffer[2000];
						std::size_t received;
						status = clientSocketReceive.receive(buffer, sizeof(buffer), received);
						if (status == sf::Socket::Done) {
							mesage = buffer;
							std::cout << "Client with port: [" << clientSocketReceive.getRemotePort() << "] SEND: " << mesage << std::endl;
						}
						else if(status == sf::Socket::Disconnected) {
							mySocketSelector.remove(clientSocketReceive);
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
								clientSocket.send(mesage.c_str(), mesage.length()+1);
							}
						}
					}
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