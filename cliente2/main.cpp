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

// Función para quitar espacios al inicio y final de una cadena.
std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

int leerPuertoDesdeConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: No se pudo abrir el archivo de configuracion: " << filename << std::endl;
        return 5000; // Valor por defecto si no se pudo abrir el archivo
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line); // Eliminamos espacios innecesarios
        // Verificamos que la línea comience exactamente con "PUERTO="
        if (line.rfind("PUERTO=", 0) == 0) {
            try {
                // Convertimos la parte numérica a entero
                return std::stoi(line.substr(7));
            } catch (const std::exception& e) {
                std::cerr << "Error al convertir el puerto: " << e.what() << std::endl;
                return 5000; // Retornamos el valor por defecto en caso de error
            }
        }
    }
    return 5000; // Valor por defecto si no se encontró la línea
}

void receptor(int puerto) {
    int rec_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (rec_sock < 0) {
        perror("socket receptor");
        exit(1);
    }

    sockaddr_in my_addr{};
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(puerto);
    my_addr.sin_addr.s_addr = INADDR_ANY; // Escuchar en todas las interfaces

    if (bind(rec_sock, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind receptor");
        exit(1);
    }

    if (listen(rec_sock, 5) < 0) {
        perror("listen receptor");
        exit(1);
    }

    std::cout << "Escuchando mensajes en el puerto " << puerto << "...\n";

    while (true) {
        sockaddr_in sender_addr{};
        socklen_t addrlen = sizeof(sender_addr);
        int sender_sock = accept(rec_sock, (struct sockaddr*)&sender_addr, &addrlen);
        if (sender_sock < 0) {
            perror("accept receptor");
            continue;
        }
        char rec_buffer[1024];
        ssize_t rec_bytes = recv(sender_sock, rec_buffer, sizeof(rec_buffer) - 1, 0);
        if (rec_bytes > 0) {
            rec_buffer[rec_bytes] = '\0';
            std::cout << "\n[Mensaje recibido] " << rec_buffer << std::endl;
        }
        close(sender_sock);
    }
    close(rec_sock);
}

int main() {
    std::string nombre;
    std::cout << "Ingrese su nombre de usuario: ";
    std::getline(std::cin, nombre);

    int puerto = leerPuertoDesdeConfig("cliente2/config.txt");
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

    // Crear un proceso para recibir mensajes
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        // Proceso hijo: receptor de mensajes
        receptor(puerto);
        exit(0);
    }

    while (true) {
        std::string destinatario, texto;
        std::cout << "\nEnviar a (nombre de usuario): ";
        std::getline(std::cin, destinatario);
        std::cout << "Mensaje: ";
        std::getline(std::cin, texto);
    
        std::string mensaje_envio = destinatario + "|" + nombre + ": " + texto;
    
        int mensaje_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (mensaje_sock < 0) {
            perror("socket");
            continue;
        }
    
        sockaddr_in server_addr2{};
        server_addr2.sin_family = AF_INET;
        server_addr2.sin_port = htons(9000); // mismo puerto del servidor
        inet_pton(AF_INET, "127.0.0.1", &server_addr2.sin_addr); // servidor local
    
        if (connect(mensaje_sock, (struct sockaddr*)&server_addr2, sizeof(server_addr2)) < 0) {
            perror("connect");
            close(mensaje_sock);
            continue;
        }
    
        send(mensaje_sock, mensaje_envio.c_str(), mensaje_envio.size(), 0);
        std::cout << "Mensaje enviado a " << destinatario << std::endl;
    
        close(mensaje_sock);
    }

    return 0;
}