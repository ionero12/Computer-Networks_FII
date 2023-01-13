#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

typedef struct thData{
    char username[50];
    int fd;
    int id;
} thData;

thData *clients[10];

char mesaj_de_la_client[300];
char mesaj_pentru_client[300];
int view_mess_status=0;

#define PORT 5555

static void *treat(void *); //functia executata de ficare thread ce realizeaza comunicarea cu clientu
void raspunde(void *);


int message_id(char fisier[])
{
    int id=0;
    FILE *file=fopen(fisier,"r");
    if(file == NULL)
    {
        perror("[server] Eroare la fopen(1).\n");
        return 0;
    }
    else 
    {
        char line[200];
        while(fgets(line,sizeof(line),file)!=NULL)
        {
            id+=1;
        }
    }
    fclose(file);
    return id;
}

void add_to_struct(thData *client)
{
    int i;
    for(i=0; i<10; i++)
        if(clients[i]==NULL)
        {
            clients[i]=client;
            break;
        }
}

void remove_from_struct(thData *client)
{
    int i;
    for(i=0; i<10; i++)
        if(clients[i]==client)
        {
            clients[i]=NULL;
            break;
        }
}

int user_exist(char user[])
{
    FILE *file=fopen("users.txt","r");
    if(file == NULL)
    {
        perror("[server] Eroare la fopen(6).\n");
        return errno;
    }
    else 
    {
        char line[200];
        while(fgets(line,sizeof(line),file) != NULL)
        {
            char username[50];
            username[0]='\0';
            int i=0;
            while(line[i]!='@')
            {
                username[i]=line[i];
                i+=1;
            }
            username[i]='\0';
            if(strcmp(user,username)==0)
                return 1;
        }
    }
    fclose(file);
    return 0;
}

int login_function(char email[], char parola[])
{
    int bytes;
    FILE *file=fopen("users.txt","r");
    if(file == NULL)
    {
        perror("[server] Eroare la fopen(2).\n");
        return errno;
    }
    else 
    {
        char line[200];
        while(fgets(line,sizeof(line),file) != NULL)
        {
            char *e=strtok(line," ");
            char *p=strtok(NULL,"\n");
            if((strcmp(email,e)==0) && (strcmp(parola,p)==0))
                return 1;
        }
    }
    fclose(file);                     
    return 0;
}

int register_function(char email[], char parola[])
{
    int bytes;
    FILE *file=fopen("users.txt", "a+");
    if(file == NULL)
    {
        perror("[server] Eroare la open(3).\n");
        return errno;
    }
    else 
    {
        char line[200];
        while(fgets(line,sizeof(line),file) != NULL)
        {
            char *e=strtok(line," ");
            char *p=strtok(NULL,"\n");
            if((strcmp(email,e)==0) && (strcmp(parola,p)==0))
                return 0; //acest cont exista deja
        }
        if(strchr(email,'@')==NULL)
            return -1; //email invalid
        char utilizator[200];
        bzero(utilizator,200);
        utilizator[0]='\0';
        strcat(utilizator, email);
        strcat(utilizator, " ");
        strcat(utilizator, parola);
        strcat(utilizator, "\n");
        bytes=fprintf(file,"%s", utilizator);
        if(bytes<=0)
        {
            perror("[server] Eroare la fprintf(1).\n");
            return errno;
        }
    }
    fclose(file);
    return 1;
}

int existing_users_function()
{
    FILE *file=fopen("users.txt","r");
    if(file == NULL)
    {
        perror("[server] Eroare la fopen(4).\n");
        return 0;
    }
    else 
    {
        char line[200], lista[1000];
        bzero(lista,1000);
        lista[0]='\n';
        strcat(lista,"Utilizatorii existenti sunt: \n");
        while(fgets(line,sizeof(line),file) != NULL)
        {
            char username[50];
            username[0]='\0';
            int i=0;
            while(line[i]!='@')
            {
                username[i]=line[i];
                i+=1;
            }
            username[i]='\0';
            strcat(lista,username);
            strcat(lista, "\n");
        }
        strcat(lista,"\n");
        strcpy(mesaj_pentru_client,lista);
    }
    fclose(file);
    return 1;
}

int online_users_function()
{
    char online_users[500];
    bzero(online_users,500);
    strcat(online_users,"\n");
    int i;
    for(i=0; i<10; i++)
        if(clients[i]!=NULL)
        {
            strcat(online_users, clients[i]->username);
            strcat(online_users, "\n");
        }
    strcat(online_users,"\n");
    strcpy(mesaj_pentru_client,online_users);
    return 1;
}

int send_message(char from_user[], char to_user[], char mesaj[])
{
    char nume_fis1[150], nume_fis2[150], nume_fis3[150];
    bzero(nume_fis1,150);
    bzero(nume_fis2,150);
    bzero(nume_fis3,150);
    int bytes;
    int id;
    if(user_exist(to_user)==1)
    {
        if(strcmp(from_user,to_user)<0)
            bytes=sprintf(nume_fis1, "%s-%s.txt", from_user,to_user);
        else
            if(strcmp(from_user,to_user)>0)
                bytes=sprintf(nume_fis1,"%s-%s.txt", to_user, from_user);
            else
                return -1;//au acelasi nume
        if(bytes<=0)
        {
            perror("[server] Eroare la sprintf(4).\n");
            return errno;
        }
        bytes=sprintf(nume_fis2, "%s_sent.txt", from_user);
        if(bytes<=0)
        {
            perror("[server] Eroare la sprintf(5).\n");
            return errno;
        }
        bytes=sprintf(nume_fis3, "%s_received.txt", to_user);
        if(bytes<=0)
        {
            perror("[server] Eroare la sprintf(6).\n");
            return errno;
        }
        FILE *file1=fopen(nume_fis1, "a+");
        FILE *file2=fopen(nume_fis2, "a+");
        FILE *file3=fopen(nume_fis3, "a+");
        if(file1 == NULL || file2 == NULL || file3 == NULL)
        {
            perror("[server] Eroare la fopen(9).\n");
            return errno;
        }
        else
        {
            bytes=fprintf(file1, "%s: %s\n",from_user, mesaj);
            if(bytes<=0)
            {
                perror("[server] Eroare la fprintf(2).\n");
                return errno;
            }
            bytes=fprintf(file2, "sent_to %s: %s\n", to_user, mesaj);
            if(bytes<=0)
            {
                perror("[server] Eroare la fprintf(3).\n");
                return errno;
            }
            id=message_id(nume_fis3);
            bytes=fprintf(file3, "[id: %d] %s: %s\n",id, from_user, mesaj);
            if(bytes<=0)
            {
                perror("[server] Eroare la fprintf(4).\n");
                return errno;
            }
        }
        char msj_edt[300];
        bzero(msj_edt,300);
        sprintf(msj_edt, "[id: %d] from %s: %s\n",id,from_user,mesaj);
        for(int i=0;i<10;i++)
        {   
            if(clients[i]!=NULL && strcmp(clients[i]->username,to_user)==0)            
            {
                if(send(clients[i]->fd, msj_edt, strlen(msj_edt), 0)==-1)
                {
                    perror("Eroare la write(1) catre client.\n");
                    break;
                }
            }
        }
        fclose(file1);
        fclose(file2);
        fclose(file3);
        return 1;
    }
    else  
        return 0; //user nu exista
}

int reply_message_function(char user[], char id[], char text[])
{
    int bytes, id_mes=atoi(id);
    char fisier[150];
    bzero(fisier,150);
    bytes=sprintf(fisier,"%s_received.txt",user);
    if(bytes<=0)
    {
        perror("[server] Eroare la sprintf(7).\n");
        return errno;
    }
    int max_id=message_id(fisier);
    if(id_mes>max_id)
    {
        return -1;//nu exista id ul respectiv
    }
    FILE *file=fopen(fisier,"r");
    if(file == NULL)
    {
        perror("[server] Eroare la fopen(10).\n");
        return 0; // nu ai la ce mesaje sa raspunzi
    }
    else 
    {
        char sablon[100], to_user[50];
        bzero(sablon,100);
        bzero(to_user,50);
        int k=0;
        sprintf(sablon,"[id: %d]",id_mes);
        char line[200];
        while(fgets(line,sizeof(line),file)!=NULL)
        {
            if(strstr(line,sablon)!=NULL)
            {
                for(int i=0;i<=strlen(line)-1;i++)
                    if(line[i]==']')
                        for(int j=i+2;j<=strlen(line)-1 && line[j]!=':';j++)
                            to_user[k++]=line[j];
            }
        }
        int ok=send_message(user,to_user,text);
        
    }
    fclose(file);
    return 1;
}

int view_received_messages_function(char user[])
{
    int bytes;
    char fisier[150];
    bzero(fisier,150);
    bytes=sprintf(fisier,"%s_received.txt",user);
    if(bytes<=0)
    {
        perror("[server] Eroare la sprintf(1).\n");
        return errno;
    }
    FILE *file=fopen(fisier,"r");
    if(file == NULL)
    {
        perror("[server] Eroare la fopen(5).\n");
        return 0;
    }
    else 
    {
        char line[200], mesaje_primite[1000];
        bzero(mesaje_primite,1000);
        mesaje_primite[0]='\n';
        while(fgets(line,sizeof(line),file)!=NULL)
        {
            strcat(mesaje_primite,line);
        }
        strcat(mesaje_primite,"\n");
        strcpy(mesaj_pentru_client,mesaje_primite);
    }
    fclose(file);
    //view_mess_status=1;
    return 1;
}

int view_history_function(char history_from[],char history_with[])
{
    int bytes;
    char fisier[150], mesaje_primite[1000];
    bzero(fisier,150);
    bzero(mesaje_primite,1000);
    if(strcmp(history_with,"eu")==0)
    {
        bytes=sprintf(fisier,"%s_sent.txt",history_from);
        if(bytes<=0)
        {
            perror("[server] Eroare la sprintf(2).\n");
            return errno;
        }
        FILE *file=fopen(fisier,"r");
        if(file == NULL)
        {
            perror("[server] Eroare la fopen(7).\n");
            return 0;
        }
        else 
        {
            char line[200];
            bzero(mesaje_primite,1000);
            mesaje_primite[0]='\n';
            while(fgets(line,sizeof(line),file)!=NULL)
            {
                strcat(mesaje_primite,line);
            }
            strcat(mesaje_primite,"\n");
        }
        fclose(file);
    }
    else  
    {
        if(user_exist(history_with)==1)
        {   
            if(strcmp(history_from,history_with)<0)
                bytes=sprintf(fisier, "%s-%s.txt", history_from,history_with);
            else
                if(strcmp(history_from,history_with)>0)
                    bytes=sprintf(fisier,"%s-%s.txt", history_with, history_from);
                else
                    return 0;
            if(bytes<=0)
            {
                perror("[server] Eroare la sprintf(3).\n");
                return errno;
            }
            FILE *file=fopen(fisier,"r");
            printf("2\n");
            if(file == NULL)
            {
                perror("[server] Eroare la fopen(8).\n");
                return 0;
            }
            else 
            {
                char line[200];
                bzero(mesaje_primite,1000);
                mesaje_primite[0]='\n';
                while(fgets(line,sizeof(line),file)!=NULL)
                {
                    strcat(mesaje_primite,line);
                }
                strcat(mesaje_primite,"\n");
            }
            fclose(file);
        }
        else
            return -1;
    }
    strcpy(mesaj_pentru_client,mesaje_primite);
    return 1;
}


int main()
{
    struct sockaddr_in server; //struct folosita de server
    struct sockaddr_in from;
    int sd; // descriptor de socket
    pthread_t th[10]; //identif thread-urilor care se vor crea
    int id=0;

    sd=socket(AF_INET, SOCK_STREAM, 0);
    if(sd==-1)
    {
        perror("[server] Eroare la socket(1).\n");
        return errno;
    }

    int opt=1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&server,sizeof(server));
    bzero(&from,sizeof(from));

    server.sin_family=AF_INET;
    server.sin_addr.s_addr=htonl(INADDR_ANY);
    server.sin_port=htons(PORT);

    if(bind(sd, (struct sockaddr *)&server, sizeof(server))==-1) // se ataseaza socketul
    {
        perror("[server] Eroare la bind(1).\n");
        return errno;
    }

    if(listen(sd, 10)==-1)  // asteptam clienti, maxim 10
    {
        perror("[server] Eroare la listen(1).\n");
        return errno;
    }

    while(1)
    {
        int client;
        thData *cli; //param functia exec de thread
        socklen_t len=sizeof(from);

        printf("Asteptam la portul %d\n", PORT);
        fflush(stdout);

        client=accept(sd, (struct sockaddr *) &from, &len);
        if(client<0)
        {
            perror("[server] Eroare la accept(1).\n");
            return errno;
        }
        cli=(thData *)malloc(sizeof(thData));
        cli->fd=client;
        cli->id=id++;
        pthread_create(&th[id],NULL,&treat,cli);
    }
}

static void *treat(void *arg)
{
    struct thData clienti;
    clienti=*((struct thData*)arg);
    printf("[thread]- %d - Asteptam mesajul..\n",clienti.id);
    fflush(stdout);
    pthread_detach(pthread_self());
    raspunde((struct thData*)arg);
    close((intptr_t)arg);
    return(NULL);
}

void raspunde(void *arg)
{
    thData *clienti=(thData *)arg;
    int bytes;
    int logged_status=0, view_msg_status=0;
    bzero(mesaj_pentru_client,300);
    strcpy(mesaj_pentru_client,"\nIntroduceti comanda dorita: \n");
    bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
    if(bytes<=0)
    {
        printf("[Thread %d]\n",clienti->id);
        perror("[server] Eroare la write(2) catre client.\n");
    }
    while(1)
    { 
        bzero(mesaj_pentru_client,300);        
        bytes=read(clienti->fd,mesaj_de_la_client,sizeof(mesaj_de_la_client));
        if(bytes<=0)
        {
            printf("[Thread %d]\n",clienti->id);
            perror ("Eroare la read(1) de la client.\n");
        }
        printf("comanda e %s\n", mesaj_de_la_client);
        if(strcmp(mesaj_de_la_client,"Login")==0)
        {
            if(logged_status==0)
            {
                char email[300], parola[300];
                bzero(email,300);
                bzero(parola,300);
                strcpy(mesaj_pentru_client,"Introduceti adresa de email: \n");
                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                if(bytes <= 0)
                {
                    printf("[Thread %d]\n",clienti->id);
                    perror ("Eroare la write(3) catre client.\n");
                }
                bytes=read(clienti->fd,email,sizeof(email));
                if(bytes <= 0)
                {
                    printf("[Thread %d]\n",clienti->id);
                    perror ("Eroare la read(2) de la client.\n");
                }
                strcpy(mesaj_pentru_client,"Introduceti parola: \n");
                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                if(bytes <= 0)
                {
                    printf("[Thread %d]\n",clienti->id);
                    perror ("Eroare la write(4) catre client.\n");
                }
                bytes=read(clienti->fd,parola,sizeof(parola));
                if(bytes <= 0)
                {
                    printf("[Thread %d]\n",clienti->id);
                    perror ("Eroare la read(3) de la client.\n");
                }
                int ok=login_function(email, parola);
                if(ok==1)
                {
                    char username[50];
                    int i=0;
                    username[0]='\0';
                    while(email[i]!='@')
                    {
                        username[i]=email[i];
                        i+=1;
                    }
                    username[i]='\0';

                    int conectat=0;
                    for(int i=0;i<10;i++)
                        if(clients[i]!=NULL)
                            if(strcmp(clients[i]->username,username)==0)
                                conectat=1;
                    if(conectat==1)
                        strcpy(mesaj_pentru_client,"\nSunteti deja conectat in alt terminal.\n");
                    else
                    {
                        add_to_struct(clienti);
                        logged_status=1;
                        strcpy(clienti->username,username);

                        char fisier[150];
                        sprintf(fisier,"%s_received.txt",clienti->username);
                        int id=message_id(fisier);
                        char id_c[5];
                        sprintf(id_c,"%d",id);
                        strcpy(mesaj_pentru_client, "\nV-ati conectat cu succes!\n\n Lista utilizatori\n Lista utilizatori online\n Trimiteti mesaj\n Reply\n Vizualizati mesajele primite\n Vizualizati istoricul mesajelor\n Logout\n Exit\n\n");
                        strcat(mesaj_pentru_client, "Aveti ");
                        strcat(mesaj_pentru_client, id_c);
                        strcat(mesaj_pentru_client," mesaje necitite!\n\n");
                        
                        printf("Utilizatorul %s s-a conectat la server.\n",email);
                        printf("Utilizatorul %s este online.\n",clienti->username);
                    }
                }
                else   
                {
                    strcpy(mesaj_pentru_client,"\nLogarea a esuat! Email sau parola gresite.\n");
                }
                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                if(bytes<=0)
                {
                    printf("[Thread %d]\n",clienti->id);
                    perror ("Eroare la write(5) catre client.\n");
                }
            }
            else  
            {
                strcpy(mesaj_pentru_client,"Sunteti deja conectat cu alt cont!\n");
                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                if(bytes<=0)
                {
                    printf("[Thread %d]\n",clienti->id);
                    perror ("Eroare la write(6) catre client.\n");
                }
            }
        }
        else 
            if(strcmp(mesaj_de_la_client, "Register")==0)
            {
                if(logged_status==0)
                {   
                    char email[300], parola[300];
                    bzero(email,300);
                    bzero(parola,300);
                    strcpy(mesaj_pentru_client, "\nIntroduceti o adresa de email: \n");
                    bytes=write(clienti->fd, mesaj_pentru_client, sizeof(mesaj_pentru_client));
                    if(bytes <= 0)
                    {
                        printf("[Thread %d]\n",clienti->id);
                        perror("[server] Eroare la write(7) catre client.\n");
                    }
                    bytes=read(clienti->fd,email,sizeof(email));
                    if(bytes <= 0)
                    {
                        printf("[Thread %d]\n",clienti->id);
                        perror("[server] Eroare la read(4) de la client.\n");
                    }
                    strcpy(mesaj_pentru_client, "Introduceti o parola: \n");
                    bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                    if(bytes <= 0)
                    {
                        printf("[Thread %d]\n",clienti->id);
                        perror("[server] Eroare la write(8) catre client.\n");
                    }
                    bytes=read(clienti->fd,parola,sizeof(parola));
                    if(bytes <= 0)
                    {
                        printf("[Thread %d]\n",clienti->id);
                        perror("[server] Eroare la read(5) de la client.\n");
                    }
                    int ok=register_function(email, parola);
                    if(ok==-1)
                    {
                        strcpy(mesaj_pentru_client,"\nEmail invalid.\n");
                    }
                    else  
                        if(ok==0)
                        {
                            strcpy(mesaj_pentru_client,"\nAcest cont exista deja.\n");
                        }
                        else
                        {
                            add_to_struct(clienti);
                            logged_status=1;

                            char username[50];
                            int i=0;
                            username[0]='\0';
                            while(email[i]!='@')
                            {
                                username[i]=email[i];
                                i+=1;
                            }
                            username[i]='\0';
                            strcpy(clienti->username,username);

                            strcpy(mesaj_pentru_client, "\nV-ati creeat contul cu succes!\n\n Lista utilizatori\n Lista utilizatori online\n Trimiteti mesaj\n Reply\n Vizualizati mesajele primite\n Vizualizati istoricul mesajelor\n Logout\n Exit\n\n");
                            printf("\nUtilizatorul %s si-a creeat cont cu succes.\n", email);
                            printf("Utilizatorul %s este online.\n",clienti->username);
                        }
                    bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                    if(bytes<=0)
                    {
                        printf("[Thread %d]\n",clienti->id);
                        perror("[server] Eroare la write(9) catre client.\n");
                    }      
                }
                else 
                {
                    strcpy(mesaj_pentru_client, "Sunteti deja conectat cu alt cont.\n");
                    bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                    if(bytes<=0)
                    {
                        printf("[Thread %d]\n",clienti->id);
                        perror("[server] Eroare la write(10) catre client.\n");
                    }
                }
            }
            else
                if(strcmp(mesaj_de_la_client, "Lista utilizatori")==0)
                {
                    if(logged_status==0)
                    {
                        strcpy(mesaj_pentru_client, "\nTrebuie sa fiti conectat pentru a executa comanda\n\n Login\n Register\n Exit\n\n");
                        bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                        if(bytes<=0)
                        {
                            printf("[Thread %d]\n",clienti->id);
                            perror("[server] Eroare la write(11) catre client.\n");
                        }
                    }
                    else
                    {   
                        int ok=existing_users_function();
                        if(ok==1)
                        {
                            bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                            if(bytes<=0)
                            {
                                printf("[Thread %d]\n",clienti->id);
                                perror("[server] Eroare la write(12) catre client.\n");
                            }
                        }
                    }
                }
                else  
                    if(strcmp(mesaj_de_la_client,"Lista utilizatori online")==0)
                    {
                        if(logged_status==0)
                        {
                            strcpy(mesaj_pentru_client, "\nTrebuie sa fiti conectat pentru a executa comanda\n\n Login\n Register\n Exit\n\n");
                            bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                            if(bytes<=0)
                            {                                    
                                printf("[Thread %d]\n",clienti->id);
                                perror("[server] Eroare la write(13) catre client.\n");
                            }
                        }
                        else  
                        {
                            int ok=online_users_function();
                            bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                            if(bytes<=0)
                            {                                    
                                printf("[Thread %d]\n",clienti->id);
                                perror("[server] Eroare la write(14) catre client.\n");
                            }
                        }
                    }
                    else  
                        if(strcmp(mesaj_de_la_client,"Trimiteti mesaj")==0)
                        {
                            if(logged_status==0)
                            {
                                strcpy(mesaj_pentru_client, "\nTrebuie sa fiti conectat pentru a executa comanda\n\n Login\n Register\n Exit\n\n");
                                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                if(bytes<=0)
                                {                                    
                                    printf("[Thread %d]\n",clienti->id);
                                    perror("[server] Eroare la write(16) catre client.\n");
                                }
                            }
                            else 
                            {
                                char to_user[300], text[300];
                                bzero(to_user,300);
                                bzero(text,300);
                                strcpy(mesaj_pentru_client,"Introduceti user-ul careuia doriti sa-i trimiteti mesaj (user-ul este format doar din nume, fara '@gmail.com'): \n");
                                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                if(bytes<=0)
                                {
                                    printf("[Thread %d]\n",clienti->id);
                                    perror("[server] Eroare la write(17) catre client.\n");
                                }
                                bytes=read(clienti->fd,to_user,sizeof(to_user));
                                if(bytes<=0)
                                {
                                    printf("[Thread %d]\n",clienti->id);
                                    perror("[server] Eroare la read(6) de la client.\n");
                                }
                                strcpy(mesaj_pentru_client,"Introduceti textul pe care vreti sa-l trimiteti.\n");
                                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                if(bytes<=0)
                                {
                                    printf("[Thread %d]\n",clienti->id);
                                    perror("[server] Eroare la write(18) catre client.\n");
                                }
                                bytes=read(clienti->fd,text,sizeof(text));
                                if(bytes<=0)
                                {
                                    printf("[Thread %d]\n",clienti->id);
                                    perror("[server] Eroare la read(7) de la client.\n");
                                }
                                int ok=send_message(clienti->username,to_user,text);
                                if(ok==-1)
                                {  
                                    strcpy(mesaj_pentru_client,"\nNu va puteti trimtie mesaj singur.\n");
                                }
                                else  
                                    if(ok==0)
                                    {
                                        strcpy(mesaj_pentru_client,"\nUser-ul nu exista.\n");
                                    }
                                    else  
                                    {
                                        strcpy(mesaj_pentru_client, "\nMesaj trimis.\n");
                                    }
                                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                if(bytes<=0)
                                {
                                    printf("[Thread %d]\n",clienti->id);
                                    perror("[server] Eroare la write(19) catre client.\n");
                                }
                            }
                        }
                        else  
                            if(strcmp(mesaj_de_la_client,"Reply")==0)
                            {
                                char id[300],text[300];
                                bzero(id,300);
                                bzero(text,300);
                                strcpy(mesaj_pentru_client,"Introduceti id-ul mesajului la care doriti sa raspundeti (numar): \n");
                                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                if(bytes<=0)
                                {
                                    printf("[Thread %d]\n",clienti->id);
                                    perror("[server] Eroare la write(20) catre client.\n");
                                }
                                bytes=read(clienti->fd,id,sizeof(id));
                                if(bytes<=0)
                                {
                                    printf("[Thread %d]\n",clienti->id);
                                    perror("[server] Eroare la read(8) de la client.\n");
                                }
                                strcpy(mesaj_pentru_client,"Introduceti textul pe care vreti sa-l trimiteti.\n");
                                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                if(bytes<=0)
                                {
                                    printf("[Thread %d]\n",clienti->id);
                                    perror("[server] Eroare la write(21) catre client.\n");
                                }
                                bytes=read(clienti->fd,text,sizeof(text));
                                if(bytes<=0)
                                {
                                    printf("[Thread %d]\n",clienti->id);
                                    perror("[server] Eroare la read(9) de la client.\n");
                                }
                                int ok=reply_message_function(clienti->username,id, text);
                                if(ok==-1)
                                {
                                    strcpy(mesaj_pentru_client,"\nNu exista mesajul cu id-ul ales de dumneavoastra.\n");
                                }
                                else  
                                    if(ok==0)
                                    {
                                        strcpy(mesaj_pentru_client,"\nNu aveti la ce mesaje sa raspundeti.\n");
                                    }
                                    else  
                                    {
                                        strcpy(mesaj_pentru_client,"\nReply trimis.\n");
                                    }
                                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                if(bytes<=0)
                                {
                                    printf("[Thread %d]\n",clienti->id);
                                    perror("[server] Eroare la write(22) catre client.\n");
                                }
                            }
                            else  
                                if(strcmp(mesaj_de_la_client,"Vizualizati mesajele primite")==0)
                                {
                                    if(logged_status==0)
                                    {
                                        strcpy(mesaj_pentru_client, "\nTrebuie sa fiti conectat pentru a executa comanda\n\n Login\n Register\n Exit\n\n");
                                        bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                        if(bytes<=0)
                                        {
                                            printf("[Thread %d]\n",clienti->id);
                                            perror("[server] Eroare la write(23) catre client.\n");
                                        }
                                    }
                                    else  
                                    {
                                        int ok=view_received_messages_function(clienti->username);
                                        view_msg_status=1;
                                        if(ok==0)
                                        {
                                            strcpy(mesaj_pentru_client,"\nNu exista mesaje primite.\n");
                                        }
                                        bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                        if(bytes<=0)
                                        {
                                            printf("[Thread %d]\n",clienti->id);   
                                            perror("[server] Eroare la write(24) catre client.\n");
                                        }
                                    }
                                }
                                else  
                                    if(strcmp(mesaj_de_la_client,"Vizualizati istoricul mesajelor")==0)
                                    {
                                        if(logged_status==0)
                                        {
                                            strcpy(mesaj_pentru_client, "\nTrebuie sa fiti conectat pentru a executa comanda\n\n Login\n Register\n Exit\n\n");
                                            bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                            if(bytes<=0)
                                            {
                                                printf("[Thread %d]\n",clienti->id);   
                                                perror("[server] Eroare la write(25) catre client.\n");          
                                            }
                                        }
                                        else  
                                        {
                                            char history_with[300];
                                            strcpy(mesaj_pentru_client,"\nIntroduceti user-ul cu care doriti sa vizualizati istoricul conversatiei (user-ul este format doar din nume, fara '@gmail.com')\nPentru propriul istoric introduceti <eu>: \n");
                                            bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                            if(bytes<=0)
                                            {
                                                printf("[Thread %d]\n",clienti->id);   
                                                perror("[server] Eroare la write(26) catre client.\n");
                                            }
                                            bytes=read(clienti->fd,history_with,sizeof(history_with));
                                            if(bytes<=0)
                                            {
                                                printf("[Thread %d]\n",clienti->id);   
                                                perror("[server] Eroare la read(10) de la client.\n");
                                            }
                                            int ok=view_history_function(clienti->username, history_with);
                                            if(ok==-1)
                                            {
                                                strcpy(mesaj_pentru_client,"\nAcest user nu exista.\n");
                                            }
                                            else  
                                                if(ok==0)
                                                {
                                                    strcpy(mesaj_pentru_client,"\nNu exista mesaje.\n");
                                                }
                                            bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                            if(bytes<=0)
                                            {
                                                printf("[Thread %d]\n",clienti->id);   
                                                perror("[server] Eroare la write(27) catre client.\n");
                                            }
                                        }
                                    }
                                    else  
                                        if(strcmp(mesaj_de_la_client,"Logout")==0)
                                        {
                                            if(logged_status==0)
                                            {
                                                strcpy(mesaj_pentru_client, "\nNu sunteti conectat!\n\n Login\n Register\n Exit\n\n");
                                            }
                                            else 
                                            {
                                                printf("Utilzatorul %s e offline.\n",clienti->username);

                                                if(view_msg_status==1)
                                                {
                                                    char fisier[150];
                                                    sprintf(fisier,"%s_received.txt",clienti->username);
                                                    remove(fisier);
                                                }

                                                logged_status=0;
                                                remove_from_struct(clienti);
                                                strcpy(mesaj_pentru_client, "\nV-ati deconectat cu succes!\n\n Login\n Register\n Exit\n\n");
                                            }
                                            bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                            if(bytes<=0)
                                            {
                                                printf("[Thread %d]\n",clienti->id);   
                                                perror("[server] Eroare la write(28) catre client.\n");
                                            }
                                        }
                                        else  
                                            if(strcmp(mesaj_de_la_client,"Exit")==0)
                                            {
                                                printf("Utilizatorul %s e offline.\n", clienti->username);

                                                if(view_msg_status==1)
                                                {
                                                    char fisier[150];
                                                    sprintf(fisier,"%s_received.txt",clienti->username);
                                                    remove(fisier);
                                                }

                                                logged_status=0;
                                                remove_from_struct(clienti);
                                                break;
                                            }
                                            else 
                                            {
                                                strcpy(mesaj_pentru_client, "\nAceasta comanda nu exista\n\n");
                                                bytes=write(clienti->fd,mesaj_pentru_client,sizeof(mesaj_pentru_client));
                                                if(bytes<=0) 
                                                {
                                                    printf("[Thread %d]\n",clienti->id);   
                                                    perror("[server] Eroare la write(29) catre client.\n");
                                                }
                                            }
                                                
    }
}


