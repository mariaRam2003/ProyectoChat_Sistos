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
#define BUFF_SIZE 5000
int client_count = 0;
char* client_user_list[MAX_CLIENTS];
char* client_ips[MAX_CLIENTS];
char* client_status[MAX_CLIENTS];
int client_fds[MAX_CLIENTS];
int free_spaces[MAX_CLIENTS] = {0};
pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t glob_var_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;
/**
 * @brief This struct holds the server information
 */
struct ServerInfo {
    int socket_fd;
    struct sockaddr_in server_addr;
};

void change_status(char* username, char* status, int client_fd){
    int user_index = -1;
    pthread_mutex_lock(&glob_var_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++){
        if (strcmp(username, client_user_list[i]) == 0){
            user_index = i;
        }
    }
    pthread_mutex_unlock(&glob_var_mutex);

    if (user_index < 0){
        printf("usuario no encontrado \n");
        return;
    }

    pthread_mutex_lock(&glob_var_mutex);
    client_status[user_index] = status;
    pthread_mutex_unlock(&glob_var_mutex);


    Chat__ServerResponse srvr_res = CHAT__SERVER_RESPONSE__INIT;
    srvr_res.option = 3;
    srvr_res.code = 200;
    srvr_res.servermessage = "success\n";

    size_t len = chat__server_response__get_packed_size(&srvr_res);
    void* buffer = malloc(len);
    if (buffer == NULL){
        pthread_mutex_lock(&stdout_mutex);
        printf("Error al alocar la memoria del buffer\n");
        pthread_mutex_unlock(&stdout_mutex);
    }

    chat__server_response__pack(&srvr_res, buffer);
    pthread_mutex_lock(&socket_mutex);
    if (send(client_fd, buffer, len, 0) < 0){
        printf("Error sending message to recipient\n");
    }
    pthread_mutex_unlock(&socket_mutex);
}


void send_message(char* recipient_, char* message_, char* sender_){
    int recipient_socket = -1;
    // We look for the recipient's socket fd
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (strcmp(client_user_list[i], recipient_) == 0) { // Compare strings for equality
            printf("user list: %s\n", client_user_list[i]);
            printf("recipient: %s\n", recipient_);
            recipient_socket = client_fds[i];
            break;
        }
    }

    printf("recipient socket: %d\n", recipient_socket);

    Chat__ServerResponse srvr_res = CHAT__SERVER_RESPONSE__INIT;

    if (recipient_socket == -1){
        printf("Couldnt find recipient user\n");
        return;
    }

    Chat__MessageCommunication msg = CHAT__MESSAGE_COMMUNICATION__INIT;
    msg.message = message_;
    msg.recipient = sender_;
    msg.sender = recipient_;

    srvr_res.option = 2;
    srvr_res.code = 200;
    srvr_res.servermessage = "success\n";
    srvr_res.messagecommunication = &msg;

    size_t len = chat__server_response__get_packed_size(&srvr_res);
    void* buffer = malloc(len);
    if (buffer == NULL){
        printf("Error at assigning buffer memory, option 2\n");
    }
    chat__server_response__pack(&srvr_res, buffer);

    pthread_mutex_lock(&socket_mutex);
    if (send(recipient_socket, buffer, len, 0) < 0){
        printf("Error sending message to recipient\n");
    }
    pthread_mutex_unlock(&socket_mutex);

}

void send_everyone(char* message_, char* sender_){
    for (int i = 0; i < MAX_CLIENTS; i++){
        char* user = client_user_list[i];

        if (user == NULL){
            continue;
        }else{
            send_message(user, message_, sender_);
        }
    }
}

/**
 * Function to register a user
 * @param user the user to be registrated
 * @param ip user's ip adress
 * @param client_fd socket file descriptor form client
 */
void user_registration(char* user, char* ip, int client_fd){
    if (client_count >= MAX_CLIENTS){
        pthread_mutex_lock(&stdout_mutex);
        printf("Connection rejected, too many connections at the time..\n");
        pthread_mutex_unlock(&stdout_mutex);
        return;
    }

    int index = -1;
    // registramos la data del usuario
    pthread_mutex_lock(&glob_var_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++){
        if (free_spaces[i] == 0){
            free_spaces[i] = 1;
            client_user_list[i] = user;
            client_fds[i] = client_fd;
            client_ips[i] = ip;
            index = i;
            break;
        }
    }
    pthread_mutex_unlock(&glob_var_mutex);

    // response initialization
    Chat__ServerResponse srvr_response = CHAT__SERVER_RESPONSE__INIT;

    if (index < 0){
        pthread_mutex_lock(&stdout_mutex);
        printf("Error at registering user...\n");
        pthread_mutex_unlock(&stdout_mutex);

        // respuesta del server
        srvr_response.option = 1;
        srvr_response.code = 400;
        srvr_response.servermessage = "Ocurrio un error al registrar el usuario\n";
    }else{
        pthread_mutex_lock(&stdout_mutex);
        printf("Registration of user %s succesful\n", client_user_list[index]);
        pthread_mutex_unlock(&stdout_mutex);

        // respuesta del server
        srvr_response.option = 1;
        srvr_response.code = 200;
        srvr_response.servermessage = "Registrado con exito\n";
    }

    size_t len = chat__server_response__get_packed_size(&srvr_response);
    void *buffer = malloc(len);
    if (buffer == NULL){
        pthread_mutex_lock(&stdout_mutex);
        printf("Error assigning buffer \n");
        pthread_mutex_unlock(&stdout_mutex);

        chat__server_response__free_unpacked(&srvr_response, NULL);
        return;
    }

    // Serializamos la respuesta
    chat__server_response__pack(&srvr_response, buffer);

    // enviamos la respuesta de regreso al cliente
    if (send(client_fd, buffer, len, 0) < 0){
        pthread_mutex_lock(&stdout_mutex);
        printf("Error at sending response \n");
        pthread_mutex_unlock(&stdout_mutex);
    }

    free(buffer);
}

/**
 * @brief This functions initializes the server
 * @param port string representing the port number
 * @return server_info struct containing the socket file descriptor and the server address
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
 * This function reads the data from the client's socket and handles the request
 * @param cli_sock_fd socket's file descriptor
 * @return NULL
 */
void *handle_client(void *cli_sock_fd) {
    // param casting
    int client_fd = *((int*) cli_sock_fd);
    int condition = 1;

    while(condition) {
        // allocating buffer memory
        void *buffer = malloc(BUFF_SIZE);

        // reading bytes from client socket
        int bytes_received = recv(client_fd, buffer, BUFF_SIZE, 0);
        if(bytes_received < 0){
            printf("socket disconnected \n");
            continue;
        }else if (bytes_received == 0) {
            // Client has closed the connection
            pthread_mutex_lock(&stdout_mutex);
            printf("Socket was closed incorrectly...\n");
            pthread_mutex_unlock(&stdout_mutex);
            break;
        }else{
            pthread_mutex_lock(&stdout_mutex);
            printf("Received %d bytes of data: %d\n", bytes_received, *((int *) buffer));
            pthread_mutex_unlock(&stdout_mutex);
        }

        // getting pointer of proto
        Chat__ClientPetition *cli_petition = chat__client_petition__unpack(NULL, bytes_received, buffer);
        free(buffer);  // Free allocated buffer after unpacking
        if (cli_petition == NULL) {
            pthread_mutex_lock(&stdout_mutex);
            fprintf(stderr, "Error unpacking message\n");
            pthread_mutex_unlock(&stdout_mutex);
            return NULL;
        }

        int option = cli_petition->option;

        switch(option){
            case 1: {
                pthread_mutex_lock(&stdout_mutex);
                printf("Opcion 1 \n");
                pthread_mutex_unlock(&stdout_mutex);

                Chat__UserRegistration *registration = cli_petition->registration;
                char *username = registration->username;
                char *ip = registration->ip;
                user_registration(username, ip, client_fd);
                break;
            }

            case 2: {
                break;
            }
            case 3: {
                pthread_mutex_lock(&stdout_mutex);
                printf("Opcion 3 Cambio de status\n");
                pthread_mutex_unlock(&stdout_mutex);

                Chat__ChangeStatus *ch_status = cli_petition->change;
                char* user = ch_status->username;
                char* status = ch_status->status;

                change_status(user, status, client_fd);
                break;
            }

            case 4: {
                pthread_mutex_lock(&stdout_mutex);
                printf("Opcion 4\n");
                pthread_mutex_unlock(&stdout_mutex);
                const char everyone[] = "everyone";

                Chat__MessageCommunication *msgCom = cli_petition->messagecommunication;
                char *message = msgCom->message;
                char *recipient = msgCom->recipient;
                char *sender = msgCom->sender;

                if (strcmp(everyone, recipient) == 0){
                    send_everyone(message, sender);
                } else{
                    send_message(recipient, message, sender);
                }

                break;
            }

            case 5: {
                printf("Opcion 5\n");
                break;
            }

            case 6: {
                printf("Opcion 6\n");
                break;
            }
            case 7:{
                printf("Opcion 7\n");
                condition = 0;
                break;
            }
            default: {
                pthread_mutex_lock(&stdout_mutex);
                printf("Opcion invalida \n");
                pthread_mutex_unlock(&stdout_mutex);
                break;
            }
        }

        // Clean up
        chat__client_petition__free_unpacked(cli_petition, NULL);

    }

    // close socket
    close(client_fd);

    pthread_exit(NULL);
}


int main(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "Port number not provided. Program terminated\n");
        exit(1);
    }

    struct ServerInfo info = init_server(argv[1]);

    while(1){
        // Accept a new connection
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

        client_count++;
    }
}
