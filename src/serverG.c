#include "serverG.h"

int main (int argc, char * argv[]) {
    int serverV_SFD, listenFD, connectionFD, enable = TRUE;
    struct sockaddr_in client, serverGIndrizzo;
    unsigned short int serverGPorta, identificatoreRichiesta;
    pid_t childPid;
    
    //--Verifichiamo che il ServerG sia stato avviato con i parametri che si aspetta di avere
    checkUtilizzo(argc, (const char **) argv, SERVER_G_ARGS_NO, messaggioAtteso);
    //--Ricaviamo il numero di porta a partire dal valore passato come argomento via terminale all'avvio del ServerG
    serverGPorta = (unsigned short int) strtoul((const char * restrict) argv[1], (char ** restrict) NULL, 10);
    //--Richiamiamo un errore se questo valore non dovesse essere valido
    if (serverGPorta == 0 && (errno == EINVAL || errno == ERANGE))
        raiseError(STRTOUL_SCOPE, STRTOUL_ERROR);
    
    //--Impostiamo la comunicazione col clientS e clientT
    listenFD = wsocket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, & enable, (socklen_t) sizeof(enable)) == -1) raiseError(SET_SOCK_OPT_SCOPE, SET_SOCK_OPT_ERROR);
    //--Impostiamo a 0 i byte di serverGIndrizzo e client
    memset((void *) & serverGIndrizzo, 0, sizeof(serverGIndrizzo));
    memset((void *) & client, 0, sizeof(client));
    //..Inizializziamo i campi della struct sockaddr_in
    serverGIndrizzo.sin_family      = AF_INET;//utilizziamo protoccolo iPv4
    serverGIndrizzo.sin_addr.s_addr = htonl(INADDR_ANY);//accetta le richieste da qualsiasi indirizzo
    serverGIndrizzo.sin_port        = htons(serverGPorta);//inizializzazione della porta
    //Impostazione dei campi (comprese le porte) del serverGIndrizzo
    wbind(listenFD, (struct sockaddr *) & serverGIndrizzo, (socklen_t) sizeof(serverGIndrizzo));
    //--Mettiamo il serverG in ascolto sulla socket creata
    wlisten(listenFD, LISTEN_QUEUE_SIZE * LISTEN_QUEUE_SIZE);
    signal(SIGCHLD, SIG_IGN);
    
    while (TRUE) {
        ssize_t fullReadReturnValue;
        socklen_t lunghezzaIndirizzoClient = (socklen_t) sizeof(client);
        while ((connectionFD = waccept(listenFD, (struct sockaddr *) & client, (socklen_t *) & lunghezzaIndirizzoClient)) < 0 && (errno == EINTR));
        //--Attendiamo tramite fullRead l'identificativo del mittente col quale si è messo in collegamento.
        if ((fullReadReturnValue = fullRead(connectionFD, (void *) & identificatoreRichiesta, sizeof(identificatoreRichiesta))) != 0) raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);
        
        if ((childPid = fork()) == -1) {
            raiseError(FORK_SCOPE, FORK_ERROR);
        } else if (childPid == 0) {
            // Processo figlio che chiude il FD realtivo "all'ascolto" delle nuove connessioni in arrivo per il ServerG
            wclose(listenFD);
            // Richiesta di instaurare una connessione con il ServerV
            serverV_SFD = creaConnessioneConServerV(percorsoFileConfigurazioneServerG);
            
            //--Controlliamo l'ID del mittente (ClientS o ClientT).
            switch (identificatoreRichiesta) {
                // ClientS
                case clientS_viaServerG_Sender:
                    clientS_RequestHandler(connectionFD, serverV_SFD);
                    break;
                // ClientT
                case clientT_viaServerG_Sender:
                    clientT_RequestHandler(connectionFD, serverV_SFD);
                    break;
                // ID sconosciuto
                default:
                    raiseError(INVALID_SENDER_ID_SCOPE, INVALID_SENDER_ID_ERROR);
                    break;
            }
            
            wclose(connectionFD);
            wclose(serverV_SFD);
            exit(0);
        }
        // Padre che chiude il socket file descriptor che realizza la connessione con il clientUtente collegatosi.
        wclose(connectionFD);
    }
}

//--Questa procedura si occupa della gestione del servizio con un ClientS
void clientS_RequestHandler (int connectionFD, int serverV_SFD) {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    ssize_t fullWriteReturnValue, fullReadReturnValue;
    unsigned short int clientS_viaServerG_SenderID = clientS_viaServerG_Sender;
    
    //--Allochiamo dinamicamente la memoria necessaria per il pacchetto da inviare al ClientS e quello da ricevere dal ServerV.
    serverGRispondeAClientS * nuovaRispostaServerG = (serverGRispondeAClientS *) calloc(1, sizeof(* nuovaRispostaServerG));
    serverVRispondeAServerGVersoClientS * nuovaRispostaServerV = (serverVRispondeAServerGVersoClientS *) calloc(1, sizeof(* nuovaRispostaServerV));
    if (!nuovaRispostaServerG) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    if (!nuovaRispostaServerV) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    
    // fullRead per leggere il codice della tessera sanitaria.
    if ((fullReadReturnValue = fullRead(connectionFD, (void *) codiceTesseraSanitaria, sizeof(char) * LUNGHEZZA_CODICE_TESSERA_SANITARIA)) != 0) raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);
    // fullWrite per scrivere e inviare l'ID del Client e il suo codice di tessera sanitaria nel pacchetto da inviare al ServerV.
    if ((fullWriteReturnValue = fullWrite(serverV_SFD, (const void *) & clientS_viaServerG_SenderID, sizeof(clientS_viaServerG_SenderID))) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    if ((fullWriteReturnValue = fullWrite(serverV_SFD, (const void *) codiceTesseraSanitaria, sizeof(char) * LUNGHEZZA_CODICE_TESSERA_SANITARIA)) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    // fullRead per attendere in lettura il pacchetto di risposta da parte del ServerV.
    if ((fullReadReturnValue = fullRead(serverV_SFD, (void *) nuovaRispostaServerV, sizeof(* nuovaRispostaServerV))) != 0) raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);
    
    
    //--Copiamo i parametri del pacchetto di risposta del ServerV nel pacchetto da inviare al ClientS
    strncpy((char *) nuovaRispostaServerG->codiceTesseraSanitaria, (const char *) nuovaRispostaServerV->codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    nuovaRispostaServerG->requestResult = nuovaRispostaServerV->requestResult;
    // fullWrite per inviare il pacchetto al ClientS.
    if ((fullWriteReturnValue = fullWrite(connectionFD, (const void *) nuovaRispostaServerG, sizeof(* nuovaRispostaServerG))) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    free(nuovaRispostaServerV);
    free(nuovaRispostaServerG);
}

void clientT_RequestHandler (int connectionFD, int serverV_SFD) {
    unsigned short int clientT_viaServerG_SenderID = clientT_viaServerG_Sender;
    ssize_t fullWriteReturnValue, fullReadReturnValue;
    
    /*
     Allochiamo dinamicamente la memoria  necessaria per il pacchetto da ricevere dal ClientT, quello da inviare al ClientT,
     quello da inviare al ServerV e quello da ricevere dal ServerV.
    */
    clientTRichiedeAServerG * nuovaRichiestaClientT = (clientTRichiedeAServerG *) calloc(1, sizeof(* nuovaRichiestaClientT));
    serverGRispondeAClientT * nuovaRispostaServerG = (serverGRispondeAClientT *) calloc(1, sizeof(* nuovaRispostaServerG));
    serverGRichiedeAServerV_PerContoDiClientT * newServerG_Request = (serverGRichiedeAServerV_PerContoDiClientT *) calloc(1, sizeof(* newServerG_Request));
    serverVRispondeAServerGVersoClientT * nuovaRispostaServerV = (serverVRispondeAServerGVersoClientT *) calloc(1, sizeof(* nuovaRispostaServerV));
    if (!nuovaRispostaServerG) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    if (!nuovaRispostaServerV) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    if (!nuovaRichiestaClientT) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    if (!newServerG_Request) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    
    // fullRead per leggere la richiesta del ClientT
    if ((fullReadReturnValue = fullRead(connectionFD, (void *) nuovaRichiestaClientT, (size_t) sizeof(* nuovaRichiestaClientT))) != 0) raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);
    strncpy((char *) newServerG_Request->codiceTesseraSanitaria, (const char *) nuovaRichiestaClientT->codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    newServerG_Request->updateValue = nuovaRichiestaClientT->updateValue;
    // fullWrite per scrivere e inviare al ServerV ID e richiesta al ServerV
    if ((fullWriteReturnValue = fullWrite(serverV_SFD, (const void *) & clientT_viaServerG_SenderID, sizeof(clientT_viaServerG_SenderID))) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    if ((fullWriteReturnValue = fullWrite(serverV_SFD, (const void *) newServerG_Request, sizeof(* newServerG_Request))) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    // fullRead per attendere e leggere la risposta dal ServerV
    if ((fullReadReturnValue = fullRead(serverV_SFD, (void *) nuovaRispostaServerV, sizeof(* nuovaRispostaServerV))) != 0) raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);
    
    strncpy((char *) nuovaRispostaServerG->codiceTesseraSanitaria, (const char *) nuovaRispostaServerV->codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    nuovaRispostaServerG->updateResult = nuovaRispostaServerV->updateResult;
    // fullWrite per scrivere e invoare la risposta precedente del ServerV al ClientT
    if ((fullWriteReturnValue = fullWrite(connectionFD, (const void *) nuovaRispostaServerG, sizeof(* nuovaRispostaServerG))) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    free(nuovaRichiestaClientT);
    free(nuovaRispostaServerG);
    free(newServerG_Request);
    free(nuovaRispostaServerV);
}
