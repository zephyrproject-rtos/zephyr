/**
  ******************************************************************************
  * @file    stm32f3xx_ll_system.h
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
#ifndef __STM32F3xx_LL_SYSTEM_H
#define __STM32F3xx_LL_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx.h"

/** @addtogroup STM32F3xx_LL_Driver
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

/* Offset used to access to SYSCFG_CFGR1 and SYSCFG_CFGR3 registers */
#define SYSCFG_OFFSET_CFGR1    0x00000000U
#define SYSCFG_OFFSET_CFGR3    0x00000050U

/* Mask used for TIM breaks functions */
#if defined(SYSCFG_CFGR2_PVD_LOCK) && defined(SYSCFG_CFGR2_SRAM_PARITY_LOCK)
#define SYSCFG_MASK_TIM_BREAK (SYSCFG_CFGR2_LOCKUP_LOCK | SYSCFG_CFGR2_SRAM_PARITY_LOCK | SYSCFG_CFGR2_PVD_LOCK)
#elif defined(SYSCFG_CFGR2_PVD_LOCK) && !defined(SYSCFG_CFGR2_SRAM_PARITY_LOCK)
#define SYSCFG_MASK_TIM_BREAK (SYSCFG_CFGR2_LOCKUP_LOCK | SYSCFG_CFGR2_PVD_LOCK)
#elif !defined(SYSCFG_CFGR2_PVD_LOCK) && defined(SYSCFG_CFGR2_SRAM_PARITY_LOCK)
#define SYSCFG_MASK_TIM_BREAK (SYSCFG_CFGR2_LOCKUP_LOCK | SYSCFG_CFGR2_SRAM_PARITY_LOCK)
#else
#define SYSCFG_MASK_TIM_BREAK (SYSCFG_CFGR2_LOCKUP_LOCK)
#endif /* SYSCFG_CFGR2_PVD_LOCK && SYSCFG_CFGR2_SRAM_PARITY_LOCK */

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
#define LL_SYSCFG_REMAP_FLASH              (uint32_t)0x00000000                                /* Main Flash memory mapped at 0x00000000 */
#define LL_SYSCFG_REMAP_SYSTEMFLASH        SYSCFG_CFGR1_MEM_MODE_0                             /* System Flash memory mapped at 0x00000000 */
#define LL_SYSCFG_REMAP_SRAM               (SYSCFG_CFGR1_MEM_MODE_1 | SYSCFG_CFGR1_MEM_MODE_0) /* Embedded SRAM mapped at 0x00000000 */
#if defined(FMC_BANK1)
#define LL_SYSCFG_REMAP_FMC                SYSCFG_CFGR1_MEM_MODE_2                             /*<! FMC Bank (Only the first two banks) */
#endif /* FMC_BANK1 */
/**
  * @}
  */

#if defined(SYSCFG_CFGR3_SPI1_RX_DMA_RMP)
/** @defgroup SYSTEM_LL_EC_SPI1_DMA_RMP_RX SYSCFG SPI1 RX/TX DMA1 request REMAP
  * @{
  */
#define LL_SYSCFG_SPI1RX_RMP_DMA1_CH2    (SYSCFG_CFGR3_SPI1_RX_DMA_RMP << 16U | (uint32_t)0x00000000U)          /*!< SPI1_RX mapped on DMA1 CH2 */
#define LL_SYSCFG_SPI1RX_RMP_DMA1_CH4    (SYSCFG_CFGR3_SPI1_RX_DMA_RMP << 16U | SYSCFG_CFGR3_SPI1_RX_DMA_RMP_0) /*!< SPI1_RX mapped on DMA1 CH4 */
#define LL_SYSCFG_SPI1RX_RMP_DMA1_CH6    (SYSCFG_CFGR3_SPI1_RX_DMA_RMP << 16U | SYSCFG_CFGR3_SPI1_RX_DMA_RMP_1) /*!< SPI1_RX mapped on DMA1 CH6 */
#define LL_SYSCFG_SPI1TX_RMP_DMA1_CH3    (SYSCFG_CFGR3_SPI1_TX_DMA_RMP << 16U | (uint32_t)0x00000000U)          /*!< SPI1_TX mapped on DMA1 CH3 */
#define LL_SYSCFG_SPI1TX_RMP_DMA1_CH5    (SYSCFG_CFGR3_SPI1_TX_DMA_RMP << 16U | SYSCFG_CFGR3_SPI1_TX_DMA_RMP_0) /*!< SPI1_TX mapped on DMA1 CH5 */
#define LL_SYSCFG_SPI1TX_RMP_DMA1_CH7    (SYSCFG_CFGR3_SPI1_TX_DMA_RMP << 16U | SYSCFG_CFGR3_SPI1_TX_DMA_RMP_1) /*!< SPI1_TX mapped on DMA1 CH7 */
/**
  * @}
  */
#endif /* SYSCFG_CFGR3_SPI1_RX_DMA_RMP */

#if defined(SYSCFG_CFGR3_I2C1_RX_DMA_RMP)
/** @defgroup SYSTEM_LL_EC_I2C1_DMA_RMP_RX SYSCFG I2C1 RX/TX DMA1 request REMAP
  * @{
  */
#define LL_SYSCFG_I2C1RX_RMP_DMA1_CH7    (SYSCFG_CFGR3_I2C1_RX_DMA_RMP << 16U | (uint32_t)0x00000000U)          /*!< I2C1_RX mapped on DMA1 CH7 */
#define LL_SYSCFG_I2C1RX_RMP_DMA1_CH3    (SYSCFG_CFGR3_I2C1_RX_DMA_RMP << 16U | SYSCFG_CFGR3_I2C1_RX_DMA_RMP_0) /*!< I2C1_RX mapped on DMA1 CH3 */
#define LL_SYSCFG_I2C1RX_RMP_DMA1_CH5    (SYSCFG_CFGR3_I2C1_RX_DMA_RMP << 16U | SYSCFG_CFGR3_I2C1_RX_DMA_RMP_1) /*!< I2C1_RX mapped on DMA1 CH5 */
#define LL_SYSCFG_I2C1TX_RMP_DMA1_CH6    (SYSCFG_CFGR3_I2C1_TX_DMA_RMP << 16U | (uint32_t)0x00000000U)          /*!< I2C1_TX mapped on DMA1 CH6 */
#define LL_SYSCFG_I2C1TX_RMP_DMA1_CH2    (SYSCFG_CFGR3_I2C1_TX_DMA_RMP << 16U | SYSCFG_CFGR3_I2C1_TX_DMA_RMP_0) /*!< I2C1_TX mapped on DMA1 CH2 */
#define LL_SYSCFG_I2C1TX_RMP_DMA1_CH4    (SYSCFG_CFGR3_I2C1_TX_DMA_RMP << 16U | SYSCFG_CFGR3_I2C1_TX_DMA_RMP_1) /*!< I2C1_TX mapped on DMA1 CH4 */
/**
  * @}
  */

#endif /* SYSCFG_CFGR3_I2C1_RX_DMA_RMP */

#if defined(SYSCFG_CFGR1_ADC24_DMA_RMP) || defined(SYSCFG_CFGR3_ADC2_DMA_RMP)
/** @defgroup SYSTEM_LL_EC_ADC24_DMA_REMAP SYSCFG ADC DMA request REMAP
  * @{
  */
#if defined (SYSCFG_CFGR1_ADC24_DMA_RMP)
#define LL_SYSCFG_ADC24_RMP_DMA2_CH12    (SYSCFG_OFFSET_CFGR1 << 24U | SYSCFG_CFGR1_ADC24_DMA_RMP << 8U | (uint32_t)0x00000000U)        /*!< ADC24 DMA requests mapped on DMA2 channels 1 and 2 */
#define LL_SYSCFG_ADC24_RMP_DMA2_CH34    (SYSCFG_OFFSET_CFGR1 << 24U | SYSCFG_CFGR1_ADC24_DMA_RMP << 8U | SYSCFG_CFGR1_ADC24_DMA_RMP)   /*!< ADC24 DMA requests mapped on DMA2 channels 3 and 4 */
#endif /*SYSCFG_CFGR1_ADC24_DMA_RMP*/
#if defined (SYSCFG_CFGR3_ADC2_DMA_RMP)
#define LL_SYSCFG_ADC2_RMP_DMA1_CH2      (SYSCFG_OFFSET_CFGR3 << 24U | SYSCFG_CFGR3_ADC2_DMA_RMP_0 << 8U | (uint32_t)0x00000000U)       /*!< ADC2 mapped on DMA1 channel 2 */
#define LL_SYSCFG_ADC2_RMP_DMA1_CH4      (SYSCFG_OFFSET_CFGR3 << 24U | SYSCFG_CFGR3_ADC2_DMA_RMP_0 << 8U | SYSCFG_CFGR3_ADC2_DMA_RMP_0) /*!< ADC2 mapped on DMA1 channel 4 */
#define LL_SYSCFG_ADC2_RMP_DMA2          (SYSCFG_OFFSET_CFGR3 << 24U | SYSCFG_CFGR3_ADC2_DMA_RMP_1 << 8U | (uint32_t)0x00000000U)       /*!< ADC2 mapped on DMA2 */
#define LL_SYSCFG_ADC2_RMP_DMA1          (SYSCFG_OFFSET_CFGR3 << 24U | SYSCFG_CFGR3_ADC2_DMA_RMP_1 << 8U | SYSCFG_CFGR3_ADC2_DMA_RMP_1) /*!< ADC2 mapped on DMA1 */
#endif /*SYSCFG_CFGR3_ADC2_DMA_RMP*/
/**
  * @}
  */

#endif /* SYSCFG_CFGR1_ADC24_DMA_RMP || SYSCFG_CFGR3_ADC2_DMA_RMP */

/** @defgroup SYSTEM_LL_EC_DAC1_DMA2_REMAP SYSCFG DAC1/2 DMA1/2 request REMAP
  * @{
  */
#define LL_SYSCFG_DAC1_CH1_RMP_DMA2_CH3     ((SYSCFG_CFGR1_TIM6DAC1Ch1_DMA_RMP << 8U) | (uint32_t)0x00000000U)              /*!< DAC_CH1 DMA requests mapped on DMA2 channel 3 */
#define LL_SYSCFG_DAC1_CH1_RMP_DMA1_CH3     ((SYSCFG_CFGR1_TIM6DAC1Ch1_DMA_RMP << 8U) | SYSCFG_CFGR1_TIM6DAC1Ch1_DMA_RMP)   /*!< DAC_CH1 DMA requests mapped on DMA1 channel 3 */
#if defined(SYSCFG_CFGR1_TIM7DAC1Ch2_DMA_RMP)
#define LL_SYSCFG_DAC1_OUT2_RMP_DMA2_CH4    ((SYSCFG_CFGR1_TIM7DAC1Ch2_DMA_RMP << 8U) | (uint32_t)0x00000000U)              /*!< DAC1_OUT2 DMA requests mapped on DMA2 channel 4 */
#define LL_SYSCFG_DAC1_OUT2_RMP_DMA1_CH4    ((SYSCFG_CFGR1_TIM7DAC1Ch2_DMA_RMP << 8U) | SYSCFG_CFGR1_TIM7DAC1Ch2_DMA_RMP)   /*!< DAC1_OUT2 DMA requests mapped on DMA1 channel 4 */
#endif /*SYSCFG_CFGR1_TIM7DAC1Ch2_DMA_RMP*/
#if defined(SYSCFG_CFGR1_TIM18DAC2Ch1_DMA_RMP)
#define LL_SYSCFG_DAC2_OUT1_RMP_DMA2_CH5    ((SYSCFG_CFGR1_TIM18DAC2Ch1_DMA_RMP << 8U) | (uint32_t)0x00000000U)             /*!< DAC2_OUT1 DMA requests mapped on DMA2 channel 5 */
#define LL_SYSCFG_DAC2_OUT1_RMP_DMA1_CH5    ((SYSCFG_CFGR1_TIM18DAC2Ch1_DMA_RMP << 8U) | SYSCFG_CFGR1_TIM18DAC2Ch1_DMA_RMP) /*!< DAC2_OUT1 DMA requests mapped on DMA1 channel 5 */
#endif /*SYSCFG_CFGR1_TIM18DAC2Ch1_DMA_RMP*/
#if defined(SYSCFG_CFGR1_DAC2Ch1_DMA_RMP)
#define LL_SYSCFG_DAC2_CH1_RMP_NO           ((SYSCFG_CFGR1_DAC2Ch1_DMA_RMP << 8U) | (uint32_t)0x00000000U)                  /*!< No remap */
#define LL_SYSCFG_DAC2_CH1_RMP_DMA1_CH5     ((SYSCFG_CFGR1_DAC2Ch1_DMA_RMP << 8U) | SYSCFG_CFGR1_DAC2Ch1_DMA_RMP)           /*!< DAC2_CH1 DMA requests mapped on DMA1 channel 5 */
#endif /*SYSCFG_CFGR1_DAC2Ch1_DMA_RMP*/
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_TIM16_DMA1_REMAP SYSCFG TIM DMA request REMAP
  * @{
  */
#define LL_SYSCFG_TIM16_RMP_DMA1_CH3        ((SYSCFG_CFGR1_TIM16_DMA_RMP << 8U) | (uint32_t)0x00000000U)                     /*!< TIM16_CH1 and TIM16_UP DMA requests mapped on DMA1 channel 3 */
#define LL_SYSCFG_TIM16_RMP_DMA1_CH6        ((SYSCFG_CFGR1_TIM16_DMA_RMP << 8U) | SYSCFG_CFGR1_TIM16_DMA_RMP)                /*!< TIM16_CH1 and TIM16_UP DMA requests mapped on DMA1 channel 6 */
#define LL_SYSCFG_TIM17_RMP_DMA1_CH1        ((SYSCFG_CFGR1_TIM17_DMA_RMP << 8U) | (uint32_t)0x00000000U)                     /*!< TIM17_CH1 and TIM17_UP DMA requests mapped on DMA1 channel 1 */
#define LL_SYSCFG_TIM17_RMP_DMA1_CH7        ((SYSCFG_CFGR1_TIM17_DMA_RMP << 8U) | SYSCFG_CFGR1_TIM17_DMA_RMP)                /*!< TIM17_CH1 and TIM17_UP DMA requests mapped on DMA1 channel 7 */
#define LL_SYSCFG_TIM6_RMP_DMA2_CH3         ((SYSCFG_CFGR1_TIM6DAC1Ch1_DMA_RMP << 8U) | (uint32_t)0x00000000U)               /*!< TIM6 DMA requests mapped on DMA2 channel 3 */
#define LL_SYSCFG_TIM6_RMP_DMA1_CH3         ((SYSCFG_CFGR1_TIM6DAC1Ch1_DMA_RMP << 8U) | SYSCFG_CFGR1_TIM6DAC1Ch1_DMA_RMP)    /*!< TIM6 DMA requests mapped on DMA1 channel 3 */
#if defined(SYSCFG_CFGR1_TIM7DAC1Ch2_DMA_RMP)
#define LL_SYSCFG_TIM7_RMP_DMA2_CH4         ((SYSCFG_CFGR1_TIM7DAC1Ch2_DMA_RMP << 8U) | (uint32_t)0x00000000U)               /*!< TIM7 DMA requests mapped on DMA2 channel 4 */
#define LL_SYSCFG_TIM7_RMP_DMA1_CH4         ((SYSCFG_CFGR1_TIM7DAC1Ch2_DMA_RMP << 8U) | SYSCFG_CFGR1_TIM7DAC1Ch2_DMA_RMP)    /*!< TIM7 DMA requests mapped on DMA1 channel 4 */
#endif /*SYSCFG_CFGR1_TIM7DAC1Ch2_DMA_RMP*/
#if defined(SYSCFG_CFGR1_TIM18DAC2Ch1_DMA_RMP)
#define LL_SYSCFG_TIM18_RMP_DMA2_CH5        ((SYSCFG_CFGR1_TIM18DAC2Ch1_DMA_RMP << 8U) | (uint32_t)0x00000000U)              /*!< TIM18 DMA requests mapped on DMA2 channel 5 */
#define LL_SYSCFG_TIM18_RMP_DMA1_CH5        ((SYSCFG_CFGR1_TIM18DAC2Ch1_DMA_RMP << 8U) | SYSCFG_CFGR1_TIM18DAC2Ch1_DMA_RMP)  /*!< TIM18 DMA requests mapped on DMA1 channel 5 */
#endif /*SYSCFG_CFGR1_TIM18DAC2Ch1_DMA_RMP*/
/**
  * @}
  */

#if defined(SYSCFG_CFGR1_TIM1_ITR3_RMP) || defined(SYSCFG_CFGR1_ENCODER_MODE)
/** @defgroup SYSTEM_LL_EC_TIM1_ITR3_RMP_TIM4 SYSCFG TIM REMAP
  * @{
  */
#if defined(SYSCFG_CFGR1_TIM1_ITR3_RMP)
#define LL_SYSCFG_TIM1_ITR3_RMP_TIM4_TRGO      ((SYSCFG_CFGR1_TIM1_ITR3_RMP << 8U) | (uint32_t)0x00000000U)              /*!< TIM1_ITR3 = TIM4_TRGO */
#define LL_SYSCFG_TIM1_ITR3_RMP_TIM17_OC       ((SYSCFG_CFGR1_TIM1_ITR3_RMP << 8U) | SYSCFG_CFGR1_TIM1_ITR3_RMP)         /*!< TIM1_ITR3 = TIM17_OC */
#endif /* SYSCFG_CFGR1_TIM1_ITR3_RMP */
#if defined(SYSCFG_CFGR1_ENCODER_MODE)
#define LL_SYSCFG_TIM15_ENCODEMODE_NOREDIRECTION ((SYSCFG_CFGR1_ENCODER_MODE << 8U) | (uint32_t)0x00000000U)               /*!< No redirection */
#define LL_SYSCFG_TIM15_ENCODEMODE_TIM2          ((SYSCFG_CFGR1_ENCODER_MODE_0 << 8U) | SYSCFG_CFGR1_ENCODER_MODE_0)       /*!< TIM2 IC1 and TIM2 IC2 are connected to TIM15 IC1 and TIM15 IC2 respectively */
#if defined(SYSCFG_CFGR1_ENCODER_MODE_TIM3)
#define LL_SYSCFG_TIM15_ENCODEMODE_TIM3          ((SYSCFG_CFGR1_ENCODER_MODE_TIM3 << 8U) | SYSCFG_CFGR1_ENCODER_MODE_TIM3) /*!< TIM3 IC1 and TIM3 IC2 are connected to TIM15 IC1 and TIM15 IC2 respectively */
#endif /* SYSCFG_CFGR1_ENCODER_MODE_TIM3 */
#if defined(SYSCFG_CFGR1_ENCODER_MODE_TIM4)
#define LL_SYSCFG_TIM15_ENCODEMODE_TIM4          ((SYSCFG_CFGR1_ENCODER_MODE_TIM4 << 8U) | SYSCFG_CFGR1_ENCODER_MODE_TIM4) /*!< TIM4 IC1 and TIM4 IC2 are connected to TIM15 IC1 and TIM15 IC2 respectively */
#endif /* SYSCFG_CFGR1_ENCODER_MODE_TIM4 */
#endif /* SYSCFG_CFGR1_ENCODER_MODE */
/**
  * @}
  */

#endif /* SYSCFG_CFGR1_TIM1_ITR3_RMP || SYSCFG_CFGR1_ENCODER_MODE */

#if defined(SYSCFG_CFGR4_ADC12_EXT2_RMP)
/** @defgroup SYSTEM_LL_EC_ADC12_EXT2_RMP_TIM1 SYSCFG ADC Trigger REMAP
  * @{
  */
#define LL_SYSCFG_ADC12_EXT2_RMP_TIM1_CC3      ((SYSCFG_CFGR4_ADC12_EXT2_RMP << 16U) | (uint32_t)0x00000000U)           /*!< Input trigger of ADC12 regular channel EXT2:Trigger source is TIM1_CC3 */
#define LL_SYSCFG_ADC12_EXT2_RMP_TIM20_TRGO    ((SYSCFG_CFGR4_ADC12_EXT2_RMP << 16U) | SYSCFG_CFGR4_ADC12_EXT2_RMP)     /*!< Input trigger of ADC12 regular channel EXT2:Trigger source is TIM20_TRGO */
#define LL_SYSCFG_ADC12_EXT3_RMP_TIM2_CC2      ((SYSCFG_CFGR4_ADC12_EXT3_RMP << 16U) | (uint32_t)0x00000000U)           /*!< Input trigger of ADC12 regular channel EXT3:Trigger source is TIM2_CC2 */
#define LL_SYSCFG_ADC12_EXT3_RMP_TIM20_TRGO2   ((SYSCFG_CFGR4_ADC12_EXT3_RMP << 16U) | SYSCFG_CFGR4_ADC12_EXT3_RMP)     /*!< Input trigger of ADC12 regular channel EXT3:Trigger source is TIM20_TRGO2 */
#define LL_SYSCFG_ADC12_EXT5_RMP_TIM4_CC4      ((SYSCFG_CFGR4_ADC12_EXT5_RMP << 16U) | (uint32_t)0x00000000U)           /*!< Input trigger of ADC12 regular channel EXT5:Trigger source is TIM4_CC4 */
#define LL_SYSCFG_ADC12_EXT5_RMP_TIM20_CC1     ((SYSCFG_CFGR4_ADC12_EXT5_RMP << 16U) | SYSCFG_CFGR4_ADC12_EXT5_RMP)     /*!< Input trigger of ADC12 regular channel EXT5:Trigger source is TIM20_CC1 */
#define LL_SYSCFG_ADC12_EXT13_RMP_TIM6_TRGO    ((SYSCFG_CFGR4_ADC12_EXT13_RMP << 16U) | (uint32_t)0x00000000U)          /*!< Input trigger of ADC12 regular channel EXT13:Trigger source is TIM6_TRGO */
#define LL_SYSCFG_ADC12_EXT13_RMP_TIM20_CC2    ((SYSCFG_CFGR4_ADC12_EXT13_RMP << 16U) | SYSCFG_CFGR4_ADC12_EXT13_RMP)   /*!< Input trigger of ADC12 regular channel EXT13:Trigger source is TIM20_CC2 */
#define LL_SYSCFG_ADC12_EXT15_RMP_TIM3_CC4     ((SYSCFG_CFGR4_ADC12_EXT15_RMP << 16U) | (uint32_t)0x00000000U)          /*!< Input trigger of ADC12 regular channel EXT15:Trigger source is TIM3_CC4 */
#define LL_SYSCFG_ADC12_EXT15_RMP_TIM20_CC3    ((SYSCFG_CFGR4_ADC12_EXT15_RMP << 16U) | SYSCFG_CFGR4_ADC12_EXT15_RMP)   /*!< Input trigger of ADC12 regular channel EXT15:Trigger source is TIM20_CC3 */
#define LL_SYSCFG_ADC12_JEXT3_RMP_TIM2_CC1     ((SYSCFG_CFGR4_ADC12_JEXT3_RMP << 16U) | (uint32_t)0x00000000U)          /*!< Input trigger of ADC12 regular channel JEXT3:Trigger source is TIM2_CC1 */
#define LL_SYSCFG_ADC12_JEXT3_RMP_TIM20_TRGO   ((SYSCFG_CFGR4_ADC12_JEXT3_RMP << 16U) | SYSCFG_CFGR4_ADC12_JEXT3_RMP)   /*!< Input trigger of ADC12 regular channel JEXT3:Trigger source is TIM20_TRGO */
#define LL_SYSCFG_ADC12_JEXT6_RMP_EXTI_LINE_15 ((SYSCFG_CFGR4_ADC12_JEXT6_RMP << 16U) | (uint32_t)0x00000000U)          /*!< Input trigger of ADC12 regular channel JEXT6:Trigger source is EXTI_LINE_15 */
#define LL_SYSCFG_ADC12_JEXT6_RMP_TIM20_TRGO2  ((SYSCFG_CFGR4_ADC12_JEXT6_RMP << 16U) | SYSCFG_CFGR4_ADC12_JEXT6_RMP)   /*!< Input trigger of ADC12 regular channel JEXT6:Trigger source is TIM20_TRGO2 */
#define LL_SYSCFG_ADC12_JEXT13_RMP_TIM3_CC1    ((SYSCFG_CFGR4_ADC12_JEXT13_RMP << 16U) | (uint32_t)0x00000000U)         /*!< Input trigger of ADC12 regular channel JEXT13:Trigger source is TIM3_CC1 */
#define LL_SYSCFG_ADC12_JEXT13_RMP_TIM20_CC4   ((SYSCFG_CFGR4_ADC12_JEXT13_RMP << 16U) | SYSCFG_CFGR4_ADC12_JEXT13_RMP) /*!< Input trigger of ADC12 regular channel JEXT13:Trigger source is TIM20_CC4 */
#define LL_SYSCFG_ADC34_EXT5_RMP_EXTI_LINE_2   ((SYSCFG_CFGR4_ADC34_EXT5_RMP << 16U) | (uint32_t)0x00000000U)           /*!< Input trigger of ADC34 regular channel EXT5:Trigger source is EXTI_LINE_2 */
#define LL_SYSCFG_ADC34_EXT5_RMP_TIM20_TRGO    ((SYSCFG_CFGR4_ADC34_EXT5_RMP << 16U) | SYSCFG_CFGR4_ADC34_EXT5_RMP)     /*!< Input trigger of ADC34 regular channel EXT5:Trigger source is TIM20_TRGO */
#define LL_SYSCFG_ADC34_EXT6_RMP_TIM4_CC1      ((SYSCFG_CFGR4_ADC34_EXT6_RMP << 16U) | (uint32_t)0x00000000U)           /*!< Input trigger of ADC34 regular channel EXT6:Trigger source is TIM4_CC1 */
#define LL_SYSCFG_ADC34_EXT6_RMP_TIM20_TRGO2   ((SYSCFG_CFGR4_ADC34_EXT6_RMP << 16U) | SYSCFG_CFGR4_ADC34_EXT6_RMP)     /*!< Input trigger of ADC34 regular channel EXT6:Trigger source is TIM20_TRGO2 */
#define LL_SYSCFG_ADC34_EXT15_RMP_TIM2_CC1     ((SYSCFG_CFGR4_ADC34_EXT15_RMP << 16U) | (uint32_t)0x00000000U)          /*!< Input trigger of ADC34 regular channel EXT15:Trigger source is  TIM2_CC1 */
#define LL_SYSCFG_ADC34_EXT15_RMP_TIM20_CC1    ((SYSCFG_CFGR4_ADC34_EXT15_RMP << 16U) | SYSCFG_CFGR4_ADC34_EXT15_RMP)   /*!< Input trigger of ADC34 regular channel EXT15:Trigger source is TIM20_CC1 */
#define LL_SYSCFG_ADC34_JEXT5_RMP_TIM4_CC3     ((SYSCFG_CFGR4_ADC34_JEXT5_RMP << 16U) | (uint32_t)0x00000000U)          /*!< Input trigger of ADC34 regular channel JEXT5:Trigger source is TIM4_CC3 */
#define LL_SYSCFG_ADC34_JEXT5_RMP_TIM20_TRGO   ((SYSCFG_CFGR4_ADC34_JEXT5_RMP << 16U) | SYSCFG_CFGR4_ADC34_JEXT5_RMP)   /*!< Input trigger of ADC34 regular channel JEXT5:Trigger source is TIM20_TRGO */
#define LL_SYSCFG_ADC34_JEXT11_RMP_TIM1_CC3    ((SYSCFG_CFGR4_ADC34_JEXT11_RMP << 16U) | (uint32_t)0x00000000U)         /*!< Input trigger of ADC34 regular channel JEXT11:Trigger source is TIM1_CC3 */
#define LL_SYSCFG_ADC34_JEXT11_RMP_TIM20_TRGO2 ((SYSCFG_CFGR4_ADC34_JEXT11_RMP << 16U) | SYSCFG_CFGR4_ADC34_JEXT11_RMP) /*!< Input trigger of ADC34 regular channel JEXT11:Trigger source is TIM20_TRGO2 */
#define LL_SYSCFG_ADC34_JEXT14_RMP_TIM7_TRGO   ((SYSCFG_CFGR4_ADC34_JEXT14_RMP << 16U) | (uint32_t)0x00000000U)         /*!< Input trigger of ADC34 regular channel JEXT14:Trigger source is TIM7_TRGO */
#define LL_SYSCFG_ADC34_JEXT14_RMP_TIM20_CC2   ((SYSCFG_CFGR4_ADC34_JEXT14_RMP << 16U) | SYSCFG_CFGR4_ADC34_JEXT14_RMP) /*!< Input trigger of ADC34 regular channel JEXT14:Trigger source is TIM20_CC2 */
/**
  * @}
  */

#endif /* SYSCFG_CFGR4_ADC12_EXT2_RMP */

#if defined(SYSCFG_CFGR1_DAC1_TRIG1_RMP) || defined(SYSCFG_CFGR3_TRIGGER_RMP)
/** @defgroup SYSTEM_LL_EC_DAC1_TRIG1_REMAP SYSCFG DAC1 Trigger REMAP
  * @{
  */
#if defined(SYSCFG_CFGR1_DAC1_TRIG1_RMP)
#define LL_SYSCFG_DAC1_TRIG1_RMP_TIM8_TRGO         (SYSCFG_OFFSET_CFGR1 << 24U | SYSCFG_CFGR1_DAC1_TRIG1_RMP << 4 | (uint32_t)0x00000000U)       /*!< No remap: DAC trigger TRIG1 is TIM8_TRGO */
#define LL_SYSCFG_DAC1_TRIG1_RMP_TIM3_TRGO         (SYSCFG_OFFSET_CFGR1 << 24U | SYSCFG_CFGR1_DAC1_TRIG1_RMP << 4 | SYSCFG_CFGR1_DAC1_TRIG1_RMP) /*!< DAC trigger is TIM3_TRGO */
#endif /* SYSCFG_CFGR1_DAC1_TRIG1_RMP */
#if defined(SYSCFG_CFGR3_DAC1_TRG3_RMP)
#define LL_SYSCFG_DAC1_TRIG3_RMP_TIM15_TRGO        (SYSCFG_OFFSET_CFGR3 << 24U | SYSCFG_CFGR3_DAC1_TRG3_RMP << 4 | (uint32_t)0x00000000U)        /*!< DAC trigger is TIM15_TRGO */
#define LL_SYSCFG_DAC1_TRIG3_RMP_HRTIM1_DAC1_TRIG1 (SYSCFG_OFFSET_CFGR3 << 24U | SYSCFG_CFGR3_DAC1_TRG3_RMP << 4 | SYSCFG_CFGR3_DAC1_TRG3_RMP)   /*!< DAC trigger is HRTIM1_DAC1_TRIG1 */
#endif /* SYSCFG_CFGR3_DAC1_TRG3_RMP */
#if defined(SYSCFG_CFGR3_DAC1_TRG5_RMP)
#define LL_SYSCFG_DAC1_TRIG5_RMP_NO                (SYSCFG_OFFSET_CFGR3 << 24U | SYSCFG_CFGR3_DAC1_TRG5_RMP << 4 | (uint32_t)0x00000000U)        /*!<  No remap  */
#define LL_SYSCFG_DAC1_TRIG5_RMP_HRTIM1_DAC1_TRIG2 (SYSCFG_OFFSET_CFGR3 << 24U | SYSCFG_CFGR3_DAC1_TRG5_RMP << 4 | SYSCFG_CFGR3_DAC1_TRG5_RMP)   /*!< DAC trigger is HRTIM1_DAC1_TRIG2 */
#endif /* SYSCFG_CFGR3_DAC1_TRG5_RMP */
/**
  * @}
  */

#endif /* SYSCFG_CFGR1_DAC1_TRIG1_RMP || SYSCFG_CFGR3_TRIGGER_RMP */

/** @defgroup SYSTEM_LL_EC_I2C_FASTMODEPLUS SYSCFG I2C FASTMODEPLUS
  * @{
  */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB6     SYSCFG_CFGR1_I2C_PB6_FMP  /*!< I2C PB6 Fast mode plus */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB7     SYSCFG_CFGR1_I2C_PB7_FMP  /*!< I2C PB7 Fast mode plus */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB8     SYSCFG_CFGR1_I2C_PB8_FMP  /*!< I2C PB8 Fast mode plus */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB9     SYSCFG_CFGR1_I2C_PB9_FMP  /*!< I2C PB9 Fast mode plus */
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C1    SYSCFG_CFGR1_I2C1_FMP     /*!< I2C1 Fast mode plus    */
#if defined(SYSCFG_CFGR1_I2C2_FMP)
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C2    SYSCFG_CFGR1_I2C2_FMP     /*!< I2C2 Fast mode plus    */
#endif /*SYSCFG_CFGR1_I2C2_FMP*/
#if defined(SYSCFG_CFGR1_I2C3_FMP)
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C3    SYSCFG_CFGR1_I2C3_FMP     /*!< I2C3 Fast mode plus    */
#endif /*SYSCFG_CFGR1_I2C3_FMP*/
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_EXTI_PORT SYSCFG EXTI PORT
  * @{
  */
#define LL_SYSCFG_EXTI_PORTA               (uint32_t)0U /*!< EXTI PORT A  */
#define LL_SYSCFG_EXTI_PORTB               (uint32_t)1U /*!< EXTI PORT B  */
#define LL_SYSCFG_EXTI_PORTC               (uint32_t)2U /*!< EXTI PORT C  */
#define LL_SYSCFG_EXTI_PORTD               (uint32_t)3U /*!< EXTI PORT D  */
#if defined(GPIOE)
#define LL_SYSCFG_EXTI_PORTE               (uint32_t)4U /*!< EXTI PORT E  */
#endif /* GPIOE */
#define LL_SYSCFG_EXTI_PORTF               (uint32_t)5U /*!< EXTI PORT F  */
#if defined(GPIOG)
#define LL_SYSCFG_EXTI_PORTG               (uint32_t)6U /*!< EXTI PORT G  */
#endif /* GPIOG */
#if defined(GPIOH)
#define LL_SYSCFG_EXTI_PORTH               (uint32_t)7U /*!< EXTI PORT H  */
#endif /* GPIOH */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_EXTI_LINE SYSCFG EXTI LINE
  * @{
  */
#define LL_SYSCFG_EXTI_LINE0               (uint32_t)(0x000FU << 16U | 0U)  /* EXTI_POSITION_0  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE1               (uint32_t)(0x00F0U << 16U | 0U)  /* EXTI_POSITION_4  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE2               (uint32_t)(0x0F00U << 16U | 0U)  /* EXTI_POSITION_8  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE3               (uint32_t)(0xF000U << 16U | 0U)  /* EXTI_POSITION_12 | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE4               (uint32_t)(0x000FU << 16U | 1U)  /* EXTI_POSITION_0  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE5               (uint32_t)(0x00F0U << 16U | 1U)  /* EXTI_POSITION_4  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE6               (uint32_t)(0x0F00U << 16U | 1U)  /* EXTI_POSITION_8  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE7               (uint32_t)(0xF000U << 16U | 1U)  /* EXTI_POSITION_12 | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE8               (uint32_t)(0x000FU << 16U | 2U)  /* EXTI_POSITION_0  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE9               (uint32_t)(0x00F0U << 16U | 2U)  /* EXTI_POSITION_4  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE10              (uint32_t)(0x0F00U << 16U | 2U)  /* EXTI_POSITION_8  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE11              (uint32_t)(0xF000U << 16U | 2U)  /* EXTI_POSITION_12 | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE12              (uint32_t)(0x000FU << 16U | 3U)  /* EXTI_POSITION_0  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE13              (uint32_t)(0x00F0U << 16U | 3U)  /* EXTI_POSITION_4  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE14              (uint32_t)(0x0F00U << 16U | 3U)  /* EXTI_POSITION_8  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE15              (uint32_t)(0xF000U << 16U | 3U)  /* EXTI_POSITION_12 | EXTICR[3] */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_TIMBREAK SYSCFG TIMER BREAK
  * @{
  */
#if defined(SYSCFG_CFGR2_PVD_LOCK)
#define LL_SYSCFG_TIMBREAK_PVD             SYSCFG_CFGR2_PVD_LOCK           /*!< Enables and locks the PVD connection with TIMx Break Input and also the PVDE and PLS bits of the Power Control Interface */
#endif /*SYSCFG_CFGR2_PVD_LOCK*/
#if defined(SYSCFG_CFGR2_SRAM_PARITY_LOCK)
#define LL_SYSCFG_TIMBREAK_SRAM_PARITY     SYSCFG_CFGR2_SRAM_PARITY_LOCK   /*!< Enables and locks the SRAM_PARITY error signal with Break Input of TIMx */
#endif /* SYSCFG_CFGR2_SRAM_PARITY_LOCK */
#define LL_SYSCFG_TIMBREAK_LOCKUP          SYSCFG_CFGR2_LOCKUP_LOCK        /*!< Enables and locks the LOCKUP (Hardfault) output of CortexM0 with Break Input of TIMx */
/**
  * @}
  */

#if defined(SYSCFG_RCR_PAGE0)
/** @defgroup SYSTEM_LL_EC_CCMSRAMWRP SYSCFG CCM SRAM WRP
  * @{
  */
#define LL_SYSCFG_CCMSRAMWRP_PAGE0         SYSCFG_RCR_PAGE0  /*!< ICODE SRAM Write protection page 0  */
#define LL_SYSCFG_CCMSRAMWRP_PAGE1         SYSCFG_RCR_PAGE1  /*!< ICODE SRAM Write protection page 1  */
#define LL_SYSCFG_CCMSRAMWRP_PAGE2         SYSCFG_RCR_PAGE2  /*!< ICODE SRAM Write protection page 2  */
#define LL_SYSCFG_CCMSRAMWRP_PAGE3         SYSCFG_RCR_PAGE3  /*!< ICODE SRAM Write protection page 3  */
#if defined(SYSCFG_RCR_PAGE4)
#define LL_SYSCFG_CCMSRAMWRP_PAGE4         SYSCFG_RCR_PAGE4  /*!< ICODE SRAM Write protection page 4  */
#define LL_SYSCFG_CCMSRAMWRP_PAGE5         SYSCFG_RCR_PAGE5  /*!< ICODE SRAM Write protection page 5  */
#define LL_SYSCFG_CCMSRAMWRP_PAGE6         SYSCFG_RCR_PAGE6  /*!< ICODE SRAM Write protection page 6  */
#define LL_SYSCFG_CCMSRAMWRP_PAGE7         SYSCFG_RCR_PAGE7  /*!< ICODE SRAM Write protection page 7  */
#endif
#if defined(SYSCFG_RCR_PAGE8)
#define LL_SYSCFG_CCMSRAMWRP_PAGE8         SYSCFG_RCR_PAGE8  /*!< ICODE SRAM Write protection page 8  */
#define LL_SYSCFG_CCMSRAMWRP_PAGE9         SYSCFG_RCR_PAGE9  /*!< ICODE SRAM Write protection page 9  */
#define LL_SYSCFG_CCMSRAMWRP_PAGE10        SYSCFG_RCR_PAGE10 /*!< ICODE SRAM Write protection page 10 */
#define LL_SYSCFG_CCMSRAMWRP_PAGE11        SYSCFG_RCR_PAGE11 /*!< ICODE SRAM Write protection page 11 */
#define LL_SYSCFG_CCMSRAMWRP_PAGE12        SYSCFG_RCR_PAGE12 /*!< ICODE SRAM Write protection page 12 */
#define LL_SYSCFG_CCMSRAMWRP_PAGE13        SYSCFG_RCR_PAGE13 /*!< ICODE SRAM Write protection page 13 */
#define LL_SYSCFG_CCMSRAMWRP_PAGE14        SYSCFG_RCR_PAGE14 /*!< ICODE SRAM Write protection page 14 */
#define LL_SYSCFG_CCMSRAMWRP_PAGE15        SYSCFG_RCR_PAGE15 /*!< ICODE SRAM Write protection page 15 */
#endif
/**
  * @}
  */

#endif /* SYSCFG_RCR_PAGE0 */

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
#define LL_DBGMCU_APB1_GRP1_TIM2_STOP      DBGMCU_APB1_FZ_DBG_TIM2_STOP          /*!< TIM2 counter stopped when core is halted */
#if defined(DBGMCU_APB1_FZ_DBG_TIM3_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM3_STOP      DBGMCU_APB1_FZ_DBG_TIM3_STOP          /*!< TIM3 counter stopped when core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_TIM3_STOP*/
#if defined(DBGMCU_APB1_FZ_DBG_TIM4_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM4_STOP      DBGMCU_APB1_FZ_DBG_TIM4_STOP          /*!< TIM4 counter stopped when core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_TIM4_STOP*/
#if defined(DBGMCU_APB1_FZ_DBG_TIM5_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM5_STOP      DBGMCU_APB1_FZ_DBG_TIM5_STOP          /*!< TIM5 counter stopped when core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_TIM5_STOP*/
#define LL_DBGMCU_APB1_GRP1_TIM6_STOP      DBGMCU_APB1_FZ_DBG_TIM6_STOP          /*!< TIM6 counter stopped when core is halted */
#if defined(DBGMCU_APB1_FZ_DBG_TIM7_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM7_STOP      DBGMCU_APB1_FZ_DBG_TIM7_STOP          /*!< TIM7 counter stopped when core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_TIM7_STOP*/
#if defined(DBGMCU_APB1_FZ_DBG_TIM12_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM12_STOP     DBGMCU_APB1_FZ_DBG_TIM12_STOP         /*!< TIM12 counter stopped when core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_TIM12_STOP*/
#if defined(DBGMCU_APB1_FZ_DBG_TIM13_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM13_STOP     DBGMCU_APB1_FZ_DBG_TIM13_STOP         /*!< TIM13 counter stopped when core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_TIM13_STOP*/
#if defined(DBGMCU_APB1_FZ_DBG_TIM14_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM14_STOP     DBGMCU_APB1_FZ_DBG_TIM14_STOP         /*!< TIM14 counter stopped when core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_TIM14_STOP*/
#if defined(DBGMCU_APB1_FZ_DBG_TIM18_STOP)
#define LL_DBGMCU_APB1_GRP1_TIM18_STOP     DBGMCU_APB1_FZ_DBG_TIM18_STOP         /*!< TIM18 counter stopped when core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_TIM18_STOP*/
#define LL_DBGMCU_APB1_GRP1_RTC_STOP       DBGMCU_APB1_FZ_DBG_RTC_STOP           /*!< RTC counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_WWDG_STOP      DBGMCU_APB1_FZ_DBG_WWDG_STOP          /*!< Debug Window Watchdog stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_IWDG_STOP      DBGMCU_APB1_FZ_DBG_IWDG_STOP          /*!< Debug Independent Watchdog stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C1_STOP      DBGMCU_APB1_FZ_DBG_I2C1_SMBUS_TIMEOUT /*!< I2C1 SMBUS timeout mode stopped when Core is halted */
#if defined(DBGMCU_APB1_FZ_DBG_I2C2_SMBUS_TIMEOUT)
#define LL_DBGMCU_APB1_GRP1_I2C2_STOP      DBGMCU_APB1_FZ_DBG_I2C2_SMBUS_TIMEOUT /*!< I2C2 SMBUS timeout mode stopped when Core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_I2C2_SMBUS_TIMEOUT*/
#if defined(DBGMCU_APB1_FZ_DBG_I2C3_SMBUS_TIMEOUT)
#define LL_DBGMCU_APB1_GRP1_I2C3_STOP      DBGMCU_APB1_FZ_DBG_I2C3_SMBUS_TIMEOUT /*!< I2C3 SMBUS timeout mode stopped when Core is halted */
#endif /*DBGMCU_APB1_FZ_DBG_I2C3_SMBUS_TIMEOUT*/
#if defined(DBGMCU_APB1_FZ_DBG_CAN_STOP)
#define LL_DBGMCU_APB1_GRP1_CAN_STOP       DBGMCU_APB1_FZ_DBG_CAN_STOP            /*!< CAN debug stopped when Core is halted  */
#endif /*DBGMCU_APB1_FZ_DBG_CAN_STOP*/
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB2_GRP1_STOP_IP DBGMCU APB2 GRP1 STOP IP
  * @{
  */
#if defined(DBGMCU_APB2_FZ_DBG_TIM1_STOP)
#define LL_DBGMCU_APB2_GRP1_TIM1_STOP      DBGMCU_APB2_FZ_DBG_TIM1_STOP   /*!< TIM1 counter stopped when core is halted */
#endif /*DBGMCU_APB2_FZ_DBG_TIM1_STOP*/
#if defined(DBGMCU_APB2_FZ_DBG_TIM8_STOP)
#define LL_DBGMCU_APB2_GRP1_TIM8_STOP      DBGMCU_APB2_FZ_DBG_TIM8_STOP   /*!< TIM8 counter stopped when core is halted */
#endif /*DBGMCU_APB2_FZ_DBG_TIM8_STOP*/
#define LL_DBGMCU_APB2_GRP1_TIM15_STOP     DBGMCU_APB2_FZ_DBG_TIM15_STOP  /*!< TIM15 counter stopped when core is halted */
#define LL_DBGMCU_APB2_GRP1_TIM16_STOP     DBGMCU_APB2_FZ_DBG_TIM16_STOP  /*!< TIM16 counter stopped when core is halted */
#define LL_DBGMCU_APB2_GRP1_TIM17_STOP     DBGMCU_APB2_FZ_DBG_TIM17_STOP  /*!< TIM17 counter stopped when core is halted */
#if defined(DBGMCU_APB2_FZ_DBG_TIM19_STOP)
#define LL_DBGMCU_APB2_GRP1_TIM19_STOP     DBGMCU_APB2_FZ_DBG_TIM19_STOP  /*!< TIM19 counter stopped when core is halted */
#endif /*DBGMCU_APB2_FZ_DBG_TIM19_STOP*/
#if defined(DBGMCU_APB2_FZ_DBG_TIM20_STOP)
#define LL_DBGMCU_APB2_GRP1_TIM20_STOP     DBGMCU_APB2_FZ_DBG_TIM20_STOP  /*!< TIM20 counter stopped when core is halted */
#endif /*DBGMCU_APB2_FZ_DBG_TIM20_STOP*/
#if defined(DBGMCU_APB2_FZ_DBG_HRTIM1_STOP)
#define LL_DBGMCU_APB2_GRP1_HRTIM1_STOP    DBGMCU_APB2_FZ_DBG_HRTIM1_STOP /*!< HRTIM1 counter stopped when core is halted */
#endif /*DBGMCU_APB2_FZ_DBG_HRTIM1_STOP*/
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_LATENCY FLASH LATENCY
  * @{
  */
#define LL_FLASH_LATENCY_0                 0x00000000U             /*!< FLASH Zero Latency cycle */
#define LL_FLASH_LATENCY_1                 FLASH_ACR_LATENCY_0     /*!< FLASH One Latency cycle */
#define LL_FLASH_LATENCY_2                 FLASH_ACR_LATENCY_1     /*!< FLASH Two Latency cycles */
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
  *         @arg @ref LL_SYSCFG_REMAP_FMC (*)
  *
  *         (*) value not defined in all devices.
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
  *         @arg @ref LL_SYSCFG_REMAP_FMC (*)
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetRemapMemory(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_MEM_MODE));
}

#if defined(SYSCFG_CFGR3_SPI1_RX_DMA_RMP)
/**
  * @brief  Set DMA request remapping bits for SPI
  * @rmtoll SYSCFG_CFGR3 SPI1_RX_DMA_RMP  LL_SYSCFG_SetRemapDMA_SPI\n
  *         SYSCFG_CFGR3 SPI1_TX_DMA_RMP  LL_SYSCFG_SetRemapDMA_SPI
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_SPI1RX_RMP_DMA1_CH2
  *         @arg @ref LL_SYSCFG_SPI1RX_RMP_DMA1_CH4
  *         @arg @ref LL_SYSCFG_SPI1RX_RMP_DMA1_CH6
  *         @arg @ref LL_SYSCFG_SPI1TX_RMP_DMA1_CH3
  *         @arg @ref LL_SYSCFG_SPI1TX_RMP_DMA1_CH5
  *         @arg @ref LL_SYSCFG_SPI1TX_RMP_DMA1_CH7
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapDMA_SPI(uint32_t Remap)
{
  MODIFY_REG(SYSCFG->CFGR3, (Remap >> 16U), (Remap & 0x0000FFFF));
}
#endif /* SYSCFG_CFGR3_SPI1_RX_DMA_RMP */

#if defined(SYSCFG_CFGR3_I2C1_RX_DMA_RMP)
/**
  * @brief  Set DMA request remapping bits for I2C
  * @rmtoll SYSCFG_CFGR3 I2C1_RX_DMA_RMP  LL_SYSCFG_SetRemapDMA_I2C\n
  *         SYSCFG_CFGR3 I2C1_TX_DMA_RMP  LL_SYSCFG_SetRemapDMA_I2C
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_I2C1RX_RMP_DMA1_CH7
  *         @arg @ref LL_SYSCFG_I2C1RX_RMP_DMA1_CH3
  *         @arg @ref LL_SYSCFG_I2C1RX_RMP_DMA1_CH5
  *         @arg @ref LL_SYSCFG_I2C1TX_RMP_DMA1_CH6
  *         @arg @ref LL_SYSCFG_I2C1TX_RMP_DMA1_CH2
  *         @arg @ref LL_SYSCFG_I2C1TX_RMP_DMA1_CH4
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapDMA_I2C(uint32_t Remap)
{
  MODIFY_REG(SYSCFG->CFGR3, (Remap >> 16U), (Remap & 0x0000FFFF));
}
#endif /* SYSCFG_CFGR3_I2C1_RX_DMA_RMP */

#if defined(SYSCFG_CFGR1_ADC24_DMA_RMP) || defined(SYSCFG_CFGR3_ADC2_DMA_RMP)
/**
  * @brief  Set DMA request remapping bits for ADC
  * @rmtoll SYSCFG_CFGR1 ADC24_DMA_RMP  LL_SYSCFG_SetRemapDMA_ADC\n
  *         SYSCFG_CFGR3 ADC2_DMA_RMP   LL_SYSCFG_SetRemapDMA_ADC
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_ADC24_RMP_DMA2_CH12 (*)
  *         @arg @ref LL_SYSCFG_ADC24_RMP_DMA2_CH34 (*)
  *         @arg @ref LL_SYSCFG_ADC2_RMP_DMA1_CH2 (*)
  *         @arg @ref LL_SYSCFG_ADC2_RMP_DMA1_CH4 (*)
  *         @arg @ref LL_SYSCFG_ADC2_RMP_DMA2 (*)
  *         @arg @ref LL_SYSCFG_ADC2_RMP_DMA1 (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapDMA_ADC(uint32_t Remap)
{
  __IO uint32_t *reg = (__IO uint32_t *)(uint32_t)(SYSCFG_BASE + (Remap >> 24U));
  MODIFY_REG(*reg, (Remap & 0x00FF0000U) >> 8U, (Remap & 0x0000FFFFU));
}
#endif /* SYSCFG_CFGR1_ADC24_DMA_RMP || SYSCFG_CFGR3_ADC2_DMA_RMP */

/**
  * @brief  Set DMA request remapping bits for DAC
  * @rmtoll SYSCFG_CFGR1 TIM6DAC1Ch1_DMA_RMP  LL_SYSCFG_SetRemapDMA_DAC\n
  *         SYSCFG_CFGR1 DAC2Ch1_DMA_RMP      LL_SYSCFG_SetRemapDMA_DAC
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_DAC1_CH1_RMP_DMA2_CH3
  *         @arg @ref LL_SYSCFG_DAC1_CH1_RMP_DMA1_CH3
  *         @arg @ref LL_SYSCFG_DAC1_OUT2_RMP_DMA2_CH4 (*)
  *         @arg @ref LL_SYSCFG_DAC1_OUT2_RMP_DMA1_CH4 (*)
  *         @arg @ref LL_SYSCFG_DAC2_OUT1_RMP_DMA2_CH5 (*)
  *         @arg @ref LL_SYSCFG_DAC2_OUT1_RMP_DMA1_CH5 (*)
  *         @arg @ref LL_SYSCFG_DAC2_CH1_RMP_NO (*)
  *         @arg @ref LL_SYSCFG_DAC2_CH1_RMP_DMA1_CH5 (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapDMA_DAC(uint32_t Remap)
{
  MODIFY_REG(SYSCFG->CFGR1, (Remap & 0x00FF0000U) >> 8U, (Remap & 0x0000FF00U));
}

/**
  * @brief  Set DMA request remapping bits for TIM
  * @rmtoll SYSCFG_CFGR1 TIM16_DMA_RMP        LL_SYSCFG_SetRemapDMA_TIM\n
  *         SYSCFG_CFGR1 TIM17_DMA_RMP        LL_SYSCFG_SetRemapDMA_TIM\n
  *         SYSCFG_CFGR1 TIM6DAC1Ch1_DMA_RMP  LL_SYSCFG_SetRemapDMA_TIM\n
  *         SYSCFG_CFGR1 TIM7DAC1Ch2_DMA_RMP  LL_SYSCFG_SetRemapDMA_TIM\n
  *         SYSCFG_CFGR1 TIM18DAC2Ch1_DMA_RMP LL_SYSCFG_SetRemapDMA_TIM
  * @param  Remap This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_TIM16_RMP_DMA1_CH3 or @ref LL_SYSCFG_TIM16_RMP_DMA1_CH6
  *         @arg @ref LL_SYSCFG_TIM17_RMP_DMA1_CH1 or @ref LL_SYSCFG_TIM17_RMP_DMA1_CH7
  *         @arg @ref LL_SYSCFG_TIM6_RMP_DMA2_CH3 or @ref LL_SYSCFG_TIM6_RMP_DMA1_CH3
  *         @arg @ref LL_SYSCFG_TIM7_RMP_DMA2_CH4 or @ref LL_SYSCFG_TIM7_RMP_DMA1_CH4 (*)
  *         @arg @ref LL_SYSCFG_TIM18_RMP_DMA2_CH5 or @ref LL_SYSCFG_TIM18_RMP_DMA1_CH5 (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapDMA_TIM(uint32_t Remap)
{
  MODIFY_REG(SYSCFG->CFGR1, (Remap & 0x00FF0000U) >> 8U, (Remap & 0x0000FF00U));
}

#if defined(SYSCFG_CFGR1_TIM1_ITR3_RMP) || defined(SYSCFG_CFGR1_ENCODER_MODE)
/**
  * @brief  Set Timer input remap
  * @rmtoll SYSCFG_CFGR1 TIM1_ITR3_RMP  LL_SYSCFG_SetRemapInput_TIM\n
  *         SYSCFG_CFGR1 ENCODER_MODE   LL_SYSCFG_SetRemapInput_TIM
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_TIM1_ITR3_RMP_TIM4_TRGO (*)
  *         @arg @ref LL_SYSCFG_TIM1_ITR3_RMP_TIM17_OC (*)
  *         @arg @ref LL_SYSCFG_TIM15_ENCODEMODE_NOREDIRECTION (*)
  *         @arg @ref LL_SYSCFG_TIM15_ENCODEMODE_TIM2 (*)
  *         @arg @ref LL_SYSCFG_TIM15_ENCODEMODE_TIM3 (*)
  *         @arg @ref LL_SYSCFG_TIM15_ENCODEMODE_TIM4 (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapInput_TIM(uint32_t Remap)
{
   MODIFY_REG(SYSCFG->CFGR1, (Remap & 0xFF00FF00U) >> 8U, (Remap & 0x00FF00FFU));
}
#endif /* SYSCFG_CFGR1_TIM1_ITR3_RMP || SYSCFG_CFGR1_ENCODER_MODE */

#if defined(SYSCFG_CFGR4_ADC12_EXT2_RMP)
/**
  * @brief  Set ADC Trigger remap
  * @rmtoll SYSCFG_CFGR4 ADC12_EXT2_RMP    LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC12_EXT3_RMP    LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC12_EXT5_RMP    LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC12_EXT13_RMP   LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC12_EXT15_RMP   LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC12_JEXT3_RMP   LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC12_JEXT6_RMP   LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC12_JEXT13_RMP  LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC34_EXT5_RMP    LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC34_EXT6_RMP    LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC34_EXT15_RMP   LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC34_JEXT5_RMP   LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC34_JEXT11_RMP  LL_SYSCFG_SetRemapTrigger_ADC\n
  *         SYSCFG_CFGR4 ADC34_JEXT14_RMP  LL_SYSCFG_SetRemapTrigger_ADC
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_ADC12_EXT2_RMP_TIM1_CC3
  *         @arg @ref LL_SYSCFG_ADC12_EXT2_RMP_TIM20_TRGO
  *         @arg @ref LL_SYSCFG_ADC12_EXT3_RMP_TIM2_CC2
  *         @arg @ref LL_SYSCFG_ADC12_EXT3_RMP_TIM20_TRGO2
  *         @arg @ref LL_SYSCFG_ADC12_EXT5_RMP_TIM4_CC4
  *         @arg @ref LL_SYSCFG_ADC12_EXT5_RMP_TIM20_CC1
  *         @arg @ref LL_SYSCFG_ADC12_EXT13_RMP_TIM6_TRGO
  *         @arg @ref LL_SYSCFG_ADC12_EXT13_RMP_TIM20_CC2
  *         @arg @ref LL_SYSCFG_ADC12_EXT15_RMP_TIM3_CC4
  *         @arg @ref LL_SYSCFG_ADC12_EXT15_RMP_TIM20_CC3
  *         @arg @ref LL_SYSCFG_ADC12_JEXT3_RMP_TIM2_CC1
  *         @arg @ref LL_SYSCFG_ADC12_JEXT3_RMP_TIM20_TRGO
  *         @arg @ref LL_SYSCFG_ADC12_JEXT6_RMP_EXTI_LINE_15
  *         @arg @ref LL_SYSCFG_ADC12_JEXT6_RMP_TIM20_TRGO2
  *         @arg @ref LL_SYSCFG_ADC12_JEXT13_RMP_TIM3_CC1
  *         @arg @ref LL_SYSCFG_ADC12_JEXT13_RMP_TIM20_CC4
  *         @arg @ref LL_SYSCFG_ADC34_EXT5_RMP_EXTI_LINE_2
  *         @arg @ref LL_SYSCFG_ADC34_EXT5_RMP_TIM20_TRGO
  *         @arg @ref LL_SYSCFG_ADC34_EXT6_RMP_TIM4_CC1
  *         @arg @ref LL_SYSCFG_ADC34_EXT6_RMP_TIM20_TRGO2
  *         @arg @ref LL_SYSCFG_ADC34_EXT15_RMP_TIM2_CC1
  *         @arg @ref LL_SYSCFG_ADC34_EXT15_RMP_TIM20_CC1
  *         @arg @ref LL_SYSCFG_ADC34_JEXT5_RMP_TIM4_CC3
  *         @arg @ref LL_SYSCFG_ADC34_JEXT5_RMP_TIM20_TRGO
  *         @arg @ref LL_SYSCFG_ADC34_JEXT11_RMP_TIM1_CC3
  *         @arg @ref LL_SYSCFG_ADC34_JEXT11_RMP_TIM20_TRGO2
  *         @arg @ref LL_SYSCFG_ADC34_JEXT14_RMP_TIM7_TRGO
  *         @arg @ref LL_SYSCFG_ADC34_JEXT14_RMP_TIM20_CC2
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapTrigger_ADC(uint32_t Remap)
{
  MODIFY_REG(SYSCFG->CFGR4, (Remap & 0xFFFF0000U) >> 16U, (Remap & 0x0000FFFFU));
}
#endif /* SYSCFG_CFGR4_ADC12_EXT2_RMP */

#if defined(SYSCFG_CFGR1_DAC1_TRIG1_RMP) || defined(SYSCFG_CFGR3_TRIGGER_RMP)
/**
  * @brief  Set DAC Trigger remap
  * @rmtoll SYSCFG_CFGR1 DAC1_TRIG1_RMP  LL_SYSCFG_SetRemapTrigger_DAC\n
  *         SYSCFG_CFGR3 DAC1_TRG3_RMP   LL_SYSCFG_SetRemapTrigger_DAC\n
  *         SYSCFG_CFGR3 DAC1_TRG5_RMP   LL_SYSCFG_SetRemapTrigger_DAC
  * @param  Remap This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_DAC1_TRIG1_RMP_TIM8_TRGO (*)
  *         @arg @ref LL_SYSCFG_DAC1_TRIG1_RMP_TIM3_TRGO (*)
  *         @arg @ref LL_SYSCFG_DAC1_TRIG3_RMP_TIM15_TRGO (*)
  *         @arg @ref LL_SYSCFG_DAC1_TRIG3_RMP_HRTIM1_DAC1_TRIG1 (*)
  *         @arg @ref LL_SYSCFG_DAC1_TRIG5_RMP_NO (*)
  *         @arg @ref LL_SYSCFG_DAC1_TRIG5_RMP_HRTIM1_DAC1_TRIG2 (*)
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetRemapTrigger_DAC(uint32_t Remap)
{
  __IO uint32_t *reg = (__IO uint32_t *)(uint32_t)(SYSCFG_BASE + (Remap >> 24U));
  MODIFY_REG(*reg, (Remap & 0x00F00F00U) >> 4U, (Remap & 0x000F00F0U));
}
#endif /* SYSCFG_CFGR1_DAC1_TRIG1_RMP || SYSCFG_CFGR3_TRIGGER_RMP */

#if defined(SYSCFG_CFGR1_USB_IT_RMP)
/**
  * @brief  Enable USB interrupt remap
  * @note  Remap the USB interrupts (USB_HP, USB_LP and USB_WKUP) on interrupt lines 74, 75 and 76
  * respectively
  * @rmtoll SYSCFG_CFGR1 USB_IT_RMP    LL_SYSCFG_EnableRemapIT_USB
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableRemapIT_USB(void)
{
  SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_USB_IT_RMP);
}

/**
  * @brief  Disable USB interrupt remap
  * @rmtoll SYSCFG_CFGR1 USB_IT_RMP    LL_SYSCFG_DisableRemapIT_USB
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableRemapIT_USB(void)
{
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_USB_IT_RMP);
}
#endif /* SYSCFG_CFGR1_USB_IT_RMP */

#if defined(SYSCFG_CFGR1_VBAT)
/**
  * @brief  Enable VBAT monitoring (to enable the power switch to deliver VBAT voltage on ADC channel 18 input)
  * @rmtoll SYSCFG_CFGR1 VBAT          LL_SYSCFG_EnableVBATMonitoring
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableVBATMonitoring(void)
{
  SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_VBAT);
}

/**
  * @brief  Disable VBAT monitoring
  * @rmtoll SYSCFG_CFGR1 VBAT          LL_SYSCFG_DisableVBATMonitoring
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableVBATMonitoring(void)
{
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_VBAT);
}
#endif /* SYSCFG_CFGR1_VBAT */

/**
  * @brief  Enable the I2C fast mode plus driving capability.
  * @rmtoll SYSCFG_CFGR1 I2C_PB6_FMP   LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_PB7_FMP   LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_PB8_FMP   LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_PB9_FMP   LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C1_FMP      LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C2_FMP      LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR1 I2C3_FMP      LL_SYSCFG_EnableFastModePlus
  * @param  ConfigFastModePlus This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB6
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB7
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB8
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB9
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C1
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C2 (*)
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C3 (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableFastModePlus(uint32_t ConfigFastModePlus)
{
  SET_BIT(SYSCFG->CFGR1, ConfigFastModePlus);
}

/**
  * @brief  Disable the I2C fast mode plus driving capability.
  * @rmtoll SYSCFG_CFGR1 I2C_PB6_FMP   LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_PB7_FMP   LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_PB8_FMP   LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C_PB9_FMP   LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C1_FMP      LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C2_FMP      LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR1 I2C3_FMP      LL_SYSCFG_DisableFastModePlus
  * @param  ConfigFastModePlus This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB6
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB7
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB8
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB9
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C1
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C2 (*)
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C3 (*)
  *
  *         (*) value not defined in all devices.
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
  return (READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_0) == (SYSCFG_CFGR1_FPU_IE_0));
}

/**
  * @brief  Check if Floating Point Unit Divide-by-zero Interrupt source is enabled or disabled.
  * @rmtoll SYSCFG_CFGR1 FPU_IE_1      LL_SYSCFG_IsEnabledIT_FPU_DZC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIT_FPU_DZC(void)
{
  return (READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_1) == (SYSCFG_CFGR1_FPU_IE_1));
}

/**
  * @brief  Check if Floating Point Unit Underflow Interrupt source is enabled or disabled.
  * @rmtoll SYSCFG_CFGR1 FPU_IE_2      LL_SYSCFG_IsEnabledIT_FPU_UFC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIT_FPU_UFC(void)
{
  return (READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_2) == (SYSCFG_CFGR1_FPU_IE_2));
}

/**
  * @brief  Check if Floating Point Unit Overflow Interrupt source is enabled or disabled.
  * @rmtoll SYSCFG_CFGR1 FPU_IE_3      LL_SYSCFG_IsEnabledIT_FPU_OFC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIT_FPU_OFC(void)
{
  return (READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_3) == (SYSCFG_CFGR1_FPU_IE_3));
}

/**
  * @brief  Check if Floating Point Unit Input denormal Interrupt source is enabled or disabled.
  * @rmtoll SYSCFG_CFGR1 FPU_IE_4      LL_SYSCFG_IsEnabledIT_FPU_IDC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIT_FPU_IDC(void)
{
  return (READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_4) == (SYSCFG_CFGR1_FPU_IE_4));
}

/**
  * @brief  Check if Floating Point Unit Inexact Interrupt source is enabled or disabled.
  * @rmtoll SYSCFG_CFGR1 FPU_IE_5      LL_SYSCFG_IsEnabledIT_FPU_IXC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIT_FPU_IXC(void)
{
  return (READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_FPU_IE_5) == (SYSCFG_CFGR1_FPU_IE_5));
}

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
  *         @arg @ref LL_SYSCFG_EXTI_PORTF
  *         @arg @ref LL_SYSCFG_EXTI_PORTG (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTH (*)
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
  MODIFY_REG(SYSCFG->EXTICR[Line & 0xFF], (Line >> 16U), Port << POSITION_VAL((Line >> 16U)));
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
  *         @arg @ref LL_SYSCFG_EXTI_PORTF
  *         @arg @ref LL_SYSCFG_EXTI_PORTG (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTH (*)
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetEXTISource(uint32_t Line)
{
  return (uint32_t)(READ_BIT(SYSCFG->EXTICR[Line & 0xFF], (Line >> 16U)) >> POSITION_VAL(Line >> 16U));
}

/**
  * @brief  Set connections to TIMx Break inputs
  * @rmtoll SYSCFG_CFGR2 LOCKUP_LOCK       LL_SYSCFG_SetTIMBreakInputs\n
  *         SYSCFG_CFGR2 SRAM_PARITY_LOCK  LL_SYSCFG_SetTIMBreakInputs\n
  *         SYSCFG_CFGR2 PVD_LOCK          LL_SYSCFG_SetTIMBreakInputs
  * @param  Break This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_TIMBREAK_PVD (*)
  *         @arg @ref LL_SYSCFG_TIMBREAK_SRAM_PARITY (*)
  *         @arg @ref LL_SYSCFG_TIMBREAK_LOCKUP
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetTIMBreakInputs(uint32_t Break)
{
  MODIFY_REG(SYSCFG->CFGR2, SYSCFG_MASK_TIM_BREAK, Break);
}

/**
  * @brief  Get connections to TIMx Break inputs
  * @rmtoll SYSCFG_CFGR2 LOCKUP_LOCK       LL_SYSCFG_GetTIMBreakInputs\n
  *         SYSCFG_CFGR2 SRAM_PARITY_LOCK  LL_SYSCFG_GetTIMBreakInputs\n
  *         SYSCFG_CFGR2 PVD_LOCK          LL_SYSCFG_GetTIMBreakInputs
  * @retval Returned value can be can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_TIMBREAK_PVD (*)
  *         @arg @ref LL_SYSCFG_TIMBREAK_SRAM_PARITY (*)
  *         @arg @ref LL_SYSCFG_TIMBREAK_LOCKUP
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetTIMBreakInputs(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CFGR2, SYSCFG_MASK_TIM_BREAK));
}

#if defined(SYSCFG_CFGR2_BYP_ADDR_PAR)
/**
  * @brief  Disable RAM Parity Check Disable
  * @rmtoll SYSCFG_CFGR2 BYP_ADDR_PAR  LL_SYSCFG_DisableSRAMParityCheck
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableSRAMParityCheck(void)
{
  SET_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_BYP_ADDR_PAR);
}
#endif /* SYSCFG_CFGR2_BYP_ADDR_PAR */

#if defined(SYSCFG_CFGR2_SRAM_PE)
/**
  * @brief  Check if SRAM parity error detected
  * @rmtoll SYSCFG_CFGR2 SRAM_PE       LL_SYSCFG_IsActiveFlag_SP
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_SP(void)
{
  return (READ_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_SRAM_PE) == (SYSCFG_CFGR2_SRAM_PE));
}

/**
  * @brief  Clear SRAM parity error flag
  * @rmtoll SYSCFG_CFGR2 SRAM_PE       LL_SYSCFG_ClearFlag_SP
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_ClearFlag_SP(void)
{
  SET_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_SRAM_PE);
}
#endif /* SYSCFG_CFGR2_SRAM_PE */

#if defined(SYSCFG_RCR_PAGE0)
/**
  * @brief  Enable CCM SRAM page write protection
  * @note   Write protection is cleared only by a system reset
  * @rmtoll SYSCFG_RCR   PAGE0         LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE1         LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE2         LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE3         LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE4         LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE5         LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE6         LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE7         LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE8         LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE9         LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE10        LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE11        LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE12        LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE13        LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE14        LL_SYSCFG_EnableCCM_SRAMPageWRP\n
  *         SYSCFG_RCR   PAGE15        LL_SYSCFG_EnableCCM_SRAMPageWRP
  * @param  PageWRP This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE0
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE1
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE2
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE3
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE4 (*)
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE5 (*)
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE6 (*)
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE7 (*)
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE8 (*)
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE9 (*)
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE10 (*)
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE11 (*)
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE12 (*)
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE13 (*)
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE14 (*)
  *         @arg @ref LL_SYSCFG_CCMSRAMWRP_PAGE15 (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableCCM_SRAMPageWRP(uint32_t PageWRP)
{
  SET_BIT(SYSCFG->RCR, PageWRP);
}
#endif /* SYSCFG_RCR_PAGE0 */

/**
  * @}
  */

/** @defgroup SYSTEM_LL_EF_DBGMCU DBGMCU
  * @{
  */

/**
  * @brief  Return the device identifier
  * @note For STM32F303xC, STM32F358xx and STM32F302xC devices, the device ID is 0x422
  * @note For STM32F373xx and STM32F378xx devices, the device ID is 0x432
  * @note For STM32F303x8, STM32F334xx and STM32F328xx devices, the device ID is 0x438.
  * @note For STM32F302x8, STM32F301x8 and STM32F318xx devices, the device ID is 0x439
  * @note For STM32F303xE, STM32F398xx and STM32F302xE devices, the device ID is 0x446
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
  *         APB1_FZ      DBG_TIM12_STOP          LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_TIM13_STOP          LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_TIM14_STOP          LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_TIM18_STOP          LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_RTC_STOP            LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_WWDG_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_IWDG_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_I2C1_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_I2C2_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_I2C3_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         APB1_FZ      DBG_CAN_STOP  LL_DBGMCU_APB1_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM3_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM4_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM5_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM6_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM7_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM12_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM13_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM14_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM18_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C2_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C3_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_CAN_STOP (*)
  *
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
  *         APB1_FZ      DBG_TIM12_STOP          LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_TIM13_STOP          LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_TIM14_STOP          LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_TIM18_STOP          LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_RTC_STOP            LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_WWDG_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_IWDG_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_I2C1_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_I2C2_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_I2C3_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         APB1_FZ      DBG_CAN_STOP  LL_DBGMCU_APB1_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM3_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM4_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM5_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM6_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM7_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM12_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM13_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM14_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM18_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C2_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C3_STOP (*)
  *         @arg @ref LL_DBGMCU_APB1_GRP1_CAN_STOP (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB1FZ, Periphs);
}

/**
  * @brief  Freeze APB2 peripherals
  * @rmtoll APB2_FZ      DBG_TIM1_STOP    LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         APB2_FZ      DBG_TIM8_STOP    LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         APB2_FZ      DBG_TIM15_STOP   LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         APB2_FZ      DBG_TIM16_STOP   LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         APB2_FZ      DBG_TIM17_STOP   LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         APB2_FZ      DBG_TIM19_STOP   LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         APB2_FZ      DBG_TIM20_STOP   LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         APB2_FZ      DBG_HRTIM1_STOP  LL_DBGMCU_APB2_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM1_STOP (*)
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM8_STOP (*)
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM15_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM16_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM17_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM19_STOP (*)
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM20_STOP (*)
  *         @arg @ref LL_DBGMCU_APB2_GRP1_HRTIM1_STOP (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB2_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB2FZ, Periphs);
}

/**
  * @brief  Unfreeze APB2 peripherals
  * @rmtoll APB2_FZ      DBG_TIM1_STOP    LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         APB2_FZ      DBG_TIM8_STOP    LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         APB2_FZ      DBG_TIM15_STOP   LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         APB2_FZ      DBG_TIM16_STOP   LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         APB2_FZ      DBG_TIM17_STOP   LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         APB2_FZ      DBG_TIM19_STOP   LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         APB2_FZ      DBG_TIM20_STOP   LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         APB2_FZ      DBG_HRTIM1_STOP  LL_DBGMCU_APB2_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM1_STOP (*)
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM8_STOP (*)
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM15_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM16_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM17_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM19_STOP (*)
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM20_STOP (*)
  *         @arg @ref LL_DBGMCU_APB2_GRP1_HRTIM1_STOP (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB2_GRP1_UnFreezePeriph(uint32_t Periphs)
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
  *         @arg @ref LL_FLASH_LATENCY_2
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
  */
__STATIC_INLINE uint32_t LL_FLASH_GetLatency(void)
{
  return (uint32_t)(READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY));
}

/**
  * @brief  Enable Prefetch
  * @rmtoll FLASH_ACR    PRFTBE         LL_FLASH_EnablePrefetch
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnablePrefetch(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_PRFTBE );
}

/**
  * @brief  Disable Prefetch
  * @rmtoll FLASH_ACR    PRFTBE         LL_FLASH_DisablePrefetch
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisablePrefetch(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_PRFTBE );
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

#if defined(FLASH_ACR_HLFCYA)
/**
  * @brief  Enable Flash Half Cycle Access
  * @rmtoll FLASH_ACR    HLFCYA        LL_FLASH_EnableHalfCycleAccess
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnableHalfCycleAccess(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_HLFCYA);
}

/**
  * @brief  Disable Flash Half Cycle Access
  * @rmtoll FLASH_ACR    HLFCYA        LL_FLASH_DisableHalfCycleAccess
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisableHalfCycleAccess(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_HLFCYA);
}

/**
  * @brief  Check if  Flash Half Cycle Access is enabled or not
  * @rmtoll FLASH_ACR    HLFCYA        LL_FLASH_IsHalfCycleAccessEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_FLASH_IsHalfCycleAccessEnabled(void)
{
  return (READ_BIT(FLASH->ACR, FLASH_ACR_HLFCYA) == (FLASH_ACR_HLFCYA));
}
#endif /* FLASH_ACR_HLFCYA */



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

#endif /* __STM32F3xx_LL_SYSTEM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
