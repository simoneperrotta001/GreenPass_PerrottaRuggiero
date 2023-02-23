#include "UsageUtility.h"

//-- Questa procedura effettua un controllo dei parametri relativi ad ogni entità invocata
void checkUtilizzo (int argc, const char * argv[], int expected_argc, const char * messaggioAtteso) {
    if (argc != expected_argc) {
        if (fprintf(stderr, (const char * restrict) "Usage: %s %s\n", argv[0], messaggioAtteso) < 0) lanciaErrore(FPRINTF_SCOPE, FPRINTF_ERROR);
        lanciaErrore(CHECK_USAGE_SCOPE, CHECK_USAGE_ERROR);
    }
}

/*-- Questa procedura si occupa della gestione degli errori con la visualizzazione dello scope e di conseguenza
  termina il processo.*/
void lanciaErrore (char * errorScope, int exitCode) {
    if (fprintf(stderr, (const char * restrict) "Scope: %s - Error #%d\n", errorScope, exitCode) < 0) lanciaErrore(FPRINTF_SCOPE, FPRINTF_ERROR);
    exit(exitCode);
}

/*
-- Questa procedura si occupa della gestione degli errori con la visualizzazione dello scope e di conseguenza
  termina il thread.
*/
void threadlanciaErrore (char * errorScope, int exitCode) {
    if (fprintf(stderr, (const char * restrict)  "Scope: %s - Error #%d\n", errorScope, exitCode) < 0) lanciaErrore(FPRINTF_SCOPE, FPRINTF_ERROR);
    pthread_exit(NULL);
}

ssize_t fullRead (int fileDescriptor, void * buffer, size_t nBytes) {
    size_t nBytesLeft = nBytes;
    ssize_t nBytesRead;
    
    while (nBytesLeft > 0) {
        // == -1
        if ((nBytesRead = read(fileDescriptor, buffer, nBytesLeft)) < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                return nBytesRead;
            }
        } else if (nBytesRead == 0) {
            break;
        }
        
        nBytesLeft -= nBytesRead;
        buffer += nBytesRead;
    }
    
    return nBytesLeft;
}

ssize_t fullWrite (int fileDescriptor, const void * buffer, size_t nBytes) {
    size_t nBytesLeft = nBytes;
    ssize_t nBytesWrite;
    
    while (nBytesLeft > 0) {
        // == -1
        if ((nBytesWrite = write(fileDescriptor, buffer, nBytesLeft)) < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                return nBytesWrite;
            }
        }
        
        nBytesLeft -= nBytesWrite;
        buffer += nBytesWrite;
    }
    
    return nBytesLeft;
}
