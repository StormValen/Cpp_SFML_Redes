#include <SFML\Network.hpp>
#include <iostream>


int main()
{
	sf::TcpSocket socket;
	std::string textoAEnviar="";

		sf::TcpListener listener;
		listener.listen(50000);
		listener.accept(socket);
		textoAEnviar = "Mensaje desde servidor\n";

	std::string texto = "Conexion con ... " + (socket.getRemoteAddress()).toString() + ":" + std::to_string(socket.getRemotePort()) + "\n";
	std::cout << texto;

	socket.send(textoAEnviar.c_str(), texto.length());

	char buffer[100];
	size_t bytesReceived;
	socket.receive(buffer, 100, bytesReceived);

	buffer[bytesReceived] = '\0';
	std::cout << "Mensaje recibido: " << buffer << std::endl;

	system("pause");
	
	return 0;

}