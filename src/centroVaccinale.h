#ifndef centroVaccinale_h
#define centroVaccinale_h

#include "GreenPassUtility.h"



#define CENTRO_VACCINALE_ARGS_NO 2



const char * expectedUsageMessage = "<Centro Vaccinale Port>", * configFilePathCentroVaccinale = "../conf/centroVaccinale.conf";

void clientCitizenRequestHandler    (int connectionFileDescriptor, int serverV_SocketFileDescriptor);

#endif /* centroVaccinale_h */
