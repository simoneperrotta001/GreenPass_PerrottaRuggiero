#include "WrapperFunction.h"
/*--In questo file troviamo tutte le funzioni wrappate necessarie per la creazione, gestione e
eliminazione dei socket per la comunicazione tra client e Server*/
int wsocket (int dominio, int tipo, int protocollo) {
    int socketFD;
    if ((socketFD = socket(dominio, tipo, protocollo)) == -1) raiseError(WSOCKET_SCOPE, SOCKET_ERROR);
    return socketFD;
}

void wconnect (int socketFD, const struct sockaddr * indirizzo, socklen_t lunghezzaIndirizzo) {
    if (connect(socketFD, indirizzo, lunghezzaIndirizzo) == -1) raiseError(WCONNECT_SCOPE, CONNECT_ERROR);
}

void wclose (int socketFD) {
    if (close(socketFD) == -1) raiseError(WCLOSE_SCOPE, CLOSE_ERROR);
}

void wbind (int socketFD, const struct sockaddr * indirizzo, socklen_t lunghezzaIndirizzo) {
    if (bind(socketFD, indirizzo, lunghezzaIndirizzo) == -1) raiseError(WBIND_SCOPE, BIND_ERROR);
}

void wlisten (int socketFD, int backlog) {
    if (listen(socketFD, backlog) == -1) raiseError(WLISTEN_SCOPE, LISTEN_ERROR);
}

int waccept (int socketFD, struct sockaddr * restrict indirizzo, socklen_t * restrict lunghezzaIndirizzo) {
    int communicationsocketFD;
    if ((communicationsocketFD = accept(socketFD, indirizzo, lunghezzaIndirizzo)) == -1) raiseError(WACCEPT_SCOPE, ACCEPT_ERROR);
    return communicationsocketFD;
}

//--Verifico se l'array di caratteri passato risulta essere un IP valido in formato IPv4 o meno
void checkIP (char * IP_string) {
    char tempBuffer[16];
    if ((inet_pton(AF_INET, (const char * restrict) IP_string, (void *) tempBuffer)) <= 0) raiseError(CHECK_IP_SCOPE, CHECK_IP_ERROR);
}
