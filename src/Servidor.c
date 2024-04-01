#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <protobuf-c/protobuf-c.h>
#include "proto/chat_protocol.pb-c.h"

/**
 * @brief This function is used to display error message
 *
 * @param msg the error message
 */
void error(const char *msg) {
    perror(msg);
    exit(1);
}

//UserInfo fun(){
//    UserInfo info = USER_INFO__INIT;
//    info.username = "gusanitor8";
//    info.status = "activo";
//    info.ip = "127.0.0.1";
//}


int main(int argc, char *argv[]){
    Chat__UserInfo info = CHAT__USER_INFO__INIT;
    info.username = "gusanitor8";
    info.status = "activo";
    info.ip = "127.0.0.1";

    // We check if parameters are provided
    if (argc < 2) {
        fprintf(stderr, "Port number not provided. Program terminated\n");
        exit(1);
    }

    int sockfd, newsockfd, portno;
    char buffer[255]; // Buffer representing the message

    // These structs hold the server and client address information
    struct sockaddr_in serv_addr, cli_addr;

    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    // We set the server address information
    serv_addr.sin_family = AF_INET; // ipv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno); // Convert port number to network byte order

    // We bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("Binding failed");
    }

    // We listen for incoming connections
    printf("Server listening on port %d\n", portno);
        listen(sockfd, 5);

    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0) {
        error("Error on accept");
    }

    while(1){
        bzero(buffer, 255);
        int n = read(newsockfd, buffer, 255);
        if (n < 0) {
            error("Error reading from socket");
        }

        printf("Client: %s\n", buffer);
        bzero(buffer, 255);
        fgets(buffer, 255, stdin);

        n = write(newsockfd, buffer, strlen(buffer));
        if (n < 0) {
            error("Error writing to socket");
        }

        int i = strncmp("Bye", buffer, 3);
        if (i == 0) {
            break;
        }
    }

    close(newsockfd);
    close(sockfd);
    return 0;
}