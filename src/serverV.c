#include "serverV.h"

int main (int argc, char * argv[]) {
    int listenFD, connectionFileDescriptor, enable = TRUE, threadCreationReturnValue;
    pthread_t singleTID;
    pthread_attr_t attr;
    unsigned short int serverV_Port, requestIdentifier;
    struct sockaddr_in serverV_Address, client;

    //-- Controlliamo che il ServerG sia stato avviato con i parametri corretti
    checkUsage(argc, (const char **) argv, SERVER_V_ARGS_NO, messaggioAtteso);
    //-- Ricaviamo il numero di porta
    serverV_Port = (unsigned short int) strtoul((const char * restrict) argv[1], (char ** restrict) NULL, 10);
    //--Se il numero di porta non dovesse essere valido viene lanciato un errore
    if (serverV_Port == 0 && (errno == EINVAL || errno == ERANGE)) raiseError(STRTOUL_SCOPE, STRTOUL_ERROR);
    
    listenFD = wsocket(AF_INET, SOCK_STREAM, 0);
    //--Durante l'applicazione del meccanismo di IPC, con le socket, impostiamo l'opzione di riutilizzo degli indirizzi
    if (setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, & enable, (socklen_t) sizeof(enable))  == -1) raiseError(SET_SOCK_OPT_SCOPE, SET_SOCK_OPT_ERROR);
    memset((void *) & serverV_Address, 0, sizeof(serverV_Address));
    memset((void *) & client, 0, sizeof(client));
    serverV_Address.sin_family      = AF_INET;
    serverV_Address.sin_addr.s_addr = htonl(INADDR_ANY);
    serverV_Address.sin_port        = htons(serverV_Port);
    wbind(listenFD, (struct sockaddr *) & serverV_Address, (socklen_t) sizeof(serverV_Address));
    wlisten(listenFD, LISTEN_QUEUE_SIZE * LISTEN_QUEUE_SIZE);
    
    
    //--Inizializziamo un mutex che servirà per effettuare la mutua esclusione per l'accesso al file serverV.dat
    if (pthread_mutex_init(& fileSystemAccessMutex, (const pthread_mutexattr_t *) NULL) != 0) raiseError(PTHREAD_MUTEX_INIT_SCOPE, PTHREAD_MUTEX_INIT_ERROR);
    /*
    --Inizializziamo un mutex su un puntatore a intero che punta a un intero rappresentate il socket file descriptor che
    il singolo thread userà per mettersi in collegamento con il ServerG o con il CentroVaccinale.
    */
    if (pthread_mutex_init(& connectionFileDescriptorMutex, (const pthread_mutexattr_t *) NULL) != 0) raiseError(PTHREAD_MUTEX_INIT_SCOPE, PTHREAD_MUTEX_INIT_ERROR);
    // --Inizializziamo gli attributi di entrambi i mutex.
    if (pthread_attr_init(& attr) != 0) raiseError(PTHREAD_MUTEX_ATTR_INIT_SCOPE, PTHREAD_MUTEX_ATTR_INIT_ERROR);
    /*
    The pthread_detach() function marks the thread identified by
    thread as detached.  When a detached thread terminates, its
    resources are automatically released back to the system without
    the need for another thread to join with the terminated thread.
    */
    if (pthread_attr_setdetachstate(& attr, PTHREAD_CREATE_DETACHED) != 0) raiseError(PTHREAD_ATTR_DETACH_STATE_SCOPE, PTHREAD_ATTR_DETACH_STATE_ERROR);
    
    while (TRUE) {
        ssize_t fullReadReturnValue;
        socklen_t clientAddressLength = (socklen_t) sizeof(client);
        while ((connectionFileDescriptor = waccept(listenFD, (struct sockaddr *) & client, (socklen_t *) & clientAddressLength)) < 0 && (errno == EINTR));
        // fullRead per leggere l'ID dell'entità connessa con il ServerV.
        if ((fullReadReturnValue = fullRead(connectionFileDescriptor, (void *) & requestIdentifier, sizeof(requestIdentifier))) != 0) raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);

        //--Preleviamo il mutex sul socket file descriptor di connessione.
        if (pthread_mutex_lock(& connectionFileDescriptorMutex) != 0) raiseError(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR);
        // Allochiamo l'area di memoria necessaria per contenere il duplicato del FD connesso
        int * threadConnectionFileDescriptor = (int *) calloc(1, sizeof(* threadConnectionFileDescriptor));
        if (!threadConnectionFileDescriptor) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
        /*
        The dup() system call allocates a new file descriptor that refers
        to the same open file description as the descriptor oldfd. The new
        file descriptor number is guaranteed to be the lowest-numbered
        file descriptor that was unused in the calling process.

        After a successful return, the old and new file descriptors may
        be used interchangeably.  Since the two file descriptors refer to
        the same open file description, they share file offset and file
        status flags; for example, if the file offset is modified by
        using lseek(2) on one of the file descriptors, the offset is also
        changed for the other file descriptor.

        The two file descriptors do not share file descriptor flags (the
        close-on-exec flag).  The close-on-exec flag (FD_CLOEXEC; see
        fcntl(2)) for the duplicate descriptor is off.
         */
        if ((* threadConnectionFileDescriptor = dup(connectionFileDescriptor)) < 0) raiseError(DUP_SCOPE, DUP_ERROR);
        
        // --Rilasciamo il mutex usufruito per il socket file descriptor originale
        if (pthread_mutex_unlock(& connectionFileDescriptorMutex) != 0)  raiseError(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR);
        // Individuiamo la routine apposita da invocare a seconda del mittente della richiesta pervenuta al ServerV
        switch (requestIdentifier) {
            // Gestione servizio per il Centro vaccinale.
            case centroVaccinaleSender:
                //--Sempre con lo stesso TID creiamo un thread apposito per gestire il servizio.
                if ((threadCreationReturnValue = pthread_create(& singleTID, & attr, & centroVaccinaleRequestHandler, threadConnectionFileDescriptor)) != 0) raiseError(PTHREAD_CREATE_SCOPE, PTHREAD_CREATE_ERROR);
                break;
            // Gestione servizio per il ClientS via ServerG.
            case clientS_viaServerG_Sender:
                //--Sempre con lo stesso TID creiamo un thread apposito per gestire il servizio.
                if ((threadCreationReturnValue = pthread_create(& singleTID, & attr, & clientS_viaServerG_RequestHandler, threadConnectionFileDescriptor)) != 0) raiseError(PTHREAD_CREATE_SCOPE, PTHREAD_CREATE_ERROR);
                break;
            // Gestione servizio per il ClientT via ServerG.
            case clientT_viaServerG_Sender:
                //--Sempre con lo stesso TID creiamo un thread apposito per gestire il servizio.
                if ((threadCreationReturnValue = pthread_create(& singleTID, & attr, & clientT_viaServerG_RequestHandler, threadConnectionFileDescriptor)) != 0) raiseError(PTHREAD_CREATE_SCOPE, PTHREAD_CREATE_ERROR);
                break;
            default:
                raiseError(INVALID_SENDER_ID_SCOPE, INVALID_SENDER_ID_ERROR);
                break;
        }
        /*
        Chiudiamo il socket file descriptor relativo a quella connessione nel thread main e il flusso principale del
        ServerV si rimette in attesa di una nuova connessione.
        */
        wclose(connectionFileDescriptor);
    }
    //--Visto che si è allocata memoria dinamica in precedenza deallochiamo tale memoria invocando destory su tutti gli attributi
    if (pthread_attr_destroy(& attr) != 0) raiseError(PTHREAD_MUTEX_ATTR_DESTROY_SCOPE, PTHREAD_MUTEX_ATTR_DESTROY_ERROR);
    wclose(listenFD);
    exit(0);
}

void * centroVaccinaleRequestHandler (void * args) {
    /*
    -- Utilizziamo il mutex per deferenziare in mutua esclusione il valore contenuto nel puntatore della routine assegnata
     al thread e deallocare la memoria dinamica ad esso associata.
    */
    if (pthread_mutex_lock(& connectionFileDescriptorMutex) != 0) threadRaiseError(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR);
    int threadConnectionFileDescriptor = * ((int *) args);
    free(args);
    //-- Rilasciamo il mutex
    if (pthread_mutex_unlock(& connectionFileDescriptorMutex) != 0) threadRaiseError(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR);
    ssize_t fullWriteReturnValue, fullReadReturnValue, getLineBytes;
    size_t effectiveLineLength = 0;
    char * singleLine = NULL, * nowDateString = NULL;
    char dateCopiedFromFile[LUNGHEZZA_DATA];
    struct tm firstTime, secondTime;
    time_t scheduledVaccinationDate, requestVaccinationDate = 0;
    double elapsedMonths;
    enum boolean isVaccineBlocked = FALSE, codiceTesseraSanitariaWasFound = FALSE;
    FILE * originalFilePointer, * tempFilePointer;
    
    //--Allochiamo la memoria per i dati provenienti dal Centro Vaccinale e la risposta del ServerV
    centroVaccinaleRequestToServerV * newCentroVaccinaleRequest = (centroVaccinaleRequestToServerV *) calloc(1, sizeof(* newCentroVaccinaleRequest));
    if (!newCentroVaccinaleRequest) threadAbort(CALLOC_SCOPE, CALLOC_ERROR, threadConnectionFileDescriptor, NULL);
    
    serverV_ReplyToCentroVaccinale * newServerV_Reply = (serverV_ReplyToCentroVaccinale *) calloc(1, sizeof(* newServerV_Reply));
    if (!newServerV_Reply) threadAbort(CALLOC_SCOPE, CALLOC_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest);
    
    // fullRead per attendere la richeisdta del centro Vaccinale
    if ((fullReadReturnValue = fullRead(threadConnectionFileDescriptor, (void *) newCentroVaccinaleRequest, sizeof(* newCentroVaccinaleRequest))) != 0) threadAbort(FULL_READ_SCOPE, (int) fullReadReturnValue, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply);
    // Copia del codice della tessera sanitaria nel pacchetto di risposta.
    strncpy((char *) newServerV_Reply->codiceTesseraSanitaria, (const char *) newCentroVaccinaleRequest->codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    newServerV_Reply->requestResult = FALSE;
    
    //--Utilizziamo il mutex

    if (pthread_mutex_lock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply);
    // Apertura in modalità sola lettura per il file "serverV.dat"
    originalFilePointer = fopen(dataPath, "r");
    // Apertura in modalità sola scrittura per il file "tempServerV.dat"
    tempFilePointer = fopen(tempDataPath, "w");
    if (!originalFilePointer || !tempFilePointer) {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply);
        threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply);
    }
    
    //--Verifichiamo la validità del Green Pass (se risulta scaduto o meno)
    while ((getLineBytes = getline((char ** restrict) & singleLine, (size_t * restrict) & effectiveLineLength, (FILE * restrict) originalFilePointer)) != -1) {
        /*
         --Controlliamo se il numero di tessera sanitaria incontrato nel file corrisponde a quello inviato dal Centro Vaccinale
         e che già è presente nel pacchetto di risposta del ServerV.
         */
        if ((strncmp((const char *) newServerV_Reply->codiceTesseraSanitaria, (const char *) singleLine, LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1)) == 0) {
            //--Se c'è corrispondenza
            codiceTesseraSanitariaWasFound = TRUE;
            //--Copiamo la data all'interno del file in una variabile di tipo stringa
            strncpy((char *) dateCopiedFromFile, (const char *) singleLine + LUNGHEZZA_CODICE_TESSERA_SANITARIA, LUNGHEZZA_DATA - 1);
            dateCopiedFromFile[LUNGHEZZA_DATA - 1] = '\0';
            memset(& firstTime, 0, sizeof(firstTime));
            memset(& secondTime, 0, sizeof(secondTime));
            
            //--Confrontiamo tale data con la data attuale
            firstTime.tm_mday = (int) strtol((const char * restrict) & dateCopiedFromFile[0], (char ** restrict) NULL, 10);
            firstTime.tm_mon = ((int) strtol((const char * restrict) & dateCopiedFromFile[3], (char ** restrict) NULL, 10) - 1);
            firstTime.tm_year = ((int) strtol((const char * restrict) & dateCopiedFromFile[6], (char ** restrict) NULL, 10) - 1900);
            nowDateString = getNowDate();
            secondTime.tm_mday = ((int) strtol((const char * restrict) & nowDateString[0], (char ** restrict) NULL, 10));
            secondTime.tm_mon = ((int) strtol((const char * restrict) & nowDateString[3], (char ** restrict) NULL, 10) - 1);
            secondTime.tm_year = ((int) strtol((const char * restrict) & nowDateString[6], (char ** restrict) NULL, 10) - 1900);
            
            if ((firstTime.tm_mday == 0 || firstTime.tm_mon == 0 || firstTime.tm_year == 0 || secondTime.tm_mday == 0 || secondTime.tm_mon == 0 || secondTime.tm_year == 0) && (errno == EINVAL || errno == ERANGE)) {
                fclose(originalFilePointer);
                fclose(tempFilePointer);
                if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
                threadAbort(STRTOL_SCOPE, STRTOL_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
            }
            
            if (((scheduledVaccinationDate = mktime(& firstTime)) == -1) || ((requestVaccinationDate = mktime(& secondTime)) == -1)) {
                fclose(originalFilePointer);
                fclose(tempFilePointer);
                if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
                threadAbort(MKTIME_SCOPE, MKTIME_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
            }
            
            //--Calcoliamo i mesi di differenza tra le due date
            elapsedMonths = ((((difftime(scheduledVaccinationDate, requestVaccinationDate) / 60) / 60) / 24) / 31);
            /*
             Se il numero di mesi passati è minore del numero di mesi da aspettare per la successiva vaccinazione e
             il numero di mesi di differenza è maggiore di 0 il Vanilla Green Pass non è ancora scaduto.
             */
            if (elapsedMonths <= (MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE + 1) && elapsedMonths > 0) {
                // Copiamo la data dal file "serverV.dat" nella risposta da mandare al "CentroVaccinale"
                strncpy((char *) newServerV_Reply->dataScadenzaGreenPass, (const char *) dateCopiedFromFile, LUNGHEZZA_DATA);
                isVaccineBlocked = TRUE;
            }
            break;
        }
    }
    
    //--Se è possibile fare la vaccinazione
    if (!isVaccineBlocked) {
        //--Copiamo la data che è stata inviata dal Centro Vaccinale al pacchetto di risposta.
        strncpy((char *) newServerV_Reply->dataScadenzaGreenPass, (const char *) newCentroVaccinaleRequest->dataScadenzaGreenPass, LUNGHEZZA_DATA);
        //--Aggiungiamo l'esito della richiesta di vaccinazione
        newServerV_Reply->requestResult = TRUE;
        //--Chiudiamo e riapriamo del file per riinizializzare il file pointer all'inizio del file
        fclose(originalFilePointer);
        originalFilePointer = fopen(dataPath, "r");
        if (!originalFilePointer) {
            fclose(tempFilePointer);
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
            threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
        }
        
        //--Salviamo nel file serverV.dat la nuova data di scadenza della vaccinazione
        while ((getLineBytes = getline(& singleLine, & effectiveLineLength, originalFilePointer)) != -1) {
            if ((strncmp((const char *) newServerV_Reply->codiceTesseraSanitaria, (const char *) singleLine, LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1)) != 0) {
                //--Confontiamo il codice della tessera sanitaria di input del client con quello memorizzato nel file "serverV.dat"
                if (fprintf(tempFilePointer, "%s", singleLine) < 0) {
                    fclose(originalFilePointer);
                    fclose(tempFilePointer);
                    //Copiamo tutto il file "serverV.dat nel file temporaneo "tempServerV.dat"
                    if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
                    threadAbort(FPRINTF_SCOPE, FPRINTF_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
                }
            }
        }
        
        //--Inseriamo nel file temporaneo della tupla: codice, data validità e stato di validità.
        if (fprintf(tempFilePointer, "%s:%s:%s\n", newServerV_Reply->codiceTesseraSanitaria, newServerV_Reply->dataScadenzaGreenPass, "1") < 0) {
            fclose(originalFilePointer);
            fclose(tempFilePointer);
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
            threadAbort(FPRINTF_SCOPE, FPRINTF_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
        }
        
        // Update Files
        fclose(originalFilePointer);
        fclose(tempFilePointer);
        //--Eliminiamo il file originale
        if (remove(dataPath) != 0) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
            threadAbort(REMOVE_SCOPE, REMOVE_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
        }
        
        //--Ridenominiamo il file temporaneo con il nome del file originale
        if (rename(tempDataPath, dataPath) != 0) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
            threadAbort(RENAME_SCOPE, RENAME_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
        }
        
        //--Creiamo un file temporaneo e lo apriamo
        tempFilePointer = fopen(tempDataPath, "w+");
        if (!tempFilePointer) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
            threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
        }
        //Chiudiamo il file temporaneo
        fclose(tempFilePointer);
    } else {
        fclose(originalFilePointer);
        fclose(tempFilePointer);
    }

    //--Inviamo la rispsota se il codice della tessera sanitaria è stato individuato
    if (codiceTesseraSanitariaWasFound) {
        // Rilasciamo il mutex sul file system per l'accesso ai file.
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
        
        // fullWrite per l'invio della risposta al Centro Vaccinale dell'esito positivo o negativo della richiesta effettuata
        if ((fullWriteReturnValue = fullWrite(threadConnectionFileDescriptor, (const void *) newServerV_Reply, sizeof(* newServerV_Reply))) != 0) threadAbort(FULL_WRITE_SCOPE, (int) fullWriteReturnValue, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply, nowDateString, singleLine);
        free(nowDateString);
        free(singleLine);
    } else {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply);
        
        // fullWrite per l'invio della risposta al Centro Vaccinale dell'esito negativo della richiesta effettuata
        if ((fullWriteReturnValue = fullWrite(threadConnectionFileDescriptor, (const void *) newServerV_Reply, sizeof(* newServerV_Reply))) != 0) threadAbort(FULL_WRITE_SCOPE, (int) fullWriteReturnValue, threadConnectionFileDescriptor, newCentroVaccinaleRequest, newServerV_Reply);
    }
    
    wclose(threadConnectionFileDescriptor);
    free(newCentroVaccinaleRequest);
    free(newServerV_Reply);
    pthread_exit(NULL);
}

void * clientS_viaServerG_RequestHandler(void * args) {
    /*
     * -- Utilizziamo il mutex per deferenziare, in mutua esclusione, il valore contenuto nel puntatore della routine assegnata
     al thread.
    */
    if (pthread_mutex_lock(& connectionFileDescriptorMutex) != 0) threadRaiseError(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR);
    int threadConnectionFileDescriptor = * ((int *) args);
    free(args);
    //--Rilasciamo il mutex
    if (pthread_mutex_unlock(& connectionFileDescriptorMutex) != 0) threadRaiseError(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR);
    ssize_t fullWriteReturnValue, fullReadReturnValue, getLineBytes;
    size_t effectiveLineLength = 0;
    char * singleLine = NULL, * nowDateString = NULL;
    struct tm firstTime, secondTime;
    time_t scheduledVaccinationDate = 0, requestVaccinationDate = 0;
    double elapsedMonths;
    unsigned short int greenPassStatus;
    enum boolean isGreenPassExpired = TRUE, codiceTesseraSanitariaWasFound = FALSE, isGreenPassValid = FALSE;
    FILE * originalFilePointer;
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA], dateCopiedFromFile[LUNGHEZZA_DATA], greenPassStatusString[2];

    
    //--Allocchiamo la memoria per il pacchetto risposta del ServerV
    serverV_ReplyToServerG_clientS * newServerV_Reply = (serverV_ReplyToServerG_clientS *) calloc(1, sizeof(* newServerV_Reply));
    if (!newServerV_Reply) threadAbort(CALLOC_SCOPE, CALLOC_ERROR, threadConnectionFileDescriptor, NULL);

    // fullRead per attendere la ricezione del codice della tessera sanitaria
    if ((fullReadReturnValue = fullRead(threadConnectionFileDescriptor, (void *) codiceTesseraSanitaria, (size_t) LUNGHEZZA_CODICE_TESSERA_SANITARIA * sizeof(char))) != 0) threadAbort(FULL_READ_SCOPE, (int) fullReadReturnValue, threadConnectionFileDescriptor, newServerV_Reply);

    //--Copiamo l' HCN nel pacchetto di risposta
    strncpy((char *) newServerV_Reply->codiceTesseraSanitaria, (const char *) codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    newServerV_Reply->requestResult = FALSE;

    //--Utilizziamo il mutex per accedere alla sezione critica in mutua esclusione

    if (pthread_mutex_lock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR, threadConnectionFileDescriptor, newServerV_Reply);
    //--Apriamo il file "serverV.dat" in lettura
    originalFilePointer = fopen(dataPath, "r");
    if (!originalFilePointer) {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerV_Reply);
        threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadConnectionFileDescriptor, newServerV_Reply);
    }

    //--Verifichiamo la validità del Green Pass (se risulta scaduto o meno)
    while ((getLineBytes = getline(& singleLine, & effectiveLineLength, originalFilePointer)) != -1) {
        /*
         --Controlliamo se il numero di tessera sanitaria incontrato nel file corrisponde a quello inviato dal Centro Vaccinale
         e che già è presente nel pacchetto di risposta del ServerV.
         */
        if ((strncmp((const char *) newServerV_Reply->codiceTesseraSanitaria, (const char *) singleLine, LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1)) == 0) {
            //--Se c'è corrispondenza
            codiceTesseraSanitariaWasFound = TRUE;
            //--Copiamo la data all'interno del file in una variabile di tipo stringa
            strncpy((char *) dateCopiedFromFile, (const char *) singleLine + LUNGHEZZA_CODICE_TESSERA_SANITARIA, LUNGHEZZA_DATA - 1);
            dateCopiedFromFile[LUNGHEZZA_DATA - 1] = '\0';
            memset(& firstTime, 0, sizeof(firstTime));
            memset(& secondTime, 0, sizeof(secondTime));

            //--Confrontiamo tale data con la data attuale
            firstTime.tm_mday = (int) strtol((const char * restrict) & dateCopiedFromFile[0], (char ** restrict) NULL, 10);
            firstTime.tm_mon = ((int) strtol((const char * restrict) & dateCopiedFromFile[3], (char ** restrict) NULL, 10) - 1);
            firstTime.tm_year = ((int) strtol((const char * restrict) & dateCopiedFromFile[6], (char ** restrict) NULL, 10) - 1900);
            nowDateString = getNowDate();
            secondTime.tm_mday = ((int) strtol((const char * restrict) & nowDateString[0], (char ** restrict) NULL, 10));
            secondTime.tm_mon = ((int) strtol((const char * restrict) & nowDateString[3], (char ** restrict) NULL, 10) - 1);
            secondTime.tm_year = ((int) strtol((const char * restrict) & nowDateString[6], (char ** restrict) NULL, 10) - 1900);

            if ((firstTime.tm_mday == 0 || firstTime.tm_mon == 0 || firstTime.tm_year == 0 || secondTime.tm_mday == 0 || secondTime.tm_mon == 0 || secondTime.tm_year == 0) && (errno == EINVAL || errno == ERANGE)) {
                fclose(originalFilePointer);
                if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerV_Reply, nowDateString, singleLine);
                threadAbort(STRTOL_SCOPE, STRTOL_ERROR, threadConnectionFileDescriptor, newServerV_Reply, nowDateString, singleLine);
            }

            if (((scheduledVaccinationDate = mktime(& firstTime)) == -1) || ((requestVaccinationDate = mktime(& secondTime)) == -1)) {
                fclose(originalFilePointer);
                if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerV_Reply, nowDateString, singleLine);
                threadAbort(MKTIME_SCOPE, MKTIME_ERROR, threadConnectionFileDescriptor, newServerV_Reply, nowDateString, singleLine);
            }

            //--Calcoliamo i mesi di differenza tra le due date
            elapsedMonths = ((((difftime(scheduledVaccinationDate, requestVaccinationDate) / 60) / 60) / 24) / 31);
            /*
             Se il numero di mesi passati è minore del numero di mesi da aspettare per la successiva vaccinazione e
             il numero di mesi di differenza è maggiore di 0 il Vanilla Green Pass non è ancora scaduto.
             */
            if (elapsedMonths <= (MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE + 1) && elapsedMonths > 0) isGreenPassExpired = FALSE;
            strncpy((char *) greenPassStatusString, (const char *) singleLine + LUNGHEZZA_CODICE_TESSERA_SANITARIA + LUNGHEZZA_DATA, 1);
            greenPassStatusString[1] = '\0';
            //--Convertiamo lo stato del Green Pass da stringa a intero
            greenPassStatus = (unsigned short int) strtoul((const char * restrict) greenPassStatusString, (char ** restrict) NULL, 10);
            
            //--Controlliamo la validità dello stato: se FALSE significa che non è valido, altrimenti è valido.
            if ((greenPassStatus == FALSE) && (errno == EINVAL || errno == ERANGE)) {
                fclose(originalFilePointer);
                if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerV_Reply, nowDateString, singleLine);
                threadAbort(STRTOUL_SCOPE, STRTOUL_ERROR, threadConnectionFileDescriptor, newServerV_Reply, nowDateString, singleLine);
            }
            if (greenPassStatus == TRUE) isGreenPassValid = TRUE;
            break;
        }
    }

    fclose(originalFilePointer);
    //--Se troviamo il numero di tessera sanitaria nel file "ServerV.dat"
    if (codiceTesseraSanitariaWasFound) {
        //--Utilizziamo il mutex per l'accesso al file system.
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerV_Reply, nowDateString, singleLine);

        //Se il Green Pass è valido e non è scaduto, si inserisce nel pacchetto di risposta l'esito dei controlli
        if (isGreenPassValid && !isGreenPassExpired) newServerV_Reply->requestResult = TRUE;
        
        // fullWrite per inviare il pacchetto
        if ((fullWriteReturnValue = fullWrite(threadConnectionFileDescriptor, (const void *) newServerV_Reply, sizeof(* newServerV_Reply))) != 0) threadAbort(FULL_WRITE_SCOPE, (int) fullWriteReturnValue, threadConnectionFileDescriptor, newServerV_Reply, nowDateString, singleLine);
        free(nowDateString);
        free(singleLine);
    } else {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerV_Reply);
        
        if ((fullWriteReturnValue = fullWrite(threadConnectionFileDescriptor, (const void *) newServerV_Reply, sizeof(* newServerV_Reply))) != 0) threadAbort(FULL_WRITE_SCOPE, (int) fullWriteReturnValue, threadConnectionFileDescriptor, newServerV_Reply);
    }

    free(newServerV_Reply);
    wclose(threadConnectionFileDescriptor);
    pthread_exit(NULL);
}

void * clientT_viaServerG_RequestHandler(void * args) {
    /*
     * -- Utilizziamo il mutex per deferenziare, in mutua esclusione, il valore contenuto nel puntatore della routine assegnata
     al thread.
    */
    if (pthread_mutex_lock(& connectionFileDescriptorMutex) != 0) threadRaiseError(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR);
    int threadConnectionFileDescriptor = * ((int *) args);
    free(args);
    // Rilasciamo il mutex
    if (pthread_mutex_unlock(& connectionFileDescriptorMutex) != 0) threadRaiseError(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR);
    ssize_t fullWriteReturnValue, fullReadReturnValue, getLineBytes;
    size_t effectiveLineLength = 0;
    char * singleLine = NULL;;
    enum boolean codiceTesseraSanitariaWasFound = FALSE;
    FILE * originalFilePointer, * tempFilePointer = NULL;
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA], dateCopiedFromFile[LUNGHEZZA_DATA];

    
    //--Allochiamo memoria per i pacchetti: il primo proveniente dal ServerG e il secondo di risposta dal ServerV.
    serverG_RequestToServerV_onBehalfOfClientT * newServerG_Request = (serverG_RequestToServerV_onBehalfOfClientT *) calloc(1, sizeof(* newServerG_Request));
    if (!newServerG_Request) threadAbort(CALLOC_SCOPE, CALLOC_ERROR, threadConnectionFileDescriptor, NULL);

    serverV_ReplyToServerG_clientT * newServerV_Reply = (serverV_ReplyToServerG_clientT *) calloc(1, sizeof(* newServerV_Reply));
    if (!newServerV_Reply) threadAbort(CALLOC_SCOPE, CALLOC_ERROR, threadConnectionFileDescriptor, newServerG_Request);

    // fullRead per attendere la ricezione del pacchetto di richiesta del ServerG
    if ((fullReadReturnValue = fullRead(threadConnectionFileDescriptor, (void *) newServerG_Request, sizeof(* newServerG_Request))) != 0) {
        threadAbort(FULL_READ_SCOPE, (int) fullReadReturnValue, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply);
    }

    // --Copiamo il codice della tessera sanitaria nel pacchetto di risposta
    strncpy((char *) codiceTesseraSanitaria, (const char *) newServerG_Request->codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    strncpy((char *) newServerV_Reply->codiceTesseraSanitaria, (const char *) codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    newServerV_Reply->updateResult = FALSE;

    //--Utilizziamo il mutex per accedere alla sezione critica in mutua esclusione ed effetturare alcune operazioni
    if (pthread_mutex_lock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_LOCK_SCOPE, PTHREAD_MUTEX_LOCK_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply);
    originalFilePointer = fopen(dataPath, "r");
    if (!originalFilePointer) {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerV_Reply);
        threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply);
    }

    //--Leggiamo il file
    while ((getLineBytes = getline(& singleLine, & effectiveLineLength, originalFilePointer)) != -1) {
        //--Controlliamo la coincidenza tra il codice di tessera sanitaria di input e quelli nel file
        if ((strncmp((const char *) newServerV_Reply->codiceTesseraSanitaria, (const char *) singleLine, LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1)) == 0) {
            //--Se c'è
            codiceTesseraSanitariaWasFound = TRUE;
            //--Salviamo la data di scadenza del Vannila Green Pass
            strncpy((char *) dateCopiedFromFile, (const char *) singleLine + LUNGHEZZA_CODICE_TESSERA_SANITARIA, LUNGHEZZA_DATA - 1);
            dateCopiedFromFile[LUNGHEZZA_DATA - 1] = '\0';
            break;
        }
    }

    fclose(originalFilePointer);
    //--Se il codice è stato trovato si può modificare lo stato di validità del Vanilla Green Pass
    if (codiceTesseraSanitariaWasFound) {
        //--Apriamo i due file: l'originale e il temporaneo
        originalFilePointer = fopen(dataPath, "r");
        tempFilePointer = fopen(tempDataPath, "w");
        if (!originalFilePointer || !tempFilePointer) {
            fclose(originalFilePointer);
            fclose(tempFilePointer);
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
            threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
        }

        //--Salviamo nel file serverV.dat il nuovo stato del Vanilla Green Pass + lettura linea per linea del file originale.
        while ((getLineBytes = getline(& singleLine, & effectiveLineLength, originalFilePointer)) != -1) {
            // Se non c'è corrispondenza tra i codici della tessera sanitaria
            if ((strncmp(newServerV_Reply->codiceTesseraSanitaria, singleLine, LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1)) != 0) {
                //--Copiamo la riga del file "serverV.dat" in "tempServerV.dat"
                if (fprintf(tempFilePointer, "%s", singleLine) < 0) {
                    fclose(originalFilePointer);
                    fclose(tempFilePointer);
                    if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
                    threadAbort(FPRINTF_SCOPE, FPRINTF_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
                }
            }
        }

        //--Inseriamo i nuovi valori del Vanilla Green Pass modificato nel file temporaneo
        if (fprintf(tempFilePointer, "%s:%s:%hu\n", newServerV_Reply->codiceTesseraSanitaria, dateCopiedFromFile, newServerG_Request->updateValue) < 0) {
            fclose(originalFilePointer);
            fclose(tempFilePointer);
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
            threadAbort(FPRINTF_SCOPE, FPRINTF_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
        }

        // updateFile
        fclose(originalFilePointer);
        fclose(tempFilePointer);
        //--Cancelliamo il file originale
        if (remove(dataPath) != 0) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
            threadAbort(REMOVE_SCOPE, REMOVE_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
        }

        //--Ridenominiamo il temporaneo con il nome del file originale
        if (rename(tempDataPath, dataPath) != 0) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
            threadAbort(RENAME_SCOPE, RENAME_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
        }

        //--Creaiamo il file temporaneo
        tempFilePointer = fopen(tempDataPath, "w+");
        if (!tempFilePointer) {
            if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
            threadAbort(FOPEN_SCOPE, FOPEN_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply, singleLine);
        }
        //--Chiudiamo il file temporaneo
        fclose(tempFilePointer);
        //--Salviamo l'esito della modifica nel pacchetto di risposta
        newServerV_Reply->updateResult = TRUE;
        free(singleLine);
    }

    // fullWrite per l'invio del pacchetto al ServerG
    if ((fullWriteReturnValue = fullWrite(threadConnectionFileDescriptor, (const void *) newServerV_Reply, sizeof(* newServerV_Reply))) != 0) {
        if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply);
        threadAbort(FULL_WRITE_SCOPE, (int) fullWriteReturnValue, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply);
    }

    //--Rilasciamo il mutex
    if (pthread_mutex_unlock(& fileSystemAccessMutex) != 0) threadAbort(PTHREAD_MUTEX_UNLOCK_SCOPE, PTHREAD_MUTEX_UNLOCK_ERROR, threadConnectionFileDescriptor, newServerG_Request, newServerV_Reply);
    wclose(threadConnectionFileDescriptor);
    free(newServerV_Reply);
    free(newServerG_Request);
    pthread_exit(NULL);
}

/*
 --Questa procedura si occupa di deallocare tutte le risorse passate tramite una lista di puntatori di lunghezza
 variabile.
*/
void threadAbort (char * errorScope, int exitCode, int threadConnectionFileDescriptor, void * arg1, ...) {
    /*
    --Chiudiamo il socket file descriptor di connessione usato dal thread di turno per comunicare col
    ServerG o con il CentroVaccinale
    */
    wclose(threadConnectionFileDescriptor);
    va_list argumentsList;
    void * currentElement;
    if (arg1 != NULL) {
        free(arg1);
        va_start(argumentsList, arg1);
        // Deallochiamo tutta la memoria dinamica sottoforma di lista di puntatori a void
        while ((currentElement = va_arg(argumentsList, void *)) != 0) free(currentElement);
        va_end(argumentsList);
    }
    threadRaiseError(errorScope, exitCode);
}
