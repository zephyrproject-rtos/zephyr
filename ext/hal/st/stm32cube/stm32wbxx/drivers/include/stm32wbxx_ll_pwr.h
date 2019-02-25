/**
  ******************************************************************************
  * @file    stm32wbxx_ll_pwr.h
  * @author  MCD Application Team
  * @brief   Header file of PWR LL module.
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
#ifndef STM32WBxx_LL_PWR_H
#define STM32WBxx_LL_PWR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx.h"

/** @addtogroup STM32WBxx_LL_Driver
  * @{
  */

#if defined(PWR)

/** @defgroup PWR_LL PWR
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
/** @defgroup PWR_LL_Private_Constants PWR Private Constants
  * @{
  */
/** @defgroup PWR_SMPS_Calibration PWR SMPS calibration
  * @{
  */
#define SMPS_VOLTAGE_CAL_ADDR              ((uint32_t*) (0x1FFF7558UL))         /* SMPS output voltage calibration level corresponding to voltage "SMPS_VOLTAGE_CAL_VOLTAGE_MV" */
#define SMPS_VOLTAGE_CAL_POS               (8UL)                                /* SMPS output voltage calibration level bitfield position */
#define SMPS_VOLTAGE_CAL                   (0xFUL << SMPS_VOLTAGE_CAL_POS)      /* SMPS output voltage calibration level bitfield mask */
#define SMPS_VOLTAGE_CAL_VOLTAGE_MV        (1500UL)                             /* SMPS output voltage calibration value (unit: mV) */
#define SMPS_VOLTAGE_BASE_MV               (1200UL)                             /* SMPS output voltage base value (unit: mV) */
#define SMPS_VOLTAGE_STEP_MV               (  50UL)                             /* SMPS output voltage step (unit: mV) */
/**
  * @}
  */

/**
  * @}
  */

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
#define LL_PWR_SCR_CWUF                    PWR_SCR_CWUF
#define LL_PWR_SCR_CWUF5                   PWR_SCR_CWUF5
#define LL_PWR_SCR_CWUF4                   PWR_SCR_CWUF4
#define LL_PWR_SCR_CWUF3                   PWR_SCR_CWUF3
#define LL_PWR_SCR_CWUF2                   PWR_SCR_CWUF2
#define LL_PWR_SCR_CWUF1                   PWR_SCR_CWUF1
#define LL_PWR_SCR_CC2HF                   PWR_SCR_CC2HF
#define LL_PWR_SCR_C802AF                  PWR_SCR_C802AF
#define LL_PWR_SCR_CBLEAF                  PWR_SCR_CBLEAF
#define LL_PWR_SCR_CCRPEF                  PWR_SCR_CCRPEF
#define LL_PWR_SCR_C802WUF                 PWR_SCR_C802WUF
#define LL_PWR_SCR_CBLEWUF                 PWR_SCR_CBLEWUF
#define LL_PWR_SCR_CBORHF                  PWR_SCR_CBORHF
#define LL_PWR_SCR_CSMPSFBF                PWR_SCR_CSMPSFBF
#define LL_PWR_EXTSCR_CCRPF                PWR_EXTSCR_CCRPF
#define LL_PWR_EXTSCR_C2CSSF               PWR_EXTSCR_C2CSSF
#define LL_PWR_EXTSCR_C1CSSF               PWR_EXTSCR_C1CSSF
/**
  * @}
  */

/** @defgroup PWR_LL_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_PWR_ReadReg function
  * @{
  */
#define LL_PWR_SR1_WUFI                    PWR_SR1_WUFI
#define LL_PWR_SR1_WUF5                    PWR_SR1_WUF5
#define LL_PWR_SR1_WUF4                    PWR_SR1_WUF4
#define LL_PWR_SR1_WUF3                    PWR_SR1_WUF3
#define LL_PWR_SR1_WUF2                    PWR_SR1_WUF2
#define LL_PWR_SR1_WUF1                    PWR_SR1_WUF1
#define LL_PWR_SR2_PVMO3                   PWR_SR2_PVMO3
#define LL_PWR_SR2_PVMO1                   PWR_SR2_PVMO1
#define LL_PWR_SR2_PVDO                    PWR_SR2_PVDO
#define LL_PWR_SR2_VOSF                    PWR_SR2_VOSF
#define LL_PWR_SR2_REGLPF                  PWR_SR2_REGLPF
#define LL_PWR_SR2_REGLPS                  PWR_SR2_REGLPS

/* BOR flags */
#define LL_PWR_FLAG_BORH                   PWR_SR1_BORHF  /* BORH interrupt flag */

/* SMPS flags */
#define LL_PWR_FLAG_SMPS                   PWR_SR2_SMPSF  /* SMPS step down converter ready flag */
#define LL_PWR_FLAG_SMPSB                  PWR_SR2_SMPSBF /* SMPS step down converter in bypass mode flag */
#define LL_PWR_FLAG_SMPSFB                 PWR_SR1_SMPSFB /* SMPS step down converter forced in bypass mode interrupt flag */

/* Radio (BLE or 802.15.4) flags */
#define LL_PWR_FLAG_BLEWU                  PWR_SR1_BLEWUF  /* BLE wakeup interrupt flag */
#define LL_PWR_FLAG_802WU                  PWR_SR1_802WUF  /* 802.15.4 wakeup interrupt flag */
#define LL_PWR_FLAG_BLEA                   PWR_SR1_BLEAF   /* BLE end of activity interrupt flag */
#define LL_PWR_FLAG_802A                   PWR_SR1_802AF   /* 802.15.4 end of activity interrupt flag */
#define LL_PWR_FLAG_CRPE                   PWR_SR1_CRPEF   /* Critical radio phase end of activity interrupt flag */
#define LL_PWR_FLAG_CRP                    PWR_EXTSCR_CRPF /* Critical radio system phase */

/* Multicore flags */
#define LL_PWR_EXTSCR_C1SBF                PWR_EXTSCR_C1SBF   /* System standby flag for CPU1 */
#define LL_PWR_EXTSCR_C1STOPF              PWR_EXTSCR_C1STOPF /* System stop flag for CPU1 */
#define LL_PWR_EXTSCR_C1DS                 PWR_EXTSCR_C1DS    /* CPU1 deepsleep mode */
#define LL_PWR_EXTSCR_C2SBF                PWR_EXTSCR_C2SBF   /* System standby flag for CPU2 */
#define LL_PWR_EXTSCR_C2STOPF              PWR_EXTSCR_C2STOPF /* System stop flag for CPU2 */
#define LL_PWR_EXTSCR_C2DS                 PWR_EXTSCR_C2DS    /* CPU2 deepsleep mode */
#define LL_PWR_SR1_C2HF                    PWR_SR1_C2HF       /* CPU2 hold interrupt flag */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_REGU_VOLTAGE REGU VOLTAGE
  * @{
  */
#define LL_PWR_REGU_VOLTAGE_SCALE1         (PWR_CR1_VOS_0) /* Regulator voltage output range 1 mode, typical output voltage at 1.2 V, system frequency up to 64 MHz. */
#define LL_PWR_REGU_VOLTAGE_SCALE2         (PWR_CR1_VOS_1) /* Regulator voltage output range 2 mode, typical output voltage at 1.0 V, system frequency up to 16 MHz. */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_MODE_PWR MODE PWR
  * @{
  */
#define LL_PWR_MODE_STOP0                  (0x000000000U)
#define LL_PWR_MODE_STOP1                  (PWR_CR1_LPMS_0)
#define LL_PWR_MODE_STOP2                  (PWR_CR1_LPMS_1)
#define LL_PWR_MODE_STANDBY                (PWR_CR1_LPMS_1 | PWR_CR1_LPMS_0)
#define LL_PWR_MODE_SHUTDOWN               (PWR_CR1_LPMS_2)
/**
  * @}
  */

/** @defgroup PWR_LL_EC_FLASH_LPRUN_POWER_DOWN_MODE Flash power-down mode during low-power run mode
  * @{
  */
#define LL_PWR_FLASH_LPRUN_MODE_IDLE       (0x000000000U)
#define LL_PWR_FLASH_LPRUN_MODE_POWER_DOWN (PWR_CR1_FPDR)
/**
  * @}
  */

/** @defgroup PWR_LL_EC_FLASH_SLEEP_POWER_DOWN_MODE Flash power-down mode during sleep mode
  * @{
  */
#define LL_PWR_FLASH_SLEEP_MODE_IDLE       (0x000000000U)
#define LL_PWR_FLASH_SLEEP_MODE_POWER_DOWN (PWR_CR1_FPDS)
/**
  * @}
  */

/** @defgroup PWR_LL_EC_PVM_VDDUSB_1 Peripheral voltage monitoring
  * @{
  */
#define LL_PWR_PVM_VDDUSB_1_2V             (PWR_CR2_PVME1)     /* Monitoring VDDUSB vs. 1.2V */
#define LL_PWR_PVM_VDDA_1_62V              (PWR_CR2_PVME3)     /* Monitoring VDDA vs. 1.62V  */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_PVDLEVEL PVDLEVEL
  * @{
  */
#define LL_PWR_PVDLEVEL_0                  (0x00000000U)                                   /* VPVD0 around 2.0 V */
#define LL_PWR_PVDLEVEL_1                  (PWR_CR2_PLS_0)                                 /* VPVD1 around 2.2 V */
#define LL_PWR_PVDLEVEL_2                  (PWR_CR2_PLS_1)                                 /* VPVD2 around 2.4 V */
#define LL_PWR_PVDLEVEL_3                  (PWR_CR2_PLS_1 | PWR_CR2_PLS_0)                 /* VPVD3 around 2.5 V */
#define LL_PWR_PVDLEVEL_4                  (PWR_CR2_PLS_2)                                 /* VPVD4 around 2.6 V */
#define LL_PWR_PVDLEVEL_5                  (PWR_CR2_PLS_2 | PWR_CR2_PLS_0)                 /* VPVD5 around 2.8 V */
#define LL_PWR_PVDLEVEL_6                  (PWR_CR2_PLS_2 | PWR_CR2_PLS_1)                 /* VPVD6 around 2.9 V */
#define LL_PWR_PVDLEVEL_7                  (PWR_CR2_PLS_2 | PWR_CR2_PLS_1 | PWR_CR2_PLS_0) /* External input analog voltage   (Compare internally to VREFINT) */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_WAKEUP WAKEUP
  * @{
  */
#define LL_PWR_WAKEUP_PIN1                 (PWR_CR3_EWUP1)
#define LL_PWR_WAKEUP_PIN2                 (PWR_CR3_EWUP2)
#define LL_PWR_WAKEUP_PIN3                 (PWR_CR3_EWUP3)
#define LL_PWR_WAKEUP_PIN4                 (PWR_CR3_EWUP4)
#define LL_PWR_WAKEUP_PIN5                 (PWR_CR3_EWUP5)
/**
  * @}
  */

/** @defgroup PWR_LL_EC_BATT_CHARG_RESISTOR BATT CHARG RESISTOR
  * @{
  */
#define LL_PWR_BATT_CHARG_RESISTOR_5K      (0x00000000U)
#define LL_PWR_BATT_CHARGRESISTOR_1_5K     (PWR_CR4_VBRS)
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
#define LL_PWR_GPIO_E                      ((uint32_t)(&(PWR->PUCRE)))
#define LL_PWR_GPIO_H                      ((uint32_t)(&(PWR->PUCRH)))
/**
  * @}
  */

/** @defgroup PWR_LL_EC_GPIO_BIT GPIO BIT
  * @{
  */
/* Note: LL_PWR_GPIO_BIT_x defined from port C because all pins are available */
/*       for PWR pull-up and pull-down.                                       */
#define LL_PWR_GPIO_BIT_0                  (PWR_PUCRC_PC0)
#define LL_PWR_GPIO_BIT_1                  (PWR_PUCRC_PC1)
#define LL_PWR_GPIO_BIT_2                  (PWR_PUCRC_PC2)
#define LL_PWR_GPIO_BIT_3                  (PWR_PUCRC_PC3)
#define LL_PWR_GPIO_BIT_4                  (PWR_PUCRC_PC4)
#define LL_PWR_GPIO_BIT_5                  (PWR_PUCRC_PC5)
#define LL_PWR_GPIO_BIT_6                  (PWR_PUCRC_PC6)
#define LL_PWR_GPIO_BIT_7                  (PWR_PUCRC_PC7)
#define LL_PWR_GPIO_BIT_8                  (PWR_PUCRC_PC8)
#define LL_PWR_GPIO_BIT_9                  (PWR_PUCRC_PC9)
#define LL_PWR_GPIO_BIT_10                 (PWR_PUCRC_PC10)
#define LL_PWR_GPIO_BIT_11                 (PWR_PUCRC_PC11)
#define LL_PWR_GPIO_BIT_12                 (PWR_PUCRC_PC12)
#define LL_PWR_GPIO_BIT_13                 (PWR_PUCRC_PC13)
#define LL_PWR_GPIO_BIT_14                 (PWR_PUCRC_PC14)
#define LL_PWR_GPIO_BIT_15                 (PWR_PUCRC_PC15)
/**
  * @}
  */

/** @defgroup PWR_LL_EC_BOR_CONFIGURATION BOR configuration
  * @{
  */
#define LL_PWR_BOR_SYSTEM_RESET            (0x00000000U)     /*!< BOR will generate a system reset  */
#define LL_PWR_BOR_SMPS_FORCE_BYPASS       (PWR_CR5_BORHC)   /*!< BOR will for SMPS step down converter in bypass mode */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_SMPS_OPERATING_MODES SMPS step down converter operating modes
  * @{
  */
/* Note: Literals values are defined from register SR2 bits SMPSF and SMPSBF  */
/*       but they are also used as register CR5 bits SMPSEN and SMPSBEN,      */
/*       as used by all SMPS operating mode functions targetting different    */
/*       registers:                                                           */
/*       "LL_PWR_SMPS_SetMode()", "LL_PWR_SMPS_GetMode()"                     */
/*       and "LL_PWR_SMPS_GetEffectiveMode()".                                */
#define LL_PWR_SMPS_BYPASS                 (PWR_SR2_SMPSBF) /*!< SMPS step down in bypass mode. */
#define LL_PWR_SMPS_STEP_DOWN              (PWR_SR2_SMPSF)  /*!< SMPS step down in step down mode if system low power mode is run, LP run or stop0. If system low power mode is stop1, stop2, standby, shutdown, then SMPS is forced in mode open to preserve energy stored in decoupling capacitor as long as possible. */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_SMPS_STARTUP_CURRENT SMPS step down converter supply startup current selection
  * @{
  */
#define LL_PWR_SMPS_STARTUP_CURRENT_80MA   (0x00000000U)                                            /*!< SMPS step down converter supply startup current 80mA */
#define LL_PWR_SMPS_STARTUP_CURRENT_100MA  (                                      PWR_CR5_SMPSSC_0) /*!< SMPS step down converter supply startup current 100mA */
#define LL_PWR_SMPS_STARTUP_CURRENT_120MA  (                   PWR_CR5_SMPSSC_1                   ) /*!< SMPS step down converter supply startup current 120mA */
#define LL_PWR_SMPS_STARTUP_CURRENT_140MA  (                   PWR_CR5_SMPSSC_1 | PWR_CR5_SMPSSC_0) /*!< SMPS step down converter supply startup current 140mA */
#define LL_PWR_SMPS_STARTUP_CURRENT_160MA  (PWR_CR5_SMPSSC_2                                      ) /*!< SMPS step down converter supply startup current 160mA */
#define LL_PWR_SMPS_STARTUP_CURRENT_180MA  (PWR_CR5_SMPSSC_2 |                    PWR_CR5_SMPSSC_0) /*!< SMPS step down converter supply startup current 180mA */
#define LL_PWR_SMPS_STARTUP_CURRENT_200MA  (PWR_CR5_SMPSSC_2 | PWR_CR5_SMPSSC_1                   ) /*!< SMPS step down converter supply startup current 200mA */
#define LL_PWR_SMPS_STARTUP_CURRENT_220MA  (PWR_CR5_SMPSSC_2 | PWR_CR5_SMPSSC_1 | PWR_CR5_SMPSSC_0) /*!< SMPS step down converter supply startup current 220mA */
/**
  * @}
  */

/** @defgroup PWR_LL_EC_SMPS_OUTPUT_VOLTAGE_LEVEL SMPS step down converter output voltage scaling voltage level
  * @{
  */
/* Note: SMPS voltage is trimmed during device production to control
         the actual voltage level variation from device to device. */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V20  (0x00000000U)                                                                   /*!< SMPS step down converter supply output voltage 1.20V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V25  (                                                            PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.25V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V30  (                                        PWR_CR5_SMPSVOS_1                    ) /*!< SMPS step down converter supply output voltage 1.30V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V35  (                                        PWR_CR5_SMPSVOS_1 | PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.35V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V40  (                    PWR_CR5_SMPSVOS_2                                        ) /*!< SMPS step down converter supply output voltage 1.40V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V45  (                    PWR_CR5_SMPSVOS_2 |                     PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.45V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V50  (                    PWR_CR5_SMPSVOS_2 | PWR_CR5_SMPSVOS_1                    ) /*!< SMPS step down converter supply output voltage 1.50V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V55  (                    PWR_CR5_SMPSVOS_2 | PWR_CR5_SMPSVOS_1 | PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.55V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V60  (PWR_CR5_SMPSVOS_3                                                            ) /*!< SMPS step down converter supply output voltage 1.60V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V65  (PWR_CR5_SMPSVOS_3 |                                         PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.65V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V70  (PWR_CR5_SMPSVOS_3 |                     PWR_CR5_SMPSVOS_1                    ) /*!< SMPS step down converter supply output voltage 1.70V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V75  (PWR_CR5_SMPSVOS_3 |                     PWR_CR5_SMPSVOS_1 | PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.75V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V80  (PWR_CR5_SMPSVOS_3 | PWR_CR5_SMPSVOS_2                                        ) /*!< SMPS step down converter supply output voltage 1.80V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V85  (PWR_CR5_SMPSVOS_3 | PWR_CR5_SMPSVOS_2 |                     PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.85V */
#define LL_PWR_SMPS_OUTPUT_VOLTAGE_1V90  (PWR_CR5_SMPSVOS_3 | PWR_CR5_SMPSVOS_2 | PWR_CR5_SMPSVOS_1                    ) /*!< SMPS step down converter supply output voltage 1.90V */
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
  * @brief  Switch from run main mode to run low-power mode.
  * @rmtoll CR1          LPR           LL_PWR_EnterLowPowerRunMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnterLowPowerRunMode(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_LPR);
}

/**
  * @brief  Switch from run main mode to low-power mode.
  * @rmtoll CR1          LPR           LL_PWR_ExitLowPowerRunMode
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ExitLowPowerRunMode(void)
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
  * @brief  Set the main internal regulator output voltage
  * @note   A delay is required for the internal regulator to be ready
  *         after the voltage scaling has been changed.
  *         Check whether regulator reached the selected voltage level
  *         can be done using function @ref LL_PWR_IsActiveFlag_VOS().
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
  return (uint32_t)(READ_BIT(PWR->CR1, PWR_CR1_VOS));
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
  * @brief  Set Low-Power mode
  * @rmtoll CR1          LPMS          LL_PWR_SetPowerMode
  * @param  LowPowerMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_MODE_STOP0
  *         @arg @ref LL_PWR_MODE_STOP1
  *         @arg @ref LL_PWR_MODE_STOP2
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
  *         @arg @ref LL_PWR_MODE_STOP2
  *         @arg @ref LL_PWR_MODE_STANDBY
  *         @arg @ref LL_PWR_MODE_SHUTDOWN
  */
__STATIC_INLINE uint32_t LL_PWR_GetPowerMode(void)
{
  return (uint32_t)(READ_BIT(PWR->CR1, PWR_CR1_LPMS));
}

/**
  * @brief  Set flash power-down mode during low-power run mode
  * @rmtoll CR1          FPDR          LL_PWR_SetFlashPowerModeLPRun
  * @param  FlashLowPowerMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_FLASH_LPRUN_MODE_IDLE
  *         @arg @ref LL_PWR_FLASH_LPRUN_MODE_POWER_DOWN
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetFlashPowerModeLPRun(uint32_t FlashLowPowerMode)
{
  /* Unlock bit FPDR */
  WRITE_REG(PWR->CR1, 0x0000C1B0U);

  /* Update bit FPDR */
  MODIFY_REG(PWR->CR1, PWR_CR1_FPDR, FlashLowPowerMode);
}

/**
  * @brief  Get flash power-down mode during low-power run mode
  * @rmtoll CR1          FPDR          LL_PWR_GetFlashPowerModeLPRun
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_FLASH_LPRUN_MODE_IDLE
  *         @arg @ref LL_PWR_FLASH_LPRUN_MODE_POWER_DOWN
  */
__STATIC_INLINE uint32_t LL_PWR_GetFlashPowerModeLPRun(void)
{
  return (uint32_t)(READ_BIT(PWR->CR1, PWR_CR1_FPDR));
}

/**
  * @brief  Set flash power-down mode during sleep mode
  * @rmtoll CR1          FPDS          LL_PWR_SetFlashPowerModeSleep
  * @param  FlashLowPowerMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_FLASH_SLEEP_MODE_IDLE
  *         @arg @ref LL_PWR_FLASH_SLEEP_MODE_POWER_DOWN
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SetFlashPowerModeSleep(uint32_t FlashLowPowerMode)
{
  MODIFY_REG(PWR->CR1, PWR_CR1_FPDS, FlashLowPowerMode);
}

/**
  * @brief  Get flash power-down mode during sleep mode
  * @rmtoll CR1          FPDS          LL_PWR_GetFlashPowerModeSleep
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_FLASH_SLEEP_MODE_IDLE
  *         @arg @ref LL_PWR_FLASH_SLEEP_MODE_POWER_DOWN
  */
__STATIC_INLINE uint32_t LL_PWR_GetFlashPowerModeSleep(void)
{
  return (uint32_t)(READ_BIT(PWR->CR1, PWR_CR1_FPDS));
}

/**
  * @brief  Enable VDDUSB supply
  * @rmtoll CR2          USV           LL_PWR_EnableVddUSB
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableVddUSB(void)
{
  SET_BIT(PWR->CR2, PWR_CR2_USV);
}

/**
  * @brief  Disable VDDUSB supply
  * @rmtoll CR2          USV           LL_PWR_DisableVddUSB
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableVddUSB(void)
{
  CLEAR_BIT(PWR->CR2, PWR_CR2_USV);
}

/**
  * @brief  Check if VDDUSB supply is enabled
  * @rmtoll CR2          USV           LL_PWR_IsEnabledVddUSB
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledVddUSB(void)
{
  return ((READ_BIT(PWR->CR2, PWR_CR2_USV) == (PWR_CR2_USV)) ? 1UL : 0UL);
}


/**
  * @brief  Enable the Power Voltage Monitoring on a peripheral
  * @rmtoll CR2          PVME1         LL_PWR_EnablePVM\n
  *         CR2          PVME3         LL_PWR_EnablePVM
  * @param  PeriphVoltage This parameter can be one of the following values:
  *         @arg @ref LL_PWR_PVM_VDDUSB_1_2V
  *         @arg @ref LL_PWR_PVM_VDDA_1_62V
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnablePVM(uint32_t PeriphVoltage)
{
  SET_BIT(PWR->CR2, PeriphVoltage);
}

/**
  * @brief  Disable the Power Voltage Monitoring on a peripheral
  * @rmtoll CR2          PVME1         LL_PWR_DisablePVM\n
  *         CR2          PVME3         LL_PWR_DisablePVM
  * @param  PeriphVoltage This parameter can be one of the following values:
  *         @arg @ref LL_PWR_PVM_VDDUSB_1_2V
  *         @arg @ref LL_PWR_PVM_VDDA_1_62V
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisablePVM(uint32_t PeriphVoltage)
{
  CLEAR_BIT(PWR->CR2, PeriphVoltage);
}

/**
  * @brief  Check if Power Voltage Monitoring is enabled on a peripheral
  * @rmtoll CR2          PVME1         LL_PWR_IsEnabledPVM\n
  *         CR2          PVME3         LL_PWR_IsEnabledPVM
  * @param  PeriphVoltage This parameter can be one of the following values:
  *         @arg @ref LL_PWR_PVM_VDDUSB_1_2V
  *         @arg @ref LL_PWR_PVM_VDDA_1_62V
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledPVM(uint32_t PeriphVoltage)
{
  return ((READ_BIT(PWR->CR2, PeriphVoltage) == (PeriphVoltage)) ? 1UL : 0UL);
}

/**
  * @brief  Configure the voltage threshold detected by the Power Voltage Detector
  * @rmtoll CR2          PLS           LL_PWR_SetPVDLevel
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
  MODIFY_REG(PWR->CR2, PWR_CR2_PLS, PVDLevel);
}

/**
  * @brief  Get the voltage threshold detection
  * @rmtoll CR2          PLS           LL_PWR_GetPVDLevel
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
  return (uint32_t)(READ_BIT(PWR->CR2, PWR_CR2_PLS));
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
  * @brief  Check if pull-up and pull-down configuration is enabled
  * @rmtoll CR3          APC           LL_PWR_IsEnabledPUPDCfg
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledPUPDCfg(void)
{
  return ((READ_BIT(PWR->CR3, PWR_CR3_APC) == (PWR_CR3_APC)) ? 1UL : 0UL);
}

/**
  * @brief  Enable SRAM2 content retention in Standby mode
  * @rmtoll CR3          RRS           LL_PWR_EnableSRAM2Retention
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableSRAM2Retention(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_RRS);
}

/**
  * @brief  Disable SRAM2 content retention in Standby mode
  * @rmtoll CR3          RRS           LL_PWR_DisableSRAM2Retention
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableSRAM2Retention(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_RRS);
}

/**
  * @brief  Check if SRAM2 content retention in Standby mode  is enabled
  * @rmtoll CR3          RRS           LL_PWR_IsEnabledSRAM2Retention
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledSRAM2Retention(void)
{
  return ((READ_BIT(PWR->CR3, PWR_CR3_RRS) == (PWR_CR3_RRS)) ? 1UL : 0UL);
}

/**
  * @brief  Enable the WakeUp PINx functionality
  * @rmtoll CR3          EWUP1         LL_PWR_EnableWakeUpPin\n
  *         CR3          EWUP2         LL_PWR_EnableWakeUpPin\n
  *         CR3          EWUP3         LL_PWR_EnableWakeUpPin\n
  *         CR3          EWUP4         LL_PWR_EnableWakeUpPin\n
  *         CR3          EWUP5         LL_PWR_EnableWakeUpPin\n
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
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
  *         CR3          EWUP3         LL_PWR_DisableWakeUpPin\n
  *         CR3          EWUP4         LL_PWR_DisableWakeUpPin\n
  *         CR3          EWUP5         LL_PWR_DisableWakeUpPin\n
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
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
  *         CR3          EWUP3         LL_PWR_IsEnabledWakeUpPin\n
  *         CR3          EWUP4         LL_PWR_IsEnabledWakeUpPin\n
  *         CR3          EWUP5         LL_PWR_IsEnabledWakeUpPin\n
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
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
  *         @arg @ref LL_PWR_BATT_CHARG_RESISTOR_5K
  *         @arg @ref LL_PWR_BATT_CHARGRESISTOR_1_5K
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
  *         @arg @ref LL_PWR_BATT_CHARG_RESISTOR_5K
  *         @arg @ref LL_PWR_BATT_CHARGRESISTOR_1_5K
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
  *         CR4          WP3           LL_PWR_SetWakeUpPinPolarityLow\n
  *         CR4          WP4           LL_PWR_SetWakeUpPinPolarityLow\n
  *         CR4          WP5           LL_PWR_SetWakeUpPinPolarityLow
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
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
  *         CR4          WP3           LL_PWR_SetWakeUpPinPolarityHigh\n
  *         CR4          WP4           LL_PWR_SetWakeUpPinPolarityHigh\n
  *         CR4          WP5           LL_PWR_SetWakeUpPinPolarityHigh
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
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
  *         CR4          WP3           LL_PWR_IsWakeUpPinPolarityLow\n
  *         CR4          WP4           LL_PWR_IsWakeUpPinPolarityLow\n
  *         CR4          WP5           LL_PWR_IsWakeUpPinPolarityLow
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsWakeUpPinPolarityLow(uint32_t WakeUpPin)
{
  return ((READ_BIT(PWR->CR4, WakeUpPin) == (WakeUpPin)) ? 1UL : 0UL);
}

/**
  * @brief  Enable GPIO pull-up state in Standby and Shutdown modes
  * @note   Some pins are not configurable for pulling in Standby and Shutdown
  *         modes. Refer to reference manual for available pins and ports.
  * @rmtoll PUCRA        PU0-15        LL_PWR_EnableGPIOPullUp\n
  *         PUCRB        PU0-15        LL_PWR_EnableGPIOPullUp\n
  *         PUCRC        PU0-15        LL_PWR_EnableGPIOPullUp\n
  *         PUCRD        PU0-15        LL_PWR_EnableGPIOPullUp\n
  *         PUCRE        PU0-15        LL_PWR_EnableGPIOPullUp\n
  *         PUCRH        PU0-15        LL_PWR_EnableGPIOPullUp
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_E
  *         @arg @ref LL_PWR_GPIO_H
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
  * @note   Some pins are not configurable for pulling in Standby and Shutdown
  *         modes. Refer to reference manual for available pins and ports.
  * @rmtoll PUCRA        PU0-15        LL_PWR_DisableGPIOPullUp\n
  *         PUCRB        PU0-15        LL_PWR_DisableGPIOPullUp\n
  *         PUCRC        PU0-15        LL_PWR_DisableGPIOPullUp\n
  *         PUCRD        PU0-15        LL_PWR_DisableGPIOPullUp\n
  *         PUCRE        PU0-15        LL_PWR_DisableGPIOPullUp\n
  *         PUCRH        PU0-15        LL_PWR_DisableGPIOPullUp
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_E
  *         @arg @ref LL_PWR_GPIO_H
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
  *         PUCRE        PU0-15        LL_PWR_IsEnabledGPIOPullUp\n
  *         PUCRH        PU0-15        LL_PWR_IsEnabledGPIOPullUp
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_E
  *         @arg @ref LL_PWR_GPIO_H
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
  * @note   Some pins are not configurable for pulling in Standby and Shutdown
  *         modes. Refer to reference manual for available pins and ports.
  * @rmtoll PDCRA        PD0-15        LL_PWR_EnableGPIOPullDown\n
  *         PDCRB        PD0-15        LL_PWR_EnableGPIOPullDown\n
  *         PDCRC        PD0-15        LL_PWR_EnableGPIOPullDown\n
  *         PDCRD        PD0-15        LL_PWR_EnableGPIOPullDown\n
  *         PDCRE        PD0-15        LL_PWR_EnableGPIOPullDown\n
  *         PDCRH        PD0-15        LL_PWR_EnableGPIOPullDown
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_E
  *         @arg @ref LL_PWR_GPIO_H
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
  register __IO uint32_t temp = (uint32_t)(GPIO) + 4UL;
  SET_BIT(*((uint32_t *)(temp)), GPIONumber);
}

/**
  * @brief  Disable GPIO pull-down state in Standby and Shutdown modes
  * @note   Some pins are not configurable for pulling in Standby and Shutdown
  *         modes. Refer to reference manual for available pins and ports.
  * @rmtoll PDCRA        PD0-15        LL_PWR_DisableGPIOPullDown\n
  *         PDCRB        PD0-15        LL_PWR_DisableGPIOPullDown\n
  *         PDCRC        PD0-15        LL_PWR_DisableGPIOPullDown\n
  *         PDCRD        PD0-15        LL_PWR_DisableGPIOPullDown\n
  *         PDCRE        PD0-15        LL_PWR_DisableGPIOPullDown\n
  *         PDCRH        PD0-15        LL_PWR_DisableGPIOPullDown
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_E
  *         @arg @ref LL_PWR_GPIO_H
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
  register __IO uint32_t temp = (uint32_t)(GPIO) + 4UL;
  CLEAR_BIT(*((uint32_t *)(temp)), GPIONumber);
}

/**
  * @brief  Check if GPIO pull-down state is enabled
  * @rmtoll PDCRA        PD0-15        LL_PWR_IsEnabledGPIOPullDown\n
  *         PDCRB        PD0-15        LL_PWR_IsEnabledGPIOPullDown\n
  *         PDCRC        PD0-15        LL_PWR_IsEnabledGPIOPullDown\n
  *         PDCRD        PD0-15        LL_PWR_IsEnabledGPIOPullDown\n
  *         PDCRE        PD0-15        LL_PWR_IsEnabledGPIOPullDown\n
  *         PDCRH        PD0-15        LL_PWR_IsEnabledGPIOPullDown
  * @param  GPIO This parameter can be one of the following values:
  *         @arg @ref LL_PWR_GPIO_A
  *         @arg @ref LL_PWR_GPIO_B
  *         @arg @ref LL_PWR_GPIO_C
  *         @arg @ref LL_PWR_GPIO_D
  *         @arg @ref LL_PWR_GPIO_E
  *         @arg @ref LL_PWR_GPIO_H
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
  register __IO uint32_t temp = (uint32_t)(GPIO) + 4UL;
  return ((READ_BIT(*((uint32_t *)(temp)), GPIONumber) == (GPIONumber)) ? 1UL : 0UL);
}

/**
  * @brief  Set BOR configuration
  * @rmtoll CR5          BORHC         LL_PWR_SetBORConfig
  * @param  BORConfiguration This parameter can be one of the following values:
  *         @arg @ref LL_PWR_BOR_SYSTEM_RESET
  *         @arg @ref LL_PWR_BOR_SMPS_FORCE_BYPASS
  */
__STATIC_INLINE void LL_PWR_SetBORConfig(uint32_t BORConfiguration)
{
  MODIFY_REG(PWR->CR5, PWR_CR5_BORHC, BORConfiguration);
}

/**
  * @brief  Get BOR configuration
  * @rmtoll CR5          BORHC         LL_PWR_GetBORConfig
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_BOR_SYSTEM_RESET
  *         @arg @ref LL_PWR_BOR_SMPS_FORCE_BYPASS
  */
__STATIC_INLINE uint32_t LL_PWR_GetBORConfig(void)
{
  return (uint32_t)(READ_BIT(PWR->CR5, PWR_CR5_BORHC));
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_Configuration_SMPS Configuration of SMPS
  * @{
  */

/**
  * @brief  Set SMPS operating mode
  * @note   When SMPS step down converter SMPS mode is enabled,
  *         it is good practice to enable the BORH to monitor the supply:
  *         in this case, when the supply drops below the SMPS step down
  *         converter SMPS mode operating supply level,
  *         switching on the fly is performed automaticcaly
  *         and interruption is generated.
  *         Refer to function @ref LL_PWR_SetBORConfig().
  * @note   Occurence of SMPS step down converter forced in bypass mode
  *         can be monitored by flag and interruption.
  *         Refer to functions
  *         @ref LL_PWR_IsActiveFlag_SMPSFB(), @ref LL_PWR_ClearFlag_SMPSFB(),
  *         @ref LL_PWR_EnableIT_BORH_SMPSFB().
  * @rmtoll CR5          SMPSEN        LL_PWR_SMPS_SetMode \n
  *         CR5          SMPSBEN       LL_PWR_SMPS_SetMode
  * @param  OperatingMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_SMPS_BYPASS
  *         @arg @ref LL_PWR_SMPS_STEP_DOWN (1)
  *
  *         (1) SMPS operating mode step down or open depends on system low-power mode:
  *              - step down mode if system low power mode is run, LP run or stop0,
  *              - open mode if system low power mode is stop1, stop2, standby or shutdown
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SMPS_SetMode(uint32_t OperatingMode)
{
  /* Note: Operation on bits performed to keep compatibility of literals      */
  /*       for all SMPS operating mode functions:                             */
  /*       "LL_PWR_SMPS_SetMode()", "LL_PWR_SMPS_GetMode()"                   */
  /*       and "LL_PWR_SMPS_GetEffectiveMode()".                              */
  MODIFY_REG(PWR->CR5, PWR_CR5_SMPSEN, (OperatingMode & PWR_SR2_SMPSF) << (PWR_CR5_SMPSEN_Pos - PWR_SR2_SMPSF_Pos));
}

/**
  * @brief  Get SMPS operating mode
  * @rmtoll CR5          SMPSEN        LL_PWR_SMPS_GetMode \n
  *         CR5          SMPSBEN       LL_PWR_SMPS_GetMode
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_SMPS_BYPASS
  *         @arg @ref LL_PWR_SMPS_STEP_DOWN (1)
  *
  *         (1) SMPS operating mode step down or open depends on system low-power mode:
  *              - step down mode if system low power mode is run, LP run or stop0,
  *              - open mode if system low power mode is stop1, stop2, standby or shutdown
  */
__STATIC_INLINE uint32_t LL_PWR_SMPS_GetMode(void)
{
  /* Note: Operation on bits performed to keep compatibility of literals      */
  /*       for all SMPS operating mode functions:                             */
  /*       "LL_PWR_SMPS_SetMode()", "LL_PWR_SMPS_GetMode()"                   */
  /*       and "LL_PWR_SMPS_GetEffectiveMode()".                              */
  register uint32_t OperatingMode = (READ_BIT(PWR->CR5, PWR_CR5_SMPSEN) >> (PWR_CR5_SMPSEN_Pos - PWR_SR2_SMPSF_Pos));

  OperatingMode = (OperatingMode | ((~OperatingMode >> 1U) & PWR_SR2_SMPSBF));

  return OperatingMode;
}

/**
  * @brief  Get SMPS effective operating mode
  * @note   SMPS operating mode can be changed by hardware, therefore
  *         requested operating mode can differ from effective low power mode.
  *         - dependency on system low-power mode:
  *           - step down mode if system low power mode is run, LP run or stop0,
  *           - open mode if system low power mode is stop1, stop2, standby or shutdown
  *         - dependency on BOR level:
  *           - bypass mode if supply voltage drops below BOR level
  * @note   This functions check flags of SMPS operating modes step down
  *         and bypass. If the SMPS is not among these 2 operating modes,
  *         then it can be in mode off or open.
  * @rmtoll SR2          SMPSF         LL_PWR_SMPS_GetEffectiveMode \n
  *         SR2          SMPSBF        LL_PWR_SMPS_GetEffectiveMode
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_SMPS_BYPASS
  *         @arg @ref LL_PWR_SMPS_STEP_DOWN (1)
  *
  *         (1) SMPS operating mode step down or open depends on system low-power mode:
  *              - step down mode if system low power mode is run, LP run or stop0,
  *              - open mode if system low power mode is stop1, stop2, standby or shutdown
  */
__STATIC_INLINE uint32_t LL_PWR_SMPS_GetEffectiveMode(void)
{
  return (uint32_t)(READ_BIT(PWR->SR2, (PWR_SR2_SMPSF | PWR_SR2_SMPSBF)));
}

/**
  * @brief  SMPS step down converter enable
  * @note   This function can be used for specific usage of the SMPS,
  *         for general usage of the SMPS the function
  *         @ref LL_PWR_SMPS_SetMode() should be used instead.
  * @rmtoll CR5          SMPSEN        LL_PWR_SMPS_Enable
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SMPS_Enable(void)
{
  SET_BIT(PWR->CR5, PWR_CR5_SMPSEN);
}

/**
  * @brief  SMPS step down converter enable
  * @note   This function can be used for specific usage of the SMPS,
  *         for general usage of the SMPS the function
  *         @ref LL_PWR_SMPS_SetMode() should be used instead.
  * @rmtoll CR5          SMPSEN        LL_PWR_SMPS_Disable
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SMPS_Disable(void)
{
  CLEAR_BIT(PWR->CR5, PWR_CR5_SMPSEN);
}

/**
  * @brief  Check if the SMPS step down converter is enabled
  * @rmtoll CR5          SMPSEN        LL_PWR_SMPS_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_SMPS_IsEnabled(void)
{
  return ((READ_BIT(PWR->CR5, PWR_CR5_SMPSEN) == (PWR_CR5_SMPSEN)) ? 1UL : 0UL);
}

/**
  * @brief  Set SMPS step down converter supply startup current selection
  * @rmtoll CR5          SMPSSC        LL_PWR_SMPS_SetStartupCurrent
  * @param  StartupCurrent This parameter can be one of the following values:
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_80MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_100MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_120MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_140MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_160MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_180MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_200MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_220MA
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SMPS_SetStartupCurrent(uint32_t StartupCurrent)
{
  MODIFY_REG(PWR->CR5, PWR_CR5_SMPSSC, StartupCurrent);
}

/**
  * @brief  Get SMPS step down converter supply startup current selection
  * @rmtoll CR5          SMPSSC        LL_PWR_SMPS_GetStartupCurrent
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_80MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_100MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_120MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_140MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_160MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_180MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_200MA
  *         @arg @ref LL_PWR_SMPS_STARTUP_CURRENT_220MA
  */
__STATIC_INLINE uint32_t LL_PWR_SMPS_GetStartupCurrent(void)
{
  return (uint32_t)(READ_BIT(PWR->CR5, PWR_CR5_SMPSSC));
}

/**
  * @brief  Set SMPS step down converter output voltage scaling
  * @note   SMPS output voltage is calibrated in production,
  *         calibration parameters are applied to the voltage level parameter
  *         to reach the requested voltage value.
  * @rmtoll CR5          SMPSVOS       LL_PWR_SMPS_SetOutputVoltageLevel
  * @param  OutputVoltageLevel This parameter can be one of the following values:
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V20
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V25
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V30
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V35
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V40
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V45
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V50
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V55
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V60
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V65
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V70
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V75
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V80
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V85
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V90
  * @retval None
  */
__STATIC_INLINE void LL_PWR_SMPS_SetOutputVoltageLevel(uint32_t OutputVoltageLevel)
{
  register __IO const uint32_t OutputVoltageLevel_calibration = (((*SMPS_VOLTAGE_CAL_ADDR) & SMPS_VOLTAGE_CAL) >> SMPS_VOLTAGE_CAL_POS);  /* SMPS output voltage level calibrated in production */
  register int32_t TrimmingSteps;                               /* Trimming steps between theorical output voltage and calibrated output voltage */
  register int32_t OutputVoltageLevelTrimmed;                   /* SMPS output voltage level after calibration: trimming value added to required level */

  if(OutputVoltageLevel_calibration == 0UL)
  {
    /* Device with SMPS output voltage not calibrated in production: Apply output voltage value directly */

    /* Update register */
    MODIFY_REG(PWR->CR5, PWR_CR5_SMPSVOS, OutputVoltageLevel);
  }
  else
  {
    /* Device with SMPS output voltage calibrated in production: Apply output voltage value after correction by calibration value */

    TrimmingSteps = ((int32_t)OutputVoltageLevel_calibration - (int32_t)(LL_PWR_SMPS_OUTPUT_VOLTAGE_1V50 >> PWR_CR5_SMPSVOS_Pos));
    OutputVoltageLevelTrimmed = ((int32_t)((uint32_t)(OutputVoltageLevel >> PWR_CR5_SMPSVOS_Pos)) + (int32_t)TrimmingSteps);

    /* Clamp value to voltage trimming bitfield range */
    if(OutputVoltageLevelTrimmed < 0)
    {
      OutputVoltageLevelTrimmed = 0;
    }
    else if(OutputVoltageLevelTrimmed > (int32_t)PWR_CR5_SMPSVOS)
    {
      OutputVoltageLevelTrimmed = (int32_t)PWR_CR5_SMPSVOS;
    }
    else
    {
    }

    /* Update register */
    MODIFY_REG(PWR->CR5, PWR_CR5_SMPSVOS, (uint32_t)OutputVoltageLevelTrimmed);
  }
}

/**
  * @brief  Get SMPS step down converter output voltage scaling
  * @note   SMPS output voltage is calibrated in production,
  *         calibration parameters are applied to the voltage level parameter
  *         to return the effective voltage value.
  * @rmtoll CR5          SMPSVOS       LL_PWR_SMPS_GetOutputVoltageLevel
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V20
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V25
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V30
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V35
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V40
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V45
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V50
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V55
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V60
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V65
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V70
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V75
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V80
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V85
  *         @arg @ref LL_PWR_SMPS_OUTPUT_VOLTAGE_1V90
  */
__STATIC_INLINE uint32_t LL_PWR_SMPS_GetOutputVoltageLevel(void)
{
  register __IO const uint32_t OutputVoltageLevel_calibration = (((*SMPS_VOLTAGE_CAL_ADDR) & SMPS_VOLTAGE_CAL) >> SMPS_VOLTAGE_CAL_POS);  /* SMPS output voltage level calibrated in production */
  register int32_t TrimmingSteps;                               /* Trimming steps between theorical output voltage and calibrated output voltage */
  register int32_t OutputVoltageLevelTrimmed;                   /* SMPS output voltage level after calibration: trimming value added to required level */

  if(OutputVoltageLevel_calibration == 0UL)
  {
    /* Device with SMPS output voltage not calibrated in production: Return output voltage value directly */

    return (uint32_t)(READ_BIT(PWR->CR5, PWR_CR5_SMPSVOS));
  }
  else
  {
    /* Device with SMPS output voltage calibrated in production: Return output voltage value after correction by calibration value */

    TrimmingSteps = ((int32_t)OutputVoltageLevel_calibration - (int32_t)(LL_PWR_SMPS_OUTPUT_VOLTAGE_1V50 >> PWR_CR5_SMPSVOS_Pos)); /* Trimming steps between theorical output voltage and calibrated output voltage */

    OutputVoltageLevelTrimmed = ((int32_t)((uint32_t)READ_BIT(PWR->CR5, PWR_CR5_SMPSVOS)) - TrimmingSteps);

    /* Clamp value to voltage range */
    if(OutputVoltageLevelTrimmed < 0)
    {
      OutputVoltageLevelTrimmed = (int32_t)LL_PWR_SMPS_OUTPUT_VOLTAGE_1V20;
    }
    else if(OutputVoltageLevelTrimmed > (int32_t)PWR_CR5_SMPSVOS)
    {
      OutputVoltageLevelTrimmed = (int32_t)LL_PWR_SMPS_OUTPUT_VOLTAGE_1V90;
    }
    else
    {
    }

    return (uint32_t)OutputVoltageLevelTrimmed;
  }
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_Configuration_Multicore Configuration of multicore, intended to be executed by CPU1
  * @{
  */

/**
  * @brief  Boot CPU2 after reset or wakeup from stop or standby modes
  * @rmtoll CR4          C2BOOT        LL_PWR_EnableBootC2
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableBootC2(void)
{
  SET_BIT(PWR->CR4, PWR_CR4_C2BOOT);
}

/**
  * @brief  Release bit to boot CPU2 after reset or wakeup from stop or standby
  *         modes
  * @rmtoll CR4          C2BOOT        LL_PWR_DisableBootC2
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableBootC2(void)
{
  CLEAR_BIT(PWR->CR4, PWR_CR4_C2BOOT);
}

/**
  * @brief  Check if bit to boot CPU2 after reset or wakeup from stop or standby
  *         modes is set
  * @rmtoll CR4          C2BOOT        LL_PWR_IsEnabledBootC2
  * @retval State of bit (1 or 0)
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledBootC2(void)
{
  return ((READ_BIT(PWR->CR4, PWR_CR4_C2BOOT) == (PWR_CR4_C2BOOT)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_Configuration_CPU2 Configuration of CPU2, intended to be executed by CPU2
  * @{
  */

/**
  * @brief  Set Low-Power mode for CPU2
  * @rmtoll C2CR1        LPMS          LL_C2_PWR_SetPowerMode
  * @param  LowPowerMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_MODE_STOP0
  *         @arg @ref LL_PWR_MODE_STOP1
  *         @arg @ref LL_PWR_MODE_STOP2
  *         @arg @ref LL_PWR_MODE_STANDBY
  *         @arg @ref LL_PWR_MODE_SHUTDOWN
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_SetPowerMode(uint32_t LowPowerMode)
{
  MODIFY_REG(PWR->C2CR1, PWR_C2CR1_LPMS, LowPowerMode);
}

/**
  * @brief  Get Low-Power mode for CPU2
  * @rmtoll C2CR1        LPMS          LL_C2_PWR_GetPowerMode
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_MODE_STOP0
  *         @arg @ref LL_PWR_MODE_STOP1
  *         @arg @ref LL_PWR_MODE_STOP2
  *         @arg @ref LL_PWR_MODE_STANDBY
  *         @arg @ref LL_PWR_MODE_SHUTDOWN
  */
__STATIC_INLINE uint32_t LL_C2_PWR_GetPowerMode(void)
{
  return (uint32_t)(READ_BIT(PWR->C2CR1, PWR_C2CR1_LPMS));
}

/**
  * @brief  Set flash power-down mode during low-power run mode for CPU2
  * @rmtoll C2CR1        FPDR          LL_C2_PWR_SetFlashPowerModeLPRun
  * @param  FlashLowPowerMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_FLASH_LPRUN_MODE_IDLE
  *         @arg @ref LL_PWR_FLASH_LPRUN_MODE_POWER_DOWN
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_SetFlashPowerModeLPRun(uint32_t FlashLowPowerMode)
{
  /* Unlock bit FPDR */
  WRITE_REG(PWR->C2CR1, 0x0000C1B0U);

  /* Update bit FPDR */
  MODIFY_REG(PWR->C2CR1, PWR_C2CR1_FPDR, FlashLowPowerMode);
}

/**
  * @brief  Get flash power-down mode during low-power run mode for CPU2
  * @rmtoll C2CR1        FPDR          LL_C2_PWR_GetFlashPowerModeLPRun
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_FLASH_LPRUN_MODE_IDLE
  *         @arg @ref LL_PWR_FLASH_LPRUN_MODE_POWER_DOWN
  */
__STATIC_INLINE uint32_t LL_C2_PWR_GetFlashPowerModeLPRun(void)
{
  return (uint32_t)(READ_BIT(PWR->C2CR1, PWR_C2CR1_FPDR));
}

/**
  * @brief  Set flash power-down mode during sleep mode for CPU2
  * @rmtoll C2CR1        FPDS          LL_C2_PWR_SetFlashPowerModeSleep
  * @param  FlashLowPowerMode This parameter can be one of the following values:
  *         @arg @ref LL_PWR_FLASH_SLEEP_MODE_IDLE
  *         @arg @ref LL_PWR_FLASH_SLEEP_MODE_POWER_DOWN
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_SetFlashPowerModeSleep(uint32_t FlashLowPowerMode)
{
  MODIFY_REG(PWR->C2CR1, PWR_C2CR1_FPDS, FlashLowPowerMode);
}

/**
  * @brief  Get flash power-down mode during sleep mode for CPU2
  * @rmtoll C2CR1        FPDS          LL_C2_PWR_GetFlashPowerModeSleep
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_PWR_FLASH_SLEEP_MODE_IDLE
  *         @arg @ref LL_PWR_FLASH_SLEEP_MODE_POWER_DOWN
  */
__STATIC_INLINE uint32_t LL_C2_PWR_GetFlashPowerModeSleep(void)
{
  return (uint32_t)(READ_BIT(PWR->C2CR1, PWR_C2CR1_FPDS));
}


/**
  * @brief  Enable Internal Wake-up line for CPU2
  * @rmtoll C2CR3        EIWUL         LL_C2_PWR_EnableInternWU
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_EnableInternWU(void)
{
  SET_BIT(PWR->C2CR3, PWR_C2CR3_EIWUL);
}

/**
  * @brief  Disable Internal Wake-up line for CPU2
  * @rmtoll C2CR3        EIWUL         LL_C2_PWR_DisableInternWU
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_DisableInternWU(void)
{
  CLEAR_BIT(PWR->C2CR3, PWR_C2CR3_EIWUL);
}

/**
  * @brief  Check if Internal Wake-up line is enabled for CPU2
  * @rmtoll C2CR3        EIWUL         LL_C2_PWR_IsEnabledInternWU
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C2_PWR_IsEnabledInternWU(void)
{
  return ((READ_BIT(PWR->C2CR3, PWR_C2CR3_EIWUL) == (PWR_C2CR3_EIWUL)) ? 1UL : 0UL);
}

/**
  * @brief  Enable the WakeUp PINx functionality
  * @rmtoll C2CR3        EWUP1         LL_C2_PWR_EnableWakeUpPin\n
  *         C2CR3        EWUP2         LL_C2_PWR_EnableWakeUpPin\n
  *         C2CR3        EWUP3         LL_C2_PWR_EnableWakeUpPin\n
  *         C2CR3        EWUP4         LL_C2_PWR_EnableWakeUpPin\n
  *         C2CR3        EWUP5         LL_C2_PWR_EnableWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_EnableWakeUpPin(uint32_t WakeUpPin)
{
  SET_BIT(PWR->C2CR3, WakeUpPin);
}

/**
  * @brief  Disable the WakeUp PINx functionality
  * @rmtoll C2CR3        EWUP1         LL_C2_PWR_DisableWakeUpPin\n
  *         C2CR3        EWUP2         LL_C2_PWR_DisableWakeUpPin\n
  *         C2CR3        EWUP3         LL_C2_PWR_DisableWakeUpPin\n
  *         C2CR3        EWUP4         LL_C2_PWR_DisableWakeUpPin\n
  *         C2CR3        EWUP5         LL_C2_PWR_DisableWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_DisableWakeUpPin(uint32_t WakeUpPin)
{
  CLEAR_BIT(PWR->C2CR3, WakeUpPin);
}

/**
  * @brief  Check if the WakeUp PINx functionality is enabled
  * @rmtoll C2CR3        EWUP1         LL_C2_PWR_IsEnabledWakeUpPin\n
  *         C2CR3        EWUP2         LL_C2_PWR_IsEnabledWakeUpPin\n
  *         C2CR3        EWUP3         LL_C2_PWR_IsEnabledWakeUpPin\n
  *         C2CR3        EWUP4         LL_C2_PWR_IsEnabledWakeUpPin\n
  *         C2CR3        EWUP5         LL_C2_PWR_IsEnabledWakeUpPin
  * @param  WakeUpPin This parameter can be one of the following values:
  *         @arg @ref LL_PWR_WAKEUP_PIN1
  *         @arg @ref LL_PWR_WAKEUP_PIN2
  *         @arg @ref LL_PWR_WAKEUP_PIN3
  *         @arg @ref LL_PWR_WAKEUP_PIN4
  *         @arg @ref LL_PWR_WAKEUP_PIN5
  * @retval None
  */
__STATIC_INLINE uint32_t LL_C2_PWR_IsEnabledWakeUpPin(uint32_t WakeUpPin)
{
  return ((READ_BIT(PWR->C2CR3, WakeUpPin) == (WakeUpPin)) ? 1UL : 0UL);
}

/**
  * @brief  Enable pull-up and pull-down configuration for CPU2
  * @rmtoll C2CR3        APC           LL_C2_PWR_EnablePUPDCfg
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_EnablePUPDCfg(void)
{
  SET_BIT(PWR->C2CR3, PWR_C2CR3_APC);
}

/**
  * @brief  Disable pull-up and pull-down configuration for CPU2
  * @rmtoll C2CR3        APC           LL_C2_PWR_DisablePUPDCfg
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_DisablePUPDCfg(void)
{
  CLEAR_BIT(PWR->C2CR3, PWR_C2CR3_APC);
}

/**
  * @brief  Check if pull-up and pull-down configuration is enabled for CPU2
  * @rmtoll C2CR3        APC           LL_C2_PWR_IsEnabledPUPDCfg
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C2_PWR_IsEnabledPUPDCfg(void)
{
  return ((READ_BIT(PWR->C2CR3, PWR_C2CR3_APC) == (PWR_C2CR3_APC)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_Configuration_CPU2_Radio Configuration of radio (BLE or 802.15.4) of CPU2, intended to be executed by CPU2
  * @{
  */

/**
  * @brief  Wakeup BLE controller from its sleep mode
  * @note   This bit is automatically reset when BLE controller
  *         exit its sleep mode.
  * @rmtoll C2CR1        BLEEWKUP      LL_C2_PWR_WakeUp_BLE
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_WakeUp_BLE(void)
{
  SET_BIT(PWR->C2CR1, PWR_C2CR1_BLEEWKUP);
}

/**
  * @brief  Check if the BLE controller is woken-up from
  *         low-power mode.
  * @rmtoll C2CR1        BLEEWKUP      LL_C2_PWR_IsWokenUp_BLE
  * @retval State of bit (1 or 0) (value "0": BLE is not woken-up)
  */
__STATIC_INLINE uint32_t LL_C2_PWR_IsWokenUp_BLE(void)
{
  return ((READ_BIT(PWR->C2CR1, PWR_C2CR1_BLEEWKUP) == (PWR_C2CR1_BLEEWKUP)) ? 1UL : 0UL);
}

/**
  * @brief  Wakeup 802.15.4 controller from its sleep mode
  * @note   This bit is automatically reset when 802.15.4 controller
  *         exit its sleep mode.
  * @rmtoll C2CR1        802EWKUP      LL_C2_PWR_WakeUp_802_15_4
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_WakeUp_802_15_4(void)
{
  SET_BIT(PWR->C2CR1, PWR_C2CR1_802EWKUP);
}

/**
  * @brief  Check if the 802.15.4 controller is woken-up from
  *         low-power mode.
  * @rmtoll C2CR1        802EWKUP      LL_C2_PWR_IsWokenUp_802_15_4
  * @retval State of bit (1 or 0) (value "0": 802.15.4 is not woken-up)
  */
__STATIC_INLINE uint32_t LL_C2_PWR_IsWokenUp_802_15_4(void)
{
  return ((READ_BIT(PWR->C2CR1, PWR_C2CR1_802EWKUP) == (PWR_C2CR1_802EWKUP)) ? 1UL : 0UL);
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
  * @brief  Get Wake-up Flag 5
  * @rmtoll SR1          WUF5          LL_PWR_IsActiveFlag_WU5
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU5(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_WUF5) == (PWR_SR1_WUF5)) ? 1UL : 0UL);
}

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
  * @brief  Get Wake-up Flag 3
  * @rmtoll SR1          WUF3          LL_PWR_IsActiveFlag_WU3
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_WU3(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_WUF3) == (PWR_SR1_WUF3)) ? 1UL : 0UL);
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
  * @brief  Clear Wake-up Flags
  * @rmtoll SCR          CWUF          LL_PWR_ClearFlag_WU
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CWUF);
}

/**
  * @brief  Clear Wake-up Flag 5
  * @rmtoll SCR          CWUF5         LL_PWR_ClearFlag_WU5
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU5(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CWUF5);
}

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
  * @brief  Clear Wake-up Flag 3
  * @rmtoll SCR          CWUF3         LL_PWR_ClearFlag_WU3
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_WU3(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CWUF3);
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


/**
  * @brief  Indicate whether VDDA voltage is below or above PVM3 threshold
  * @rmtoll SR2          PVMO3         LL_PWR_IsActiveFlag_PVMO3
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_PVMO3(void)
{
  return ((READ_BIT(PWR->SR2, PWR_SR2_PVMO3) == (PWR_SR2_PVMO3)) ? 1UL : 0UL);
}


/**
  * @brief  Indicate whether VDDUSB voltage is below or above PVM1 threshold
  * @rmtoll SR2          PVMO1         LL_PWR_IsActiveFlag_PVMO1
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_PVMO1(void)
{
  return ((READ_BIT(PWR->SR2, PWR_SR2_PVMO1) == (PWR_SR2_PVMO1)) ? 1UL : 0UL);
}

/**
  * @brief  Indicate whether VDD voltage is below or above the selected PVD threshold
  * @rmtoll SR2          PVDO          LL_PWR_IsActiveFlag_PVDO
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_PVDO(void)
{
  return ((READ_BIT(PWR->SR2, PWR_SR2_PVDO) == (PWR_SR2_PVDO)) ? 1UL : 0UL);
}

/**
  * @brief  Indicate whether the regulator is ready in the selected voltage range or if its output voltage is still changing to the required voltage level
  * @rmtoll SR2          VOSF          LL_PWR_IsActiveFlag_VOS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_VOS(void)
{
  return ((READ_BIT(PWR->SR2, PWR_SR2_VOSF) == (PWR_SR2_VOSF)) ? 1UL : 0UL);
}

/**
  * @brief  Indicate whether the regulator is ready in main mode or is in low-power mode
  * @note   Take care, return value "0" means the regulator is ready. Return value "1" means the output voltage range is still changing.
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
  * @brief  Get BORH interrupt flag
  * @rmtoll SR1          BORHF         LL_PWR_IsActiveFlag_BORH
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_BORH(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_BORHF) == (PWR_SR1_BORHF)) ? 1UL : 0UL);
}

/**
  * @brief  Clear BORH interrupt flag
  * @rmtoll SCR          CBORHF        LL_PWR_ClearFlag_BORH
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_BORH(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CBORHF);
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_FLAG_Management_SMPS FLAG management for SMPS
  * @{
  */

/**
  * @brief  Get SMPS step down converter forced in bypass mode interrupt flag
  * @note   To activate flag of SMPS step down converter forced in bypass mode
  *         by BORH, BOR must be preliminarily configured to control SMPS
  *         operating mode.
  *         Refer to function @ref LL_PWR_SetBORConfig().
  * @rmtoll SR1          SMPSFBF       LL_PWR_IsActiveFlag_SMPSFB
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_SMPSFB(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_SMPSFBF) == (PWR_SR1_SMPSFBF)) ? 1UL : 0UL);
}

/**
  * @brief  Clear SMPS step down converter forced in bypass mode interrupt flag
  * @note   To activate flag of SMPS step down converter forced in bypass mode
  *         by BORH, BOR must be preliminarily configured to control SMPS
  *         operating mode.
  *         Refer to function @ref LL_PWR_SetBORConfig().
  * @rmtoll SCR          CSMPSFBF      LL_PWR_ClearFlag_SMPSFB
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_SMPSFB(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CSMPSFBF);
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_FLAG_Management_Radio FLAG management for radio (BLE or 802.15.4)
  * @{
  */

/**
  * @brief  Get BLE wakeup interrupt flag
  * @rmtoll SR1          BLEWUF        LL_PWR_IsActiveFlag_BLEWU
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_BLEWU(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_BLEWUF) == (PWR_SR1_BLEWUF)) ? 1UL : 0UL);
}

/**
  * @brief  Get 802.15.4 wakeup interrupt flag
  * @rmtoll SR1          802WUF        LL_PWR_IsActiveFlag_802WU
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_802WU(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_802WUF) == (PWR_SR1_802WUF)) ? 1UL : 0UL);
}

/**
  * @brief  Get BLE end of activity interrupt flag
  * @rmtoll SR1          BLEAF         LL_PWR_IsActiveFlag_BLEA
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_BLEA(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_BLEAF) == (PWR_SR1_BLEAF)) ? 1UL : 0UL);
}

/**
  * @brief  Get 802.15.4 end of activity interrupt flag
  * @rmtoll SR1          802AF         LL_PWR_IsActiveFlag_802A
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_802A(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_802AF) == (PWR_SR1_802AF)) ? 1UL : 0UL);
}

/**
  * @brief  Get critical radio phase end of activity interrupt flag
  * @rmtoll SR1          CRPEF         LL_PWR_IsActiveFlag_CRPE
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_CRPE(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_CRPEF) == (PWR_SR1_CRPEF)) ? 1UL : 0UL);
}

/**
  * @brief  Get critical radio system phase flag
  * @rmtoll EXTSCR       CRPF          LL_PWR_IsActiveFlag_CRP
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_CRP(void)
{
  return ((READ_BIT(PWR->EXTSCR, PWR_EXTSCR_CRPF) == (PWR_EXTSCR_CRPF)) ? 1UL : 0UL);
}

/**
  * @brief  Clear BLE wakeup interrupt flag
  * @rmtoll SCR          BLEWU         LL_PWR_ClearFlag_BLEWU
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_BLEWU(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CBLEWUF);
}

/**
  * @brief  Clear 802.15.4 wakeup interrupt flag
  * @rmtoll SCR          802WU         LL_PWR_ClearFlag_802WU
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_802WU(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_C802WUF);
}

/**
  * @brief  Clear BLE end of activity interrupt flag
  * @rmtoll SCR          BLEAF         LL_PWR_ClearFlag_BLEA
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_BLEA(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CBLEAF);
}

/**
  * @brief  Clear 802.15.4 end of activity interrupt flag
  * @rmtoll SCR          802AF         LL_PWR_ClearFlag_802A
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_802A(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_C802AF);
}

/**
  * @brief  Clear critical radio phase end of activity interrupt flag
  * @rmtoll SCR          CCRPEF        LL_PWR_ClearFlag_CRPE
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_CRPE(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CCRPEF);
}

/**
  * @brief  Clear critical radio system phase flag
  * @rmtoll EXTSCR       CCRP          LL_PWR_ClearFlag_CRP
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_CRP(void)
{
  WRITE_REG(PWR->EXTSCR, PWR_EXTSCR_CCRPF);
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_FLAG_Management_Multicore FLAG management for multicore
  * @{
  */

/**
  * @brief  Get CPU2 hold interrupt flag
  * @rmtoll SCR          CC2HF         LL_PWR_IsActiveFlag_C2H
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_C2H(void)
{
  return ((READ_BIT(PWR->SR1, PWR_SR1_C2HF) == (PWR_SR1_C2HF)) ? 1UL : 0UL);
}

/**
  * @brief  Get system stop flag for CPU1
  * @rmtoll EXTSCR       C1STOPF       LL_PWR_IsActiveFlag_C1STOP
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_C1STOP(void)
{
  return ((READ_BIT(PWR->EXTSCR, PWR_EXTSCR_C1STOPF) == (PWR_EXTSCR_C1STOPF)) ? 1UL : 0UL);
}

/**
  * @brief  Get system standby flag for CPU1
  * @rmtoll EXTSCR       C1SBF         LL_PWR_IsActiveFlag_C1SB
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_C1SB(void)
{
  return ((READ_BIT(PWR->EXTSCR, PWR_EXTSCR_C1SBF) == (PWR_EXTSCR_C1SBF)) ? 1UL : 0UL);
}

/**
  * @brief  Get deepsleep mode for CPU1
  * @rmtoll EXTSCR       C1DS          LL_PWR_IsActiveFlag_C1DS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_C1DS(void)
{
  return ((READ_BIT(PWR->EXTSCR, PWR_EXTSCR_C1DS) == (PWR_EXTSCR_C1DS)) ? 1UL : 0UL);
}

/**
  * @brief  System stop flag for CPU2
  * @rmtoll EXTSCR       C2STOPF       LL_PWR_IsActiveFlag_C2STOP
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_C2STOP(void)
{
  return ((READ_BIT(PWR->EXTSCR, PWR_EXTSCR_C2STOPF) == (PWR_EXTSCR_C2STOPF)) ? 1UL : 0UL);
}

/**
  * @brief  System standby flag for CPU2
  * @rmtoll EXTSCR       C2SBF         LL_PWR_IsActiveFlag_C2SB
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_C2SB(void)
{
  return ((READ_BIT(PWR->EXTSCR, PWR_EXTSCR_C2SBF) == (PWR_EXTSCR_C2SBF)) ? 1UL : 0UL);
}

/**
  * @brief  Get deepsleep mode for CPU2
  * @rmtoll EXTSCR       C2DS          LL_PWR_IsActiveFlag_C2DS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsActiveFlag_C2DS(void)
{
  return ((READ_BIT(PWR->EXTSCR, PWR_EXTSCR_C2DS) == (PWR_EXTSCR_C2DS)) ? 1UL : 0UL);
}

/**
  * @brief  Clear CPU2 hold interrupt flag
  * @rmtoll SCR          CC2HF         LL_PWR_ClearFlag_C2H
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_C2H(void)
{
  WRITE_REG(PWR->SCR, PWR_SCR_CC2HF);
}
/**
  * @brief  Clear standby and stop flags for CPU1
  * @rmtoll EXTSCR       C1CSSF        LL_PWR_ClearFlag_C1STOP_C1STB
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_C1STOP_C1STB(void)
{
  WRITE_REG(PWR->EXTSCR, PWR_EXTSCR_C1CSSF);
}

/**
  * @brief  Clear standby and stop flags for CPU2
  * @rmtoll EXTSCR       C2CSSF        LL_PWR_ClearFlag_C2STOP_C2STB
  * @retval None
  */
__STATIC_INLINE void LL_PWR_ClearFlag_C2STOP_C2STB(void)
{
  WRITE_REG(PWR->EXTSCR, PWR_EXTSCR_C2CSSF);
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_IT_Management_SMPS PWR IT management for SMPS
  * @{
  */

/**
  * @brief  Enable SMPS step down converter forced in bypass mode by BORH
  *         interrupt for CPU1
  * @note   To activate flag of SMPS step down converter forced in bypass mode
  *         by BORH, BOR must be preliminarily configured to control SMPS
  *         operating mode.
  *         Refer to function @ref LL_PWR_SetBORConfig().
  * @rmtoll CR3          EBORHSMPSFB   LL_PWR_EnableIT_BORH_SMPSFB
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableIT_BORH_SMPSFB(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_EBORHSMPSFB);
}

/**
  * @brief  Disable SMPS step down converter forced in bypass mode by BORH
  *         interrupt for CPU1
  * @rmtoll CR3          EBORHSMPSFB   LL_PWR_DisableIT_BORH_SMPSFB
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableIT_BORH_SMPSFB(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_EBORHSMPSFB);
}

/**
  * @brief  Check if SMPS step down converter forced in bypass mode by BORH
  *         interrupt is enabled for CPU1
  * @rmtoll CR3          EBORHSMPSFB   LL_PWR_IsEnabledIT_BORH_SMPSFB
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledIT_BORH_SMPSFB(void)
{
  return ((READ_BIT(PWR->CR3, PWR_CR3_EBORHSMPSFB) == (PWR_CR3_EBORHSMPSFB)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_IT_Management_Radio PWR IT management for radio (BLE or 802.15.4)
  * @{
  */

/**
  * @brief  Enable BLE end of activity interrupt for CPU1
  * @rmtoll CR3          EBLEA         LL_PWR_EnableIT_BLEA
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableIT_BLEA(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_EBLEA);
}

/**
  * @brief  Enable 802.15.4 end of activity interrupt for CPU1
  * @rmtoll CR3          E802A         LL_PWR_EnableIT_802A
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableIT_802A(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_E802A);
}


/**
  * @brief  Disable BLE end of activity interrupt for CPU1
  * @rmtoll CR3          EBLEA         LL_PWR_DisableIT_BLEA
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableIT_BLEA(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_EBLEA);
}

/**
  * @brief  Disable 802.15.4 end of activity interrupt for CPU1
  * @rmtoll CR3          E802A         LL_PWR_DisableIT_802A
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableIT_802A(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_E802A);
}

/**
  * @brief  Check if BLE end of activity interrupt is enabled for CPU1
  * @rmtoll CR3          EBLEA         LL_PWR_IsEnabledIT_BLEA
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledIT_BLEA(void)
{
  return ((READ_BIT(PWR->CR3, PWR_CR3_EBLEA) == (PWR_CR3_EBLEA)) ? 1UL : 0UL);
}

/**
  * @brief  Check if 802.15.4 end of activity interrupt is enabled for CPU1
  * @rmtoll CR3          E802A         LL_PWR_IsEnabledIT_802A
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledIT_802A(void)
{
  return ((READ_BIT(PWR->CR3, PWR_CR3_E802A) == (PWR_CR3_E802A)) ? 1UL : 0UL);
}

/**
  * @brief  Enable critical radio phase end of activity interrupt for CPU1
  * @rmtoll CR3          ECRPE         LL_PWR_EnableIT_802A
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableIT_CRPE(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_ECRPE);
}

/**
  * @brief  Disable critical radio phase end of activity interrupt for CPU1
  * @rmtoll CR3          ECRPE         LL_PWR_DisableIT_802A
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableIT_CRPE(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_ECRPE);
}

/**
  * @brief  Check if critical radio phase end of activity interrupt is enabled for CPU1
  * @rmtoll CR3          ECRPE         LL_PWR_IsEnabledIT_802A
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledIT_CRPE(void)
{
  return ((READ_BIT(PWR->CR3, PWR_CR3_ECRPE) == (PWR_CR3_ECRPE)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_IT_Management_Multicore PWR IT management for multicore
  * @{
  */

/**
  * @brief  Enable CPU2 hold interrupt for CPU1
  * @rmtoll CR3          EC2H          LL_PWR_EnableIT_HoldCPU2
  * @retval None
  */
__STATIC_INLINE void LL_PWR_EnableIT_HoldCPU2(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_EC2H);
}

/**
  * @brief  Disable 802.15.4 host wakeup interrupt for CPU2
  * @rmtoll CR3          EC2H          LL_PWR_DisableIT_HoldCPU2
  * @retval None
  */
__STATIC_INLINE void LL_PWR_DisableIT_HoldCPU2(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_EC2H);
}

/**
  * @brief  Check if BLE host wakeup interrupt is enabled for CPU2
  * @rmtoll CR3          EC2H          LL_PWR_IsEnabledIT_HoldCPU2
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_PWR_IsEnabledIT_HoldCPU2(void)
{
  return ((READ_BIT(PWR->CR3, PWR_CR3_EC2H) == (PWR_CR3_EC2H)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup PWR_LL_EF_IT_Management_CPU2 PWR IT management of CPU2, intended to be executed by CPU2
  * @{
  */

/**
  * @brief  Enable BLE host wakeup interrupt for CPU2
  * @rmtoll C2CR3        EBLEWUP       LL_C2_PWR_EnableIT_BLEWU
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_EnableIT_BLEWU(void)
{
  SET_BIT(PWR->C2CR3, PWR_C2CR3_EBLEWUP);
}

/**
  * @brief  Enable 802.15.4 host wakeup interrupt for CPU2
  * @rmtoll C2CR3        E802WUP       LL_C2_PWR_EnableIT_802WU
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_EnableIT_802WU(void)
{
  SET_BIT(PWR->C2CR3, PWR_C2CR3_E802WUP);
}

/**
  * @brief  Disable BLE host wakeup interrupt for CPU2
  * @rmtoll C2CR3        EBLEWUP       LL_C2_PWR_DisableIT_BLEWU
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_DisableIT_BLEWU(void)
{
  CLEAR_BIT(PWR->C2CR3, PWR_C2CR3_EBLEWUP);
}

/**
  * @brief  Disable 802.15.4 host wakeup interrupt for CPU2
  * @rmtoll C2CR3        E802WUP       LL_C2_PWR_DisableIT_802WU
  * @retval None
  */
__STATIC_INLINE void LL_C2_PWR_DisableIT_802WU(void)
{
  CLEAR_BIT(PWR->C2CR3, PWR_C2CR3_E802WUP);
}

/**
  * @brief  Check if BLE host wakeup interrupt is enabled for CPU2
  * @rmtoll C2CR3        EBLEWUP       LL_C2_PWR_IsEnabledIT_BLEWU
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C2_PWR_IsEnabledIT_BLEWU(void)
{
  return ((READ_BIT(PWR->C2CR3, PWR_C2CR3_EBLEWUP) == (PWR_C2CR3_EBLEWUP)) ? 1UL : 0UL);
}

/**
  * @brief  Check if 802.15.4 host wakeup interrupt is enabled for CPU2
  * @rmtoll C2CR3        E802WUP       LL_C2_PWR_IsEnabledIT_802WU
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C2_PWR_IsEnabledIT_802WU(void)
{
  return ((READ_BIT(PWR->C2CR3, PWR_C2CR3_E802WUP) == (PWR_C2CR3_E802WUP)) ? 1UL : 0UL);
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

#endif /* STM32WBxx_LL_PWR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
