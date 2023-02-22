#ifndef clientT_h
#define clientT_h

#include "GreenPassUtility.h"



# define CLIENT_T_ARGS_NO 3



const char * expectedUsageMessage = "<Numero Tessera Sanitaria> <Nuovo Stato Green Pass (0 = NON VALIDO / 1 = VALIDO)>", * configFilePath = "../conf/clientT.conf";

int setupClientT            (int argc,                          char * argv[],                  char ** healthCaardNumber,                  int * newGreenPassStatus);
void updateGreenPass        (int serverG_SocketFileDescriptor,  const void * codiceTesseraSanitaria,  const unsigned short int newGreenPassStatus                         );

#endif /* clientT_h */
