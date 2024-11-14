#include <bits/stdc++.h>
#include <thread>
#include <cstring>      // Para memset
#include <arpa/inet.h>  // Para sockaddr_in y htons
#include <unistd.h>     // Para close
#include "comunes.hpp"

using namespace std;

// Info y la direccion del archivo
map<FileInfo,string> files;
map<string,string> name_fullpath;

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

void connectToServer(ip_port info_cliente, ip_port info_server) {
    const char* server_ip = info_server.ip.c_str();
    int server_port = info_server.port;

    const char* client_ip = info_cliente.ip.c_str();
    int client_port = info_cliente.port;

    struct sockaddr_in serv_addr, client_addr;

    // Crear el socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error al crear el socket.\n";
        return;
    }

    // Configurar la dirección del servidor
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(client_ip);
    client_addr.sin_port = htons(client_port);
    
    // Asignar la IP y el puerto del cliente al socket
    if (bind(sock, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        std::cerr << "Error al hacer bind en el cliente" << std::endl;
        close(sock);
        return;
    }

    // Configurar la dirección del servidor
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    
    // Convertir la dirección IP del servidor
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Dirección del servidor no válida" << std::endl;
        close(sock);
        return;
    }


    // Conectar al servidor
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error al conectar con el servidor" << std::endl;
        close(sock);
        return;
    }

    int cant_archivos = files.size();
    // Convertir el número a network byte order y enviarlo al servidor
    if(!sendIntThroughRed(sock, cant_archivos)){
        cout << "Error enviando cantidad de archivos iniciales al servidor\n";
        return;
    }

    cout << "Entero enviado al servidor: " << cant_archivos << std::endl;

    // Serializar y enviar los datos de cada archivo
    for (const auto& [fileInfo, fileName] : files) {
        if (!sendFileInfoWithNameThroughRed(sock,fileInfo,fileName)){
            cerr << "Error enviado por red el archivo: " << fileName << endl;
            break;
        }
    }

    cout << "Archivos enviados al servidor\n";
    while (true){
        //Recibir nombre del archivo que se quiere pedir
        
        cout << "Introduzca el nombre del archivo que quiere obtener. Presione Enter en una línea vacía para terminar:\nFind ";
        string file_name;
        getline(cin, file_name);

        int data_size = file_name.size();
        if (data_size < 1){
            cout << "Introduzca un nombre valido.\n";
            continue;
        }

        //Enviar el nombre del archivo
        if (!sendStringThroughRed(sock,file_name)){
            cout << "xd\n";
            cerr << "Error enviando el nombre de archivo por red\n";
            break;
        }
        /*
        int number_files_to_choose;
        if (!receiveIntThroughRed(sock,number_files_to_choose)){
            cerr << "Error al recibir la cantidad de archivos con el nombre pedido\n";
            return;
        }
        */
       cout <<"haha\n";
    }
    close(sock);
}

void start( ip_port client_info, ip_port server_info, string ruta ){
    files = map<FileInfo,string> ();
    listFilesInDirectory(ruta);
    print_files_saved();

    // Crear un hilo para conectar al servidor
    thread serverThread(connectToServer, client_info, server_info);
    serverThread.join();

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
    - TO DO LIST
        - Buscar archivo
        - Seleccionar el archivo a enviar
        - Comunicar cliente con cliente
        - Dividir el archivo en secciones
        - Enviar partes de archivo
        - Unir pedazis ee archivos

*/