#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <protobuf-c/protobuf-c.h>
#include "proto/chat_protocol.pb-c.h"

// mutex
pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t glob_var_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;


/**
 *
 */
typedef struct {
    char username[50];
    char status[20];
    char ip[16];
    int port;
    int sock_fd;
} ClientInfo;

void* listener(void* sock_fd);

void* speaker(void* sock_fd);

void send_client_petition(int sockfd, Chat__ClientPetition* petition){
    size_t len = chat__client_petition__get_packed_size(petition);
    void* buffer = malloc(len);

    if (buffer == NULL){
        pthread_mutex_lock(&stdout_mutex);
        printf("Error al alocar memoria del buffer\n");
        pthread_mutex_unlock(&stdout_mutex);
    }

    chat__client_petition__pack(petition, buffer);

    if (send(sockfd, buffer, len, 0) < 0) {
        pthread_mutex_lock(&stdout_mutex);
        perror("Error al enviar mensaje de petición del cliente al servidor");
        pthread_mutex_unlock(&stdout_mutex);

        free(buffer);
        return;
    }

    pthread_mutex_lock(&stdout_mutex);
    printf("Peticion enviada\n");
    pthread_mutex_unlock(&stdout_mutex);

    // Liberar la memoria del mensaje serializado
    free(buffer);
}

void user_registration(char* username, char* ip, int sockfd){
    char status[] = "activo\n";

    Chat__UserRegistration registration = CHAT__USER_REGISTRATION__INIT;
    registration.username = username;
    registration.ip = ip;

    pthread_mutex_lock(&stdout_mutex);
    printf("usuario: %s\n", username);
    printf("ip: %s\n", ip);
    pthread_mutex_unlock(&stdout_mutex);

    // Crear una petición del cliente para el registro
    Chat__ClientPetition petition_register = CHAT__CLIENT_PETITION__INIT;
    petition_register.option = 1; // Opción para registro
    petition_register.registration = &registration;

    send_client_petition(sockfd, &petition_register);
}

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {
    // Verificar argumentos de línea de comandos
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <nombredeusuario> <IPdelservidor> <puertodelservidor>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Establecer conexión con el servidor
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error al crear socket");
        exit(EXIT_FAILURE);
    }

    // Inicializar información del cliente
    ClientInfo client;
    strcpy(client.username, argv[1]);
    strcpy(client.ip, argv[2]);
    client.port = atoi(argv[3]);
    strcpy(client.status, "activo");
    client.sock_fd = sockfd;

    // Informacion acerca del protocolo
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(client.ip);
    server_addr.sin_port = htons(client.port);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al conectar al servidor");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Usuario: %s ", client.username);
    printf("conectado al servidor...\n");

    // desde aqui se mandan los rquests del usuario
    pthread_t thread_speak, thread_listen;
    pthread_create(&thread_speak, NULL, speaker, (void *) &client);
    pthread_create(&thread_listen, NULL, listener, (void *) &sockfd);

    // Para que el file descriptor no se cierre hasta que ambos procesos culminen
    pthread_join(thread_speak, NULL);
    pthread_join(thread_listen, NULL);

    close(sockfd);
    printf("Conexion cerrada!\n");
}


/**
 * This function listens for any incoming responses from the server
 * @param sock_fd
 * @return
 */
void* listener(void* sock_fd){
    // entramos a un while para escuchar los requests y respuestas que nos puedan llegar
    while(1){}
}

/**
 * This function sends users requests over to the server
 * @param sock_fd
 * @return
 */
void* speaker(void* client_info){
    ClientInfo client_information = *((ClientInfo*) client_info);

    // Lo primero que debe hacer el cliente es registrar su usuario
    user_registration(client_information.username, client_information.ip, client_information.sock_fd);

    // entramos a un while para escuchar las peticiones del usuario
    while(1){}
}