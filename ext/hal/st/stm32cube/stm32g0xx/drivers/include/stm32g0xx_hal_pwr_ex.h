/**
  ******************************************************************************
  * @file    stm32g0xx_hal_pwr_ex.h
  * @author  MCD Application Team
  * @brief   Header file of PWR HAL Extended module.
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
#ifndef STM32G0xx_HAL_PWR_EX_H
#define STM32G0xx_HAL_PWR_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal_def.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @defgroup PWREx PWREx
  * @brief PWR Extended HAL module driver
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup PWREx_Exported_Constants  PWR Extended Exported Constants
  * @{
  */

/** @defgroup PWREx_VBAT_Battery_Charging_Selection  PWR battery charging resistor selection
  * @{
  */
#define PWR_BATTERY_CHARGING_RESISTOR_5     (0x00000000u)   /*!< VBAT charging through a 5 kOhms resistor   */
#define PWR_BATTERY_CHARGING_RESISTOR_1_5   PWR_CR4_VBRS    /*!< VBAT charging through a 1.5 kOhms resistor */
/**
  * @}
  */

/** @defgroup PWREx_GPIO_Bit_Number  GPIO bit position
  * @brief    for I/O pull up/down setting in standby/shutdown mode
  * @{
  */
#define PWR_GPIO_BIT_0                      PWR_PUCRB_PU0   /*!< GPIO port I/O pin 0  */
#define PWR_GPIO_BIT_1                      PWR_PUCRB_PU1   /*!< GPIO port I/O pin 1  */
#define PWR_GPIO_BIT_2                      PWR_PUCRB_PU2   /*!< GPIO port I/O pin 2  */
#define PWR_GPIO_BIT_3                      PWR_PUCRB_PU3   /*!< GPIO port I/O pin 3  */
#define PWR_GPIO_BIT_4                      PWR_PUCRB_PU4   /*!< GPIO port I/O pin 4  */
#define PWR_GPIO_BIT_5                      PWR_PUCRB_PU5   /*!< GPIO port I/O pin 5  */
#define PWR_GPIO_BIT_6                      PWR_PUCRB_PU6   /*!< GPIO port I/O pin 6  */
#define PWR_GPIO_BIT_7                      PWR_PUCRB_PU7   /*!< GPIO port I/O pin 7  */
#define PWR_GPIO_BIT_8                      PWR_PUCRB_PU8   /*!< GPIO port I/O pin 8  */
#define PWR_GPIO_BIT_9                      PWR_PUCRB_PU9   /*!< GPIO port I/O pin 9  */
#define PWR_GPIO_BIT_10                     PWR_PUCRB_PU10  /*!< GPIO port I/O pin 10 */
#define PWR_GPIO_BIT_11                     PWR_PUCRB_PU11  /*!< GPIO port I/O pin 11 */
#define PWR_GPIO_BIT_12                     PWR_PUCRB_PU12  /*!< GPIO port I/O pin 12 */
#define PWR_GPIO_BIT_13                     PWR_PUCRB_PU13  /*!< GPIO port I/O pin 13 */
#define PWR_GPIO_BIT_14                     PWR_PUCRB_PU14  /*!< GPIO port I/O pin 14 */
#define PWR_GPIO_BIT_15                     PWR_PUCRB_PU15  /*!< GPIO port I/O pin 15 */
/**
  * @}
  */

/** @defgroup PWREx_GPIO_Port  GPIO Port
  * @{
  */
#define PWR_GPIO_A                          (0x00000000u)  /*!< GPIO port A */
#define PWR_GPIO_B                          (0x00000001u)  /*!< GPIO port B */
#define PWR_GPIO_C                          (0x00000002u)  /*!< GPIO port C */
#define PWR_GPIO_D                          (0x00000003u)  /*!< GPIO port D */
#define PWR_GPIO_F                          (0x00000005u)  /*!< GPIO port F */
/**
  * @}
  */

/** @defgroup PWREx_Flash_PowerDown  Flash Power Down modes
  * @{
  */
#define PWR_FLASHPD_LPRUN                   PWR_CR1_FPD_LPRUN  /*!< Enable Flash power down in low power run mode */
#define PWR_FLASHPD_LPSLEEP                 PWR_CR1_FPD_LPSLP  /*!< Enable Flash power down in low power sleep mode */
#define PWR_FLASHPD_STOP                    PWR_CR1_FPD_STOP   /*!< Enable Flash power down in stop mode */
/**
  * @}
  */

/** @defgroup PWREx_Regulator_Voltage_Scale  PWR Regulator voltage scale
  * @{
  */
#define PWR_REGULATOR_VOLTAGE_SCALE1        PWR_CR1_VOS_0  /*!< Voltage scaling range 1 */
#define PWR_REGULATOR_VOLTAGE_SCALE2        PWR_CR1_VOS_1  /*!< Voltage scaling range 2 */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/** @addtogroup  PWREx_Private_Macros   PWR Extended Private Macros
  * @{
  */
#define IS_PWR_BATTERY_RESISTOR_SELECT(__RESISTOR__) (((__RESISTOR__) == PWR_BATTERY_CHARGING_RESISTOR_5) || \
                                                      ((__RESISTOR__) == PWR_BATTERY_CHARGING_RESISTOR_1_5))

#define IS_PWR_GPIO_BIT_NUMBER(__BIT_NUMBER__)  ((((__BIT_NUMBER__) & 0x0000FFFFu) != 0x00u) && \
                                                 (((__BIT_NUMBER__) & 0xFFFF0000u) == 0x00u))

#define IS_PWR_GPIO(__GPIO__)               (((__GPIO__) == PWR_GPIO_A) || \
                                             ((__GPIO__) == PWR_GPIO_B) || \
                                             ((__GPIO__) == PWR_GPIO_C) || \
                                             ((__GPIO__) == PWR_GPIO_D) || \
                                             ((__GPIO__) == PWR_GPIO_F))

#define IS_PWR_FLASH_POWERDOWN(__MODE__)    ((((__MODE__) & (PWR_FLASHPD_LPRUN | PWR_FLASHPD_LPSLEEP | PWR_FLASHPD_STOP)) != 0x00u) && \
                                             (((__MODE__) & ~(PWR_FLASHPD_LPRUN | PWR_FLASHPD_LPSLEEP | PWR_FLASHPD_STOP)) == 0x00u))

#define IS_PWR_VOLTAGE_SCALING_RANGE(RANGE) (((RANGE) == PWR_REGULATOR_VOLTAGE_SCALE1) || \
                                             ((RANGE) == PWR_REGULATOR_VOLTAGE_SCALE2))

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup PWREx_Exported_Functions  PWR Extended Exported Functions
  * @{
  */

/** @defgroup PWREx_Exported_Functions_Group1  Extended Peripheral Control functions
  * @{
  */

/* Peripheral Control functions  **********************************************/
void              HAL_PWREx_EnableBatteryCharging(uint32_t ResistorSelection);
void              HAL_PWREx_DisableBatteryCharging(void);
#if defined(PWR_CR3_ENB_ULP)
void              HAL_PWREx_EnablePORMonitorSampling(void);
void              HAL_PWREx_DisablePORMonitorSampling(void);
#endif
void              HAL_PWREx_EnableInternalWakeUpLine(void);
void              HAL_PWREx_DisableInternalWakeUpLine(void);
HAL_StatusTypeDef HAL_PWREx_EnableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber);
HAL_StatusTypeDef HAL_PWREx_DisableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber);
HAL_StatusTypeDef HAL_PWREx_EnableGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber);
HAL_StatusTypeDef HAL_PWREx_DisableGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber);
void              HAL_PWREx_EnablePullUpPullDownConfig(void);
void              HAL_PWREx_DisablePullUpPullDownConfig(void);
#if defined(PWR_CR3_RRS)
void              HAL_PWREx_EnableSRAMRetention(void);
void              HAL_PWREx_DisableSRAMRetention(void);
#endif
void              HAL_PWREx_EnableFlashPowerDown(uint32_t PowerMode);
void              HAL_PWREx_DisableFlashPowerDown(uint32_t PowerMode);
uint32_t          HAL_PWREx_GetVoltageRange(void);
HAL_StatusTypeDef HAL_PWREx_ControlVoltageScaling(uint32_t VoltageScaling);

/* Low Power modes configuration functions ************************************/
void              HAL_PWREx_EnableLowPowerRunMode(void);
HAL_StatusTypeDef HAL_PWREx_DisableLowPowerRunMode(void);
#if defined(PWR_SHDW_SUPPORT)
void              HAL_PWREx_EnterSHUTDOWNMode(void);
#endif

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


#endif /* STM32G0xx_HAL_PWR_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
