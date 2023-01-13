#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <utmp.h>
#include <fcntl.h>

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
    int fdScriere = open("canal1", O_WRONLY);
    if (fdScriere  == -1) 
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
    int fdCitire = open("canal2", O_RDONLY);
    if (fdCitire  == -1) 
    {
        perror("open2");
        exit(EXIT_FAILURE);
    }

    char comanda_tastatura[150];
    comanda_tastatura[0] = '\0';
    int caract_comanda_tastatura=0;
    int ok=1;

    while (ok==1) { 

        caract_comanda_tastatura = read(0,comanda_tastatura,150);
        if(caract_comanda_tastatura==-1)
        {
            perror("read1");
            exit(EXIT_FAILURE);
        }
        comanda_tastatura[caract_comanda_tastatura]='\0';

        if(strcmp(comanda_tastatura,"quit\n")==0)
            ok=0;        
        else
        {
            if(write(fdScriere, comanda_tastatura, strlen(comanda_tastatura))==-1)
            {
                perror("write1");
                exit(EXIT_FAILURE);
            }
            char raspuns_primit[300];
            raspuns_primit[0]='\0';
            int caract_raspuns_primit = 0;
            caract_raspuns_primit=read(fdCitire,raspuns_primit,300);
            if(caract_raspuns_primit==-1)
            {
                perror("read2");
                exit(EXIT_FAILURE);
            }
            raspuns_primit[caract_raspuns_primit]='\0';
            printf("%s\n",raspuns_primit);             
        }
    }
    close(fdScriere);
    close(fdCitire);
}