#include "clientS.h"

int main (int argc, char * argv[]) {
    char * codiceTesseraSanitaria;
    
    //--Prepariamo il ClientS a connettersi con il ServerG
    int serverG_SocketFileDescriptor = setupClientS(argc, argv, & codiceTesseraSanitaria);
    //--Mettiamo il terminatore alla fine
    codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1] = '\0';
    
    
    /*
    --Richiamiamo la funzione che permette di mandare la richiesta che verifica la presenza di un eventuale Green Pass
    associato.
    */
    checkGreenPass(serverG_SocketFileDescriptor, (const void *) codiceTesseraSanitaria, (size_t) sizeof(char) * LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    wclose(serverG_SocketFileDescriptor);
    free(codiceTesseraSanitaria);
    exit(0);
}

int setupClientS (int argc, char * argv[], char ** codiceTesseraSanitaria) {
    struct sockaddr_in serverG_Address;
    char * stringServerG_IP = NULL;
    unsigned short int serverG_Port;
    int serverG_SocketFileDescriptor;
    
    //--Verifichiamo che il ClientS sia stato avviato con i parametri che si aspetta di avere
    checkUsage(argc, (const char **) argv, CLIENT_S_ARGS_NO, messaggioAtteso);
    //--Verifichiamo che il codice di tessera sanitaria immesso sia del formato e della lunghezza giusta
    checkHealtCardNumber(argv[1]);
    //--Allochiamo memoria per il codice di tessera sanitaria
    * codiceTesseraSanitaria = (char *) calloc(LUNGHEZZA_CODICE_TESSERA_SANITARIA, sizeof(char));
    if (! * codiceTesseraSanitaria) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    //--Copiamo il codice della tessera sanitaria ricevuto in input (senza terminatore quindi con lunghezza -1) nel puntatore ad array di char codiceTesseraSanitaria
    strncpy(* codiceTesseraSanitaria, (const char *) argv[1], LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1);
    
    //--Ricaviamo i parametri fondamentali per contattare il CentroVaccinale.
    retrieveConfigurationData(percorsoFileConfigurazione, & stringServerG_IP, & serverG_Port);
    serverG_SocketFileDescriptor = wsocket(AF_INET, SOCK_STREAM, 0);
    memset((void *) & serverG_Address, 0, sizeof(serverG_Address));
    serverG_Address.sin_family = AF_INET;
    serverG_Address.sin_port   = htons(serverG_Port);
    if (inet_pton(AF_INET, (const char * restrict) stringServerG_IP, (void *) & serverG_Address.sin_addr) <= 0) raiseError(INET_PTON_SCOPE, INET_PTON_ERROR);
    
    wconnect(serverG_SocketFileDescriptor, (struct sockaddr *) & serverG_Address, (socklen_t) sizeof(serverG_Address));
    if (fprintf(stdout, "\nVerifica GreenPass\nNumero tessera sanitaria: %s\n\n... A breve verra' mostrato se il GreenPass inserito risulta essere valido...\n", * codiceTesseraSanitaria) < 0) raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    free(stringServerG_IP);
    return serverG_SocketFileDescriptor;
}

void checkGreenPass (int serverG_SocketFileDescriptor, const void * codiceTesseraSanitaria, size_t nBytes) {
    ssize_t fullWriteReturnValue, fullReadReturnValue;
    unsigned short int clientS_SenderID = clientS_viaServerG_Sender;
    
    //--Allochiamo la memoria per il pacchetto di risposta del ServerG.
    serverG_ReplyToClientS * newServerG_Reply = (serverG_ReplyToClientS *) calloc(1, sizeof(* newServerG_Reply));
    if (!newServerG_Reply) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    
    if (fprintf(stdout, "\n... Verifica in corso ...\n") < 0) raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    // fullWrite per la scrittura e invio dell'ID del ClientS al ServerG.
    if ((fullWriteReturnValue = fullWrite(serverG_SocketFileDescriptor, (const void *) & clientS_SenderID, sizeof(clientS_SenderID))) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    // fullWrite per la scrittura e invio del codice di tessera sanitaria del ClientS al ServerG.
    if ((fullWriteReturnValue = fullWrite(serverG_SocketFileDescriptor, codiceTesseraSanitaria, nBytes)) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    
    
    /*
    fullRead per ottenere e leggere la risposta da parte del ServerG. Tale risposta è caratterizzata da una
    serie di parametri: Codice Tessera Sanitaria ed esito della verifica del Vanilla Green Pass associato
    (se esistente). La risposta verrà salvata in "newServerG_Reply".
    */
    if ((fullReadReturnValue = fullRead(serverG_SocketFileDescriptor, (void *) newServerG_Reply, sizeof(* newServerG_Reply))) != 0) raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);
    
    
    /*
    --Effettuiamo un ultimo controllo su quest'ultimo parametro: se è FALSE, allora non esiste un codice di tessera
    sanitaria pari a quello fornito associato ad un Vanilla Green Pass o il Vanilla Green Pass associato al
    codice di tessera sanitaria fornito risulta non essere valido. Se invece l'ultimo campo del pacchetto di
    risposta è TRUE, significa che il Vanilla Green Pass associato è valido.
    */
    if (newServerG_Reply->requestResult == FALSE) {
        if (fprintf(stdout, "\nLa tessera sanitaria immessa non risulta essere associata a un GreenPass attualmente valido.\nArrivederci.\n") < 0) raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    } else {
        if (fprintf(stdout, "\nLa tessera sanitaria immessa risulta essere associata a un GreenPass attualmente valido.\nArrivederci.\n") < 0) raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    }
    free(newServerG_Reply);
}
