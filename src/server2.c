#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <protobuf-c/protobuf-c.h>
#include "proto/chat_protocol.pb-c.h"

#define MAX_CLIENTS 10
#define BUFF_SIZE 5000
int client_count = 0;
pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief This struct holds the server information
 */
struct ServerInfo {
    int socket_fd;
    struct sockaddr_in server_addr;
};

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

void *handle_client(void *cli_sock_fd) {
    // param casting
    int client_fd = *((int*) cli_sock_fd);

    // allocating buffer memory
    void* buffer = malloc(BUFF_SIZE);

    // reading bytes from client socket
    int bytes_received = recv(client_fd, buffer, BUFF_SIZE, 0);
    if (bytes_received == -1) {
        perror("recv");
    } else {
        printf("Received %d bytes of data: %d\n", bytes_received, *((int*) buffer));
    }

    // getting pointer of proto
    Chat__ClientPetition *cli_petition = chat__client_petition__unpack(NULL, bytes_received, buffer);
    free(buffer);  // Free allocated buffer after unpacking
    if (cli_petition == NULL) {
        fprintf(stderr, "Error unpacking message\n");
        exit(EXIT_FAILURE);
    }

    int option = cli_petition->option;

    pthread_mutex_lock(&stdout_mutex);
    printf("option %d", option);
    pthread_mutex_unlock(&stdout_mutex);

    // Clean up
    chat__client_petition__free_unpacked(cli_petition, NULL);

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
