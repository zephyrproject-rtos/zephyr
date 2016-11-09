/**
  ******************************************************************************
  * @file    stm32l0xx_ll_system.h
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
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
#ifndef __STM32L0xx_LL_SYSTEM_H
#define __STM32L0xx_LL_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx.h"

/** @addtogroup STM32L0xx_LL_Driver
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

/* Defines used for position in the register */
#define DBGMCU_REVID_POSITION         (uint32_t)16U

/**
 * @brief Power-down in Run mode Flash key
 */
#define FLASH_PDKEY1                  ((uint32_t)0x04152637U) /*!< Flash power down key1 */
#define FLASH_PDKEY2                  ((uint32_t)0xFAFBFCFDU) /*!< Flash power down key2: used with FLASH_PDKEY1 
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

/** @defgroup SYSTEM_LL_EC_REMAP SYSCFG Memory Remap
* @{
*/
#define LL_SYSCFG_REMAP_FLASH              (uint32_t)0x00000000U                                 /*!< Main Flash memory mapped at 0x00000000              */
#define LL_SYSCFG_REMAP_SYSTEMFLASH        SYSCFG_CFGR1_MEM_MODE_0                               /*!< System Flash memory mapped at 0x00000000            */
#define LL_SYSCFG_REMAP_SRAM               (SYSCFG_CFGR1_MEM_MODE_1 | SYSCFG_CFGR1_MEM_MODE_0)   /*!< SRAM mapped at 0x00000000                           */

/**
  * @}
  */

#if defined(SYSCFG_CFGR1_UFB)
/** @defgroup SYSTEM_LL_EC_BANKMODE SYSCFG Bank Mode
  * @{
  */
#define LL_SYSCFG_BANKMODE_BANK1           (uint32_t)0x00000000U     /*!< Flash Bank1 mapped at 0x08000000 (and aliased at 0x00000000),
                                                                          Flash Bank2 mapped at 0x08018000 (and aliased at 0x00018000),
                                                                          Data EEPROM Bank1 mapped at 0x08080000 (and aliased at 0x00080000),
                                                                          Data EEPROM Bank2 mapped at 0x08080C00 (and aliased at 0x00080C00) */
#define LL_SYSCFG_BANKMODE_BANK2           SYSCFG_CFGR1_UFB          /*!< Flash Bank2 mapped at 0x08000000 (and aliased at 0x00000000),
                                                                          Flash Bank1 mapped at 0x08018000 (and aliased at 0x00018000),
                                                                          Data EEPROM Bank2 mapped at 0x08080000 (and aliased at 0x00080000),
                                                                          Data EEPROM Bank1 mapped at 0x08080C00 (and aliased at 0x00080C00) */
/**
  * @}
  */

#endif /* SYSCFG_CFGR1_UFB */

/** @defgroup SYSTEM_LL_EC_BOOTMODE SYSCFG Boot Mode
* @{
*/
#define LL_SYSCFG_BOOTMODE_FLASH           (uint32_t)0x00000000U                                 /*!< Main Flash memory boot mode              */
#define LL_SYSCFG_BOOTMODE_SYSTEMFLASH     SYSCFG_CFGR1_BOOT_MODE_0                              /*!< System Flash memory boot mode            */
#define LL_SYSCFG_BOOTMODE_SRAM            (SYSCFG_CFGR1_BOOT_MODE_1 | SYSCFG_CFGR1_BOOT_MODE_0) /*!< SRAM boot mode                           */

/**
  * @}
  */

#if defined(SYSCFG_CFGR2_CAPA)
/** @defgroup SYSTEM_LL_EC_CFGR2 SYSCFG VLCD Rail Connection
  * @{
  */

#define LL_SYSCFG_CAPA_VLCD2_PB2           SYSCFG_CFGR2_CAPA_0       /*!< Connect PB2  pin to LCD_VLCD2 rails supply voltage  */
#define LL_SYSCFG_CAPA_VLCD1_PB12          SYSCFG_CFGR2_CAPA_1       /*!< Connect PB12 pin to LCD_VLCD1 rails supply voltage  */
#define LL_SYSCFG_CAPA_VLCD3_PB0           SYSCFG_CFGR2_CAPA_2       /*!< Connect PB0  pin to LCD_VLCD3 rails supply voltage  */
#if defined (SYSCFG_CFGR2_CAPA_3)
#define LL_SYSCFG_CAPA_VLCD1_PE11          SYSCFG_CFGR2_CAPA_3       /*!< Connect PE11 pin to LCD_VLCD1 rails supply voltage  */
#endif /* SYSCFG_CFGR2_CAPA_3 */
#if defined (SYSCFG_CFGR2_CAPA_4)
#define LL_SYSCFG_CAPA_VLCD3_PE12          SYSCFG_CFGR2_CAPA_4       /*!< Connect PE12 pin to LCD_VLCD3 rails supply voltage  */
#endif /* SYSCFG_CFGR2_CAPA_4 */
/**
  * @}
  */
#endif /* SYSCFG_CFGR2_CAPA */

/** @defgroup SYSTEM_LL_EC_I2C_FASTMODEPLUS SYSCFG I2C FASTMODEPLUS
  * @{
  */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB6     SYSCFG_CFGR2_I2C_PB6_FMP  /*!< Enable Fast Mode Plus on PB6       */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB7     SYSCFG_CFGR2_I2C_PB7_FMP  /*!< Enable Fast Mode Plus on PB7       */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB8     SYSCFG_CFGR2_I2C_PB8_FMP  /*!< Enable Fast Mode Plus on PB8       */
#define LL_SYSCFG_I2C_FASTMODEPLUS_PB9     SYSCFG_CFGR2_I2C_PB9_FMP  /*!< Enable Fast Mode Plus on PB9       */
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C1    SYSCFG_CFGR2_I2C1_FMP     /*!< Enable Fast Mode Plus on I2C1 pins */
#if defined(SYSCFG_CFGR2_I2C2_FMP)
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C2    SYSCFG_CFGR2_I2C2_FMP     /*!< Enable Fast Mode Plus on I2C2 pins */
#endif /* SYSCFG_CFGR2_I2C2_FMP */
#if defined(SYSCFG_CFGR2_I2C3_FMP)
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C3    SYSCFG_CFGR2_I2C3_FMP     /*!< Enable Fast Mode Plus on I2C3 pins */
#endif /* SYSCFG_CFGR2_I2C3_FMP */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_VREFINT_CONTROL SYSCFG VREFINT Control
  * @{
  */
#define LL_SYSCFG_VREFINT_CONNECT_NONE        (uint32_t)0x00000000U             /*!< No pad connected to VREFINT_ADC */
#define LL_SYSCFG_VREFINT_CONNECT_IO1         SYSCFG_CFGR3_VREF_OUT_0           /*!< PB0 connected to VREFINT_ADC */
#define LL_SYSCFG_VREFINT_CONNECT_IO2         SYSCFG_CFGR3_VREF_OUT_1           /*!< PB1 connected to VREFINT_ADC */
#define LL_SYSCFG_VREFINT_CONNECT_IO1_IO2     (SYSCFG_CFGR3_VREF_OUT_0 | SYSCFG_CFGR3_VREF_OUT_1)   /*!< PB0 and PB1 connected to VREFINT_ADC */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_EXTI_PORT SYSCFG EXTI Port
  * @{
  */
#define LL_SYSCFG_EXTI_PORTA               (uint32_t)0U              /*!< EXTI PORT A */
#define LL_SYSCFG_EXTI_PORTB               (uint32_t)1U              /*!< EXTI PORT B */
#define LL_SYSCFG_EXTI_PORTC               (uint32_t)2U              /*!< EXTI PORT C */
#if defined(GPIOD_BASE)
#define LL_SYSCFG_EXTI_PORTD               (uint32_t)3U              /*!< EXTI PORT D */
#endif /*GPIOD_BASE*/
#if defined(GPIOE_BASE)
#define LL_SYSCFG_EXTI_PORTE               (uint32_t)4U              /*!< EXTI PORT E */
#endif /*GPIOE_BASE*/
#if defined(GPIOH_BASE)
#define LL_SYSCFG_EXTI_PORTH               (uint32_t)5U              /*!< EXTI PORT H */
#endif /*GPIOH_BASE*/
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_EXTI_LINE SYSCFG EXTI Line
  * @{
  */
#define LL_SYSCFG_EXTI_LINE0               (uint32_t)(0U  << 16U | 0U)  /*!< EXTI_POSITION_0  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE1               (uint32_t)(4U  << 16U | 0U)  /*!< EXTI_POSITION_4  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE2               (uint32_t)(8U  << 16U | 0U)  /*!< EXTI_POSITION_8  | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE3               (uint32_t)(12U << 16U | 0U)  /*!< EXTI_POSITION_12 | EXTICR[0] */
#define LL_SYSCFG_EXTI_LINE4               (uint32_t)(0U  << 16U | 1U)  /*!< EXTI_POSITION_0  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE5               (uint32_t)(4U  << 16U | 1U)  /*!< EXTI_POSITION_4  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE6               (uint32_t)(8U  << 16U | 1U)  /*!< EXTI_POSITION_8  | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE7               (uint32_t)(12U << 16U | 1U)  /*!< EXTI_POSITION_12 | EXTICR[1] */
#define LL_SYSCFG_EXTI_LINE8               (uint32_t)(0U  << 16U | 2U)  /*!< EXTI_POSITION_0  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE9               (uint32_t)(4U  << 16U | 2U)  /*!< EXTI_POSITION_4  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE10              (uint32_t)(8U  << 16U | 2U)  /*!< EXTI_POSITION_8  | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE11              (uint32_t)(12U << 16U | 2U)  /*!< EXTI_POSITION_12 | EXTICR[2] */
#define LL_SYSCFG_EXTI_LINE12              (uint32_t)(0U  << 16U | 3U)  /*!< EXTI_POSITION_0  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE13              (uint32_t)(4U  << 16U | 3U)  /*!< EXTI_POSITION_4  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE14              (uint32_t)(8U  << 16U | 3U)  /*!< EXTI_POSITION_8  | EXTICR[3] */
#define LL_SYSCFG_EXTI_LINE15              (uint32_t)(12U << 16U | 3U)  /*!< EXTI_POSITION_12 | EXTICR[3] */
/**
  * @}
  */



/** @defgroup SYSTEM_LL_EC_ABP1_GRP1_STOP_IP DBGMCU ABP1 GRP1 STOP IP
  * @{
  */
#define LL_DBGMCU_ABP1_GRP1_TIM2_STOP      DBGMCU_APB1_FZ_DBG_TIM2_STOP    /*!< TIM2 counter stopped when core is halted */
#if defined(TIM3)
#define LL_DBGMCU_ABP1_GRP1_TIM3_STOP      DBGMCU_APB1_FZ_DBG_TIM3_STOP    /*!< TIM3 counter stopped when core is halted */
#endif /*TIM3*/
#if defined(TIM6)
#define LL_DBGMCU_ABP1_GRP1_TIM6_STOP      DBGMCU_APB1_FZ_DBG_TIM6_STOP    /*!< TIM6 counter stopped when core is halted */
#endif /*TIM6*/
#if defined(TIM7)
#define LL_DBGMCU_ABP1_GRP1_TIM7_STOP      DBGMCU_APB1_FZ_DBG_TIM7_STOP    /*!< TIM7 counter stopped when core is halted */
#endif /*TIM7*/
#define LL_DBGMCU_ABP1_GRP1_RTC_STOP       DBGMCU_APB1_FZ_DBG_RTC_STOP     /*!< RTC Calendar frozen when core is halted */
#define LL_DBGMCU_ABP1_GRP1_WWDG_STOP      DBGMCU_APB1_FZ_DBG_WWDG_STOP    /*!< Debug Window Watchdog stopped when Core is halted */
#define LL_DBGMCU_ABP1_GRP1_IWDG_STOP      DBGMCU_APB1_FZ_DBG_IWDG_STOP    /*!< Debug Independent Watchdog stopped when Core is halted */
#define LL_DBGMCU_ABP1_GRP1_I2C1_STOP      DBGMCU_APB1_FZ_DBG_I2C1_STOP    /*!< I2C1 SMBUS timeout mode stopped when Core is halted */
#if defined(I2C2)
#define LL_DBGMCU_ABP1_GRP1_I2C2_STOP      DBGMCU_APB1_FZ_DBG_I2C2_STOP    /*!< I2C2 SMBUS timeout mode stopped when Core is halted */
#endif /*I2C2*/
#if defined(I2C3)
#define LL_DBGMCU_ABP1_GRP1_I2C3_STOP      DBGMCU_APB1_FZ_DBG_I2C3_STOP    /*!< I2C3 SMBUS timeout mode stopped when Core is halted */
#endif /*I2C3*/
#define LL_DBGMCU_ABP1_GRP1_LPTIM1_STOP    DBGMCU_APB1_FZ_DBG_LPTIMER_STOP /*!< LPTIM1 counter stopped when core is halted */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_ABP2_GRP1_STOP_IP DBGMCU ABP2 GRP1 STOP IP
  * @{
  */
#if defined(TIM22)
#define LL_DBGMCU_ABP2_GRP1_TIM22_STOP     DBGMCU_APB2_FZ_DBG_TIM22_STOP /*!< TIM22 counter stopped when core is halted */
#endif /*TIM22*/
#define LL_DBGMCU_ABP2_GRP1_TIM21_STOP     DBGMCU_APB2_FZ_DBG_TIM21_STOP /*!< TIM21 counter stopped when core is halted */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_LATENCY FLASH LATENCY
  * @{
  */
#define LL_FLASH_LATENCY_0                 ((uint32_t)0x00000000U) /*!< FLASH Zero Latency cycle */
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

#if defined(SYSCFG_CFGR1_UFB)
/**
  * @brief  Select Flash bank mode (Bank flashed at 0x08000000)
  * @rmtoll SYSCFG_CFGR1 UFB           LL_SYSCFG_SetFlashBankMode
  * @param  Bank This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_BANKMODE_BANK1
  *         @arg @ref LL_SYSCFG_BANKMODE_BANK2
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetFlashBankMode(uint32_t Bank)
{
  MODIFY_REG(SYSCFG->CFGR1, SYSCFG_CFGR1_UFB, Bank);
}

/**
  * @brief  Get Flash bank mode (Bank flashed at 0x08000000)
  * @rmtoll SYSCFG_CFGR1 UFB           LL_SYSCFG_GetFlashBankMode
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_SYSCFG_BANKMODE_BANK1
  *         @arg @ref LL_SYSCFG_BANKMODE_BANK2
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetFlashBankMode(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_UFB));
}
#endif /* SYSCFG_CFGR1_UFB */

/**
  * @brief  Get Boot mode selected by the boot pins status bits
  * @note   It indicates the boot mode selected by the boot pins. Bit 9
  *         corresponds to the complement of nBOOT1 bit in the FLASH_OPTR register.
  *         Its value is defined in the option bytes. Bit 8 corresponds to the
  *         value sampled on the BOOT0 pin.
  * @rmtoll SYSCFG_CFGR1 BOOT_MODE      LL_SYSCFG_GetBootMode
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_SYSCFG_BOOTMODE_FLASH
  *         @arg @ref LL_SYSCFG_BOOTMODE_SYSTEMFLASH
  *         @arg @ref LL_SYSCFG_BOOTMODE_SRAM
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetBootMode(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_BOOT_MODE));
}

/**
  * @brief  Firewall protection enabled
  * @rmtoll SYSCFG_CFGR2 FWDIS         LL_SYSCFG_EnableFirewall
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableFirewall(void)
{
  CLEAR_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_FWDISEN);
}

/**
  * @brief  Check if Firewall protection is enabled or not
  * @rmtoll SYSCFG_CFGR2 FWDIS         LL_SYSCFG_IsEnabledFirewall
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledFirewall(void)
{
  return !(READ_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_FWDISEN) == SYSCFG_CFGR2_FWDISEN);
}

#if defined(SYSCFG_CFGR2_CAPA)
/**
  * @brief  Set VLCD rail connection to optional external capacitor
  * @note   One to three external capacitors can be connected to pads to do
  *         VLCD biasing.
  *         - LCD_VLCD1 rail can be connected to PB12 or PE11(*),
  *         - LCD_VLCD2 rail can be connected to PB2,
  *         - LCD_VLCD3 rail can be connected to PB0 or PE12(*)
  * @rmtoll SYSCFG_CFGR2 CAPA      LL_SYSCFG_SetVLCDRailConnection
  * @param  IoPinConnect This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_CAPA_VLCD1_PB12
  *         @arg @ref LL_SYSCFG_CAPA_VLCD1_PE11(*)
  *         @arg @ref LL_SYSCFG_CAPA_VLCD2_PB2
  *         @arg @ref LL_SYSCFG_CAPA_VLCD3_PB0
  *         @arg @ref LL_SYSCFG_CAPA_VLCD3_PE12(*)
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetVLCDRailConnection(uint32_t IoPinConnect)
{
  MODIFY_REG(SYSCFG->CFGR2, SYSCFG_CFGR2_CAPA, IoPinConnect);
}


/**
  * @brief  Get VLCD rail connection configuration
  * @note   One to three external capacitors can be connected to pads to do
  *         VLCD biasing.
  *         - LCD_VLCD1 rail can be connected to PB12 or PE11(*),
  *         - LCD_VLCD2 rail can be connected to PB2,
  *         - LCD_VLCD3 rail can be connected to PB0 or PE12(*)
  * @rmtoll SYSCFG_CFGR2 CAPA      LL_SYSCFG_GetVLCDRailConnection
  * @retval Returned value can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_CAPA_VLCD1_PB12
  *         @arg @ref LL_SYSCFG_CAPA_VLCD1_PE11(*)
  *         @arg @ref LL_SYSCFG_CAPA_VLCD2_PB2
  *         @arg @ref LL_SYSCFG_CAPA_VLCD3_PB0
  *         @arg @ref LL_SYSCFG_CAPA_VLCD3_PE12(*)
  *
  *         (*) value not defined in all devices
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetVLCDRailConnection(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_CAPA));
}
#endif

/**
  * @brief  Enable the I2C fast mode plus driving capability.
  * @rmtoll SYSCFG_CFGR2 I2C_PBx_FMP   LL_SYSCFG_EnableFastModePlus\n
  *         SYSCFG_CFGR2 I2Cx_FMP      LL_SYSCFG_EnableFastModePlus
  * @param  ConfigFastModePlus This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB6
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB7
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB8
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB9
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C1
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C2 (*)
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C3 (*)
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableFastModePlus(uint32_t ConfigFastModePlus)
{
  SET_BIT(SYSCFG->CFGR2, ConfigFastModePlus);
}

/**
  * @brief  Disable the I2C fast mode plus driving capability.
  * @rmtoll SYSCFG_CFGR2 I2C_PBx_FMP   LL_SYSCFG_DisableFastModePlus\n
  *         SYSCFG_CFGR2 I2Cx_FMP      LL_SYSCFG_DisableFastModePlus
  * @param  ConfigFastModePlus This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB6
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB7
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB8
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_PB9
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C1
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C2 (*)
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C3 (*)
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableFastModePlus(uint32_t ConfigFastModePlus)
{
  CLEAR_BIT(SYSCFG->CFGR2, ConfigFastModePlus);
}

/**
  * @brief  Select which pad is connected to VREFINT_ADC
  * @rmtoll SYSCFG_CFGR3 SEL_VREF_OUT  LL_SYSCFG_VREFINT_SetConnection
  * @param  IoPinConnect This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_VREFINT_CONNECT_NONE
  *         @arg @ref LL_SYSCFG_VREFINT_CONNECT_IO1
  *         @arg @ref LL_SYSCFG_VREFINT_CONNECT_IO2
  *         @arg @ref LL_SYSCFG_VREFINT_CONNECT_IO1_IO2
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_VREFINT_SetConnection(uint32_t IoPinConnect)
{
  MODIFY_REG(SYSCFG->CFGR3, SYSCFG_CFGR3_VREF_OUT, IoPinConnect);
}

/**
  * @brief  Get pad connection to VREFINT_ADC
  * @rmtoll SYSCFG_CFGR3 SEL_VREF_OUT  LL_SYSCFG_VREFINT_GetConnection
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_SYSCFG_VREFINT_CONNECT_NONE
  *         @arg @ref LL_SYSCFG_VREFINT_CONNECT_IO1
  *         @arg @ref LL_SYSCFG_VREFINT_CONNECT_IO2
  *         @arg @ref LL_SYSCFG_VREFINT_CONNECT_IO1_IO2
  */
__STATIC_INLINE uint32_t LL_SYSCFG_VREFINT_GetConnection(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_VREF_OUT));
}

/**
  * @brief  Buffer used to generate VREFINT reference for ADC enable
  * @note   The VrefInit buffer to ADC through internal path is also
  *         enabled using function LL_ADC_SetCommonPathInternalCh()
  *         with parameter LL_ADC_PATH_INTERNAL_VREFINT
  * @rmtoll SYSCFG_CFGR3 ENBUF_VREFINT_ADC   LL_SYSCFG_VREFINT_EnableADC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_VREFINT_EnableADC(void)
{
  SET_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUF_VREFINT_ADC);
}

/**
  * @brief  Buffer used to generate VREFINT reference for ADC disable
  * @rmtoll SYSCFG_CFGR3 ENBUF_VREFINT_ADC   LL_SYSCFG_VREFINT_DisableADC
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_VREFINT_DisableADC(void)
{
  CLEAR_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUF_VREFINT_ADC);
}

/**
  * @brief  Buffer used to generate temperature sensor reference for ADC enable
  * @rmtoll SYSCFG_CFGR3 ENBUF_SENSOR_ADC    LL_SYSCFG_TEMPSENSOR_Enable
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_TEMPSENSOR_Enable(void)
{
  SET_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUF_SENSOR_ADC);
}

/**
  * @brief  Buffer used to generate temperature sensor reference for ADC disable
  * @rmtoll SYSCFG_CFGR3 ENBUF_SENSOR_ADC    LL_SYSCFG_TEMPSENSOR_Disable
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_TEMPSENSOR_Disable(void)
{
  CLEAR_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUF_SENSOR_ADC);
}

/**
  * @brief  Buffer used to generate VREFINT reference for comparator enable
  * @rmtoll SYSCFG_CFGR3 ENBUF_VREFINT_COMP  LL_SYSCFG_VREFINT_EnableCOMP
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_VREFINT_EnableCOMP(void)
{
  SET_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP);
}

/**
  * @brief  Buffer used to generate VREFINT reference for comparator disable
  * @rmtoll SYSCFG_CFGR3 ENBUF_VREFINT_COMP  LL_SYSCFG_VREFINT_DisableCOMP
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_VREFINT_DisableCOMP(void)
{
  CLEAR_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP);
}

#if defined (RCC_HSI48_SUPPORT)
/**
  * @brief  Buffer used to generate VREFINT reference for HSI48 oscillator enable
  * @rmtoll SYSCFG_CFGR3 ENREF_HSI48         LL_SYSCFG_VREFINT_EnableHSI48
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_VREFINT_EnableHSI48(void)
{
  SET_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENREF_HSI48);
}

/**
  * @brief  Buffer used to generate VREFINT reference for HSI48 oscillator disable
  * @rmtoll SYSCFG_CFGR3 ENREF_HSI48         LL_SYSCFG_VREFINT_DisableHSI48
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_VREFINT_DisableHSI48(void)
{
  CLEAR_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENREF_HSI48);
}
#endif

/**
  * @brief  Check if VREFINT is ready or not
  * @note   When set, it indicates that VREFINT is available for BOR, PVD and LCD
  * @rmtoll SYSCFG_CFGR3 VREFINT_RDYF        LL_SYSCFG_VREFINT_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_VREFINT_IsReady(void)
{
  return (READ_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_VREFINT_RDYF) == SYSCFG_CFGR3_VREFINT_RDYF);
}

/**
  * @brief  Lock the whole content of SYSCFG_CFGR3 register
  * @note   After SYSCFG_CFGR3 register lock, only read access available.
  *         Only system hardware reset unlocks SYSCFG_CFGR3 register.
  * @rmtoll SYSCFG_CFGR3 REF_LOCK            LL_SYSCFG_VREFINT_Lock
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_VREFINT_Lock(void)
{
  SET_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_REF_LOCK);
}

/**
  * @brief  Check if SYSCFG_CFGR3 register is locked (only read access) or not
  * @note   When set, it indicates that SYSCFG_CFGR3 register is locked, only read access available
  * @rmtoll SYSCFG_CFGR3 REF_LOCK              LL_SYSCFG_VREFINT_IsLocked
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_VREFINT_IsLocked(void)
{
  return (READ_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_REF_LOCK) == SYSCFG_CFGR3_REF_LOCK);
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
  *         @arg @ref LL_SYSCFG_EXTI_PORTH (*)
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
  MODIFY_REG(SYSCFG->EXTICR[Line & 0xFFU], SYSCFG_EXTICR1_EXTI0 << (Line >> 16U), Port << (Line >> 16U));
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
  *         @arg @ref LL_SYSCFG_EXTI_PORTH (*)
  *
  *         (*) value not defined in all devices
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetEXTISource(uint32_t Line)
{
  return (uint32_t)(READ_BIT(SYSCFG->EXTICR[Line & 0xFFU], (SYSCFG_EXTICR1_EXTI0 << (Line >> 16U))) >> (Line >> 16U));
}


/**
  * @}
  */


/** @defgroup SYSTEM_LL_EF_DBGMCU DBGMCU
  * @{
  */

/**
  * @brief  Return the device identifier
  * @rmtoll DBGMCU_IDCODE DEV_ID        LL_DBGMCU_GetDeviceID
  * @retval Values between Min_Data=0x00 and Max_Data=0x7FF (ex: L053 -> 0x417, L073 -> 0x447)
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
  return (uint32_t)(READ_BIT(DBGMCU->IDCODE, DBGMCU_IDCODE_REV_ID) >> DBGMCU_REVID_POSITION);
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
  * @brief  Freeze APB1 peripherals (group1 peripherals)
  * @rmtoll APB1FZ      DBG_TIM2_STOP     LL_DBGMCU_ABP1_GRP1_FreezePeriph\n
  *         APB1FZ      DBG_TIM3_STOP     LL_DBGMCU_ABP1_GRP1_FreezePeriph\n
  *         APB1FZ      DBG_TIM6_STOP     LL_DBGMCU_ABP1_GRP1_FreezePeriph\n
  *         APB1FZ      DBG_TIM7_STOP     LL_DBGMCU_ABP1_GRP1_FreezePeriph\n
  *         APB1FZ      DBG_RTC_STOP      LL_DBGMCU_ABP1_GRP1_FreezePeriph\n
  *         APB1FZ      DBG_WWDG_STOP     LL_DBGMCU_ABP1_GRP1_FreezePeriph\n
  *         APB1FZ      DBG_IWDG_STOP     LL_DBGMCU_ABP1_GRP1_FreezePeriph\n
  *         APB1FZ      DBG_I2C1_STOP     LL_DBGMCU_ABP1_GRP1_FreezePeriph\n
  *         APB1FZ      DBG_I2C2_STOP     LL_DBGMCU_ABP1_GRP1_FreezePeriph\n
  *         APB1FZ      DBG_I2C3_STOP     LL_DBGMCU_ABP1_GRP1_FreezePeriph\n
  *         APB1FZ      DBG_LPTIMER_STOP  LL_DBGMCU_ABP1_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_TIM3_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_TIM6_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_TIM7_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_I2C2_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_I2C3_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_LPTIM1_STOP
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_ABP1_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB1FZ, Periphs);
}

/**
  * @brief  Unfreeze APB1 peripherals (group1 peripherals)
  * @rmtoll APB1FZ      DBG_TIM2_STOP     LL_DBGMCU_ABP1_GRP1_UnFreezePeriph\n
  *         APB1FZ      DBG_TIM3_STOP     LL_DBGMCU_ABP1_GRP1_UnFreezePeriph\n
  *         APB1FZ      DBG_TIM6_STOP     LL_DBGMCU_ABP1_GRP1_UnFreezePeriph\n
  *         APB1FZ      DBG_TIM7_STOP     LL_DBGMCU_ABP1_GRP1_UnFreezePeriph\n
  *         APB1FZ      DBG_RTC_STOP      LL_DBGMCU_ABP1_GRP1_UnFreezePeriph\n
  *         APB1FZ      DBG_WWDG_STOP     LL_DBGMCU_ABP1_GRP1_UnFreezePeriph\n
  *         APB1FZ      DBG_IWDG_STOP     LL_DBGMCU_ABP1_GRP1_UnFreezePeriph\n
  *         APB1FZ      DBG_I2C1_STOP     LL_DBGMCU_ABP1_GRP1_UnFreezePeriph\n
  *         APB1FZ      DBG_I2C2_STOP     LL_DBGMCU_ABP1_GRP1_UnFreezePeriph\n
  *         APB1FZ      DBG_I2C3_STOP     LL_DBGMCU_ABP1_GRP1_UnFreezePeriph\n
  *         APB1FZ      DBG_LPTIMER_STOP  LL_DBGMCU_ABP1_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_TIM3_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_TIM6_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_TIM7_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_WWDG_STOP
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_IWDG_STOP
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_I2C2_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_I2C3_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP1_GRP1_LPTIM1_STOP
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_ABP1_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB1FZ, Periphs);
}

/**
  * @brief  Freeze APB2 peripherals
  * @rmtoll APB2FZ      DBG_TIM22_STOP  LL_DBGMCU_ABP2_GRP1_FreezePeriph\n
  *         APB2FZ      DBG_TIM21_STOP  LL_DBGMCU_ABP2_GRP1_FreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_ABP2_GRP1_TIM22_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP2_GRP1_TIM21_STOP
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_ABP2_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB2FZ, Periphs);
}

/**
  * @brief  Unfreeze APB2 peripherals
  * @rmtoll APB2FZ      DBG_TIM22_STOP  LL_DBGMCU_ABP2_GRP1_UnFreezePeriph\n
  *         APB2FZ      DBG_TIM21_STOP  LL_DBGMCU_ABP2_GRP1_UnFreezePeriph
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_ABP2_GRP1_TIM22_STOP (*)
  *         @arg @ref LL_DBGMCU_ABP2_GRP1_TIM21_STOP
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_ABP2_GRP1_UnFreezePeriph(uint32_t Periphs)
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
  * @brief  Enable buffers used as a cache during read access
  * @rmtoll FLASH_ACR    DISAB_BUF     LL_FLASH_EnableBuffers
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnableBuffers(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_DISAB_BUF);
}

/**
  * @brief  Disable buffers used as a cache during read access
  * @note   When disabled, every read will access the NVM even for
  *         an address already read (for example, the previous address).
  * @rmtoll FLASH_ACR    DISAB_BUF     LL_FLASH_DisableBuffers
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisableBuffers(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_DISAB_BUF);
}

/**
  * @brief  Enable pre-read
  * @note   When enabled, the memory interface stores the last address
  *         read as data and tries to read the next one when no other
  *         read or write or prefetch operation is ongoing.
  *         It is automatically disabled every time the buffers are disabled.
  * @rmtoll FLASH_ACR    PRE_READ      LL_FLASH_EnablePreRead
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_EnablePreRead(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_PRE_READ);
}

/**
  * @brief  Disable pre-read
  * @rmtoll FLASH_ACR    PRE_READ      LL_FLASH_DisablePreRead
  * @retval None
  */
__STATIC_INLINE void LL_FLASH_DisablePreRead(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_PRE_READ);
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

#endif /* __STM32L0xx_LL_SYSTEM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
