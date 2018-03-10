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
int main()
{
	sf::TcpSocket sock;
	sf::Socket::Status status;
	status = sock.connect("192.168.1.34", 50000); //te conectas con el bootstrap
	if (status != sf::Socket::Done) {
		std::cout << "Error" << std::endl;
	}
	sf::Packet packet;
	Direction direction;
	status = sock.receive(packet); //recives las ip y puertos
	sock.disconnect();
	if (status != sf::Socket::Done) {
		std::cout << "Error" << std::endl;
	}
	int prob = 0;
	packet >> prob;
	for (int i = 0; i <prob ; i++) {
		packet >> direction.IP >> direction.port;
		aStr.push_back(direction); //añades las ip y puertos al vector
	}
	for (int i = 0; i < aStr.size(); i++) {
		sf::TcpSocket *sock = new sf::TcpSocket; // creas un nuevo socket para conectarte con cada peer
		status = sock->connect(aStr[i].IP, aStr[i].port);
		if (status != sf::Socket::Done) {
			std::cout << "Error" << std::endl;
		}
		aPeers.push_back(sock);
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
	return 0;
}