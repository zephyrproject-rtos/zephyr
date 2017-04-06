/***************************************************************************//**
 * @file system_efm32wg.h
 * @brief CMSIS Cortex-M4 System Layer for EFM32WG devices.
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

#ifndef SYSTEM_EFM32WG_H
#define SYSTEM_EFM32WG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*******************************************************************************
 **************************   GLOBAL VARIABLES   *******************************
 ******************************************************************************/

extern uint32_t SystemCoreClock;    /**< System Clock Frequency (Core Clock) */

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/* Interrupt routines - prototypes */
void Reset_Handler(void);
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

void DMA_IRQHandler(void);
void GPIO_EVEN_IRQHandler(void);
void TIMER0_IRQHandler(void);
void USART0_RX_IRQHandler(void);
void USART0_TX_IRQHandler(void);
void USB_IRQHandler(void);
void ACMP0_IRQHandler(void);
void ADC0_IRQHandler(void);
void DAC0_IRQHandler(void);
void I2C0_IRQHandler(void);
void I2C1_IRQHandler(void);
void GPIO_ODD_IRQHandler(void);
void TIMER1_IRQHandler(void);
void TIMER2_IRQHandler(void);
void TIMER3_IRQHandler(void);
void USART1_RX_IRQHandler(void);
void USART1_TX_IRQHandler(void);
void LESENSE_IRQHandler(void);
void USART2_RX_IRQHandler(void);
void USART2_TX_IRQHandler(void);
void UART0_RX_IRQHandler(void);
void UART0_TX_IRQHandler(void);
void UART1_RX_IRQHandler(void);
void UART1_TX_IRQHandler(void);
void LEUART0_IRQHandler(void);
void LEUART1_IRQHandler(void);
void LETIMER0_IRQHandler(void);
void PCNT0_IRQHandler(void);
void PCNT1_IRQHandler(void);
void PCNT2_IRQHandler(void);
void RTC_IRQHandler(void);
void BURTC_IRQHandler(void);
void CMU_IRQHandler(void);
void VCMP_IRQHandler(void);
void LCD_IRQHandler(void);
void MSC_IRQHandler(void);
void AES_IRQHandler(void);
void EBI_IRQHandler(void);
void EMU_IRQHandler(void);
void FPUEH_IRQHandler(void);

uint32_t SystemCoreClockGet(void);
uint32_t SystemMaxCoreClockGet(void);

/**************************************************************************//**
 * @brief
 *   Update CMSIS SystemCoreClock variable.
 *
 * @details
 *   CMSIS defines a global variable SystemCoreClock that shall hold the
 *   core frequency in Hz. If the core frequency is dynamically changed, the
 *   variable must be kept updated in order to be CMSIS compliant.
 *
 *   Notice that if only changing core clock frequency through the EFM32 CMU
 *   API, this variable will be kept updated. This function is only provided
 *   for CMSIS compliance and if a user modifies the the core clock outside
 *   the CMU API.
 *****************************************************************************/
static __INLINE void SystemCoreClockUpdate(void)
{
  SystemCoreClockGet();
}

void SystemInit(void);
uint32_t SystemHFClockGet(void);
uint32_t SystemHFXOClockGet(void);
void SystemHFXOClockSet(uint32_t freq);
uint32_t SystemLFRCOClockGet(void);
uint32_t SystemULFRCOClockGet(void);
uint32_t SystemLFXOClockGet(void);
void SystemLFXOClockSet(uint32_t freq);

#ifdef __cplusplus
}
#endif
#endif /* SYSTEM_EFM32WG_H */
