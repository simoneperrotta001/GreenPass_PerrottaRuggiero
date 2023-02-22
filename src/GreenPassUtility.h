#ifndef GreenPassUtility_h
#define GreenPassUtility_h

#include "NetWrapper.h"



#define CHECK_HEALTH_CARD_NUMBER_SCOPE "checkcodiceTesseraSanitaria"
#define CHECK_HEALTH_CARD_NUMBER_ERROR 200

#define INVALID_SENDER_ID_SCOPE "invalidSenderID_serverV"
#define INVALID_SENDER_ID_ERROR 201

#define INVALID_UPDATE_STATUS_SCOPE "invalidGreenPassUpdateStatus"
#define INVALID_UPDATE_STATUS_ERROR 202

#define MONTHS_TO_WAIT_FOR_NEXT_VACCINATION 5
#define LUNGHEZZA_CODICE_TESSERA_SANITARIA 17
#define DATE_LENGTH 11
#define MONTHS_IN_A_YEAR 12



typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    char greenPassExpirationDate[DATE_LENGTH];
    unsigned short int requestResult;
} centroVaccinaleReplyToClientCitizen;

typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    char greenPassExpirationDate[DATE_LENGTH];
} centroVaccinaleRequestToServerV;

typedef struct {
    char codiceTesseraSanitaria[LUNGHEZZA_CODICE_TESSERA_SANITARIA];
    char greenPassExpirationDate[DATE_LENGTH];
    unsigned short int requestResult;
} serverV_ReplyToCentroVaccinale;

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

void checkHealtCardNumber           (char * codiceTesseraSanitaria                                                                            );
void retrieveConfigurationData      (const char * configFilePath, char ** configurationIP, unsigned short int * configurationPort       );
char * getVaccineExpirationDate     (void                                                                                               );
char * getNowDate                   (void                                                                                               );
int createConnectionWithServerV     (const char * configFilePath                                                                        );

#endif /* GreenPassUtility_h */
