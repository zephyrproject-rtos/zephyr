/**
  ******************************************************************************
  * @file    stm32l4xx_hal_dma_ex.c
  * @author  MCD Application Team
  * @brief   DMA Extension HAL module driver
  *         This file provides firmware functions to manage the following
  *         functionalities of the DMA Extension peripheral:
  *           + Extended features functions
  *
  @verbatim
  ==============================================================================
                        ##### How to use this driver #####
  ==============================================================================
  [..]
  The DMA Extension HAL driver can be used as follows:

   (+) Configure the DMA_MUX Synchronization Block using HAL_DMAEx_ConfigMuxSync function.
   (+) Configure the DMA_MUX Request Generator Block using HAL_DMAEx_ConfigMuxRequestGenerator function.
       Functions HAL_DMAEx_EnableMuxRequestGenerator and HAL_DMAEx_DisableMuxRequestGenerator can then be used
       to respectively enable/disable the request generator.

   (+) To handle the DMAMUX Interrupts, the function  HAL_DMAEx_MUX_IRQHandler should be called from
       the DMAMUX IRQ handler i.e DMAMUX1_OVR_IRQHandler.
       As only one interrupt line is available for all DMAMUX channels and request generators , HAL_DMAEx_MUX_IRQHandler should be
       called with, as parameter, the appropriate DMA handle as many as used DMAs in the user project
      (exception done if a given DMA is not using the DMAMUX SYNC block neither a request generator)

  @endverbatim
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

#if defined(DMAMUX1)

/** @addtogroup STM32L4xx_HAL_Driver
  * @{
  */

/** @defgroup DMAEx DMAEx
  * @brief DMA Extended HAL module driver
  * @{
  */

#ifdef HAL_DMA_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private Constants ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/


/** @defgroup DMAEx_Exported_Functions DMAEx Exported Functions
  * @{
  */

/** @defgroup DMAEx_Exported_Functions_Group1 DMAEx Extended features functions
 *  @brief   Extended features functions
 *
@verbatim
 ===============================================================================
                #####  Extended features functions  #####
 ===============================================================================
    [..]  This section provides functions allowing to:

    (+) Configure the DMAMUX Synchronization Block using HAL_DMAEx_ConfigMuxSync function.
    (+) Configure the DMAMUX Request Generator Block using HAL_DMAEx_ConfigMuxRequestGenerator function.
       Functions HAL_DMAEx_EnableMuxRequestGenerator and HAL_DMAEx_DisableMuxRequestGenerator can then be used
       to respectively enable/disable the request generator.

@endverbatim
  * @{
  */


/**
  * @brief  Configure the DMAMUX synchronization parameters for a given DMA channel (instance).
  * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA channel.
  * @param  pSyncConfig : pointer to HAL_DMA_MuxSyncConfigTypeDef : contains the DMAMUX synchronization parameters
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMAEx_ConfigMuxSync(DMA_HandleTypeDef *hdma, HAL_DMA_MuxSyncConfigTypeDef *pSyncConfig)
{
  /* Check the parameters */
  assert_param(IS_DMA_ALL_INSTANCE(hdma->Instance));

  assert_param(IS_DMAMUX_SYNC_SIGNAL_ID(pSyncConfig->SyncSignalID));

  assert_param(IS_DMAMUX_SYNC_POLARITY(pSyncConfig-> SyncPolarity));
  assert_param(IS_DMAMUX_SYNC_STATE(pSyncConfig->SyncEnable));
  assert_param(IS_DMAMUX_SYNC_EVENT(pSyncConfig->EventEnable));
  assert_param(IS_DMAMUX_SYNC_REQUEST_NUMBER(pSyncConfig->RequestNumber));

  /*Check if the DMA state is ready */
  if(hdma->State == HAL_DMA_STATE_READY)
  {
    /* Process Locked */
    __HAL_LOCK(hdma);

    /* Set the new synchronization parameters (and keep the request ID filled during the Init)*/
    MODIFY_REG( hdma->DMAmuxChannel->CCR, \
               (~DMAMUX_CxCR_DMAREQ_ID) , \
               ((pSyncConfig->SyncSignalID) << DMAMUX_CxCR_SYNC_ID_Pos) | ((pSyncConfig->RequestNumber - 1U) << DMAMUX_CxCR_NBREQ_Pos) | \
               pSyncConfig->SyncPolarity | (pSyncConfig->SyncEnable << DMAMUX_CxCR_SE_Pos) | \
                 (pSyncConfig->EventEnable << DMAMUX_CxCR_EGE_Pos));

    /* Process UnLocked */
    __HAL_UNLOCK(hdma);

    return HAL_OK;
  }
  else
  {
    /*DMA State not Ready*/
    return HAL_ERROR;
  }
}

/**
  * @brief  Configure the DMAMUX request generator block used by the given DMA channel (instance).
  * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA channel.
  * @param  pRequestGeneratorConfig : pointer to HAL_DMA_MuxRequestGeneratorConfigTypeDef :
  *         contains the request generator parameters.
  *
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMAEx_ConfigMuxRequestGenerator (DMA_HandleTypeDef *hdma, HAL_DMA_MuxRequestGeneratorConfigTypeDef *pRequestGeneratorConfig)
{
  /* Check the parameters */
  assert_param(IS_DMA_ALL_INSTANCE(hdma->Instance));

  assert_param(IS_DMAMUX_REQUEST_GEN_SIGNAL_ID(pRequestGeneratorConfig->SignalID));

  assert_param(IS_DMAMUX_REQUEST_GEN_POLARITY(pRequestGeneratorConfig->Polarity));
  assert_param(IS_DMAMUX_REQUEST_GEN_REQUEST_NUMBER(pRequestGeneratorConfig->RequestNumber));

  /* check if the DMA state is ready
     and DMA is using a DMAMUX request generator block
  */
  if((hdma->State == HAL_DMA_STATE_READY) && (hdma->DMAmuxRequestGen != 0U))
  {
    /* Process Locked */
    __HAL_LOCK(hdma);

    /* Set the request generator new parameters*/
    hdma->DMAmuxRequestGen->RGCR = pRequestGeneratorConfig->SignalID | \
                                  ((pRequestGeneratorConfig->RequestNumber - 1U) << POSITION_VAL(DMAMUX_RGxCR_GNBREQ))| \
                                  pRequestGeneratorConfig->Polarity;
   /* Process UnLocked */
   __HAL_UNLOCK(hdma);

   return HAL_OK;
 }
 else
 {
   return HAL_ERROR;
 }
}

/**
  * @brief  Enable the DMAMUX request generator block used by the given DMA channel (instance).
  * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA channel.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMAEx_EnableMuxRequestGenerator (DMA_HandleTypeDef *hdma)
{
  /* Check the parameters */
  assert_param(IS_DMA_ALL_INSTANCE(hdma->Instance));

  /* check if the DMA state is ready
     and DMA is using a DMAMUX request generator block
  */
  if((hdma->State != HAL_DMA_STATE_RESET) && (hdma->DMAmuxRequestGen != 0))
  {

    /* Enable the request generator*/
    hdma->DMAmuxRequestGen->RGCR |= DMAMUX_RGxCR_GE;

   return HAL_OK;
 }
 else
 {
   return HAL_ERROR;
 }
}

/**
  * @brief  Disable the DMAMUX request generator block used by the given DMA channel (instance).
  * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA channel.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMAEx_DisableMuxRequestGenerator (DMA_HandleTypeDef *hdma)
{
  /* Check the parameters */
  assert_param(IS_DMA_ALL_INSTANCE(hdma->Instance));

  /* check if the DMA state is ready
     and DMA is using a DMAMUX request generator block
  */
  if((hdma->State != HAL_DMA_STATE_RESET) && (hdma->DMAmuxRequestGen != 0))
  {

    /* Disable the request generator*/
    hdma->DMAmuxRequestGen->RGCR &= ~DMAMUX_RGxCR_GE;

    return HAL_OK;
  }
  else
  {
    return HAL_ERROR;
  }
}

/**
  * @brief  Handles DMAMUX interrupt request.
  * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
  *               the configuration information for the specified DMA channel.
  * @retval None
  */
void HAL_DMAEx_MUX_IRQHandler(DMA_HandleTypeDef *hdma)
{
  /* Check for DMAMUX Synchronization overrun */
  if((hdma->DMAmuxChannelStatus->CSR & hdma->DMAmuxChannelStatusMask) != 0U)
  {
    /* Disable the synchro overrun interrupt */
    hdma->DMAmuxChannel->CCR &= ~DMAMUX_CxCR_SOIE;

    /* Clear the DMAMUX synchro overrun flag */
    hdma->DMAmuxChannelStatus->CFR = hdma->DMAmuxChannelStatusMask;

    /* Update error code */
    hdma->ErrorCode |= HAL_DMA_ERROR_SYNC;

    if(hdma->XferErrorCallback != NULL)
    {
      /* Transfer error callback */
      hdma->XferErrorCallback(hdma);
    }
  }

  if(hdma->DMAmuxRequestGen != 0)
  {
   /* if using a DMAMUX request generator block Check for DMAMUX request generator overrun */
    if((hdma->DMAmuxRequestGenStatus->RGSR & hdma->DMAmuxRequestGenStatusMask) != 0U)
    {
      /* Disable the request gen overrun interrupt */
      hdma->DMAmuxRequestGen->RGCR &= ~DMAMUX_RGxCR_OIE;

      /* Clear the DMAMUX request generator overrun flag */
      hdma->DMAmuxRequestGenStatus->RGCFR = hdma->DMAmuxRequestGenStatusMask;

      /* Update error code */
      hdma->ErrorCode |= HAL_DMA_ERROR_REQGEN;

      if(hdma->XferErrorCallback != NULL)
      {
        /* Transfer error callback */
        hdma->XferErrorCallback(hdma);
      }
    }
  }
}

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_DMA_MODULE_ENABLED */

/**
  * @}
  */

/**
  * @}
  */

#endif /* DMAMUX1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
