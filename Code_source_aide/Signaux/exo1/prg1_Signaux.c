//1 - Gestion du traitant

//a) Regardez prg1.c et testez-le avec le caractère de contrôle (Ctrl-C ).Modifiez le code de sorte que l'exécution se termine dès le 1er Ctrl-C. 

// b) Ajoutez-y un traitant pour le signal SIGTSTP (Ctrl-Z) qui affiche «suspension du processus» ---- et suspend effectivement le processus comme le fait le traitant par défaut sur la frappe de Ctrl-Z . Pour y parvenir utiliser SIGACTION.

// c) Optez  pour l’utilisation du traitant de Sigaction afin de faciliter la lecture de la structure siginfo_t. Assurez vous d’identifier le numéro de signal, le pid du processus qui a transmis le signal et du groupe auquel appartient ce processus.


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void erreur(const char* msg) {

  fprintf(stderr,"%s\n",msg);

  }

void traitantSIGINT(int num) {
  if (num!=SIGINT)
    erreur("Pb sur SigInt...");
  printf("\n---- Ctrl-C fin directe !-----\n");
  exit(1);
}

void traitantSIGTSTP(int num) {
  printf("\n-----Suspension du processus---- \n");
  //struct siginfo_t info=getcontext();
  kill(getpid(),SIGSTOP);
  exit(2);
}


int main(int argc,char* argv[]) {

    int s,i;
    if (argc-1 != 1) {
        fprintf(stderr,"Appel : %s <nb de secondes>\n",argv[1]);
        return 1;
        }
    s=atoi(argv[1]);

    struct sigaction action;
    action.sa_handler=traitantSIGTSTP;
    action.sa_flags=SA_SIGINFO;
    signal(SIGINT,traitantSIGINT);
    if(sigaction(SIGTSTP,&action,NULL)!=0){
      printf("probleme dans sigaction 1\n");
    }


    for (i=1;i<=s;i++) {
        sleep(1);
        printf("\r %d secondes ecoules...",i);
        fflush(stdout);               /* force affichage */
    }
    printf("\n");

    return 0;
}

