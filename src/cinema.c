/* cinema.c */
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


#define NBSEM 4 // a changer selon le code et le nb de sem necessaire
#define NBPSalle 2 //Nombre de salles du cinema
#define NBCH 2 // Nombre de caisse avec hotesse
#define NBCA 1 // Nombre de caisse automatique
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

typedef struct // structure representant un film
{
    char* nomFilm; // string avec nom du film
    int duree; // duree en secondes (normalement on met en minutes mais j'ai)
            // pas trop envie d'attendre 75 minutes que le code se debloque ;)
    int date; //date a laquelle commence le film
    char* categorie;//categorie du film
}film_t;

typedef struct
{
    film_t filmProjete; // struct avec info du film
    int nbPlacesDispo; // nb de place dispo dans la salle
    int nbPlaceOccupees;
    int nbPlaceOccupeesAbonnes;
}salle_t;

typedef struct //structure mise dans le segment de memoire partagee
{
    int NbCaisseHotesseOccupees;
    int NbCaisseAutoOccupees;
    salle_t* sallesCine; // tableau de salle a init dans main()
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

	    	ushort array[NBSEM] = {0,0,0,1};  //0 val pour les caisse_hotesse, 0 val pour les caisses_auto,
                                              // 0 pour les salles  1 pour le mutex
                                              // a voir si on en laisse 4 ou 3 suffise
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

void Client_cinema (int i,bool internet, bool caisseAuto)
{
    switch (internet) {
        case true:
        //va dans la salle direct


        break;
        default:
            if (caisseAuto)
            {
                if(((structure_partagee*)ptr_mem_partagee)->NbCaisseAutoOccupees==NBCA)
                {
                    printf("Le client %d se bloque car pas de caisse auto disponibles\n", i);

                }
            }else{
                if(((structure_partagee*)ptr_mem_partagee)->NbCaisseHotesseOccupees==NBCH)
                {
                    printf("Le client %d se bloque car pas de caisse disponible \n", i);

                }
            }

            //choix de la salle //
            //une petite fonction qui choisit une salle serai pas mal
            // elle retourne le numero de la salle choisi
            printf("Le client %d prend part dans la salle \n", i);
        	((structure_partagee*)ptr_mem_partagee)->sallesCine->filmProjete->nbPlacesOccupees++;
        break;
    }
}

void Client_Abonne_cinema (int i,bool internet) // a voir après reponse du prof
{

}


void changeBillet (int i)
{

}


void * fonc_Client(int i)
{
	if (! fork())
	{
		srand(time(NULL));
		printf("Le client %d arrive dans le cinéma\n",(int)i);
		Client_cinema((int)i);

		/*printf("Le client %d regarde son film\n",(int)i);
		sleep(6);
		Se_rhabiller((int)i);*/
		printf("Le client %d quitte le cinema\n",(int) i);
		exit(1);
	}
}

void * fonc_Abonne(int i)
{
	if (! fork())
	{
		srand(time(NULL));
		printf("Le client %d arrive dans le cinéma\n",(int)i);
		Client_cinema((int)i);

		/*printf("Le client %d regarde son film\n",(int)i);
		sleep(6);
		Se_rhabiller((int)i);*/
		printf("Le client %d quitte le cinema\n",(int) i);
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
	data.NbCaisseAutoOccupees=0;
	data.NbCaisseHotesseOccupees=0;
    data.sallesCine = // tableau de salle avec des films et tout
                    // fonction qui le retourne ou fait a la main comme
                    //des sacs !

	*((structure_partagee*)ptr_mem_partagee) = data;

	fonc_Client(0);
	fonc_Client(1);
	fonc_Client(2);
	//fonc_Client(3);
	//fonc_Client(4);     // A decommenter pour plus de personne
	//fonc_Client(5);
	//fonc_Client(6);

	for (j=1; j<=3; j++) wait(0);

    /******** A LAISSER ET A COPIER DANS UNE SURCHARGE DE SIGNAL !****/
	shmdt(ptr_mem_partagee); //suppression de la memoire partagée

	printf("Suppression du sémaphore.\n");
	semctl(semid,0,IPC_RMID,NULL);
	printf("Suppression du segment de mémoire partagée.\n");
	shmctl(mem_ID,IPC_RMID,NULL);
    return 0;
}
