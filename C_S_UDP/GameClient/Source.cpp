#include <stdlib.h>
//#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
//#include <wait.h>
#include <thread>
#include <stdio.h>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML\Network.hpp>

#define MAX 100
#define SIZE_TABLERO 64
#define SIZE_FILA_TABLERO 25
#define LADO_CASILLA 20
#define RADIO_AVATAR 10.f
#define OFFSET_AVATAR 1

#define SIZE_TABLERO 64
#define LADO_CASILLA 20
#define RADIO_AVATAR 10.f
#define OFFSET_AVATAR 1

enum TipoProceso { RATON, GATO, PADRE };
char tablero[SIZE_TABLERO];
struct Player
{
	int ID;
	float posX, posY;
	std::string name;
};

/**
* Si vale true --> nos permite marcar casilla con el mouse
* Si vale false --> No podemos interactuar con el tablero y aparece un letrero de "esperando"
*/
bool tienesTurno = true;

/**
* Ahora mismo no tiene efecto, pero luego lo necesitarás para validar los movimientos
* en función de si eres el gato o el ratón.
*/
TipoProceso quienSoy = TipoProceso::RATON;


/**
* Cuando el jugador clica en la pantalla, se nos da una coordenada del 0 al 500.
* Esta función la transforma a una posición entre el 0 y el 24
*/
sf::Vector2f TransformaCoordenadaACasilla(int _x, int _y)
{
	float xCasilla = _x / LADO_CASILLA;
	float yCasilla = _y / LADO_CASILLA;
	sf::Vector2f casilla(xCasilla, yCasilla);
	return casilla;
}

/**
* Si guardamos las posiciones de las piezas con valores del 0 al 7,
* esta función las transforma a posición de ventana (pixel), que va del 0 al 512
*/
sf::Vector2f BoardToWindows(sf::Vector2f _position)
{
	return sf::Vector2f(_position.x*LADO_CASILLA, _position.y*LADO_CASILLA);
}
sf::UdpSocket socket;
Player player;
std::vector<Player>Players;
int ID;

void Listen() {
	sf::Packet pack;
	sf::IpAddress IP;
	unsigned short port;
	if (socket.receive(pack, IP, port) != sf::Socket::Done) {
		std::cout << "Errorasgagas";
	}
	std::string welcome = "";
	int size = 0;
	pack >> player.ID >> player.posX >> player.posY;
	Players.push_back(player);
	//std::cout << Players[i].name << welcome << Players[i].ID << " y posicion " << Players[i].posX << " " << Players[i].posY << std::endl;

}
void Connection(){
	bool send = false;
	sf::Packet packetLog;
	std::string name;
	std::cout << "Introduce tu nombre" << std::endl;
	std::cin >> name;
	packetLog << name;
	sf::Clock c;
	c.restart();
	while (!send) {
		if (c.getElapsedTime().asMilliseconds() >= 500) {
			if (socket.send(packetLog, "localhost", 50000) != sf::Socket::Done) {
				std::cout << "Error al enviar" << std::endl;
			}
			c.restart();
			send = true;
		}
	}
	packetLog.clear();
	sf::IpAddress IP;
	unsigned short port;
	sf::Packet packR;
	if (socket.receive(packR,IP , port) != sf::Socket::Done) {
		std::cout << "Error al recivir";
	}
	std::string welcome = "";
	int size =0;
	packR >> size;
	for (int i = 0; i < size; i++) {
		packR >> player.name >> welcome >> player.ID >> player.posX >> player.posY;
		Players.push_back(player);
		std::cout << Players[i].name << welcome << Players[i].ID << " y posicion " << Players[i].posX << " " << Players[i].posY << std::endl;
	}
	packR.clear();
}
void Ping() {
	//socket.setBlocking(false);
	sf::IpAddress IP;
	sf::Clock clock;
	unsigned short port;
	sf::Packet packPingR, packPingS;
	clock.restart();
	while (true) {
		if (socket.receive(packPingR, IP, port) != sf::Socket::Done) {
			std::cout << "Error el recivir ping" << std::endl;
		}
		else {
			std::string ACK;
			packPingR >> ACK;
			ACK = "ACK";
			packPingS << player.ID << ACK;

			//if (clock.getElapsedTime().asMilliseconds() >= 100) {
				if (socket.send(packPingS, "localhost", 50000) != sf::Socket::Done) {
					std::cout << "Error al enviar" << std::endl;
				}
				//std::cout << "send";
				clock.restart();
			}
		//}
		packPingR.clear();
		packPingS.clear();
	}
}
/**
* Contiene el código SFML que captura el evento del clic del mouse y el código que pinta por pantalla
*/
void Gameplay()
{
	sf::Vector2f casillaOrigen, casillaDestino;
	bool casillaMarcada = false;

	sf::RenderWindow window(sf::VideoMode(500, 500), "El Gato y el Raton");
	while (window.isOpen())
	{
		sf::Event event;
		//Ping();
		//Este primer WHILE es para controlar los eventos del mouse
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed:
				window.close();
				socket.unbind();
				break;
			case sf::Event::MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Left && tienesTurno)
				{
					int x = event.mouseButton.x;
					int y = event.mouseButton.y;
					if (!casillaMarcada)
					{
						casillaOrigen = TransformaCoordenadaACasilla(x, y);
						casillaMarcada = true;
						//TODO: Comprobar que la casilla marcada coincide con las posición del raton (si le toca al ratón)
						//o con la posicion de alguna de las piezas del gato (si le toca al gato)

					}
					else
					{
						casillaDestino = TransformaCoordenadaACasilla(x, y);
						if (casillaOrigen.x == casillaDestino.x && casillaOrigen.y == casillaDestino.y)
						{
							casillaMarcada = false;
							//Si me vuelven a picar sobre la misma casilla, la desmarco
						}
						else
						{
							if (quienSoy == TipoProceso::RATON)
							{
								//TODO: Validar que el destino del ratón es correcto

								//TODO: Si es correcto, modificar la posición del ratón y enviar las posiciones al padre

							}
							else if (quienSoy == TipoProceso::GATO)
							{
								//TODO: Validar que el destino del gato es correcto

								//TODO: Si es correcto, modificar la posición de la pieza correspondiente del gato y enviar las posiciones al padre
							}
						}
					}
				}
				break;
			default:
				break;

			}
		}

		window.clear();

		//A partir de aquí es para pintar por pantalla
		//Este FOR es para el tablero
		for (int i = 0; i<SIZE_FILA_TABLERO; i++)
		{
			for (int j = 0; j<SIZE_FILA_TABLERO; j++)
			{
				sf::RectangleShape rectBlanco(sf::Vector2f(LADO_CASILLA, LADO_CASILLA));
				rectBlanco.setFillColor(sf::Color::White);
				if (i % 2 == 0)
				{
					//Empieza por el blanco
					if (j % 2 == 0)
					{
						rectBlanco.setPosition(sf::Vector2f(i*LADO_CASILLA, j*LADO_CASILLA));
						window.draw(rectBlanco);
					}
				}
				else
				{
					//Empieza por el negro
					if (j % 2 == 1)
					{
						rectBlanco.setPosition(sf::Vector2f(i*LADO_CASILLA, j*LADO_CASILLA));
						window.draw(rectBlanco);
					}
				}
			}
		}

		//TODO: Para pintar el circulito del ratón
		/*sf::CircleShape shapeRaton(RADIO_AVATAR);
		shapeRaton.setFillColor(sf::Color::Blue);
		sf::Vector2f posicionRaton(player.posX, player.posY);
		posicionRaton = BoardToWindows(posicionRaton);
		shapeRaton.setPosition(posicionRaton);
		window.draw(shapeRaton);*/

		for (int i = 0; i < Players.size(); i++) {
			sf::CircleShape shapeRaton(RADIO_AVATAR);
			if (i == player.ID) {
				shapeRaton.setFillColor(sf::Color::Blue);
			}
			else {
				shapeRaton.setFillColor(sf::Color::Red);
			}
			sf::Vector2f positionGato1(TransformaCoordenadaACasilla(Players[i].posX, Players[i].posY));
			positionGato1 = BoardToWindows(positionGato1);
			shapeRaton.setPosition(positionGato1);

			window.draw(shapeRaton);
		}
	/*	//Pintamos los cuatro circulitos del gato
		sf::CircleShape shapeGato(RADIO_AVATAR);
		shapeGato.setFillColor(sf::Color::Red);

		sf::Vector2f positionGato1(1.f, 0.f);
		positionGato1 = BoardToWindows(positionGato1);
		shapeGato.setPosition(positionGato1);

		window.draw(shapeGato);

		sf::Vector2f positionGato2(3.f, 0.f);
		positionGato2 = BoardToWindows(positionGato2);
		shapeGato.setPosition(positionGato2);

		window.draw(shapeGato);

		sf::Vector2f positionGato3(5.f, 0.f);
		positionGato3 = BoardToWindows(positionGato3);
		shapeGato.setPosition(positionGato3);

		window.draw(shapeGato);*/
		if (!tienesTurno)
		{
			//Si no tengo el turno, pinto un letrerito de "Esperando..."
			sf::Font font;
			std::string pathFont = "liberation_sans/LiberationSans-Regular.ttf";
			if (!font.loadFromFile(pathFont))
			{
				std::cout << "No se pudo cargar la fuente" << std::endl;
			}


			sf::Text textEsperando("Esperando...", font);
			textEsperando.setPosition(sf::Vector2f(180, 200));
			textEsperando.setCharacterSize(30);
			textEsperando.setStyle(sf::Text::Bold);
			textEsperando.setFillColor(sf::Color::Green);
			window.draw(textEsperando);
		}
		else
		{
			//Si tengo el turno y hay una casilla marcada, la marco con un recuadro amarillo.
			if (casillaMarcada)
			{
				sf::RectangleShape rect(sf::Vector2f(LADO_CASILLA, LADO_CASILLA));
				rect.setPosition(sf::Vector2f(casillaOrigen.x*LADO_CASILLA, casillaOrigen.y*LADO_CASILLA));
				rect.setFillColor(sf::Color::Transparent);
				rect.setOutlineThickness(5);
				rect.setOutlineColor(sf::Color::Yellow);
				window.draw(rect);
			}
		}
		window.display();
	}
}

int main()
{
	Connection();
	std::thread tr(&Ping);
	Gameplay();
	tr.join();

	return 0;
}