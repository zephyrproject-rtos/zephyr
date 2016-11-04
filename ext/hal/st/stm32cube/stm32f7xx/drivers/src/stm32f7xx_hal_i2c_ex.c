/**
  ******************************************************************************
  * @file    stm32f7xx_hal_i2c_ex.c
  * @author  MCD Application Team
  * @version V1.1.1
  * @date    01-July-2016
  * @brief   I2C Extended HAL module driver.
  *          This file provides firmware functions to manage the following 
  *          functionalities of I2C Extended peripheral:
  *           + Extended features functions
  *
  @verbatim
  ==============================================================================
               ##### I2C peripheral Extended features  #####
  ==============================================================================

  [..] Comparing to other previous devices, the I2C interface for STM32F7XX
       devices contains the following additional features

       (+) Possibility to disable or enable Analog Noise Filter
       (+) Use of a configured Digital Noise Filter
       (+) Disable or enable Fast Mode Plus (available only for STM32F76xxx/STM32F77xxx 
           devices)

                     ##### How to use this driver #####
  ==============================================================================
  [..] This driver provides functions to:
    (#) Configure I2C Analog noise filter using the function HAL_I2CEx_ConfigAnalogFilter()
    (#) Configure I2C Digital noise filter using the function HAL_I2CEx_ConfigDigitalFilter()
    (#) Configure the enable or disable of fast mode plus driving capability using the functions :
          (++) HAL_I2CEx_EnableFastModePlus()
          (++) HAL_I2CEx_DisbleFastModePlus()
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
#include "stm32f7xx_hal.h"

/** @addtogroup STM32F7xx_HAL_Driver
  * @{
  */

/** @defgroup I2CEx I2C Extended HAL module driver
  * @brief I2C Extended HAL module driver
  * @{
  */

#ifdef HAL_I2C_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup I2CEx_Exported_Functions I2C Extended Exported Functions
  * @{
  */

/** @defgroup I2CEx_Exported_Functions_Group1 Extended features functions
  * @brief    Extended features functions
 *
@verbatim
 ===============================================================================
                      ##### Extended features functions #####
 ===============================================================================
    [..] This section provides functions allowing to:
      (+) Configure Noise Filters
      (+) Configure Fast Mode Plus

@endverbatim
  * @{
  */

/**
  * @brief  Configure I2C Analog noise filter.
  * @param  hi2c: Pointer to a I2C_HandleTypeDef structure that contains
  *               the configuration information for the specified I2Cx peripheral.
  * @param  AnalogFilter: New state of the Analog filter.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_I2CEx_ConfigAnalogFilter(I2C_HandleTypeDef *hi2c, uint32_t AnalogFilter)
{
  /* Check the parameters */
  assert_param(IS_I2C_ALL_INSTANCE(hi2c->Instance));
  assert_param(IS_I2C_ANALOG_FILTER(AnalogFilter));
  
  if(hi2c->State == HAL_I2C_STATE_READY)
  {
    /* Process Locked */
    __HAL_LOCK(hi2c);
    
    hi2c->State = HAL_I2C_STATE_BUSY;
    
    /* Disable the selected I2C peripheral */
    __HAL_I2C_DISABLE(hi2c);
    
    /* Reset I2Cx ANOFF bit */
    hi2c->Instance->CR1 &= ~(I2C_CR1_ANFOFF);
    
    /* Set analog filter bit*/
    hi2c->Instance->CR1 |= AnalogFilter;
    
    __HAL_I2C_ENABLE(hi2c);
    
    hi2c->State = HAL_I2C_STATE_READY;
    
    /* Process Unlocked */
    __HAL_UNLOCK(hi2c);
    
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief  Configure I2C Digital noise filter.
  * @param  hi2c: Pointer to a I2C_HandleTypeDef structure that contains
  *               the configuration information for the specified I2Cx peripheral.
  * @param  DigitalFilter: Coefficient of digital noise filter between 0x00 and 0x0F.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_I2CEx_ConfigDigitalFilter(I2C_HandleTypeDef *hi2c, uint32_t DigitalFilter)
{
  uint32_t tmpreg = 0;
  
  /* Check the parameters */
  assert_param(IS_I2C_ALL_INSTANCE(hi2c->Instance));
  assert_param(IS_I2C_DIGITAL_FILTER(DigitalFilter));
  
  if(hi2c->State == HAL_I2C_STATE_READY)
  {
    /* Process Locked */
    __HAL_LOCK(hi2c);
    
    hi2c->State = HAL_I2C_STATE_BUSY;
    
    /* Disable the selected I2C peripheral */
    __HAL_I2C_DISABLE(hi2c);
    
    /* Get the old register value */
    tmpreg = hi2c->Instance->CR1;
    
    /* Reset I2Cx DNF bits [11:8] */
    tmpreg &= ~(I2C_CR1_DNF);
    
    /* Set I2Cx DNF coefficient */
    tmpreg |= DigitalFilter << 8;
    
    /* Store the new register value */
    hi2c->Instance->CR1 = tmpreg;
    
    __HAL_I2C_ENABLE(hi2c);
    
    hi2c->State = HAL_I2C_STATE_READY;
    
    /* Process Unlocked */
    __HAL_UNLOCK(hi2c);
    
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

#if defined (STM32F765xx) || defined (STM32F767xx) || defined (STM32F769xx) || defined (STM32F777xx) || defined (STM32F779xx)
/**
  * @brief Enable the I2C fast mode plus driving capability.
  * @param ConfigFastModePlus: Selects the pin.
  *   This parameter can be one of the @ref I2CEx_FastModePlus values
  * @retval None
  */
void HAL_I2CEx_EnableFastModePlus(uint32_t ConfigFastModePlus)
{
  /* Check the parameter */
  assert_param(IS_I2C_FASTMODEPLUS(ConfigFastModePlus));
  
  /* Enable SYSCFG clock */
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  
  /* Enable fast mode plus driving capability for selected pin */
  SET_BIT(SYSCFG->PMC, (uint32_t)ConfigFastModePlus);
}

/**
  * @brief Disable the I2C fast mode plus driving capability.
  * @param ConfigFastModePlus: Selects the pin.
  *   This parameter can be one of the @ref I2CEx_FastModePlus values
  * @retval None
  */
void HAL_I2CEx_DisableFastModePlus(uint32_t ConfigFastModePlus)
{
  /* Check the parameter */
  assert_param(IS_I2C_FASTMODEPLUS(ConfigFastModePlus));
  
  /* Enable SYSCFG clock */
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  
  /* Disable fast mode plus driving capability for selected pin */
  CLEAR_BIT(SYSCFG->PMC, (uint32_t)ConfigFastModePlus);
}
#endif /* STM32F767xx || STM32F769xx || STM32F777xx || STM32F779xx */

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_I2C_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
