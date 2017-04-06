/**************************************************************************//**
 * @file startup_efm32wg.c
 * @brief CMSIS Compatible EFM32WG startup file in C for IAR EWARM
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

__weak void ACMP0_IRQHandler(void)
{
  while(1);
}

__weak void ADC0_IRQHandler(void)
{
  while(1);
}

__weak void DAC0_IRQHandler(void)
{
  while(1);
}

__weak void I2C0_IRQHandler(void)
{
  while(1);
}

__weak void I2C1_IRQHandler(void)
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

__weak void TIMER2_IRQHandler(void)
{
  while(1);
}

__weak void TIMER3_IRQHandler(void)
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

__weak void LESENSE_IRQHandler(void)
{
  while(1);
}

__weak void USART2_RX_IRQHandler(void)
{
  while(1);
}

__weak void USART2_TX_IRQHandler(void)
{
  while(1);
}

__weak void UART0_RX_IRQHandler(void)
{
  while(1);
}

__weak void UART0_TX_IRQHandler(void)
{
  while(1);
}

__weak void UART1_RX_IRQHandler(void)
{
  while(1);
}

__weak void UART1_TX_IRQHandler(void)
{
  while(1);
}

__weak void LEUART0_IRQHandler(void)
{
  while(1);
}

__weak void LEUART1_IRQHandler(void)
{
  while(1);
}

__weak void LETIMER0_IRQHandler(void)
{
  while(1);
}

__weak void PCNT0_IRQHandler(void)
{
  while(1);
}

__weak void PCNT1_IRQHandler(void)
{
  while(1);
}

__weak void PCNT2_IRQHandler(void)
{
  while(1);
}

__weak void RTC_IRQHandler(void)
{
  while(1);
}

__weak void BURTC_IRQHandler(void)
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

__weak void LCD_IRQHandler(void)
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

__weak void EBI_IRQHandler(void)
{
  while(1);
}

__weak void EMU_IRQHandler(void)
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

    (void *) DMA_IRQHandler,  /* 0 - DMA */
    (void *) GPIO_EVEN_IRQHandler,  /* 1 - GPIO_EVEN */
    (void *) TIMER0_IRQHandler,  /* 2 - TIMER0 */
    (void *) USART0_RX_IRQHandler,  /* 3 - USART0_RX */
    (void *) USART0_TX_IRQHandler,  /* 4 - USART0_TX */
    (void *) USB_IRQHandler,  /* 5 - USB */
    (void *) ACMP0_IRQHandler,  /* 6 - ACMP0 */
    (void *) ADC0_IRQHandler,  /* 7 - ADC0 */
    (void *) DAC0_IRQHandler,  /* 8 - DAC0 */
    (void *) I2C0_IRQHandler,  /* 9 - I2C0 */
    (void *) I2C1_IRQHandler,  /* 10 - I2C1 */
    (void *) GPIO_ODD_IRQHandler,  /* 11 - GPIO_ODD */
    (void *) TIMER1_IRQHandler,  /* 12 - TIMER1 */
    (void *) TIMER2_IRQHandler,  /* 13 - TIMER2 */
    (void *) TIMER3_IRQHandler,  /* 14 - TIMER3 */
    (void *) USART1_RX_IRQHandler,  /* 15 - USART1_RX */
    (void *) USART1_TX_IRQHandler,  /* 16 - USART1_TX */
    (void *) LESENSE_IRQHandler,  /* 17 - LESENSE */
    (void *) USART2_RX_IRQHandler,  /* 18 - USART2_RX */
    (void *) USART2_TX_IRQHandler,  /* 19 - USART2_TX */
    (void *) UART0_RX_IRQHandler,  /* 20 - UART0_RX */
    (void *) UART0_TX_IRQHandler,  /* 21 - UART0_TX */
    (void *) UART1_RX_IRQHandler,  /* 22 - UART1_RX */
    (void *) UART1_TX_IRQHandler,  /* 23 - UART1_TX */
    (void *) LEUART0_IRQHandler,  /* 24 - LEUART0 */
    (void *) LEUART1_IRQHandler,  /* 25 - LEUART1 */
    (void *) LETIMER0_IRQHandler,  /* 26 - LETIMER0 */
    (void *) PCNT0_IRQHandler,  /* 27 - PCNT0 */
    (void *) PCNT1_IRQHandler,  /* 28 - PCNT1 */
    (void *) PCNT2_IRQHandler,  /* 29 - PCNT2 */
    (void *) RTC_IRQHandler,  /* 30 - RTC */
    (void *) BURTC_IRQHandler,  /* 31 - BURTC */
    (void *) CMU_IRQHandler,  /* 32 - CMU */
    (void *) VCMP_IRQHandler,  /* 33 - VCMP */
    (void *) LCD_IRQHandler,  /* 34 - LCD */
    (void *) MSC_IRQHandler,  /* 35 - MSC */
    (void *) AES_IRQHandler,  /* 36 - AES */
    (void *) EBI_IRQHandler,  /* 37 - EBI */
    (void *) EMU_IRQHandler,  /* 38 - EMU */
    (void *) FPUEH_IRQHandler,  /* 39 - FPUEH */

};
