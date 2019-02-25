/**
  ******************************************************************************
  * @file    stm32wbxx_hal_ipcc.c
  * @author  MCD Application Team
  * @brief   IPCC HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the Inter-Processor communication controller
  *          peripherals (IPCC).
  *           + Initialization and de-initialization functions
  *           + Configuration, notification and interrupts handling
  *           + Peripheral State and Error functions
  @verbatim
  ==============================================================================
                        ##### How to use this driver #####
  ==============================================================================
    [..]
      The IPCC HAL driver can be used as follows:

      (#) Declare a IPCC_HandleTypeDef handle structure, for example: IPCC_HandleTypeDef hipcc;
      (#) Initialize the IPCC low level resources by implementing the HAL_IPCC_MspInit() API:
        (##) Enable the IPCC interface clock
        (##) NVIC configuration if you need to use interrupt process
            (+++) Configure the IPCC interrupt priority
            (+++) Enable the NVIC IPCC IRQ

      (#) Initialize the IPCC registers by calling the HAL_IPCC_Init() API which trig
          HAL_IPCC_MspInit().

      (#) Implement the interrupt callbacks for transmission and reception to use the driver in interrupt mode

      (#) Associate those callback to the corresponding channel and direction using HAL_IPCC_ConfigChannel().
          This is the interrupt mode.
          If no callback are configured for a given channel and direction, it is up to the user to poll the
          status of the communication (polling mode).

      (#) Notify the other MCU when a message is available in a chosen channel
          or when a message has been retrieved from a chosen channel by calling
          the HAL_IPCC_NotifyCPU() API.

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
#include "stm32wbxx_hal.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @addtogroup IPCC
  * @{
  */

#ifdef HAL_IPCC_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup IPCC_Private_Constants IPCC Private Constants
  * @{
  */
#define IPCC_ALL_RX_BUF 0x0000003FU /*!< Mask for all RX buffers. */
#define IPCC_ALL_TX_BUF 0x003F0000U /*!< Mask for all TX buffers. */
#define CHANNEL_INDEX_Msk 0x0000000FU /*!< Mask the channel index to avoid overflow */
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @defgroup IPCC_Private_Functions IPCC Private Functions
  * @{
  */
void IPCC_MaskInterrupt(uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir);
void IPCC_UnmaskInterrupt(uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir);
void IPCC_SetDefaultCallbacks(IPCC_HandleTypeDef *hipcc);
void IPCC_Reset_Register(IPCC_CommonTypeDef *Instance);
/**
  * @}
  */

/** @addtogroup IPCC_Exported_Functions
  * @{
  */

/** @addtogroup IPCC_Exported_Functions_Group1
 *  @brief    Initialization and de-initialization functions
 *
@verbatim
 ===============================================================================
             ##### Initialization and de-initialization functions  #####
 ===============================================================================
    [..]  This subsection provides a set of functions allowing to initialize and
          deinitialize the IPCC peripheral:

      (+) User must Implement HAL_IPCC_MspInit() function in which he configures
          all related peripherals resources (CLOCK and NVIC ).

      (+) Call the function HAL_IPCC_Init() to configure the IPCC register.

      (+) Call the function HAL_PKA_DeInit() to restore the default configuration
          of the selected IPCC peripheral.

@endverbatim
  * @{
  */

/**
  * @brief  Initialize the IPCC peripheral.
  * @param  hipcc IPCC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IPCC_Init(IPCC_HandleTypeDef *hipcc)
{
  HAL_StatusTypeDef err = HAL_OK;

  /* Check the IPCC handle allocation */
  if (hipcc != NULL)
  {
    /* Check the parameters */
    assert_param(IS_IPCC_ALL_INSTANCE(hipcc->Instance));

    IPCC_CommonTypeDef *currentInstance = IPCC_C1;

    if (hipcc->State == HAL_IPCC_STATE_RESET)
    {
      /* Init the low level hardware : CLOCK, NVIC */
      HAL_IPCC_MspInit(hipcc);
    }

    /* Reset all registers of the current cpu to default state */
    IPCC_Reset_Register(currentInstance);

    /* Activate the interrupts */
    currentInstance->CR |= (IPCC_CR_RXOIE | IPCC_CR_TXFIE);

    /* Clear callback pointers */
    IPCC_SetDefaultCallbacks(hipcc);

    /* Reset all callback notification request */
    hipcc->callbackRequest = 0;

    hipcc->State = HAL_IPCC_STATE_READY;
  }
  else
  {
    err = HAL_ERROR;
  }

  return err;
}

/**
  * @brief  DeInitialize the IPCC peripheral.
  * @param  hipcc IPCC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IPCC_DeInit(IPCC_HandleTypeDef *hipcc)
{
  HAL_StatusTypeDef err = HAL_OK;

  /* Check the IPCC handle allocation */
  if (hipcc != NULL)
  {
    assert_param(IS_IPCC_ALL_INSTANCE(hipcc->Instance));
    IPCC_CommonTypeDef *currentInstance = IPCC_C1;

    /* Set the state to busy */
    hipcc->State = HAL_IPCC_STATE_BUSY;

    /* Reset all registers of the current cpu to default state */
    IPCC_Reset_Register(currentInstance);

    /* Clear callback pointers */
    IPCC_SetDefaultCallbacks(hipcc);

    /* Reset all callback notification request */
    hipcc->callbackRequest = 0;

    /* DeInit the low level hardware : CLOCK, NVIC */
    HAL_IPCC_MspDeInit(hipcc);

    hipcc->State = HAL_IPCC_STATE_RESET;
  }
  else
  {
    err = HAL_ERROR;
  }

  return err;
}

/**
  * @brief Initialize the IPCC MSP.
  * @param  hipcc IPCC handle
  * @retval None
  */
__weak void HAL_IPCC_MspInit(IPCC_HandleTypeDef *hipcc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hipcc);

  /* NOTE : This function should not be modified. When the callback is needed
            the HAL_IPCC_MspInit should be implemented in the user file
   */
}

/**
  * @brief IPCC MSP DeInit
  * @param  hipcc IPCC handle
  * @retval None
  */
__weak void HAL_IPCC_MspDeInit(IPCC_HandleTypeDef *hipcc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hipcc);

  /* NOTE : This function should not be modified. When the callback is needed
            the HAL_IPCC_MspDeInit should be implemented in the user file
   */
}

/**
  * @}
  */


/** @addtogroup IPCC_Exported_Functions_Group2
 *  @brief    Configuration, notification and Irq handling functions.
 *
@verbatim
 ===============================================================================
              ##### IO operation functions #####
 ===============================================================================
    [..]  This section provides functions to allow two MCU to communicate.

    (#) For a given channel (from 0 to IPCC_CHANNEL_NUMBER), for a given direction
        IPCC_CHANNEL_DIR_TX or IPCC_CHANNEL_DIR_RX, you can choose to communicate
        in polling mode or in interrupt mode using IPCC.
        By default, the IPCC HAL driver handle the communication in polling mode.
        By setting a callback for a channel/direction, this communication use
        the interrupt mode.

    (#) Polling mode:
       (++) To transmit information, use HAL_IPCC_NotifyCPU() with
            IPCC_CHANNEL_DIR_TX. To know when the other processor has handled
            the notification, poll the communication using HAL_IPCC_NotifyCPU
            with IPCC_CHANNEL_DIR_TX.

       (++) To receive information, poll the status of the communication with
            HAL_IPCC_GetChannelStatus with IPCC_CHANNEL_DIR_RX. To notify the other
            processor that the information has been received, use HAL_IPCC_NotifyCPU
            with IPCC_CHANNEL_DIR_RX.

    (#) Interrupt mode:
       (++) Configure a callback for the channel and the direction using HAL_IPCC_ConfigChannel().
            This callback will be triggered under interrupt.

       (++) To transmit information, use HAL_IPCC_NotifyCPU() with
            IPCC_CHANNEL_DIR_TX. The callback configured with HAL_IPCC_ConfigChannel() and
            IPCC_CHANNEL_DIR_TX will be triggered once the communication has been handled by the
            other processor.

       (++) To receive information, the callback configured with HAL_IPCC_ConfigChannel() and
            IPCC_CHANNEL_DIR_RX will be triggered on reception of a communication.To notify the other
            processor that the information has been received, use HAL_IPCC_NotifyCPU
            with IPCC_CHANNEL_DIR_RX.

       (++) HAL_IPCC_TX_IRQHandler must be added to the IPCC TX IRQHandler

       (++) HAL_IPCC_RX_IRQHandler must be added to the IPCC RX IRQHandler
@endverbatim
  * @{
  */

/**
  * @brief  Activate the callback notification on receive/transmit interrupt
  * @param  hipcc IPCC handle
  * @param  ChannelIndex Channel number
  *          This parameter can be one of the following values:
  *            @arg IPCC_CHANNEL_1: IPCC Channel 1
  *            @arg IPCC_CHANNEL_2: IPCC Channel 2
  *            @arg IPCC_CHANNEL_3: IPCC Channel 3
  *            @arg IPCC_CHANNEL_4: IPCC Channel 4
  *            @arg IPCC_CHANNEL_5: IPCC Channel 5
  *            @arg IPCC_CHANNEL_6: IPCC Channel 6
  * @param  ChannelDir Channel direction
  * @param  cb Interrupt callback
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IPCC_ActivateNotification(IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir, ChannelCb cb)
{
  HAL_StatusTypeDef err = HAL_OK;

  /* Check the IPCC handle allocation */
  if (hipcc != NULL)
  {
    /* Check the parameters */
    assert_param(IS_IPCC_ALL_INSTANCE(hipcc->Instance));

    /* Check IPCC state */
    if (hipcc->State == HAL_IPCC_STATE_READY)
    {
      /* Set callback and register masking information */
      if (ChannelDir == IPCC_CHANNEL_DIR_TX)
      {
        hipcc->ChannelCallbackTx[ChannelIndex] = cb;
        hipcc->callbackRequest |= (IPCC_MR_CH1FM_Msk << (ChannelIndex & CHANNEL_INDEX_Msk));
      }
      else
      {
        hipcc->ChannelCallbackRx[ChannelIndex] = cb;
        hipcc->callbackRequest |= (IPCC_MR_CH1OM_Msk << (ChannelIndex & CHANNEL_INDEX_Msk));
      }

      /* Unmask only the channels in reception (Transmission channel mask/unmask is done in HAL_IPCC_NotifyCPU) */
      if (ChannelDir == IPCC_CHANNEL_DIR_RX)
      {
        IPCC_UnmaskInterrupt(ChannelIndex, ChannelDir);
      }
    }
    else
    {
      err = HAL_ERROR;
    }
  }
  else
  {
    err = HAL_ERROR;
  }
  return err;
}

/**
  * @brief  Remove the callback notification on receive/transmit interrupt
  * @param  hipcc IPCC handle
  * @param  ChannelIndex Channel number
  *          This parameter can be one of the following values:
  *            @arg IPCC_CHANNEL_1: IPCC Channel 1
  *            @arg IPCC_CHANNEL_2: IPCC Channel 2
  *            @arg IPCC_CHANNEL_3: IPCC Channel 3
  *            @arg IPCC_CHANNEL_4: IPCC Channel 4
  *            @arg IPCC_CHANNEL_5: IPCC Channel 5
  *            @arg IPCC_CHANNEL_6: IPCC Channel 6
  * @param  ChannelDir Channel direction
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IPCC_DeActivateNotification(IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir)
{
  HAL_StatusTypeDef err = HAL_OK;

  /* Check the IPCC handle allocation */
  if (hipcc != NULL)
  {
    /* Check the parameters */
    assert_param(IS_IPCC_ALL_INSTANCE(hipcc->Instance));

    /* Check IPCC state */
    if (hipcc->State == HAL_IPCC_STATE_READY)
    {
      /* Set default callback and register masking information */
      if (ChannelDir == IPCC_CHANNEL_DIR_TX)
      {
        hipcc->ChannelCallbackTx[ChannelIndex] = HAL_IPCC_TxCallback;
        hipcc->callbackRequest &= ~(IPCC_MR_CH1FM_Msk << (ChannelIndex & CHANNEL_INDEX_Msk));
      }
      else
      {
        hipcc->ChannelCallbackRx[ChannelIndex] = HAL_IPCC_RxCallback;
        hipcc->callbackRequest &= ~(IPCC_MR_CH1OM_Msk << (ChannelIndex & CHANNEL_INDEX_Msk));
      }

      /* Mask the interrupt */
      IPCC_MaskInterrupt(ChannelIndex, ChannelDir);
    }
    else
    {
      err = HAL_ERROR;
    }
  }
  else
  {
    err = HAL_ERROR;
  }
  return err;
}

/**
  * @brief  Get state of IPCC channel
  * @param  hipcc IPCC handle
  * @param  ChannelIndex Channel number
  *          This parameter can be one of the following values:
  *            @arg IPCC_CHANNEL_1: IPCC Channel 1
  *            @arg IPCC_CHANNEL_2: IPCC Channel 2
  *            @arg IPCC_CHANNEL_3: IPCC Channel 3
  *            @arg IPCC_CHANNEL_4: IPCC Channel 4
  *            @arg IPCC_CHANNEL_5: IPCC Channel 5
  *            @arg IPCC_CHANNEL_6: IPCC Channel 6
  * @param  ChannelDir Channel direction
  * @retval Channel status
  */
IPCC_CHANNELStatusTypeDef HAL_IPCC_GetChannelStatus(IPCC_HandleTypeDef const *const hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir)
{
  uint32_t channel_state;
  IPCC_CommonTypeDef *currentInstance = IPCC_C1;
  IPCC_CommonTypeDef *otherInstance = IPCC_C2;

  /* Check the parameters */
  assert_param(IS_IPCC_ALL_INSTANCE(hipcc->Instance));

  /* Read corresponding channel depending of the MCU and the direction */
  if (ChannelDir == IPCC_CHANNEL_DIR_TX)
  {
    channel_state = (currentInstance->SR) & (IPCC_SR_CH1F_Msk << (ChannelIndex & CHANNEL_INDEX_Msk));
  }
  else
  {
    channel_state = (otherInstance->SR) & (IPCC_SR_CH1F_Msk << (ChannelIndex & CHANNEL_INDEX_Msk));
  }

  return (channel_state == 0UL) ? IPCC_CHANNEL_STATUS_FREE : IPCC_CHANNEL_STATUS_OCCUPIED ;
}

/**
  * @brief  Notify remote processor
  * @param  hipcc IPCC handle
  * @param  ChannelIndex Channel number
  *          This parameter can be one of the following values:
  *            @arg IPCC_CHANNEL_1: IPCC Channel 1
  *            @arg IPCC_CHANNEL_2: IPCC Channel 2
  *            @arg IPCC_CHANNEL_3: IPCC Channel 3
  *            @arg IPCC_CHANNEL_4: IPCC Channel 4
  *            @arg IPCC_CHANNEL_5: IPCC Channel 5
  *            @arg IPCC_CHANNEL_6: IPCC Channel 6
  * @param  ChannelDir Channel direction
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IPCC_NotifyCPU(IPCC_HandleTypeDef const *const hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir)
{
  HAL_StatusTypeDef err = HAL_OK;
  uint32_t mask;
  IPCC_CommonTypeDef *currentInstance = IPCC_C1;

  /* Check the parameters */
  assert_param(IS_IPCC_ALL_INSTANCE(hipcc->Instance));

  /* Check if IPCC is initiliased */
  if (hipcc->State == HAL_IPCC_STATE_READY)
  {
    /* For IPCC_CHANNEL_DIR_TX, set the status. For IPCC_CHANNEL_DIR_RX, clear the status */
    currentInstance->SCR |= ((ChannelDir == IPCC_CHANNEL_DIR_TX) ? IPCC_SCR_CH1S : IPCC_SCR_CH1C) << (ChannelIndex & CHANNEL_INDEX_Msk) ;

    /* Unmask interrupt if the callback is requested */
    mask = ((ChannelDir == IPCC_CHANNEL_DIR_TX) ? IPCC_MR_CH1FM_Msk : IPCC_MR_CH1OM_Msk) << (ChannelIndex & CHANNEL_INDEX_Msk) ;
    if ((hipcc->callbackRequest & mask) == mask)
    {
      IPCC_UnmaskInterrupt(ChannelIndex, ChannelDir);
    }
  }
  else
  {
    err = HAL_ERROR;
  }

  return err;
}

/**
  * @}
  */

/** @addtogroup IPCC_IRQ_Handler_and_Callbacks
 * @{
 */

/**
  * @brief  This function handles IPCC Tx Free interrupt request.
  * @param  hipcc IPCC handle
  * @retval None
  */
void HAL_IPCC_TX_IRQHandler(IPCC_HandleTypeDef *const hipcc)
{
  uint32_t irqmask;
  uint32_t bit_pos;
  uint32_t ch_count = 0U;
  IPCC_CommonTypeDef *currentInstance = IPCC_C1;

  /* check the Tx free channels which are not masked */
  irqmask = ~(currentInstance->MR) & IPCC_ALL_TX_BUF;
  irqmask = irqmask & ~(currentInstance->SR << IPCC_MR_CH1FM_Pos);

  while (irqmask != 0UL)  /* if several bits are set, it loops to serve all of them */
  {
    bit_pos = 1UL << (IPCC_MR_CH1FM_Pos + (ch_count & CHANNEL_INDEX_Msk));

    if ((irqmask & bit_pos) != 0U)
    {
      /* mask the channel Free interrupt  */
      currentInstance->MR |= bit_pos;
      if (hipcc->ChannelCallbackTx[ch_count] != NULL)
      {
        hipcc->ChannelCallbackTx[ch_count](hipcc, ch_count, IPCC_CHANNEL_DIR_TX);
      }
      irqmask =  irqmask & ~(bit_pos);
    }
    ch_count++;
  }
}

/**
  * @brief  This function handles IPCC Rx Occupied interrupt request.
  * @param  hipcc : IPCC handle
  * @retval None
  */
void HAL_IPCC_RX_IRQHandler(IPCC_HandleTypeDef *const hipcc)
{
  uint32_t irqmask;
  uint32_t bit_pos;
  uint32_t ch_count = 0U;
  IPCC_CommonTypeDef *currentInstance = IPCC_C1;
  IPCC_CommonTypeDef *otherInstance = IPCC_C2;

  /* check the Rx occupied channels which are not masked */
  irqmask = ~(currentInstance->MR) & IPCC_ALL_RX_BUF;
  irqmask = irqmask & otherInstance->SR;

  while (irqmask != 0UL)  /* if several bits are set, it loops to serve all of them */
  {
    bit_pos = 1UL << (ch_count & CHANNEL_INDEX_Msk);

    if ((irqmask & bit_pos) != 0U)
    {
      /* mask the channel occupied interrupt */
      currentInstance->MR |= bit_pos;
      if (hipcc->ChannelCallbackRx[ch_count] != NULL)
      {
        hipcc->ChannelCallbackRx[ch_count](hipcc, ch_count, IPCC_CHANNEL_DIR_RX);
      }
      irqmask = irqmask & ~(bit_pos);
    }
    ch_count++;
  }
}

/**
  * @brief Rx occupied callback
  * @param hipcc IPCC handle
  * @param ChannelIndex Channel number
  *          This parameter can be one of the following values:
  *            @arg IPCC_CHANNEL_1: IPCC Channel 1
  *            @arg IPCC_CHANNEL_2: IPCC Channel 2
  *            @arg IPCC_CHANNEL_3: IPCC Channel 3
  *            @arg IPCC_CHANNEL_4: IPCC Channel 4
  *            @arg IPCC_CHANNEL_5: IPCC Channel 5
  *            @arg IPCC_CHANNEL_6: IPCC Channel 6
  * @param ChannelDir Channel direction
  */
__weak void HAL_IPCC_RxCallback(IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hipcc);
  UNUSED(ChannelIndex);
  UNUSED(ChannelDir);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_IPCC_RxCallback can be implemented in the user file
   */
}

/**
  * @brief Tx free callback
  * @param hipcc IPCC handle
  * @param ChannelIndex Channel number
  *          This parameter can be one of the following values:
  *            @arg IPCC_CHANNEL_1: IPCC Channel 1
  *            @arg IPCC_CHANNEL_2: IPCC Channel 2
  *            @arg IPCC_CHANNEL_3: IPCC Channel 3
  *            @arg IPCC_CHANNEL_4: IPCC Channel 4
  *            @arg IPCC_CHANNEL_5: IPCC Channel 5
  *            @arg IPCC_CHANNEL_6: IPCC Channel 6
  * @param ChannelDir Channel direction
  */
__weak void HAL_IPCC_TxCallback(IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hipcc);
  UNUSED(ChannelIndex);
  UNUSED(ChannelDir);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_IPCC_TxCallback can be implemented in the user file
   */
}

/**
  * @}
  */

/** @addtogroup IPCC_Exported_Functions_Group3
 *  @brief   IPCC Peripheral State and Error functions
 *
@verbatim
  ==============================================================================
            ##### Peripheral State and Error functions #####
  ==============================================================================
    [..]
    This subsection permit to get in run-time the status of the peripheral
    and the data flow.

@endverbatim
  * @{
  */

/**
  * @brief Return the IPCC handle state.
  * @param  hipcc IPCC handle
  * @retval IPCC handle state
  */
HAL_IPCC_StateTypeDef HAL_IPCC_GetState(IPCC_HandleTypeDef const *const hipcc)
{
  return hipcc->State;
}

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup IPCC_Private_Functions
  * @{
  */

/**
  * @brief  Mask IPCC interrupts.
  * @param  ChannelIndex Channel number
  *          This parameter can be one of the following values:
  *            @arg IPCC_CHANNEL_1: IPCC Channel 1
  *            @arg IPCC_CHANNEL_2: IPCC Channel 2
  *            @arg IPCC_CHANNEL_3: IPCC Channel 3
  *            @arg IPCC_CHANNEL_4: IPCC Channel 4
  *            @arg IPCC_CHANNEL_5: IPCC Channel 5
  *            @arg IPCC_CHANNEL_6: IPCC Channel 6
  * @param  ChannelDir Channel direction
  */
void IPCC_MaskInterrupt(uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir)
{
  IPCC_CommonTypeDef *currentInstance = IPCC_C1;
  if (ChannelDir == IPCC_CHANNEL_DIR_TX)
  {
    /* Mask interrupt */
    currentInstance->MR |= (IPCC_MR_CH1FM_Msk << (ChannelIndex & CHANNEL_INDEX_Msk));
  }
  else
  {
    /* Mask interrupt */
    currentInstance->MR |= (IPCC_MR_CH1OM_Msk << (ChannelIndex & CHANNEL_INDEX_Msk));
  }
}
/**
  * @brief  Unmask IPCC interrupts.
  * @param  ChannelIndex Channel number
  *          This parameter can be one of the following values:
  *            @arg IPCC_CHANNEL_1: IPCC Channel 1
  *            @arg IPCC_CHANNEL_2: IPCC Channel 2
  *            @arg IPCC_CHANNEL_3: IPCC Channel 3
  *            @arg IPCC_CHANNEL_4: IPCC Channel 4
  *            @arg IPCC_CHANNEL_5: IPCC Channel 5
  *            @arg IPCC_CHANNEL_6: IPCC Channel 6
  * @param  ChannelDir Channel direction
  */
void IPCC_UnmaskInterrupt(uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir)
{
  IPCC_CommonTypeDef *currentInstance = IPCC_C1;
  if (ChannelDir == IPCC_CHANNEL_DIR_TX)
  {
    /* Unmask interrupt */
    currentInstance->MR &= ~(IPCC_MR_CH1FM_Msk << (ChannelIndex & CHANNEL_INDEX_Msk));
  }
  else
  {
    /* Unmask interrupt */
    currentInstance->MR &= ~(IPCC_MR_CH1OM_Msk << (ChannelIndex & CHANNEL_INDEX_Msk));
  }
}

/**
  * @brief Reset all callbacks of the handle to NULL.
  * @param  hipcc IPCC handle
  */
void IPCC_SetDefaultCallbacks(IPCC_HandleTypeDef *hipcc)
{
  uint32_t i;
  /* Set all callbacks to default */
  for (i = 0; i < IPCC_CHANNEL_NUMBER; i++)
  {
    hipcc->ChannelCallbackRx[i] = HAL_IPCC_RxCallback;
    hipcc->ChannelCallbackTx[i] = HAL_IPCC_TxCallback;
  }
}

/**
  * @brief Reset IPCC register to default value for the concerned instance.
  * @param  Instance pointer to register
  */
void IPCC_Reset_Register(IPCC_CommonTypeDef *Instance)
{
  /* Disable RX and TX interrupts */
  Instance->CR  = 0x00000000U;

  /* Mask RX and TX interrupts */
  Instance->MR  = (IPCC_ALL_TX_BUF | IPCC_ALL_RX_BUF);

  /* Clear RX status */
  Instance->SCR = IPCC_ALL_RX_BUF;
}

/**
  * @}
  */

#endif /* HAL_IPCC_MODULE_ENABLED */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
