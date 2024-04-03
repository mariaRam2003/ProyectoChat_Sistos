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

// mutex
pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t glob_var_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;


void *handle_client(void* sock_fd);

void add_client(char* user, char* ip, char* status){
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

void handle_add_client(Chat__ClientPetition* cli_petition){
    Chat__UserRegistration *registration = cli_petition->registration;
    char* username = registration->username;
    char* ip = registration->ip;
    char state[20] = "activo";

    add_client(username, ip, state);
}


void option_manager(int option, int sockdf, Chat__ClientPetition* cli_petition){

    switch(option){
        case 1:{
            handle_add_client(cli_petition);
            print_user_list();
            break;
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