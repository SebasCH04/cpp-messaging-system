#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

//codigos ANSI para colores
#define COLOR_GREEN "\033[1;32m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_RESET "\033[0m"

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
    return 5000;
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
    my_addr.sin_addr.s_addr = INADDR_ANY; //escuchar en todas las interfaces

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
            //se imprime el mensaje recibido en azul
            std::cout << "\n" << COLOR_BLUE << "[Mensaje recibido] " << rec_buffer << COLOR_RESET << std::endl;
        }
        close(sender_sock);
    }
    close(rec_sock);
}

int main() {
    //solicitar al usuario su nombre
    std::string nombre;
    std::cout << "Ingrese su nombre de usuario: ";
    std::getline(std::cin, nombre);

    //leer el puerto desde el archivo de configuracion 
    int puerto = leerPuertoDesdeConfig("cliente2/config.txt");
    std::string ip = obtenerIP();

    //conformar el mensaje de registro y enviarlo al servidor
    std::string mensajeRegistro = nombre + "|" + ip + "|" + std::to_string(puerto);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket registro");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000); //puerto del servidor
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect registro");
        close(sock);
        return 1;
    }

    send(sock, mensajeRegistro.c_str(), mensajeRegistro.size(), 0);
    std::cout << "\nRegistro enviado al servidor: " << mensajeRegistro << std::endl;
    close(sock);

    //crear un proceso para recibir mensajes, se usa fork
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        //proceso hijo es el encargado de recibir mensajes
        receptor(puerto);
        exit(0);
    }

    //proceso padre es el encargado del envio de mensajes
    while (true) {
        std::string destinatario, texto;
        std::cout << "\nEnviar a (nombre de usuario): ";
        std::getline(std::cin, destinatario);
        std::cout << "Mensaje: ";
        std::getline(std::cin, texto);
    
        std::string mensaje_envio = destinatario + "|" + nombre + ": " + texto;
    
        int mensaje_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (mensaje_sock < 0) {
            perror("socket envio");
            continue;
        }
    
        sockaddr_in server_addr2{};
        server_addr2.sin_family = AF_INET;
        server_addr2.sin_port = htons(9000); //mismo puerto del servidor
        inet_pton(AF_INET, "127.0.0.1", &server_addr2.sin_addr);
    
        if (connect(mensaje_sock, (struct sockaddr*)&server_addr2, sizeof(server_addr2)) < 0) {
            perror("connect envio");
            close(mensaje_sock);
            continue;
        }
    
        send(mensaje_sock, mensaje_envio.c_str(), mensaje_envio.size(), 0);
        //se muestra el mensaje enviado en verde
        std::cout << COLOR_GREEN << "[Mensaje enviado a " << destinatario << "] " << texto << COLOR_RESET << std::endl;
    
        close(mensaje_sock);
    }

    return 0;
}