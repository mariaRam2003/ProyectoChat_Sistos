#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define MAX_MSG_SIZE 100
#define MAX_USERS 10

typedef struct {
    char username[MAX_MSG_SIZE];
    char ip[INET_ADDRSTRLEN];
    int sockfd;
    int active;
    pthread_t thread_id;
} User;

User users[MAX_USERS];
int num_users = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para garantizar exclusión mutua

void *handle_client(void *arg) {
    int index = *((int *)arg);
    int sockfd = users[index].sockfd;
    char recvline[MAX_MSG_SIZE];
    
    while (read(sockfd, recvline, MAX_MSG_SIZE) > 0) {
        // Manejar los mensajes recibidos del cliente
        
        // Ejemplo: Imprimir el mensaje recibido en el servidor
        printf("Mensaje recibido de %s: %s\n", users[index].username, recvline);
        
        // Ejemplo: Enviar el mensaje recibido a todos los usuarios conectados (broadcast)
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_users; i++) {
            if (users[i].active && i != index) {
                write(users[i].sockfd, recvline, strlen(recvline) + 1);
            }
        }
        pthread_mutex_unlock(&mutex);
    }
    
    // El cliente ha cerrado la conexión
    printf("%s se ha desconectado.\n", users[index].username);
    pthread_mutex_lock(&mutex);
    users[index].active = 0;
    close(sockfd);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int find_user_index(const char *username) {
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

void *accept_connections(void *arg) {
    int listen_fd = *((int *)arg);
    
    while (1) {
        int comm_fd;
        struct sockaddr_in clientaddr;
        socklen_t client_len = sizeof(clientaddr);
        
        comm_fd = accept(listen_fd, (struct sockaddr *)&clientaddr, &client_len);
        if (comm_fd < 0) {
            perror("Error al aceptar la conexión");
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientaddr.sin_addr), client_ip, INET_ADDRSTRLEN);
        
        printf("Cliente conectado desde %s:%d\n", client_ip, ntohs(clientaddr.sin_port));
        
        char username[MAX_MSG_SIZE];
        read(comm_fd, username, MAX_MSG_SIZE);
        
        pthread_mutex_lock(&mutex);
        int index = find_user_index(username);
        if (index == -1 && num_users < MAX_USERS) {
            strcpy(users[num_users].username, username);
            strcpy(users[num_users].ip, client_ip);
            users[num_users].sockfd = comm_fd;
            users[num_users].active = 1;
            pthread_create(&users[num_users].thread_id, NULL, handle_client, &num_users);
            num_users++;
        } else {
            printf("Error: Nombre de usuario ya en uso o máximo de usuarios alcanzado.\n");
            write(comm_fd, "ERROR", strlen("ERROR") + 1);
            close(comm_fd);
        }
        pthread_mutex_unlock(&mutex);
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int listen_fd;
    struct sockaddr_in servaddr;
    
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
    
    if (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Error al hacer el bind");
        exit(EXIT_FAILURE);
    }
    
    if (listen(listen_fd, 10) < 0) {
        perror("Error al hacer el listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Servidor de chat iniciado. Esperando conexiones...\n");
    
    pthread_t thread;
    pthread_create(&thread, NULL, accept_connections, &listen_fd);
    
    pthread_join(thread, NULL);
    
    return 0;
}
