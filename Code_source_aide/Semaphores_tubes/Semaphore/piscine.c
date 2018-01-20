/* piscine.c */
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h> 
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>


#define NBSEM 3
#define NBP 2 //Nombre de paniers disponibles
#define NBC 2 // nombre de cabines disponibles
#define IFLAGS (SEMPERM | IPC_CREAT)
#define SKEY   (key_t) IPC_PRIVATE
#define SEMPERM 0600
#define CLEF 12345 // cle utilise pour le segment de memoire partagee

int semid ;
struct sembuf sem_oper_P ;  /* Operation P */
struct sembuf sem_oper_V ;  /* Operation V */

/* initialisation de la memoire partagee*/
int mem_ID;
void* ptr_mem_partagee;

typedef struct //structure mise dans le segment de memoire partagee
{
	int NbCabinesOccupees;
	int NbPaniersOccupes;
}structure_partagee;

int initsem(key_t semkey) 
{
	int status = 0;		
	int semid_init;
   	union semun {
		int val;
		struct semid_ds *stat;
		ushort * array;
	} ctl_arg;
    if ((semid_init = semget(semkey, NBSEM, IFLAGS)) > 0) {
		
	    	ushort array[3] = {0,0,1};  //0 val pour les paniers, 0 val pour les cabines  1 pour le mutex
	    	ctl_arg.array = array;
	    	status = semctl(semid_init, 0, SETALL, ctl_arg);
    }
   if (semid_init == -1 || status == -1) { 
	perror("Erreur initsem");
	return (-1);
    } else return (semid_init);
}

void P(int semnum) {

	sem_oper_P.sem_num = semnum;
	sem_oper_P.sem_op  = -1 ;
	sem_oper_P.sem_flg = 0 ;
	semop(semid,&sem_oper_P,1);
	
}

void V(int semnum) {

	sem_oper_V.sem_num = semnum;
	sem_oper_V.sem_op  = 1 ;
	sem_oper_V.sem_flg  = 0 ;
	semop(semid,&sem_oper_V,1);
}


void Client_piscine (int i)
{
	
	/* prend un panier */
	if(((structure_partagee*)ptr_mem_partagee)->NbPaniersOccupes>=NBP)
	{
		printf("Le client %d se bloque car pas de panier disponible \n", i);
		P(0);
    }

	P(2);
	printf("Le client %d prend un panier \n", i);
	((structure_partagee*)ptr_mem_partagee)->NbPaniersOccupes++;
	V(2);

	if (((structure_partagee*)ptr_mem_partagee)->NbCabinesOccupees >= NBC)  {
 		printf("Le client %d se bloque car pas de cabine disponible \n", i);
 		P(1);
    }
    
    /* entre dans une cabine */
	P(2);
	printf("Le client %d entre dans une cabine \n", i);
	((structure_partagee*)ptr_mem_partagee)->NbCabinesOccupees++;
	V(2);
	sleep(5);
	
	
	P(2);
	printf("Le client %d met son maillot de bain et libere la cabine \n", i);
	((structure_partagee*)ptr_mem_partagee)->NbCabinesOccupees--;
	V(2);	
	V(1);
	

}

	
void Se_rhabiller (int i)
{
	if (((structure_partagee*)ptr_mem_partagee)->NbCabinesOccupees>=NBC)  
	{
		printf("Le nageur %d veut se rhabiller mais pas de cabine disponible \n", i);
		P(1);	   	
	}	
	/* entre dans une cabine */
	P(2);	
	printf("Le nageur %d entre dans une cabine \n", i);
	((structure_partagee*)ptr_mem_partagee)->NbCabinesOccupees++;
	V(2);

	sleep(5);
	P(2);
	printf("Le nageur %d se change et libere la cabine \n", i);
	((structure_partagee*)ptr_mem_partagee)->NbCabinesOccupees--; 	
	V(2);
	V(1);

	P(2);
	printf("Le nageur %d libere le panier \n", i);
	((structure_partagee*)ptr_mem_partagee)->NbPaniersOccupes--;
	V(2);
	V(0);
	
}


void * fonc_Client(int i)
{
	if (! fork())
	{
		srand(time(NULL));
		printf("Le client %d arrive ...\n",(int)i);
		Client_piscine((int)i);
		/* temps de natation */
		printf("Le client %d nage ...\n",(int)i);
		sleep(6);
		Se_rhabiller((int)i);
		printf("Le client %d quitte la piscine\n",(int) i);
		exit(1);
	}
	
}


int main()
{
	int j;
	structure_partagee data;

	semid=initsem(SKEY); // initialisation du semaphore

	mem_ID = shmget(CLEF, sizeof(data), 0666 | IPC_CREAT);
	ptr_mem_partagee = shmat(mem_ID, NULL, 0);

	/*initialisation du nombre de panier et de cabine*/
	data.NbCabinesOccupees=0;
	data.NbPaniersOccupes=0;
	
	*((structure_partagee*)ptr_mem_partagee) = data;

	fonc_Client(0);
	fonc_Client(1);
	fonc_Client(2);
	//fonc_Client(3);
	//fonc_Client(4);     // A decommenter pour plus de personne
	//fonc_Client(5);
	//fonc_Client(6);
	
	for (j=1; j<=3; j++) wait(0);

	shmdt(ptr_mem_partagee); //suppression de la memoire partagée

	printf("Suppression du sémaphore.\n");
	semctl(semid,0,IPC_RMID,NULL);
	printf("Suppression du segment de mémoire partagée.\n");
	shmctl(mem_ID,IPC_RMID,NULL);
    return 0;
}
