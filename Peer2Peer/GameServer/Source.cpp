#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>
#include <SFML\Graphics.hpp>

#define MAX_PLAYERS 4

struct Direction //almazenar la direccion de cada peer
{
	sf::String IP;
	unsigned short port;
};
std::vector<Direction> aPeers; //almazenar los peers

int main()
{
	sf::TcpListener listener;
	sf::Socket::Status status =  listener.listen(50000);
	if (status != sf::Socket::Done) {
		std::cout << "Conection error" << std::endl;
	}
	for (int i = 0; i < MAX_PLAYERS; i++) {
		sf::TcpSocket sock;
		status = listener.accept(sock);
		if (status != sf::Socket::Done) {//creas la conexion con quien quiera conectarse, los peers
			std::cout << "Error" << std::endl;
		}
		else {
			Direction direction;
			if (aPeers.empty()) {
				//No hay nadie conectado, tienes que añadir la ip y puerto del primer peer pero no le pasas nada
				direction.IP = sock.getRemoteAddress().toString();
				direction.port = sock.getRemotePort();
				aPeers.push_back(direction);
			}
			else {
				sf::Packet packet;
				//tienes que enviar al peer las ip y puertos de los otros peers ya conectados
				packet << aPeers.size();
				for (int i = 0; i < aPeers.size(); i++) {
					packet << aPeers[i].IP << aPeers[i].port;
				}
				sock.send(packet);// envias el paquete con la ip y el puerto
				direction.IP = sock.getRemoteAddress().toString();
				direction.port = sock.getRemotePort();
				aPeers.push_back(direction); //añader el nuevo peer que te ha pedido conexion
			}
		}
		sock.disconnect();
	}
	listener.close();
	return 0;
}
