#ifndef clientS_h
#define clientS_h

#include "GreenPassUtility.h"



# define CLIENT_S_ARGS_NO 2



const char * expectedUsageMessage = "<Numero Tessera Sanitaria da Controllare>", * configFilePath = "../conf/clientS.conf";

int setupClientS        (int argc,                          char * argv[],                  char ** codiceTesseraSanitaria);
void checkGreenPass     (int serverG_SocketFileDescriptor,  const void * codiceTesseraSanitaria,  size_t nBytes           );

#endif /* clientS_h */
