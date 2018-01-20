/*
/on utilise uniquement kill et signal
/pere->fils->arme le signal SIGUSR1-> reception du signal donne "hello world"
*/
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

void gest_sig(int signum){
	printf("Hello world !\n");
	exit(5);
}

int main(int argc, char const *argv[])
{
	pid_t pid;
	pid=fork();

	switch(pid){

		case -1:
			printf("Erreur\n");
			exit(1);
			break;

		case 0:
			signal(SIGUSR1,gest_sig);
			while(1);
			break;

		default:
			sleep(3);
			kill(pid,SIGUSR1);
			break;
	}
	wait(&pid);
	return 0;
}