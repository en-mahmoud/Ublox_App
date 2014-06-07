/*
 GSM.h - library for the GSM Playground - GSM Shield for Arduino
 Released under the Creative Commons Attribution-Share Alike 3.0 License
 http://www.creativecommons.org/licenses/by-sa/3.0/
 www.hwkitchen.com
*/
#ifndef __LIBUBX_H__
#define __LIBUBX_H__


typedef enum {
    UBX_E_OK = 0,                       /**< Success */
    UBX_E_ERROR,                        /**< Error, handling required */
    UBX_E_UNKNOWN_RESPONSE,                      /**< Warning, can be ignored */
	UBX_E_TIMEOUT
} UBX_ERROR;

typedef enum {
    UBX_GSM_S_NOT_SEARCHING = 0,                       /**< Success */
    UBX_GSM_S_REGISTERED,                        /**< Error, handling required */
    UBX_GSM_S_SEARCHING,                      /**< Warning, can be ignored */
	UBX_GSM_S_DENIED_OR_UNKNOWN,
	UBX_GSM_S_ERROR,
	UBX_GSM_S_UNKNOWN_RESPONSE,
	UBX_GSM_S_TIMEOUT
} UBX_GSM_STATUS;

typedef enum {
    UBX_GPRS_S_NOT_SEARCHING = 0,                       /**< Success */
    UBX_GPRS_S_REGISTERED,                        /**< Error, handling required */
    UBX_GPRS_S_SEARCHING,                      /**< Warning, can be ignored */
	UBX_GPRS_S_DENIED_OR_UNKNOWN,
	UBX_GPRS_S_ERROR,
	UBX_GPRS_S_UNKNOWN_RESPONSE,
	UBX_GPRS_S_TIMEOUT
} UBX_GPRS_STATUS;

typedef enum {
    UBX_PDP_S_ACTIVE = 0,                       /**< Success */
    UBX_PDP_S_NOT_ACTIVE,                        /**< Error, handling required */
    UBX_PDP_S_ERROR,                      /**< Warning, can be ignored */
	UBX_PDP_S_UNKNOWN_RESPONSE,
	UBX_PDP_S_TIMEOUT
} UBX_PDP_STATUS;

typedef enum {
    UBX_SMS_S_RECEIVED = 0,                       /**< Success */
    UBX_SMS_S_NOT_RECEIVED,                        /**< Error, handling required */
	UBX_SMS_S_FULL,
	UBX_SMS_S_NOT_FULL,
    UBX_SMS_S_ERROR,                      /**< Warning, can be ignored */
	UBX_SMS_S_UNKNOWN_RESPONSE,
	UBX_SMS_S_TIMEOUT
} UBX_SMS_STATUS;

enum at_resp_enum {
	AT_RESP_ERR_NO_RESP = -1,   // nothing received
	AT_RESP_ERR_DIF_RESP = 0,   // response_string is different from the response
	AT_RESP_OK = 1              // response_string was included in the response
};


char SendATCmdWaitResp (const rom char *AT_cmd_string,
						unsigned int start_comm_tmout,
						unsigned int max_interchar_tmout,
						const rom char *response_string,
						unsigned char no_of_attempts);

UBX_ERROR ubxGetModemStatus(void);
						
UBX_GSM_STATUS ubxGetGsmNetworkStatus(void);

UBX_ERROR ubxRegisterGsmNetwork(void);
	
UBX_GPRS_STATUS ubxGetGprsNetworkStatus(void);
	
UBX_ERROR ubxRegisterGprsNetwork(void);
	
UBX_PDP_STATUS ubxGetPdpServiceStatus(void);
	
UBX_ERROR ubxRegisterPdpService(void);
	
void ubxConfigurePdp(const char *apn);

unsigned char ubxInitShortMessageService(void);

UBX_SMS_STATUS ubxIsShortMessageReceived(void);

void ubxReadShortMessage(void);
	
UBX_ERROR ubxDeleteGsmShortMessage(int position) ;

void ubxSetShortMessageHandler(void(*p)(char*));

#endif
