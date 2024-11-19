#ifndef COMUNES_HPP
#define COMUNES_HPP

#include <string>
#include <tuple>
#include <iostream>
#include <arpa/inet.h> // Para htonl y funciones de red
#include <sys/socket.h> // Para send
#include <sstream> // Para ostringstream
#include <cctype>
#include <algorithm>


typedef struct ip_port{
    std::string ip;
    int port;

    // Sobrecargar el operador == para comparar ip y port
    bool operator==(const ip_port& other) const {
        return ip == other.ip && port == other.port;
    }

    // Sobrecargar el operador < para comparar ip y port
    bool operator<(const ip_port& other) const {
        // Primero comparamos las IPs
        if (ip != other.ip) {
            return ip < other.ip;  // Orden lexicográfico de las IPs
        }
        // Si las IPs son iguales, comparamos los puertos
        return port < other.port;
    }

} ip_port;

class FileInfo {
public:
    long long hash1;
    long long hash2;
    size_t size;

    // Constructor para inicializar
    FileInfo(long long h1 = 0, long long h2 = 0, size_t s = 0)
        : hash1(h1), hash2(h2), size(s) {}

    // Método para mostrar los datos de FileInfo
    void display() const {
        std::cout << "Hash1: " << hash1 << ", Hash2: " << hash2 << ", Size: " << size << " bytes" << std::endl;
    }

    // Comparar FileInfo usando std::tie
    // para usar map
    bool operator<(const FileInfo& other) const {
        return std::tie(hash1, hash2, size) < std::tie(other.hash1, other.hash2, other.size);
    }
    bool operator==(const FileInfo& other) const {
        return std::tie(hash1, hash2, size) == std::tie(other.hash1, other.hash2, other.size);
    }
};

typedef std::pair<FileInfo,ip_port> pairfi_ip;

std::string ip_port_to_str(ip_port info);
bool parseIpPort(const std::string& str, ip_port& result);
std::string getFirstTokenIstring(std::istringstream &stream);
std::string ltrim(const std::string& str);
std::string rtrim(const std::string& str);
std::string trim(const std::string& str);

bool sendCharThroughRed(int sock, char value);
bool receiveCharThroughRed(int sock, char& value);
bool sendIntThroughRed(int sock, int value);
bool receiveIntThroughRed(int sock, int& value);
bool sendStringThroughRed(int sock, std::string value);
bool receiveStringThroughRed(int sock, std::string &value);
bool sendFileInfoThroughRed(int sock, FileInfo fileInfo);
bool receiveFileInfoThroughRed(int sock, FileInfo &fileInfo);
bool sendFileInfoWithNameThroughRed(int sock, FileInfo fileInfo, std::string fileName);
bool receiveFileInfoWithNameThroughRed(int sock, FileInfo &fileInfo, std::string &fileName);
bool sendIpThroughRed(int sock, ip_port ip);
bool receiveIpThroughRed(int sock, ip_port &ip);
bool send_pairFi_Ip_ThroughRed(int sock, pairfi_ip p_fi_ip);
bool receive_pairFi_Ip_ThroughRed(int sock, pairfi_ip &p_fi_ip);
bool sendBytesThroughRed(int sock, std::vector<char> &data);
bool receiveBytesThroughRed(int sock, std::vector<char> &buffer);

#endif
