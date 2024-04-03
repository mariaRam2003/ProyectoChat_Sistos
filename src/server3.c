#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <protobuf-c/protobuf-c.h>
#include "proto/chat_protocol.pb-c.h"

#define MAX_CLIENTS 10
#define BUFF_SIZE 8192

// define memory usage
#define MESSAGE_LEN 200
#define USER_LEN 20

int client_count = 0;

// informacion de los clientes
char* client_user_list[MAX_CLIENTS];
char* client_ips[MAX_CLIENTS];
char* client_status[MAX_CLIENTS];
int sock_fds[MAX_CLIENTS];

// mutex
pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t glob_var_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;


void *handle_client(void* sock_fd);

void handle_state_change(Chat__ClientPetition* cli_petition, int my_sockfd){}

int get_sock_fd(char* user){
    for(int i = 0; i < MAX_CLIENTS; i++){
        if (strcmp(user, client_user_list[i]) == 0){
            int sock_fd = sock_fds[i];
            return sock_fd;
        }
    }

    return -1;
}

void send_one_msg(Chat__ClientPetition* cli_petition, int my_sockfd){
    Chat__MessageCommunication *msgComm = cli_petition->messagecommunication;
    char* msg = msgComm->message;
    char* rec = msgComm->recipient;
    char* sender = msgComm->sender;

    int rec_sockfd = get_sock_fd(rec);

    Chat__MessageCommunication comm = CHAT__MESSAGE_COMMUNICATION__INIT;
    comm.message = strdup(msg);
    comm.sender = strdup(sender);
    comm.recipient = strdup(rec);

    Chat__ServerResponse srvr_response = CHAT__SERVER_RESPONSE__INIT;
    srvr_response.option = 4;
    srvr_response.code = 200;
    srvr_response.servermessage = "Se meando el mensaje yupi \n";
    srvr_response.messagecommunication = &comm;

    size_t len = chat__server_response__get_packed_size(&srvr_response);
    void* buffer = malloc(len);

    chat__server_response__pack(&srvr_response, buffer);

    if (send(rec_sockfd, buffer, len, 0) < 0){
        pthread_mutex_lock(&stdout_mutex);
        printf("error al mendarle el mensaje al cliente\n");
        pthread_mutex_unlock(&stdout_mutex);
    }

    pthread_mutex_lock(&stdout_mutex);
    printf("Se mando el mensaje al cliente\n");
    pthread_mutex_unlock(&stdout_mutex);

    free(buffer);
}

void broadcast(Chat__ClientPetition* cli_petition, int my_sockfd){
    pthread_mutex_lock(&stdout_mutex);
    printf("Llegue a la funcion broadcast\n");
    pthread_mutex_unlock(&stdout_mutex);

    Chat__MessageCommunication *msgComm = cli_petition->messagecommunication;
    char* msg = msgComm->message;
    char* rec = msgComm->recipient;
    char* sender = msgComm->sender;

    pthread_mutex_lock(&stdout_mutex);
    printf("Llegue a la msgComm \n");
    pthread_mutex_unlock(&stdout_mutex);


    Chat__MessageCommunication comm = CHAT__MESSAGE_COMMUNICATION__INIT;
    comm.message = strdup(msg);
    comm.sender = strdup(sender);
    comm.recipient = strdup(rec);

    pthread_mutex_lock(&stdout_mutex);
    printf("Llegue a commm\n");
    pthread_mutex_unlock(&stdout_mutex);


    Chat__ServerResponse srvr_response = CHAT__SERVER_RESPONSE__INIT;
    srvr_response.option = 4;
    srvr_response.code = 200;
    srvr_response.servermessage = "Se meando el mensaje yupi \n";
    srvr_response.messagecommunication = &comm;


    pthread_mutex_lock(&stdout_mutex);
    printf("srvr response\n");
    pthread_mutex_unlock(&stdout_mutex);

    size_t len = chat__server_response__get_packed_size(&srvr_response);
    void* buffer = malloc(len);
    if (buffer == NULL){
        perror("No se pudo asignar memoria al buffer\n");
    }

    chat__server_response__pack(&srvr_response, buffer);

    for (int i = 0; i < MAX_CLIENTS; i++){
        if(sock_fds[i] != 0){
            if (send(sock_fds[i], buffer, len, 0) < 0){
                pthread_mutex_lock(&stdout_mutex);
                printf("error al mendarle el mensaje al cliente con socket %d\n", sock_fds[i]);
                pthread_mutex_unlock(&stdout_mutex);
            }else{
                pthread_mutex_lock(&stdout_mutex);
                printf("enviado cone exito a %d\n", sock_fds[i]);
                pthread_mutex_unlock(&stdout_mutex);
            }
        }
    }
    free(buffer);
}

void handle_send_message(Chat__ClientPetition* cli_petition, int my_sockfd){

    pthread_mutex_lock(&stdout_mutex);
    printf("Llegue a handle send message\n");
    pthread_mutex_unlock(&stdout_mutex);

    Chat__MessageCommunication *msgComm = cli_petition->messagecommunication;
    char* rec = msgComm->recipient;
    char everyone[20] = "everyone";

    pthread_mutex_lock(&stdout_mutex);
    printf("everyone dec\n");
    pthread_mutex_unlock(&stdout_mutex);

    if (strcmp(everyone, rec) == 0){
        broadcast(cli_petition, my_sockfd);
        pthread_mutex_lock(&stdout_mutex);
        printf("comparison 2\n");
        pthread_mutex_unlock(&stdout_mutex);
    }else{
        pthread_mutex_lock(&stdout_mutex);
        printf("comparison 1\n");
        pthread_mutex_unlock(&stdout_mutex);
        send_one_msg(cli_petition, my_sockfd);
    }

}

void add_client(char* user, char* ip, char* status, int sock_fd){
    char* new_user = malloc(USER_LEN);
    char* new_ip = malloc(USER_LEN);
    char* new_status = malloc(USER_LEN);
    strcpy(new_user, user);
    strcpy(new_ip, ip);
    strcpy(new_status, status);

    pthread_mutex_lock(&glob_var_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++){
        if (client_user_list[i] == NULL){
            client_user_list[i] = new_user;
            client_ips[i] = new_ip;
            client_status[i] = new_status;
            sock_fds[i] = sock_fd;
            client_count++;
            break;
        }
    }
    pthread_mutex_unlock(&glob_var_mutex);
}

void print_user_list(){
    pthread_mutex_lock(&glob_var_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++){
        if (client_user_list[i] != NULL){
            pthread_mutex_lock(&stdout_mutex);
            printf("user: %s\n", client_user_list[i]);
            pthread_mutex_unlock(&stdout_mutex);
        }
    }
    pthread_mutex_unlock(&glob_var_mutex);
}

void handle_add_client(Chat__ClientPetition* cli_petition, int sock_fd){
    Chat__UserRegistration *registration = cli_petition->registration;
    char* username = registration->username;
    char* ip = registration->ip;
    char state[20] = "activo";

    add_client(username, ip, state, sock_fd);
}

void handle_user_list(Chat__ClientPetition* cli_petition, int client_fd){
    Chat__UserRequest *user_req = cli_petition->users;
    char* user = user_req->user;
    char everyone[15] = "everyone";

    // Lista de usuarios conectados
    print_user_list();

    if (strcmp(user, everyone) == 0){
        // Crear una estructura de respuesta con la lista de usuarios conectados
        Chat__ConnectedUsersResponse user_response = CHAT__CONNECTED_USERS_RESPONSE__INIT;

        // Crear un array para almacenar los usuarios conectados
        Chat__UserInfo* user_info_array[MAX_CLIENTS];

        int count = 0;
        pthread_mutex_lock(&glob_var_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_user_list[i] != NULL) {
                // Asignar memoria para una nueva estructura Chat__UserInfo
                Chat__UserInfo* user_info = malloc(sizeof(Chat__UserInfo));
                if (user_info == NULL) {
                    // Manejar fallo en la asignación de memoria
                    fprintf(stderr, "Error: No se pudo asignar memoria para Chat__UserInfo\n");
                    exit(EXIT_FAILURE);
                }

                // Inicializar la estructura Chat__UserInfo
                chat__user_info__init(user_info);

                // Asignar valores a los campos
                user_info->username = strdup(client_user_list[i]);
                user_info->ip = strdup(client_ips[i]);
                user_info->status = strdup(client_status[i]);

                // Almacenar el puntero en el array
                user_info_array[count++] = user_info;
            }
        }
        pthread_mutex_unlock(&glob_var_mutex);

        // Asignar la lista de usuarios conectados a la respuesta
        user_response.connectedusers = user_info_array;
        user_response.n_connectedusers = count;

        // Crear la respuesta del servidor
        Chat__ServerResponse server_response = CHAT__SERVER_RESPONSE__INIT;
        server_response.option = 2;
        server_response.code = 200;
        server_response.servermessage = "Lista de usuarios conectados";
        server_response.connectedusers = &user_response;

        // Serializar la respuesta del servidor
        size_t len = chat__server_response__get_packed_size(&server_response);
        void* buffer = malloc(len);
        if (buffer == NULL){
            perror("Error al asignar memoria");
            return;
        }
        chat__server_response__pack(&server_response, buffer);

        // Enviar la respuesta al cliente
        if (send(client_fd, buffer, len, 0) < 0){
            perror("Error al enviar mensaje al cliente");
        }
        free(buffer);
    }
}



void option_manager(int option, int sockfd, Chat__ClientPetition* cli_petition){

    switch(option){
        case 1:{
            handle_add_client(cli_petition, sockfd);
            //print_user_list();
            break;
        }
        case 2:{
            print_user_list();
            handle_user_list(cli_petition, sockfd);
            break;
        }
        case 3:{
            // cambio de estado
            handle_state_change(cli_petition, sockfd);
        }
        case 4:{
            // Enviar mensaje
            handle_send_message(cli_petition, sockfd);
        }

    }
}

/**
 * @brief This struct holds the server information
 */
struct ServerInfo {
    int socket_fd;
    struct sockaddr_in server_addr;
};

/**
 * This funcction initializes the server
 * @param port
 * @return
 */
struct ServerInfo init_server(char* port){
    int socket_fd;
    struct sockaddr_in server_addr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(port));

    if (bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error on binding");
        exit(1);
    }

    listen(socket_fd, MAX_CLIENTS);

    struct ServerInfo info;
    info.socket_fd = socket_fd;
    info.server_addr = server_addr;

    return info;
}

/**
 * main function
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[]){
    if (argc < 2) {
        fprintf(stderr, "Port number not provided. Program terminated\n");
        exit(1);
    }

    struct ServerInfo info = init_server(argv[1]);

    // Estamos en un loop para escuchar nuevas conexiones
    while(1){
        struct sockaddr_in client_addr;
        int cli_len = sizeof(client_addr);
        int client_socket = accept(info.socket_fd, (struct sockaddr *) &client_addr, &cli_len);

        if (client_socket < 0) {
            perror("Error on accept");
            exit(1);
        }

        if (client_count >= MAX_CLIENTS) {
            printf("Max clients reached. Connection refused\n");
            close(client_socket);
            continue;
        }

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, (void *) &client_socket);
    }

}

void* handle_client(void* sock_fd){
    int client_fd = *((int*) sock_fd);

    while(1){
        void *buffer = malloc(BUFF_SIZE);
        int bytes_recieved =recv(client_fd, buffer, BUFF_SIZE, 0);

        if( bytes_recieved < 0){
            pthread_mutex_lock(&stdout_mutex);
            printf("Error at reading bytes...\n");
            pthread_mutex_unlock(&stdout_mutex);
            return NULL;
        }else if(bytes_recieved == 0){
            pthread_mutex_lock(&stdout_mutex);
            printf("Socket was closed incorrectly...\n");
            pthread_mutex_unlock(&stdout_mutex);
            return NULL;
            // TODO: manejar salida
        }

        pthread_mutex_lock(&stdout_mutex);
        printf("Received %d bytes of data: %d\n", bytes_recieved, *((int *) buffer));
        pthread_mutex_unlock(&stdout_mutex);

        Chat__ClientPetition *cli_petition = chat__client_petition__unpack(NULL, bytes_recieved, buffer);
        free(buffer);

        if (cli_petition == NULL) {
            pthread_mutex_lock(&stdout_mutex);
            fprintf(stderr, "Error unpacking message\n");
            pthread_mutex_unlock(&stdout_mutex);
            return NULL;
        }

        int option = cli_petition->option;

        pthread_mutex_lock(&stdout_mutex);
        printf("opcion elegida: %d \n", option);
        pthread_mutex_unlock(&stdout_mutex);

        option_manager(option, client_fd, cli_petition);

        chat__client_petition__free_unpacked(cli_petition, NULL);
    }
}