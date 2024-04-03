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

// informacion de los clientes
char* client_user_list[MAX_CLIENTS];
char* client_ips[MAX_CLIENTS];
char* client_status[MAX_CLIENTS];

// mutex
pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t glob_var_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void* sock_fd);

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

        client_count++;
    }

}