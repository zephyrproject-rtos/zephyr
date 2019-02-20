/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_pwr.h
  * @author  MCD Application Team
  * @brief   Header file of PWR HAL module.
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
#ifndef __STM32MP1xx_HAL_PWR_H
#define __STM32MP1xx_HAL_PWR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @addtogroup PWR
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup PWR_Exported_Types PWR Exported Types
  * @{
  */
/**
  * @brief  PWR PVD configuration structure definition
  */
typedef struct
{
  uint32_t PVDLevel;   /*!< PVDLevel: Specifies the PVD detection level.
                            This parameter can be a value of @ref PWR_PVD_detection_level */

  uint32_t Mode;      /*!< Mode: Specifies the operating mode for the selected pins.
                           This parameter can be a value of @ref PWR_PVD_Mode */
} PWR_PVDTypeDef;
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup PWR_Exported_Constants PWR Exported Constants
  * @{
  */

/** @defgroup PWR_PVD_detection_level PWR PVD detection level
  * @{
  */
#define PWR_PVDLEVEL_0                  PWR_CR1_PLS_LEV0 /* 1.95V */
#define PWR_PVDLEVEL_1                  PWR_CR1_PLS_LEV1 /* 2.1V  */
#define PWR_PVDLEVEL_2                  PWR_CR1_PLS_LEV2 /* 2.25V */
#define PWR_PVDLEVEL_3                  PWR_CR1_PLS_LEV3 /* 2.4V  */
#define PWR_PVDLEVEL_4                  PWR_CR1_PLS_LEV4 /* 2.55V */
#define PWR_PVDLEVEL_5                  PWR_CR1_PLS_LEV5 /* 2.7V  */
#define PWR_PVDLEVEL_6                  PWR_CR1_PLS_LEV6 /* 2.85V */
#define PWR_PVDLEVEL_7                  PWR_CR1_PLS_LEV7 /* External voltage level on PVD_IN
                                                            (compared to internal VREFINT) */

#define IS_PWR_PVD_LEVEL(__LEVEL__)     (((__LEVEL__) == PWR_PVDLEVEL_0) || ((__LEVEL__) == PWR_PVDLEVEL_1)|| \
                                         ((__LEVEL__) == PWR_PVDLEVEL_2) || ((__LEVEL__) == PWR_PVDLEVEL_3)|| \
                                         ((__LEVEL__) == PWR_PVDLEVEL_4) || ((__LEVEL__) == PWR_PVDLEVEL_5)|| \
                                         ((__LEVEL__) == PWR_PVDLEVEL_6) || ((__LEVEL__) == PWR_PVDLEVEL_7))
/**
  * @}
  */

/** @defgroup PWR_PVD_Mode PWR PVD Mode
  * @{
  */
#define PWR_PVD_MODE_NORMAL                 ((uint32_t)0x00000000U)   /*!< Basic mode is used */
#define PWR_PVD_MODE_IT_RISING              ((uint32_t)0x00010001U)   /*!< External Interrupt Mode with Rising edge trigger detection */
#define PWR_PVD_MODE_IT_FALLING             ((uint32_t)0x00010002U)   /*!< External Interrupt Mode with Falling edge trigger detection */
#define PWR_PVD_MODE_IT_RISING_FALLING      ((uint32_t)0x00010003U)   /*!< External Interrupt Mode with Rising/Falling edge trigger detection */

#define IS_PWR_PVD_MODE(__MODE__)          (((__MODE__) == PWR_PVD_MODE_NORMAL) || ((__MODE__) == PWR_PVD_MODE_IT_RISING)  || \
                                            ((__MODE__) == PWR_PVD_MODE_IT_FALLING) || ((__MODE__) == PWR_PVD_MODE_IT_RISING_FALLING))

/**
  * @}
  */

/** @defgroup PWR_Regulator_state_in_STOP_mode PWR Regulator state in STOP_mode
  * @{
  */
#define PWR_MAINREGULATOR_ON                        ((uint32_t)0x00000000)
#define PWR_LOWPOWERREGULATOR_ON                    PWR_CR1_LPDS

#define IS_PWR_REGULATOR(__REGULATOR__)     (((__REGULATOR__) == PWR_MAINREGULATOR_ON) || \
                                             ((__REGULATOR__) == PWR_LOWPOWERREGULATOR_ON))
/**
  * @}
  */

/** @defgroup PWR_SLEEP_mode_entry PWR SLEEP mode entry
  * @{
  */
#define PWR_SLEEPENTRY_WFI              ((uint8_t)0x01)
#define PWR_SLEEPENTRY_WFE              ((uint8_t)0x02)
#define IS_PWR_SLEEP_ENTRY(__ENTRY__)   (((__ENTRY__) == PWR_SLEEPENTRY_WFI) || ((__ENTRY__) == PWR_SLEEPENTRY_WFE))
/**
  * @}
  */

/** @defgroup PWR_STOP_mode_entry PWR STOP mode entry
  * @{
  */
#define PWR_STOPENTRY_WFI               ((uint8_t)0x01)
#define PWR_STOPENTRY_WFE               ((uint8_t)0x02)

#define IS_PWR_STOP_ENTRY(__ENTRY__)    (((__ENTRY__) == PWR_STOPENTRY_WFI) || ((__ENTRY__) == PWR_STOPENTRY_WFE))
/**
  * @}
  */



/** @defgroup PWR_Flag PWR Flag
  * @{
  */
#define PWR_FLAG_SB                     ((uint8_t)0x01U)   /* System STANDBY Flag  */
#define PWR_FLAG_STOP                   ((uint8_t)0x02U)   /* STOP Flag */
#define PWR_FLAG_CSB                    ((uint8_t)0x03U)   /* MPU CSTANBY flag */
#define PWR_FLAG_AVDO                   ((uint8_t)0x06U)
#define PWR_FLAG_PVDO                   ((uint8_t)0x07U)
#define PWR_FLAG_BRR                    ((uint8_t)0x08U)
#define PWR_FLAG_RRR                    ((uint8_t)0x09U)
#define PWR_FLAG_VBATL                  ((uint8_t)0x0AU)
#define PWR_FLAG_VBATH                  ((uint8_t)0x0BU)
#define PWR_FLAG_TEMPL                  ((uint8_t)0x0CU)
#define PWR_FLAG_TEMPH                  ((uint8_t)0x0DU)
#define PWR_FLAG_11R                    ((uint8_t)0x0EU)
#define PWR_FLAG_18R                    ((uint8_t)0x0FU)
#define PWR_FLAG_USB                    ((uint8_t)0x10U)
/**
  * @}
  */

/**
  * @}
  */


/* Exported macro ------------------------------------------------------------*/

/** @defgroup PWR_Exported_Macro PWR Exported Macro
  * @{
  */

/** @brief  Check PWR flag is set or not.
  * @param  __FLAG__: specifies the flag to check.
  *           This parameter can be one of the following values:
  *            @arg PWR_FLAG_PVDO: PVD Output. This flag is valid only if PVD is enabled
  *                  by the HAL_PWR_EnablePVD() function. The PVD is stopped by Standby mode
  *            @arg PWR_FLAG_AVDO: AVD Output. This flag is valid only if AVD is enabled
  *                  by the HAL_PWREx_EnableAVD() function. The AVD is stopped by Standby mode
  *            @arg PWR_FLAG_SB: StandBy flag. This flag indicates that the system was
  *                  resumed from StandBy mode.
  *            @arg PWR_FLAG_BRR: Backup regulator ready flag. This bit is not reset
  *                  when the device wakes up from Standby mode or by a system reset
  *                  or power reset.
  *            @arg PWR_FLAG_RRR: Retention Regulator ready flag. This bit is not reset
  *                  when the device wakes up from Standby mode or by a system reset
  *                  or power reset.
  *            @arg PWR_FLAG_VBATL:
  *            @arg PWR_FLAG_VBATH:
  *            @arg PWR_FLAG_TEMPL:
  *            @arg PWR_FLAG_TEMPH:
  *            @arg PWR_FLAG_11R: 1V1 regulator supply ready
  *            @arg PWR_FLAG_18R: 1V8 regulator supply ready
  *            @arg PWR_FLAG_USB: USB 3.3V supply ready
  *            @arg PWR_FLAG_SB: StandBy flag
  *            @arg PWR_FLAG_STOP: STOP flag
  *            @arg PWR_FLAG_CSB_MPU: MPU CSTANBY flag
  *
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#ifdef CORE_CM4
#define __HAL_PWR_GET_FLAG(__FLAG__) ( \
((__FLAG__) == PWR_FLAG_PVDO)?((PWR->CSR1 & PWR_CSR1_PVDO) == PWR_CSR1_PVDO) : \
((__FLAG__) == PWR_FLAG_AVDO)?((PWR->CSR1 & PWR_CSR1_AVDO) == PWR_CSR1_AVDO) : \
((__FLAG__) == PWR_FLAG_BRR)?((PWR->CR2 & PWR_CR2_BRRDY) == PWR_CR2_BRRDY) : \
((__FLAG__) == PWR_FLAG_RRR)?((PWR->CR2 & PWR_CR2_RRRDY) == PWR_CR2_RRRDY) : \
((__FLAG__) == PWR_FLAG_VBATL)?((PWR->CR2 & PWR_CR2_VBATL) == PWR_CR2_VBATL) : \
((__FLAG__) == PWR_FLAG_VBATH)?((PWR->CR2 & PWR_CR2_VBATH) == PWR_CR2_VBATH) : \
((__FLAG__) == PWR_FLAG_TEMPL)?((PWR->CR2 & PWR_CR2_TEMPL) == PWR_CR2_TEMPL) : \
((__FLAG__) == PWR_FLAG_TEMPH)?((PWR->CR2 & PWR_CR2_TEMPH) == PWR_CR2_TEMPH) : \
((__FLAG__) == PWR_FLAG_11R)?((PWR->CR3 & PWR_CR3_REG11RDY) == PWR_CR3_REG11RDY) : \
((__FLAG__) == PWR_FLAG_18R)?((PWR->CR3 & PWR_CR3_REG18RDY) == PWR_CR3_REG18RDY) : \
((__FLAG__) == PWR_FLAG_USB)?((PWR->CR3 & PWR_CR3_USB33RDY) == PWR_CR3_USB33RDY) : \
((__FLAG__) == PWR_FLAG_SB)?((PWR->MCUCR & PWR_MCUCR_SBF) ==  PWR_MCUCR_SBF) : \
((__FLAG__) == PWR_FLAG_STOP)?((PWR->MCUCR & PWR_MCUCR_STOPF) == PWR_MCUCR_STOPF) : \
((PWR->MPUCR & PWR_MPUCR_SBF_MPU) == PWR_MPUCR_SBF_MPU))
#elif defined (CORE_CA7)
#define __HAL_PWR_GET_FLAG(__FLAG__) ( \
((__FLAG__) == PWR_FLAG_PVDO)?((PWR->CSR1 & PWR_CSR1_PVDO) == PWR_CSR1_PVDO) : \
((__FLAG__) == PWR_FLAG_AVDO)?((PWR->CSR1 & PWR_CSR1_AVDO) == PWR_CSR1_AVDO) : \
((__FLAG__) == PWR_FLAG_BRR)?((PWR->CR2 & PWR_CR2_BRRDY) == PWR_CR2_BRRDY) : \
((__FLAG__) == PWR_FLAG_RRR)?((PWR->CR2 & PWR_CR2_RRRDY) == PWR_CR2_RRRDY) : \
((__FLAG__) == PWR_FLAG_VBATL)?((PWR->CR2 & PWR_CR2_VBATL) == PWR_CR2_VBATL) : \
((__FLAG__) == PWR_FLAG_VBATH)?((PWR->CR2 & PWR_CR2_VBATH) == PWR_CR2_VBATH) : \
((__FLAG__) == PWR_FLAG_TEMPL)?((PWR->CR2 & PWR_CR2_TEMPL) == PWR_CR2_TEMPL) : \
((__FLAG__) == PWR_FLAG_TEMPH)?((PWR->CR2 & PWR_CR2_TEMPH) == PWR_CR2_TEMPH) : \
((__FLAG__) == PWR_FLAG_11R)?((PWR->CR3 & PWR_CR3_REG11RDY) == PWR_CR3_REG11RDY) : \
((__FLAG__) == PWR_FLAG_18R)?((PWR->CR3 & PWR_CR3_REG18RDY) == PWR_CR3_REG18RDY) : \
((__FLAG__) == PWR_FLAG_USB)?((PWR->CR3 & PWR_CR3_USB33RDY) == PWR_CR3_USB33RDY) : \
((__FLAG__) == PWR_FLAG_SB)?((PWR->MPUCR & PWR_MPUCR_SBF) ==  PWR_MPUCR_SBF) : \
((__FLAG__) == PWR_FLAG_STOP)?((PWR->MPUCR & PWR_MPUCR_STOPF) == PWR_MPUCR_STOPF) : \
((PWR->MPUCR & PWR_MPUCR_SBF_MPU) == PWR_MPUCR_SBF_MPU))
#endif /*CORE_CA7*/

#ifdef CORE_CM4
/** @brief  Clear the PWR's flags.
  * @param  __FLAG__: specifies the flag to clear.
  *           This parameter can be one of the following values:
  *            @arg @ref PWR_FLAG_STOP
  *            @arg @ref PWR_FLAG_SB
  * @retval None.
  */
#define __HAL_PWR_CLEAR_FLAG(__FLAG__)  SET_BIT(PWR->MCUCR, PWR_MCUCR_CSSF);
#elif defined (CORE_CA7)
/** @brief  Clear the PWR's flags.
  * @param  __FLAG__: specifies the flag to clear.
  *           This parameter can be one of the following values:
  *            @arg @ref PWR_FLAG_STOP
  *            @arg @ref PWR_FLAG_SB
  *            @arg @ref PWR_FLAG_CSB flags
    * @retval None.
  */
#define __HAL_PWR_CLEAR_FLAG(__FLAG__)  SET_BIT(PWR->MPUCR, PWR_MPUCR_CSSF);


#endif /*CORE_CA7*/


/**
  * @brief Enable the PVD AVD Exti Line.
  * @retval None.
  */
#ifdef CORE_CM4
#define __HAL_PWR_PVD_AVD_EXTI_ENABLE_IT()  SET_BIT(EXTI_C2->IMR1, PWR_EXTI_LINE_PVD_AVD)
#elif defined (CORE_CA7)
#define __HAL_PWR_PVD_AVD_EXTI_ENABLE_IT()  SET_BIT(EXTI_C1->IMR1, PWR_EXTI_LINE_PVD_AVD)
#endif /*CORE_CA7*/


/**
  * @brief Disable the PVD AVD EXTI Line.
  * @retval None.
  */
#ifdef CORE_CM4
#define __HAL_PWR_PVD_AVD_EXTI_DISABLE_IT() CLEAR_BIT(EXTI_C2->IMR1, PWR_EXTI_LINE_PVD_AVD)
#elif defined (CORE_CA7)
#define __HAL_PWR_PVD_AVD_EXTI_DISABLE_IT() CLEAR_BIT(EXTI_C1->IMR1, PWR_EXTI_LINE_PVD_AVD)
#endif /*CORE_CA7*/


/**
  * @brief Enable the PVD AVD Extended Interrupt Rising Trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_AVD_EXTI_ENABLE_RISING_EDGE()  SET_BIT(EXTI->RTSR1, PWR_EXTI_LINE_PVD_AVD)

/**
  * @brief Disable the PVD AVD Extended Interrupt Rising Trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_AVD_EXTI_DISABLE_RISING_EDGE()  CLEAR_BIT(EXTI->RTSR1, PWR_EXTI_LINE_PVD_AVD)


/**
  * @brief Enable the PVD AVD Extended Interrupt Falling Trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_AVD_EXTI_ENABLE_FALLING_EDGE()  SET_BIT(EXTI->FTSR1, PWR_EXTI_LINE_PVD_AVD)


/**
  * @brief Disable the PVD AVD Extended Interrupt Falling Trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_AVD_EXTI_DISABLE_FALLING_EDGE()  CLEAR_BIT(EXTI->FTSR1, PWR_EXTI_LINE_PVD_AVD)


/**
  * @brief  PVD AVD EXTI line configuration: set rising & falling edge trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_AVD_EXTI_ENABLE_RISING_FALLING_EDGE() \
do { \
      __HAL_PWR_PVD_AVD_EXTI_ENABLE_RISING_EDGE(); \
      __HAL_PWR_PVD_AVD_EXTI_ENABLE_FALLING_EDGE(); \
} while(0);

/**
  * @brief Disable the PVD AVD Extended Interrupt Rising & Falling Trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_AVD_EXTI_DISABLE_RISING_FALLING_EDGE() \
do { \
      __HAL_PWR_PVD_AVD_EXTI_DISABLE_RISING_EDGE(); \
      __HAL_PWR_PVD_AVD_EXTI_DISABLE_FALLING_EDGE(); \
} while(0);


/**
  * @brief Check whether the specified PVD AVD EXTI interrupt flag is set or not.
  * @retval EXTI PVD AVD Line Status.
  */
#define __HAL_PWR_PVD_AVD_EXTI_GET_FLAG() \
        (((EXTI->RPR1 & PWR_EXTI_LINE_PVD_AVD) == PWR_EXTI_LINE_PVD_AVD) || \
        ((EXTI->FPR1 & PWR_EXTI_LINE_PVD_AVD) == PWR_EXTI_LINE_PVD_AVD))


/**
  * @brief Clear the PVD AVD Exti flag.
  * @retval None.
  */
#define __HAL_PWR_PVD_AVD_EXTI_CLEAR_FLAG() \
do { \
      SET_BIT(EXTI->RPR1, PWR_EXTI_LINE_PVD_AVD); \
      SET_BIT(EXTI->FPR1, PWR_EXTI_LINE_PVD_AVD); \
} while(0);


/**
  * @brief  Generates a Software interrupt on selected EXTI line.
  * @retval None
  */
/* PVD AVD Event in Bank1 */
#define __HAL_PWR_PVD_AVD_EXTI_GENERATE_SWIT() SET_BIT(EXTI->SWIER1, PWR_EXTI_LINE_PVD_AVD )


/* Include PWR HAL Extension module */
#include "stm32mp1xx_hal_pwr_ex.h"

/**
  * @brief  Enable WKUPx pin wakeup interrupt on AIEC for core 2.
  * @param  __WKUP_LINE__: specifies the WKUP pin line.
  *         This parameter can be one of the following values:
  *            @arg AIEC_WKUP1_WAKEUP: Wakeup pin 1 line
  *            @arg AIEC_WKUP1_WAKEUP: Wakeup pin 2 line
  *            @arg AIEC_WKUP1_WAKEUP: Wakeup pin 3 line
  *            @arg AIEC_WKUP1_WAKEUP: Wakeup pin 4 line
  *            @arg AIEC_WKUP1_WAKEUP: Wakeup pin 5 line
  *            @arg AIEC_WKUP1_WAKEUP: Wakeup pin 6 line
  * @retval None
  */
#ifdef CORE_CM4
#define __HAL_WKUP_EXTI_ENABLE_IT(__WKUP_LINE__)          (EXTI_C2->IMR2 |= (__WKUP_LINE__))
#elif defined(CORE_CA7)
#define __HAL_WKUP_EXTI_ENABLE_IT(__WKUP_LINE__)          (EXTI_C1->IMR2 |= (__WKUP_LINE__))
#endif /*CORE_CA7*/

/**
  * @brief  Disable WKUPx pin wakeup interrupt on AIEC for core 2.
  *   * @param  __WKUP_LINE__: specifies the WKUP pin line.
  *         This parameter can be one of the following values:
  *            @arg AIEC_WKUP1_WAKEUP: Wakeup pin 1 line
  *            @arg AIEC_WKUP2_WAKEUP: Wakeup pin 2 line
  *            @arg AIEC_WKUP3_WAKEUP: Wakeup pin 3 line
  *            @arg AIEC_WKUP4_WAKEUP: Wakeup pin 4 line
  *            @arg AIEC_WKUP5_WAKEUP: Wakeup pin 5 line
  *            @arg AIEC_WKUP6_WAKEUP: Wakeup pin 6 line
  * @retval None
  */
#ifdef CORE_CM4
#define __HAL_WKUP_EXTI_DISABLE_IT(__WKUP_LINE__)         CLEAR_BIT(EXTI_C2->IMR2, __WKUP_LINE__)
#elif defined(CORE_CA7)
#define __HAL_WKUP_EXTI_DISABLE_IT(__WKUP_LINE__)         CLEAR_BIT(EXTI_C1->IMR2, __WKUP_LINE__)
#endif /*CORE_CA7*/

/**
* @}
*/



/* Exported functions --------------------------------------------------------*/
/** @defgroup PWR_Exported_Functions PWR Exported Functions
  * @{
  */

/** @defgroup PWR_Exported_Functions_Group1 Initialization and de-initialization functions
 * @{
 */
/* Initialization and de-initialization functions *****************************/
void HAL_PWR_DeInit(void);
void HAL_PWR_EnableBkUpAccess(void);
void HAL_PWR_DisableBkUpAccess(void);
/**
* @}
*/

/* Peripheral Control functions  **********************************************/
/** @defgroup PWR_Exported_Functions_Group2 Peripheral Control functions
 * @{
 */
/* PVD configuration */
void HAL_PWR_ConfigPVD(PWR_PVDTypeDef *sConfigPVD);
void HAL_PWR_EnablePVD(void);
void HAL_PWR_DisablePVD(void);

/* WakeUp pins configuration */
void HAL_PWR_EnableWakeUpPin(uint32_t WakeUpPinPolarity);
void HAL_PWR_DisableWakeUpPin(uint32_t WakeUpPinx);

/* WakeUp pin IT functions */
void HAL_PWR_EnableWakeUpPinIT(uint32_t WakeUpPinx);
void HAL_PWR_DisableWakeUpPinIT(uint32_t WakeUpPinx);


/* Low Power modes entry */
void HAL_PWR_EnterSTOPMode(uint32_t Regulator, uint8_t STOPEntry);
void HAL_PWR_EnterSLEEPMode(uint32_t Regulator, uint8_t SLEEPEntry);
void HAL_PWR_EnterSTANDBYMode(void);


/* Power PVD IRQ Callback */
void HAL_PWR_PVDCallback(void);

/**
* @}
*/

/* Cortex System Control functions  *******************************************/
/** @defgroup PWR_Exported_Functions_Group3 Cortex System Control functions
 * @{
 */
void HAL_PWR_EnableSleepOnExit(void);
void HAL_PWR_DisableSleepOnExit(void);
void HAL_PWR_EnableSEVOnPend(void);
void HAL_PWR_DisableSEVOnPend(void);
/**
* @}
*/

/**
  * @}
  */


/* Private constants ---------------------------------------------------------*/

/** @defgroup PWR_Private_Constants PWR Private Constants
  * @{
  */

/** @defgroup PWR_EXTI_LINE_PVD_AVD PWR EXTI Line PVD AVD
  * @{
  */
#define PWR_EXTI_LINE_PVD_AVD   EXTI_IMR1_IM16 /*!< External interrupt line 16
                                                    Connected to the PVD AVD EXTI
                                                    Line */
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

#endif /* __STM32MP1xx_HAL_PWR_H */



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
