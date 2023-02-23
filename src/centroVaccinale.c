#include "centroVaccinale.h"

int main (int argc, char * argv[]) {
    int serverV_SFD,
    listenFileDescriptor,
    connectionFileDescriptor,
    enable = TRUE;
    struct sockaddr_in client, centroVaccinaleIndirizzo;
    unsigned short int centroVaccinalePorta;
    pid_t childPid;
    //--Verifichiamo che il centroVaccinale sia stato avviato con i parametri corretti
    checkUsage(argc, (const char **) argv, NUMERO_PARAMETRI_CENTRO_VACCINALE, messaggioAtteso);
    //--Ricaviamo il numero di porta a partire dal valore passato come parametro da terminale all'avvio del CentroVaccinale
    centroVaccinalePorta = (unsigned short int) strtoul((const char * restrict) argv[1], (char ** restrict) NULL, 10);
    if (centroVaccinalePorta == 0 && (errno == EINVAL || errno == ERANGE)) raiseError(STRTOUL_SCOPE, STRTOUL_ERROR);
    
    //--Impostiamo la comunicazione col clientCitizen
    listenFileDescriptor = wsocket(AF_INET, SOCK_STREAM, 0);
    //--Durante l'applicazione del meccanismo di IPC via socket impostiamo l'opzione di riutilizzo degli indirizzi
    if (setsockopt(listenFileDescriptor, SOL_SOCKET, SO_REUSEADDR, & enable, (socklen_t) sizeof(enable))  == -1) raiseError(SET_SOCK_OPT_SCOPE, SET_SOCK_OPT_ERROR);
    memset((void *) & centroVaccinaleIndirizzo, 0, sizeof(centroVaccinaleIndirizzo));
    memset((void *) & client, 0, sizeof(client));
    
    centroVaccinaleIndirizzo.sin_family      = AF_INET;
    centroVaccinaleIndirizzo.sin_addr.s_addr = htonl(INADDR_ANY);
    centroVaccinaleIndirizzo.sin_port        = htons(centroVaccinalePorta);
    wbind(listenFileDescriptor, (struct sockaddr *) & centroVaccinaleIndirizzo, (socklen_t) sizeof(centroVaccinaleIndirizzo));
    wlisten(listenFileDescriptor, LISTEN_QUEUE_SIZE);
    signal(SIGCHLD, SIG_IGN);
    
    while (TRUE) {
        socklen_t clientAddressLength = (socklen_t) sizeof(client);
        while ((connectionFileDescriptor = waccept(listenFileDescriptor, (struct sockaddr *) & client, (socklen_t *) & clientAddressLength)) < 0 && (errno == EINTR));
        if ((childPid = fork()) == -1) {
            raiseError(FORK_SCOPE, FORK_ERROR);
        } else if (childPid == 0) {
            // Processo figlio che chiude il FD realtivo "all'ascolto" delle nuove connessioni in arrivo per il CentroVaccinale
            wclose(listenFileDescriptor);
            //--Realizziamo un collegamento con il ServerV all'interno del processo figlio generato
            serverV_SFD = createConnectionWithServerV(percorsoFileConfigurazioneCentroVaccinale);
            //--Invochiamo la routine per la gestione della richiesta del ClientCitizen collegatosi.
            clientCitizenRequestHandler(connectionFileDescriptor, serverV_SFD);
            exit(0);
        }
        // Processo padre che chiude il socket file descriptor che realizza la connessione con il ClientCitizen collegatosi.
        wclose(connectionFileDescriptor);
    }
    
    // Codice mai eseguito
    wclose(listenFileDescriptor);
    exit(0);
}

void clientCitizenRequestHandler (int connectionFileDescriptor, int serverV_SFD) {
    char * vaccineExpirationDate;
    //--Allochiamo dinamica la memoria necessaria per il pacchetto da inviare al ClientCitizen e quello da inviare al ServerV.
    centroVaccinaleReplyToClientCitizen * rispostaCentroVaccinale = (centroVaccinaleReplyToClientCitizen *) calloc(1, sizeof(centroVaccinaleReplyToClientCitizen));
    centroVaccinaleRequestToServerV * newCentroVaccinaleRequest = (centroVaccinaleRequestToServerV *) calloc(1, sizeof(centroVaccinaleRequestToServerV));
    serverV_ReplyToCentroVaccinale * newServerV_Reply = (serverV_ReplyToCentroVaccinale *) calloc(1, sizeof(serverV_ReplyToCentroVaccinale));
    if (!rispostaCentroVaccinale) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    if (!newCentroVaccinaleRequest) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    if (!newServerV_Reply) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    
    char buffer[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    ssize_t fullWriteReturnValue, fullReadReturnValue;
    unsigned short int centroVaccinaleSenderID = centroVaccinaleSender;
    
    // fullRead per la lettura del codice della Tessera Sanitaria
    if ((fullReadReturnValue = fullRead(connectionFileDescriptor, (void *) buffer, (size_t) LUNGHEZZA_CODICE_TESSERA_SANITARIA * sizeof(char))) != 0) raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);
    // Copiamo il codice della tessera sanitaria nel primo campoo del pacchetto di invio al ServerV.
    strncpy((char *) newCentroVaccinaleRequest->codiceTesseraSanitaria, (const char *)  buffer, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    // Calcoliamo il periodo di validità del Vanilla Green Pass
    vaccineExpirationDate = getVaccineExpirationDate();
    // Copiamo il valore di tale periodo nel pacchetto di invio al ServerV.
    strncpy((char *) newCentroVaccinaleRequest->dataScadenzaGreenPass, (const char *) vaccineExpirationDate, LUNGHEZZA_DATA);
    
    // fullWrite per scrivere e inviare il pacchetto al ServerV
    if ((fullWriteReturnValue = fullWrite(serverV_SFD, (const void *) & centroVaccinaleSenderID, sizeof(centroVaccinaleSenderID))) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    if ((fullWriteReturnValue = fullWrite(serverV_SFD, (const void *) newCentroVaccinaleRequest, sizeof(* newCentroVaccinaleRequest))) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    // fullRead per la rettura e la ricezione del pacchetto di risposta dal ServerV
    if ((fullReadReturnValue = fullRead(serverV_SFD, (void *) newServerV_Reply, sizeof(* newServerV_Reply))) != 0) raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);
    
    // Copiamo i valori di risposta del ServerV nel pacchetto di risposta da parte del centroVaccinale al ClientCitizen
    strncpy((char *) rispostaCentroVaccinale->codiceTesseraSanitaria, (const char *) newServerV_Reply->codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    strncpy((char *) rispostaCentroVaccinale->dataScadenzaGreenPass, (const char *) newServerV_Reply->dataScadenzaGreenPass, LUNGHEZZA_DATA);
    rispostaCentroVaccinale->requestResult = newServerV_Reply->requestResult == TRUE ? TRUE : FALSE;
    
    // Inviamo il pacchetto al clientCitizen tramite FullWrite sul descrittore connesso
    if ((fullWriteReturnValue = fullWrite(connectionFileDescriptor, (const void *) rispostaCentroVaccinale, (size_t) sizeof(* rispostaCentroVaccinale))) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    
    free(rispostaCentroVaccinale);
    free(newCentroVaccinaleRequest);
    free(newServerV_Reply);
    free(vaccineExpirationDate);
    wclose(connectionFileDescriptor);
    wclose(serverV_SFD);
}
