/**
  ******************************************************************************
  * @file    stm32wbxx_ll_system.h
  * @author  MCD Application Team
  * @brief   Header file of SYSTEM LL module.
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
    [..]
    The LL SYSTEM driver contains a set of generic APIs that can be
    used by user:
      (+) Some of the FLASH features need to be handled in the SYSTEM file.
      (+) Access to DBGCMU registers
      (+) Access to SYSCFG registers
      (+) Access to VREFBUF registers

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32WBxx_LL_SYSTEM_H
#define STM32WBxx_LL_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx.h"

/** @addtogroup STM32WBxx_LL_Driver
  * @{
  */

#if defined (FLASH) || defined (SYSCFG) || defined (DBGMCU) || defined (VREFBUF)

/** @defgroup SYSTEM_LL SYSTEM
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
/** @defgroup SYSTEM_LL_Private_Constants SYSTEM Private Constants
  * @{
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup SYSTEM_LL_Exported_Constants SYSTEM Exported Constants
  * @{
  */

/** @defgroup SYSTEM_LL_EC_REMAP SYSCFG REMAP
* @{
*/
#define LL_SYSCFG_REMAP_FLASH                   0x00000000U                                           /*!< Main Flash memory mapped at 0x00000000   */
#define LL_SYSCFG_REMAP_SYSTEMFLASH             SYSCFG_MEMRMP_MEM_MODE_0                              /*!< System Flash memory mapped at 0x00000000 */
#define LL_SYSCFG_REMAP_SRAM                    (SYSCFG_MEMRMP_MEM_MODE_1 | SYSCFG_MEMRMP_MEM_MODE_0) /*!< SRAM1 mapped at 0x00000000               */
#define LL_SYSCFG_REMAP_QUADSPI                 (SYSCFG_MEMRMP_MEM_MODE_2 | SYSCFG_MEMRMP_MEM_MODE_1) /*!< QUADSPI memory mapped at 0x00000000      */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_I2C_FASTMODEPLUS SYSCFG I2C FASTMODEPLUS
  * @{
  */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB6          SYSCFG_CFGR1_I2C_PB6_FMP /*!< Enable Fast Mode Plus on PB6       */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB7          SYSCFG_CFGR1_I2C_PB7_FMP /*!< Enable Fast Mode Plus on PB7       */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB8          SYSCFG_CFGR1_I2C_PB8_FMP /*!< Enable Fast Mode Plus on PB8       */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB9          SYSCFG_CFGR1_I2C_PB9_FMP /*!< Enable Fast Mode Plus on PB9       */
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C1         SYSCFG_CFGR1_I2C1_FMP    /*!< Enable Fast Mode Plus on I2C1 pins */
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C3         SYSCFG_CFGR1_I2C3_FMP    /*!< Enable Fast Mode Plus on I2C3 pins */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_EXTI_PORT SYSCFG EXTI PORT
  * @{
  */
#define LL_SYSCFG_EXTI_PORTA                    0U /*!< EXTI PORT A */
#define LL_SYSCFG_EXTI_PORTB                    1U /*!< EXTI PORT B */
#define LL_SYSCFG_EXTI_PORTC                    2U /*!< EXTI PORT C */
#define LL_SYSCFG_EXTI_PORTD                    3U /*!< EXTI PORT D */
#define LL_SYSCFG_EXTI_PORTE                    4U /*!< EXTI PORT E */
#define LL_SYSCFG_EXTI_PORTH                    7U /*!< EXTI PORT H */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_EXTI_LINE SYSCFG EXTI LINE
  * @{
  */
#define LL_SYSCFG_EXTI_LINE0                    (uint32_t)((0x000FU << 16U) | 0U) /*!< EXTI_POSITION_0  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE1                    (uint32_t)((0x00F0U << 16U) | 0U) /*!< EXTI_POSITION_4  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE2                    (uint32_t)((0x0F00U << 16U) | 0U) /*!< EXTI_POSITION_8  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE3                    (uint32_t)((0xF000U << 16U) | 0U) /*!< EXTI_POSITION_12 | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE4                    (uint32_t)((0x000FU << 16U) | 1U) /*!< EXTI_POSITION_0  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE5                    (uint32_t)((0x00F0U << 16U) | 1U) /*!< EXTI_POSITION_4  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE6                    (uint32_t)((0x0F00U << 16U) | 1U) /*!< EXTI_POSITION_8  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE7                    (uint32_t)((0xF000U << 16U) | 1U) /*!< EXTI_POSITION_12 | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE8                    (uint32_t)((0x000FU << 16U) | 2U) /*!< EXTI_POSITION_0  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE9                    (uint32_t)((0x00F0U << 16U) | 2U) /*!< EXTI_POSITION_4  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE10                   (uint32_t)((0x0F00U << 16U) | 2U) /*!< EXTI_POSITION_8  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE11                   (uint32_t)((0xF000U << 16U) | 2U) /*!< EXTI_POSITION_12 | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE12                   (uint32_t)((0x000FU << 16U) | 3U) /*!< EXTI_POSITION_0  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE13                   (uint32_t)((0x00F0U << 16U) | 3U) /*!< EXTI_POSITION_4  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE14                   (uint32_t)((0x0F00U << 16U) | 3U) /*!< EXTI_POSITION_8  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE15                   (uint32_t)((0xF000U << 16U) | 3U) /*!< EXTI_POSITION_12 | EXTICR[3] */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_TIMBREAK SYSCFG TIMER BREAK
  * @{
  */
#define LL_SYSCFG_TIMBREAK_ECC                  SYSCFG_CFGR2_ECCL       /*!< Enables and locks the ECC error signal
                                                                              with Break Input of TIM1/16/17                                */
#define LL_SYSCFG_TIMBREAK_PVD                  SYSCFG_CFGR2_PVDL       /*!< Enables and locks the PVD connection
                                                                              with TIM1/16/17 Break Input
                                                                              and also the PVDE and PLS bits of the Power Control Interface */
#define LL_SYSCFG_TIMBREAK_SRAM2_PARITY         SYSCFG_CFGR2_SPL        /*!< Enables and locks the SRAM2_PARITY error signal
                                                                              with Break Input of TIM1/16/17                                */
#define LL_SYSCFG_TIMBREAK_LOCKUP               SYSCFG_CFGR2_CLL        /*!< Enables and locks the LOCKUP output of CortexM4
                                                                              with Break Input of TIM1/16/17                                */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_SRAM2WRP SYSCFG SRAM2 WRITE PROTECTION
  * @{
  */
#define LL_SYSCFG_SRAM2WRP_PAGE0                SYSCFG_SWPR1_PAGE0       /*!< SRAM2A Write protection page 0  */
#define LL_SYSCFG_SRAM2WRP_PAGE1                SYSCFG_SWPR1_PAGE1       /*!< SRAM2A Write protection page 1  */
#define LL_SYSCFG_SRAM2WRP_PAGE2                SYSCFG_SWPR1_PAGE2       /*!< SRAM2A Write protection page 2  */
#define LL_SYSCFG_SRAM2WRP_PAGE3                SYSCFG_SWPR1_PAGE3       /*!< SRAM2A Write protection page 3  */
#define LL_SYSCFG_SRAM2WRP_PAGE4                SYSCFG_SWPR1_PAGE4       /*!< SRAM2A Write protection page 4  */
#define LL_SYSCFG_SRAM2WRP_PAGE5                SYSCFG_SWPR1_PAGE5       /*!< SRAM2A Write protection page 5  */
#define LL_SYSCFG_SRAM2WRP_PAGE6                SYSCFG_SWPR1_PAGE6       /*!< SRAM2A Write protection page 6  */
#define LL_SYSCFG_SRAM2WRP_PAGE7                SYSCFG_SWPR1_PAGE7       /*!< SRAM2A Write protection page 7  */
#define LL_SYSCFG_SRAM2WRP_PAGE8                SYSCFG_SWPR1_PAGE8       /*!< SRAM2A Write protection page 8  */
#define LL_SYSCFG_SRAM2WRP_PAGE9                SYSCFG_SWPR1_PAGE9       /*!< SRAM2A Write protection page 9  */
#define LL_SYSCFG_SRAM2WRP_PAGE10               SYSCFG_SWPR1_PAGE10      /*!< SRAM2A Write protection page 10 */
#define LL_SYSCFG_SRAM2WRP_PAGE11               SYSCFG_SWPR1_PAGE11      /*!< SRAM2A Write protection page 11 */
#define LL_SYSCFG_SRAM2WRP_PAGE12               SYSCFG_SWPR1_PAGE12      /*!< SRAM2A Write protection page 12 */
#define LL_SYSCFG_SRAM2WRP_PAGE13               SYSCFG_SWPR1_PAGE13      /*!< SRAM2A Write protection page 13 */
#define LL_SYSCFG_SRAM2WRP_PAGE14               SYSCFG_SWPR1_PAGE14      /*!< SRAM2A Write protection page 14 */
#define LL_SYSCFG_SRAM2WRP_PAGE15               SYSCFG_SWPR1_PAGE15      /*!< SRAM2A Write protection page 15 */
#define LL_SYSCFG_SRAM2WRP_PAGE16               SYSCFG_SWPR1_PAGE16      /*!< SRAM2A Write protection page 16 */
#define LL_SYSCFG_SRAM2WRP_PAGE17               SYSCFG_SWPR1_PAGE17      /*!< SRAM2A Write protection page 17 */
#define LL_SYSCFG_SRAM2WRP_PAGE18               SYSCFG_SWPR1_PAGE18      /*!< SRAM2A Write protection page 18 */
#define LL_SYSCFG_SRAM2WRP_PAGE19               SYSCFG_SWPR1_PAGE19      /*!< SRAM2A Write protection page 19 */
#define LL_SYSCFG_SRAM2WRP_PAGE20               SYSCFG_SWPR1_PAGE20      /*!< SRAM2A Write protection page 20 */
#define LL_SYSCFG_SRAM2WRP_PAGE21               SYSCFG_SWPR1_PAGE21      /*!< SRAM2A Write protection page 21 */
#define LL_SYSCFG_SRAM2WRP_PAGE22               SYSCFG_SWPR1_PAGE22      /*!< SRAM2A Write protection page 22 */
#define LL_SYSCFG_SRAM2WRP_PAGE23               SYSCFG_SWPR1_PAGE23      /*!< SRAM2A Write protection page 23 */
#define LL_SYSCFG_SRAM2WRP_PAGE24               SYSCFG_SWPR1_PAGE24      /*!< SRAM2A Write protection page 24 */
#define LL_SYSCFG_SRAM2WRP_PAGE25               SYSCFG_SWPR1_PAGE25      /*!< SRAM2A Write protection page 25 */
#define LL_SYSCFG_SRAM2WRP_PAGE26               SYSCFG_SWPR1_PAGE26      /*!< SRAM2A Write protection page 26 */
#define LL_SYSCFG_SRAM2WRP_PAGE27               SYSCFG_SWPR1_PAGE27      /*!< SRAM2A Write protection page 27 */
#define LL_SYSCFG_SRAM2WRP_PAGE28               SYSCFG_SWPR1_PAGE28      /*!< SRAM2A Write protection page 28 */
#define LL_SYSCFG_SRAM2WRP_PAGE29               SYSCFG_SWPR1_PAGE29      /*!< SRAM2A Write protection page 29 */
#define LL_SYSCFG_SRAM2WRP_PAGE30               SYSCFG_SWPR1_PAGE30      /*!< SRAM2A Write protection page 30 */
#define LL_SYSCFG_SRAM2WRP_PAGE31               SYSCFG_SWPR1_PAGE31      /*!< SRAM2A Write protection page 31 */

#define LL_SYSCFG_SRAM2WRP_PAGE32               SYSCFG_SWPR2_PAGE32      /*!< SRAM2B Write protection page 32 */
#define LL_SYSCFG_SRAM2WRP_PAGE33               SYSCFG_SWPR2_PAGE33      /*!< SRAM2B Write protection page 33 */
#define LL_SYSCFG_SRAM2WRP_PAGE34               SYSCFG_SWPR2_PAGE34      /*!< SRAM2B Write protection page 34 */
#define LL_SYSCFG_SRAM2WRP_PAGE35               SYSCFG_SWPR2_PAGE35      /*!< SRAM2B Write protection page 35 */
#define LL_SYSCFG_SRAM2WRP_PAGE36               SYSCFG_SWPR2_PAGE36      /*!< SRAM2B Write protection page 36 */
#define LL_SYSCFG_SRAM2WRP_PAGE37               SYSCFG_SWPR2_PAGE37      /*!< SRAM2B Write protection page 37 */
#define LL_SYSCFG_SRAM2WRP_PAGE38               SYSCFG_SWPR2_PAGE38      /*!< SRAM2B Write protection page 38 */
#define LL_SYSCFG_SRAM2WRP_PAGE39               SYSCFG_SWPR2_PAGE39      /*!< SRAM2B Write protection page 39 */
#define LL_SYSCFG_SRAM2WRP_PAGE40               SYSCFG_SWPR2_PAGE40      /*!< SRAM2B Write protection page 40 */
#define LL_SYSCFG_SRAM2WRP_PAGE41               SYSCFG_SWPR2_PAGE41      /*!< SRAM2B Write protection page 41 */
#define LL_SYSCFG_SRAM2WRP_PAGE42               SYSCFG_SWPR2_PAGE42      /*!< SRAM2B Write protection page 42 */
#define LL_SYSCFG_SRAM2WRP_PAGE43               SYSCFG_SWPR2_PAGE43      /*!< SRAM2B Write protection page 43 */
#define LL_SYSCFG_SRAM2WRP_PAGE44               SYSCFG_SWPR2_PAGE44      /*!< SRAM2B Write protection page 44 */
#define LL_SYSCFG_SRAM2WRP_PAGE45               SYSCFG_SWPR2_PAGE45      /*!< SRAM2B Write protection page 45 */
#define LL_SYSCFG_SRAM2WRP_PAGE46               SYSCFG_SWPR2_PAGE46      /*!< SRAM2B Write protection page 46 */
#define LL_SYSCFG_SRAM2WRP_PAGE47               SYSCFG_SWPR2_PAGE47      /*!< SRAM2B Write protection page 47 */
#define LL_SYSCFG_SRAM2WRP_PAGE48               SYSCFG_SWPR2_PAGE48      /*!< SRAM2B Write protection page 48 */
#define LL_SYSCFG_SRAM2WRP_PAGE49               SYSCFG_SWPR2_PAGE49      /*!< SRAM2B Write protection page 49 */
#define LL_SYSCFG_SRAM2WRP_PAGE50               SYSCFG_SWPR2_PAGE50      /*!< SRAM2B Write protection page 50 */
#define LL_SYSCFG_SRAM2WRP_PAGE51               SYSCFG_SWPR2_PAGE51      /*!< SRAM2B Write protection page 51 */
#define LL_SYSCFG_SRAM2WRP_PAGE52               SYSCFG_SWPR2_PAGE52      /*!< SRAM2B Write protection page 52 */
#define LL_SYSCFG_SRAM2WRP_PAGE53               SYSCFG_SWPR2_PAGE53      /*!< SRAM2B Write protection page 53 */
#define LL_SYSCFG_SRAM2WRP_PAGE54               SYSCFG_SWPR2_PAGE54      /*!< SRAM2B Write protection page 54 */
#define LL_SYSCFG_SRAM2WRP_PAGE55               SYSCFG_SWPR2_PAGE55      /*!< SRAM2B Write protection page 55 */
#define LL_SYSCFG_SRAM2WRP_PAGE56               SYSCFG_SWPR2_PAGE56      /*!< SRAM2B Write protection page 56 */
#define LL_SYSCFG_SRAM2WRP_PAGE57               SYSCFG_SWPR2_PAGE57      /*!< SRAM2B Write protection page 57 */
#define LL_SYSCFG_SRAM2WRP_PAGE58               SYSCFG_SWPR2_PAGE58      /*!< SRAM2B Write protection page 58 */
#define LL_SYSCFG_SRAM2WRP_PAGE59               SYSCFG_SWPR2_PAGE59      /*!< SRAM2B Write protection page 59 */
#define LL_SYSCFG_SRAM2WRP_PAGE60               SYSCFG_SWPR2_PAGE60      /*!< SRAM2B Write protection page 60 */
#define LL_SYSCFG_SRAM2WRP_PAGE61               SYSCFG_SWPR2_PAGE61      /*!< SRAM2B Write protection page 61 */
#define LL_SYSCFG_SRAM2WRP_PAGE62               SYSCFG_SWPR2_PAGE62      /*!< SRAM2B Write protection page 62 */
#define LL_SYSCFG_SRAM2WRP_PAGE63               SYSCFG_SWPR2_PAGE63      /*!< SRAM2B Write protection page 63 */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_IM SYSCFG CPU1 INTERRUPT MASK
  * @{
  */
#define LL_SYSCFG_GRP1_TIM1                     SYSCFG_IMR1_TIM1IM     /*!< Enabling of interrupt from Timer 1 to CPU1                    */
#define LL_SYSCFG_GRP1_TIM16                    SYSCFG_IMR1_TIM16IM    /*!< Enabling of interrupt from Timer 16 to CPU1                   */
#define LL_SYSCFG_GRP1_TIM17                    SYSCFG_IMR1_TIM17IM    /*!< Enabling of interrupt from Timer 17 to CPU1                   */

#define LL_SYSCFG_GRP1_EXTI5                    SYSCFG_IMR1_EXTI5IM    /*!< Enabling of interrupt from External Interrupt Line 5 to CPU1  */
#define LL_SYSCFG_GRP1_EXTI6                    SYSCFG_IMR1_EXTI6IM    /*!< Enabling of interrupt from External Interrupt Line 6 to CPU1  */
#define LL_SYSCFG_GRP1_EXTI7                    SYSCFG_IMR1_EXTI7IM    /*!< Enabling of interrupt from External Interrupt Line 7 to CPU1  */
#define LL_SYSCFG_GRP1_EXTI8                    SYSCFG_IMR1_EXTI8IM    /*!< Enabling of interrupt from External Interrupt Line 8 to CPU1  */
#define LL_SYSCFG_GRP1_EXTI9                    SYSCFG_IMR1_EXTI9IM    /*!< Enabling of interrupt from External Interrupt Line 9 to CPU1  */
#define LL_SYSCFG_GRP1_EXTI10                   SYSCFG_IMR1_EXTI10IM   /*!< Enabling of interrupt from External Interrupt Line 10 to CPU1 */
#define LL_SYSCFG_GRP1_EXTI11                   SYSCFG_IMR1_EXTI11IM   /*!< Enabling of interrupt from External Interrupt Line 11 to CPU1 */
#define LL_SYSCFG_GRP1_EXTI12                   SYSCFG_IMR1_EXTI12IM   /*!< Enabling of interrupt from External Interrupt Line 12 to CPU1 */
#define LL_SYSCFG_GRP1_EXTI13                   SYSCFG_IMR1_EXTI13IM   /*!< Enabling of interrupt from External Interrupt Line 13 to CPU1 */
#define LL_SYSCFG_GRP1_EXTI14                   SYSCFG_IMR1_EXTI14IM   /*!< Enabling of interrupt from External Interrupt Line 14 to CPU1 */
#define LL_SYSCFG_GRP1_EXTI15                   SYSCFG_IMR1_EXTI15IM   /*!< Enabling of interrupt from External Interrupt Line 15 to CPU1 */

#define LL_SYSCFG_GRP2_PVM1                     SYSCFG_IMR2_PVM1IM     /*!< Enabling of interrupt from Power Voltage Monitoring 1 to CPU1 */
#define LL_SYSCFG_GRP2_PVM3                     SYSCFG_IMR2_PVM3IM     /*!< Enabling of interrupt from Power Voltage Monitoring 3 to CPU1 */
#define LL_SYSCFG_GRP2_PVD                      SYSCFG_IMR2_PVDIM      /*!< Enabling of interrupt from Power Voltage Detector to CPU1     */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_C2_IM SYSCFG CPU2 INTERRUPT MASK
  * @{
  */
#define LL_C2_SYSCFG_GRP1_RTCSTAMP_RTCTAMP_LSECSS  SYSCFG_C2IMR1_RTCSTAMPTAMPLSECSSIM /*!< Enabling of interrupt from RTC TimeStamp, RTC Tampers
                                                                                      and LSE Clock Security System to CPU2                             */
#define LL_C2_SYSCFG_GRP1_RTCWKUP               SYSCFG_C2IMR1_RTCWKUPIM  /*!< Enabling of interrupt from RTC Wakeup to CPU2                     */
#define LL_C2_SYSCFG_GRP1_RTCALARM              SYSCFG_C2IMR1_RTCALARMIM /*!< Enabling of interrupt from RTC Alarms to CPU2                     */
#define LL_C2_SYSCFG_GRP1_RCC                   SYSCFG_C2IMR1_RCCIM      /*!< Enabling of interrupt from RCC to CPU2                            */
#define LL_C2_SYSCFG_GRP1_FLASH                 SYSCFG_C2IMR1_FLASHIM    /*!< Enabling of interrupt from FLASH to CPU2                          */
#define LL_C2_SYSCFG_GRP1_PKA                   SYSCFG_C2IMR1_PKAIM      /*!< Enabling of interrupt from Public Key Accelerator to CPU2         */
#define LL_C2_SYSCFG_GRP1_RNG                   SYSCFG_C2IMR1_RNGIM      /*!< Enabling of interrupt from Random Number Generator to CPU2        */
#define LL_C2_SYSCFG_GRP1_AES1                  SYSCFG_C2IMR1_AES1IM     /*!< Enabling of interrupt from Advanced Encryption Standard 1 to CPU2 */
#define LL_C2_SYSCFG_GRP1_COMP                  SYSCFG_C2IMR1_COMPIM     /*!< Enabling of interrupt from Comparator to CPU2                     */
#define LL_C2_SYSCFG_GRP1_ADC                   SYSCFG_C2IMR1_ADCIM      /*!< Enabling of interrupt from Analog Digital Converter to CPU2       */

#define LL_C2_SYSCFG_GRP1_EXTI0                 SYSCFG_C2IMR1_EXTI0IM    /*!< Enabling of interrupt from External Interrupt Line 0 to CPU2      */
#define LL_C2_SYSCFG_GRP1_EXTI1                 SYSCFG_C2IMR1_EXTI1IM    /*!< Enabling of interrupt from External Interrupt Line 1 to CPU2      */
#define LL_C2_SYSCFG_GRP1_EXTI2                 SYSCFG_C2IMR1_EXTI2IM    /*!< Enabling of interrupt from External Interrupt Line 2 to CPU2      */
#define LL_C2_SYSCFG_GRP1_EXTI3                 SYSCFG_C2IMR1_EXTI3IM    /*!< Enabling of interrupt from External Interrupt Line 3 to CPU2      */
#define LL_C2_SYSCFG_GRP1_EXTI4                 SYSCFG_C2IMR1_EXTI4IM    /*!< Enabling of interrupt from External Interrupt Line 4 to CPU2      */
#define LL_C2_SYSCFG_GRP1_EXTI5                 SYSCFG_C2IMR1_EXTI5IM    /*!< Enabling of interrupt from External Interrupt Line 5 to CPU2      */
#define LL_C2_SYSCFG_GRP1_EXTI6                 SYSCFG_C2IMR1_EXTI6IM    /*!< Enabling of interrupt from External Interrupt Line 6 to CPU2      */
#define LL_C2_SYSCFG_GRP1_EXTI7                 SYSCFG_C2IMR1_EXTI7IM    /*!< Enabling of interrupt from External Interrupt Line 7 to CPU2      */
#define LL_C2_SYSCFG_GRP1_EXTI8                 SYSCFG_C2IMR1_EXTI8IM    /*!< Enabling of interrupt from External Interrupt Line 8 to CPU2      */
#define LL_C2_SYSCFG_GRP1_EXTI9                 SYSCFG_C2IMR1_EXTI9IM    /*!< Enabling of interrupt from External Interrupt Line 9 to CPU2      */
#define LL_C2_SYSCFG_GRP1_EXTI10                SYSCFG_C2IMR1_EXTI10IM   /*!< Enabling of interrupt from External Interrupt Line 10 to CPU2     */
#define LL_C2_SYSCFG_GRP1_EXTI11                SYSCFG_C2IMR1_EXTI11IM   /*!< Enabling of interrupt from External Interrupt Line 11 to CPU2     */
#define LL_C2_SYSCFG_GRP1_EXTI12                SYSCFG_C2IMR1_EXTI12IM   /*!< Enabling of interrupt from External Interrupt Line 12 to CPU2     */
#define LL_C2_SYSCFG_GRP1_EXTI13                SYSCFG_C2IMR1_EXTI13IM   /*!< Enabling of interrupt from External Interrupt Line 13 to CPU2     */
#define LL_C2_SYSCFG_GRP1_EXTI14                SYSCFG_C2IMR1_EXTI14IM   /*!< Enabling of interrupt from External Interrupt Line 14 to CPU2     */
#define LL_C2_SYSCFG_GRP1_EXTI15                SYSCFG_C2IMR1_EXTI15IM   /*!< Enabling of interrupt from External Interrupt Line 15 to CPU2     */

#define LL_C2_SYSCFG_GRP2_DMA1CH1               SYSCFG_C2IMR2_DMA1CH1IM  /*!< Enabling of interrupt from DMA1 Channel 1 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA1CH2               SYSCFG_C2IMR2_DMA1CH2IM  /*!< Enabling of interrupt from DMA1 Channel 2 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA1CH3               SYSCFG_C2IMR2_DMA1CH3IM  /*!< Enabling of interrupt from DMA1 Channel 3 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA1CH4               SYSCFG_C2IMR2_DMA1CH4IM  /*!< Enabling of interrupt from DMA1 Channel 4 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA1CH5               SYSCFG_C2IMR2_DMA1CH5IM  /*!< Enabling of interrupt from DMA1 Channel 5 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA1CH6               SYSCFG_C2IMR2_DMA1CH6IM  /*!< Enabling of interrupt from DMA1 Channel 6 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA1CH7               SYSCFG_C2IMR2_DMA1CH7IM  /*!< Enabling of interrupt from DMA1 Channel 7 to CPU2                 */

#define LL_C2_SYSCFG_GRP2_DMA2CH1               SYSCFG_C2IMR2_DMA2CH1IM  /*!< Enabling of interrupt from DMA2 Channel 1 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA2CH2               SYSCFG_C2IMR2_DMA2CH2IM  /*!< Enabling of interrupt from DMA2 Channel 2 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA2CH3               SYSCFG_C2IMR2_DMA2CH3IM  /*!< Enabling of interrupt from DMA2 Channel 3 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA2CH4               SYSCFG_C2IMR2_DMA2CH4IM  /*!< Enabling of interrupt from DMA2 Channel 4 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA2CH5               SYSCFG_C2IMR2_DMA2CH5IM  /*!< Enabling of interrupt from DMA2 Channel 5 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA2CH6               SYSCFG_C2IMR2_DMA2CH6IM  /*!< Enabling of interrupt from DMA2 Channel 6 to CPU2                 */
#define LL_C2_SYSCFG_GRP2_DMA2CH7               SYSCFG_C2IMR2_DMA2CH7IM  /*!< Enabling of interrupt from DMA2 Channel 7 to CPU2                 */

#define LL_C2_SYSCFG_GRP2_DMAMUX1               SYSCFG_C2IMR2_DMAMUX1IM  /*!< Enabling of interrupt from DMAMUX1 to CPU2                        */
#define LL_C2_SYSCFG_GRP2_PVM1                  SYSCFG_C2IMR2_PVM1IM     /*!< Enabling of interrupt from Power Voltage Monitoring 1 to CPU2     */
#define LL_C2_SYSCFG_GRP2_PVM3                  SYSCFG_C2IMR2_PVM3IM     /*!< Enabling of interrupt from Power Voltage Monitoring 3 to CPU2     */
#define LL_C2_SYSCFG_GRP2_PVD                   SYSCFG_C2IMR2_PVDIM      /*!< Enabling of interrupt from Power Voltage Detector to CPU2         */
#define LL_C2_SYSCFG_GRP2_TSC                   SYSCFG_C2IMR2_TSCIM      /*!< Enabling of interrupt from Touch Sensing Controller to CPU2       */
#define LL_C2_SYSCFG_GRP2_LCD                   SYSCFG_C2IMR2_LCDIM      /*!< Enabling of interrupt from Liquid Crystal Display to CPU2         */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_SECURE_IP_ACCESS SYSCFG SECURE IP ACCESS
  * @{
  */
#define LL_SYSCFG_SECURE_ACCESS_AES1            SYSCFG_SIPCR_SAES1       /*!< Enabling the security access of Advanced Encryption Standard 1 KEY[7:0] */
#define LL_SYSCFG_SECURE_ACCESS_AES2            SYSCFG_SIPCR_SAES2       /*!< Enabling the security access of Advanced Encryption Standard 2          */
#define LL_SYSCFG_SECURE_ACCESS_PKA             SYSCFG_SIPCR_SPKA        /*!< Enabling the security access of Public Key Accelerator                  */
#define LL_SYSCFG_SECURE_ACCESS_RNG             SYSCFG_SIPCR_SRNG        /*!< Enabling the security access of Random Number Generator                 */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB1_GRP1_STOP_IP DBGMCU CPU1 APB1 GRP1 STOP IP
  * @{
  */
#define LL_DBGMCU_APB1_GRP1_TIM2_STOP      DBGMCU_APB1FZR1_DBG_TIM2_STOP   /*!< The counter clock of TIM2 is stopped when the core is halted              */
#define LL_DBGMCU_APB1_GRP1_RTC_STOP       DBGMCU_APB1FZR1_DBG_RTC_STOP    /*!< The clock of the RTC counter is stopped when the core is halted           */
#define LL_DBGMCU_APB1_GRP1_WWDG_STOP      DBGMCU_APB1FZR1_DBG_WWDG_STOP   /*!< The window watchdog counter clock is stopped when the core is halted      */
#define LL_DBGMCU_APB1_GRP1_IWDG_STOP      DBGMCU_APB1FZR1_DBG_IWDG_STOP   /*!< The independent watchdog counter clock is stopped when the core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C1_STOP      DBGMCU_APB1FZR1_DBG_I2C1_STOP   /*!< The I2C1 SMBus timeout is frozen                                          */
#define LL_DBGMCU_APB1_GRP1_I2C3_STOP      DBGMCU_APB1FZR1_DBG_I2C3_STOP   /*!< The I2C3 SMBus timeout is frozen                                          */
#define LL_DBGMCU_APB1_GRP1_LPTIM1_STOP    DBGMCU_APB1FZR1_DBG_LPTIM1_STOP /*!< The counter clock of LPTIM1 is stopped when the core is halted            */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_C2_APB1_GRP1_STOP_IP DBGMCU CPU2 APB1 GRP1 STOP IP
  * @{
  */
#define LL_C2_DBGMCU_APB1_GRP1_TIM2_STOP   DBGMCU_C2APB1FZR1_DBG_TIM2_STOP   /*!< The counter clock of TIM2 is stopped when the core is halted              */
#define LL_C2_DBGMCU_APB1_GRP1_RTC_STOP    DBGMCU_C2APB1FZR1_DBG_RTC_STOP    /*!< The clock of the RTC counter is stopped when the core is halted           */
#define LL_C2_DBGMCU_APB1_GRP1_IWDG_STOP   DBGMCU_C2APB1FZR1_DBG_IWDG_STOP   /*!< The independent watchdog counter clock is stopped when the core is halted */
#define LL_C2_DBGMCU_APB1_GRP1_I2C1_STOP   DBGMCU_C2APB1FZR1_DBG_I2C1_STOP   /*!< The I2C1 SMBus timeout is frozen                                          */
#define LL_C2_DBGMCU_APB1_GRP1_I2C3_STOP   DBGMCU_C2APB1FZR1_DBG_I2C3_STOP   /*!< The I2C3 SMBus timeout is frozen                                          */
#define LL_C2_DBGMCU_APB1_GRP1_LPTIM1_STOP DBGMCU_C2APB1FZR1_DBG_LPTIM1_STOP /*!< The counter clock of LPTIM1 is stopped when the core is halted            */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB1_GRP2_STOP_IP DBGMCU CPU1 APB1 GRP2 STOP IP
  * @{
  */
#define LL_DBGMCU_APB1_GRP2_LPTIM2_STOP    DBGMCU_APB1FZR2_DBG_LPTIM2_STOP /*!< The counter clock of LPTIM2 is stopped when the core is halted            */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_C2_APB1_GRP2_STOP_IP DBGMCU CPU2 APB1 GRP2 STOP IP
  * @{
  */
#define LL_C2_DBGMCU_APB1_GRP2_LPTIM2_STOP DBGMCU_C2APB1FZR2_DBG_LPTIM2_STOP /*!< The counter clock of LPTIM2 is stopped when the core is halted            */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB2_GRP1_STOP_IP DBGMCU CPU1 APB2 GRP1 STOP IP
  * @{
  */
#define LL_DBGMCU_APB2_GRP1_TIM1_STOP      DBGMCU_APB2FZR_DBG_TIM1_STOP   /*!< The counter clock of TIM1 is stopped when the core is halted              */
#define LL_DBGMCU_APB2_GRP1_TIM16_STOP     DBGMCU_APB2FZR_DBG_TIM16_STOP  /*!< The counter clock of TIM16 is stopped when the core is halted             */
#define LL_DBGMCU_APB2_GRP1_TIM17_STOP     DBGMCU_APB2FZR_DBG_TIM17_STOP  /*!< The counter clock of TIM17 is stopped when the core is halted             */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_C2_APB2_GRP1_STOP_IP DBGMCU CPU2 APB2 GRP1 STOP IP
  * @{
  */
#define LL_C2_DBGMCU_APB2_GRP1_TIM1_STOP   DBGMCU_C2APB2FZR_DBG_TIM1_STOP   /*!< The counter clock of TIM1 is stopped when the core is halted              */
#define LL_C2_DBGMCU_APB2_GRP1_TIM16_STOP  DBGMCU_C2APB2FZR_DBG_TIM16_STOP  /*!< The counter clock of TIM16 is stopped when the core is halted             */
#define LL_C2_DBGMCU_APB2_GRP1_TIM17_STOP  DBGMCU_C2APB2FZR_DBG_TIM17_STOP  /*!< The counter clock of TIM17 is stopped when the core is halted             */
/**
  * @}
  */

#if defined(VREFBUF)
/** @defgroup SYSTEM_LL_EC_VOLTAGE VREFBUF VOLTAGE
  * @{
  */
#define LL_VREFBUF_VOLTAGE_SCALE0          0x00000000U     /*!< Voltage reference scale 0 (VREF_OUT1) */
#define LL_VREFBUF_VOLTAGE_SCALE1          VREFBUF_CSR_VRS /*!< Voltage reference scale 1 (VREF_OUT2) */
/**
  * @}
  */
#endif /* VREFBUF */

/** @defgroup SYSTEM_LL_EC_LATENCY FLASH LATENCY
  * @{
  */
#define LL_FLASH_LATENCY_0                 FLASH_ACR_LATENCY_0WS   /*!< FLASH Zero wait state   */
#define LL_FLASH_LATENCY_1                 FLASH_ACR_LATENCY_1WS   /*!< FLASH One wait state    */
#define LL_FLASH_LATENCY_2                 FLASH_ACR_LATENCY_2WS   /*!< FLASH Two wait states   */
#define LL_FLASH_LATENCY_3                 FLASH_ACR_LATENCY_3WS   /*!< FLASH Three wait states */

/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @defgroup SYSTEM_LL_Exported_Functions SYSTEM Exported Functions
  * @{
  */

/** @defgroup SYSTEM_LL_EF_SYSCFG SYSCFG
  * @{
  */

/**
  * @brief  Set memory mapping at address 0x00000000
  * @rmtoll SYSCFG_MEMRMP MEM_MODE      LL_SYSCFG_SetRemapMemory
  * @param  Memory This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_REMAP_FLASH
  *         @arg @ref LL_SYSCFG_REMAP_SYSTEMFLASH
  *         @arg @ref LL_SYSCFG_REMAP_SRAM
  *         @arg @ref LL_SYSCFG_REMAP_QUADSPI
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapMemory(uint32_t Memory)
{
  MODIFY_REG(SYSCFG->MEMRMP, SYSCFG_MEMRMP_MEM_MODE, Memory);
}

/**
  * @brief  Get memory mapping at address 0x00000000
  * @rmtoll SYSCFG_MEMRMP MEM_MODE      LL_SYSCFG_GetRemapMemory
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_SYSCFG_REMAP_FLASH
  *         @arg @ref LL_SYSCFG_REMAP_SYSTEMFLASH
  *         @arg @ref LL_SYSCFG_REMAP_SRAM
  *         @arg @ref LL_SYSCFG_REMAP_QUADSPI
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetRemapMemory(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_MEM_MODE));
}

/**
  * @brief  Enable I/O analog switch voltage booster.
  * @note   When voltage booster is enabled, I/O analog switches are supplied
  *         by a dedicated voltage booster, from VDD power domain. This is
  *         the recommended configuration with low VDDA voltage operation.
  * @note   The I/O analog switch voltage booster is relevant for peripherals
  *         using I/O in analog input: ADC and COMP.
  *         However, COMP inputs have a high impedance and
  *         voltage booster do not impact performance significantly.
  *         Therefore, the voltage booster is mainly intended for
  *         usage with ADC.
  * @rmtoll SYSCFG_CFGR1 BOOSTEN       LL_SYSCFG_EnableAnalogBooster
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableAnalogBooster(void)
{
  SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_BOOSTEN);
}

/**
  * @brief  Disable I/O analog switch voltage booster.
  * @note   When voltage booster is enabled, I/O analog switches are supplied
  *         by a dedicated voltage booster, from VDD power domain. This is
  *         the recommended configuration with low VDDA voltage operation.
  * @note   The I/O analog switch voltage booster is relevant for peripherals
  *         using I/O in analog input: ADC and COMP.
  *         However, COMP inputs have a high impedance and
  *         voltage booster do not impact performance significantly.
  *         Therefore, the voltage booster is mainly intended for
  *         usage with ADC.
  * @rmtoll SYSCFG_CFGR1 BOOSTEN       LL_SYSCFG_DisableAnalogBooster
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableAnalogBooster(void)
{
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_BOOSTEN);
}

/**
  * @brief  Enable the Analog GPIO switch to control voltage selection
  *         when the supply voltage is supplied by VDDA
  * @rmtoll SYSCFG_CFGR1   ANASWVDD   LL_SYSCFG_EnableAnalogGpioSwitch
  * @note   Activating the gpio switch enable IOs analog switches supplied by VDDA
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableAnalogGpioSwitch(void)
{
  SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_ANASWVDD);
}

/**
  * @brief  Disable the Analog GPIO switch to control voltage selection
  *         when the supply voltage is supplied by VDDA
  * @rmtoll SYSCFG_CFGR1   ANASWVDD   LL_SYSCFG_DisableAnalogGpioSwitch
  * @note   Activating the gpio switch enable IOs analog switches supplied by VDDA
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableAnalogGpioSwitch(void)
{
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_ANASWVDD);
}

/**
  * @brief  Enable the I2C fast mode plus driving capability.
  * @rmtoll SYSCFG_CFGR1 I2C_PBx_FMP   LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2Cx_FMP      LL_SYSCFG_EnableFastModePlus
  * @param  ConfigFastModePlus This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB6
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB7
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB8
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB9
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C1
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C3
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableFastModePlus(uint32_t ConfigFastModePlus)
{
  SET_BIT(SYSCFG->CFGR1, ConfigFastModePlus);
}

/**
  * @brief  Disable the I2C fast mode plus driving capability.
  * @rmtoll SYSCFG_CFGR1 I2C_PBx_FMP   LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2Cx_FMP      LL_SYSCFG_DisableFastModePlus
  * @param  ConfigFastModePlus This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB6
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB7
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB8
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB9
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C1
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C3
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableFastModePlus(uint32_t ConfigFastModePlus)
{
  CLEAR_BIT(SYSCFG->CFGR1, ConfigFastModePlus);
}

/**
  * @brief  Enable Floating Point Unit Invalid operation Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_0      LL_SYSCFG_EnableIT_FPU_IOC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableIT_FPU_IOC(void)
{
  SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_0);
}

/**
  * @brief  Enable Floating Point Unit Divide-by-zero Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_1      LL_SYSCFG_EnableIT_FPU_DZC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableIT_FPU_DZC(void)
{
  SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_1);
}

/**
  * @brief  Enable Floating Point Unit Underflow Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_2      LL_SYSCFG_EnableIT_FPU_UFC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableIT_FPU_UFC(void)
{
  SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_2);
}

/**
  * @brief  Enable Floating Point Unit Overflow Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_3      LL_SYSCFG_EnableIT_FPU_OFC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableIT_FPU_OFC(void)
{
  SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_3);
}

/**
  * @brief  Enable Floating Point Unit Input denormal Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_4      LL_SYSCFG_EnableIT_FPU_IDC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableIT_FPU_IDC(void)
{
  SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_4);
}

/**
  * @brief  Enable Floating Point Unit Inexact Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_5      LL_SYSCFG_EnableIT_FPU_IXC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableIT_FPU_IXC(void)
{
  SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_5);
}

/**
  * @brief  Disable Floating Point Unit Invalid operation Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_0      LL_SYSCFG_DisableIT_FPU_IOC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableIT_FPU_IOC(void)
{
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_0);
}

/**
  * @brief  Disable Floating Point Unit Divide-by-zero Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_1      LL_SYSCFG_DisableIT_FPU_DZC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableIT_FPU_DZC(void)
{
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_1);
}

/**
  * @brief  Disable Floating Point Unit Underflow Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_2      LL_SYSCFG_DisableIT_FPU_UFC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableIT_FPU_UFC(void)
{
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_2);
}

/**
  * @brief  Disable Floating Point Unit Overflow Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_3      LL_SYSCFG_DisableIT_FPU_OFC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableIT_FPU_OFC(void)
{
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_3);
}

/**
  * @brief  Disable Floating Point Unit Input denormal Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_4      LL_SYSCFG_DisableIT_FPU_IDC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableIT_FPU_IDC(void)
{
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_4);
}

/**
  * @brief  Disable Floating Point Unit Inexact Interrupt
  * @rmtoll SYSCFG_CFGR1 FPU_IE_5      LL_SYSCFG_DisableIT_FPU_IXC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableIT_FPU_IXC(void)
{
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_5);
}

/**
  * @brief  Check if Floating Point Unit Invalid operation Interrupt source is enabled or disabled.
  * @rmtoll SYSCFG_CFGR1 FPU_IE_0      LL_SYSCFG_IsEnabledIT_FPU_IOC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIT_FPU_IOC(void)
{
  return ((READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_0) == (SYSCFG_CFGR1_FPU_IE_0)) ? 1UL : 0UL);
}

/**
  * @brief  Check if Floating Point Unit Divide-by-zero Interrupt source is enabled or disabled.
  * @rmtoll SYSCFG_CFGR1 FPU_IE_1      LL_SYSCFG_IsEnabledIT_FPU_DZC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIT_FPU_DZC(void)
{
  return ((READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_1) == (SYSCFG_CFGR1_FPU_IE_1)) ? 1UL : 0UL);
}

/**
  * @brief  Check if Floating Point Unit Underflow Interrupt source is enabled or disabled.
  * @rmtoll SYSCFG_CFGR1 FPU_IE_2      LL_SYSCFG_IsEnabledIT_FPU_UFC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIT_FPU_UFC(void)
{
  return ((READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_2) == (SYSCFG_CFGR1_FPU_IE_2)) ? 1UL : 0UL);
}

/**
  * @brief  Check if Floating Point Unit Overflow Interrupt source is enabled or disabled.
  * @rmtoll SYSCFG_CFGR1 FPU_IE_3      LL_SYSCFG_IsEnabledIT_FPU_OFC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIT_FPU_OFC(void)
{
  return ((READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_3) == (SYSCFG_CFGR1_FPU_IE_3)) ? 1UL : 0UL);
}

/**
  * @brief  Check if Floating Point Unit Input denormal Interrupt source is enabled or disabled.
  * @rmtoll SYSCFG_CFGR1 FPU_IE_4      LL_SYSCFG_IsEnabledIT_FPU_IDC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIT_FPU_IDC(void)
{
  return ((READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_4) == (SYSCFG_CFGR1_FPU_IE_4)) ? 1UL : 0UL);
}

/**
  * @brief  Check if Floating Point Unit Inexact Interrupt source is enabled or disabled.
  * @rmtoll SYSCFG_CFGR1 FPU_IE_5      LL_SYSCFG_IsEnabledIT_FPU_IXC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIT_FPU_IXC(void)
{
  return ((READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_5) == (SYSCFG_CFGR1_FPU_IE_5)) ? 1UL : 0UL);
}

/**
  * @brief  Configure source input for the EXTI external interrupt.
  * @rmtoll SYSCFG_EXTICR1 EXTIx         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTIx         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTIx         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTIx         LL_SYSCFG_SetEXTISource
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_EXTI_PORTA
  *         @arg @ref LL_SYSCFG_EXTI_PORTB
  *         @arg @ref LL_SYSCFG_EXTI_PORTC
  *         @arg @ref LL_SYSCFG_EXTI_PORTD
  *         @arg @ref LL_SYSCFG_EXTI_PORTE
  *         @arg @ref LL_SYSCFG_EXTI_PORTH
  *
  * @param  Line This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_EXTI_LINE0
  *         @arg @ref LL_SYSCFG_EXTI_LINE1
  *         @arg @ref LL_SYSCFG_EXTI_LINE2
  *         @arg @ref LL_SYSCFG_EXTI_LINE3
  *         @arg @ref LL_SYSCFG_EXTI_LINE4
  *         @arg @ref LL_SYSCFG_EXTI_LINE5
  *         @arg @ref LL_SYSCFG_EXTI_LINE6
  *         @arg @ref LL_SYSCFG_EXTI_LINE7
  *         @arg @ref LL_SYSCFG_EXTI_LINE8
  *         @arg @ref LL_SYSCFG_EXTI_LINE9
  *         @arg @ref LL_SYSCFG_EXTI_LINE10
  *         @arg @ref LL_SYSCFG_EXTI_LINE11
  *         @arg @ref LL_SYSCFG_EXTI_LINE12
  *         @arg @ref LL_SYSCFG_EXTI_LINE13
  *         @arg @ref LL_SYSCFG_EXTI_LINE14
  *         @arg @ref LL_SYSCFG_EXTI_LINE15
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetEXTISource(uint32_t Port, uint32_t Line)
{
  MODIFY_REG(SYSCFG->EXTICR[Line & 0x03U], (Line >> 16U), (Port << ((POSITION_VAL((Line >> 16U))) & 0x0000000FUL)));
}

/**
  * @brief  Get the configured defined for specific EXTI Line
  * @rmtoll SYSCFG_EXTICR1 EXTIx         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTIx         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTIx         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTIx         LL_SYSCFG_GetEXTISource
  * @param  Line This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_EXTI_LINE0
  *         @arg @ref LL_SYSCFG_EXTI_LINE1
  *         @arg @ref LL_SYSCFG_EXTI_LINE2
  *         @arg @ref LL_SYSCFG_EXTI_LINE3
  *         @arg @ref LL_SYSCFG_EXTI_LINE4
  *         @arg @ref LL_SYSCFG_EXTI_LINE5
  *         @arg @ref LL_SYSCFG_EXTI_LINE6
  *         @arg @ref LL_SYSCFG_EXTI_LINE7
  *         @arg @ref LL_SYSCFG_EXTI_LINE8
  *         @arg @ref LL_SYSCFG_EXTI_LINE9
  *         @arg @ref LL_SYSCFG_EXTI_LINE10
  *         @arg @ref LL_SYSCFG_EXTI_LINE11
  *         @arg @ref LL_SYSCFG_EXTI_LINE12
  *         @arg @ref LL_SYSCFG_EXTI_LINE13
  *         @arg @ref LL_SYSCFG_EXTI_LINE14
  *         @arg @ref LL_SYSCFG_EXTI_LINE15
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_SYSCFG_EXTI_PORTA
  *         @arg @ref LL_SYSCFG_EXTI_PORTB
  *         @arg @ref LL_SYSCFG_EXTI_PORTC
  *         @arg @ref LL_SYSCFG_EXTI_PORTD
  *         @arg @ref LL_SYSCFG_EXTI_PORTE
  *         @arg @ref LL_SYSCFG_EXTI_PORTH
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetEXTISource(uint32_t Line)
{
  return (uint32_t)(READ_BIT(SYSCFG->EXTICR[Line & 0x03U], (Line >> 16U)) >> (POSITION_VAL(Line >> 16U) & 0x0000000FUL) );
}

/**
  * @brief  Enable SRAM2 Erase (starts a hardware SRAM2 erase operation. This bit is
  * automatically cleared at the end of the SRAM2 erase operation.)
  * @note This bit is write-protected: setting this bit is possible only after the
  *       correct key sequence is written in the SYSCFG_SKR register.
  * @rmtoll SYSCFG_SCSR  SRAM2ER       LL_SYSCFG_EnableSRAM2Erase
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableSRAM2Erase(void)
{
  SET_BIT(SYSCFG->SCSR, SYSCFG_SCSR_SRAM2ER);
}

/**
  * @brief  Check if SRAM2 erase operation is on going
  * @rmtoll SYSCFG_SCSR  SRAM2BSY      LL_SYSCFG_IsSRAM2EraseOngoing
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsSRAM2EraseOngoing(void)
{
  return ((READ_BIT(SYSCFG->SCSR, SYSCFG_SCSR_SRAM2BSY) == (SYSCFG_SCSR_SRAM2BSY)) ? 1UL : 0UL);
}

/**
  * @brief  Disable CPU2 SRAM fetch (execution) (This bit can be set by Firmware
  *         and will only be reset by a Hardware reset, including a reset after Standby.)
  * @note Firmware writing 0 has no effect.
  * @rmtoll SYSCFG_SCSR  C2RFD         LL_SYSCFG_DisableSRAMFetch
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableSRAMFetch(void)
{
  SET_BIT(SYSCFG->SCSR, SYSCFG_SCSR_C2RFD);
}

/**
  * @brief  Check if CPU2 SRAM fetch is enabled
  * @rmtoll SYSCFG_SCSR  C2RFD         LL_SYSCFG_IsEnabledSRAMFetch
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledSRAMFetch(void)
{
  return ((READ_BIT(SYSCFG->SCSR, SYSCFG_SCSR_C2RFD) != (SYSCFG_SCSR_C2RFD)) ? 1UL : 0UL);
}

/**
  * @brief  Set connections to TIM1/16/17 Break inputs
  * @rmtoll SYSCFG_CFGR2 CLL           LL_SYSCFG_SetTIMBreakInputs\n
  *         SYSCFG_CFGR2 SPL           LL_SYSCFG_SetTIMBreakInputs\n
  *         SYSCFG_CFGR2 PVDL          LL_SYSCFG_SetTIMBreakInputs\n
  *         SYSCFG_CFGR2 ECCL          LL_SYSCFG_SetTIMBreakInputs
  * @param  Break This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_TIMBREAK_ECC
  *         @arg @ref LL_SYSCFG_TIMBREAK_PVD
  *         @arg @ref LL_SYSCFG_TIMBREAK_SRAM2_PARITY
  *         @arg @ref LL_SYSCFG_TIMBREAK_LOCKUP
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetTIMBreakInputs(uint32_t Break)
{
  MODIFY_REG(SYSCFG->CFGR2, SYSCFG_CFGR2_CLL | SYSCFG_CFGR2_SPL | SYSCFG_CFGR2_PVDL | SYSCFG_CFGR2_ECCL, Break);
}

/**
  * @brief  Get connections to TIM1/16/17 Break inputs
  * @rmtoll SYSCFG_CFGR2 CLL           LL_SYSCFG_GetTIMBreakInputs\n
  *         SYSCFG_CFGR2 SPL           LL_SYSCFG_GetTIMBreakInputs\n
  *         SYSCFG_CFGR2 PVDL          LL_SYSCFG_GetTIMBreakInputs\n
  *         SYSCFG_CFGR2 ECCL          LL_SYSCFG_GetTIMBreakInputs
  * @retval Returned value can be can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_TIMBREAK_ECC
  *         @arg @ref LL_SYSCFG_TIMBREAK_PVD
  *         @arg @ref LL_SYSCFG_TIMBREAK_SRAM2_PARITY
  *         @arg @ref LL_SYSCFG_TIMBREAK_LOCKUP
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetTIMBreakInputs(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_CLL | SYSCFG_CFGR2_SPL | SYSCFG_CFGR2_PVDL | SYSCFG_CFGR2_ECCL));
}

/**
  * @brief  Check if SRAM2 parity error detected
  * @rmtoll SYSCFG_CFGR2 SPF           LL_SYSCFG_IsActiveFlag_SP
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_SP(void)
{
  return ((READ_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_SPF) == (SYSCFG_CFGR2_SPF)) ? 1UL : 0UL);
}

/**
  * @brief  Clear SRAM2 parity error flag
  * @rmtoll SYSCFG_CFGR2 SPF           LL_SYSCFG_ClearFlag_SP
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_ClearFlag_SP(void)
{
  SET_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_SPF);
}

/**
  * @brief  Enable SRAM2 page write protection for Pages in range 0 to 31
  * @note Write protection is cleared only by a system reset
  * @rmtoll SYSCFG_SWPR1 PxWP          LL_SYSCFG_EnableSRAM2PageWRP_0_31
  * @param  SRAM2WRP This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE0
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE1
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE2
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE3
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE4
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE5
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE6
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE7
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE8
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE9
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE10
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE11
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE12
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE13
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE14
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE15
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE16
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE17
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE18
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE19
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE20
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE21
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE22
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE23
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE24
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE25
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE26
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE27
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE28
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE29
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE30
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE31
  * @retval None
  */
/* Legacy define */
#define LL_SYSCFG_EnableSRAM2PageWRP    LL_SYSCFG_EnableSRAM2PageWRP_0_31
__STATIC_INLINE void LL_SYSCFG_EnableSRAM2PageWRP_0_31(uint32_t SRAM2WRP)
{
  SET_BIT(SYSCFG->SWPR1, SRAM2WRP);
}

/**
  * @brief  Enable SRAM2 page write protection for Pages in range 32 to 63
  * @note Write protection is cleared only by a system reset
  * @rmtoll SYSCFG_SWPR2 PxWP          LL_SYSCFG_EnableSRAM2PageWRP_32_63
  * @param  SRAM2WRP This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE32
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE33
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE34
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE35
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE36
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE37
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE38
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE39
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE40
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE41
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE42
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE43
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE44
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE45
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE46
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE47
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE48
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE49
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE50
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE51
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE52
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE53
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE54
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE55
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE56
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE57
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE58
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE59
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE60
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE61
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE62
  *         @arg @ref LL_SYSCFG_SRAM2WRP_PAGE63
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableSRAM2PageWRP_32_63(uint32_t SRAM2WRP)
{
  SET_BIT(SYSCFG->SWPR2, SRAM2WRP);
}

/**
  * @brief  SRAM2 page write protection lock prior to erase
  * @rmtoll SYSCFG_SKR   KEY           LL_SYSCFG_LockSRAM2WRP
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_LockSRAM2WRP(void)
{
  /* Writing a wrong key reactivates the write protection */
  WRITE_REG(SYSCFG->SKR, 0x00U);
}

/**
  * @brief  SRAM2 page write protection unlock prior to erase
  * @rmtoll SYSCFG_SKR   KEY           LL_SYSCFG_UnlockSRAM2WRP
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_UnlockSRAM2WRP(void)
{
  /* unlock the write protection of the SRAM2ER bit */
  WRITE_REG(SYSCFG->SKR, 0xCAU);
  WRITE_REG(SYSCFG->SKR, 0x53U);
}

/**
  * @brief  Enable CPU1 Interrupt Mask
  * @rmtoll SYSCFG_IMR1  TIM1IM      LL_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_IMR1  TIM16IM     LL_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_IMR1  TIM17IM     LL_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_IMR1  EXTIxIM     LL_SYSCFG_GRP1_EnableIT
  * @param  Interrupt This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_GRP1_TIM1
  *         @arg @ref LL_SYSCFG_GRP1_TIM16
  *         @arg @ref LL_SYSCFG_GRP1_TIM17
  *         @arg @ref LL_SYSCFG_GRP1_EXTI5
  *         @arg @ref LL_SYSCFG_GRP1_EXTI6
  *         @arg @ref LL_SYSCFG_GRP1_EXTI7
  *         @arg @ref LL_SYSCFG_GRP1_EXTI8
  *         @arg @ref LL_SYSCFG_GRP1_EXTI9
  *         @arg @ref LL_SYSCFG_GRP1_EXTI10
  *         @arg @ref LL_SYSCFG_GRP1_EXTI11
  *         @arg @ref LL_SYSCFG_GRP1_EXTI12
  *         @arg @ref LL_SYSCFG_GRP1_EXTI13
  *         @arg @ref LL_SYSCFG_GRP1_EXTI14
  *         @arg @ref LL_SYSCFG_GRP1_EXTI15
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_GRP1_EnableIT(uint32_t Interrupt)
{
  CLEAR_BIT(SYSCFG->IMR1, Interrupt);
}

/**
  * @brief  Enable CPU1 Interrupt Mask
  * @rmtoll SYSCFG_IMR1  PVM1IM      LL_SYSCFG_GRP2_EnableIT\n
  *         SYSCFG_IMR1  PVM3IM      LL_SYSCFG_GRP2_EnableIT\n
  *         SYSCFG_IMR1  PVDIM       LL_SYSCFG_GRP2_EnableIT
  * @param  Interrupt This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_GRP2_PVM1
  *         @arg @ref LL_SYSCFG_GRP2_PVM3
  *         @arg @ref LL_SYSCFG_GRP2_PVD
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_GRP2_EnableIT(uint32_t Interrupt)
{
  CLEAR_BIT(SYSCFG->IMR2, Interrupt);
}

/**
  * @brief  Disable CPU1 Interrupt Mask
  * @rmtoll SYSCFG_IMR1  TIM1IM      LL_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_IMR1  TIM16IM     LL_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_IMR1  TIM17IM     LL_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_IMR1  EXTIxIM     LL_SYSCFG_GRP1_DisableIT
  * @param  Interrupt This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_GRP1_TIM1
  *         @arg @ref LL_SYSCFG_GRP1_TIM16
  *         @arg @ref LL_SYSCFG_GRP1_TIM17
  *         @arg @ref LL_SYSCFG_GRP1_EXTI5
  *         @arg @ref LL_SYSCFG_GRP1_EXTI6
  *         @arg @ref LL_SYSCFG_GRP1_EXTI7
  *         @arg @ref LL_SYSCFG_GRP1_EXTI8
  *         @arg @ref LL_SYSCFG_GRP1_EXTI9
  *         @arg @ref LL_SYSCFG_GRP1_EXTI10
  *         @arg @ref LL_SYSCFG_GRP1_EXTI11
  *         @arg @ref LL_SYSCFG_GRP1_EXTI12
  *         @arg @ref LL_SYSCFG_GRP1_EXTI13
  *         @arg @ref LL_SYSCFG_GRP1_EXTI14
  *         @arg @ref LL_SYSCFG_GRP1_EXTI15
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_GRP1_DisableIT(uint32_t Interrupt)
{
  SET_BIT(SYSCFG->IMR1, Interrupt);
}

/**
  * @brief  Disable CPU1 Interrupt Mask
  * @rmtoll SYSCFG_IMR2  PVM1IM      LL_SYSCFG_GRP2_DisableIT\n
  *         SYSCFG_IMR2  PVM3IM      LL_SYSCFG_GRP2_DisableIT\n
  *         SYSCFG_IMR2  PVDIM       LL_SYSCFG_GRP2_DisableIT
  * @param  Interrupt This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_GRP2_PVM1
  *         @arg @ref LL_SYSCFG_GRP2_PVM3
  *         @arg @ref LL_SYSCFG_GRP2_PVD
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_GRP2_DisableIT(uint32_t Interrupt)
{
  SET_BIT(SYSCFG->IMR2, Interrupt);
}

/**
  * @brief  Indicate if CPU1 Interrupt Mask is enabled
  * @rmtoll SYSCFG_IMR1  TIM1IM      LL_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_IMR1  TIM16IM     LL_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_IMR1  TIM17IM     LL_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_IMR1  EXTIxIM     LL_SYSCFG_GRP1_IsEnabledIT
  * @param  Interrupt This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_GRP1_TIM1
  *         @arg @ref LL_SYSCFG_GRP1_TIM16
  *         @arg @ref LL_SYSCFG_GRP1_TIM17
  *         @arg @ref LL_SYSCFG_GRP1_EXTI5
  *         @arg @ref LL_SYSCFG_GRP1_EXTI6
  *         @arg @ref LL_SYSCFG_GRP1_EXTI7
  *         @arg @ref LL_SYSCFG_GRP1_EXTI8
  *         @arg @ref LL_SYSCFG_GRP1_EXTI9
  *         @arg @ref LL_SYSCFG_GRP1_EXTI10
  *         @arg @ref LL_SYSCFG_GRP1_EXTI11
  *         @arg @ref LL_SYSCFG_GRP1_EXTI12
  *         @arg @ref LL_SYSCFG_GRP1_EXTI13
  *         @arg @ref LL_SYSCFG_GRP1_EXTI14
  *         @arg @ref LL_SYSCFG_GRP1_EXTI15
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GRP1_IsEnabledIT(uint32_t Interrupt)
{
  return ((READ_BIT(SYSCFG->IMR1, Interrupt) != (Interrupt)) ? 1UL : 0UL);
}

/**
  * @brief  Indicate if CPU1 Interrupt Mask is enabled
  * @rmtoll SYSCFG_IMR2  PVM1IM      LL_SYSCFG_GRP2_IsEnabledIT\n
  *         SYSCFG_IMR2  PVM3IM      LL_SYSCFG_GRP2_IsEnabledIT\n
  *         SYSCFG_IMR2  PVDIM       LL_SYSCFG_GRP2_IsEnabledIT
  * @param  Interrupt This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_GRP2_PVM1
  *         @arg @ref LL_SYSCFG_GRP2_PVM3
  *         @arg @ref LL_SYSCFG_GRP2_PVD
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GRP2_IsEnabledIT(uint32_t Interrupt)
{
  return ((READ_BIT(SYSCFG->IMR2, Interrupt) != (Interrupt)) ? 1UL : 0UL);
}

/**
  * @brief  Enable CPU2 Interrupt Mask
  * @rmtoll SYSCFG_C2IMR1  RTCSTAMPTAMPLSECSSIM      LL_C2_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_C2IMR1  RTCWKUPIM   LL_C2_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_C2IMR1  RTCALARMIM  LL_C2_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_C2IMR1  RCCIM       LL_C2_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_C2IMR1  FLASHIM     LL_C2_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_C2IMR1  PKAIM       LL_C2_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_C2IMR1  RNGIM       LL_C2_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_C2IMR1  AES1IM      LL_C2_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_C2IMR1  COMPIM      LL_C2_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_C2IMR1  ADCIM       LL_C2_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_C2IMR1  EXTIxIM     LL_C2_SYSCFG_GRP1_EnableIT
  * @param  Interrupt This parameter can be a combination of the following values:
  *         @arg @ref LL_C2_SYSCFG_GRP1_RTCSTAMP_RTCTAMP_LSECSS
  *         @arg @ref LL_C2_SYSCFG_GRP1_RTCWKUP
  *         @arg @ref LL_C2_SYSCFG_GRP1_RTCALARM
  *         @arg @ref LL_C2_SYSCFG_GRP1_RCC
  *         @arg @ref LL_C2_SYSCFG_GRP1_FLASH
  *         @arg @ref LL_C2_SYSCFG_GRP1_PKA
  *         @arg @ref LL_C2_SYSCFG_GRP1_RNG
  *         @arg @ref LL_C2_SYSCFG_GRP1_AES1
  *         @arg @ref LL_C2_SYSCFG_GRP1_COMP
  *         @arg @ref LL_C2_SYSCFG_GRP1_ADC
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI0
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI1
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI2
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI3
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI4
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI5
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI6
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI7
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI8
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI9
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI10
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI11
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI12
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI13
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI14
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI15
  * @retval None
  */
__STATIC_INLINE void LL_C2_SYSCFG_GRP1_EnableIT(uint32_t Interrupt)
{
  CLEAR_BIT(SYSCFG->C2IMR1, Interrupt);
}

/**
  * @brief  Enable CPU2 Interrupt Mask
  * @rmtoll SYSCFG_C2IMR2  DMA1CHxIM   LL_C2_SYSCFG_GRP2_EnableIT\n
  *         SYSCFG_C2IMR2  DMA2CHxIM   LL_C2_SYSCFG_GRP2_EnableIT\n
  *         SYSCFG_C2IMR2  PVM1IM      LL_C2_SYSCFG_GRP2_EnableIT\n
  *         SYSCFG_C2IMR2  PVM3IM      LL_C2_SYSCFG_GRP2_EnableIT\n
  *         SYSCFG_C2IMR2  PVDIM       LL_C2_SYSCFG_GRP2_EnableIT\n
  *         SYSCFG_C2IMR2  TSCIM       LL_C2_SYSCFG_GRP2_EnableIT\n
  *         SYSCFG_C2IMR2  LCDIM       LL_C2_SYSCFG_GRP2_EnableIT
  * @param  Interrupt This parameter can be a combination of the following values:
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH1
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH2
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH3
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH4
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH5
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH6
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH7
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH1
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH2
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH3
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH4
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH5
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH6
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH7
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMAMUX1
  *         @arg @ref LL_C2_SYSCFG_GRP2_PVM1
  *         @arg @ref LL_C2_SYSCFG_GRP2_PVM3
  *         @arg @ref LL_C2_SYSCFG_GRP2_PVD
  *         @arg @ref LL_C2_SYSCFG_GRP2_TSC
  *         @arg @ref LL_C2_SYSCFG_GRP2_LCD
  * @retval None
  */
__STATIC_INLINE void LL_C2_SYSCFG_GRP2_EnableIT(uint32_t Interrupt)
{
  CLEAR_BIT(SYSCFG->C2IMR2, Interrupt);
}

/**
  * @brief  Disable CPU2 Interrupt Mask
  * @rmtoll SYSCFG_C2IMR1  RTCSTAMPTAMPLSECSSIM      LL_C2_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_C2IMR1  RTCWKUPIM   LL_C2_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_C2IMR1  RTCALARMIM  LL_C2_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_C2IMR1  RCCIM       LL_C2_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_C2IMR1  FLASHIM     LL_C2_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_C2IMR1  PKAIM       LL_C2_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_C2IMR1  RNGIM       LL_C2_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_C2IMR1  AES1IM      LL_C2_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_C2IMR1  COMPIM      LL_C2_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_C2IMR1  ADCIM       LL_C2_SYSCFG_GRP1_DisableIT\n
  *         SYSCFG_C2IMR1  EXTIxIM     LL_C2_SYSCFG_GRP1_DisableIT
  * @param  Interrupt This parameter can be a combination of the following values:
  *         @arg @ref LL_C2_SYSCFG_GRP1_RTCSTAMP_RTCTAMP_LSECSS
  *         @arg @ref LL_C2_SYSCFG_GRP1_RTCWKUP
  *         @arg @ref LL_C2_SYSCFG_GRP1_RTCALARM
  *         @arg @ref LL_C2_SYSCFG_GRP1_RCC
  *         @arg @ref LL_C2_SYSCFG_GRP1_FLASH
  *         @arg @ref LL_C2_SYSCFG_GRP1_PKA
  *         @arg @ref LL_C2_SYSCFG_GRP1_RNG
  *         @arg @ref LL_C2_SYSCFG_GRP1_AES1
  *         @arg @ref LL_C2_SYSCFG_GRP1_COMP
  *         @arg @ref LL_C2_SYSCFG_GRP1_ADC
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI0
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI1
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI2
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI3
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI4
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI5
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI6
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI7
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI8
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI9
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI10
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI11
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI12
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI13
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI14
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI15
  * @retval None
  */
__STATIC_INLINE void LL_C2_SYSCFG_GRP1_DisableIT(uint32_t Interrupt)
{
  SET_BIT(SYSCFG->C2IMR1, Interrupt);
}

/**
  * @brief  Disable CPU2 Interrupt Mask
  * @rmtoll SYSCFG_C2IMR2  DMA1CHxIM   LL_C2_SYSCFG_GRP2_DisableIT\n
  *         SYSCFG_C2IMR2  DMA2CHxIM   LL_C2_SYSCFG_GRP2_DisableIT\n
  *         SYSCFG_C2IMR2  PVM1IM      LL_C2_SYSCFG_GRP2_DisableIT\n
  *         SYSCFG_C2IMR2  PVM3IM      LL_C2_SYSCFG_GRP2_DisableIT\n
  *         SYSCFG_C2IMR2  PVDIM       LL_C2_SYSCFG_GRP2_DisableIT\n
  *         SYSCFG_C2IMR2  TSCIM       LL_C2_SYSCFG_GRP2_DisableIT\n
  *         SYSCFG_C2IMR2  LCDIM       LL_C2_SYSCFG_GRP2_DisableIT
  * @param  Interrupt This parameter can be a combination of the following values:
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH1
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH2
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH3
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH4
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH5
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH6
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH7
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH1
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH2
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH3
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH4
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH5
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH6
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH7
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMAMUX1
  *         @arg @ref LL_C2_SYSCFG_GRP2_PVM1
  *         @arg @ref LL_C2_SYSCFG_GRP2_PVM3
  *         @arg @ref LL_C2_SYSCFG_GRP2_PVD
  *         @arg @ref LL_C2_SYSCFG_GRP2_TSC
  *         @arg @ref LL_C2_SYSCFG_GRP2_LCD
  * @retval None
  */
__STATIC_INLINE void LL_C2_SYSCFG_GRP2_DisableIT(uint32_t Interrupt)
{
  SET_BIT(SYSCFG->C2IMR2, Interrupt);
}

/**
  * @brief  Indicate if CPU2 Interrupt Mask is enabled
  * @rmtoll SYSCFG_C2IMR1  RTCSTAMPTAMPLSECSSIM      LL_C2_SYSCFG_GRP1_EnableIT\n
  *         SYSCFG_C2IMR1  RTCWKUPIM   LL_C2_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_C2IMR1  RTCALARMIM  LL_C2_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_C2IMR1  RCCIM       LL_C2_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_C2IMR1  FLASHIM     LL_C2_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_C2IMR1  PKAIM       LL_C2_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_C2IMR1  RNGIM       LL_C2_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_C2IMR1  AES1IM      LL_C2_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_C2IMR1  COMPIM      LL_C2_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_C2IMR1  ADCIM       LL_C2_SYSCFG_GRP1_IsEnabledIT\n
  *         SYSCFG_C2IMR1  EXTIxIM     LL_C2_SYSCFG_GRP1_IsEnabledIT
  * @param  Interrupt This parameter can be one of the following values:
  *         @arg @ref LL_C2_SYSCFG_GRP1_RTCSTAMP_RTCTAMP_LSECSS
  *         @arg @ref LL_C2_SYSCFG_GRP1_RTCWKUP
  *         @arg @ref LL_C2_SYSCFG_GRP1_RTCALARM
  *         @arg @ref LL_C2_SYSCFG_GRP1_RCC
  *         @arg @ref LL_C2_SYSCFG_GRP1_FLASH
  *         @arg @ref LL_C2_SYSCFG_GRP1_PKA
  *         @arg @ref LL_C2_SYSCFG_GRP1_RNG
  *         @arg @ref LL_C2_SYSCFG_GRP1_AES1
  *         @arg @ref LL_C2_SYSCFG_GRP1_COMP
  *         @arg @ref LL_C2_SYSCFG_GRP1_ADC
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI0
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI1
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI2
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI3
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI4
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI5
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI6
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI7
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI8
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI9
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI10
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI11
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI12
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI13
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI14
  *         @arg @ref LL_C2_SYSCFG_GRP1_EXTI15
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C2_SYSCFG_GRP1_IsEnabledIT(uint32_t Interrupt)
{
  return ((READ_BIT(SYSCFG->C2IMR1, Interrupt) != (Interrupt)) ? 1UL : 0UL);
}

/**
  * @brief  Indicate if CPU2 Interrupt Mask is enabled
  * @rmtoll SYSCFG_C2IMR2  DMA1CHxIM   LL_C2_SYSCFG_GRP2_IsEnabledIT\n
  *         SYSCFG_C2IMR2  DMA2CHxIM   LL_C2_SYSCFG_GRP2_IsEnabledIT\n
  *         SYSCFG_C2IMR2  PVM1IM      LL_C2_SYSCFG_GRP2_IsEnabledIT\n
  *         SYSCFG_C2IMR2  PVM3IM      LL_C2_SYSCFG_GRP2_IsEnabledIT\n
  *         SYSCFG_C2IMR2  PVDIM       LL_C2_SYSCFG_GRP2_IsEnabledIT\n
  *         SYSCFG_C2IMR2  TSCIM       LL_C2_SYSCFG_GRP2_IsEnabledIT\n
  *         SYSCFG_C2IMR2  LCDIM       LL_C2_SYSCFG_GRP2_IsEnabledIT
  * @param  Interrupt This parameter can be one of the following values:
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH1
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH2
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH3
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH4
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH5
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH6
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA1CH7
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH1
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH2
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH3
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH4
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH5
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH6
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMA2CH7
  *         @arg @ref LL_C2_SYSCFG_GRP2_DMAMUX1
  *         @arg @ref LL_C2_SYSCFG_GRP2_PVM1
  *         @arg @ref LL_C2_SYSCFG_GRP2_PVM3
  *         @arg @ref LL_C2_SYSCFG_GRP2_PVD
  *         @arg @ref LL_C2_SYSCFG_GRP2_TSC
  *         @arg @ref LL_C2_SYSCFG_GRP2_LCD
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C2_SYSCFG_GRP2_IsEnabledIT(uint32_t Interrupt)
{
  return ((READ_BIT(SYSCFG->C2IMR2, Interrupt) != (Interrupt)) ? 1UL : 0UL);
}

/**
  * @brief  Enable the access for security IP
  * @rmtoll SYSCFG_SIPCR SAES1         LL_SYSCFG_EnableSecurityAccess\n
  *         SYSCFG_CFGR1 SAES2         LL_SYSCFG_EnableSecurityAccess\n
  *         SYSCFG_CFGR1 SPKA          LL_SYSCFG_EnableSecurityAccess\n
  *         SYSCFG_CFGR1 SRNG          LL_SYSCFG_EnableSecurityAccess
  * @param  SecurityAccess This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_AES1
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_AES2
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_PKA
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_RNG
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableSecurityAccess(uint32_t SecurityAccess)
{
  SET_BIT(SYSCFG->SIPCR, SecurityAccess);
}

/**
  * @brief  Disable the access for security IP
  * @rmtoll SYSCFG_SIPCR SAES1         LL_SYSCFG_DisableSecurityAccess\n
  *         SYSCFG_CFGR1 SAES2         LL_SYSCFG_DisableSecurityAccess\n
  *         SYSCFG_CFGR1 SPKA          LL_SYSCFG_DisableSecurityAccess\n
  *         SYSCFG_CFGR1 SRNG          LL_SYSCFG_DisableSecurityAccess
  * @param  SecurityAccess This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_AES1
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_AES2
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_PKA
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_RNG
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableSecurityAccess(uint32_t SecurityAccess)
{
  CLEAR_BIT(SYSCFG->SIPCR, SecurityAccess);
}

/**
  * @brief  Indicate if access for security IP is enabled
  * @rmtoll SYSCFG_SIPCR SAES1         LL_SYSCFG_IsEnabledSecurityAccess\n
  *         SYSCFG_CFGR1 SAES2         LL_SYSCFG_IsEnabledSecurityAccess\n
  *         SYSCFG_CFGR1 SPKA          LL_SYSCFG_IsEnabledSecurityAccess\n
  *         SYSCFG_CFGR1 SRNG          LL_SYSCFG_IsEnabledSecurityAccess
  * @param  SecurityAccess This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_AES1
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_AES2
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_PKA
  *         @arg @ref LL_SYSCFG_SECURE_ACCESS_RNG
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledSecurityAccess(uint32_t SecurityAccess)
{
  return ((READ_BIT(SYSCFG->SIPCR, SecurityAccess) == (SecurityAccess)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup SYSTEM_LL_EF_DBGMCU DBGMCU
  * @note  DBGMCU is only accessible by Cortex M4
  *        To access on DBGMCU, Cortex M0+ need to request to the Cortex M4
  *        the action.
  * @{
  */

/**
  * @brief  Return the device identifier
  * @note   For STM32WBxxxx devices, the device ID is 0x495
  * @rmtoll DBGMCU_IDCODE DEV_ID        LL_DBGMCU_GetDeviceID
  * @retval Values between Min_Data=0x00 and Max_Data=0xFFF (ex: device ID is 0x495)
  */
__STATIC_INLINE uint32_t LL_DBGMCU_GetDeviceID(void)
{
  return (uint32_t)(READ_BIT(DBGMCU->IDCODE, DBGMCU_IDCODE_DEV_ID));
}

/**
  * @brief  Return the device revision identifier
  * @note   This field indicates the revision of the device.
  * @rmtoll DBGMCU_IDCODE REV_ID        LL_DBGMCU_GetRevisionID
  * @retval Values between Min_Data=0x00 and Max_Data=0xFFFF
  */
__STATIC_INLINE uint32_t LL_DBGMCU_GetRevisionID(void)
{
  return (uint32_t)(READ_BIT(DBGMCU->IDCODE, DBGMCU_IDCODE_REV_ID) >> DBGMCU_IDCODE_REV_ID_Pos);
}

/**
  * @brief  Enable the Debug Module during SLEEP mode
  * @rmtoll DBGMCU_CR    DBG_SLEEP     LL_DBGMCU_EnableDBGSleepMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_EnableDBGSleepMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP);
}

/**
  * @brief  Disable the Debug Module during SLEEP mode
  * @rmtoll DBGMCU_CR    DBG_SLEEP     LL_DBGMCU_DisableDBGSleepMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_DisableDBGSleepMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP);
}

/**
  * @brief  Enable the Debug Module during STOP mode
  * @rmtoll DBGMCU_CR    DBG_STOP      LL_DBGMCU_EnableDBGStopMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_EnableDBGStopMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STOP);
}

/**
  * @brief  Disable the Debug Module during STOP mode
  * @rmtoll DBGMCU_CR    DBG_STOP      LL_DBGMCU_DisableDBGStopMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_DisableDBGStopMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STOP);
}

/**
  * @brief  Enable the Debug Module during STANDBY mode
  * @rmtoll DBGMCU_CR    DBG_STANDBY   LL_DBGMCU_EnableDBGStandbyMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_EnableDBGStandbyMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STANDBY);
}

/**
  * @brief  Disable the Debug Module during STANDBY mode
  * @rmtoll DBGMCU_CR    DBG_STANDBY   LL_DBGMCU_DisableDBGStandbyMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_DisableDBGStandbyMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STANDBY);
}

/**
  * @brief  Enable the clock for Trace port
  * @rmtoll DBGMCU_CR TRACE_CLKEN         LL_DBGMCU_EnableTraceClock\n
  */
__STATIC_INLINE void LL_DBGMCU_EnableTraceClock(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_TRACE_IOEN);
}

/**
  * @brief  Disable the clock for Trace port
  * @rmtoll DBGMCU_CR TRACE_CLKEN         LL_DBGMCU_DisableTraceClock\n
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_DisableTraceClock(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_TRACE_IOEN);
}

/**
  * @brief  Indicate if the clock for Trace port is enabled
  * @rmtoll DBGMCU_CR TRACE_CLKEN         LL_DBGMCU_IsEnabledTraceClock\n
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DBGMCU_IsEnabledTraceClock(void)
{
  return ((READ_BIT(DBGMCU->CR, DBGMCU_CR_TRACE_IOEN) == (DBGMCU_CR_TRACE_IOEN)) ? 1UL : 0UL);
}

/**
  * @brief  Enable the external trigger ouput
  * @note   When enable the external trigger is output (state of bit 1),
  *         TRGIO pin is connected to TRGOUT.
  * @rmtoll DBGMCU_CR TRGOEN         LL_DBGMCU_EnableTriggerOutput\n
  */
__STATIC_INLINE void LL_DBGMCU_EnableTriggerOutput(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_TRGOEN);
}

/**
  * @brief  Disable the external trigger ouput
  * @note   When disable external trigger is input (state of bit 0),
  *         TRGIO pin is connected to TRGIN.
  * @rmtoll DBGMCU_CR TRGOEN         LL_DBGMCU_DisableTriggerOutput\n
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_DisableTriggerOutput(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_TRGOEN);
}

/**
  * @brief  Indicate if the external trigger is output or input direction
  * @note   When the external trigger is output (state of bit 1),
  *         TRGIO pin is connected to TRGOUT.
  *         When the external trigger is input (state of bit 0),
  *         TRGIO pin is connected to TRGIN.
  * @rmtoll DBGMCU_CR TRGOEN         LL_DBGMCU_EnableTriggerOutput\n
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DBGMCU_IsEnabledTriggerOutput(void)
{
  return ((READ_BIT(DBGMCU->CR, DBGMCU_CR_TRGOEN) == (DBGMCU_CR_TRGOEN)) ? 1UL : 0UL);
}

/**
  * @brief  Freeze CPU1 APB1 peripherals (group1 peripherals)
  * @rmtoll DBGMCU_APB1FZR1 DBG_xxxx_STOP  LL_DBGMCU_APB1_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_LPTIM1_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB1FZR1, Periphs);
}

/**
  * @brief  Freeze CPU2 APB1 peripherals (group1 peripherals)
  * @rmtoll DBGMCU_C2APB1FZR1 DBG_xxxx_STOP  LL_C2_DBGMCU_APB1_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_RTC_STOP
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_I2C3_STOP
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_LPTIM1_STOP
  * @retval None
  */
__STATIC_INLINE void LL_C2_DBGMCU_APB1_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->C2APB1FZR1, Periphs);
}

/**
  * @brief  Freeze CPU1 APB1 peripherals (group2 peripherals)
  * @rmtoll DBGMCU_APB1FZR2 DBG_xxxx_STOP  LL_DBGMCU_APB1_GRP2_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP2_LPTIM2_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP2_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB1FZR2, Periphs);
}

/**
  * @brief  Freeze CPU2 APB1 peripherals (group2 peripherals)
  * @rmtoll DBGMCU_C2APB1FZR2 DBG_xxxx_STOP  LL_C2_DBGMCU_APB1_GRP2_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP2_LPTIM2_STOP
  * @retval None
  */
__STATIC_INLINE void LL_C2_DBGMCU_APB1_GRP2_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->C2APB1FZR2, Periphs);
}

/**
  * @brief  Unfreeze CPU1 APB1 peripherals (group1 peripherals)
  * @rmtoll DBGMCU_APB1FZR1 DBG_xxxx_STOP  LL_DBGMCU_APB1_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_LPTIM1_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB1FZR1, Periphs);
}

/**
  * @brief  Unfreeze CPU2 APB1 peripherals (group1 peripherals)
  * @rmtoll DBGMCU_C2APB1FZR1 DBG_xxxx_STOP  LL_C2_DBGMCU_APB1_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_RTC_STOP
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_I2C3_STOP
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP1_LPTIM1_STOP
  * @retval None
  */
__STATIC_INLINE void LL_C2_DBGMCU_APB1_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->C2APB1FZR1, Periphs);
}

/**
  * @brief  Unfreeze CPU1 APB1 peripherals (group2 peripherals)
  * @rmtoll DBGMCU_APB1FZR2 DBG_xxxx_STOP  LL_DBGMCU_APB1_GRP2_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP2_LPTIM2_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP2_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB1FZR2, Periphs);
}

/**
  * @brief  Unfreeze CPU2 APB1 peripherals (group2 peripherals)
  * @rmtoll DBGMCU_C2APB1FZR2 DBG_xxxx_STOP  LL_C2_DBGMCU_APB1_GRP2_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_C2_DBGMCU_APB1_GRP2_LPTIM2_STOP
  * @retval None
  */
__STATIC_INLINE void LL_C2_DBGMCU_APB1_GRP2_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->C2APB1FZR2, Periphs);
}

/**
  * @brief  Freeze CPU1 APB2 peripherals
  * @rmtoll DBGMCU_APB2FZR DBG_TIMx_STOP  LL_DBGMCU_APB2_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM1_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM16_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM17_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB2_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB2FZR, Periphs);
}

/**
  * @brief  Freeze CPU2 APB2 peripherals
  * @rmtoll DBGMCU_C2APB2FZR DBG_TIMx_STOP  LL_C2_DBGMCU_APB2_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_C2_DBGMCU_APB2_GRP1_TIM1_STOP
  *         @arg @ref LL_C2_DBGMCU_APB2_GRP1_TIM16_STOP
  *         @arg @ref LL_C2_DBGMCU_APB2_GRP1_TIM17_STOP
  * @retval None
  */
__STATIC_INLINE void LL_C2_DBGMCU_APB2_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->C2APB2FZR, Periphs);
}

/**
  * @brief  Unfreeze CPU1 APB2 peripherals
  * @rmtoll DBGMCU_APB2FZR DBG_TIMx_STOP  LL_DBGMCU_APB2_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM1_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM16_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM17_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB2_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB2FZR, Periphs);
}

/**
  * @brief  Unfreeze CPU2 APB2 peripherals
  * @rmtoll DBGMCU_C2APB2FZR DBG_TIMx_STOP  LL_C2_DBGMCU_APB2_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_C2_DBGMCU_APB2_GRP1_TIM1_STOP
  *         @arg @ref LL_C2_DBGMCU_APB2_GRP1_TIM16_STOP
  *         @arg @ref LL_C2_DBGMCU_APB2_GRP1_TIM17_STOP
  * @retval None
  */
__STATIC_INLINE void LL_C2_DBGMCU_APB2_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->C2APB2FZR, Periphs);
}

/**
  * @}
  */

#if defined(VREFBUF)
/** @defgroup SYSTEM_LL_EF_VREFBUF VREFBUF
  * @{
  */

/**
  * @brief  Enable Internal voltage reference
  * @rmtoll VREFBUF_CSR  ENVR          LL_VREFBUF_Enable
  * @retval None
  */
__STATIC_INLINE void LL_VREFBUF_Enable(void)
{
  SET_BIT(VREFBUF->CSR, VREFBUF_CSR_ENVR);
}

/**
  * @brief  Disable Internal voltage reference
  * @rmtoll VREFBUF_CSR  ENVR          LL_VREFBUF_Disable
  * @retval None
  */
__STATIC_INLINE void LL_VREFBUF_Disable(void)
{
  CLEAR_BIT(VREFBUF->CSR, VREFBUF_CSR_ENVR);
}

/**
  * @brief  Enable high impedance (VREF+pin is high impedance)
  * @rmtoll VREFBUF_CSR  HIZ           LL_VREFBUF_EnableHIZ
  * @retval None
  */
__STATIC_INLINE void LL_VREFBUF_EnableHIZ(void)
{
  SET_BIT(VREFBUF->CSR, VREFBUF_CSR_HIZ);
}

/**
  * @brief  Disable high impedance (VREF+pin is internally connected to the voltage reference buffer output)
  * @rmtoll VREFBUF_CSR  HIZ           LL_VREFBUF_DisableHIZ
  * @retval None
  */
__STATIC_INLINE void LL_VREFBUF_DisableHIZ(void)
{
  CLEAR_BIT(VREFBUF->CSR, VREFBUF_CSR_HIZ);
}

/**
  * @brief  Set the Voltage reference scale
  * @rmtoll VREFBUF_CSR  VRS           LL_VREFBUF_SetVoltageScaling
  * @param  Scale This parameter can be one of the following values:
  *         @arg @ref LL_VREFBUF_VOLTAGE_SCALE0
  *         @arg @ref LL_VREFBUF_VOLTAGE_SCALE1
  * @retval None
  */
__STATIC_INLINE void LL_VREFBUF_SetVoltageScaling(uint32_t Scale)
{
  MODIFY_REG(VREFBUF->CSR, VREFBUF_CSR_VRS, Scale);
}

/**
  * @brief  Get the Voltage reference scale
  * @rmtoll VREFBUF_CSR  VRS           LL_VREFBUF_GetVoltageScaling
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_VREFBUF_VOLTAGE_SCALE0
  *         @arg @ref LL_VREFBUF_VOLTAGE_SCALE1
  */
__STATIC_INLINE uint32_t LL_VREFBUF_GetVoltageScaling(void)
{
  return (uint32_t)(READ_BIT(VREFBUF->CSR, VREFBUF_CSR_VRS));
}

/**
  * @brief  Check if Voltage reference buffer is ready
  * @rmtoll VREFBUF_CSR  VRR           LL_VREFBUF_IsVREFReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_VREFBUF_IsVREFReady(void)
{
  return ((READ_BIT(VREFBUF->CSR, VREFBUF_CSR_VRR) == (VREFBUF_CSR_VRR)) ? 1UL : 0UL);
}

/**
  * @brief  Get the trimming code for VREFBUF calibration
  * @rmtoll VREFBUF_CCR  TRIM          LL_VREFBUF_GetTrimming
  * @retval Between 0 and 0x3F
  */
__STATIC_INLINE uint32_t LL_VREFBUF_GetTrimming(void)
{
  return (uint32_t)(READ_BIT(VREFBUF->CCR, VREFBUF_CCR_TRIM));
}

/**
  * @brief  Set the trimming code for VREFBUF calibration (Tune the internal reference buffer voltage)
  * @rmtoll VREFBUF_CCR  TRIM          LL_VREFBUF_SetTrimming
  * @param  Value Between 0 and 0x3F
  * @retval None
  */
__STATIC_INLINE void LL_VREFBUF_SetTrimming(uint32_t Value)
{
  WRITE_REG(VREFBUF->CCR, Value);
}

/**
  * @}
  */
#endif /* VREFBUF */

/** @defgroup SYSTEM_LL_EF_FLASH FLASH
  * @{
  */

/**
  * @brief  Set FLASH Latency
  * @rmtoll FLASH_ACR    LATENCY       LL_FLASH_SetLatency
  * @param  Latency This parameter can be one of the following values:
  *         @arg @ref LL_FLASH_LATENCY_0
  *         @arg @ref LL_FLASH_LATENCY_1
  *         @arg @ref LL_FLASH_LATENCY_2
  *         @arg @ref LL_FLASH_LATENCY_3
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_SetLatency(uint32_t Latency)
{
  MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, Latency);
}

/**
  * @brief  Get FLASH Latency
  * @rmtoll FLASH_ACR    LATENCY       LL_FLASH_GetLatency
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_FLASH_LATENCY_0
  *         @arg @ref LL_FLASH_LATENCY_1
  *         @arg @ref LL_FLASH_LATENCY_2
  *         @arg @ref LL_FLASH_LATENCY_3
  */
__STATIC_INLINE uint32_t LL_FLASH_GetLatency(void)
{
  return (uint32_t)(READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY));
}

/**
  * @brief  Enable Prefetch
  * @rmtoll FLASH_ACR    PRFTEN        LL_FLASH_EnablePrefetch
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnablePrefetch(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_PRFTEN);
}

/**
  * @brief  Disable Prefetch
  * @rmtoll FLASH_ACR    PRFTEN        LL_FLASH_DisablePrefetch
  * @rmtoll FLASH_C2ACR  PRFTEN        LL_FLASH_DisablePrefetch
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisablePrefetch(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_PRFTEN);
}

/**
  * @brief  Check if Prefetch buffer is enabled
  * @rmtoll FLASH_ACR    PRFTEN        LL_FLASH_IsPrefetchEnabled
  * @rmtoll FLASH_C2ACR  C2PRFTEN      LL_FLASH_IsPrefetchEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_FLASH_IsPrefetchEnabled(void)
{
  return ((READ_BIT(FLASH->ACR, FLASH_ACR_PRFTEN) == (FLASH_ACR_PRFTEN)) ? 1UL : 0UL);
}

/**
  * @brief  Enable Instruction cache
  * @rmtoll FLASH_ACR    ICEN          LL_FLASH_EnableInstCache
  * @rmtoll FLASH_C2ACR  ICEN          LL_FLASH_EnableInstCache
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnableInstCache(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_ICEN);
}

/**
  * @brief  Disable Instruction cache
  * @rmtoll FLASH_ACR    ICEN          LL_FLASH_DisableInstCache
  * @rmtoll FLASH_C2ACR  ICEN          LL_FLASH_DisableInstCache
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisableInstCache(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_ICEN);
}

/**
  * @brief  Enable Data cache
  * @rmtoll FLASH_ACR    DCEN          LL_FLASH_EnableDataCache
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnableDataCache(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_DCEN);
}

/**
  * @brief  Disable Data cache
  * @rmtoll FLASH_ACR    DCEN          LL_FLASH_DisableDataCache
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisableDataCache(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_DCEN);
}

/**
  * @brief  Enable Instruction cache reset
  * @note  bit can be written only when the instruction cache is disabled
  * @rmtoll FLASH_ACR    ICRST         LL_FLASH_EnableInstCacheReset
  * @rmtoll FLASH_C2ACR  ICRST         LL_FLASH_EnableInstCacheReset
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnableInstCacheReset(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_ICRST);
}

/**
  * @brief  Disable Instruction cache reset
  * @rmtoll FLASH_ACR    ICRST         LL_FLASH_DisableInstCacheReset
  * @rmtoll FLASH_C2ACR  ICRST         LL_FLASH_DisableInstCacheReset
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisableInstCacheReset(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_ICRST);
}

/**
  * @brief  Enable Data cache reset
  * @note bit can be written only when the data cache is disabled
  * @rmtoll FLASH_ACR    DCRST         LL_FLASH_EnableDataCacheReset
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnableDataCacheReset(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_DCRST);
}

/**
  * @brief  Disable Data cache reset
  * @rmtoll FLASH_ACR    DCRST         LL_FLASH_DisableDataCacheReset
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisableDataCacheReset(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_DCRST);
}

/**
  * @brief  Suspend new program or erase operation request
  * @note   Any new Flash program and erase operation on both CPU side will be suspended
  *         until this bit and the same bit in Flash CPU2 access control register (FLASH_C2ACR) are
  *         cleared. The PESD bit in both the Flash status register (FLASH_SR) and Flash
  *         CPU2 status register (FLASH_C2SR) register will be set when at least one PES
  *         bit in FLASH_ACR or FLASH_C2ACR is set.
  * @rmtoll FLASH_ACR    PES         LL_FLASH_SuspendOperation
  * @rmtoll FLASH_C2ACR  PES         LL_FLASH_SuspendOperation
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_SuspendOperation(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_PES);
}

/**
  * @brief  Allow new program or erase operation request
  * @note   Any new Flash program and erase operation on both CPU side will be allowed
  *         until one of this bit or the same bit in Flash CPU2 access control register (FLASH_C2ACR) is
  *         set. The PESD bit in both the Flash status register (FLASH_SR) and Flash
  *         CPU2 status register (FLASH_C2SR) register will be clear when both PES
  *         bit in FLASH_ACR or FLASH_C2ACR is cleared.
  * @rmtoll FLASH_ACR    PES      LL_FLASH_AllowOperation
  * @rmtoll FLASH_C2ACR  PES      LL_FLASH_AllowOperation
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_AllowOperation(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_PES);
}

/**
  * @brief  Check if new program or erase operation request from CPU2 is suspended
  * @rmtoll FLASH_ACR    PES         LL_FLASH_IsOperationSuspended
  * @rmtoll FLASH_C2ACR  PES         LL_FLASH_IsOperationSuspended
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_FLASH_IsOperationSuspended(void)
{
  return ((READ_BIT(FLASH->ACR, FLASH_ACR_PES) == (FLASH_ACR_PES)) ? 1UL : 0UL);
}

/**
  * @brief  Check if new program or erase operation request from CPU1 or CPU2 is suspended
  * @rmtoll FLASH_SR      PESD         LL_FLASH_IsActiveFlag_OperationSuspended
  * @rmtoll FLASH_C2SR    PESD         LL_FLASH_IsActiveFlag_OperationSuspended
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_FLASH_IsActiveFlag_OperationSuspended(void)
{
  return ((READ_BIT(FLASH->SR, FLASH_SR_PESD) == (FLASH_SR_PESD)) ? 1UL : 0UL);
}

/**
  * @brief  Set EMPTY flag information as Flash User area empty
  * @rmtoll FLASH_ACR    EMPTY      LL_FLASH_SetEmptyFlag
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_SetEmptyFlag(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_EMPTY);
}

/**
  * @brief  Clear EMPTY flag information as Flash User area programmed
  * @rmtoll FLASH_ACR    EMPTY      LL_FLASH_ClearEmptyFlag
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_ClearEmptyFlag(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_EMPTY);
}

/**
  * @brief  Check if the EMPTY flag is set or reset
  * @rmtoll FLASH_ACR    EMPTY      LL_FLASH_IsEmptyFlag
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_FLASH_IsEmptyFlag(void)
{
  return ((READ_BIT(FLASH->ACR, FLASH_ACR_EMPTY) == FLASH_ACR_EMPTY) ? 1UL : 0UL);
}

/**
  * @brief  Get IPCC buffer base address
  * @rmtoll FLASH_IPCCBR    IPCCDBA       LL_FLASH_GetIPCCBufferAddr
  * @retval IPCC data buffer base address offset
  */
__STATIC_INLINE uint32_t LL_FLASH_GetIPCCBufferAddr(void)
{
  return (uint32_t)(READ_BIT(FLASH->IPCCBR, FLASH_IPCCBR_IPCCDBA));
}

/**
  * @brief  Get CPU2 boot reset vector
  * @rmtoll FLASH_SRRVR    SBRV       LL_FLASH_GetC2BootResetVect
  * @retval CPU2 boot reset vector
  */
__STATIC_INLINE uint32_t LL_FLASH_GetC2BootResetVect(void)
{
  return (uint32_t)(READ_BIT(FLASH->SRRVR, FLASH_SRRVR_SBRV));
}

/**
  * @brief  Return the Unique Device Number
  * @note   The 64-bit UID64 may be used by Firmware to derive BLE 48-bit Device Address EUI-48 or
  *         802.15.4 64-bit Device Address EUI-64.
  * @retval Values between Min_Data=0x00 and Max_Data=0xFFFF
  */
__STATIC_INLINE uint32_t LL_FLASH_GetUDN(void)
{
  return (uint32_t)(READ_REG(*((uint32_t *)UID64_BASE)));
}

/**
  * @brief  Return the Device ID
  * @note   The 64-bit UID64 may be used by Firmware to derive BLE 48-bit Device Address EUI-48 or
  *         802.15.4 64-bit Device Address EUI-64.
  *         For STM32WBxxxx devices, the device ID is 0x05
  * @retval Values between Min_Data=0x00 and Max_Data=0xFF (ex: Device ID is 0x05)
  */
__STATIC_INLINE uint32_t LL_FLASH_GetDeviceID(void)
{
  return (uint32_t)((READ_REG(*((uint32_t *)UID64_BASE + 1U))) & 0x000000FFU);
}

/**
  * @brief  Return the ST Company ID
  * @note   The 64-bit UID64 may be used by Firmware to derive BLE 48-bit Device Address EUI-48 or
  *         802.15.4 64-bit Device Address EUI-64.
  *         For STM32WBxxxx devices, the ST Compagny ID is 0x0080E1
  * @retval Values between Min_Data=0x00 and Max_Data=0xFFFFFF (ex: ST Compagny ID is 0x0080E1)
  */
__STATIC_INLINE uint32_t LL_FLASH_GetSTCompanyID(void)
{
  return (uint32_t)(((READ_REG(*((uint32_t *)UID64_BASE + 1U))) >> 8U ) & 0x00FFFFFFU);
}
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* defined (FLASH) || defined (SYSCFG) || defined (DBGMCU) || defined (VREFBUF) */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32WBxx_LL_SYSTEM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
