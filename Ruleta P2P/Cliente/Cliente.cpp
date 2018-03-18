#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <string>
#include <SFML\Graphics.hpp>
#define MAX_PLAYERS 1

enum GameState{ Logged, Bed, Winner, EndGame, Chat, nextRound} state;

struct Direction //almazenar la direccion de cada peer
{
	std::string myIP;
	unsigned short port;
};

struct Player {
	std::string name;
	int money, bet, betMoney, moneyWin;
	bool nextRound = false;
};
Player player;
std::vector<Player> Players;
std::vector<Direction> aStr;
sf::Socket::Status status;
std::vector<sf::TcpSocket*> aPeers;
std::vector<std::string> aMensajes;
sf::String mensaje = "";

void msgChat(std::string msg) {
	aMensajes.push_back(msg);
}

void PeerConnection() {
	sf::Packet packet;
	Direction direction;
	sf::TcpSocket sock;

	unsigned short myPort;

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
	//LOGIN
	std::cout << "Bienvenid@ al casino, porfavor introduce tu nombre." << std::endl;
	std::cin >> player.name;
	std::cout << "Introduce tu dinero" << std::endl;
	std::cin >> player.money;
	Players.push_back(player);
	sf::Packet packLog;
	packLog << player.name << player.money;
	for (int i = 1; i <= aPeers.size(); i++) {
		status = aPeers[i-1]->send(packLog);
		if (status == sf::Socket::Done) {
		}
		else if (status == sf::Socket::Disconnected) {
			std::cout << "Un peer se ha desconecatado" << std::endl;
		}
	}
	packLog.clear();
	for (int i = 1; i <= aPeers.size(); i++) {
		status = aPeers[i-1]->receive(packLog);
		if (status == sf::Socket::Done) {
			Player pAux;
			packLog >> pAux.name >> pAux.money;
			Players.push_back(pAux);
		}
		else if(status == sf::Socket::Disconnected){
			std::cout << "Un peer se ha desconecatado" << std::endl;

		}
	}
	for (int i = 1; i < Players.size(); i++) {
		msgChat("> " + Players[i].name + " se ha conectado y tiene " + std::to_string(Players[i].money) + " monedas");
	}
	packLog.clear();
	state = nextRound;
	listener.close();
}

void MSG() {
	if (state == Logged) {
		msgChat("> Dealer: Introduce el dinero de la apusta");
		player.nextRound = false;
	}
	if (state == Bed) {
		msgChat("> Dealer: Introduce un numero del 0 al 34 para decidir a que apostar");
	}
	if (state == Chat) {
		msgChat("> Dealer: Mientras sale el numero ganador y se hacen todos los calculas entrais en el modo chat");
	}
	if (state == Winner) {
		//calcular random
		srand(time(NULL));
		int number = rand() % 35;
		for (int i = 0; i < Players.size(); i++) {
			if (Players[i].bet == number) {
				Players[i].moneyWin = Players[i].betMoney * 36;
			}
			if (Players[i].bet != number) {
				Players[i].moneyWin = Players[i].betMoney * 0;
			}
			Players[i].money += Players[i].moneyWin;
		}
		msgChat("> Dealer: Atencion tenemos resultado, el numero ganador ha sido " + std::to_string(number));
		for (int i = 0; i < Players.size(); i++) {
			msgChat("> Dealer: " + Players[i].name + " ha ganado en esta ronda " + std::to_string(Players[i].moneyWin));
			Players[i].bet = 0;
			Players[i].betMoney = 0;
			Players[i].moneyWin = 0;
		}
		msgChat("> Dealer: Se ha acabado esta ronda, tienes " + std::to_string(player.money));	
	}
	if (state == nextRound) {
		msgChat("> Dealer: Para pasar a la siguiente ronda escribir todos ready");
			for (int i = 0; i < Players.size(); i++) {
				if (Players[i].nextRound == true) {
					msgChat("> Dealer: Comienza la siguiente ronda");
				}
				std::cout << Players[i].nextRound << std::endl;
			}
			//state = Logged;
	}
}

void thread_Chat() {
	// ----- WINDOW UI ----- //
	sf::Vector2i screenDimensions(800, 600);
	sf::RenderWindow window;
	window.create(sf::VideoMode(screenDimensions.x, screenDimensions.y), "Chat");

	sf::Font font;
	if (!font.loadFromFile("calibri.ttf"))
	{
		std::cout << "Can't load the font file" << std::endl;
	}

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
		sf::Packet pack;
		sf::String string;
		//RECIVE
		if (state != EndGame) {
			for (int i = 1; i <= aPeers.size(); i++) {
				aPeers[i - 1]->setBlocking(false);
				status = aPeers[i - 1]->receive(pack);
				if (status == sf::Socket::Done) {
					if (state == Logged) {
						pack >> player.betMoney;
						string ="> Ha apostado " + std::to_string(player.betMoney);
						msgChat("> [ " + Players[i].name + " ]" + string);
					}
					else if (state == Bed) {
						pack >> player.bet;
						string = " Ha apostado al " + std::to_string(player.bet);
						msgChat("> [ " + Players[i].name + " ]" + string);
					}
					else if (state == Chat || state == Winner) {
						pack >> string;
						msgChat("> [ " + Players[i].name + " ]" + string);
					}
					else if (state == Chat || state == nextRound) {
						pack >> string;
						if (string == "ready") {
							Players[i].nextRound = true;
							msgChat("> [ " + Players[i].name + " ]" + string);

						}else{
							msgChat("> [ " + Players[i].name + " ]" + string);
						}
					}
					pack.clear();
				}
				else if (status == sf::Socket::Disconnected) {
					msgChat("Un peer se ha desconecatado");

				}
			}		
		}
		sf::Event evento;
		while (window.pollEvent(evento))
		{
			switch (evento.type)
			{
			case sf::Event::Closed:
				state = EndGame;
				window.close();
				break;
			case sf::Event::KeyPressed:
				if (evento.key.code == sf::Keyboard::Escape) {
					state = EndGame;
					window.close();
				}
				else if (evento.key.code == sf::Keyboard::Return)
				{
					std::string aux = "";
					if (state == Logged) {
						aux = mensaje;
						player.betMoney = stoi(aux);
						pack << player.betMoney;
					}
					if (state == Bed) {
						aux = mensaje;
						player.bet = stoi(aux);
						pack << player.bet;
					}
					if (state == Chat || state == Winner) {
						pack << mensaje;
					}
					if (state == Chat || state == nextRound) {
						pack << mensaje;
					}
					for (int i = 1; i <= aPeers.size(); i++) {
						aPeers[i - 1]->setBlocking(false);
						status = aPeers[i - 1]->send(pack);
					}
					if (status == sf::Socket::Done) {
						if (mensaje == "exit") {
							msgChat("Sesion finalizada");
							state = EndGame;
							window.close();
						}
						if (mensaje == "money") {
							msgChat("> Tienes " + player.money);
						}
						if (mensaje == "ready") {
							player.nextRound = true;
						}
						else {
							msgChat("> [Yo] " + mensaje);
						}
					}
					else if (status == sf::Socket::Disconnected) {
						msgChat("> Un peer se ha desconecatado");
					}
					
					pack.clear();
					if (aMensajes.size() > 25)
					{
						aMensajes.erase(aMensajes.begin(), aMensajes.begin() + 1);
					}
					mensaje = ">";
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
}

int main()
{
	PeerConnection();
	std::thread tr(&thread_Chat);
	std::thread game(&MSG);
	//MSG();

	//state = EndGame;	
	tr.join();
	game.join();
}