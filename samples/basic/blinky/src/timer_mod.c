
#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <inttypes.h>
#include <fsl_gpt.h>
#include "timer_mod.h"

/*ISR_DIRECT_DECLARE(timer_handler)
{

    GPT_ClearStatusFlags(SYSTICK_BASE, kGPT_OutputCompare1Flag);


    if (GPT_GetOutputCompareValue(SYSTICK_BASE, kGPT_OutputCompare_Channel1) != 1500000 - 1)
    {

        GPT_SetOutputCompareValue(SYSTICK_BASE, kGPT_OutputCompare_Channel1, 1500000 - 1);
    }

	printk("\tAlhumdulilah timer INT\n");



    __DSB();
    
    return 0;
 }
 
void TimerInterruptSetup(void)
{
	
    gpt_config_t config;
    GPT_GetDefaultConfig(&config);
    config.enableRunInDoze = true;
    config.clockSource     = kGPT_ClockSource_Osc;


    GPT_Init(SYSTICK_BASE, &config);

    GPT_SetOscClockDivider(SYSTICK_BASE, 16);


    GPT_SetOutputCompareValue(SYSTICK_BASE, kGPT_OutputCompare_Channel1, 1500000-1);

    GPT_EnableInterrupts(SYSTICK_BASE, kGPT_OutputCompare1InterruptEnable);




	IRQ_DIRECT_CONNECT(GPT1_IRQn, 7, timer_handler, 0);
	irq_enable(GPT1_IRQn);

    GPT_StartTimer(SYSTICK_BASE);
   
    
    printk("iA setup timer\n");
}*/




