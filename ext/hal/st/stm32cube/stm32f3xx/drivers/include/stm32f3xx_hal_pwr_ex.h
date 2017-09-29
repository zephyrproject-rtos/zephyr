/**
  ******************************************************************************
  * @file    stm32f3xx_hal_pwr_ex.h
  * @author  MCD Application Team
  * @brief   Header file of PWR HAL Extended module.
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
#ifndef __STM32F3xx_HAL_PWR_EX_H
#define __STM32F3xx_HAL_PWR_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal_def.h"

/** @addtogroup STM32F3xx_HAL_Driver
  * @{
  */

/** @addtogroup PWREx
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup PWREx_Exported_Types PWR Extended Exported Types
 *  @{
 */
#if defined(STM32F302xE) || defined(STM32F303xE) || \
    defined(STM32F302xC) || defined(STM32F303xC) || \
    defined(STM32F303x8) || defined(STM32F334x8) || \
    defined(STM32F301x8) || defined(STM32F302x8) || \
    defined(STM32F373xC)
/**
  * @brief  PWR PVD configuration structure definition
  */
typedef struct
{
  uint32_t PVDLevel;   /*!< PVDLevel: Specifies the PVD detection level
                            This parameter can be a value of @ref PWREx_PVD_detection_level */

  uint32_t Mode;       /*!< Mode: Specifies the operating mode for the selected pins.
                            This parameter can be a value of @ref PWREx_PVD_Mode */
}PWR_PVDTypeDef;
#endif /* STM32F302xE || STM32F303xE || */
       /* STM32F302xC || STM32F303xC || */
       /* STM32F303x8 || STM32F334x8 || */
       /* STM32F301x8 || STM32F302x8 || */
       /* STM32F373xC                   */

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup PWREx_Exported_Constants PWR Extended Exported Constants
  * @{
  */

#if defined(STM32F302xE) || defined(STM32F303xE) || \
    defined(STM32F302xC) || defined(STM32F303xC) || \
    defined(STM32F303x8) || defined(STM32F334x8) || \
    defined(STM32F301x8) || defined(STM32F302x8) || \
    defined(STM32F373xC)

/** @defgroup PWREx_PVD_detection_level PWR Extended PVD detection level
  * @{
  */
#define PWR_PVDLEVEL_0                  PWR_CR_PLS_LEV0    /*!< PVD threshold around 2.2 V */
#define PWR_PVDLEVEL_1                  PWR_CR_PLS_LEV1    /*!< PVD threshold around 2.3 V */
#define PWR_PVDLEVEL_2                  PWR_CR_PLS_LEV2    /*!< PVD threshold around 2.4 V */
#define PWR_PVDLEVEL_3                  PWR_CR_PLS_LEV3    /*!< PVD threshold around 2.5 V */
#define PWR_PVDLEVEL_4                  PWR_CR_PLS_LEV4    /*!< PVD threshold around 2.6 V */
#define PWR_PVDLEVEL_5                  PWR_CR_PLS_LEV5    /*!< PVD threshold around 2.7 V */
#define PWR_PVDLEVEL_6                  PWR_CR_PLS_LEV6    /*!< PVD threshold around 2.8 V */
#define PWR_PVDLEVEL_7                  PWR_CR_PLS_LEV7    /*!< PVD threshold around 2.9 V */
/**
  * @}
  */

/** @defgroup PWREx_PVD_Mode PWR Extended PVD Mode
  * @{
  */
#define PWR_PVD_MODE_NORMAL                 (0x00000000U)   /*!< Basic mode is used */
#define PWR_PVD_MODE_IT_RISING              (0x00010001U)   /*!< External Interrupt Mode with Rising edge trigger detection */
#define PWR_PVD_MODE_IT_FALLING             (0x00010002U)   /*!< External Interrupt Mode with Falling edge trigger detection */
#define PWR_PVD_MODE_IT_RISING_FALLING      (0x00010003U)   /*!< External Interrupt Mode with Rising/Falling edge trigger detection */
#define PWR_PVD_MODE_EVENT_RISING           (0x00020001U)   /*!< Event Mode with Rising edge trigger detection */
#define PWR_PVD_MODE_EVENT_FALLING          (0x00020002U)   /*!< Event Mode with Falling edge trigger detection */
#define PWR_PVD_MODE_EVENT_RISING_FALLING   (0x00020003U)   /*!< Event Mode with Rising/Falling edge trigger detection */
/**
  * @}
  */

#define PWR_EXTI_LINE_PVD  EXTI_IMR_MR16  /*!< External interrupt line 16 Connected to the PVD EXTI Line */

#endif /* STM32F302xE || STM32F303xE || */
       /* STM32F302xC || STM32F303xC || */
       /* STM32F303x8 || STM32F334x8 || */
       /* STM32F301x8 || STM32F302x8 || */
       /* STM32F373xC                   */

#if defined(STM32F373xC) || defined(STM32F378xx)
/** @defgroup PWREx_SDADC_ANALOGx PWR Extended SDADC ANALOGx
  * @{
  */
#define PWR_SDADC_ANALOG1              ((uint32_t)PWR_CR_ENSD1)   /*!< Enable SDADC1 */
#define PWR_SDADC_ANALOG2              ((uint32_t)PWR_CR_ENSD2)   /*!< Enable SDADC2 */
#define PWR_SDADC_ANALOG3              ((uint32_t)PWR_CR_ENSD3)   /*!< Enable SDADC3 */
/**
  * @}
  */
#endif /* STM32F373xC || STM32F378xx */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup PWREx_Exported_Macros PWR Extended Exported Macros
  * @{
  */

#if defined(STM32F302xE) || defined(STM32F303xE) || \
    defined(STM32F302xC) || defined(STM32F303xC) || \
    defined(STM32F303x8) || defined(STM32F334x8) || \
    defined(STM32F301x8) || defined(STM32F302x8) || \
    defined(STM32F373xC)

/**
  * @brief Enable interrupt on PVD Exti Line 16.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_IT()      (EXTI->IMR |= (PWR_EXTI_LINE_PVD))

/**
  * @brief Disable interrupt on PVD Exti Line 16.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_IT()     (EXTI->IMR &= ~(PWR_EXTI_LINE_PVD))

/**
  * @brief Generate a Software interrupt on selected EXTI line.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_GENERATE_SWIT()  (EXTI->SWIER |= (PWR_EXTI_LINE_PVD))

/**
  * @brief Enable event on PVD Exti Line 16.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_EVENT()   (EXTI->EMR |= (PWR_EXTI_LINE_PVD))

/**
  * @brief Disable event on PVD Exti Line 16.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_EVENT()  (EXTI->EMR &= ~(PWR_EXTI_LINE_PVD))

/**
  * @brief Disable the PVD Extended Interrupt Rising Trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_RISING_EDGE()  CLEAR_BIT(EXTI->RTSR, PWR_EXTI_LINE_PVD)

/**
  * @brief Disable the PVD Extended Interrupt Falling Trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_FALLING_EDGE()  CLEAR_BIT(EXTI->FTSR, PWR_EXTI_LINE_PVD)

/**
  * @brief Disable the PVD Extended Interrupt Rising & Falling Trigger.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_DISABLE_RISING_FALLING_EDGE()  __HAL_PWR_PVD_EXTI_DISABLE_RISING_EDGE();__HAL_PWR_PVD_EXTI_DISABLE_FALLING_EDGE();

/**
  * @brief  PVD EXTI line configuration: set falling edge trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_FALLING_EDGE()  EXTI->FTSR |= (PWR_EXTI_LINE_PVD)

/**
  * @brief  PVD EXTI line configuration: set rising edge trigger.
  * @retval None.
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_RISING_EDGE()   EXTI->RTSR |= (PWR_EXTI_LINE_PVD)

/**
  * @brief  Enable the PVD Extended Interrupt Rising & Falling Trigger.
  * @retval None
  */
#define __HAL_PWR_PVD_EXTI_ENABLE_RISING_FALLING_EDGE()   __HAL_PWR_PVD_EXTI_ENABLE_RISING_EDGE();__HAL_PWR_PVD_EXTI_ENABLE_FALLING_EDGE();

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

#endif /* STM32F302xE || STM32F303xE || */
       /* STM32F302xC || STM32F303xC || */
       /* STM32F303x8 || STM32F334x8 || */
       /* STM32F301x8 || STM32F302x8 || */
       /* STM32F373xC                   */

/**
  * @}
  */

/* Private macros --------------------------------------------------------*/
/** @addtogroup  PWREx_Private_Macros   PWR Extended Private Macros
  * @{
  */

#if defined(STM32F302xE) || defined(STM32F303xE) || \
    defined(STM32F302xC) || defined(STM32F303xC) || \
    defined(STM32F303x8) || defined(STM32F334x8) || \
    defined(STM32F301x8) || defined(STM32F302x8) || \
    defined(STM32F373xC)
#define IS_PWR_PVD_LEVEL(LEVEL) (((LEVEL) == PWR_PVDLEVEL_0) || ((LEVEL) == PWR_PVDLEVEL_1)|| \
                                 ((LEVEL) == PWR_PVDLEVEL_2) || ((LEVEL) == PWR_PVDLEVEL_3)|| \
                                 ((LEVEL) == PWR_PVDLEVEL_4) || ((LEVEL) == PWR_PVDLEVEL_5)|| \
                                 ((LEVEL) == PWR_PVDLEVEL_6) || ((LEVEL) == PWR_PVDLEVEL_7))

#define IS_PWR_PVD_MODE(MODE) (((MODE) == PWR_PVD_MODE_IT_RISING)|| ((MODE) == PWR_PVD_MODE_IT_FALLING) || \
                              ((MODE) == PWR_PVD_MODE_IT_RISING_FALLING) || ((MODE) == PWR_PVD_MODE_EVENT_RISING) || \
                              ((MODE) == PWR_PVD_MODE_EVENT_FALLING) || ((MODE) == PWR_PVD_MODE_EVENT_RISING_FALLING) || \
                              ((MODE) == PWR_PVD_MODE_NORMAL))
#endif /* STM32F302xE || STM32F303xE || */
       /* STM32F302xC || STM32F303xC || */
       /* STM32F303x8 || STM32F334x8 || */
       /* STM32F301x8 || STM32F302x8 || */
       /* STM32F373xC                   */

#if defined(STM32F373xC) || defined(STM32F378xx)
#define IS_PWR_SDADC_ANALOG(SDADC) (((SDADC) == PWR_SDADC_ANALOG1) || \
                                    ((SDADC) == PWR_SDADC_ANALOG2) || \
                                    ((SDADC) == PWR_SDADC_ANALOG3))
#endif /* STM32F373xC || STM32F378xx */


/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @addtogroup PWREx_Exported_Functions PWR Extended Exported Functions
 *  @{
 */

/** @addtogroup PWREx_Exported_Functions_Group1 Peripheral Extended Control Functions
  * @{
  */
/* Peripheral Extended control functions **************************************/
#if defined(STM32F302xE) || defined(STM32F303xE) || \
    defined(STM32F302xC) || defined(STM32F303xC) || \
    defined(STM32F303x8) || defined(STM32F334x8) || \
    defined(STM32F301x8) || defined(STM32F302x8) || \
    defined(STM32F373xC)
void HAL_PWR_ConfigPVD(PWR_PVDTypeDef *sConfigPVD);
void HAL_PWR_EnablePVD(void);
void HAL_PWR_DisablePVD(void);
void HAL_PWR_PVD_IRQHandler(void);
void HAL_PWR_PVDCallback(void);
#endif /* STM32F302xE || STM32F303xE || */
       /* STM32F302xC || STM32F303xC || */
       /* STM32F303x8 || STM32F334x8 || */
       /* STM32F301x8 || STM32F302x8 || */
       /* STM32F373xC                   */

#if defined(STM32F373xC) || defined(STM32F378xx)
void HAL_PWREx_EnableSDADC(uint32_t Analogx);
void HAL_PWREx_DisableSDADC(uint32_t Analogx);
#endif /* STM32F373xC || STM32F378xx */

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

#endif /* __STM32F3xx_HAL_PWR_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
