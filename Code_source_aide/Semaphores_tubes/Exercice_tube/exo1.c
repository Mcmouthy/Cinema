#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "exo1.h"

int countNbChar(char* texte, char c, char cmaj)
{
    int nb=0;
    for (int i = 0; i < strlen(texte); ++i)
    {
        if (texte[i]==c || texte[i]==cmaj)
        {
            nb++;
        }
    }
    return nb;

}

void childChain(int i, char* texte, char* result,char* tabChar,char* tabCharMaj)
{
    int status;
    int tube2[2];
    pipe(tube2);
    int child=fork();
    switch(child)
    {
        case 0:
            dup2(tube2[0],0);
            if (i<24)
            {
                close(tube2[1]);
            }
            read(0,texte,sizeof(texte));
            int nb=countNbChar(texte,tabChar[i],tabCharMaj[i]);
            char strnum[12];
            result=texte;
            if(nb>0){
                result=strcat(result," ");
                sprintf(strnum,"%c:%d",tabCharMaj[i],nb);
                result=strcat(result,strnum);
            }
            if (i<25)
            {
                childChain(i+1, texte, result,tabChar,tabCharMaj);
            }else
            {
                printf("%s\n",result);
                exit(i);
            }
            exit(i);
            

            break;
        default:
            close(tube2[0]);
            dup2(tube2[1],1);
            write(1,texte,sizeof(texte));
            wait(&status);
            break;
    }
}

int main(int argc, char** argv)
{
    char line[100];
    char texte[256];
    char* result;
    int status;
    char* tabChar="abcdefghijklmnopqrstuvwxyz";
    char* tabCharMaj="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int tube[2];
    pipe(tube);
    int child=fork();
    switch(child)
    {
        case 0:
            dup2(tube[0],0);
            close(tube[1]);
            read(0,texte,sizeof(texte));
            int nb=countNbChar(texte,tabChar[0],tabCharMaj[0]);
            char strnum[12];
            result=texte;
            if(nb>0){
                result=strcat(result," ");
                sprintf(strnum,"%c:%d",tabCharMaj[0],nb);
                result=strcat(result,strnum);
            }
            childChain(1, texte, result,tabChar,tabCharMaj);
            exit(0);
            break;
        default:
            close(tube[0]);
            dup2(tube[1],1);
            FILE* fichier;
            fichier=fopen("text","r");

            if(fichier==NULL)
            {
                perror("Erreur ouverture fichier\n");
                exit(0);
            }else
            {
                if(fgets(texte,sizeof(line),fichier)==0)
                {
                    perror("erreur lecture\n");
                    exit(1);
                }
                write(1,texte,sizeof(texte));
                wait(&status);
            }
            break;
    }

    return 0;
}