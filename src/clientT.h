#ifndef clientT_h
#define clientT_h

#include "GreenPassUtility.h"

# define CLIENT_T_ARGS_NO 3

const char * messaggioAtteso = "<Numero Tessera Sanitaria> <Nuovo Stato  GreenPass (0 = NON VALIDO / 1 = VALIDO)>", * percorsoFileConfigurazione = "../conf/clientT.conf";

int setupClientT            (int argc,                          char * argv[],                  char ** healthCaardNumber,                  int * nuovoStatoGreenPass);
void updateGreenPass        (int serverG_SFD,  const void * codiceTesseraSanitaria,  const unsigned short int nuovoStatoGreenPass                         );

#endif