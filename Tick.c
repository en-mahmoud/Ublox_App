#include <p18cxxx.h>

#define MAX_SYS_TIMERS			5
#define ENTER_CRITICAL()   	 	INTCONbits.GIE = 0
#define EXIT_CRITICAL()   	 	INTCONbits.GIE = 1

typedef struct tmr_t
{
	unsigned char	TmrEn;
	unsigned int	TmrCtr;
}TMR_t, *TMR_p_t;

static TMR_t TmrTbl[MAX_SYS_TIMERS];

//----------------------------------------------------------------------
void TickInit(void)
{
	unsigned char i = 0;
	TMR_t *ptr = TmrTbl;
	// Initialize timers structures
	for( ; i < MAX_SYS_TIMERS; i++)
	{
		ptr->TmrEn = 0;
		ptr->TmrCtr = 0;
		ptr++;
	}
}
//----------------------------------------------------------------------
unsigned char TickExpired(unsigned char id)
{
	unsigned char t;
	ENTER_CRITICAL();
	t = !(TmrTbl[id].TmrEn);
	EXIT_CRITICAL();
	return t;
}
//----------------------------------------------------------------------
void TickStart(unsigned char id, unsigned int cnt)
{
	ENTER_CRITICAL();
	TmrTbl[id].TmrEn = 1;
	TmrTbl[id].TmrCtr = cnt;
	EXIT_CRITICAL();
}
//----------------------------------------------------------------------
void TickHandler(void)
{
	unsigned char i = 0;
	TMR_t *ptr = TmrTbl;

	// scan the array and update timers
	for( ; i < MAX_SYS_TIMERS; i++)
	{
		if(ptr->TmrEn)
		{
			ptr->TmrCtr--;
			if(ptr->TmrCtr == 0)
			{
				// Stop timer
				ptr->TmrEn = 0;
			}
		}
		ptr++;
	}
}
//----------------------------------------------------------------------
/*
	if(TickGet() - t >= TICK_SECOND/2ul)
	{
		t = TickGet();
		LED0_IO ^= 1;
	}
	
	if(!TmrChk(0))
	{
		LED0_IO ^= 1;
		TMR_Reset(0);
		TMR_Start(0);
	}
*/