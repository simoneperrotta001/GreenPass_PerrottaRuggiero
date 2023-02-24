#include "GreenPassUtility.h"

//--Controlliamo il formato e lunghezza del codice della tessera Sanitaria
void checkCodiceTesseraSanitaria (char * codiceTesseraSanitaria) {
    size_t lunghezzaCodiceTesseraSanitaria = strlen(codiceTesseraSanitaria);

    if (lunghezzaCodiceTesseraSanitaria + 1 != LUNGHEZZA_CODICE_TESSERA_SANITARIA)
        raiseError(CHECK_HEALTH_CARD_NUMBER_SCOPE, CHECK_HEALTH_CARD_NUMBER_ERROR);
}


/*--Questa funzione si occupa di acquisire la configurazione riportata nel file di configurazione passato come parametro,
e di estrarre l'indirizzo IP e la porta del server.*/
void ritornaDatiDiConfigurazione (const char * percorsoFileConfigurazione, char ** configurationIP, unsigned short int * configurationPort) {
    FILE * filePointer;
    size_t IPlength = 0, portLength = 0;
    ssize_t getLineBytes;
    char * tempStringConfigurationIP = NULL, * stringServerindirizzoPort = NULL;

    //--Apriamo il file di configurazione passato come parametro
    filePointer = fopen(percorsoFileConfigurazione, "r");
    if (!filePointer) raiseError(FOPEN_SCOPE, FOPEN_ERROR);
    if ((getLineBytes = getline((char ** restrict) & tempStringConfigurationIP, (size_t * restrict) & IPlength, (FILE * restrict) filePointer)) == -1)
        raiseError(GETLINE_SCOPE, GETLINE_ERROR);
    * configurationIP = (char *) calloc(strlen(tempStringConfigurationIP), sizeof(char));
    if (! *configurationIP) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    strncpy(* configurationIP, (const char *) tempStringConfigurationIP, strlen(tempStringConfigurationIP) - 1);
    checkIP(* configurationIP);
    if ((getLineBytes = getline((char ** restrict) & stringServerindirizzoPort, (size_t * restrict) & portLength, (FILE * restrict) filePointer)) == -1)
        raiseError(GETLINE_SCOPE, GETLINE_ERROR);
    * configurationPort = (unsigned short int) strtoul((const char * restrict) stringServerindirizzoPort, (char ** restrict) NULL, 10);
    if (configurationPort == 0 && (errno == EINVAL || errno == ERANGE))
        raiseError(STRTOUL_SCOPE, STRTOUL_ERROR);
    //--Deallochiamo i puntatori
    free(tempStringConfigurationIP);
    free(stringServerindirizzoPort);
    //--Chiudiamo del file
    fclose(filePointer);
}

//--Calcoliamo il periodo di validità dello specifico GreenPass
char * getdataScadenzaVaccino (void) {
    time_t systemTimeSeconds = time(NULL);
    //--Consideriamo la data di sistema attuale
    struct tm * expirationDateTimeInfo = localtime((const time_t *) & systemTimeSeconds);
    
    //--Tronchiamo la data al primo giorno del mese
    expirationDateTimeInfo->tm_mday = 1;
    
    //--Aggiungiamo 6 mesi e si verifica se è cambiato l'anno
    if (expirationDateTimeInfo->tm_mon + MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE + 1 > 11) {
        expirationDateTimeInfo->tm_year += 1;
        expirationDateTimeInfo->tm_mon = (expirationDateTimeInfo->tm_mon + MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE + 1) % MESI_IN_ANNO;
    } else {
        expirationDateTimeInfo->tm_mon = (expirationDateTimeInfo->tm_mon + MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE + 1);
    }
    
    //--Convertiamo in stringa della data e ritorno al chiamante
    char * dataScadenzaVaccino = (char *) calloc(LUNGHEZZA_DATA, sizeof(char));

    if (!dataScadenzaVaccino)
        raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    sprintf(dataScadenzaVaccino, "%02d-%02d-%d", expirationDateTimeInfo->tm_mday, expirationDateTimeInfo->tm_mon, expirationDateTimeInfo->tm_year + 1900);
    dataScadenzaVaccino[LUNGHEZZA_DATA - 1] = '\0';
    return dataScadenzaVaccino;
}

//--Questa procedura si occupa di calcolare e restutire la data attuale
char * getNowDate (void) {
    time_t tempTimeSeconds = time(NULL);
    struct tm * nowDateTimeInfo = localtime((const time_t *) & tempTimeSeconds);
    char * nowDate = (char *) calloc(LUNGHEZZA_DATA, sizeof(char));

    if (!nowDate)
        raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    
    sprintf(nowDate, "%02d-%02d-%d", nowDateTimeInfo->tm_mday, nowDateTimeInfo->tm_mon + 1, nowDateTimeInfo->tm_year + 1900);
    nowDate[LUNGHEZZA_DATA - 1] = '\0';
    return nowDate;
}

/*--Attraverso la seguente funzione possiamo ritornare il socket file descriptor di una connessione impostata col serverV.
In input passiamo il file di configurazione che è stato fornito al CentroVaccinale o al ServerG per mettersi in contatto con il ServerV.*/
int creaConnessioneConServerV (const char * percorsoFileConfigurazione) {
    struct sockaddr_in serverV_indirizzo;
    char * stringServerV_indirizzoIP = NULL;
    unsigned short int serverV_Port;
    int serverV_SFD;
    
    //--Ritorno del file di configurazione
    ritornaDatiDiConfigurazione(percorsoFileConfigurazione, & stringServerV_indirizzoIP, & serverV_Port);
    //--Impostiamo la comunicazione col serverV
    serverV_SFD = wsocket(AF_INET, SOCK_STREAM, 0);
    memset((void *) & serverV_indirizzo, 0, sizeof(serverV_indirizzo));
    serverV_indirizzo.sin_family = AF_INET;
    serverV_indirizzo.sin_port   = htons(serverV_Port);
    if (inet_pton(AF_INET, (const char * restrict) stringServerV_indirizzoIP, (void *) & serverV_indirizzo.sin_addr) <= 0)
        raiseError(INET_PTON_SCOPE, INET_PTON_ERROR);
    wconnect(serverV_SFD, (struct sockaddr *) & serverV_indirizzo, (socklen_t) sizeof(serverV_indirizzo));
    free(stringServerV_indirizzoIP);
    return serverV_SFD;
}
