#ifndef clientCitizen_h
#define clientCitizen_h

#include "GreenPassUtility.h"



# define NUMERO_PARAMETRI_CLIENT_CITIZEN 2



const char * messaggioAtteso = "<Numero Tessera Sanitaria>", * percorsoFileConfigurazione = "../conf/clientCitizen.conf";

int setupClientCitizen (int argc, char * argv[], char ** codiceTesseraSanitaria);
void somministraVaccinazione (int centroVaccinaleSFD, const void * codiceTesseraSanitaria,  size_t lunghezzaCodiceTessera );

#endif
