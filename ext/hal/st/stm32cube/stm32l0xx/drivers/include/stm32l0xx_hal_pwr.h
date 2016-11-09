/**
  ******************************************************************************
  * @file    stm32l0xx_hal_pwr.h
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
  * @brief   Header file of PWR HAL module.
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
#ifndef __STM32L0xx_HAL_PWR_H
#define __STM32L0xx_HAL_PWR_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal_def.h"

/** @addtogroup STM32L0xx_HAL_Driver
  * @{
  */

/** @defgroup PWR PWR
  * @{
  */

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

  uint32_t Mode;       /*!< Mode: Specifies the operating mode for the selected pins.
                            This parameter can be a value of @ref PWR_PVD_Mode */
}PWR_PVDTypeDef;

/**
  * @}
  */

/** @addtogroup PWR_Private
  * @{
  */ 

#define PWR_EXTI_LINE_PVD      EXTI_FTSR_TR16  /*!< External interrupt line 16 Connected to the PVD EXTI Line */

/**
  * @}
  */

/** @defgroup PWR_Exported_Constants PWR Exported Constants
  * @{
  */

/** @defgroup PWR_register_alias_address PWR Register alias address
  * @{
  */
#define PWR_WAKEUP_PIN1                PWR_CSR_EWUP1
#define PWR_WAKEUP_PIN2                PWR_CSR_EWUP2
#if defined (STM32L011xx) || defined (STM32L021xx) ||  defined (STM32L031xx) || defined (STM32L041xx) || defined (STM32L071xx) || \
    defined (STM32L072xx) || defined (STM32L073xx) || defined (STM32L081xx) || defined (STM32L082xx) || defined (STM32L083xx)
#define PWR_WAKEUP_PIN3                PWR_CSR_EWUP3
#endif
/**
  * @}
  */

/** @defgroup PWR_PVD_detection_level PVD detection level
  * @{
  */
#define PWR_PVDLEVEL_0                 PWR_CR_PLS_LEV0
#define PWR_PVDLEVEL_1                 PWR_CR_PLS_LEV1
#define PWR_PVDLEVEL_2                 PWR_CR_PLS_LEV2
#define PWR_PVDLEVEL_3                 PWR_CR_PLS_LEV3
#define PWR_PVDLEVEL_4                 PWR_CR_PLS_LEV4
#define PWR_PVDLEVEL_5                 PWR_CR_PLS_LEV5
#define PWR_PVDLEVEL_6                 PWR_CR_PLS_LEV6
#define PWR_PVDLEVEL_7                 PWR_CR_PLS_LEV7  /* External input analog voltage 
                                                           (Compare internally to VREFINT) */
/**
  * @}
  */

/** @defgroup PWR_PVD_Mode PWR PVD Mode
  * @{
  */
#define PWR_PVD_MODE_NORMAL                 ((uint32_t)0x00000000U)   /*!< basic mode is used */
#define PWR_PVD_MODE_IT_RISING              ((uint32_t)0x00010001U)   /*!< External Interrupt Mode with Rising edge trigger detection */
#define PWR_PVD_MODE_IT_FALLING             ((uint32_t)0x00010002U)   /*!< External Interrupt Mode with Falling edge trigger detection */
#define PWR_PVD_MODE_IT_RISING_FALLING      ((uint32_t)0x00010003U)   /*!< External Interrupt Mode with Rising/Falling edge trigger detection */
#define PWR_PVD_MODE_EVENT_RISING           ((uint32_t)0x00020001U)   /*!< Event Mode with Rising edge trigger detection */
#define PWR_PVD_MODE_EVENT_FALLING          ((uint32_t)0x00020002U)   /*!< Event Mode with Falling edge trigger detection */
#define PWR_PVD_MODE_EVENT_RISING_FALLING   ((uint32_t)0x00020003U)   /*!< Event Mode with Rising/Falling edge trigger detection */

/**
  * @}
  */

/** @defgroup PWR_Regulator_state_in_SLEEP_STOP_mode PWR Regulator state in SLEEP/STOP mode
  * @{
  */
#define PWR_MAINREGULATOR_ON           ((uint32_t)0x00000000U)
#define PWR_LOWPOWERREGULATOR_ON       PWR_CR_LPSDSR

/**
  * @}
  */

/** @defgroup PWR_SLEEP_mode_entry PWR SLEEP mode entry
  * @{
  */
#define PWR_SLEEPENTRY_WFI             ((uint8_t)0x01U)
#define PWR_SLEEPENTRY_WFE             ((uint8_t)0x02U)
/**
  * @}
  */

/** @defgroup PWR_STOP_mode_entry PWR STOP mode entry
  * @{
  */
#define PWR_STOPENTRY_WFI              ((uint8_t)0x01U)
#define PWR_STOPENTRY_WFE              ((uint8_t)0x02U)
/**
  * @}
  */

/** @defgroup PWR_Regulator_Voltage_Scale PWR Regulator Voltage Scale
  * @{
  */

#define PWR_REGULATOR_VOLTAGE_SCALE1   PWR_CR_VOS_0
#define PWR_REGULATOR_VOLTAGE_SCALE2   PWR_CR_VOS_1
#define PWR_REGULATOR_VOLTAGE_SCALE3   PWR_CR_VOS

#define IS_PWR_VOLTAGE_SCALING_RANGE(RANGE) (((RANGE) == PWR_REGULATOR_VOLTAGE_SCALE1) || \
                                             ((RANGE) == PWR_REGULATOR_VOLTAGE_SCALE2) || \
                                             ((RANGE) == PWR_REGULATOR_VOLTAGE_SCALE3))
/**
  * @}
  */

/** @defgroup PWR_Flag PWR Flag
  * @{
  */
#define PWR_FLAG_WU                    PWR_CSR_WUF
#define PWR_FLAG_SB                    PWR_CSR_SBF
#define PWR_FLAG_PVDO                  PWR_CSR_PVDO
#define PWR_FLAG_VREFINTRDY            PWR_CSR_VREFINTRDYF
#define PWR_FLAG_VOS                   PWR_CSR_VOSF
#define PWR_FLAG_REGLP                 PWR_CSR_REGLPF


/**
  * @}
  */

/**
  * @}
  */

/** @defgroup PWR_Exported_Macro PWR Exported Macros
  * @{
  */
/** @brief  macros configure the main internal regulator output voltage.
  *         When exiting Low Power Run Mode or during dynamic voltage scaling configuration,
  *         the reference manual recommends to poll PWR_FLAG_REGLP bit to wait for the regulator 
  *         to reach main mode (resp. to get stabilized) for a transition from 0 to 1. 
  *         Only then the clock can be increased.
  *
  * @param  __REGULATOR__: specifies the regulator output voltage to achieve
  *         a tradeoff between performance and power consumption when the device does
  *         not operate at the maximum frequency (refer to the datasheets for more details).
  *          This parameter can be one of the following values:
  *            @arg PWR_REGULATOR_VOLTAGE_SCALE1: Regulator voltage output Scale 1 mode,
  *                                                System frequency up to 32 MHz.
  *            @arg PWR_REGULATOR_VOLTAGE_SCALE2: Regulator voltage output Scale 2 mode,
  *                                                System frequency up to 16 MHz.
  *            @arg PWR_REGULATOR_VOLTAGE_SCALE3: Regulator voltage output Scale 3 mode,
  *                                                System frequency up to 4.2 MHz
  * @retval None
  */
#define __HAL_PWR_VOLTAGESCALING_CONFIG(__REGULATOR__) (MODIFY_REG(PWR->CR, PWR_CR_VOS, (__REGULATOR__)))

/** @brief  Check PWR flag is set or not.
  * @param  __FLAG__: specifies the flag to check.
  *           This parameter can be one of the following values:
  *            @arg PWR_FLAG_WU: Wake Up flag. This flag indicates that a wakeup event
  *                  was received from the WKUP pin or from the RTC alarm (Alarm B),
  *                  RTC Tamper event, RTC TimeStamp event or RTC Wakeup.
  *                  An additional wakeup event is detected if the WKUP pin is enabled
  *                  (by setting the EWUP bit) when the WKUP pin level is already high.
  *            @arg PWR_FLAG_SB: StandBy flag. This flag indicates that the system was
  *                  resumed from StandBy mode.
  *            @arg PWR_FLAG_PVDO: PVD Output. This flag is valid only if PVD is enabled 
  *                  by the HAL_PWR_EnablePVD() function. The PVD is stopped by Standby mode.
  *                  For this reason, this bit is equal to 0 after Standby or reset
  *                  until the PVDE bit is set.
  *            @arg PWR_FLAG_VREFINTRDY: Internal voltage reference (VREFINT) ready flag.
  *                 This bit indicates the state of the internal voltage reference, VREFINT.
  *            @arg PWR_FLAG_VOS: Voltage Scaling select flag. A delay is required for
  *                 the internal regulator to be ready after the voltage range is changed.
  *                 The VOSF bit indicates that the regulator has reached the voltage level
  *                 defined with bits VOS of PWR_CR register.
  *            @arg PWR_FLAG_REGLP: Regulator LP flag. When the MCU exits from Low power run
  *                 mode, this bit stays at 1 until the regulator is ready in main mode.
  *                 A polling on this bit is recommended to wait for the regulator main mode.
  *                 This bit is reset by hardware when the regulator is ready.
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_PWR_GET_FLAG(__FLAG__)                  ((PWR->CSR & (__FLAG__)) == (__FLAG__))

/** @brief  Clear the PWR pending flags.
  * @param  __FLAG__: specifies the flag to clear.
  *          This parameter can be one of the following values:
  *            @arg PWR_FLAG_WU: Wake Up flag
  *            @arg PWR_FLAG_SB: StandBy flag
  */
#define __HAL_PWR_CLEAR_FLAG(__FLAG__)                SET_BIT(PWR->CR, (__FLAG__) << 2U)

/**
  * @brief Enable interrupt on PVD Exti Line 16.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_IT()      SET_BIT(EXTI->IMR, PWR_EXTI_LINE_PVD)

/**
  * @brief Disable interrupt on PVD Exti Line 16. 
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_IT()     CLEAR_BIT(EXTI->IMR, PWR_EXTI_LINE_PVD)

/**
  * @brief Enable event on PVD Exti Line 16.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_EVENT()   SET_BIT(EXTI->EMR, PWR_EXTI_LINE_PVD)

/**
  * @brief Disable event on PVD Exti Line 16.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_EVENT()  CLEAR_BIT(EXTI->EMR, PWR_EXTI_LINE_PVD)


/**
  * @brief  PVD EXTI line configuration: set falling edge trigger.  
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_FALLING_EDGE()  SET_BIT(EXTI->FTSR, PWR_EXTI_LINE_PVD)


/**
  * @brief Disable the PVD Extended Interrupt Falling Trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_FALLING_EDGE()  CLEAR_BIT(EXTI->FTSR, PWR_EXTI_LINE_PVD)


/**
  * @brief  PVD EXTI line configuration: set rising edge trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_RISING_EDGE()   SET_BIT(EXTI->RTSR, PWR_EXTI_LINE_PVD)

/**
  * @brief Disable the PVD Extended Interrupt Rising Trigger.
  * This parameter can be:
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_RISING_EDGE()  CLEAR_BIT(EXTI->RTSR, PWR_EXTI_LINE_PVD)

/**
  * @brief  PVD EXTI line configuration: set rising & falling edge trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_RISING_FALLING_EDGE()   do { __HAL_PWR_PVD_EXTI_ENABLE_RISING_EDGE();__HAL_PWR_PVD_EXTI_ENABLE_FALLING_EDGE(); } while(0);

/**
  * @brief Disable the PVD Extended Interrupt Rising & Falling Trigger.
  * This parameter can be:
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_RISING_FALLING_EDGE()  do { __HAL_PWR_PVD_EXTI_DISABLE_RISING_EDGE();__HAL_PWR_PVD_EXTI_DISABLE_FALLING_EDGE(); } while(0);



/**
  * @brief Check whether the specified PVD EXTI interrupt flag is set or not.
  * @retval EXTI PVD Line Status.
  */
#define __HAL_PWR_PVD_EXTI_GET_FLAG()       (EXTI->PR & (PWR_EXTI_LINE_PVD))

/**
  * @brief Clear the PVD EXTI flag.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_CLEAR_FLAG()     (EXTI->PR = (PWR_EXTI_LINE_PVD))

/**
  * @brief Generate a Software interrupt on selected EXTI line.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_GENERATE_SWIT()  SET_BIT(EXTI->SWIER, PWR_EXTI_LINE_PVD)

/**
  * @brief Generate a Software interrupt on selected EXTI line.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_GENERATE_SWIT()            SET_BIT(EXTI->SWIER, PWR_EXTI_LINE_PVD)

/**
  * @}
  */

/** @addtogroup PWR_Private
  * @{
  */
#define IS_PWR_PVD_LEVEL(LEVEL) (((LEVEL) == PWR_PVDLEVEL_0) || ((LEVEL) == PWR_PVDLEVEL_1)|| \
                                 ((LEVEL) == PWR_PVDLEVEL_2) || ((LEVEL) == PWR_PVDLEVEL_3)|| \
                                 ((LEVEL) == PWR_PVDLEVEL_4) || ((LEVEL) == PWR_PVDLEVEL_5)|| \
                                 ((LEVEL) == PWR_PVDLEVEL_6) || ((LEVEL) == PWR_PVDLEVEL_7))

#define IS_PWR_PVD_MODE(MODE) (((MODE) == PWR_PVD_MODE_IT_RISING)|| ((MODE) == PWR_PVD_MODE_IT_FALLING) || \
                              ((MODE) == PWR_PVD_MODE_IT_RISING_FALLING) || ((MODE) == PWR_PVD_MODE_EVENT_RISING) || \
                              ((MODE) == PWR_PVD_MODE_EVENT_FALLING) || ((MODE) == PWR_PVD_MODE_EVENT_RISING_FALLING) || \
                              ((MODE) == PWR_PVD_MODE_NORMAL)) 

#if defined (STM32L071xx) || defined (STM32L072xx) || defined (STM32L073xx) || defined (STM32L081xx) || defined (STM32L082xx) || defined (STM32L083xx)
#define IS_PWR_WAKEUP_PIN(PIN) (((PIN) == PWR_WAKEUP_PIN1) || \
                                ((PIN) == PWR_WAKEUP_PIN2) || \
                                ((PIN) == PWR_WAKEUP_PIN3))
#elif defined (STM32L051xx) || defined (STM32L052xx) || defined (STM32L053xx) || defined (STM32L061xx) || defined (STM32L062xx) || defined (STM32L063xx)
#define IS_PWR_WAKEUP_PIN(PIN) (((PIN) == PWR_WAKEUP_PIN1) || \
                                ((PIN) == PWR_WAKEUP_PIN2))
#elif defined (STM32L031xx) || defined (STM32L041xx)
#define IS_PWR_WAKEUP_PIN(PIN) (((PIN) == PWR_WAKEUP_PIN1) || \
                                ((PIN) == PWR_WAKEUP_PIN2))
#elif defined (STM32L011xx) || defined (STM32L021xx)
#define IS_PWR_WAKEUP_PIN(PIN) (((PIN) == PWR_WAKEUP_PIN1) || \
                                ((PIN) == PWR_WAKEUP_PIN3))
#endif

#define IS_PWR_REGULATOR(REGULATOR) (((REGULATOR) == PWR_MAINREGULATOR_ON) || \
                                     ((REGULATOR) == PWR_LOWPOWERREGULATOR_ON))
#define IS_PWR_SLEEP_ENTRY(ENTRY) (((ENTRY) == PWR_SLEEPENTRY_WFI) || ((ENTRY) == PWR_SLEEPENTRY_WFE))

#define IS_PWR_STOP_ENTRY(ENTRY) (((ENTRY) == PWR_STOPENTRY_WFI) || ((ENTRY) == PWR_STOPENTRY_WFE))

/**
  * @}
  */

/* Include PWR HAL Extension module */
#include "stm32l0xx_hal_pwr_ex.h"

/** @defgroup PWR_Exported_Functions PWR Exported Functions
  * @{
  */
  
/** @defgroup PWR_Exported_Functions_Group1 Initialization and de-initialization functions 
  * @{
  */
void HAL_PWR_DeInit(void);
void HAL_PWR_EnableBkUpAccess(void);
void HAL_PWR_DisableBkUpAccess(void);
/**
  * @}
  */

/** @defgroup PWR_Exported_Functions_Group2 Low Power modes configuration functions
  * @{
  */

/* PVD control functions  ************************************************/
void HAL_PWR_ConfigPVD(PWR_PVDTypeDef *sConfigPVD);
void HAL_PWR_EnablePVD(void);
void HAL_PWR_DisablePVD(void);
void HAL_PWR_PVD_IRQHandler(void);
void HAL_PWR_PVDCallback(void);

/* WakeUp pins configuration functions ****************************************/
void HAL_PWR_EnableWakeUpPin(uint32_t WakeUpPinx);
void HAL_PWR_DisableWakeUpPin(uint32_t WakeUpPinx);

/* Low Power modes configuration functions ************************************/
void HAL_PWR_EnterSTOPMode(uint32_t Regulator, uint8_t STOPEntry);
void HAL_PWR_EnterSLEEPMode(uint32_t Regulator, uint8_t SLEEPEntry);
void HAL_PWR_EnterSTANDBYMode(void);

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

/* Define the private group ***********************************/
/**************************************************************/
/** @defgroup PWR_Private PWR Private
  * @{
  */
/**
  * @}
  */
/**************************************************************/

/**
  * @}
  */

/**
  * @}
  */
  
#ifdef __cplusplus
}
#endif


#endif /* __STM32L0xx_HAL_PWR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

