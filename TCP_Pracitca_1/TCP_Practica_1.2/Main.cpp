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
sf::SocketSelector socketSelector;
std::vector<sf::TcpSocket*> clients;
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
		if (status == sf::Socket::Done) {
			if (buffer_Thread == ">exit") {
				aMensajes->push_back("La sesion ha finalizado");
				tBucle = false;
			}
			else {
				shared_msg(aMensajes, buffer_Thread);
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

	//  ----- SELECTION MODE ----- //
	char connectionType, mode;
	std::string executingMode;
	char buffer[2000];
	std::size_t received;
	std::string Stext = "";

	std::cout << "Enter (s) for Server, Enter (c) for Client: ";
	std::cin >> connectionType;

	if (connectionType == 's')
	{
		std::cout << "Enter (b) for Blocking + Threading, Enter (n) for NonBlocking, Enter (s) for Blocking + SocketSelector: " << std::endl;
		std::cin >> executingMode;

		if (executingMode == "b") {
			status = listener.listen(50000);
			status = listener.accept(socket); 
			if (status != sf::Socket::Done) { //check port
				std::cout << "Error de puerto" << std::endl;
			}
			//Stext += "Server";
			mode = 's';
			listener.close();
		}
		else if (executingMode == "n") {
			status = listener.listen(50000);
			status = listener.accept(socket);
			if (status != sf::Socket::Done) {//check port
				std::cout << "Error de puerto" << std::endl;
			}
			//Stext += "Server";
			mode = 's';
			//unblock the sockets
			listener.setBlocking(false);
			socket.setBlocking(false);
			listener.close();
		}
		else if (executingMode == "s") {
			status = listener.listen(50000);
			status = listener.accept(socket);
			if (status != sf::Socket::Done) { //check port
				std::cout << "Error de puerto" << std::endl;
			}
			//Stext += "Server";
			mode = 's';
			listener.close();
		}
		//Se envia el modo de ejecucion al cliente.
		socket.send(executingMode.c_str(), executingMode.length() + 1);
	}
	else if (connectionType == 'c')
	{
		status = socket.connect("192.168.1.33", 50000);
		//Stext += "Client";
		mode = 'r';

		//Se recive el modo de ejecucion desde el server.
		socket.receive(buffer, sizeof(buffer), received);
		executingMode = buffer;

		if(executingMode == "b"){
			std::cout << "INFO: (b) Blocking + Threading" << std::endl;
		}
		else if (executingMode == "n") {
			std::cout << "INFO: (n) NonBlonking" << std::endl;
			socket.setBlocking(false); //unblock de socket
		}
		else if (executingMode == "s") {
			std::cout << "INFO: (s) Bloking + Socket Selector" << std::endl;
		}
		
	}
	 // ----- CONFIRM CONNECTION ----- //
	socket.send(Stext.c_str(), Stext.length() + 1);
	socket.receive(buffer, sizeof(buffer), received);
	std::cout << buffer << std::endl;

	// ----- MODOS DE EJECUCION ----- // 
	if (executingMode == "b") { //Blocking +  Thread
		std::thread tr(&thread_recived, &aMensajes);
		done = false;

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
							Stext = mensaje;
							//std::cout << Stext << std::endl;
							//if (Stext != ">exit") {
								
								status = socket.send(Stext.c_str(), Stext.length() + 1);
								if (status == sf::Socket::Done) {
									if (Stext == ">exit") {
										aMensajes.push_back("La sesion ha finalizado");				
										window.close();
										//return 0;
									}
									else {
										shared_msg(&aMensajes, mensaje);
										//aMensajes.push_back(mensaje);
									}
								}
								else if (status == sf::Socket::Disconnected) {
									aMensajes.push_back("Se ha producido una desconexion");
									window.close();
								}
								
								mensaje = ">";
							//}
													
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
		
	
	else if (executingMode == "n") { //NonBloking
		done = false;
		while (!done)
		{
			while (window.isOpen())
			{
				//RECIVE
				std::size_t received;
				char buffer_Thread[2000];
				status = socket.receive(buffer_Thread, sizeof(buffer_Thread), received);
				/*if (status == sf::Socket::NotReady) { //no se necesita, tampoco funciona
					continue;
				}*/
				 if (status == sf::Socket::Done) {
					aMensajes.push_back(buffer_Thread);
				}
				else if (status == sf::Socket::Disconnected) {
					aMensajes.push_back("Se ha producido una desconexion");
					socket.disconnect();
					break;
				}
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
							size_t bytesSent;
							Stext = mensaje;
							status = socket.send(Stext.c_str(), Stext.length(), bytesSent); //se modifica el send para comprobar que llega todo el mensaje
							if (status != sf::Socket::Done) {
								if (status == sf::Socket::Disconnected) {
									aMensajes.push_back("Se ha producido una desconexion");
									socket.disconnect();
									break;
								}
								else if (status == sf::Socket::Partial) {
									//aMensajes.push_back("No se han enviado todos los datos");
									//si no llega todo el mensaje hay que hacer un bucle y enviar lo que falta
									while (bytesSent < Stext.length()) {
										//std::cout << "PArtial" << std::endl;
										std::string msgRest = "";
										for (size_t i = bytesSent; i < Stext.length(); i++) {
											msgRest = Stext[i];
										}
										socket.send(msgRest.c_str(), msgRest.length(), bytesSent);
									}
								}
							}
							else {
								aMensajes.push_back(mensaje);
							}
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
	}
	else if (executingMode == "s") { //Socket Selector
		
	}
	
	socket.disconnect();
	return 0;
}