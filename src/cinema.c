/* cinema.c */
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>
#include <sys/shm.h>
#include <memory.h>
#include <sys/time.h>
#include "cinema.h"

int semid;
int nombreFilms;
struct sembuf sem_oper_P;  /* Operation P */
struct sembuf sem_oper_V;  /* Operation V */

struct timeval tm;


/* initialisation de la memoire partagee*/
int mem_ID;
void *ptr_mem_partagee;
salle_t *salle;
film_t *films;

int initsem(key_t semkey) {
    int status = 0;
    int semid_init;
    union semun {
        int val;
        struct semid_ds *stat;
        ushort *array;
    } ctl_arg;
    if ((semid_init = semget(semkey, NBSEM, IFLAGS)) > 0) {

        ushort array[NBSEM] = {0, 0, 1, 1, 1, 1, 0};  //0 val pour les caisse_hotesse, 0 val pour les caisses_auto,
        // 1 pour le mutex
        ctl_arg.array = array;
        status = semctl(semid_init, 0, SETALL, ctl_arg);
    }
    if (semid_init == -1 || status == -1) {
        perror("Erreur initsem");
        return (-1);
    } else return (semid_init);
}

void P(int semnum) {

    sem_oper_P.sem_num = (unsigned short) semnum;
    sem_oper_P.sem_op = -1;
    sem_oper_P.sem_flg = 0;
    semop(semid, &sem_oper_P, 1);

}

void V(int semnum) {

    sem_oper_V.sem_num = (unsigned short) semnum;
    sem_oper_V.sem_op = 1;
    sem_oper_V.sem_flg = 0;
    semop(semid, &sem_oper_V, 1);
}

void traitantSIGINT(int num) {
    if (num != SIGINT) {
        printf("Pb sur SigInt...");
    } else {
        printf("\n---- Ctrl-C détachement du segment de mémoire partagée !-----\n");
        shmdt(ptr_mem_partagee);
        printf("\n---- Ctrl-C suppression du segment de mémoire partagée !-----\n");
        shmctl(mem_ID, IPC_RMID, NULL);
        printf("\n---- Ctrl-C suppression des sémaphores !-----\n");
        semctl(semid, 0, IPC_RMID, NULL);
    }
    exit(1);
}

void traitantSIGTSTP(int num) {
    if (num != SIGTSTP) {
        printf("Pb sur Sigtstp...");
    } else {
        printf("\n---- Ctrl-Z détachement du segment de mémoire partagée !-----\n");
        shmdt(ptr_mem_partagee);
        printf("\n---- Ctrl-Z suppression du segment de mémoire partagée !-----\n");
        shmctl(mem_ID, IPC_RMID, NULL);
        printf("\n---- Ctrl-Z suppression des sémaphores !-----\n");
        semctl(semid, 0, IPC_RMID, NULL);
    }
    exit(1);
}

void traitantSIGSTOP(int num) {
    if (num != SIGSTOP) {
        printf("Pb sur Sigstop...");
    } else {
        printf("\n---- kill detected détachement du segment de mémoire partagée !-----\n");
        shmdt(ptr_mem_partagee);
        printf("\n----kill detected suppression du segment de mémoire partagée !-----\n");
        shmctl(mem_ID, IPC_RMID, NULL);
        printf("\n---- kill detected suppression des sémaphores !-----\n");
        semctl(semid, 0, IPC_RMID, NULL);
    }
    exit(1);
}

void Client_cinema(int i, char internet, char caisseAuto, char *filmJarte,char firstPassage) {
    int numeroSalle = 0;
    gettimeofday(&tm, NULL);
    srandom((unsigned int) (tm.tv_sec + tm.tv_usec * 1000000ul));
    switch (internet) {
        case 1:
            //va dans la salle direct
            /*a changer*/
            P(MutexSallesAccess);
            ((structure_partagee *) ptr_mem_partagee)->sallesCine[0].nbPlacesOccupees++;
            V(MutexSallesAccess);
            printf("Le client %d prend part dans la salle %d \n", i, 0);
            break;
        default:
            if (caisseAuto) {
                P(MutexNBAutoOccupees);
                if (((structure_partagee *) ptr_mem_partagee)->NbCaisseAutoOccupees >= NBCA) {
                    V(MutexNBAutoOccupees);
                    printf("Le client %d se bloque car pas de caisse auto disponibles\n", i);
                    P(SemCaisseAuto);
                    P(MutexNBAutoOccupees);
                }
                ((structure_partagee *) ptr_mem_partagee)->NbCaisseAutoOccupees++;
                V(MutexNBAutoOccupees);
                numeroSalle = choixSalle(filmJarte);

                int laissepasser = rand() % 10;
                if (laissepasser % 2) //si laisse passer est impair
                {
                    //changement de billet car doute de l'caisse_hotesse
                    printf("L'hotesse a des doutes et demandes au client %d de choisir un autre film\n", i);

                    filmJarte[numeroSalle] = 1;
                    numeroSalle = choixSalle(filmJarte);

                }
                P(MutexSallesAccess);
                ((structure_partagee *) ptr_mem_partagee)->sallesCine[numeroSalle].nbPlacesOccupees++;
                V(MutexSallesAccess);

                P(MutexNBAutoOccupees);
                ((structure_partagee *) ptr_mem_partagee)->NbCaisseAutoOccupees--;
                V(MutexNBAutoOccupees);
                V(SemCaisseAuto);

                // elle retourne le numero de la salle choisi
                printf("Le client %d prend part dans la salle \n", i);

            } else {
                P(MutexNbAbonnesAttente);
                while  (((structure_partagee *) ptr_mem_partagee)->NbAbonnesAttente>0)
                {
                    V(MutexNbAbonnesAttente);
                    printf("Le client non abonne %d se bloque car des abonnés sont en attentes\n", i);
                    P(SemAbonneAttente);
                    P(MutexNbAbonnesAttente);
                }
                V(MutexNbAbonnesAttente);
                P(MutexNBHotOccupees);
                if (((structure_partagee *) ptr_mem_partagee)->NbCaisseHotesseOccupees >= NBCH) {
                    printf("Le client %d se bloque car pas de caisse disponible \n", i);
                    V(MutexNBHotOccupees);
                    P(SemCaisseHot);
                }else{
                    V(MutexNBHotOccupees);
                }

                P(MutexNBHotOccupees);
                ((structure_partagee *) ptr_mem_partagee)->NbCaisseHotesseOccupees++;
                V(MutexNBHotOccupees);
                //choix de la salle //
                printf("le client %d met du temps a choisir son film\n", i);
                numeroSalle = choixSalle(filmJarte);
                sleep(3);
                int laissepasser = rand() % 10;
                if (laissepasser % 2) //si laisse passer est impair
                {
//                    printf("Laisser passer : %d\n",laissepasser);
                    //changement de billet car doute de l'caisse_hotesse
                    printf("L'hotesse a des doutes et demandes au client %d de choisir un autre film "
                                   "que %s \n", i,
                           ((structure_partagee *) ptr_mem_partagee)->sallesCine[numeroSalle].filmProjete.nomFilm);
                    sleep(3);
                    filmJarte[numeroSalle] = 1;
                    numeroSalle = choixSalle(filmJarte);
                }
                P(MutexSallesAccess);
                ((structure_partagee *) ptr_mem_partagee)->sallesCine[numeroSalle].nbPlacesOccupees++;
                //printf("nbPlacesOccupees %d  dans salle de %s\n",((structure_partagee *) ptr_mem_partagee)->sallesCine[numeroSalle].nbPlacesOccupees,((structure_partagee *) ptr_mem_partagee)->sallesCine[numeroSalle].filmProjete.nomFilm );

                V(MutexSallesAccess);

                P(MutexNBHotOccupees);
                ((structure_partagee *) ptr_mem_partagee)->NbCaisseHotesseOccupees--;
                V(MutexNBHotOccupees);
                V(SemCaisseHot);

                printf("Le client %d prend part dans la salle %s \n", i,
                       ((structure_partagee *) ptr_mem_partagee)->sallesCine[numeroSalle].filmProjete.nomFilm);
                sleep(3);
                firstPassage=0;
                int alea = rand() % 20 + 10;
                if (alea == 11 || alea == 12 || alea == 13) {
                    printf("le client %d change de film\n", i);
                    filmJarte[numeroSalle] = 1;
                    firstPassage=1;
                    Client_cinema(i, internet, caisseAuto, filmJarte,firstPassage);
                }
                if (firstPassage==0)
                {
                    printf("le client %d attend le lancement du film %s\n",i,((structure_partagee *) ptr_mem_partagee)->sallesCine[numeroSalle].filmProjete.nomFilm);
                }
                P(MutexSallesAccess);
                while (((structure_partagee *) ptr_mem_partagee)->sallesCine[numeroSalle].lancement==0)
                {
                    V(MutexSallesAccess);
                    sleep(2);
                    printf("l'abonné regarde %s\n",((structure_partagee *) ptr_mem_partagee)->sallesCine[numeroSalle].filmProjete.duree );
                    P(MutexSallesAccess);
                }
                V(MutexSallesAccess);
                printf("l'abonné regarde %s\n",((structure_partagee *) ptr_mem_partagee)->sallesCine[numeroSalle].filmProjete.nomFilm );
                sleep(((structure_partagee *) ptr_mem_partagee)->sallesCine[numeroSalle].filmProjete.duree);
            }
            break;
    }
}

char *Client_setFilmJarte(int nombreFilm) {
    char *filmJarte = malloc(sizeof(char) * nombreFilm);
    for (int i = 0; i < nombreFilm; ++i) {
        filmJarte[i] = -1;
    }
    return filmJarte;
}

int choixSalle(char *film) {
    int i = 0;
    int salleChoisit = 0;
    char *salleParcourut;
    salleParcourut = malloc(sizeof(char) * nombreFilms);
    for (int j = 0; j < nombreFilms; ++j) {
        if (film[j] == 1) { /*si le film est à 1, le client ne peut pas le regarder*/
            salleParcourut[j] = 1;
        } else {
            salleParcourut[j] = 0; /*tableau de booléen si salle parcourut ou non*/
        }
    }
    while (1) {
        int cut = 0;
        int alea = rand() % nombreFilms;
        if (film[alea] != 1) {
            P(MutexSallesAccess);
            if (salle[alea].nbPlacesDispo != ((structure_partagee *) ptr_mem_partagee)->sallesCine[alea].nbPlacesOccupees
                && ((structure_partagee *) ptr_mem_partagee)->sallesCine[alea].lancement!=1) { /*salle dispo -> prendre place*/
                salleChoisit = alea;
                V(MutexSallesAccess);
                break;
            } else { /*salle non dispo, la salle est donc parcouru*/
                V(MutexSallesAccess);
                salleParcourut[alea] = 1;
            }
        }
        for (int j = 0; j < nombreFilms; ++j) {
            if (salleParcourut[j] == 1) {
                cut++;
            }
        }
        if (cut == nombreFilms) {  /*si cut == nombreFilm alors toutes les salles sont parcourut*/
            salleChoisit = -1; /*le client ne choisit pas de salle*/
            break;
        }
        i++;
    }
    return salleChoisit;
}

void Client_Abonne_cinema(int i,char* filmJarte,char firstPassage)
{
    int numSalle;
    gettimeofday(&tm, NULL);
    srandom((unsigned int) (tm.tv_sec + tm.tv_usec * 1000000ul));
    P(MutexNBHotOccupees);
    if (((structure_partagee *) ptr_mem_partagee)->NbCaisseHotesseOccupees >= NBCH) {
        printf("L'abonné %d se bloque car pas de caisse disponible \n", i);
        V(MutexNBHotOccupees);
        P(MutexNbAbonnesAttente);
        ((structure_partagee *) ptr_mem_partagee)->NbAbonnesAttente++;
        V(MutexNbAbonnesAttente);
        P(SemCaisseHot);
        P(MutexNbAbonnesAttente);
        ((structure_partagee *) ptr_mem_partagee)->NbAbonnesAttente--;
        V(MutexNbAbonnesAttente);
        P(MutexNBHotOccupees);
        V(SemAbonneAttente);
    }
    ((structure_partagee *) ptr_mem_partagee)->NbCaisseHotesseOccupees++;
    V(MutexNBHotOccupees);
    //choix de la salle //
    printf("l'abonné %d met du temps a choisir son film\n", i);
    numSalle = choixSalle(filmJarte);
    sleep(3);
    srand((unsigned int) time(NULL));
    int laissepasser = rand() % 10;
    if (laissepasser == 1 || laissepasser == 2 || laissepasser == 3) //si laisse vaut 1 ou 2 ou 3
    {
        //changement de billet car doute de l'caisse_hotesse
        printf("L'hotesse a des doutes et demandes a l'abonné %d de choisir un autre film "
                       "que %s \n", i,
               ((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].filmProjete.nomFilm);
        sleep(3);
        filmJarte[numSalle] = 1;
        numSalle = choixSalle(filmJarte);
    }
    if (numSalle==-1)
    {
        printf("Aucun film ne convient à l'abonné %d, il se part !\n",i);
        P(MutexNBHotOccupees);
        ((structure_partagee *) ptr_mem_partagee)->NbCaisseHotesseOccupees--;
        V(MutexNBHotOccupees);
        exit(2);
    }
    P(MutexSallesAccess);
    ((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].nbPlacesOccupees++;
    printf("nbPlacesOccupees %d  dans salle de %s\n",((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].nbPlacesOccupees,((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].filmProjete.nomFilm );
    ((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].nbPlacesOccupeesAbonnes++;
    V(MutexSallesAccess);

    P(MutexNBHotOccupees);
    ((structure_partagee *) ptr_mem_partagee)->NbCaisseHotesseOccupees--;
    V(MutexNBHotOccupees);
    V(SemCaisseHot);
    V(SemAbonneAttente);

    P(MutexSallesAccess);
    printf("L'abonné %d prend part dans la salle %s \n", i,
           ((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].filmProjete.nomFilm);
    V(MutexSallesAccess);
    sleep(3);
    firstPassage=0;
    srand((unsigned int) time(NULL));
    int alea = rand() % 20 + 10;
    if (alea == 11 || alea == 12 || alea == 13) {
      printf("l'abonné %d change de film\n", i);
      P(MutexSallesAccess);
      ((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].nbPlacesOccupees--;
      ((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].nbPlacesOccupeesAbonnes--;
      V(MutexSallesAccess);
      filmJarte[numSalle]=1;
      firstPassage=1;
      Client_Abonne_cinema(i, filmJarte,firstPassage);

     }
     if (firstPassage==0)
     {
         printf("le abonné %d attend le lancement du film %s\n",i,((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].filmProjete.nomFilm);
     }
     P(MutexSallesAccess);
     while (((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].lancement==0)
     {
         V(MutexSallesAccess);
         sleep(2);
         P(MutexSallesAccess);
     }
     V(MutexSallesAccess);
     printf("l'abonné regarde %s\n",((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].filmProjete.nomFilm );
     sleep(((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].filmProjete.duree);
}


void *fonc_Client(int i) {
    if (!fork()) {
        srand((unsigned int) time(NULL));
        printf("Le client %d arrive dans le cinéma\n", i);
        char *filmJarte = Client_setFilmJarte(compteurLine(FILEWAY));
        Client_cinema(i, 0, rand()%1, filmJarte,0);
        printf("Le client %d quitte le cinema\n", i);
        exit(1);
    }
}

void *fonc_Abonne(int i) {
    if (!fork()) {
        srand((unsigned int) time(NULL));
        printf("Le client abonné %d arrive dans le cinéma\n", i);
        char *filmJarte = Client_setFilmJarte(compteurLine(FILEWAY));
        Client_Abonne_cinema(i, filmJarte,0);
        printf("Le client abonné %d quitte le cinema\n", i);
        exit(1);
    }
}

int compteurLine(char *dossier) {
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    int compteur = 0;
    fp = fopen(dossier, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    while ((getline(&line, &len, fp)) != -1) {
        compteur++;
    }
    fclose(fp);
    if (line)
        free(line);
    return compteur;
}

void initFilmSalle(int nombreFilm) {
    salle = (salle_t *) malloc(sizeof(salle_t) * nombreFilm);
    films = (film_t *) malloc(sizeof(film_t) * nombreFilm);
    FILE *fp;
    fp = fopen(FILEWAY, "r");
    char line[LINESIZE];
    const char s[2] = "|";
    char *token;
    int i = 0;
    if (fp) {
        while (fgets(line, sizeof(line), fp)) { /*on lit ligne par ligne les information*/
            films[i].id = i;

            token = strtok(line, s);/*à chaque fois qu'on rencontre le séparateur s, on divise la ligne.*/
            films[i].nomFilm = malloc(strlen(token) + 1);
            strcpy(films[i].nomFilm, token);

            token = strtok(NULL, s);
            films[i].duree = atoi(token);

            token = strtok(NULL, s);
            films[i].date = atoi(token);

            token = strtok(NULL, "\n"); /*on rencontre le retour à la ligne, c'est la dernière valeur.*/
            films[i].categorie = malloc(strlen(token) + 1);
            strcpy(films[i].categorie, token);

            salle[i].filmProjete = films[i];
            salle[i].nbPlacesDispo = rand() % 30 + 10;
            salle[i].nbPlacesOccupees = 0;
            salle[i].nbPlacesOccupeesAbonnes = 0;
            salle[i].lancement=0;
            i++;
        }
        // read out the array
        fclose(fp);
    } else {
        printf("error opening fp");
    }
}

int main() {
    int j;
    structure_partagee data;
    srand((unsigned int) time(NULL));
    signal(SIGINT, traitantSIGINT); // catch ctrl+C
    signal(SIGTSTP, traitantSIGTSTP); //catch ctrl+z
    signal(SIGSTOP, traitantSIGSTOP); // catch kill -STOP pid

    semid = initsem(SKEY); // initialisation du semaphore

    /*initialisation du nombre de panier et de cabine*/
    nombreFilms = compteurLine(FILEWAY);
    initFilmSalle(nombreFilms);
    data.sallesCine = salle;
    data.filmsCine = films;
    data.NbCaisseAutoOccupees = 0;
    data.NbCaisseHotesseOccupees = 0;

    mem_ID = shmget(CLEF, sizeof(data), 0666 | IPC_CREAT);
    ptr_mem_partagee = shmat(mem_ID, NULL, 0);
    *((structure_partagee *) ptr_mem_partagee) = data;

    for (size_t k = 0; k < 10; k++)
    {
        if (rand()%2) //si 0 client normal
        {
            fonc_Client(k);
            sleep(1);
        }else{ // sinon abonné
            fonc_Abonne(k);
            sleep(1);
        }
        if (k%10==0)
        {
            sleep(3);
        }
    }
    sleep(15);
    printf("ATTENTION LES PROJECTIONS VONT BIENTOT COMMENCER \n");
    // for (size_t a = 0; a < NombreCheck; a++)
    // {
        for (size_t pass = 0; pass < nombreFilms; pass++)
        {
            printf("nombre dans salle %d : %d\n",pass,((structure_partagee *) ptr_mem_partagee)->sallesCine[pass].nbPlacesOccupees );
            // if ((((structure_partagee *) ptr_mem_partagee)->sallesCine[pass].nbPlacesOccupees)
            // > (((structure_partagee *) ptr_mem_partagee)->sallesCine[pass].nbPlacesOccupees*3/4))
            // {
                ((structure_partagee *) ptr_mem_partagee)->sallesCine[pass].lancement=1;
            //}

        }
    //}
    //fonc_Abonne(1);
    //fonc_Abonne(2);
    //fonc_Client(0);
    //fonc_Client(3);
    //fonc_Client(3);
    //fonc_Client(4);     // A decommenter pour plus de personne
    //fonc_Client(5);
    //fonc_Client(6);


//
    for (j = 0; j < 10; j++) wait(0);

    shmdt(ptr_mem_partagee); //detachement de la memoire partagee
    printf("Suppression du sémaphore.\n");
    semctl(semid, 0, IPC_RMID, NULL);
    printf("Suppression du segment de mémoire partagée.\n");
    shmctl(mem_ID, IPC_RMID, NULL);
    return 0;
}
