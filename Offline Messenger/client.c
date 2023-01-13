#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

int port; // port de conectare
int sd;
void write_msg();
void read_msg();

int main(int argc, char *argv[])
{
    int bytes;
    struct sockaddr_in server;
    char initial[52], char_id[4], *command, *username, *password;
    pthread_t send_thread, recv_thread;

    if(argc!=3)
    {
        printf("Sinxata: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    port=atoi(argv[2]);

    sd=socket(AF_INET,SOCK_STREAM,0);
    if(sd==-1)
    {
        perror("[client] Eroare la socket(1).\n");
        return errno;
    }
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=inet_addr(argv[1]);
    server.sin_port=htons(port);

    if(connect(sd, (struct sockaddr *) &server,sizeof(struct sockaddr))==-1)
    {
        perror("[client] Eroare la connect(1).\n");
        return errno;
    }

    printf("Login\n");
    printf("Register\n");
    printf("Exit\n\n");
    fflush(stdout);

    if(pthread_create(&send_thread, NULL, (void *)write_msg, NULL)!=0)
    {
        printf("[client] Eroare la pthread-create(1).\n");
        return errno;
    }

    if(pthread_create(&recv_thread, NULL, (void *)read_msg, NULL)!=0)
    {
        printf("[client] Eroare la pthread-create(2).\n");
        return errno;
    }

    while(1)
    {

    }
    
    close (sd);
}

void write_msg()
{
    char msg[300];
    int bytes;
    bzero(msg,300);

    while(1)
    {
        read(0,msg,sizeof(msg));
        msg[strlen(msg)-1]='\0';
        if(strcmp(msg, "Exit")==0)
        {
            bytes=write(sd, msg, sizeof(msg));
            if(bytes<=0)
            {
                perror("[client] Eroare la write(1) catre server.\n");
            }
            exit(0);
        }
        else
        {
            bytes=write(sd, msg, sizeof(msg));
            if(bytes<=0)
            {
                perror("Error writing to server.\n");
            }
        }
        bzero(msg, 300);
    }
}

void read_msg()
{
    char msg[300];
    int bytes;
    bzero(msg,300);

    while(1)
    {
        int bytes=read(sd, msg, sizeof(msg));
        if(bytes<=0)
        {
            perror("[client] Eroare la read(1) de la server.\n");
        }
        else
        {
            printf("%s", msg); 
            fflush(stdout);
        }
        bzero(msg, 300);
    }
}