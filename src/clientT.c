#include "clientT.h"

int main (int argc, char * argv[]) {
    char * codiceTesseraSanitaria;
    
    //--Eseguiamo un setup preliminare per la connessione da parte del ClientT con il ServerG.
    int newGreenPassStatus, serverG_SFD = setupClientT(argc, argv, & codiceTesseraSanitaria, & newGreenPassStatus);
    codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1] = '\0';
    
    /*
    --Richiamiamo la funzione che permette di mandare la richiesta di riattivazione o l'invalidazione del Vanilla
    Green Pass di uno specifico cittadino a partire da un codice di tessera sanitaria fornito.
    */
    updateGreenPass(serverG_SFD, (const void *) codiceTesseraSanitaria, (const unsigned short int) newGreenPassStatus);
    wclose(serverG_SFD);
    free(codiceTesseraSanitaria);
    exit(0);
}

int setupClientT (int argc, char * argv[], char ** codiceTesseraSanitaria, int * newGreenPassStatus) {
    struct sockaddr_in serverGIndrizzo;
    char * stringServerG_IP = NULL;
    unsigned short int serverGPorta;
    int serverG_SFD;
    
    //--Verifichiamo che il ClientT sia stato avviato con i parametri che si aspetta di avere.
    checkUtilizzo(argc, (const char **) argv, CLIENT_T_ARGS_NO, messaggioAtteso);
    //--Verifichiamo che il codice di tessera sanitaria immesso sia del formato e della lunghezza giusta.
    checkCodiceTesseraSanitaria(argv[1]);
    * newGreenPassStatus = (unsigned short int) strtoul((const char * restrict) argv[2], (char ** restrict) NULL, 10);
    if (* newGreenPassStatus == 0 && (errno == EINVAL || errno == ERANGE)) lanciaErrore(STRTOUL_SCOPE, STRTOUL_ERROR);
    if (* newGreenPassStatus != TRUE && * newGreenPassStatus != FALSE) lanciaErrore(INVALID_UPDATE_STATUS_SCOPE, INVALID_UPDATE_STATUS_ERROR);
    
    
    //-Allochiamo la memoria necessaria per la tessera sanitaria
    * codiceTesseraSanitaria = (char *) calloc(LUNGHEZZA_CODICE_TESSERA_SANITARIA, sizeof(char));
    if (! * codiceTesseraSanitaria) lanciaErrore(CALLOC_SCOPE, CALLOC_ERROR);
    strncpy(* codiceTesseraSanitaria, (const char *) argv[1], LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1);
    ritornaDatiDiConfigurazione(percorsoFileConfigurazione, & stringServerG_IP, & serverGPorta);
    
    serverG_SFD = wsocket(AF_INET, SOCK_STREAM, 0);
    memset((void *) & serverGIndrizzo, 0, sizeof(serverGIndrizzo));
    serverGIndrizzo.sin_family = AF_INET;
    serverGIndrizzo.sin_port   = htons(serverGPorta);
    if (inet_pton(AF_INET, (const char * restrict) stringServerG_IP, (void *) & serverGIndrizzo.sin_addr) <= 0) lanciaErrore(INET_PTON_SCOPE, INET_PTON_ERROR);
    
    wconnect(serverG_SFD, (struct sockaddr *) & serverGIndrizzo, (socklen_t) sizeof(serverGIndrizzo));
    if (fprintf(stdout, "\nAggiornamento Validita' GreenPass\nNumero tessera sanitaria: %s\n\n... A breve verra' inviato il nuovo stato di validita' del Green Pass...\n", * codiceTesseraSanitaria) < 0) lanciaErrore(FPRINTF_SCOPE, FPRINTF_ERROR);
    free(stringServerG_IP);
    return serverG_SFD;
}

void updateGreenPass (int serverG_SFD, const void * codiceTesseraSanitaria, const unsigned short int newGreenPassStatus) {
    ssize_t fullWriteReturnValue, fullReadReturnValue;
    unsigned short int clientT_SenderID = clientT_viaServerG_Sender;
    //--Allochiamo la memoria per il pacchetto di risposta del ServerG.
    serverG_ReplyToClientT * newServerG_Reply = (serverG_ReplyToClientT *) calloc(1, sizeof(* newServerG_Reply));
    //--Allochiamo la memoria per il pacchetto di richiesta da parte del ClientT al ServerG.
    clientT_RequestToServerG * newClientT_Request = (clientT_RequestToServerG *) calloc(1, sizeof(* newClientT_Request));
    if (!newServerG_Reply) lanciaErrore(CALLOC_SCOPE, CALLOC_ERROR);
    if (!newClientT_Request) lanciaErrore(CALLOC_SCOPE, CALLOC_ERROR);
    
    
    //--Copiamo il codice della tessera sanitaria nel pacchetto di richiesta.
    strncpy(newClientT_Request->codiceTesseraSanitaria, codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    
    
    /*
    --Associamo il valore dell'aggiornamento al secondo parametro del pacchetto di richiesta per
    decidere se convalidare o invalidare il Vanilla Green Pass
    */
    newClientT_Request->updateValue = newGreenPassStatus;
    
    if (fprintf(stdout, "\n... Aggiornamento in corso ...\n") < 0) lanciaErrore(FPRINTF_SCOPE, FPRINTF_ERROR);
    // fullWrite per inviare "ClientT_SenderID"
    if ((fullWriteReturnValue = fullWrite(serverG_SFD, (const void *) & clientT_SenderID, sizeof(clientT_SenderID))) != 0) lanciaErrore(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    // fullWrite per inviare il pacchetto "newClientT_Request"
    if ((fullWriteReturnValue = fullWrite(serverG_SFD, (const void *) newClientT_Request, sizeof(* newClientT_Request))) != 0) lanciaErrore(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    // fullRead per aspettare e successivamente leggere la risposta da parte del ServerG.
    if ((fullReadReturnValue = fullRead(serverG_SFD, (void *) newServerG_Reply, sizeof(* newServerG_Reply))) != 0) lanciaErrore(FULL_READ_SCOPE, (int) fullReadReturnValue);
    
    
    /*
    --Analizziamo il valore relativo all'esito dell'aggiornamento: se risulta essere FALSE, allora significa che l'aggiornamento
     non è andato a buon fine, altrimenti  significa che lo stato di validità del Vanilla Green Pass è stato aggiornato correttamente.
    */
    if (newServerG_Reply->updateResult == FALSE) {
        if (fprintf(stdout, "\nL'aggiornamento del Green Pass associato alla tessera sanitaria %s, non e' andato a buon fine.\nArrivederci.\n", newServerG_Reply->codiceTesseraSanitaria) < 0) lanciaErrore(FPRINTF_SCOPE, FPRINTF_ERROR);
    } else {
        if (fprintf(stdout, "\nL'aggiornamento del Green Pass associato alla tessera sanitaria %s, e' andato a buon fine.\nArrivederci.\n", newServerG_Reply->codiceTesseraSanitaria) < 0) lanciaErrore(FPRINTF_SCOPE, FPRINTF_ERROR);
    }
    free(newServerG_Reply);
    free(newClientT_Request);
}
