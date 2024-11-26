#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#define QLEN 2 // Cantidad de conexiones simultáneas permitidas
#define MAX_TOKENS 20 // Tokens permitidos como argumentos de los comandos 
#define BUF_SIZE 8192 // Tamaño del buffer para recibir comandos

/*
    Elimina los espacios en blanco al inicio y al final de la cadena
*/
void trim(char *str) {
    char *start = str; // Apuntador al primer caracter de la cadena str
    char *end;

    // Mueve el puntero hasta que ya no encuentre espacios en blanco
    while (isspace((unsigned char)*start)) start++;

    // Comprueba si el apuntador llego al final de la cadena
    // Esto indica que toda la cadena esta vacía
    if (*start == '\0') {
        str[0] = '\0';
        return;
    }

    // Copia la cadena (sin espacios) a la posición inicial
    memmove(str, start, strlen(start) + 1);

    // Puntero al último caracter de la cadena
    end = str + strlen(str) - 1;

    // Retrodece el puntero mientras el caracter sea un espacio en blanco
    while (end > str && isspace((unsigned char)*end)) end--;

    // Coloca el caracter de final de cadena en el apuntador actual
    *(end + 1) = '\0';
}

/*
    Toma una cadena y las separa en diferentes tokens usando el delimitador de espacios " "
    Almacena los tokens en un aputandor de array tipo char (arg_list[])
    Devuelve el número de elementos separados
*/
int split(const char *buf_peticion, char *arg_list[]) {
    int count = 0; // Cantidad de tokens
    char *token; 
    char *buffer_copy = strdup(buf_peticion); // Copia de la cadena original

    token = strtok(buffer_copy, " "); // Divide la cadena por espacios
    while (token != NULL && count < MAX_TOKENS - 1) { // Mientras que haya tokens
        arg_list[count++] = strdup(token);  // Almacena el token en el array
        token = strtok(NULL, " ");  // Llama nuevamente a la función strtok
    }

    arg_list[count] = NULL; // En el último elemento del contador agrega NULL
    free(buffer_copy); // Libera la memoria dinámica de la copia de la cadena
    return count; // Retorna el número de tokens
}

/*
    Ejecuta un comando en un proceso hijo y redirige su salida estándar a través de un pipe
    enviando la salida al un descriptor de archivo (cliente_fd)
    Recibe un comando, la lista de argumentos y el file descriptor de un cliente
*/
void spawn(char *command, char *arg_list[], int client_fd) {
    int pipe_fd[2]; // Array para uso del pipe | pipe_fd[0] = lectura | pipe_fd[1] = escritura
    if (pipe(pipe_fd) == -1) { // Crea el pipe, si falla en la creación lo notifica y retorna la función
        perror("Error al crear el pipe");
        return;
    }

    pid_t pid = fork(); // Creación de un proceso hijo
    if (pid == 0) { // Ejecución del proceso hijo
        
        close(pipe_fd[0]);  // Cierra el lado de lectura del pipe
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirige el stdout al pipe
        dup2(pipe_fd[1], STDERR_FILENO); // Redirige el stderr al pipe
        close(pipe_fd[1]);  // Cierra el lado de escritura del pipe

        execvp(command, arg_list); // Ejecuta el comando recibido junto con los argumentos
            // Este proceso remplaza la ejecución del proceso hijo
        
        perror("Error al ejecutar el comando"); // Si ocurre un error con la ejecución del comando, muestra el error
        
        exit(1); // Termina la ejecución del proceso hijo con errores
    
    } else if (pid > 0) { // Proceso padre
        
        close(pipe_fd[1]);  // Cierra la escritura del pipe

        char buffer[BUF_SIZE]; // Buffer que almacena los datos leídos por el pipe
        ssize_t bytes_read; // Variable para almacenar la cantidad de bytes leídos
        
        // Lee los datos del pipe
        while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0'; // Agrega un carácter nulo al final del buffer
            send(client_fd, buffer, bytes_read, 0); // Envía el buffer al cliente
        }

        // Envía una señal de fin de transmisión
        send(client_fd, "<EOF>", 5, 0);

        close(pipe_fd[0]);  // Cerrar el lado de lectura del pipe
        wait(NULL);         // Esperar a que el proceso hijo termine

    } else {
        perror("Error al crear el proceso hijo");
    }
}

/*
    Función principal de ejecución del servidor
    Recibe el puerto en que estará escuchando el servidor
*/
int main(int argc, char *argv[]) {
    struct sockaddr_in servidor, cliente; // Configuración de servidor y del cliente
    struct hostent *info_cliente; // Información del cliente
    int fd_s, fd_c; // FileDescriptor del cliente y el servidor
    socklen_t longClient; // Longitud de la estructura del cliente
    int num_cliente = 0; // Numero de clientes conectados

    char *arg_list[MAX_TOKENS]; // Almacena los argumentos recibidos por el cliente
    char buf_peticion[BUF_SIZE];  // Buffer para recibir datos del cliente

    // Creación del socket servidor
    fd_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Configura el servidor
    memset((char *)&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;
    servidor.sin_port = htons((u_short)atoi(argv[1]));
    memset(&(servidor.sin_zero), '\0', 8);

    // Asocia el fileDescriptor con la configuración del servidor
    bind(fd_s, (struct sockaddr *)&servidor, sizeof(servidor));

    // Escucha conexiones
    listen(fd_s, QLEN);

    longClient = sizeof(cliente); // Establece el tamaño de la estructura del cliente

    printf("Esperando conexiones...\n");

    while (1) { // Mantiene el servidor levantado
        
        // Espera una conexión entrante y lo asocia con el fileDescriptor del cliente
        fd_c = accept(fd_s, (struct sockaddr *)&cliente, &longClient);
        if (fd_c < 0) { // En caso de que falle
            perror("Error al aceptar conexión");
            continue;
        }

        num_cliente++; // Incrementa el contador de clientes
        // Obtiene la información del cliente
        info_cliente = gethostbyaddr((char *)&cliente.sin_addr, sizeof(cliente.sin_addr), AF_INET);
        printf("Conexión aceptada de cliente #%d (%s)\n", num_cliente,
               (info_cliente) ? info_cliente->h_name : inet_ntoa(cliente.sin_addr));

        if (fork() == 0) { // Crear un proceso hijo para manejar al cliente
            
            // Este es el proceso hijo
            
            close(fd_s); // El hijo cierra el file descriptor del servidor

            do { // Este bucle recibe y procesa los mensajes del cliente
                
                memset(buf_peticion, '\0', sizeof(buf_peticion)); // Limpia el buffer

                int n = recv(fd_c, buf_peticion, sizeof(buf_peticion), 0); // Datos recibidos por el cliente

                if (n <= 0) { // Valida que el cliente no haya enviado una cadena vacía
                    break; 
                }

                buf_peticion[n] = '\0'; // Asegura que el último caracter de cadena termine correctamente
                
                trim(buf_peticion); // Elimina los espacios en blanco

                // Verificar si el cliente quiere salir
                if (strcmp(buf_peticion, "exit") == 0 || strcmp(buf_peticion, "salir") == 0) {
                    printf("El cliente solicitó salir. Cerrando conexión.\n");
                    break;
                }

                printf("Mensaje del cliente %d: %s\n", num_cliente, buf_peticion);

                int count = split(buf_peticion, arg_list); // Divide la cadena por espacios
                
                if (count > 0) { // Valida que haya tokens leídos
                    spawn(arg_list[0], arg_list, fd_c); // Ejecuta el comando recibido del cliente 
                                                        // y trasmiste la salida del comando
                }
                for (int i = 0; i < count; i++) {
                    free(arg_list[i]); // Liberar memoria de los argumentos
                }
            } while (1);

            printf("Se ha cerrado la conexión con el cliente... Este hijo termina\n");
            close(fd_c); // Cierra el file descriptor del cliente
            exit(0); // Termina la ejecución del proceso hijo correctamente
        }

        // El proceso padre continúa esperando nuevas conexiones
        close(fd_c);
    }

    close(fd_s); // Cierra el file descriptor del servidor
    shutdown(fd_s, SHUT_RDWR); // Cierra completamente el socket
    exit(0); // Finaliza el programa
}
