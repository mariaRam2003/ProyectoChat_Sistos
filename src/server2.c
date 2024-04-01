#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <protobuf-c/protobuf-c.h>
#include "proto/chat_protocol.pb-c.h"

#define MAX_CLIENTS 10

/**
 * @brief This struct holds the server information
 */
struct server_info {
    int socket_fd;
    struct sockaddr_in server_addr;
};

/**
 * @brief This functions initializes the server
 * @param port string representing the port number
 * @return server_info struct containing the socket file descriptor and the server address
 */
server_info init_server(char* port){
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
    server_addr.sin_port = htons(atoi(argv[1]));

    if (bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error on binding");
        exit(1);
    }

    listen(socket_fd, MAX_CLIENTS);

    server_info info;
    info.socket_fd = socket_fd;
    info.server_addr = server_addr;

    return info;
}

void *handle_client(void *arg) {

}


int main(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "Port number not provided. Program terminated\n");
        exit(1);
    }

    server_info info = init_server(argv[1]);


}
