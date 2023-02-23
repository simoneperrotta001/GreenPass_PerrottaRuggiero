#ifndef GreenPassUtility_h
#define GreenPassUtility_h

#include "WrapperFunction.h"



#define CHECK_HEALTH_CARD_NUMBER_SCOPE "checkcodiceTesseraSanitaria"
#define CHECK_HEALTH_CARD_NUMBER_ERROR 200

#define INVALID_SENDER_ID_SCOPE "invalidSenderID_serverV"
#define INVALID_SENDER_ID_ERROR 201

#define INVALID_UPDATE_STATUS_SCOPE "invalidGreenPassUpdateStatus"
#define INVALID_UPDATE_STATUS_ERROR 202

#define MESI_ATTESA_PROSSIMA_SOMMINISTRAZIONE 6
#define LUNGHEZZA_CODICE_TESSERA_SANITARIA 17
#define LUNGHEZZA_DATA 11
#define MESI_IN_ANNO 12



typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    char dataScadenzaGreenPass[LUNGHEZZA_DATA];
    unsigned short int requestResult;
} centroVaccinaleRispondeAClientUtente;

typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    char dataScadenzaGreenPass[LUNGHEZZA_DATA];
} centroVaccinaleRichiedeAServerV;

typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    char dataScadenzaGreenPass[LUNGHEZZA_DATA];
    unsigned short int requestResult;
} serverVRispondeACentroVaccinale;

typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    unsigned short int requestResult;
} serverV_ReplyToServerG_clientS;

typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    unsigned short int updateValue;
} serverG_RequestToServerV_onBehalfOfClientT;

typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    unsigned short int updateResult;
} serverV_ReplyToServerG_clientT;

typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    unsigned short int requestResult;
} serverG_ReplyToClientS;

typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    unsigned short int updateValue;
} clientT_RequestToServerG;

typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    unsigned short int updateResult;
} serverG_ReplyToClientT;

enum sender {
    centroVaccinaleSender,
    clientS_viaServerG_Sender,
    clientT_viaServerG_Sender
};

void checkCodiceTesseraSanitaria           (char * codiceTesseraSanitaria);
void ritornaDatiDiConfigurazione      (const char * percorsoFileConfigurazione, char ** configurationIP, unsigned short int * configurationPort);
char * getdataScadenzaVaccino     (void );
char * getNowDate                   (void );
int creaConnessioneConServerV     (const char * percorsoFileConfigurazione);

#endif /* GreenPassUtility_h */
