#include "WrapperFunction.h"
/*--In questo file troviamo tutte le funzioni wrappate necessarie per la creazione, gestione e
eliminazione dei socket per la comunicazione tra client e Server*/
int wsocket (int domain, int type, int protocol) {
    int socketFileDescriptor;
    if ((socketFileDescriptor = socket(domain, type, protocol)) == -1) lanciaErrore(WSOCKET_SCOPE, SOCKET_ERROR);
    return socketFileDescriptor;
}

void wconnect (int socketFileDescriptor, const struct sockaddr * address, socklen_t addressLength) {
    if (connect(socketFileDescriptor, address, addressLength) == -1) lanciaErrore(WCONNECT_SCOPE, CONNECT_ERROR);
}

void wclose (int socketFileDescriptor) {
    if (close(socketFileDescriptor) == -1) lanciaErrore(WCLOSE_SCOPE, CLOSE_ERROR);
}

void wbind (int socketFileDescriptor, const struct sockaddr * address, socklen_t addressLength) {
    if (bind(socketFileDescriptor, address, addressLength) == -1) lanciaErrore(WBIND_SCOPE, BIND_ERROR);
}

void wlisten (int socketFileDescriptor, int backlog) {
    if (listen(socketFileDescriptor, backlog) == -1) lanciaErrore(WLISTEN_SCOPE, LISTEN_ERROR);
}

int waccept (int socketFileDescriptor, struct sockaddr * restrict address, socklen_t * restrict addressLength) {
    int communicationSocketFileDescriptor;
    if ((communicationSocketFileDescriptor = accept(socketFileDescriptor, address, addressLength)) == -1) lanciaErrore(WACCEPT_SCOPE, ACCEPT_ERROR);
    return communicationSocketFileDescriptor;
}

//--Verifico se l'array di caratteri passato risulta essere un IP valido in formato IPv4 o meno
void checkIP (char * IP_string) {
    char tempBuffer[16];
    if ((inet_pton(AF_INET, (const char * restrict) IP_string, (void *) tempBuffer)) <= 0) lanciaErrore(CHECK_IP_SCOPE, CHECK_IP_ERROR);
}
