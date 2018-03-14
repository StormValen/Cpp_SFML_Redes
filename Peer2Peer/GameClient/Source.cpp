#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <string>
#include <SFML\Graphics.hpp>
#define MAX_PLAYERS 1
std::vector<sf::TcpSocket*> aPeers;

struct Direction //almazenar la direccion de cada peer
{
	std::string myIP;
	unsigned short port;
};

struct Player {
	std::string name, money, bet, betMoney;
};
std::vector<Player> Players;
std::vector<Direction> aStr;
sf::Packet packet;
Direction direction;
sf::TcpSocket sock;
std::mutex mu;
unsigned short myPort;

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
		std::cout << "Ya estan todos los jugadores listos" << std::endl;
		listener.close();

	//---------------------------establecer conexion-----------------------------------
	bool end = false;
	//std::thread tr(&thread_recived, &aMensajes);
	while (!end) {
		Player player;
		std::string s_mensaje;
		size_t bSent;
		std::cout << "Bienvenido al casino, cual es tu nombre? " << std::endl;
		std::cin >> player.name;
		std::cout << "Cuanto dinero vas a ingresar para poder jugar?" << std::endl;
		std::cin >> player.money;
		for (int i = 0; i < aPeers.size(); i++) {
			s_mensaje = player.name + player.money;
			status = aPeers[i]->send(s_mensaje.c_str(), s_mensaje.length() + 1, bSent);
			if (status != sf::Socket::Done) {
				if (status == sf::Socket::Partial) {
					while (bSent < s_mensaje.length()) {
						std::string msgRest = "";
						for (size_t i = bSent; i < s_mensaje.length(); i++) {
							msgRest = s_mensaje[i];
						}
						aPeers[i - 1]->send(s_mensaje.c_str(), s_mensaje.length(), bSent);
					}
				}
			}
			else {
				Players.push_back(player);
			}
		}
		std::size_t received;
		char recv[200];
		for (int i = 0; i < aPeers.size(); i++) {
			aPeers[i]->setBlocking(false); //nonblocking para que en el recive no se bloquee al prinicpio
			status = aPeers[i]->receive(recv, sizeof(recv), received);
			if (status != sf::Socket::Done) {
				if (status == sf::Socket::Error) {
					std::cout << "se ha producido un error al enviar entre peers" << std::endl;
				}
				else if (status == sf::Socket::Disconnected) {
					std::cout << "Se ha desconectado el peer con puerto : " << aPeers[i]->getRemotePort() << std::endl;
				}
			}
			else {
				std::cout << recv << std::endl;
			}
		}

	}
}