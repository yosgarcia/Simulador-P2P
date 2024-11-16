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
bool parseIpPort(const std::string& str, ip_port& result);
std::string getFirstTokenIstring(std::istringstream &stream);
std::string ltrim(const std::string& str);
std::string rtrim(const std::string& str);
std::string trim(const std::string& str);
bool sendIntThroughRed(int sock, int value);
bool receiveIntThroughRed(int sock, int& value);
bool sendStringThroughRed(int sock, std::string value);
bool receiveStringThroughRed(int sock, std::string &value);
bool sendFileInfoThroughRed(int sock, FileInfo fileInfo);
bool receiveFileInfoThroughRed(int sock, FileInfo &fileInfo);
bool sendFileInfoWithNameThroughRed(int sock, FileInfo fileInfo, std::string fileName);
bool receiveFileInfoWithNameThroughRed(int sock, FileInfo &fileInfo, std::string &fileName);
bool receiveIpThroughRed(int sock, ip_port &ip);
bool sendIpThroughRed(int sock, ip_port ip);

#endif
