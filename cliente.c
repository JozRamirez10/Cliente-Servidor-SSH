#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUF_SIZE 8192

/*
    Estabece un cliente que se conectao a un servidor por TCP
    Envia comandos y muestra las respuestas recibidas
    Recibe una dirección IP y un puerto
*/
int main(int argc, char *argv[]) {
    int fd; // file descriptor del cliente
    struct sockaddr_in servidor; // Estructura que almacena la dirección del servidor
    char buffer[BUF_SIZE]; // Buffer que almacena los mensajes enviados y recibidos

    if (argc != 3) { // Verifica que el programa haya recibido una dirección IP y un puerto
        fprintf(stderr, "Uso: %s <IP del servidor> <puerto>\n", argv[0]);
        exit(1);
    }

    // Crear el socket
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { // Si falla la creación del socket
        perror("Error al crear el socket");
        exit(1);
    }

    // Configura la dirección del servidor
    memset(&servidor, 0, sizeof(servidor)); // Limpia la estructura del servidor
    servidor.sin_family = AF_INET; // IPv4
    servidor.sin_port = htons(atoi(argv[2])); // Convierte el número de puerto a formato de red
    if (inet_pton(AF_INET, argv[1], &servidor.sin_addr) <= 0) { // Convierte la dirección IP a binario
        perror("Dirección inválida");
        close(fd);
        exit(1);
    }

    // Conexión con el servidor
    if (connect(fd, (struct sockaddr *)&servidor, sizeof(servidor)) < 0) {
        perror("Error al conectar al servidor");
        close(fd);
        exit(1);
    }

    printf("Conectado al servidor\n");

    while (1) { // Envía y recibe datos
        
        memset(buffer, 0, BUF_SIZE); // Limpia el buffer

        printf("\n$ >> ");

        if (fgets(buffer, BUF_SIZE, stdin) == NULL) { // Lee la entrada estandar y almacena su contenido en el buffer
            break; // Sale si no se puede leer
        }

        buffer[strcspn(buffer, "\n")] = '\0'; // Elimina el salto de línea final

        // Verifica si la entrada es vacía
        if (strlen(buffer) == 0) {
            continue; // Repetir la iteración
        }

        // Envia el comando al servidor
        if (send(fd, buffer, strlen(buffer), 0) < 0) {
            perror("Error al enviar comando");
            break;
        }

        // Verifica si el cliente quiere salir
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "salir") == 0) {
            printf("El cliente ha cerrado la conexión.\n");
            break;
        }

        while (1) {
            memset(buffer, 0, BUF_SIZE); // Limpia el buffer
            ssize_t bytes = recv(fd, buffer, BUF_SIZE - 1, 0); // Recibe los datos del servidor
            if (bytes <= 0) break; // Si no recibe datos

            buffer[bytes] = '\0'; // Asegura que la cadena tenga un final de cadena válido
            if (strstr(buffer, "<EOF>") != NULL) { // Detecta el final de la cadena
                buffer[strstr(buffer, "<EOF>") - buffer] = '\0'; // Lee la cadena hasta el delimitador <EOF>
                printf("%s", buffer); 
                break;
            }

            printf("%s", buffer);
        }
        printf("\n");
    }
    close(fd); // Cierra el file descriptor del cliente
    return 0;
}
