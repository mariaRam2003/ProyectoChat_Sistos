#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <protobuf-c/protobuf-c.h>
#include "proto/chat_protocol.pb-c.h"

#define MAX_MSG_SIZE 100

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <nombredeusuario> <IPdelservidor> <puertodelservidor>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char username[MAX_MSG_SIZE];
    strcpy(username, argv[1]);

    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // crea el socket
    if (sockfd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    bzero(&servaddr, sizeof(servaddr)); // inicializa la estructura servaddr a 0
    servaddr.sin_family = AF_INET; // asigna la familia de direcciones
    servaddr.sin_port = htons(atoi(argv[3])); // asigna el puerto
    inet_pton(AF_INET, argv[2], &(servaddr.sin_addr)); // asigna la direccion IP

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) { // conecta el socket al servidor
        perror("Error al conectar al servidor");
        exit(EXIT_FAILURE);
    }

    printf("\n\n\t\t--------Se inicio el chat--------\n\n");

    char sendline[MAX_MSG_SIZE];
    char recvline[MAX_MSG_SIZE];

    while (1) {
        printf("Ingrese un comando (chat <usuario> <mensaje>, status <estado>, list, info <usuario>, ayuda, salir):\n");
        fgets(sendline, MAX_MSG_SIZE, stdin);
        sendline[strcspn(sendline, "\n")] = '\0'; // Eliminar el carácter de nueva línea del final

        // Enviar el comando al servidor
        write(sockfd, sendline, strlen(sendline) + 1);

        // Salir del bucle si el comando es "salir"
        if (strcmp(sendline, "salir") == 0) {
            break;
        }

        // Leer la respuesta del servidor
        read(sockfd, recvline, MAX_MSG_SIZE);

        // Procesar la respuesta del servidor y mostrarla al usuario
        printf("%s\n", recvline);
    }

    // Cerrar el socket
    close(sockfd);

    return 0;
}