#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>
#include <SFML\Graphics.hpp>
#define MAX_PLAYERS 4
std::vector<sf::TcpSocket*> aPeers;


struct Direction //almazenar la direccion de cada peer
{
	sf::IpAddress myIP;
	unsigned short port;
};
std::vector<Direction> aStr;
sf::Packet packet;
Direction direction;

std::mutex mu;


int main()
{
	sf::TcpSocket sock;
	sf::Socket::Status status;
	status = sock.connect("localhost", 50000); //te conectas con el bootstrap
	if (status != sf::Socket::Done) {
		std::cout << "Error al conectarte al bootstrap" << std::endl;
	}
	std::cout << "Connected with the bootstrap with port: " << sock.getLocalPort() << std::endl;
	
	status = sock.receive(packet); //recives las ip y puertos
	//el primer peer se queda bloqueado aqui por lo q no puede establecer conexiones con los otros
	//sock.disconnect();
	if (status != sf::Socket::Done) {
		if (status == sf::Socket::Disconnected) {
			std::cout << "Se ha desconectado un peer" << std::endl;
		}
		else {
			std::cout << "Error no se ha recibido nada" << std::endl;
		}
	}
	else {
		size_t prob;
		packet >> prob;
		for (int i = 0; i < prob; i++) {
			packet >> direction.port;
			aStr.push_back(direction); //añades las ip y puertos al vector
			//std::cout << prob << " " << aStr.size() << std::endl;
		}
	}
	for (int i = 0; i < aStr.size(); i++) {
		sf::TcpSocket *sockAux = new sf::TcpSocket; // creas un nuevo socket para conectarte con cada peer
		status = sockAux->connect("localhost", aStr[i].port);
		if (status != sf::Socket::Done) {
			std::cout << "Error al conectarte con el peer de ip: " << aStr[i].myIP << "y puerto: " << aStr[i].port << "mi puerto es: " << sock.getLocalPort()<< std::endl;
		}
		std::cout << aStr[i].port << std::endl;
		aPeers.push_back(sockAux);
	}
	while(aPeers.size() < MAX_PLAYERS) {
		sf::TcpListener listener;
		status = listener.listen(sock.getLocalPort()); //si aun no hay 4 jugadores escuchas por tu puerto para tener mas conexiones
		std::cout << sock.getLocalPort();
		if (status != sf::Socket::Done) {
			std::cout << "Error" << std::endl;
		}
		for (int i = aPeers.size(); i > MAX_PLAYERS; i--) {
		sf::TcpSocket *sockNew = new sf::TcpSocket;
		listener.accept(*sockNew);
		aPeers.push_back(sockNew);
		}
		listener.close();
	}
	std::cout << "La sala esta llena" << std::endl;

	//---------------------------establecer conexion-----------------------------------
	//bool end = false;
	//std::thread tr(&thread_recived, &aMensajes);
	/*while (!end) {
		for (int i = 0; i < aPeers.size(); i++) {
			status = aPeers[i]->receive(packet);
			if (status != sf::Socket::Done) {
				if (status == sf::Socket::Error) {
					std::cout << "se ha producido un error al enviar entre peers" << std::endl;
				}
				else if (status == sf::Socket::Disconnected) {
					std::cout << "Se ha desconectado el peer con puerto : " << aPeers[i]->getRemotePort() << std::endl;
				}
				else if (status == sf::Socket::Partial) {

				}
			}
			else {
				
			}
		}
	}*/
}