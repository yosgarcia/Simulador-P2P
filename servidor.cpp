#include <bits/stdc++.h>
#include <thread>
#include <mutex>

#include <cstring>      // Para memset
#include <arpa/inet.h>  // Para sockaddr_in y htons
#include <unistd.h>  
#include "comunes.hpp"
using namespace std;


// tabla (marca, lista de conocido como)
//class Tabla
//      - Marca
//      - List<strings, ip>
// 

typedef pair<FileInfo,ip_port> data_file;
// hola.jpg (12,1,20) -> cliente 1
// hola.jpg (11,1,25) -> cliente 2
/*
(12,1,20) -> archivitio en 1033.1222.112.
            -> miarchivo en 1222.464.333.

(21,1,33) -> archivitio en 5555.30393.112.
            -> prube en 1222.777.333.               

*/

//  archivito, {{1033.1222.122, la direccion},{5555.x.x y la ip}}

map<string, vector<data_file>> fileDatabase;
int cant_servidores;
mutex mutexito;


// Función start_server modificada para manejar una conexión específica
void start_server(int client_socket, ip_port ip_port_client) {
    int cant_archivos_network_order;
    int valread = recv(client_socket, &cant_archivos_network_order, sizeof(cant_archivos_network_order), 0);
    if (valread <= 0) {
        std::cerr << "Error al recibir el número de archivos" << std::endl;
        close(client_socket);
        return;
    }

    int cant_archivos = ntohl(cant_archivos_network_order);
    std::cout << "Número de archivos recibidos: " << cant_archivos << std::endl;
    
    std::map<FileInfo, std::string> local;

    // Recibir datos de cada archivo
    for (int i = 0; i < cant_archivos; ++i) {
        int data_size_network_order;
        int valread = recv(client_socket, &data_size_network_order, sizeof(data_size_network_order), 0);
        if (valread <= 0) {
            std::cerr << "Error al recibir el tamaño del archivo" << std::endl;
            break;
        }

        int data_size = ntohl(data_size_network_order);
        std::cout << "Tamaño del archivo a recibir: " << data_size << " bytes" << std::endl;

        send(client_socket, "OK", 2, 0);

        // Leer los datos del archivo
        char buffer[1024];
        std::string file_data;
        int bytes_received = 0;

        while (bytes_received < data_size) {
            int chunk_size = recv(client_socket, buffer, sizeof(buffer), 0);
            if (chunk_size <= 0) {
                std::cerr << "Error al recibir los datos del archivo" << std::endl;
                break;
            }
            bytes_received += chunk_size;
            file_data.append(buffer, chunk_size);
        }

        // Procesar los datos recibidos
        std::istringstream dataStream(file_data);
        long long hash1, hash2;
        size_t size;
        std::string fileName;
                
        while (dataStream >> hash1 >> hash2 >> size) {
            std::getline(dataStream, fileName);
            if (!fileName.empty() && fileName[0] == ' ') {
                fileName.erase(0, 1);
            }
            FileInfo fileInfo(hash1, hash2, size);
            local[fileInfo] = fileName;

            std::cout << "Archivo recibido:" << std::endl;
            std::cout << "  Hash1: " << hash1 << std::endl;
            std::cout << "  Hash2: " << hash2 << std::endl;
            std::cout << "  Tamaño: " << size << " bytes" << std::endl;
            std::cout << "  Nombre: " << fileName << std::endl;
        }
        send(client_socket, "OK", 2, 0);
    }

    mutexito.lock();
    for (const auto& [fileInfo, fileName] : local) {
        data_file data = {fileInfo, ip_port_client};
        fileDatabase[fileName].push_back(data);
    }
    mutexito.unlock();

    close(client_socket);
}

void start(){
    string ip = "127.0.0.1";
    int port = 8080;
    int cant_clientes = 2;
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Crear el socket de escucha
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Error al crear el socket" << std::endl;
        return;
    }

    // Configurar la dirección del socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str());
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error al hacer bind" << std::endl;
        close(server_fd);
        return;
    }
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Error al escuchar" << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "Servidor escuchando en " << ip << ":" << port << "..." << std::endl;

    while (true) {
        int client_socket;
        struct sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);

        // Aceptar una nueva conexión de cliente
        if ((client_socket = accept(server_fd, (struct sockaddr*)&client_address, &client_addrlen)) < 0) {
            std::cerr << "Error al aceptar conexión" << std::endl;
            continue;
        }

        // Obtener la dirección IP y puerto del cliente
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_address.sin_port);

        cout << "Nueva conexión de " << client_ip << ":" << client_port << std::endl;

        // Pasar IP y puerto del cliente a `start_server`
        ip_port client_info = {client_ip, client_port};

        // Crear un hilo para cada cliente usando `start_server`
        std::thread client_thread(start_server, client_socket, client_info);
        client_thread.detach();
    }

    // Cerrar el socket de escucha cuando se termina
    close(server_fd);
}

int main(){
    start();
    return 0;
}
