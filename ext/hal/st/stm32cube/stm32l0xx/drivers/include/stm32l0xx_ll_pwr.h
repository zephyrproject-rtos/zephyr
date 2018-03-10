/**
  ******************************************************************************
  * @file    stm32l0xx_ll_pwr.h
  * @author  MCD Application Team
  * @brief   Header file of PWR LL module.
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
#ifndef __STM32L0xx_LL_PWR_H
#define __STM32L0xx_LL_PWR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx.h"

/** @addtogroup STM32L0xx_LL_Driver
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
#define LL_PWR_CR_CSBF                     PWR_CR_CSBF            /*!< Clear standby flag */
#define LL_PWR_CR_CWUF                     PWR_CR_CWUF            /*!< Clear wakeup flag */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_PWR_ReadReg function
  * @{
  */
#define LL_PWR_CSR_WUF                     PWR_CSR_WUF            /*!< Wakeup flag */
#define LL_PWR_CSR_SBF                     PWR_CSR_SBF            /*!< Standby flag */
#if defined(PWR_PVD_SUPPORT)
#define LL_PWR_CSR_PVDO                    PWR_CSR_PVDO           /*!< Power voltage detector output flag */
#endif /* PWR_PVD_SUPPORT */
#if defined(PWR_CSR_VREFINTRDYF)
#define LL_PWR_CSR_VREFINTRDYF             PWR_CSR_VREFINTRDYF    /*!< VREFINT ready flag */
#endif /* PWR_CSR_VREFINTRDYF */
#define LL_PWR_CSR_VOS                     PWR_CSR_VOSF           /*!< Voltage scaling select flag */
#define LL_PWR_CSR_REGLPF                  PWR_CSR_REGLPF         /*!< Regulator low power flag */
#define LL_PWR_CSR_EWUP1                   PWR_CSR_EWUP1          /*!< Enable WKUP pin 1 */
#define LL_PWR_CSR_EWUP2                   PWR_CSR_EWUP2          /*!< Enable WKUP pin 2 */
#if defined(PWR_CSR_EWUP3)
#define LL_PWR_CSR_EWUP3                   PWR_CSR_EWUP3          /*!< Enable WKUP pin 3 */
#endif /* PWR_CSR_EWUP3 */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_REGU_VOLTAGE Regulator Voltage
  * @{
  */
#define LL_PWR_REGU_VOLTAGE_SCALE1         (PWR_CR_VOS_0)                   /*!< 1.8V (range 1) */
#define LL_PWR_REGU_VOLTAGE_SCALE2         (PWR_CR_VOS_1)                   /*!< 1.5V (range 2) */
#define LL_PWR_REGU_VOLTAGE_SCALE3         (PWR_CR_VOS_0 | PWR_CR_VOS_1)    /*!< 1.2V (range 3) */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_MODE_PWR Mode Power
  * @{
  */
#define LL_PWR_MODE_STOP                      0x00000000U                    /*!< Enter Stop mode when the CPU enters deepsleep */
#define LL_PWR_MODE_STANDBY                   (PWR_CR_PDDS)                  /*!< Enter Standby mode when the CPU enters deepsleep */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_REGU_MODE_LP_MODES  Regulator Mode In Low Power Modes
  * @{
  */
#define LL_PWR_REGU_LPMODES_MAIN           0x00000000U        /*!< Voltage regulator in main mode during deepsleep/sleep/low-power run mode */
#define LL_PWR_REGU_LPMODES_LOW_POWER      (PWR_CR_LPSDSR)    /*!< Voltage regulator in low-power mode during deepsleep/sleep/low-power run mode */
/**
  * @}
  */
#if defined(PWR_CR_LPDS)
/** @defgroup PWR_LL_EC_REGU_MODE_DS_MODE  Regulator Mode In Deep Sleep Mode
 * @{
 */
#define LL_PWR_REGU_DSMODE_MAIN        0x00000000U           /*!< Voltage regulator in main mode during deepsleep mode when PWR_CR_LPSDSR = 0 */
#define LL_PWR_REGU_DSMODE_LOW_POWER   (PWR_CR_LPDS)         /*!< Voltage regulator in low-power mode during deepsleep mode when PWR_CR_LPSDSR = 0 */
/**
  * @}
  */
#endif /* PWR_CR_LPDS */

#if defined(PWR_PVD_SUPPORT)
/** @defgroup PWR_LL_EC_PVDLEVEL Power Voltage Detector Level
  * @{
  */
#define LL_PWR_PVDLEVEL_0                  (PWR_CR_PLS_LEV0)      /*!< Voltage threshold detected by PVD 1.9 V */
#define LL_PWR_PVDLEVEL_1                  (PWR_CR_PLS_LEV1)      /*!< Voltage threshold detected by PVD 2.1 V */
#define LL_PWR_PVDLEVEL_2                  (PWR_CR_PLS_LEV2)      /*!< Voltage threshold detected by PVD 2.3 V */
#define LL_PWR_PVDLEVEL_3                  (PWR_CR_PLS_LEV3)      /*!< Voltage threshold detected by PVD 2.5 V */
#define LL_PWR_PVDLEVEL_4                  (PWR_CR_PLS_LEV4)      /*!< Voltage threshold detected by PVD 2.7 V */
#define LL_PWR_PVDLEVEL_5                  (PWR_CR_PLS_LEV5)      /*!< Voltage threshold detected by PVD 2.9 V */
#define LL_PWR_PVDLEVEL_6                  (PWR_CR_PLS_LEV6)      /*!< Voltage threshold detected by PVD 3.1 V */
#define LL_PWR_PVDLEVEL_7                  (PWR_CR_PLS_LEV7)      /*!< External input analog voltage   (Compare internally to VREFINT) */
/**
  * @}
  */
#endif /* PWR_PVD_SUPPORT */
/** @defgroup PWR_LL_EC_WAKEUP_PIN  Wakeup Pins
  * @{
  */
#define LL_PWR_WAKEUP_PIN1                 (PWR_CSR_EWUP1)        /*!< WKUP pin 1 : PA0 */
#define LL_PWR_WAKEUP_PIN2                 (PWR_CSR_EWUP2)        /*!< WKUP pin 2 : PC13 */
#if defined(PWR_CSR_EWUP3)
#define LL_PWR_WAKEUP_PIN3                 (PWR_CSR_EWUP3)        /*!< WKUP pin 3 : PE6 or PA2 according to device */
#endif /* PWR_CSR_EWUP3 */
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

/** @defgroup PWR_LL_EM_WRITE_READ Common write and read registers Macros
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
  * @brief  Switch the regulator from main mode to low-power mode
  * @rmtoll CR    LPRUN       LL_PWR_EnableLowPowerRunMode
  * @note   Remind to set the regulator to low power before enabling
  *         LowPower run mode (bit @ref LL_PWR_REGU_LPMODES_LOW_POWER).
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableLowPowerRunMode(void)
{
  SET_BIT(PWR->CR, PWR_CR_LPRUN);
}

/**
  * @brief  Switch the regulator from low-power mode to main mode
  * @rmtoll CR    LPRUN       LL_PWR_DisableLowPowerRunMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableLowPowerRunMode(void)
{
  CLEAR_BIT(PWR->CR, PWR_CR_LPRUN);
}

/**
  * @brief  Check if the regulator is in low-power mode
  * @rmtoll CR    LPRUN       LL_PWR_IsEnabledLowPowerRunMode
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledLowPowerRunMode(void)
{
  return (READ_BIT(PWR->CR, PWR_CR_LPRUN) == (PWR_CR_LPRUN));
}

/**
  * @brief  Set voltage regulator to low-power and switch from 
  *         run main mode to run low-power mode.
  * @rmtoll CR    LPSDSR       LL_PWR_EnterLowPowerRunMode\n
  *         CR    LPRUN        LL_PWR_EnterLowPowerRunMode
  * @note   This "high level" function is introduced to provide functional
  *         compatibility with other families. Notice that the two registers
  *         have to be written sequentially, so this function is not atomic.
  *         To assure atomicity you can call separately the following functions:
  *         - @ref LL_PWR_SetRegulModeLP(@ref LL_PWR_REGU_LPMODES_LOW_POWER);
  *         - @ref LL_PWR_EnableLowPowerRunMode();
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnterLowPowerRunMode(void)
{
  SET_BIT(PWR->CR, PWR_CR_LPSDSR); /* => LL_PWR_SetRegulModeLP(LL_PWR_REGU_LPMODES_LOW_POWER) */
  SET_BIT(PWR->CR, PWR_CR_LPRUN);  /* => LL_PWR_EnableLowPowerRunMode() */
}

/**
  * @brief  Set voltage regulator to main and switch from 
  *         run main mode to low-power mode.
  * @rmtoll CR    LPSDSR       LL_PWR_ExitLowPowerRunMode\n
  *         CR    LPRUN        LL_PWR_ExitLowPowerRunMode
  * @note   This "high level" function is introduced to provide functional   
  *         compatibility with other families. Notice that the two registers 
  *         have to be written sequentially, so this function is not atomic.
  *         To assure atomicity you can call separately the following functions:
  *         - @ref LL_PWR_DisableLowPowerRunMode();
  *         - @ref LL_PWR_SetRegulModeLP(@ref LL_PWR_REGU_LPMODES_MAIN);
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ExitLowPowerRunMode(void)
{
  CLEAR_BIT(PWR->CR, PWR_CR_LPRUN);   /* => LL_PWR_DisableLowPowerRunMode() */
  CLEAR_BIT(PWR->CR, PWR_CR_LPSDSR);  /* => LL_PWR_SetRegulModeLP(LL_PWR_REGU_LPMODES_MAIN) */
}
/**
  * @brief  Set the main internal regulator output voltage
  * @rmtoll CR    VOS       LL_PWR_SetRegulVoltageScaling
  * @param  VoltageScaling This parameter can be one of the following values:
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE1
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE2
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE3
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetRegulVoltageScaling(uint32_t VoltageScaling)
{
  MODIFY_REG(PWR->CR, PWR_CR_VOS, VoltageScaling);
}

/**
  * @brief  Get the main internal regulator output voltage
  * @rmtoll CR    VOS       LL_PWR_GetRegulVoltageScaling
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE1
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE2
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE3
  */
__STATIC_INLINE uint32_t LL_PWR_GetRegulVoltageScaling(void)
{
  return (uint32_t)(READ_BIT(PWR->CR, PWR_CR_VOS));
}

/**
  * @brief  Enable access to the backup domain
  * @rmtoll CR    DBP       LL_PWR_EnableBkUpAccess
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableBkUpAccess(void)
{
  SET_BIT(PWR->CR, PWR_CR_DBP);
}

/**
  * @brief  Disable access to the backup domain
  * @rmtoll CR    DBP       LL_PWR_DisableBkUpAccess
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableBkUpAccess(void)
{
  CLEAR_BIT(PWR->CR, PWR_CR_DBP);
}

/**
  * @brief  Check if the backup domain is enabled
  * @rmtoll CR    DBP       LL_PWR_IsEnabledBkUpAccess
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledBkUpAccess(void)
{
  return (READ_BIT(PWR->CR, PWR_CR_DBP) == (PWR_CR_DBP));
}

/**
  * @brief  Set voltage regulator mode during low power modes
  * @rmtoll CR    LPSDSR       LL_PWR_SetRegulModeLP
  * @param  RegulMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_REGU_LPMODES_MAIN
  *         @arg @ref LL_PWR_REGU_LPMODES_LOW_POWER
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetRegulModeLP(uint32_t RegulMode)
{
  MODIFY_REG(PWR->CR, PWR_CR_LPSDSR, RegulMode);
}

/**
  * @brief  Get voltage regulator mode during low power modes
  * @rmtoll CR    LPSDSR       LL_PWR_GetRegulModeLP
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_REGU_LPMODES_MAIN
  *         @arg @ref LL_PWR_REGU_LPMODES_LOW_POWER
  */
__STATIC_INLINE uint32_t LL_PWR_GetRegulModeLP(void)
{
  return (uint32_t)(READ_BIT(PWR->CR, PWR_CR_LPSDSR));
}

#if defined(PWR_CR_LPDS)
/**
  * @brief  Set voltage regulator mode during deep sleep mode
  * @rmtoll CR    LPDS         LL_PWR_SetRegulModeDS
  * @param  RegulMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_REGU_DSMODE_MAIN
  *         @arg @ref LL_PWR_REGU_DSMODE_LOW_POWER
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetRegulModeDS(uint32_t RegulMode)
{
  MODIFY_REG(PWR->CR, PWR_CR_LPDS, RegulMode);
}

/**
  * @brief  Get voltage regulator mode during deep sleep mode
  * @rmtoll CR    LPDS         LL_PWR_GetRegulModeDS
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_REGU_DSMODE_MAIN
  *         @arg @ref LL_PWR_REGU_DSMODE_LOW_POWER
  */
__STATIC_INLINE uint32_t LL_PWR_GetRegulModeDS(void)
{
  return (uint32_t)(READ_BIT(PWR->CR, PWR_CR_LPDS));
}
#endif /* PWR_CR_LPDS */

/**
  * @brief  Set power down mode when CPU enters deepsleep
  * @rmtoll CR    PDDS         LL_PWR_SetPowerMode
  * @param  PDMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_MODE_STOP
  *         @arg @ref LL_PWR_MODE_STANDBY
  * @note   Set the regulator to low power (bit @ref LL_PWR_REGU_LPMODES_LOW_POWER)  
  *         before setting MODE_STOP. If the regulator remains in "main mode",   
  *         it consumes more power without providing any additional feature. 
  *         In MODE_STANDBY the regulator is automatically off.
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetPowerMode(uint32_t PDMode)
{
  MODIFY_REG(PWR->CR, PWR_CR_PDDS, PDMode);
}

/**
  * @brief  Get power down mode when CPU enters deepsleep
  * @rmtoll CR    PDDS         LL_PWR_GetPowerMode
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_MODE_STOP
  *         @arg @ref LL_PWR_MODE_STANDBY
  */
__STATIC_INLINE uint32_t LL_PWR_GetPowerMode(void)
{
  return (uint32_t)(READ_BIT(PWR->CR, PWR_CR_PDDS));
}

#if defined(PWR_PVD_SUPPORT)
/**
  * @brief  Configure the voltage threshold detected by the Power Voltage Detector
  * @rmtoll CR    PLS       LL_PWR_SetPVDLevel
  * @param  PVDLevel This parameter can be one of the following values:
  *         @arg @ref LL_PWR_PVDLEVEL_0
  *         @arg @ref LL_PWR_PVDLEVEL_1
  *         @arg @ref LL_PWR_PVDLEVEL_2
  *         @arg @ref LL_PWR_PVDLEVEL_3
  *         @arg @ref LL_PWR_PVDLEVEL_4
  *         @arg @ref LL_PWR_PVDLEVEL_5
  *         @arg @ref LL_PWR_PVDLEVEL_6
  *         @arg @ref LL_PWR_PVDLEVEL_7
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetPVDLevel(uint32_t PVDLevel)
{
  MODIFY_REG(PWR->CR, PWR_CR_PLS, PVDLevel);
}

/**
  * @brief  Get the voltage threshold detection
  * @rmtoll CR    PLS       LL_PWR_GetPVDLevel
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_PVDLEVEL_0
  *         @arg @ref LL_PWR_PVDLEVEL_1
  *         @arg @ref LL_PWR_PVDLEVEL_2
  *         @arg @ref LL_PWR_PVDLEVEL_3
  *         @arg @ref LL_PWR_PVDLEVEL_4
  *         @arg @ref LL_PWR_PVDLEVEL_5
  *         @arg @ref LL_PWR_PVDLEVEL_6
  *         @arg @ref LL_PWR_PVDLEVEL_7
  */
__STATIC_INLINE uint32_t LL_PWR_GetPVDLevel(void)
{
  return (uint32_t)(READ_BIT(PWR->CR, PWR_CR_PLS));
}

/**
  * @brief  Enable Power Voltage Detector
  * @rmtoll CR    PVDE       LL_PWR_EnablePVD
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnablePVD(void)
{
  SET_BIT(PWR->CR, PWR_CR_PVDE);
}

/**
  * @brief  Disable Power Voltage Detector
  * @rmtoll CR    PVDE       LL_PWR_DisablePVD
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisablePVD(void)
{
  CLEAR_BIT(PWR->CR, PWR_CR_PVDE);
}

/**
  * @brief  Check if Power Voltage Detector is enabled
  * @rmtoll CR    PVDE       LL_PWR_IsEnabledPVD
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledPVD(void)
{
  return (READ_BIT(PWR->CR, PWR_CR_PVDE) == (PWR_CR_PVDE));
}
#endif /* PWR_PVD_SUPPORT */

/**
  * @brief  Enable the WakeUp PINx functionality
  * @rmtoll CSR   EWUP1       LL_PWR_EnableWakeUpPin\n
  * @rmtoll CSR   EWUP2       LL_PWR_EnableWakeUpPin\n
  * @rmtoll CSR   EWUP3       LL_PWR_EnableWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3 (*)
  *
  *         (*) not available on all devices
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableWakeUpPin(uint32_t WakeUpPin)
{
  SET_BIT(PWR->CSR, WakeUpPin);
}

/**
  * @brief  Disable the WakeUp PINx functionality
  * @rmtoll CSR   EWUP1       LL_PWR_DisableWakeUpPin\n
  * @rmtoll CSR   EWUP2       LL_PWR_DisableWakeUpPin\n
  * @rmtoll CSR   EWUP3       LL_PWR_DisableWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3 (*)
  *
  *         (*) not available on all devices
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableWakeUpPin(uint32_t WakeUpPin)
{
  CLEAR_BIT(PWR->CSR, WakeUpPin);
}

/**
  * @brief  Check if the WakeUp PINx functionality is enabled
  * @rmtoll CSR   EWUP1       LL_PWR_IsEnabledWakeUpPin\n
  * @rmtoll CSR   EWUP2       LL_PWR_IsEnabledWakeUpPin\n
  * @rmtoll CSR   EWUP3       LL_PWR_IsEnabledWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3 (*)
  *
  *         (*) not available on all devices
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledWakeUpPin(uint32_t WakeUpPin)
{
  return (READ_BIT(PWR->CSR, WakeUpPin) == (WakeUpPin));
}

/**
  * @brief  Enable ultra low-power mode by enabling VREFINT switch off in low-power modes
  * @rmtoll CR    ULP       LL_PWR_EnableUltraLowPower
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableUltraLowPower(void)
{
  SET_BIT(PWR->CR, PWR_CR_ULP);
}

/**
  * @brief  Disable ultra low-power mode by disabling VREFINT switch off in low-power modes
  * @rmtoll CR    ULP       LL_PWR_DisableUltraLowPower
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableUltraLowPower(void)
{
  CLEAR_BIT(PWR->CR, PWR_CR_ULP);
}

/**
  * @brief  Check if ultra low-power mode is enabled by checking if VREFINT switch off in low-power modes is enabled
  * @rmtoll CR    ULP       LL_PWR_IsEnabledUltraLowPower
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledUltraLowPower(void)
{
  return (READ_BIT(PWR->CR, PWR_CR_ULP) == (PWR_CR_ULP));
}

/**
  * @brief  Enable fast wakeup by ignoring VREFINT startup time when exiting from low-power mode
  * @rmtoll CR    FWU       LL_PWR_EnableFastWakeUp
  * @note   Works in conjunction with ultra low power mode.
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableFastWakeUp(void)
{
  SET_BIT(PWR->CR, PWR_CR_FWU);
}

/**
  * @brief  Disable fast wakeup by waiting VREFINT startup time when exiting from low-power mode
  * @rmtoll CR    FWU       LL_PWR_DisableFastWakeUp
  * @note   Works in conjunction with ultra low power mode.
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableFastWakeUp(void)
{
  CLEAR_BIT(PWR->CR, PWR_CR_FWU);
}

/**
  * @brief  Check if fast wakeup is enabled by checking if VREFINT startup time when exiting from low-power mode is ignored
  * @rmtoll CR    FWU       LL_PWR_IsEnabledFastWakeUp
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledFastWakeUp(void)
{
  return (READ_BIT(PWR->CR, PWR_CR_FWU) == (PWR_CR_FWU));
}

/**
  * @brief  Enable non-volatile memory (Flash and EEPROM) keeping off feature when exiting from low-power mode
  * @rmtoll CR    DS_EE_KOFF       LL_PWR_EnableNVMKeptOff
  * @note   When enabled, after entering low-power mode (Stop or Standby only), if RUN_PD of FLASH_ACR register
  *         is also set, the Flash memory will not be woken up when exiting from deepsleep mode.
  *         When enabled, the EEPROM will not be woken up when exiting from low-power mode (if the bit RUN_PD is set)
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableNVMKeptOff(void)
{
  SET_BIT(PWR->CR, PWR_CR_DSEEKOFF);
}

/**
  * @brief  Disable non-volatile memory (Flash and EEPROM) keeping off feature when exiting from low-power mode
  * @rmtoll CR    DS_EE_KOFF       LL_PWR_DisableNVMKeptOff
  * @note   When disabled, Flash memory is woken up when exiting from deepsleep mode even if the bit RUN_PD is set
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableNVMKeptOff(void)
{
  CLEAR_BIT(PWR->CR, PWR_CR_DSEEKOFF);
}

/**
  * @brief  Check if non-volatile memory (Flash and EEPROM) keeping off feature when exiting from low-power mode is enabled
  * @rmtoll CR    DS_EE_KOFF       LL_PWR_IsEnabledNVMKeptOff
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledNVMKeptOff(void)
{
  return (READ_BIT(PWR->CR, PWR_CR_DSEEKOFF) == (PWR_CR_DSEEKOFF));
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_FLAG_Management FLAG_Management
  * @{
  */

/**
  * @brief  Get Wake-up Flag
  * @rmtoll CSR   WUF       LL_PWR_IsActiveFlag_WU
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU(void)
{
  return (READ_BIT(PWR->CSR, PWR_CSR_WUF) == (PWR_CSR_WUF));
}

/**
  * @brief  Get Standby Flag
  * @rmtoll CSR   SBF       LL_PWR_IsActiveFlag_SB
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_SB(void)
{
  return (READ_BIT(PWR->CSR, PWR_CSR_SBF) == (PWR_CSR_SBF));
}

#if defined(PWR_PVD_SUPPORT)
/**
  * @brief  Indicate whether VDD voltage is below the selected PVD threshold
  * @rmtoll CSR   PVDO       LL_PWR_IsActiveFlag_PVDO
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_PVDO(void)
{
  return (READ_BIT(PWR->CSR, PWR_CSR_PVDO) == (PWR_CSR_PVDO));
}
#endif /* PWR_PVD_SUPPORT */

#if defined(PWR_CSR_VREFINTRDYF)
/**
  * @brief  Get Internal Reference VrefInt Flag
  * @rmtoll CSR   VREFINTRDYF       LL_PWR_IsActiveFlag_VREFINTRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_VREFINTRDY(void)
{
  return (READ_BIT(PWR->CSR, PWR_CSR_VREFINTRDYF) == (PWR_CSR_VREFINTRDYF));
}
#endif /* PWR_CSR_VREFINTRDYF */
/**
  * @brief  Indicate whether the regulator is ready in the selected voltage range or if its output voltage is still changing to the required voltage level
  * @rmtoll CSR   VOSF       LL_PWR_IsActiveFlag_VOSF
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_VOSF(void)
{
  return (READ_BIT(PWR->CSR, LL_PWR_CSR_VOS) == (LL_PWR_CSR_VOS));
}
/**
  * @brief Indicate whether the regulator is ready in main mode or is in low-power mode
  * @rmtoll CSR   REGLPF       LL_PWR_IsActiveFlag_REGLPF
  * @note Take care, return value "0" means the regulator is ready.  Return value "1" means the output voltage range is still changing.
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_REGLPF(void)
{
  return (READ_BIT(PWR->CSR, PWR_CSR_REGLPF) == (PWR_CSR_REGLPF));
}
/**
  * @brief  Clear Standby Flag
  * @rmtoll CR   CSBF       LL_PWR_ClearFlag_SB
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_SB(void)
{
  SET_BIT(PWR->CR, PWR_CR_CSBF);
}

/**
  * @brief  Clear Wake-up Flags
  * @rmtoll CR   CWUF       LL_PWR_ClearFlag_WU
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU(void)
{
  SET_BIT(PWR->CR, PWR_CR_CWUF);
}
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

#endif /* __STM32L0xx_LL_PWR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
