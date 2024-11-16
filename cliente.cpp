#include <bits/stdc++.h>
#include <thread>
#include <cstring>      // Para memset
#include <arpa/inet.h>  // Para sockaddr_in y htons
#include <unistd.h>     // Para close
#include <iomanip>
#include "comunes.hpp"

using namespace std;

// Info y la direccion del archivo
map<FileInfo,string> files;
map<string,string> name_fullpath;
mutex mutexito;

pair<long long, long long> hashes(ifstream &file) {
    const long long f_prime = 197;  // Primo más grande
    const long long f_mod = 1e9 + 9; // Módulo grande para evitar colisiones
    long long f_hash_value = 0;
    long long f_power = 1; // Para la potencia del primo

    const long long s_prime1 = 1234567891; // Un primo grande
    const long long s_mod = 1e9 + 21;     // Módulo grande para evitar colisiones
    long long s_hash_value = 0;
    long long s_power = 1;

    char byte;
    while (file.get(byte)) {
        // Aplicamos una combinación de multiplicación y desplazamiento
        s_hash_value = (s_hash_value + byte * s_power) % s_mod;
        s_power = (s_power * s_prime1) % s_mod; // Incrementamos la potencia del primo
        
        // Hashing polinómico: h(x) = (h(x-1) * p + byte) % mod
        f_hash_value = (f_hash_value + (byte * f_power) % f_mod) % f_mod;
        f_power = (f_power * f_prime) % f_mod; // Incrementa la potencia del primo
    }
    return {f_hash_value,s_hash_value};
}


long long hash_2(ifstream& file) {
    const long long prime1 = 1234567891; // Un primo grande
    const long long prime2 = 987654319; // Otro primo grande
    const long long mod = 1e9 + 21;     // Módulo grande para evitar colisiones

    long long hash_value = 0;
    long long power = 1;

    char byte;
    while (file.get(byte)) {
        // Aplicamos una combinación de multiplicación y desplazamiento
        hash_value = (hash_value + byte * power) % mod;
        power = (power * prime1) % mod; // Incrementamos la potencia del primo
    }
    return hash_value;
}


size_t getFileSize(std::ifstream &file) {
    streampos current_pos = file.tellg();
    file.seekg(0, std::ios::end);
    streampos file_size = file.tellg();
    file.seekg(current_pos, std::ios::beg);
    return static_cast<size_t>(file_size);  // Usando size_t
}


void listFilesInDirectory(const string& directoryPath){
    // Validar que el directorio si exista
    if(!filesystem::exists(directoryPath) || !filesystem::is_directory(directoryPath)){
        cerr << "El directorio no existe o no es un directorio valida.\n";
        return;
    }

    for (const auto& entry : filesystem::directory_iterator(directoryPath)) {
        if (filesystem::is_regular_file(entry.status())) {
            //std::cout << "Archivo: " << entry.path() << " (Tamaño: " << filesystem::file_size(entry.path()) << " bytes)\n";
            ifstream file(entry.path());
            if (file.is_open()) {
                pair<long long, long long> two_hash = hashes(file);
                long long hash1 = two_hash.first;
                long long hash2 = two_hash.second;
                size_t size = filesystem::file_size(entry.path());
                string fileName = entry.path().filename().string();
                string filePath = entry.path().string();
                FileInfo fileInfo(hash1, hash2, size);
                files[fileInfo] = fileName;
                name_fullpath[fileName] = filePath;
            }
            else
            {
                cerr << "No se pudo abrir el archivo: " << entry.path() << std::endl;
            }
        }
        else if (filesystem::is_directory(entry.status())) {
            //cout << "Directorio: " << entry.path() << '\n';
            listFilesInDirectory(entry.path().string());
        }
    }
}


void print_files_saved(){
    for (auto &pair: files){
        pair.first.display();
        cout << pair.second << "\n\n";
    }
}
int connect_servers(int &sock, ip_port myself, ip_port to_connect) {
    const char* to_connect_ip = to_connect.ip.c_str();
    int to_connect_port = to_connect.port;

    const char* myself_ip = myself.ip.c_str();
    int myself_port = myself.port;

    struct sockaddr_in to_connect_addr, myself_addr;

    // Crear el socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error al crear el socket.\n";
        return 0;
    }

    myself_addr.sin_family = AF_INET;
    myself_addr.sin_addr.s_addr = inet_addr(myself_ip);
    myself_addr.sin_port = htons(myself_port);
    
    if (bind(sock, (struct sockaddr*)&myself_addr, sizeof(myself_addr)) < 0) {
        cerr << "Error al hacer bind en el cliente" << endl;
        close(sock);
        return 0;
    }

    to_connect_addr.sin_family = AF_INET;
    to_connect_addr.sin_port = htons(to_connect_port);
    
    if (inet_pton(AF_INET, to_connect_ip, &to_connect_addr.sin_addr) <= 0) {
        cerr << "Dirección del servidor no válida" << endl;
        close(sock);
        return 0;
    }

    if (connect(sock, (struct sockaddr*)&to_connect_addr, sizeof(to_connect_addr)) < 0) {
        cerr << "Error al conectar con el servidor" << endl;
        close(sock);
        return 0;
    }
    return 1;
}

void talkToClient(ip_port myself, ip_port new_client){

}

void talkToServer(int sock, ip_port info_cliente) {

    int cant_archivos = files.size();
    // Convertir el número a network byte order y enviarlo al servidor
    if(!sendIntThroughRed(sock, cant_archivos)){
        cout << "Error enviando cantidad de archivos iniciales al servidor\n";
        return;
    }

    // Serializar y enviar los datos de cada archivo
    for (const auto& [fileInfo, fileName] : files) {
        if (!sendFileInfoWithNameThroughRed(sock,fileInfo,fileName)){
            cerr << "Error enviado por red el archivo: " << fileName << endl;
            break;
        }
    }

    while (true){
        //Recibir nombre del archivo que se quiere pedir
        cout << "Si quiere buscar un archivo. Escriba find <nombre_archivo>\n";
        cout << "Si quiere pedir un archivo. Escriba request <size> <hash1> <hash2>\n";

        string inp_line;
        getline(cin, inp_line);
        istringstream inp_stream (inp_line);

        //consumir el primer token
        string instr = getFirstTokenIstring(inp_stream);

        if (instr == "request"){
            //Enviar instruccion
            if (!sendStringThroughRed(sock,instr)){
                cerr << "Error enviando el string "<< instr << " al servidor\n";
                break;
            }

            //Formar estructura
            long long sel_size, sel_h1, sel_h2;
            inp_stream >> sel_size >> sel_h1 >> sel_h2;
            FileInfo sel_fi (sel_h1,sel_h2, sel_size);

            //Enviar estructura
            if (!sendFileInfoThroughRed(sock,sel_fi)){
                cerr << "Error enviando S H H elegido\n";
                break;
            }
            //recibir cantidad_ips
            int num_ips;
            if (!receiveIntThroughRed(sock,num_ips)){
                cerr << "Error recibiendo la cantidad de ips\n";
                break;
            }
            if (num_ips == 0){
                cout << "No hay clientes que tengan el archivo buscado\n";
                continue;
            }
            string new_file_name;
            cout << "Introduzca el nombre del nuevo archivo.\n";
            getline(cin,new_file_name);
            cout << "El nombre del nuevo archivo sera:<" << new_file_name << ">\n";

            //recibir ips
            vector<ip_port> ips;
            for (int i=0; i<num_ips; i++){
                ip_port cur_ip;
                if(!receiveIpThroughRed(sock,cur_ip)){
                    cerr << "Error recibiendo la ip " << i+1 << endl;
                    break;
                }
                ips.push_back(cur_ip);
            }
            vector<thread> vthreads;
            vector<int> sockets;

            for (auto &cur_ip:ips){
                cout << "generaria conexion con " << cur_ip.ip << ':' << cur_ip.port << endl;
                int n_sock;
                if (connect_servers(n_sock, info_cliente, cur_ip)){
                    cerr << "Error conectando con el cliente " << cur_ip.ip << endl;
                    break;
                }
                //preguntarle a este servidor si realmente tiene el archivo con ese s s h
                if (!sendFileInfoThroughRed(n_sock, sel_fi)){
                    cerr << "Error enviando file info\n";
                    break;
                }
                int hasit;
                if (!receiveIntThroughRed(n_sock, hasit)){
                    cerr << "Error recibiendo confirmacion de si el cliente tiene ese archivo buscado\n";
                    break;
                }
                if (hasit){
                    cout << "lo tiene yei\n";
                    sockets.push_back(n_sock);
                }
                else{
                    close(n_sock);
                }
            }

            //decir que no logro guardar el archivo
            if (!sendIntThroughRed(sock,0)){
                cerr <<" xd\n";
                break;
            }
            
            /* Procesos para recibir ips que tienen el archivo*/

            //- C1 open conection with c2,c3.. clients
            //- c2,c3,c4 clients receives connection
            //- Ask c2,c3,c4 if they have the file
            //- c2,c3,c4 tells c1 if the have it or dont
            //- then for each that have, c1 says from what byte to other byte to receive
            //- c2,c3,c4 receveis it, sends it in chunks of 1024 bytes
            //- c1 receives all of the bytes
            //- c1 appends them
            //- c1 writes new file
            //- c1 adds it in his local memory
            //- c1 tells server he has that file now
            
        }
        else if(instr == "find"){
            //Enviar instruccion
            cout << "Enviando instruccion: " << instr << endl;

            if(!sendStringThroughRed(sock, instr)){
                cerr << "Error enviando el string "<< instr << " al servidor\n";
                break;
            }

            //Recibir el nombre del substring seleccionado
            string sel_file = trim(inp_stream.str());

            cout <<"Enviado el string:<" << sel_file << ">\n";
            //Mandar el substring seleccionado
            if (!sendStringThroughRed(sock,sel_file)){
                cerr << "Error enviando el string: " << sel_file << " al servidor\n";
            }

            cout <<"Esperando cantidad de archivos\n";

            //Recibir la cantidad de archivos que contienen el substring
            int num_files_found;
            if (!receiveIntThroughRed(sock,num_files_found)){
                cerr << "Error al recibir la cantidad de archivos con el nombre pedido\n";
                return;
            }
            if (num_files_found == 0){
                cout << "No se encontraron archivos con ese nombre\n";
                continue;
            }
            cout << "num_files_found different from zero: " << num_files_found << endl;


            //Recibir todos los FileInfo
            
            cout << "\n\nImprimiendo información de los archivos encontrados...\n";
            cout << "------------------------------------------------------\n";
            // Títulos de las columnas con alineación
            cout << setw(10) << right << "Size" << "   |   "
                    << setw(15) << right << "Hash1" << "   |   "
                    << setw(15) << right << "Hash2" << endl;

            // Línea de separación
            cout << "------------------------------------------------------\n";

            for (int i = 0; i < num_files_found; i++) {
                FileInfo fileInfoReceived;
                if (!receiveFileInfoThroughRed(sock, fileInfoReceived)) {
                    std::cerr << "Error al recibir el listado de archivos\n";
                    continue;
                }

                cout << setw(10) << right << fileInfoReceived.size << "   |   "
                        << setw(15) << right << fileInfoReceived.hash1 << "   |   "
                        << setw(15) << right << fileInfoReceived.hash2 << endl;
            }
            cout << "______________________________________________________\n";

        }
        else if (instr==";wq") break;
        else cout << "Ingrese un comando valido.\n";

    }

    close(sock);
}

void answerClient(int sock, ip_port sec_client_info){
    FileInfo fileinfo;
    if (!receiveFileInfoThroughRed(sock, fileinfo)){
        cerr << "Error recibiendo fileinfo\n";
        return;
    }
    mutexito.lock();
    string filename = files[fileinfo];
    string filepath = name_fullpath[filename];
    ifstream file(filepath);
    if (!file){
        //Comunicar que no tengo el archivo
        if (!sendIntThroughRed(sock,0)){
            cerr << "Error comunicando que no tengo el archivo\n";
            return;
        }
        close(sock);
        return;
    }
    //Comunicar que si lo tengo
    if (!sendIntThroughRed(sock,1)){
        cerr << "Error comunicando que si tengo el archivo\n";
        return;
    }

    close(sock);
    return;
}

void listeningNewClients(ip_port info_cliente){
    const char* client_ip = info_cliente.ip.c_str();
    int client_port = info_cliente.port;

    struct sockaddr_in serv_addr, client_addr;

    // Crear el socket
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        cerr << "Error al crear el socket.\n";
        return;
    }

    // Configurar la dirección del servidor
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(client_ip);
    client_addr.sin_port = htons(client_port);
    
    // Asignar la IP y el puerto del cliente al socket
    if (bind(listen_sock, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        std::cerr << "Error al hacer bind en el cliente" << std::endl;
        close(listen_sock);
        return;
    }
    if (listen(listen_sock, 10) < 0) {
        cerr << "Error al escuchar" << endl;
        close(listen_sock);
        return;
    }
    
    cout << "Cliente escuchando en " << info_cliente.ip << ":" << info_cliente.port << "..." << endl;

    while (true) {
        int n_client_socket;
        struct sockaddr_in n_client_address;
        socklen_t n_client_addrlen = sizeof(n_client_address);

        // Aceptar una nueva conexión de cliente
        if ((n_client_socket = accept(listen_sock, (struct sockaddr*)&n_client_address, &n_client_addrlen)) < 0) {
            cerr << "Error al aceptar conexión" << endl;
            continue;
        }

        // Obtener la dirección IP y puerto del cliente
        char n_client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &n_client_address.sin_addr, n_client_ip, INET_ADDRSTRLEN);
        int n_client_port = ntohs(n_client_address.sin_port);

        cout << "Nueva conexión de " << n_client_ip << ":" << n_client_port << std::endl;

        // Pasar IP y puerto del cliente a `start_server`
        ip_port n_client_info = {n_client_ip, n_client_port};

        // Crear un hilo para cada cliente usando `start_server`
        thread client_thread(answerClient, n_client_socket, n_client_info);
        client_thread.detach();
    }
}

void start(ip_port ip_client, ip_port ip_server){
    string ip = ip_client.ip;
    int port = ip_client.port;
    int cant_clientes = 2;
    int sock_listen;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Crear el socket de escucha
    if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Error al crear el socket" << std::endl;
        return;
    }

    // Configurar la dirección del socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str());
    address.sin_port = htons(port);
    if (bind(sock_listen, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error al hacer bind" << std::endl;
        close(sock_listen);
        return;
    }
    if (listen(sock_listen, 10) < 0) {
        std::cerr << "Error al escuchar" << std::endl;
        close(sock_listen);
        return;
    }

    cout << "Cliente escuchando en " << ip << ":" << port << "..." << endl;

    // Conexión al servidor
    int server_socket;
    struct sockaddr_in server_address;
    
    // Crear el socket para conectarse con el servidor
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Error al crear el socket para el servidor" << std::endl;
        close(sock_listen);
        return;
    }

    // Configurar la dirección del servidor
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(ip_server.port);
    if (inet_pton(AF_INET, ip_server.ip.c_str(), &server_address.sin_addr) <= 0) {
        std::cerr << "Dirección IP no válida o no soportada" << std::endl;
        close(server_socket);
        close(sock_listen);
        return;
    }

    // Conectar al servidor
    if (connect(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Error al conectar con el servidor" << std::endl;
        close(server_socket);
        close(sock_listen);
        return;
    }

    cout << "Conectado al servidor " << ip_server.ip << ":" << ip_server.port << endl;

    // Crear un hilo para manejar la comunicación con el servidor
    thread server_thread(talkToServer, server_socket, ip_client);
    server_thread.detach();

    while (true) {
        int client_socket;
        struct sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);

        // Aceptar una nueva conexión de cliente
        if ((client_socket = accept(sock_listen, (struct sockaddr*)&client_address, &client_addrlen)) < 0) {
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
        std::thread client_thread(answerClient, client_socket, client_info);
        client_thread.detach();
    }

    // Cerrar el socket de escucha cuando se termina
    close(sock_listen);
}
void start( ip_port client_info, ip_port server_info, string ruta ){
    files = map<FileInfo,string> ();
    listFilesInDirectory(ruta);
    print_files_saved();

    // Crear un hilo para conectar al servidor
    start(client_info, server_info);
}


int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Uso: " << argv[0] << " server_ip:puerto cliente_ip:puerto \"ruta con espacios\"\n";
        return 1;
    }

    ip_port info_server;
    ip_port info_cliente;

    if (!parseIpPort(argv[1], info_server)) {
        cerr << "Formato inválido para server_ip:puerto. Use ip:puerto\n";
        return 1;
    }

    if (!parseIpPort(argv[2], info_cliente)) {
        cerr << "Formato inválido para cliente_ip:puerto. Use ip:puerto\n";
        return 1;
    }

    // Concatenar todos los argumentos restantes en una única cadena para la ruta
    string ruta;
    for (int i = 3; i < argc; ++i) {
        if (i > 3) ruta += " "; // Añade espacio entre palabras de la ruta
        ruta += argv[i];
    }

    std::cout << "Server IP: " << info_server.ip << ", Puerto: " << info_server.port << "\n";
    std::cout << "Cliente IP: " << info_cliente.ip << ", Puerto: " << info_cliente.port << "\n";
    std::cout << "Ruta: " << ruta << "\n";

    start(info_cliente, info_server, ruta);

    return 0;
}
//server  - 127.0.0.1:8080
//cliente - 127.0.0.2:8082
//ruta    - /home/santy/Documentos/Sistemas Operativos/Simulador-P2P/carpetas_prueba/carpeta_1
// ./cliente 127.0.0.1:8080 127.0.0.2:8082 /home/santy/Documentos/Sistemas Operativos/Simulador-P2P/carpetas_prueba/carpeta_1
// ./cliente 127.0.0.1:8080 127.0.0.2:8083 /home/santy/Documentos/Sistemas Operativos/Simulador-P2P/carpetas_prueba/carpeta_3


/*
    - TO DO LISTconnection
        - Asks the name of the new file
        - Server sends ip ports to client
        - C1 open conection with c2,c3.. clients
        - c2,c3,c4 clients receives connection
        - Ask c2,c3,c4 if they have the file
        - c2,c3,c4 tells c1 if the have it or dont
        - then for each that have, c1 says from what byte to other byte to receive
        - c2,c3,c4 receveis it, sends it in chunks of 1024 bytes
        - c1 receives all of the bytes
        - c1 appends them
        - c1 writes new file
        - c1 adds it in his local memory
        - c1 tells server he has that file now

*/