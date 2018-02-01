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
    takeSitInRoom(numSalle);
    //((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].nbPlacesOccupeesAbonnes++; // add it in takeSitInRoom
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
      leaveSitInRoom(numSalle);
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
     while (/*((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].lancement==0*/1)
     {
         V(MutexSallesAccess);
         sleep(2);
         P(MutexSallesAccess);
     }
     V(MutexSallesAccess);
     printf("l'abonné regarde %s\n",((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].filmProjete.nomFilm );
     sleep(((structure_partagee *) ptr_mem_partagee)->sallesCine[numSalle].filmProjete.duree);
}
