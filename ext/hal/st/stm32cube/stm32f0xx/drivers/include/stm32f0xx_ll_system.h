/**
  ******************************************************************************
  * @file    stm32f0xx_ll_system.h
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

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
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
#ifndef __STM32F0xx_LL_SYSTEM_H
#define __STM32F0xx_LL_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"

/** @addtogroup STM32F0xx_LL_Driver
  * @{
  */

#if defined (FLASH) || defined (SYSCFG) || defined (DBGMCU)

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

/** @defgroup SYSTEM_LL_EC_REMAP SYSCFG Remap
* @{
*/
#define LL_SYSCFG_REMAP_FLASH              (uint32_t)0x00000000U                               /*!< Main Flash memory mapped at 0x00000000 */
#define LL_SYSCFG_REMAP_SYSTEMFLASH        SYSCFG_CFGR1_MEM_MODE_0                             /*!< System Flash memory mapped at 0x00000000 */
#define LL_SYSCFG_REMAP_SRAM               (SYSCFG_CFGR1_MEM_MODE_1 | SYSCFG_CFGR1_MEM_MODE_0) /*!< Embedded SRAM mapped at 0x00000000 */
/**
  * @}
  */

#if defined(SYSCFG_CFGR1_IR_MOD)
/** @defgroup SYSTEM_LL_EC_IR_MOD SYSCFG IR Modulation
  * @{
  */
#define LL_SYSCFG_IR_MOD_TIM16       (SYSCFG_CFGR1_IR_MOD_0 & SYSCFG_CFGR1_IR_MOD_1)    /*!< Timer16 is selected as IR Modulation enveloppe source */
#define LL_SYSCFG_IR_MOD_USART1      (SYSCFG_CFGR1_IR_MOD_0)                            /*!< USART1 is selected as IR Modulation enveloppe source */
#define LL_SYSCFG_IR_MOD_USART4      (SYSCFG_CFGR1_IR_MOD_1)                            /*!< USART4 is selected as IR Modulation enveloppe source */
/**
  * @}
  */

#endif /* SYSCFG_CFGR1_IR_MOD */

#if defined(SYSCFG_CFGR1_USART1TX_DMA_RMP) || defined(SYSCFG_CFGR1_USART1RX_DMA_RMP) || defined(SYSCFG_CFGR1_USART2_DMA_RMP) || defined(SYSCFG_CFGR1_USART3_DMA_RMP)
/** @defgroup SYSTEM_LL_EC_USART1TX_RMP SYSCFG USART DMA Remap
  * @{
  */
#if defined (SYSCFG_CFGR1_USART1TX_DMA_RMP)
#define LL_SYSCFG_USART1TX_RMP_DMA1CH2     ((SYSCFG_CFGR1_USART1TX_DMA_RMP >> 8U) | (uint32_t)0x00000000U)         /*!< USART1_TX DMA request mapped on DMA channel 2U */
#define LL_SYSCFG_USART1TX_RMP_DMA1CH4     ((SYSCFG_CFGR1_USART1TX_DMA_RMP >> 8U) | SYSCFG_CFGR1_USART1TX_DMA_RMP) /*!< USART1_TX DMA request mapped on DMA channel 4U */
#endif /*SYSCFG_CFGR1_USART1TX_DMA_RMP*/
#if defined (SYSCFG_CFGR1_USART1RX_DMA_RMP)
#define LL_SYSCFG_USART1RX_RMP_DMA1CH3     ((SYSCFG_CFGR1_USART1RX_DMA_RMP >> 8U) | (uint32_t)0x00000000U)         /*!< USART1_RX DMA request mapped on DMA channel 3U */
#define LL_SYSCFG_USART1RX_RMP_DMA1CH5     ((SYSCFG_CFGR1_USART1RX_DMA_RMP >> 8U) | SYSCFG_CFGR1_USART1RX_DMA_RMP) /*!< USART1_RX DMA request mapped on DMA channel 5 */
#endif /*SYSCFG_CFGR1_USART1RX_DMA_RMP*/
#if defined (SYSCFG_CFGR1_USART2_DMA_RMP)
#define LL_SYSCFG_USART2_RMP_DMA1CH54      ((SYSCFG_CFGR1_USART2_DMA_RMP >> 8U) | (uint32_t)0x00000000U)           /*!< USART2_RX and USART2_TX DMA requests mapped on DMA channel 5 and 4U respectively */
#define LL_SYSCFG_USART2_RMP_DMA1CH67      ((SYSCFG_CFGR1_USART2_DMA_RMP >> 8U) | SYSCFG_CFGR1_USART2_DMA_RMP)     /*!< USART2_RX and USART2_TX DMA requests mapped on DMA channel 6 and 7 respectively */
#endif /*SYSCFG_CFGR1_USART2_DMA_RMP*/
#if defined (SYSCFG_CFGR1_USART3_DMA_RMP)
#define LL_SYSCFG_USART3_RMP_DMA1CH67      ((SYSCFG_CFGR1_USART3_DMA_RMP >> 8U) | (uint32_t)0x00000000U)           /*!< USART3_RX and USART3_TX DMA requests mapped on DMA channel 6 and 7 respectively */
#define LL_SYSCFG_USART3_RMP_DMA1CH32      ((SYSCFG_CFGR1_USART3_DMA_RMP >> 8U) | SYSCFG_CFGR1_USART3_DMA_RMP)     /*!< USART3_RX and USART3_TX DMA requests mapped on DMA channel 3U and 2U respectively */
#endif /* SYSCFG_CFGR1_USART3_DMA_RMP */
/**
  * @}
  */
#endif /* SYSCFG_CFGR1_USART1TX_DMA_RMP || SYSCFG_CFGR1_USART1RX_DMA_RMP || SYSCFG_CFGR1_USART2_DMA_RMP || SYSCFG_CFGR1_USART3_DMA_RMP */

#if defined (SYSCFG_CFGR1_SPI2_DMA_RMP)
/** @defgroup SYSTEM_LL_EC_SPI2_RMP_DMA1 SYSCFG SPI2 DMA Remap
  * @{
  */
#define LL_SYSCFG_SPI2_RMP_DMA1_CH45       (uint32_t)0x00000000U      /*!< SPI2_RX and SPI2_TX DMA requests mapped on DMA channel 4U and 5 respectively */
#define LL_SYSCFG_SPI2_RMP_DMA1_CH67       SYSCFG_CFGR1_SPI2_DMA_RMP  /*!< SPI2_RX and SPI2_TX DMA requests mapped on DMA channel 6 and 7 respectively */
/**
  * @}
  */

#endif /*SYSCFG_CFGR1_SPI2_DMA_RMP*/

#if defined (SYSCFG_CFGR1_I2C1_DMA_RMP)
/** @defgroup SYSTEM_LL_EC_I2C1_RMP_DMA1 SYSCFG I2C1 DMA Remap
  * @{
  */
#define LL_SYSCFG_I2C1_RMP_DMA1_CH32       (uint32_t)0x00000000U      /*!< I2C1_RX and I2C1_TX DMA requests mapped on DMA channel 3U and 2U respectively */
#define LL_SYSCFG_I2C1_RMP_DMA1_CH76       SYSCFG_CFGR1_I2C1_DMA_RMP  /*!< I2C1_RX and I2C1_TX DMA requests mapped on DMA channel 7 and 6 respectively */
/**
  * @}
  */

#endif /*SYSCFG_CFGR1_I2C1_DMA_RMP*/

#if defined(SYSCFG_CFGR1_ADC_DMA_RMP)
/** @defgroup SYSTEM_LL_EC_ADC1_RMP_DMA1 SYSCFG ADC1 DMA Remap
  * @{
  */
#define LL_SYSCFG_ADC1_RMP_DMA1_CH1        (uint32_t)0x00000000U     /*!< ADC DMA request mapped on DMA channel 1U */
#define LL_SYSCFG_ADC1_RMP_DMA1_CH2        SYSCFG_CFGR1_ADC_DMA_RMP  /*!< ADC DMA request mapped on DMA channel 2U */
/**
  * @}
  */

#endif /* SYSCFG_CFGR1_ADC_DMA_RMP */

#if defined(SYSCFG_CFGR1_TIM16_DMA_RMP) || defined(SYSCFG_CFGR1_TIM17_DMA_RMP) || defined(SYSCFG_CFGR1_TIM1_DMA_RMP) || defined(SYSCFG_CFGR1_TIM2_DMA_RMP) || defined(SYSCFG_CFGR1_TIM3_DMA_RMP)
/** @defgroup SYSTEM_LL_EC_TIM16_RMP_DMA1 SYSCFG TIM DMA Remap
  * @{
  */
#if defined(SYSCFG_CFGR1_TIM16_DMA_RMP)
#if defined (SYSCFG_CFGR1_TIM16_DMA_RMP2)
#define LL_SYSCFG_TIM16_RMP_DMA1_CH3       (((SYSCFG_CFGR1_TIM16_DMA_RMP | SYSCFG_CFGR1_TIM16_DMA_RMP2) >> 8U) | (uint32_t)0x00000000U)        /*!< TIM16_CH1 and TIM16_UP DMA requests mapped on DMA channel 3 */
#define LL_SYSCFG_TIM16_RMP_DMA1_CH4       (((SYSCFG_CFGR1_TIM16_DMA_RMP | SYSCFG_CFGR1_TIM16_DMA_RMP2) >> 8U) | SYSCFG_CFGR1_TIM16_DMA_RMP)   /*!< TIM16_CH1 and TIM16_UP DMA requests mapped on DMA channel 4 */
#define LL_SYSCFG_TIM16_RMP_DMA1_CH6       ((SYSCFG_CFGR1_TIM16_DMA_RMP2 >> 8U) | SYSCFG_CFGR1_TIM16_DMA_RMP2) /*!< TIM16_CH1 and TIM16_UP DMA requests mapped on DMA channel 6 */
#else
#define LL_SYSCFG_TIM16_RMP_DMA1_CH3       ((SYSCFG_CFGR1_TIM16_DMA_RMP >> 8U) | (uint32_t)0x00000000U)        /*!< TIM16_CH1 and TIM16_UP DMA requests mapped on DMA channel 3 */
#define LL_SYSCFG_TIM16_RMP_DMA1_CH4       ((SYSCFG_CFGR1_TIM16_DMA_RMP >> 8U) | SYSCFG_CFGR1_TIM16_DMA_RMP)   /*!< TIM16_CH1 and TIM16_UP DMA requests mapped on DMA channel 4 */
#endif /* SYSCFG_CFGR1_TIM16_DMA_RMP2 */
#endif /* SYSCFG_CFGR1_TIM16_DMA_RMP */
#if defined(SYSCFG_CFGR1_TIM17_DMA_RMP)
#if defined (SYSCFG_CFGR1_TIM17_DMA_RMP2)
#define LL_SYSCFG_TIM17_RMP_DMA1_CH1       (((SYSCFG_CFGR1_TIM17_DMA_RMP | SYSCFG_CFGR1_TIM17_DMA_RMP2) >> 8U) | (uint32_t)0x00000000U)        /*!< TIM17_CH1 and TIM17_UP DMA requests mapped on DMA channel 1 */
#define LL_SYSCFG_TIM17_RMP_DMA1_CH2       (((SYSCFG_CFGR1_TIM17_DMA_RMP | SYSCFG_CFGR1_TIM17_DMA_RMP2) >> 8U) | SYSCFG_CFGR1_TIM17_DMA_RMP)   /*!< TIM17_CH1 and TIM17_UP DMA requests mapped on DMA channel 2 */
#define LL_SYSCFG_TIM17_RMP_DMA1_CH7       ((SYSCFG_CFGR1_TIM17_DMA_RMP2 >> 8U) | SYSCFG_CFGR1_TIM17_DMA_RMP2) /*!< TIM17_CH1 and TIM17_UP DMA requests mapped on DMA channel 7 */
#else
#define LL_SYSCFG_TIM17_RMP_DMA1_CH1       ((SYSCFG_CFGR1_TIM17_DMA_RMP >> 8U) | (uint32_t)0x00000000U)        /*!< TIM17_CH1 and TIM17_UP DMA requests mapped on DMA channel 1 */
#define LL_SYSCFG_TIM17_RMP_DMA1_CH2       ((SYSCFG_CFGR1_TIM17_DMA_RMP >> 8U) | SYSCFG_CFGR1_TIM17_DMA_RMP)   /*!< TIM17_CH1 and TIM17_UP DMA requests mapped on DMA channel 2 */
#endif /* SYSCFG_CFGR1_TIM17_DMA_RMP2 */
#endif /* SYSCFG_CFGR1_TIM17_DMA_RMP */
#if defined (SYSCFG_CFGR1_TIM1_DMA_RMP)
#define LL_SYSCFG_TIM1_RMP_DMA1_CH234      ((SYSCFG_CFGR1_TIM1_DMA_RMP >> 8U) | (uint32_t)0x00000000U)         /*!< TIM1_CH1, TIM1_CH2 and TIM1_CH3 DMA requests mapped on DMAchannel 2, 3 and 4 respectively */
#define LL_SYSCFG_TIM1_RMP_DMA1_CH6        ((SYSCFG_CFGR1_TIM1_DMA_RMP >> 8U) | SYSCFG_CFGR1_TIM1_DMA_RMP)     /*!< TIM1_CH1, TIM1_CH2 and TIM1_CH3 DMA requests mapped on DMA channel 6 */
#endif /*SYSCFG_CFGR1_TIM1_DMA_RMP*/
#if defined (SYSCFG_CFGR1_TIM2_DMA_RMP)
#define LL_SYSCFG_TIM2_RMP_DMA1_CH34       ((SYSCFG_CFGR1_TIM2_DMA_RMP >> 8U) | (uint32_t)0x00000000U)          /*!< TIM2_CH2 and TIM2_CH4 DMA requests mapped on DMA channel 3 and 4 respectively */
#define LL_SYSCFG_TIM2_RMP_DMA1_CH7        ((SYSCFG_CFGR1_TIM2_DMA_RMP >> 8U) | SYSCFG_CFGR1_TIM2_DMA_RMP)      /*!< TIM2_CH2 and TIM2_CH4 DMA requests mapped on DMA channel 7 */
#endif /*SYSCFG_CFGR1_TIM2_DMA_RMP*/
#if defined (SYSCFG_CFGR1_TIM3_DMA_RMP)
#define LL_SYSCFG_TIM3_RMP_DMA1_CH4        ((SYSCFG_CFGR1_TIM3_DMA_RMP >> 8U) | (uint32_t)0x00000000U)          /*!< TIM3_CH1 and TIM3_TRIG DMA requests mapped on DMA channel 4 */
#define LL_SYSCFG_TIM3_RMP_DMA1_CH6        ((SYSCFG_CFGR1_TIM3_DMA_RMP >> 8U) | SYSCFG_CFGR1_TIM3_DMA_RMP)      /*!< TIM3_CH1 and TIM3_TRIG DMA requests mapped on DMA channel 6 */
#endif /*SYSCFG_CFGR1_TIM3_DMA_RMP*/
/**
  * @}
  */

#endif /* SYSCFG_CFGR1_TIM16_DMA_RMP || SYSCFG_CFGR1_TIM17_DMA_RMP || SYSCFG_CFGR1_TIM1_DMA_RMP || SYSCFG_CFGR1_TIM2_DMA_RMP || SYSCFG_CFGR1_TIM3_DMA_RMP */

/** @defgroup SYSTEM_LL_EC_I2C_FASTMODEPLUS SYSCFG I2C FASTMODEPLUS
  * @{
  */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB6     SYSCFG_CFGR1_I2C_FMP_PB6  /*!< I2C PB6 Fast mode plus */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB7     SYSCFG_CFGR1_I2C_FMP_PB7  /*!< I2C PB7 Fast mode plus */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB8     SYSCFG_CFGR1_I2C_FMP_PB8  /*!< I2C PB8 Fast mode plus */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB9     SYSCFG_CFGR1_I2C_FMP_PB9  /*!< I2C PB9 Fast mode plus */
#if defined(SYSCFG_CFGR1_I2C_FMP_I2C1)
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C1    SYSCFG_CFGR1_I2C_FMP_I2C1 /*!< Enable Fast Mode Plus on PB10, PB11, PF6 and PF7  */
#endif /*SYSCFG_CFGR1_I2C_FMP_I2C1*/
#if defined(SYSCFG_CFGR1_I2C_FMP_I2C2)
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C2    SYSCFG_CFGR1_I2C_FMP_I2C2 /*!< Enable I2C2 Fast mode plus  */
#endif /*SYSCFG_CFGR1_I2C_FMP_I2C2*/
#if defined(SYSCFG_CFGR1_I2C_FMP_PA9)
#define LL_SYSCFG_I2C_FASTMODEPLUS_PA9     SYSCFG_CFGR1_I2C_FMP_PA9 /*!< Enable Fast Mode Plus on PA9  */
#endif /*SYSCFG_CFGR1_I2C_FMP_PA9*/
#if defined(SYSCFG_CFGR1_I2C_FMP_PA10)
#define LL_SYSCFG_I2C_FASTMODEPLUS_PA10    SYSCFG_CFGR1_I2C_FMP_PA10 /*!< Enable Fast Mode Plus on PA10 */
#endif /*SYSCFG_CFGR1_I2C_FMP_PA10*/
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_EXTI_PORT SYSCFG EXTI PORT
  * @{
  */
#define LL_SYSCFG_EXTI_PORTA               (uint32_t)0U               /*!< EXTI PORT A */
#define LL_SYSCFG_EXTI_PORTB               (uint32_t)1U               /*!< EXTI PORT B */
#define LL_SYSCFG_EXTI_PORTC               (uint32_t)2U               /*!< EXTI PORT C */
#if defined(GPIOD_BASE)
#define LL_SYSCFG_EXTI_PORTD               (uint32_t)3U               /*!< EXTI PORT D */
#endif /*GPIOD_BASE*/
#if defined(GPIOE_BASE)
#define LL_SYSCFG_EXTI_PORTE               (uint32_t)4U               /*!< EXTI PORT E */
#endif /*GPIOE_BASE*/
#define LL_SYSCFG_EXTI_PORTF               (uint32_t)5U               /*!< EXTI PORT F */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_EXTI_LINE SYSCFG EXTI LINE
  * @{
  */
#define LL_SYSCFG_EXTI_LINE0               (uint32_t)(0U << 16U | 0U)  /*!< EXTI_POSITION_0  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE1               (uint32_t)(4U << 16U | 0U)  /*!< EXTI_POSITION_4  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE2               (uint32_t)(8U << 16U | 0U)  /*!< EXTI_POSITION_8  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE3               (uint32_t)(12U << 16U | 0U) /*!< EXTI_POSITION_12 | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE4               (uint32_t)(0U << 16U | 1U)  /*!< EXTI_POSITION_0  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE5               (uint32_t)(4U << 16U | 1U)  /*!< EXTI_POSITION_4  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE6               (uint32_t)(8U << 16U | 1U)  /*!< EXTI_POSITION_8  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE7               (uint32_t)(12U << 16U | 1U) /*!< EXTI_POSITION_12 | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE8               (uint32_t)(0U << 16U | 2U)  /*!< EXTI_POSITION_0  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE9               (uint32_t)(4U << 16U | 2U)  /*!< EXTI_POSITION_4  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE10              (uint32_t)(8U << 16U | 2U)  /*!< EXTI_POSITION_8  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE11              (uint32_t)(12U << 16U | 2U) /*!< EXTI_POSITION_12 | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE12              (uint32_t)(0U << 16U | 3U)  /*!< EXTI_POSITION_0  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE13              (uint32_t)(4U << 16U | 3U)  /*!< EXTI_POSITION_4  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE14              (uint32_t)(8U << 16U | 3U)  /*!< EXTI_POSITION_8  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE15              (uint32_t)(12U << 16U | 3U) /*!< EXTI_POSITION_12 | EXTICR[3] */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_TIMBREAK SYSCFG TIMER BREAK
  * @{
  */
#if defined(SYSCFG_CFGR2_PVD_LOCK)
#define LL_SYSCFG_TIMBREAK_PVD             SYSCFG_CFGR2_PVD_LOCK  /*!< Enables and locks the PVD connection
                                                                       with TIM1/15/16U/17 Break Input and also
                                                                       the PVDE and PLS bits of the Power Control Interface */
#endif /*SYSCFG_CFGR2_PVD_LOCK*/
#define LL_SYSCFG_TIMBREAK_SRAM_PARITY     SYSCFG_CFGR2_SRAM_PARITY_LOCK   /*!< Enables and locks the SRAM_PARITY error signal
                                                                                with Break Input of TIM1/15/16/17 */
#define LL_SYSCFG_TIMBREAK_LOCKUP          SYSCFG_CFGR2_LOCKUP_LOCK   /*!< Enables and locks the LOCKUP (Hardfault) output of
                                                                           CortexM0 with Break Input of TIM1/15/16/17 */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB1_GRP1_STOP_IP  DBGMCU APB1 GRP1 STOP IP
  * @{
  */
#if defined(DBGMCU_APB1_FZ_DBG_TIM2_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM2_STOP      DBGMCU_APB1_FZ_DBG_TIM2_STOP        /*!< TIM2 counter stopped when core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_TIM2_STOP*/
#define LL_DBGMCU_APB1_GRP1_TIM3_STOP      DBGMCU_APB1_FZ_DBG_TIM3_STOP        /*!< TIM3 counter stopped when core is halted */
#if defined(DBGMCU_APB1_FZ_DBG_TIM6_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM6_STOP      DBGMCU_APB1_FZ_DBG_TIM6_STOP        /*!< TIM6 counter stopped when core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_TIM6_STOP*/
#if defined(DBGMCU_APB1_FZ_DBG_TIM7_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM7_STOP      DBGMCU_APB1_FZ_DBG_TIM7_STOP        /*!< TIM7 counter stopped when core is halted  */
#endif /*DBGMCU_APB1_FZ_DBG_TIM7_STOP*/
#define LL_DBGMCU_APB1_GRP1_TIM14_STOP     DBGMCU_APB1_FZ_DBG_TIM14_STOP       /*!< TIM14 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_RTC_STOP       DBGMCU_APB1_FZ_DBG_RTC_STOP         /*!< RTC Calendar frozen when core is halted */
#define LL_DBGMCU_APB1_GRP1_WWDG_STOP      DBGMCU_APB1_FZ_DBG_WWDG_STOP        /*!< Debug Window Watchdog stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_IWDG_STOP      DBGMCU_APB1_FZ_DBG_IWDG_STOP        /*!< Debug Independent Watchdog stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C1_STOP      DBGMCU_APB1_FZ_DBG_I2C1_SMBUS_TIMEOUT /*!< I2C1 SMBUS timeout mode stopped when Core is halted */
#if defined(DBGMCU_APB1_FZ_DBG_CAN_STOP)
#define LL_DBGMCU_APB1_GRP1_CAN_STOP       DBGMCU_APB1_FZ_DBG_CAN_STOP         /*!< CAN debug stopped when Core is halted  */
#endif /*DBGMCU_APB1_FZ_DBG_CAN_STOP*/
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB1 GRP2_STOP_IP DBGMCU APB1 GRP2 STOP IP
  * @{
  */
#define LL_DBGMCU_APB1_GRP2_TIM1_STOP      DBGMCU_APB2_FZ_DBG_TIM1_STOP        /*!< TIM1 counter stopped when core is halted */
#if defined(DBGMCU_APB2_FZ_DBG_TIM15_STOP)
#define LL_DBGMCU_APB1_GRP2_TIM15_STOP     DBGMCU_APB2_FZ_DBG_TIM15_STOP       /*!< TIM15 counter stopped when core is halted  */
#endif /*DBGMCU_APB2_FZ_DBG_TIM15_STOP*/
#define LL_DBGMCU_APB1_GRP2_TIM16_STOP     DBGMCU_APB2_FZ_DBG_TIM16_STOP       /*!< TIM16 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP2_TIM17_STOP     DBGMCU_APB2_FZ_DBG_TIM17_STOP       /*!< TIM17 counter stopped when core is halted */
/**
  * @}
  */

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
  * @rmtoll SYSCFG_CFGR1 MEM_MODE      LL_SYSCFG_SetRemapMemory
  * @param  Memory This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_REMAP_FLASH
  *         @arg @ref LL_SYSCFG_REMAP_SYSTEMFLASH
  *         @arg @ref LL_SYSCFG_REMAP_SRAM
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapMemory(uint32_t Memory)
{
  MODIFY_REG(SYSCFG->CFGR1, SYSCFG_CFGR1_MEM_MODE, Memory);
}

/**
  * @brief  Get memory mapping at address 0x00000000
  * @rmtoll SYSCFG_CFGR1 MEM_MODE      LL_SYSCFG_GetRemapMemory
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_SYSCFG_REMAP_FLASH
  *         @arg @ref LL_SYSCFG_REMAP_SYSTEMFLASH
  *         @arg @ref LL_SYSCFG_REMAP_SRAM
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetRemapMemory(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_MEM_MODE));
}

#if defined(SYSCFG_CFGR1_IR_MOD)
/**
  * @brief  Set IR Modulation Envelope signal source.
  * @rmtoll SYSCFG_CFGR1 IR_MOD  LL_SYSCFG_SetIRModEnvelopeSignal
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_IR_MOD_TIM16
  *         @arg @ref LL_SYSCFG_IR_MOD_USART1
  *         @arg @ref LL_SYSCFG_IR_MOD_USART4
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetIRModEnvelopeSignal(uint32_t Source)
{
  MODIFY_REG(SYSCFG->CFGR1, SYSCFG_CFGR1_IR_MOD, Source);
}

/**
  * @brief  Get IR Modulation Envelope signal source.
  * @rmtoll SYSCFG_CFGR1 IR_MOD  LL_SYSCFG_GetIRModEnvelopeSignal
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_SYSCFG_IR_MOD_TIM16
  *         @arg @ref LL_SYSCFG_IR_MOD_USART1
  *         @arg @ref LL_SYSCFG_IR_MOD_USART4
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetIRModEnvelopeSignal(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_IR_MOD));
}
#endif /* SYSCFG_CFGR1_IR_MOD */

#if defined(SYSCFG_CFGR1_USART1TX_DMA_RMP) || defined(SYSCFG_CFGR1_USART1RX_DMA_RMP) || defined(SYSCFG_CFGR1_USART2_DMA_RMP) || defined(SYSCFG_CFGR1_USART3_DMA_RMP)
/**
  * @brief  Set DMA request remapping bits for USART
  * @rmtoll SYSCFG_CFGR1 USART1TX_DMA_RMP  LL_SYSCFG_SetRemapDMA_USART\n
  *         SYSCFG_CFGR1 USART1RX_DMA_RMP  LL_SYSCFG_SetRemapDMA_USART\n
  *         SYSCFG_CFGR1 USART2_DMA_RMP  LL_SYSCFG_SetRemapDMA_USART\n
  *         SYSCFG_CFGR1 USART3_DMA_RMP  LL_SYSCFG_SetRemapDMA_USART
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_USART1TX_RMP_DMA1CH2 (*)
  *         @arg @ref LL_SYSCFG_USART1TX_RMP_DMA1CH4 (*)
  *         @arg @ref LL_SYSCFG_USART1RX_RMP_DMA1CH3 (*)
  *         @arg @ref LL_SYSCFG_USART1RX_RMP_DMA1CH5 (*)
  *         @arg @ref LL_SYSCFG_USART2_RMP_DMA1CH54 (*)
  *         @arg @ref LL_SYSCFG_USART2_RMP_DMA1CH67 (*)
  *         @arg @ref LL_SYSCFG_USART3_RMP_DMA1CH67 (*)
  *         @arg @ref LL_SYSCFG_USART3_RMP_DMA1CH32 (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapDMA_USART(uint32_t Remap)
{
  MODIFY_REG(SYSCFG->CFGR1, (Remap & 0x00FF00FFU) << 8U, (Remap & 0xFF00FF00U));
}
#endif /* SYSCFG_CFGR1_USART1TX_DMA_RMP || SYSCFG_CFGR1_USART1RX_DMA_RMP || SYSCFG_CFGR1_USART2_DMA_RMP || SYSCFG_CFGR1_USART3_DMA_RMP */

#if defined(SYSCFG_CFGR1_SPI2_DMA_RMP)
/**
  * @brief  Set DMA request remapping bits for SPI
  * @rmtoll SYSCFG_CFGR1 SPI2_DMA_RMP  LL_SYSCFG_SetRemapDMA_SPI
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_SPI2_RMP_DMA1_CH45
  *         @arg @ref LL_SYSCFG_SPI2_RMP_DMA1_CH67
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapDMA_SPI(uint32_t Remap)
{
  MODIFY_REG(SYSCFG->CFGR1, SYSCFG_CFGR1_SPI2_DMA_RMP, Remap);
}
#endif /* SYSCFG_CFGR1_SPI2_DMA_RMP */

#if defined(SYSCFG_CFGR1_I2C1_DMA_RMP)
/**
  * @brief  Set DMA request remapping bits for I2C
  * @rmtoll SYSCFG_CFGR1 I2C1_DMA_RMP  LL_SYSCFG_SetRemapDMA_I2C
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_I2C1_RMP_DMA1_CH32
  *         @arg @ref LL_SYSCFG_I2C1_RMP_DMA1_CH76
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapDMA_I2C(uint32_t Remap)
{
  MODIFY_REG(SYSCFG->CFGR1, SYSCFG_CFGR1_I2C1_DMA_RMP, Remap);
}
#endif /* SYSCFG_CFGR1_I2C1_DMA_RMP */

#if defined(SYSCFG_CFGR1_ADC_DMA_RMP)
/**
  * @brief  Set DMA request remapping bits for ADC
  * @rmtoll SYSCFG_CFGR1 ADC_DMA_RMP   LL_SYSCFG_SetRemapDMA_ADC
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_ADC1_RMP_DMA1_CH1
  *         @arg @ref LL_SYSCFG_ADC1_RMP_DMA1_CH2
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapDMA_ADC(uint32_t Remap)
{
  MODIFY_REG(SYSCFG->CFGR1, SYSCFG_CFGR1_ADC_DMA_RMP, Remap);
}
#endif /* SYSCFG_CFGR1_ADC_DMA_RMP */

#if defined(SYSCFG_CFGR1_TIM16_DMA_RMP) || defined(SYSCFG_CFGR1_TIM17_DMA_RMP) || defined(SYSCFG_CFGR1_TIM1_DMA_RMP) || defined(SYSCFG_CFGR1_TIM2_DMA_RMP) || defined(SYSCFG_CFGR1_TIM3_DMA_RMP)
/**
  * @brief  Set DMA request remapping bits for TIM
  * @rmtoll SYSCFG_CFGR1 TIM16_DMA_RMP  LL_SYSCFG_SetRemapDMA_TIM\n
  *         SYSCFG_CFGR1 TIM17_DMA_RMP  LL_SYSCFG_SetRemapDMA_TIM\n
  *         SYSCFG_CFGR1 TIM16_DMA_RMP2 LL_SYSCFG_SetRemapDMA_TIM\n
  *         SYSCFG_CFGR1 TIM17_DMA_RMP2 LL_SYSCFG_SetRemapDMA_TIM\n
  *         SYSCFG_CFGR1 TIM1_DMA_RMP   LL_SYSCFG_SetRemapDMA_TIM\n
  *         SYSCFG_CFGR1 TIM2_DMA_RMP   LL_SYSCFG_SetRemapDMA_TIM\n
  *         SYSCFG_CFGR1 TIM3_DMA_RMP   LL_SYSCFG_SetRemapDMA_TIM
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_TIM16_RMP_DMA1_CH3 (*)
  *         @arg @ref LL_SYSCFG_TIM16_RMP_DMA1_CH4 (*)
  *         @arg @ref LL_SYSCFG_TIM16_RMP_DMA1_CH6 (*)
  *         @arg @ref LL_SYSCFG_TIM17_RMP_DMA1_CH1 (*)
  *         @arg @ref LL_SYSCFG_TIM17_RMP_DMA1_CH2 (*)
  *         @arg @ref LL_SYSCFG_TIM17_RMP_DMA1_CH7 (*)
  *         @arg @ref LL_SYSCFG_TIM1_RMP_DMA1_CH234 (*)
  *         @arg @ref LL_SYSCFG_TIM1_RMP_DMA1_CH6 (*)
  *         @arg @ref LL_SYSCFG_TIM2_RMP_DMA1_CH34 (*)
  *         @arg @ref LL_SYSCFG_TIM2_RMP_DMA1_CH7 (*)
  *         @arg @ref LL_SYSCFG_TIM3_RMP_DMA1_CH4 (*)
  *         @arg @ref LL_SYSCFG_TIM3_RMP_DMA1_CH6 (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapDMA_TIM(uint32_t Remap)
{
  MODIFY_REG(SYSCFG->CFGR1, (Remap & 0x00FF00FFU) << 8U, (Remap & 0xFF00FF00U));
}
#endif /* SYSCFG_CFGR1_TIM16_DMA_RMP || SYSCFG_CFGR1_TIM17_DMA_RMP || SYSCFG_CFGR1_TIM1_DMA_RMP || SYSCFG_CFGR1_TIM2_DMA_RMP || SYSCFG_CFGR1_TIM3_DMA_RMP */

#if defined(SYSCFG_CFGR1_PA11_PA12_RMP)
/**
  * @brief  Enable PIN pair PA11/12 mapped instead of PA9/10 (control the mapping of either
  * PA9/10 or PA11/12 pin pair on small pin-count packages)
  * @rmtoll SYSCFG_CFGR1 PA11_PA12_RMP  LL_SYSCFG_EnablePinRemap
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnablePinRemap(void)
{
  SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_PA11_PA12_RMP);
}

/**
  * @brief  Disable PIN pair PA11/12 mapped instead of PA9/10 (control the mapping of either
  * PA9/10 or PA11/12 pin pair on small pin-count packages)
  * @rmtoll SYSCFG_CFGR1 PA11_PA12_RMP  LL_SYSCFG_DisablePinRemap
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisablePinRemap(void)
{
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_PA11_PA12_RMP);
}
#endif /* SYSCFG_CFGR1_PA11_PA12_RMP */

/**
  * @brief  Enable the I2C fast mode plus driving capability.
  * @rmtoll SYSCFG_CFGR1 I2C_FMP_PB6   LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_PB7   LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_PB8   LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_PB9   LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_I2C1  LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_I2C2  LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_PA9   LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_PA10  LL_SYSCFG_EnableFastModePlus
  * @param  ConfigFastModePlus This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB6
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB7
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB8
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB9
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C1 (*)
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C2 (*)
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PA9 (*)
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PA10 (*)
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableFastModePlus(uint32_t ConfigFastModePlus)
{
  SET_BIT(SYSCFG->CFGR1, ConfigFastModePlus);
}

/**
  * @brief  Disable the I2C fast mode plus driving capability.
  * @rmtoll SYSCFG_CFGR1 I2C_FMP_PB6   LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_PB7   LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_PB8   LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_PB9   LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_I2C1  LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_I2C2  LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_PA9   LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_FMP_PA10  LL_SYSCFG_DisableFastModePlus
  * @param  ConfigFastModePlus This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB6
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB7
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB8
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB9
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C1 (*)
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C2 (*)
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PA9 (*)
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PA10 (*)
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableFastModePlus(uint32_t ConfigFastModePlus)
{
  CLEAR_BIT(SYSCFG->CFGR1, ConfigFastModePlus);
}

/**
  * @brief  Configure source input for the EXTI external interrupt.
  * @rmtoll SYSCFG_EXTICR1 EXTI0         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI1         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI2         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI3         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI4         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI5         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI6         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI7         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI8         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI9         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI10        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI11        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI12        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI13        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI14        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI15        LL_SYSCFG_SetEXTISource
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_EXTI_PORTA
  *         @arg @ref LL_SYSCFG_EXTI_PORTB
  *         @arg @ref LL_SYSCFG_EXTI_PORTC
  *         @arg @ref LL_SYSCFG_EXTI_PORTD (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTE (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTF
  *
  *         (*) value not defined in all devices
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
  MODIFY_REG(SYSCFG->EXTICR[Line & 0xFF], SYSCFG_EXTICR1_EXTI0 << (Line >> 16), Port << (Line >> 16));
}

/**
  * @brief  Get the configured defined for specific EXTI Line
  * @rmtoll SYSCFG_EXTICR1 EXTI0         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI1         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI2         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR1 EXTI3         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI4         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI5         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI6         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR2 EXTI7         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI8         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI9         LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI10        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR3 EXTI11        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI12        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI13        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI14        LL_SYSCFG_SetEXTISource\n
  *         SYSCFG_EXTICR4 EXTI15        LL_SYSCFG_SetEXTISource
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
  *         @arg @ref LL_SYSCFG_EXTI_PORTD (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTE (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTF
  *
  *         (*) value not defined in all devices
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetEXTISource(uint32_t Line)
{
  return (uint32_t)(READ_BIT(SYSCFG->EXTICR[Line & 0xFF], (SYSCFG_EXTICR1_EXTI0 << (Line >> 16))) >> (Line >> 16));
}

#if defined(SYSCFG_ITLINE0_SR_EWDG)
/**
  * @brief  Check if Window watchdog interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE0 SR_EWDG       LL_SYSCFG_IsActiveFlag_WWDG
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_WWDG(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[0], SYSCFG_ITLINE0_SR_EWDG) == (SYSCFG_ITLINE0_SR_EWDG));
}
#endif /* SYSCFG_ITLINE0_SR_EWDG */

#if defined(SYSCFG_ITLINE1_SR_PVDOUT)
/**
  * @brief  Check if PVD supply monitoring interrupt occurred or not (EXTI line 16).
  * @rmtoll SYSCFG_ITLINE1 SR_PVDOUT     LL_SYSCFG_IsActiveFlag_PVDOUT
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_PVDOUT(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[1], SYSCFG_ITLINE1_SR_PVDOUT) == (SYSCFG_ITLINE1_SR_PVDOUT));
}
#endif /* SYSCFG_ITLINE1_SR_PVDOUT */

#if defined(SYSCFG_ITLINE1_SR_VDDIO2)
/**
  * @brief  Check if VDDIO2 supply monitoring interrupt occurred or not (EXTI line 31).
  * @rmtoll SYSCFG_ITLINE1 SR_VDDIO2     LL_SYSCFG_IsActiveFlag_VDDIO2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_VDDIO2(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[1], SYSCFG_ITLINE1_SR_VDDIO2) == (SYSCFG_ITLINE1_SR_VDDIO2));
}
#endif /* SYSCFG_ITLINE1_SR_VDDIO2 */

#if defined(SYSCFG_ITLINE2_SR_RTC_WAKEUP)
/**
  * @brief  Check if RTC Wake Up interrupt occurred or not (EXTI line 20).
  * @rmtoll SYSCFG_ITLINE2 SR_RTC_WAKEUP  LL_SYSCFG_IsActiveFlag_RTC_WAKEUP
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_RTC_WAKEUP(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[2], SYSCFG_ITLINE2_SR_RTC_WAKEUP) == (SYSCFG_ITLINE2_SR_RTC_WAKEUP));
}
#endif /* SYSCFG_ITLINE2_SR_RTC_WAKEUP */

#if defined(SYSCFG_ITLINE2_SR_RTC_TSTAMP)
/**
  * @brief  Check if RTC Tamper and TimeStamp interrupt occurred or not (EXTI line 19).
  * @rmtoll SYSCFG_ITLINE2 SR_RTC_TSTAMP  LL_SYSCFG_IsActiveFlag_RTC_TSTAMP
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_RTC_TSTAMP(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[2], SYSCFG_ITLINE2_SR_RTC_TSTAMP) == (SYSCFG_ITLINE2_SR_RTC_TSTAMP));
}
#endif /* SYSCFG_ITLINE2_SR_RTC_TSTAMP */

#if defined(SYSCFG_ITLINE2_SR_RTC_ALRA)
/**
  * @brief  Check if RTC Alarm interrupt occurred or not (EXTI line 17).
  * @rmtoll SYSCFG_ITLINE2 SR_RTC_ALRA   LL_SYSCFG_IsActiveFlag_RTC_ALRA
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_RTC_ALRA(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[2], SYSCFG_ITLINE2_SR_RTC_ALRA) == (SYSCFG_ITLINE2_SR_RTC_ALRA));
}
#endif /* SYSCFG_ITLINE2_SR_RTC_ALRA */

#if defined(SYSCFG_ITLINE3_SR_FLASH_ITF)
/**
  * @brief  Check if Flash interface interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE3 SR_FLASH_ITF  LL_SYSCFG_IsActiveFlag_FLASH_ITF
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_FLASH_ITF(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[3], SYSCFG_ITLINE3_SR_FLASH_ITF) == (SYSCFG_ITLINE3_SR_FLASH_ITF));
}
#endif /* SYSCFG_ITLINE3_SR_FLASH_ITF */

#if defined(SYSCFG_ITLINE4_SR_CRS)
/**
  * @brief  Check if Clock recovery system interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE4 SR_CRS        LL_SYSCFG_IsActiveFlag_CRS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_CRS(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[4], SYSCFG_ITLINE4_SR_CRS) == (SYSCFG_ITLINE4_SR_CRS));
}
#endif /* SYSCFG_ITLINE4_SR_CRS */

#if defined(SYSCFG_ITLINE4_SR_CLK_CTRL)
/**
  * @brief  Check if Reset and clock control interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE4 SR_CLK_CTRL   LL_SYSCFG_IsActiveFlag_CLK_CTRL
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_CLK_CTRL(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[4], SYSCFG_ITLINE4_SR_CLK_CTRL) == (SYSCFG_ITLINE4_SR_CLK_CTRL));
}
#endif /* SYSCFG_ITLINE4_SR_CLK_CTRL */

#if defined(SYSCFG_ITLINE5_SR_EXTI0)
/**
  * @brief  Check if EXTI line 0 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE5 SR_EXTI0      LL_SYSCFG_IsActiveFlag_EXTI0
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI0(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[5], SYSCFG_ITLINE5_SR_EXTI0) == (SYSCFG_ITLINE5_SR_EXTI0));
}
#endif /* SYSCFG_ITLINE5_SR_EXTI0 */

#if defined(SYSCFG_ITLINE5_SR_EXTI1)
/**
  * @brief  Check if EXTI line 1 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE5 SR_EXTI1      LL_SYSCFG_IsActiveFlag_EXTI1
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI1(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[5], SYSCFG_ITLINE5_SR_EXTI1) == (SYSCFG_ITLINE5_SR_EXTI1));
}
#endif /* SYSCFG_ITLINE5_SR_EXTI1 */

#if defined(SYSCFG_ITLINE6_SR_EXTI2)
/**
  * @brief  Check if EXTI line 2 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE6 SR_EXTI2      LL_SYSCFG_IsActiveFlag_EXTI2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI2(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[6], SYSCFG_ITLINE6_SR_EXTI2) == (SYSCFG_ITLINE6_SR_EXTI2));
}
#endif /* SYSCFG_ITLINE6_SR_EXTI2 */

#if defined(SYSCFG_ITLINE6_SR_EXTI3)
/**
  * @brief  Check if EXTI line 3 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE6 SR_EXTI3      LL_SYSCFG_IsActiveFlag_EXTI3
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI3(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[6], SYSCFG_ITLINE6_SR_EXTI3) == (SYSCFG_ITLINE6_SR_EXTI3));
}
#endif /* SYSCFG_ITLINE6_SR_EXTI3 */

#if defined(SYSCFG_ITLINE7_SR_EXTI4)
/**
  * @brief  Check if EXTI line 4 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI4      LL_SYSCFG_IsActiveFlag_EXTI4
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI4(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI4) == (SYSCFG_ITLINE7_SR_EXTI4));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI4 */

#if defined(SYSCFG_ITLINE7_SR_EXTI5)
/**
  * @brief  Check if EXTI line 5 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI5      LL_SYSCFG_IsActiveFlag_EXTI5
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI5(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI5) == (SYSCFG_ITLINE7_SR_EXTI5));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI5 */

#if defined(SYSCFG_ITLINE7_SR_EXTI6)
/**
  * @brief  Check if EXTI line 6 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI6      LL_SYSCFG_IsActiveFlag_EXTI6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI6(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI6) == (SYSCFG_ITLINE7_SR_EXTI6));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI6 */

#if defined(SYSCFG_ITLINE7_SR_EXTI7)
/**
  * @brief  Check if EXTI line 7 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI7      LL_SYSCFG_IsActiveFlag_EXTI7
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI7(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI7) == (SYSCFG_ITLINE7_SR_EXTI7));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI7 */

#if defined(SYSCFG_ITLINE7_SR_EXTI8)
/**
  * @brief  Check if EXTI line 8 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI8      LL_SYSCFG_IsActiveFlag_EXTI8
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI8(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI8) == (SYSCFG_ITLINE7_SR_EXTI8));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI8 */

#if defined(SYSCFG_ITLINE7_SR_EXTI9)
/**
  * @brief  Check if EXTI line 9 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI9      LL_SYSCFG_IsActiveFlag_EXTI9
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI9(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI9) == (SYSCFG_ITLINE7_SR_EXTI9));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI9 */

#if defined(SYSCFG_ITLINE7_SR_EXTI10)
/**
  * @brief  Check if EXTI line 10 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI10     LL_SYSCFG_IsActiveFlag_EXTI10
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI10(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI10) == (SYSCFG_ITLINE7_SR_EXTI10));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI10 */

#if defined(SYSCFG_ITLINE7_SR_EXTI11)
/**
  * @brief  Check if EXTI line 11 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI11     LL_SYSCFG_IsActiveFlag_EXTI11
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI11(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI11) == (SYSCFG_ITLINE7_SR_EXTI11));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI11 */

#if defined(SYSCFG_ITLINE7_SR_EXTI12)
/**
  * @brief  Check if EXTI line 12 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI12     LL_SYSCFG_IsActiveFlag_EXTI12
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI12(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI12) == (SYSCFG_ITLINE7_SR_EXTI12));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI12 */

#if defined(SYSCFG_ITLINE7_SR_EXTI13)
/**
  * @brief  Check if EXTI line 13 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI13     LL_SYSCFG_IsActiveFlag_EXTI13
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI13(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI13) == (SYSCFG_ITLINE7_SR_EXTI13));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI13 */

#if defined(SYSCFG_ITLINE7_SR_EXTI14)
/**
  * @brief  Check if EXTI line 14 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI14     LL_SYSCFG_IsActiveFlag_EXTI14
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI14(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI14) == (SYSCFG_ITLINE7_SR_EXTI14));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI14 */

#if defined(SYSCFG_ITLINE7_SR_EXTI15)
/**
  * @brief  Check if EXTI line 15 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE7 SR_EXTI15     LL_SYSCFG_IsActiveFlag_EXTI15
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_EXTI15(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[7], SYSCFG_ITLINE7_SR_EXTI15) == (SYSCFG_ITLINE7_SR_EXTI15));
}
#endif /* SYSCFG_ITLINE7_SR_EXTI15 */

#if defined(SYSCFG_ITLINE8_SR_TSC_EOA)
/**
  * @brief  Check if Touch sensing controller end of acquisition interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE8 SR_TSC_EOA    LL_SYSCFG_IsActiveFlag_TSC_EOA
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TSC_EOA(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[8], SYSCFG_ITLINE8_SR_TSC_EOA) == (SYSCFG_ITLINE8_SR_TSC_EOA));
}
#endif /* SYSCFG_ITLINE8_SR_TSC_EOA */

#if defined(SYSCFG_ITLINE8_SR_TSC_MCE)
/**
  * @brief  Check if Touch sensing controller max counterror interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE8 SR_TSC_MCE    LL_SYSCFG_IsActiveFlag_TSC_MCE
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TSC_MCE(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[8], SYSCFG_ITLINE8_SR_TSC_MCE) == (SYSCFG_ITLINE8_SR_TSC_MCE));
}
#endif /* SYSCFG_ITLINE8_SR_TSC_MCE */

#if defined(SYSCFG_ITLINE9_SR_DMA1_CH1)
/**
  * @brief  Check if DMA1 channel 1 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE9 SR_DMA1_CH1   LL_SYSCFG_IsActiveFlag_DMA1_CH1
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA1_CH1(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[9], SYSCFG_ITLINE9_SR_DMA1_CH1) == (SYSCFG_ITLINE9_SR_DMA1_CH1));
}
#endif /* SYSCFG_ITLINE9_SR_DMA1_CH1 */

#if defined(SYSCFG_ITLINE10_SR_DMA1_CH2)
/**
  * @brief  Check if DMA1 channel 2 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE10 SR_DMA1_CH2   LL_SYSCFG_IsActiveFlag_DMA1_CH2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA1_CH2(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[10], SYSCFG_ITLINE10_SR_DMA1_CH2) == (SYSCFG_ITLINE10_SR_DMA1_CH2));
}
#endif /* SYSCFG_ITLINE10_SR_DMA1_CH2 */

#if defined(SYSCFG_ITLINE10_SR_DMA1_CH3)
/**
  * @brief  Check if DMA1 channel 3 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE10 SR_DMA1_CH3   LL_SYSCFG_IsActiveFlag_DMA1_CH3
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA1_CH3(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[10], SYSCFG_ITLINE10_SR_DMA1_CH3) == (SYSCFG_ITLINE10_SR_DMA1_CH3));
}
#endif /* SYSCFG_ITLINE10_SR_DMA1_CH3 */

#if defined(SYSCFG_ITLINE10_SR_DMA2_CH1)
/**
  * @brief  Check if DMA2 channel 1 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE10 SR_DMA2_CH1   LL_SYSCFG_IsActiveFlag_DMA2_CH1
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA2_CH1(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[10], SYSCFG_ITLINE10_SR_DMA2_CH1) == (SYSCFG_ITLINE10_SR_DMA2_CH1));
}
#endif /* SYSCFG_ITLINE10_SR_DMA2_CH1 */

#if defined(SYSCFG_ITLINE10_SR_DMA2_CH2)
/**
  * @brief  Check if DMA2 channel 2 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE10 SR_DMA2_CH2   LL_SYSCFG_IsActiveFlag_DMA2_CH2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA2_CH2(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[10], SYSCFG_ITLINE10_SR_DMA2_CH2) == (SYSCFG_ITLINE10_SR_DMA2_CH2));
}
#endif /* SYSCFG_ITLINE10_SR_DMA2_CH2 */

#if defined(SYSCFG_ITLINE11_SR_DMA1_CH4)
/**
  * @brief  Check if DMA1 channel 4 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE11 SR_DMA1_CH4   LL_SYSCFG_IsActiveFlag_DMA1_CH4
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA1_CH4(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[11], SYSCFG_ITLINE11_SR_DMA1_CH4) == (SYSCFG_ITLINE11_SR_DMA1_CH4));
}
#endif /* SYSCFG_ITLINE11_SR_DMA1_CH4 */

#if defined(SYSCFG_ITLINE11_SR_DMA1_CH5)
/**
  * @brief  Check if DMA1 channel 5 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE11 SR_DMA1_CH5   LL_SYSCFG_IsActiveFlag_DMA1_CH5
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA1_CH5(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[11], SYSCFG_ITLINE11_SR_DMA1_CH5) == (SYSCFG_ITLINE11_SR_DMA1_CH5));
}
#endif /* SYSCFG_ITLINE11_SR_DMA1_CH5 */

#if defined(SYSCFG_ITLINE11_SR_DMA1_CH6)
/**
  * @brief  Check if DMA1 channel 6 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE11 SR_DMA1_CH6   LL_SYSCFG_IsActiveFlag_DMA1_CH6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA1_CH6(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[11], SYSCFG_ITLINE11_SR_DMA1_CH6) == (SYSCFG_ITLINE11_SR_DMA1_CH6));
}
#endif /* SYSCFG_ITLINE11_SR_DMA1_CH6 */

#if defined(SYSCFG_ITLINE11_SR_DMA1_CH7)
/**
  * @brief  Check if DMA1 channel 7 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE11 SR_DMA1_CH7   LL_SYSCFG_IsActiveFlag_DMA1_CH7
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA1_CH7(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[11], SYSCFG_ITLINE11_SR_DMA1_CH7) == (SYSCFG_ITLINE11_SR_DMA1_CH7));
}
#endif /* SYSCFG_ITLINE11_SR_DMA1_CH7 */

#if defined(SYSCFG_ITLINE11_SR_DMA2_CH3)
/**
  * @brief  Check if DMA2 channel 3 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE11 SR_DMA2_CH3   LL_SYSCFG_IsActiveFlag_DMA2_CH3
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA2_CH3(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[11], SYSCFG_ITLINE11_SR_DMA2_CH3) == (SYSCFG_ITLINE11_SR_DMA2_CH3));
}
#endif /* SYSCFG_ITLINE11_SR_DMA2_CH3 */

#if defined(SYSCFG_ITLINE11_SR_DMA2_CH4)
/**
  * @brief  Check if DMA2 channel 4 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE11 SR_DMA2_CH4   LL_SYSCFG_IsActiveFlag_DMA2_CH4
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA2_CH4(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[11], SYSCFG_ITLINE11_SR_DMA2_CH4) == (SYSCFG_ITLINE11_SR_DMA2_CH4));
}
#endif /* SYSCFG_ITLINE11_SR_DMA2_CH4 */

#if defined(SYSCFG_ITLINE11_SR_DMA2_CH5)
/**
  * @brief  Check if DMA2 channel 5 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE11 SR_DMA2_CH5   LL_SYSCFG_IsActiveFlag_DMA2_CH5
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DMA2_CH5(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[11], SYSCFG_ITLINE11_SR_DMA2_CH5) == (SYSCFG_ITLINE11_SR_DMA2_CH5));
}
#endif /* SYSCFG_ITLINE11_SR_DMA2_CH5 */

#if defined(SYSCFG_ITLINE12_SR_ADC)
/**
  * @brief  Check if ADC interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE12 SR_ADC        LL_SYSCFG_IsActiveFlag_ADC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_ADC(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[12], SYSCFG_ITLINE12_SR_ADC) == (SYSCFG_ITLINE12_SR_ADC));
}
#endif /* SYSCFG_ITLINE12_SR_ADC */

#if defined(SYSCFG_ITLINE12_SR_COMP1)
/**
  * @brief  Check if Comparator 1 interrupt occurred or not (EXTI line 21).
  * @rmtoll SYSCFG_ITLINE12 SR_COMP1      LL_SYSCFG_IsActiveFlag_COMP1
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_COMP1(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[12], SYSCFG_ITLINE12_SR_COMP1) == (SYSCFG_ITLINE12_SR_COMP1));
}
#endif /* SYSCFG_ITLINE12_SR_COMP1 */

#if defined(SYSCFG_ITLINE12_SR_COMP2)
/**
  * @brief  Check if Comparator 2 interrupt occurred or not (EXTI line 22).
  * @rmtoll SYSCFG_ITLINE12 SR_COMP2      LL_SYSCFG_IsActiveFlag_COMP2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_COMP2(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[12], SYSCFG_ITLINE12_SR_COMP2) == (SYSCFG_ITLINE12_SR_COMP2));
}
#endif /* SYSCFG_ITLINE12_SR_COMP2 */

#if defined(SYSCFG_ITLINE13_SR_TIM1_BRK)
/**
  * @brief  Check if Timer 1 break interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE13 SR_TIM1_BRK   LL_SYSCFG_IsActiveFlag_TIM1_BRK
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM1_BRK(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[13], SYSCFG_ITLINE13_SR_TIM1_BRK) == (SYSCFG_ITLINE13_SR_TIM1_BRK));
}
#endif /* SYSCFG_ITLINE13_SR_TIM1_BRK */

#if defined(SYSCFG_ITLINE13_SR_TIM1_UPD)
/**
  * @brief  Check if Timer 1 update interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE13 SR_TIM1_UPD   LL_SYSCFG_IsActiveFlag_TIM1_UPD
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM1_UPD(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[13], SYSCFG_ITLINE13_SR_TIM1_UPD) == (SYSCFG_ITLINE13_SR_TIM1_UPD));
}
#endif /* SYSCFG_ITLINE13_SR_TIM1_UPD */

#if defined(SYSCFG_ITLINE13_SR_TIM1_TRG)
/**
  * @brief  Check if Timer 1 trigger interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE13 SR_TIM1_TRG   LL_SYSCFG_IsActiveFlag_TIM1_TRG
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM1_TRG(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[13], SYSCFG_ITLINE13_SR_TIM1_TRG) == (SYSCFG_ITLINE13_SR_TIM1_TRG));
}
#endif /* SYSCFG_ITLINE13_SR_TIM1_TRG */

#if defined(SYSCFG_ITLINE13_SR_TIM1_CCU)
/**
  * @brief  Check if Timer 1 commutation interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE13 SR_TIM1_CCU   LL_SYSCFG_IsActiveFlag_TIM1_CCU
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM1_CCU(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[13], SYSCFG_ITLINE13_SR_TIM1_CCU) == (SYSCFG_ITLINE13_SR_TIM1_CCU));
}
#endif /* SYSCFG_ITLINE13_SR_TIM1_CCU */

#if defined(SYSCFG_ITLINE14_SR_TIM1_CC)
/**
  * @brief  Check if Timer 1 capture compare interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE14 SR_TIM1_CC    LL_SYSCFG_IsActiveFlag_TIM1_CC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM1_CC(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[14], SYSCFG_ITLINE14_SR_TIM1_CC) == (SYSCFG_ITLINE14_SR_TIM1_CC));
}
#endif /* SYSCFG_ITLINE14_SR_TIM1_CC */

#if defined(SYSCFG_ITLINE15_SR_TIM2_GLB)
/**
  * @brief  Check if Timer 2 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE15 SR_TIM2_GLB   LL_SYSCFG_IsActiveFlag_TIM2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM2(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[15], SYSCFG_ITLINE15_SR_TIM2_GLB) == (SYSCFG_ITLINE15_SR_TIM2_GLB));
}
#endif /* SYSCFG_ITLINE15_SR_TIM2_GLB */

#if defined(SYSCFG_ITLINE16_SR_TIM3_GLB)
/**
  * @brief  Check if Timer 3 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE16 SR_TIM3_GLB   LL_SYSCFG_IsActiveFlag_TIM3
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM3(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[16], SYSCFG_ITLINE16_SR_TIM3_GLB) == (SYSCFG_ITLINE16_SR_TIM3_GLB));
}
#endif /* SYSCFG_ITLINE16_SR_TIM3_GLB */

#if defined(SYSCFG_ITLINE17_SR_DAC)
/**
  * @brief  Check if DAC underrun interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE17 SR_DAC        LL_SYSCFG_IsActiveFlag_DAC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_DAC(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[17], SYSCFG_ITLINE17_SR_DAC) == (SYSCFG_ITLINE17_SR_DAC));
}
#endif /* SYSCFG_ITLINE17_SR_DAC */

#if defined(SYSCFG_ITLINE17_SR_TIM6_GLB)
/**
  * @brief  Check if Timer 6 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE17 SR_TIM6_GLB   LL_SYSCFG_IsActiveFlag_TIM6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM6(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[17], SYSCFG_ITLINE17_SR_TIM6_GLB) == (SYSCFG_ITLINE17_SR_TIM6_GLB));
}
#endif /* SYSCFG_ITLINE17_SR_TIM6_GLB */

#if defined(SYSCFG_ITLINE18_SR_TIM7_GLB)
/**
  * @brief  Check if Timer 7 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE18 SR_TIM7_GLB   LL_SYSCFG_IsActiveFlag_TIM7
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM7(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[18], SYSCFG_ITLINE18_SR_TIM7_GLB) == (SYSCFG_ITLINE18_SR_TIM7_GLB));
}
#endif /* SYSCFG_ITLINE18_SR_TIM7_GLB */

#if defined(SYSCFG_ITLINE19_SR_TIM14_GLB)
/**
  * @brief  Check if Timer 14 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE19 SR_TIM14_GLB  LL_SYSCFG_IsActiveFlag_TIM14
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM14(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[19], SYSCFG_ITLINE19_SR_TIM14_GLB) == (SYSCFG_ITLINE19_SR_TIM14_GLB));
}
#endif /* SYSCFG_ITLINE19_SR_TIM14_GLB */

#if defined(SYSCFG_ITLINE20_SR_TIM15_GLB)
/**
  * @brief  Check if Timer 15 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE20 SR_TIM15_GLB  LL_SYSCFG_IsActiveFlag_TIM15
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM15(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[20], SYSCFG_ITLINE20_SR_TIM15_GLB) == (SYSCFG_ITLINE20_SR_TIM15_GLB));
}
#endif /* SYSCFG_ITLINE20_SR_TIM15_GLB */

#if defined(SYSCFG_ITLINE21_SR_TIM16_GLB)
/**
  * @brief  Check if Timer 16 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE21 SR_TIM16_GLB  LL_SYSCFG_IsActiveFlag_TIM16
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM16(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[21], SYSCFG_ITLINE21_SR_TIM16_GLB) == (SYSCFG_ITLINE21_SR_TIM16_GLB));
}
#endif /* SYSCFG_ITLINE21_SR_TIM16_GLB */

#if defined(SYSCFG_ITLINE22_SR_TIM17_GLB)
/**
  * @brief  Check if Timer 17 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE22 SR_TIM17_GLB  LL_SYSCFG_IsActiveFlag_TIM17
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_TIM17(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[22], SYSCFG_ITLINE22_SR_TIM17_GLB) == (SYSCFG_ITLINE22_SR_TIM17_GLB));
}
#endif /* SYSCFG_ITLINE22_SR_TIM17_GLB */

#if defined(SYSCFG_ITLINE23_SR_I2C1_GLB)
/**
  * @brief  Check if I2C1 interrupt occurred or not, combined with EXTI line 23.
  * @rmtoll SYSCFG_ITLINE23 SR_I2C1_GLB   LL_SYSCFG_IsActiveFlag_I2C1
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_I2C1(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[23], SYSCFG_ITLINE23_SR_I2C1_GLB) == (SYSCFG_ITLINE23_SR_I2C1_GLB));
}
#endif /* SYSCFG_ITLINE23_SR_I2C1_GLB */

#if defined(SYSCFG_ITLINE24_SR_I2C2_GLB)
/**
  * @brief  Check if I2C2 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE24 SR_I2C2_GLB   LL_SYSCFG_IsActiveFlag_I2C2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_I2C2(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[24], SYSCFG_ITLINE24_SR_I2C2_GLB) == (SYSCFG_ITLINE24_SR_I2C2_GLB));
}
#endif /* SYSCFG_ITLINE24_SR_I2C2_GLB */

#if defined(SYSCFG_ITLINE25_SR_SPI1)
/**
  * @brief  Check if SPI1 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE25 SR_SPI1       LL_SYSCFG_IsActiveFlag_SPI1
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_SPI1(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[25], SYSCFG_ITLINE25_SR_SPI1) == (SYSCFG_ITLINE25_SR_SPI1));
}
#endif /* SYSCFG_ITLINE25_SR_SPI1 */

#if defined(SYSCFG_ITLINE26_SR_SPI2)
/**
  * @brief  Check if SPI2 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE26 SR_SPI2       LL_SYSCFG_IsActiveFlag_SPI2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_SPI2(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[26], SYSCFG_ITLINE26_SR_SPI2) == (SYSCFG_ITLINE26_SR_SPI2));
}
#endif /* SYSCFG_ITLINE26_SR_SPI2 */

#if defined(SYSCFG_ITLINE27_SR_USART1_GLB)
/**
  * @brief  Check if USART1 interrupt occurred or not, combined with EXTI line 25.
  * @rmtoll SYSCFG_ITLINE27 SR_USART1_GLB  LL_SYSCFG_IsActiveFlag_USART1
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_USART1(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[27], SYSCFG_ITLINE27_SR_USART1_GLB) == (SYSCFG_ITLINE27_SR_USART1_GLB));
}
#endif /* SYSCFG_ITLINE27_SR_USART1_GLB */

#if defined(SYSCFG_ITLINE28_SR_USART2_GLB)
/**
  * @brief  Check if USART2 interrupt occurred or not, combined with EXTI line 26.
  * @rmtoll SYSCFG_ITLINE28 SR_USART2_GLB  LL_SYSCFG_IsActiveFlag_USART2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_USART2(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[28], SYSCFG_ITLINE28_SR_USART2_GLB) == (SYSCFG_ITLINE28_SR_USART2_GLB));
}
#endif /* SYSCFG_ITLINE28_SR_USART2_GLB */

#if defined(SYSCFG_ITLINE29_SR_USART3_GLB)
/**
  * @brief  Check if USART3 interrupt occurred or not, combined with EXTI line 28.
  * @rmtoll SYSCFG_ITLINE29 SR_USART3_GLB  LL_SYSCFG_IsActiveFlag_USART3
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_USART3(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[29], SYSCFG_ITLINE29_SR_USART3_GLB) == (SYSCFG_ITLINE29_SR_USART3_GLB));
}
#endif /* SYSCFG_ITLINE29_SR_USART3_GLB */

#if defined(SYSCFG_ITLINE29_SR_USART4_GLB)
/**
  * @brief  Check if USART4 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE29 SR_USART4_GLB  LL_SYSCFG_IsActiveFlag_USART4
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_USART4(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[29], SYSCFG_ITLINE29_SR_USART4_GLB) == (SYSCFG_ITLINE29_SR_USART4_GLB));
}
#endif /* SYSCFG_ITLINE29_SR_USART4_GLB */

#if defined(SYSCFG_ITLINE29_SR_USART5_GLB)
/**
  * @brief  Check if USART5 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE29 SR_USART5_GLB  LL_SYSCFG_IsActiveFlag_USART5
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_USART5(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[29], SYSCFG_ITLINE29_SR_USART5_GLB) == (SYSCFG_ITLINE29_SR_USART5_GLB));
}
#endif /* SYSCFG_ITLINE29_SR_USART5_GLB */

#if defined(SYSCFG_ITLINE29_SR_USART6_GLB)
/**
  * @brief  Check if USART6 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE29 SR_USART6_GLB  LL_SYSCFG_IsActiveFlag_USART6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_USART6(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[29], SYSCFG_ITLINE29_SR_USART6_GLB) == (SYSCFG_ITLINE29_SR_USART6_GLB));
}
#endif /* SYSCFG_ITLINE29_SR_USART6_GLB */

#if defined(SYSCFG_ITLINE29_SR_USART7_GLB)
/**
  * @brief  Check if USART7 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE29 SR_USART7_GLB  LL_SYSCFG_IsActiveFlag_USART7
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_USART7(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[29], SYSCFG_ITLINE29_SR_USART7_GLB) == (SYSCFG_ITLINE29_SR_USART7_GLB));
}
#endif /* SYSCFG_ITLINE29_SR_USART7_GLB */

#if defined(SYSCFG_ITLINE29_SR_USART8_GLB)
/**
  * @brief  Check if USART8 interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE29 SR_USART8_GLB  LL_SYSCFG_IsActiveFlag_USART8
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_USART8(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[29], SYSCFG_ITLINE29_SR_USART8_GLB) == (SYSCFG_ITLINE29_SR_USART8_GLB));
}
#endif /* SYSCFG_ITLINE29_SR_USART8_GLB */

#if defined(SYSCFG_ITLINE30_SR_CAN)
/**
  * @brief  Check if CAN interrupt occurred or not.
  * @rmtoll SYSCFG_ITLINE30 SR_CAN        LL_SYSCFG_IsActiveFlag_CAN
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_CAN(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[30], SYSCFG_ITLINE30_SR_CAN) == (SYSCFG_ITLINE30_SR_CAN));
}
#endif /* SYSCFG_ITLINE30_SR_CAN */

#if defined(SYSCFG_ITLINE30_SR_CEC)
/**
  * @brief  Check if CEC interrupt occurred or not, combined with EXTI line 27.
  * @rmtoll SYSCFG_ITLINE30 SR_CEC        LL_SYSCFG_IsActiveFlag_CEC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_CEC(void)
{
  return (READ_BIT(SYSCFG->IT_LINE_SR[30], SYSCFG_ITLINE30_SR_CEC) == (SYSCFG_ITLINE30_SR_CEC));
}
#endif /* SYSCFG_ITLINE30_SR_CEC */

/**
  * @brief  Set connections to TIMx Break inputs
  * @rmtoll SYSCFG_CFGR2 LOCKUP_LOCK   LL_SYSCFG_SetTIMBreakInputs\n
  *         SYSCFG_CFGR2 SRAM_PARITY_LOCK  LL_SYSCFG_SetTIMBreakInputs\n
  *         SYSCFG_CFGR2 PVD_LOCK      LL_SYSCFG_SetTIMBreakInputs
  * @param  Break This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_TIMBREAK_PVD (*)
  *         @arg @ref LL_SYSCFG_TIMBREAK_SRAM_PARITY
  *         @arg @ref LL_SYSCFG_TIMBREAK_LOCKUP
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetTIMBreakInputs(uint32_t Break)
{
#if defined(SYSCFG_CFGR2_PVD_LOCK)
  MODIFY_REG(SYSCFG->CFGR2, SYSCFG_CFGR2_LOCKUP_LOCK | SYSCFG_CFGR2_SRAM_PARITY_LOCK | SYSCFG_CFGR2_PVD_LOCK, Break);
#else
  MODIFY_REG(SYSCFG->CFGR2, SYSCFG_CFGR2_LOCKUP_LOCK | SYSCFG_CFGR2_SRAM_PARITY_LOCK, Break);
#endif /*SYSCFG_CFGR2_PVD_LOCK*/
}

/**
  * @brief  Get connections to TIMx Break inputs
  * @rmtoll SYSCFG_CFGR2 LOCKUP_LOCK   LL_SYSCFG_GetTIMBreakInputs\n
  *         SYSCFG_CFGR2 SRAM_PARITY_LOCK  LL_SYSCFG_GetTIMBreakInputs\n
  *         SYSCFG_CFGR2 PVD_LOCK      LL_SYSCFG_GetTIMBreakInputs
  * @retval Returned value can be can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_TIMBREAK_PVD (*)
  *         @arg @ref LL_SYSCFG_TIMBREAK_SRAM_PARITY
  *         @arg @ref LL_SYSCFG_TIMBREAK_LOCKUP
  *
  *         (*) value not defined in all devices
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetTIMBreakInputs(void)
{
#if defined(SYSCFG_CFGR2_PVD_LOCK)
  return (uint32_t)(READ_BIT(SYSCFG->CFGR2,
                             SYSCFG_CFGR2_LOCKUP_LOCK | SYSCFG_CFGR2_SRAM_PARITY_LOCK | SYSCFG_CFGR2_PVD_LOCK));
#else
  return (uint32_t)(READ_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_LOCKUP_LOCK | SYSCFG_CFGR2_SRAM_PARITY_LOCK));
#endif /*SYSCFG_CFGR2_PVD_LOCK*/
}

/**
  * @brief  Check if SRAM parity error detected
  * @rmtoll SYSCFG_CFGR2 SRAM_PEF      LL_SYSCFG_IsActiveFlag_SP
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_SP(void)
{
  return (READ_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_SRAM_PEF) == (SYSCFG_CFGR2_SRAM_PEF));
}

/**
  * @brief  Clear SRAM parity error flag
  * @rmtoll SYSCFG_CFGR2 SRAM_PEF      LL_SYSCFG_ClearFlag_SP
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_ClearFlag_SP(void)
{
  SET_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_SRAM_PEF);
}

/**
  * @}
  */

/** @defgroup SYSTEM_LL_EF_DBGMCU DBGMCU
  * @{
  */

/**
  * @brief  Return the device identifier
  * @note For STM32F03x devices, the device ID is 0x444
  * @note For STM32F04x devices, the device ID is 0x445.
  * @note For STM32F05x devices, the device ID is 0x440
  * @note For STM32F07x devices, the device ID is 0x448
  * @note For STM32F09x devices, the device ID is 0x442
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
          For example, it is read as 0x1000 for Revision 1.0.
  * @rmtoll DBGMCU_IDCODE REV_ID        LL_DBGMCU_GetRevisionID
  * @retval Values between Min_Data=0x00 and Max_Data=0xFFFF
  */
__STATIC_INLINE uint32_t LL_DBGMCU_GetRevisionID(void)
{
  return (uint32_t)(READ_BIT(DBGMCU->IDCODE, DBGMCU_IDCODE_REV_ID) >> DBGMCU_IDCODE_REV_ID_Pos);
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
  * @brief  Freeze APB1 peripherals (group1 peripherals)
  * @rmtoll DBGMCU_APB1FZ DBG_TIM2_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ DBG_TIM3_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ DBG_TIM6_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ DBG_TIM7_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ DBG_TIM14_STOP          LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ DBG_RTC_STOP            LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ DBG_WWDG_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ DBG_IWDG_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ DBG_I2C1_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ DBG_CAN_STOP  LL_DBGMCU_APB1_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM6_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM7_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM14_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_CAN_STOP (*)
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB1FZ, Periphs);
}

/**
  * @brief  Unfreeze APB1 peripherals (group1 peripherals)
  * @rmtoll DBGMCU_APB1FZ DBG_TIM2_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ DBG_TIM3_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ DBG_TIM6_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ DBG_TIM7_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ DBG_TIM14_STOP          LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ DBG_RTC_STOP            LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ DBG_WWDG_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ DBG_IWDG_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ DBG_I2C1_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ DBG_CAN_STOP            LL_DBGMCU_APB1_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM6_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM7_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM14_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_CAN_STOP (*)
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB1FZ, Periphs);
}

/**
  * @brief  Freeze APB1 peripherals (group2 peripherals)
  * @rmtoll DBGMCU_APB2FZ DBG_TIM1_STOP   LL_DBGMCU_APB1_GRP2_FreezePeriph\n
  *         DBGMCU_APB2FZ DBG_TIM15_STOP  LL_DBGMCU_APB1_GRP2_FreezePeriph\n
  *         DBGMCU_APB2FZ DBG_TIM16_STOP  LL_DBGMCU_APB1_GRP2_FreezePeriph\n
  *         DBGMCU_APB2FZ DBG_TIM17_STOP  LL_DBGMCU_APB1_GRP2_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP2_TIM1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP2_TIM15_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP2_TIM16_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP2_TIM17_STOP
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP2_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB2FZ, Periphs);
}

/**
  * @brief  Unfreeze APB1 peripherals (group2 peripherals)
  * @rmtoll DBGMCU_APB2FZ DBG_TIM1_STOP   LL_DBGMCU_APB1_GRP2_UnFreezePeriph\n
  *         DBGMCU_APB2FZ DBG_TIM15_STOP  LL_DBGMCU_APB1_GRP2_UnFreezePeriph\n
  *         DBGMCU_APB2FZ DBG_TIM16_STOP  LL_DBGMCU_APB1_GRP2_UnFreezePeriph\n
  *         DBGMCU_APB2FZ DBG_TIM17_STOP  LL_DBGMCU_APB1_GRP2_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP2_TIM1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP2_TIM15_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP2_TIM16_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP2_TIM17_STOP
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP2_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB2FZ, Periphs);
}
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EF_FLASH FLASH
  * @{
  */

/**
  * @brief  Set FLASH Latency
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
  * @rmtoll FLASH_ACR    PRFTBE        LL_FLASH_EnablePrefetch
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnablePrefetch(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_PRFTBE);
}

/**
  * @brief  Disable Prefetch
  * @rmtoll FLASH_ACR    PRFTBE        LL_FLASH_DisablePrefetch
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisablePrefetch(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_PRFTBE);
}

/**
  * @brief  Check if Prefetch buffer is enabled
  * @rmtoll FLASH_ACR    PRFTBS        LL_FLASH_IsPrefetchEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_FLASH_IsPrefetchEnabled(void)
{
  return (READ_BIT(FLASH->ACR, FLASH_ACR_PRFTBS) == (FLASH_ACR_PRFTBS));
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

#endif /* defined (FLASH) || defined (SYSCFG) || defined (DBGMCU) */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F0xx_LL_SYSTEM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
