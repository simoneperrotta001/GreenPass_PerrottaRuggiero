#include "clientUtente.h"

int main (int argc, char * argv[]) {
    char * codiceTesseraSanitaria;
    //--Controlliamo il formato della tessera sanitaria e creiamo un socket file descriptor, che verrà poi restituito, collegato al centroVaccinale.
    int centroVaccinaleSFD = setupclientUtente(argc, argv, & codiceTesseraSanitaria);
    codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1] = '\0';

    /*
     --Effettuiamo la richiesta di vaccinazione ricevendo quindi il Greenpass. I parametri della funzione sono:
    -Il socket file descriptor del centroVaccinale;
    -Il codice della tessera sanitaria;
    -La lunghezza del codice della tessera, ovvero 16;*/
    somministraVaccinazione(centroVaccinaleSFD, (const void *) codiceTesseraSanitaria, (size_t) sizeof(char) * LUNGHEZZA_CODICE_TESSERA_SANITARIA);
    wclose(centroVaccinaleSFD);
    free(codiceTesseraSanitaria);
    exit(0);
}

int setupclientUtente (int argc, char * argv[], char ** codiceTesseraSanitaria) {
    int centroVaccinaleSFD;//lunghezzaCodiceTessera,
    struct sockaddr_in centroVaccinaleIndirizzo;
    char * stringcentroVaccinaleIndirizzoIP = NULL;
    unsigned short int centroVaccinalePorta;
    
    //--Verifichiamo che il clientUtente sia stato avviato con i parametri corretti
    checkUtilizzo(argc, (const char **) argv, NUMERO_PARAMETRI_CLIENT_CITIZEN, messaggioAtteso);
    //--Verichiamo che il codice di tessera sanitaria immesso sia del formato e della lunghezza giusta
    checkCodiceTesseraSanitaria(argv[1]);

    //--Allochiamo memoria per il codice di tessera sanitaria
    * codiceTesseraSanitaria = (char *) calloc(LUNGHEZZA_CODICE_TESSERA_SANITARIA, sizeof(char));
    if (! * codiceTesseraSanitaria) lanciaErrore(CALLOC_SCOPE, CALLOC_ERROR);

    //--Copiamo il valore passato dal terminale nel codiceTesseraSanitaria
    strncpy(* codiceTesseraSanitaria, (const char *) argv[1], LUNGHEZZA_CODICE_TESSERA_SANITARIA - 1);

    //--Ricaviamo i parametri  per contattare il CentroVaccinale.
    ritornaDatiDiConfigurazione(percorsoFileConfigurazione, & stringcentroVaccinaleIndirizzoIP, & centroVaccinalePorta);
    
    centroVaccinaleSFD = wsocket(AF_INET, SOCK_STREAM, 0);
    memset((void *) & centroVaccinaleIndirizzo, 0, sizeof(centroVaccinaleIndirizzo));
    centroVaccinaleIndirizzo.sin_family = AF_INET;
    centroVaccinaleIndirizzo.sin_port   = htons(centroVaccinalePorta);
    if (inet_pton(AF_INET, (const char * restrict) stringcentroVaccinaleIndirizzoIP, (void *) & centroVaccinaleIndirizzo.sin_addr) <= 0) lanciaErrore(INET_PTON_SCOPE, INET_PTON_ERROR);
    
    wconnect(centroVaccinaleSFD, (struct sockaddr *) & centroVaccinaleIndirizzo, (socklen_t) sizeof(centroVaccinaleIndirizzo));
    if (fprintf(stdout, "\nCiao e benvenuto al centro vaccinale.\n  Il tuo numero di tessera sanitaria e': %s\n Ora ti verra' somministrato il vaccino.\n", * codiceTesseraSanitaria) < 0) lanciaErrore(FPRINTF_SCOPE, FPRINTF_ERROR);
    free(stringcentroVaccinaleIndirizzoIP);
    return centroVaccinaleSFD;
}

void somministraVaccinazione (int centroVaccinaleSFD, const void * codiceTesseraSanitaria, size_t lunghezzaCodiceTessera) {
    ssize_t fullWriteReturnValue, fullReadReturnValue;
    //--Allochiamo memoria per il pacchetto di risposta del centroVaccinale
    centroVaccinaleRispondeAClientUtente * rispostaCentroVaccinale = (centroVaccinaleRispondeAClientUtente *) calloc(1, sizeof(* rispostaCentroVaccinale));
    if (!rispostaCentroVaccinale) lanciaErrore(CALLOC_SCOPE, CALLOC_ERROR);
    
    //fullWrite per la scrittura e invio del codice della tessera sanitaria al Centro Vaccinale
    if ((fullWriteReturnValue = fullWrite(centroVaccinaleSFD, codiceTesseraSanitaria, lunghezzaCodiceTessera)) != 0) lanciaErrore(FULL_WRITE_SCOPE, (int) fullWriteReturnValue);

    /*fullRead per ottenere e leggere la risposta da parte del CentroVaccinale. Avremo come risposta una serie
    di parametri: Codice Tessera Sanitaria, Data Scadenza GreenPass ed esito della richiesta.
    La risposta verrà salvata in "rispostaCentroVaccinale".*/
    if ((fullReadReturnValue = fullRead(centroVaccinaleSFD, (void *) rispostaCentroVaccinale, sizeof(* rispostaCentroVaccinale))) != 0) lanciaErrore(FULL_READ_SCOPE, (int) fullReadReturnValue);
    

    /*
    --Verifihciamo se la risposta conterrà come terzo parametro un valore FALSE allora non è stato possibile
    somministrare una nuova dose di vaccino, in quanto non è passato abbastanza tempo da superare la soglia
    minima per effettuare una nuova vaccinazione. Se invece il valore del terzo campo sarà TRUE, allora
    significa che la vaccinazione è andata a buon fine.
    Successivamente attraverso il clientUtente liberiamo la memoria occupata, rilascia le risorse e chiude il socket file descriptor richiesto in precedenza.*/
    if (rispostaCentroVaccinale->requestResult == FALSE) {
        if (fprintf(stdout, "\nNon può al momento ricevere un'altra somministrazione di vaccino. \n E' necessario che passino altri %d mesi dall'ultima somministrazione.\nLa data a partire dalla quale può effettuare l'ulteriore somministrazione e': %s\n", MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE, rispostaCentroVaccinale->dataScadenzaGreenPass) < 0) lanciaErrore(FPRINTF_SCOPE, FPRINTF_ERROR);
    } else {
        if (fprintf(stdout, "\nVaccinazione effettuata con successo.\nLa data a partire dalla quale puoi effettuare l'ulteriore somministrazione e': %s\nArrivederci.\n", rispostaCentroVaccinale->dataScadenzaGreenPass) < 0) lanciaErrore(FPRINTF_SCOPE, FPRINTF_ERROR);
    }
    free(rispostaCentroVaccinale);
}
