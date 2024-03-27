#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h> // Se agrega la inclusión de la biblioteca time.h

#define MAX_MSG_SIZE 100

int main(void) { // Se cambia 'void' por 'int'
    char cadena[MAX_MSG_SIZE];
    int listen_fd, comm_fd;
    struct sockaddr_in servaddr;
    FILE *myf = fopen("conversacionServidor.txt", "a");
    time_t t;
    struct tm *tm;
    char hora[100];
    char *tmp;
    char sendline[MAX_MSG_SIZE] = "Hola, soy el servidor\n";

    listen_fd = socket(AF_INET, SOCK_STREAM, 0); //crea el socket
    bzero(&servaddr, sizeof(servaddr)); //inicializa la estructura servaddr a 0

    servaddr.sin_family = AF_INET; //asigna la familia de direcciones
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //asigna la direccion IP
    servaddr.sin_port = htons(22000); //asigna el puerto

    bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)); //asigna la direccion IP y el puerto al socket
    listen(listen_fd, 10); //pone el socket en modo pasivo, esperando conexiones
    comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL); //acepta la conexion

    printf("\n\n\t\t--------Se inicio el chat--------\n\n");
    fputs("\n\n\t\t--------Se inicio el chat--------\n\n", myf);

    while(!strstr(cadena, "adios") && !strstr(sendline, "adios")) {
        bzero(cadena, MAX_MSG_SIZE);
        t = time(NULL);
        tm = localtime(&t);
        strftime(hora, 100, "\n otro usuario (%H :%M) -> ", tm);

        read(comm_fd, cadena, MAX_MSG_SIZE);
        tmp = strncpy(hora, cadena, 100); // Usar strncpy para copiar la cadena
        printf("%s", tmp);
        fputs(tmp, myf);

        if (!strstr(cadena, "adios")) {
            strftime(hora, 100, "yo %H :%M -> ", tm);
            printf("%s", hora);
            fgets(sendline, MAX_MSG_SIZE, stdin);
            tmp = strcat(hora, sendline);
            write(comm_fd, sendline, strlen(sendline) + 1);
            fputs(tmp, myf);
        }
    }
    printf("\n\n Conversacion finalizada");
    fputs("\n Se genero el archivo -> conversacionServidor.txt", myf);
    fclose(myf);
    return 0; // Se agrega el valor de retorno
}