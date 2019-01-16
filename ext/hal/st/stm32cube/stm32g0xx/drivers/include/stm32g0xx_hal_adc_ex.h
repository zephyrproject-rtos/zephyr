/**
  ******************************************************************************
  * @file    stm32g0xx_hal_adc_ex.h
  * @author  MCD Application Team
  * @brief   Header file of ADC HAL extended module.
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
#ifndef STM32G0xx_HAL_ADC_EX_H
#define STM32G0xx_HAL_ADC_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal_def.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @addtogroup ADCEx
  * @{
  */ 

/* Exported types ------------------------------------------------------------*/
/** @defgroup ADCEx_Exported_Types ADC Extended Exported Types
  * @{
  */

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup ADCEx_Exported_Constants ADC Extended Exported Constants
  * @{
  */

/** @defgroup ADC_HAL_EC_GROUPS  ADC instance - Groups
  * @{
  */
#define ADC_REGULAR_GROUP                  (LL_ADC_GROUP_REGULAR)           /*!< ADC group regular (available on all STM32 devices) */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/** @defgroup ADCEx_Private_Macro_internal_HAL_driver ADC Extended Private Macros
  * @{
  */
/* Macro reserved for internal HAL driver usage, not intended to be used in   */
/* code of final user.                                                        */

/**
  * @brief Check whether or not ADC is independent.
  * @param __HANDLE__ ADC handle.
  * @note  When multimode feature is not available, the macro always returns SET.   
  * @retval SET (ADC is independent) or RESET (ADC is not).
  */
#define ADC_IS_INDEPENDENT(__HANDLE__)   (SET)


/**
  * @brief Calibration factor size verification (7 bits maximum).
  * @param __CALIBRATION_FACTOR__ Calibration factor value.
  * @retval SET (__CALIBRATION_FACTOR__ is within the authorized size) or RESET (__CALIBRATION_FACTOR__ is too large)
  */
#define IS_ADC_CALFACT(__CALIBRATION_FACTOR__) ((__CALIBRATION_FACTOR__) <= (0x7FU))

/**
  * @brief Verify the ADC oversampling ratio. 
  * @param __RATIO__ programmed ADC oversampling ratio.
  * @retval SET (__RATIO__ is a valid value) or RESET (__RATIO__ is invalid)
  */
#define IS_ADC_OVERSAMPLING_RATIO(__RATIO__)      (((__RATIO__) == ADC_OVERSAMPLING_RATIO_2   ) || \
                                                   ((__RATIO__) == ADC_OVERSAMPLING_RATIO_4   ) || \
                                                   ((__RATIO__) == ADC_OVERSAMPLING_RATIO_8   ) || \
                                                   ((__RATIO__) == ADC_OVERSAMPLING_RATIO_16  ) || \
                                                   ((__RATIO__) == ADC_OVERSAMPLING_RATIO_32  ) || \
                                                   ((__RATIO__) == ADC_OVERSAMPLING_RATIO_64  ) || \
                                                   ((__RATIO__) == ADC_OVERSAMPLING_RATIO_128 ) || \
                                                   ((__RATIO__) == ADC_OVERSAMPLING_RATIO_256 ))

/**
  * @brief Verify the ADC oversampling shift. 
  * @param __SHIFT__ programmed ADC oversampling shift.
  * @retval SET (__SHIFT__ is a valid value) or RESET (__SHIFT__ is invalid)
  */
#define IS_ADC_RIGHT_BIT_SHIFT(__SHIFT__)        (((__SHIFT__) == ADC_RIGHTBITSHIFT_NONE) || \
                                                  ((__SHIFT__) == ADC_RIGHTBITSHIFT_1   ) || \
                                                  ((__SHIFT__) == ADC_RIGHTBITSHIFT_2   ) || \
                                                  ((__SHIFT__) == ADC_RIGHTBITSHIFT_3   ) || \
                                                  ((__SHIFT__) == ADC_RIGHTBITSHIFT_4   ) || \
                                                  ((__SHIFT__) == ADC_RIGHTBITSHIFT_5   ) || \
                                                  ((__SHIFT__) == ADC_RIGHTBITSHIFT_6   ) || \
                                                  ((__SHIFT__) == ADC_RIGHTBITSHIFT_7   ) || \
                                                  ((__SHIFT__) == ADC_RIGHTBITSHIFT_8   ))

/**
  * @brief Verify the ADC oversampling triggered mode. 
  * @param __MODE__ programmed ADC oversampling triggered mode. 
  * @retval SET (__MODE__ is valid) or RESET (__MODE__ is invalid)
  */
#define IS_ADC_TRIGGERED_OVERSAMPLING_MODE(__MODE__) (((__MODE__) == ADC_TRIGGEREDMODE_SINGLE_TRIGGER) || \
                                                      ((__MODE__) == ADC_TRIGGEREDMODE_MULTI_TRIGGER) ) 


/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/
/** @addtogroup ADCEx_Exported_Functions
  * @{
  */

/** @addtogroup ADCEx_Exported_Functions_Group1
  * @{
  */
/* IO operation functions *****************************************************/

/* ADC calibration */
HAL_StatusTypeDef       HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef* hadc);
uint32_t                HAL_ADCEx_Calibration_GetValue(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef       HAL_ADCEx_Calibration_SetValue(ADC_HandleTypeDef* hadc, uint32_t CalibrationFactor);

/* ADC IRQHandler and Callbacks used in non-blocking modes (Interruption) */
void                    HAL_ADCEx_LevelOutOfWindow2Callback(ADC_HandleTypeDef* hadc);
void                    HAL_ADCEx_LevelOutOfWindow3Callback(ADC_HandleTypeDef* hadc);
void                    HAL_ADCEx_EndOfSamplingCallback(ADC_HandleTypeDef* hadc);
void                    HAL_ADCEx_ChannelConfigReadyCallback(ADC_HandleTypeDef* hadc);

/**
  * @}
  */

/** @addtogroup ADCEx_Exported_Functions_Group2
  * @{
  */
/* Peripheral Control functions ***********************************************/
HAL_StatusTypeDef       HAL_ADCEx_DisableVoltageRegulator(ADC_HandleTypeDef* hadc);

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

#endif /* STM32G0xx_HAL_ADC_EX_H */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
