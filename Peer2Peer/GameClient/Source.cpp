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
	std::string IP;
	unsigned short port;
};
std::vector<Direction> aStr;
sf::Packet packet;
Direction direction;
int main()
{
	sf::TcpSocket sock;
	sf::Socket::Status status;
	status = sock.connect("localhost", 50000); //te conectas con el bootstrap
	if (status != sf::Socket::Done) {
		std::cout << "Error al conectarte al bootstrap" << std::endl;
	}
	std::cout << "Connected with the bootstrap" << std::endl;
	std::cout << sock.getLocalPort();
	status = sock.receive(packet); //recives las ip y puertos
	//sock.disconnect();
	if (status != sf::Socket::Done) {
		std::cout << "Error no se ha recibido nada" << std::endl;
	}
	else {
		size_t prob;
		packet >> prob;
		for (int i = 0; i < prob; i++) {
			packet >> direction.port;
			std::cout << direction.port << direction.IP;
			aStr.push_back(direction); //añades las ip y puertos al vector
		}
		for (int i = 0; i < aStr.size(); i++) {
			sf::TcpSocket *sock = new sf::TcpSocket; // creas un nuevo socket para conectarte con cada peer
			status = sock->connect("localhost", aStr[i].port);
			if (status != sf::Socket::Done) {
				std::cout << "Error al conectarte con el peer de ip: " << aStr[i].IP << "y puerto: " << aStr[i].port << std::endl;
			}
			aPeers.push_back(sock);
			std::cout << aPeers.size();
		}
		if (aPeers.size() < MAX_PLAYERS) {
			sf::TcpListener listener;
			status = listener.listen(sock.getLocalPort()); //si aun no hay 4 jugadores escuchas por tu puerto para tener mas conexiones
			if (status != sf::Socket::Done) {
				std::cout << "Error" << std::endl;
			}
			for (int i = aPeers.size(); i > MAX_PLAYERS; i--) {
				sf::TcpSocket *sockNew = new sf::TcpSocket;
				listener.accept(*sockNew); //????????????
				aPeers.push_back(sockNew);
			}
			listener.close();
		}
	}
}