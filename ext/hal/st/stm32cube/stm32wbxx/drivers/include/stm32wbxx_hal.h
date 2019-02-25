/**
  ******************************************************************************
  * @file    stm32wbxx_hal.h
  * @author  MCD Application Team
  * @brief   This file contains all the functions prototypes for the HAL
  *          module driver.
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
#ifndef STM32WBxx_HAL_H
#define STM32WBxx_HAL_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_conf.h"
#include "stm32wbxx_ll_system.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @defgroup HAL HAL
  * @{
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup HAL_Exported_Constants HAL Exported Constants
  * @{
  */

/** @defgroup HAL_TICK_FREQ Tick Frequency
  * @{
  */
#define  HAL_TICK_FREQ_10HZ         100U
#define  HAL_TICK_FREQ_100HZ        10U
#define  HAL_TICK_FREQ_1KHZ         1U
#define  HAL_TICK_FREQ_DEFAULT      HAL_TICK_FREQ_1KHZ

/**
  * @}
  */

/** @defgroup SYSCFG_Exported_Constants SYSCFG Exported Constants
  * @{
  */

/** @defgroup SYSCFG_BootMode BOOT Mode
  * @{
  */
#define SYSCFG_BOOT_MAINFLASH           LL_SYSCFG_REMAP_FLASH           /*!< Main Flash memory mapped at 0x00000000   */
#define SYSCFG_BOOT_SYSTEMFLASH         LL_SYSCFG_REMAP_SYSTEMFLASH     /*!< System Flash memory mapped at 0x00000000 */
#define SYSCFG_BOOT_SRAM                LL_SYSCFG_REMAP_SRAM            /*!< SRAM1 mapped at 0x00000000               */
#define SYSCFG_BOOT_QUADSPI             LL_SYSCFG_REMAP_QUADSPI         /*!< QUADSPI memory mapped at 0x00000000      */
/**
  * @}
  */

/** @defgroup SYSCFG_FPU_Interrupts FPU Interrupts
  * @{
  */
#define SYSCFG_IT_FPU_IOC               SYSCFG_CFGR1_FPU_IE_0           /*!< Floating Point Unit Invalid operation Interrupt */
#define SYSCFG_IT_FPU_DZC               SYSCFG_CFGR1_FPU_IE_1           /*!< Floating Point Unit Divide-by-zero Interrupt    */
#define SYSCFG_IT_FPU_UFC               SYSCFG_CFGR1_FPU_IE_2           /*!< Floating Point Unit Underflow Interrupt         */
#define SYSCFG_IT_FPU_OFC               SYSCFG_CFGR1_FPU_IE_3           /*!< Floating Point Unit Overflow Interrupt          */
#define SYSCFG_IT_FPU_IDC               SYSCFG_CFGR1_FPU_IE_4           /*!< Floating Point Unit Input denormal Interrupt    */
#define SYSCFG_IT_FPU_IXC               SYSCFG_CFGR1_FPU_IE_5           /*!< Floating Point Unit Inexact Interrupt           */

/**
  * @}
  */

/** @defgroup SYSCFG_SRAM2WRP SRAM2 Page Write protection (0 to 31)
  * @{
  */
#define SYSCFG_SRAM2WRP_PAGE0           LL_SYSCFG_SRAM2WRP_PAGE0        /*!< SRAM2A Write protection page 0  */
#define SYSCFG_SRAM2WRP_PAGE1           LL_SYSCFG_SRAM2WRP_PAGE1        /*!< SRAM2A Write protection page 1  */
#define SYSCFG_SRAM2WRP_PAGE2           LL_SYSCFG_SRAM2WRP_PAGE2        /*!< SRAM2A Write protection page 2  */
#define SYSCFG_SRAM2WRP_PAGE3           LL_SYSCFG_SRAM2WRP_PAGE3        /*!< SRAM2A Write protection page 3  */
#define SYSCFG_SRAM2WRP_PAGE4           LL_SYSCFG_SRAM2WRP_PAGE4        /*!< SRAM2A Write protection page 4  */
#define SYSCFG_SRAM2WRP_PAGE5           LL_SYSCFG_SRAM2WRP_PAGE5        /*!< SRAM2A Write protection page 5  */
#define SYSCFG_SRAM2WRP_PAGE6           LL_SYSCFG_SRAM2WRP_PAGE6        /*!< SRAM2A Write protection page 6  */
#define SYSCFG_SRAM2WRP_PAGE7           LL_SYSCFG_SRAM2WRP_PAGE7        /*!< SRAM2A Write protection page 7  */
#define SYSCFG_SRAM2WRP_PAGE8           LL_SYSCFG_SRAM2WRP_PAGE8        /*!< SRAM2A Write protection page 8  */
#define SYSCFG_SRAM2WRP_PAGE9           LL_SYSCFG_SRAM2WRP_PAGE9        /*!< SRAM2A Write protection page 9  */
#define SYSCFG_SRAM2WRP_PAGE10          LL_SYSCFG_SRAM2WRP_PAGE10       /*!< SRAM2A Write protection page 10 */
#define SYSCFG_SRAM2WRP_PAGE11          LL_SYSCFG_SRAM2WRP_PAGE11       /*!< SRAM2A Write protection page 11 */
#define SYSCFG_SRAM2WRP_PAGE12          LL_SYSCFG_SRAM2WRP_PAGE12       /*!< SRAM2A Write protection page 12 */
#define SYSCFG_SRAM2WRP_PAGE13          LL_SYSCFG_SRAM2WRP_PAGE13       /*!< SRAM2A Write protection page 13 */
#define SYSCFG_SRAM2WRP_PAGE14          LL_SYSCFG_SRAM2WRP_PAGE14       /*!< SRAM2A Write protection page 14 */
#define SYSCFG_SRAM2WRP_PAGE15          LL_SYSCFG_SRAM2WRP_PAGE15       /*!< SRAM2A Write protection page 15 */
#define SYSCFG_SRAM2WRP_PAGE16          LL_SYSCFG_SRAM2WRP_PAGE16       /*!< SRAM2A Write protection page 16 */
#define SYSCFG_SRAM2WRP_PAGE17          LL_SYSCFG_SRAM2WRP_PAGE17       /*!< SRAM2A Write protection page 17 */
#define SYSCFG_SRAM2WRP_PAGE18          LL_SYSCFG_SRAM2WRP_PAGE18       /*!< SRAM2A Write protection page 18 */
#define SYSCFG_SRAM2WRP_PAGE19          LL_SYSCFG_SRAM2WRP_PAGE19       /*!< SRAM2A Write protection page 19 */
#define SYSCFG_SRAM2WRP_PAGE20          LL_SYSCFG_SRAM2WRP_PAGE20       /*!< SRAM2A Write protection page 20 */
#define SYSCFG_SRAM2WRP_PAGE21          LL_SYSCFG_SRAM2WRP_PAGE21       /*!< SRAM2A Write protection page 21 */
#define SYSCFG_SRAM2WRP_PAGE22          LL_SYSCFG_SRAM2WRP_PAGE22       /*!< SRAM2A Write protection page 22 */
#define SYSCFG_SRAM2WRP_PAGE23          LL_SYSCFG_SRAM2WRP_PAGE23       /*!< SRAM2A Write protection page 23 */
#define SYSCFG_SRAM2WRP_PAGE24          LL_SYSCFG_SRAM2WRP_PAGE24       /*!< SRAM2A Write protection page 24 */
#define SYSCFG_SRAM2WRP_PAGE25          LL_SYSCFG_SRAM2WRP_PAGE25       /*!< SRAM2A Write protection page 25 */
#define SYSCFG_SRAM2WRP_PAGE26          LL_SYSCFG_SRAM2WRP_PAGE26       /*!< SRAM2A Write protection page 26 */
#define SYSCFG_SRAM2WRP_PAGE27          LL_SYSCFG_SRAM2WRP_PAGE27       /*!< SRAM2A Write protection page 27 */
#define SYSCFG_SRAM2WRP_PAGE28          LL_SYSCFG_SRAM2WRP_PAGE28       /*!< SRAM2A Write protection page 28 */
#define SYSCFG_SRAM2WRP_PAGE29          LL_SYSCFG_SRAM2WRP_PAGE29       /*!< SRAM2A Write protection page 29 */
#define SYSCFG_SRAM2WRP_PAGE30          LL_SYSCFG_SRAM2WRP_PAGE30       /*!< SRAM2A Write protection page 30 */
#define SYSCFG_SRAM2WRP_PAGE31          LL_SYSCFG_SRAM2WRP_PAGE31       /*!< SRAM2A Write protection page 31 */

/**
  * @}
  */

/** @defgroup SYSCFG_SRAM2WRP_32_63 SRAM2 Page Write protection (32 to 63)
  * @{
  */
#define SYSCFG_SRAM2WRP_PAGE32          LL_SYSCFG_SRAM2WRP_PAGE32       /*!< SRAM2B Write protection page 32 */
#define SYSCFG_SRAM2WRP_PAGE33          LL_SYSCFG_SRAM2WRP_PAGE33       /*!< SRAM2B Write protection page 33 */
#define SYSCFG_SRAM2WRP_PAGE34          LL_SYSCFG_SRAM2WRP_PAGE34       /*!< SRAM2B Write protection page 34 */
#define SYSCFG_SRAM2WRP_PAGE35          LL_SYSCFG_SRAM2WRP_PAGE35       /*!< SRAM2B Write protection page 35 */
#define SYSCFG_SRAM2WRP_PAGE36          LL_SYSCFG_SRAM2WRP_PAGE36       /*!< SRAM2B Write protection page 36 */
#define SYSCFG_SRAM2WRP_PAGE37          LL_SYSCFG_SRAM2WRP_PAGE37       /*!< SRAM2B Write protection page 37 */
#define SYSCFG_SRAM2WRP_PAGE38          LL_SYSCFG_SRAM2WRP_PAGE38       /*!< SRAM2B Write protection page 38 */
#define SYSCFG_SRAM2WRP_PAGE39          LL_SYSCFG_SRAM2WRP_PAGE39       /*!< SRAM2B Write protection page 39 */
#define SYSCFG_SRAM2WRP_PAGE40          LL_SYSCFG_SRAM2WRP_PAGE40       /*!< SRAM2B Write protection page 40 */
#define SYSCFG_SRAM2WRP_PAGE41          LL_SYSCFG_SRAM2WRP_PAGE41       /*!< SRAM2B Write protection page 41 */
#define SYSCFG_SRAM2WRP_PAGE42          LL_SYSCFG_SRAM2WRP_PAGE42       /*!< SRAM2B Write protection page 42 */
#define SYSCFG_SRAM2WRP_PAGE43          LL_SYSCFG_SRAM2WRP_PAGE43       /*!< SRAM2B Write protection page 43 */
#define SYSCFG_SRAM2WRP_PAGE44          LL_SYSCFG_SRAM2WRP_PAGE44       /*!< SRAM2B Write protection page 44 */
#define SYSCFG_SRAM2WRP_PAGE45          LL_SYSCFG_SRAM2WRP_PAGE45       /*!< SRAM2B Write protection page 45 */
#define SYSCFG_SRAM2WRP_PAGE46          LL_SYSCFG_SRAM2WRP_PAGE46       /*!< SRAM2B Write protection page 46 */
#define SYSCFG_SRAM2WRP_PAGE47          LL_SYSCFG_SRAM2WRP_PAGE47       /*!< SRAM2B Write protection page 47 */
#define SYSCFG_SRAM2WRP_PAGE48          LL_SYSCFG_SRAM2WRP_PAGE48       /*!< SRAM2B Write protection page 48 */
#define SYSCFG_SRAM2WRP_PAGE49          LL_SYSCFG_SRAM2WRP_PAGE49       /*!< SRAM2B Write protection page 49 */
#define SYSCFG_SRAM2WRP_PAGE50          LL_SYSCFG_SRAM2WRP_PAGE50       /*!< SRAM2B Write protection page 50 */
#define SYSCFG_SRAM2WRP_PAGE51          LL_SYSCFG_SRAM2WRP_PAGE51       /*!< SRAM2B Write protection page 51 */
#define SYSCFG_SRAM2WRP_PAGE52          LL_SYSCFG_SRAM2WRP_PAGE52       /*!< SRAM2B Write protection page 52 */
#define SYSCFG_SRAM2WRP_PAGE53          LL_SYSCFG_SRAM2WRP_PAGE53       /*!< SRAM2B Write protection page 53 */
#define SYSCFG_SRAM2WRP_PAGE54          LL_SYSCFG_SRAM2WRP_PAGE54       /*!< SRAM2B Write protection page 54 */
#define SYSCFG_SRAM2WRP_PAGE55          LL_SYSCFG_SRAM2WRP_PAGE55       /*!< SRAM2B Write protection page 55 */
#define SYSCFG_SRAM2WRP_PAGE56          LL_SYSCFG_SRAM2WRP_PAGE56       /*!< SRAM2B Write protection page 56 */
#define SYSCFG_SRAM2WRP_PAGE57          LL_SYSCFG_SRAM2WRP_PAGE57       /*!< SRAM2B Write protection page 57 */
#define SYSCFG_SRAM2WRP_PAGE58          LL_SYSCFG_SRAM2WRP_PAGE58       /*!< SRAM2B Write protection page 58 */
#define SYSCFG_SRAM2WRP_PAGE59          LL_SYSCFG_SRAM2WRP_PAGE59       /*!< SRAM2B Write protection page 59 */
#define SYSCFG_SRAM2WRP_PAGE60          LL_SYSCFG_SRAM2WRP_PAGE60       /*!< SRAM2B Write protection page 60 */
#define SYSCFG_SRAM2WRP_PAGE61          LL_SYSCFG_SRAM2WRP_PAGE61       /*!< SRAM2B Write protection page 61 */
#define SYSCFG_SRAM2WRP_PAGE62          LL_SYSCFG_SRAM2WRP_PAGE62       /*!< SRAM2B Write protection page 62 */
#define SYSCFG_SRAM2WRP_PAGE63          LL_SYSCFG_SRAM2WRP_PAGE63       /*!< SRAM2B Write protection page 63 */

/**
  * @}
  */

#if defined(VREFBUF)
/** @defgroup SYSCFG_VREFBUF_VoltageScale VREFBUF Voltage Scale
  * @{
  */
#define SYSCFG_VREFBUF_VOLTAGE_SCALE0   LL_VREFBUF_VOLTAGE_SCALE0       /*!< Voltage reference scale 0 (VREF_OUT1) */
#define SYSCFG_VREFBUF_VOLTAGE_SCALE1   LL_VREFBUF_VOLTAGE_SCALE1       /*!< Voltage reference scale 1 (VREF_OUT2) */

/**
  * @}
  */

/** @defgroup SYSCFG_VREFBUF_HighImpedance VREFBUF High Impedance
  * @{
  */
#define SYSCFG_VREFBUF_HIGH_IMPEDANCE_DISABLE   0x00000000U             /*!< VREF_plus pin is internally connected to Voltage reference buffer output */
#define SYSCFG_VREFBUF_HIGH_IMPEDANCE_ENABLE    VREFBUF_CSR_HIZ         /*!< VREF_plus pin is high impedance */

/**
  * @}
  */
#endif /* VREFBUF */

/** @defgroup SYSCFG_SRAM_flags_definition SRAM Flags
  * @{
  */

#define SYSCFG_FLAG_SRAM2_PE            SYSCFG_CFGR2_SPF                /*!< SRAM2 parity error */
#define SYSCFG_FLAG_SRAM2_BUSY          SYSCFG_SCSR_SRAM2BSY            /*!< SRAM2 busy by erase operation */

/**
  * @}
  */

/** @defgroup SYSCFG_FastModePlus_GPIO Fast-mode Plus on GPIO
  * @{
  */

/** @brief  Fast-mode Plus driving capability on a specific GPIO
  */
#define SYSCFG_FASTMODEPLUS_PB6         SYSCFG_CFGR1_I2C_PB6_FMP        /*!< Enable Fast-mode Plus on PB6 */
#define SYSCFG_FASTMODEPLUS_PB7         SYSCFG_CFGR1_I2C_PB7_FMP        /*!< Enable Fast-mode Plus on PB7 */
#define SYSCFG_FASTMODEPLUS_PB8         SYSCFG_CFGR1_I2C_PB8_FMP        /*!< Enable Fast-mode Plus on PB8 */
#define SYSCFG_FASTMODEPLUS_PB9         SYSCFG_CFGR1_I2C_PB9_FMP        /*!< Enable Fast-mode Plus on PB9 */

/**
 * @}
 */

/** @defgroup Secure_IP_Write_Access Secure IP Write Access
  * @{
  */
#define HAL_SYSCFG_SECURE_ACCESS_AES1   LL_SYSCFG_SECURE_ACCESS_AES1    /*!< Enabling the security access of Advanced Encryption Standard 1 KEY[7:0] */
#define HAL_SYSCFG_SECURE_ACCESS_AES2   LL_SYSCFG_SECURE_ACCESS_AES2    /*!< Enabling the security access of Advanced Encryption Standard 2          */
#define HAL_SYSCFG_SECURE_ACCESS_PKA    LL_SYSCFG_SECURE_ACCESS_PKA     /*!< Enabling the security access of Public Key Accelerator                  */
#define HAL_SYSCFG_SECURE_ACCESS_RNG    LL_SYSCFG_SECURE_ACCESS_RNG     /*!< Enabling the security access of Random Number Generator                 */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup HAL_Exported_Macros HAL Exported Macros
  * @{
  */

/** @defgroup DBGMCU_Exported_Macros DBGMCU Exported Macros
  * @{
  */

/** @brief  Freeze and Unfreeze Peripherals in Debug mode
  */

/** @defgroup DBGMCU_APBx_GRPx_STOP_IP DBGMCU CPU1 APBx GRPx STOP IP
  * @{
  */
#if defined(LL_DBGMCU_APB1_GRP1_TIM2_STOP)
#define __HAL_DBGMCU_FREEZE_TIM2()              LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_TIM2_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM2()            LL_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_DBGMCU_APB1_GRP1_TIM2_STOP)
#endif

#if defined(LL_DBGMCU_APB1_GRP1_RTC_STOP)
#define __HAL_DBGMCU_FREEZE_RTC()               LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_RTC_STOP)
#define __HAL_DBGMCU_UNFREEZE_RTC()             LL_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_DBGMCU_APB1_GRP1_RTC_STOP)
#endif

#if defined(LL_DBGMCU_APB1_GRP1_WWDG_STOP)
#define __HAL_DBGMCU_FREEZE_WWDG()              LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_WWDG_STOP)
#define __HAL_DBGMCU_UNFREEZE_WWDG()            LL_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_DBGMCU_APB1_GRP1_WWDG_STOP)
#endif

#if defined(LL_DBGMCU_APB1_GRP1_IWDG_STOP)
#define __HAL_DBGMCU_FREEZE_IWDG()              LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_IWDG_STOP)
#define __HAL_DBGMCU_UNFREEZE_IWDG()            LL_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_DBGMCU_APB1_GRP1_IWDG_STOP)
#endif

#if defined(LL_DBGMCU_APB1_GRP1_I2C1_STOP)
#define __HAL_DBGMCU_FREEZE_I2C1_TIMEOUT()      LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_I2C1_STOP)
#define __HAL_DBGMCU_UNFREEZE_I2C1_TIMEOUT()    LL_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_DBGMCU_APB1_GRP1_I2C1_STOP)
#endif

#if defined(LL_DBGMCU_APB1_GRP1_I2C3_STOP)
#define __HAL_DBGMCU_FREEZE_I2C3_TIMEOUT()      LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_I2C3_STOP)
#define __HAL_DBGMCU_UNFREEZE_I2C3_TIMEOUT()    LL_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_DBGMCU_APB1_GRP1_I2C3_STOP)
#endif

#if defined(LL_DBGMCU_APB1_GRP1_LPTIM1_STOP)
#define __HAL_DBGMCU_FREEZE_LPTIM1()            LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_LPTIM1_STOP)
#define __HAL_DBGMCU_UNFREEZE_LPTIM1()          LL_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_DBGMCU_APB1_GRP1_LPTIM1_STOP)
#endif

#if defined(LL_DBGMCU_APB1_GRP2_LPTIM2_STOP)
#define __HAL_DBGMCU_FREEZE_LPTIM2()            LL_DBGMCU_APB1_GRP2_FreezePeriph(LL_DBGMCU_APB1_GRP2_LPTIM2_STOP)
#define __HAL_DBGMCU_UNFREEZE_LPTIM2()          LL_DBGMCU_APB1_GRP2_UnFreezePeriph(LL_DBGMCU_APB1_GRP2_LPTIM2_STOP)
#endif

#if defined(LL_DBGMCU_APB2_GRP1_TIM1_STOP)
#define __HAL_DBGMCU_FREEZE_TIM1()              LL_DBGMCU_APB2_GRP1_FreezePeriph(LL_DBGMCU_APB2_GRP1_TIM1_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM1()            LL_DBGMCU_APB2_GRP1_UnFreezePeriph(LL_DBGMCU_APB2_GRP1_TIM1_STOP)
#endif

#if defined(LL_DBGMCU_APB2_GRP1_TIM16_STOP)
#define __HAL_DBGMCU_FREEZE_TIM16()             LL_DBGMCU_APB2_GRP1_FreezePeriph(LL_DBGMCU_APB2_GRP1_TIM16_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM16()           LL_DBGMCU_APB2_GRP1_UnFreezePeriph(LL_DBGMCU_APB2_GRP1_TIM16_STOP)
#endif

#if defined(LL_DBGMCU_APB2_GRP1_TIM17_STOP)
#define __HAL_DBGMCU_FREEZE_TIM17()             LL_DBGMCU_APB2_GRP1_FreezePeriph(LL_DBGMCU_APB2_GRP1_TIM17_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM17()           LL_DBGMCU_APB2_GRP1_UnFreezePeriph(LL_DBGMCU_APB2_GRP1_TIM17_STOP)
#endif

/**
  * @}
  */

/** @defgroup DBGMCU_C2_APBx_GRPx_STOP_IP DBGMCU CPU2 APBx GRPx STOP IP
  * @{
  */
#if defined(LL_C2_DBGMCU_APB1_GRP1_TIM2_STOP)
#define __HAL_C2_DBGMCU_FREEZE_TIM2()           LL_C2_DBGMCU_APB1_GRP1_FreezePeriph(LL_C2_DBGMCU_APB1_GRP1_TIM2_STOP)
#define __HAL_C2_DBGMCU_UNFREEZE_TIM2()         LL_C2_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_C2_DBGMCU_APB1_GRP1_TIM2_STOP)
#endif

#if defined(LL_C2_DBGMCU_APB1_GRP1_RTC_STOP)
#define __HAL_C2_DBGMCU_FREEZE_RTC()            LL_C2_DBGMCU_APB1_GRP1_FreezePeriph(LL_C2_DBGMCU_APB1_GRP1_RTC_STOP)
#define __HAL_C2_DBGMCU_UNFREEZE_RTC()          LL_C2_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_C2_DBGMCU_APB1_GRP1_RTC_STOP)
#endif

#if defined(LL_C2_DBGMCU_APB1_GRP1_IWDG_STOP)
#define __HAL_C2_DBGMCU_FREEZE_IWDG()           LL_C2_DBGMCU_APB1_GRP1_FreezePeriph(LL_C2_DBGMCU_APB1_GRP1_IWDG_STOP)
#define __HAL_C2_DBGMCU_UNFREEZE_IWDG()         LL_C2_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_C2_DBGMCU_APB1_GRP1_IWDG_STOP)
#endif

#if defined(LL_C2_DBGMCU_APB1_GRP1_I2C1_STOP)
#define __HAL_C2_DBGMCU_FREEZE_I2C1_TIMEOUT()   LL_C2_DBGMCU_APB1_GRP1_FreezePeriph(LL_C2_DBGMCU_APB1_GRP1_I2C1_STOP)
#define __HAL_C2_DBGMCU_UNFREEZE_I2C1_TIMEOUT() LL_C2_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_C2_DBGMCU_APB1_GRP1_I2C1_STOP)
#endif

#if defined(LL_C2_DBGMCU_APB1_GRP1_I2C3_STOP)
#define __HAL_C2_DBGMCU_FREEZE_I2C3_TIMEOUT()   LL_C2_DBGMCU_APB1_GRP1_FreezePeriph(LL_C2_DBGMCU_APB1_GRP1_I2C3_STOP)
#define __HAL_C2_DBGMCU_UNFREEZE_I2C3_TIMEOUT() LL_C2_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_C2_DBGMCU_APB1_GRP1_I2C3_STOP)
#endif

#if defined(LL_C2_DBGMCU_APB1_GRP1_LPTIM1_STOP)
#define __HAL_C2_DBGMCU_FREEZE_LPTIM1()         LL_C2_DBGMCU_APB1_GRP1_FreezePeriph(LL_C2_DBGMCU_APB1_GRP1_LPTIM1_STOP)
#define __HAL_C2_DBGMCU_UNFREEZE_LPTIM1()       LL_C2_DBGMCU_APB1_GRP1_UnFreezePeriph(LL_C2_DBGMCU_APB1_GRP1_LPTIM1_STOP)
#endif

#if defined(LL_C2_DBGMCU_APB1_GRP2_LPTIM2_STOP)
#define __HAL_C2_DBGMCU_FREEZE_LPTIM2()         LL_C2_DBGMCU_APB1_GRP2_FreezePeriph(LL_C2_DBGMCU_APB1_GRP2_LPTIM2_STOP)
#define __HAL_C2_DBGMCU_UNFREEZE_LPTIM2()       LL_C2_DBGMCU_APB1_GRP2_UnFreezePeriph(LL_C2_DBGMCU_APB1_GRP2_LPTIM2_STOP)
#endif

#if defined(LL_C2_DBGMCU_APB2_GRP1_TIM1_STOP)
#define __HAL_C2_DBGMCU_FREEZE_TIM1()           LL_C2_DBGMCU_APB2_GRP1_FreezePeriph(LL_C2_DBGMCU_APB2_GRP1_TIM1_STOP)
#define __HAL_C2_DBGMCU_UNFREEZE_TIM1()         LL_C2_DBGMCU_APB2_GRP1_UnFreezePeriph(LL_C2_DBGMCU_APB2_GRP1_TIM1_STOP)
#endif

#if defined(LL_C2_DBGMCU_APB2_GRP1_TIM16_STOP)
#define __HAL_C2_DBGMCU_FREEZE_TIM16()          LL_C2_DBGMCU_APB2_GRP1_FreezePeriph(LL_C2_DBGMCU_APB2_GRP1_TIM16_STOP)
#define __HAL_C2_DBGMCU_UNFREEZE_TIM16()        LL_C2_DBGMCU_APB2_GRP1_UnFreezePeriph(LL_C2_DBGMCU_APB2_GRP1_TIM16_STOP)
#endif

#if defined(LL_C2_DBGMCU_APB2_GRP1_TIM17_STOP)
#define __HAL_C2_DBGMCU_FREEZE_TIM17()          LL_C2_DBGMCU_APB2_GRP1_FreezePeriph(LL_C2_DBGMCU_APB2_GRP1_TIM17_STOP)
#define __HAL_C2_DBGMCU_UNFREEZE_TIM17()        LL_C2_DBGMCU_APB2_GRP1_UnFreezePeriph(LL_C2_DBGMCU_APB2_GRP1_TIM17_STOP)
#endif

/**
  * @}
  */

/**
  * @}
  */

/** @defgroup SYSCFG_Exported_Macros SYSCFG Exported Macros
  * @{
  */

/** @brief  Main Flash memory mapped at 0x00000000
  */
#define __HAL_SYSCFG_REMAPMEMORY_FLASH()        LL_SYSCFG_SetRemapMemory(LL_SYSCFG_REMAP_FLASH)

/** @brief  System Flash memory mapped at 0x00000000
  */
#define __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH()  LL_SYSCFG_SetRemapMemory(LL_SYSCFG_REMAP_SYSTEMFLASH)

/** @brief  Embedded SRAM mapped at 0x00000000
  */
#define __HAL_SYSCFG_REMAPMEMORY_SRAM()         LL_SYSCFG_SetRemapMemory(LL_SYSCFG_REMAP_SRAM)

/** @brief  QUADSPI mapped at 0x00000000.
  */
#define __HAL_SYSCFG_REMAPMEMORY_QUADSPI()      LL_SYSCFG_SetRemapMemory(LL_SYSCFG_REMAP_QUADSPI)

/**
  * @brief  Return the boot mode as configured by user.
  * @retval The boot mode as configured by user. The returned value can be one
  *         of the following values:
  *           @arg @ref SYSCFG_BOOT_MAINFLASH
  *           @arg @ref SYSCFG_BOOT_SYSTEMFLASH
  *           @arg @ref SYSCFG_BOOT_SRAM
  *           @arg @ref SYSCFG_BOOT_QUADSPI
  */
#define __HAL_SYSCFG_GET_BOOT_MODE()            LL_SYSCFG_GetRemapMemory()

/** @brief  SRAM2 page 0 to 31 write protection enable macro
  * @param  __SRAM2WRP__  This parameter can be a combination of values of @ref SYSCFG_SRAM2WRP
  * @note   Write protection can only be disabled by a system reset
  */
/* Legacy define */
#define __HAL_SYSCFG_SRAM2_WRP_1_31_ENABLE      __HAL_SYSCFG_SRAM2_WRP_0_31_ENABLE
#define __HAL_SYSCFG_SRAM2_WRP_0_31_ENABLE(__SRAM2WRP__)    do {assert_param(IS_SYSCFG_SRAM2WRP_PAGE((__SRAM2WRP__)));\
                                                                LL_SYSCFG_EnableSRAM2PageWRP_0_31(__SRAM2WRP__);\
                                                            }while(0)

/** @brief  SRAM2 page 32 to 63 write protection enable macro
  * @param  __SRAM2WRP__  This parameter can be a combination of values of @ref SYSCFG_SRAM2WRP_32_63
  * @note   Write protection can only be disabled by a system reset
  */
#define __HAL_SYSCFG_SRAM2_WRP_32_63_ENABLE(__SRAM2WRP__)   do {assert_param(IS_SYSCFG_SRAM2WRP_PAGE((__SRAM2WRP__)));\
                                                                LL_SYSCFG_EnableSRAM2PageWRP_32_63(__SRAM2WRP__);\
                                                            }while(0)

/** @brief  SRAM2 page write protection unlock prior to erase
  * @note   Writing a wrong key reactivates the write protection
  */
#define __HAL_SYSCFG_SRAM2_WRP_UNLOCK()         LL_SYSCFG_UnlockSRAM2WRP()

/** @brief  SRAM2 erase
  * @note   __SYSCFG_GET_FLAG(SYSCFG_FLAG_SRAM2_BUSY) may be used to check end of erase
  */
#define __HAL_SYSCFG_SRAM2_ERASE()              LL_SYSCFG_EnableSRAM2Erase()

/** @brief  Floating Point Unit interrupt enable/disable macros
  * @param __INTERRUPT__  This parameter can be a value of @ref SYSCFG_FPU_Interrupts
  */
#define __HAL_SYSCFG_FPU_INTERRUPT_ENABLE(__INTERRUPT__)    do {assert_param(IS_SYSCFG_FPU_INTERRUPT((__INTERRUPT__)));\
                                                                SET_BIT(SYSCFG->CFGR1, (__INTERRUPT__));\
                                                            }while(0)

#define __HAL_SYSCFG_FPU_INTERRUPT_DISABLE(__INTERRUPT__)   do {assert_param(IS_SYSCFG_FPU_INTERRUPT((__INTERRUPT__)));\
                                                                CLEAR_BIT(SYSCFG->CFGR1, (__INTERRUPT__));\
                                                            }while(0)

/** @brief  SYSCFG Break ECC lock.
  *         Enable and lock the connection of Flash ECC error connection to TIM1/16/17 Break input.
  * @note   The selected configuration is locked and can be unlocked only by system reset.
  */
#define __HAL_SYSCFG_BREAK_ECC_LOCK()           LL_SYSCFG_SetTIMBreakInputs(LL_SYSCFG_TIMBREAK_ECC)

/** @brief  SYSCFG Break Cortex-M4 Lockup lock.
  *         Enable and lock the connection of Cortex-M4 LOCKUP (Hardfault) output to TIM1/16/17 Break input.
  * @note   The selected configuration is locked and can be unlocked only by system reset.
  */
#define __HAL_SYSCFG_BREAK_LOCKUP_LOCK()        LL_SYSCFG_SetTIMBreakInputs(LL_SYSCFG_TIMBREAK_LOCKUP)

/** @brief  SYSCFG Break PVD lock.
  *         Enable and lock the PVD connection to Timer1/16/17 Break input, as well as the PVDE and PLS[2:0] in the PWR_CR2 register.
  * @note   The selected configuration is locked and can be unlocked only by system reset.
  */
#define __HAL_SYSCFG_BREAK_PVD_LOCK()           LL_SYSCFG_SetTIMBreakInputs(LL_SYSCFG_TIMBREAK_PVD)

/** @brief  SYSCFG Break SRAM2 parity lock.
  *         Enable and lock the SRAM2 parity error signal connection to TIM1/16/17 Break input.
  * @note   The selected configuration is locked and can be unlocked by system reset.
  */
#define __HAL_SYSCFG_BREAK_SRAM2PARITY_LOCK()   LL_SYSCFG_SetTIMBreakInputs(LL_SYSCFG_TIMBREAK_SRAM2_PARITY)

/** @brief  Check SYSCFG flag is set or not.
  * @param  __FLAG__  specifies the flag to check.
  *         This parameter can be one of the following values:
  *            @arg @ref SYSCFG_FLAG_SRAM2_PE   SRAM2 Parity Error Flag
  *            @arg @ref SYSCFG_FLAG_SRAM2_BUSY SRAM2 Erase Ongoing
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_SYSCFG_GET_FLAG(__FLAG__)         ((((((__FLAG__) == SYSCFG_SCSR_SRAM2BSY)? SYSCFG->SCSR : SYSCFG->CFGR2) & (__FLAG__))!= 0U) ? 1U : 0U)

/** @brief  Set the SPF bit to clear the SRAM Parity Error Flag.
  */
#define __HAL_SYSCFG_CLEAR_FLAG()               LL_SYSCFG_ClearFlag_SP()

/** @brief  Fast mode Plus driving capability enable/disable macros
  * @param __FASTMODEPLUS__ This parameter can be a value of @ref SYSCFG_FastModePlus_GPIO
  */
#define __HAL_SYSCFG_FASTMODEPLUS_ENABLE(__FASTMODEPLUS__)  do {assert_param(IS_SYSCFG_FASTMODEPLUS((__FASTMODEPLUS__))); \
                                                                LL_SYSCFG_EnableFastModePlus(__FASTMODEPLUS__);           \
                                                               }while(0)

#define __HAL_SYSCFG_FASTMODEPLUS_DISABLE(__FASTMODEPLUS__) do {assert_param(IS_SYSCFG_FASTMODEPLUS((__FASTMODEPLUS__))); \
                                                                LL_SYSCFG_DisableFastModePlus(__FASTMODEPLUS__);          \
                                                               }while(0)

/**
  * @}
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup HAL_Private_Macros HAL Private Macros
  * @{
  */

/** @defgroup SYSCFG_Private_Macros SYSCFG Private Macros
  * @{
  */

#define IS_SYSCFG_FPU_INTERRUPT(__INTERRUPT__)          ((((__INTERRUPT__) & SYSCFG_IT_FPU_IOC) == SYSCFG_IT_FPU_IOC) || \
                                                         (((__INTERRUPT__) & SYSCFG_IT_FPU_DZC) == SYSCFG_IT_FPU_DZC) || \
                                                         (((__INTERRUPT__) & SYSCFG_IT_FPU_UFC) == SYSCFG_IT_FPU_UFC) || \
                                                         (((__INTERRUPT__) & SYSCFG_IT_FPU_OFC) == SYSCFG_IT_FPU_OFC) || \
                                                         (((__INTERRUPT__) & SYSCFG_IT_FPU_IDC) == SYSCFG_IT_FPU_IDC) || \
                                                         (((__INTERRUPT__) & SYSCFG_IT_FPU_IXC) == SYSCFG_IT_FPU_IXC))

#define IS_SYSCFG_SRAM2WRP_PAGE(__PAGE__)               (((__PAGE__) > 0U) && ((__PAGE__) <= 0xFFFFFFFFU))

#define IS_SYSCFG_VREFBUF_VOLTAGE_SCALE(__SCALE__)      (((__SCALE__) == SYSCFG_VREFBUF_VOLTAGE_SCALE0) || \
                                                         ((__SCALE__) == SYSCFG_VREFBUF_VOLTAGE_SCALE1))

#define IS_SYSCFG_VREFBUF_HIGH_IMPEDANCE(__VALUE__)     (((__VALUE__) == SYSCFG_VREFBUF_HIGH_IMPEDANCE_DISABLE) || \
                                                         ((__VALUE__) == SYSCFG_VREFBUF_HIGH_IMPEDANCE_ENABLE))

#define IS_SYSCFG_VREFBUF_TRIMMING(__VALUE__)           (((__VALUE__) > 0U) && ((__VALUE__) <= VREFBUF_CCR_TRIM))

#define IS_SYSCFG_FASTMODEPLUS(__PIN__)                 ((((__PIN__) & SYSCFG_FASTMODEPLUS_PB6)  == SYSCFG_FASTMODEPLUS_PB6)  || \
                                                         (((__PIN__) & SYSCFG_FASTMODEPLUS_PB7)  == SYSCFG_FASTMODEPLUS_PB7)  || \
                                                         (((__PIN__) & SYSCFG_FASTMODEPLUS_PB8)  == SYSCFG_FASTMODEPLUS_PB8)  || \
                                                         (((__PIN__) & SYSCFG_FASTMODEPLUS_PB9)  == SYSCFG_FASTMODEPLUS_PB9))

#define IS_SYSCFG_SECURITY_ACCESS(__VALUE__)            ((((__VALUE__) & HAL_SYSCFG_SECURE_ACCESS_AES1)  == HAL_SYSCFG_SECURE_ACCESS_AES1)  || \
                                                         (((__VALUE__) & HAL_SYSCFG_SECURE_ACCESS_AES2)  == HAL_SYSCFG_SECURE_ACCESS_AES2)  || \
                                                         (((__VALUE__) & HAL_SYSCFG_SECURE_ACCESS_PKA)   == HAL_SYSCFG_SECURE_ACCESS_PKA)   || \
                                                         (((__VALUE__) & HAL_SYSCFG_SECURE_ACCESS_RNG)   == HAL_SYSCFG_SECURE_ACCESS_RNG))

/**
  * @}
  */

/**
  * @}
  */

/** @defgroup HAL_Private_Macros HAL Private Macros
  * @{
  */
#define IS_TICKFREQ(FREQ) (((FREQ) == HAL_TICK_FREQ_10HZ)  || \
                           ((FREQ) == HAL_TICK_FREQ_100HZ) || \
                           ((FREQ) == HAL_TICK_FREQ_1KHZ))
/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/

/** @defgroup HAL_Exported_Functions HAL Exported Functions
  * @{
  */

/** @defgroup HAL_Exported_Functions_Group1 HAL Initialization and Configuration functions
  * @{
  */

/* Initialization and Configuration functions  ******************************/
HAL_StatusTypeDef HAL_Init(void);
HAL_StatusTypeDef HAL_DeInit(void);
void HAL_MspInit(void);
void HAL_MspDeInit(void);

HAL_StatusTypeDef HAL_InitTick (uint32_t TickPriority);

/**
  * @}
  */

/** @defgroup HAL_Exported_Functions_Group2 HAL Control functions
  * @{
  */

/* Peripheral Control functions  ************************************************/
void HAL_IncTick(void);
void HAL_Delay(uint32_t Delay);
uint32_t HAL_GetTick(void);
uint32_t HAL_GetTickPrio(void);
HAL_StatusTypeDef HAL_SetTickFreq(uint32_t Freq);
uint32_t HAL_GetTickFreq(void);
void HAL_SuspendTick(void);
void HAL_ResumeTick(void);
uint32_t HAL_GetHalVersion(void);
uint32_t HAL_GetREVID(void);
uint32_t HAL_GetDEVID(void);
uint32_t HAL_GetUIDw0(void);
uint32_t HAL_GetUIDw1(void);
uint32_t HAL_GetUIDw2(void);

/**
  * @}
  */

/** @defgroup HAL_Exported_Functions_Group3 HAL Debug functions
  * @{
  */

/* DBGMCU Peripheral Control functions  *****************************************/
void HAL_DBGMCU_EnableDBGSleepMode(void);
void HAL_DBGMCU_DisableDBGSleepMode(void);
void HAL_DBGMCU_EnableDBGStopMode(void);
void HAL_DBGMCU_DisableDBGStopMode(void);
void HAL_DBGMCU_EnableDBGStandbyMode(void);
void HAL_DBGMCU_DisableDBGStandbyMode(void);
/**
  * @}
  */

/* Exported variables ---------------------------------------------------------*/
/** @addtogroup HAL_Exported_Variables
  * @{
  */
extern __IO uint32_t uwTick;
extern uint32_t uwTickPrio;
extern uint32_t uwTickFreq;
/**
  * @}
  */

/** @addtogroup HAL_Exported_Functions_Group4 HAL System Configuration functions
  * @{
  */

/* SYSCFG Control functions  ****************************************************/
void HAL_SYSCFG_SRAM2Erase(void);
void HAL_SYSCFG_DisableSRAMFetch(void);
uint32_t HAL_SYSCFG_IsEnabledSRAMFetch(void);

void HAL_SYSCFG_VREFBUF_VoltageScalingConfig(uint32_t VoltageScaling);
void HAL_SYSCFG_VREFBUF_HighImpedanceConfig(uint32_t Mode);
void HAL_SYSCFG_VREFBUF_TrimmingConfig(uint32_t TrimmingValue);
HAL_StatusTypeDef HAL_SYSCFG_EnableVREFBUF(void);
void HAL_SYSCFG_DisableVREFBUF(void);

void HAL_SYSCFG_EnableIOBooster(void);
void HAL_SYSCFG_DisableIOBooster(void);
void HAL_SYSCFG_EnableIOVdd(void);
void HAL_SYSCFG_DisableIOVdd(void);

void HAL_SYSCFG_EnableSecurityAccess(uint32_t SecurityAccess);
void HAL_SYSCFG_DisableSecurityAccess(uint32_t SecurityAccess);
uint32_t HAL_SYSCFG_IsEnabledSecurityAccess(uint32_t SecurityAccess);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32WBxx_HAL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
