/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_dma_ex.c
  * @author  MCD Application Team
  * @brief   DMA Extension HAL module driver
  *          This file provides firmware functions to manage the following
  *          functionalities of the DMA Extension peripheral:
  *           + Extended features functions
  *
  @verbatim
  ==============================================================================
                        ##### How to use this driver #####
  ==============================================================================
  [..]
  The DMA Extension HAL driver can be used as follows:
   (+) Start a multi buffer transfer using the HAL_DMA_MultiBufferStart() function
       for polling mode or HAL_DMA_MultiBufferStart_IT() for interrupt mode.

   (+) Configure the DMA_MUX Synchronization Block using HAL_DMAEx_ConfigMuxSync function.
   (+) Configure the DMA_MUX Request Generator Block using HAL_DMAEx_ConfigMuxRequestGenerator function.
       Functions HAL_DMAEx_EnableMuxRequestGenerator and HAL_DMAEx_DisableMuxRequestGenerator can then be used
       to respectively enable/disable the request generator.

   (+) To handle the DMAMUX Interrupts, the function  HAL_DMAEx_MUX_IRQHandler should be called from
       the DMAMUX IRQ handler i.e DMAMUX1_OVR_IRQHandler or DMAMUX2_OVR_IRQHandler .
       As only one interrupt line is available for all DMAMUX channels and request generators , HAL_DMA_MUX_IRQHandler should be
       called with, as parameter, the appropriate DMA handle as many as used DMAs in the user project
      (exception done if a given DMA is not using the DMAMUX SYNC block neither a request generator)

     -@-  In Memory-to-Memory transfer mode, Multi (Double) Buffer mode is not allowed.
     -@-  When Multi (Double) Buffer mode is enabled, the transfer is circular by default.
     -@-  In Multi (Double) buffer mode, it is possible to update the base address for
          the AHB memory port on the fly (DMA_SxM0AR or DMA_SxM1AR) when the stream is enabled.
     -@-  Multi (Double) buffer mode is only possible with D2 DMAs i.e DMA1 or DMA2. not BDMA.
          Multi (Double) buffer mode is not possible with D3 BDMA.

  @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @defgroup DMAEx DMAEx
  * @brief DMA Extended HAL module driver
  * @{
  */

#ifdef HAL_DMA_MODULE_ENABLED

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private Constants ---------------------------------------------------------*/
#define DMAMUX_POSITION_CxCR_SE      (uint32_t)POSITION_VAL(DMAMUX_CxCR_SE)    /*!< Required for left shift of the DMAMUX SYNC enable/disable       */
#define DMAMUX_POSITION_CxCR_EGE     (uint32_t)POSITION_VAL(DMAMUX_CxCR_EGE)   /*!< Required for left shift of the DMAMUX SYNC EVENT enable/disable */
/* Private macros ------------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/** @addtogroup DMAEx_Private_Functions
  * @{
  */

static void DMA_MultiBufferSetConfig(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);

/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/

/** @addtogroup DMAEx_Exported_Functions
  * @{
  */


/** @addtogroup DMAEx_Exported_Functions_Group1
  *
@verbatim
 ===============================================================================
                #####  Extended features functions  #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Configure the source, destination address and data length and
          Start MultiBuffer DMA transfer
      (+) Configure the source, destination address and data length and
          Start MultiBuffer DMA transfer with interrupt
      (+) Change on the fly the memory0 or memory1 address.
      (+) Configure the DMA_MUX Synchronization Block using HAL_DMAEx_ConfigMuxSync function.
      (+) Configure the DMA_MUX Request Generator Block using HAL_DMAEx_ConfigMuxRequestGenerator function.
      (+) Functions HAL_DMAEx_EnableMuxRequestGenerator and HAL_DMAEx_DisableMuxRequestGenerator can then be used
          to respectively enable/disable the request generator.
      (+) Handle DMAMUX interrupts using HAL_DMAEx_MUX_IRQHandler : should be called from
          the DMAMUX IRQ handler i.e DMAMUX1_OVR_IRQHandler or DMAMUX2_OVR_IRQHandler

@endverbatim
  * @{
  */


/**
  * @brief  Starts the multi_buffer DMA Transfer.
  * @param  hdma      : pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA Stream.
  * @param  SrcAddress: The source memory Buffer address
  * @param  DstAddress: The destination memory Buffer address
  * @param  SecondMemAddress: The second memory Buffer address in case of multi buffer Transfer
  * @param  DataLength: The length of data to be transferred from source to destination
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMAEx_MultiBufferStart(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t SecondMemAddress, uint32_t DataLength)
{
  HAL_StatusTypeDef status = HAL_OK;
  __IO uint32_t *ifcRegister_Base; /* DMA Stream Interrupt Clear register */

  /* Check the parameters */
  assert_param(IS_DMA_BUFFER_SIZE(DataLength));

  /* Memory-to-memory transfer not supported in double buffering mode */
  /* double buffering mode not supported for BDMA (D3 DMA)            */
  if ((IS_DMA_INSTANCE(hdma) == 0U) || (hdma->Init.Direction == DMA_MEMORY_TO_MEMORY))
  {
    hdma->ErrorCode = HAL_DMA_ERROR_NOT_SUPPORTED;
    status = HAL_ERROR;
  }
  else
  {
    /* Process Locked */
    __HAL_LOCK(hdma);

    if (HAL_DMA_STATE_READY == hdma->State)
    {
      /* Change DMA peripheral state */
      hdma->State = HAL_DMA_STATE_BUSY;

      /* Initialize the error code */
      hdma->ErrorCode = HAL_DMA_ERROR_NONE;

      /* Enable the double buffer mode */
      ((DMA_Stream_TypeDef *)hdma->Instance)->CR |= (uint32_t)DMA_SxCR_DBM;

      /* Configure DMA Stream destination address */
      ((DMA_Stream_TypeDef *)hdma->Instance)->M1AR = SecondMemAddress;

      /* Configure the source, destination address and the data length */
      DMA_MultiBufferSetConfig(hdma, SrcAddress, DstAddress, DataLength);

      /* Calculate the interrupt clear flag register (IFCR) base address  */
      ifcRegister_Base = (uint32_t *)((uint32_t)(hdma->StreamBaseAddress + 8U));

      /* Clear all flags */
      *ifcRegister_Base = 0x3FUL << hdma->StreamIndex;

      /* Clear the DMAMUX synchro overrun flag */
      hdma->DMAmuxChannelStatus->CFR = hdma->DMAmuxChannelStatusMask;

      if(hdma->DMAmuxRequestGen != NULL)
      {
        /* Clear the DMAMUX request generator overrun flag */
        hdma->DMAmuxRequestGenStatus->RGCFR = hdma->DMAmuxRequestGenStatusMask;
      }

      /* Enable the peripheral */
      __HAL_DMA_ENABLE(hdma);
    }
    else
    {
      /* Set the error code to busy */
      hdma->ErrorCode = HAL_DMA_ERROR_BUSY;

      /* Return error status */
      status = HAL_ERROR;
    }
  }
  return status;
}

/**
  * @brief  Starts the multi_buffer DMA Transfer with interrupt enabled.
  * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA Stream.
  * @param  SrcAddress: The source memory Buffer address
  * @param  DstAddress: The destination memory Buffer address
  * @param  SecondMemAddress: The second memory Buffer address in case of multi buffer Transfer
  * @param  DataLength: The length of data to be transferred from source to destination
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMAEx_MultiBufferStart_IT(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t SecondMemAddress, uint32_t DataLength)
{
  HAL_StatusTypeDef status = HAL_OK;
  __IO uint32_t *ifcRegister_Base; /* DMA Stream Interrupt Clear register */

  /* Check the parameters */
  assert_param(IS_DMA_BUFFER_SIZE(DataLength));

  /* Memory-to-memory transfer not supported in double buffering mode */
  if ((IS_DMA_INSTANCE(hdma) == 0U) || (hdma->Init.Direction == DMA_MEMORY_TO_MEMORY))
  {
    hdma->ErrorCode = HAL_DMA_ERROR_NOT_SUPPORTED;
    return HAL_ERROR;
  }

  /* Process locked */
  __HAL_LOCK(hdma);

  if (HAL_DMA_STATE_READY == hdma->State)
  {
    /* Change DMA peripheral state */
    hdma->State = HAL_DMA_STATE_BUSY;

    /* Initialize the error code */
    hdma->ErrorCode = HAL_DMA_ERROR_NONE;

    /* Enable the Double buffer mode */
    ((DMA_Stream_TypeDef *)hdma->Instance)->CR |= (uint32_t)DMA_SxCR_DBM;

    /* Configure DMA Stream destination address */
    ((DMA_Stream_TypeDef *)hdma->Instance)->M1AR = SecondMemAddress;

    /* Configure the source, destination address and the data length */
    DMA_MultiBufferSetConfig(hdma, SrcAddress, DstAddress, DataLength);

    /* Calculate the interrupt clear flag register (IFCR) base address  */
    ifcRegister_Base = (uint32_t *)((uint32_t)(hdma->StreamBaseAddress + 8U));

    /* Clear all flags */
    *ifcRegister_Base = 0x3FUL << hdma->StreamIndex;

    /* Clear the DMAMUX synchro overrun flag */
    hdma->DMAmuxChannelStatus->CFR = hdma->DMAmuxChannelStatusMask;

    if(hdma->DMAmuxRequestGen != NULL)
    {
      /* Clear the DMAMUX request generator overrun flag */
      hdma->DMAmuxRequestGenStatus->RGCFR = hdma->DMAmuxRequestGenStatusMask;
    }

    /* Enable Common interrupts*/
    MODIFY_REG(((DMA_Stream_TypeDef *)hdma->Instance)->CR, (DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_HT), (DMA_IT_TC | DMA_IT_TE | DMA_IT_DME));
    ((DMA_Stream_TypeDef *)hdma->Instance)->FCR |= DMA_IT_FE;

    if (hdma->XferHalfCpltCallback != NULL)
    {
      /*Enable Half Transfer IT if corresponding Callback is set*/
      ((DMA_Stream_TypeDef *)hdma->Instance)->CR  |= DMA_IT_HT;
    }

    /* Check if DMAMUX Synchronization is enabled*/
    if ((hdma->DMAmuxChannel->CCR & DMAMUX_CxCR_SE) != 0U)
    {
      /* Enable DMAMUX sync overrun IT*/
      hdma->DMAmuxChannel->CCR |= DMAMUX_CxCR_SOIE;
    }

    if(hdma->DMAmuxRequestGen != NULL)
    {
      /* if using DMAMUX request generator, enable the DMAMUX request generator overrun IT*/
      /* enable the request gen overrun IT*/
      hdma->DMAmuxRequestGen->RGCR |= DMAMUX_RGxCR_OIE;
    }

    /* Enable the peripheral */
    __HAL_DMA_ENABLE(hdma);
  }
  else
  {
    /* Set the error code to busy */
    hdma->ErrorCode = HAL_DMA_ERROR_BUSY;

    /* Return error status */
    status = HAL_ERROR;
  }
  return status;
}

/**
  * @brief  Change the memory0 or memory1 address on the fly.
  * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA Stream.
  * @param  Address:    The new address
  * @param  memory:     the memory to be changed, This parameter can be one of
  *                     the following values:
  *                      MEMORY0 /
  *                      MEMORY1
  * @note   The MEMORY0 address can be changed only when the current transfer use
  *         MEMORY1 and the MEMORY1 address can be changed only when the current
  *         transfer use MEMORY0.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMAEx_ChangeMemory(DMA_HandleTypeDef *hdma, uint32_t Address, HAL_DMA_MemoryTypeDef memory)
{
  if (memory == MEMORY0)
  {
    /* change the memory0 address */
    ((DMA_Stream_TypeDef *)hdma->Instance)->M0AR = Address;
  }
  else
  {
    /* change the memory1 address */
    ((DMA_Stream_TypeDef *)hdma->Instance)->M1AR = Address;
  }

  return HAL_OK;
}

/**
  * @brief  Configure the DMAMUX synchronization parameters for a given DMA stream (instance).
  * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA Stream.
  * @param  pSyncConfig : pointer to HAL_DMA_MuxSyncConfigTypeDef : contains the DMAMUX synchronization parameters
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMAEx_ConfigMuxSync(DMA_HandleTypeDef *hdma, HAL_DMA_MuxSyncConfigTypeDef *pSyncConfig)
{
  uint32_t syncSignalID = 0;
  uint32_t syncPolarity = 0;

  /* Check the parameters */
  assert_param(IS_DMA_STREAM_ALL_INSTANCE(hdma->Instance));
  assert_param(IS_DMAMUX_SYNC_STATE(pSyncConfig->SyncEnable));
  assert_param(IS_DMAMUX_SYNC_EVENT(pSyncConfig->EventEnable));
  assert_param(IS_DMAMUX_SYNC_REQUEST_NUMBER(pSyncConfig->RequestNumber));

  if (pSyncConfig->SyncEnable == ENABLE)
  {
    assert_param(IS_DMAMUX_SYNC_POLARITY(pSyncConfig->SyncPolarity));
    assert_param(IS_DMAMUX_SYNC_SIGNAL_ID(pSyncConfig->SyncSignalID));
    syncSignalID = pSyncConfig->SyncSignalID;
    syncPolarity = pSyncConfig->SyncPolarity;
  }

  /*Check if the DMA state is ready */
  if (hdma->State == HAL_DMA_STATE_READY)
  {
    /* Process Locked */
    __HAL_LOCK(hdma);

    /* Disable the synchronization and event generation before applying a new config */
    CLEAR_BIT(hdma->DMAmuxChannel->CCR, (DMAMUX_CxCR_SE | DMAMUX_CxCR_EGE));

    /* Set the new synchronization parameters (and keep the request ID filled during the Init)*/
    MODIFY_REG(hdma->DMAmuxChannel->CCR, \
               (~DMAMUX_CxCR_DMAREQ_ID), \
               (syncSignalID << POSITION_VAL(DMAMUX_CxCR_SYNC_ID))       | \
               ((pSyncConfig->RequestNumber - 1U) << POSITION_VAL(DMAMUX_CxCR_NBREQ)) | \
               syncPolarity | ((uint32_t)pSyncConfig->SyncEnable << DMAMUX_POSITION_CxCR_SE)    | \
               ((uint32_t)pSyncConfig->EventEnable << DMAMUX_POSITION_CxCR_EGE));

    /* Process Locked */
    __HAL_UNLOCK(hdma);

    return HAL_OK;
  }
  else
  {
    /* Set the error code to busy */
    hdma->ErrorCode = HAL_DMA_ERROR_BUSY;

    /* Return error status */
    return HAL_ERROR;
  }
}

/**
  * @brief  Configure the DMAMUX request generator block used by the given DMA stream (instance).
  * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA Stream.
  * @param  pRequestGeneratorConfig : pointer to HAL_DMA_MuxRequestGeneratorConfigTypeDef :
  *         contains the request generator parameters.
  *
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMAEx_ConfigMuxRequestGenerator(DMA_HandleTypeDef *hdma, HAL_DMA_MuxRequestGeneratorConfigTypeDef *pRequestGeneratorConfig)
{
  HAL_StatusTypeDef status;

  /* Check the parameters */
  assert_param(IS_DMA_STREAM_ALL_INSTANCE(hdma->Instance));

  assert_param(IS_DMAMUX_REQUEST_GEN_SIGNAL_ID(pRequestGeneratorConfig->SignalID));

  assert_param(IS_DMAMUX_REQUEST_GEN_POLARITY(pRequestGeneratorConfig->Polarity));
  assert_param(IS_DMAMUX_REQUEST_GEN_REQUEST_NUMBER(pRequestGeneratorConfig->RequestNumber));

  /* check if the DMA state is ready
     and DMA is using a DMAMUX request generator block
  */
  if(hdma->DMAmuxRequestGen == NULL)
  {
    /* Set the error code to busy */
    hdma->ErrorCode = HAL_DMA_ERROR_PARAM;

    /* error status */
    status = HAL_ERROR;
  }
  else if ((hdma->State == HAL_DMA_STATE_READY) && ((hdma->DMAmuxRequestGen->RGCR & DMAMUX_RGxCR_GE) == 0U))
  {
    /* RequestGenerator must be disable prior to the configuration i.e GE bit is 0 */

    /* Process Locked */
    __HAL_LOCK(hdma);

    /* Set the request generator new parameters*/
    hdma->DMAmuxRequestGen->RGCR = pRequestGeneratorConfig->SignalID | \
                                   ((pRequestGeneratorConfig->RequestNumber - 1U) << POSITION_VAL(DMAMUX_RGxCR_GNBREQ)) | \
                                   pRequestGeneratorConfig->Polarity;
    /* Process Locked */
    __HAL_UNLOCK(hdma);

    return HAL_OK;
  }
  else
  {
    /* Set the error code to busy */
    hdma->ErrorCode = HAL_DMA_ERROR_BUSY;

    /* error status */
    status = HAL_ERROR;
  }

  return status;
}

/**
  * @brief  Enable the DMAMUX request generator block used by the given DMA stream (instance).
  * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA Stream.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMAEx_EnableMuxRequestGenerator(DMA_HandleTypeDef *hdma)
{
  /* Check the parameters */
  assert_param(IS_DMA_STREAM_ALL_INSTANCE(hdma->Instance));

  /* check if the DMA state is ready
     and DMA is using a DMAMUX request generator block
  */
  if((hdma->State != HAL_DMA_STATE_RESET) && (hdma->DMAmuxRequestGen != NULL))
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
  * @brief  Disable the DMAMUX request generator block used by the given DMA stream (instance).
  * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA Stream.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMAEx_DisableMuxRequestGenerator(DMA_HandleTypeDef *hdma)
{
  /* Check the parameters */
  assert_param(IS_DMA_STREAM_ALL_INSTANCE(hdma->Instance));

  /* check if the DMA state is ready
     and DMA is using a DMAMUX request generator block
  */
  if((hdma->State != HAL_DMA_STATE_RESET) && (hdma->DMAmuxRequestGen != NULL))
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
  *               the configuration information for the specified DMA Stream.
  * @retval None
  */
void HAL_DMAEx_MUX_IRQHandler(DMA_HandleTypeDef *hdma)
{
  /* Check for DMAMUX Synchronization overrun */
  if ((hdma->DMAmuxChannelStatus->CSR & hdma->DMAmuxChannelStatusMask) != 0U)
  {
    /* Disable the synchro overrun interrupt */
    hdma->DMAmuxChannel->CCR &= ~DMAMUX_CxCR_SOIE;

    /* Clear the DMAMUX synchro overrun flag */
    hdma->DMAmuxChannelStatus->CFR = hdma->DMAmuxChannelStatusMask;

    /* Update error code */
    hdma->ErrorCode |= HAL_DMA_ERROR_SYNC;

    if (hdma->XferErrorCallback != NULL)
    {
      /* Transfer error callback */
      hdma->XferErrorCallback(hdma);
    }
  }

  if(hdma->DMAmuxRequestGen != NULL)
  {
    /* if using a DMAMUX request generator block Check for DMAMUX request generator overrun */
    if ((hdma->DMAmuxRequestGenStatus->RGSR & hdma->DMAmuxRequestGenStatusMask) != 0U)
    {
      /* Disable the request gen overrun interrupt */
      hdma->DMAmuxRequestGen->RGCR &= ~DMAMUX_RGxCR_OIE;

      /* Clear the DMAMUX request generator overrun flag */
      hdma->DMAmuxRequestGenStatus->RGCFR = hdma->DMAmuxRequestGenStatusMask;

      /* Update error code */
      hdma->ErrorCode |= HAL_DMA_ERROR_REQGEN;

      if (hdma->XferErrorCallback != NULL)
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

/** @addtogroup DMAEx_Private_Functions
  * @{
  */

/**
  * @brief  Set the DMA Transfer parameter.
  * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
  *                     the configuration information for the specified DMA Stream.
  * @param  SrcAddress: The source memory Buffer address
  * @param  DstAddress: The destination memory Buffer address
  * @param  DataLength: The length of data to be transferred from source to destination
  * @retval HAL status
  */
static void DMA_MultiBufferSetConfig(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
  /* Configure DMA Stream data length */
  ((DMA_Stream_TypeDef *)hdma->Instance)->NDTR = DataLength;

  /* Peripheral to Memory */
  if ((hdma->Init.Direction) == DMA_MEMORY_TO_PERIPH)
  {
    /* Configure DMA Stream destination address */
    ((DMA_Stream_TypeDef *)hdma->Instance)->PAR = DstAddress;

    /* Configure DMA Stream source address */
    ((DMA_Stream_TypeDef *)hdma->Instance)->M0AR = SrcAddress;
  }
  /* Memory to Peripheral */
  else
  {
    /* Configure DMA Stream source address */
    ((DMA_Stream_TypeDef *)hdma->Instance)->PAR = SrcAddress;

    /* Configure DMA Stream destination address */
    ((DMA_Stream_TypeDef *)hdma->Instance)->M0AR = DstAddress;
  }
}

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
