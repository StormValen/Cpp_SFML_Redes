#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <string>
#include <SFML\Graphics.hpp>
#define MAX_PLAYERS 4
std::vector<sf::TcpSocket*> aPeers;


struct Direction //almazenar la direccion de cada peer
{
	std::string myIP;
	unsigned short port;
};
std::vector<Direction> aStr;
sf::Packet packet;
Direction direction;
sf::TcpSocket sock;
std::mutex mu;
unsigned short myPort;

/*void shared_msg(std::vector<std::string> *aMensajes, sf::String string) {
	mu.lock();
	aMensajes->push_back(string);
	if (aMensajes->size() > 25)
	{
		aMensajes->erase(aMensajes->begin(), aMensajes->begin() + 1);
	}
	mu.unlock();
}

// ----- BLOCKING THREAD ----- //
void thread_recived() {
	bool tBucle = true;
	while (tBucle) {
		sf::Socket::Status status = sock.receive(packet); //recives las ip y puertos
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
	}
}*/
int main()
{
	sf::Socket::Status status;
	status = sock.connect("localhost", 50000); //te conectas con el bootstrap
	if (status != sf::Socket::Done) {
		std::cout << "Error al conectarte al bootstrap" << std::endl;
	}
	std::cout << "Connected with the bootstrap with port: " << sock.getLocalPort() << std::endl;

	status = sock.receive(packet); //recives las ip y puertos
	//el primer peer se queda bloqueado aqui por lo q no puede establecer conexiones con los otros
	myPort = sock.getLocalPort();

	if (status != sf::Socket::Done) {
		if (status == sf::Socket::Disconnected) {
			std::cout << "Se ha desconectado un peer" << std::endl;
		}
		else {
			std::cout << "Error no se ha recibido nada" << std::endl;
		}
	}
	else {
		int prob;
		packet >> prob;
		for (int i = 1; i <= prob; i++) {
			packet >> direction.port; //IP
			aStr.push_back(direction); //añades las ip y puertos al vector
		}
	}
	for (int i = 1; i <= aStr.size(); i++) {
		sf::TcpSocket *sockAux = new sf::TcpSocket; // creas un nuevo socket para conectarte con cada peer
		status = sockAux->connect("localhost", aStr[i-1].port);
		if (status != sf::Socket::Done) {
			std::cout << "Error al conectarte con el peer de ip: " << aStr[i-1].myIP << "y puerto: " << aStr[i-1].port << "mi puerto es: " << myPort << std::endl;
		}
		else {
			std::cout << "Conectado con el peer puerto: " << aStr[i - 1].port <<" "  << std::endl;
			aPeers.push_back(sockAux);
		}
	}
	sock.disconnect();
		sf::TcpListener listener;
		status = listener.listen(myPort); //si aun no hay 4 jugadores escuchas por tu puerto para tener mas conexiones
		if (status != sf::Socket::Done) {
			std::cout << "no se puede vincular al puerto " << myPort << std::endl;
		}
		while (aPeers.size() < MAX_PLAYERS) { //recorres un bucle desde tu posicion del vector y escuchas hasta que este lleno(si eres el 2 peer, tendras que escuchar 2 vezes)
			sf::TcpSocket *sockNew = new sf::TcpSocket;
			status = listener.accept(*sockNew);
			if (status != sf::Socket::Done) {
			std::cout << "Error al aceptar la conexion " << std::endl;
				}
			else {
				aPeers.push_back(sockNew);
				std::cout << aPeers.size() << std::endl;
			}
		}

		//sock.disconnect();
		listener.close();

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