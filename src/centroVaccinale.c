#include "centroVaccinale.h"

int main (int argc, char * argv[]) {
    int serverV_SFD,
    listenFD,
    connectionFD,
    enable = TRUE;
    struct sockaddr_in client, centroVaccinaleIndirizzo;
    unsigned short int centroVaccinalePorta;
    pid_t childPid;
    //--Verifichiamo che il centroVaccinale sia stato avviato con i parametri corretti
    checkUtilizzo(argc, (const char **) argv, NUMERO_PARAMETRI_CENTRO_VACCINALE, messaggioAtteso);
    //--Ricaviamo il numero di porta a partire dal valore passato come parametro da terminale all'avvio del CentroVaccinale
    centroVaccinalePorta = (unsigned short int) strtoul((const char * restrict) argv[1], (char ** restrict) NULL, 10);
    if (centroVaccinalePorta == 0 && (errno == EINVAL || errno == ERANGE)) lanciaErrore(STRTOUL_SCOPE, STRTOUL_ERROR);
    
    //--Impostiamo la comunicazione col clientUtente
    listenFD = wsocket(AF_INET, SOCK_STREAM, 0);
    //--Durante l'applicazione del meccanismo di IPC via socket impostiamo l'opzione di riutilizzo degli indirizzi
    if (setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, & enable, (socklen_t) sizeof(enable))  == -1) lanciaErrore(SET_SOCK_OPT_SCOPE, SET_SOCK_OPT_ERROR);
    memset((void *) & centroVaccinaleIndirizzo, 0, sizeof(centroVaccinaleIndirizzo));
    memset((void *) & client, 0, sizeof(client));
    
    centroVaccinaleIndirizzo.sin_family      = AF_INET;
    centroVaccinaleIndirizzo.sin_addr.s_addr = htonl(INADDR_ANY);
    centroVaccinaleIndirizzo.sin_port        = htons(centroVaccinalePorta);
    wbind(listenFD, (struct sockaddr *) & centroVaccinaleIndirizzo, (socklen_t) sizeof(centroVaccinaleIndirizzo));
    wlisten(listenFD, LISTEN_QUEUE_SIZE);
    signal(SIGCHLD, SIG_IGN);
    
    while (TRUE) {
        socklen_t lunghezzaIndirizzoClient = (socklen_t) sizeof(client);
        while ((connectionFD = waccept(listenFD, (struct sockaddr *) & client, (socklen_t *) & lunghezzaIndirizzoClient)) < 0 && (errno == EINTR));
        if ((childPid = fork()) == -1) {
            lanciaErrore(FORK_SCOPE, FORK_ERROR);
        } else if (childPid == 0) {
            //Processo figlio che chiude il FD realtivo "all'ascolto" delle nuove connessioni in arrivo per il CentroVaccinale
            wclose(listenFD);
            //--Realizziamo un collegamento con il ServerV all'interno del processo figlio generato
            serverV_SFD = creaConnessioneConServerV(percorsoFileConfigurazioneCentroVaccinale);
            //--Invochiamo la routine per la gestione della richiesta del clientUtente collegatosi.
            gestoreRichiesteClientUtente(connectionFD, serverV_SFD);
            exit(0);
        }
        // Processo padre che chiude il socket file descriptor che realizza la connessione con il clientUtente collegatosi.
        wclose(connectionFD);
    }
}

void gestoreRichiesteClientUtente (int connectionFD, int serverV_SFD) {
    char * dataScadenzaVaccino;
    //--Allochiamo dinamica la memoria necessaria per il pacchetto da inviare al clientUtente e quello da inviare al ServerV.
    centroVaccinaleRispondeAClientUtente * rispostaCentroVaccinale = (centroVaccinaleRispondeAClientUtente *) calloc(1, sizeof(centroVaccinaleRispondeAClientUtente));
    centroVaccinaleRichiedeAServerV * nuovaRichiestaCentroVaccinale = (centroVaccinaleRichiedeAServerV *) calloc(1, sizeof(centroVaccinaleRichiedeAServerV));
    serverVRispondeACentroVaccinale * nuovaRispostaServerV = (serverVRispondeACentroVaccinale *) calloc(1, sizeof(serverVRispondeACentroVaccinale));
    if (!rispostaCentroVaccinale) lanciaErrore(CALLOC_SCOPE, CALLOC_ERROR);
    if (!nuovaRichiestaCentroVaccinale) lanciaErrore(CALLOC_SCOPE, CALLOC_ERROR);
    if (!nuovaRispostaServerV) lanciaErrore(CALLOC_SCOPE, CALLOC_ERROR);
    
    char buffer[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    ssize_t fullWriteReturnValue, fullReadReturnValue;
    unsigned short int centroVaccinaleSenderID = centroVaccinaleSender;
    
    // fullRead per la lettura del codice della Tessera Sanitaria
    if ((fullReadReturnValue = fullRead(connectionFD, (void *) buffer, (size_t) LUNGHEZZA_CODICE_TESSERA_SANITARIA * sizeof(char))) != 0) lanciaErrore(FULL_READ_SCOPE, (int) fullReadReturnValue);
    //--Copiamo il codice della tessera sanitaria nel primo campoo del pacchetto di invio al ServerV.
    strncpy((char *) nuovaRichiestaCentroVaccinale->codiceTesseraSanitaria, (const char *)  buffer, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    //--Calcoliamo il periodo di validità del Vanilla Green Pass
    dataScadenzaVaccino = getdataScadenzaVaccino();
    //--Copiamo il valore di tale periodo nel pacchetto di invio al ServerV.
    strncpy((char *) nuovaRichiestaCentroVaccinale->dataScadenzaGreenPass, (const char *) dataScadenzaVaccino, LUNGHEZZA_DATA);
    
    // fullWrite per scrivere e inviare il pacchetto al ServerV
    if ((fullWriteReturnValue = fullWrite(serverV_SFD, (const void *) & centroVaccinaleSenderID, sizeof(centroVaccinaleSenderID))) != 0) lanciaErrore(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    if ((fullWriteReturnValue = fullWrite(serverV_SFD, (const void *) nuovaRichiestaCentroVaccinale, sizeof(* nuovaRichiestaCentroVaccinale))) != 0) lanciaErrore(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    // fullRead per la rettura e la ricezione del pacchetto di risposta dal ServerV
    if ((fullReadReturnValue = fullRead(serverV_SFD, (void *) nuovaRispostaServerV, sizeof(* nuovaRispostaServerV))) != 0) lanciaErrore(FULL_READ_SCOPE, (int) fullReadReturnValue);
    
    //--Copiamo i valori di risposta del ServerV nel pacchetto di risposta da parte del centroVaccinale al clientUtente
    strncpy((char *) rispostaCentroVaccinale->codiceTesseraSanitaria, (const char *) nuovaRispostaServerV->codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    strncpy((char *) rispostaCentroVaccinale->dataScadenzaGreenPass, (const char *) nuovaRispostaServerV->dataScadenzaGreenPass, LUNGHEZZA_DATA);
    rispostaCentroVaccinale->requestResult = nuovaRispostaServerV->requestResult == TRUE ? TRUE : FALSE;
    
    //--Inviamo il pacchetto al clientUtente tramite FullWrite sul descrittore connesso
    if ((fullWriteReturnValue = fullWrite(connectionFD, (const void *) rispostaCentroVaccinale, (size_t) sizeof(* rispostaCentroVaccinale))) != 0) lanciaErrore(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    
    free(rispostaCentroVaccinale);
    free(nuovaRichiestaCentroVaccinale);
    free(nuovaRispostaServerV);
    free(dataScadenzaVaccino);
    wclose(connectionFD);
    wclose(serverV_SFD);
}
