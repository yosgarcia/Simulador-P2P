#include <bits/stdc++.h>
#include <thread>
#include <mutex>
#include <cstring>      
#include <arpa/inet.h>  // Para sockaddr_in y htons
#include <unistd.h>  
#include "comunes.hpp"
using namespace std;



map<string, set<FileInfo>> name_sfileinfo;
map<FileInfo, set<ip_port>> fileinfo_sip;

int cant_servidores;
mutex mutexito;


//file info y ip:puerto del cliente
set<pairfi_ip> searchFileBySubstring(const string subString){
    set<pairfi_ip> results;

    for (const auto& [fileName, registers] : name_sfileinfo) {
        if (fileName.find(subString) != string::npos){
            for (const auto& file_info : registers) {
                //buscar todos los que tienen ese ip
                for (const auto& ip_info: fileinfo_sip[file_info]){
                    results.insert({file_info,ip_info});
                }
            }
        }
    }
    return results;
}


// Función start_server modificada para manejar una conexión específica
void start_server(int client_socket) {
    //obtener la ip_puerto reales?
    ip_port ip_port_client;
    if (!receiveIpThroughRed(client_socket,ip_port_client)){
        cerr << "Error recibiendo la ip verdadera del cliente\n";
        return;
    }
    //cout << "Hablando con " << ip_port_to_str(ip_port_client) << endl;
    //Recibir cantida de archivos iniciales por cliente

    int cant_archivos;
    if (!receiveIntThroughRed(client_socket,cant_archivos)){
        cerr << "Error al recibir el numero de archivos\n";
        close(client_socket);
        return;
    }

    map<FileInfo, string> local;

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
        fileinfo_sip[fileInfo].insert(ip_port_client);
    }
    mutexito.unlock();

    while (true){
        cout << "Esperando instrucciones\n";
        //Recibir instruccion
        string instr;
        if (!receiveStringThroughRed(client_socket,instr)){
            cerr << "Error recibiendo el string de instruccion\n";
            break;;
        }

        if (instr == "request"){
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
            for (auto &cur_ip: fileinfo_sip[info_selected]){
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
            string new_file_str;
            // Ver si el servidor logro finalizar o no (todos los clientes borraron el archivo)
            int success;
            if (!receiveIntThroughRed(client_socket,success)){
                cerr << "Error recibiendo si el cliente logro guardar el archivo\n";
                break;
            }
            if (!success){
                continue;
            }
            if (!receiveStringThroughRed(client_socket,new_file_str)){
                cerr << "Error recibiendo el nombre del nuevo archivo\n";
                break;
            }
            //successful save
            FileInfo n_fi;
            if (!receiveFileInfoThroughRed(client_socket,n_fi)){
                cerr << "Error recibiendo el file info que el cliente logro guardar\n";
                break;
            }
            mutexito.lock();
            name_sfileinfo[new_file_str].insert(n_fi);
            fileinfo_sip[n_fi].insert(ip_port_client);
            mutexito.unlock();

            //Indicar guardado en server
            if (!sendIntThroughRed(client_socket,1)){
                cerr << "Error indicando al cliente que el archivo fue guardado\n";
                break;
            }

            cout << "Archivo: <" << new_file_str << "> guardado\n";
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
            set<pairfi_ip> data = searchFileBySubstring(file_name);
            mutexito.unlock();


            //Enviar numero de archivos
            if (!sendIntThroughRed(client_socket,data.size())){
                cerr << "Error enviando la cantidad de archivos con el substring buscado\n";
                break;
            }
            
            //Enviar cada archivo
            for (auto &fi_ip: data){
                if (!send_pairFi_Ip_ThroughRed(client_socket,fi_ip)){
                    cerr << "Error comunicando pair de file info e ip\n";
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
        cerr << "Error al crear el socket" << endl;
        return;
    }

    // Configurar la dirección del socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str());
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Error al hacer bind" << endl;
        close(server_fd);
        return;
    }
    if (listen(server_fd, 10) < 0) {
        cerr << "Error al escuchar" << endl;
        close(server_fd);
        return;
    }

    cout << "Servidor escuchando en " << ip << ":" << port << "..." << endl;

    while (true) {
        int client_socket;
        struct sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);

        // Aceptar una nueva conexión de cliente
        if ((client_socket = accept(server_fd, (struct sockaddr*)&client_address, &client_addrlen)) < 0) {
            cerr << "Error al aceptar conexión" << endl;
            continue;
        }

        // Obtener la dirección IP y puerto del cliente
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_address.sin_port);

        cout << "\nNueva conexión recibida: " << client_ip << ":" << client_port << "\n\n";

        // Pasar IP y puerto del cliente a `start_server`
        ip_port client_info = {client_ip, client_port};

        // Crear un hilo para cada cliente usando `start_server`
        thread client_thread(start_server, client_socket);
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

/*
g++ comunes.cpp servidor.cpp -o servidor

./servidor 127.0.0.1:8080
./servidor 192.168.100.197:8080

./cliente 192.168.100.197:8080 192.168.100.197:8081 /home/santy/Documentos/Sistemas Operativos/Simulador-P2P/carpetas_prueba/carpeta_1
./cliente 192.168.100.197:8080 192.168.100.197:8082 /home/santy/Documentos/Sistemas Operativos/Simulador-P2P/carpetas_prueba/carpeta_2
./cliente 192.168.100.197:8080 192.168.100.197:8083 /home/santy/Documentos/Sistemas Operativos/Simulador-P2P/carpetas_prueba/carpeta_3
./cliente 192.168.100.197:8080 192.168.100.197:8085 /home/santy/Documentos/Sistemas Operativos/Simulador-P2P/carpetas_prueba/carpeta_5

*/