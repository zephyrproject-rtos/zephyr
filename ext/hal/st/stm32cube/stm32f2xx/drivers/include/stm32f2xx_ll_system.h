/**
  ******************************************************************************
  * @file    stm32f2xx_ll_system.h
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
#ifndef __STM32F2xx_LL_SYSTEM_H
#define __STM32F2xx_LL_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx.h"

/** @addtogroup STM32F2xx_LL_Driver
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

/** @defgroup SYSTEM_LL_EC_REMAP SYSCFG REMAP
  * @{
  */
#define LL_SYSCFG_REMAP_FLASH              (uint32_t)0x00000000                                  /*!< Main Flash memory mapped at 0x00000000              */
#define LL_SYSCFG_REMAP_SYSTEMFLASH        SYSCFG_MEMRMP_MEM_MODE_0                              /*!< System Flash memory mapped at 0x00000000            */
#define LL_SYSCFG_REMAP_FSMC               SYSCFG_MEMRMP_MEM_MODE_1                              /*!< FSMC(NOR/PSRAM 1 and 2) mapped at 0x00000000                          */
#define LL_SYSCFG_REMAP_SRAM               (SYSCFG_MEMRMP_MEM_MODE_1 | SYSCFG_MEMRMP_MEM_MODE_0) /*!< SRAM1 mapped at 0x00000000                          */
/**
  * @}
  */

#if defined(SYSCFG_PMC_MII_RMII_SEL)
/** @defgroup SYSTEM_LL_EC_PMC SYSCFG PMC
  * @{
  */
#define LL_SYSCFG_PMC_ETHMII               (uint32_t)0x00000000                                /*!< ETH Media MII interface */
#define LL_SYSCFG_PMC_ETHRMII              (uint32_t)SYSCFG_PMC_MII_RMII_SEL                   /*!< ETH Media RMII interface */

/**
  * @}
  */
#endif /* SYSCFG_PMC_MII_RMII_SEL */



/** @defgroup SYSTEM_LL_EC_I2C_FASTMODEPLUS SYSCFG I2C FASTMODEPLUS
  * @{
  */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_EXTI_PORT SYSCFG EXTI PORT
  * @{
  */
#define LL_SYSCFG_EXTI_PORTA               (uint32_t)0               /*!< EXTI PORT A                        */
#define LL_SYSCFG_EXTI_PORTB               (uint32_t)1               /*!< EXTI PORT B                        */
#define LL_SYSCFG_EXTI_PORTC               (uint32_t)2               /*!< EXTI PORT C                        */
#define LL_SYSCFG_EXTI_PORTD               (uint32_t)3               /*!< EXTI PORT D                        */
#define LL_SYSCFG_EXTI_PORTE               (uint32_t)4               /*!< EXTI PORT E                        */
#if defined(GPIOF)
#define LL_SYSCFG_EXTI_PORTF               (uint32_t)5               /*!< EXTI PORT F                        */
#endif /* GPIOF */
#if defined(GPIOG)
#define LL_SYSCFG_EXTI_PORTG               (uint32_t)6               /*!< EXTI PORT G                        */
#endif /* GPIOG */
#define LL_SYSCFG_EXTI_PORTH               (uint32_t)7               /*!< EXTI PORT H                        */
#if defined(GPIOI)
#define LL_SYSCFG_EXTI_PORTI               (uint32_t)8               /*!< EXTI PORT I                        */
#endif /* GPIOI */
#if defined(GPIOJ)
#define LL_SYSCFG_EXTI_PORTJ               (uint32_t)9               /*!< EXTI PORT J                        */
#endif /* GPIOJ */
#if defined(GPIOK)
#define LL_SYSCFG_EXTI_PORTK               (uint32_t)10              /*!< EXTI PORT k                        */
#endif /* GPIOK */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_EXTI_LINE SYSCFG EXTI LINE
  * @{
  */
#define LL_SYSCFG_EXTI_LINE0               (uint32_t)(0x000FU << 16 | 0)  /*!< EXTI_POSITION_0  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE1               (uint32_t)(0x00F0U << 16 | 0)  /*!< EXTI_POSITION_4  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE2               (uint32_t)(0x0F00U << 16 | 0)  /*!< EXTI_POSITION_8  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE3               (uint32_t)(0xF000U << 16 | 0)  /*!< EXTI_POSITION_12 | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE4               (uint32_t)(0x000FU << 16 | 1)  /*!< EXTI_POSITION_0  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE5               (uint32_t)(0x00F0U << 16 | 1)  /*!< EXTI_POSITION_4  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE6               (uint32_t)(0x0F00U << 16 | 1)  /*!< EXTI_POSITION_8  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE7               (uint32_t)(0xF000U << 16 | 1)  /*!< EXTI_POSITION_12 | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE8               (uint32_t)(0x000FU << 16 | 2)  /*!< EXTI_POSITION_0  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE9               (uint32_t)(0x00F0U << 16 | 2)  /*!< EXTI_POSITION_4  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE10              (uint32_t)(0x0F00U << 16 | 2)  /*!< EXTI_POSITION_8  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE11              (uint32_t)(0xF000U << 16 | 2)  /*!< EXTI_POSITION_12 | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE12              (uint32_t)(0x000FU << 16 | 3)  /*!< EXTI_POSITION_0  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE13              (uint32_t)(0x00F0U << 16 | 3)  /*!< EXTI_POSITION_4  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE14              (uint32_t)(0x0F00U << 16 | 3)  /*!< EXTI_POSITION_8  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE15              (uint32_t)(0xF000U << 16 | 3)  /*!< EXTI_POSITION_12 | EXTICR[3] */
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
#define LL_DBGMCU_APB1_GRP1_TIM2_STOP      DBGMCU_APB1_FZ_DBG_TIM2_STOP          /*!< TIM2 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM3_STOP      DBGMCU_APB1_FZ_DBG_TIM3_STOP          /*!< TIM3 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM4_STOP      DBGMCU_APB1_FZ_DBG_TIM4_STOP          /*!< TIM4 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM5_STOP      DBGMCU_APB1_FZ_DBG_TIM5_STOP          /*!< TIM5 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM6_STOP      DBGMCU_APB1_FZ_DBG_TIM6_STOP          /*!< TIM6 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM7_STOP      DBGMCU_APB1_FZ_DBG_TIM7_STOP          /*!< TIM7 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM12_STOP     DBGMCU_APB1_FZ_DBG_TIM12_STOP         /*!< TIM12 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM13_STOP     DBGMCU_APB1_FZ_DBG_TIM13_STOP         /*!< TIM13 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM14_STOP     DBGMCU_APB1_FZ_DBG_TIM14_STOP         /*!< TIM14 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_RTC_STOP       DBGMCU_APB1_FZ_DBG_RTC_STOP           /*!< RTC counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_WWDG_STOP      DBGMCU_APB1_FZ_DBG_WWDG_STOP          /*!< Debug Window Watchdog stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_IWDG_STOP      DBGMCU_APB1_FZ_DBG_IWDG_STOP          /*!< Debug Independent Watchdog stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C1_STOP      DBGMCU_APB1_FZ_DBG_I2C1_SMBUS_TIMEOUT /*!< I2C1 SMBUS timeout mode stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C2_STOP      DBGMCU_APB1_FZ_DBG_I2C2_SMBUS_TIMEOUT /*!< I2C2 SMBUS timeout mode stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C3_STOP      DBGMCU_APB1_FZ_DBG_I2C3_SMBUS_TIMEOUT /*!< I2C3 SMBUS timeout mode stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_CAN1_STOP      DBGMCU_APB1_FZ_DBG_CAN1_STOP          /*!< CAN1 debug stopped when Core is halted  */
#define LL_DBGMCU_APB1_GRP1_CAN2_STOP      DBGMCU_APB1_FZ_DBG_CAN2_STOP          /*!< CAN2 debug stopped when Core is halted  */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB2_GRP1_STOP_IP DBGMCU APB2 GRP1 STOP IP
  * @{
  */
#define LL_DBGMCU_APB2_GRP1_TIM1_STOP      DBGMCU_APB2_FZ_DBG_TIM1_STOP   /*!< TIM1 counter stopped when core is halted */
#define LL_DBGMCU_APB2_GRP1_TIM8_STOP      DBGMCU_APB2_FZ_DBG_TIM8_STOP   /*!< TIM8 counter stopped when core is halted */
#define LL_DBGMCU_APB2_GRP1_TIM9_STOP      DBGMCU_APB2_FZ_DBG_TIM9_STOP   /*!< TIM9 counter stopped when core is halted */
#define LL_DBGMCU_APB2_GRP1_TIM10_STOP     DBGMCU_APB2_FZ_DBG_TIM10_STOP   /*!< TIM10 counter stopped when core is halted */
#define LL_DBGMCU_APB2_GRP1_TIM11_STOP     DBGMCU_APB2_FZ_DBG_TIM11_STOP   /*!< TIM11 counter stopped when core is halted */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_LATENCY FLASH LATENCY
  * @{
  */
#define LL_FLASH_LATENCY_0                 FLASH_ACR_LATENCY_0WS   /*!< FLASH Zero wait state */
#define LL_FLASH_LATENCY_1                 FLASH_ACR_LATENCY_1WS   /*!< FLASH One wait state */
#define LL_FLASH_LATENCY_2                 FLASH_ACR_LATENCY_2WS   /*!< FLASH Two wait states */
#define LL_FLASH_LATENCY_3                 FLASH_ACR_LATENCY_3WS   /*!< FLASH Three wait states */
#define LL_FLASH_LATENCY_4                 FLASH_ACR_LATENCY_4WS   /*!< FLASH Four wait states */
#define LL_FLASH_LATENCY_5                 FLASH_ACR_LATENCY_5WS   /*!< FLASH five wait state */
#define LL_FLASH_LATENCY_6                 FLASH_ACR_LATENCY_6WS   /*!< FLASH six wait state */
#define LL_FLASH_LATENCY_7                 FLASH_ACR_LATENCY_7WS   /*!< FLASH seven wait states */
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
  *         @arg @ref LL_SYSCFG_REMAP_FSMC
  *         @arg @ref LL_SYSCFG_REMAP_SRAM
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
  *         @arg @ref LL_SYSCFG_REMAP_FSMC
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetRemapMemory(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_MEM_MODE));
}

/**
  * @brief  Enables the Compensation cell Power Down
  * @rmtoll SYSCFG_CMPCR CMP_PD      LL_SYSCFG_EnableCompensationCell
  * @note   The I/O compensation cell can be used only when the device supply
  *         voltage ranges from 2.4 to 3.6 V
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableCompensationCell(void)
{
  SET_BIT(SYSCFG->CMPCR, SYSCFG_CMPCR_CMP_PD);
}

/**
  * @brief  Disables the Compensation cell Power Down
  * @rmtoll SYSCFG_CMPCR CMP_PD      LL_SYSCFG_DisableCompensationCell
  * @note   The I/O compensation cell can be used only when the device supply
  *         voltage ranges from 2.4 to 3.6 V
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableCompensationCell(void)
{
  CLEAR_BIT(SYSCFG->CMPCR, SYSCFG_CMPCR_CMP_PD);
}

/**
  * @brief  Get Compensation Cell ready Flag
  * @rmtoll SYSCFG_CMPCR READY  LL_SYSCFG_IsActiveFlag_CMPCR
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsActiveFlag_CMPCR(void)
{
  return (READ_BIT(SYSCFG->CMPCR, SYSCFG_CMPCR_READY) == (SYSCFG_CMPCR_READY));
}
#if defined(SYSCFG_PMC_MII_RMII_SEL)
/**
  * @brief  Select Ethernet PHY interface 
  * @rmtoll SYSCFG_PMC MII_RMII_SEL       LL_SYSCFG_SetPHYInterface
  * @param  Interface This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_PMC_ETHMII
  *         @arg @ref LL_SYSCFG_PMC_ETHRMII
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetPHYInterface(uint32_t Interface)
{
  MODIFY_REG(SYSCFG->PMC, SYSCFG_PMC_MII_RMII_SEL, Interface);
}

/**
  * @brief  Get Ethernet PHY interface 
  * @rmtoll SYSCFG_PMC MII_RMII_SEL       LL_SYSCFG_GetPHYInterface
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_SYSCFG_PMC_ETHMII
  *         @arg @ref LL_SYSCFG_PMC_ETHRMII
  * @retval None
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetPHYInterface(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->PMC, SYSCFG_PMC_MII_RMII_SEL));
}
#endif /* SYSCFG_PMC_MII_RMII_SEL */



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
  *         @arg @ref LL_SYSCFG_EXTI_PORTF (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTG (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTH
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
  MODIFY_REG(SYSCFG->EXTICR[Line & 0xFF], (Line >> 16), Port << POSITION_VAL((Line >> 16)));
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
  *         @arg @ref LL_SYSCFG_EXTI_PORTF (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTG (*)
  *         @arg @ref LL_SYSCFG_EXTI_PORTH
  *         (*) value not defined in all devices
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
  * @note For STM32F2xxxx ,the device ID is 0x411
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
          For example, it is read as revA -> 0x1000,revZ -> 0x1001, revB -> 0x2000, revY -> 0x2001, revX -> 0x2003, rev1 -> 0x2007, revV -> 0x200F, rev2 -> 0x201F
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
  * @rmtoll DBGMCU_APB1_FZ      DBG_TIM2_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM3_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM4_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM5_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM6_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM7_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM12_STOP          LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM13_STOP          LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM14_STOP          LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_RTC_STOP            LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_WWDG_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_IWDG_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_I2C1_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_I2C2_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_I2C3_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_CAN1_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_CAN2_STOP           LL_DBGMCU_APB1_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM4_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM5_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM6_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM7_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM12_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM13_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM14_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_CAN1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_CAN2_STOP
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
  * @rmtoll DBGMCU_APB1_FZ      DBG_TIM2_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM3_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM4_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM5_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM6_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM7_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM12_STOP          LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM13_STOP          LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_TIM14_STOP          LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_RTC_STOP            LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_WWDG_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_IWDG_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_I2C1_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_I2C2_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_I2C3_SMBUS_TIMEOUT  LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_CAN1_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1_FZ      DBG_CAN2_STOP           LL_DBGMCU_APB1_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM4_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM5_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM6_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM7_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM12_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM13_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM14_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_CAN1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_CAN2_STOP
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
  * @rmtoll DBGMCU_APB2_FZ      DBG_TIM1_STOP    LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         DBGMCU_APB2_FZ      DBG_TIM8_STOP    LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         DBGMCU_APB2_FZ      DBG_TIM9_STOP    LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         DBGMCU_APB2_FZ      DBG_TIM10_STOP   LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         DBGMCU_APB2_FZ      DBG_TIM11_STOP   LL_DBGMCU_APB2_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM1_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM8_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM9_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM10_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM11_STOP
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
  * @rmtoll DBGMCU_APB2_FZ      DBG_TIM1_STOP    LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB2_FZ      DBG_TIM8_STOP    LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB2_FZ      DBG_TIM9_STOP    LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB2_FZ      DBG_TIM10_STOP   LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB2_FZ      DBG_TIM11_STOP   LL_DBGMCU_APB2_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM1_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM8_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM9_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM10_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM11_STOP
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
  *         @arg @ref LL_FLASH_LATENCY_3
  *         @arg @ref LL_FLASH_LATENCY_4
  *         @arg @ref LL_FLASH_LATENCY_5
  *         @arg @ref LL_FLASH_LATENCY_6
  *         @arg @ref LL_FLASH_LATENCY_7
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
  *         @arg @ref LL_FLASH_LATENCY_4
  *         @arg @ref LL_FLASH_LATENCY_5
  *         @arg @ref LL_FLASH_LATENCY_6
  *         @arg @ref LL_FLASH_LATENCY_7
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
  * @brief  Enable Instruction cache
  * @rmtoll FLASH_ACR    ICEN          LL_FLASH_EnableInstCache
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnableInstCache(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_ICEN);
}

/**
  * @brief  Disable Instruction cache
  * @rmtoll FLASH_ACR    ICEN          LL_FLASH_DisableInstCache
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
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnableInstCacheReset(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_ICRST);
}

/**
  * @brief  Disable Instruction cache reset
  * @rmtoll FLASH_ACR    ICRST         LL_FLASH_DisableInstCacheReset
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

#endif /* __STM32F2xx_LL_SYSTEM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
