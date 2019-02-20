/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_pwr_ex.h
  * @author  MCD Application Team
  * @brief   Header file of PWR HAL Extension module.
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
#ifndef __STM32MP1xx_HAL_PWR_EX_H
#define __STM32MP1xx_HAL_PWR_EX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"



/** @addtogroup STM32MP1xx_HAL_Driver
 * @{
 */

/** @addtogroup PWREx
 * @{
 */

/* Exported types ------------------------------------------------------------*/
/** @defgroup PWREx_Exported_Types PWREx Exported Types
  * @{
  */
/**
  * @brief  PWREx AVD configuration structure definition
  */
typedef struct
{
  uint32_t AVDLevel;       /*!<   AVDLevel: Specifies the AVD detection level.
                                  This parameter can be a value of @ref PWREx AVD detection level */
  uint32_t Mode;            /*!< Mode: Specifies the operating mode for the selected pins.
                                  This parameter can be a value of @ref PWREx AVD Mode */
} PWREx_AVDTypeDef;
/**
  * @}
  */


/* Exported constants --------------------------------------------------------*/

/** @defgroup PWREx_Exported_Constants PWREx Exported Constants
  * @{
  */

/** @defgroup PWREx_Exported_Constants_Group1 PWREx_WakeUp_Pins
 * @{
 */

#ifdef CORE_CA7
/* Defines for legacy purpose */
#define PWR_WAKEUP_PIN_MASK            PWR_MPUWKUPENR_WKUPEN
#define PWR_WAKEUP_PIN6                PWR_MPUWKUPENR_WKUPEN_6
#define PWR_WAKEUP_PIN5                PWR_MPUWKUPENR_WKUPEN_5
#define PWR_WAKEUP_PIN4                PWR_MPUWKUPENR_WKUPEN_4
#define PWR_WAKEUP_PIN3                PWR_MPUWKUPENR_WKUPEN_3
#define PWR_WAKEUP_PIN2                PWR_MPUWKUPENR_WKUPEN_2
#define PWR_WAKEUP_PIN1                PWR_MPUWKUPENR_WKUPEN_1
/* Defines for legacy purpose */

/* High level + No pull */
#define PWR_WAKEUP_PIN6_HIGH            PWR_MPUWKUPENR_WKUPEN_6
#define PWR_WAKEUP_PIN5_HIGH            PWR_MPUWKUPENR_WKUPEN_5
#define PWR_WAKEUP_PIN4_HIGH            PWR_MPUWKUPENR_WKUPEN_4
#define PWR_WAKEUP_PIN3_HIGH            PWR_MPUWKUPENR_WKUPEN_3
#define PWR_WAKEUP_PIN2_HIGH            PWR_MPUWKUPENR_WKUPEN_2
#define PWR_WAKEUP_PIN1_HIGH            PWR_MPUWKUPENR_WKUPEN_1
/* Low level + No pull */
#define PWR_WAKEUP_PIN6_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_6 | PWR_MPUWKUPENR_WKUPEN_6)
#define PWR_WAKEUP_PIN5_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_5 | PWR_MPUWKUPENR_WKUPEN_5)
#define PWR_WAKEUP_PIN4_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_4 | PWR_MPUWKUPENR_WKUPEN_4)
#define PWR_WAKEUP_PIN3_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_3 | PWR_MPUWKUPENR_WKUPEN_3)
#define PWR_WAKEUP_PIN2_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_2 | PWR_MPUWKUPENR_WKUPEN_2)
#define PWR_WAKEUP_PIN1_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_1 | PWR_MPUWKUPENR_WKUPEN_1)
#endif /*CORE_CA7*/

#ifdef CORE_CM4
/* Defines for legacy purpose */
#define PWR_WAKEUP_PIN_MASK            PWR_MCUWKUPENR_WKUPEN
#define PWR_WAKEUP_PIN6                PWR_MCUWKUPENR_WKUPEN6
#define PWR_WAKEUP_PIN5                PWR_MCUWKUPENR_WKUPEN5
#define PWR_WAKEUP_PIN4                PWR_MCUWKUPENR_WKUPEN4
#define PWR_WAKEUP_PIN3                PWR_MCUWKUPENR_WKUPEN3
#define PWR_WAKEUP_PIN2                PWR_MCUWKUPENR_WKUPEN2
#define PWR_WAKEUP_PIN1                PWR_MCUWKUPENR_WKUPEN1
/* Defines for legacy purpose */

/* High level + No pull */
#define PWR_WAKEUP_PIN6_HIGH            PWR_MCUWKUPENR_WKUPEN6
#define PWR_WAKEUP_PIN5_HIGH            PWR_MCUWKUPENR_WKUPEN5
#define PWR_WAKEUP_PIN4_HIGH            PWR_MCUWKUPENR_WKUPEN4
#define PWR_WAKEUP_PIN3_HIGH            PWR_MCUWKUPENR_WKUPEN3
#define PWR_WAKEUP_PIN2_HIGH            PWR_MCUWKUPENR_WKUPEN2
#define PWR_WAKEUP_PIN1_HIGH            PWR_MCUWKUPENR_WKUPEN1
/* Low level + No pull */
#define PWR_WAKEUP_PIN6_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_6 | PWR_MCUWKUPENR_WKUPEN6)
#define PWR_WAKEUP_PIN5_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_5 | PWR_MCUWKUPENR_WKUPEN5)
#define PWR_WAKEUP_PIN4_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_4 | PWR_MCUWKUPENR_WKUPEN4)
#define PWR_WAKEUP_PIN3_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_3 | PWR_MCUWKUPENR_WKUPEN3)
#define PWR_WAKEUP_PIN2_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_2 | PWR_MCUWKUPENR_WKUPEN2)
#define PWR_WAKEUP_PIN1_LOW             (uint32_t)(PWR_WKUPCR_WKUPP_1 | PWR_MCUWKUPENR_WKUPEN1)
#endif /*CORE_CM4*/

/* High level + Pull-up */
#define PWR_WAKEUP_PIN6_HIGH_PULLUP     (uint32_t)(PWR_MPUWKUPENR_WKUPEN_6 | PWR_WKUPCR_WKUPPUPD6_0 )
#define PWR_WAKEUP_PIN5_HIGH_PULLUP     (uint32_t)(PWR_MPUWKUPENR_WKUPEN_5 | PWR_WKUPCR_WKUPPUPD5_0 )
#define PWR_WAKEUP_PIN4_HIGH_PULLUP     (uint32_t)(PWR_MPUWKUPENR_WKUPEN_4 | PWR_WKUPCR_WKUPPUPD4_0 )
#define PWR_WAKEUP_PIN3_HIGH_PULLUP     (uint32_t)(PWR_MPUWKUPENR_WKUPEN_3 | PWR_WKUPCR_WKUPPUPD3_0 )
#define PWR_WAKEUP_PIN2_HIGH_PULLUP     (uint32_t)(PWR_MPUWKUPENR_WKUPEN_2 | PWR_WKUPCR_WKUPPUPD2_0 )
#define PWR_WAKEUP_PIN1_HIGH_PULLUP     (uint32_t)(PWR_MPUWKUPENR_WKUPEN_1 | PWR_WKUPCR_WKUPPUPD1_0 )
/* Low level + Pull-up */
#define PWR_WAKEUP_PIN6_LOW_PULLUP      (uint32_t)(PWR_WKUPCR_WKUPP_6 | PWR_MPUWKUPENR_WKUPEN_6 | PWR_WKUPCR_WKUPPUPD6_0)
#define PWR_WAKEUP_PIN5_LOW_PULLUP      (uint32_t)(PWR_WKUPCR_WKUPP_5 | PWR_MPUWKUPENR_WKUPEN_5 | PWR_WKUPCR_WKUPPUPD5_0)
#define PWR_WAKEUP_PIN4_LOW_PULLUP      (uint32_t)(PWR_WKUPCR_WKUPP_4 | PWR_MPUWKUPENR_WKUPEN_4 | PWR_WKUPCR_WKUPPUPD4_0)
#define PWR_WAKEUP_PIN3_LOW_PULLUP      (uint32_t)(PWR_WKUPCR_WKUPP_3 | PWR_MPUWKUPENR_WKUPEN_3 | PWR_WKUPCR_WKUPPUPD3_0)
#define PWR_WAKEUP_PIN2_LOW_PULLUP      (uint32_t)(PWR_WKUPCR_WKUPP_2 | PWR_MPUWKUPENR_WKUPEN_2 | PWR_WKUPCR_WKUPPUPD2_0)
#define PWR_WAKEUP_PIN1_LOW_PULLUP      (uint32_t)(PWR_WKUPCR_WKUPP_1 | PWR_MPUWKUPENR_WKUPEN_1 | PWR_WKUPCR_WKUPPUPD1_0)
/* High level + Pull-down */
#define PWR_WAKEUP_PIN6_HIGH_PULLDOWN   (uint32_t)(PWR_MPUWKUPENR_WKUPEN_6 | PWR_WKUPCR_WKUPPUPD6_1 )
#define PWR_WAKEUP_PIN5_HIGH_PULLDOWN   (uint32_t)(PWR_MPUWKUPENR_WKUPEN_5 | PWR_WKUPCR_WKUPPUPD5_1 )
#define PWR_WAKEUP_PIN4_HIGH_PULLDOWN   (uint32_t)(PWR_MPUWKUPENR_WKUPEN_4 | PWR_WKUPCR_WKUPPUPD4_1 )
#define PWR_WAKEUP_PIN3_HIGH_PULLDOWN   (uint32_t)(PWR_MPUWKUPENR_WKUPEN_3 | PWR_WKUPCR_WKUPPUPD3_1 )
#define PWR_WAKEUP_PIN2_HIGH_PULLDOWN   (uint32_t)(PWR_MPUWKUPENR_WKUPEN_2 | PWR_WKUPCR_WKUPPUPD2_1 )
#define PWR_WAKEUP_PIN1_HIGH_PULLDOWN   (uint32_t)(PWR_MPUWKUPENR_WKUPEN_1 | PWR_WKUPCR_WKUPPUPD1_1 )
/* Low level + Pull-down */
#define PWR_WAKEUP_PIN6_LOW_PULLDOWN    (uint32_t)(PWR_WKUPCR_WKUPP_6 | PWR_MPUWKUPENR_WKUPEN_6 | PWR_WKUPCR_WKUPPUPD6_1)
#define PWR_WAKEUP_PIN5_LOW_PULLDOWN    (uint32_t)(PWR_WKUPCR_WKUPP_5 | PWR_MPUWKUPENR_WKUPEN_5 | PWR_WKUPCR_WKUPPUPD5_1)
#define PWR_WAKEUP_PIN4_LOW_PULLDOWN    (uint32_t)(PWR_WKUPCR_WKUPP_4 | PWR_MPUWKUPENR_WKUPEN_4 | PWR_WKUPCR_WKUPPUPD4_1)
#define PWR_WAKEUP_PIN3_LOW_PULLDOWN    (uint32_t)(PWR_WKUPCR_WKUPP_3 | PWR_MPUWKUPENR_WKUPEN_3 | PWR_WKUPCR_WKUPPUPD3_1)
#define PWR_WAKEUP_PIN2_LOW_PULLDOWN    (uint32_t)(PWR_WKUPCR_WKUPP_2 | PWR_MPUWKUPENR_WKUPEN_2 | PWR_WKUPCR_WKUPPUPD2_1)
#define PWR_WAKEUP_PIN1_LOW_PULLDOWN    (uint32_t)(PWR_WKUPCR_WKUPP_1 | PWR_MPUWKUPENR_WKUPEN_1 | PWR_WKUPCR_WKUPPUPD1_1)
/**
  * @}
  */

/** @defgroup PWREx_Exported_Constants_Group2 PWREx Wakeup Pins Flags
 * @{
 */
#define PWR_WAKEUP_PIN_FLAG1            PWR_WKUPFR_WKUPF1 /*!< Wakeup event on pin 1 */
#define PWR_WAKEUP_PIN_FLAG2            PWR_WKUPFR_WKUPF2 /*!< Wakeup event on pin 2 */
#define PWR_WAKEUP_PIN_FLAG3            PWR_WKUPFR_WKUPF3 /*!< Wakeup event on pin 3 */
#define PWR_WAKEUP_PIN_FLAG4            PWR_WKUPFR_WKUPF4 /*!< Wakeup event on pin 4 */
#define PWR_WAKEUP_PIN_FLAG5            PWR_WKUPFR_WKUPF5 /*!< Wakeup event on pin 5 */
#define PWR_WAKEUP_PIN_FLAG6            PWR_WKUPFR_WKUPF6 /*!< Wakeup event on pin 6 */
/**
 * @}
 */


/** @defgroup PWREx_Exported_Constants_Group3 PWREx Core definition
 * @{
 */
#define PWR_CORE_CPU1               ((uint32_t)0x00)
#define PWR_CORE_CPU2               ((uint32_t)0x01)
/**
 * @}
 */


/** @defgroup PWREx_Exported_Constants_Group4 PWREx AVD detection level
  * @{
  */
#define PWR_AVDLEVEL_0  PWR_CR1_ALS_LEV0 /* 1.7 V */
#define PWR_AVDLEVEL_1  PWR_CR1_ALS_LEV1 /* 2.1 V */
#define PWR_AVDLEVEL_2  PWR_CR1_ALS_LEV2 /* 2.5 V */
#define PWR_AVDLEVEL_3  PWR_CR1_ALS_LEV3 /* 2.8 V */
/**
  * @}
  */

/** @defgroup PWREx_Exported_Constants_Group5 PWREx AVD Mode
  * @{
  */
#define PWR_AVD_MODE_NORMAL                ((uint32_t)0x00000000U)   /*!< Basic mode is used */
#define PWR_AVD_MODE_IT_RISING             ((uint32_t)0x00010001U)   /*!< External Interrupt Mode with Rising edge trigger detection */
#define PWR_AVD_MODE_IT_FALLING            ((uint32_t)0x00010002U)   /*!< External Interrupt Mode with Falling edge trigger detection */
#define PWR_AVD_MODE_IT_RISING_FALLING     ((uint32_t)0x00010003U)   /*!< External Interrupt Mode with Rising/Falling edge trigger detection */
/**
  * @}
  */

/** @defgroup PWREx_Exported_Constants_Group6 PWR battery charging resistor selection
  * @{
  */
#define PWR_BATTERY_CHARGING_RESISTOR_5    ((uint32_t)0x00000000U) /*!< VBAT charging through a 5 kOhm resistor   */
#define PWR_BATTERY_CHARGING_RESISTOR_1_5  PWR_CR3_VBRS            /*!< VBAT charging through a 1.5 kOhm resistor */
/**
  * @}
  */

/** @defgroup PWREx_Exported_Constants_Group7 PWREx VBAT Thresholds
  * @{
  */
#define PWR_VBAT_BETWEEN_HIGH_LOW_THRESHOLD  ((uint32_t)0x00000000U)
#define PWR_VBAT_BELOW_LOW_THRESHOLD         PWR_CR2_VBATL  /*!< Vsw low threshold is ~1.35V */
#define PWR_VBAT_ABOVE_HIGH_THRESHOLD        PWR_CR2_VBATH  /*!< Vsw high threshold is ~3.6V */
/**
  * @}
  */

/** @defgroup PWREx_Exported_Constants_Group8 PWREx Temperature Thresholds
  * @{
  */
#define PWR_TEMP_BETWEEN_HIGH_LOW_THRESHOLD  ((uint32_t)0x00000000U)
#define PWR_TEMP_BELOW_LOW_THRESHOLD         PWR_CR2_TEMPL
#define PWR_TEMP_ABOVE_HIGH_THRESHOLD        PWR_CR2_TEMPH
/**
  * @}
  */

/**
 * @}
 */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup PWREx_Exported_Macro PWREx Exported Macro
  *  @{
  */

/** @brief  Check Wake Up flag is set or not.
 * @param  __WUFLAG__: specifies the Wake Up flag to check.
 *          This parameter can be one of the following values:
 *            @arg PWR_WAKEUP_PIN_FLAG1: Wakeup Pin Flag 1
 *            @arg PWR_WAKEUP_PIN_FLAG2: Wakeup Pin Flag 2
 *            @arg PWR_WAKEUP_PIN_FLAG3: Wakeup Pin Flag 3
 *            @arg PWR_WAKEUP_PIN_FLAG4: Wakeup Pin Flag 4
 *            @arg PWR_WAKEUP_PIN_FLAG5: Wakeup Pin Flag 5
 *            @arg PWR_WAKEUP_PIN_FLAG6: Wakeup Pin Flag 6
 */
#define __HAL_PWR_GET_WAKEUP_FLAG(__WUFLAG__) (PWR->WKUPFR & (__WUFLAG__))

/** @brief  Clear the WakeUp pins flags.
 * @param  __WUFLAG__: specifies the Wake Up pin flag to clear.
 *          This parameter can be one of the following values:
 *            @arg PWR_WAKEUP_PIN_FLAG1: Wakeup Pin Flag 1
 *            @arg PWR_WAKEUP_PIN_FLAG2: Wakeup Pin Flag 2
 *            @arg PWR_WAKEUP_PIN_FLAG3: Wakeup Pin Flag 3
 *            @arg PWR_WAKEUP_PIN_FLAG4: Wakeup Pin Flag 4
 *            @arg PWR_WAKEUP_PIN_FLAG5: Wakeup Pin Flag 5
 *            @arg PWR_WAKEUP_PIN_FLAG6: Wakeup Pin Flag 6
 */
#define __HAL_PWR_CLEAR_WAKEUP_FLAG(__WUFLAG__)   SET_BIT(PWR->WKUPCR, (__WUFLAG__))

/**
 * @}
 */

/* Exported functions --------------------------------------------------------*/
/** @defgroup PWREx_Exported_Functions PWREx Exported Functions
  * @{
  */

/** @defgroup PWREx_Exported_Functions_Group1 Low power control functions
  * @{
  */
/* Power core holding functions */
HAL_StatusTypeDef HAL_PWREx_HoldCore(uint32_t CPU);
void              HAL_PWREx_ReleaseCore(uint32_t CPU);


/* Power Wakeup PIN IRQ Handler */
void HAL_PWREx_WAKEUP_PIN_IRQHandler(void);
void HAL_PWREx_WKUP1_Callback(void);
void HAL_PWREx_WKUP2_Callback(void);
void HAL_PWREx_WKUP3_Callback(void);
void HAL_PWREx_WKUP4_Callback(void);
void HAL_PWREx_WKUP5_Callback(void);
void HAL_PWREx_WKUP6_Callback(void);


/**
 * @}
 */

/** @defgroup PWREx_Exported_Functions_Group2 Peripherals control functions
  * @{
  */
/* Backup regulator control functions */
HAL_StatusTypeDef HAL_PWREx_EnableBkUpReg(void);
HAL_StatusTypeDef HAL_PWREx_DisableBkUpReg(void);

/* Retention regulator control functions */
HAL_StatusTypeDef HAL_PWREx_EnableRetReg(void);
HAL_StatusTypeDef HAL_PWREx_DisableRetReg(void);

/* 1V1 regulator control functions */
HAL_StatusTypeDef HAL_PWREx_Enable1V1Reg(void);
HAL_StatusTypeDef HAL_PWREx_Disable1V1Reg(void);

/* 1V8 regulator control functions */
HAL_StatusTypeDef HAL_PWREx_Enable1V8Reg(void);
HAL_StatusTypeDef HAL_PWREx_Disable1V8Reg(void);

/* Battery control functions */
void HAL_PWREx_EnableBatteryCharging(uint32_t ResistorValue);
void HAL_PWREx_DisableBatteryCharging(void);
/**
 * @}
 */

/** @defgroup PWREx_Exported_Functions_Group3 Power Monitoring functions
  * @{
  */
/* Power VBAT/Temperature monitoring functions */
void HAL_PWREx_EnableMonitoring(void);
void HAL_PWREx_DisableMonitoring(void);
uint32_t HAL_PWREx_GetTemperatureLevel(void);
uint32_t HAL_PWREx_GetVBATLevel(void);

/* USB Voltage level detector functions */
HAL_StatusTypeDef HAL_PWREx_EnableUSBVoltageDetector(void);
HAL_StatusTypeDef HAL_PWREx_DisableUSBVoltageDetector(void);

/* Power AVD configuration functions */
void HAL_PWREx_ConfigAVD(PWREx_AVDTypeDef *sConfigAVD);
void HAL_PWREx_EnableAVD(void);
void HAL_PWREx_DisableAVD(void);

/* Power PVD/AVD IRQ Handler */
void HAL_PWREx_PVD_AVD_IRQHandler(void);
void HAL_PWREx_AVDCallback(void);
/**
 * @}
 */

/**
 * @}
 */


/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/** @defgroup PWREx_Private_Macros PWREx Private Macros
  * @{
  */

/** @defgroup PWREx_IS_PWR_Definitions PWREx Private macros to check input parameters
  * @{
  */

#define IS_PWR_WAKEUP_PIN(__PIN__)         (((__PIN__) == PWR_WAKEUP_PIN1)       || \
                                           ((__PIN__) == PWR_WAKEUP_PIN2)       || \
                                           ((__PIN__) == PWR_WAKEUP_PIN3)       || \
                                           ((__PIN__) == PWR_WAKEUP_PIN4)       || \
                                           ((__PIN__) == PWR_WAKEUP_PIN5)       || \
                                           ((__PIN__) == PWR_WAKEUP_PIN6)       || \
                                           ((__PIN__) == PWR_WAKEUP_PIN1_HIGH)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN2_HIGH)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN3_HIGH)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN4_HIGH)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN5_HIGH)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN6_HIGH)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN1_LOW)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN2_LOW)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN3_LOW)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN4_LOW)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN5_LOW)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN6_LOW)    || \
                                           ((__PIN__) == PWR_WAKEUP_PIN6_HIGH_PULLUP)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN5_HIGH_PULLUP)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN4_HIGH_PULLUP)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN3_HIGH_PULLUP)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN2_HIGH_PULLUP)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN1_HIGH_PULLUP)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN6_LOW_PULLUP)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN5_LOW_PULLUP)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN4_LOW_PULLUP)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN3_LOW_PULLUP)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN2_LOW_PULLUP)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN1_LOW_PULLUP)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN6_HIGH_PULLDOWN)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN5_HIGH_PULLDOWN)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN4_HIGH_PULLDOWN)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN3_HIGH_PULLDOWN)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN2_HIGH_PULLDOWN)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN1_HIGH_PULLDOWN)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN6_LOW_PULLDOWN)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN5_LOW_PULLDOWN)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN4_LOW_PULLDOWN)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN3_LOW_PULLDOWN)   || \
                                           ((__PIN__) == PWR_WAKEUP_PIN2_LOW_PULLDOWN)  || \
                                           ((__PIN__) == PWR_WAKEUP_PIN1_LOW_PULLDOWN))

#define IS_PWR_AVD_LEVEL(LEVEL)  (((LEVEL) == PWR_AVDLEVEL_0) || ((LEVEL) == PWR_AVDLEVEL_1) || \
                                  ((LEVEL) == PWR_AVDLEVEL_2) || ((LEVEL) == PWR_AVDLEVEL_3))

#define IS_PWR_AVD_MODE(MODE)  (((MODE) == PWR_AVD_MODE_IT_RISING)|| ((MODE) == PWR_AVD_MODE_IT_FALLING) || \
                                ((MODE) == PWR_AVD_MODE_IT_RISING_FALLING) || ((MODE) == PWR_AVD_MODE_NORMAL))

#define IS_PWR_BATTERY_RESISTOR_SELECT(RESISTOR)  (((RESISTOR) == PWR_BATTERY_CHARGING_RESISTOR_5) ||\
                                                   ((RESISTOR) == PWR_BATTERY_CHARGING_RESISTOR_1_5))

#define IS_PWR_CORE(CPU)  (((CPU) == PWR_CORE_CPU1) || ((CPU) == PWR_CORE_CPU2))
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


#endif /* __STM32MP1xx_HAL_PWR_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
