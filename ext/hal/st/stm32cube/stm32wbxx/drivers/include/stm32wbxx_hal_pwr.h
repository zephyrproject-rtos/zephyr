/**
  ******************************************************************************
  * @file    stm32wbxx_hal_pwr.h
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
#ifndef STM32WBxx_HAL_PWR_H
#define STM32WBxx_HAL_PWR_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_def.h"

/* Include low level driver */
#include "stm32wbxx_ll_pwr.h"
#include "stm32wbxx_ll_exti.h"

/** @addtogroup STM32WBxx_HAL_Driver
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

/**
  * @brief  PWR PVD configuration structure definition
  */
typedef struct
{
  uint32_t PVDLevel;       /*!< PVDLevel: Specifies the PVD detection level.
                                This parameter can be a value of @ref PWR_PVD_detection_level. */

  uint32_t Mode;           /*!< Mode: Specifies the operating mode for the selected pins.
                                This parameter can be a value of @ref PWR_PVD_Mode. */
}PWR_PVDTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup PWR_Exported_Constants PWR Exported Constants
  * @{
  */

/** @defgroup PWR_PVD_detection_level  Power Voltage Detector Level selection
  * @note     Refer datasheet for selection voltage value
  * @{
  */
#define PWR_PVDLEVEL_0                  (0x00000000U)                                    /*!< PVD threshold around 2.0 V */
#define PWR_PVDLEVEL_1                  (                                PWR_CR2_PLS_0)  /*!< PVD threshold around 2.2 V */
#define PWR_PVDLEVEL_2                  (                PWR_CR2_PLS_1                )  /*!< PVD threshold around 2.4 V */
#define PWR_PVDLEVEL_3                  (                PWR_CR2_PLS_1 | PWR_CR2_PLS_0)  /*!< PVD threshold around 2.5 V */
#define PWR_PVDLEVEL_4                  (PWR_CR2_PLS_2                                )  /*!< PVD threshold around 2.6 V */
#define PWR_PVDLEVEL_5                  (PWR_CR2_PLS_2                 | PWR_CR2_PLS_0)  /*!< PVD threshold around 2.8 V */
#define PWR_PVDLEVEL_6                  (PWR_CR2_PLS_2 | PWR_CR2_PLS_1                )  /*!< PVD threshold around 2.9 V */
#define PWR_PVDLEVEL_7                  (PWR_CR2_PLS_2 | PWR_CR2_PLS_1 | PWR_CR2_PLS_0)  /*!< External input analog voltage (compared internally to VREFINT) */
/**
  * @}
  */

/** @defgroup PWR_PVD_Mode  PWR PVD interrupt and event mode
  * @{
  */
/* Note: On STM32WB serie, power PVD event is not available on AIEC lines     */
/*       (only interruption is available through AIEC line 16).               */
#define PWR_PVD_MODE_NORMAL                 (0x00000000U)                           /*!< Basic mode is used */

#define PWR_PVD_MODE_IT_RISING              (PVD_MODE_IT | PVD_RISING_EDGE)         /*!< External Interrupt Mode with Rising edge trigger detection */
#define PWR_PVD_MODE_IT_FALLING             (PVD_MODE_IT | PVD_FALLING_EDGE)        /*!< External Interrupt Mode with Falling edge trigger detection */
#define PWR_PVD_MODE_IT_RISING_FALLING      (PVD_MODE_IT | PVD_RISING_FALLING_EDGE) /*!< External Interrupt Mode with Rising/Falling edge trigger detection */
/**
  * @}
  */

/* Note: On STM32WB serie, power PVD event is not available on AIEC lines     */
/*       (only interruption is available through AIEC line 16).               */

/** @defgroup PWR_Low_Power_Mode_Selection  PWR Low Power Mode Selection
  * @{
  */
#define PWR_LOWPOWERMODE_STOP0              (0x00000000u)                         /*!< Stop 0: stop mode with main regulator */
#define PWR_LOWPOWERMODE_STOP1              (PWR_CR1_LPMS_0)                      /*!< Stop 1: stop mode with low power regulator */
#define PWR_LOWPOWERMODE_STOP2              (PWR_CR1_LPMS_1)                      /*!< Stop 2: stop mode with low power regulator and VDD12I interruptible digital core domain supply OFF (less peripherals activated than low power mode stop 1 to reduce power consumption)*/
#define PWR_LOWPOWERMODE_STANDBY            (PWR_CR1_LPMS_0 | PWR_CR1_LPMS_1)     /*!< Standby mode */
#define PWR_LOWPOWERMODE_SHUTDOWN           (PWR_CR1_LPMS_2)                      /*!< Shutdown mode */
/**
  * @}
  */

/** @defgroup PWR_Regulator_state_in_SLEEP_STOP_mode  PWR regulator mode
  * @{
  */
#define PWR_MAINREGULATOR_ON                (0x00000000U)               /*!< Regulator in main mode      */
#define PWR_LOWPOWERREGULATOR_ON            (PWR_CR1_LPR)               /*!< Regulator in low-power mode */
/**
  * @}
  */

/** @defgroup PWR_SLEEP_mode_entry  PWR SLEEP mode entry
  * @{
  */
#define PWR_SLEEPENTRY_WFI                  ((uint8_t)0x01)         /*!< Wait For Interruption instruction to enter Sleep mode */
#define PWR_SLEEPENTRY_WFE                  ((uint8_t)0x02)         /*!< Wait For Event instruction to enter Sleep mode        */
/**
  * @}
  */

/** @defgroup PWR_STOP_mode_entry  PWR STOP mode entry
  * @{
  */
#define PWR_STOPENTRY_WFI                   ((uint8_t)0x01)         /*!< Wait For Interruption instruction to enter Stop mode */
#define PWR_STOPENTRY_WFE                   ((uint8_t)0x02)         /*!< Wait For Event instruction to enter Stop mode        */
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

/** @defgroup PWR_PVD_EXTI_LINE  PWR PVD external interrupt line
  * @{
  */
#define PWR_EXTI_LINE_PVD  (LL_EXTI_LINE_16)   /*!< External interrupt line 16 Connected to the PWR PVD */
/**
  * @}
  */

/** @defgroup PWR_PVD_Mode_Mask PWR PVD Mode Mask
  * @{
  */
/* Note: On STM32WB serie, power PVD event is not available on AIEC lines     */
/*       (only interruption is available through AIEC line 16).               */
#define PVD_MODE_IT               (0x00010000U)  /*!< Mask for interruption yielded by PVD threshold crossing */
#define PVD_RISING_EDGE           (0x00000001U)  /*!< Mask for rising edge set as PVD trigger                 */
#define PVD_FALLING_EDGE          (0x00000002U)  /*!< Mask for falling edge set as PVD trigger                */
#define PVD_RISING_FALLING_EDGE   (0x00000003U)  /*!< Mask for rising and falling edges set as PVD trigger    */
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
  * @param __FLAG__ specifies the flag to check.
  *           This parameter can be one of the following values:
  *
  *            /--------------------------------SR1-------------------------------/
  *            @arg @ref PWR_FLAG_WUF1  Wake Up Flag 1. Indicates that a wakeup event
  *                                     was received from the WKUP pin 1.
  *            @arg @ref PWR_FLAG_WUF2  Wake Up Flag 2. Indicates that a wakeup event
  *                                     was received from the WKUP pin 2.
  *            @arg @ref PWR_FLAG_WUF3  Wake Up Flag 3. Indicates that a wakeup event
  *                                     was received from the WKUP pin 3.
  *            @arg @ref PWR_FLAG_WUF4  Wake Up Flag 4. Indicates that a wakeup event
  *                                     was received from the WKUP pin 4.
  *            @arg @ref PWR_FLAG_WUF5  Wake Up Flag 5. Indicates that a wakeup event
  *                                     was received from the WKUP pin 5.
  *
  *            @arg @ref PWR_FLAG_BHWF      BLE_Host WakeUp Flag
  *            @arg @ref PWR_FLAG_FRCBYPI   SMPS Forced in Bypass Interrupt Flag
  *            @arg @ref PWR_FLAG_RFPHASEI  Radio Phase Interrupt Flag
  *            @arg @ref PWR_FLAG_BLEACTI   BLE Activity Interrupt Flag
  *            @arg @ref PWR_FLAG_802ACTI   802.15.4 Activity Interrupt Flag
  *            @arg @ref PWR_FLAG_HOLDC2I   CPU2 on-Hold Interrupt Flag
  *            @arg @ref PWR_FLAG_WUFI      Wake-Up Flag Internal. Set when a wakeup is detected on
  *                                         the internal wakeup line.
  *
  *            @arg @ref PWR_FLAG_SMPSRDYF  SMPS Ready Flag
  *            @arg @ref PWR_FLAG_SMPSBYPF  SMPS Bypass Flag
  *
  *            /--------------------------------SR2-------------------------------/
  *            @arg @ref PWR_FLAG_REGLPS Low Power Regulator Started. Indicates whether or not the
  *                                      low-power regulator is ready.
  *            @arg @ref PWR_FLAG_REGLPF Low Power Regulator Flag. Indicates whether the
  *                                      regulator is ready in main mode or is in low-power mode.
  *
  *            @arg @ref PWR_FLAG_VOSF   Voltage Scaling Flag. Indicates whether the regulator is ready
  *                                      in the selected voltage range or is still changing to the required voltage level.
  *            @arg @ref PWR_FLAG_PVDO   Power Voltage Detector Output. Indicates whether VDD voltage is
  *                                      below or above the selected PVD threshold.
  *
  *            @arg @ref PWR_FLAG_PVMO1 Peripheral Voltage Monitoring Output 1. Indicates whether VDDUSB voltage is
  *                                     is below or above PVM1 threshold (applicable when USB feature is supported).
  *            @arg @ref PWR_FLAG_PVMO3 Peripheral Voltage Monitoring Output 3. Indicates whether VDDA voltage is
  *                                     is below or above PVM3 threshold.
  *
  *           /----------------------------EXTSCR--------------------------/
  *            @arg @ref PWR_FLAG_STOP              System Stop Flag for CPU1.
  *            @arg @ref PWR_FLAG_SB                System Standby Flag for CPU1.
  *
  *            @arg @ref PWR_FLAG_C2STOP            System Stop Flag for CPU2.
  *            @arg @ref PWR_FLAG_C2SB              System Standby Flag for CPU2.
  *
  *            @arg @ref PWR_FLAG_CRITICAL_RF_PHASE Critical radio system phase flag.
  *
  *            @arg @ref PWR_FLAG_C1DEEPSLEEP       CPU1 DeepSleep Flag.
  *            @arg @ref PWR_FLAG_C2DEEPSLEEP       CPU2 DeepSleep Flag.
  *
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_PWR_GET_FLAG(__FLAG__)  ( ((((uint8_t)(__FLAG__)) >> 5U) == 1U)  ?\
                                      (PWR->SR1 & (1U << ((__FLAG__) & 31U))) :\
                                      ((((((uint8_t)(__FLAG__)) >> 5U) == 2U)) ?\
                                      (PWR->SR2 & (1U << ((__FLAG__) & 31U))) :\
                                      (PWR->EXTSCR & (1U << ((__FLAG__) & 31U))) ) )


/** @brief  Clear a specific PWR flag.
  * @note   Clearing of flags {PWR_FLAG_STOP, PWR_FLAG_SB}
  *         and flags {PWR_FLAG_C2STOP, PWR_FLAG_C2SB} are grouped:
  *         clearing of one flag also clears the other one.
  * @param __FLAG__ specifies the flag to clear.
  *          This parameter can be one of the following values:
  *
  *            /--------------------------------SCR (SRR)------------------------------/
  *            @arg @ref PWR_FLAG_WUF1  Wake Up Flag 1. Indicates that a wakeup event
  *                                     was received from the WKUP pin 1.
  *            @arg @ref PWR_FLAG_WUF2  Wake Up Flag 2. Indicates that a wakeup event
  *                                     was received from the WKUP pin 2.
  *            @arg @ref PWR_FLAG_WUF3  Wake Up Flag 3. Indicates that a wakeup event
  *                                     was received from the WKUP pin 3.
  *            @arg @ref PWR_FLAG_WUF4  Wake Up Flag 4. Indicates that a wakeup event
  *                                     was received from the WKUP pin 4.
  *            @arg @ref PWR_FLAG_WUF5  Wake Up Flag 5. Indicates that a wakeup event
  *                                     was received from the WKUP pin 5.
  *            @arg @ref PWR_FLAG_WU    Encompasses all five Wake Up Flags.
  *
  *            @arg @ref PWR_FLAG_BHWF      Clear BLE_Host Wakeup Flag.
  *            @arg @ref PWR_FLAG_FRCBYPI   Clear SMPS Forced in Bypass Interrupt Flag.
  *            @arg @ref PWR_FLAG_RFPHASEI  RF Phase Interrupt Clear.
  *            @arg @ref PWR_FLAG_BLEACTI   BLE Activity Interrupt Clear.
  *            @arg @ref PWR_FLAG_802ACTI   802.15.4. Activity Interrupt Clear.
  *            @arg @ref PWR_FLAG_HOLDC2I   CPU2 on-Hold Interrupt Clear.
  *
  *           /----------------------------EXTSCR--------------------------/
  *            @arg @ref PWR_FLAG_STOP      System Stop Flag for CPU1.
  *            @arg @ref PWR_FLAG_SB        System Standby Flag for CPU1.
  *
  *            @arg @ref PWR_FLAG_C2STOP    System Stop Flag for CPU2.
  *            @arg @ref PWR_FLAG_C2SB      System Standby Flag for CPU2.
  *
  *            @arg @ref PWR_FLAG_CRITICAL_RF_PHASE  RF phase Flag.
  *
  * @retval None
  */
#define __HAL_PWR_CLEAR_FLAG(__FLAG__)   ( ((((uint8_t)(__FLAG__)) >> 5U) == 1U) ?\
                                         ( (((uint8_t)(__FLAG__)) == PWR_FLAG_WU) ?\
                                         (PWR->SCR  = (__FLAG__)) : (PWR->SCR = (1U << ((__FLAG__) & 31U))) ) :\
                                         ( (((uint8_t)(__FLAG__)) == PWR_FLAG_CRITICAL_RF_PHASE) ?\
                                         SET_BIT (PWR->EXTSCR, PWR_EXTSCR_CCRPF) : ( ((((uint8_t)((__FLAG__)) & 31U) <= PWR_EXTSCR_C1STOPF_Pos) ?\
                                         SET_BIT (PWR->EXTSCR, PWR_EXTSCR_C1CSSF): SET_BIT (PWR->EXTSCR, PWR_EXTSCR_C2CSSF)) ) ))

/**
  * @brief Enable the PVD Extended Interrupt C1 Line.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_IT()   LL_EXTI_EnableIT_0_31(PWR_EXTI_LINE_PVD)

/**
  * @brief Enable the PVD Extended Interrupt C2 Line.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTIC2_ENABLE_IT()   LL_C2_EXTI_EnableIT_0_31(PWR_EXTI_LINE_PVD)


/**
  * @brief Disable the PVD Extended Interrupt C1 Line.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_IT()  LL_EXTI_DisableIT_0_31(PWR_EXTI_LINE_PVD)

/**
  * @brief Disable the PVD Extended Interrupt C2 Line.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTIC2_DISABLE_IT()  LL_C2_EXTI_DisableIT_0_31(PWR_EXTI_LINE_PVD)

/* Note: On STM32WB serie, power PVD event is not available on AIEC lines     */
/*       (only interruption is available through AIEC line 16).               */

/**
  * @brief Enable the PVD Extended Interrupt Rising Trigger.
  * @note  PVD flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVD voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_RISING_EDGE()   LL_EXTI_EnableRisingTrig_0_31(PWR_EXTI_LINE_PVD)

/**
  * @brief Disable the PVD Extended Interrupt Rising Trigger.
  * @note  PVD flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVD voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_RISING_EDGE()  LL_EXTI_DisableFallingTrig_0_31(PWR_EXTI_LINE_PVD)

/**
  * @brief Enable the PVD Extended Interrupt Falling Trigger.
  * @note  PVD flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVD voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_FALLING_EDGE()   LL_EXTI_EnableFallingTrig_0_31(PWR_EXTI_LINE_PVD)


/**
  * @brief Disable the PVD Extended Interrupt Falling Trigger.
  * @note  PVD flag polarity is inverted compared to EXTI line, therefore
  *        EXTI rising and falling logic edges are inverted versus PVD voltage edges.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_FALLING_EDGE()  LL_EXTI_DisableRisingTrig_0_31(PWR_EXTI_LINE_PVD)


/**
  * @brief  Enable the PVD Extended Interrupt Rising & Falling Trigger.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_RISING_FALLING_EDGE()  \
  do {                                                   \
    __HAL_PWR_PVD_EXTI_ENABLE_RISING_EDGE();             \
    __HAL_PWR_PVD_EXTI_ENABLE_FALLING_EDGE();            \
  } while(0)

/**
  * @brief Disable the PVD Extended Interrupt Rising & Falling Trigger.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_RISING_FALLING_EDGE()  \
  do {                                                    \
    __HAL_PWR_PVD_EXTI_DISABLE_RISING_EDGE();             \
    __HAL_PWR_PVD_EXTI_DISABLE_FALLING_EDGE();            \
  } while(0)

/**
  * @brief  Generate a Software interrupt on selected EXTI line.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_GENERATE_SWIT() LL_EXTI_GenerateSWI_0_31(PWR_EXTI_LINE_PVD)

/**
  * @brief Check whether or not the PVD EXTI interrupt flag is set.
  * @retval EXTI PVD Line Status.
  */
#define __HAL_PWR_PVD_EXTI_GET_FLAG()  LL_EXTI_ReadFlag_0_31(PWR_EXTI_LINE_PVD)

/**
  * @brief Clear the PVD EXTI interrupt flag.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_CLEAR_FLAG()  LL_EXTI_ClearFlag_0_31(PWR_EXTI_LINE_PVD)

/**
  * @}
  */


/* Private macros --------------------------------------------------------*/
/** @defgroup PWR_Private_Macros  PWR Private Macros
  * @{
  */

#define IS_PWR_PVD_LEVEL(LEVEL) (((LEVEL) == PWR_PVDLEVEL_0) || ((LEVEL) == PWR_PVDLEVEL_1)|| \
                                 ((LEVEL) == PWR_PVDLEVEL_2) || ((LEVEL) == PWR_PVDLEVEL_3)|| \
                                 ((LEVEL) == PWR_PVDLEVEL_4) || ((LEVEL) == PWR_PVDLEVEL_5)|| \
                                 ((LEVEL) == PWR_PVDLEVEL_6) || ((LEVEL) == PWR_PVDLEVEL_7))

#define IS_PWR_PVD_MODE(MODE)  (((MODE) == PWR_PVD_MODE_NORMAL)              ||\
                                ((MODE) == PWR_PVD_MODE_IT_RISING)           ||\
                                ((MODE) == PWR_PVD_MODE_IT_FALLING)          ||\
                                ((MODE) == PWR_PVD_MODE_IT_RISING_FALLING))



#define IS_PWR_REGULATOR(REGULATOR)               (((REGULATOR) == PWR_MAINREGULATOR_ON)    || \
                                                   ((REGULATOR) == PWR_LOWPOWERREGULATOR_ON))


#define IS_PWR_SLEEP_ENTRY(ENTRY)                 (((ENTRY) == PWR_SLEEPENTRY_WFI) || \
                                                   ((ENTRY) == PWR_SLEEPENTRY_WFE))

#define IS_PWR_STOP_ENTRY(ENTRY)                  (((ENTRY) == PWR_STOPENTRY_WFI) || \
                                                   ((ENTRY) == PWR_STOPENTRY_WFE))
/**
  * @}
  */

/* Include PWR HAL Extended module */
#include "stm32wbxx_hal_pwr_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @defgroup PWR_Exported_Functions  PWR Exported Functions
  * @{
  */

/** @defgroup PWR_Exported_Functions_Group1  Initialization and de-initialization functions
  * @{
  */

/* Initialization and de-initialization functions *******************************/
void              HAL_PWR_DeInit(void);

void              HAL_PWR_EnableBkUpAccess(void);
void              HAL_PWR_DisableBkUpAccess(void);
/**
  * @}
  */

/** @defgroup PWR_Exported_Functions_Group2  Peripheral Control functions
  * @{
  */
/* Peripheral Control functions  ************************************************/
HAL_StatusTypeDef HAL_PWR_ConfigPVD(PWR_PVDTypeDef *sConfigPVD);
void              HAL_PWR_EnablePVD(void);
void              HAL_PWR_DisablePVD(void);

/* WakeUp pins configuration functions ****************************************/
void              HAL_PWR_EnableWakeUpPin(uint32_t WakeUpPinPolarity);
void              HAL_PWR_DisableWakeUpPin(uint32_t WakeUpPinx);

/* Low Power modes configuration functions ************************************/
void              HAL_PWR_EnterSTOPMode(uint32_t Regulator, uint8_t STOPEntry);
void              HAL_PWR_EnterSLEEPMode(uint32_t Regulator, uint8_t SLEEPEntry);
void              HAL_PWR_EnterSTANDBYMode(void);

void              HAL_PWR_PVDCallback(void);
void              HAL_PWR_EnableSleepOnExit(void);
void              HAL_PWR_DisableSleepOnExit(void);

void              HAL_PWR_EnableSEVOnPend(void);
void              HAL_PWR_DisableSEVOnPend(void);


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

/**
  * @}
  */

#ifdef __cplusplus
}
#endif


#endif /* STM32WBxx_HAL_PWR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
