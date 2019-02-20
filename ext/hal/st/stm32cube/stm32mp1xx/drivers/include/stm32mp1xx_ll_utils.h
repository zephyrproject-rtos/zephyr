/**
  ******************************************************************************
  * @file    stm32mp1xx_ll_utils.h
  * @author  MCD Application Team
  * @brief   Header file of UTILS LL module.
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
    [..]
    The LL UTILS driver contains a set of generic APIs that can be
    used by user:
      (+) Device electronic signature
      (+) Timing functions
      (+) PLL configuration functions

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
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
#ifndef STM32MP1xx_LL_UTILS_H
#define STM32MP1xx_LL_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx.h"

/** @addtogroup STM32MP1xx_LL_Driver
  * @{
  */

/** @defgroup UTILS_LL UTILS
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
/** @defgroup UTILS_LL_Private_Constants UTILS Private Constants
  * @{
  */

/* Max delay can be used in LL_mDelay */
#define LL_MAX_DELAY                  0xFFFFFFFFU

/**
 * @brief Unique device ID register base address
 */
#define UID_BASE_ADDRESS              UID_BASE

/**
 * @brief Package data register base address
 */
#define PACKAGE_BASE_ADDRESS          PACKAGE_BASE

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup UTILS_LL_Private_Macros UTILS Private Macros
  * @{
  */
/**
  * @}
  */
/* Exported types ------------------------------------------------------------*/
/** @defgroup UTILS_LL_ES_INIT UTILS Exported structures
  * @{
  */
/**
  * @brief  UTILS PLL structure definition
  */
typedef struct
{
  uint32_t PLLM;   /*!< Division factor for PLL VCO input clock.
                        This parameter can be a value  between 1 and 64 */

  uint32_t PLLN;   /*!< Multiplication factor for PLL VCO output clock.
                        This parameter must be a number between 4 and 512 */

  uint32_t PLLP;   /*!< Division for the P divider
                        This parameter can be a value between 1 and 128

                        This feature can be modified afterwards using unitary
                        functions @ref LL_RCC_PLL1_SetP, @ref LL_RCC_PLL2_SetP,
                        @ref LL_RCC_PLL3_SetP and @ref LL_RCC_PLL4_SetP */

  uint32_t PLLQ;   /*!< Division for the Q divider
                        This parameter can be a value between 1 and 128

                        This feature can be modified afterwards using unitary
                        functions @ref LL_RCC_PLL2_SetQ, @ref LL_RCC_PLL3_SetQ
                        and @ref LL_RCC_PLL4_SetQ*/

  uint32_t PLLR;   /*!< Division for the R divider
                        This parameter can be a value between 1 and 128

                        This feature can be modified afterwards using unitary
                        functions @ref LL_RCC_PLL2_SetR, @ref LL_RCC_PLL3_SetR
                        and @ref LL_RCC_PLL4_SetR */

  uint32_t PLLFRACV;  /*!< Fractional part of the multiplication factor for PLLx VCO.
                        This parameter can be a value between 0 and 8191 (0x1FFF) */
} LL_UTILS_PLLTypeDef;

/**
  * @brief  UTILS PLLs system structure definition
  */
typedef struct
{
  LL_UTILS_PLLTypeDef PLL1;   /*!< PLL1 structure parameters */

  LL_UTILS_PLLTypeDef PLL2;   /*!< PLL2 structure parameters */

  LL_UTILS_PLLTypeDef PLL3;   /*!< PLL3 structure parameters */

  LL_UTILS_PLLTypeDef PLL4;   /*!< PLL4 structure parameters */

} LL_UTILS_PLLsInitTypeDef;

/**
  * @brief  UTILS System, AHB and APB buses clock configuration structure definition
  */
typedef struct
{
  uint32_t MPUDivider;            /*!< The MPU divider. This clock is derived from the CK_PLL1_P clock.
                                     This parameter can be a value of @ref RCC_LL_EC_MPU_DIV

                                     This feature can be modified afterwards using unitary function
                                     @ref LL_RCC_SetMPUPrescaler(). */

  uint32_t AXIDivider;            /*!< The AXI divider. This clock is derived from the AXISSRC clock.
                                     This parameter can be a value of @ref RCC_LL_EC_AXI_DIV

                                     This feature can be modified afterwards using unitary function
                                     @ref LL_RCC_SetACLKPrescaler(). */

  uint32_t MCUDivider;            /*!< The MCU divider. This clock is derived from the MCUSSRC muxer.
                                     This parameter can be a value of @ref RCC_LL_EC_MCU_DIV

                                     This feature can be modified afterwards using unitary function
                                     @ref LL_RCC_SetMLHCLKPrescaler(). */

  uint32_t APB1CLKDivider;        /*!< The APB1 clock (PCLK1) divider. This clock is derived from the MCU divider.
                                       This parameter can be a value of @ref RCC_LL_EC_APB1_DIV

                                       This feature can be modified afterwards using unitary function
                                       @ref LL_RCC_SetAPB1Prescaler(). */

  uint32_t APB2CLKDivider;        /*!< The APB2 clock (PCLK2) divider. This clock is derived from the MCU divider.
                                       This parameter can be a value of @ref RCC_LL_EC_APB2_DIV

                                       This feature can be modified afterwards using unitary function
                                       @ref LL_RCC_SetAPB2Prescaler(). */

  uint32_t APB3CLKDivider;        /*!< The APB2 clock (PCLK3) divider. This clock is derived from the MCU divider.
                                       This parameter can be a value of @ref RCC_LL_EC_APB3_DIV

                                       This feature can be modified afterwards using unitary function
                                       @ref LL_RCC_SetAPB3Prescaler(). */

  uint32_t APB4CLKDivider;        /*!< The APB4 clock (PCLK4) divider. This clock is derived from the AXIDIV divider.
                                       This parameter can be a value of @ref RCC_LL_EC_APB4_DIV

                                       This feature can be modified afterwards using unitary function
                                       @ref LL_RCC_SetAPB4Prescaler(). */

  uint32_t APB5CLKDivider;        /*!< The APB5 clock (PCLK5) divider. This clock is derived from the AXIDIV divider.
                                       This parameter can be a value of @ref RCC_LL_EC_APB5_DIV

                                       This feature can be modified afterwards using unitary function
                                       @ref LL_RCC_SetAPB5Prescaler(). */
} LL_UTILS_ClkInitTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup UTILS_LL_Exported_Constants UTILS Exported Constants
  * @{
  */

/** @defgroup UTILS_EC_HSE_BYPASS HSE Bypass activation
  * @{
  */
#define LL_UTILS_HSEBYPASS_OFF    0x00000000U         /*!< HSE Bypass is disabled */
#define LL_UTILS_HSEBYPASS_ON     RCC_OCENSETR_HSEBYP /*!< HSE Bypass is enabled */
#define LL_UTILS_HSEBYPASSDIG_ON  RCC_OCENSETR_DIGBYP /*!< HSE Bypass Digital is enabled */

/**
  * @}
  */

/** @defgroup UTILS_EC_PACKAGETYPE PACKAGE TYPE
  * @{
  */
#define LL_UTILS_PACKAGETYPE_TFBGA257         1U      /*!< TFBGA257 package type */
#define LL_UTILS_PACKAGETYPE_TFBGA361         2U      /*!< TFBGA361 package type */
#define LL_UTILS_PACKAGETYPE_LFBGA354         3U      /*!< LFBGA354 package type */
#define LL_UTILS_PACKAGETYPE_LFBGA448         4U      /*!< LFBGA448 package type */

/**
  * @}
  */

/** @defgroup UTILS_EC_RPN DEVICE PART NUMBER
  * @{
  */
#define LL_UTILS_RPN_STM32MP157Cxx            0U      /*!< STM32MP157Cxx Part Number */
#define LL_UTILS_RPN_STM32MP157Axx            1U      /*!< STM32MP157Axx Part Number */

/**
  * @}
  */

/** @defgroup UTILS_EC_DV DEVICE ID VERSION
  * @{
  */
#define LL_UTILS_DV_ID_STM32MP15xxx           0x500U  /*!< STM32MP15xxx Device ID */

/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @defgroup UTILS_LL_Exported_Functions UTILS Exported Functions
  * @{
  */

/** @defgroup UTILS_EF_DEVICE_ELECTRONIC_SIGNATURE DEVICE ELECTRONIC SIGNATURE
  * @{
  */

/**
  * @brief  Get Word0 of the unique device identifier (UID based on 96 bits)
  * @retval UID[31:0]
  */
__STATIC_INLINE uint32_t LL_GetUID_Word0(void)
{
  return (uint32_t)(READ_REG(*((uint32_t *)UID_BASE_ADDRESS)));
}

/**
  * @brief  Get Word1 of the unique device identifier (UID based on 96 bits)
  * @retval UID[63:32]
  */
__STATIC_INLINE uint32_t LL_GetUID_Word1(void)
{
  return (uint32_t)(READ_REG(*((uint32_t *)(UID_BASE_ADDRESS + 4U))));
}

/**
  * @brief  Get Word2 of the unique device identifier (UID based on 96 bits)
  * @retval UID[95:64]
  */
__STATIC_INLINE uint32_t LL_GetUID_Word2(void)
{
  return (uint32_t)(READ_REG(*((uint32_t *)(UID_BASE_ADDRESS + 8U))));
}

/**
  * @brief  Get Package type
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_UTILS_PACKAGETYPE_TFBGA257
  *         @arg @ref LL_UTILS_PACKAGETYPE_TFBGA361
  *         @arg @ref LL_UTILS_PACKAGETYPE_LFBGA354
  *         @arg @ref LL_UTILS_PACKAGETYPE_LFBGA448
  */
__STATIC_INLINE uint32_t LL_GetPackageType(void)
{
  return (uint32_t)(READ_BIT(*(uint32_t *)PACKAGE_BASE_ADDRESS, PKG_ID) >> PKG_ID_Pos);
}

/**
  * @brief  Get Device Part Number
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_UTILS_RPN_STM32MP157Cxx
  *         @arg @ref LL_UTILS_RPN_STM32MP157Axx
  */
__STATIC_INLINE uint32_t LL_GetDevicePartNumber(void)
{
  return (uint32_t)(READ_BIT(*(uint32_t *)RPN_BASE, RPN_ID) >> RPN_ID_Pos);
}

/**
  * @brief  Get Device Version ID
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_UTILS_DV_ID_STM32MP15xxx
  */
__STATIC_INLINE uint32_t LL_GetDeviceVersionDevID(void)
{
  return (uint32_t)(READ_BIT(*(uint32_t *)DV_BASE, DV_DEV_ID) >> DV_DEV_ID_Pos);
}

/**
  * @brief  Get Device Version Rev ID
  * @retval Returned value is Silicon version
  */
__STATIC_INLINE uint32_t LL_GetDeviceVersionRevID(void)
{
  return (uint32_t)(READ_BIT(*(uint32_t *)DV_BASE, DV_REV_ID) >> DV_REV_ID_Pos);
}

/**
  * @}
  */

/** @defgroup UTILS_LL_EF_DELAY DELAY
  * @{
  */
#if defined(CORE_CM4)
/**
  * @brief  This function configures the Cortex-M SysTick source of the time base.
  * @param  CPU_Frequency Core frequency in Hz. It can be calculated thanks to RCC
  *         helper macro or function @ref LL_RCC_GetSystemClocksFreq
  *         - Use  MCU_Frequency structure element returned by function above
  * @note   When a RTOS is used, it is recommended to avoid changing the SysTick
  *         configuration by calling this function, for a delay use rather osDelay RTOS service.
  * @param  Ticks Number of ticks
  * @retval None
  */
__STATIC_INLINE void LL_InitTick(uint32_t CPU_Frequency, uint32_t Ticks)
{
  /* Configure the SysTick to have interrupt in 1ms time base */
  SysTick->LOAD  = (uint32_t)((CPU_Frequency / Ticks) - 1UL);  /* set reload register */
  SysTick->VAL   = 0UL;                                       /* Load the SysTick Counter Value */
  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                   SysTick_CTRL_ENABLE_Msk;                   /* Enable the Systick Timer */
}

void        LL_Init1msTick(uint32_t CPU_Frequency);
void        LL_mDelay(uint32_t Delay);
#endif /* CORE_CM4 */

/**
  * @}
  */

/** @defgroup UTILS_EF_SYSTEM SYSTEM
  * @{
  */

void        LL_SetSystemCoreClock(uint32_t CPU_Frequency);
ErrorStatus LL_PLL_ConfigSystemClock_HSE(uint32_t HSEFrequency,
                                         uint32_t HSEBypass,
                                         LL_UTILS_PLLsInitTypeDef *UTILS_PLLInitStruct,
                                         LL_UTILS_ClkInitTypeDef *UTILS_ClkInitStruct);

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

#endif /* STM32MP1xx_LL_UTILS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
