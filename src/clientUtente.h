#ifndef clientUtente_h
#define clientUtente_h

#include "GreenPassUtility.h"

# define NUMERO_PARAMETRI_CLIENT_CITIZEN 2

const char * messaggioAtteso = "<Numero Tessera Sanitaria>", * percorsoFileConfigurazione = "../conf/clientUtente.conf";

int setupclientUtente (int argc, char * argv[], char ** codiceTesseraSanitaria);
void somministraVaccinazione (int centroVaccinaleSFD, const void * codiceTesseraSanitaria,  size_t lunghezzaCodiceTessera );

#endif
