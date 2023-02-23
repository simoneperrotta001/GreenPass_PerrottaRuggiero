#ifndef clientS_h
#define clientS_h

#include "GreenPassUtility.h"



# define CLIENT_S_ARGS_NO 2



const char * messaggioAtteso = "<Numero Tessera Sanitaria da Controllare>", * percorsoFileConfigurazione = "../conf/clientS.conf";

int setupClientS        (int argc,                          char * argv[],                  char ** codiceTesseraSanitaria);
void checkGreenPass     (int serverG_SFD,  const void * codiceTesseraSanitaria,  size_t nBytes           );

#endif /* clientS_h */
