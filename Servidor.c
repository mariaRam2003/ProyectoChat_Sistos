#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdioh>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>


void main(void){
    char cadena[100];
    int listen_fd, comm_fd;
    struct sockaddr_in servaddr;
    FILE *myf = fopen("conversacionServidor.txt", "a");
    time_t t;
    struct tm *tm;
    char hora[100];
    char *tmp;
    char sendline[100] = "Hola, soy el servidor\n";

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

    while(!strstr(cadena, "adios") && !strstr(sendline, "adios")){
        bzero(cadena, 100);
        t = time(NULL);
        tm = localtime(&t);
        strftime(hora, 100, "\n otro usuario (%H :%M) -> ", tm);

        read(comm_fd, cadena, 100);
        tmp = strcat(hora, cadena);
        printf("%s", tmp);
        fputs(tmp,myf);
        if (!strstr(cadena, "adios")){
            strftime(hora, 100, "yo %H :%M -> ", tm);
            printf("%s", hora);
            gets(sendline);
            tmp = strcat(hora, sendline);
            write(comm_fd, sendline, strlen(sendline)+1);
            fputs(tmp,myf);
        }
    }
    printf("\n\n Conversacion finalizada");
    fputs("\n Se genero el archivo -> conversacionServidor.txt", myf);
    fclose(myf);
}
