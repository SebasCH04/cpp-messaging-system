#include <iostream>
#include <unordered_map>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> 

struct Usuario {
    std::string ip;
    int puerto;
};

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Escucha en todas las interfaces
    server_addr.sin_port = htons(9000); // Puerto fijo del servidor

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::unordered_map<std::string, Usuario> usuarios;

    std::cout << "Servidor escuchando en el puerto 9000..." << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_size = sizeof(client_addr);
        int client_socket = accept(server_fd, (sockaddr*)&client_addr, &client_size);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        char buffer[1024];
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string mensaje(buffer);

            size_t pos1 = mensaje.find('|');
            size_t pos2 = mensaje.find('|', pos1 + 1);

            // Registro de usuario: nombre|ip|puerto
            if (pos1 != std::string::npos && pos2 != std::string::npos && mensaje.find(':') == std::string::npos) {
                std::string nombre = mensaje.substr(0, pos1);
                std::string ip = mensaje.substr(pos1 + 1, pos2 - pos1 - 1);
                int puerto = std::stoi(mensaje.substr(pos2 + 1));

                usuarios[nombre] = Usuario{ip, puerto};
                std::cout << "Usuario registrado: " << nombre << " (" << ip << ":" << puerto << ")" << std::endl;

            // Envío de mensaje: destinatario|remitente: mensaje
            } else if (pos1 != std::string::npos) {
                std::string destinatario = mensaje.substr(0, pos1);
                std::string contenido = mensaje.substr(pos1 + 1);

                if (usuarios.find(destinatario) != usuarios.end()) {
                    Usuario& destino = usuarios[destinatario];

                    int dest_sock = socket(AF_INET, SOCK_STREAM, 0);
                    if (dest_sock < 0) {
                        perror("socket destinatario");
                        continue;
                    }

                    sockaddr_in dest_addr{};
                    dest_addr.sin_family = AF_INET;
                    dest_addr.sin_port = htons(destino.puerto);
                    inet_pton(AF_INET, destino.ip.c_str(), &dest_addr.sin_addr);

                    if (connect(dest_sock, (sockaddr*)&dest_addr, sizeof(dest_addr)) == 0) {
                        send(dest_sock, contenido.c_str(), contenido.size(), 0);
                        std::cout << "Mensaje enviado a " << destinatario << ": " << contenido << std::endl;
                    } else {
                        std::cerr << "No se pudo conectar con el destinatario " << destinatario << std::endl;
                    }

                    close(dest_sock);
                } else {
                    std::cerr << "Destinatario '" << destinatario << "' no encontrado en la tabla de usuarios.\n";
                }
            }

        }

        close(client_socket);
    }

    close(server_fd);
    return 0;
}
