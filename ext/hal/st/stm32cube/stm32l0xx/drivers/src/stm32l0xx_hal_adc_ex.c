/**
  ******************************************************************************
  * @file    stm32l0xx_hal_adc_ex.c
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of the Analog to Digital Convertor (ADC)
  *          peripheral:
  *           + Start calibration.
  *           + Read the calibration factor.
  *           + Set a calibration factor.
  *          
  @verbatim
  ==============================================================================
                    ##### ADC specific features #####
  ==============================================================================
  [..] 
  (#) Self calibration.


                     ##### How to use this driver #####
  ==============================================================================
    [..]

    (#) Call HAL_ADCEx_Calibration_Start() to start calibration
    
    (#) Read the calibration factor using HAL_ADCEx_Calibration_GetValue()
  
    (#) User can set a his calibration factor using HAL_ADCEx_Calibration_SetValue()
  
    @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"

/** @addtogroup STM32L0xx_HAL_Driver
  * @{
  */

#ifdef HAL_ADC_MODULE_ENABLED

/** @addtogroup ADCEx 
  * @brief ADC driver modules
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Fixed timeout values for ADC calibration, enable settling time, disable  */
  /* settling time.                                                           */
  /* Values defined to be higher than worst cases: low clock frequency,       */
  /* maximum prescaler.                                                       */
  /* Unit: ms                                                                 */
  #define ADC_CALIBRATION_TIMEOUT      10U      

/* Delay for VREFINT stabilization time. */
/* Internal reference startup time max value is 3ms  (refer to device datasheet, parameter TVREFINT). */
/* Unit: ms */
#define SYSCFG_BUF_VREFINT_ENABLE_TIMEOUT       ((uint32_t) 3U)

/* Delay for TEMPSENSOR stabilization time. */
/* Temperature sensor startup time max value is 10us  (refer to device datasheet, parameter tSTART). */
/* Unit: ms */
#define SYSCFG_BUF_TEMPSENSOR_ENABLE_TIMEOUT    ((uint32_t) 1U)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/


/** @addtogroup ADCEx_Exported_Functions
 *  @brief    ADC Extended features functions 
 *
@verbatim   
 ===============================================================================
            ##### ADC Extended features functions #####
 ===============================================================================  
    [..]
This subsection provides functions allowing to:
      (+) Start calibration.
      (+) Get calibration factor.
      (+) Set calibration factor.
      (+) Enable VREFInt.
      (+) Disable VREFInt.
      (+) Enable VREFInt TempSensor.
      (+) Disable VREFInt TempSensor.

@endverbatim
  * @{
  */

/** @addtogroup ADCEx_Exported_Functions_Group3
  * @{
  */

/**
  * @brief  Start an automatic calibration
  * @param  hadc: pointer to a ADC_HandleTypeDef structure that contains
  *         the configuration information for the specified ADC.
  * @param  SingleDiff: Selection of single-ended or differential input
  *          This parameter can be only of the following values:
  *            @arg ADC_SINGLE_ENDED: Channel in mode input single ended
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef* hadc, uint32_t SingleDiff)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  uint32_t tickstart=0U;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* Calibration prerequisite: ADC must be disabled. */
  if (ADC_IS_ENABLE(hadc) == RESET)
  {
    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State, 
                      HAL_ADC_STATE_REG_BUSY,
                      HAL_ADC_STATE_BUSY_INTERNAL);
    
    /* Start ADC calibration */
    hadc->Instance->CR |= ADC_CR_ADCAL;

    tickstart = HAL_GetTick();  

    /* Wait for calibration completion */
    while(HAL_IS_BIT_SET(hadc->Instance->CR, ADC_CR_ADCAL))
    {
      if((HAL_GetTick() - tickstart) > ADC_CALIBRATION_TIMEOUT)
      {
        /* Update ADC state machine to error */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_BUSY_INTERNAL,
                          HAL_ADC_STATE_ERROR_INTERNAL);
        
        /* Process unlocked */
        __HAL_UNLOCK(hadc);
        
        return HAL_ERROR;
      }
    }
    
    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_BUSY_INTERNAL,
                      HAL_ADC_STATE_READY);
  }
  else
  {
    /* Update ADC state machine to error */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
    
    tmp_hal_status = HAL_ERROR;
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}


/**
  * @brief  Get the calibration factor.
  * @param  hadc: ADC handle.
  * @param  SingleDiff: This parameter can be only:
  *           @arg ADC_SINGLE_ENDED: Channel in mode input single ended.
  * @retval Calibration value.
  */
uint32_t HAL_ADCEx_Calibration_GetValue(ADC_HandleTypeDef* hadc, uint32_t SingleDiff)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_SINGLE_DIFFERENTIAL(SingleDiff)); 
  
  /* Return the ADC calibration value */ 
  return ((hadc->Instance->CALFACT) & 0x0000007FU);
}

/**
  * @brief  Set the calibration factor to overwrite automatic conversion result.
  *         ADC must be enabled and no conversion is ongoing.
  * @param  hadc: ADC handle
  * @param  SingleDiff: This parameter can be only:
  *           @arg ADC_SINGLE_ENDED: Channel in mode input single ended.
  * @param  CalibrationFactor: Calibration factor (coded on 7 bits maximum)
  * @retval HAL state
  */
HAL_StatusTypeDef HAL_ADCEx_Calibration_SetValue(ADC_HandleTypeDef* hadc, uint32_t SingleDiff, uint32_t CalibrationFactor)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_SINGLE_DIFFERENTIAL(SingleDiff)); 
  assert_param(IS_ADC_CALFACT(CalibrationFactor)); 
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* Verification of hardware constraints before modifying the calibration    */
  /* factors register: ADC must be enabled, no conversion on going.           */
  if ( (ADC_IS_ENABLE(hadc) != RESET)                            &&
       (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)  )
  {
    /* Set the selected ADC calibration value */ 
    hadc->Instance->CALFACT &= ~ADC_CALFACT_CALFACT;
    hadc->Instance->CALFACT |= CalibrationFactor;
  }
  else
  {
    /* Update ADC state machine to error */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
    /* Update ADC state machine to error */
    SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
    
    /* Update ADC state machine to error */
    tmp_hal_status = HAL_ERROR;
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Enables the buffer of Vrefint for the ADC, required when device is in mode low-power (low-power run, low-power sleep or stop mode)
  *         This function must be called before function HAL_ADC_Init() 
  *         (in case of previous ADC operations: function HAL_ADC_DeInit() must be called first)
  *         For more details on procedure and buffer current consumption, refer to device reference manual.
  * @note   This is functional only if the LOCK is not set.
  * @retval None
*/
HAL_StatusTypeDef HAL_ADCEx_EnableVREFINT(void)
{
  uint32_t tickstart = 0U;
  
  /* Enable the Buffer for the ADC by setting ENBUF_SENSOR_ADC bit in the CFGR3 register */
  SET_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUF_VREFINT_ADC);
  
  /* Wait for Vrefint buffer effectively enabled */
  /* Get tick count */
  tickstart = HAL_GetTick();
  
  while(HAL_IS_BIT_CLR(SYSCFG->CFGR3, SYSCFG_CFGR3_VREFINT_RDYF))
  {
    if((HAL_GetTick() - tickstart) > SYSCFG_BUF_VREFINT_ENABLE_TIMEOUT)
    { 
      return HAL_ERROR;
    }
  }
  
  return HAL_OK;
}

/**
  * @brief Disables the Buffer Vrefint for the ADC.
  * @note This is functional only if the LOCK is not set.
  * @retval None
  */
void HAL_ADCEx_DisableVREFINT(void)
{
  /* Disable the Vrefint by resetting ENBUF_SENSOR_ADC bit in the CFGR3 register */
  CLEAR_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUF_VREFINT_ADC);
}

/**
* @brief  Enables the buffer of temperature sensor for the ADC, required when device is in mode low-power (low-power run, low-power sleep or stop mode)
*         This function must be called before function HAL_ADC_Init()
*         (in case of previous ADC operations: function HAL_ADC_DeInit() must be called first)
*         For more details on procedure and buffer current consumption, refer to device reference manual.
* @note   This is functional only if the LOCK is not set.
* @retval None
*/
HAL_StatusTypeDef HAL_ADCEx_EnableVREFINTTempSensor(void)
{
  uint32_t tickstart = 0U;
  
  /* Enable the Buffer for the ADC by setting ENBUF_SENSOR_ADC bit in the CFGR3 register */
  SET_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUF_SENSOR_ADC);
  
  /* Wait for Vrefint buffer effectively enabled */
  /* Get tick count */
  tickstart = HAL_GetTick();
  
  while(HAL_IS_BIT_CLR(SYSCFG->CFGR3, SYSCFG_CFGR3_VREFINT_RDYF))
  {
    if((HAL_GetTick() - tickstart) > SYSCFG_BUF_TEMPSENSOR_ENABLE_TIMEOUT)
    { 
      return HAL_ERROR;
    }
  }
  
  return HAL_OK;
}

/**
  * @brief Disables the VREFINT and Sensor for the ADC.
  * @note This is functional only if the LOCK is not set.
  * @retval None
  */
void HAL_ADCEx_DisableVREFINTTempSensor(void)
{
  /* Disable the Vrefint by resetting ENBUF_SENSOR_ADC bit in the CFGR3 register */
  CLEAR_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUF_SENSOR_ADC);
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_ADC_MODULE_ENABLED */
/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
