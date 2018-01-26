#define NBSEM 5 // a changer selon le code et le nb de sem necessaire
#define NBPSalle 2 //Nombre de salles du cinema
#define NBCH 1 // Nombre de caisse avec hotesse
#define NBCA 1 // Nombre de caisse automatique
#define IFLAGS (SEMPERM | IPC_CREAT)
#define SKEY   (key_t) IPC_PRIVATE
#define SEMPERM 0600
#define CLEF 12345 // cle utilise pour le segment de memoire partagee
#define LINESIZE 128
#define FILEWAY "../texts/film.txt"
#define SemCaisseHot 0
#define SemCaisseAuto 1
#define MutexNBHotOccupees 2
#define MutexNBAutoOccupees 3
#define MutexSallesAccess 4

typedef struct // structure representant un film
{
    int id;
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
    int nbPlacesOccupees;
    int nbPlacesOccupeesAbonnes;
}salle_t;

typedef struct //structure mise dans le segment de memoire partagee
{
    int NbCaisseHotesseOccupees;
    int NbCaisseAutoOccupees;
    salle_t* sallesCine; // tableau de salle a init dans main()
    film_t* filmsCine;
}structure_partagee;

int initsem(key_t semkey);
void P(int semnum);
void V(int semnum);
void traitantSIGINT(int num);
void traitantSIGTSTP(int num);
void traitantSIGSTOP(int num);
void Client_cinema (int i,char internet, char caisseAuto, char *filmJarte);
void Client_Abonne_cinema (int i,char internet);
int changeBillet (int i);
void * fonc_Client(int i);
void * fonc_Abonne(int i);
void initFilmSalle(int nombreFilm);
int compteurLine(char *dossier);
void displaySalle(salle_t salle);
int choixSalle(char *film);
char* Client_setFilmJarte(int nombreFilm);
