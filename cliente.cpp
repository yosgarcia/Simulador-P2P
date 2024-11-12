#include <bits/stdc++.h>
#include "comunes.hpp"

using namespace std;

// Info y la direccion del archivo
map<FileInfo,string> files;

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
                FileInfo fileInfo(hash1, hash2, size);
                files[fileInfo] = entry.path().string();
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


void start(){
    files = map<FileInfo,string> ();
    listFilesInDirectory("/home/santy/Documentos/Sistemas Operativos/Simulador-P2P/carpetas_prueba/carpeta_1");
    print_files_saved();
}
int main(){
    start();

    return 0;
}