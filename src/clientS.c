#include "clientS.h"

int main (int argc, char * argv[]) {
    char * codiceTesseraSanitaria;
    
    //--Prepariamo il ClientS a connettersi con il ServerG
    int serverG_SFD = setupClientS(argc, argv, & codiceTesseraSanitaria);
    //--Mettiamo il terminatore alla fine
    codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1] = '\0';
    
    
    /*
    --Richiamiamo la funzione che permette di mandare la richiesta la quale verifica la presenza di un eventuale  GreenPass
    associato.
    */
    checkGreenPass(serverG_SFD, (const void *) codiceTesseraSanitaria, (size_t) sizeof(char) * LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    wclose(serverG_SFD);
    free(codiceTesseraSanitaria);
    exit(0);
}

int setupClientS (int argc, char * argv[], char ** codiceTesseraSanitaria) {
    struct sockaddr_in serverGIndrizzo;
    char * stringServerG_IP = NULL;
    unsigned short int serverGPorta;
    int serverG_SFD;
    
    //--Verifichiamo che il ClientS sia stato avviato con i parametri che si aspetta di avere
    checkUtilizzo(argc, (const char **) argv, CLIENT_S_ARGS_NO, messaggioAtteso);
    //--Verifichiamo che il codice di tessera sanitaria immesso sia del formato e della lunghezza giusta
    checkCodiceTesseraSanitaria(argv[1]);
    //--Allochiamo memoria per il codice di tessera sanitaria
    * codiceTesseraSanitaria = (char *) calloc(LUNGHEZZA_CODICE_TESSERA_SANITARIA, sizeof(char));
    if (! * codiceTesseraSanitaria) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    //--Copiamo il codice della tessera sanitaria ricevuto in input (senza terminatore quindi con lunghezza -1) nel puntatore ad array di char codiceTesseraSanitaria
    strncpy(* codiceTesseraSanitaria, (const char *) argv[1], LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1);
    
    //--Ricaviamo i parametri fondamentali per contattare il CentroVaccinale
    getDatiDiConfigurazione(percorsoFileConfigurazione, & stringServerG_IP, & serverGPorta);
    //--Creiamo il socket per la comunicazione con serverG
    serverG_SFD = wsocket(AF_INET, SOCK_STREAM, 0);
    memset((void *) & serverGIndrizzo, 0, sizeof(serverGIndrizzo));
    serverGIndrizzo.sin_family = AF_INET;
    serverGIndrizzo.sin_port   = htons(serverGPorta);
    if (inet_pton(AF_INET, (const char * restrict) stringServerG_IP, (void *) & serverGIndrizzo.sin_addr) <= 0)
        raiseError(INET_PTON_SCOPE, INET_PTON_ERROR);
    //--Ci connettiamo al serverG per poter effettuare l'eventuale modifica allo stato di validità del GreenPass
    wconnect(serverG_SFD, (struct sockaddr *) & serverGIndrizzo, (socklen_t) sizeof(serverGIndrizzo));
    if (fprintf(stdout, "\nVerifica GreenPass\nNumero tessera sanitaria: %s\n\n... A breve verra' mostrato se il GreenPass inserito risulta essere valido...\n", * codiceTesseraSanitaria) < 0)
        raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    free(stringServerG_IP);
    return serverG_SFD;
}

void checkGreenPass (int serverG_SFD, const void * codiceTesseraSanitaria, size_t nBytes) {
    ssize_t fullWriteReturnValue, fullReadReturnValue;
    unsigned short int clientS_SenderID = clientS_viaServerG_Sender;
    
    //--Allochiamo la memoria per il pacchetto di risposta del ServerG
    serverGRispondeAClientS * nuovaRispostaServerG = (serverGRispondeAClientS *) calloc(1, sizeof(* nuovaRispostaServerG));
    if (!nuovaRispostaServerG)
        raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    
    if (fprintf(stdout, "\n... Verifica in corso ...\n") < 0) raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    // fullWrite per la scrittura e invio dell'ID del ClientS al ServerG
    if ((fullWriteReturnValue = fullWrite(serverG_SFD, (const void *) & clientS_SenderID, sizeof(clientS_SenderID))) != 0)
        raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    // fullWrite per la scrittura e invio del codice di tessera sanitaria del ClientS al ServerG.
    if ((fullWriteReturnValue = fullWrite(serverG_SFD, codiceTesseraSanitaria, nBytes)) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);

    /*fullRead per ottenere e leggere la risposta da parte del ServerG. Tale risposta è caratterizzata da una
    serie di parametri: Codice Tessera Sanitaria ed esito della verifica del  GreenPass associato
    (se esistente).*/
    if ((fullReadReturnValue = fullRead(serverG_SFD, (void *) nuovaRispostaServerG, sizeof(* nuovaRispostaServerG))) != 0)
        raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);

    /*--Effettuiamo un ultimo controllo su quest'ultimo parametro: se è FALSE, allora non esiste un codice di tessera
    sanitaria pari a quello fornito associato ad un  GreenPass o il  GreenPass associato al
    codice di tessera sanitaria fornito risulta non essere valido. Se invece l'ultimo campo del pacchetto di
    risposta è TRUE, significa che il    GreenPass associato è valido*/
    if (nuovaRispostaServerG->requestResult == FALSE) {
        if (fprintf(stdout, "\nLa tessera sanitaria immessa non risulta essere associata a un GreenPass attualmente valido.\n") < 0)
            raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    } else {
        if (fprintf(stdout, "\nLa tessera sanitaria immessa risulta essere associata a un GreenPass attualmente valido.\n") < 0)
            raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    }
    free(nuovaRispostaServerG);
}
