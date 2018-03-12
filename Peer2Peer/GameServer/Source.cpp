#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <string>
#include <SFML\Graphics.hpp>

#define MAX_PLAYERS 4

struct Direction //almazenar la direccion de cada peer
{
	std::string myIP;
	unsigned short port;
};
std::vector<Direction> aPeers; //almazenar los peers
sf::TcpSocket sock;

int main()
{
	sf::TcpListener listener;
	sf::Socket::Status status =  listener.listen(50000);
	if (status != sf::Socket::Done) {
		std::cout << "Conection error" << std::endl;
	}
	for (int i = 0; i < MAX_PLAYERS; i++) {
		//sf::TcpSocket *sock = new sf::TcpSocket;
		status = listener.accept(sock);
		if (status != sf::Socket::Done) {//creas la conexion con quien quiera conectarse, los peers
			std::cout << "Error al crear la conexion" << std::endl;
		}
		else {
			Direction direction;
			//No hay nadie conectado, tienes que añadir la ip y puerto del primer peer pero no le pasas nada
			sf::Packet packet;
			//tienes que enviar al peer las ip y puertos de los otros peers ya conectados
			packet << (int)aPeers.size();
			for (int j = 0; j < aPeers.size(); j++) {
				packet << aPeers[j].port ; //añadir la IP q peta
				//packet << aPeers[j].IP;
			}
			sock.send(packet);// envias el paquete con la ip y el puerto
			direction.myIP = "localhost";
			direction.port = sock.getRemotePort();
			aPeers.push_back(direction); //añader el nuevo peer que te ha pedido conexion
			packet.clear();
		}
		//sock->disconnect();
	}
	listener.close();
	return 0;
}
