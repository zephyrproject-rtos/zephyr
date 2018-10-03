/**
  ******************************************************************************
  * @file    stm32f7xx_ll_pwr.h
  * @author  MCD Application Team
  * @brief   Header file of PWR LL module.
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
#ifndef __STM32F7xx_LL_PWR_H
#define __STM32F7xx_LL_PWR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx.h"

/** @addtogroup STM32F7xx_LL_Driver
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
#define LL_PWR_CR1_CSBF                    PWR_CR1_CSBF            /*!< Clear standby flag */

#define LL_PWR_CR2_CWUF6                   PWR_CR2_CWUF6           /*!< Clear WKUP pin 6 */
#define LL_PWR_CR2_CWUF5                   PWR_CR2_CWUF5           /*!< Clear WKUP pin 5 */
#define LL_PWR_CR2_CWUF4                   PWR_CR2_CWUF4           /*!< Clear WKUP pin 4 */
#define LL_PWR_CR2_CWUF3                   PWR_CR2_CWUF3           /*!< Clear WKUP pin 3 */
#define LL_PWR_CR2_CWUF2                   PWR_CR2_CWUF2           /*!< Clear WKUP pin 2 */
#define LL_PWR_CR2_CWUF1                   PWR_CR2_CWUF1           /*!< Clear WKUP pin 1 */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_PWR_ReadReg function
  * @{
  */
#define LL_PWR_CSR1_WUIF                   PWR_CSR1_WUIF           /*!< Wakeup flag */
#define LL_PWR_CSR1_SBF                    PWR_CSR1_SBF            /*!< Standby flag */
#define LL_PWR_CSR1_PVDO                   PWR_CSR1_PVDO           /*!< Power voltage detector output flag */
#define LL_PWR_CSR1_BRR                    PWR_CSR1_BRR            /*!< Backup Regulator ready flag */
#define LL_PWR_CSR1_VOSRDY                 PWR_CSR1_VOSRDY         /*!< Voltage scaling select flag */
#define LL_PWR_CSR1_ODRDY                  PWR_CSR1_ODRDY          /*!< Over-drive mode ready */
#define LL_PWR_CSR1_ODSWRDY                PWR_CSR1_ODSWRDY        /*!< Over-drive mode switching ready */
#define LL_PWR_CSR1_UDRDY                  PWR_CSR1_UDRDY          /*!< Under-drive ready flag */

#define LL_PWR_CSR2_EWUP1                  PWR_CSR2_EWUP1          /*!< Enable WKUP pin 1 */
#define LL_PWR_CSR2_EWUP2                  PWR_CSR2_EWUP2          /*!< Enable WKUP pin 2 */
#define LL_PWR_CSR2_EWUP3                  PWR_CSR2_EWUP3          /*!< Enable WKUP pin 3 */
#define LL_PWR_CSR2_EWUP4                  PWR_CSR2_EWUP4          /*!< Enable WKUP pin 4 */
#define LL_PWR_CSR2_EWUP5                  PWR_CSR2_EWUP5          /*!< Enable WKUP pin 5 */
#define LL_PWR_CSR2_EWUP6                  PWR_CSR2_EWUP6          /*!< Enable WKUP pin 6 */
/**
  * @}
  */
  
/** @defgroup PWR_LL_EC_MODE_PWR Mode Power
  * @{
  */
#define LL_PWR_MODE_STOP_MAINREGU                     0x00000000U                                    /*!< Enter Stop mode (with main Regulator ON) when the CPU enters deepsleep */
#define LL_PWR_MODE_STOP_MAINREGU_UNDERDRIVE          (PWR_CR1_MRUDS | PWR_CR1_FPDS)                 /*!< Enter Stop mode (with main Regulator in under-drive mode) when the CPU enters deepsleep */
#define LL_PWR_MODE_STOP_LPREGU                       PWR_CR1_LPDS                                   /*!< Enter Stop mode (with low power Regulator ON) when the CPU enters deepsleep */
#define LL_PWR_MODE_STOP_LPREGU_UNDERDRIVE            (PWR_CR1_LPDS | PWR_CR1_LPUDS | PWR_CR1_FPDS)  /*!< Enter Stop mode (with low power Regulator in under-drive mode) when the CPU enters deepsleep */
#define LL_PWR_MODE_STANDBY                           PWR_CR1_PDDS                                   /*!< Enter Standby mode when the CPU enters deepsleep */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_REGU_VOLTAGE Regulator Voltage
  * @{
  */
#define LL_PWR_REGU_VOLTAGE_SCALE3         PWR_CR1_VOS_0
#define LL_PWR_REGU_VOLTAGE_SCALE2         PWR_CR1_VOS_1
#define LL_PWR_REGU_VOLTAGE_SCALE1         (PWR_CR1_VOS_0 | PWR_CR1_VOS_1)
/**
  * @}
  */

/** @defgroup PWR_LL_EC_REGU_MODE_DS_MODE  Regulator Mode In Deep Sleep Mode
  * @{
  */
#define LL_PWR_REGU_DSMODE_MAIN        0x00000000U                    /*!< Voltage Regulator in main mode during deepsleep mode */
#define LL_PWR_REGU_DSMODE_LOW_POWER   PWR_CR1_LPDS                   /*!< Voltage Regulator in low-power mode during deepsleep mode */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_PVDLEVEL Power Voltage Detector Level
  * @{
  */
#define LL_PWR_PVDLEVEL_0                  PWR_CR1_PLS_LEV0      /*!< Voltage threshold detected by PVD 2.0 V */
#define LL_PWR_PVDLEVEL_1                  PWR_CR1_PLS_LEV1      /*!< Voltage threshold detected by PVD 2.1 V */
#define LL_PWR_PVDLEVEL_2                  PWR_CR1_PLS_LEV2      /*!< Voltage threshold detected by PVD 2.3 V */
#define LL_PWR_PVDLEVEL_3                  PWR_CR1_PLS_LEV3      /*!< Voltage threshold detected by PVD 2.5 V */
#define LL_PWR_PVDLEVEL_4                  PWR_CR1_PLS_LEV4      /*!< Voltage threshold detected by PVD 2.6 V */
#define LL_PWR_PVDLEVEL_5                  PWR_CR1_PLS_LEV5      /*!< Voltage threshold detected by PVD 2.7 V */
#define LL_PWR_PVDLEVEL_6                  PWR_CR1_PLS_LEV6      /*!< Voltage threshold detected by PVD 2.8 V */
#define LL_PWR_PVDLEVEL_7                  PWR_CR1_PLS_LEV7      /*!< Voltage threshold detected by PVD 2.9 V */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_WAKEUP_PIN  Wakeup Pins
  * @{
  */
#define LL_PWR_WAKEUP_PIN1                 PWR_CSR2_EWUP1        /*!< WKUP pin 1 : PA0  */
#define LL_PWR_WAKEUP_PIN2                 PWR_CSR2_EWUP2        /*!< WKUP pin 2 : PA2  */
#define LL_PWR_WAKEUP_PIN3                 PWR_CSR2_EWUP3        /*!< WKUP pin 3 : PC1  */
#define LL_PWR_WAKEUP_PIN4                 PWR_CSR2_EWUP4        /*!< WKUP pin 4 : PC13 */
#define LL_PWR_WAKEUP_PIN5                 PWR_CSR2_EWUP5        /*!< WKUP pin 5 : PI8  */
#define LL_PWR_WAKEUP_PIN6                 PWR_CSR2_EWUP6        /*!< WKUP pin 6 : PI11 */
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
  * @brief  Enable Under Drive Mode
  * @rmtoll CR1    UDEN       LL_PWR_EnableUnderDriveMode
  * @note  This mode is enabled only with STOP low power mode.
  *        In this mode, the 1.2V domain is preserved in reduced leakage mode. This 
  *        mode is only available when the main Regulator or the low power Regulator 
  *        is in low voltage mode.      
  * @note  If the Under-drive mode was enabled, it is automatically disabled after 
  *        exiting Stop mode. 
  *        When the voltage Regulator operates in Under-drive mode, an additional  
  *        startup delay is induced when waking up from Stop mode.
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableUnderDriveMode(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_UDEN);
}

/**
  * @brief  Disable Under Drive Mode
  * @rmtoll CR1    UDEN       LL_PWR_DisableUnderDriveMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableUnderDriveMode(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_UDEN);
}

/**
  * @brief  Check if Under Drive Mode is enabled
  * @rmtoll CR1    UDEN       LL_PWR_IsEnabledUnderDriveMode
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledUnderDriveMode(void)
{
  return (READ_BIT(PWR->CR1, PWR_CR1_UDEN) == (PWR_CR1_UDEN));
}

/**
  * @brief  Enable Over drive switching
  * @rmtoll CR1    ODSWEN       LL_PWR_EnableOverDriveSwitching
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableOverDriveSwitching(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_ODSWEN);
}

/**
  * @brief  Disable Over drive switching
  * @rmtoll CR1    ODSWEN       LL_PWR_DisableOverDriveSwitching
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableOverDriveSwitching(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_ODSWEN);
}

/**
  * @brief  Check if Over drive switching is enabled
  * @rmtoll CR1    ODSWEN       LL_PWR_IsEnabledOverDriveSwitching
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledOverDriveSwitching(void)
{
  return (READ_BIT(PWR->CR1, PWR_CR1_ODSWEN) == (PWR_CR1_ODSWEN));
}

/**
  * @brief  Enable Over drive Mode
  * @rmtoll CR1    ODEN       LL_PWR_EnableOverDriveMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableOverDriveMode(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_ODEN);
}

/**
  * @brief  Disable Over drive Mode
  * @rmtoll CR1    ODEN       LL_PWR_DisableOverDriveMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableOverDriveMode(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_ODEN);
}

/**
  * @brief  Check if Over drive switching is enabled
  * @rmtoll CR1    ODEN       LL_PWR_IsEnabledOverDriveMode
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledOverDriveMode(void)
{
  return (READ_BIT(PWR->CR1, PWR_CR1_ODEN) == (PWR_CR1_ODEN));
}

/**
  * @brief  Set the main internal Regulator output voltage
  * @rmtoll CR1    VOS       LL_PWR_SetRegulVoltageScaling
  * @param  VoltageScaling This parameter can be one of the following values:
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE1
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE2
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE3
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetRegulVoltageScaling(uint32_t VoltageScaling)
{
  MODIFY_REG(PWR->CR1, PWR_CR1_VOS, VoltageScaling);
}

/**
  * @brief  Get the main internal Regulator output voltage
  * @rmtoll CR1    VOS       LL_PWR_GetRegulVoltageScaling
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE1
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE2
  *         @arg @ref LL_PWR_REGU_VOLTAGE_SCALE3
  */
__STATIC_INLINE uint32_t LL_PWR_GetRegulVoltageScaling(void)
{
  return (uint32_t)(READ_BIT(PWR->CR1, PWR_CR1_VOS));
}

/**
  * @brief  Enable Main Regulator in deepsleep under-drive Mode
  * @rmtoll CR1    MRUDS       LL_PWR_EnableMainRegulatorDeepSleepUDMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableMainRegulatorDeepSleepUDMode(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_MRUDS);
}

/**
  * @brief  Disable Main Regulator in deepsleep under-drive Mode
  * @rmtoll CR1    MRUDS       LL_PWR_DisableMainRegulatorDeepSleepUDMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableMainRegulatorDeepSleepUDMode(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_MRUDS);
}

/**
  * @brief  Check if Main Regulator in deepsleep under-drive Mode is enabled
  * @rmtoll CR1    MRUDS       LL_PWR_IsEnabledMainRegulatorDeepSleepUDMode
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledMainRegulatorDeepSleepUDMode(void)
{
  return (READ_BIT(PWR->CR1, PWR_CR1_MRUDS) == (PWR_CR1_MRUDS));
}

/**
  * @brief  Enable Low Power Regulator in deepsleep under-drive Mode
  * @rmtoll CR1    LPUDS       LL_PWR_EnableLowPowerRegulatorDeepSleepUDMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableLowPowerRegulatorDeepSleepUDMode(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_LPUDS);
}

/**
  * @brief  Disable Low Power Regulator in deepsleep under-drive Mode
  * @rmtoll CR1    LPUDS       LL_PWR_DisableLowPowerRegulatorDeepSleepUDMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableLowPowerRegulatorDeepSleepUDMode(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_LPUDS);
}

/**
  * @brief  Check if Low Power Regulator in deepsleep under-drive Mode is enabled
  * @rmtoll CR1    LPUDS       LL_PWR_IsEnabledLowPowerRegulatorDeepSleepUDMode
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledLowPowerRegulatorDeepSleepUDMode(void)
{
  return (READ_BIT(PWR->CR1, PWR_CR1_LPUDS) == (PWR_CR1_LPUDS));
}

/**
  * @brief  Enable the Flash Power Down in Stop Mode
  * @rmtoll CR1    FPDS       LL_PWR_EnableFlashPowerDown
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableFlashPowerDown(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_FPDS);
}

/**
  * @brief  Disable the Flash Power Down in Stop Mode
  * @rmtoll CR1    FPDS       LL_PWR_DisableFlashPowerDown
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableFlashPowerDown(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_FPDS);
}

/**
  * @brief  Check if the Flash Power Down in Stop Mode is enabled
  * @rmtoll CR1    FPDS       LL_PWR_IsEnabledFlashPowerDown
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledFlashPowerDown(void)
{
  return (READ_BIT(PWR->CR1, PWR_CR1_FPDS) == (PWR_CR1_FPDS));
}

/**
  * @brief  Enable access to the backup domain
  * @rmtoll CR1    DBP       LL_PWR_EnableBkUpAccess
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableBkUpAccess(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_DBP);
}

/**
  * @brief  Disable access to the backup domain
  * @rmtoll CR1    DBP       LL_PWR_DisableBkUpAccess
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableBkUpAccess(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_DBP);
}

/**
  * @brief  Check if the backup domain is enabled
  * @rmtoll CR1    DBP       LL_PWR_IsEnabledBkUpAccess
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledBkUpAccess(void)
{
  return (READ_BIT(PWR->CR1, PWR_CR1_DBP) == (PWR_CR1_DBP));
}

/**
  * @brief  Enable Backup Regulator
  * @rmtoll CSR1    BRE       LL_PWR_EnableBkUpRegulator
  * @note When set, the Backup Regulator (used to maintain backup SRAM content in Standby and
  *       VBAT modes) is enabled. If BRE is reset, the backup Regulator is switched off. The backup
  *       SRAM can still be used but its content will be lost in the Standby and VBAT modes. Once set,
  *       the application must wait that the Backup Regulator Ready flag (BRR) is set to indicate that
  *       the data written into the RAM will be maintained in the Standby and VBAT modes.
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableBkUpRegulator(void)
{
  SET_BIT(PWR->CSR1, PWR_CSR1_BRE);
}

/**
  * @brief  Disable Backup Regulator
  * @rmtoll CSR1    BRE       LL_PWR_DisableBkUpRegulator
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableBkUpRegulator(void)
{
  CLEAR_BIT(PWR->CSR1, PWR_CSR1_BRE);
}

/**
  * @brief  Check if the backup Regulator is enabled
  * @rmtoll CSR1    BRE      LL_PWR_IsEnabledBkUpRegulator
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledBkUpRegulator(void)
{
  return (READ_BIT(PWR->CSR1, PWR_CSR1_BRE) == (PWR_CSR1_BRE));
}

/**
  * @brief  Set voltage Regulator mode during deep sleep mode
  * @rmtoll CR1    LPDS         LL_PWR_SetRegulModeDS
  * @param  RegulMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_REGU_DSMODE_MAIN
  *         @arg @ref LL_PWR_REGU_DSMODE_LOW_POWER
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetRegulModeDS(uint32_t RegulMode)
{
  MODIFY_REG(PWR->CR1, PWR_CR1_LPDS, RegulMode);
}

/**
  * @brief  Get voltage Regulator mode during deep sleep mode
  * @rmtoll CR1    LPDS         LL_PWR_GetRegulModeDS
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_REGU_DSMODE_MAIN
  *         @arg @ref LL_PWR_REGU_DSMODE_LOW_POWER
  */
__STATIC_INLINE uint32_t LL_PWR_GetRegulModeDS(void)
{
  return (uint32_t)(READ_BIT(PWR->CR1, PWR_CR1_LPDS));
}

/**
  * @brief  Set Power Down mode when CPU enters deepsleep
  * @rmtoll CR1    PDDS         LL_PWR_SetPowerMode\n
  *         CR1    LPDS         LL_PWR_SetPowerMode\n
  *         CR1    FPDS         LL_PWR_SetPowerMode\n
  *         CR1    LPUDS        LL_PWR_SetPowerMode\n  
  *         CR1    MRUDS        LL_PWR_SetPowerMode
  * @param  PDMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_MODE_STOP_MAINREGU
  *         @arg @ref LL_PWR_MODE_STOP_MAINREGU_UNDERDRIVE
  *         @arg @ref LL_PWR_MODE_STOP_LPREGU
  *         @arg @ref LL_PWR_MODE_STOP_LPREGU_UNDERDRIVE
  *         @arg @ref LL_PWR_MODE_STANDBY
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetPowerMode(uint32_t PDMode)
{
  MODIFY_REG(PWR->CR1, (PWR_CR1_PDDS | PWR_CR1_LPDS | PWR_CR1_FPDS | PWR_CR1_LPUDS | PWR_CR1_MRUDS), PDMode);
}

/**
  * @brief  Get Power Down mode when CPU enters deepsleep
  * @rmtoll CR1    PDDS         LL_PWR_GetPowerMode\n
  *         CR1    LPDS         LL_PWR_GetPowerMode\n
  *         CR1    FPDS         LL_PWR_GetPowerMode\n
  *         CR1    LPUDS        LL_PWR_GetPowerMode\n  
  *         CR1    MRUDS        LL_PWR_GetPowerMode
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_MODE_STOP_MAINREGU
  *         @arg @ref LL_PWR_MODE_STOP_MAINREGU_UNDERDRIVE
  *         @arg @ref LL_PWR_MODE_STOP_LPREGU
  *         @arg @ref LL_PWR_MODE_STOP_LPREGU_UNDERDRIVE
  *         @arg @ref LL_PWR_MODE_STANDBY
  */
__STATIC_INLINE uint32_t LL_PWR_GetPowerMode(void)
{
  return (uint32_t)(READ_BIT(PWR->CR1, (PWR_CR1_PDDS | PWR_CR1_LPDS | PWR_CR1_FPDS | PWR_CR1_LPUDS | PWR_CR1_MRUDS)));
}

/**
  * @brief  Configure the voltage threshold detected by the Power Voltage Detector
  * @rmtoll CR1    PLS       LL_PWR_SetPVDLevel
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
  MODIFY_REG(PWR->CR1, PWR_CR1_PLS, PVDLevel);
}

/**
  * @brief  Get the voltage threshold detection
  * @rmtoll CR1    PLS       LL_PWR_GetPVDLevel
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
  return (uint32_t)(READ_BIT(PWR->CR1, PWR_CR1_PLS));
}

/**
  * @brief  Enable Power Voltage Detector
  * @rmtoll CR1    PVDE       LL_PWR_EnablePVD
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnablePVD(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_PVDE);
}

/**
  * @brief  Disable Power Voltage Detector
  * @rmtoll CR1    PVDE       LL_PWR_DisablePVD
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisablePVD(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_PVDE);
}

/**
  * @brief  Check if Power Voltage Detector is enabled
  * @rmtoll CR1    PVDE       LL_PWR_IsEnabledPVD
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledPVD(void)
{
  return (READ_BIT(PWR->CR1, PWR_CR1_PVDE) == (PWR_CR1_PVDE));
}

/**
  * @brief  Enable the WakeUp PINx functionality
  * @rmtoll CSR2   EWUP1       LL_PWR_EnableWakeUpPin\n
  *         CSR2   EWUP2       LL_PWR_EnableWakeUpPin\n
  *         CSR2   EWUP3       LL_PWR_EnableWakeUpPin\n
  *         CSR2   EWUP4       LL_PWR_EnableWakeUpPin\n
  *         CSR2   EWUP5       LL_PWR_EnableWakeUpPin\n
  *         CSR2   EWUP6       LL_PWR_EnableWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableWakeUpPin(uint32_t WakeUpPin)
{
  SET_BIT(PWR->CSR2, WakeUpPin);
}

/**
  * @brief  Disable the WakeUp PINx functionality
  * @rmtoll CSR2   EWUP1       LL_PWR_DisableWakeUpPin\n
  *         CSR2   EWUP2       LL_PWR_DisableWakeUpPin\n
  *         CSR2   EWUP3       LL_PWR_DisableWakeUpPin\n
  *         CSR2   EWUP4       LL_PWR_DisableWakeUpPin\n
  *         CSR2   EWUP5       LL_PWR_DisableWakeUpPin\n
  *         CSR2   EWUP6       LL_PWR_DisableWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableWakeUpPin(uint32_t WakeUpPin)
{
  CLEAR_BIT(PWR->CSR2, WakeUpPin);
}

/**
  * @brief  Check if the WakeUp PINx functionality is enabled
  * @rmtoll CSR2   EWUP1       LL_PWR_IsEnabledWakeUpPin\n
  *         CSR2   EWUP2       LL_PWR_IsEnabledWakeUpPin\n
  *         CSR2   EWUP3       LL_PWR_IsEnabledWakeUpPin\n
  *         CSR2   EWUP4       LL_PWR_IsEnabledWakeUpPin\n
  *         CSR2   EWUP5       LL_PWR_IsEnabledWakeUpPin\n
  *         CSR2   EWUP6       LL_PWR_IsEnabledWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledWakeUpPin(uint32_t WakeUpPin)
{
  return (READ_BIT(PWR->CSR2, WakeUpPin) == (WakeUpPin));
}

/**
  * @brief  Set the Wake-Up pin polarity low for the event detection
  * @rmtoll CR2   WUPP1       LL_PWR_SetWakeUpPinPolarityLow\n
  *         CR2   WUPP2       LL_PWR_SetWakeUpPinPolarityLow\n
  *         CR2   WUPP3       LL_PWR_SetWakeUpPinPolarityLow\n
  *         CR2   WUPP4       LL_PWR_SetWakeUpPinPolarityLow\n
  *         CR2   WUPP5       LL_PWR_SetWakeUpPinPolarityLow\n
  *         CR2   WUPP6       LL_PWR_SetWakeUpPinPolarityLow
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetWakeUpPinPolarityLow(uint32_t WakeUpPin)
{
  SET_BIT(PWR->CR2, WakeUpPin);
}

/**
  * @brief  Set the Wake-Up pin polarity high for the event detection
  * @rmtoll CR2   WUPP1       LL_PWR_SetWakeUpPinPolarityHigh\n
  *         CR2   WUPP2       LL_PWR_SetWakeUpPinPolarityHigh\n
  *         CR2   WUPP3       LL_PWR_SetWakeUpPinPolarityHigh\n
  *         CR2   WUPP4       LL_PWR_SetWakeUpPinPolarityHigh\n
  *         CR2   WUPP5       LL_PWR_SetWakeUpPinPolarityHigh\n
  *         CR2   WUPP6       LL_PWR_SetWakeUpPinPolarityHigh
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetWakeUpPinPolarityHigh(uint32_t WakeUpPin)
{
  CLEAR_BIT(PWR->CR2, WakeUpPin);
}

/**
  * @brief  Get the Wake-Up pin polarity for the event detection
  * @rmtoll CR2   WUPP1       LL_PWR_IsWakeUpPinPolarityLow\n
  *         CR2   WUPP2       LL_PWR_IsWakeUpPinPolarityLow\n
  *         CR2   WUPP3       LL_PWR_IsWakeUpPinPolarityLow\n
  *         CR2   WUPP4       LL_PWR_IsWakeUpPinPolarityLow\n
  *         CR2   WUPP5       LL_PWR_IsWakeUpPinPolarityLow\n
  *         CR2   WUPP6       LL_PWR_IsWakeUpPinPolarityLow
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  *         @arg @ref LL_PWR_WAKEUP_PIN6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsWakeUpPinPolarityLow(uint32_t WakeUpPin)
{
  return (READ_BIT(PWR->CR2, WakeUpPin) == (WakeUpPin));
}

/**
  * @brief  Enable Internal WakeUp
  * @rmtoll CSR1    EIWUP       LL_PWR_EnableInternalWakeUp
  * @note This API must be used when RTC events (Alarm A or Alarm B, RTC Tamper, RTC TimeStamp
  *  or RTC Wakeup time) are used to wake up the system from Standby mode.
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableInternalWakeUp(void)
{
  SET_BIT(PWR->CSR1, PWR_CSR1_EIWUP);
}

/**
  * @brief  Disable Internal WakeUp
  * @rmtoll CSR1    EIWUP       LL_PWR_DisableInternalWakeUp
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableInternalWakeUp(void)
{
  CLEAR_BIT(PWR->CSR1, PWR_CSR1_EIWUP);
}

/**
  * @brief  Check if the Internal WakeUp functionality is enabled
  * @rmtoll CSR1   EIWUP       LL_PWR_IsEnabledInternalWakeUp
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledInternalWakeUp(void)
{
  return (READ_BIT(PWR->CSR1, PWR_CSR1_EIWUP) == (PWR_CSR1_EIWUP));
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_FLAG_Management FLAG_Management
  * @{
  */

/**
  * @brief  Get Wake-up Flag 6
  * @rmtoll CSR2          WUPF6          LL_PWR_IsActiveFlag_WU6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU6(void)
{
  return (READ_BIT(PWR->CSR2, PWR_CSR2_WUPF6) == (PWR_CSR2_WUPF6));
}

/**
  * @brief  Get Wake-up Flag 5
  * @rmtoll CSR2          WUPF5          LL_PWR_IsActiveFlag_WU5
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU5(void)
{
  return (READ_BIT(PWR->CSR2, PWR_CSR2_WUPF5) == (PWR_CSR2_WUPF5));
}

/**
  * @brief  Get Wake-up Flag 4
  * @rmtoll CSR2          WUPF4          LL_PWR_IsActiveFlag_WU4
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU4(void)
{
  return (READ_BIT(PWR->CSR2, PWR_CSR2_WUPF4) == (PWR_CSR2_WUPF4));
}

/**
  * @brief  Get Wake-up Flag 3
  * @rmtoll CSR2          WUPF3          LL_PWR_IsActiveFlag_WU3
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU3(void)
{
  return (READ_BIT(PWR->CSR2, PWR_CSR2_WUPF3) == (PWR_CSR2_WUPF3));
}

/**
  * @brief  Get Wake-up Flag 2
  * @rmtoll CSR2          WUPF2          LL_PWR_IsActiveFlag_WU2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU2(void)
{
  return (READ_BIT(PWR->CSR2, PWR_CSR2_WUPF2) == (PWR_CSR2_WUPF2));
}

/**
  * @brief  Get Wake-up Flag 1
  * @rmtoll CSR2          WUPF1          LL_PWR_IsActiveFlag_WU1
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU1(void)
{
  return (READ_BIT(PWR->CSR2, PWR_CSR2_WUPF1) == (PWR_CSR2_WUPF1));
}

/**
  * @brief  Get Standby Flag
  * @rmtoll CSR1   SBF       LL_PWR_IsActiveFlag_SB
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_SB(void)
{
  return (READ_BIT(PWR->CSR1, PWR_CSR1_SBF) == (PWR_CSR1_SBF));
}

/**
  * @brief  Indicate whether VDD voltage is below the selected PVD threshold
  * @rmtoll CSR1   PVDO       LL_PWR_IsActiveFlag_PVDO
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_PVDO(void)
{
  return (READ_BIT(PWR->CSR1, PWR_CSR1_PVDO) == (PWR_CSR1_PVDO));
}

/**
  * @brief  Get Backup Regulator ready Flag
  * @rmtoll CSR1   BRR       LL_PWR_IsActiveFlag_BRR
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_BRR(void)
{
  return (READ_BIT(PWR->CSR1, PWR_CSR1_BRR) == (PWR_CSR1_BRR));
}

/**
  * @brief  Indicate whether the Regulator is ready in the selected voltage range or if its output voltage is still changing to the required voltage level
  * @rmtoll CSR1   VOSRDY       LL_PWR_IsActiveFlag_VOS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_VOS(void)
{
  return (READ_BIT(PWR->CSR1, PWR_CSR1_VOSRDY) == (PWR_CSR1_VOSRDY));
}

/**
  * @brief  Indicate whether the Over-Drive mode is ready or not
  * @rmtoll CSR1   ODRDY       LL_PWR_IsActiveFlag_OD
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_OD(void)
{
  return (READ_BIT(PWR->CSR1, PWR_CSR1_ODRDY) == (PWR_CSR1_ODRDY));
}

/**
  * @brief  Indicate whether the Over-Drive mode switching is ready or not
  * @rmtoll CSR1   ODSWRDY       LL_PWR_IsActiveFlag_ODSW
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_ODSW(void)
{
  return (READ_BIT(PWR->CSR1, PWR_CSR1_ODSWRDY) == (PWR_CSR1_ODSWRDY));
}

/**
  * @brief  Indicate whether the Under-Drive mode is ready or not
  * @rmtoll CSR1   UDRDY       LL_PWR_IsActiveFlag_UD
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_UD(void)
{
  return (READ_BIT(PWR->CSR1, PWR_CSR1_UDRDY) == (PWR_CSR1_UDRDY));
}

/**
  * @brief  Clear Standby Flag
  * @rmtoll CR1   CSBF       LL_PWR_ClearFlag_SB
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_SB(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_CSBF);
}

/**
  * @brief  Clear Wake-up Flag 6
  * @rmtoll CR2          CWUF6         LL_PWR_ClearFlag_WU6
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU6(void)
{
  WRITE_REG(PWR->CR2, PWR_CR2_CWUPF6);
}

/**
  * @brief  Clear Wake-up Flag 5
  * @rmtoll CR2          CWUF5         LL_PWR_ClearFlag_WU5
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU5(void)
{
  WRITE_REG(PWR->CR2, PWR_CR2_CWUPF5);
}

/**
  * @brief  Clear Wake-up Flag 4
  * @rmtoll CR2          CWUF4         LL_PWR_ClearFlag_WU4
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU4(void)
{
  WRITE_REG(PWR->CR2, PWR_CR2_CWUPF4);
}

/**
  * @brief  Clear Wake-up Flag 3
  * @rmtoll CR2          CWUF3         LL_PWR_ClearFlag_WU3
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU3(void)
{
  WRITE_REG(PWR->CR2, PWR_CR2_CWUPF3);
}

/**
  * @brief  Clear Wake-up Flag 2
  * @rmtoll CR2          CWUF2         LL_PWR_ClearFlag_WU2
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU2(void)
{
  WRITE_REG(PWR->CR2, PWR_CR2_CWUPF2);
}

/**
  * @brief  Clear Wake-up Flag 1
  * @rmtoll CR2          CWUF1         LL_PWR_ClearFlag_WU1
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU1(void)
{
  WRITE_REG(PWR->CR2, PWR_CR2_CWUPF1);
}

/**
  * @brief  Clear Under-Drive ready Flag
  * @rmtoll CSR1          UDRDY         LL_PWR_ClearFlag_UD
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_UD(void)
{
  WRITE_REG(PWR->CSR1, PWR_CSR1_UDRDY);
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

#endif /* __STM32F7xx_LL_PWR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
