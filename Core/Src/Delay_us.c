#include "Delay_us.h"

#define SYSCLOCK 36000000U //  36 ћгц

// DWT дл€ четких задержек
#define DWT_CYCCNT *(volatile unsigned long *)0xE0001004
#define DWT_CONTROL *(volatile unsigned long *)0xE0001000
#define SCB_DEMCR *(volatile unsigned long *)0xE000EDFC


void Delay_ms(uint32_t ms)     // мс
{
	uint32_t ms_count_tick = ms*(SystemCoreClock/1000);
	SCB_DEMCR |=CoreDebug_DEMCR_TRCENA_Msk;
	DWT_CYCCNT = 0;
	DWT_CONTROL |= DWT_CTRL_CYCCNTENA_Msk;
	while(DWT_CYCCNT < ms_count_tick);
	DWT_CONTROL &= ~DWT_CTRL_CYCCNTENA_Msk;
}

void Delay_us(uint32_t us)     // мкс
{
	uint32_t ms_count_tick = us*(SystemCoreClock/1000000);
	SCB_DEMCR |=CoreDebug_DEMCR_TRCENA_Msk;
	DWT_CYCCNT = 0;
	DWT_CONTROL |= DWT_CTRL_CYCCNTENA_Msk;
	while(DWT_CYCCNT < ms_count_tick);
	DWT_CONTROL &= ~DWT_CTRL_CYCCNTENA_Msk;
}
