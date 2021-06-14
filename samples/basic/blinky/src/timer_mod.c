
#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <inttypes.h>
#include <fsl_gpt.h>
#include "timer_mod.h"

void TimerInterruptSetup(void)
{
    gpt_config_t config;
    GPT_GetDefaultConfig(&config);
    config.enableRunInDoze = true;
    config.clockSource     = kGPT_ClockSource_Osc;

    /* Initialize timer */
    GPT_Init(SYSTICK_BASE, &config);
    /* Divide GPT clock source frequency to 24/16 */
    GPT_SetOscClockDivider(SYSTICK_BASE, 16);

    /* Set timer period. 1 sec */
    GPT_SetOutputCompareValue(SYSTICK_BASE, kGPT_OutputCompare_Channel1, 1500000-1);
    /* Enable timer interrupt */
    GPT_EnableInterrupts(SYSTICK_BASE, kGPT_OutputCompare1InterruptEnable);

    NVIC_SetPriority(SYSTICK_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);
    /* Enable NVIC interrupt */
    NVIC_EnableIRQ(SYSTICK_IRQn);
    /* Start counting */
    GPT_StartTimer(SYSTICK_BASE);
    
    printk("iA setup timer\n");
}

/* The systick interrupt handler. */
void SYSTICK_HANDLER(void)
{
    /* Clear interrupt flag.*/
    GPT_ClearStatusFlags(SYSTICK_BASE, kGPT_OutputCompare1Flag);

    /* This is the first tick since the MCU left a low power mode the
     * compare value need to be reset to its default.
     */
    if (GPT_GetOutputCompareValue(SYSTICK_BASE, kGPT_OutputCompare_Channel1) != 1500000 - 1)
    {
        /* Counter will be reset and cause minor accuracy loss */
        GPT_SetOutputCompareValue(SYSTICK_BASE, kGPT_OutputCompare_Channel1, 1500000 - 1);
    }

	printk("iA timer INT\n");

    /* Add for ARM errata 838869, affects Cortex-M7, Cortex-M7F Store immediate overlapping
    exception return operation might vector to incorrect interrupt */
    __DSB();
}
