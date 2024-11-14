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


vector<data_file> searchFileBySubstring(const string subString){
    vector<data_file> results;

    // Recorrer cada entrada en la base de datos de archivos
    for (const auto& [fileName, registers] : fileDatabase) {
        if (fileName.find(subString) != string::npos) {  // Verifica si subcadena está en el nombre del archivo
            //el file actual contiene el substring buscado
            for (const auto& data_file_interested : registers) {
                results.push_back(data_file_interested);
            }
        }
    }
    return results;
}





// Función start_server modificada para manejar una conexión específica
void start_server(int client_socket, ip_port ip_port_client) {


    // -----------------------------------------------
    /*
    char buffer[1024];
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        cerr << "Error al recibir datos del cliente" << endl;
        close(client_socket);
        return;
    }
    buffer[bytes_read] = '\0';

    string command = buffer;
    if(command.rfind("FIND ", 0) == 0){
        string substring = command.substr(7);
        auto results = searchFileBySubstring(substring);
        stringstream respuesta;
        respuesta << "Resultados de búsqueda para '" << substring << "':\n";
        for (const auto& [fileName, fileInfo, clientInfo] : substring) {
            respuesta << "  - Archivo: " << fileName << "\n";
            respuesta << "    - Tamaño: " << fileInfo.size << " bytes\n";
            respuesta << "    - Hash1: " << fileInfo.hash1 << "\n";
            respuesta << "    - Hash2: " << fileInfo.hash2 << "\n";
            respuesta << "    - Ubicación: IP " << clientInfo.ip << ", Puerto: " << clientInfo.port << "\n";
        }

        // Enviar la respuesta al cliente
        send(client_socket, respuesta.str().c_str(), respuesta.str().length(), 0);
    }
*/
    // -----------------------------------------------

    int cant_archivos;
    if (!receiveIntThroughRed(client_socket,cant_archivos)){
        cerr << "Error al recibir el numero de archivos\n";
        close(client_socket);
        return;
    }

    std::cout << "Número de archivos recibidos: " << cant_archivos << std::endl;
    
    std::map<FileInfo, std::string> local;

    // Recibir datos de cada archivo
    for (int i = 0; i < cant_archivos; ++i) {
        // Recibir datos por red
        FileInfo fileInfo;
        string fileName;

        if (!receiveFileInfoWithNameThroughRed(client_socket,fileInfo, fileName)){
            cerr << "Error recibiendo datos de un archivo\n";
            break;
        }
        local[fileInfo] = fileName;

    }
    //Guardar archivos iniciales en la bd

    mutexito.lock();
    for (const auto& [fileInfo, fileName] : local) {
        data_file data = {fileInfo, ip_port_client};
        fileDatabase[fileName].push_back(data);
    }
    mutexito.unlock();

    while (true){
        //Leer nombre del string consultado
        string file_name;
        if (!receiveStringThroughRed(client_socket,file_name)){
            cerr << "Error recibiendo el nombre del archivo por red\n";
            break;
        }

        mutexito.lock();

        cout << "Buscando archivo con substring: " << file_name << endl;
        vector<data_file> data = searchFileBySubstring(file_name);
        cout << data.size() << endl;

        mutexito.unlock();
        cout << "fin\n";
        /*
        int number_files_to_choose = data.size();
        if (!sendIntThroughRed(client_socket,number_files_to_choose)){
            cerr << "Error al enviar la cantidad de archivos a escoger que tienen el substring: " << file_name << endl;
            break;
        }
    */
  

    }

    close(client_socket);
}

void start(ip_port ip_server){
    string ip = ip_server.ip;
    int port = ip_server.port;
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

int main(int argc, char* argv[]){
    if (argc < 2){
        cerr << "indique el ip:puerto a donde se va a correr el servidor\n";
        return 1;
    }
    ip_port ip_server;
    if (!parseIpPort(argv[1], ip_server)){
        cerr << "Formato inválido para server_ip:puerto. Use ip:puerto\n";
        return 1;
    }

    start(ip_server);
    return 0;
}

// g++ comunes.cpp servidor.cpp -o servidor
// ./servidor 127.0.0.1:8080