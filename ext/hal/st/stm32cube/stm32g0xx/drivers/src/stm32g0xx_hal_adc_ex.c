/**
  ******************************************************************************
  * @file    stm32g0xx_hal_adc_ex.c
  * @author  MCD Application Team
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of the Analog to Digital Convertor (ADC)
  *          peripheral:
  *           + Operation functions
  *             ++ Calibration
  *               +++ ADC automatic self-calibration
  *               +++ Calibration factors get or set
  *          Other functions (generic functions) are available in file 
  *          "stm32g0xx_hal_adc.c".
  *
  @verbatim
  [..] 
  (@) Sections "ADC peripheral features" and "How to use this driver" are
      available in file of generic functions "stm32g0xx_hal_adc.c".
  [..]
  @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @defgroup ADCEx ADCEx
  * @brief ADC Extended HAL module driver
  * @{
  */

#ifdef HAL_ADC_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/** @defgroup ADCEx_Private_Constants ADC Extended Private Constants
  * @{
  */

/* Fixed timeout value for ADC calibration.                                   */
/* Values defined to be higher than worst cases: maximum ratio between ADC    */
/* and CPU clock frequencies.                                                 */
/* Example of profile low frequency : ADC frequency at 31.25kHz (ADC clock    */
/* source PLL 8MHz, ADC clock prescaler 256), CPU frequency 52MHz.            */
/* Calibration time max = 116 / fADC (refer to datasheet)                     */
/*                      = 193 024 CPU cycles                                  */
#define ADC_CALIBRATION_TIMEOUT         (193024UL)   /*!< ADC calibration time-out value (unit: CPU cycles) */
/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/** @defgroup ADCEx_Exported_Functions ADC Extended Exported Functions
  * @{
  */

/** @defgroup ADCEx_Exported_Functions_Group1 Extended Input and Output operation functions
  * @brief    Extended IO operation functions
  *
@verbatim
 ===============================================================================
                      ##### IO operation functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      
      (+) Perform the ADC self-calibration.
      (+) Get calibration factors.
      (+) Set calibration factors.

@endverbatim
  * @{
  */

/**
  * @brief  Perform an ADC automatic self-calibration
  *         Calibration prerequisite: ADC must be disabled (execute this
  *         function before HAL_ADC_Start() or after HAL_ADC_Stop() ).
  * @note   Calibration factor can be read after calibration, using function
  *         HAL_ADC_GetValue() (value on 7 bits: from DR[6;0]).
  * @param  hadc       ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status;
  __IO uint32_t wait_loop_index = 0UL;
  uint32_t backup_setting_adc_dma_transfer; /* Note: Variable not declared as volatile because register read is already declared as volatile */
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* Calibration prerequisite: ADC must be disabled. */
  
  /* Disable the ADC (if not already disabled) */
  tmp_hal_status = ADC_Disable(hadc);
  
  /* Check if ADC is effectively disabled */
  if (LL_ADC_IsEnabled(hadc->Instance) == 0UL)
  {
    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State, 
                      HAL_ADC_STATE_REG_BUSY,
                      HAL_ADC_STATE_BUSY_INTERNAL);
    
    /* Disable ADC DMA transfer request during calibration */
    /* Note: Specificity of this STM32 serie: Calibration factor is           */
    /*       available in data register and also transfered by DMA.           */
    /*       To not insert ADC calibration factor among ADC conversion data   */
    /*       in array variable, DMA transfer must be disabled during          */
    /*       calibration.                                                     */
    backup_setting_adc_dma_transfer = READ_BIT(hadc->Instance->CFGR1, ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG);
    CLEAR_BIT(hadc->Instance->CFGR1, ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG);
    
    /* Start ADC calibration */
    SET_BIT(hadc->Instance->CR, ADC_CR_ADCAL);
    
    /* Wait for calibration completion */
    while(LL_ADC_IsCalibrationOnGoing(hadc->Instance) != 0UL)
    {
      wait_loop_index++;
      if (wait_loop_index >= ADC_CALIBRATION_TIMEOUT)
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
    
    /* Restore ADC DMA transfer request after calibration */
    SET_BIT(hadc->Instance->CFGR1, backup_setting_adc_dma_transfer);
    
    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_BUSY_INTERNAL,
                      HAL_ADC_STATE_READY);
  }
  else
  {
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
    
    /* Note: No need to update variable "tmp_hal_status" here: already set    */
    /*       to state "HAL_ERROR" by function disabling the ADC.              */
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Get the calibration factor.
  * @param hadc ADC handle.
  * @retval Calibration value.
  */
uint32_t HAL_ADCEx_Calibration_GetValue(ADC_HandleTypeDef* hadc)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Return the selected ADC calibration value */
  return ((hadc->Instance->CALFACT) & 0x0000007FU);
}

/**
  * @brief  Set the calibration factor to overwrite automatic conversion result.
  *         ADC must be enabled and no conversion is ongoing.
  * @param hadc ADC handle
  * @param CalibrationFactor Calibration factor (coded on 7 bits maximum)
  * @retval HAL state
  */
HAL_StatusTypeDef HAL_ADCEx_Calibration_SetValue(ADC_HandleTypeDef* hadc, uint32_t CalibrationFactor)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  uint32_t tmp_adc_is_conversion_on_going_regular;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_CALFACT(CalibrationFactor));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* Verification of hardware constraints before modifying the calibration    */
  /* factors register: ADC must be enabled, no conversion on going.           */
  tmp_adc_is_conversion_on_going_regular = LL_ADC_REG_IsConversionOngoing(hadc->Instance);
  
  if (   (LL_ADC_IsEnabled(hadc->Instance) != 0UL)
      && (tmp_adc_is_conversion_on_going_regular == 0UL)
     )
  {
    hadc->Instance->CALFACT &= ~ADC_CALFACT_CALFACT;
    hadc->Instance->CALFACT |= CalibrationFactor;
  }
  else
  {
    /* Update ADC state machine */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
    /* Update ADC error code */
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
  * @brief  Analog watchdog 2 callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_LevelOutOfWindow2Callback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  
  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADCEx_LevelOutOfWindow2Callback must be implemented in the user file.
  */
}

/**
  * @brief  Analog watchdog 3 callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_LevelOutOfWindow3Callback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  
  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADCEx_LevelOutOfWindow3Callback must be implemented in the user file.
  */
}


/**
  * @brief  End Of Sampling callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_EndOfSamplingCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  
  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADCEx_EndOfSamplingCallback must be implemented in the user file.
  */
}

/**
  * @brief  ADC channel configuration ready callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_ChannelConfigReadyCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  
  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADCEx_ChannelConfigReadyCallback must be implemented in the user file.
  */
}

/**
  * @}
  */

/**
  * @brief  Disable ADC voltage regulator.
  * @note   Disabling voltage regulator allows to save power. This operation can
  *         be carried out only when ADC is disabled.
  * @note   To enable again the voltage regulator, the user is expected to 
  *         resort to HAL_ADC_Init() API.
  * @param hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_DisableVoltageRegulator(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Setting of this feature is conditioned to ADC state: ADC must be ADC disabled */
  if (LL_ADC_IsEnabled(hadc->Instance) == 0UL)
  {
    LL_ADC_DisableInternalRegulator(hadc->Instance);
    tmp_hal_status = HAL_OK;
  }
  else
  {
    tmp_hal_status = HAL_ERROR;
  }
  
  return tmp_hal_status;
}

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

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
