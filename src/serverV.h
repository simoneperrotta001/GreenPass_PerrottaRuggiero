#ifndef serverV_h
#define serverV_h

#include "GreenPassUtility.h"



#define SERVER_V_ARGS_NO 2



pthread_mutex_t fileSystemAccessMutex;
pthread_mutex_t connectionFileDescriptorMutex;
const char * dataPath =     "../data/serverV.dat";
const char * tempDataPath = "../data/tempServerV.dat";
const char * expectedUsageMessage = "<ServerV Port>";

void * centroVaccinaleRequestHandler        (void * args                                                                                );
void * clientS_viaServerG_RequestHandler    (void * args                                                                                );
void * clientT_viaServerG_RequestHandler    (void * args                                                                                );
void threadAbort                            (char * errorScope,     int exitCode, int threadConnectionFileDescriptor, void * arg1, ...  );
#endif /* serverV_h */
