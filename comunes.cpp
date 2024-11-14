#include <iostream>
#include "comunes.hpp"


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