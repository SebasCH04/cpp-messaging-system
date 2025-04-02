#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

std::string obtenerIP() {
    char host[256];
    if (gethostname(host, sizeof(host)) == -1) {
        perror("gethostname");
        return "0.0.0.0";
    }

    struct hostent* he = gethostbyname(host);
    if (!he) {
        perror("gethostbyname");
        return "0.0.0.0";
    }

    struct in_addr** addr_list = (struct in_addr**)he->h_addr_list;
    return inet_ntoa(*addr_list[0]);
}

int leerPuertoDesdeConfig(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("PUERTO=", 0) == 0) {
            return std::stoi(line.substr(7));
        }
    }
    return 5000; // Valor por defecto
}

int main() {
    std::string nombre;
    std::cout << "Ingrese su nombre de usuario: ";
    std::getline(std::cin, nombre);

    int puerto = leerPuertoDesdeConfig("config.txt");
    std::string ip = obtenerIP();

    std::string mensaje = nombre + "|" + ip + "|" + std::to_string(puerto);

    // Crear socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000); // puerto del servidor
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // IP del servidor

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    send(sock, mensaje.c_str(), mensaje.size(), 0);
    std::cout << "Registro enviado al servidor: " << mensaje << std::endl;

    close(sock);
    return 0;
}
