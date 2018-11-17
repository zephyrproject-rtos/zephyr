/**************************************************************************//**
 * @file system_efm32hg.h
 * @brief CMSIS Cortex-M System Layer for EFM32 devices.
 * @version 5.6.0
 ******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories, Inc. www.silabs.com</b>
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

#ifndef SYSTEM_EFM32HG_H
#define SYSTEM_EFM32HG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**************************************************************************//**
 * @addtogroup Parts
 * @{
 *****************************************************************************/
/**************************************************************************//**
 * @addtogroup EFM32HG EFM32HG
 * @{
 *****************************************************************************/

/*******************************************************************************
 **************************   GLOBAL VARIABLES   *******************************
 ******************************************************************************/

extern uint32_t SystemCoreClock;    /**< System Clock Frequency (Core Clock) */

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/* Interrupt routines - prototypes */
void Reset_Handler(void);           /**< Reset Handler */
void NMI_Handler(void);             /**< NMI Handler */
void HardFault_Handler(void);       /**< Hard Fault Handler */
void SVC_Handler(void);             /**< SVCall Handler */
void PendSV_Handler(void);          /**< PendSV Handler */
void SysTick_Handler(void);         /**< SysTick Handler */

void DMA_IRQHandler(void);          /**< DMA IRQ Handler */
void GPIO_EVEN_IRQHandler(void);    /**< GPIO EVEN IRQ Handler */
void TIMER0_IRQHandler(void);       /**< TIMER0 IRQ Handler */
void ACMP0_IRQHandler(void);        /**< ACMP0 IRQ Handler */
void ADC0_IRQHandler(void);         /**< ADC0 IRQ Handler */
void I2C0_IRQHandler(void);         /**< I2C0 IRQ Handler */
void GPIO_ODD_IRQHandler(void);     /**< GPIO ODD IRQ Handler */
void TIMER1_IRQHandler(void);       /**< TIMER1 IRQ Handler */
void USART1_RX_IRQHandler(void);    /**< USART1 RX IRQ Handler */
void USART1_TX_IRQHandler(void);    /**< USART1 TX IRQ Handler */
void LEUART0_IRQHandler(void);      /**< LEUART0 IRQ Handler */
void PCNT0_IRQHandler(void);        /**< PCNT0 IRQ Handler */
void RTC_IRQHandler(void);          /**< RTC IRQ Handler */
void CMU_IRQHandler(void);          /**< CMU IRQ Handler */
void VCMP_IRQHandler(void);         /**< VCMP IRQ Handler */
void MSC_IRQHandler(void);          /**< MSC IRQ Handler */
void AES_IRQHandler(void);          /**< AES IRQ Handler */
void USART0_RX_IRQHandler(void);    /**< USART0 RX IRQ Handler */
void USART0_TX_IRQHandler(void);    /**< USART0 TX IRQ Handler */
void USB_IRQHandler(void);          /**< USB IRQ Handler */
void TIMER2_IRQHandler(void);       /**< TIMER2 IRQ Handler */

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
  (void)SystemCoreClockGet();
}

void SystemInit(void);
uint32_t SystemHFClockGet(void);
uint32_t SystemHFXOClockGet(void);
void SystemHFXOClockSet(uint32_t freq);
uint32_t SystemLFRCOClockGet(void);
uint32_t SystemULFRCOClockGet(void);
uint32_t SystemLFXOClockGet(void);
void SystemLFXOClockSet(uint32_t freq);

/** @} End of group EFM32HG */
/** @} End of group Parts */

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_EFM32HG_H */
