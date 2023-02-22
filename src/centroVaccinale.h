#ifndef centroVaccinale_h
#define centroVaccinale_h

#include "GreenPassUtility.h"



#define NUMERO_PARAMETRI_CENTRO_VACCINALE 2



const char * messaggioAtteso = "<Centro Vaccinale Porta>", * percorsoFileConfigurazioneCentroVaccinale = "../conf/centroVaccinale.conf";

void clientCitizenRequestHandler    (int connectionFileDescriptor, int serverV_SFD);

#endif /* centroVaccinale_h */
