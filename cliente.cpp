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
string path_folder;
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



size_t getFileSize(ifstream &file) {
    streampos current_pos = file.tellg();
    file.seekg(0, ios::end);
    streampos file_size = file.tellg();
    file.seekg(current_pos, ios::beg);
    return static_cast<size_t>(file_size);  // Usando size_t
}

void printLine(){
    cout << "__________________________________________________________________________________________\n";
}

void printHeadline(){
    printf("Imprimiendo informacion de los archivos encontrados.\n");
    printLine();
    cout << setw(10) << right << "Size" << "   |   " << setw(15) << right << "Hash1" << "   |   "
        << setw(15) << right << "Hash2" << "   |   " << setw(15) << right << "IP" << "   |   "
        << setw(6) << right << "Port" << endl;
    printLine();
}

void printDataReceived(pairfi_ip& data){
    FileInfo filedata = data.first;
    ip_port ipdata = data.second;

    cout << setw(10) << right << filedata.size << "   |   "
    << setw(15) << right << filedata.hash1 << "   |   "
    << setw(15) << right << filedata.hash2 << "   |   "
    << setw(15) << right << ipdata.ip << "   |   "
    << setw(6) << right << ipdata.port << endl;
}


void listFilesInDirectory(const string& directoryPath){
    // Validar que el directorio si exista
    if(!filesystem::exists(directoryPath) || !filesystem::is_directory(directoryPath)){
        cerr << "El directorio no existe o no es un directorio valida.\n";
        return;
    }

    for (const auto& entry : filesystem::directory_iterator(directoryPath)) {
        if (filesystem::is_regular_file(entry.status())) {
            //cout << "Archivo: " << entry.path() << " (Tamaño: " << filesystem::file_size(entry.path()) << " bytes)\n";
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
                cerr << "No se pudo abrir el archivo: " << entry.path() << endl;
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


int connect_to(ip_port target) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        cerr << "Error creating socket for " << target.ip << ":" << target.port << "\n";
        return -1;
    }

    struct sockaddr_in target_address;
    target_address.sin_family = AF_INET;
    target_address.sin_port = htons(target.port);
    inet_pton(AF_INET, target.ip.c_str(), &target_address.sin_addr);

    if (connect(sock_fd, (struct sockaddr*)&target_address, sizeof(target_address)) < 0) {
        cerr << "Error connecting to " << target.ip << ":" << target.port << "\n";
        close(sock_fd);
        return -1;
    }

    //cout << "Connected to " << target.ip << ":" << target.port << "\n";
    return sock_fd;
}


void answerClient(int sock, ip_port sec_client_info){
    FileInfo fileinfo;
    string mensajito;

    if (!receiveFileInfoThroughRed(sock, fileinfo)){
        cerr << "Error recibiendo fileinfo\n";
        return;
    }
    mutexito.lock();
    string filename = files[fileinfo];
    string filepath = name_fullpath[filename];
    ifstream file(filepath, ios::binary); //solo lectura en binario
    mutexito.unlock();

    //Comunicar que no tengo el archivo
    if (!file){
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

    int start, end;
    if (!receiveIntThroughRed(sock, start)){
        cerr << "Error recibiendo puntero a primera parte archivo\n";
        return;
    }
    if (!receiveIntThroughRed(sock,end)){
        cerr << "Error recibiendo puntero hacia segunda parte archivo\n";
        return;
    }

    file.seekg(start, ios::beg);

    // Calcular cuántos bytes necesitamos leer
    size_t bytes_to_read = end - start + 1;
    
    string buffer;
    buffer.resize(bytes_to_read);

    file.read(&buffer[0], bytes_to_read);

    if (file.fail()) {
        cerr << "Error leyendo los bytes del archivo\n";
        return;
    }

    // Enviar los bytes leídos al cliente
    if (!sendStringThroughRed(sock, buffer)) {
        cerr << "Error enviando los bytes al cliente\n";
        return;
    }

    close(sock);
    return;
}

void talkToClient(int sock, int start, int end, string &myBytes){
    if(!sendIntThroughRed(sock,start)){
        cerr << "Error indicando la primera parte del archivo\n";
        close(sock);
        return;
    }
    if (!sendIntThroughRed(sock, end)){
        cerr << "Error indicando la segunda parte del archivo\n";
        close(sock);
        return;
    }
    if(!receiveStringThroughRed(sock, myBytes)){
        cerr << "Error recibiendo los bytes del archivo\n";
        close(sock);
        return;
    }
    close (sock);
    return;
}


void writeFile(const string& filename, const vector<string>& bytes_portions) {
    string file_name_with_path = path_folder + "/" + filename;
    // Abrir el archivo en modo binario para escribir los bytes
    ofstream output_file(file_name_with_path, std::ios::binary);
    
    if (!output_file) {
        cerr << "Error al abrir el archivo para escritura." << endl;
        return;
    }

    // Escribir todas las porciones de bytes en el archivo, en el orden
    for (const auto& portion : bytes_portions) {
        output_file.write(portion.data(), portion.size()); 
    }

    output_file.close(); // Cerrar el archivo después de escribir
    if (output_file) {
        cout << "Archivo escrito con éxito." << endl;
    } else {
        cerr << "Error al escribir el archivo." << endl;
    }
}


void talkToServer(int sock, ip_port info_cliente) {
    if (!sendIpThroughRed(sock,info_cliente)){
        cout << "Error enviando la ip al server\n";
        return;
    }

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
                //cout << "generaria conexion con " << cur_ip.ip << ':' << cur_ip.port << endl;
                int n_sock = connect_to(cur_ip);
                if (n_sock<0){
                    cerr << "Error al establecer la conexion\n";
                    break;
                }
                //conexion valida

                //preguntarle a este cliente si realmente tiene el archivo con ese s h h
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
                    sockets.push_back(n_sock);
                }
                else{
                    close(n_sock);
                }
            }
            int num_useful_clients = sockets.size();
            if (num_useful_clients == 0){
                cout << "No hay clientes actuales con ese archivo\n";
                if (!sendIntThroughRed(sock,0)){
                    cerr <<" Error indicando que no puede guardar el archivo\n";
                    break;
                }
                continue;
            }
            // 4 clientes lo tengan , y el archivo sea 2 bytes
            // 0 1 2 3 (el 2 y el 3 sobran, hay que cerrar esos sockets)
            bool more_clients_than_size = false;
            if (num_useful_clients > sel_size){
                //hay clientes que no hacen falta
                num_useful_clients = sel_size;
                // cerrar socket 2 y 3
                for (int i = sel_size; i < sockets.size(); i++){
                    close(sockets[i]);
                }

            }
            // 11 bytes, 3 clientes
            // portion = 3
            // i=0 0 - 2 = 3
            // i=1 3 - 5 = 3
            // i=2 6 - 10 = 5

            vector<string> bytes_portions (num_useful_clients, string ());
            if (sel_size > 0){
                int portion_per_client = sel_size/num_useful_clients;
                int remainder = sel_size % num_useful_clients;
                for (int i =0; i < num_useful_clients; i++){
                    int start = i * portion_per_client;
                    int end = start + portion_per_client - 1;

                    if (i == num_useful_clients - 1) end += remainder;

                    int sockito = sockets[i];
                    vthreads.push_back(thread(talkToClient, sockito,start,end, ref(bytes_portions[i])));
                }

                //esperar a que todos los hilos reciban su parte
                for (auto &t: vthreads){
                    if (t.joinable()) {  // Verificar si el hilo es "unido"
                        t.join();         // Esperar a que termine
                    }
                }
            }
            else{
                bytes_portions = {""};
            }

            writeFile(new_file_name, bytes_portions);

            mutexito.lock();
            files[sel_fi] = new_file_name;
            name_fullpath[new_file_name] = path_folder + "/" + new_file_name;
            mutexito.unlock();

            // Avisar al server que tengo este nuevo archivo
            if (!sendIntThroughRed(sock,1)){
                cerr <<" Error avisando al server que ya tengo el archivo nuevo\n";
                break;
            }
            // Enviar el nombre del nuevo archivo
            if (!sendStringThroughRed(sock,new_file_name)){
                cerr << " Error enviando el nombre de archivo" << new_file_name << endl;
                break;
            }

            // Avisar al server que tengo este nuevo archivo
            if(!sendFileInfoThroughRed(sock, sel_fi)){
                cerr << "Error enviando al server la info del archivo nuevo\n";
                break;
            }

            //Esperar a que el server se actualize
            int saved_on_sever;
            if (!receiveIntThroughRed(sock, saved_on_sever)){
                cerr << "Error recibiendo la confirmacion de guardado en el server\n";
                break;
            }
            
        }
        else if(instr == "find"){
            //Enviar instruccion

            if(!sendStringThroughRed(sock, instr)){
                cerr << "Error enviando el string "<< instr << " al servidor\n";
                break;
            }

            //Recibir el nombre del substring seleccionado
            string sel_file = trim(inp_stream.str());

            //Mandar el substring seleccionado
            if (!sendStringThroughRed(sock,sel_file)){
                cerr << "Error enviando el string: " << sel_file << " al servidor\n";
            }

            //Recibir la cantidad de archivos que contienen el substring
            int num_files_found;
            if (!receiveIntThroughRed(sock,num_files_found)){
                cerr << "Error al recibir la cantidad de archivos con el nombre pedido\n";
                return;
            }
            if (num_files_found == 0){
                cout << "No se encontraron archivos con esos metadatos\n";
                continue;
            }

            printHeadline();
            for (int i = 0; i < num_files_found; i++) {
                pairfi_ip data;
                if (!receive_pairFi_Ip_ThroughRed(sock, data)) {
                    cerr << "Error al recibir el listado de archivos\n";
                    continue;
                }
                printDataReceived(data);
            }
            printLine();

        }
        else if (instr==";wq") break;
        else cout << "Ingrese un comando valido.\n";

    }

    close(sock);
}



// Function for the client to connect to the server and listen for other clients
void start_client(ip_port server_info, ip_port client_info) {
    string server_ip = server_info.ip;
    int server_port = server_info.port;
    string client_ip = client_info.ip;
    int client_port = client_info.port;

    // Create socket for connecting to the server
    int client_fd; //file descriptor
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Error al crear socket para el servidor\n";
        return;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr);

    // Connect to the main server
    if (connect(client_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        cerr << "Error al conectar con el servidor\n";
        close(client_fd);
        return;
    }

    cout << "Conectado al servidor principal en " << server_ip << ":" << server_port << endl;

    // Create a thread to communicate with the main server
    thread server_thread(talkToServer,client_fd,client_info);
    server_thread.detach();

    // Set up a listening socket for other clients
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cerr << "Error al crear el socket para clientes\n";
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(client_ip.c_str());
    address.sin_port = htons(client_port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Error al hacer bind para escuchar clientes\n";
        close(server_fd);
        return;
    }

    if (listen(server_fd, 10) < 0) {
        cerr << "Error al escuchar para conexiones de clientes\n";
        close(server_fd);
        return;
    }

    cout << "Esperando conexiones de otros clientes en " << client_ip << ":" << client_port << "...\n\n";

    while (true) {
        int new_socket;
        struct sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);

        if ((new_socket = accept(server_fd, (struct sockaddr*)&client_address, &client_addrlen)) < 0) {
            cerr << "Error al aceptar conexión de otro cliente\n";
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_address.sin_port);

        cout << "Nueva conexión de cliente: " << client_ip << ":" << client_port << endl;

        // Create a thread to handle this client
        thread client_thread(answerClient, new_socket, client_info);
        client_thread.detach();
    }

    // Close the listening socket when done
    close(server_fd);
}

void start( ip_port client_info, ip_port server_info ){
    files = map<FileInfo,string> ();
    listFilesInDirectory(path_folder);
    start_client(server_info, client_info);
}


int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Uso: " << argv[0] << " server_ip:puerto cliente_ip:puerto \"ruta con espacios\"\n";
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
    path_folder = "";
    for (int i = 3; i < argc; ++i) {
        if (i > 3) path_folder += " "; // Añade espacio entre palabras de la ruta
        path_folder += argv[i];
    }

    cout << "Server IP: " << info_server.ip << ", Puerto: " << info_server.port << "\n";
    cout << "Cliente IP: " << info_cliente.ip << ", Puerto: " << info_cliente.port << "\n";
    cout << "Ruta: " << path_folder << "\n\n";

    start(info_cliente, info_server);

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