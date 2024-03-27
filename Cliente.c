#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>


void main(void){
    char cadena[100];
    int sockfd;
    struct sockaddr_in servaddr;
    FILE *myf = fopen("conversacionCliente.txt", "a");
    time_t t;
    struct tm *tm;
    char hora[100];
    char *tmp;
    char sendline[100] = "Hola, soy el servidor\n";

    sockfd = socket(AF_INET, SOCK_STREAM, 0); //crea el socket
    bzero(&servaddr, sizeof(servaddr)); //inicializa la estructura servaddr a 0

    servaddr.sin_family = AF_INET; //asigna la familia de direcciones
    servaddr.sin_port = htons(22000); //asigna el puerto

    inet_pton(AF_INET, "192.168.1.13", &(servaddr.sin_addr)); //asigna la direccion IP
    connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)); //conecta el socket al servidor

    printf("\n\n\t\t--------Se inicio el chat--------\n\n");
    fputs("\n\n\t\t--------Se inicio el chat--------\n\n", myf);

    while(!strstr(cadena, "adios") && !strstr(sendline, "adios")){
        bzero(cadena, 100);
        t = time(NULL);
        tm = localtime(&t);
        strftime(hora, 100, "yo %H :%M -> ", tm); 
        printf("%s", hora);
        gets(sendline);
        tmp = strcat(hora, cadena);
        fputs(tmp,myf);
        write(sockfd, sendline, strlen(sendline)+1);

        if (!strstr(cadena, "adios")){
            strftime(hora, 100, "\n otro usuario (%H :%M) -> ", tm);
            read(sockfd, cadena, 100);
            tmp = strcat(hora, sendline);
            printf("%s", tmp);
            fputs(tmp,myf);
        }
    }
    printf("\n\n Conversacion finalizada");
    fputs("\n Se genero el archivo -> conversacionCliente.txt", myf);
    fclose(myf);
}    