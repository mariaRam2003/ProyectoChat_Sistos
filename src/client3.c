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
#define MAX_BUFF_SIZE 8192
#define MESSAGE_LEN 200
#define USER_LEN 20

// Mutex
pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char username[USER_LEN];
    char status[20];
    char ip[16];
    int port;
    int sock_fd;
} ClientInfo;

void* listener(void* sock_fd);

void* speaker(void* sock_fd);

void listen_message(Chat__ServerResponse *response){
    Chat__MessageCommunication *msgComm = response->messagecommunication;
    char* message = msgComm->message;
    char* recipient = msgComm->recipient;
    char* sender = msgComm->sender;

    pthread_mutex_lock(&stdout_mutex);
    printf("%s: %s", sender, message);
    pthread_mutex_unlock(&stdout_mutex);
}

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

void send_change_status(int sockfd, char* username, char* status){
    // Crear el mensaje de cambio de estado
    Chat__ChangeStatus change_status = CHAT__CHANGE_STATUS__INIT;
    change_status.username = strdup(username); // Copiar el valor del nombre de usuario
    change_status.status = strdup(status);     // Copiar el valor del estado

    // Crear la petición del cliente para cambiar el estado
    Chat__ClientPetition petition_change_status = CHAT__CLIENT_PETITION__INIT;
    petition_change_status.option = 3; // Opción para cambio de estado
    petition_change_status.change = &change_status;

    // Enviar la petición del cliente al servidor para cambiar el estado
    send_client_petition(sockfd, &petition_change_status);

    // Liberar la memoria asignada por strdup
    free(change_status.username);
    free(change_status.status);
}

void* set_inactive(void* client_info){
    ClientInfo* client_information = (ClientInfo*)client_info;
    sleep(50); // Esperar 8 segundos

    // Cambiar el estado a inactivo
    send_change_status(client_information->sock_fd, client_information->username, "inactivo");

    // Mostrar el mensaje de inactividad
    pthread_mutex_lock(&stdout_mutex);
    printf("Status cambiado debido a inactividad\n");
    pthread_mutex_unlock(&stdout_mutex);

    return NULL;
}

int display_menu(){
    int option;
    char input[1024];

    printf("\n--- Menú ---\n");
    printf("1. Registro de Usuarios\n");
    printf("2. Listar usuarios conectados\n");
    printf("3. Cambiar de status\n");
    printf("4. Enviar mensajes\n");
    printf("5. Informacion de un usuario en particular\n");
    printf("6. Ayuda\n");
    printf("7. Salir\n");
    printf("Seleccione una opción: ");

    fgets(input, sizeof(input), stdin);
    sscanf(input, "%d", &option);

    return option;
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

void request_user_info(int sockfd, const char* username) {
    // Crear la estructura de solicitud de información de usuario
    Chat__UserRequest user_request = CHAT__USER_REQUEST__INIT;
    user_request.user = (char*)username;

    // Crear la estructura de petición del cliente
    Chat__ClientPetition client_petition = CHAT__CLIENT_PETITION__INIT;
    client_petition.option = 5; // Opción para obtener información de un usuario en particular
    client_petition.users = &user_request;

    // Enviar la petición al servidor
    send_client_petition(sockfd, &client_petition);
}

void handle_user_info(Chat__ConnectedUsersResponse* user_response) {
    if (user_response == NULL) {
        printf("No se encontró información para el usuario solicitado.\n");
        return;
    }

    int user_count = user_response->n_connectedusers;
    printf("Información del usuario:\n");

    for (int i = 0; i < user_count; i++) {
        Chat__UserInfo* user_info = user_response->connectedusers[i];
        printf("Nombre de usuario: %s\n", user_info->username);
        printf("IP: %s\n", user_info->ip);
        printf("Estado: %s\n", user_info->status);
    }
}

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

    // Desde aquí se mandan los requests del usuario
    pthread_t thread_speak, thread_listen, thread_inactive;
    pthread_create(&thread_speak, NULL, speaker, (void *)&client);
    pthread_create(&thread_listen, NULL, listener, (void *)&sockfd);
    pthread_create(&thread_inactive, NULL, set_inactive, (void *)&client);

    // Para que el file descriptor no se cierre hasta que ambos procesos culminen
    pthread_join(thread_speak, NULL);
    pthread_join(thread_listen, NULL);
    pthread_join(thread_inactive, NULL);

    close(sockfd);
    printf("Conexion cerrada!\n");
    return 0;
}

void* listener(void* sock_fd){
    int sockfd = *((int *) sock_fd);

    // Entramos a un while para escuchar los requests y respuestas que nos puedan llegar
    while(1){
        // Recibir el mensaje de broadcasting del servidor
        char buffer[MAX_BUFF_SIZE];
        ssize_t bytes_received = recv(sockfd, buffer, MAX_BUFF_SIZE, 0);
        if (bytes_received <= 0) {
            perror("Error al recibir mensaje de broadcasting del servidor");
            exit(EXIT_FAILURE);
            continue;
        }

        pthread_mutex_lock(&stdout_mutex);
        printf("longitud en bytes de la respuesta recibida: %d\n",(int) bytes_received);
        pthread_mutex_unlock(&stdout_mutex);

        // Deserializar el mensaje de broadcasting
        Chat__ServerResponse *response = chat__server_response__unpack(NULL, bytes_received, buffer);
        if (response == NULL) {
            pthread_mutex_lock(&stdout_mutex);
            perror("Error al deserializar mensaje de broadcasting del servidor");
            pthread_mutex_unlock(&stdout_mutex);
            continue;
        }

        int code = response->code;
        int option = response->option;

        pthread_mutex_lock(&stdout_mutex);
        printf("opcion elegida: %d\n", option);
        pthread_mutex_unlock(&stdout_mutex);

        switch(option){
            case 1:{
                break;
            }
            case 2:{
                break;
            }
            case 3:{
                break;
            }
            case 4:{
                break;
            }
            case 5:{
                // Manejar respuesta de información de usuario en particular
                Chat__ConnectedUsersResponse* user_response = response->connectedusers;
                handle_user_info(user_response);
                break;
            }
            case 6:{
                break;
            }
            case 7:{
                break;
            }
        }

        // Liberar la memoria de la respuesta del servidor
        chat__server_response__free_unpacked(response, NULL);
    }
}

void* speaker(void* client_info){
    ClientInfo client_information = *((ClientInfo*) client_info);

    // Lo primero que debe hacer el cliente es registrar su usuario
    user_registration(client_information.username, client_information.ip, client_information.sock_fd);

    // Entramos a un while para escuchar las peticiones del usuario
    while(1){
        int option = display_menu();

        pthread_mutex_lock(&stdout_mutex);
        printf("opcion elegida: %d\n", option);
        pthread_mutex_unlock(&stdout_mutex);

        switch (option) {
            case 1: {
                // Registro de usuarios
                pthread_mutex_lock(&stdout_mutex);
                printf("Ya estas registrado tontito duhhh... \n");
                pthread_mutex_unlock(&stdout_mutex);
                break;
            }
            case 2: {
                // Usuarios conectados
                break;
            };
            case 3: {
                // Cambio de estado
                char new_status[20];
                pthread_mutex_lock(&stdout_mutex);
                printf("Ingrese el nuevo estado (ACTIVO, OCUPADO o INACTIVO): ");
                fgets(new_status, sizeof(new_status), stdin);
                pthread_mutex_unlock(&stdout_mutex);
                send_change_status(client_information.sock_fd, client_information.username, new_status);
                break;
            };
            case 4: {
                // Enviar mensajes
                break;
            };
            case 5: {
                // Informacion de un usuario en particular
                char target_username[USER_LEN];
                printf("Ingrese el nombre de usuario del que desea obtener información: ");
                fgets(target_username, sizeof(target_username), stdin);
                // Eliminar el salto de línea del final
                target_username[strcspn(target_username, "\n")] = 0;
                // Enviar la solicitud de información de usuario al servidor
                request_user_info(client_information.sock_fd, target_username);
                break;
            };
            case 6: {
                // Ayuda
                break;
            };
            case 7: {
                // Salida
                break;
            };
            default:{
                break;
            }
        }
    }
    return NULL;
}
