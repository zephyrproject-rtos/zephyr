/**************************************************************************//**
 * @file startup_efr32fg1p.c
 * @brief CMSIS Compatible EFR32FG1P startup file in C for IAR EWARM
 * @version 5.1.2
 ******************************************************************************
 * @section License
 * <b>Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com</b>
 ******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Laboratories, Inc.
 * has no obligation to support this Software. Silicon Laboratories, Inc. is
 * providing the Software "AS IS", with no express or implied warranties of any
 * kind, including, but not limited to, any implied warranties of
 * merchantability or fitness for any particular purpose or warranties against
 * infringement of any proprietary rights of a third party.
 *
 * Silicon Laboratories, Inc. will not be liable for any consequential,
 * incidental, or special damages, or any other relief, or for any claim by
 * any third party, arising from your use of this Software.
 *
 *****************************************************************************/

#include "em_device.h"        /* The correct device header file. */

#pragma language=extended
#pragma segment="CSTACK"

/* IAR start function */
extern void __iar_program_start(void);
/* CMSIS init function */
extern void SystemInit(void);

/* Auto defined by linker */
extern unsigned char CSTACK$$Limit;

__weak void Reset_Handler(void)
{
  SystemInit();
  __iar_program_start();
}

__weak void NMI_Handler(void)
{
  while(1);
}

__weak void HardFault_Handler(void)
{
  while(1);
}

__weak void MemManage_Handler(void)
{
  while(1);
}

__weak void BusFault_Handler(void)
{
  while(1);
}

__weak void UsageFault_Handler(void)
{
  while(1);
}

__weak void SVC_Handler(void)
{
  while(1);
}

__weak void DebugMon_Handler(void)
{
  while(1);
}

__weak void PendSV_Handler(void)
{
  while(1);
}

__weak void SysTick_Handler(void)
{
  while(1);
}


__weak void EMU_IRQHandler(void)
{
  while(1);
}

__weak void FRC_PRI_IRQHandler(void)
{
  while(1);
}

__weak void WDOG0_IRQHandler(void)
{
  while(1);
}

__weak void FRC_IRQHandler(void)
{
  while(1);
}

__weak void MODEM_IRQHandler(void)
{
  while(1);
}

__weak void RAC_SEQ_IRQHandler(void)
{
  while(1);
}

__weak void RAC_RSM_IRQHandler(void)
{
  while(1);
}

__weak void BUFC_IRQHandler(void)
{
  while(1);
}

__weak void LDMA_IRQHandler(void)
{
  while(1);
}

__weak void GPIO_EVEN_IRQHandler(void)
{
  while(1);
}

__weak void TIMER0_IRQHandler(void)
{
  while(1);
}

__weak void USART0_RX_IRQHandler(void)
{
  while(1);
}

__weak void USART0_TX_IRQHandler(void)
{
  while(1);
}

__weak void ACMP0_IRQHandler(void)
{
  while(1);
}

__weak void ADC0_IRQHandler(void)
{
  while(1);
}

__weak void IDAC0_IRQHandler(void)
{
  while(1);
}

__weak void I2C0_IRQHandler(void)
{
  while(1);
}

__weak void GPIO_ODD_IRQHandler(void)
{
  while(1);
}

__weak void TIMER1_IRQHandler(void)
{
  while(1);
}

__weak void USART1_RX_IRQHandler(void)
{
  while(1);
}

__weak void USART1_TX_IRQHandler(void)
{
  while(1);
}

__weak void LEUART0_IRQHandler(void)
{
  while(1);
}

__weak void PCNT0_IRQHandler(void)
{
  while(1);
}

__weak void CMU_IRQHandler(void)
{
  while(1);
}

__weak void MSC_IRQHandler(void)
{
  while(1);
}

__weak void CRYPTO_IRQHandler(void)
{
  while(1);
}

__weak void LETIMER0_IRQHandler(void)
{
  while(1);
}

__weak void AGC_IRQHandler(void)
{
  while(1);
}

__weak void PROTIMER_IRQHandler(void)
{
  while(1);
}

__weak void RTCC_IRQHandler(void)
{
  while(1);
}

__weak void SYNTH_IRQHandler(void)
{
  while(1);
}

__weak void CRYOTIMER_IRQHandler(void)
{
  while(1);
}

__weak void RFSENSE_IRQHandler(void)
{
  while(1);
}

__weak void FPUEH_IRQHandler(void)
{
  while(1);
}

/* With IAR, the CSTACK is defined via project options settings */
#pragma data_alignment=256
#pragma location = ".intvec"
const void * const __vector_table[]=  {
  &CSTACK$$Limit,
  (void *) Reset_Handler,           /*  1 - Reset (start instruction) */
  (void *) NMI_Handler,             /*  2 - NMI */
  (void *) HardFault_Handler,       /*  3 - HardFault */
  (void *) MemManage_Handler,
  (void *) BusFault_Handler,
  (void *) UsageFault_Handler,
  (void *) 0,
  (void *) 0,
  (void *) 0,
  (void *) 0,
  (void *) SVC_Handler,
  (void *) DebugMon_Handler,
  (void *) 0,
  (void *) PendSV_Handler,
  (void *) SysTick_Handler,
  (void *) EMU_IRQHandler,  /* 0 - EMU */
  (void *) FRC_PRI_IRQHandler,  /* 1 - FRC_PRI */
  (void *) WDOG0_IRQHandler,  /* 2 - WDOG0 */
  (void *) FRC_IRQHandler,  /* 3 - FRC */
  (void *) MODEM_IRQHandler,  /* 4 - MODEM */
  (void *) RAC_SEQ_IRQHandler,  /* 5 - RAC_SEQ */
  (void *) RAC_RSM_IRQHandler,  /* 6 - RAC_RSM */
  (void *) BUFC_IRQHandler,  /* 7 - BUFC */
  (void *) LDMA_IRQHandler,  /* 8 - LDMA */
  (void *) GPIO_EVEN_IRQHandler,  /* 9 - GPIO_EVEN */
  (void *) TIMER0_IRQHandler,  /* 10 - TIMER0 */
  (void *) USART0_RX_IRQHandler,  /* 11 - USART0_RX */
  (void *) USART0_TX_IRQHandler,  /* 12 - USART0_TX */
  (void *) ACMP0_IRQHandler,  /* 13 - ACMP0 */
  (void *) ADC0_IRQHandler,  /* 14 - ADC0 */
  (void *) IDAC0_IRQHandler,  /* 15 - IDAC0 */
  (void *) I2C0_IRQHandler,  /* 16 - I2C0 */
  (void *) GPIO_ODD_IRQHandler,  /* 17 - GPIO_ODD */
  (void *) TIMER1_IRQHandler,  /* 18 - TIMER1 */
  (void *) USART1_RX_IRQHandler,  /* 19 - USART1_RX */
  (void *) USART1_TX_IRQHandler,  /* 20 - USART1_TX */
  (void *) LEUART0_IRQHandler,  /* 21 - LEUART0 */
  (void *) PCNT0_IRQHandler,  /* 22 - PCNT0 */
  (void *) CMU_IRQHandler,  /* 23 - CMU */
  (void *) MSC_IRQHandler,  /* 24 - MSC */
  (void *) CRYPTO_IRQHandler,  /* 25 - CRYPTO */
  (void *) LETIMER0_IRQHandler,  /* 26 - LETIMER0 */
  (void *) AGC_IRQHandler,  /* 27 - AGC */
  (void *) PROTIMER_IRQHandler,  /* 28 - PROTIMER */
  (void *) RTCC_IRQHandler,  /* 29 - RTCC */
  (void *) SYNTH_IRQHandler,  /* 30 - SYNTH */
  (void *) CRYOTIMER_IRQHandler,  /* 31 - CRYOTIMER */
  (void *) RFSENSE_IRQHandler,  /* 32 - RFSENSE */
  (void *) FPUEH_IRQHandler,  /* 33 - FPUEH */

};
