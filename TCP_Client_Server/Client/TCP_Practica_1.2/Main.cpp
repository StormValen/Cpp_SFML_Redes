#include <SFML\Graphics.hpp>
#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>


#define MAX_MENSAJES 30
sf::TcpSocket socket;
sf::TcpListener listener;
sf::Socket::Status status;
std::mutex mu;
bool done;

// ----- MUTEX ----- //
void shared_msg(std::vector<std::string> *aMensajes, sf::String string) {
	mu.lock();
	aMensajes->push_back(string);
	if (aMensajes->size() > 25)
	{
		aMensajes->erase(aMensajes->begin(), aMensajes->begin() + 1);
	}
	mu.unlock();
}

// ----- BLOCKING THREAD ----- //
void thread_recived(std::vector<std::string> *aMensajes) {
	bool tBucle = true;
	while (tBucle) {
		std::size_t received;
		char buffer_Thread[2000];
		status = socket.receive(buffer_Thread, sizeof(buffer_Thread), received);
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
}

int main()
{
	// ----- WINDOW UI ----- //
	std::vector<std::string> aMensajes;
	sf::Vector2i screenDimensions(800, 600);
	sf::RenderWindow window;
	window.create(sf::VideoMode(screenDimensions.x, screenDimensions.y), "Chat");

	sf::Font font;
	if (!font.loadFromFile("digital.ttf"))
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

	sf::IpAddress ip = sf::IpAddress::getLocalAddress();

	std::string clientName = "";
	std::cout << "Input your name: " << std::endl;
	std::cin >> clientName;

	//  ----- SELECTION MODE ----- //
	char connectionType, mode;
	std::string executingMode;
	char buffer[2000];
	std::size_t received;
	std::string Stext = "";

	status = socket.connect("localhost", 50000);
	if (status != sf::Socket::Done) {
		std::cout << "ERROR: Can't open listener";
		exit(0);
	}
	Stext += "Conexion establecida con un cliente nuevo";
	mode = 'r';
	executingMode = "b";
	
	 // ----- CONFIRM CONNECTION ----- //
	socket.send(clientName.c_str(), clientName.length() + 1);
	socket.receive(buffer, sizeof(buffer), received);
	std::cout << buffer << std::endl;

	// ----- MODOS DE EJECUCION ----- // 
	if (executingMode == "b") { //Blocking +  Thread
		std::thread tr(&thread_recived, &aMensajes);
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

						Stext = "[ " + clientName + " ]   " + mensaje;
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
		socket.disconnect();
		tr.join();
	}
	return 0;
}