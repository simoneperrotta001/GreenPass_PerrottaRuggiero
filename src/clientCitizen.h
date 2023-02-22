#ifndef clientCitizen_h
#define clientCitizen_h

#include "GreenPassUtility.h"



# define CLIENT_CITIZEN_ARGS_NO 2



const char * expectedUsageMessage = "<Numero Tessera Sanitaria>", * configFilePath = "../conf/clientCitizen.conf";

int setupClientCitizen      (int argc,                                  char * argv[],                  char ** codiceTesseraSanitaria    );
void getVaccination         (int centroVaccinaleSocketFileDescriptor,   const void * codiceTesseraSanitaria,  size_t nBytes               );

#endif
