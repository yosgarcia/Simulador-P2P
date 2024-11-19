#include <iostream>
#include "comunes.hpp"


std::string ip_port_to_str(ip_port info){
    std::string ans = info.ip;
    ans+=":";
    ans+=std::to_string(info.port);
    return ans;

}
bool parseIpPort(const std::string& str, ip_port& result) {
    size_t colonPos = str.find(':');
    if (colonPos == std::string::npos) return false;
    
    result.ip = str.substr(0, colonPos);
    try {
        result.port = std::stoi(str.substr(colonPos + 1));
    } catch (...) {
        return false;
    }
    return true;
}

// Eliminar espacios en blanco de la izquierda
std::string ltrim(const std::string& str) {
    std::string s = str;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    return s;
}
// Eliminar espacios en blanco de la derecha
std::string rtrim(const std::string& str) {
    std::string s = str;
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}

// Eliminar espacios en blanco de ambos lados
std::string trim(const std::string& str) {
    return ltrim(rtrim(str));
}

std::string getFirstTokenIstring(std::istringstream &stream) {
    std::string value;
    stream >> value;  

    std::string remaining;
    getline(stream, remaining);  // Leer el resto del flujo después del primer token

    // Actualizar el contenido del flujo original con el resto
    stream.str(remaining);  // Asignar el nuevo contenido
    stream.clear();          // Reiniciar el estado del flujo para nueva lectura

    return value;
}

bool sendCharThroughRed(int sock, char value) {
    // Enviar valor
    if (send(sock, &value, sizeof(value), 0) == -1) {
        std::cerr << "Error al enviar el char: " << value << std::endl;
        return false;
    }

    // Recibir confirmación
    char confirmacion[3];
    int confirmacion_recibida = recv(sock, confirmacion, sizeof(confirmacion), 0);
    if (confirmacion_recibida <= 0) {
        std::cerr << "Error recibiendo confirmación\n";
        return false;
    }

    return true;
}

bool receiveCharThroughRed(int sock, char& value) {
    // Recibir valor
    int valread = recv(sock, &value, sizeof(value), 0);
    if (valread <= 0) {
        std::cerr << "Error al recibir char por red\n";
        return false;
    }

    // Enviar confirmación
    send(sock, "OK", 2, 0);
    return true;
}


bool sendIntThroughRed(int sock, int value){
    //Enviar valor
    int net_value = htonl(value);
    if (send(sock,&net_value, sizeof(value), 0) == -1){
        std::cerr << "Error al enviar el entero: " << value << std::endl;
        return false;
    }
    //Recibir confirmacion
    char confirmacion[3];
    int confirmacion_recibida = recv(sock, confirmacion, sizeof(confirmacion), 0);
    return true;
}

bool receiveIntThroughRed(int sock, int& value){
    //Recibir valor
    int data_size_network_order;
    int valread = recv(sock, &data_size_network_order, sizeof(data_size_network_order), 0);
    if (valread <= 0) {
            std::cerr << "Error al recibir entero por red\n";
            return false;
        }
    value = ntohl(data_size_network_order);

    //Enviar confirmacion
    send(sock, "OK", 2, 0);

    return true;

}
bool sendStringThroughRed(int sock, std::string value){
    //Enviar tamanno del string
    int data_size = value.size();
    if (!sendIntThroughRed(sock,data_size)){
        return false;
    }
    //Enviar el string
    send(sock, value.c_str(), value.size(), 0);
    
    //Recibir confirmacion
    char confirmacion[3];
    int confirmacion_recibida = recv(sock, confirmacion, sizeof(confirmacion), 0);
    return true;
}


bool receiveStringThroughRed(int sock, std::string &value){
    int data_size;
    if (!receiveIntThroughRed(sock,data_size)){
        return false;
    }

    char buffer[1024];
    int bytes_received = 0;

    while (bytes_received < data_size) {
        int chunk_size = recv(sock, buffer, sizeof(buffer), 0);
        if (chunk_size <= 0) {
            std::cerr << "Error al recibir los datos del archivo" << std::endl;
            return false;
        }
        bytes_received += chunk_size;
        value.append(buffer, chunk_size);
    }

    //Enviar confirmacion
    send(sock, "OK", 2, 0);

    return true;
}

bool sendFileInfoThroughRed(int sock, FileInfo fileInfo){
    std::ostringstream dataStream;
    dataStream << fileInfo.hash1 << " " << fileInfo.hash2 << " " << fileInfo.size << "\n";
    std::string data = dataStream.str();

    if (!sendStringThroughRed(sock, data)){
        std::cerr << "Error al enviar por red el archivo  con hash" << fileInfo.hash1<< std::endl;
        return false;
    }

    return true;
}

bool receiveFileInfoThroughRed(int sock, FileInfo &fileInfo){
    std::string file_data;
    if (!receiveStringThroughRed(sock,file_data)){
        std::cerr << "Error recibiendo informacion de un archivo\n";
        return false;
    }

    // Procesar datos
    std::istringstream dataStream(file_data);
    long long hash1, hash2;
    size_t size;
        
    dataStream >> hash1 >> hash2 >> size;
    fileInfo = FileInfo (hash1,hash2, size);
    return true;
}

bool sendFileInfoWithNameThroughRed(int sock, FileInfo fileInfo, std::string fileName){
    std::ostringstream dataStream;
    dataStream << fileInfo.hash1 << " " << fileInfo.hash2 << " " << fileInfo.size << " " << fileName << "\n";
    std::string data = dataStream.str();

    if (!sendStringThroughRed(sock, data)){
        std::cerr << "Error al enviar por red el archivo " << fileName<< std::endl;
        return false;
    }

    return true;
}

bool receiveFileInfoWithNameThroughRed(int sock, FileInfo &fileInfo, std::string &fileName){
    std::string file_data;
    if (!receiveStringThroughRed(sock,file_data)){
        std::cerr << "Error recibiendo informacion de un archivo\n";
        return false;
    }

    // Procesar datos
    std::istringstream dataStream(file_data);
    long long hash1, hash2;
    size_t size;
        
    dataStream >> hash1 >> hash2 >> size;
    std::getline(dataStream, fileName);
    if (!fileName.empty() && fileName[0] == ' ') {
        fileName.erase(0, 1);
    }

    fileInfo = FileInfo (hash1,hash2, size);
    return true;
}

bool receiveIpThroughRed(int sock,ip_port &ip){
    std::string ip_string;
    if (!receiveStringThroughRed(sock, ip_string)){
        std::cerr << "Error recibiendo la ip\n";
        return false;
    }
    int port;
    if (!receiveIntThroughRed(sock,port)){
        std:: cerr << "Error recibiendo el puerto\n";
        return false;
    }
    ip.ip = ip_string;
    ip.port = port;
    return true;
}

bool sendIpThroughRed(int sock, ip_port ip){
    std::string ip_string = ip.ip;
    if (!sendStringThroughRed(sock,ip_string)){
        std::cerr << "Error enviando la ip " << ip.ip << std::endl;
        return false;
    }
    if (!sendIntThroughRed(sock,ip.port)){
        std::cerr << "Error enviando el puerto " << ip.port << std::endl;
        return false;
    }
    return true;

}

bool send_pairFi_Ip_ThroughRed(int sock, pairfi_ip p_fi_ip){
    if (!sendFileInfoThroughRed(sock,p_fi_ip.first)){
        return false;
    }
    if (!sendIpThroughRed(sock,p_fi_ip.second)){
        return false;
    }
    return true;
}
bool receive_pairFi_Ip_ThroughRed(int sock, pairfi_ip &p_fi_ip){
    if (!receiveFileInfoThroughRed(sock,p_fi_ip.first)){
        return false;
    }
    if (!receiveIpThroughRed(sock,p_fi_ip.second)){
        return false;
    }
    return true;
}

bool sendBytesThroughRed(int sock, std::vector<char> &data) {
    // Enviar la cantidad de bytes (en formato de red)
    int size = data.size();
    if (!sendIntThroughRed(sock, size)) {
        std::cerr << "Error enviando el tamaño de los bytes\n";
        return false;
    }

    for (int i=0; i<size; i++){
        if (!sendCharThroughRed(sock,data[i])){
            std::cerr << "Error enviando char\n";
            return false;
        }
    }
    return true;
}


 
bool receiveBytesThroughRed(int sock, std::vector<char> &buffer) {
    // Recibir el tamaño de los bytes (en formato de red)
    int data_size;
    if (!receiveIntThroughRed(sock, data_size)) {
        std::cerr << "Error recibiendo el tamaño de los bytes\n";
        return false;
    }
    for (int i=0 ;i< data_size;i++){
        char c;
        if (!receiveCharThroughRed(sock,c)){
            std::cerr << "Error recibiendo un byte\n";
            return false;
        }
        buffer.push_back(c);
    }

    return true;
}