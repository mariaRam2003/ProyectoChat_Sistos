#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <protobuf-c/protobuf-c.h>
#include "proto/chat_protocol.pb-c.h"

#define MAX_MESSAGE_LEN 1024

typedef struct {
    char username[50];
    char status[20];
    char ip[16];
    int port;
} ClientInfo;

// Función para mostrar el menú
void display_menu() {
    printf("\n--- Menú ---\n");
    printf("1. Chatear con todos los usuarios (broadcasting)\n");
    printf("2. Enviar y recibir mensajes directos\n");
    printf("3. Cambiar de status\n");
    printf("4. Listar usuarios conectados\n");
    printf("5. Desplegar información de un usuario\n");
    printf("6. Ayuda\n");
    printf("7. Salir\n");
    printf("Seleccione una opción: ");
}

void send_client_petition(int sockfd, const Chat__ClientPetition *petition) {
    // Serializar el mensaje de petición del cliente
    size_t message_len = chat__client_petition__get_packed_size(petition);
    uint8_t *message_buffer = malloc(message_len);
    chat__client_petition__pack(petition, message_buffer);

    // Enviar el mensaje de petición del cliente al servidor
    if (send(sockfd, message_buffer, message_len, 0) == -1) {
        perror("Error al enviar mensaje de petición del cliente al servidor");
        free(message_buffer);
        exit(EXIT_FAILURE);
    }

    // Liberar memoria del mensaje de petición del cliente
    free(message_buffer);
}

void receive_direct_message(int sockfd) {
    // Recibir el mensaje de comunicación del servidor
    uint8_t buffer[MAX_MESSAGE_LEN];
    ssize_t bytes_received = recv(sockfd, buffer, MAX_MESSAGE_LEN, 0);
    if (bytes_received <= 0) {
        perror("Error al recibir mensaje de comunicación del servidor");
        exit(EXIT_FAILURE);
    }

    // Deserializar el mensaje de comunicación
    Chat__MessageCommunication *communication = chat__message_communication__unpack(NULL, bytes_received, buffer);
    if (communication == NULL) {
        perror("Error al deserializar mensaje de comunicación del servidor");
        exit(EXIT_FAILURE);
    }

    // Imprimir el mensaje en el formato deseado
    printf("%s: %s\n", communication->sender, communication->message);

    // Liberar la memoria del mensaje de comunicación
    chat__message_communication__free_unpacked(communication, NULL);
}

int main(int argc, char *argv[]) {
    // Verificar argumentos de línea de comandos
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <nombredeusuario> <IPdelservidor> <puertodelservidor>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Inicializar información del cliente
    ClientInfo client;
    strcpy(client.username, argv[1]);
    strcpy(client.ip, argv[2]);
    client.port = atoi(argv[3]);
    strcpy(client.status, "activo"); // Establecer el estado inicial del cliente

    // Establecer conexión con el servidor
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error al crear socket");
        exit(EXIT_FAILURE);
    }

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

    // Crear un mensaje de registro del usuario
    Chat__UserRegistration registration = CHAT__USER_REGISTRATION__INIT;
    registration.username = client.username;
    registration.ip = client.ip;

    // Crear una petición del cliente para el registro
    Chat__ClientPetition petition_register = CHAT__CLIENT_PETITION__INIT;
    petition_register.option = 1; // Opción para registro
    petition_register.registration = registration;

    // Enviar la petición del cliente al servidor para el registro
    send_client_petition(sockfd, &petition_register);

    // Bucle principal para mostrar el menú y procesar las opciones
    int option;
    char input[1024];
    while (1) {
        display_menu();
        fgets(input, sizeof(input), stdin);
        sscanf(input, "%d", &option);

        switch (option) {
            case 1:
                printf("Chatear con todos los usuarios\n");
                // Solicitar al usuario que ingrese el mensaje a enviar
                printf("Ingrese el mensaje a enviar a todos los usuarios: ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = '\0'; // Eliminar el carácter de nueva línea del final del mensaje

                // Crear un mensaje de comunicación
                Chat__MessageCommunication communication_broadcast = CHAT__MESSAGE_COMMUNICATION__INIT;
                communication_broadcast.message = input;
                communication_broadcast.sender = client.username;

                // Crear una petición del cliente para enviar un mensaje a todos los usuarios
                Chat__ClientPetition petition_broadcast = CHAT__CLIENT_PETITION__INIT;
                petition_broadcast.option = 4; // Opción para enviar un mensaje de broadcasting
                petition_broadcast.messagecommunication = communication_broadcast;

                // Enviar la petición del cliente al servidor para enviar un mensaje a todos los usuarios
                send_client_petition(sockfd, &petition_broadcast);
                break;

            case 2:
                printf("Enviar y recibir mensajes directos\n");
                // Solicitar al usuario que ingrese el nombre del destinatario y el mensaje a enviar
                printf("Ingrese el nombre del destinatario: ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = '\0'; // Eliminar el carácter de nueva línea del final del nombre
                char recipient[50];
                strcpy(recipient, input);

                printf("Ingrese el mensaje a enviar a %s: ", recipient);
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = '\0'; // Eliminar el carácter de nueva línea del final del mensaje

                // Enviar el mensaje directo al servidor
                send_message_to_server(sockfd, input, recipient, client.username);

                // Recibir mensajes directos pendientes
                receive_direct_message(sockfd);
                break;

            case 3:
                printf("Cambiar de status\n");
                break;
            case 4:
                printf("Listar usuarios conectados\n");
                break;
            case 5:
                printf("Desplegar información de un usuario\n");
                break;
            case 6:
                printf("Ayuda\n");
                break;
            case 7:
                printf("Salir\n");
                // Liberar memoria y cerrar socket al finalizar
                close(sockfd);
                exit(0);
            default:
                printf("Opción no válida. Por favor, seleccione una opción válida.\n");
                break;
        }
    }

    return 0;
}
