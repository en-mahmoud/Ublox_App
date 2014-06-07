#include <p18cxxx.h>
#include <delays.h>
#include <stdio.h>
#include <string.h>
#include "serial.h"
#include "libubx.h"
#include "tick.h"

#pragma config WDT = OFF, OSC = HS, LVP = OFF, PWRT = ON, BOREN = OFF

void ShortMessageHandler(char *ptr);

#if defined(HI_TECH_C)
	void interrupt low_priority LowISR(void)
#else
	#pragma interruptlow LowISR
	void LowISR(void)
#endif
	{
	    // Low priority ISR goes here
	}

#if defined(HI_TECH_C)
	void interrupt HighISR(void)
#else
	#pragma interruptlow HighISR
	void HighISR(void)
#endif
	{
		// High priority ISR goes here
		if(PIR1 & 0x20)
			SerialHandler();
		if(INTCONbits.TMR0IF)
		{
			// Reset interrupt flag
			INTCONbits.TMR0IF = 0;
			TMR0L = 95;
			TickHandler();
		}
	    #if defined(STACK_USE_UART2TCP_BRIDGE)
			UART2TCPBridgeISR();
		#endif

		#if defined(ZG_CS_TRIS)
			zgEintISR();
		#endif // ZG_CS_TRIS
	}
#if !defined(HI_TECH_C)
	#pragma code lowVector=0x18
	void LowVector(void){_asm goto LowISR _endasm}
	#pragma code highVector=0x8
	void HighVector(void){_asm goto HighISR _endasm}
	#pragma code // Return to default code section
#endif


typedef enum
{
	UBX_MODULE_TEST = 0,
	GSM_REGISTRATION_TEST,
	GPRS_REGISTRATION_TEST,
	PDP_CONTEXT_ACTIVATION_TEST,
	SEND_APPLICATION_DATA
}SYSTEM_STATE;

SYSTEM_STATE state = UBX_MODULE_TEST;

void main (void)
{
	unsigned char result;
	// Initialize PORTD
	LATD = 0;
	TRISD = 0;
	// Initialize Timer 0
	T0CON = 0xC6;
	INTCONbits.TMR0IE = 1;
	// Initialize tick module
	TickInit();
	// Initialize serial module
	SerialInit();
	// Enable interrupts
	INTCON |= 0xC0;
/*
	sprintf(command_buf, (const far rom char*)"Still Testing...\r\n");
	SerialWriteString(command_buf);
	SerialWriteRomString("This is another test\r\n");
*/
	// check if the module is present
	while(1)
	{
		if(SendATCmdWaitResp("AT\r", 100, 10, "OK", 1) == AT_RESP_OK)
		{
			// mySerial.println("Modem is Ok");
			break;
		}
		// mySerial.println("Modem is Not responding");
		// wait for one second then try again
		Delay10KTCYx(200);
	}
	// disable echo
	while(1)
	{
		if(SendATCmdWaitResp("ATE0\r", 100, 10, "OK", 1) == AT_RESP_OK)
		{
			// mySerial.println("echo disabled");
			break;
		}
		// mySerial.println("error while disabling echo");
		// wait for one second then try again
		Delay10KTCYx(200);
	}
	// check sim presence
	while(1)
	{
		if(SendATCmdWaitResp("AT+CPIN?\r", 100, 10, "+CPIN: READY", 1) == AT_RESP_OK)
		{
			// mySerial.println("SIM is ok");
			break;
		}
		// mySerial.println("please inset SIM card");
		Delay10KTCYx(200);
	}
	// configure Pdp context(vodafone)
	while(1)
	{
		if(SendATCmdWaitResp("AT+UPSD=0,1,\"internet.vodafone.net\"\r", 100, 10, "OK", 1) == AT_RESP_OK)
		{
			// mySerial.println("APN is ok");
			break;
		}
		// mySerial.println("wait until APN is OK");
		Delay10KTCYx(200);
	}
	while(1)
	{
		if(SendATCmdWaitResp("AT+UPSD=0,2,\"internet\"\r", 100, 10, "OK", 1) == AT_RESP_OK)
		{
			// mySerial.println("username is ok");
			break;
		}
		// mySerial.println("wait until username is OK");
		Delay10KTCYx(200);
	}
	while(1)
	{
		if(SendATCmdWaitResp("AT+UPSD=0,3,\"internet\"\r", 100, 10, "OK", 1) == AT_RESP_OK)
		{
			// mySerial.println("password is ok");
			break;
		}
		// mySerial.println("wait until password is OK");
		Delay10KTCYx(200);
	}
	while(1)
	{
		if(SendATCmdWaitResp("AT+UPSD=0,6,1\r", 100, 10, "OK", 1) == AT_RESP_OK)
		{
			// mySerial.println("PAP is ok");
			break;
		}
		// mySerial.println("wait until PAP is OK");
		Delay10KTCYx(200);
	}
	while(1)
	{
		if(SendATCmdWaitResp("AT+UPSD=0,7,\"0.0.0.0\"\r", 100, 10, "OK", 1) == AT_RESP_OK)
		{
			// mySerial.println("dynamic ip is ok");
			break;
		}
		// mySerial.println("wait until dynamic ip is OK");
		Delay10KTCYx(200);
	}
	// init SMS service
	while(1)
	{
		if(SendATCmdWaitResp("AT+CMGF=1\r", 100, 10, "OK", 1) == AT_RESP_OK)
			break;
		// mySerial.println("wait until CMGF is OK");
		Delay10KTCYx(200);
	}
	while(1)
	{
		if(SendATCmdWaitResp("AT+CPMS=\"SM\",\"SM\",\"SM\"\r", 100, 10, "OK", 1) == AT_RESP_OK)
		{
			// mySerial.println("SMS service is initialized correctly");
			break;
		}
		// mySerial.println("wait until CPMS is OK");
		Delay10KTCYx(200);
	}
	// wait until the module get registered to a network
	while(1)
	{
		if(SendATCmdWaitResp("AT+CREG?\r", 100, 10, "+CREG: 0,1", 1) == AT_RESP_OK)
		{
			// mySerial.println("Ok, the module is registered now");
			break;
		}
		// mySerial.println("please wait until get registered");
		Delay10KTCYx(200);
	}
	// initialize SMS handler
	ubxSetShortMessageHandler(ShortMessageHandler);
/************************************************************************************************
 *									Main Application
 ************************************************************************************************/
	while(1)
	{
/*
		if(SerialDataAvailable())
			SerialWriteByte(SerialReadByte());
*/
		// Main loop activity indicator
		if(TickExpired(1))
		{
			LATDbits.LATD2 ^= 1;
			// repeat every 0.5 seconds
			TickStart(1, 50);
		}
		// Check SMS
		if(TickExpired(2))
		{
			Delay10KTCYx(200);
			result = ubxIsShortMessageReceived();
			if(result == UBX_SMS_S_RECEIVED)
			{
				// mySerial.println("New message received");
				ubxReadShortMessage();
			}
			else if(result == UBX_SMS_S_NOT_RECEIVED)
			{
				// mySerial.println("NO message received");
			}
			// repeat every 10 seconds
			TickStart(2, 1000);
		}
		// Check various conditions and connect to server
		if(TickExpired(3))
		{
			switch(state)
			{
				case UBX_MODULE_TEST:
					Delay10KTCYx(200);
					result = ubxGetModemStatus();
					if(result == UBX_E_OK)
					{
						// mySerial.println("Modem is OK");
					}
					else if (result == UBX_E_TIMEOUT)
					{
						// there is timeout
						// mySerial.println("Modem is not responding");
						break;
					}
					else if(result == UBX_E_ERROR)
					{
						// mySerial.println("Modem responded with ERROR");
						break;
					}
					else
					{
						// mySerial.println("Unknown response");
						break;
					}
				case GSM_REGISTRATION_TEST:
					Delay10KTCYx(200);
					result = ubxGetGsmNetworkStatus();
					if(result == UBX_GSM_S_REGISTERED)
					{
						// mySerial.println("Modem is GSM registered");
					}
					else if (result == UBX_GSM_S_TIMEOUT)
					{
						// there is timeout
						// mySerial.println("Modem is not responding");
						break;
					}
					else if(result == UBX_GSM_S_SEARCHING)
					{
						//the module is searching for network
						// mySerial.println("Modem lost network coverage and is searching for it");
						break;
					}
					else if(result == UBX_GSM_S_NOT_SEARCHING)
					{
						// modem not registered and not searching for a network
						// mySerial.println("Modem is not currently searching a new operator to register to, so force GSM registration");
						// force registration
						ubxRegisterGsmNetwork();
						break;
					}
					else if(result == UBX_GSM_S_DENIED_OR_UNKNOWN)
					{
						// mySerial.println("Registration is denied or unknown");
						break;
					}
					else if(result == UBX_GSM_S_ERROR)
					{
						// mySerial.println("Modem responded with ERROR");
						break;
					}
					else
					{
						// mySerial.println("Unknown response");
						break;
					}
				case GPRS_REGISTRATION_TEST:
					Delay10KTCYx(200);
					result = ubxGetGprsNetworkStatus();
					if(result == UBX_GPRS_S_REGISTERED)
					{
						// mySerial.println("Modem is GPRS registered");
					}
					else if(result == UBX_GPRS_S_TIMEOUT)
					{
						// there is timeout
						// mySerial.println("Modem is not responding");
						break;
					}
					else if(result == UBX_GPRS_S_SEARCHING)
					{
						// the module is searching for network
						// mySerial.println("Modem lost network coverage and is seraching for it");
						break;
					}
					else if(result == UBX_GPRS_S_NOT_SEARCHING)
					{
						// modem not registered and not searching for a network
						// mySerial.println("Modem is not currently searching a new operator to register to, so force GPRS registration");
						ubxRegisterGprsNetwork();
						break;
					}
					else if(result == UBX_GPRS_S_DENIED_OR_UNKNOWN)
					{
						// mySerial.println("Registration is denied or unknown");
						break;
					}
					else if(result == UBX_GPRS_S_ERROR)
					{
						// mySerial.println("Modem responded with ERROR");
						break;
					}
					else
					{
						// mySerial.println("Unknown response");
						break;
					}
				case PDP_CONTEXT_ACTIVATION_TEST:
					Delay10KTCYx(200);
					result = ubxGetPdpServiceStatus();
					if(result == UBX_PDP_S_ACTIVE)
					{
						// mySerial.println("PDP context is active");
					}
					else if(result == UBX_PDP_S_TIMEOUT)
					{
						// there is timeout
						// mySerial.println("Modem is not responding");
						break;			
					}
					else if(result == UBX_PDP_S_NOT_ACTIVE)
					{
						// mySerial.println("PDP context is not active, so activate it");
						Delay10KTCYx(255);
						ubxRegisterPdpService();
						break;
					}
					else if(result == UBX_PDP_S_ERROR)
					{
						// mySerial.println("Modem responded with ERROR");
						break;
					}
					else
					{
						// mySerial.println("Unknown response");
						break;
					}
				case SEND_APPLICATION_DATA:
					Delay10KTCYx(200);
					// mySerial.println("connecting to server...");
			}
			// repeat every 60 seconds
			TickStart(3, 6000);
		}
	}
}
/*
 *		SMS reception callback function
 */
void ShortMessageHandler(char *ptr)
{
	// check message content
	if(!strcmppgm2ram(ptr, (const rom far char *)"*#000A"))
	{
		LATDbits.LATD1 = 1;
	}
	else if(!strcmppgm2ram(ptr, (const rom far char *)"*#000B"))
	{
		LATDbits.LATD1 = 0;
	}
	else if(!strcmppgm2ram(ptr, (const rom far char *)"*#000C"))
	{

	}
	else if(!strcmppgm2ram(ptr, (const rom far char *)"*#000D"))
	{

	}
}
