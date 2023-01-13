#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utmp.h>
#include <sys/socket.h>

int logat = 0;

int main() 
{
    if (access("canal1", F_OK) == -1) // verifica daca "nu" exista canal1
    { 
        if (mknod("canal1", S_IFIFO | 0666, 0) == -1) // creeaza canal1 si verifica erorile
        { 
            perror("fifo1");
            exit(EXIT_FAILURE);
        }
    }
    int fdCitire = open("canal1", O_RDONLY);
    if (fdCitire == -1) 
    {
        perror("open1");
        exit(EXIT_FAILURE);
    }

    if (access("canal2", F_OK) == -1) // verifica daca "nu" exista canal2
    { 
        if (mknod("canal2", S_IFIFO | 0666, 0) == -1) // creeaza canal2 si verifica erorile
        { 
            perror("fifo2");
            exit(EXIT_FAILURE);
        }
    }
    int fdScriere = open("canal2", O_WRONLY);
    if (fdScriere == -1) 
    {
        perror("open2");
        exit(EXIT_FAILURE);
    }

    char comanda_primita[150]; // comanda primita din client
    int caract_comanda_primita = 0;
    char user_status[150]; // status user 
    int caract_user_status=0;
    char users[150]; // useri 
    int caract_users=0;

    do {
        caract_comanda_primita = read(fdCitire, comanda_primita ,150);
        if(caract_comanda_primita==-1)
        {
            perror("read1");
            exit(EXIT_FAILURE);
        }
        comanda_primita[caract_comanda_primita] = '\0';
        if(comanda_primita[0]=='l'&&comanda_primita[1]=='o'&&comanda_primita[2]=='g'&&comanda_primita[3]=='i'&&comanda_primita[4]=='n'&&comanda_primita[5]==' '&&comanda_primita[6]==':'&&comanda_primita[7]==' ') //comanda login
        {   
            pid_t executie1 = fork();
            if(executie1==-1)
            {
                perror("fork1");
                exit(EXIT_FAILURE);
            }
            if(executie1 == 0)     //proces copil
            {
                char nume[32];
                strcpy(nume,comanda_primita);
                strcpy(nume,nume+8);     // stergerea "login : ", pastram doar numele
                nume[strlen(nume)-1]='\0';  

                int fdNume;
                fdNume=open("names.txt", O_RDONLY);
                if(fdNume==-1)
                {
                    perror("open3");
                    exit(EXIT_FAILURE);
                }
                char nume_citite[1000];
                nume_citite[0]='\0';
                int caract_nume_citite = 0;
                caract_nume_citite=read(fdNume,nume_citite,1000);
                if(caract_nume_citite==-1)
                {
                    perror("read2");
                    exit(EXIT_FAILURE);
                }
                nume_citite[caract_nume_citite]='\0';

                char *pointer = strtok(nume_citite,"\n");
                while(pointer)
                {   
                    if(strcmp(nume,pointer)==0)
                    {
                        close(fdNume);
                        exit(10);
                    }
                    pointer=strtok(NULL,"\n");
                }
                close(fdNume);
                exit(-1);
            }
            else //proces parinte
            {
                int stat_exec;
                waitpid(executie1,&stat_exec,0);
                if(WEXITSTATUS(stat_exec)==10)
                {
                    if(logat==0)
                    {
                        logat=1;
                        if(write(fdScriere,"V-ati conectat cu succes.",strlen("V-ati conectat cu succes."))==-1)
                        {
                            perror("write1");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if(write(fdScriere,"Sunteti deja logat.",strlen("Sunteti deja logat."))==-1)
                        {
                            perror("write2");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else
                {   if(logat==0)
		    {	
		    	if(write(fdScriere,"Numele nu a fost gasit.",strlen("Numele nu a fost gasit."))==-1)
			{
			    perror("write3");
			    exit(EXIT_FAILURE);
			}
		    }
		    else
		    	if(write(fdScriere,"Sunteti deja logat.",strlen("Sunteti deja logat."))==-1)
                        {
                            perror("write4");
                            exit(EXIT_FAILURE);
                        }
                }
                wait(NULL);
            }
        }
        else 
            if(strcmp(comanda_primita,"logout\n") == 0) // comanda logout
            {
                if(logat==1)
                {
                    logat=0;
                    if(write(fdScriere, "V-ati deconectat cu succes.", strlen("V-ati deconectat cu succes"))==-1)
                    {
                        perror("write5");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                    if(write(fdScriere,"Nu sunteti conectat.", strlen("Nu sunteti conectat."))==-1)
                    {
                        perror("write6");
                        exit(EXIT_FAILURE);
                    }
            }
            else 
                if(strcmp(comanda_primita,"get-logged-users\n")==0) // comanda get-logged-users
                {    
                    char get_logged[150];
                    get_logged[0]='\0';
                    if( logat==1 )
                    {
                        int socket1[2]; // write 0 , read 1
                        if(socketpair(AF_UNIX, SOCK_STREAM, 0, socket1)==-1)
                        {
                            perror("socket1");
                            exit(EXIT_FAILURE);
                        }
                        pid_t executie2 = fork();
                        if(executie2==-1)
                        {
                            perror("fork2");
                            exit(EXIT_FAILURE);
                        }
                        if( executie2 ==0 ) //proces copil
                        {
                            close(socket1[1]);
                            struct utmp *info;
                            setutent();
                            info=getutent();
                            while(info)
                            {
                                if(info->ut_type==USER_PROCESS)
                                {
                                    
                                    time_t time;
                                    strcpy(get_logged,&info->ut_user);
                                    strcat(get_logged,"   ");
                                    strcat(get_logged, &info->ut_host);
                                    strcat(get_logged,"   ");
                                    time=&info->ut_tv.tv_sec;
                                    //strcat(get_logged, ctime(time));
                                    //strcat(get_logged,"   ");
                                    get_logged[strlen(get_logged)]='\0';
                                    write(socket1[0],get_logged,150);
                                }
                                info=getutent();
                            }
                            close(socket1[0]);
                            exit(2);
                        }
                        else // proces parinte
                        {
                            close(socket1[0]);
                            char socket_primit[150];
                            socket_primit[0]='\0';
                            int status;
                            waitpid(executie2,&status,0);
                            if(WEXITSTATUS(status)==2)
                            {
                                int caract_socket_primit=read(socket1[1],socket_primit, 150);
                                if(caract_socket_primit==-1)
                                {
                                    perror("read3");
                                    exit(EXIT_FAILURE);
                                }
                                if(write(fdScriere, socket_primit, caract_socket_primit)==-1)
                                {
                                    perror("write7");
                                    exit(EXIT_FAILURE);
                                }
                                close(socket1[1]);
                            }
                            wait(NULL);
                        }
                    }
                    else
                        if(write(fdScriere,"Nu se poate executa deoarece nu sunteti logat.",strlen("Nu se poate executa deoarece nu sunteti logat."))==-1)
                        {
                            perror("write8");
                            exit(EXIT_FAILURE);
                        }
                }
                else
                    if(strstr(comanda_primita,"get-proc-info : ")!=0) // comanda get-proc-info : pid
                    {
                        int fd1[2];  
                        if(pipe(fd1)==-1)
                        {
                            perror("pipe1");
                            exit(EXIT_FAILURE);
                        }
                        pid_t executie3 = fork();
                        if(executie3==-1)
                        {
                            perror("fork3");
                            exit(EXIT_FAILURE);
                        }
                        if( executie3 == 0 ) //proces copil
                        {
                            close(fd1[0]);
                            if(logat == 1)
                            {
                                char pid[5];
                                pid[0]='\0';
                                strcpy(pid,comanda_primita);
                                strcpy(pid,pid+16); // sterge "get-proc-info : " , ramane doar pid-ul
                                pid[strlen(pid)-1]='\0';
                                char sursa[50]; 
                                sursa[0]='\0';
                                strcat(sursa,"/");
                                strcat(sursa,"proc");
                                strcat(sursa,"/");
                                strcat(sursa,pid);
                                strcat(sursa,"/");
                                strcat(sursa,"status");

                                int fd_usernames= open(sursa,O_RDONLY);
                                if(fd_usernames == -1)
                                    if(write(fd1[1],"Procesul nu exista.",strlen("Procesul nu exista."))==-1)
                                    {
                                        perror("write9");
                                        exit(EXIT_FAILURE);
                                    }
                                char info_proces[1000]; 
                                info_proces[0] = '\0'; 
                                int caract_info_proces= read(fd_usernames,info_proces,1000); 
                                if(caract_info_proces==-1)
                                {
                                    perror("read4");
                                    exit(EXIT_FAILURE);
                                }
                                info_proces[caract_info_proces]='\0';

                                do 
                                {
                                    char *pointer = strtok(info_proces,"\n");
                                    while(pointer)
                                    {
                                        if(strstr(pointer,"Name")!=0 || strstr(pointer,"State")!=0 || strstr(pointer,"PPid")!=0 || strstr(pointer,"VmSize")!=0 || strstr(pointer,"Uid")!=0)  //de adaugat campurile lipsa
                                        {
                                            if(write(fd1[1],pointer,strlen(pointer))==-1)
                                            {
                                                perror("write10");
                                                exit(EXIT_FAILURE);
                                            }
                                            if(write(fd1[1],"\n",1)==-1)
                                            {
                                                perror("write11");
                                                exit(EXIT_FAILURE);
                                            }
                                        }
                                        pointer=strtok(NULL,"\n");
                                    }
                                    caract_info_proces = read(fd_usernames,info_proces,1000); 
                                    if(caract_info_proces==-1)
                                    {
                                        perror("read5");
                                        exit(EXIT_FAILURE);
                                    }
                                    info_proces[caract_info_proces]='\0';
                                } while(caract_info_proces);
                            }
                            else 
                                if(write(fd1[1],"Nu se poate executa deoarece nu sunteti logat.",strlen("Nu se poate executa deoarece nu sunteti logat."))==-1)
                                {
                                    perror("write12");
                                    exit(EXIT_FAILURE);
                                }
                        }
                        else  //proces parinte
                        { 
                            close(fd1[1]);
                            char aux[256]; 
                            aux[0]='\0'; 
                            int caract_aux=read(fd1[0],aux,256);
                            if(caract_aux==-1)
                            {
                                perror("read6");
                                exit(EXIT_FAILURE);
                            }
                            aux[caract_aux]='\0';
                            if(write(fdScriere,aux,caract_aux)==-1)
                            {
                                perror("write13");
                                exit(EXIT_FAILURE);
                            }
                            wait(NULL);
                        }
                    } 
                    else
                    {
                        if(write(fdScriere,"Comanda invalida.", strlen("Comanda invalida."))==-1)
                        {
                            perror("write14");
                            exit(EXIT_FAILURE);
                        }
                    }
    } while(caract_comanda_primita);    
    close(fdCitire);
    close(fdScriere);
}