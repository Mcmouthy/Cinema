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
#include "cinema.h"

int semid;
struct sembuf sem_oper_P;  /* Operation P */
struct sembuf sem_oper_V;  /* Operation V */

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

        ushort array[NBSEM] = {0, 0, 1};  //0 val pour les caisse_hotesse, 0 val pour les caisses_auto,
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

void Client_cinema(int i, char internet, char caisseAuto) {
    switch (internet) {
        case 1:
            //va dans la salle direct
            /*a changer*/
            P(2);
            ((structure_partagee *) ptr_mem_partagee)->sallesCine[0].nbPlacesOccupees++;
            V(2);
            printf("Le client %d prend part dans la salle %d \n", i,0);
            break;
        default:
            if (caisseAuto) {
                P(2);
                if (((structure_partagee *) ptr_mem_partagee)->NbCaisseAutoOccupees >= NBCA)
                {
                    V(2);
                    printf("Le client %d se bloque car pas de caisse auto disponibles\n", i);
                    P(1);
                    P(2);
                }
                ((structure_partagee *) ptr_mem_partagee)->NbCaisseAutoOccupees++;
                V(2);
                int numSalle=0; /***changer pour la fonction de choix de rémy***/

                int laissepasser=rand()%10;
                if (laissepasser%2) //si laisse passer est impair
                {
                    //changement de billet car doute de l'caisse_hotesse
                    printf("L'hotesse a des doutes et demandes au client %d de choisir un autre film\n",i);
                    numSalle=changeBillet(i);
                }
                P(2);
                ((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].nbPlacesOccupees++;
                V(2);

                P(2);
                ((structure_partagee *) ptr_mem_partagee)->NbCaisseAutoOccupees--;
                V(2);
                V(1);

                // elle retourne le numero de la salle choisi
                printf("Le client %d prend part dans la salle \n", i);

            } else {
                P(2);
                if (((structure_partagee *) ptr_mem_partagee)->NbCaisseHotesseOccupees >= NBCH) {
                    printf("Le client %d se bloque car pas de caisse disponible \n", i);
                    V(2);
                    P(0);
                    P(2);
                }
                ((structure_partagee *) ptr_mem_partagee)->NbCaisseHotesseOccupees++;
                V(2);
                //choix de la salle //
                int numsalle=0; /***changer pour la fonction de choix de rémy***/
                printf("le client %d met du temps a choisir son film\n",i );
                sleep(3);
                srand(time(NULL));
                int laissepasser=rand()%10;
                printf("laissepasser : %d\n",laissepasser );
                if (laissepasser%2) //si laisse passer est impair
                {
                    //changement de billet car doute de l'caisse_hotesse
                    printf("L'hotesse a des doutes et demandes au client %d de choisir un autre film\n",i);
                    sleep(3);
                    numsalle=changeBillet(i);
                }
                P(2);
                ((structure_partagee *) ptr_mem_partagee)->sallesCine[numsalle].nbPlacesOccupees++;
                V(2);

                P(2);
                ((structure_partagee *) ptr_mem_partagee)->NbCaisseHotesseOccupees--;
                V(2);
                V(0);

                printf("Le client %d prend part dans la salle %d \n", i,numsalle);
                sleep(10);
            }
            int salle_choisit = choixSalle(); /*choix de la salle*/
            printf("Le client %d prend part dans la salle \n", i);
            ((structure_partagee *) ptr_mem_partagee)->sallesCine[0].nbPlacesOccupees++;
            break;
    }
}

int choixSalle() {
    int salleChoisit=0;
    int i = 0;
    int cut = 0;
    int *salleParcourut;
    int nombreFilm = compteurLine(FILEWAY);
    for (int j = 0; j < nombreFilm; ++j) {
        salleParcourut[i] = 0; /*tableau de booléen si salle parcourut ou non*/
    }
    while (1) {
        int alea = rand() % nombreFilm;
        if (salle[alea].nbPlacesDispo != salle[alea].nbPlacesOccupees) { /*salle dispo -> prendre place*/
            salleChoisit=alea;
            break;
        }else{ /*salle non dispo, la salle est donc parcouru*/
            salleParcourut[alea]=1;
        }
        for (int j = 0; j < nombreFilm; ++j) {
            cut+=salleParcourut[j]; /*cut = nombre de booléen à true*/
        }
        if(cut == nombreFilm){  /*si cut == nombreFilm alors toutes les salles sont parcourut*/
            salleChoisit=-1; /*le client ne choisit pas de salle*/
            break;
        }
    }
    return salleChoisit;
}

void Client_Abonne_cinema(int i, char internet) // a voir après reponse du prof
{
    /*choix de la salle*/
    int numsalle=0; /******a changer*********/
    P(2);
    ((structure_partagee *) ptr_mem_partagee)->sallesCine[numsalle].nbPlacesOccupees++;
    V(2);
    printf("Le client %d prend part dans la salle %d\n", i,numsalle);

}


int changeBillet(int i) {
    return 0;
}


void *fonc_Client(int i) {
    if (!fork()) {
        srand(time(NULL));
        printf("Le client %d arrive dans le cinéma\n", i);
        Client_cinema(i, 0, 0);

        /*printf("Le client %d regarde son film\n",(int)i);
        sleep(6);
        Se_rhabiller((int)i);*/
        printf("Le client %d quitte le cinema\n", i);
        exit(1);
    }
}

void *fonc_Abonne(int i) {
    if (!fork()) {
        srand((unsigned int) time(NULL));
        printf("Le client abonné %d arrive dans le cinéma\n", i);
        Client_Abonne_cinema(i, 1);

        /*printf("Le client %d regarde son film\n",(int)i);
        sleep(6);
        Se_rhabiller((int)i);*/
        printf("Le client abonné %d quitte le cinema\n", i);
        exit(1);
    }
}

void displaySalle(salle_t lasalle) {
    printf("Titre du film projeté : %s |", lasalle.filmProjete.nomFilm);
    printf("Nombre de place disponible :%d | ", lasalle.nbPlacesDispo);
    printf("Nombre de place occupées :%d | ", lasalle.nbPlacesOccupees);
    printf("Nombre de place occupées par un Abonnées :%d | ", lasalle.nbPlacesOccupeesAbonnes);
    printf("\n");
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
            salle[i].nbPlacesDispo = rand() % 40 + 10;
//            salle[i].nbPlacesDispo = 0;
            salle[i].nbPlacesOccupees = 0;
            salle[i].nbPlacesOccupeesAbonnes = 0;
            i++;
        }
        // read out the array
        fclose(fp);
    } else {
        printf("error opening fp");
    }
}

int main() {
    structure_partagee data;
    srand((unsigned int) time(NULL));
    signal(SIGINT, traitantSIGINT); // catch ctrl+C
    signal(SIGTSTP, traitantSIGTSTP); //catch ctrl+z
    signal(SIGSTOP, traitantSIGSTOP); // catch kill -STOP pid

    semid = initsem(SKEY); // initialisation du semaphore

    /*initialisation du nombre de panier et de cabine*/
    int nombreFilm = compteurLine(FILEWAY);
    initFilmSalle(nombreFilm);
    data.sallesCine = salle;
    data.filmsCine = films;
    data.NbCaisseAutoOccupees = 0;
    data.NbCaisseHotesseOccupees = 0;

    printf("_-_-_-_SALLE_-_-_-_-\n");
    for (int i = 0; i < nombreFilm; ++i) {
        displaySalle(data.sallesCine[i]);
    }
    mem_ID = shmget(CLEF, sizeof(data), 0666 | IPC_CREAT);
    ptr_mem_partagee = shmat(mem_ID, NULL, 0);
    *((structure_partagee *) ptr_mem_partagee) = data;

    fonc_Client(0);
    fonc_Client(1);
    //fonc_Client(2);
    //fonc_Client(3);
    //fonc_Client(4);     // A decommenter pour plus de personne
    //fonc_Client(5);
    //fonc_Client(6);

    for (j=0; j<2; j++) wait(0);

    shmdt(ptr_mem_partagee); //detachement de la memoire partagee
    printf("Suppression du sémaphore.\n");
    semctl(semid, 0, IPC_RMID, NULL);
    printf("Suppression du segment de mémoire partagée.\n");
    shmctl(mem_ID, IPC_RMID, NULL);
    return 0;
}
