#include "clientCitizen.h"

int main (int argc, char * argv[]) {
    char * codiceTesseraSanitaria;
    
    //tramite la seguente funzione controlliamo che gli argomenti passati siano corretti.
    //Nello specifico controlliamo che il formato della tessera sanitaria sia quello previsto.
    //Dopodichè verrà creato un socket file descriptor, che verrà poi restituito, collegato al centroVaccinale.
    int centroVaccinaleSFD = setupClientCitizen(argc, argv, & codiceTesseraSanitaria);
    codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1] = '\0';

    //Effettuiamo la richiesta di vaccinazione ricevendo quindi il Greenpass. I parametri della funzione sono:
    //-Il socket file descriptor del centroVaccinale;
    //-Il codice della tessera sanitaria;
    //-La lunghezza del codice della tessera, ovvero 16;
    somministraVaccinazione(centroVaccinaleSFD, (const void *) codiceTesseraSanitaria, (size_t) sizeof(char) * LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    wclose(centroVaccinaleSFD);
    free(codiceTesseraSanitaria);
    exit(0);
}

int setupClientCitizen (int argc, char * argv[], char ** codiceTesseraSanitaria) {
    int centroVaccinaleSFD;//lunghezzaCodiceTessera,
    struct sockaddr_in centroVaccinaleIndirizzo;
    char * stringcentroVaccinaleIndirizzoIP = NULL;
    unsigned short int centroVaccinalePorta;
    
    // Si verifica che il clientCitizen sia stato avviato con i parametri corretti
    checkUsage(argc, (const char **) argv, NUMERO_PARAMETRI_CLIENT_CITIZEN, messaggioAtteso);
    // Si verifica che il codice di tessera sanitaria immesso sia del formato e della lunghezza giusta
    checkHealtCardNumber(argv[1]);

    //Si alloca quantità di memoria per il codiceTesseraSanitaria e si controlla che ciò sia stato fatto correttamente
    * codiceTesseraSanitaria = (char *) calloc(LUNGHEZZA_CODICE_TESSERA_SANITARIA, sizeof(char));
    if (! * codiceTesseraSanitaria) raiseError(CALLOC_SCOPE, CALLOC_ERROR);

    // Si copia il valore passato dal terminale nel codiceTesseraSanitaria
    strncpy(* codiceTesseraSanitaria, (const char *) argv[1], LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1);

    //Si ricavano i parametri  per contattare il CentroVaccinale dal file di configurazione "percorsoFileConfigurazione"
    retrieveConfigurationData(percorsoFileConfigurazione, & stringcentroVaccinaleIndirizzoIP, & centroVaccinalePorta);
    
    centroVaccinaleSFD = wsocket(AF_INET, SOCK_STREAM, 0);
    memset((void *) & centroVaccinaleIndirizzo, 0, sizeof(centroVaccinaleIndirizzo));
    centroVaccinaleIndirizzo.sin_family = AF_INET;
    centroVaccinaleIndirizzo.sin_port   = htons(centroVaccinalePorta);
    if (inet_pton(AF_INET, (const char * restrict) stringcentroVaccinaleIndirizzoIP, (void *) & centroVaccinaleIndirizzo.sin_addr) <= 0) raiseError(INET_PTON_SCOPE, INET_PTON_ERROR);
    
    wconnect(centroVaccinaleSFD, (struct sockaddr *) & centroVaccinaleIndirizzo, (socklen_t) sizeof(centroVaccinaleIndirizzo));
    if (fprintf(stdout, "\nCiao e benvenuto al centro vaccinale.\n  Il tuo numero di tessera sanitaria e': %s\n Ora ti verra' somministrato il vaccino.\n", * codiceTesseraSanitaria) < 0) raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    free(stringcentroVaccinaleIndirizzoIP);
    return centroVaccinaleSFD;
}

void somministraVaccinazione (int centroVaccinaleSFD, const void * codiceTesseraSanitaria, size_t lunghezzaCodiceTessera) {
    ssize_t fullWriteReturnValue, fullReadReturnValue;
    // Si alloca la memoria per il pacchetto di risposta del Centro Vaccinale.
    centroVaccinaleReplyToClientCitizen * newCentroVaccinaleReply = (centroVaccinaleReplyToClientCitizen *) calloc(1, sizeof(* newCentroVaccinaleReply));
    if (!newCentroVaccinaleReply) raiseError(CALLOC_SCOPE, CALLOC_ERROR);
    
    // fullWrite per la scrittura e invio del codice della tessera sanitaria del cittadino al Centro Vaccinale.
    if ((fullWriteReturnValue = fullWrite(centroVaccinaleSFD, codiceTesseraSanitaria, lunghezzaCodiceTessera)) != 0) raiseError(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);
    
    /*
    fullRead per ottenere e leggere la risposta da parte del CentroVaccinale. Tale risposta è caratterizzata da
    una serie di parametri: Codice Tessera Sanitaria, Data Scadenza Vanilla Green Pass ed esito della richiesta.
    La risposta verrà salvata in "newCentrovaccinaleReply".
    */
    if ((fullReadReturnValue = fullRead(centroVaccinaleSFD, (void *) newCentroVaccinaleReply, sizeof(* newCentroVaccinaleReply))) != 0) raiseError(FULL_READ_SCOPE, (int) fullReadReturnValue);
    
    /*
    Si effettua un controllo sul terzo campo: se esso conterrà il valore FALSE, allora ciò vorrà dire che non
    è stato possibile inoculare una nuova dose di vaccino, in quanto non è passata la soglia temporale
    minima per effettuare una nuova vaccinazione. Se invece il terzo campo conterrà il valore TRUE, allora
    significa che la vaccinazione è andata a buon fine. Fatto ciò, il ClientCitizen libera la memoria occupata,
    rilascia le risorse e chiude il socket file descriptor richiesto in precedenza.
    */
    if (newCentroVaccinaleReply->requestResult == FALSE) {
        if (fprintf(stdout, "\nNon può al momento ricevere un'altra somministrazione di vaccino. \n E' necessario che passino altri %d mesi dall'ultima somministrazione.\nLa data a partire dalla quale può effettuare l'ulteriore somministrazione e': %s\n", MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE, newCentroVaccinaleReply->dataScadenzaGreenPass) < 0) raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    } else {
        if (fprintf(stdout, "\nVaccinazione effettuata con successo.\nLa data a partire dalla quale puoi effettuare l'ulteriore somministrazione e': %s\nArrivederci.\n", newCentroVaccinaleReply->dataScadenzaGreenPass) < 0) raiseError(FPRINTF_SCOPE, FPRINTF_ERROR);
    }
    free(newCentroVaccinaleReply);
}
