/**
  ******************************************************************************
  * @file    stm32l1xx_ll_system.h
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
      (+) Access to Routing Interfaces registers

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L1xx_LL_SYSTEM_H
#define __STM32L1xx_LL_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx.h"

/** @addtogroup STM32L1xx_LL_Driver
  * @{
  */

#if defined (FLASH) || defined (SYSCFG) || defined (DBGMCU) || defined(RI)

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
 * @brief Power-down in Run mode Flash key
 */
#define FLASH_PDKEY1                  (0x04152637U) /*!< Flash power down key1 */
#define FLASH_PDKEY2                  (0xFAFBFCFDU) /*!< Flash power down key2: used with FLASH_PDKEY1 
                                                       to unlock the RUN_PD bit in FLASH_ACR */

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
#define LL_SYSCFG_REMAP_FLASH              (0x00000000U)                                         /*<! Main Flash memory mapped at 0x00000000 */
#define LL_SYSCFG_REMAP_SYSTEMFLASH        SYSCFG_MEMRMP_MEM_MODE_0                              /*<! System Flash memory mapped at 0x00000000 */
#define LL_SYSCFG_REMAP_SRAM               (SYSCFG_MEMRMP_MEM_MODE_1 | SYSCFG_MEMRMP_MEM_MODE_0) /*<! Embedded SRAM mapped at 0x00000000 */
#if defined(FSMC_R_BASE)
#define LL_SYSCFG_REMAP_FMC                SYSCFG_MEMRMP_MEM_MODE_1                              /*<! FSMC Bank1 (NOR/PSRAM 1 and 2) mapped at 0x00000000 */
#endif /* FSMC_R_BASE */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_BOOT SYSCFG BOOT MODE
  * @{
  */
#define LL_SYSCFG_BOOTMODE_FLASH               (0x00000000U)             /*<! Main Flash memory boot mode */
#define LL_SYSCFG_BOOTMODE_SYSTEMFLASH         SYSCFG_MEMRMP_BOOT_MODE_0 /*<! System Flash memory boot mode */
#if defined(FSMC_BANK1)
#define LL_SYSCFG_BOOTMODE_FSMC                SYSCFG_MEMRMP_BOOT_MODE_1 /*<! FSMC boot mode */
#endif /* FSMC_BANK1 */
#define LL_SYSCFG_BOOTMODE_SRAM                SYSCFG_MEMRMP_BOOT_MODE   /*<! Embedded SRAM boot mode */
/**
  * @}
  */

#if defined(LCD)
/** @defgroup SYSTEM_LL_EC_LCDCAPA SYSCFG LCD capacitance connection
  * @{
  */
#define LL_SYSCFG_LCDCAPA_PB2              SYSCFG_PMC_LCD_CAPA_0 /*<! controls the connection of VLCDrail2 on PB2/LCD_VCAP2 */
#define LL_SYSCFG_LCDCAPA_PB12             SYSCFG_PMC_LCD_CAPA_1 /*<! controls the connection of VLCDrail1 on PB12/LCD_VCAP1 */
#define LL_SYSCFG_LCDCAPA_PB0              SYSCFG_PMC_LCD_CAPA_2 /*<! controls the connection of VLCDrail3 on PB0/LCD_VCAP3 */
#define LL_SYSCFG_LCDCAPA_PE11             SYSCFG_PMC_LCD_CAPA_3 /*<! controls the connection of VLCDrail1 on PE11/LCD_VCAP1 */
#define LL_SYSCFG_LCDCAPA_PE12             SYSCFG_PMC_LCD_CAPA_4 /*<! controls the connection of VLCDrail3 on PE12/LCD_VCAP3 */
/**
  * @}
  */

#endif /* LCD */

/** @defgroup SYSTEM_LL_EC_EXTI SYSCFG EXTI PORT
  * @{
  */
#define LL_SYSCFG_EXTI_PORTA               0U /*!< EXTI PORT A                        */
#define LL_SYSCFG_EXTI_PORTB               1U /*!< EXTI PORT B                        */
#define LL_SYSCFG_EXTI_PORTC               2U /*!< EXTI PORT C                        */
#define LL_SYSCFG_EXTI_PORTD               3U /*!< EXTI PORT D                        */
#if defined(GPIOE)
#define LL_SYSCFG_EXTI_PORTE               4U /*!< EXTI PORT E                        */
#endif /* GPIOE */
#if defined(GPIOF)
#define LL_SYSCFG_EXTI_PORTF               6U /*!< EXTI PORT F                        */
#endif /* GPIOF */
#if defined(GPIOG)
#define LL_SYSCFG_EXTI_PORTG               7U /*!< EXTI PORT G                        */
#endif /* GPIOG */
#define LL_SYSCFG_EXTI_PORTH               5U /*!< EXTI PORT H                        */
/**
  * @}
  */

/** @addtogroup SYSTEM_LL_EC_SYSCFG EXTI LINE
  * @{
  */
#define LL_SYSCFG_EXTI_LINE0               (uint32_t)(0x000FU << 16U | 0U)  /* EXTI_POSITION_0 | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE1               (uint32_t)(0x00F0U << 16U | 0U)  /* EXTI_POSITION_4 | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE2               (uint32_t)(0x0F00U << 16U | 0U)  /* EXTI_POSITION_8 | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE3               (uint32_t)(0xF000U << 16U | 0U)  /* EXTI_POSITION_12 | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE4               (uint32_t)(0x000FU << 16U | 1U)  /* EXTI_POSITION_0 | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE5               (uint32_t)(0x00F0U << 16U | 1U)  /* EXTI_POSITION_4 | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE6               (uint32_t)(0x0F00U << 16U | 1U)  /* EXTI_POSITION_8 | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE7               (uint32_t)(0xF000U << 16U | 1U)  /* EXTI_POSITION_12 | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE8               (uint32_t)(0x000FU << 16U | 2U)  /* EXTI_POSITION_0 | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE9               (uint32_t)(0x00F0U << 16U | 2U)  /* EXTI_POSITION_4 | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE10              (uint32_t)(0x0F00U << 16U | 2U)  /* EXTI_POSITION_8 | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE11              (uint32_t)(0xF000U << 16U | 2U)  /* EXTI_POSITION_12 | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE12              (uint32_t)(0x000FU << 16U | 3U)  /* EXTI_POSITION_0 | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE13              (uint32_t)(0x00F0U << 16U | 3U)  /* EXTI_POSITION_4 | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE14              (uint32_t)(0x0F00U << 16U | 3U)  /* EXTI_POSITION_8 | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE15              (uint32_t)(0xF000U << 16U | 3U)  /* EXTI_POSITION_12 | EXTICR[3] */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_TRACE DBGMCU TRACE Pin Assignment
  * @{
  */
#define LL_DBGMCU_TRACE_NONE               0x00000000U                                     /*!< TRACE pins not assigned (default state) */
#define LL_DBGMCU_TRACE_ASYNCH             DBGMCU_CR_TRACE_IOEN                            /*!< TRACE pin assignment for Asynchronous Mode */
#define LL_DBGMCU_TRACE_SYNCH_SIZE1        (DBGMCU_CR_TRACE_IOEN | DBGMCU_CR_TRACE_MODE_0) /*!< TRACE pin assignment for Synchronous Mode with a TRACEDATA size of 1 */
#define LL_DBGMCU_TRACE_SYNCH_SIZE2        (DBGMCU_CR_TRACE_IOEN | DBGMCU_CR_TRACE_MODE_1) /*!< TRACE pin assignment for Synchronous Mode with a TRACEDATA size of 2 */
#define LL_DBGMCU_TRACE_SYNCH_SIZE4        (DBGMCU_CR_TRACE_IOEN | DBGMCU_CR_TRACE_MODE)   /*!< TRACE pin assignment for Synchronous Mode with a TRACEDATA size of 4 */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB1_GRP1_STOP_IP DBGMCU APB1 GRP1 STOP IP
  * @{
  */
#define LL_DBGMCU_APB1_GRP1_TIM2_STOP      DBGMCU_APB1_FZ_DBG_TIM2_STOP             /*!< TIM2 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM3_STOP      DBGMCU_APB1_FZ_DBG_TIM3_STOP             /*!< TIM3 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM4_STOP      DBGMCU_APB1_FZ_DBG_TIM4_STOP             /*!< TIM4 counter stopped when core is halted */
#if defined (DBGMCU_APB1_FZ_DBG_TIM5_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM5_STOP      DBGMCU_APB1_FZ_DBG_TIM5_STOP             /*!< TIM5 counter stopped when core is halted */
#endif /* DBGMCU_APB1_FZ_DBG_TIM5_STOP */
#define LL_DBGMCU_APB1_GRP1_TIM6_STOP      DBGMCU_APB1_FZ_DBG_TIM6_STOP             /*!< TIM6 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM7_STOP      DBGMCU_APB1_FZ_DBG_TIM7_STOP             /*!< TIM7 counter stopped when core is halted */
#if defined (DBGMCU_APB1_FZ_DBG_RTC_STOP)
#define LL_DBGMCU_APB1_GRP1_RTC_STOP       DBGMCU_APB1_FZ_DBG_RTC_STOP              /*!< RTC Counter stopped when Core is halted */
#endif /* DBGMCU_APB1_FZ_DBG_RTC_STOP */
#define LL_DBGMCU_APB1_GRP1_WWDG_STOP      DBGMCU_APB1_FZ_DBG_WWDG_STOP             /*!< Debug Window Watchdog stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_IWDG_STOP      DBGMCU_APB1_FZ_DBG_IWDG_STOP             /*!< Debug Independent Watchdog stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C1_STOP      DBGMCU_APB1_FZ_DBG_I2C1_SMBUS_TIMEOUT    /*!< I2C1 SMBUS timeout mode stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C2_STOP      DBGMCU_APB1_FZ_DBG_I2C2_SMBUS_TIMEOUT    /*!< I2C2 SMBUS timeout mode stopped when Core is halted */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB2_GRP1_STOP_IP DBGMCU APB2 GRP1 STOP IP
  * @{
  */
#define LL_DBGMCU_APB2_GRP1_TIM9_STOP      DBGMCU_APB2_FZ_DBG_TIM9_STOP             /*!< TIM9 counter stopped when core is halted */
#define LL_DBGMCU_APB2_GRP1_TIM10_STOP     DBGMCU_APB2_FZ_DBG_TIM10_STOP            /*!< TIM10 counter stopped when core is halted */
#define LL_DBGMCU_APB2_GRP1_TIM11_STOP     DBGMCU_APB2_FZ_DBG_TIM11_STOP            /*!< TIM11 counter stopped when core is halted */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_TIM_SELECT RI TIM selection
  * @{
  */
#define LL_RI_TIM_SELECT_NONE              (0x00000000U)           /*!< No timer selected */
#define LL_RI_TIM_SELECT_TIM2              RI_ICR_TIM_0            /*!< Timer 2 selected */
#define LL_RI_TIM_SELECT_TIM3              RI_ICR_TIM_1            /*!< Timer 3 selected */
#define LL_RI_TIM_SELECT_TIM4              RI_ICR_TIM              /*!< Timer 4 selected */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_INPUTCAPTURE RI Input Capture number
  * @{
  */
#define LL_RI_INPUTCAPTURE_1               (RI_ICR_IC1 | RI_ICR_IC1OS) /*!< Input Capture 1 select output */
#define LL_RI_INPUTCAPTURE_2               (RI_ICR_IC2 | RI_ICR_IC2OS) /*!< Input Capture 2 select output */
#define LL_RI_INPUTCAPTURE_3               (RI_ICR_IC3 | RI_ICR_IC3OS) /*!< Input Capture 3 select output */
#define LL_RI_INPUTCAPTURE_4               (RI_ICR_IC4 | RI_ICR_IC4OS) /*!< Input Capture 4 select output */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_INPUTCAPTUREROUTING RI Input Capture Routing
  * @{
  */
                                                         /* TIMx_IC1 TIMx_IC2  TIMx_IC3  TIMx_IC4   */  
#define LL_RI_INPUTCAPTUREROUTING_0        (0x00000000U) /*!< PA0       PA1      PA2       PA3      */
#define LL_RI_INPUTCAPTUREROUTING_1        (0x00000001U) /*!< PA4       PA5      PA6       PA7      */
#define LL_RI_INPUTCAPTUREROUTING_2        (0x00000002U) /*!< PA8       PA9      PA10      PA11     */
#define LL_RI_INPUTCAPTUREROUTING_3        (0x00000003U) /*!< PA12      PA13     PA14      PA15     */
#define LL_RI_INPUTCAPTUREROUTING_4        (0x00000004U) /*!< PC0       PC1      PC2       PC3      */
#define LL_RI_INPUTCAPTUREROUTING_5        (0x00000005U) /*!< PC4       PC5      PC6       PC7      */
#define LL_RI_INPUTCAPTUREROUTING_6        (0x00000006U) /*!< PC8       PC9      PC10      PC11     */
#define LL_RI_INPUTCAPTUREROUTING_7        (0x00000007U) /*!< PC12      PC13     PC14      PC15     */
#define LL_RI_INPUTCAPTUREROUTING_8        (0x00000008U) /*!< PD0       PD1      PD2       PD3      */
#define LL_RI_INPUTCAPTUREROUTING_9        (0x00000009U) /*!< PD4       PD5      PD6       PD7      */
#define LL_RI_INPUTCAPTUREROUTING_10       (0x0000000AU) /*!< PD8       PD9      PD10      PD11     */
#define LL_RI_INPUTCAPTUREROUTING_11       (0x0000000BU) /*!< PD12      PD13     PD14      PD15     */
#if defined(GPIOE)
#define LL_RI_INPUTCAPTUREROUTING_12       (0x0000000CU) /*!< PE0       PE1      PE2       PE3      */
#define LL_RI_INPUTCAPTUREROUTING_13       (0x0000000DU) /*!< PE4       PE5      PE6       PE7      */
#define LL_RI_INPUTCAPTUREROUTING_14       (0x0000000EU) /*!< PE8       PE9      PE10      PE11     */
#define LL_RI_INPUTCAPTUREROUTING_15       (0x0000000FU) /*!< PE12      PE13     PE14      PE15     */
#endif /* GPIOE */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_IOSWITCH_LINKED_ADC RI IO Switch linked to ADC
  * @{
  */
#define LL_RI_IOSWITCH_CH0                 RI_ASCR1_CH_0    /*!< CH[3:0] GR1[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH1                 RI_ASCR1_CH_1    /*!< CH[3:0] GR1[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH2                 RI_ASCR1_CH_2    /*!< CH[3:0] GR1[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH3                 RI_ASCR1_CH_3    /*!< CH[3:0] GR1[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH4                 RI_ASCR1_CH_4    /*!< CH4: Analog switch control     */
#define LL_RI_IOSWITCH_CH5                 RI_ASCR1_CH_5    /*!< CH5: Comparator 1 analog switch*/
#define LL_RI_IOSWITCH_CH6                 RI_ASCR1_CH_6    /*!< CH[7:6] GR2[2:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH7                 RI_ASCR1_CH_7    /*!< CH[7:6] GR2[2:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH8                 RI_ASCR1_CH_8    /*!< CH[9:8] GR3[2:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH9                 RI_ASCR1_CH_9    /*!< CH[9:8] GR3[2:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH10                RI_ASCR1_CH_10   /*!< CH[13:10] GR8[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH11                RI_ASCR1_CH_11   /*!< CH[13:10] GR8[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH12                RI_ASCR1_CH_12   /*!< CH[13:10] GR8[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH13                RI_ASCR1_CH_13   /*!< CH[13:10] GR8[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH14                RI_ASCR1_CH_14   /*!< CH[15:14] GR9[2:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH15                RI_ASCR1_CH_15   /*!< CH[15:14] GR9[2:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH18                RI_ASCR1_CH_18   /*!< CH[21:18]/GR7[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH19                RI_ASCR1_CH_19   /*!< CH[21:18]/GR7[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH20                RI_ASCR1_CH_20   /*!< CH[21:18]/GR7[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH21                RI_ASCR1_CH_21   /*!< CH[21:18]/GR7[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH22                RI_ASCR1_CH_22   /*!< Analog I/O switch control of channels CH22 */
#define LL_RI_IOSWITCH_CH23                RI_ASCR1_CH_23   /*!< Analog I/O switch control of channels CH23  */
#define LL_RI_IOSWITCH_CH24                RI_ASCR1_CH_24   /*!< Analog I/O switch control of channels CH24  */
#define LL_RI_IOSWITCH_CH25                RI_ASCR1_CH_25   /*!< Analog I/O switch control of channels CH25  */
#define LL_RI_IOSWITCH_VCOMP               RI_ASCR1_VCOMP   /*!< VCOMP (ADC channel 26) is an internal switch 
                                                                 used to connect selected channel to COMP1 non inverting input */
#if defined(RI_ASCR1_CH_27)
#define LL_RI_IOSWITCH_CH27                RI_ASCR1_CH_27   /*!< CH[30:27]/GR11[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH28                RI_ASCR1_CH_28   /*!< CH[30:27]/GR11[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH29                RI_ASCR1_CH_29   /*!< CH[30:27]/GR11[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH30                RI_ASCR1_CH_30   /*!< CH[30:27]/GR11[4:1]: I/O Analog switch control */
#define LL_RI_IOSWITCH_CH31                RI_ASCR1_CH_31   /*!< CH31/GR11-5 I/O Analog switch control */
#endif /* RI_ASCR1_CH_27 */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_IOSWITCH_NOT_LINKED_ADC RI IO Switch not linked to ADC
  * @{
  */
#define LL_RI_IOSWITCH_GR10_1              RI_ASCR2_GR10_1 /*!< GR10-1 I/O analog switch control */
#define LL_RI_IOSWITCH_GR10_2              RI_ASCR2_GR10_2 /*!< GR10-2 I/O analog switch control */
#define LL_RI_IOSWITCH_GR10_3              RI_ASCR2_GR10_3 /*!< GR10-3 I/O analog switch control */
#define LL_RI_IOSWITCH_GR10_4              RI_ASCR2_GR10_4 /*!< GR10-4 I/O analog switch control */
#define LL_RI_IOSWITCH_GR6_1               RI_ASCR2_GR6_1  /*!< GR6-1 I/O analog switch control  */
#define LL_RI_IOSWITCH_GR6_2               RI_ASCR2_GR6_2  /*!< GR6-2 I/O analog switch control  */
#define LL_RI_IOSWITCH_GR5_1               RI_ASCR2_GR5_1  /*!< GR5-1 I/O analog switch control  */
#define LL_RI_IOSWITCH_GR5_2               RI_ASCR2_GR5_2  /*!< GR5-2 I/O analog switch control  */
#define LL_RI_IOSWITCH_GR5_3               RI_ASCR2_GR5_3  /*!< GR5-3 I/O analog switch control  */
#define LL_RI_IOSWITCH_GR4_1               RI_ASCR2_GR4_1  /*!< GR4-1 I/O analog switch control  */
#define LL_RI_IOSWITCH_GR4_2               RI_ASCR2_GR4_2  /*!< GR4-2 I/O analog switch control  */
#define LL_RI_IOSWITCH_GR4_3               RI_ASCR2_GR4_3  /*!< GR4-3 I/O analog switch control  */
#if defined(RI_ASCR2_CH0b)
#define LL_RI_IOSWITCH_CH0b                RI_ASCR2_CH0b   /*!< CH0b-GR03-3 I/O analog switch control  */
#if defined(RI_ASCR2_CH1b)
#define LL_RI_IOSWITCH_CH1b                RI_ASCR2_CH1b   /*!< CH1b-GR03-4 I/O analog switch control  */
#define LL_RI_IOSWITCH_CH2b                RI_ASCR2_CH2b   /*!< CH2b-GR03-5 I/O analog switch control  */
#define LL_RI_IOSWITCH_CH3b                RI_ASCR2_CH3b   /*!< CH3b-GR09-3 I/O analog switch control  */
#define LL_RI_IOSWITCH_CH6b                RI_ASCR2_CH6b   /*!< CH6b-GR09-4 I/O analog switch control  */
#define LL_RI_IOSWITCH_CH7b                RI_ASCR2_CH7b   /*!< CH7b-GR02-3 I/O analog switch control  */
#define LL_RI_IOSWITCH_CH8b                RI_ASCR2_CH8b   /*!< CH8b-GR02-4 I/O analog switch control  */
#define LL_RI_IOSWITCH_CH9b                RI_ASCR2_CH9b   /*!< CH9b-GR02-5 I/O analog switch control  */
#define LL_RI_IOSWITCH_CH10b               RI_ASCR2_CH10b  /*!< CH10b-GR07-5 I/O analog switch control */
#define LL_RI_IOSWITCH_CH11b               RI_ASCR2_CH11b  /*!< CH11b-GR07-6 I/O analog switch control */
#define LL_RI_IOSWITCH_CH12b               RI_ASCR2_CH12b  /*!< CH12b-GR07-7 I/O analog switch control */
#endif /* RI_ASCR2_CH1b */
#define LL_RI_IOSWITCH_GR6_3               RI_ASCR2_GR6_3  /*!< GR6-3 I/O analog switch control  */
#define LL_RI_IOSWITCH_GR6_4               RI_ASCR2_GR6_4  /*!< GR6-4 I/O analog switch control  */
#endif /* RI_ASCR2_CH0b */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_HSYTERESIS_PORT RI HSYTERESIS PORT
  * @{
  */
#define LL_RI_HSYTERESIS_PORT_A            0U         /*!< HYSTERESIS PORT A  */
#define LL_RI_HSYTERESIS_PORT_B            1U         /*!< HYSTERESIS PORT B  */
#define LL_RI_HSYTERESIS_PORT_C            2U         /*!< HYSTERESIS PORT C  */
#define LL_RI_HSYTERESIS_PORT_D            3U         /*!< HYSTERESIS PORT D  */
#if defined(GPIOE)
#define LL_RI_HSYTERESIS_PORT_E            4U         /*!< HYSTERESIS PORT E  */
#endif /* GPIOE */
#if defined(GPIOF)
#define LL_RI_HSYTERESIS_PORT_F            5U         /*!< HYSTERESIS PORT F  */
#endif /* GPIOF */
#if defined(GPIOG)
#define LL_RI_HSYTERESIS_PORT_G            6U         /*!< HYSTERESIS PORT G  */
#endif /* GPIOG */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_PIN RI PIN
  * @{
  */
#define LL_RI_PIN_0                        ((uint16_t)0x0001U)  /*!< Pin 0 selected */
#define LL_RI_PIN_1                        ((uint16_t)0x0002U)  /*!< Pin 1 selected */
#define LL_RI_PIN_2                        ((uint16_t)0x0004U)  /*!< Pin 2 selected */
#define LL_RI_PIN_3                        ((uint16_t)0x0008U)  /*!< Pin 3 selected */
#define LL_RI_PIN_4                        ((uint16_t)0x0010U)  /*!< Pin 4 selected */
#define LL_RI_PIN_5                        ((uint16_t)0x0020U)  /*!< Pin 5 selected */
#define LL_RI_PIN_6                        ((uint16_t)0x0040U)  /*!< Pin 6 selected */
#define LL_RI_PIN_7                        ((uint16_t)0x0080U)  /*!< Pin 7 selected */
#define LL_RI_PIN_8                        ((uint16_t)0x0100U)  /*!< Pin 8 selected */
#define LL_RI_PIN_9                        ((uint16_t)0x0200U)  /*!< Pin 9 selected */
#define LL_RI_PIN_10                       ((uint16_t)0x0400U)  /*!< Pin 10 selected */
#define LL_RI_PIN_11                       ((uint16_t)0x0800U)  /*!< Pin 11 selected */
#define LL_RI_PIN_12                       ((uint16_t)0x1000U)  /*!< Pin 12 selected */
#define LL_RI_PIN_13                       ((uint16_t)0x2000U)  /*!< Pin 13 selected */
#define LL_RI_PIN_14                       ((uint16_t)0x4000U)  /*!< Pin 14 selected */
#define LL_RI_PIN_15                       ((uint16_t)0x8000U)  /*!< Pin 15 selected */
#define LL_RI_PIN_ALL                      ((uint16_t)0xFFFFU)  /*!< All pins selected */
/**
  * @}
  */

#if defined(RI_ASMR1_PA)
/** @defgroup SYSTEM_LL_EC_PORT RI PORT
  * @{
  */
#define LL_RI_PORT_A                       0U         /*!< PORT A   */
#define LL_RI_PORT_B                       1U         /*!< PORT B   */
#define LL_RI_PORT_C                       2U         /*!< PORT C   */
#if defined(GPIOF)
#define LL_RI_PORT_F                       3U         /*!< PORT F   */
#endif /* GPIOF */
#if defined(GPIOG)
#define LL_RI_PORT_G                       4U         /*!< PORT G   */
#endif /* GPIOG */
/**
  * @}
  */

#endif /* RI_ASMR1_PA */


/** @defgroup SYSTEM_LL_EC_LATENCY FLASH LATENCY
  * @{
  */
#define LL_FLASH_LATENCY_0                 0x00000000U             /*!< FLASH Zero Latency cycle */
#define LL_FLASH_LATENCY_1                 FLASH_ACR_LATENCY       /*!< FLASH One Latency cycle */
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
  *         @arg @ref LL_SYSCFG_REMAP_FMC (*)
  *
  *         (*) value not defined in all devices
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
  *         @arg @ref LL_SYSCFG_REMAP_FMC (*)
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetRemapMemory(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_MEM_MODE));
}

/**
  * @brief  Return the boot mode as configured by user.
  * @rmtoll SYSCFG_MEMRMP BOOT_MODE     LL_SYSCFG_GetBootMode
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_SYSCFG_BOOTMODE_FLASH
  *         @arg @ref LL_SYSCFG_BOOTMODE_SYSTEMFLASH
  *         @arg @ref LL_SYSCFG_BOOTMODE_FSMC (*)
  *         @arg @ref LL_SYSCFG_BOOTMODE_SRAM
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetBootMode(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_BOOT_MODE));
}

/**
  * @brief  Enable internal pull-up on USB DP line.
  * @rmtoll SYSCFG_PMC   USB_PU        LL_SYSCFG_EnableUSBPullUp
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableUSBPullUp(void)
{
  SET_BIT(SYSCFG->PMC, SYSCFG_PMC_USB_PU);
}

/**
  * @brief  Disable internal pull-up on USB DP line.
  * @rmtoll SYSCFG_PMC   USB_PU        LL_SYSCFG_DisableUSBPullUp
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableUSBPullUp(void)
{
  CLEAR_BIT(SYSCFG->PMC, SYSCFG_PMC_USB_PU);
}

#if defined(LCD)
/**
  * @brief  Enable decoupling capacitance connection.
  * @rmtoll SYSCFG_PMC   LCD_CAPA      LL_SYSCFG_EnableLCDCapacitanceConnection
  * @param  Pin This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_LCDCAPA_PB2
  *         @arg @ref LL_SYSCFG_LCDCAPA_PB12
  *         @arg @ref LL_SYSCFG_LCDCAPA_PB0
  *         @arg @ref LL_SYSCFG_LCDCAPA_PE11
  *         @arg @ref LL_SYSCFG_LCDCAPA_PE12
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableLCDCapacitanceConnection(uint32_t Pin)
{
  SET_BIT(SYSCFG->PMC, Pin);
}

/**
  * @brief  DIsable decoupling capacitance connection.
  * @rmtoll SYSCFG_PMC   LCD_CAPA      LL_SYSCFG_DisableLCDCapacitanceConnection
  * @param  Pin This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_LCDCAPA_PB2
  *         @arg @ref LL_SYSCFG_LCDCAPA_PB12
  *         @arg @ref LL_SYSCFG_LCDCAPA_PB0
  *         @arg @ref LL_SYSCFG_LCDCAPA_PE11
  *         @arg @ref LL_SYSCFG_LCDCAPA_PE12
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableLCDCapacitanceConnection(uint32_t Pin)
{
  CLEAR_BIT(SYSCFG->PMC, Pin);
}
#endif /* LCD */

/**
  * @brief  Configure source input for the EXTI external interrupt.
  * @rmtoll SYSCFG_EXTICR1 EXTI0         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI1         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI2         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI3         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI4         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI5         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI6         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI7         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI8         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI9         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI10        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI11        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI12        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI13        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI14        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI15        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI0         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI1         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI2         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI3         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI4         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI5         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI6         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI7         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI8         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI9         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI10        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI11        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI12        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI13        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI14        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI15        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI0         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI1         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI2         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI3         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI4         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI5         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI6         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI7         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI8         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI9         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI10        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI11        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI12        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI13        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI14        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI15        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI0         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI1         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI2         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI3         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI4         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI5         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI6         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI7         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI8         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI9         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI10        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI11        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI12        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI13        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI14        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI15        LL_SYSCFG_SetEXTISource
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_EXTI_PORTA
  *         @arg @ref LL_SYSCFG_EXTI_PORTB
  *         @arg @ref LL_SYSCFG_EXTI_PORTC
  *         @arg @ref LL_SYSCFG_EXTI_PORTD
  *         @arg @ref LL_SYSCFG_EXTI_PORTE (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTF (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTG (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTH
  *
  *         (*) value not defined in all devices.
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
  MODIFY_REG(SYSCFG->EXTICR[Line & 0xFF], (Line >> 16), Port << POSITION_VAL((Line >> 16)));
}

/**
  * @brief  Get the configured defined for specific EXTI Line
  * @rmtoll SYSCFG_EXTICR1 EXTI0         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI1         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI2         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI3         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI4         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI5         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI6         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI7         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI8         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI9         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI10        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI11        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI12        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI13        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI14        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI15        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI0         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI1         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI2         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI3         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI4         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI5         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI6         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI7         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI8         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI9         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI10        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI11        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI12        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI13        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI14        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI15        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI0         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI1         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI2         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI3         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI4         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI5         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI6         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI7         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI8         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI9         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI10        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI11        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI12        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI13        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI14        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI15        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI0         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI1         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI2         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI3         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI4         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI5         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI6         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI7         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI8         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI9         LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI10        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI11        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI12        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI13        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI14        LL_SYSCFG_GetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI15        LL_SYSCFG_GetEXTISource
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
  *         @arg @ref LL_SYSCFG_EXTI_PORTE (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTF (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTG (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTH
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetEXTISource(uint32_t Line)
{
  return (uint32_t)(READ_BIT(SYSCFG->EXTICR[Line & 0xFF], (Line >> 16)) >> POSITION_VAL(Line >> 16));
}

/**
  * @}
  */

/** @defgroup SYSTEM_LL_EF_DBGMCU DBGMCU
  * @{
  */

/**
  * @brief  Return the device identifier
  * @note 0x416: Cat.1 device\n
  *       0x429: Cat.2 device\n
  *       0x427: Cat.3 device\n
  *       0x436: Cat.4 device or Cat.3 device(1)\n
  *       0x437: Cat.5 device\n
  *
  *       (1) Cat.3 devices: STM32L15xxC or STM3216xxC devices with 
  *       RPN ending with letter 'A', in WLCSP64 packages or with more then 100 pin.
  * @rmtoll DBGMCU_IDCODE DEV_ID        LL_DBGMCU_GetDeviceID
  * @retval Values between Min_Data=0x00 and Max_Data=0xFFF
  */
__STATIC_INLINE uint32_t LL_DBGMCU_GetDeviceID(void)
{
  return (uint32_t)(READ_BIT(DBGMCU->IDCODE, DBGMCU_IDCODE_DEV_ID));
}

/**
  * @brief  Return the device revision identifier
  * @note This field indicates the revision of the device.
          For example, it is read as Cat.1 RevA -> 0x1000, Cat.2 Rev Z -> 0x1018...
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
  * @brief  Set Trace pin assignment control
  * @rmtoll DBGMCU_CR    TRACE_IOEN    LL_DBGMCU_SetTracePinAssignment\n
  *         DBGMCU_CR    TRACE_MODE    LL_DBGMCU_SetTracePinAssignment
  * @param  PinAssignment This parameter can be one of the following values:
  *         @arg @ref LL_DBGMCU_TRACE_NONE
  *         @arg @ref LL_DBGMCU_TRACE_ASYNCH
  *         @arg @ref LL_DBGMCU_TRACE_SYNCH_SIZE1
  *         @arg @ref LL_DBGMCU_TRACE_SYNCH_SIZE2
  *         @arg @ref LL_DBGMCU_TRACE_SYNCH_SIZE4
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_SetTracePinAssignment(uint32_t PinAssignment)
{
  MODIFY_REG(DBGMCU->CR, DBGMCU_CR_TRACE_IOEN | DBGMCU_CR_TRACE_MODE, PinAssignment);
}

/**
  * @brief  Get Trace pin assignment control
  * @rmtoll DBGMCU_CR    TRACE_IOEN    LL_DBGMCU_GetTracePinAssignment\n
  *         DBGMCU_CR    TRACE_MODE    LL_DBGMCU_GetTracePinAssignment
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_DBGMCU_TRACE_NONE
  *         @arg @ref LL_DBGMCU_TRACE_ASYNCH
  *         @arg @ref LL_DBGMCU_TRACE_SYNCH_SIZE1
  *         @arg @ref LL_DBGMCU_TRACE_SYNCH_SIZE2
  *         @arg @ref LL_DBGMCU_TRACE_SYNCH_SIZE4
  */
__STATIC_INLINE uint32_t LL_DBGMCU_GetTracePinAssignment(void)
{
  return (uint32_t)(READ_BIT(DBGMCU->CR, DBGMCU_CR_TRACE_IOEN | DBGMCU_CR_TRACE_MODE));
}

/**
  * @brief  Freeze APB1 peripherals (group1 peripherals)
  * @rmtoll APB1_FZ      DBG_TIM2_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_TIM3_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_TIM4_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_TIM5_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_TIM6_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_TIM7_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_RTC_STOP            LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_WWDG_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_IWDG_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_I2C1_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_I2C2_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM4_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM5_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM6_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM7_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_RTC_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C2_STOP
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB1FZ, Periphs);
}

/**
  * @brief  Unfreeze APB1 peripherals (group1 peripherals)
  * @rmtoll APB1_FZ      DBG_TIM2_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_TIM3_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_TIM4_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_TIM5_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_TIM6_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_TIM7_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_RTC_STOP            LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_WWDG_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_IWDG_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_I2C1_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_I2C2_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM4_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM5_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM6_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM7_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_RTC_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C2_STOP
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB1FZ, Periphs);
}

/**
  * @brief  Freeze APB2 peripherals
  * @rmtoll APB2_FZ      DBG_TIM9_STOP   LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         APB2_FZ      DBG_TIM10_STOP  LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         APB2_FZ      DBG_TIM11_STOP  LL_DBGMCU_APB2_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM9_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM10_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM11_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB2_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB2FZ, Periphs);
}

/**
  * @brief  Unfreeze APB2 peripherals
  * @rmtoll APB2_FZ      DBG_TIM9_STOP   LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         APB2_FZ      DBG_TIM10_STOP  LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         APB2_FZ      DBG_TIM11_STOP  LL_DBGMCU_APB2_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM9_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM10_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM11_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB2_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB2FZ, Periphs);
}

/**
  * @}
  */

/** @defgroup SYSTEM_LL_EF_RI RI
  * @{
  */

/**
  * @brief  Configures the routing interface to map Input Capture x of TIMx to a selected I/O pin.
  * @rmtoll RI_ICR       IC1OS         LL_RI_SetRemapInputCapture_TIM\n
  *         RI_ICR       IC2OS         LL_RI_SetRemapInputCapture_TIM\n
  *         RI_ICR       IC3OS         LL_RI_SetRemapInputCapture_TIM\n
  *         RI_ICR       IC4OS         LL_RI_SetRemapInputCapture_TIM\n
  *         RI_ICR       TIM           LL_RI_SetRemapInputCapture_TIM\n
  *         RI_ICR       IC1           LL_RI_SetRemapInputCapture_TIM\n
  *         RI_ICR       IC2           LL_RI_SetRemapInputCapture_TIM\n
  *         RI_ICR       IC3           LL_RI_SetRemapInputCapture_TIM\n
  *         RI_ICR       IC4           LL_RI_SetRemapInputCapture_TIM
  * @param  TIM_Select This parameter can be one of the following values:
  *         @arg @ref LL_RI_TIM_SELECT_NONE
  *         @arg @ref LL_RI_TIM_SELECT_TIM2
  *         @arg @ref LL_RI_TIM_SELECT_TIM3
  *         @arg @ref LL_RI_TIM_SELECT_TIM4
  * @param  InputCaptureChannel This parameter can be one of the following values:
  *         @arg @ref LL_RI_INPUTCAPTURE_1
  *         @arg @ref LL_RI_INPUTCAPTURE_2
  *         @arg @ref LL_RI_INPUTCAPTURE_3
  *         @arg @ref LL_RI_INPUTCAPTURE_4
  * @param  Input This parameter can be one of the following values:
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_0
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_1
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_2
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_3
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_4
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_5
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_6
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_7
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_8
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_9
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_10
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_11
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_12 (*)
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_13 (*)
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_14 (*)
  *         @arg @ref LL_RI_INPUTCAPTUREROUTING_15 (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RI_SetRemapInputCapture_TIM(uint32_t TIM_Select, uint32_t InputCaptureChannel, uint32_t Input)
{
  MODIFY_REG(RI->ICR, 
             RI_ICR_TIM | (InputCaptureChannel & (RI_ICR_IC4 | RI_ICR_IC3 | RI_ICR_IC2 | RI_ICR_IC1)) | (InputCaptureChannel & (RI_ICR_IC4OS | RI_ICR_IC3OS | RI_ICR_IC2OS | RI_ICR_IC1OS)), 
             TIM_Select | (InputCaptureChannel & (RI_ICR_IC4 | RI_ICR_IC3 | RI_ICR_IC2 | RI_ICR_IC1)) | (Input << POSITION_VAL(InputCaptureChannel)));
}

/**
  * @brief  Disable the TIM Input capture remap (select the standard AF)
  * @rmtoll RI_ICR       IC1           LL_RI_DisableRemapInputCapture_TIM\n
  *         RI_ICR       IC2           LL_RI_DisableRemapInputCapture_TIM\n
  *         RI_ICR       IC3           LL_RI_DisableRemapInputCapture_TIM\n
  *         RI_ICR       IC4           LL_RI_DisableRemapInputCapture_TIM
  * @param  InputCaptureChannel This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_INPUTCAPTURE_1
  *         @arg @ref LL_RI_INPUTCAPTURE_2
  *         @arg @ref LL_RI_INPUTCAPTURE_3
  *         @arg @ref LL_RI_INPUTCAPTURE_4
  * @retval None
  */
__STATIC_INLINE void LL_RI_DisableRemapInputCapture_TIM(uint32_t InputCaptureChannel)
{
  CLEAR_BIT(RI->ICR, (InputCaptureChannel & (RI_ICR_IC4 | RI_ICR_IC3 | RI_ICR_IC2 | RI_ICR_IC1)));
}

/**
  * @brief  Close the routing interface Input Output switches linked to ADC.
  * @rmtoll RI_ASCR1     CH            LL_RI_CloseIOSwitchLinkedToADC\n
  *         RI_ASCR1     VCOMP         LL_RI_CloseIOSwitchLinkedToADC
  * @param  IOSwitch This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_IOSWITCH_CH0
  *         @arg @ref LL_RI_IOSWITCH_CH1
  *         @arg @ref LL_RI_IOSWITCH_CH2
  *         @arg @ref LL_RI_IOSWITCH_CH3
  *         @arg @ref LL_RI_IOSWITCH_CH4
  *         @arg @ref LL_RI_IOSWITCH_CH5
  *         @arg @ref LL_RI_IOSWITCH_CH6
  *         @arg @ref LL_RI_IOSWITCH_CH7
  *         @arg @ref LL_RI_IOSWITCH_CH8
  *         @arg @ref LL_RI_IOSWITCH_CH9
  *         @arg @ref LL_RI_IOSWITCH_CH10
  *         @arg @ref LL_RI_IOSWITCH_CH11
  *         @arg @ref LL_RI_IOSWITCH_CH12
  *         @arg @ref LL_RI_IOSWITCH_CH13
  *         @arg @ref LL_RI_IOSWITCH_CH14
  *         @arg @ref LL_RI_IOSWITCH_CH15
  *         @arg @ref LL_RI_IOSWITCH_CH18
  *         @arg @ref LL_RI_IOSWITCH_CH19
  *         @arg @ref LL_RI_IOSWITCH_CH20
  *         @arg @ref LL_RI_IOSWITCH_CH21
  *         @arg @ref LL_RI_IOSWITCH_CH22
  *         @arg @ref LL_RI_IOSWITCH_CH23
  *         @arg @ref LL_RI_IOSWITCH_CH24
  *         @arg @ref LL_RI_IOSWITCH_CH25
  *         @arg @ref LL_RI_IOSWITCH_VCOMP
  *         @arg @ref LL_RI_IOSWITCH_CH27 (*)
  *         @arg @ref LL_RI_IOSWITCH_CH28 (*)
  *         @arg @ref LL_RI_IOSWITCH_CH29 (*)
  *         @arg @ref LL_RI_IOSWITCH_CH30 (*)
  *         @arg @ref LL_RI_IOSWITCH_CH31 (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RI_CloseIOSwitchLinkedToADC(uint32_t IOSwitch)
{
  SET_BIT(RI->ASCR1, IOSwitch);
}

/**
  * @brief  Open the routing interface Input Output switches linked to ADC.
  * @rmtoll RI_ASCR1     CH            LL_RI_OpenIOSwitchLinkedToADC\n
  *         RI_ASCR1     VCOMP         LL_RI_OpenIOSwitchLinkedToADC
  * @param  IOSwitch This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_IOSWITCH_CH0
  *         @arg @ref LL_RI_IOSWITCH_CH1
  *         @arg @ref LL_RI_IOSWITCH_CH2
  *         @arg @ref LL_RI_IOSWITCH_CH3
  *         @arg @ref LL_RI_IOSWITCH_CH4
  *         @arg @ref LL_RI_IOSWITCH_CH5
  *         @arg @ref LL_RI_IOSWITCH_CH6
  *         @arg @ref LL_RI_IOSWITCH_CH7
  *         @arg @ref LL_RI_IOSWITCH_CH8
  *         @arg @ref LL_RI_IOSWITCH_CH9
  *         @arg @ref LL_RI_IOSWITCH_CH10
  *         @arg @ref LL_RI_IOSWITCH_CH11
  *         @arg @ref LL_RI_IOSWITCH_CH12
  *         @arg @ref LL_RI_IOSWITCH_CH13
  *         @arg @ref LL_RI_IOSWITCH_CH14
  *         @arg @ref LL_RI_IOSWITCH_CH15
  *         @arg @ref LL_RI_IOSWITCH_CH18
  *         @arg @ref LL_RI_IOSWITCH_CH19
  *         @arg @ref LL_RI_IOSWITCH_CH20
  *         @arg @ref LL_RI_IOSWITCH_CH21
  *         @arg @ref LL_RI_IOSWITCH_CH22
  *         @arg @ref LL_RI_IOSWITCH_CH23
  *         @arg @ref LL_RI_IOSWITCH_CH24
  *         @arg @ref LL_RI_IOSWITCH_CH25
  *         @arg @ref LL_RI_IOSWITCH_VCOMP
  *         @arg @ref LL_RI_IOSWITCH_CH27 (*)
  *         @arg @ref LL_RI_IOSWITCH_CH28 (*)
  *         @arg @ref LL_RI_IOSWITCH_CH29 (*)
  *         @arg @ref LL_RI_IOSWITCH_CH30 (*)
  *         @arg @ref LL_RI_IOSWITCH_CH31 (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RI_OpenIOSwitchLinkedToADC(uint32_t IOSwitch)
{
  CLEAR_BIT(RI->ASCR1, IOSwitch);
}

/**
  * @brief  Enable the switch control mode.
  * @rmtoll RI_ASCR1     SCM           LL_RI_EnableSwitchControlMode
  * @retval None
  */
__STATIC_INLINE void LL_RI_EnableSwitchControlMode(void)
{
  SET_BIT(RI->ASCR1, RI_ASCR1_SCM);
}

/**
  * @brief  Disable the switch control mode.
  * @rmtoll RI_ASCR1     SCM           LL_RI_DisableSwitchControlMode
  * @retval None
  */
__STATIC_INLINE void LL_RI_DisableSwitchControlMode(void)
{
  CLEAR_BIT(RI->ASCR1, RI_ASCR1_SCM);
}

/**
  * @brief  Close the routing interface Input Output switches not linked to ADC.
  * @rmtoll RI_ASCR2     GR10_1        LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR10_2        LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR10_3        LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR10_4        LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR6_1         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR6_2         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR5_1         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR5_2         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR5_3         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR4_1         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR4_2         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR4_3         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR4_4         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH0b          LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH1b          LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH2b          LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH3b          LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH6b          LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH7b          LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH8b          LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH9b          LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH10b         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH11b         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH12b         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR6_3         LL_RI_CloseIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR6_4         LL_RI_CloseIOSwitchNotLinkedToADC
  * @param  IOSwitch This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_IOSWITCH_GR10_1
  *         @arg @ref LL_RI_IOSWITCH_GR10_2
  *         @arg @ref LL_RI_IOSWITCH_GR10_3
  *         @arg @ref LL_RI_IOSWITCH_GR10_4
  *         @arg @ref LL_RI_IOSWITCH_GR6_1
  *         @arg @ref LL_RI_IOSWITCH_GR6_2
  *         @arg @ref LL_RI_IOSWITCH_GR5_1
  *         @arg @ref LL_RI_IOSWITCH_GR5_2
  *         @arg @ref LL_RI_IOSWITCH_GR5_3
  *         @arg @ref LL_RI_IOSWITCH_GR4_1
  *         @arg @ref LL_RI_IOSWITCH_GR4_2
  *         @arg @ref LL_RI_IOSWITCH_GR4_3
  *         @arg @ref LL_RI_IOSWITCH_CH0b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH1b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH2b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH3b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH6b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH7b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH8b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH9b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH10b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH11b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH12b (*)
  *         @arg @ref LL_RI_IOSWITCH_GR6_3
  *         @arg @ref LL_RI_IOSWITCH_GR6_4
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RI_CloseIOSwitchNotLinkedToADC(uint32_t IOSwitch)
{
  SET_BIT(RI->ASCR2, IOSwitch);
}

/**
  * @brief  Open the routing interface Input Output switches not linked to ADC.
  * @rmtoll RI_ASCR2     GR10_1        LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR10_2        LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR10_3        LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR10_4        LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR6_1         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR6_2         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR5_1         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR5_2         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR5_3         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR4_1         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR4_2         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR4_3         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR4_4         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH0b          LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH1b          LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH2b          LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH3b          LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH6b          LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH7b          LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH8b          LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH9b          LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH10b         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH11b         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     CH12b         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR6_3         LL_RI_OpenIOSwitchNotLinkedToADC\n
  *         RI_ASCR2     GR6_4         LL_RI_OpenIOSwitchNotLinkedToADC
  * @param  IOSwitch This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_IOSWITCH_GR10_1
  *         @arg @ref LL_RI_IOSWITCH_GR10_2
  *         @arg @ref LL_RI_IOSWITCH_GR10_3
  *         @arg @ref LL_RI_IOSWITCH_GR10_4
  *         @arg @ref LL_RI_IOSWITCH_GR6_1
  *         @arg @ref LL_RI_IOSWITCH_GR6_2
  *         @arg @ref LL_RI_IOSWITCH_GR5_1
  *         @arg @ref LL_RI_IOSWITCH_GR5_2
  *         @arg @ref LL_RI_IOSWITCH_GR5_3
  *         @arg @ref LL_RI_IOSWITCH_GR4_1
  *         @arg @ref LL_RI_IOSWITCH_GR4_2
  *         @arg @ref LL_RI_IOSWITCH_GR4_3
  *         @arg @ref LL_RI_IOSWITCH_CH0b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH1b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH2b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH3b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH6b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH7b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH8b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH9b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH10b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH11b (*)
  *         @arg @ref LL_RI_IOSWITCH_CH12b (*)
  *         @arg @ref LL_RI_IOSWITCH_GR6_3
  *         @arg @ref LL_RI_IOSWITCH_GR6_4
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RI_OpenIOSwitchNotLinkedToADC(uint32_t IOSwitch)
{
  CLEAR_BIT(RI->ASCR2, IOSwitch);
}

/**
  * @brief  Enable Hysteresis of the input schmitt triger of the port X
  * @rmtoll RI_HYSCR1    PA            LL_RI_EnableHysteresis\n
  *         RI_HYSCR1    PB            LL_RI_EnableHysteresis\n
  *         RI_HYSCR1    PC            LL_RI_EnableHysteresis\n
  *         RI_HYSCR1    PD            LL_RI_EnableHysteresis\n
  *         RI_HYSCR1    PE            LL_RI_EnableHysteresis\n
  *         RI_HYSCR1    PF            LL_RI_EnableHysteresis\n
  *         RI_HYSCR1    PG            LL_RI_EnableHysteresis\n
  *         RI_HYSCR2    PA            LL_RI_EnableHysteresis\n
  *         RI_HYSCR2    PB            LL_RI_EnableHysteresis\n
  *         RI_HYSCR2    PC            LL_RI_EnableHysteresis\n
  *         RI_HYSCR2    PD            LL_RI_EnableHysteresis\n
  *         RI_HYSCR2    PE            LL_RI_EnableHysteresis\n
  *         RI_HYSCR2    PF            LL_RI_EnableHysteresis\n
  *         RI_HYSCR2    PG            LL_RI_EnableHysteresis\n
  *         RI_HYSCR3    PA            LL_RI_EnableHysteresis\n
  *         RI_HYSCR3    PB            LL_RI_EnableHysteresis\n
  *         RI_HYSCR3    PC            LL_RI_EnableHysteresis\n
  *         RI_HYSCR3    PD            LL_RI_EnableHysteresis\n
  *         RI_HYSCR3    PE            LL_RI_EnableHysteresis\n
  *         RI_HYSCR3    PF            LL_RI_EnableHysteresis\n
  *         RI_HYSCR3    PG            LL_RI_EnableHysteresis\n
  *         RI_HYSCR4    PA            LL_RI_EnableHysteresis\n
  *         RI_HYSCR4    PB            LL_RI_EnableHysteresis\n
  *         RI_HYSCR4    PC            LL_RI_EnableHysteresis\n
  *         RI_HYSCR4    PD            LL_RI_EnableHysteresis\n
  *         RI_HYSCR4    PE            LL_RI_EnableHysteresis\n
  *         RI_HYSCR4    PF            LL_RI_EnableHysteresis\n
  *         RI_HYSCR4    PG            LL_RI_EnableHysteresis
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref LL_RI_HSYTERESIS_PORT_A
  *         @arg @ref LL_RI_HSYTERESIS_PORT_B
  *         @arg @ref LL_RI_HSYTERESIS_PORT_C
  *         @arg @ref LL_RI_HSYTERESIS_PORT_D
  *         @arg @ref LL_RI_HSYTERESIS_PORT_E (*)
  *         @arg @ref LL_RI_HSYTERESIS_PORT_F (*)
  *         @arg @ref LL_RI_HSYTERESIS_PORT_G (*)
  *
  *         (*) value not defined in all devices.
  * @param  Pin This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_PIN_0
  *         @arg @ref LL_RI_PIN_1
  *         @arg @ref LL_RI_PIN_2
  *         @arg @ref LL_RI_PIN_3
  *         @arg @ref LL_RI_PIN_4
  *         @arg @ref LL_RI_PIN_5
  *         @arg @ref LL_RI_PIN_6
  *         @arg @ref LL_RI_PIN_7
  *         @arg @ref LL_RI_PIN_8
  *         @arg @ref LL_RI_PIN_9
  *         @arg @ref LL_RI_PIN_10
  *         @arg @ref LL_RI_PIN_11
  *         @arg @ref LL_RI_PIN_12
  *         @arg @ref LL_RI_PIN_13
  *         @arg @ref LL_RI_PIN_14
  *         @arg @ref LL_RI_PIN_15
  *         @arg @ref LL_RI_PIN_ALL
  * @retval None
  */
__STATIC_INLINE void LL_RI_EnableHysteresis(uint32_t Port, uint32_t Pin)
{
  __IO uint32_t *reg = (__IO uint32_t *)(uint32_t)((uint32_t)(&RI->HYSCR1) + (Port >> 1)); 
  CLEAR_BIT(*reg, Pin << (16 * (Port & 1)));
}

/**
  * @brief  Disable Hysteresis of the input schmitt triger of the port X
  * @rmtoll RI_HYSCR1    PA            LL_RI_DisableHysteresis\n
  *         RI_HYSCR1    PB            LL_RI_DisableHysteresis\n
  *         RI_HYSCR1    PC            LL_RI_DisableHysteresis\n
  *         RI_HYSCR1    PD            LL_RI_DisableHysteresis\n
  *         RI_HYSCR1    PE            LL_RI_DisableHysteresis\n
  *         RI_HYSCR1    PF            LL_RI_DisableHysteresis\n
  *         RI_HYSCR1    PG            LL_RI_DisableHysteresis\n
  *         RI_HYSCR2    PA            LL_RI_DisableHysteresis\n
  *         RI_HYSCR2    PB            LL_RI_DisableHysteresis\n
  *         RI_HYSCR2    PC            LL_RI_DisableHysteresis\n
  *         RI_HYSCR2    PD            LL_RI_DisableHysteresis\n
  *         RI_HYSCR2    PE            LL_RI_DisableHysteresis\n
  *         RI_HYSCR2    PF            LL_RI_DisableHysteresis\n
  *         RI_HYSCR2    PG            LL_RI_DisableHysteresis\n
  *         RI_HYSCR3    PA            LL_RI_DisableHysteresis\n
  *         RI_HYSCR3    PB            LL_RI_DisableHysteresis\n
  *         RI_HYSCR3    PC            LL_RI_DisableHysteresis\n
  *         RI_HYSCR3    PD            LL_RI_DisableHysteresis\n
  *         RI_HYSCR3    PE            LL_RI_DisableHysteresis\n
  *         RI_HYSCR3    PF            LL_RI_DisableHysteresis\n
  *         RI_HYSCR3    PG            LL_RI_DisableHysteresis\n
  *         RI_HYSCR4    PA            LL_RI_DisableHysteresis\n
  *         RI_HYSCR4    PB            LL_RI_DisableHysteresis\n
  *         RI_HYSCR4    PC            LL_RI_DisableHysteresis\n
  *         RI_HYSCR4    PD            LL_RI_DisableHysteresis\n
  *         RI_HYSCR4    PE            LL_RI_DisableHysteresis\n
  *         RI_HYSCR4    PF            LL_RI_DisableHysteresis\n
  *         RI_HYSCR4    PG            LL_RI_DisableHysteresis
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref LL_RI_HSYTERESIS_PORT_A
  *         @arg @ref LL_RI_HSYTERESIS_PORT_B
  *         @arg @ref LL_RI_HSYTERESIS_PORT_C
  *         @arg @ref LL_RI_HSYTERESIS_PORT_D
  *         @arg @ref LL_RI_HSYTERESIS_PORT_E (*)
  *         @arg @ref LL_RI_HSYTERESIS_PORT_F (*)
  *         @arg @ref LL_RI_HSYTERESIS_PORT_G (*)
  *
  *         (*) value not defined in all devices.
  * @param  Pin This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_PIN_0
  *         @arg @ref LL_RI_PIN_1
  *         @arg @ref LL_RI_PIN_2
  *         @arg @ref LL_RI_PIN_3
  *         @arg @ref LL_RI_PIN_4
  *         @arg @ref LL_RI_PIN_5
  *         @arg @ref LL_RI_PIN_6
  *         @arg @ref LL_RI_PIN_7
  *         @arg @ref LL_RI_PIN_8
  *         @arg @ref LL_RI_PIN_9
  *         @arg @ref LL_RI_PIN_10
  *         @arg @ref LL_RI_PIN_11
  *         @arg @ref LL_RI_PIN_12
  *         @arg @ref LL_RI_PIN_13
  *         @arg @ref LL_RI_PIN_14
  *         @arg @ref LL_RI_PIN_15
  *         @arg @ref LL_RI_PIN_ALL
  * @retval None
  */
__STATIC_INLINE void LL_RI_DisableHysteresis(uint32_t Port, uint32_t Pin)
{
  __IO uint32_t *reg = (__IO uint32_t *)(uint32_t)((uint32_t)(&RI->HYSCR1) + ((Port >> 1) << 2)); 
  SET_BIT(*reg, Pin << (16 * (Port & 1)));
}

#if defined(RI_ASMR1_PA)
/**
  * @brief  Control analog switches of port X through the ADC interface or RI_ASCRx registers.
  * @rmtoll RI_ASMR1     PA            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR1     PB            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR1     PC            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR1     PF            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR1     PG            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR2     PA            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR2     PB            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR2     PC            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR2     PF            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR2     PG            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR3     PA            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR3     PB            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR3     PC            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR3     PF            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR3     PG            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR4     PA            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR4     PB            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR4     PC            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR4     PF            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR4     PG            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR5     PA            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR5     PB            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR5     PC            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR5     PF            LL_RI_ControlSwitchByADC\n
  *         RI_ASMR5     PG            LL_RI_ControlSwitchByADC
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref LL_RI_PORT_A
  *         @arg @ref LL_RI_PORT_B
  *         @arg @ref LL_RI_PORT_C
  *         @arg @ref LL_RI_PORT_F (*)
  *         @arg @ref LL_RI_PORT_G (*)
  *
  *         (*) value not defined in all devices.
  * @param  Pin This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_PIN_0
  *         @arg @ref LL_RI_PIN_1
  *         @arg @ref LL_RI_PIN_2
  *         @arg @ref LL_RI_PIN_3
  *         @arg @ref LL_RI_PIN_4
  *         @arg @ref LL_RI_PIN_5
  *         @arg @ref LL_RI_PIN_6
  *         @arg @ref LL_RI_PIN_7
  *         @arg @ref LL_RI_PIN_8
  *         @arg @ref LL_RI_PIN_9
  *         @arg @ref LL_RI_PIN_10
  *         @arg @ref LL_RI_PIN_11
  *         @arg @ref LL_RI_PIN_12
  *         @arg @ref LL_RI_PIN_13
  *         @arg @ref LL_RI_PIN_14
  *         @arg @ref LL_RI_PIN_15
  *         @arg @ref LL_RI_PIN_ALL
  * @retval None
  */
__STATIC_INLINE void LL_RI_ControlSwitchByADC(uint32_t Port, uint32_t Pin)
{
  __IO uint32_t *reg = (__IO uint32_t *)(uint32_t)((uint32_t)(&RI->ASMR1) + ((Port * 3U) << 2)); 
  CLEAR_BIT(*reg, Pin);
}
#endif /* RI_ASMR1_PA */

#if defined(RI_ASMR1_PA)
/**
  * @brief  Control analog switches of port X by the timer OC.
  * @rmtoll RI_ASMR1     PA            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR1     PB            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR1     PC            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR1     PF            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR1     PG            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR2     PA            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR2     PB            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR2     PC            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR2     PF            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR2     PG            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR3     PA            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR3     PB            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR3     PC            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR3     PF            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR3     PG            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR4     PA            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR4     PB            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR4     PC            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR4     PF            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR4     PG            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR5     PA            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR5     PB            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR5     PC            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR5     PF            LL_RI_ControlSwitchByTIM\n
  *         RI_ASMR5     PG            LL_RI_ControlSwitchByTIM
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref LL_RI_PORT_A
  *         @arg @ref LL_RI_PORT_B
  *         @arg @ref LL_RI_PORT_C
  *         @arg @ref LL_RI_PORT_F (*)
  *         @arg @ref LL_RI_PORT_G (*)
  *
  *         (*) value not defined in all devices.
  * @param  Pin This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_PIN_0
  *         @arg @ref LL_RI_PIN_1
  *         @arg @ref LL_RI_PIN_2
  *         @arg @ref LL_RI_PIN_3
  *         @arg @ref LL_RI_PIN_4
  *         @arg @ref LL_RI_PIN_5
  *         @arg @ref LL_RI_PIN_6
  *         @arg @ref LL_RI_PIN_7
  *         @arg @ref LL_RI_PIN_8
  *         @arg @ref LL_RI_PIN_9
  *         @arg @ref LL_RI_PIN_10
  *         @arg @ref LL_RI_PIN_11
  *         @arg @ref LL_RI_PIN_12
  *         @arg @ref LL_RI_PIN_13
  *         @arg @ref LL_RI_PIN_14
  *         @arg @ref LL_RI_PIN_15
  *         @arg @ref LL_RI_PIN_ALL
  * @retval None
  */
__STATIC_INLINE void LL_RI_ControlSwitchByTIM(uint32_t Port, uint32_t Pin)
{
  __IO uint32_t *reg = (__IO uint32_t *)(uint32_t)((uint32_t)(&RI->ASMR1) + ((Port * 3U) << 2)); 
  SET_BIT(*reg, Pin);
}
#endif /* RI_ASMR1_PA */

#if defined(RI_CMR1_PA)
/**
  * @brief  Mask the input of port X during the capacitive sensing acquisition.
  * @rmtoll RI_CMR1      PA            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR1      PB            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR1      PC            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR1      PF            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR1      PG            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR2      PA            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR2      PB            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR2      PC            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR2      PF            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR2      PG            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR3      PA            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR3      PB            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR3      PC            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR3      PF            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR3      PG            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR4      PA            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR4      PB            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR4      PC            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR4      PF            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR4      PG            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR5      PA            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR5      PB            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR5      PC            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR5      PF            LL_RI_MaskChannelDuringAcquisition\n
  *         RI_CMR5      PG            LL_RI_MaskChannelDuringAcquisition
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref LL_RI_PORT_A
  *         @arg @ref LL_RI_PORT_B
  *         @arg @ref LL_RI_PORT_C
  *         @arg @ref LL_RI_PORT_F (*)
  *         @arg @ref LL_RI_PORT_G (*)
  *
  *         (*) value not defined in all devices.
  * @param  Pin This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_PIN_0
  *         @arg @ref LL_RI_PIN_1
  *         @arg @ref LL_RI_PIN_2
  *         @arg @ref LL_RI_PIN_3
  *         @arg @ref LL_RI_PIN_4
  *         @arg @ref LL_RI_PIN_5
  *         @arg @ref LL_RI_PIN_6
  *         @arg @ref LL_RI_PIN_7
  *         @arg @ref LL_RI_PIN_8
  *         @arg @ref LL_RI_PIN_9
  *         @arg @ref LL_RI_PIN_10
  *         @arg @ref LL_RI_PIN_11
  *         @arg @ref LL_RI_PIN_12
  *         @arg @ref LL_RI_PIN_13
  *         @arg @ref LL_RI_PIN_14
  *         @arg @ref LL_RI_PIN_15
  *         @arg @ref LL_RI_PIN_ALL
  * @retval None
  */
__STATIC_INLINE void LL_RI_MaskChannelDuringAcquisition(uint32_t Port, uint32_t Pin)
{
  __IO uint32_t *reg = (__IO uint32_t *)(uint32_t)((uint32_t)(&RI->CMR1) + ((Port * 3U) << 2)); 
  CLEAR_BIT(*reg, Pin);
}
#endif /* RI_CMR1_PA */

#if defined(RI_CMR1_PA)
/**
  * @brief  Unmask the input of port X during the capacitive sensing acquisition.
  * @rmtoll RI_CMR1      PA            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR1      PB            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR1      PC            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR1      PF            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR1      PG            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR2      PA            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR2      PB            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR2      PC            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR2      PF            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR2      PG            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR3      PA            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR3      PB            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR3      PC            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR3      PF            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR3      PG            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR4      PA            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR4      PB            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR4      PC            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR4      PF            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR4      PG            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR5      PA            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR5      PB            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR5      PC            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR5      PF            LL_RI_UnmaskChannelDuringAcquisition\n
  *         RI_CMR5      PG            LL_RI_UnmaskChannelDuringAcquisition
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref LL_RI_PORT_A
  *         @arg @ref LL_RI_PORT_B
  *         @arg @ref LL_RI_PORT_C
  *         @arg @ref LL_RI_PORT_F (*)
  *         @arg @ref LL_RI_PORT_G (*)
  *
  *         (*) value not defined in all devices.
  * @param  Pin This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_PIN_0
  *         @arg @ref LL_RI_PIN_1
  *         @arg @ref LL_RI_PIN_2
  *         @arg @ref LL_RI_PIN_3
  *         @arg @ref LL_RI_PIN_4
  *         @arg @ref LL_RI_PIN_5
  *         @arg @ref LL_RI_PIN_6
  *         @arg @ref LL_RI_PIN_7
  *         @arg @ref LL_RI_PIN_8
  *         @arg @ref LL_RI_PIN_9
  *         @arg @ref LL_RI_PIN_10
  *         @arg @ref LL_RI_PIN_11
  *         @arg @ref LL_RI_PIN_12
  *         @arg @ref LL_RI_PIN_13
  *         @arg @ref LL_RI_PIN_14
  *         @arg @ref LL_RI_PIN_15
  *         @arg @ref LL_RI_PIN_ALL
  * @retval None
  */
__STATIC_INLINE void LL_RI_UnmaskChannelDuringAcquisition(uint32_t Port, uint32_t Pin)
{
  __IO uint32_t *reg = (__IO uint32_t *)(uint32_t)((uint32_t)(&RI->CMR1) + ((Port * 3U) << 2)); 
  SET_BIT(*reg, Pin);
}
#endif /* RI_CMR1_PA */

#if defined(RI_CICR1_PA)
/**
  * @brief  Identify channel for timer input capture
  * @rmtoll RI_CICR1     PA            LL_RI_IdentifyChannelIO\n
  *         RI_CICR1     PB            LL_RI_IdentifyChannelIO\n
  *         RI_CICR1     PC            LL_RI_IdentifyChannelIO\n
  *         RI_CICR1     PF            LL_RI_IdentifyChannelIO\n
  *         RI_CICR1     PG            LL_RI_IdentifyChannelIO\n
  *         RI_CICR2     PA            LL_RI_IdentifyChannelIO\n
  *         RI_CICR2     PB            LL_RI_IdentifyChannelIO\n
  *         RI_CICR2     PC            LL_RI_IdentifyChannelIO\n
  *         RI_CICR2     PF            LL_RI_IdentifyChannelIO\n
  *         RI_CICR2     PG            LL_RI_IdentifyChannelIO\n
  *         RI_CICR3     PA            LL_RI_IdentifyChannelIO\n
  *         RI_CICR3     PB            LL_RI_IdentifyChannelIO\n
  *         RI_CICR3     PC            LL_RI_IdentifyChannelIO\n
  *         RI_CICR3     PF            LL_RI_IdentifyChannelIO\n
  *         RI_CICR3     PG            LL_RI_IdentifyChannelIO\n
  *         RI_CICR4     PA            LL_RI_IdentifyChannelIO\n
  *         RI_CICR4     PB            LL_RI_IdentifyChannelIO\n
  *         RI_CICR4     PC            LL_RI_IdentifyChannelIO\n
  *         RI_CICR4     PF            LL_RI_IdentifyChannelIO\n
  *         RI_CICR4     PG            LL_RI_IdentifyChannelIO\n
  *         RI_CICR5     PA            LL_RI_IdentifyChannelIO\n
  *         RI_CICR5     PB            LL_RI_IdentifyChannelIO\n
  *         RI_CICR5     PC            LL_RI_IdentifyChannelIO\n
  *         RI_CICR5     PF            LL_RI_IdentifyChannelIO\n
  *         RI_CICR5     PG            LL_RI_IdentifyChannelIO
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref LL_RI_PORT_A
  *         @arg @ref LL_RI_PORT_B
  *         @arg @ref LL_RI_PORT_C
  *         @arg @ref LL_RI_PORT_F (*)
  *         @arg @ref LL_RI_PORT_G (*)
  *
  *         (*) value not defined in all devices.
  * @param  Pin This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_PIN_0
  *         @arg @ref LL_RI_PIN_1
  *         @arg @ref LL_RI_PIN_2
  *         @arg @ref LL_RI_PIN_3
  *         @arg @ref LL_RI_PIN_4
  *         @arg @ref LL_RI_PIN_5
  *         @arg @ref LL_RI_PIN_6
  *         @arg @ref LL_RI_PIN_7
  *         @arg @ref LL_RI_PIN_8
  *         @arg @ref LL_RI_PIN_9
  *         @arg @ref LL_RI_PIN_10
  *         @arg @ref LL_RI_PIN_11
  *         @arg @ref LL_RI_PIN_12
  *         @arg @ref LL_RI_PIN_13
  *         @arg @ref LL_RI_PIN_14
  *         @arg @ref LL_RI_PIN_15
  *         @arg @ref LL_RI_PIN_ALL
  * @retval None
  */
__STATIC_INLINE void LL_RI_IdentifyChannelIO(uint32_t Port, uint32_t Pin)
{
  __IO uint32_t *reg = (__IO uint32_t *)(uint32_t)((uint32_t)(&RI->CICR1) + ((Port * 3U) << 2)); 
  CLEAR_BIT(*reg, Pin);
}
#endif /* RI_CICR1_PA */

#if defined(RI_CICR1_PA)
/**
  * @brief  Identify sampling capacitor for timer input capture
  * @rmtoll RI_CICR1     PA            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR1     PB            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR1     PC            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR1     PF            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR1     PG            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR2     PA            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR2     PB            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR2     PC            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR2     PF            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR2     PG            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR3     PA            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR3     PB            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR3     PC            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR3     PF            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR3     PG            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR4     PA            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR4     PB            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR4     PC            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR4     PF            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR4     PG            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR5     PA            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR5     PB            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR5     PC            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR5     PF            LL_RI_IdentifySamplingCapacitorIO\n
  *         RI_CICR5     PG            LL_RI_IdentifySamplingCapacitorIO
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref LL_RI_PORT_A
  *         @arg @ref LL_RI_PORT_B
  *         @arg @ref LL_RI_PORT_C
  *         @arg @ref LL_RI_PORT_F (*)
  *         @arg @ref LL_RI_PORT_G (*)
  *
  *         (*) value not defined in all devices.
  * @param  Pin This parameter can be a combination of the following values:
  *         @arg @ref LL_RI_PIN_0
  *         @arg @ref LL_RI_PIN_1
  *         @arg @ref LL_RI_PIN_2
  *         @arg @ref LL_RI_PIN_3
  *         @arg @ref LL_RI_PIN_4
  *         @arg @ref LL_RI_PIN_5
  *         @arg @ref LL_RI_PIN_6
  *         @arg @ref LL_RI_PIN_7
  *         @arg @ref LL_RI_PIN_8
  *         @arg @ref LL_RI_PIN_9
  *         @arg @ref LL_RI_PIN_10
  *         @arg @ref LL_RI_PIN_11
  *         @arg @ref LL_RI_PIN_12
  *         @arg @ref LL_RI_PIN_13
  *         @arg @ref LL_RI_PIN_14
  *         @arg @ref LL_RI_PIN_15
  *         @arg @ref LL_RI_PIN_ALL
  * @retval None
  */
__STATIC_INLINE void LL_RI_IdentifySamplingCapacitorIO(uint32_t Port, uint32_t Pin)
{
  __IO uint32_t *reg = (__IO uint32_t *)(uint32_t)((uint32_t)(&RI->CICR1) + ((Port * 3U) << 2)); 
  SET_BIT(*reg, Pin);
}
#endif /* RI_CICR1_PA */

/**
  * @}
  */

/** @defgroup SYSTEM_LL_EF_FLASH FLASH
  * @{
  */

/**
  * @brief  Set FLASH Latency
  * @note   Latetency can be modified only when ACC64 is set. (through function @ref LL_FLASH_Enable64bitAccess)
  * @rmtoll FLASH_ACR    LATENCY       LL_FLASH_SetLatency
  * @param  Latency This parameter can be one of the following values:
  *         @arg @ref LL_FLASH_LATENCY_0
  *         @arg @ref LL_FLASH_LATENCY_1
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
  */
__STATIC_INLINE uint32_t LL_FLASH_GetLatency(void)
{
  return (uint32_t)(READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY));
}

/**
  * @brief  Enable Prefetch
  * @note   Prefetch can be enabled only when ACC64 is set. (through function @ref LL_FLASH_Enable64bitAccess)
  * @rmtoll FLASH_ACR    PRFTEN        LL_FLASH_EnablePrefetch
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnablePrefetch(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_PRFTEN);
}

/**
  * @brief  Disable Prefetch
  * @note   Prefetch can be disabled only when ACC64 is set. (through function @ref LL_FLASH_Enable64bitAccess)
  * @rmtoll FLASH_ACR    PRFTEN        LL_FLASH_DisablePrefetch
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisablePrefetch(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_PRFTEN);
}

/**
  * @brief  Check if Prefetch buffer is enabled
  * @rmtoll FLASH_ACR    PRFTEN        LL_FLASH_IsPrefetchEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_FLASH_IsPrefetchEnabled(void)
{
  return (READ_BIT(FLASH->ACR, FLASH_ACR_PRFTEN) == (FLASH_ACR_PRFTEN));
}

/**
  * @brief  Enable 64-bit access
  * @rmtoll FLASH_ACR    ACC64         LL_FLASH_Enable64bitAccess
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_Enable64bitAccess(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_ACC64);
}

/**
  * @brief  Disable 64-bit access
  * @rmtoll FLASH_ACR    ACC64         LL_FLASH_Disable64bitAccess
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_Disable64bitAccess(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_ACC64);
}

/**
  * @brief  Check if 64-bit access is enabled
  * @rmtoll FLASH_ACR    ACC64         LL_FLASH_Is64bitAccessEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_FLASH_Is64bitAccessEnabled(void)
{
  return (READ_BIT(FLASH->ACR, FLASH_ACR_ACC64) == (FLASH_ACR_ACC64));
}


/**
  * @brief  Enable Flash Power-down mode during run mode or Low-power run mode
  * @note Flash memory can be put in power-down mode only when the code is executed
  *       from RAM
  * @note Flash must not be accessed when power down is enabled
  * @note Flash must not be put in power-down while a program or an erase operation
  *       is on-going
  * @rmtoll FLASH_ACR    RUN_PD        LL_FLASH_EnableRunPowerDown\n
  *         FLASH_PDKEYR PDKEY1        LL_FLASH_EnableRunPowerDown\n
  *         FLASH_PDKEYR PDKEY2        LL_FLASH_EnableRunPowerDown
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnableRunPowerDown(void)
{
  /* Following values must be written consecutively to unlock the RUN_PD bit in
     FLASH_ACR */
  WRITE_REG(FLASH->PDKEYR, FLASH_PDKEY1);
  WRITE_REG(FLASH->PDKEYR, FLASH_PDKEY2);
  SET_BIT(FLASH->ACR, FLASH_ACR_RUN_PD);
}

/**
  * @brief  Disable Flash Power-down mode during run mode or Low-power run mode
  * @rmtoll FLASH_ACR    RUN_PD        LL_FLASH_DisableRunPowerDown\n
  *         FLASH_PDKEYR PDKEY1        LL_FLASH_DisableRunPowerDown\n
  *         FLASH_PDKEYR PDKEY2        LL_FLASH_DisableRunPowerDown
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisableRunPowerDown(void)
{
  /* Following values must be written consecutively to unlock the RUN_PD bit in
     FLASH_ACR */
  WRITE_REG(FLASH->PDKEYR, FLASH_PDKEY1);
  WRITE_REG(FLASH->PDKEYR, FLASH_PDKEY2);
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_RUN_PD);
}

/**
  * @brief  Enable Flash Power-down mode during Sleep or Low-power sleep mode
  * @note Flash must not be put in power-down while a program or an erase operation
  *       is on-going
  * @rmtoll FLASH_ACR    SLEEP_PD      LL_FLASH_EnableSleepPowerDown
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnableSleepPowerDown(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_SLEEP_PD);
}

/**
  * @brief  Disable Flash Power-down mode during Sleep or Low-power sleep mode
  * @rmtoll FLASH_ACR    SLEEP_PD      LL_FLASH_DisableSleepPowerDown
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisableSleepPowerDown(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_SLEEP_PD);
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

#endif /* defined (FLASH) || defined (SYSCFG) || defined (DBGMCU) || defined(RI) */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32L1xx_LL_SYSTEM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
