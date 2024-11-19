# Simulador-P2P
El proyecto consiste en hacer una simulación de un sistema P2P que se comunique a través de sockets para poder encontrar diferentes archivos en una red. Para ello se va a apoyar en una tabla hash que debe ser realizada por los mismos miembros del grupo que permita valorar si dos archivos son el mismo, aún si a alguno se le ha cambiado el nombre.

## Compilar
g++ comunes.cpp servidor.cpp -o servidor
g++ comunes.cpp cliente.cpp -o cliente

## Ejecutar (usar ip real)
./servidor 127.0.0.1:8080

./cliente 127.0.0.1:8080 127.0.0.11:8081 /home/santy/Documentos/Sistemas Operativos/Simulador-P2P/carpetas_prueba/carpeta_1


### Santiago Ramos
### Yosward Garcia