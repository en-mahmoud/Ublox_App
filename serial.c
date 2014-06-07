#include <p18cxxx.h>
#include <delays.h>

#define SERIAL_BUFFER_SIZE 		64
#define ENTER_CRITICAL()   	 	INTCONbits.GIE = 0
#define EXIT_CRITICAL()   	 	INTCONbits.GIE = 1

struct ring_buffer
{
	unsigned char buffer[SERIAL_BUFFER_SIZE];
	unsigned char head;
	unsigned char tail;
	unsigned char cnt;
};

static volatile struct ring_buffer rx_buffer = {{0}, 0, 0, 0};

void SerialInit(void)
{
	// set TRISC 6, 7 bits
	TRISC |= (1 << 7) + (1 << 6);
	// set TXEN, BRGH bits
	TXSTA |= (1 << 5) + (1 << 2);
	// set SPEN, CREN bits
	RCSTA |= (1 << 7) + (1 << 4);
	// set baud rate to 9600 @ 8MHz
	SPBRG = 51;
	// enable receive interrupt
	PIE1bits.RCIE = 1;
}
//----------------------------------------------------------------------
void SerialWriteByte(char c)
{
	TXREG = c;
	// delay to make sure TMRT goes down
	Delay10TCYx(1);
	// loop until TMRT returns high
	while(!(TXSTA & 0x02));
}
//----------------------------------------------------------------------
void SerialWriteString(const char *s)
{
    //send command string
    while(*s)
		SerialWriteByte(*s++);
}
//----------------------------------------------------------------------
void SerialWriteRomString(const rom char *s)
{
    //send command string
    while(*s)
		SerialWriteByte(*s++);
}
//----------------------------------------------------------------------
unsigned char SerialReadByte(void)
{
	unsigned char data;
	ENTER_CRITICAL();
	// check to see if the queue is empty
	if(rx_buffer.cnt > 0)
	{
		data = rx_buffer.buffer[rx_buffer.head++];
		if(rx_buffer.head == SERIAL_BUFFER_SIZE)
		{
			rx_buffer.head = 0;
		}
		rx_buffer.cnt--;
	}
	else
		data = 0xff;
	EXIT_CRITICAL();
	return data;
}
//----------------------------------------------------------------------
void SerialHandler(void)
{
	// check to see if the queue is full
	if(rx_buffer.cnt < SERIAL_BUFFER_SIZE)
	{
		rx_buffer.buffer[rx_buffer.tail++] = RCREG;
		if(rx_buffer.tail == SERIAL_BUFFER_SIZE)
		{
			rx_buffer.tail = 0;
		}
		// increment queue elements
		rx_buffer.cnt++;
	}
}
//----------------------------------------------------------------------
unsigned char SerialDataAvailable(void)
{
	unsigned char data;
	ENTER_CRITICAL();
	data = rx_buffer.cnt;
	EXIT_CRITICAL();
	return data;
}
//----------------------------------------------------------------------
