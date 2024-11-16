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

map<string, set<FileInfo>> name_sfileinfo;
map<FileInfo, vector<ip_port>> fileinfo_vip;

int cant_servidores;
mutex mutexito;


vector<FileInfo> searchFileBySubstring(const string subString){
    vector<FileInfo> results;

    // Recorrer cada entrada en la base de datos de archivos
    for (const auto& [fileName, registers] : name_sfileinfo) {
        if (fileName.find(subString) != string::npos){
            // Verifica si subcadena está en el nombre del archivo
            // el file actual contiene el substring buscado
            for (const auto& data_file_interested : registers) {
                results.push_back(data_file_interested);
            }
        }
    }
    return results;
}





// Función start_server modificada para manejar una conexión específica
void start_server(int client_socket, ip_port ip_port_client) {
    //Recibir cantida de archivos iniciales por cliente

    int cant_archivos;
    if (!receiveIntThroughRed(client_socket,cant_archivos)){
        cerr << "Error al recibir el numero de archivos\n";
        close(client_socket);
        return;
    }

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
        name_sfileinfo[fileName].insert(fileInfo);
        fileinfo_vip[fileInfo].push_back(ip_port_client);
    }
    mutexito.unlock();

    while (true){
        cout << "Esperando instruccion\n";
        //Recibir instruccion
        string instr;
        if (!receiveStringThroughRed(client_socket,instr)){
            cerr << "Error recibiendo el string de instruccion\n";
            break;;
        }

        if (instr == "request"){
            cout << "Estoy en request\n";
            //Recibir S H H del cliente
            FileInfo info_selected;
            if (!receiveFileInfoThroughRed(client_socket,info_selected)){
                cerr << "Error recibiendo el S H H escogido";
                break;
            }
            info_selected.display();

            //Buscar cantidad ips que tienen ese archivo y no es el mismo cliente
            mutexito.lock();
            vector<ip_port> ips;
            for (auto &cur_ip: fileinfo_vip[info_selected]){
                if (cur_ip.ip == ip_port_client.ip && cur_ip.port == ip_port_client.port){
                    continue;
                }
                ips.push_back(cur_ip);
            }
            mutexito.unlock();

            int num_ips = ips.size();

            if (!sendIntThroughRed(client_socket,num_ips)){
                cerr << "Error enviando num_ips: " << num_ips << endl;
                break;
            }
            if (num_ips == 0){
                continue;
            }
            //Enviar ips
            for (auto &cur_ip: ips){
                if (!sendIpThroughRed(client_socket,cur_ip)){
                    cerr << "Error enviando la ip " << cur_ip.ip << endl;
                    break;
                }
            }
            // Ver si el servidor logro finalizar o no (todos los clientes borraron el archivo)
            int success;
            if (!receiveIntThroughRed(client_socket,success)){
                cerr << "Error recibiendo si el cliente tuvo exito con el archivo\n";
                return;
            } 
            if (!success) continue;
            string new_file_str;
            if (!receiveStringThroughRed(client_socket,new_file_str)){
                cerr << "Error recibiendo el nombre del nuevo archivo\n";
                break;
            }
            //save the file
            mutexito.lock();
            name_sfileinfo[new_file_str].insert(info_selected);
            fileinfo_vip[info_selected].push_back(ip_port_client);
            mutexito.unlock();

            cout << "Archivo:<" << new_file_str << ">guardado\n";
        }
        else if (instr == "find"){
            //Recibir nombre del substring
            string file_name;
            if (!receiveStringThroughRed(client_socket, file_name)){
                cerr << "Error recibiendo el nombre del archivo por red\n";
                break;
            }

            //Buscar archivos que contienen el substring
            mutexito.lock();
            vector<FileInfo> data = searchFileBySubstring(file_name);
            mutexito.unlock();


            //Enviar numero de archivos
            if (!sendIntThroughRed(client_socket,data.size())){
                cerr << "Error enviando la cantidad de archivos con el substring buscado\n";
                break;
            }
            
            //Enviar cada archivo
            for (int i=0; i<data.size(); i++){
                FileInfo cur_data = data[i];
                if (!sendFileInfoThroughRed(client_socket,cur_data)){
                    cerr << "Error enviando S H H de un archivo al cliente\n";
                    break;
                }
            }
        } else if (instr=="wq;"){
            break;
        } else {
            cout << "Instruccion recibida incorrecta\n";
        }
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
//request 4261194 399067255 45019805