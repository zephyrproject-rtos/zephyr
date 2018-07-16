/**************************************************************************//**
 * @file startup_efm32hg.c
 * @brief CMSIS Compatible EFM32HG startup file in C for IAR EWARM
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

__weak void SVC_Handler(void)
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

__weak void DMA_IRQHandler(void)
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

__weak void ACMP0_IRQHandler(void)
{
  while(1);
}

__weak void ADC0_IRQHandler(void)
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

__weak void RTC_IRQHandler(void)
{
  while(1);
}

__weak void CMU_IRQHandler(void)
{
  while(1);
}

__weak void VCMP_IRQHandler(void)
{
  while(1);
}

__weak void MSC_IRQHandler(void)
{
  while(1);
}

__weak void AES_IRQHandler(void)
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

__weak void USB_IRQHandler(void)
{
  while(1);
}

__weak void TIMER2_IRQHandler(void)
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
    (void *) 0,
    (void *) 0,
    (void *) 0,
    (void *) 0,
    (void *) 0,
    (void *) 0,
    (void *) 0,
    (void *) SVC_Handler,
    (void *) 0,
    (void *) 0,
    (void *) PendSV_Handler,
    (void *) SysTick_Handler,

    (void *) DMA_IRQHandler,  /* 0 - DMA */
    (void *) GPIO_EVEN_IRQHandler,  /* 1 - GPIO_EVEN */
    (void *) TIMER0_IRQHandler,  /* 2 - TIMER0 */
    (void *) ACMP0_IRQHandler,  /* 3 - ACMP0 */
    (void *) ADC0_IRQHandler,  /* 4 - ADC0 */
    (void *) I2C0_IRQHandler,  /* 5 - I2C0 */
    (void *) GPIO_ODD_IRQHandler,  /* 6 - GPIO_ODD */
    (void *) TIMER1_IRQHandler,  /* 7 - TIMER1 */
    (void *) USART1_RX_IRQHandler,  /* 8 - USART1_RX */
    (void *) USART1_TX_IRQHandler,  /* 9 - USART1_TX */
    (void *) LEUART0_IRQHandler,  /* 10 - LEUART0 */
    (void *) PCNT0_IRQHandler,  /* 11 - PCNT0 */
    (void *) RTC_IRQHandler,  /* 12 - RTC */
    (void *) CMU_IRQHandler,  /* 13 - CMU */
    (void *) VCMP_IRQHandler,  /* 14 - VCMP */
    (void *) MSC_IRQHandler,  /* 15 - MSC */
    (void *) AES_IRQHandler,  /* 16 - AES */
    (void *) USART0_RX_IRQHandler,  /* 17 - USART0_RX */
    (void *) USART0_TX_IRQHandler,  /* 18 - USART0_TX */
    (void *) USB_IRQHandler,  /* 19 - USB */
    (void *) TIMER2_IRQHandler,  /* 20 - TIMER2 */

};
