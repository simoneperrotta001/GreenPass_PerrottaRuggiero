#include "serverV.h"

int main (int argc, char * argv[]) {
    int listenFD, connectionFD, enable = TRUE, threadCreationReturnValue;
    pthread_t singleTID;
    pthread_attr_t attr;
    unsigned short int portaServerV, identificatoreRichiesta;
    struct sockaddr_in indirizzoServerV, client;

    //-- Controlliamo che il ServerG sia stato avviato con i parametri corretti
    checkUtilizzo(argc, (const char **) argv, SERVER_V_ARGS_NO, messaggioAtteso);
    //-- Ricaviamo il numero di porta
    portaServerV = (unsigned short int) strtoul((const char * restrict) argv[1], (char ** restrict) NULL, 10);
    //--Se il numero di porta non dovesse essere valido viene lanciato un errore
    if (portaServerV == 0 && (errno == EINVAL || errno == ERANGE))
        raiseError(STRTOUL_SCOPE, STRTOUL_ERROR);
    //--Creiamo il socket per mettere in ascolto il ServerV, così da poter ricevere richeiste di connessione da parte di ServerG e centroVaccinale
    listenFD = wsocket(AF_INET, SOCK_STREAM, 0);
    //--Durante l'applicazione del meccanismo di IPC, con le socket, impostiamo l'opzione di riutilizzo degli indirizzi
    if (setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, & enable, (socklen_t) sizeof(enable))  == -1)
        raiseError(SET_SOCK_OPT_SCOPE, SET_SOCK_OPT_ERROR);
    memset((void *) & indirizzoServerV, 0, sizeof(indirizzoServerV));
    memset((void *) & client, 0, sizeof(client));
    indirizzoServerV.sin_family      = AF_INET;
    indirizzoServerV.sin_addr.s_addr = htonl(INADDR_ANY);
    indirizzoServerV.sin_port        = htons(portaServerV);
    wbind(listenFD, (struct sockaddr *) & indirizzoServerV, (socklen_t) sizeof(indirizzoServerV));
    wlisten(listenFD, LISTEN_QUEUE_SIZE * LISTEN_QUEUE_SIZE);
    
    
    //--Inizializziamo un mutex che servirà per effettuare la mutua esclusione per l'accesso al file serverV.dat
    if (pthread_mutex_init(& fileSystemAccessMutex, (const pthread_mutexattr_t *) NULL) != 0)
        raiseError(PTHREAD_MUTEX_INIT_SCOPE, PTHREAD_MUTEX_INIT_ERROR);
    /*
    --Inizializziamo un mutex su un puntatore a intero che punta a un intero rappresentate il socket file descriptor che
    il singolo thread userà per mettersi in collegamento con il ServerG o con il CentroVaccinale.*/
    if (pthread_mutex_init(& connectionFDMutex, (const pthread_mutexattr_t *) NULL) != 0)
        raiseError(PTHREAD_MUTEX_INIT_SCOPE, PTHREAD_MUTEX_INIT_ERROR);
    // --Inizializziamo gli attributi di entrambi i mutex.
    if (pthread_attr_init(& attr) != 0) raiseError(PTHREAD_MUTEX_ATTR_INIT_SCOPE, PTHREAD_MUTEX_ATTR_INIT_ERROR);
    /*--La seguente funzione segna uno specifico thread. Quando questo thread termina
    le sue risorse saranno automaticamente deallocate e ritornate al sistema senza l'aiuto
    di un altro thread.*/
    if (pthread_attr_setdetachstate(& attr, PTHREAD_CREATE_DETACHED) != 0)
        raiseError(PTHREAD_ATTR_DETACH_STATE_SCOPE, PTHREAD_ATTR_DETACH_STATE_ERROR);
    
    while (TRUE) {
        ssize_t fullReadReturnValue;
        socklen_t lunghezzaIndirizzoClient = (socklen_t) sizeof(client);
        while ((connectionFD = waccept(listenFD, (struct sockaddr *) & client, (socklen_t *) & lunghezzaIndirizzoClient)) < 0 && (errno == EINTR));
        // fullRead per leggere l'ID dell'entità connessa con il ServerV.
        if ((fullReadReturnValue = fullRead(connectionFD, (void *) & identificatoreRichiesta, sizeof(identificatoreRichiesta))) != 0) raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);

        //--Preleviamo il mutex sul socket file descriptor di connessione.
        if (pthread_mutex_lock(& connectionFDMutex) != 0) raiseError(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR);
        // Allochiamo l'area di memoria necessaria per contenere il duplicato del FD connesso
        int * threadconnectionFD = (int *) calloc(1, sizeof(* threadconnectionFD));
        if (!threadconnectionFD)
            raiseError(CALLOC_SCOPE, CALLOC_ERROR);
        /*--La funzione dup() in C è utilizzata per duplicare un file descriptor esistente. La funzione prende in input un intero che rappresenta
         il file descriptor del file da duplicare e ritorna un intero che rappresenta il nuovo file descriptor del file duplicato.
         Il nuovo file descriptor punta allo stesso file del file descriptor originale, quindi le due descrizioni del file sono identiche,
         ma indipendenti. In altre parole, le modifiche apportate a un file descriptor non influenzano l'altro e viceversa.
         L'utilità principale della funzione dup() è quella di creare una seconda copia del file descriptor associato a un file aperto in modo
         che lo stesso file possa essere letto o scritto da due parti diverse del programma senza dover riaprire il file.*/
        if ((* threadconnectionFD = dup(connectionFD)) < 0)
            raiseError(DUP_SCOPE, DUP_ERROR);
        
        // --Rilasciamo il mutex usufruito per il socket file descriptor originale
        if (pthread_mutex_unlock(& connectionFDMutex) != 0)
            raiseError(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR);
        // Individuiamo la routine apposita da invocare a seconda del mittente della richiesta pervenuta al ServerV
        switch (identificatoreRichiesta) {
            //--Gestione servizio per il Centro vaccinale
            case centroVaccinaleSender:
                //--Sempre con lo stesso TID creiamo un thread apposito per gestire il servizio.
                if ((threadCreationReturnValue = pthread_create(& singleTID, & attr, & centroVaccinaleRequestHandler, threadconnectionFD)) != 0) raiseError(PTHREAD_CREATE_SCOPE, PTHREAD_CREATE_ERROR);
                break;
            //--Gestione servizio per il ClientS via ServerG
            case clientS_viaServerG_Sender:
                //--Sempre con lo stesso TID creiamo un thread apposito per gestire il servizio.
                if ((threadCreationReturnValue = pthread_create(& singleTID, & attr, & clientS_viaServerG_RequestHandler, threadconnectionFD)) != 0) raiseError(PTHREAD_CREATE_SCOPE, PTHREAD_CREATE_ERROR);
                break;
            //--Gestione servizio per il ClientT via ServerG
            case clientT_viaServerG_Sender:
                //--Sempre con lo stesso TID creiamo un thread apposito per gestire il servizio.
                if ((threadCreationReturnValue = pthread_create(& singleTID, & attr, & clientT_viaServerG_RequestHandler, threadconnectionFD)) != 0) raiseError(PTHREAD_CREATE_SCOPE, PTHREAD_CREATE_ERROR);
                break;
            default:
                raiseError(INVALID_SENDER_ID_SCOPE, INVALID_SENDER_ID_ERROR);
                break;
        }
        /*--Chiudiamo il socket file descriptor relativo a quella connessione nel thread main e il flusso principale del
        ServerV si rimette in attesa di una nuova connessione.*/
        wclose(connectionFD);
    }
    //--Deallochiamo la memoria dinamica allocata in precedenza utilizzando destroy
    if (pthread_attr_destroy(& attr) != 0)
        raiseError(PTHREAD_MUTEX_ATTR_DESTROY_SCOPE, PTHREAD_MUTEX_ATTR_DESTROY_ERROR);
    wclose(listenFD);
    exit(0);
}

void * centroVaccinaleRequestHandler (void * args) {
    /*-- Utilizziamo il mutex per deferenziare in mutua esclusione il valore contenuto nel puntatore della routine assegnata
     al thread e deallocare la memoria dinamica ad esso associata.*/
    if (pthread_mutex_lock(& connectionFDMutex) != 0)
        threadraiseError(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR);
    int threadconnectionFD = * ((int *) args);
    free(args);
    //-- Rilasciamo il mutex
    if (pthread_mutex_unlock(& connectionFDMutex) != 0)
        threadraiseError(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR);
    ssize_t fullWriteReturnValue, fullReadReturnValue, getLineBytes;
    size_t effectiveLineLength = 0;
    char * singleLine = NULL, * dataAttualeString = NULL;
    char dateCopiedFromFile[LUNGHEZZA_DATA];
    struct tm firstTime, secondTime;
    time_t scheduledVaccinationDate, requestVaccinationDate = 0;
    double mesiPassati;
    enum boolean VaccinoBloccato = FALSE, codiceTesseraSanitariaTrovato = FALSE;
    FILE * originalFilePointer, * tempFilePointer;
    
    //--Allochiamo la memoria per i dati provenienti dal Centro Vaccinale e la risposta del ServerV
    centroVaccinaleRichiedeAServerV * nuovaRichiestaCentroVaccinale = (centroVaccinaleRichiedeAServerV *) calloc(1, sizeof(* nuovaRichiestaCentroVaccinale));
    if (!nuovaRichiestaCentroVaccinale)
        threadAbort(CALLOC_SCOPE, CALLOC_ERROR, threadconnectionFD, NULL);
    
    serverVRispondeACentroVaccinale * nuovaRispostaServerV = (serverVRispondeACentroVaccinale *) calloc(1, sizeof(* nuovaRispostaServerV));
    if (!nuovaRispostaServerV)
        threadAbort(CALLOC_SCOPE, CALLOC_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale);
    
    //--fullRead per attendere la richeista del centro Vaccinale
    if ((fullReadReturnValue = fullRead(threadconnectionFD, (void *) nuovaRichiestaCentroVaccinale, sizeof(* nuovaRichiestaCentroVaccinale))) != 0)
        threadAbort(FULL_READ_SCOPE, (int) fullReadReturnValue, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV);
    // Copia del codice della tessera sanitaria nel pacchetto di risposta.
    strncpy((char *) nuovaRispostaServerV->codiceTesseraSanitaria, (const char *) nuovaRichiestaCentroVaccinale->codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    nuovaRispostaServerV->requestResult = FALSE;
    
    //--Utilizziamo il mutex

    if (pthread_mutex_lock(& fileSystemAccessMutex) != 0)
        threadAbort(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV);
    //--Apertura in modalità sola lettura per il file "serverV.dat"
    originalFilePointer = fopen(dataPath, "r");
    //--Apertura in modalità sola scrittura per il file "tempServerV.dat"
    tempFilePointer = fopen(tempDataPath, "w");
    if (!originalFilePointer || !tempFilePointer) {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
            threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV);
        threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV);
    }
    
    //--Verifichiamo la validità del  GreenPass (se risulta scaduto o meno)
    while ((getLineBytes = getline((char ** restrict) & singleLine, (size_t * restrict) & effectiveLineLength, (FILE * restrict) originalFilePointer)) != -1) {
        /*
         --Controlliamo se il numero di tessera sanitaria incontrato nel file corrisponde a quello inviato dal Centro Vaccinale
         e che già è presente nel pacchetto di risposta del ServerV.
         */
        if ((strncmp((const char *) nuovaRispostaServerV->codiceTesseraSanitaria, (const char *) singleLine, LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1)) == 0) {
            //--Se c'è corrispondenza
            codiceTesseraSanitariaTrovato = TRUE;
            //--Copiamo la data all'interno del file in una variabile di tipo stringa
            strncpy((char *) dateCopiedFromFile, (const char *) singleLine + LUNGHEZZA_CODICE_TESSERA_SANITARIA, LUNGHEZZA_DATA - 1);
            dateCopiedFromFile[LUNGHEZZA_DATA - 1] = '\0';
            memset(& firstTime, 0, sizeof(firstTime));
            memset(& secondTime, 0, sizeof(secondTime));
            
            //--Confrontiamo tale data con la data attuale
            firstTime.tm_mday = (int) strtol((const char * restrict) & dateCopiedFromFile[0], (char ** restrict) NULL, 10);
            firstTime.tm_mon = ((int) strtol((const char * restrict) & dateCopiedFromFile[3], (char ** restrict) NULL, 10) - 1);
            firstTime.tm_year = ((int) strtol((const char * restrict) & dateCopiedFromFile[6], (char ** restrict) NULL, 10) - 1900);
            dataAttualeString = getdataAttuale();
            secondTime.tm_mday = ((int) strtol((const char * restrict) & dataAttualeString[0], (char ** restrict) NULL, 10));
            secondTime.tm_mon = ((int) strtol((const char * restrict) & dataAttualeString[3], (char ** restrict) NULL, 10) - 1);
            secondTime.tm_year = ((int) strtol((const char * restrict) & dataAttualeString[6], (char ** restrict) NULL, 10) - 1900);
            
            if ((firstTime.tm_mday == 0 || firstTime.tm_mon == 0 || firstTime.tm_year == 0 || secondTime.tm_mday == 0 || secondTime.tm_mon == 0 || secondTime.tm_year == 0) && (errno == EINVAL || errno == ERANGE)) {
                fclose(originalFilePointer);
                fclose(tempFilePointer);
                if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                    threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
                threadAbort(STRTOL_SCOPE, STRTOL_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
            }
            
            if (((scheduledVaccinationDate = mktime(& firstTime)) == -1) || ((requestVaccinationDate = mktime(& secondTime)) == -1)) {
                fclose(originalFilePointer);
                fclose(tempFilePointer);
                if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                    threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
                threadAbort(MKTIME_SCOPE, MKTIME_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
            }
            
            //--Calcoliamo i mesi di differenza tra le due date
            mesiPassati = ((((difftime(scheduledVaccinationDate, requestVaccinationDate) / 60) / 60) / 24) / 31);
            /*
             Se il numero di mesi passati è minore del numero di mesi da aspettare per la successiva vaccinazione e
             il numero di mesi di differenza è maggiore di 0 il    GreenPass non è ancora scaduto.
             */
            if (mesiPassati <= (MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE + 1) && mesiPassati > 0) {
                // Copiamo la data dal file "serverV.dat" nella risposta da mandare al "CentroVaccinale"
                strncpy((char *) nuovaRispostaServerV->dataScadenzaGreenPass, (const char *) dateCopiedFromFile, LUNGHEZZA_DATA);
                VaccinoBloccato = TRUE;
            }
            break;
        }
    }
    
    //--Se è possibile fare la vaccinazione
    if (!VaccinoBloccato) {
        //--Copiamo la data che è stata inviata dal Centro Vaccinale al pacchetto di risposta.
        strncpy((char *) nuovaRispostaServerV->dataScadenzaGreenPass, (const char *) nuovaRichiestaCentroVaccinale->dataScadenzaGreenPass, LUNGHEZZA_DATA);
        //--Aggiungiamo l'esito della richiesta di vaccinazione
        nuovaRispostaServerV->requestResult = TRUE;
        //--Chiudiamo e riapriamo del file per riinizializzare il file pointer all'inizio del file
        fclose(originalFilePointer);
        originalFilePointer = fopen(dataPath, "r");
        if (!originalFilePointer) {
            fclose(tempFilePointer);
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
            threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
        }
        
        //--Salviamo nel file serverV.dat la nuova data di scadenza della vaccinazione
        while ((getLineBytes = getline(& singleLine, & effectiveLineLength, originalFilePointer)) != -1) {
            if ((strncmp((const char *) nuovaRispostaServerV->codiceTesseraSanitaria, (const char *) singleLine, LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1)) != 0) {
                //--Confontiamo il codice della tessera sanitaria di input del client con quello memorizzato nel file "serverV.dat"
                if (fprintf(tempFilePointer, "%s", singleLine) < 0) {
                    fclose(originalFilePointer);
                    fclose(tempFilePointer);
                    //Copiamo tutto il file "serverV.dat nel file temporaneo "tempServerV.dat"
                    if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                        threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
                    threadAbort(FPRINTF_SCOPE, FPRINTF_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
                }
            }
        }
        
        //--Inseriamo nel file temporaneo della tupla: codice, data validità e stato di validità.
        if (fprintf(tempFilePointer, "%s:%s:%s\n", nuovaRispostaServerV->codiceTesseraSanitaria, nuovaRispostaServerV->dataScadenzaGreenPass, "1") < 0) {
            fclose(originalFilePointer);
            fclose(tempFilePointer);
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
            threadAbort(FPRINTF_SCOPE, FPRINTF_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
        }
        
        // Update Files
        fclose(originalFilePointer);
        fclose(tempFilePointer);
        //--Eliminiamo il file originale
        if (remove(dataPath) != 0) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
            threadAbort(REMOVE_SCOPE, REMOVE_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
        }
        
        //--Ridenominiamo il file temporaneo con il nome del file originale
        if (rename(tempDataPath, dataPath) != 0) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
            threadAbort(RENAME_SCOPE, RENAME_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
        }
        
        //--Creiamo un file temporaneo e lo apriamo
        tempFilePointer = fopen(tempDataPath, "w+");
        if (!tempFilePointer) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
            threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
        }
        //Chiudiamo il file temporaneo
        fclose(tempFilePointer);
    } else {
        fclose(originalFilePointer);
        fclose(tempFilePointer);
    }

    //--Inviamo la rispsota se il codice della tessera sanitaria è stato individuato
    if (codiceTesseraSanitariaTrovato) {
        // Rilasciamo il mutex sul file system per l'accesso ai file.
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
            threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
        
        // fullWrite per l'invio della risposta al Centro Vaccinale dell'esito positivo o negativo della richiesta effettuata
        if ((fullWriteReturnValue = fullWrite(threadconnectionFD, (const void *) nuovaRispostaServerV, sizeof(* nuovaRispostaServerV))) != 0) threadAbort(FULL_WRITE_SCOPE, (int) fullWriteReturnValue, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV, dataAttualeString, singleLine);
        free(dataAttualeString);
        free(singleLine);
    } else {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
            threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV);
        
        // fullWrite per l'invio della risposta al Centro Vaccinale dell'esito negativo della richiesta effettuata
        if ((fullWriteReturnValue = fullWrite(threadconnectionFD, (const void *) nuovaRispostaServerV, sizeof(* nuovaRispostaServerV))) != 0)
            threadAbort(FULL_WRITE_SCOPE, (int) fullWriteReturnValue, threadconnectionFD, nuovaRichiestaCentroVaccinale, nuovaRispostaServerV);
    }
    
    wclose(threadconnectionFD);
    free(nuovaRichiestaCentroVaccinale);
    free(nuovaRispostaServerV);
    pthread_exit(NULL);
}

void * clientS_viaServerG_RequestHandler(void * args) {
    /*-- Utilizziamo il mutex per deferenziare, in mutua esclusione, il valore contenuto nel puntatore della routine assegnata
     al thread.*/
    if (pthread_mutex_lock(& connectionFDMutex) != 0)
        threadraiseError(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR);
    int threadconnectionFD = * ((int *) args);
    free(args);
    //--Rilasciamo il mutex
    if (pthread_mutex_unlock(& connectionFDMutex) != 0)
        threadraiseError(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR);
    ssize_t fullWriteReturnValue, fullReadReturnValue, getLineBytes;
    size_t effectiveLineLength = 0;
    char * singleLine = NULL, * dataAttualeString = NULL;
    struct tm firstTime, secondTime;
    time_t scheduledVaccinationDate = 0, requestVaccinationDate = 0;
    double mesiPassati;
    unsigned short int greenPassStatus;
    enum boolean isGreenPassExpired = TRUE, codiceTesseraSanitariaTrovato = FALSE, isGreenPassValid = FALSE;
    FILE * originalFilePointer;
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA], dateCopiedFromFile[LUNGHEZZA_DATA], greenPassStatusString[2];

    
    //--Allocchiamo la memoria per il pacchetto risposta del ServerV
    serverV_ReplyToServerG_clientS * nuovaRispostaServerV = (serverV_ReplyToServerG_clientS *) calloc(1, sizeof(* nuovaRispostaServerV));
    if (!nuovaRispostaServerV)
        threadAbort(CALLOC_SCOPE, CALLOC_ERROR, threadconnectionFD, NULL);

    // fullRead per attendere la ricezione del codice della tessera sanitaria
    if ((fullReadReturnValue = fullRead(threadconnectionFD, (void *) codiceTesseraSanitaria, (size_t) LUNGHEZZA_CODICE_TESSERA_SANITARIA * sizeof(char))) != 0)
        threadAbort(FULL_READ_SCOPE, (int) fullReadReturnValue, threadconnectionFD, nuovaRispostaServerV);

    //--Copiamo l' HCN nel pacchetto di risposta
    strncpy((char *) nuovaRispostaServerV->codiceTesseraSanitaria, (const char *) codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    nuovaRispostaServerV->requestResult = FALSE;

    //--Utilizziamo il mutex per accedere alla sezione critica in mutua esclusione

    if (pthread_mutex_lock(& fileSystemAccessMutex) != 0)
        threadAbort(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR, threadconnectionFD, nuovaRispostaServerV);
    //--Apriamo il file "serverV.dat" in lettura
    originalFilePointer = fopen(dataPath, "r");
    if (!originalFilePointer) {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
            threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRispostaServerV);
        threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadconnectionFD, nuovaRispostaServerV);
    }

    //--Verifichiamo la validità del  GreenPass (se risulta scaduto o meno)
    while ((getLineBytes = getline(& singleLine, & effectiveLineLength, originalFilePointer)) != -1) {
        /*
         --Controlliamo se il numero di tessera sanitaria incontrato nel file corrisponde a quello inviato dal Centro Vaccinale
         e che già è presente nel pacchetto di risposta del ServerV.
         */
        if ((strncmp((const char *) nuovaRispostaServerV->codiceTesseraSanitaria, (const char *) singleLine, LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1)) == 0) {
            //--Se c'è corrispondenza
            codiceTesseraSanitariaTrovato = TRUE;
            //--Copiamo la data all'interno del file in una variabile di tipo stringa
            strncpy((char *) dateCopiedFromFile, (const char *) singleLine + LUNGHEZZA_CODICE_TESSERA_SANITARIA, LUNGHEZZA_DATA - 1);
            dateCopiedFromFile[LUNGHEZZA_DATA - 1] = '\0';
            memset(& firstTime, 0, sizeof(firstTime));
            memset(& secondTime, 0, sizeof(secondTime));

            //--Confrontiamo tale data con la data attuale
            firstTime.tm_mday = (int) strtol((const char * restrict) & dateCopiedFromFile[0], (char ** restrict) NULL, 10);
            firstTime.tm_mon = ((int) strtol((const char * restrict) & dateCopiedFromFile[3], (char ** restrict) NULL, 10) - 1);
            firstTime.tm_year = ((int) strtol((const char * restrict) & dateCopiedFromFile[6], (char ** restrict) NULL, 10) - 1900);
            dataAttualeString = getdataAttuale();
            secondTime.tm_mday = ((int) strtol((const char * restrict) & dataAttualeString[0], (char ** restrict) NULL, 10));
            secondTime.tm_mon = ((int) strtol((const char * restrict) & dataAttualeString[3], (char ** restrict) NULL, 10) - 1);
            secondTime.tm_year = ((int) strtol((const char * restrict) & dataAttualeString[6], (char ** restrict) NULL, 10) - 1900);

            if ((firstTime.tm_mday == 0 || firstTime.tm_mon == 0 || firstTime.tm_year == 0 || secondTime.tm_mday == 0 || secondTime.tm_mon == 0 || secondTime.tm_year == 0) && (errno == EINVAL || errno == ERANGE)) {
                fclose(originalFilePointer);
                if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                    threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRispostaServerV, dataAttualeString, singleLine);
                threadAbort(STRTOL_SCOPE, STRTOL_ERROR, threadconnectionFD, nuovaRispostaServerV, dataAttualeString, singleLine);
            }

            if (((scheduledVaccinationDate = mktime(& firstTime)) == -1) || ((requestVaccinationDate = mktime(& secondTime)) == -1)) {
                fclose(originalFilePointer);
                if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                    threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRispostaServerV, dataAttualeString, singleLine);
                threadAbort(MKTIME_SCOPE, MKTIME_ERROR, threadconnectionFD, nuovaRispostaServerV, dataAttualeString, singleLine);
            }

            //--Calcoliamo i mesi di differenza tra le due date
            mesiPassati = ((((difftime(scheduledVaccinationDate, requestVaccinationDate) / 60) / 60) / 24) / 31);
            /*
             Se il numero di mesi passati è minore del numero di mesi da aspettare per la successiva vaccinazione e
             il numero di mesi di differenza è maggiore di 0 il    GreenPass non è ancora scaduto.
             */
            if (mesiPassati <= (MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE + 1) && mesiPassati > 0) isGreenPassExpired = FALSE;
            strncpy((char *) greenPassStatusString, (const char *) singleLine + LUNGHEZZA_CODICE_TESSERA_SANITARIA + LUNGHEZZA_DATA, 1);
            greenPassStatusString[1] = '\0';
            //--Convertiamo lo stato del  GreenPass da stringa a intero
            greenPassStatus = (unsigned short int) strtoul((const char * restrict) greenPassStatusString, (char ** restrict) NULL, 10);
            
            //--Controlliamo la validità dello stato: se FALSE significa che non è valido, altrimenti è valido.
            if ((greenPassStatus == FALSE) && (errno == EINVAL || errno == ERANGE)) {
                fclose(originalFilePointer);
                if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                    threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRispostaServerV, dataAttualeString, singleLine);
                threadAbort(STRTOUL_SCOPE, STRTOUL_ERROR, threadconnectionFD, nuovaRispostaServerV, dataAttualeString, singleLine);
            }
            if (greenPassStatus == TRUE) isGreenPassValid = TRUE;
            break;
        }
    }

    fclose(originalFilePointer);
    //--Se troviamo il numero di tessera sanitaria nel file "ServerV.dat"
    if (codiceTesseraSanitariaTrovato) {
        //--Utilizziamo il mutex per l'accesso al file system.
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
            threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRispostaServerV, dataAttualeString, singleLine);

        //Se il  GreenPass è valido e non è scaduto, si inserisce nel pacchetto di risposta l'esito dei controlli
        if (isGreenPassValid && !isGreenPassExpired) nuovaRispostaServerV->requestResult = TRUE;
        
        // fullWrite per inviare il pacchetto
        if ((fullWriteReturnValue = fullWrite(threadconnectionFD, (const void *) nuovaRispostaServerV, sizeof(* nuovaRispostaServerV))) != 0) threadAbort(FULL_WRITE_SCOPE, (int) fullWriteReturnValue, threadconnectionFD, nuovaRispostaServerV, dataAttualeString, singleLine);
        free(dataAttualeString);
        free(singleLine);
    } else {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
            threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRispostaServerV);
        
        if ((fullWriteReturnValue = fullWrite(threadconnectionFD, (const void *) nuovaRispostaServerV, sizeof(* nuovaRispostaServerV))) != 0) threadAbort(FULL_WRITE_SCOPE, (int) fullWriteReturnValue, threadconnectionFD, nuovaRispostaServerV);
    }

    free(nuovaRispostaServerV);
    wclose(threadconnectionFD);
    pthread_exit(NULL);
}

void * clientT_viaServerG_RequestHandler(void * args) {
    /*-- Utilizziamo il mutex per deferenziare, in mutua esclusione, il valore contenuto nel puntatore della routine assegnata
     al thread.*/
    if (pthread_mutex_lock(& connectionFDMutex) != 0) threadraiseError(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR);
    int threadconnectionFD = * ((int *) args);
    free(args);
    // Rilasciamo il mutex
    if (pthread_mutex_unlock(& connectionFDMutex) != 0) threadraiseError(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR);
    ssize_t fullWriteReturnValue, fullReadReturnValue, getLineBytes;
    size_t effectiveLineLength = 0;
    char * singleLine = NULL;;
    enum boolean codiceTesseraSanitariaTrovato = FALSE;
    FILE * originalFilePointer, * tempFilePointer = NULL;
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA], dateCopiedFromFile[LUNGHEZZA_DATA];

    
    //--Allochiamo memoria per i pacchetti: il primo proveniente dal ServerG e il secondo di risposta dal ServerV.
    serverG_RequestToServerV_onBehalfOfClientT * newServerG_Request = (serverG_RequestToServerV_onBehalfOfClientT *) calloc(1, sizeof(* newServerG_Request));
    if (!newServerG_Request)
        threadAbort(CALLOC_SCOPE, CALLOC_ERROR, threadconnectionFD, NULL);

    serverV_ReplyToServerG_clientT * nuovaRispostaServerV = (serverV_ReplyToServerG_clientT *) calloc(1, sizeof(* nuovaRispostaServerV));
    if (!nuovaRispostaServerV)
        threadAbort(CALLOC_SCOPE, CALLOC_ERROR, threadconnectionFD, newServerG_Request);

    // fullRead per attendere la ricezione del pacchetto di richiesta del ServerG
    if ((fullReadReturnValue = fullRead(threadconnectionFD, (void *) newServerG_Request, sizeof(* newServerG_Request))) != 0) {
        threadAbort(FULL_READ_SCOPE, (int) fullReadReturnValue, threadconnectionFD, newServerG_Request, nuovaRispostaServerV);
    }

    // --Copiamo il codice della tessera sanitaria nel pacchetto di risposta
    strncpy((char *) codiceTesseraSanitaria, (const char *) newServerG_Request->codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    strncpy((char *) nuovaRispostaServerV->codiceTesseraSanitaria, (const char *) codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    nuovaRispostaServerV->updateResult = FALSE;

    //--Utilizziamo il mutex per accedere alla sezione critica in mutua esclusione ed effetturare alcune operazioni
    if (pthread_mutex_lock(& fileSystemAccessMutex) != 0)
        threadAbort(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV);
    originalFilePointer = fopen(dataPath, "r");
    if (!originalFilePointer) {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, nuovaRispostaServerV);
        threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV);
    }

    //--Leggiamo il file
    while ((getLineBytes = getline(& singleLine, & effectiveLineLength, originalFilePointer)) != -1) {
        //--Controlliamo la coincidenza tra il codice di tessera sanitaria di input e quelli nel file
        if ((strncmp((const char *) nuovaRispostaServerV->codiceTesseraSanitaria, (const char *) singleLine, LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1)) == 0) {
            //--Se c'è
            codiceTesseraSanitariaTrovato = TRUE;
            //--Salviamo la data di scadenza del Vannila  GreenPass
            strncpy((char *) dateCopiedFromFile, (const char *) singleLine + LUNGHEZZA_CODICE_TESSERA_SANITARIA, LUNGHEZZA_DATA - 1);
            dateCopiedFromFile[LUNGHEZZA_DATA - 1] = '\0';
            break;
        }
    }

    fclose(originalFilePointer);
    //--Se il codice è stato trovato si può modificare lo stato di validità del    GreenPass
    if (codiceTesseraSanitariaTrovato) {
        //--Apriamo i due file: l'originale e il temporaneo
        originalFilePointer = fopen(dataPath, "r");
        tempFilePointer = fopen(tempDataPath, "w");
        if (!originalFilePointer || !tempFilePointer) {
            fclose(originalFilePointer);
            fclose(tempFilePointer);
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
            threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
        }

        //--Salviamo nel file serverV.dat il nuovo stato del    GreenPass + lettura linea per linea del file originale.
        while ((getLineBytes = getline(& singleLine, & effectiveLineLength, originalFilePointer)) != -1) {
            // Se non c'è corrispondenza tra i codici della tessera sanitaria
            if ((strncmp(nuovaRispostaServerV->codiceTesseraSanitaria, singleLine, LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1)) != 0) {
                //--Copiamo la riga del file "serverV.dat" in "tempServerV.dat"
                if (fprintf(tempFilePointer, "%s", singleLine) < 0) {
                    fclose(originalFilePointer);
                    fclose(tempFilePointer);
                    if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                        threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
                    threadAbort(FPRINTF_SCOPE, FPRINTF_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
                }
            }
        }

        //--Inseriamo i nuovi valori del    GreenPass modificato nel file temporaneo
        if (fprintf(tempFilePointer, "%s:%s:%hu\n", nuovaRispostaServerV->codiceTesseraSanitaria, dateCopiedFromFile, newServerG_Request->updateValue) < 0) {
            fclose(originalFilePointer);
            fclose(tempFilePointer);
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
            threadAbort(FPRINTF_SCOPE, FPRINTF_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
        }

        // updateFile
        fclose(originalFilePointer);
        fclose(tempFilePointer);
        //--Cancelliamo il file originale
        if (remove(dataPath) != 0) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
            threadAbort(REMOVE_SCOPE, REMOVE_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
        }

        //--Ridenominiamo il temporaneo con il nome del file originale
        if (rename(tempDataPath, dataPath) != 0) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
            threadAbort(RENAME_SCOPE, RENAME_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
        }

        //--Creaiamo il file temporaneo
        tempFilePointer = fopen(tempDataPath, "w+");
        if (!tempFilePointer) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
                threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
            threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV, singleLine);
        }
        //--Chiudiamo il file temporaneo
        fclose(tempFilePointer);
        //--Salviamo l'esito della modifica nel pacchetto di risposta
        nuovaRispostaServerV->updateResult = TRUE;
        free(singleLine);
    }

    // fullWrite per l'invio del pacchetto al ServerG
    if ((fullWriteReturnValue = fullWrite(threadconnectionFD, (const void *) nuovaRispostaServerV, sizeof(* nuovaRispostaServerV))) != 0) {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
            threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV);
        threadAbort(FULL_WRITE_SCOPE, (int) fullWriteReturnValue, threadconnectionFD, newServerG_Request, nuovaRispostaServerV);
    }

    //--Rilasciamo il mutex
    if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0)
        threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadconnectionFD, newServerG_Request, nuovaRispostaServerV);
    wclose(threadconnectionFD);
    free(nuovaRispostaServerV);
    free(newServerG_Request);
    pthread_exit(NULL);
}

/*
 --Questa procedura si occupa di deallocare tutte le risorse passate tramite una lista di puntatori di lunghezza
 variabile.
*/
void threadAbort (char * errorScope, int exitCode, int threadconnectionFD, void * arg1, ...) {
    /*
    --Chiudiamo il socket file descriptor di connessione usato dal thread di turno per comunicare col
    ServerG o con il CentroVaccinale
    */
    wclose(threadconnectionFD);
    va_list argumentsList;
    void * currentElement;
    if (arg1 != NULL) {
        free(arg1);
        va_start(argumentsList, arg1);
        // Deallochiamo tutta la memoria dinamica sottoforma di lista di puntatori a void
        while ((currentElement = va_arg(argumentsList, void *)) != 0) free(currentElement);
        va_end(argumentsList);
    }
    threadraiseError(errorScope, exitCode);
}
