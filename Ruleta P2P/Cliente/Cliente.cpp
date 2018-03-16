#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <string>
#include <SFML\Graphics.hpp>
#define MAX_PLAYERS 1


struct Direction //almazenar la direccion de cada peer
{
	std::string myIP;
	unsigned short port;
};

struct Player {
	std::string name;
	int money, bet, betMoney;
};
std::vector<Player> Players;
std::vector<Direction> aStr;
std::vector<sf::TcpSocket*> aPeers;
sf::Packet packet;
Direction direction;
sf::TcpSocket sock;
std::mutex mu;
unsigned short myPort;
sf::Socket::Status status;
void PeerConnection() {
	status = sock.connect("localhost", 50000); //te conectas con el bootstrap
	if (status != sf::Socket::Done) {
		std::cout << "Error al conectarte al bootstrap" << std::endl;
	}
	//std::cout << "Connected with the bootstrap with port: " << sock.getLocalPort() << std::endl;
	status = sock.receive(packet); //recives las ip y puertos
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
		status = sockAux->connect("localhost", aStr[i - 1].port);
		if (status != sf::Socket::Done) {
			std::cout << "Error al conectarte con el peer de ip: " << aStr[i - 1].myIP << "y puerto: " << aStr[i - 1].port << "mi puerto es: " << myPort << std::endl;
		}
		else {
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
		}
	}
	std::cout << "Inicio de la partida" << std::endl;
	listener.close();
}


int main()
{
	PeerConnection();

	// ----- WINDOW UI ----- //
	std::vector<std::string> aMensajes;
	sf::Vector2i screenDimensions(800, 600);
	sf::RenderWindow window;
	window.create(sf::VideoMode(screenDimensions.x, screenDimensions.y), "Chat");

	sf::Font font;
	if (!font.loadFromFile("calibri.ttf"))
	{
		std::cout << "Can't load the font file" << std::endl;
	}

	sf::String mensaje = ">";

	sf::Text chattingText(mensaje, font, 14);
	chattingText.setFillColor(sf::Color(0, 160, 0));
	chattingText.setStyle(sf::Text::Bold);


	sf::Text text(mensaje, font, 14);
	text.setFillColor(sf::Color(0, 160, 0));
	text.setStyle(sf::Text::Bold);
	text.setPosition(0, 560);

	sf::RectangleShape separator(sf::Vector2f(800, 5));
	separator.setFillColor(sf::Color(200, 200, 200, 255));
	separator.setPosition(0, 550);

	while (window.isOpen())
	{
		sf::Event evento;
		while (window.pollEvent(evento))
		{
			switch (evento.type)
			{
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyPressed:
				if (evento.key.code == sf::Keyboard::Escape)
					window.close();
				else if (evento.key.code == sf::Keyboard::Return)
				{

					/*Stext = "[ " + clientName + " ]   " + mensaje;
					status = socket.send(Stext.c_str(), Stext.length() + 1);
					if (status == sf::Socket::Done) {
					if (Stext == ">exit") {
					aMensajes.push_back("La sesion ha finalizado");
					window.close();
					}
					else {
					shared_msg(&aMensajes, Stext);
					}
					}
					else if (status == sf::Socket::Disconnected) {
					aMensajes.push_back("Se ha producido una desconexion");
					window.close();
					}
					mensaje = ">";*/
				}
				break;
			case sf::Event::TextEntered:
				if (evento.text.unicode >= 32 && evento.text.unicode <= 126)
					mensaje += (char)evento.text.unicode;
				else if (evento.text.unicode == 8 && mensaje.getSize() > 0)
					mensaje.erase(mensaje.getSize() - 1, mensaje.getSize());
				break;
			}
		}
		window.draw(separator);
		for (size_t i = 0; i < aMensajes.size(); i++)
		{
			std::string chatting = aMensajes[i];
			chattingText.setPosition(sf::Vector2f(0, 20 * i));
			chattingText.setString(chatting);
			window.draw(chattingText);
		}
		std::string mensaje_ = mensaje + "_";
		text.setString(mensaje_);
		window.draw(text);
		window.display();
		window.clear();
	}

	bool login = false;
	Player player;
	std::string s_mensaje;
	size_t bSent;
	std::size_t received;
	char recv[200];
	for (int i = 0; i < aPeers.size(); i++) {

		sf::Packet packSend;
		aPeers[i]->setBlocking(false);
		std::cout << "Bienvenido al casino, cual es tu nombre? " << std::endl;
		std::cin >> player.name;
		std::cout << "Cuanto dinero vas a ingresar para poder jugar?" << std::endl;
		std::cin >> player.money;
		//s_mensaje = player.name;
		packSend << player.name.c_str() << player.money;
		status = aPeers[i]->send(packSend);
		//status = aPeers[i]->send(s_mensaje.c_str(), s_mensaje.length() + 1, bSent);
		if (status != sf::Socket::Done) {
			/*if (status == sf::Socket::Partial) {
			while (bSent < s_mensaje.length()) {
			std::string msgRest = "";
			for (size_t i = bSent; i < s_mensaje.length(); i++) {
			msgRest = s_mensaje[i];
			}
			aPeers[i]->send(s_mensaje.c_str(), s_mensaje.length(), bSent);
			}
			}*/
		}
		else {
			Players.push_back(player);
		}
	}
	while (!login) {
		for (int i = 0; i < aPeers.size(); i++) {
			sf::Packet packRecv;
			//std::cout << aPeers.size();
			aPeers[i]->setBlocking(false); //nonblocking para que en el recive no se bloquee al prinicpio
										   //status = aPeers[i]->receive(recv, sizeof(recv), received);
			status = aPeers[i]->receive(packRecv);
			if (status != sf::Socket::Done) {
				if (status == sf::Socket::Error) {
					std::cout << "Se ha producido un error al enviar entre peers" << std::endl;
				}
				else if (status == sf::Socket::Disconnected) {
					std::cout << "Se ha desconectado el peer con puerto : " << aPeers[i]->getRemotePort() << std::endl;
				}
			}
			else {
				packRecv >> player.name;
				Players.push_back(player);
				//std::cout << Players.size();
				for (int j = 1; j <= Players.size(); j++) {
					std::cout << "El jugador " << Players[j - 1].name << " se ha conectado y tiene " << Players[j - 1].money << "monedas" << std::endl;
				}
				//login = true;
			}
		}

	}
}