#ifndef serverG_h
#define serverG_h

#include "GreenPassUtility.h"



#define SERVER_G_ARGS_NO 2



const char * messaggioAtteso = "<ServerG Port>", * percorsoFileConfigurazioneServerG = "../conf/serverG.conf";

void clientS_RequestHandler     (int connectionFileDescriptor, int serverV_SFD);
void clientT_RequestHandler     (int connectionFileDescriptor, int serverV_SFD);

#endif /* serverG_h */
