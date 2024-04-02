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

#define MAX_MESSAGE_LEN 1024

typedef struct {
    char username[50];
    char status[20];
    char ip[16];
    int port;
} ClientInfo;

// Estructura para pasar datos al hilo listener
typedef struct {
    int sockfd;
    ClientInfo *client;
} ListenerArgs;

// Mutex para controlar el acceso a la impresión de mensajes
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Función para recibir mensajes del servidor
void *listener_thread(void *arg) {
    ListenerArgs *args = (ListenerArgs *)arg;

    while (1) {
        // Recibir el mensaje del servidor
        uint8_t buffer[MAX_MESSAGE_LEN];
        ssize_t bytes_received = recv(args->sockfd, buffer, MAX_MESSAGE_LEN, 0);
        if (bytes_received <= 0) {
            perror("Error al recibir mensaje del servidor");
            exit(EXIT_FAILURE);
        }

        // Deserializar el mensaje de respuesta del servidor
        Chat__ServerResponse *response = chat__server_response__unpack(NULL, bytes_received, buffer);
        if (response == NULL) {
            perror("Error al deserializar mensaje de respuesta del servidor");
            exit(EXIT_FAILURE);
        }

        // Procesar la respuesta del servidor
        pthread_mutex_lock(&print_mutex);
        switch (response->option) {
            case 2: // Mensaje directo
                printf("\033[0;32m%s: %s\033[0m\n", response->messagecommunication->sender, response->messagecommunication->message);
                break;
            // Agregar casos para otras opciones de respuesta del servidor aquí
            default:
                fprintf(stderr, "Opción de respuesta del servidor no reconocida: %d\n", response->option);
                break;
        }
        pthread_mutex_unlock(&print_mutex);

        // Liberar la memoria de la respuesta del servidor
        chat__server_response__free_unpacked(response, NULL);
    }

    return NULL;
}

// Función para enviar una petición del cliente al servidor
void send_client_petition(int sockfd, const Chat__ClientPetition *petition) {
    // Serializar la petición del cliente
    size_t message_len = chat__client_petition__get_packed_size(petition);
    uint8_t *message_buffer = malloc(message_len);
    chat__client_petition__pack(petition, message_buffer);

    // Enviar la petición del cliente al servidor
    if (send(sockfd, message_buffer, message_len, 0) == -1) {
        perror("Error al enviar mensaje de petición del cliente al servidor");
        free(message_buffer);
        exit(EXIT_FAILURE);
    }

    // Liberar la memoria del mensaje serializado
    free(message_buffer);
}

void receive_broadcast_message(int sockfd) {
    // Recibir el mensaje de broadcasting del servidor
    uint8_t buffer[MAX_MESSAGE_LEN];
    ssize_t bytes_received = recv(sockfd, buffer, MAX_MESSAGE_LEN, 0);
    if (bytes_received <= 0) {
        perror("Error al recibir mensaje de broadcasting del servidor");
        exit(EXIT_FAILURE);
    }

    // Deserializar el mensaje de broadcasting
    Chat__ServerResponse *response = chat__server_response__unpack(NULL, bytes_received, buffer);
    if (response == NULL) {
        perror("Error al deserializar mensaje de broadcasting del servidor");
        exit(EXIT_FAILURE);
    }

    // Verificar si el mensaje es una respuesta exitosa
    if (response->code == 200 && response->option == 4 && response->messagecommunication != NULL) {
        // Imprimir el mensaje de broadcasting recibido
        printf("Mensaje de %s: %s\n", response->messagecommunication->sender, response->messagecommunication->message);
    } else {
        // Imprimir el mensaje de error del servidor
        fprintf(stderr, "Error del servidor: %s\n", response->servermessage);
    }

    // Liberar la memoria de la respuesta del servidor
    chat__server_response__free_unpacked(response, NULL);
}


// Función para enviar un mensaje de broadcasting al servidor
void send_broadcast_message(int sockfd, const char *message, const char *sender) {
    // Crear un mensaje de comunicación para broadcasting
    Chat__MessageCommunication communication_broadcast = CHAT__MESSAGE_COMMUNICATION__INIT;
    communication_broadcast.message = (char *)message;
    communication_broadcast.recipient = "everyone"; // Destinatario para broadcasting
    communication_broadcast.sender = (char *)sender;

    // Crear una petición del cliente para enviar un mensaje de broadcasting
    Chat__ClientPetition petition_broadcast = CHAT__CLIENT_PETITION__INIT;
    petition_broadcast.option = 4; // Opción para enviar un mensaje de broadcasting
    petition_broadcast.messagecommunication = &communication_broadcast;

    // Enviar la petición del cliente al servidor para enviar un mensaje de broadcasting
    send_client_petition(sockfd, &petition_broadcast);
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
    Chat__ServerResponse *response = chat__server_response__unpack(NULL, bytes_received, buffer);
    if (response == NULL) {
        perror("Error al deserializar mensaje de comunicación del servidor");
        exit(EXIT_FAILURE);
    }

    // Verificar si el mensaje es una respuesta exitosa
    if (response->code == 200 && response->option == 2 && response->messagecommunication != NULL) {
        // Imprimir el mensaje directo recibido
        printf("\033[0;32m%s: %s\033[0m\n", response->messagecommunication->sender, response->messagecommunication->message);
    } else {
        // Imprimir el mensaje de error del servidor
        fprintf(stderr, "Error del servidor: %s\n", response->servermessage);
    }

    // Liberar la memoria de la respuesta del servidor
    chat__server_response__free_unpacked(response, NULL);
}

// Función para enviar un mensaje directo al servidor
void send_direct_message(int sockfd, const char *recipient, const char *message, const char *sender) {
    // Crear un mensaje de comunicación directa
    Chat__MessageCommunication communication_direct = CHAT__MESSAGE_COMMUNICATION__INIT;
    communication_direct.message = (char *)message;
    communication_direct.recipient = (char *)recipient;
    communication_direct.sender = (char *)sender;

    // Crear una petición del cliente para enviar un mensaje directo
    Chat__ClientPetition petition_direct = CHAT__CLIENT_PETITION__INIT;
    petition_direct.option = 4; // Opción para enviar un mensaje directo
    petition_direct.messagecommunication = &communication_direct;

    // Enviar la petición del cliente al servidor para enviar un mensaje directo
    send_client_petition(sockfd, &petition_direct);
}


void send_exit_request(int sockfd) {
    // Crear una petición del cliente para salir
    Chat__ClientPetition petition_exit = CHAT__CLIENT_PETITION__INIT;
    petition_exit.option = 7; // Opción para salir

    // Enviar la petición del cliente al servidor para salir
    send_client_petition(sockfd, &petition_exit);
}


// Función para enviar una petición de cambio de estado al servidor
void send_status_change_request(int sockfd, const char *status, ClientInfo *client) {
    // Crear un mensaje de cambio de estado
    Chat__ChangeStatus change_status = CHAT__CHANGE_STATUS__INIT;
    change_status.username = client->username;
    change_status.status = (char *)status;

    // Crear una petición del cliente para cambiar de estado
    Chat__ClientPetition petition_change_status = CHAT__CLIENT_PETITION__INIT;
    petition_change_status.option = 3; // Opción para cambiar de estado
    petition_change_status.change = &change_status;

    // Enviar la petición del cliente al servidor para cambiar de estado
    send_client_petition(sockfd, &petition_change_status);
}

// Función para recibir la respuesta del servidor
void receive_server_response(int sockfd, ClientInfo *client) {
    // Recibir la respuesta del servidor
    uint8_t buffer[MAX_MESSAGE_LEN];
    ssize_t bytes_received = recv(sockfd, buffer, MAX_MESSAGE_LEN, 0);
    if (bytes_received <= 0) {
        perror("Error al recibir respuesta del servidor");
        exit(EXIT_FAILURE);
    }

    // Deserializar la respuesta del servidor
    Chat__ServerResponse *response = chat__server_response__unpack(NULL, bytes_received, buffer);
    if (response == NULL) {
        perror("Error al deserializar respuesta del servidor");
        exit(EXIT_FAILURE);
    }

    // Procesar la respuesta del servidor según la opción
    switch (response->option) {
        case 3: // Cambio de estado
            if (response->code == 200) {
                printf("Cambio de estado exitoso. Nuevo estado: %s\n", response->change->status);
                strcpy(client->status, response->change->status); // Actualizar el estado del cliente localmente
            } else {
                fprintf(stderr, "Error al cambiar de estado: %s\n", response->servermessage);
            }
            break;
        // Agregar casos para otras opciones de respuesta del servidor aquí
        default:
            fprintf(stderr, "Opción de respuesta del servidor no reconocida: %d\n", response->option);
            break;
    }

    // Liberar la memoria de la respuesta del servidor
    chat__server_response__free_unpacked(response, NULL);
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
    petition_register.registration = &registration;

    // Enviar la petición del cliente al servidor para el registro
    send_client_petition(sockfd, &petition_register);

    // Crear argumentos para el hilo listener
    ListenerArgs listener_args;
    listener_args.sockfd = sockfd;
    listener_args.client = &client;

    // Crear el hilo listener para escuchar las respuestas del servidor
    pthread_t listener_tid;
    if (pthread_create(&listener_tid, NULL, listener_thread, (void *)&listener_args) != 0) {
        perror("Error al crear el hilo listener");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Bucle principal para mostrar el menú y procesar las opciones
    int option;
    char input[1024];
    while (1) {
        // Mostrar el menú
        printf("\n--- Menú ---\n");
        printf("1. Chatear con todos los usuarios (broadcasting)\n");
        printf("2. Enviar y recibir mensajes directos\n");
        printf("3. Cambiar de status\n");
        printf("4. Listar usuarios conectados\n");
        printf("5. Desplegar información de un usuario\n");
        printf("6. Ayuda\n");
        printf("7. Salir\n");
        printf("Seleccione una opción: ");
        
        // Leer la opción del usuario
        fgets(input, sizeof(input), stdin);
        sscanf(input, "%d", &option);

        switch (option) {
            case 1:
                printf("username: %s\n", client.username);
                printf("Chatear con todos los usuarios (broadcasting)\n");
                // Solicitar al usuario que ingrese el mensaje a enviar
                printf("Ingrese el mensaje a enviar a todos los usuarios: ");
                fgets(input, sizeof(input), stdin);

                // Enviar el mensaje de broadcasting al servidor
                send_broadcast_message(sockfd, input, client.username);

                // Recibir el mensaje de broadcasting del servidor
                receive_broadcast_message(sockfd);
                break;

            case 2:
                printf("Enviar y recibir mensajes directos\n");
                // Solicitar al usuario que ingrese el nombre del destinatario y el mensaje a enviar
                printf("Ingrese el nombre del destinatario: ");
                fgets(input, sizeof(input), stdin);
                char recipient[50];
                strcpy(recipient, input);

                printf("Ingrese el mensaje a enviar a %s: ", recipient);
                fgets(input, sizeof(input), stdin);

                // Enviar el mensaje directo al servidor
                send_direct_message(sockfd, recipient, input, client.username);

                // Esperar y recibir la respuesta del servidor
                receive_direct_message(sockfd);

                break;

            case 3:
                printf("Cambiar de status\n");
                printf("Seleccione su nuevo estado (ACTIVO, OCUPADO, INACTIVO): ");
                fgets(input, sizeof(input), stdin);
                send_status_change_request(sockfd, input, &client);
                receive_server_response(sockfd, &client);
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
                // Enviar la petición para salir al servidor
                send_exit_request(sockfd);
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
