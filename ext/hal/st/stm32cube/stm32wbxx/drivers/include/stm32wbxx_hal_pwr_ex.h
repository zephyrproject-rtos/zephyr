/**
  ******************************************************************************
  * @file    stm32wbxx_hal_pwr_ex.h
  * @author  MCD Application Team
  * @brief   Header file of PWR HAL Extended module.
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
#ifndef STM32WBxx_HAL_PWR_EX_H
#define STM32WBxx_HAL_PWR_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_def.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @defgroup PWREx PWREx
  * @brief PWR Extended HAL module driver
  * @{
  */


/* Exported types ------------------------------------------------------------*/

/** @defgroup PWREx_Exported_Types PWR Extended Exported Types
  * @{
  */

/**
  * @brief  PWR PVM configuration structure definition
  */
typedef struct
{
  uint32_t PVMType;   /*!< PVMType: Specifies which voltage is monitored and against which threshold.
                           This parameter can be a value of @ref PWREx_PVM_Type.
                           @arg @ref PWR_PVM_1 Peripheral Voltage Monitoring 1 enable: VDDUSB versus 1.2 V (applicable when USB feature is supported).
                           @arg @ref PWR_PVM_3 Peripheral Voltage Monitoring 3 enable: VDDA versus 1.62 V.
                           */

  uint32_t Mode;      /*!< Mode: Specifies the operating mode for the selected pins.
                           This parameter can be a value of @ref PWREx_PVM_Mode. */
  uint32_t WakeupTarget;   /*!< Specifies the Wakeup Target
                           This parameter can be a value of @ref PWREx_WakeUpTarget_Definition */
}PWR_PVMTypeDef;

/**
  * @brief  PWR SMPS step down configuration structure definition
  */
typedef struct
{
  uint32_t StartupCurrent; /*!< SMPS step down converter supply startup current selection.
                                This parameter can be a value of @ref PWREx_SMPS_STARTUP_CURRENT. */

  uint32_t OutputVoltage;  /*!< SMPS step down converter output voltage scaling voltage level.
                                This parameter can be a value of @ref PWREx_SMPS_OUTPUT_VOLTAGE_LEVEL */
}PWR_SMPSTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup PWREx_Exported_Constants  PWR Extended Exported Constants
  * @{
  */

/** @defgroup PWREx_WUP_Polarity Shift to apply to retrieve polarity information from PWR_WAKEUP_PINy_xxx constants
  * @{
  */
#define PWR_WUP_POLARITY_SHIFT                  0x05U   /*!< Internal constant used to retrieve wakeup pin polarity */
/**
  * @}
  */


/** @defgroup PWREx_WakeUp_Pins  PWR wake-up pins
  * @{
  */
#define PWR_WAKEUP_PIN1_HIGH            PWR_CR3_EWUP1  /*!< Wakeup pin 1 (with high level polarity) */
#define PWR_WAKEUP_PIN2_HIGH            PWR_CR3_EWUP2  /*!< Wakeup pin 2 (with high level polarity) */
#define PWR_WAKEUP_PIN3_HIGH            PWR_CR3_EWUP3  /*!< Wakeup pin 3 (with high level polarity) */
#define PWR_WAKEUP_PIN4_HIGH            PWR_CR3_EWUP4  /*!< Wakeup pin 4 (with high level polarity) */
#define PWR_WAKEUP_PIN5_HIGH            PWR_CR3_EWUP5  /*!< Wakeup pin 5 (with high level polarity) */

#define PWR_WAKEUP_PIN1_LOW             ((PWR_CR4_WP1<<PWR_WUP_POLARITY_SHIFT) | PWR_CR3_EWUP1) /*!< Wakeup pin 1 (with low level polarity) */
#define PWR_WAKEUP_PIN2_LOW             ((PWR_CR4_WP2<<PWR_WUP_POLARITY_SHIFT) | PWR_CR3_EWUP2) /*!< Wakeup pin 2 (with low level polarity) */
#define PWR_WAKEUP_PIN3_LOW             ((PWR_CR4_WP3<<PWR_WUP_POLARITY_SHIFT) | PWR_CR3_EWUP3) /*!< Wakeup pin 3 (with low level polarity) */
#define PWR_WAKEUP_PIN4_LOW             ((PWR_CR4_WP4<<PWR_WUP_POLARITY_SHIFT) | PWR_CR3_EWUP4) /*!< Wakeup pin 4 (with low level polarity) */
#define PWR_WAKEUP_PIN5_LOW             ((PWR_CR4_WP5<<PWR_WUP_POLARITY_SHIFT) | PWR_CR3_EWUP5) /*!< Wakeup pin 5 (with low level polarity) */
/**
  * @}
  */

/* Literals kept for legacy purpose */
#define PWR_WAKEUP_PIN1                 PWR_CR3_EWUP1  /*!< Wakeup pin 1 (with high level polarity) */
#define PWR_WAKEUP_PIN2                 PWR_CR3_EWUP2  /*!< Wakeup pin 2 (with high level polarity) */
#define PWR_WAKEUP_PIN3                 PWR_CR3_EWUP3  /*!< Wakeup pin 3 (with high level polarity) */
#define PWR_WAKEUP_PIN4                 PWR_CR3_EWUP4  /*!< Wakeup pin 4 (with high level polarity) */
#define PWR_WAKEUP_PIN5                 PWR_CR3_EWUP5  /*!< Wakeup pin 5 (with high level polarity) */

/** @defgroup PWREx_PIN_Polarity PWREx Pin Polarity configuration
  * @{
  */
#define PWR_PIN_POLARITY_HIGH 0x00000000U
#define PWR_PIN_POLARITY_LOW  0x00000001U
/**
  * @}
  */

/** @defgroup PWREx_PVM_Type Peripheral Voltage Monitoring type
  * @{
  */
#define PWR_PVM_1                  PWR_CR2_PVME1  /*!< Peripheral Voltage Monitoring 1 enable: VDDUSB versus 1.2 V (applicable when USB feature is supported) */
#define PWR_PVM_3                  PWR_CR2_PVME3  /*!< Peripheral Voltage Monitoring 3 enable: VDDA versus 1.62 V */
/**
  * @}
  */

/** @defgroup PWREx_PVM_Mode  PWR PVM interrupt and event mode
  * @{
  */
#define PWR_PVM_MODE_NORMAL                 (0x00000000U) /*!< basic mode is used */

#define PWR_PVM_MODE_IT_RISING              (PVM_MODE_IT | PVM_RISING_EDGE)             /*!< External Interrupt Mode with Rising edge trigger detection */
#define PWR_PVM_MODE_IT_FALLING             (PVM_MODE_IT | PVM_FALLING_EDGE)            /*!< External Interrupt Mode with Falling edge trigger detection */
#define PWR_PVM_MODE_IT_RISING_FALLING      (PVM_MODE_IT | PVM_RISING_FALLING_EDGE)     /*!< External Interrupt Mode with Rising/Falling edge trigger detection */

#define PWR_PVM_MODE_EVENT_RISING           (PVM_MODE_EVT | PVM_RISING_EDGE)          /*!< Event Mode with Rising edge trigger detection */
#define PWR_PVM_MODE_EVENT_FALLING          (PVM_MODE_EVT | PVM_FALLING_EDGE)         /*!< Event Mode with Falling edge trigger detection */
#define PWR_PVM_MODE_EVENT_RISING_FALLING   (PVM_MODE_EVT | PVM_RISING_FALLING_EDGE)  /*!< Event Mode with Rising/Falling edge trigger detection */
/**
  * @}
  */

/** @defgroup PWREx_Flash_PowerDown  Flash Power Down modes
  * @{
  */
#define PWR_FLASHPD_LPRUN                   PWR_CR1_FPDR     /*!< Enable Flash power down in low power run mode */
#define PWR_FLASHPD_LPSLEEP                 PWR_CR1_FPDS     /*!< Enable Flash power down in low power sleep mode */
/**
  * @}
  */

/** @defgroup PWREx_Regulator_Voltage_Scale  PWR Regulator voltage scale
  * @{
  */
#define PWR_REGULATOR_VOLTAGE_SCALE1       PWR_CR1_VOS_0     /*!< Regulator voltage output range 1 mode, typical output voltage at 1.2 V, system frequency up to 64 MHz */
#define PWR_REGULATOR_VOLTAGE_SCALE2       PWR_CR1_VOS_1     /*!< Regulator voltage output range 2 mode, typical output voltage at 1.0 V, system frequency up to 16 MHz */
/**
  * @}
  */


/** @defgroup PWREx_VBAT_Battery_Charging_Selection PWR battery charging resistor selection
  * @{
  */
#define PWR_BATTERY_CHARGING_RESISTOR_5          (0x00000000U) /*!< VBAT charging through a 5 kOhms resistor   */
#define PWR_BATTERY_CHARGING_RESISTOR_1_5        PWR_CR4_VBRS           /*!< VBAT charging through a 1.5 kOhms resistor */
/**
  * @}
  */

/** @defgroup PWREx_VBAT_Battery_Charging PWR battery charging
  * @{
  */
#define PWR_BATTERY_CHARGING_DISABLE        (0x00000000U)
#define PWR_BATTERY_CHARGING_ENABLE         PWR_CR4_VBE
/**
  * @}
  */

/** @defgroup PWREx_GPIO_Bit_Number GPIO bit number for I/O setting in standby/shutdown mode
  * @{
  */
#define PWR_GPIO_BIT_0   PWR_PUCRC_PC0    /*!< GPIO port I/O pin 0  */
#define PWR_GPIO_BIT_1   PWR_PUCRC_PC1    /*!< GPIO port I/O pin 1  */
#define PWR_GPIO_BIT_2   PWR_PUCRC_PC2    /*!< GPIO port I/O pin 2  */
#define PWR_GPIO_BIT_3   PWR_PUCRC_PC3    /*!< GPIO port I/O pin 3  */
#define PWR_GPIO_BIT_4   PWR_PUCRC_PC4    /*!< GPIO port I/O pin 4  */
#define PWR_GPIO_BIT_5   PWR_PUCRC_PC5    /*!< GPIO port I/O pin 5  */
#define PWR_GPIO_BIT_6   PWR_PUCRC_PC6    /*!< GPIO port I/O pin 6  */
#define PWR_GPIO_BIT_7   PWR_PUCRC_PC7    /*!< GPIO port I/O pin 7  */
#define PWR_GPIO_BIT_8   PWR_PUCRC_PC8    /*!< GPIO port I/O pin 8  */
#define PWR_GPIO_BIT_9   PWR_PUCRC_PC9    /*!< GPIO port I/O pin 9  */
#define PWR_GPIO_BIT_10  PWR_PUCRC_PC10   /*!< GPIO port I/O pin 10 */
#define PWR_GPIO_BIT_11  PWR_PUCRC_PC11   /*!< GPIO port I/O pin 11 */
#define PWR_GPIO_BIT_12  PWR_PUCRC_PC12   /*!< GPIO port I/O pin 12 */
#define PWR_GPIO_BIT_13  PWR_PUCRC_PC13   /*!< GPIO port I/O pin 14 */
#define PWR_GPIO_BIT_14  PWR_PDCRC_PC14   /*!< GPIO port I/O pin 14 */
#define PWR_GPIO_BIT_15  PWR_PUCRC_PC15   /*!< GPIO port I/O pin 15 */
/**
  * @}
  */

/** @defgroup PWREx_GPIO GPIO port
  * @{
  */
#define PWR_GPIO_A   0x00000000U      /*!< GPIO port A */
#define PWR_GPIO_B   0x00000001U      /*!< GPIO port B */
#define PWR_GPIO_C   0x00000002U      /*!< GPIO port C */
#define PWR_GPIO_D   0x00000003U      /*!< GPIO port D */
#define PWR_GPIO_E   0x00000004U      /*!< GPIO port E */
#define PWR_GPIO_H   0x00000007U      /*!< GPIO port H */
/**
  * @}
  */

/** @defgroup PWREx_BOR_CONFIGURATION BOR configuration
  * @{
  */
#define PWR_BOR_SYSTEM_RESET            (LL_PWR_BOR_SYSTEM_RESET)      /*!< BOR will generate a system reset  */
#define PWR_BOR_SMPS_FORCE_BYPASS       (LL_PWR_BOR_SMPS_FORCE_BYPASS) /*!< BOR will for SMPS step down converter in bypass mode */
/**
  * @}
  */

/** @defgroup PWREx_SMPS_OPERATING_MODES SMPS step down converter operating modes
  * @{
  */
/* Note: Literals values are defined from register SR2 bits SMPSF and SMPSBF  */
/*       but they are also used as register CR5 bits SMPSEN and SMPSBEN,      */
/*       as used by all SMPS operating mode functions targetting different    */
/*       registers:                                                           */
/*       "LL_PWR_SMPS_SetMode()", "LL_PWR_SMPS_GetMode()"                     */
/*       and "LL_PWR_SMPS_GetEffectiveMode()".                                */
#define PWR_SMPS_BYPASS                 (PWR_SR2_SMPSBF) /*!< SMPS step down in bypass mode  */
#define PWR_SMPS_STEP_DOWN              (PWR_SR2_SMPSF)  /*!< SMPS step down in step down mode if system low power mode is run, LP run or stop0. If system low power mode is stop1, stop2, standby, shutdown, then SMPS is forced in mode open to preserve energy stored in decoupling capacitor as long as possible. */
/**
  * @}
  */

/** @defgroup PWREx_SMPS_STARTUP_CURRENT SMPS step down converter supply startup current selection
  * @{
  */
#define PWR_SMPS_STARTUP_CURRENT_80MA   (0x00000000U)                                            /*!< SMPS step down converter supply startup current 80mA */
#define PWR_SMPS_STARTUP_CURRENT_100MA  (                                      PWR_CR5_SMPSSC_0) /*!< SMPS step down converter supply startup current 100mA */
#define PWR_SMPS_STARTUP_CURRENT_120MA  (                   PWR_CR5_SMPSSC_1                   ) /*!< SMPS step down converter supply startup current 120mA */
#define PWR_SMPS_STARTUP_CURRENT_140MA  (                   PWR_CR5_SMPSSC_1 | PWR_CR5_SMPSSC_0) /*!< SMPS step down converter supply startup current 140mA */
#define PWR_SMPS_STARTUP_CURRENT_160MA  (PWR_CR5_SMPSSC_2                                      ) /*!< SMPS step down converter supply startup current 160mA */
#define PWR_SMPS_STARTUP_CURRENT_180MA  (PWR_CR5_SMPSSC_2 |                    PWR_CR5_SMPSSC_0) /*!< SMPS step down converter supply startup current 180mA */
#define PWR_SMPS_STARTUP_CURRENT_200MA  (PWR_CR5_SMPSSC_2 | PWR_CR5_SMPSSC_1                   ) /*!< SMPS step down converter supply startup current 200mA */
#define PWR_SMPS_STARTUP_CURRENT_220MA  (PWR_CR5_SMPSSC_2 | PWR_CR5_SMPSSC_1 | PWR_CR5_SMPSSC_0) /*!< SMPS step down converter supply startup current 220mA */
/**
  * @}
  */

/** @defgroup PWREx_SMPS_OUTPUT_VOLTAGE_LEVEL SMPS step down converter output voltage scaling voltage level
  * @{
  */
/* Note: SMPS voltage is trimmed during device production to control
         the actual voltage level variation from device to device. */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V20  (0x00000000U)                                                                   /*!< SMPS step down converter supply output voltage 1.20V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V25  (                                                            PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.25V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V30  (                                        PWR_CR5_SMPSVOS_1                    ) /*!< SMPS step down converter supply output voltage 1.30V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V35  (                                        PWR_CR5_SMPSVOS_1 | PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.35V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V40  (                    PWR_CR5_SMPSVOS_2                                        ) /*!< SMPS step down converter supply output voltage 1.40V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V45  (                    PWR_CR5_SMPSVOS_2 |                     PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.45V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V50  (                    PWR_CR5_SMPSVOS_2 | PWR_CR5_SMPSVOS_1                    ) /*!< SMPS step down converter supply output voltage 1.50V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V55  (                    PWR_CR5_SMPSVOS_2 | PWR_CR5_SMPSVOS_1 | PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.55V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V60  (PWR_CR5_SMPSVOS_3                                                            ) /*!< SMPS step down converter supply output voltage 1.60V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V65  (PWR_CR5_SMPSVOS_3 |                                         PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.65V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V70  (PWR_CR5_SMPSVOS_3 |                     PWR_CR5_SMPSVOS_1                    ) /*!< SMPS step down converter supply output voltage 1.70V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V75  (PWR_CR5_SMPSVOS_3 |                     PWR_CR5_SMPSVOS_1 | PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.75V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V80  (PWR_CR5_SMPSVOS_3 | PWR_CR5_SMPSVOS_2                                        ) /*!< SMPS step down converter supply output voltage 1.80V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V85  (PWR_CR5_SMPSVOS_3 | PWR_CR5_SMPSVOS_2 |                     PWR_CR5_SMPSVOS_0) /*!< SMPS step down converter supply output voltage 1.85V */
#define PWR_SMPS_OUTPUT_VOLTAGE_1V90  (PWR_CR5_SMPSVOS_3 | PWR_CR5_SMPSVOS_2 | PWR_CR5_SMPSVOS_1                    ) /*!< SMPS step down converter supply output voltage 1.90V */
/**
  * @}
  */

/** @defgroup PWREx_Flag  PWR Status Flags
  *        Elements values convention: 0000 0000 0XXY YYYYb
  *           - Y YYYY  : Flag position in the XX register (5 bits)
  *           - XX  : Status register (2 bits)
  *                 - 01: SR1 register
  *                 - 10: SR2 register
  *                 - 11: C2_SCR register
  *        The only exception is PWR_FLAG_WUF, encompassing all
  *        wake-up flags and set to PWR_SR1_WUF.
  * @{
  */
/*--------------------------------SR1-------------------------------*/
#define PWR_FLAG_WUF1                       (0x0020U)   /*!< Wakeup event on wakeup pin 1 */
#define PWR_FLAG_WUF2                       (0x0021U)   /*!< Wakeup event on wakeup pin 2 */
#define PWR_FLAG_WUF3                       (0x0022U)   /*!< Wakeup event on wakeup pin 3 */
#define PWR_FLAG_WUF4                       (0x0023U)   /*!< Wakeup event on wakeup pin 4 */
#define PWR_FLAG_WUF5                       (0x0024U)   /*!< Wakeup event on wakeup pin 5 */
#define PWR_FLAG_WU                         PWR_SR1_WUF          /*!< Encompass wakeup event on all wakeup pins */

#define PWR_FLAG_BHWF                       (0x0028U)   /*!< BLE_Host WakeUp Flag */
#define PWR_FLAG_FRCBYPI                    (0x0029U)   /*!< SMPS Forced in Bypass Interrupt Flag */

#define PWR_FLAG_RFPHASEI                   (0x002BU)   /*!< Radio Phase Interrupt Flag */
#define PWR_FLAG_BLEACTI                    (0x002CU)   /*!< BLE Activity Interrupt Flag */
#define PWR_FLAG_802ACTI                    (0x002DU)   /*!< 802.15.4 Activity Interrupt Flag */
#define PWR_FLAG_HOLDC2I                    (0x002EU)   /*!< CPU2 on-Hold Interrupt Flag */
#define PWR_FLAG_WUFI                       (0x002FU)   /*!< Wakeup on internal wakeup line */

/*--------------------------------SR2-------------------------------*/
#define PWR_FLAG_SMPSRDYF                   (0x0040U)   /*!< SMPS Ready Flag */
#define PWR_FLAG_SMPSBYPF                   (0x0041U)   /*!< SMPS Bypass Flag */

#define PWR_FLAG_REGLPS                     (0x0048U)   /*!< Low-power regulator start flag */
#define PWR_FLAG_REGLPF                     (0x0049U)   /*!< Low-power regulator flag */

#define PWR_FLAG_VOSF                       (0x004AU)   /*!< Voltage scaling flag */
#define PWR_FLAG_PVDO                       (0x004BU)   /*!< Power Voltage Detector output flag */

#define PWR_FLAG_PVMO1                      (0x004CU)   /*!< Power Voltage Monitoring 1 output flag */
#define PWR_FLAG_PVMO3                      (0x004EU)   /*!< Power Voltage Monitoring 3 output flag */

/*------------------------------EXTSCR---------------------------*/
#define PWR_FLAG_SB                         (0x0068U)   /*!< System Standby flag for CPU1 */
#define PWR_FLAG_STOP                       (0x0069U)   /*!< System Stop flag for CPU1 */

#define PWR_FLAG_C2SB                       (0x006AU)   /*!< System Standby flag for CPU2 */
#define PWR_FLAG_C2STOP                     (0x006BU)   /*!< System Stop flag for CPU2 */

#define PWR_FLAG_CRITICAL_RF_PHASE          (0x006DU)   /*!< Critical radio system phase flag */
#define PWR_FLAG_C1DEEPSLEEP                (0x006EU)   /*!< CPU1 DeepSleep Flag */
#define PWR_FLAG_C2DEEPSLEEP                (0x006FU)   /*!< CPU2 DeepSleep Flag */
/**
  * @}
  */

/** @defgroup PWREx_WakeUpTarget_Definition PWR Wakeup Target Definition
  * @{
  */
#define PWR_WAKEUPTARGET_CPU1          (0x00000001U)
#define PWR_WAKEUPTARGET_CPU2          (0x00000002U)
#define PWR_WAKEUPTARGET_ALL_CPU       (PWR_WAKEUPTARGET_CPU1 | PWR_WAKEUPTARGET_CPU2)
#define PWR_WAKEUPTARGET_RF            (0x00000004U)
/**
  * @}
  */

/** @defgroup PWREx_Core_Select PWREx Core definition
  * @{
  */
#define PWR_CORE_CPU1               (0x00000000U)
#define PWR_CORE_CPU2               (0x00000001U)
/**
  * @}
  */

/**
  * @}
  */
/* Private define ------------------------------------------------------------*/
/** @defgroup PWR_Private_Defines PWR Private Defines
  * @{
  */

/** @defgroup PWREx_PVM_EXTI_LINE PWR PVM external interrupts lines
  * @{
  */
#define PWR_EXTI_LINE_PVM1  (LL_EXTI_LINE_31)  /*!< External interrupt line 31 Connected to PVM1 */
#define PWR_EXTI_LINE_PVM3  (LL_EXTI_LINE_33)  /*!< External interrupt line 33 Connected to PVM3 */
/**
  * @}
  */

/** @defgroup PWR_PVM_Mode_Mask PWR PVM Mode Mask
  * @{
  */
/* Note: On STM32WB serie, power PVD event is not available on AIEC lines     */
/*       (only interruption is available through AIEC line 16).               */
#define PVM_MODE_IT               (0x00010000U)  /*!< Mask for interruption yielded by PVM threshold crossing */
#define PVM_MODE_EVT              (0x00020000U)  /*!< Mask for event yielded by PVM threshold crossing */
#define PVM_RISING_EDGE           (0x00000001U)  /*!< Mask for rising edge set as PVM trigger                 */
#define PVM_FALLING_EDGE          (0x00000002U)  /*!< Mask for falling edge set as PVM trigger                */
#define PVM_RISING_FALLING_EDGE   (0x00000003U)  /*!< Mask for rising and falling edges set as PVM trigger    */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup PWREx_Exported_Macros PWR Extended Exported Macros
 * @{
 */

/**
  * @brief Enable the PVM1 Extended Interrupt C1 Line.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_ENABLE_IT()   LL_EXTI_EnableIT_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Enable the PVM1 Extended Interrupt C2 Line.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTIC2_ENABLE_IT()   LL_C2_EXTI_EnableIT_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Disable the PVM1 Extended Interrupt C1 Line.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_DISABLE_IT()  LL_EXTI_DisableIT_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Disable the PVM1 Extended Interrupt C2 Line.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTIC2_DISABLE_IT()  LL_C2_EXTI_DisableIT_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Enable the PVM1 Event C1 Line.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_ENABLE_EVENT()   LL_EXTI_EnableEvent_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Enable the PVM1 Event C2 Line.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTIC2_ENABLE_EVENT()   LL_C2_EXTI_EnableEvent_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Disable the PVM1 Event C1 Line.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_DISABLE_EVENT()  LL_EXTI_DisableEvent_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Disable the PVM1 Event C2 Line.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTIC2_DISABLE_EVENT()  LL_C2_EXTI_DisableEvent_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Enable the PVM1 Extended Interrupt Rising Trigger.
  * @note  PVM1 flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVM1 voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_ENABLE_RISING_EDGE()   LL_EXTI_EnableRisingTrig_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Disable the PVM1 Extended Interrupt Rising Trigger.
  * @note  PVM1 flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVM1 voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_DISABLE_RISING_EDGE()  LL_EXTI_DisableRisingTrig_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Enable the PVM1 Extended Interrupt Falling Trigger.
  * @note  PVM1 flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVM1 voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_ENABLE_FALLING_EDGE()   LL_EXTI_EnableFallingTrig_0_31(PWR_EXTI_LINE_PVM1)


/**
  * @brief Disable the PVM1 Extended Interrupt Falling Trigger.
  * @note  PVM1 flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVM1 voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_DISABLE_FALLING_EDGE()  LL_EXTI_DisableFallingTrig_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief  PVM1 EXTI line configuration: set rising & falling edge trigger.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_ENABLE_RISING_FALLING_EDGE()  \
  do {                                                    \
    __HAL_PWR_PVM1_EXTI_ENABLE_RISING_EDGE();             \
    __HAL_PWR_PVM1_EXTI_ENABLE_FALLING_EDGE();            \
  } while(0)

/**
  * @brief Disable the PVM1 Extended Interrupt Rising & Falling Trigger.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_DISABLE_RISING_FALLING_EDGE()  \
  do {                                                     \
    __HAL_PWR_PVM1_EXTI_DISABLE_RISING_EDGE();             \
    __HAL_PWR_PVM1_EXTI_DISABLE_FALLING_EDGE();            \
  } while(0)

/**
  * @brief  Generate a Software interrupt on selected EXTI line.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_GENERATE_SWIT() LL_EXTI_GenerateSWI_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Check whether the specified PVM1 EXTI interrupt flag is set or not.
  * @retval EXTI PVM1 Line Status.
  */
#define __HAL_PWR_PVM1_EXTI_GET_FLAG()  LL_EXTI_ReadFlag_0_31(PWR_EXTI_LINE_PVM1)

/**
  * @brief Clear the PVM1 EXTI flag.
  * @retval None
  */
#define __HAL_PWR_PVM1_EXTI_CLEAR_FLAG()  LL_EXTI_ClearFlag_0_31(PWR_EXTI_LINE_PVM1)


/**
  * @brief Enable the PVM3 Extended Interrupt C1 Line.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_ENABLE_IT()   LL_EXTI_EnableIT_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Enable the PVM3 Extended Interrupt C2 Line.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTIC2_ENABLE_IT()   LL_C2_EXTI_EnableIT_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Disable the PVM3 Extended Interrupt C1 Line.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_DISABLE_IT()  LL_EXTI_DisableIT_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Disable the PVM3 Extended Interrupt C2 Line.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTIC2_DISABLE_IT()  LL_C2_EXTI_DisableIT_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Enable the PVM3 Event C1 Line.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_ENABLE_EVENT()   LL_EXTI_EnableEvent_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Enable the PVM3 Event C2 Line.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTIC2_ENABLE_EVENT()   LL_C2_EXTI_EnableEvent_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Disable the PVM3 Event C1 Line.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_DISABLE_EVENT()  LL_EXTI_DisableEvent_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Disable the PVM3 Event C2 Line.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTIC2_DISABLE_EVENT()  LL_C2_EXTI_DisableEvent_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Enable the PVM3 Extended Interrupt Rising Trigger.
  * @note  PVM3 flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVM3 voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_ENABLE_RISING_EDGE()   LL_EXTI_EnableRisingTrig_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Disable the PVM3 Extended Interrupt Rising Trigger.
  * @note  PVM3 flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVM3 voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_DISABLE_RISING_EDGE()  LL_EXTI_DisableRisingTrig_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Enable the PVM3 Extended Interrupt Falling Trigger.
  * @note  PVM3 flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVM3 voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_ENABLE_FALLING_EDGE()   LL_EXTI_EnableFallingTrig_32_63(PWR_EXTI_LINE_PVM3)


/**
  * @brief Disable the PVM3 Extended Interrupt Falling Trigger.
  * @note  PVM3 flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVM3 voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_DISABLE_FALLING_EDGE()  LL_EXTI_DisableFallingTrig_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief  PVM3 EXTI line configuration: set rising & falling edge trigger.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_ENABLE_RISING_FALLING_EDGE()  \
  do {                                                    \
    __HAL_PWR_PVM3_EXTI_ENABLE_RISING_EDGE();             \
    __HAL_PWR_PVM3_EXTI_ENABLE_FALLING_EDGE();            \
  } while(0)

/**
  * @brief Disable the PVM3 Extended Interrupt Rising & Falling Trigger.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_DISABLE_RISING_FALLING_EDGE()  \
  do {                                                     \
    __HAL_PWR_PVM3_EXTI_DISABLE_RISING_EDGE();             \
    __HAL_PWR_PVM3_EXTI_DISABLE_FALLING_EDGE();            \
  } while(0)

/**
  * @brief  Generate a Software interrupt on selected EXTI line.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_GENERATE_SWIT() LL_EXTI_GenerateSWI_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Check whether the specified PVM3 EXTI interrupt flag is set or not.
  * @retval EXTI PVM3 Line Status.
  */
#define __HAL_PWR_PVM3_EXTI_GET_FLAG()  LL_EXTI_ReadFlag_32_63(PWR_EXTI_LINE_PVM3)

/**
  * @brief Clear the PVM3 EXTI flag.
  * @retval None
  */
#define __HAL_PWR_PVM3_EXTI_CLEAR_FLAG()  LL_EXTI_ClearFlag_32_63(PWR_EXTI_LINE_PVM3)


/**
  * @brief Configure the main internal regulator output voltage.
  * @param __REGULATOR__ specifies the regulator output voltage to achieve
  *         a tradeoff between performance and power consumption.
  *          This parameter can be one of the following values:
  *            @arg @ref PWR_REGULATOR_VOLTAGE_SCALE1  Regulator voltage output range 1 mode,
  *                                                typical output voltage at 1.2 V,
  *                                                system frequency up to 64 MHz.
  *            @arg @ref PWR_REGULATOR_VOLTAGE_SCALE2  Regulator voltage output range 2 mode,
  *                                                typical output voltage at 1.0 V,
  *                                                system frequency up to 16 MHz.
  * @note  This macro is similar to HAL_PWREx_ControlVoltageScaling() API but doesn't check
  *        whether or not VOSF flag is cleared when moving from range 2 to range 1. User
  *        may resort to __HAL_PWR_GET_FLAG() macro to check VOSF bit resetting.
  * @retval None
  */
#define __HAL_PWR_VOLTAGESCALING_CONFIG(__REGULATOR__) do {                                                     \
                                                            __IO uint32_t tmpreg;                               \
                                                            MODIFY_REG(PWR->CR1, PWR_CR1_VOS, (__REGULATOR__)); \
                                                            /* Delay after an RCC peripheral clock enabling */  \
                                                            tmpreg = READ_BIT(PWR->CR1, PWR_CR1_VOS);           \
                                                            UNUSED(tmpreg);                                     \
                                                          } while(0)

/**
  * @brief  Wakeup BLE controller from its sleep mode
  * @note   This bit is automatically reset when 802.15.4 controller
  *         exit its sleep mode.
  * @retval None
  */
#define __HAL_C2_PWR_WAKEUP_BLE() LL_C2_PWR_WakeUp_BLE()

/**
  * @brief  Wakeup 802.15.4 controller from its sleep mode
  * @note   This bit is automatically reset when 802.15.4 controller
  *         exit its sleep mode.
  * @retval None
  */
#define __HAL_C2_PWR_WAKEUP_802_15_4() LL_C2_PWR_WakeUp_802_15_4()

/**
  * @}
  */

/* Private macros --------------------------------------------------------*/
/** @addtogroup  PWREx_Private_Macros   PWR Extended Private Macros
  * @{
  */

#define IS_PWR_WAKEUP_PIN(PIN) (((PIN) == PWR_WAKEUP_PIN1_HIGH) || \
                                ((PIN) == PWR_WAKEUP_PIN2_HIGH) || \
                                ((PIN) == PWR_WAKEUP_PIN3_HIGH) || \
                                ((PIN) == PWR_WAKEUP_PIN4_HIGH) || \
                                ((PIN) == PWR_WAKEUP_PIN5_HIGH) || \
                                ((PIN) == PWR_WAKEUP_PIN1_LOW) || \
                                ((PIN) == PWR_WAKEUP_PIN2_LOW) || \
                                ((PIN) == PWR_WAKEUP_PIN3_LOW) || \
                                ((PIN) == PWR_WAKEUP_PIN4_LOW) || \
                                ((PIN) == PWR_WAKEUP_PIN5_LOW))

#define IS_PWR_WAKEUP_PIN_POLARITY(POLARITY)  (((POLARITY) == PWR_PIN_POLARITY_HIGH) || \
                                               ((POLARITY) == PWR_PIN_POLARITY_LOW))


#define IS_PWR_PVM_TYPE(TYPE) (((TYPE) == PWR_PVM_1) ||\
                               ((TYPE) == PWR_PVM_3))

#define IS_PWR_PVM_MODE(MODE)  (((MODE) == PWR_PVM_MODE_NORMAL)              ||\
                                ((MODE) == PWR_PVM_MODE_IT_RISING)           ||\
                                ((MODE) == PWR_PVM_MODE_IT_FALLING)          ||\
                                ((MODE) == PWR_PVM_MODE_IT_RISING_FALLING)   ||\
                                ((MODE) == PWR_PVM_MODE_EVENT_RISING)        ||\
                                ((MODE) == PWR_PVM_MODE_EVENT_FALLING)       ||\
                                ((MODE) == PWR_PVM_MODE_EVENT_RISING_FALLING))

#define IS_PWR_FLASH_POWERDOWN(__MODE__)    ((((__MODE__) & (PWR_FLASHPD_LPRUN | PWR_FLASHPD_LPSLEEP)) != 0x00u) && \
                                             (((__MODE__) & ~(PWR_FLASHPD_LPRUN | PWR_FLASHPD_LPSLEEP)) == 0x00u))

#define IS_PWR_VOLTAGE_SCALING_RANGE(RANGE) (((RANGE) == PWR_REGULATOR_VOLTAGE_SCALE1) || \
                                             ((RANGE) == PWR_REGULATOR_VOLTAGE_SCALE2))


#define IS_PWR_BATTERY_RESISTOR_SELECT(RESISTOR) (((RESISTOR) == PWR_BATTERY_CHARGING_RESISTOR_5) ||\
                                                  ((RESISTOR) == PWR_BATTERY_CHARGING_RESISTOR_1_5))

#define IS_PWR_BATTERY_CHARGING(CHARGING) (((CHARGING) == PWR_BATTERY_CHARGING_DISABLE) ||\
                                           ((CHARGING) == PWR_BATTERY_CHARGING_ENABLE))


#define IS_PWR_GPIO_BIT_NUMBER(BIT_NUMBER) (((BIT_NUMBER) & GPIO_PIN_MASK) != (uint32_t)0x00)

#define IS_PWR_GPIO(GPIO) (((GPIO) == PWR_GPIO_A) ||\
                           ((GPIO) == PWR_GPIO_B) ||\
                           ((GPIO) == PWR_GPIO_C) ||\
                           ((GPIO) == PWR_GPIO_D) ||\
                           ((GPIO) == PWR_GPIO_E) ||\
                           ((GPIO) == PWR_GPIO_H))

#define IS_PWR_SMPS_MODE(SMPS_MODE) (((SMPS_MODE) == PWR_SMPS_BYPASS)    ||\
                                     ((SMPS_MODE) == PWR_SMPS_STEP_DOWN))

#define IS_PWR_SMPS_STARTUP_CURRENT(SMPS_STARTUP_CURRENT) (((SMPS_STARTUP_CURRENT) == PWR_SMPS_STARTUP_CURRENT_80MA)  ||\
                                                           ((SMPS_STARTUP_CURRENT) == PWR_SMPS_STARTUP_CURRENT_100MA) ||\
                                                           ((SMPS_STARTUP_CURRENT) == PWR_SMPS_STARTUP_CURRENT_120MA) ||\
                                                           ((SMPS_STARTUP_CURRENT) == PWR_SMPS_STARTUP_CURRENT_140MA) ||\
                                                           ((SMPS_STARTUP_CURRENT) == PWR_SMPS_STARTUP_CURRENT_160MA) ||\
                                                           ((SMPS_STARTUP_CURRENT) == PWR_SMPS_STARTUP_CURRENT_180MA) ||\
                                                           ((SMPS_STARTUP_CURRENT) == PWR_SMPS_STARTUP_CURRENT_200MA) ||\
                                                           ((SMPS_STARTUP_CURRENT) == PWR_SMPS_STARTUP_CURRENT_220MA))

#define IS_PWR_SMPS_OUTPUT_VOLTAGE(SMPS_OUTPUT_VOLTAGE) (((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V20) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V25) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V30) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V35) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V40) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V45) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V50) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V55) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V60) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V65) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V70) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V75) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V80) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V85) ||\
                                                         ((SMPS_OUTPUT_VOLTAGE) == PWR_SMPS_OUTPUT_VOLTAGE_1V90))

#define IS_PWR_CORE(CPU)  (((CPU) == PWR_CORE_CPU1) || ((CPU) == PWR_CORE_CPU2))

#define IS_PWR_CORE_HOLD_RELEASE(CPU)  ((CPU) == PWR_CORE_CPU2)

/**
  * @}
  */


/** @addtogroup PWREx_Exported_Functions PWR Extended Exported Functions
  * @{
  */

/** @addtogroup PWREx_Exported_Functions_Group1 Extended Peripheral Control functions
  * @{
  */


/* Peripheral Control functions  **********************************************/
uint32_t          HAL_PWREx_GetVoltageRange(void);
HAL_StatusTypeDef HAL_PWREx_ControlVoltageScaling(uint32_t VoltageScaling);

void              HAL_PWREx_EnableBatteryCharging(uint32_t ResistorSelection);
void              HAL_PWREx_DisableBatteryCharging(void);

void              HAL_PWREx_EnableVddUSB(void);
void              HAL_PWREx_DisableVddUSB(void);

void              HAL_PWREx_EnableInternalWakeUpLine(void);
void              HAL_PWREx_DisableInternalWakeUpLine(void);

void              HAL_PWREx_EnableBORH_SMPSBypassIT(void);
void              HAL_PWREx_DisableBORH_SMPSBypassIT(void);
void              HAL_PWREx_EnableRFPhaseIT(void);
void              HAL_PWREx_DisableRFPhaseIT(void);
void              HAL_PWREx_EnableBLEActivityIT(void);
void              HAL_PWREx_DisableBLEActivityIT(void);
void              HAL_PWREx_Enable802ActivityIT(void);
void              HAL_PWREx_Disable802ActivityIT(void);
void              HAL_PWREx_EnableHOLDC2IT(void);
void              HAL_PWREx_DisableHOLDC2IT(void);

void              HAL_PWREx_HoldCore(uint32_t CPU);
void              HAL_PWREx_ReleaseCore(uint32_t CPU);

HAL_StatusTypeDef HAL_PWREx_EnableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber);
HAL_StatusTypeDef HAL_PWREx_DisableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber);
HAL_StatusTypeDef HAL_PWREx_EnableGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber);
HAL_StatusTypeDef HAL_PWREx_DisableGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber);
void              HAL_PWREx_EnablePullUpPullDownConfig(void);
void              HAL_PWREx_DisablePullUpPullDownConfig(void);

void              HAL_PWREx_SetBORConfig(uint32_t BORConfiguration);
uint32_t          HAL_PWREx_GetBORConfig(void);

void              HAL_PWREx_EnableSRAMRetention(void);
void              HAL_PWREx_DisableSRAMRetention(void);

void              HAL_PWREx_EnableFlashPowerDown(uint32_t PowerMode);
void              HAL_PWREx_DisableFlashPowerDown(uint32_t PowerMode);

void              HAL_PWREx_EnablePVM1(void);
void              HAL_PWREx_DisablePVM1(void);

void              HAL_PWREx_EnablePVM3(void);
void              HAL_PWREx_DisablePVM3(void);

HAL_StatusTypeDef HAL_PWREx_ConfigPVM(PWR_PVMTypeDef *sConfigPVM);

HAL_StatusTypeDef HAL_PWREx_ConfigSMPS(PWR_SMPSTypeDef *sConfigSMPS);
void              HAL_PWREx_SMPS_SetMode(uint32_t OperatingMode);
uint32_t          HAL_PWREx_SMPS_GetEffectiveMode(void);

/* WakeUp pins configuration functions ****************************************/
void              HAL_PWREx_EnableWakeUpPin(uint32_t WakeUpPinPolarity, uint32_t wakeupTarget);
uint32_t          HAL_PWREx_GetWakeupFlag(uint32_t WakeUpFlag);
HAL_StatusTypeDef HAL_PWREx_ClearWakeupFlag(uint32_t WakeUpFlag);

/* Low Power modes configuration functions ************************************/
void              HAL_PWREx_EnableLowPowerRunMode(void);
HAL_StatusTypeDef HAL_PWREx_DisableLowPowerRunMode(void);

void              HAL_PWREx_EnterSTOP0Mode(uint8_t STOPEntry);
void              HAL_PWREx_EnterSTOP1Mode(uint8_t STOPEntry);
void              HAL_PWREx_EnterSTOP2Mode(uint8_t STOPEntry);
void              HAL_PWREx_EnterSHUTDOWNMode(void);

void              HAL_PWREx_PVD_PVM_IRQHandler(void);

void              HAL_PWREx_PVM1Callback(void);
void              HAL_PWREx_PVM3Callback(void);

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


#endif /* STM32WBxx_HAL_PWR_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
