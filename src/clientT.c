#include "clientT.h"

int main (int argc, char * argv[]) {
    char * codiceTesseraSanitaria;
    
    //--Eseguiamo un setup preliminare per la connessione da parte del ClientT con il ServerG.
    int nuovoStatoGreenPass, serverG_SFD = setupClientT(argc, argv, & codiceTesseraSanitaria, & nuovoStatoGreenPass);
    codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1] = '\0';
    
    /*--Richiamiamo la funzione che permette di mandare la richiesta di riattivazione o l'invalidazione del
     GreenPass di uno specifico cittadino a partire da un codice di tessera sanitaria fornito.*/
    updateGreenPass(serverG_SFD, (const void *) codiceTesseraSanitaria, (const unsigned short int) nuovoStatoGreenPass);
    wclose(serverG_SFD);
    free(codiceTesseraSanitaria);
    exit(0);
}

int setupClientT (int argc, char * argv[], char ** codiceTesseraSanitaria, int * nuovoStatoGreenPass) {
    struct sockaddr_in serverGIndrizzo;
    char * stringServerG_IP = NULL;
    unsigned short int serverGPorta;
    int serverG_SFD;
    
    //--Verifichiamo che il ClientT sia stato avviato con i parametri che si aspetta di avere
    checkUtilizzo(argc, (const char **) argv, CLIENT_T_ARGS_NO, messaggioAtteso);
    //--Verifichiamo che il codice di tessera sanitaria immesso sia del formato e della lunghezza giusta
    checkCodiceTesseraSanitaria(argv[1]);
    * nuovoStatoGreenPass = (unsigned short int) strtoul((const char * restrict) argv[2], (char ** restrict) NULL, 10);
    if (* nuovoStatoGreenPass == 0 && (errno == EINVAL || errno == ERANGE))
        raiseError(STRTOUL_SCOPE, STRTOUL_ERROR);
    if (* nuovoStatoGreenPass != TRUE && * nuovoStatoGreenPass != FALSE)
        raiseError(INVALID_UPDATE_STATUS_SCOPE, INVALID_UPDATE_STATUS_ERROR);
    
    
    //--Allochiamo la memoria necessaria per la tessera sanitaria
    * codiceTesseraSanitaria = (char *) calloc(LUNGHEZZA_CODICE_TESSERA_SANITARIA, sizeof(char));
    if (! * codiceTesseraSanitaria)
        raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    strncpy(* codiceTesseraSanitaria, (const char *) argv[1], LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1);
    getDatiDiConfigurazione(percorsoFileConfigurazione, & stringServerG_IP, & serverGPorta);
    //--Creiamo il socket per la comunicazione con ServerG
    serverG_SFD = wsocket(AF_INET, SOCK_STREAM, 0);
    memset((void *) & serverGIndrizzo, 0, sizeof(serverGIndrizzo));
    serverGIndrizzo.sin_family = AF_INET;
    serverGIndrizzo.sin_port   = htons(serverGPorta);
    if (inet_pton(AF_INET, (const char * restrict) stringServerG_IP, (void *) & serverGIndrizzo.sin_addr) <= 0)
        raiseError(INET_PTON_SCOPE, INET_PTON_ERROR);
    //--Ci connettiamo al ServerG per effettuare la modifica allo stato di validità del GreenPass associato al codice di tessera passato da terminale
    wconnect(serverG_SFD, (struct sockaddr *) & serverGIndrizzo, (socklen_t) sizeof(serverGIndrizzo));
    if (fprintf(stdout, "\nAggiornamento Validita' GreenPass\nCodice tessera sanitaria: %s\nAggiornamento in corso...\n", * codiceTesseraSanitaria) < 0)
        raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    free(stringServerG_IP);
    return serverG_SFD;
}

void updateGreenPass (int serverG_SFD, const void * codiceTesseraSanitaria, const unsigned short int nuovoStatoGreenPass) {
    ssize_t fullWriteReturnValue, fullReadReturnValue;
    unsigned short int clientT_SenderID = clientT_viaServerG_Sender;
    //--Allochiamo la memoria per il pacchetto di risposta del ServerG
    serverGRispondeAClientT * nuovaRispostaServerG = (serverGRispondeAClientT *) calloc(1, sizeof(* nuovaRispostaServerG));
    //--Allochiamo la memoria per il pacchetto di richiesta da parte del ClientT al ServerG
    clientTRichiedeAServerG * nuovaRichiestaClientT = (clientTRichiedeAServerG *) calloc(1, sizeof(* nuovaRichiestaClientT));
    if (!nuovaRispostaServerG)
        raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    if (!nuovaRichiestaClientT)
        raiseError(CALLOC_SCOPE, CALLOC_ERROR);

    //--Copiamo il codice della tessera sanitaria nel pacchetto di richiesta
    strncpy(nuovaRichiestaClientT->codiceTesseraSanitaria, codiceTesseraSanitaria, LUNGHEZZA_CODICE_TESSERA_SANITARIA);

    /*--Associamo il valore dell'aggiornamento dello stato del GreenPass al secondo parametro del pacchetto, così da,
     tramite l'interpretazione di questo, capire se convalidare o invalidare il GreenPass*/
    nuovaRichiestaClientT->updateValue = nuovoStatoGreenPass;
    
    if (fprintf(stdout, "\nLoading...\n") < 0) raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    //--fullWrite per inviare "ClientT_SenderID"
    if ((fullWriteReturnValue = fullWrite(serverG_SFD, (const void *) & clientT_SenderID, sizeof(clientT_SenderID))) != 0)
        raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    //--fullWrite per inviare il pacchetto "nuovaRichiestaClientT"
    if ((fullWriteReturnValue = fullWrite(serverG_SFD, (const void *) nuovaRichiestaClientT, sizeof(* nuovaRichiestaClientT))) != 0)
        raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    //--fullRead per aspettare e successivamente leggere la risposta da parte del ServerG
    if ((fullReadReturnValue = fullRead(serverG_SFD, (void *) nuovaRispostaServerG, sizeof(* nuovaRispostaServerG))) != 0)
        raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);
    
    
    /*--Analizziamo il valore relativo all'esito dell'aggiornamento: se risulta essere FALSE, allora significa che l'aggiornamento
     non è andato a buon fine, altrimenti  significa che lo stato di validità del    GreenPass è stato aggiornato correttamente*/
    if (nuovaRispostaServerG->updateResult == FALSE) {
        if (fprintf(stdout, "\nNon siamo riusciti ad aggiornare il GreenPass associato alla tessera sanitaria %s.\n", nuovaRispostaServerG->codiceTesseraSanitaria) < 0) raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    } else {
        if (fprintf(stdout, "\nGrazie! Siamo risuciti ad aggiornare il  GreenPass associato alla tessera sanitaria %s.\n", nuovaRispostaServerG->codiceTesseraSanitaria) < 0) raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    }
    free(nuovaRispostaServerG);
    free(nuovaRichiestaClientT);
}
