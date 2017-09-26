/**
  ******************************************************************************
  * @file    stm32l4xx_hal_dfsdm_ex.c
  * @author  MCD Application Team
  * @brief   DFSDM Extended HAL module driver.  
  *          This file provides firmware functions to manage the following
  *          functionality of the DFSDM Peripheral Controller:
  *           + Set and get pulses skipping on channel.
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
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
#include "stm32l4xx_hal.h"

/** @addtogroup STM32L4xx_HAL_Driver
  * @{
  */

#ifdef HAL_DFSDM_MODULE_ENABLED

#if defined(STM32L4R5xx) || defined(STM32L4R7xx) || defined(STM32L4R9xx) || defined(STM32L4S5xx) || defined(STM32L4S7xx) || defined(STM32L4S9xx)

/** @defgroup DFSDMEx DFSDMEx
  * @brief DFSDM Extended HAL module driver
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/** @defgroup DFSDMEx_Exported_Functions DFSDM Extended Exported Functions
  * @{
  */

/** @defgroup DFSDMEx_Exported_Functions_Group1_Channel Extended channel operation functions
  * @brief    DFSDM extended channel operation functions
 *
@verbatim
 ===============================================================================
               ##### Extended channel operation functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Set and get value of pulses skipping on channel

@endverbatim
  * @{
  */

/**
  * @brief  Set value of pulses skipping.
  * @param  hdfsdm_channel : DFSDM channel handle.
  * @param  PulsesValue: Value of pulses to be skipped.
  *         This parameter must be a number between Min_Data = 0 and Max_Data = 63.
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_DFDSMEx_ChannelSetPulsesSkipping(DFSDM_Channel_HandleTypeDef *hdfsdm_channel, uint32_t PulsesValue)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check pulses value */
  assert_param(IS_DFSDM_CHANNEL_SKIPPING_VALUE(PulsesValue));
  
  /* Check DFSDM channel state */
  if(hdfsdm_channel->State == HAL_DFSDM_CHANNEL_STATE_READY)
  {
    /* Set new value of pulses skipping */
    hdfsdm_channel->Instance->CHDLYR = (PulsesValue & DFSDM_CHDLYR_PLSSKP);
  }
  else
  {
    status = HAL_ERROR;
  }
  return status;
}

/**
  * @brief  Get value of pulses skipping.
  * @param  hdfsdm_channel : DFSDM channel handle.
  * @param  PulsesValue: Value of pulses to be skipped.
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_DFDSMEx_ChannelGetPulsesSkipping(DFSDM_Channel_HandleTypeDef *hdfsdm_channel, uint32_t *PulsesValue)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check DFSDM channel state */
  if(hdfsdm_channel->State == HAL_DFSDM_CHANNEL_STATE_READY)
  {
    /* Get value of remaining pulses to be skipped */
    *PulsesValue = (hdfsdm_channel->Instance->CHDLYR & DFSDM_CHDLYR_PLSSKP);
  }
  else
  {
    status = HAL_ERROR;
  }
  return status;
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

#endif /* STM32L4R5xx || STM32L4R7xx || STM32L4R9xx || STM32L4S5xx || STM32L4S7xx || STM32L4S9xx */

#endif /* HAL_DFSDM_MODULE_ENABLED */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
