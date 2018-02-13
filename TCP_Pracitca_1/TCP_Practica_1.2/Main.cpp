#include <SFML\Graphics.hpp>
#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <cstring>


#define MAX_MENSAJES 30
sf::TcpSocket socket;

std::string thread_recived() {
	std::size_t received;
	char buffer_Thread[2000];
	socket.receive(buffer_Thread, sizeof(buffer_Thread), received);
	std::cout << buffer_Thread << std::endl;
	return buffer_Thread;
}

int main()
{
	std::vector<std::string> aMensajes;

	sf::Vector2i screenDimensions(800, 600);

	sf::RenderWindow window;
	window.create(sf::VideoMode(screenDimensions.x, screenDimensions.y), "Chat");

	sf::Font font;
	if (!font.loadFromFile("digital.ttf"))
	{
		std::cout << "Can't load the font file" << std::endl;
	}

	sf::String mensaje = " >";

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

	char connectionType, mode;
	char buffer[2000];
	std::size_t received;
	std::string Stext = "Connected to: ";

	std::cout << "Enter (s) for Server, Enter (c) for Client: ";
	std::cin >> connectionType;
	/*if (connectionType == 'c') {
		std::cout << "Enter the server's ip: ";
		std::cin >> ip; //192.168.122.143
	}*/
	
	if (connectionType == 's')
	{
		sf::TcpListener listener;
		listener.listen(50000);
		listener.accept(socket);
		Stext += "Server";
		mode = 's';
		listener.close();
	}
	else if (connectionType == 'c')
	{
		socket.connect("192.168.122.143", 50000);
		Stext += "Client";
		mode = 'r';
	}

	socket.send(Stext.c_str(), Stext.length() + 1);
	socket.receive(buffer, sizeof(buffer), received);
	
std::cout << buffer << std::endl;
	std::thread tr(&thread_recived);

	bool done = false;
	while (!done)
	{

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
						//SEND			
						Stext = mensaje;
						socket.send(Stext.c_str(), Stext.length() + 1);
						aMensajes.push_back(mensaje);
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
			//RECIVE

			for (size_t i = 0; i < aMensajes.size(); i++)
			{
				std::string chatting = aMensajes[i];
				chattingText.setPosition(sf::Vector2f(0, 20 * i));
				chattingText.setString(chatting);
				window.draw(chattingText);
			}
			//mensaje = "";
			std::string mensaje_ = mensaje + "_";
			text.setString(mensaje_);
			window.draw(text);


			window.display();
			window.clear();
		}
	}

	socket.disconnect();
	return 0;
}