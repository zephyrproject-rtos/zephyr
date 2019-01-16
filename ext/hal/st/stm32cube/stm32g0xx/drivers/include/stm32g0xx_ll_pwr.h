/**
  ******************************************************************************
  * @file    stm32g0xx_ll_pwr.h
  * @author  MCD Application Team
  * @brief   Header file of PWR LL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics. 
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
#ifndef STM32G0xx_LL_PWR_H
#define STM32G0xx_LL_PWR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx.h"

/** @addtogroup STM32G0xx_LL_Driver
  * @{
  */

#if defined(PWR)

/** @defgroup PWR_LL PWR
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup PWR_LL_Exported_Constants PWR Exported Constants
  * @{
  */

/** @defgroup PWR_LL_EC_CLEAR_FLAG Clear Flags Defines
  * @brief    Flags defines which can be used with LL_PWR_WriteReg function
  * @{
  */
#define LL_PWR_SCR_CSBF                    PWR_SCR_CSBF
#define LL_PWR_SCR_CWUF                    PWR_SCR_CWUF
#define LL_PWR_SCR_CWUF6                   PWR_SCR_CWUF6
#define LL_PWR_SCR_CWUF5                   PWR_SCR_CWUF5
#define LL_PWR_SCR_CWUF4                   PWR_SCR_CWUF4
#define LL_PWR_SCR_CWUF2                   PWR_SCR_CWUF2
#define LL_PWR_SCR_CWUF1                   PWR_SCR_CWUF1
/**
  * @}
  */

/** @defgroup PWR_LL_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_PWR_ReadReg function
  * @{
  */
#define LL_PWR_SR1_WUFI                    PWR_SR1_WUFI
#define LL_PWR_SR1_SBF                     PWR_SR1_SBF
#define LL_PWR_SR1_WUF6                    PWR_SR1_WUF6
#define LL_PWR_SR1_WUF5                    PWR_SR1_WUF5
#define LL_PWR_SR1_WUF4                    PWR_SR1_WUF4
#define LL_PWR_SR1_WUF2                    PWR_SR1_WUF2
#define LL_PWR_SR1_WUF1                    PWR_SR1_WUF1
#if defined(PWR_SR2_PVDO)
#define LL_PWR_SR2_PVDO                    PWR_SR2_PVDO
#endif
#define LL_PWR_SR2_VOSF                    PWR_SR2_VOSF
#define LL_PWR_SR2_REGLPF                  PWR_SR2_REGLPF
#define LL_PWR_SR2_REGLPS                  PWR_SR2_REGLPS
/**
  * @}
  */

/** @defgroup PWR_LL_EC_REGU_VOLTAGE REGU VOLTAGE
  * @{
  */
#define LL_PWR_REGU_VOLTAGE_SCALE1         PWR_CR1_VOS_0
#define LL_PWR_REGU_VOLTAGE_SCALE2         PWR_CR1_VOS_1
/**
  * @}
  */

/** @defgroup PWR_LL_EC_MODE_PWR MODE PWR
  * @{
  */
#define LL_PWR_MODE_STOP0                  (PWR_CR1_LPMS_0)
#define LL_PWR_MODE_STOP1                  (PWR_CR1_LPMS_1)
#define LL_PWR_MODE_STANDBY                (PWR_CR1_LPMS_1|PWR_CR1_LPMS_0)
#if defined (PWR_CR1_LPMS_2)
#define LL_PWR_MODE_SHUTDOWN               (PWR_CR1_LPMS_2)
#endif
/**
  * @}
  */

#if defined(PWR_CR2_PVDE)
/** @defgroup PWR_LL_EC_PVDLEVEL PVDLEVEL
  * @{
  */
#define LL_PWR_PVDLLEVEL_0                  0x000000000u                                /* VPVD0 > 2.05 V */
#define LL_PWR_PVDLLEVEL_1                  (PWR_CR2_PVDFT_0)                           /* VPVD0 > 2.2 V */
#define LL_PWR_PVDLLEVEL_2                  (PWR_CR2_PVDFT_1)                           /* VPVD1 > 2.36 V */
#define LL_PWR_PVDLLEVEL_3                  (PWR_CR2_PVDFT_1 | PWR_CR2_PVDFT_0)         /* VPVD2 > 2.52 V */
#define LL_PWR_PVDLLEVEL_4                  (PWR_CR2_PVDFT_2)                           /* VPVD3 > 2.64 V */
#define LL_PWR_PVDLLEVEL_5                  (PWR_CR2_PVDFT_2 | PWR_CR2_PVDFT_0)         /* VPVD4 > 2.81 V */
#define LL_PWR_PVDLLEVEL_6                  (PWR_CR2_PVDFT_2 | PWR_CR2_PVDFT_1)         /* VPVD5 > 2.91 V */

#define LL_PWR_PVDHLEVEL_0                  0x00000000u                                 /* VPDD0 > 2.15 V */
#define LL_PWR_PVDHLEVEL_1                  (PWR_CR2_PVDRT_0)                           /* VPVD1 > 2.3 V */
#define LL_PWR_PVDHLEVEL_2                  (PWR_CR2_PVDRT_1)                           /* VPVD1 > 2.46 V */
#define LL_PWR_PVDHLEVEL_3                  (PWR_CR2_PVDRT_1 | PWR_CR2_PVDRT_0)         /* VPVD2 > 2.62 V */
#define LL_PWR_PVDHLEVEL_4                  (PWR_CR2_PVDRT_2)                           /* VPVD3 > 2.74 V */
#define LL_PWR_PVDHLEVEL_5                  (PWR_CR2_PVDRT_2 | PWR_CR2_PVDRT_0)         /* VPVD4 > 2.91 V */
#define LL_PWR_PVDHLEVEL_6                  (PWR_CR2_PVDRT_2 | PWR_CR2_PVDRT_1)         /* VPVD5 > 3.01 V */
#define LL_PWR_PVDHLEVEL_7                  (PWR_CR2_PVDRT_2 | PWR_CR2_PVDRT_1 | PWR_CR2_PVDRT_0)  /* External input analog voltage   (Compare internally to VREFINT) */
/**
  * @}
  */
#endif

/** @defgroup PWR_LL_EC_WAKEUP WAKEUP
  * @{
  */
#define LL_PWR_WAKEUP_PIN1                 (PWR_CR3_EWUP1)
#define LL_PWR_WAKEUP_PIN2                 (PWR_CR3_EWUP2)
#define LL_PWR_WAKEUP_PIN4                 (PWR_CR3_EWUP4)
#if defined(PWR_CR3_EWUP5)
#define LL_PWR_WAKEUP_PIN5                 (PWR_CR3_EWUP5)
#endif
#define LL_PWR_WAKEUP_PIN6                 (PWR_CR3_EWUP6)
/**
  * @}
  */

/** @defgroup PWR_LL_EC_BATT_CHARG_RESISTOR BATT CHARG RESISTOR
  * @{
  */
#define LL_PWR_BATTCHARG_RESISTOR_5K       0x000000000u
#define LL_PWR_BATTCHARG_RESISTOR_1_5K     (PWR_CR4_VBRS)
/**
  * @}
  */

/** @defgroup PWR_LL_EC_GPIO GPIO
  * @{
  */
#define LL_PWR_GPIO_A                      ((uint32_t)(&(PWR->PUCRA)))
#define LL_PWR_GPIO_B                      ((uint32_t)(&(PWR->PUCRB)))
#define LL_PWR_GPIO_C                      ((uint32_t)(&(PWR->PUCRC)))
#define LL_PWR_GPIO_D                      ((uint32_t)(&(PWR->PUCRD)))
#define LL_PWR_GPIO_F                      ((uint32_t)(&(PWR->PUCRF)))
/**
  * @}
  */

/** @defgroup PWR_LL_EC_GPIO_BIT GPIO BIT
  * @{
  */
#define LL_PWR_GPIO_BIT_0                  0x00000001u
#define LL_PWR_GPIO_BIT_1                  0x00000002u
#define LL_PWR_GPIO_BIT_2                  0x00000004u
#define LL_PWR_GPIO_BIT_3                  0x00000008u
#define LL_PWR_GPIO_BIT_4                  0x00000010u
#define LL_PWR_GPIO_BIT_5                  0x00000020u
#define LL_PWR_GPIO_BIT_6                  0x00000040u
#define LL_PWR_GPIO_BIT_7                  0x00000080u
#define LL_PWR_GPIO_BIT_8                  0x00000100u
#define LL_PWR_GPIO_BIT_9                  0x00000200u
#define LL_PWR_GPIO_BIT_10                 0x00000400u
#define LL_PWR_GPIO_BIT_11                 0x00000800u
#define LL_PWR_GPIO_BIT_12                 0x00001000u
#define LL_PWR_GPIO_BIT_13                 0x00002000u
#define LL_PWR_GPIO_BIT_14                 0x00004000u
#define LL_PWR_GPIO_BIT_15                 0x00008000u
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup PWR_LL_Exported_Macros PWR Exported Macros
  * @{
  */

/** @defgroup PWR_LL_EM_WRITE_READ Common Write and read registers Macros
  * @{
  */

/**
  * @brief  Write a value in PWR register
  * @param  __REG__ Register to be written
  * @param  __VALUE__ Value to be written in the register
  * @retval None
  */
#define LL_PWR_WriteReg(__REG__, __VALUE__) WRITE_REG(PWR->__REG__, (__VALUE__))

/**
  * @brief  Read a value in PWR register
  * @param  __REG__ Register to be read
  * @retval Register value
  */
#define LL_PWR_ReadReg(__REG__) READ_REG(PWR->__REG__)
/**
  * @}
  */

/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/
/** @defgroup PWR_LL_Exported_Functions PWR Exported Functions
  * @{
  */

/** @defgroup PWR_LL_EF_Configuration Configuration
  * @{
  */
/**
  * @brief  Set the main internal regulator output voltage
  * @rmtoll CR1          VOS           LL_PWR_SetRegulVoltageScaling
  * @param  VoltageScaling This parameter can be one of the following values:
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE1
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE2
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetRegulVoltageScaling(uint32_t VoltageScaling)
{
  MODIFY_REG(PWR->CR1, PWR_CR1_VOS, VoltageScaling);
}

/**
  * @brief  Get the main internal regulator output voltage
  * @rmtoll CR1          VOS           LL_PWR_GetRegulVoltageScaling
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE1
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE2
  */
__STATIC_INLINE uint32_t LL_PWR_GetRegulVoltageScaling(void)
{
  return (READ_BIT(PWR->CR1, PWR_CR1_VOS));
}

/**
  * @brief  Switch the regulator from main mode to low-power mode
  * @rmtoll CR1          LPR           LL_PWR_EnableLowPowerRunMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableLowPowerRunMode(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_LPR);
}

/**
  * @brief  Switch the regulator from low-power mode to main mode
  * @rmtoll CR1          LPR           LL_PWR_DisableLowPowerRunMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableLowPowerRunMode(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_LPR);
}

/**
  * @brief  Check if the regulator is in low-power mode
  * @rmtoll CR1          LPR           LL_PWR_IsEnabledLowPowerRunMode
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledLowPowerRunMode(void)
{
  return ((READ_BIT(PWR->CR1, PWR_CR1_LPR) == (PWR_CR1_LPR)) ? 1UL : 0UL);
}

/**
  * @brief  Switch from run main mode to run low-power mode.
  * @rmtoll CR1          LPR           LL_PWR_EnterLowPowerRunMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnterLowPowerRunMode(void)
{
  LL_PWR_EnableLowPowerRunMode();
}

/**
  * @brief  Switch from run main mode to low-power mode.
  * @rmtoll CR1          LPR           LL_PWR_ExitLowPowerRunMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ExitLowPowerRunMode(void)
{
  LL_PWR_DisableLowPowerRunMode();
}

/**
  * @brief  Enable access to the backup domain
  * @rmtoll CR1          DBP           LL_PWR_EnableBkUpAccess
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableBkUpAccess(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_DBP);
}

/**
  * @brief  Disable access to the backup domain
  * @rmtoll CR1          DBP           LL_PWR_DisableBkUpAccess
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableBkUpAccess(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_DBP);
}

/**
  * @brief  Check if the backup domain is enabled
  * @rmtoll CR1          DBP           LL_PWR_IsEnabledBkUpAccess
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledBkUpAccess(void)
{
  return ((READ_BIT(PWR->CR1, PWR_CR1_DBP) == (PWR_CR1_DBP)) ? 1UL : 0UL);
}

/**
  * @brief  Enable Flash Power-down mode during low power sleep mode
  * @rmtoll CR1          CFIPD_SLP     LL_PWR_EnableFlashPowerDownInLPSleep
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableFlashPowerDownInLPSleep(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_FPD_LPSLP);
}

/**
  * @brief  Disable Flash Power-down mode during Low power sleep mode
  * @rmtoll CR1          CFIPD_SLP     LL_PWR_DisableFlashPowerDownInLPSleep
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableFlashPowerDownInLPSleep(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_FPD_LPSLP);
}

/**
  * @brief  Check if flash power-down mode during low power sleep mode domain is enabled
  * @rmtoll CR1          CFIPD_SLP     LL_PWR_IsEnableFlashPowerDownInLPSleep
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnableFlashPowerDownInLPSleep(void)
{
  return ((READ_BIT(PWR->CR1, PWR_CR1_FPD_LPSLP) == (PWR_CR1_FPD_LPSLP)) ? 1UL : 0UL);
}

/**
  * @brief  Enable Flash Power-down mode during low power run mode
  * @rmtoll CR1          CFIPD_RUN     LL_PWR_EnableFlashPowerDownInLPRun
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableFlashPowerDownInLPRun(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_FPD_LPRUN);
}

/**
  * @brief  Disable Flash Power-down mode during Low power run mode
  * @rmtoll CR1          CFIPD_RUN     LL_PWR_DisableFlashPowerDownInLPRun
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableFlashPowerDownInLPRun(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_FPD_LPRUN);
}

/**
  * @brief  Check if flash power-down mode during low power run mode domain is enabled
  * @rmtoll CR1          CFIPD_RUN     LL_PWR_IsEnableFlashPowerDownInLPRun
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnableFlashPowerDownInLPRun(void)
{
  return ((READ_BIT(PWR->CR1, PWR_CR1_FPD_LPRUN) == (PWR_CR1_FPD_LPRUN)) ? 1UL : 0UL);
}

/**
  * @brief  Enable Flash Power-down mode during stop mode
  * @rmtoll CR1          CFIPD_STOP    LL_PWR_EnableFlashPowerDownInStop
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableFlashPowerDownInStop(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_FPD_STOP);
}

/**
  * @brief  Disable Flash Power-down mode during stop mode
  * @rmtoll CR1          CFIPD_STOP    LL_PWR_DisableFlashPowerDownInStop
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableFlashPowerDownInStop(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_FPD_STOP);
}

/**
  * @brief  Check if flash power-down mode during stop mode domain is enabled
  * @rmtoll CR1          CFIPD_STOP    LL_PWR_IsEnableFlashPowerDownInStop
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnableFlashPowerDownInStop(void)
{
  return ((READ_BIT(PWR->CR1, PWR_CR1_FPD_STOP) == (PWR_CR1_FPD_STOP)) ? 1UL : 0UL);
}

/**
  * @brief  Set Low-Power mode
  * @rmtoll CR1          LPMS          LL_PWR_SetPowerMode
  * @param  LowPowerMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_MODE_STOP0
  *         @arg @ref LL_PWR_MODE_STOP1
  *         @arg @ref LL_PWR_MODE_STANDBY
  *         @arg @ref LL_PWR_MODE_SHUTDOWN
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetPowerMode(uint32_t LowPowerMode)
{
  MODIFY_REG(PWR->CR1, PWR_CR1_LPMS, LowPowerMode);
}

/**
  * @brief  Get Low-Power mode
  * @rmtoll CR1          LPMS          LL_PWR_GetPowerMode
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_MODE_STOP0
  *         @arg @ref LL_PWR_MODE_STOP1
  *         @arg @ref LL_PWR_MODE_STANDBY
  *         @arg @ref LL_PWR_MODE_SHUTDOWN
  */
__STATIC_INLINE uint32_t LL_PWR_GetPowerMode(void)
{
  return (uint32_t)(READ_BIT(PWR->CR1, PWR_CR1_LPMS));
}

#if defined (PWR_CR2_PVDE)
/**
  * @brief  Configure the high voltage threshold detected by the Power Voltage Detector
  * @rmtoll CR2          PLS           LL_PWR_SetPVDHighLevel
  * @param  PVDHighLevel This parameter can be one of the following values:
  *         @arg @ref LL_PWR_PVDHLEVEL_0
  *         @arg @ref LL_PWR_PVDHLEVEL_1
  *         @arg @ref LL_PWR_PVDHLEVEL_2
  *         @arg @ref LL_PWR_PVDHLEVEL_3
  *         @arg @ref LL_PWR_PVDHLEVEL_4
  *         @arg @ref LL_PWR_PVDHLEVEL_5
  *         @arg @ref LL_PWR_PVDHLEVEL_6
  *         @arg @ref LL_PWR_PVDHLEVEL_7
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetPVDHighLevel(uint32_t PVDHighLevel)
{
  MODIFY_REG(PWR->CR2, PWR_CR2_PVDRT, PVDHighLevel);
}

/**
  * @brief  Get the voltage threshold detection
  * @rmtoll CR2          PLS           LL_PWR_GetPVDHighLevel
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_PVDHLEVEL_0
  *         @arg @ref LL_PWR_PVDHLEVEL_1
  *         @arg @ref LL_PWR_PVDHLEVEL_2
  *         @arg @ref LL_PWR_PVDHLEVEL_3
  *         @arg @ref LL_PWR_PVDHLEVEL_4
  *         @arg @ref LL_PWR_PVDHLEVEL_5
  *         @arg @ref LL_PWR_PVDHLEVEL_6
  *         @arg @ref LL_PWR_PVDHLEVEL_7
  */
__STATIC_INLINE uint32_t LL_PWR_GetPVDHighLevel(void)
{
  return (uint32_t)(READ_BIT(PWR->CR2, PWR_CR2_PVDRT));
}
/**
  * @brief  Configure the low voltage threshold detected by the Power Voltage Detector
  * @rmtoll CR2          PLS           LL_PWR_SetPVDLowLevel
  * @param  PVDLowLevel This parameter can be one of the following values:
  *         @arg @ref LL_PWR_PVDLLEVEL_0
  *         @arg @ref LL_PWR_PVDLLEVEL_1
  *         @arg @ref LL_PWR_PVDLLEVEL_2
  *         @arg @ref LL_PWR_PVDLLEVEL_3
  *         @arg @ref LL_PWR_PVDLLEVEL_4
  *         @arg @ref LL_PWR_PVDLLEVEL_5
  *         @arg @ref LL_PWR_PVDLLEVEL_6
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetPVDLowLevel(uint32_t PVDLowLevel)
{
  MODIFY_REG(PWR->CR2, PWR_CR2_PVDFT, PVDLowLevel);
}

/**
  * @brief  Get the low voltage threshold detection
  * @rmtoll CR2          PLS           LL_PWR_GetPVDLowLevel
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_PVDLLEVEL_0
  *         @arg @ref LL_PWR_PVDLLEVEL_1
  *         @arg @ref LL_PWR_PVDLLEVEL_2
  *         @arg @ref LL_PWR_PVDLLEVEL_3
  *         @arg @ref LL_PWR_PVDLLEVEL_4
  *         @arg @ref LL_PWR_PVDLLEVEL_5
  *         @arg @ref LL_PWR_PVDLLEVEL_6
  */
__STATIC_INLINE uint32_t LL_PWR_GetPVDLowLevel(void)
{
  return (uint32_t)(READ_BIT(PWR->CR2, PWR_CR2_PVDFT));
}

/**
  * @brief  Enable Power Voltage Detector
  * @rmtoll CR2          PVDE          LL_PWR_EnablePVD
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnablePVD(void)
{
  SET_BIT(PWR->CR2, PWR_CR2_PVDE);
}

/**
  * @brief  Disable Power Voltage Detector
  * @rmtoll CR2          PVDE          LL_PWR_DisablePVD
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisablePVD(void)
{
  CLEAR_BIT(PWR->CR2, PWR_CR2_PVDE);
}

/**
  * @brief  Check if Power Voltage Detector is enabled
  * @rmtoll CR2          PVDE          LL_PWR_IsEnabledPVD
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledPVD(void)
{
  return ((READ_BIT(PWR->CR2, PWR_CR2_PVDE) == (PWR_CR2_PVDE)) ? 1UL : 0UL);
}
#endif

/**
  * @brief  Enable Internal Wake-up line
  * @rmtoll CR3          EIWF          LL_PWR_EnableInternWU
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableInternWU(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_EIWUL);
}

/**
  * @brief  Disable Internal Wake-up line
  * @rmtoll CR3          EIWF          LL_PWR_DisableInternWU
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableInternWU(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_EIWUL);
}

/**
  * @brief  Check if Internal Wake-up line is enabled
  * @rmtoll CR3          EIWF          LL_PWR_IsEnabledInternWU
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledInternWU(void)
{
  return ((READ_BIT(PWR->CR3, PWR_CR3_EIWUL) == (PWR_CR3_EIWUL)) ? 1UL : 0UL);
}

/**
  * @brief  Enable pull-up and pull-down configuration
  * @rmtoll CR3          APC           LL_PWR_EnablePUPDCfg
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnablePUPDCfg(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_APC);
}

/**
  * @brief  Disable pull-up and pull-down configuration
  * @rmtoll CR3          APC           LL_PWR_DisablePUPDCfg
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisablePUPDCfg(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_APC);
}

/**
  * @brief  Check if pull-up and pull-down configuration  is enabled
  * @rmtoll CR3          APC           LL_PWR_IsEnabledPUPDCfg
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledPUPDCfg(void)
{
  return ((READ_BIT(PWR->CR3, PWR_CR3_APC) == (PWR_CR3_APC)) ? 1UL : 0UL);
}

#if defined(PWR_CR3_RRS)
/**
  * @brief  Enable SRAM content retention in Standby mode
  * @rmtoll CR3          RRS           LL_PWR_EnableSRAMRetention
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableSRAMRetention(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_RRS);
}

/**
  * @brief  Disable SRAM content retention in Standby mode
  * @rmtoll CR3          RRS           LL_PWR_DisableSRAMRetention
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableSRAMRetention(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_RRS);
}

/**
  * @brief  Check if SRAM content retention in Standby mode is enabled
  * @rmtoll CR3          RRS           LL_PWR_IsEnabledSRAMRetention
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledSRAMRetention(void)
{
  return ((READ_BIT(PWR->CR3, PWR_CR3_RRS) == (PWR_CR3_RRS)) ? 1UL : 0UL);
}
#endif

#if defined(PWR_CR3_ENB_ULP)
/**
  * @brief  Enable sampling mode of LPMMU reset block
  * @rmtoll CR3          ENB_ULP       LL_PWR_EnableLPMUResetSamplingMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableLPMUResetSamplingMode(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_ENB_ULP);
}

/**
  * @brief  Disable sampling mode of LPMMU reset block
  * @rmtoll CR3          ENB_ULP       LL_PWR_DisableLPMUResetSamplingMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableLPMUResetSamplingMode(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_ENB_ULP);
}

/**
  * @brief  Check if sampling mode of LPMMU reset block
  * @rmtoll CR3          ENB_ULP       LL_PWR_IsEnableLPMUResetSamplingMode
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnableLPMUResetSamplingMode(void)
{
  return ((READ_BIT(PWR->CR3, PWR_CR3_ENB_ULP) == (PWR_CR3_ENB_ULP)) ? 1UL : 0UL);
}
#endif

/**
  * @brief  Enable the WakeUp PINx functionality
  * @rmtoll CR3          EWUP1         LL_PWR_EnableWakeUpPin\n
  *         CR3          EWUP2         LL_PWR_EnableWakeUpPin\n
  *         CR3          EWUP4         LL_PWR_EnableWakeUpPin\n
  *         CR3          EWUP5         LL_PWR_EnableWakeUpPin\n
  *         CR3          EWUP6         LL_PWR_EnableWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableWakeUpPin(uint32_t WakeUpPin)
{
  SET_BIT(PWR->CR3, WakeUpPin);
}

/**
  * @brief  Disable the WakeUp PINx functionality
  * @rmtoll CR3          EWUP1         LL_PWR_DisableWakeUpPin\n
  *         CR3          EWUP2         LL_PWR_DisableWakeUpPin\n
  *         CR3          EWUP4         LL_PWR_DisableWakeUpPin\n
  *         CR3          EWUP5         LL_PWR_DisableWakeUpPin\n
  *         CR3          EWUP6         LL_PWR_DisableWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableWakeUpPin(uint32_t WakeUpPin)
{
  CLEAR_BIT(PWR->CR3, WakeUpPin);
}

/**
  * @brief  Check if the WakeUp PINx functionality is enabled
  * @rmtoll CR3          EWUP1         LL_PWR_IsEnabledWakeUpPin\n
  *         CR3          EWUP2         LL_PWR_IsEnabledWakeUpPin\n
  *         CR3          EWUP4         LL_PWR_IsEnabledWakeUpPin\n
  *         CR3          EWUP5         LL_PWR_IsEnabledWakeUpPin\n
  *         CR3          EWUP6         LL_PWR_IsEnabledWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledWakeUpPin(uint32_t WakeUpPin)
{
  return ((READ_BIT(PWR->CR3, WakeUpPin) == (WakeUpPin)) ? 1UL : 0UL);
}

/**
  * @brief  Set the resistor impedance
  * @rmtoll CR4          VBRS          LL_PWR_SetBattChargResistor
  * @param  Resistor This parameter can be one of the following values:
  *         @arg @ref LL_PWR_BATTCHARG_RESISTOR_5K
  *         @arg @ref LL_PWR_BATTCHARG_RESISTOR_1_5K
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetBattChargResistor(uint32_t Resistor)
{
  MODIFY_REG(PWR->CR4, PWR_CR4_VBRS, Resistor);
}

/**
  * @brief  Get the resistor impedance
  * @rmtoll CR4          VBRS          LL_PWR_GetBattChargResistor
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_BATTCHARG_RESISTOR_5K
  *         @arg @ref LL_PWR_BATTCHARG_RESISTOR_1_5K
  */
__STATIC_INLINE uint32_t LL_PWR_GetBattChargResistor(void)
{
  return (uint32_t)(READ_BIT(PWR->CR4, PWR_CR4_VBRS));
}

/**
  * @brief  Enable battery charging
  * @rmtoll CR4          VBE           LL_PWR_EnableBatteryCharging
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableBatteryCharging(void)
{
  SET_BIT(PWR->CR4, PWR_CR4_VBE);
}

/**
  * @brief  Disable battery charging
  * @rmtoll CR4          VBE           LL_PWR_DisableBatteryCharging
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableBatteryCharging(void)
{
  CLEAR_BIT(PWR->CR4, PWR_CR4_VBE);
}

/**
  * @brief  Check if battery charging is enabled
  * @rmtoll CR4          VBE           LL_PWR_IsEnabledBatteryCharging
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledBatteryCharging(void)
{
  return ((READ_BIT(PWR->CR4, PWR_CR4_VBE) == (PWR_CR4_VBE)) ? 1UL : 0UL);
}

/**
  * @brief  Set the Wake-Up pin polarity low for the event detection
  * @rmtoll CR4          WP1           LL_PWR_SetWakeUpPinPolarityLow\n
  *         CR4          WP2           LL_PWR_SetWakeUpPinPolarityLow\n
  *         CR4          WP4           LL_PWR_SetWakeUpPinPolarityLow\n
  *         CR4          WP5           LL_PWR_SetWakeUpPinPolarityLow\n
  *         CR4          WP6           LL_PWR_SetWakeUpPinPolarityLow\n
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetWakeUpPinPolarityLow(uint32_t WakeUpPin)
{
  SET_BIT(PWR->CR4, WakeUpPin);
}

/**
  * @brief  Set the Wake-Up pin polarity high for the event detection
  * @rmtoll CR4          WP1           LL_PWR_SetWakeUpPinPolarityHigh\n
  *         CR4          WP2           LL_PWR_SetWakeUpPinPolarityHigh\n
  *         CR4          WP4           LL_PWR_SetWakeUpPinPolarityHigh\n
  *         CR4          WP5           LL_PWR_SetWakeUpPinPolarityHigh\n
  *         CR4          WP6           LL_PWR_SetWakeUpPinPolarityHigh
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetWakeUpPinPolarityHigh(uint32_t WakeUpPin)
{
  CLEAR_BIT(PWR->CR4, WakeUpPin);
}

/**
  * @brief  Get the Wake-Up pin polarity for the event detection
  * @rmtoll CR4          WP1           LL_PWR_IsWakeUpPinPolarityLow\n
  *         CR4          WP2           LL_PWR_IsWakeUpPinPolarityLow\n
  *         CR4          WP4           LL_PWR_IsWakeUpPinPolarityLow\n
  *         CR4          WP5           LL_PWR_IsWakeUpPinPolarityLow\n
  *         CR4          WP6           LL_PWR_IsWakeUpPinPolarityLow
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsWakeUpPinPolarityLow(uint32_t WakeUpPin)
{
  return ((READ_BIT(PWR->CR4, WakeUpPin) == (WakeUpPin)) ? 1UL : 0UL);
}

/**
  * @brief  Enable GPIO pull-up state in Standby and Shutdown modes
  * @rmtoll PUCRA        PU0-15        LL_PWR_EnableGPIOPullUp\n
  *         PUCRB        PU0-15        LL_PWR_EnableGPIOPullUp\n
  *         PUCRC        PU0-15        LL_PWR_EnableGPIOPullUp\n
  *         PUCRD        PU0-15        LL_PWR_EnableGPIOPullUp\n
  *         PUCRF        PU0-15        LL_PWR_EnableGPIOPullUp
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_F
  * @param  GPIONumber This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_BIT_0
  *         @arg @ref LL_PWR_GPIO_BIT_1
  *         @arg @ref LL_PWR_GPIO_BIT_2
  *         @arg @ref LL_PWR_GPIO_BIT_3
  *         @arg @ref LL_PWR_GPIO_BIT_4
  *         @arg @ref LL_PWR_GPIO_BIT_5
  *         @arg @ref LL_PWR_GPIO_BIT_6
  *         @arg @ref LL_PWR_GPIO_BIT_7
  *         @arg @ref LL_PWR_GPIO_BIT_8
  *         @arg @ref LL_PWR_GPIO_BIT_9
  *         @arg @ref LL_PWR_GPIO_BIT_10
  *         @arg @ref LL_PWR_GPIO_BIT_11
  *         @arg @ref LL_PWR_GPIO_BIT_12
  *         @arg @ref LL_PWR_GPIO_BIT_13
  *         @arg @ref LL_PWR_GPIO_BIT_14
  *         @arg @ref LL_PWR_GPIO_BIT_15
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber)
{
  SET_BIT(*((uint32_t *)GPIO), GPIONumber);
}

/**
  * @brief  Disable GPIO pull-up state in Standby and Shutdown modes
  * @rmtoll PUCRA        PU0-15        LL_PWR_DisableGPIOPullUp\n
  *         PUCRB        PU0-15        LL_PWR_DisableGPIOPullUp\n
  *         PUCRC        PU0-15        LL_PWR_DisableGPIOPullUp\n
  *         PUCRD        PU0-15        LL_PWR_DisableGPIOPullUp\n
  *         PUCRF        PU0-15        LL_PWR_DisableGPIOPullUp
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_F
  * @param  GPIONumber This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_BIT_0
  *         @arg @ref LL_PWR_GPIO_BIT_1
  *         @arg @ref LL_PWR_GPIO_BIT_2
  *         @arg @ref LL_PWR_GPIO_BIT_3
  *         @arg @ref LL_PWR_GPIO_BIT_4
  *         @arg @ref LL_PWR_GPIO_BIT_5
  *         @arg @ref LL_PWR_GPIO_BIT_6
  *         @arg @ref LL_PWR_GPIO_BIT_7
  *         @arg @ref LL_PWR_GPIO_BIT_8
  *         @arg @ref LL_PWR_GPIO_BIT_9
  *         @arg @ref LL_PWR_GPIO_BIT_10
  *         @arg @ref LL_PWR_GPIO_BIT_11
  *         @arg @ref LL_PWR_GPIO_BIT_12
  *         @arg @ref LL_PWR_GPIO_BIT_13
  *         @arg @ref LL_PWR_GPIO_BIT_14
  *         @arg @ref LL_PWR_GPIO_BIT_15
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber)
{
  CLEAR_BIT(*((uint32_t *)GPIO), GPIONumber);
}

/**
  * @brief  Check if GPIO pull-up state is enabled
  * @rmtoll PUCRA        PU0-15        LL_PWR_IsEnabledGPIOPullUp\n
  *         PUCRB        PU0-15        LL_PWR_IsEnabledGPIOPullUp\n
  *         PUCRC        PU0-15        LL_PWR_IsEnabledGPIOPullUp\n
  *         PUCRD        PU0-15        LL_PWR_IsEnabledGPIOPullUp\n
  *         PUCRF        PU0-15        LL_PWR_IsEnabledGPIOPullUp
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_F
  * @param  GPIONumber This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_BIT_0
  *         @arg @ref LL_PWR_GPIO_BIT_1
  *         @arg @ref LL_PWR_GPIO_BIT_2
  *         @arg @ref LL_PWR_GPIO_BIT_3
  *         @arg @ref LL_PWR_GPIO_BIT_4
  *         @arg @ref LL_PWR_GPIO_BIT_5
  *         @arg @ref LL_PWR_GPIO_BIT_6
  *         @arg @ref LL_PWR_GPIO_BIT_7
  *         @arg @ref LL_PWR_GPIO_BIT_8
  *         @arg @ref LL_PWR_GPIO_BIT_9
  *         @arg @ref LL_PWR_GPIO_BIT_10
  *         @arg @ref LL_PWR_GPIO_BIT_11
  *         @arg @ref LL_PWR_GPIO_BIT_12
  *         @arg @ref LL_PWR_GPIO_BIT_13
  *         @arg @ref LL_PWR_GPIO_BIT_14
  *         @arg @ref LL_PWR_GPIO_BIT_15
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber)
{
  return ((READ_BIT(*((uint32_t *)(GPIO)), GPIONumber) == (GPIONumber)) ? 1UL : 0UL);
}

/**
  * @brief  Enable GPIO pull-down state in Standby and Shutdown modes
  * @rmtoll PDCRA        PD0-15        LL_PWR_EnableGPIOPullDown\n
  *         PDCRB        PD0-15        LL_PWR_EnableGPIOPullDown\n
  *         PDCRC        PD0-15        LL_PWR_EnableGPIOPullDown\n
  *         PDCRD        PD0-15        LL_PWR_EnableGPIOPullDown\n
  *         PDCRF        PD0-15        LL_PWR_EnableGPIOPullDown
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_F
  * @param  GPIONumber This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_BIT_0
  *         @arg @ref LL_PWR_GPIO_BIT_1
  *         @arg @ref LL_PWR_GPIO_BIT_2
  *         @arg @ref LL_PWR_GPIO_BIT_3
  *         @arg @ref LL_PWR_GPIO_BIT_4
  *         @arg @ref LL_PWR_GPIO_BIT_5
  *         @arg @ref LL_PWR_GPIO_BIT_6
  *         @arg @ref LL_PWR_GPIO_BIT_7
  *         @arg @ref LL_PWR_GPIO_BIT_8
  *         @arg @ref LL_PWR_GPIO_BIT_9
  *         @arg @ref LL_PWR_GPIO_BIT_10
  *         @arg @ref LL_PWR_GPIO_BIT_11
  *         @arg @ref LL_PWR_GPIO_BIT_12
  *         @arg @ref LL_PWR_GPIO_BIT_13
  *         @arg @ref LL_PWR_GPIO_BIT_14
  *         @arg @ref LL_PWR_GPIO_BIT_15
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber)
{
  register uint32_t temp = (uint32_t)(GPIO) + 4U;
  SET_BIT(*((uint32_t *)(temp)), GPIONumber);
}

/**
  * @brief  Disable GPIO pull-down state in Standby and Shutdown modes
  * @rmtoll PDCRA        PD0-15        LL_PWR_DisableGPIOPullDown\n
  *         PDCRB        PD0-15        LL_PWR_DisableGPIOPullDown\n
  *         PDCRC        PD0-15        LL_PWR_DisableGPIOPullDown\n
  *         PDCRD        PD0-15        LL_PWR_DisableGPIOPullDown\n
  *         PDCRF        PD0-15        LL_PWR_DisableGPIOPullDown
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_F
  * @param  GPIONumber This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_BIT_0
  *         @arg @ref LL_PWR_GPIO_BIT_1
  *         @arg @ref LL_PWR_GPIO_BIT_2
  *         @arg @ref LL_PWR_GPIO_BIT_3
  *         @arg @ref LL_PWR_GPIO_BIT_4
  *         @arg @ref LL_PWR_GPIO_BIT_5
  *         @arg @ref LL_PWR_GPIO_BIT_6
  *         @arg @ref LL_PWR_GPIO_BIT_7
  *         @arg @ref LL_PWR_GPIO_BIT_8
  *         @arg @ref LL_PWR_GPIO_BIT_9
  *         @arg @ref LL_PWR_GPIO_BIT_10
  *         @arg @ref LL_PWR_GPIO_BIT_11
  *         @arg @ref LL_PWR_GPIO_BIT_12
  *         @arg @ref LL_PWR_GPIO_BIT_13
  *         @arg @ref LL_PWR_GPIO_BIT_14
  *         @arg @ref LL_PWR_GPIO_BIT_15
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber)
{
  register uint32_t temp = (uint32_t)(GPIO) + 4U;
  CLEAR_BIT(*((uint32_t *)(temp)), GPIONumber);
}

/**
  * @brief  Check if GPIO pull-down state is enabled
  * @rmtoll PDCRA        PD0-15        LL_PWR_IsEnabledGPIOPullDown\n
  *         PDCRB        PD0-15        LL_PWR_IsEnabledGPIOPullDown\n
  *         PDCRC        PD0-15        LL_PWR_IsEnabledGPIOPullDown\n
  *         PDCRD        PD0-15        LL_PWR_IsEnabledGPIOPullDown\n
  *         PDCRF        PD0-15        LL_PWR_IsEnabledGPIOPullDown
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_F
  * @param  GPIONumber This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_BIT_0
  *         @arg @ref LL_PWR_GPIO_BIT_1
  *         @arg @ref LL_PWR_GPIO_BIT_2
  *         @arg @ref LL_PWR_GPIO_BIT_3
  *         @arg @ref LL_PWR_GPIO_BIT_4
  *         @arg @ref LL_PWR_GPIO_BIT_5
  *         @arg @ref LL_PWR_GPIO_BIT_6
  *         @arg @ref LL_PWR_GPIO_BIT_7
  *         @arg @ref LL_PWR_GPIO_BIT_8
  *         @arg @ref LL_PWR_GPIO_BIT_9
  *         @arg @ref LL_PWR_GPIO_BIT_10
  *         @arg @ref LL_PWR_GPIO_BIT_11
  *         @arg @ref LL_PWR_GPIO_BIT_12
  *         @arg @ref LL_PWR_GPIO_BIT_13
  *         @arg @ref LL_PWR_GPIO_BIT_14
  *         @arg @ref LL_PWR_GPIO_BIT_15
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber)
{
  register uint32_t temp = (uint32_t)(GPIO) + 4UL;
  return ((READ_BIT(*((uint32_t *)(temp)), GPIONumber) == (GPIONumber)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_FLAG_Management FLAG_Management
  * @{
  */

/**
  * @brief  Get Internal Wake-up line Flag
  * @rmtoll SR1          WUFI          LL_PWR_IsActiveFlag_InternWU
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_InternWU(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_WUFI) == (PWR_SR1_WUFI)) ? 1UL : 0UL);
}

/**
  * @brief  Get Stand-By Flag
  * @rmtoll SR1          SBF           LL_PWR_IsActiveFlag_SB
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_SB(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_SBF) == (PWR_SR1_SBF)) ? 1UL : 0UL);
}

/**
  * @brief  Get Wake-up Flag 6
  * @rmtoll SR1          WUF6          LL_PWR_IsActiveFlag_WU6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU6(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_WUF6) == (PWR_SR1_WUF6)) ? 1UL : 0UL);
}

#if defined(PWR_CR3_EWUP5)
/**
  * @brief  Get Wake-up Flag 5
  * @rmtoll SR1          WUF5          LL_PWR_IsActiveFlag_WU5
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU5(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_WUF5) == (PWR_SR1_WUF5)) ? 1UL : 0UL);
}
#endif

/**
  * @brief  Get Wake-up Flag 4
  * @rmtoll SR1          WUF4          LL_PWR_IsActiveFlag_WU4
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU4(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_WUF4) == (PWR_SR1_WUF4)) ? 1UL : 0UL);
}


/**
  * @brief  Get Wake-up Flag 2
  * @rmtoll SR1          WUF2          LL_PWR_IsActiveFlag_WU2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU2(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_WUF2) == (PWR_SR1_WUF2)) ? 1UL : 0UL);
}

/**
  * @brief  Get Wake-up Flag 1
  * @rmtoll SR1          WUF1          LL_PWR_IsActiveFlag_WU1
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU1(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_WUF1) == (PWR_SR1_WUF1)) ? 1UL : 0UL);
}

/**
  * @brief  Clear Stand-By Flag
  * @rmtoll SCR          CSBF          LL_PWR_ClearFlag_SB
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_SB(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CSBF);
}

/**
  * @brief  Clear Wake-up Flags
  * @rmtoll SCR          CWUF          LL_PWR_ClearFlag_WU
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CWUF);
}

/**
  * @brief  Clear Wake-up Flag 6
  * @rmtoll SCR          CWUF6         LL_PWR_ClearFlag_WU6
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU6(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CWUF6);
}

#if defined(PWR_CR3_EWUP5)
/**
  * @brief  Clear Wake-up Flag 5
  * @rmtoll SCR          CWUF5         LL_PWR_ClearFlag_WU5
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU5(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CWUF5);
}
#endif

/**
  * @brief  Clear Wake-up Flag 4
  * @rmtoll SCR          CWUF4         LL_PWR_ClearFlag_WU4
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU4(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CWUF4);
}


/**
  * @brief  Clear Wake-up Flag 2
  * @rmtoll SCR          CWUF2         LL_PWR_ClearFlag_WU2
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU2(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CWUF2);
}

/**
  * @brief  Clear Wake-up Flag 1
  * @rmtoll SCR          CWUF1         LL_PWR_ClearFlag_WU1
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU1(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CWUF1);
}

#if defined(PWR_SR2_PVDO)
/**
  * @brief  Indicate whether VDD voltage is below or above the selected PVD
  *         threshold
  * @rmtoll SR2          PVDO          LL_PWR_IsActiveFlag_PVDO
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_PVDO(void)
{
  return ((READ_BIT(PWR->SR2, PWR_SR2_PVDO) == (PWR_SR2_PVDO)) ? 1UL : 0UL);
}
#endif

/**
  * @brief  Indicate whether the regulator is ready in the selected voltage
  *         range or if its output voltage is still changing to the required
  *         voltage level
  * @note:  Take care, return value "0" means the regulator is ready.
  *         Return value "1" means the output voltage range is still changing.
  * @rmtoll SR2          VOSF          LL_PWR_IsActiveFlag_VOS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_VOS(void)
{
  return ((READ_BIT(PWR->SR2, PWR_SR2_VOSF) == (PWR_SR2_VOSF)) ? 1UL : 0UL);
}

/**
  * @brief  Indicate whether the regulator is ready in main mode or is in
  *         low-power mode
  * @note:  Take care, return value "0" means regulator is ready in main mode
  *         Return value "1" means regulator is in low-power mode (LPR)
  * @rmtoll SR2          REGLPF        LL_PWR_IsActiveFlag_REGLPF
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_REGLPF(void)
{
  return ((READ_BIT(PWR->SR2, PWR_SR2_REGLPF) == (PWR_SR2_REGLPF)) ? 1UL : 0UL);
}

/**
  * @brief  Indicate whether or not the low-power regulator is ready
  * @rmtoll SR2          REGLPS        LL_PWR_IsActiveFlag_REGLPS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_REGLPS(void)
{
  return ((READ_BIT(PWR->SR2, PWR_SR2_REGLPS) == (PWR_SR2_REGLPS)) ? 1UL : 0UL);
}

/**
  * @brief  Indicate whether or not the flash is ready to be accessed
  * @rmtoll SR2          FLASH_RDY     LL_PWR_IsActiveFlag_FLASH_RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_FLASH_RDY(void)
{
  return ((READ_BIT(PWR->SR2, PWR_SR2_FLASH_RDY) == (PWR_SR2_FLASH_RDY)) ? 1UL : 0UL);
}


/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup PWR_LL_EF_Init De-initialization function
  * @{
  */
ErrorStatus LL_PWR_DeInit(void);
/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/**
  * @}
  */

/**
  * @}
  */

#endif /* defined(PWR) */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32G0xx_LL_PWR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
