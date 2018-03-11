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
// ----- MUTEX ----- //
/*void shared_msg(std::vector<std::string> *aMensajes, sf::String string) {
	mu.lock();
	aMensajes->push_back(string);
	if (aMensajes->size() > 25)
	{
		aMensajes->erase(aMensajes->begin(), aMensajes->begin() + 1);
	}
	mu.unlock();
}*/

// ----- BLOCKING THREAD ----- //
/*void thread_recived(std::vector<std::string> *aMensajes) {
	bool tBucle = true;
	while (tBucle) {
		std::size_t received;
		char buffer_Thread[2000];
		sf::Socket::Status status = sock.receive(buffer_Thread, sizeof(buffer_Thread), received);
		int timer = 0;
		//socket.receive(&timer, sizeof(timer), received);
		//std::cout << timer << std::endl;
		sf::String string = buffer_Thread;
		if (status == sf::Socket::Done) {
			if (string == ">exit") {
				aMensajes->push_back("La sesion ha finalizado");
				tBucle = false;
			}
			else {
				shared_msg(aMensajes, string);
			}
		}
		else if (status == sf::Socket::Disconnected) {
			aMensajes->push_back("Se ha producido una desconexion");
			tBucle = false;
		}
	}
}*/

int main()
{
	sf::TcpSocket *sock = new sf::TcpSocket;
	sf::Socket::Status status;
	status = sock->connect("localhost", 50000); //te conectas con el bootstrap
	if (status != sf::Socket::Done) {
		std::cout << "Error al conectarte al bootstrap" << std::endl;
	}
	std::cout << "Connected with the bootstrap" << std::endl;
	status = sock->receive(packet); //recives las ip y puertos
	sock->disconnect();
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
			packet >> direction.port >> direction.myIP.toString();
			std::cout << direction.port << " " << direction.myIP<< std::endl;
			aStr.push_back(direction); //añades las ip y puertos al vector
		}
	}
	for (int i = 0; i < aStr.size(); i++) {
		sf::TcpSocket *sockAux = new sf::TcpSocket; // creas un nuevo socket para conectarte con cada peer
		status = sockAux->connect(aStr[i].myIP, aStr[i].port);
		if (status != sf::Socket::Done) {
			std::cout << "Error al conectarte con el peer de ip: " << aStr[i].myIP << "y puerto: " << aStr[i].port << std::endl;
		}
		aPeers.push_back(sockAux);
	}
	if (aPeers.size() < MAX_PLAYERS) {
		sf::TcpListener listener;
		status = listener.listen(sock->getLocalPort()); //si aun no hay 4 jugadores escuchas por tu puerto para tener mas conexiones
		std::cout << sock->getLocalPort();
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