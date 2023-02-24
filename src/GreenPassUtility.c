#include "GreenPassUtility.h"

//--Controlliamo il formato e lunghezza del codice della tessera Sanitaria
void checkCodiceTesseraSanitaria (char * codiceTesseraSanitaria) {
    size_t lunghezzaCodiceTesseraSanitaria = strlen(codiceTesseraSanitaria);

    if (lunghezzaCodiceTesseraSanitaria + 1 != LUNGHEZZA_CODICE_TESSERA_SANITARIA)
        raiseError(CHECK_HEALTH_CARD_NUMBER_SCOPE, CHECK_HEALTH_CARD_NUMBER_ERROR);
}


/*--Questa funzione si occupa di acquisire la configurazione riportata nel file di configurazione passato come parametro,
e di estrarre l'indirizzo IP e la porta del server.*/
void getDatiDiConfigurazione (const char * percorsoFileConfigurazione, char ** IPConfigurazione, unsigned short int * portaConfigurazione) {
    FILE * filePointer;
    size_t lunghezzaIP = 0, lunghezzaPorta = 0;
    ssize_t getLineBytes;
    char * tempStringIPConfigurazione = NULL, * stringIndirizzoPortaServer = NULL;

    //--Apriamo il file di configurazione passato come parametro
    filePointer = fopen(percorsoFileConfigurazione, "r");
    if (!filePointer) raiseError(FOPEN_SCOPE, FOPEN_ERROR);
    if ((getLineBytes = getline((char ** restrict) & tempStringIPConfigurazione, (size_t * restrict) & lunghezzaIP, (FILE * restrict) filePointer)) == -1)
        raiseError(GETLINE_SCOPE, GETLINE_ERROR);
    * IPConfigurazione = (char *) calloc(strlen(tempStringIPConfigurazione), sizeof(char));
    if (! *IPConfigurazione) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    strncpy(* IPConfigurazione, (const char *) tempStringIPConfigurazione, strlen(tempStringIPConfigurazione) - 1);
    checkIP(* IPConfigurazione);
    if ((getLineBytes = getline((char ** restrict) & stringIndirizzoPortaServer, (size_t * restrict) & lunghezzaPorta, (FILE * restrict) filePointer)) == -1)
        raiseError(GETLINE_SCOPE, GETLINE_ERROR);
    * portaConfigurazione = (unsigned short int) strtoul((const char * restrict) stringIndirizzoPortaServer, (char ** restrict) NULL, 10);
    if (portaConfigurazione == 0 && (errno == EINVAL || errno == ERANGE))
        raiseError(STRTOUL_SCOPE, STRTOUL_ERROR);
    //--Deallochiamo i puntatori
    free(tempStringIPConfigurazione);
    free(stringIndirizzoPortaServer);
    //--Chiudiamo del file
    fclose(filePointer);
}

//--Calcoliamo il periodo di validità dello specifico GreenPass
char * getdataScadenzaVaccino (void) {
    time_t systemTimeSeconds = time(NULL);
    //--Consideriamo la data di sistema attuale
    struct tm * infoDataDiScadenza = localtime((const time_t *) & systemTimeSeconds);
    
    //--Tronchiamo la data al primo giorno del mese
    infoDataDiScadenza->tm_mday = 1;
    
    //--Aggiungiamo 6 mesi e si verifica se è cambiato l'anno
    if (infoDataDiScadenza->tm_mon + MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE + 1 > 11) {
        infoDataDiScadenza->tm_year += 1;
        infoDataDiScadenza->tm_mon = (infoDataDiScadenza->tm_mon + MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE + 1) % MESI_IN_ANNO;
    } else {
        infoDataDiScadenza->tm_mon = (infoDataDiScadenza->tm_mon + MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE + 1);
    }
    
    //--Convertiamo in stringa della data e ritorno al chiamante
    char * dataScadenzaVaccino = (char *) calloc(LUNGHEZZA_DATA, sizeof(char));

    if (!dataScadenzaVaccino)
        raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    sprintf(dataScadenzaVaccino, "%02d-%02d-%d", infoDataDiScadenza->tm_mday, infoDataDiScadenza->tm_mon, infoDataDiScadenza->tm_year + 1900);
    dataScadenzaVaccino[LUNGHEZZA_DATA - 1] = '\0';
    return dataScadenzaVaccino;
}

//--Questa procedura si occupa di calcolare e restutire la data attuale
char * getdataAttuale (void) {
    time_t tempTimeSeconds = time(NULL);
    struct tm * infoDataAttuale = localtime((const time_t *) & tempTimeSeconds);
    char * dataAttuale = (char *) calloc(LUNGHEZZA_DATA, sizeof(char));

    if (!dataAttuale)
        raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    
    sprintf(dataAttuale, "%02d-%02d-%d", infoDataAttuale->tm_mday, infoDataAttuale->tm_mon + 1, infoDataAttuale->tm_year + 1900);
    dataAttuale[LUNGHEZZA_DATA - 1] = '\0';
    return dataAttuale;
}

/*--Attraverso la seguente funzione possiamo getre il socket file descriptor di una connessione impostata col serverV.
In input passiamo il file di configurazione che è stato fornito al CentroVaccinale o al ServerG per mettersi in contatto con il ServerV.*/
int creaConnessioneConServerV (const char * percorsoFileConfigurazione) {
    struct sockaddr_in indirizzoServerV;
    char * stringindirizzoServerVIP = NULL;
    unsigned short int portaServerV;
    int serverV_SFD;
    
    //--Ritorno del file di configurazione
    getDatiDiConfigurazione(percorsoFileConfigurazione, & stringindirizzoServerVIP, & portaServerV);
    //--Impostiamo la comunicazione col serverV creando il socket
    serverV_SFD = wsocket(AF_INET, SOCK_STREAM, 0);
    memset((void *) & indirizzoServerV, 0, sizeof(indirizzoServerV));
    indirizzoServerV.sin_family = AF_INET;
    indirizzoServerV.sin_port   = htons(portaServerV);
    if (inet_pton(AF_INET, (const char * restrict) stringindirizzoServerVIP, (void *) & indirizzoServerV.sin_addr) <= 0)
        raiseError(INET_PTON_SCOPE, INET_PTON_ERROR);
    wconnect(serverV_SFD, (struct sockaddr *) & indirizzoServerV, (socklen_t) sizeof(indirizzoServerV));

    free(stringindirizzoServerVIP);
    return serverV_SFD;
}
