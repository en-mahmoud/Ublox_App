#include <string.h>
#include <delays.h>
#include <stdio.h>
#include "libubx.h"
#include "serial.h"
#include "tick.h"


// some constants for the IsRxFinished() method
#define RX_NOT_STARTED      	0
#define RX_ALREADY_STARTED		1
// length for the internal communication buffer
#define COMM_BUF_LEN        	200

enum rx_state_enum 
{
	RX_NOT_FINISHED = 0,      // not finished yet
	RX_FINISHED,              // finished, some character was received
	RX_FINISHED_STR_RECV,     // finished and expected string received
	RX_FINISHED_STR_NOT_RECV, // finished, but expected string not received
	RX_TMOUT_ERR
};


unsigned char rx_state;             		// internal state of rx state machine
unsigned int start_reception_tmout; 		// max tmout for starting reception
unsigned int interchar_tmout;       		// previous time in msec.
char comm_buf[COMM_BUF_LEN+1];  			// communication buffer +1 for 0x00 termination
char *p_comm_buf;               			// pointer to the communication buffer
unsigned char comm_buf_len;              	// num. of characters in the buffer
static void (*sms_handler_ptr)(char *);		// sms handler pointer
//------------------------------------------------------------------------------
static void RxInit(unsigned int start_comm_tmout, unsigned int max_interchar_tmout)
{
	rx_state = RX_NOT_STARTED;
	start_reception_tmout = start_comm_tmout;
	interchar_tmout = max_interchar_tmout;
	TickStart(0, start_reception_tmout);
	comm_buf[0] = 0x00; // end of string
	p_comm_buf = comm_buf;
	comm_buf_len = 0;
	//mySerial.flush(); // erase rx circular buffer
	while(SerialDataAvailable())
		SerialReadByte();
}
//------------------------------------------------------------------------------
static unsigned char IsRxFinished(void)
{
	unsigned char num_of_bytes;
	unsigned char ret_val = RX_NOT_FINISHED;  // default not finished

	// Rx state machine
	// ----------------

	if (rx_state == RX_NOT_STARTED)
	{
		// Reception is not started yet - check tmout
		if (!SerialDataAvailable())
		{
			// still no character received => check timeout
			/*  
			#ifdef DEBUG_GSMRX

			DebugPrint("\r\nDEBUG: reception timeout", 0);			
			Serial.print((unsigned long)(millis() - prev_time));	
			DebugPrint("\r\nDEBUG: start_reception_tmout\r\n", 0);			
			Serial.print(start_reception_tmout);	


			#endif
			*/
			if (TickExpired(0))
			{
				// timeout elapsed => GSM module didn't start with response
				// so communication is takes as finished
				/*
				#ifdef DEBUG_GSMRX		
				DebugPrint("\r\nDEBUG: RECEPTION TIMEOUT", 0);	
				#endif
				*/
				comm_buf[comm_buf_len] = 0x00;
				ret_val = RX_TMOUT_ERR;
			}
		}
		else
		{
			// at least one character received => so init inter-character 
			// counting process again and go to the next state
			TickStart(0, interchar_tmout); // init tmout for inter-character space
			rx_state = RX_ALREADY_STARTED;
		}
	}

	if (rx_state == RX_ALREADY_STARTED)
	{
		// Reception already started
		// check new received bytes
		// only in case we have place in the buffer
		num_of_bytes = SerialDataAvailable();
		// if there are some received bytes postpone the timeout
		if (num_of_bytes)
			TickStart(0, interchar_tmout);

		// read all received bytes      
		while (num_of_bytes)
		{
			num_of_bytes--;
			if (comm_buf_len < COMM_BUF_LEN)
			{
				// we have still place in the GSM internal comm. buffer =>
				// move available bytes from circular buffer 
				// to the rx buffer
				*p_comm_buf = SerialReadByte();

				p_comm_buf++;
				comm_buf_len++;
				comm_buf[comm_buf_len] = 0x00;  // and finish currently received characters
				// so after each character we have
				// valid string finished by the 0x00
			}
			else
			{
				// comm buffer is full, other incoming characters
				// will be discarded 
				// but despite of we have no place for other characters 
				// we still must to wait until  
				// inter-character tmout is reached

				// so just readout character from circular RS232 buffer 
				// to find out when communication id finished(no more characters
				// are received in inter-char timeout)
				SerialReadByte();
			}
		}

		// finally check the inter-character timeout 
		/*
		#ifdef DEBUG_GSMRX

		DebugPrint("\r\nDEBUG: intercharacter", 0);			
		Serial.print((unsigned long)(millis() - prev_time));	
		DebugPrint("\r\nDEBUG: interchar_tmout\r\n", 0);			
		Serial.print(interchar_tmout);	


		#endif
		*/
		if (TickExpired(0))
		{
			// timeout between received character was reached
			// reception is finished
			// ---------------------------------------------

			/*
			#ifdef DEBUG_GSMRX

			DebugPrint("\r\nDEBUG: OVER INTER TIMEOUT", 0);					
			#endif
			*/
			comm_buf[comm_buf_len] = 0x00;  // for sure finish string again
			// but it is not necessary
			ret_val = RX_FINISHED;
		}
	}
	
  	#ifdef DEBUG_GSMRX
	if (ret_val == RX_FINISHED)
	{
		DebugPrint("DEBUG: Received string\r\n", 0);
		for (int i=0; i<comm_buf_len; i++)
		{
			Serial.print(byte(comm_buf[i]));	
		}
	}
	#endif
	
	return (ret_val);
}
//------------------------------------------------------------------------------
unsigned char IsStringReceived(const rom char *compare_string)
{
	char *ch;
	unsigned char ret_val = 0;

	if(comm_buf_len)
	{
		/*
		#ifdef DEBUG_GSMRX
		DebugPrint("DEBUG: Compare the string: \r\n", 0);
		for (int i=0; i<comm_buf_len; i++){
		Serial.print(byte(comm_buf[i]));	
		}

		DebugPrint("\r\nDEBUG: with the string: \r\n", 0);
		Serial.print(compare_string);	
		DebugPrint("\r\n", 0);
		#endif
		*/
		ch = strstrrampgm(comm_buf, (const rom far char *)compare_string);
		if (ch != NULL)
		{
			ret_val = 1;
			/*#ifdef DEBUG_PRINT
			DebugPrint("\r\nDEBUG: expected string was received\r\n", 0);
			#endif
			*/
		}
		else
		{
			/*#ifdef DEBUG_PRINT
			DebugPrint("\r\nDEBUG: expected string was NOT received\r\n", 0);
			#endif
			*/
		}
	}

	return (ret_val);
}
//------------------------------------------------------------------------------
unsigned char WaitResp(unsigned int start_comm_tmout, unsigned int max_interchar_tmout)
{
	unsigned char status;

	RxInit(start_comm_tmout, max_interchar_tmout); 
	// wait until response is not finished
	do
	{
		status = IsRxFinished();
	}while(status == RX_NOT_FINISHED);
	return(status);
}
//------------------------------------------------------------------------------
char SendATCmdWaitResp (const rom char *AT_cmd_string,
						unsigned int start_comm_tmout,
						unsigned int max_interchar_tmout,
						const rom char *response_string,
						unsigned char no_of_attempts)
{
	unsigned char status;
	char ret_val = AT_RESP_ERR_NO_RESP;
	unsigned char i;

	for (i = 0; i < no_of_attempts; i++)
	{
		// delay 500 msec. before sending next repeated AT command 
		// so if we have no_of_attempts=1 tmout will not occurred
		if (i > 0)
			Delay10KTCYx(100);

		SerialWriteRomString(AT_cmd_string);
		//wait for response
		status = WaitResp(start_comm_tmout, max_interchar_tmout);
		if (status == RX_FINISHED)
		{
			// something was received but what was received?
			// ---------------------------------------------
			if(IsStringReceived(response_string))
			{
				ret_val = AT_RESP_OK;      
				break;  // response is OK => finish
			}
			else
				ret_val = AT_RESP_ERR_DIF_RESP;
		}
		else
		{
			// nothing was received
			// --------------------
			ret_val = AT_RESP_ERR_NO_RESP;
		}
	}
	return (ret_val);
}
//------------------------------------------------------------------------------
UBX_ERROR ubxGetModemStatus(void)
{
	UBX_ERROR ret_val;

	// send AT command
	SerialWriteRomString("AT\r");
	// wait response
	if (RX_TMOUT_ERR == WaitResp(500, 5))
		ret_val = UBX_E_TIMEOUT;
	// check response
	else
	{
		if(IsStringReceived("OK"))
			ret_val = UBX_E_OK;
		else if(IsStringReceived("ERROR"))
			ret_val = UBX_E_ERROR;
		else
			ret_val = UBX_E_UNKNOWN_RESPONSE;
	}

	return ret_val;
}
//------------------------------------------------------------------------------
UBX_GSM_STATUS ubxGetGsmNetworkStatus(void)
{
	UBX_GSM_STATUS ret_val;

	// send AT+CREG? command
	SerialWriteRomString("AT+CREG?\r");
	// wait response
	if (RX_TMOUT_ERR == WaitResp(500, 5))
		ret_val = UBX_GSM_S_TIMEOUT;
	// check response
	else
	{
		if(IsStringReceived("+CREG: 0,0"))
			ret_val = UBX_GSM_S_NOT_SEARCHING;
		else if(IsStringReceived("+CREG: 0,1"))
			ret_val = UBX_GSM_S_REGISTERED;
		else if(IsStringReceived("+CREG: 0,2"))
			ret_val = UBX_GSM_S_SEARCHING;
		else if(IsStringReceived("+CREG: 0,3") || IsStringReceived("+CREG: 0,4"))
			ret_val = UBX_GSM_S_DENIED_OR_UNKNOWN;
		else if(IsStringReceived("ERROR"))
			ret_val = UBX_GSM_S_ERROR;
		else
			ret_val = UBX_GSM_S_UNKNOWN_RESPONSE;
	}

	return ret_val;
}
//------------------------------------------------------------------------------
UBX_ERROR ubxRegisterGsmNetwork(void)
{
	UBX_ERROR ret_val;

	// send AT+COPS=0 command
	SerialWriteRomString("AT+COPS=0\r");
	// up to 1min timeout in case of registration!
	// it shoud be increased to 3min but this is the max of uint.
	if (RX_TMOUT_ERR == WaitResp(6000, 5))
		ret_val = UBX_E_TIMEOUT;
	// check response
	else
	{
		if(IsStringReceived("OK"))
			ret_val = UBX_E_OK;
		else if(IsStringReceived("ERROR"))
			ret_val = UBX_E_ERROR;
		else
			ret_val = UBX_E_UNKNOWN_RESPONSE;
	}

	return ret_val;
}
//------------------------------------------------------------------------------
UBX_GPRS_STATUS ubxGetGprsNetworkStatus(void)
{
	UBX_GPRS_STATUS ret_val;

	// send AT+CGREG? command
	SerialWriteRomString("AT+CGREG?\r");
	// wait response
	if (RX_TMOUT_ERR == WaitResp(500, 5))
		ret_val = UBX_GPRS_S_TIMEOUT;
	// check response
	else
	{
		if(IsStringReceived("+CGREG: 0,0"))
			ret_val = UBX_GPRS_S_NOT_SEARCHING;
		else if(IsStringReceived("+CGREG: 0,1"))
			ret_val = UBX_GPRS_S_REGISTERED;
		else if(IsStringReceived("+CGREG: 0,2"))
			ret_val = UBX_GPRS_S_SEARCHING;
		else if(IsStringReceived("+CGREG: 0,3") || IsStringReceived("+CGREG: 0,4"))
			ret_val = UBX_GPRS_S_DENIED_OR_UNKNOWN;
		else if(IsStringReceived("ERROR"))
			ret_val = UBX_GPRS_S_ERROR;
		else
			ret_val = UBX_GPRS_S_UNKNOWN_RESPONSE;
	}

	return ret_val;
}
//------------------------------------------------------------------------------
UBX_ERROR ubxRegisterGprsNetwork(void)
{
	UBX_ERROR ret_val;

	// send AT+CGATT=1 command
	SerialWriteRomString("AT+CGATT=1\r");
	// up to 1min timeout in case of registration!
	// it shoud be increased to 3min but this is the max of uint.
	if (RX_TMOUT_ERR == WaitResp(6000, 5))
		ret_val = UBX_E_TIMEOUT;
	// check response
	else
	{
		if(IsStringReceived("OK"))
			ret_val = UBX_E_OK;
		else if(IsStringReceived("ERROR"))
			ret_val = UBX_E_ERROR;
		else
			ret_val = UBX_E_UNKNOWN_RESPONSE;
	}

	return ret_val;
}
//------------------------------------------------------------------------------
UBX_PDP_STATUS ubxGetPdpServiceStatus(void)
{
	UBX_PDP_STATUS ret_val;

	// send AT+UPSND=0,8 command
	SerialWriteRomString("AT+UPSND=0,8\r");
	// wait response
	if (RX_TMOUT_ERR == WaitResp(500, 5))
		ret_val = UBX_PDP_S_TIMEOUT;
	// check response
	else
	{
		if(IsStringReceived("+UPSND: 0,8,1"))
			ret_val = UBX_PDP_S_ACTIVE;
		else if(IsStringReceived("+UPSND: 0,8,0"))
			ret_val = UBX_PDP_S_NOT_ACTIVE;
		else if(IsStringReceived("ERROR"))
			ret_val = UBX_PDP_S_ERROR;
		else
			ret_val = UBX_PDP_S_UNKNOWN_RESPONSE;
	}

	return ret_val;
}
//------------------------------------------------------------------------------
UBX_ERROR ubxRegisterPdpService(void)
{
	UBX_ERROR ret_val;

	// send AT+UPSDA=0,3 command
	SerialWriteRomString("AT+UPSDA=0,3\r");
	// up to 1min timeout in case of registration!
	// it shoud be increased to 3min but this is the max of uint.
	if (RX_TMOUT_ERR == WaitResp(6000, 5))
		ret_val = UBX_E_TIMEOUT;
	// check response
	else
	{
		if(IsStringReceived("OK"))
			ret_val = UBX_E_OK;
		else if(IsStringReceived("ERROR"))
			ret_val = UBX_E_ERROR;
		else
			ret_val = UBX_E_UNKNOWN_RESPONSE;
	}

	return ret_val;
}
//------------------------------------------------------------------------------
void ubxConfigurePdp(const char *apn)
{
/*
	// set APN
	sprintf(tx_buff, "AT+UPSD=0,1,\"%s\"", apn);
	Serial.print("AT+UPSD=0,1,\"");
	Serial.print(apn);
	Serial.println("\"");
	SendATCmdWaitResp(tx_buff, 10000, 100, "OK", 1);
	// set dynamic ip
	Serial.println("AT+UPSD=0,7,\"0.0.0.0\"");
	SendATCmdWaitResp(tx_buff, 10000, 100, "OK", 1);
*/
}
//------------------------------------------------------------------------------
/*
UBX_ERROR ubxDeleteGsmShortMessage(int position) 
{
	UBX_ERROR ret_val;

	// send AT+CMGD command
	Serial.print("AT+CMGD=");
	Serial.print((int)position);  
	Serial.println("");
	// wait for response
	if (RX_TMOUT_ERR == WaitResp(5000, 50))
		ret_val = UBX_E_TIMEOUT;
	// check response
	else
	{
		if(IsStringReceived("OK"))
			ret_val = UBX_E_OK;
		else if(IsStringReceived("ERROR"))
			ret_val = UBX_E_ERROR;
		else
			ret_val = UBX_E_UNKNOWN_RESPONSE;
	}

	return ret_val;
}
*/
//------------------------------------------------------------------------------
/*
unsigned char ubxInitShortMessageService(void)
{
	// enable text mode
	if(SendATCmdWaitResp("AT+CMGF=1", 1000, 100, "OK", 5) != AT_RESP_OK)
		return 0;
	// set storage area
	if(SendATCmdWaitResp("AT+CPMS=\"SM\",\"SM\",\"SM\"", 1000, 100, "+CPMS:", 5) != AT_RESP_OK)
		return 0;
	return 1;
}
*/
//------------------------------------------------------------------------------
UBX_SMS_STATUS ubxIsShortMessageReceived(void)
{
	UBX_SMS_STATUS ret_val;
	char *ptr, i, descr = 1;

	// send AT+CIND? command
	SerialWriteRomString("AT+CIND?\r");
	// wait for response
	if (RX_TMOUT_ERR == WaitResp(500, 5))
		ret_val = UBX_SMS_S_TIMEOUT;
	// check response
	else
	{
		if(IsStringReceived("+CIND"))
		{
			ptr = strstrrampgm(comm_buf, (const rom far char *)"+CIND");
			for(i = 0; i <28; i++)
			{
				if(ptr[i] == ',')
				{
					descr++;
					continue;
				}
				if(descr == 5)
				{
					if(ptr[i] == '1')
						ret_val = UBX_SMS_S_RECEIVED;
					else if(ptr[i] == '0')
						ret_val = UBX_SMS_S_NOT_RECEIVED;
					break;
				}
			}
		}
		else if(IsStringReceived("ERROR"))
			ret_val = UBX_SMS_S_ERROR;
		else
			ret_val = UBX_SMS_S_UNKNOWN_RESPONSE;
	}

	return ret_val;
}
//------------------------------------------------------------------------------
UBX_SMS_STATUS ubxIsShortMessageStorageFull(void)
{
	UBX_SMS_STATUS ret_val;
	char *ptr, i, descr = 1;

	// send AT+CIND? command
	SerialWriteRomString("AT+CIND?\r");
	// wait for response
	if (RX_TMOUT_ERR == WaitResp(5000, 50))
		ret_val = UBX_SMS_S_TIMEOUT;
	// check response
	else
	{
		if(IsStringReceived("+CIND"))
		{
			ptr = strstrrampgm(comm_buf, (const rom far char *)"+CIND");
			for(i = 0; i <28; i++)
			{
				if(ptr[i] == ',')
				{
					descr++;
					continue;
				}
				if(descr == 8)
				{
					if(ptr[i] == '1')
						ret_val = UBX_SMS_S_FULL;
					else if(ptr[i] == '0')
						ret_val = UBX_SMS_S_NOT_FULL;
					break;
				}
			}
		}
		else if(IsStringReceived("ERROR"))
			ret_val = UBX_SMS_S_ERROR;
		else
			ret_val = UBX_SMS_S_UNKNOWN_RESPONSE;
	}

	return ret_val;
}
//------------------------------------------------------------------------------
void ubxReadShortMessage(void)
{
	unsigned int sms_no = 301;
	char *ptr, buf[20];
	do
	{
		if(sms_no > 330)
			break;
		sprintf(buf, (const far rom char*)"AT+CMGR=%d\r", sms_no);
		SerialWriteString(buf);
		// wait for response
		if (RX_TMOUT_ERR == WaitResp(500, 5))
			return;
		// check response
		if(IsStringReceived("+CMGR"))
		{
			ptr = strstrrampgm(comm_buf, (const rom far char *)"+CMGR");
			ptr = strchr(ptr, '\n') + 1;
			*(ptr + 6) = 0;
			(*sms_handler_ptr)(ptr);
		}
		sms_no++;
		Delay10KTCYx(100);
	}while(ubxIsShortMessageReceived() == UBX_SMS_S_RECEIVED);
	// delete all read messages
	SerialWriteRomString("AT+CMGD=1,1\r");
	// wait response
	WaitResp(500, 5);
}
//------------------------------------------------------------------------------
void ubxSetShortMessageHandler(void(*p)(char*))
{
	sms_handler_ptr = p;
}
//------------------------------------------------------------------------------
