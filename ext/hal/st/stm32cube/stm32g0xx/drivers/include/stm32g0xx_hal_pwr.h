/**
  ******************************************************************************
  * @file    stm32g0xx_hal_pwr.h
  * @author  MCD Application Team
  * @brief   Header file of PWR HAL module.
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
#ifndef STM32G0xx_HAL_PWR_H
#define STM32G0xx_HAL_PWR_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal_def.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @defgroup PWR PWR
  * @brief PWR HAL module driver
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup PWR_Exported_Types PWR Exported Types
  * @{
  */

#if defined(PWR_PVD_SUPPORT)
/**
  * @brief  PWR PVD configuration structure definition
  */
typedef struct
{
  uint32_t PVDLevel;    /*!< PVDLevel: Specifies the PVD detection level.
                              This parameter can be a value or a combination of
                              @ref PWR_PVD_detection_level. */

  uint32_t Mode;        /*!< Mode: Specifies the operating mode for the selected pins.
                              This parameter can be a value of @ref PWR_PVD_Mode. */
}PWR_PVDTypeDef;
#endif
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup PWR_Exported_Constants PWR Exported Constants
  * @{
  */

#if defined(PWR_PVD_SUPPORT)
/** @defgroup PWR_PVD_detection_level  Programmable Voltage Detection levels
  * @note   see datasheet for selection voltage value
  * @{
  */
#define PWR_PVDLEVEL_RISING_0               (0x00000000u)                                           /*!< PVD threshold level 0 for rising detection */
#define PWR_PVDLEVEL_RISING_1               (PWR_CR2_PVDRT_0)                                       /*!< PVD threshold level 1 for rising detection */
#define PWR_PVDLEVEL_RISING_2               (PWR_CR2_PVDRT_1)                                       /*!< PVD threshold level 2 for rising detection */
#define PWR_PVDLEVEL_RISING_3               (PWR_CR2_PVDRT_0 | PWR_CR2_PVDRT_1)                     /*!< PVD threshold level 3 for rising detection */
#define PWR_PVDLEVEL_RISING_4               (PWR_CR2_PVDRT_2)                                       /*!< PVD threshold level 4 for rising detection */
#define PWR_PVDLEVEL_RISING_5               (PWR_CR2_PVDRT_2 | PWR_CR2_PVDRT_0)                     /*!< PVD threshold level 5 for rising detection */
#define PWR_PVDLEVEL_RISING_6               (PWR_CR2_PVDRT_2 | PWR_CR2_PVDRT_1)                     /*!< PVD threshold level 6 for rising detection */
#define PWR_PVDLEVEL_FALLING_0              (0x00000000u)                                           /*!< PVD threshold level 0 for falling detection */
#define PWR_PVDLEVEL_FALLING_1              (PWR_CR2_PVDFT_0)                                       /*!< PVD threshold level 1 for falling detection */
#define PWR_PVDLEVEL_FALLING_2              (PWR_CR2_PVDFT_1)                                       /*!< PVD threshold level 2 for falling detection */
#define PWR_PVDLEVEL_FALLING_3              (PWR_CR2_PVDFT_0 | PWR_CR2_PVDFT_1)                     /*!< PVD threshold level 3 for falling detection */
#define PWR_PVDLEVEL_FALLING_4              (PWR_CR2_PVDFT_2)                                       /*!< PVD threshold level 4 for falling detection */
#define PWR_PVDLEVEL_FALLING_5              (PWR_CR2_PVDFT_2 | PWR_CR2_PVDFT_0)                     /*!< PVD threshold level 5 for falling detection */
#define PWR_PVDLEVEL_FALLING_6              (PWR_CR2_PVDFT_2 | PWR_CR2_PVDFT_1)                     /*!< PVD threshold level 6 for falling detection */
#define PWR_PVDLEVEL_0                      (PWR_PVDLEVEL_RISING_0 | PWR_PVDLEVEL_FALLING_0)        /*!< same PVD threshold level 0 on rising & falling */
#define PWR_PVDLEVEL_1                      (PWR_PVDLEVEL_RISING_1 | PWR_PVDLEVEL_FALLING_1)        /*!< same PVD threshold level 1 on rising & falling */
#define PWR_PVDLEVEL_2                      (PWR_PVDLEVEL_RISING_2 | PWR_PVDLEVEL_FALLING_2)        /*!< same PVD threshold level 2 on rising & falling */
#define PWR_PVDLEVEL_3                      (PWR_PVDLEVEL_RISING_3 | PWR_PVDLEVEL_FALLING_3)        /*!< same PVD threshold level 3 on rising & falling */
#define PWR_PVDLEVEL_4                      (PWR_PVDLEVEL_RISING_4 | PWR_PVDLEVEL_FALLING_4)        /*!< same PVD threshold level 4 on rising & falling */
#define PWR_PVDLEVEL_5                      (PWR_PVDLEVEL_RISING_5 | PWR_PVDLEVEL_FALLING_5)        /*!< same PVD threshold level 5 on rising & falling */
#define PWR_PVDLEVEL_6                      (PWR_PVDLEVEL_RISING_6 | PWR_PVDLEVEL_FALLING_6)        /*!< same PVD threshold level 6 on rising & falling */
#define PWR_PVDLEVEL_7                      (PWR_CR2_PVDRT_2 | PWR_CR2_PVDRT_1 | PWR_CR2_PVDRT_0)   /*!< External input analog voltage (compared internally to VREFINT) */
/**
  * @}
  */

/** @defgroup PWR_PVD_Mode  PWR PVD interrupt and event mode
  * @{
  */
#define PWR_PVD_MODE_NORMAL                 (0x00000000u)  /*!< basic mode is used */
#define PWR_PVD_MODE_IT_RISING              (0x00010001u)  /*!< External Interrupt Mode with Rising edge trigger detection */
#define PWR_PVD_MODE_IT_FALLING             (0x00010002u)  /*!< External Interrupt Mode with Falling edge trigger detection */
#define PWR_PVD_MODE_IT_RISING_FALLING      (0x00010003u)  /*!< External Interrupt Mode with Rising/Falling edge trigger detection */
#define PWR_PVD_MODE_EVENT_RISING           (0x00020001u)  /*!< Event Mode with Rising edge trigger detection */
#define PWR_PVD_MODE_EVENT_FALLING          (0x00020002u)  /*!< Event Mode with Falling edge trigger detection */
#define PWR_PVD_MODE_EVENT_RISING_FALLING   (0x00020003u)  /*!< Event Mode with Rising/Falling edge trigger detection */
/**
  * @}
  */

/** @defgroup PWR_PVD_EXTI_LINE  PWR PVD external interrupt line
  * @{
  */
#define PWR_EXTI_LINE_PVD                   (EXTI_IMR1_IM16)  /*!< External interrupt line 16 connected to PVD */
/**
  * @}
  */

/** @defgroup PWR_PVD_EVENT_LINE  PWR PVD event line
  * @{
  */
#define PWR_EVENT_LINE_PVD                  (EXTI_EMR1_EM16)  /*!< Event line 16 connected to PVD */
/**
  * @}
  */
#endif

/** @defgroup PWR_WakeUp_Pins  PWR WakeUp pins
  * @{
  */
#define PWR_WAKEUP_PIN1                     PWR_CR3_EWUP1  /*!< Wakeup pin 1 (with high level detection) */
#define PWR_WAKEUP_PIN2                     PWR_CR3_EWUP2  /*!< Wakeup pin 2 (with high level detection) */
#define PWR_WAKEUP_PIN4                     PWR_CR3_EWUP4  /*!< Wakeup pin 4 (with high level detection) */
#define PWR_WAKEUP_PIN5                     PWR_CR3_EWUP5  /*!< Wakeup pin 5 (with high level detection) */
#define PWR_WAKEUP_PIN6                     PWR_CR3_EWUP6  /*!< Wakeup pin 6 (with high level detection) */
#define PWR_WAKEUP_PIN1_HIGH                PWR_CR3_EWUP1  /*!< Wakeup pin 1 (with high level detection) */
#define PWR_WAKEUP_PIN2_HIGH                PWR_CR3_EWUP2  /*!< Wakeup pin 2 (with high level detection) */
#define PWR_WAKEUP_PIN4_HIGH                PWR_CR3_EWUP4  /*!< Wakeup pin 4 (with high level detection) */
#if defined(PWR_CR3_EWUP5)
#define PWR_WAKEUP_PIN5_HIGH                PWR_CR3_EWUP5  /*!< Wakeup pin 5 (with high level detection) */
#endif
#define PWR_WAKEUP_PIN6_HIGH                PWR_CR3_EWUP6  /*!< Wakeup pin 6 (with high level detection) */
#define PWR_WAKEUP_PIN1_LOW                 ((PWR_CR4_WP1 << PWR_WUP_POLARITY_SHIFT) | PWR_CR3_EWUP1) /*!< Wakeup pin 1 (with low level detection) */
#define PWR_WAKEUP_PIN2_LOW                 ((PWR_CR4_WP2 << PWR_WUP_POLARITY_SHIFT) | PWR_CR3_EWUP2) /*!< Wakeup pin 2 (with low level detection) */
#define PWR_WAKEUP_PIN4_LOW                 ((PWR_CR4_WP4 << PWR_WUP_POLARITY_SHIFT) | PWR_CR3_EWUP4) /*!< Wakeup pin 4 (with low level detection) */
#if defined(PWR_CR3_EWUP5)
#define PWR_WAKEUP_PIN5_LOW                 ((PWR_CR4_WP5 << PWR_WUP_POLARITY_SHIFT) | PWR_CR3_EWUP5) /*!< Wakeup pin 5 (with low level detection) */
#endif
#define PWR_WAKEUP_PIN6_LOW                 ((PWR_CR4_WP6 << PWR_WUP_POLARITY_SHIFT) | PWR_CR3_EWUP6) /*!< Wakeup pin 6 (with low level detection) */
/**
  * @}
  */

/** @defgroup PWR_Low_Power_Mode_Selection  PWR Low Power Mode Selection
  * @{
  */
#define PWR_LOWPOWERMODE_STOP0              (0x00000000u)                       /*!< Stop 0: stop mode with main regulator */
#define PWR_LOWPOWERMODE_STOP1              (PWR_CR1_LPMS_0)                    /*!< Stop 1: stop mode with low power regulator */
#define PWR_LOWPOWERMODE_STANDBY            (PWR_CR1_LPMS_0 | PWR_CR1_LPMS_1)   /*!< Standby mode */
#if defined(PWR_SHDW_SUPPORT)
#define PWR_LOWPOWERMODE_SHUTDOWN           (PWR_CR1_LPMS_2)                    /*!< Shutdown mode */
#endif
/**
  * @}
  */

/** @defgroup PWR_Regulator_state_in_SLEEP_STOP_mode  PWR regulator mode
  * @{
  */
#define PWR_MAINREGULATOR_ON                (0x00000000u)  /*!< Regulator in main mode      */
#define PWR_LOWPOWERREGULATOR_ON            PWR_CR1_LPR    /*!< Regulator in low-power mode */
/**
  * @}
  */

/** @defgroup PWR_SLEEP_mode_entry  PWR SLEEP mode entry
  * @{
  */
#define PWR_SLEEPENTRY_WFI                  ((uint8_t)0x01u)        /*!< Wait For Interruption instruction to enter Sleep mode */
#define PWR_SLEEPENTRY_WFE                  ((uint8_t)0x02u)        /*!< Wait For Event instruction to enter Sleep mode        */
/**
  * @}
  */

/** @defgroup PWR_STOP_mode_entry  PWR STOP mode entry
  * @{
  */
#define PWR_STOPENTRY_WFI                   ((uint8_t)0x01u)        /*!< Wait For Interruption instruction to enter Stop mode */
#define PWR_STOPENTRY_WFE                   ((uint8_t)0x02u)        /*!< Wait For Event instruction to enter Stop mode        */
/**
  * @}
  */


/** @defgroup PWREx_Flag  PWR Status Flags
  * @brief  Elements values convention: 0000 00XX 000Y YYYYb
  *           - Y YYYY  : Flag position in the XX register (5 bits)
  *           - XX  : Status register (2 bits)
  *                 - 01: SR1 register
  *                 - 10: SR2 register
  *         The only exception is PWR_FLAG_WU, encompassing all
  *         wake-up flags and set to PWR_SR1_WUF.
  * @{
  */
#define PWR_FLAG_WUF1                       (0x00010000u | PWR_SR1_WUF1)      /*!< Wakeup event on wakeup pin 1 */
#define PWR_FLAG_WUF2                       (0x00010000u | PWR_SR1_WUF2)      /*!< Wakeup event on wakeup pin 2 */
#define PWR_FLAG_WUF4                       (0x00010000u | PWR_SR1_WUF4)      /*!< Wakeup event on wakeup pin 4 */
#if defined(PWR_CR3_EWUP5)
#define PWR_FLAG_WUF5                       (0x00010000u | PWR_SR1_WUF5)      /*!< Wakeup event on wakeup pin 5 */
#endif
#define PWR_FLAG_WUF6                       (0x00010000u | PWR_SR1_WUF6)      /*!< Wakeup event on wakeup pin 6 */
#define PWR_FLAG_WUF                        (0x00010000u | PWR_SR1_WUF)       /*!< Wakeup event on all wakeup pin  */
#define PWR_FLAG_SB                         (0x00010000u | PWR_SR1_SBF)       /*!< Standby flag */
#define PWR_FLAG_WUFI                       (0x00010000u | PWR_SR1_WUFI)      /*!< Wakeup on internal wakeup line */
#define PWR_FLAG_FLASH_READY                (0x00020000u | PWR_SR2_FLASH_RDY) /*!< Flash ready */
#define PWR_FLAG_REGLPS                     (0x00020000u | PWR_SR2_REGLPS)    /*!< Regulator Low Power started */
#define PWR_FLAG_REGLPF                     (0x00020000u | PWR_SR2_REGLPF)    /*!< Regulator Low Power flag */
#if defined(PWR_PVD_SUPPORT)
#define PWR_FLAG_PVDO                       (0x00020000u | PWR_SR2_PVDO)      /*!< Power Voltage Detector output */
#endif
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup PWR_Exported_Macros  PWR Exported Macros
  * @{
  */
/** @brief  Check whether or not a specific PWR flag is set.
  * @param  __FLAG__  specifies the flag to check.
  *         This parameter can be one a combination of following values:
  *            @arg PWR_FLAG_WUF1: Wake Up Flag 1. Indicates that a wakeup event
  *                  was received from the WKUP pin 1.
  *            @arg PWR_FLAG_WUF2: Wake Up Flag 2. Indicates that a wakeup event
  *                  was received from the WKUP pin 2.
  *            @arg PWR_FLAG_WUF4: Wake Up Flag 4. Indicates that a wakeup event
  *                  was received from the WKUP pin 4.
  *            @arg PWR_FLAG_WUF5: Wake Up Flag 5. Indicates that a wakeup event
  *                  was received from the WKUP pin 5.
  *            @arg PWR_FLAG_WUF6: Wake Up Flag 6. Indicates that a wakeup event
  *                  was received from the WKUP pin 6.
  *            @arg PWR_FLAG_SB: StandBy Flag. Indicates that the system
  *                  entered StandBy mode.
  *            @arg PWR_FLAG_WUFI: Wake-Up Flag Internal. Set when a wakeup is
  *                 detected on the internal wakeup line.
  *         OR a combination of following values:
  *            @arg PWR_FLAG_FLASH_READY: Flash is ready. Indicates whether flash
  *                 can be used or not
  *            @arg PWR_FLAG_REGLPS: Low Power Regulator Started. Indicates whether
  *                 or not the low-power regulator is ready.
  *            @arg PWR_FLAG_REGLPF: Low Power Regulator Flag. Indicates whether the
  *                 regulator is ready in main mode or is in low-power mode.
  * @if defined(STM32G081xx)
  *            @arg PWR_FLAG_PVDO: Power Voltage Detector Output. Indicates whether
  *                 VDD voltage is below or above the selected PVD threshold.
  * @endif
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_PWR_GET_FLAG(__FLAG__)        (((__FLAG__) & 0x00010000u) ?\
                                            ((PWR->SR1 & ((__FLAG__) & ~0x00030000u)) == ((__FLAG__) & ~0x00030000u)) :\
                                            ((PWR->SR2 & ((__FLAG__) & ~0x00030000u)) == ((__FLAG__) & ~0x00030000u)))

/** @brief  Clear a specific PWR flag.
  * @param  __FLAG__  specifies the flag to clear.
  *         This parameter can be a combination of following values:
  *            @arg PWR_FLAG_WUF1: Wake Up Flag 1. Indicates that a wakeup event
  *                  was received from the WKUP pin 1.
  *            @arg PWR_FLAG_WUF2: Wake Up Flag 2. Indicates that a wakeup event
  *                  was received from the WKUP pin 2.
  *            @arg PWR_FLAG_WUF4: Wake Up Flag 4. Indicates that a wakeup event
  *                  was received from the WKUP pin 4.
  *            @arg PWR_FLAG_WUF5: Wake Up Flag 5. Indicates that a wakeup event
  *                  was received from the WKUP pin 5.
  *            @arg PWR_FLAG_WUF6: Wake Up Flag 6. Indicates that a wakeup event
  *                  was received from the WKUP pin 6.
  *            @arg PWR_FLAG_WUF: Encompasses all Wake Up Flags.
  *            @arg PWR_FLAG_SB: Standby Flag. Indicates that the system
  *                  entered Standby mode.
  * @retval None
  */
#define __HAL_PWR_CLEAR_FLAG(__FLAG__)      (PWR->SCR = (__FLAG__))

#if defined(PWR_PVD_SUPPORT)
/**
  * @brief Enable the PVD Extended Interrupt Line.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_IT()            SET_BIT(EXTI->IMR1, PWR_EXTI_LINE_PVD)

/**
  * @brief Disable the PVD Extended Interrupt Line.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_IT()           CLEAR_BIT(EXTI->IMR1, PWR_EXTI_LINE_PVD)

/**
  * @brief Enable the PVD Event Line.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_EVENT()         SET_BIT(EXTI->EMR1, PWR_EVENT_LINE_PVD)

/**
  * @brief Disable the PVD Event Line.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_EVENT()        CLEAR_BIT(EXTI->EMR1, PWR_EVENT_LINE_PVD)

/**
  * @brief Enable the PVD Extended Interrupt Rising Trigger.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_RISING_EDGE()   SET_BIT(EXTI->RTSR1, PWR_EXTI_LINE_PVD)

/**
  * @brief Disable the PVD Extended Interrupt Rising Trigger.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_RISING_EDGE()  CLEAR_BIT(EXTI->RTSR1, PWR_EXTI_LINE_PVD)

/**
  * @brief Enable the PVD Extended Interrupt Falling Trigger.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_FALLING_EDGE()  SET_BIT(EXTI->FTSR1, PWR_EXTI_LINE_PVD)

/**
  * @brief Disable the PVD Extended Interrupt Falling Trigger.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_FALLING_EDGE() CLEAR_BIT(EXTI->FTSR1, PWR_EXTI_LINE_PVD)

/**
  * @brief  Enable the PVD Extended Interrupt Rising & Falling Trigger.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_RISING_FALLING_EDGE()  \
  do {                                                   \
    __HAL_PWR_PVD_EXTI_ENABLE_RISING_EDGE();             \
    __HAL_PWR_PVD_EXTI_ENABLE_FALLING_EDGE();            \
  } while(0U)

/**
  * @brief Disable the PVD Extended Interrupt Rising & Falling Trigger.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_RISING_FALLING_EDGE()  \
  do {                                                    \
    __HAL_PWR_PVD_EXTI_DISABLE_RISING_EDGE();             \
    __HAL_PWR_PVD_EXTI_DISABLE_FALLING_EDGE();            \
  } while(0U)

/**
  * @brief  Generate a Software interrupt on selected EXTI line.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_GENERATE_SWIT()        SET_BIT(EXTI->SWIER1, PWR_EXTI_LINE_PVD)

/**
  * @brief Check whether or not the PVD EXTI interrupt Rising flag is set.
  * @retval EXTI PVD Line Status.
  */
#define __HAL_PWR_PVD_EXTI_GET_RISING_FLAG()      (EXTI->RPR1 & PWR_EXTI_LINE_PVD)

/**
  * @brief Check whether or not the PVD EXTI interrupt Falling flag is set.
  * @retval EXTI PVD Line Status.
  */
#define __HAL_PWR_PVD_EXTI_GET_FALLING_FLAG()     (EXTI->FPR1 & PWR_EXTI_LINE_PVD)

/**
  * @brief Clear the PVD EXTI interrupt Rising flag.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_CLEAR_RISING_FLAG()    WRITE_REG(EXTI->RPR1, PWR_EXTI_LINE_PVD)

/**
  * @brief Clear the PVD EXTI interrupt Falling flag.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_CLEAR_FALLING_FLAG()   WRITE_REG(EXTI->FPR1, PWR_EXTI_LINE_PVD)
#endif

/**
  * @}
  */

/* Private constants-------------------------------------------------------*/
/** @defgroup PWREx_WUP_Polarity Shift to apply to retrieve polarity information from PWR_WAKEUP_PINy_xxx constants
  * @{
  */
#define PWR_WUP_POLARITY_SHIFT              0x08u   /*!< Internal constant used to retrieve wakeup pin polariry */
/**
  * @}
  */

/* Private macros --------------------------------------------------------*/
/** @defgroup PWR_Private_Macros  PWR Private Macros
  * @{
  */
#if defined(PWR_PVD_SUPPORT)
#define IS_PWR_PVD_LEVEL(LEVEL)                   (((LEVEL) & ~(PWR_CR2_PVDRT | PWR_CR2_PVDFT)) == 0x00000000u)

#define IS_PWR_PVD_MODE(MODE)                     (((MODE) == PWR_PVD_MODE_NORMAL)              || \
                                                   ((MODE) == PWR_PVD_MODE_IT_RISING)           || \
                                                   ((MODE) == PWR_PVD_MODE_IT_FALLING)          || \
                                                   ((MODE) == PWR_PVD_MODE_IT_RISING_FALLING)   || \
                                                   ((MODE) == PWR_PVD_MODE_EVENT_RISING)        || \
                                                   ((MODE) == PWR_PVD_MODE_EVENT_FALLING)       || \
                                                   ((MODE) == PWR_PVD_MODE_EVENT_RISING_FALLING))
#endif
#define IS_PWR_WAKEUP_PIN(PIN)                    ((((PIN) & ((PWR_CR4_WP << 8U) | (PWR_CR3_EWUP))) != 0x00000000u) && \
                                                   (((PIN) & ~((PWR_CR4_WP << 8U) | (PWR_CR3_EWUP))) == 0x00000000u))

#define IS_PWR_REGULATOR(REGULATOR)               (((REGULATOR) == PWR_MAINREGULATOR_ON) || \
                                                   ((REGULATOR) == PWR_LOWPOWERREGULATOR_ON))

#define IS_PWR_SLEEP_ENTRY(ENTRY)                 (((ENTRY) == PWR_SLEEPENTRY_WFI) || \
                                                   ((ENTRY) == PWR_SLEEPENTRY_WFE))

#define IS_PWR_STOP_ENTRY(ENTRY)                  (((ENTRY) == PWR_STOPENTRY_WFI) || \
                                                   ((ENTRY) == PWR_STOPENTRY_WFE))
/**
  * @}
  */

/* Include PWR HAL Extended module */
#include "stm32g0xx_hal_pwr_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @defgroup PWR_Exported_Functions  PWR Exported Functions
  * @{
  */

/** @defgroup PWR_Exported_Functions_Group1  Initialization and de-initialization functions
  * @{
  */

/* Initialization and de-initialization functions *******************************/
void              HAL_PWR_DeInit(void);
/**
  * @}
  */

/** @defgroup PWR_Exported_Functions_Group2  Peripheral Control functions
  * @{
  */
/* Peripheral Control functions  ************************************************/
void              HAL_PWR_EnableBkUpAccess(void);
void              HAL_PWR_DisableBkUpAccess(void);
#if defined(PWR_PVD_SUPPORT)
HAL_StatusTypeDef HAL_PWR_ConfigPVD(PWR_PVDTypeDef *sConfigPVD);
void              HAL_PWR_EnablePVD(void);
void              HAL_PWR_DisablePVD(void);
#endif
/* WakeUp pins configuration functions ****************************************/
void              HAL_PWR_EnableWakeUpPin(uint32_t WakeUpPinPolarity);
void              HAL_PWR_DisableWakeUpPin(uint32_t WakeUpPinx);

/* Low Power modes configuration functions ************************************/
void              HAL_PWR_EnterSLEEPMode(uint32_t Regulator, uint8_t SLEEPEntry);
void              HAL_PWR_EnterSTOPMode(uint32_t Regulator, uint8_t STOPEntry);
void              HAL_PWR_EnterSTANDBYMode(void);
void              HAL_PWR_EnableSleepOnExit(void);
void              HAL_PWR_DisableSleepOnExit(void);
void              HAL_PWR_EnableSEVOnPend(void);
void              HAL_PWR_DisableSEVOnPend(void);
#if defined(PWR_PVD_SUPPORT)
void              HAL_PWR_PVD_IRQHandler(void);
void              HAL_PWR_PVD_Rising_Callback(void);
void              HAL_PWR_PVD_Falling_Callback(void);
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


#endif /* STM32G0xx_HAL_PWR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
