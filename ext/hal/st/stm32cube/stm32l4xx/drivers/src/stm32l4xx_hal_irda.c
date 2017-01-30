/**
  ******************************************************************************
  * @file    stm32l4xx_hal_irda.c
  * @author  MCD Application Team
  * @version V1.6.0
  * @date    28-October-2016
  * @brief   IRDA HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the IrDA (Infrared Data Association) Peripheral
  *          (IRDA)
  *           + Initialization and de-initialization functions
  *           + IO operation functions
  *           + Peripheral State and Errors functions
  *           + Peripheral Control functions
  *
  @verbatim
  ==============================================================================
                        ##### How to use this driver #####
  ==============================================================================
  [..]
    The IRDA HAL driver can be used as follows:

    (#) Declare a IRDA_HandleTypeDef handle structure (eg. IRDA_HandleTypeDef hirda).
    (#) Initialize the IRDA low level resources by implementing the HAL_IRDA_MspInit() API
        in setting the associated USART or UART in IRDA mode:
        (++) Enable the USARTx/UARTx interface clock.
        (++) USARTx/UARTx pins configuration:
            (+++) Enable the clock for the USARTx/UARTx GPIOs.
            (+++) Configure these USARTx/UARTx pins (TX as alternate function pull-up, RX as alternate function Input).
        (++) NVIC configuration if you need to use interrupt process (HAL_IRDA_Transmit_IT()
             and HAL_IRDA_Receive_IT() APIs): 
            (+++) Configure the USARTx/UARTx interrupt priority.
            (+++) Enable the NVIC USARTx/UARTx IRQ handle.            
            (+++) The specific IRDA interrupts (Transmission complete interrupt,
                  RXNE interrupt and Error Interrupts) will be managed using the macros
                  __HAL_IRDA_ENABLE_IT() and __HAL_IRDA_DISABLE_IT() inside the transmit and receive process.
                
        (++) DMA Configuration if you need to use DMA process (HAL_IRDA_Transmit_DMA()
             and HAL_IRDA_Receive_DMA() APIs):
            (+++) Declare a DMA handle structure for the Tx/Rx channel.
            (+++) Enable the DMAx interface clock.
            (+++) Configure the declared DMA handle structure with the required Tx/Rx parameters.
            (+++) Configure the DMA Tx/Rx channel.
            (+++) Associate the initialized DMA handle to the IRDA DMA Tx/Rx handle.
            (+++) Configure the priority and enable the NVIC for the transfer complete interrupt on the DMA Tx/Rx channel.

    (#) Program the Baud Rate, Word Length and Parity and Mode(Receiver/Transmitter),
        the normal or low power mode and the clock prescaler in the hirda handle Init structure.

    (#) Initialize the IRDA registers by calling the HAL_IRDA_Init() API:
        (++) This API configures also the low level Hardware GPIO, CLOCK, CORTEX...etc)
             by calling the customized HAL_IRDA_MspInit() API.
        
         -@@- The specific IRDA interrupts (Transmission complete interrupt,
             RXNE interrupt and Error Interrupts) will be managed using the macros
             __HAL_IRDA_ENABLE_IT() and __HAL_IRDA_DISABLE_IT() inside the transmit and receive process.
     
    (#) Three operation modes are available within this driver :

     *** Polling mode IO operation ***
     =================================
     [..]
       (+) Send an amount of data in blocking mode using HAL_IRDA_Transmit()
       (+) Receive an amount of data in blocking mode using HAL_IRDA_Receive()

     *** Interrupt mode IO operation ***
     ===================================
     [..]
       (+) Send an amount of data in non-blocking mode using HAL_IRDA_Transmit_IT()
       (+) At transmission end of transfer HAL_IRDA_TxCpltCallback() is executed and user can
            add his own code by customization of function pointer HAL_IRDA_TxCpltCallback()
       (+) Receive an amount of data in non-blocking mode using HAL_IRDA_Receive_IT()
       (+) At reception end of transfer HAL_IRDA_RxCpltCallback() is executed and user can
            add his own code by customization of function pointer HAL_IRDA_RxCpltCallback()
       (+) In case of transfer Error, HAL_IRDA_ErrorCallback() function is executed and user can
            add his own code by customization of function pointer HAL_IRDA_ErrorCallback()

     *** DMA mode IO operation ***
     ==============================
     [..]
       (+) Send an amount of data in non-blocking mode (DMA) using HAL_IRDA_Transmit_DMA()
       (+) At transmission half of transfer HAL_IRDA_TxHalfCpltCallback() is executed and user can
            add his own code by customization of function pointer HAL_IRDA_TxHalfCpltCallback()
       (+) At transmission end of transfer HAL_IRDA_TxCpltCallback() is executed and user can
            add his own code by customization of function pointer HAL_IRDA_TxCpltCallback()
       (+) Receive an amount of data in non-blocking mode (DMA) using HAL_IRDA_Receive_DMA()
       (+) At reception half of transfer HAL_IRDA_RxHalfCpltCallback() is executed and user can
            add his own code by customization of function pointer HAL_IRDA_RxHalfCpltCallback()
       (+) At reception end of transfer HAL_IRDA_RxCpltCallback() is executed and user can
            add his own code by customization of function pointer HAL_IRDA_RxCpltCallback()
       (+) In case of transfer Error, HAL_IRDA_ErrorCallback() function is executed and user can
            add his own code by customization of function pointer HAL_IRDA_ErrorCallback()

     *** IRDA HAL driver macros list ***
     ====================================
     [..]
       Below the list of most used macros in IRDA HAL driver.

       (+) __HAL_IRDA_ENABLE: Enable the IRDA peripheral
       (+) __HAL_IRDA_DISABLE: Disable the IRDA peripheral
       (+) __HAL_IRDA_GET_FLAG : Check whether the specified IRDA flag is set or not
       (+) __HAL_IRDA_CLEAR_FLAG : Clear the specified IRDA pending flag
       (+) __HAL_IRDA_ENABLE_IT: Enable the specified IRDA interrupt
       (+) __HAL_IRDA_DISABLE_IT: Disable the specified IRDA interrupt
       (+) __HAL_IRDA_GET_IT_SOURCE: Check whether or not the specified IRDA interrupt is enabled

     [..]
       (@) You can refer to the IRDA HAL driver header file for more useful macros

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
#include "stm32l4xx_hal.h"

/** @addtogroup STM32L4xx_HAL_Driver
  * @{
  */

/** @defgroup IRDA IRDA
  * @brief HAL IRDA module driver
  * @{
  */

#ifdef HAL_IRDA_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup IRDA_Private_Constants IRDA Private Constants
  * @{
  */
#define IRDA_TEACK_REACK_TIMEOUT            1000                                   /*!< IRDA TX or RX enable acknowledge time-out value  */
#define IRDA_CR1_FIELDS  ((uint32_t)(USART_CR1_M | USART_CR1_PCE \
                                   | USART_CR1_PS | USART_CR1_TE | USART_CR1_RE))  /*!< UART or USART CR1 fields of parameters set by IRDA_SetConfig API */
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup IRDA_Private_Functions
  * @{
  */
static HAL_StatusTypeDef IRDA_SetConfig(IRDA_HandleTypeDef *hirda);
static HAL_StatusTypeDef IRDA_CheckIdleState(IRDA_HandleTypeDef *hirda);
static HAL_StatusTypeDef IRDA_WaitOnFlagUntilTimeout(IRDA_HandleTypeDef *hirda, uint32_t Flag, FlagStatus Status, uint32_t Tickstart, uint32_t Timeout);
static void IRDA_EndTxTransfer(IRDA_HandleTypeDef *hirda);
static void IRDA_EndRxTransfer(IRDA_HandleTypeDef *hirda);
static void IRDA_DMATransmitCplt(DMA_HandleTypeDef *hdma);
static void IRDA_DMATransmitHalfCplt(DMA_HandleTypeDef *hdma);
static void IRDA_DMAReceiveCplt(DMA_HandleTypeDef *hdma);
static void IRDA_DMAReceiveHalfCplt(DMA_HandleTypeDef *hdma);
static void IRDA_DMAError(DMA_HandleTypeDef *hdma);
static void IRDA_DMAAbortOnError(DMA_HandleTypeDef *hdma);
static void IRDA_DMATxAbortCallback(DMA_HandleTypeDef *hdma);
static void IRDA_DMARxAbortCallback(DMA_HandleTypeDef *hdma);
static void IRDA_DMATxOnlyAbortCallback(DMA_HandleTypeDef *hdma);
static void IRDA_DMARxOnlyAbortCallback(DMA_HandleTypeDef *hdma);
static HAL_StatusTypeDef IRDA_Transmit_IT(IRDA_HandleTypeDef *hirda);
static HAL_StatusTypeDef IRDA_EndTransmit_IT(IRDA_HandleTypeDef *hirda);
static HAL_StatusTypeDef IRDA_Receive_IT(IRDA_HandleTypeDef *hirda);
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @defgroup IRDA_Exported_Functions IRDA Exported Functions
  * @{
  */

/** @defgroup IRDA_Exported_Functions_Group1 Initialization and de-initialization functions
  *  @brief    Initialization and Configuration functions
  *
@verbatim
  ==============================================================================
              ##### Initialization and Configuration functions #####
  ==============================================================================
  [..]
  This subsection provides a set of functions allowing to initialize the USARTx
  in asynchronous IRDA mode.
  (+) For the asynchronous mode only these parameters can be configured:
      (++) Baud Rate
      (++) Word Length
      (++) Parity: If the parity is enabled, then the MSB bit of the data written
           in the data register is transmitted but is changed by the parity bit.
      (++) Power mode
      (++) Prescaler setting
      (++) Receiver/transmitter modes

  [..]
  The HAL_IRDA_Init() API follows the USART asynchronous configuration procedures
  (details for the procedures are available in reference manual).

@endverbatim

  Depending on the frame length defined by the M1 and M0 bits (7-bit, 
  8-bit or 9-bit), the possible IRDA frame formats are listed in the 
  following table.
  
    Table 1. IRDA frame format.
    +-----------------------------------------------------------------------+
    |  M1 bit |  M0 bit |  PCE bit  |             IRDA frame                |
    |---------|---------|-----------|---------------------------------------|
    |    0    |    0    |    0      |    | SB |    8 bit data   | STB |     |
    |---------|---------|-----------|---------------------------------------|
    |    0    |    0    |    1      |    | SB | 7 bit data | PB | STB |     |
    |---------|---------|-----------|---------------------------------------|
    |    0    |    1    |    0      |    | SB |    9 bit data   | STB |     |
    |---------|---------|-----------|---------------------------------------|
    |    0    |    1    |    1      |    | SB | 8 bit data | PB | STB |     |
    |---------|---------|-----------|---------------------------------------|
    |    1    |    0    |    0      |    | SB |    7 bit data   | STB |     |
    |---------|---------|-----------|---------------------------------------|
    |    1    |    0    |    1      |    | SB | 6 bit data | PB | STB |     |
    +-----------------------------------------------------------------------+

  * @{
  */

/**
  * @brief  Initialize the IRDA mode according to the specified
  *         parameters in the IRDA_InitTypeDef and initialize the associated handle.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IRDA_Init(IRDA_HandleTypeDef *hirda)
{
  /* Check the IRDA handle allocation */
  if(hirda == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the USART/UART associated to the IRDA handle */
  assert_param(IS_IRDA_INSTANCE(hirda->Instance));

  if(hirda->gState == HAL_IRDA_STATE_RESET)
  {
    /* Allocate lock resource and initialize it */
    hirda->Lock = HAL_UNLOCKED;

    /* Init the low level hardware : GPIO, CLOCK */
    HAL_IRDA_MspInit(hirda);
  }

  hirda->gState = HAL_IRDA_STATE_BUSY;

  /* Disable the Peripheral to update the configuration registers */
  __HAL_IRDA_DISABLE(hirda);

  /* Set the IRDA Communication parameters */
  if (IRDA_SetConfig(hirda) == HAL_ERROR)
  {
    return HAL_ERROR;
  }

  /* In IRDA mode, the following bits must be kept cleared:
  - LINEN, STOP and CLKEN bits in the USART_CR2 register,
  - SCEN and HDSEL bits in the USART_CR3 register.*/
  CLEAR_BIT(hirda->Instance->CR2, (USART_CR2_LINEN | USART_CR2_CLKEN | USART_CR2_STOP));
  CLEAR_BIT(hirda->Instance->CR3, (USART_CR3_SCEN | USART_CR3_HDSEL));

  /* set the UART/USART in IRDA mode */
  hirda->Instance->CR3 |= USART_CR3_IREN;

  /* Enable the Peripheral */
  __HAL_IRDA_ENABLE(hirda);

  /* TEACK and/or REACK to check before moving hirda->gState and hirda->RxState to Ready */
  return (IRDA_CheckIdleState(hirda));
}

/**
  * @brief  DeInitialize the IRDA peripheral.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IRDA_DeInit(IRDA_HandleTypeDef *hirda)
{
  /* Check the IRDA handle allocation */
  if(hirda == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the USART/UART associated to the IRDA handle */
  assert_param(IS_IRDA_INSTANCE(hirda->Instance));

  hirda->gState = HAL_IRDA_STATE_BUSY;

  /* DeInit the low level hardware */
  HAL_IRDA_MspDeInit(hirda);
  /* Disable the Peripheral */
  __HAL_IRDA_DISABLE(hirda);

  hirda->ErrorCode = HAL_IRDA_ERROR_NONE;
  hirda->gState    = HAL_IRDA_STATE_RESET;
  hirda->RxState   = HAL_IRDA_STATE_RESET;

  /* Process Unlock */
  __HAL_UNLOCK(hirda);

  return HAL_OK;
}

/**
  * @brief  Initialize the IRDA MSP.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval None
  */
__weak void HAL_IRDA_MspInit(IRDA_HandleTypeDef *hirda)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hirda);

  /* NOTE: This function should not be modified, when the callback is needed,
           the HAL_IRDA_MspInit can be implemented in the user file
   */
}

/**
  * @brief  DeInitialize the IRDA MSP.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval None
  */
__weak void HAL_IRDA_MspDeInit(IRDA_HandleTypeDef *hirda)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hirda);

  /* NOTE: This function should not be modified, when the callback is needed,
           the HAL_IRDA_MspDeInit can be implemented in the user file
   */
}

/**
  * @}
  */

/** @defgroup IRDA_Exported_Functions_Group2 IO operation functions
  *  @brief   IRDA Transmit and Receive functions
  *
@verbatim
 ===============================================================================
                      ##### IO operation functions #####
 ===============================================================================
  [..]
    This subsection provides a set of functions allowing to manage the IRDA data transfers.

  [..]
    IrDA is a half duplex communication protocol. If the Transmitter is busy, any data
    on the IrDA receive line will be ignored by the IrDA decoder and if the Receiver
    is busy, data on the TX from the USART to IrDA will not be encoded by IrDA.
    While receiving data, transmission should be avoided as the data to be transmitted
    could be corrupted.

    (#) There are two mode of transfer:
        (++) Blocking mode: the communication is performed in polling mode.
             The HAL status of all data processing is returned by the same function
             after finishing transfer.
        (++) Non-Blocking mode: the communication is performed using Interrupts
             or DMA, these API's return the HAL status.
             The end of the data processing will be indicated through the
             dedicated IRDA IRQ when using Interrupt mode or the DMA IRQ when
             using DMA mode.
             The HAL_IRDA_TxCpltCallback(), HAL_IRDA_RxCpltCallback() user callbacks
             will be executed respectively at the end of the Transmit or Receive process
             The HAL_IRDA_ErrorCallback() user callback will be executed when a communication error is detected

    (#) Blocking mode APIs are :
        (++) HAL_IRDA_Transmit()
        (++) HAL_IRDA_Receive()

    (#) Non Blocking mode APIs with Interrupt are :
        (++) HAL_IRDA_Transmit_IT()
        (++) HAL_IRDA_Receive_IT()
        (++) HAL_IRDA_IRQHandler()

    (#) Non Blocking mode functions with DMA are :
        (++) HAL_IRDA_Transmit_DMA()
        (++) HAL_IRDA_Receive_DMA()
        (++) HAL_IRDA_DMAPause()
        (++) HAL_IRDA_DMAResume()
        (++) HAL_IRDA_DMAStop()

    (#) A set of Transfer Complete Callbacks are provided in Non Blocking mode:
        (++) HAL_IRDA_TxHalfCpltCallback()
        (++) HAL_IRDA_TxCpltCallback()
        (++) HAL_IRDA_RxHalfCpltCallback()
        (++) HAL_IRDA_RxCpltCallback()
        (++) HAL_IRDA_ErrorCallback()

    (#) Non-Blocking mode transfers could be aborted using Abort API's :
        (+) HAL_IRDA_Abort()
        (+) HAL_IRDA_AbortTransmit()
        (+) HAL_IRDA_AbortReceive()
        (+) HAL_IRDA_Abort_IT()
        (+) HAL_IRDA_AbortTransmit_IT()
        (+) HAL_IRDA_AbortReceive_IT()

    (#) For Abort services based on interrupts (HAL_IRDA_Abortxxx_IT), a set of Abort Complete Callbacks are provided:
        (+) HAL_IRDA_AbortCpltCallback()
        (+) HAL_IRDA_AbortTransmitCpltCallback()
        (+) HAL_IRDA_AbortReceiveCpltCallback()

    (#) In Non-Blocking mode transfers, possible errors are split into 2 categories.
        Errors are handled as follows :
       (+) Error is considered as Recoverable and non blocking : Transfer could go till end, but error severity is 
           to be evaluated by user : this concerns Frame Error, Parity Error or Noise Error in Interrupt mode reception .
           Received character is then retrieved and stored in Rx buffer, Error code is set to allow user to identify error type,
           and HAL_IRDA_ErrorCallback() user callback is executed. Transfer is kept ongoing on IRDA side.
           If user wants to abort it, Abort services should be called by user.
       (+) Error is considered as Blocking : Transfer could not be completed properly and is aborted.
           This concerns Overrun Error In Interrupt mode reception and all errors in DMA mode.
           Error code is set to allow user to identify error type, and HAL_IRDA_ErrorCallback() user callback is executed.

@endverbatim
  * @{
  */

/**
  * @brief  Send an amount of data in blocking mode.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @param  pData Pointer to data buffer.
  * @param  Size Amount of data to be sent.
  * @param  Timeout Specify timeout value.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IRDA_Transmit(IRDA_HandleTypeDef *hirda, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  uint16_t* tmp;
  uint32_t tickstart = 0;

  /* Check that a Tx process is not already ongoing */
  if(hirda->gState == HAL_IRDA_STATE_READY)
  {
    if((pData == NULL) || (Size == 0))
    {
      return  HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hirda);

    hirda->ErrorCode = HAL_IRDA_ERROR_NONE;
    hirda->gState = HAL_IRDA_STATE_BUSY_TX;

    /* Init tickstart for timeout managment*/
    tickstart = HAL_GetTick();

    hirda->TxXferSize = Size;
    hirda->TxXferCount = Size;
    while(hirda->TxXferCount > 0)
    {
      hirda->TxXferCount--;

      if(IRDA_WaitOnFlagUntilTimeout(hirda, IRDA_FLAG_TXE, RESET, tickstart, Timeout) != HAL_OK)
      {
        return HAL_TIMEOUT;
      }
      if ((hirda->Init.WordLength == IRDA_WORDLENGTH_9B) && (hirda->Init.Parity == IRDA_PARITY_NONE))
      {
        tmp = (uint16_t*) pData;
        hirda->Instance->TDR = (*tmp & (uint16_t)0x01FF);
        pData += 2;
      }
      else
      {
        hirda->Instance->TDR = (*pData++ & (uint8_t)0xFF);
      }
    }

    if(IRDA_WaitOnFlagUntilTimeout(hirda, IRDA_FLAG_TC, RESET, tickstart, Timeout) != HAL_OK)
    {
      return HAL_TIMEOUT;
    }

    /* At end of Tx process, restore hirda->gState to Ready */
    hirda->gState = HAL_IRDA_STATE_READY;

    /* Process Unlocked */
    __HAL_UNLOCK(hirda);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief  Receive an amount of data in blocking mode.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @param  pData Pointer to data buffer.
  * @param  Size Amount of data to be received.
  * @param  Timeout Specify timeout value.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IRDA_Receive(IRDA_HandleTypeDef *hirda, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  uint16_t* tmp;
  uint16_t uhMask;
  uint32_t tickstart = 0;

  /* Check that a Rx process is not already ongoing */
  if(hirda->RxState == HAL_IRDA_STATE_READY)
  {
    if((pData == NULL) || (Size == 0))
    {
      return  HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hirda);

    hirda->ErrorCode = HAL_IRDA_ERROR_NONE;
    hirda->RxState = HAL_IRDA_STATE_BUSY_RX;

    /* Init tickstart for timeout managment*/
    tickstart = HAL_GetTick();

    hirda->RxXferSize = Size;
    hirda->RxXferCount = Size;

    /* Computation of the mask to apply to RDR register
       of the UART associated to the IRDA */
    IRDA_MASK_COMPUTATION(hirda);
    uhMask = hirda->Mask;

    /* Check data remaining to be received */
    while(hirda->RxXferCount > 0)
    {
      hirda->RxXferCount--;

      if(IRDA_WaitOnFlagUntilTimeout(hirda, IRDA_FLAG_RXNE, RESET, tickstart, Timeout) != HAL_OK)
      {
        return HAL_TIMEOUT;
      }
      if ((hirda->Init.WordLength == IRDA_WORDLENGTH_9B) && (hirda->Init.Parity == IRDA_PARITY_NONE))
      {
        tmp = (uint16_t*) pData ;
        *tmp = (uint16_t)(hirda->Instance->RDR & uhMask);
        pData +=2;
      }
      else
      {
        *pData++ = (uint8_t)(hirda->Instance->RDR & (uint8_t)uhMask);
      }
    }

    /* At end of Rx process, restore hirda->RxState to Ready */
    hirda->RxState = HAL_IRDA_STATE_READY;

    /* Process Unlocked */
    __HAL_UNLOCK(hirda);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief  Send an amount of data in interrupt mode.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
 *                the configuration information for the specified IRDA module.
  * @param  pData Pointer to data buffer.
  * @param  Size Amount of data to be sent.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IRDA_Transmit_IT(IRDA_HandleTypeDef *hirda, uint8_t *pData, uint16_t Size)
{
  /* Check that a Tx process is not already ongoing */
  if(hirda->gState == HAL_IRDA_STATE_READY)
  {
    if((pData == NULL) || (Size == 0))
    {
      return HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hirda);

    hirda->pTxBuffPtr = pData;
    hirda->TxXferSize = Size;
    hirda->TxXferCount = Size;

    hirda->ErrorCode = HAL_IRDA_ERROR_NONE;
    hirda->gState = HAL_IRDA_STATE_BUSY_TX;

    /* Process Unlocked */
    __HAL_UNLOCK(hirda);

    /* Enable the IRDA Transmit Data Register Empty Interrupt */
    SET_BIT(hirda->Instance->CR1, USART_CR1_TXEIE);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief  Receive an amount of data in interrupt mode.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @param  pData Pointer to data buffer.
  * @param  Size Amount of data to be received.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IRDA_Receive_IT(IRDA_HandleTypeDef *hirda, uint8_t *pData, uint16_t Size)
{
  /* Check that a Rx process is not already ongoing */
  if(hirda->RxState == HAL_IRDA_STATE_READY)
  {
    if((pData == NULL) || (Size == 0))
    {
      return HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hirda);

    hirda->pRxBuffPtr = pData;
    hirda->RxXferSize = Size;
    hirda->RxXferCount = Size;

    /* Computation of the mask to apply to the RDR register
       of the UART associated to the IRDA */
    IRDA_MASK_COMPUTATION(hirda);

    hirda->ErrorCode = HAL_IRDA_ERROR_NONE;
    hirda->RxState = HAL_IRDA_STATE_BUSY_RX;

    /* Process Unlocked */
    __HAL_UNLOCK(hirda);

    /* Enable the IRDA Parity Error and Data Register not empty Interrupts */
    SET_BIT(hirda->Instance->CR1, USART_CR1_PEIE| USART_CR1_RXNEIE);

    /* Enable the IRDA Error Interrupt: (Frame error, noise error, overrun error) */
    SET_BIT(hirda->Instance->CR3, USART_CR3_EIE);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief  Send an amount of data in DMA mode.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @param  pData pointer to data buffer.
  * @param  Size amount of data to be sent.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IRDA_Transmit_DMA(IRDA_HandleTypeDef *hirda, uint8_t *pData, uint16_t Size)
{
  /* Check that a Tx process is not already ongoing */
  if(hirda->gState == HAL_IRDA_STATE_READY)
  {
    if((pData == NULL) || (Size == 0))
    {
      return HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hirda);

    hirda->pTxBuffPtr = pData;
    hirda->TxXferSize = Size;
    hirda->TxXferCount = Size;

    hirda->ErrorCode = HAL_IRDA_ERROR_NONE;
    hirda->gState = HAL_IRDA_STATE_BUSY_TX;

    /* Set the IRDA DMA transfer complete callback */
    hirda->hdmatx->XferCpltCallback = IRDA_DMATransmitCplt;

    /* Set the IRDA DMA half transfer complete callback */
    hirda->hdmatx->XferHalfCpltCallback = IRDA_DMATransmitHalfCplt;

    /* Set the DMA error callback */
    hirda->hdmatx->XferErrorCallback = IRDA_DMAError;

    /* Set the DMA abort callback */
    hirda->hdmatx->XferAbortCallback = NULL;

    /* Enable the IRDA transmit DMA channel */
    HAL_DMA_Start_IT(hirda->hdmatx, (uint32_t)hirda->pTxBuffPtr, (uint32_t)&hirda->Instance->TDR, Size);

    /* Clear the TC flag in the ICR register */
    __HAL_IRDA_CLEAR_FLAG(hirda, IRDA_CLEAR_TCF);

    /* Process Unlocked */
    __HAL_UNLOCK(hirda);

    /* Enable the DMA transfer for transmit request by setting the DMAT bit
       in the USART CR3 register */
    SET_BIT(hirda->Instance->CR3, USART_CR3_DMAT);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief  Receive an amount of data in DMA mode.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @param  pData Pointer to data buffer.
  * @param  Size Amount of data to be received.
  * @note   When the IRDA parity is enabled (PCE = 1), the received data contains
  *         the parity bit (MSB position).
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IRDA_Receive_DMA(IRDA_HandleTypeDef *hirda, uint8_t *pData, uint16_t Size)
{
  /* Check that a Rx process is not already ongoing */
  if(hirda->RxState == HAL_IRDA_STATE_READY)
  {
    if((pData == NULL) || (Size == 0))
    {
      return HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hirda);

    hirda->pRxBuffPtr = pData;
    hirda->RxXferSize = Size;

    hirda->ErrorCode = HAL_IRDA_ERROR_NONE;
    hirda->RxState = HAL_IRDA_STATE_BUSY_RX;

    /* Set the IRDA DMA transfer complete callback */
    hirda->hdmarx->XferCpltCallback = IRDA_DMAReceiveCplt;

    /* Set the IRDA DMA half transfer complete callback */
    hirda->hdmarx->XferHalfCpltCallback = IRDA_DMAReceiveHalfCplt;

    /* Set the DMA error callback */
    hirda->hdmarx->XferErrorCallback = IRDA_DMAError;

    /* Set the DMA abort callback */
    hirda->hdmarx->XferAbortCallback = NULL;

    /* Enable the DMA channel */
    HAL_DMA_Start_IT(hirda->hdmarx, (uint32_t)&hirda->Instance->RDR, (uint32_t)hirda->pRxBuffPtr, Size);

    /* Process Unlocked */
    __HAL_UNLOCK(hirda);

    /* Enable the UART Parity Error Interrupt */
    SET_BIT(hirda->Instance->CR1, USART_CR1_PEIE);

    /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
    SET_BIT(hirda->Instance->CR3, USART_CR3_EIE);

    /* Enable the DMA transfer for the receiver request by setting the DMAR bit
       in the USART CR3 register */
    SET_BIT(hirda->Instance->CR3, USART_CR3_DMAR);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}


/**
  * @brief  Pause the DMA Transfer.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IRDA_DMAPause(IRDA_HandleTypeDef *hirda)
{
  /* Process Locked */
  __HAL_LOCK(hirda);

  if ((hirda->gState == HAL_IRDA_STATE_BUSY_TX) &&
      (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAT)))
  {
    /* Disable the IRDA DMA Tx request */
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAT);
  }
  if ((hirda->RxState == HAL_IRDA_STATE_BUSY_RX) &&
      (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAR)))
  {
    /* Disable PE and ERR (Frame error, noise error, overrun error) interrupts */
    CLEAR_BIT(hirda->Instance->CR1, USART_CR1_PEIE);
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_EIE);

    /* Disable the IRDA DMA Rx request */
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAR);
  }

  /* Process Unlocked */
  __HAL_UNLOCK(hirda);

  return HAL_OK;
}

/**
  * @brief  Resume the DMA Transfer.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IRDA_DMAResume(IRDA_HandleTypeDef *hirda)
{
  /* Process Locked */
  __HAL_LOCK(hirda);

  if(hirda->gState == HAL_IRDA_STATE_BUSY_TX)
  {
    /* Enable the IRDA DMA Tx request */
    SET_BIT(hirda->Instance->CR3, USART_CR3_DMAT);
  }
  if(hirda->RxState == HAL_IRDA_STATE_BUSY_RX)
  {
    /* Clear the Overrun flag before resuming the Rx transfer*/
    __HAL_IRDA_CLEAR_OREFLAG(hirda);

    /* Reenable PE and ERR (Frame error, noise error, overrun error) interrupts */
    SET_BIT(hirda->Instance->CR1, USART_CR1_PEIE);
    SET_BIT(hirda->Instance->CR3, USART_CR3_EIE);

    /* Enable the IRDA DMA Rx request */
    SET_BIT(hirda->Instance->CR3, USART_CR3_DMAR);
  }

  /* Process Unlocked */
  __HAL_UNLOCK(hirda);

  return HAL_OK;
}

/**
  * @brief  Stop the DMA Transfer.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_IRDA_DMAStop(IRDA_HandleTypeDef *hirda)
{
  /* The Lock is not implemented on this API to allow the user application
     to call the HAL IRDA API under callbacks HAL_IRDA_TxCpltCallback() / HAL_IRDA_RxCpltCallback() /
     HAL_IRDA_TxHalfCpltCallback / HAL_IRDA_RxHalfCpltCallback: 
     indeed, when HAL_DMA_Abort() API is called, the DMA TX/RX Transfer or Half Transfer complete  
     interrupt is generated if the DMA transfer interruption occurs at the middle or at the end of 
     the stream and the corresponding call back is executed. */

  /* Stop IRDA DMA Tx request if ongoing */
  if ((hirda->gState == HAL_IRDA_STATE_BUSY_TX) &&
      (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAT)))
  {
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAT);

    /* Abort the IRDA DMA Tx channel */
    if(hirda->hdmatx != NULL)
    {
      HAL_DMA_Abort(hirda->hdmatx);
    }

    IRDA_EndTxTransfer(hirda);
  }

  /* Stop IRDA DMA Rx request if ongoing */
  if ((hirda->RxState == HAL_IRDA_STATE_BUSY_RX) &&
      (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAR)))
  {
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAR);

    /* Abort the IRDA DMA Rx channel */
    if(hirda->hdmarx != NULL)
    {
      HAL_DMA_Abort(hirda->hdmarx);
    }

    IRDA_EndRxTransfer(hirda);
  }

  return HAL_OK;
}

/**
  * @brief  Abort ongoing transfers (blocking mode).
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @note   This procedure could be used for aborting any ongoing transfer started in Interrupt or DMA mode. 
  *         This procedure performs following operations :
  *           - Disable IRDA Interrupts (Tx and Rx)
  *           - Disable the DMA transfer in the peripheral register (if enabled)
  *           - Abort DMA transfer by calling HAL_DMA_Abort (in case of transfer in DMA mode)
  *           - Set handle State to READY
  * @note   This procedure is executed in blocking mode : when exiting function, Abort is considered as completed.
  * @retval HAL status
*/
HAL_StatusTypeDef HAL_IRDA_Abort(IRDA_HandleTypeDef *hirda)
{
  /* Disable TXEIE, TCIE, RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
  CLEAR_BIT(hirda->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE | USART_CR1_TXEIE | USART_CR1_TCIE));
  CLEAR_BIT(hirda->Instance->CR3, USART_CR3_EIE);

  /* Disable the IRDA DMA Tx request if enabled */
  if (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAT))
  {
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAT);

    /* Abort the IRDA DMA Tx channel : use blocking DMA Abort API (no callback) */
    if(hirda->hdmatx != NULL)
    {
      /* Set the IRDA DMA Abort callback to Null. 
         No call back execution at end of DMA abort procedure */
      hirda->hdmatx->XferAbortCallback = NULL;

      HAL_DMA_Abort(hirda->hdmatx);
    }
  }

  /* Disable the IRDA DMA Rx request if enabled */
  if (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAR))
  {
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAR);

    /* Abort the IRDA DMA Rx channel : use blocking DMA Abort API (no callback) */
    if(hirda->hdmarx != NULL)
    {
      /* Set the IRDA DMA Abort callback to Null. 
         No call back execution at end of DMA abort procedure */
      hirda->hdmarx->XferAbortCallback = NULL;

      HAL_DMA_Abort(hirda->hdmarx);
    }
  }

  /* Reset Tx and Rx transfer counters */
  hirda->TxXferCount = 0; 
  hirda->RxXferCount = 0; 

  /* Clear the Error flags in the ICR register */
  __HAL_IRDA_CLEAR_FLAG(hirda, IRDA_CLEAR_OREF | IRDA_CLEAR_NEF | IRDA_CLEAR_PEF | IRDA_CLEAR_FEF);

  /* Restore hirda->gState and hirda->RxState to Ready */
  hirda->gState  = HAL_IRDA_STATE_READY;
  hirda->RxState = HAL_IRDA_STATE_READY;

  /* Reset Handle ErrorCode to No Error */
  hirda->ErrorCode = HAL_IRDA_ERROR_NONE;

  return HAL_OK;
}

/**
  * @brief  Abort ongoing Transmit transfer (blocking mode).
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @note   This procedure could be used for aborting any ongoing Tx transfer started in Interrupt or DMA mode. 
  *         This procedure performs following operations :
  *           - Disable IRDA Interrupts (Tx)
  *           - Disable the DMA transfer in the peripheral register (if enabled)
  *           - Abort DMA transfer by calling HAL_DMA_Abort (in case of transfer in DMA mode)
  *           - Set handle State to READY
  * @note   This procedure is executed in blocking mode : when exiting function, Abort is considered as completed.
  * @retval HAL status
*/
HAL_StatusTypeDef HAL_IRDA_AbortTransmit(IRDA_HandleTypeDef *hirda)
{
  /* Disable TXEIE and TCIE interrupts */
  CLEAR_BIT(hirda->Instance->CR1, (USART_CR1_TXEIE | USART_CR1_TCIE));

  /* Disable the IRDA DMA Tx request if enabled */
  if (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAT))
  {
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAT);

    /* Abort the IRDA DMA Tx channel : use blocking DMA Abort API (no callback) */
    if(hirda->hdmatx != NULL)
    {
      /* Set the IRDA DMA Abort callback to Null. 
         No call back execution at end of DMA abort procedure */
      hirda->hdmatx->XferAbortCallback = NULL;

      HAL_DMA_Abort(hirda->hdmatx);
    }
  }

  /* Reset Tx transfer counter */
  hirda->TxXferCount = 0; 

  /* Restore hirda->gState to Ready */
  hirda->gState = HAL_IRDA_STATE_READY;

  return HAL_OK;
}

/**
  * @brief  Abort ongoing Receive transfer (blocking mode).
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @note   This procedure could be used for aborting any ongoing Rx transfer started in Interrupt or DMA mode. 
  *         This procedure performs following operations :
  *           - Disable IRDA Interrupts (Rx)
  *           - Disable the DMA transfer in the peripheral register (if enabled)
  *           - Abort DMA transfer by calling HAL_DMA_Abort (in case of transfer in DMA mode)
  *           - Set handle State to READY
  * @note   This procedure is executed in blocking mode : when exiting function, Abort is considered as completed.
  * @retval HAL status
*/
HAL_StatusTypeDef HAL_IRDA_AbortReceive(IRDA_HandleTypeDef *hirda)
{
  /* Disable RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
  CLEAR_BIT(hirda->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
  CLEAR_BIT(hirda->Instance->CR3, USART_CR3_EIE);

  /* Disable the IRDA DMA Rx request if enabled */
  if (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAR))
  {
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAR);

    /* Abort the IRDA DMA Rx channel : use blocking DMA Abort API (no callback) */
    if(hirda->hdmarx != NULL)
    {
      /* Set the IRDA DMA Abort callback to Null. 
         No call back execution at end of DMA abort procedure */
      hirda->hdmarx->XferAbortCallback = NULL;

      HAL_DMA_Abort(hirda->hdmarx);
    }
  }

  /* Reset Rx transfer counter */
  hirda->RxXferCount = 0; 

  /* Clear the Error flags in the ICR register */
  __HAL_IRDA_CLEAR_FLAG(hirda, IRDA_CLEAR_OREF | IRDA_CLEAR_NEF | IRDA_CLEAR_PEF | IRDA_CLEAR_FEF);

  /* Restore hirda->RxState to Ready */
  hirda->RxState = HAL_IRDA_STATE_READY;

  return HAL_OK;
}

/**
  * @brief  Abort ongoing transfers (Interrupt mode).
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @note   This procedure could be used for aborting any ongoing transfer started in Interrupt or DMA mode. 
  *         This procedure performs following operations :
  *           - Disable IRDA Interrupts (Tx and Rx)
  *           - Disable the DMA transfer in the peripheral register (if enabled)
  *           - Abort DMA transfer by calling HAL_DMA_Abort_IT (in case of transfer in DMA mode)
  *           - Set handle State to READY
  *           - At abort completion, call user abort complete callback
  * @note   This procedure is executed in Interrupt mode, meaning that abort procedure could be
  *         considered as completed only when user abort complete callback is executed (not when exiting function).
  * @retval HAL status
*/
HAL_StatusTypeDef HAL_IRDA_Abort_IT(IRDA_HandleTypeDef *hirda)
{
  uint32_t abortcplt = 1;
  
  /* Disable TXEIE, TCIE, RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
  CLEAR_BIT(hirda->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE | USART_CR1_TXEIE | USART_CR1_TCIE));
  CLEAR_BIT(hirda->Instance->CR3, USART_CR3_EIE);

  /* If DMA Tx and/or DMA Rx Handles are associated to IRDA Handle, DMA Abort complete callbacks should be initialised
     before any call to DMA Abort functions */
  /* DMA Tx Handle is valid */
  if(hirda->hdmatx != NULL)
  {
    /* Set DMA Abort Complete callback if IRDA DMA Tx request if enabled.
       Otherwise, set it to NULL */
    if(HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAT))
    {
      hirda->hdmatx->XferAbortCallback = IRDA_DMATxAbortCallback;
    }
    else
    {
      hirda->hdmatx->XferAbortCallback = NULL;
    }
  }
  /* DMA Rx Handle is valid */
  if(hirda->hdmarx != NULL)
  {
    /* Set DMA Abort Complete callback if IRDA DMA Rx request if enabled.
       Otherwise, set it to NULL */
    if(HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAR))
    {
      hirda->hdmarx->XferAbortCallback = IRDA_DMARxAbortCallback;
    }
    else
    {
      hirda->hdmarx->XferAbortCallback = NULL;
    }
  }
  
  /* Disable the IRDA DMA Tx request if enabled */
  if(HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAT))
  {
    /* Disable DMA Tx at UART level */
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAT);

    /* Abort the IRDA DMA Tx channel : use non blocking DMA Abort API (callback) */
    if(hirda->hdmatx != NULL)
    {
      /* IRDA Tx DMA Abort callback has already been initialised : 
         will lead to call HAL_IRDA_AbortCpltCallback() at end of DMA abort procedure */

      /* Abort DMA TX */
      if(HAL_DMA_Abort_IT(hirda->hdmatx) != HAL_OK)
      {
        hirda->hdmatx->XferAbortCallback = NULL;
      }
      else
      {
        abortcplt = 0;
      }
    }
  }

  /* Disable the IRDA DMA Rx request if enabled */
  if (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAR))
  {
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAR);

    /* Abort the IRDA DMA Rx channel : use non blocking DMA Abort API (callback) */
    if(hirda->hdmarx != NULL)
    {
      /* IRDA Rx DMA Abort callback has already been initialised : 
         will lead to call HAL_IRDA_AbortCpltCallback() at end of DMA abort procedure */

      /* Abort DMA RX */
      if(HAL_DMA_Abort_IT(hirda->hdmarx) != HAL_OK)
      {
        hirda->hdmarx->XferAbortCallback = NULL;
        abortcplt = 1;
      }
      else
      {
        abortcplt = 0;
      }
    }
  }

  /* if no DMA abort complete callback execution is required => call user Abort Complete callback */
  if (abortcplt == 1)
  {
    /* Reset Tx and Rx transfer counters */
    hirda->TxXferCount = 0; 
    hirda->RxXferCount = 0;

    /* Reset errorCode */
    hirda->ErrorCode = HAL_IRDA_ERROR_NONE;

    /* Clear the Error flags in the ICR register */
    __HAL_IRDA_CLEAR_FLAG(hirda, IRDA_CLEAR_OREF | IRDA_CLEAR_NEF | IRDA_CLEAR_PEF | IRDA_CLEAR_FEF);

    /* Restore hirda->gState and hirda->RxState to Ready */
    hirda->gState  = HAL_IRDA_STATE_READY;
    hirda->RxState = HAL_IRDA_STATE_READY;

    /* As no DMA to be aborted, call directly user Abort complete callback */
    HAL_IRDA_AbortCpltCallback(hirda);
  }

  return HAL_OK;
}

/**
  * @brief  Abort ongoing Transmit transfer (Interrupt mode).
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @note   This procedure could be used for aborting any ongoing Tx transfer started in Interrupt or DMA mode. 
  *         This procedure performs following operations :
  *           - Disable IRDA Interrupts (Tx)
  *           - Disable the DMA transfer in the peripheral register (if enabled)
  *           - Abort DMA transfer by calling HAL_DMA_Abort_IT (in case of transfer in DMA mode)
  *           - Set handle State to READY
  *           - At abort completion, call user abort complete callback
  * @note   This procedure is executed in Interrupt mode, meaning that abort procedure could be
  *         considered as completed only when user abort complete callback is executed (not when exiting function).
  * @retval HAL status
*/
HAL_StatusTypeDef HAL_IRDA_AbortTransmit_IT(IRDA_HandleTypeDef *hirda)
{
  /* Disable TXEIE and TCIE interrupts */
  CLEAR_BIT(hirda->Instance->CR1, (USART_CR1_TXEIE | USART_CR1_TCIE));

  /* Disable the IRDA DMA Tx request if enabled */
  if (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAT))
  {
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAT);

    /* Abort the IRDA DMA Tx channel : use non blocking DMA Abort API (callback) */
    if(hirda->hdmatx != NULL)
    {
      /* Set the IRDA DMA Abort callback : 
         will lead to call HAL_IRDA_AbortCpltCallback() at end of DMA abort procedure */
      hirda->hdmatx->XferAbortCallback = IRDA_DMATxOnlyAbortCallback;

      /* Abort DMA TX */
      if(HAL_DMA_Abort_IT(hirda->hdmatx) != HAL_OK)
      {
        /* Call Directly hirda->hdmatx->XferAbortCallback function in case of error */
        hirda->hdmatx->XferAbortCallback(hirda->hdmatx);
      }
    }
    else
    {
      /* Reset Tx transfer counter */
      hirda->TxXferCount = 0; 

      /* Restore hirda->gState to Ready */
      hirda->gState = HAL_IRDA_STATE_READY;

      /* As no DMA to be aborted, call directly user Abort complete callback */
      HAL_IRDA_AbortTransmitCpltCallback(hirda);
    }
  }
  else
  {
    /* Reset Tx transfer counter */
    hirda->TxXferCount = 0; 

    /* Restore hirda->gState to Ready */
    hirda->gState = HAL_IRDA_STATE_READY;

    /* As no DMA to be aborted, call directly user Abort complete callback */
    HAL_IRDA_AbortTransmitCpltCallback(hirda);
  }

  return HAL_OK;
}

/**
  * @brief  Abort ongoing Receive transfer (Interrupt mode).
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @note   This procedure could be used for aborting any ongoing Rx transfer started in Interrupt or DMA mode. 
  *         This procedure performs following operations :
  *           - Disable IRDA Interrupts (Rx)
  *           - Disable the DMA transfer in the peripheral register (if enabled)
  *           - Abort DMA transfer by calling HAL_DMA_Abort_IT (in case of transfer in DMA mode)
  *           - Set handle State to READY
  *           - At abort completion, call user abort complete callback
  * @note   This procedure is executed in Interrupt mode, meaning that abort procedure could be
  *         considered as completed only when user abort complete callback is executed (not when exiting function).
  * @retval HAL status
*/
HAL_StatusTypeDef HAL_IRDA_AbortReceive_IT(IRDA_HandleTypeDef *hirda)
{
  /* Disable RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
  CLEAR_BIT(hirda->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
  CLEAR_BIT(hirda->Instance->CR3, USART_CR3_EIE);

  /* Disable the IRDA DMA Rx request if enabled */
  if (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAR))
  {
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAR);

    /* Abort the IRDA DMA Rx channel : use non blocking DMA Abort API (callback) */
    if(hirda->hdmarx != NULL)
    {
      /* Set the IRDA DMA Abort callback : 
         will lead to call HAL_IRDA_AbortCpltCallback() at end of DMA abort procedure */
      hirda->hdmarx->XferAbortCallback = IRDA_DMARxOnlyAbortCallback;

      /* Abort DMA RX */
      if(HAL_DMA_Abort_IT(hirda->hdmarx) != HAL_OK)
      {
        /* Call Directly hirda->hdmarx->XferAbortCallback function in case of error */
        hirda->hdmarx->XferAbortCallback(hirda->hdmarx);
      }
    }
    else
    {
      /* Reset Rx transfer counter */
      hirda->RxXferCount = 0; 

      /* Clear the Error flags in the ICR register */
      __HAL_IRDA_CLEAR_FLAG(hirda, IRDA_CLEAR_OREF | IRDA_CLEAR_NEF | IRDA_CLEAR_PEF | IRDA_CLEAR_FEF);

      /* Restore hirda->RxState to Ready */
      hirda->RxState = HAL_IRDA_STATE_READY;

      /* As no DMA to be aborted, call directly user Abort complete callback */
      HAL_IRDA_AbortReceiveCpltCallback(hirda);
    }
  }
  else
  {
    /* Reset Rx transfer counter */
    hirda->RxXferCount = 0; 

    /* Clear the Error flags in the ICR register */
    __HAL_IRDA_CLEAR_FLAG(hirda, IRDA_CLEAR_OREF | IRDA_CLEAR_NEF | IRDA_CLEAR_PEF | IRDA_CLEAR_FEF);

    /* Restore hirda->RxState to Ready */
    hirda->RxState = HAL_IRDA_STATE_READY;

    /* As no DMA to be aborted, call directly user Abort complete callback */
    HAL_IRDA_AbortReceiveCpltCallback(hirda);
  }

  return HAL_OK;
}

/**
  * @brief  Handle IRDA interrupt request.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval None
  */
void HAL_IRDA_IRQHandler(IRDA_HandleTypeDef *hirda)
{
  uint32_t isrflags   = READ_REG(hirda->Instance->ISR);
  uint32_t cr1its     = READ_REG(hirda->Instance->CR1);
  uint32_t cr3its;
  uint32_t errorflags;

  /* If no error occurs */
  errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
  if (errorflags == RESET)
  {
    /* IRDA in mode Receiver ---------------------------------------------------*/
    if(((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
    {
      IRDA_Receive_IT(hirda);
      return;
    }
  }  

  /* If some errors occur */
  cr3its = READ_REG(hirda->Instance->CR3);
  if(   (errorflags != RESET) 
     && (    ((cr3its & USART_CR3_EIE) != RESET)
          || ((cr1its & (USART_CR1_RXNEIE | USART_CR1_PEIE)) != RESET)) )
  {
    /* IRDA parity error interrupt occurred -------------------------------------*/
    if(((isrflags & USART_ISR_PE) != RESET) && ((cr1its & USART_CR1_PEIE) != RESET))
    {
      __HAL_IRDA_CLEAR_IT(hirda, IRDA_CLEAR_PEF);

      hirda->ErrorCode |= HAL_IRDA_ERROR_PE;
    }

    /* IRDA frame error interrupt occurred --------------------------------------*/
    if(((isrflags & USART_ISR_FE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
    {
      __HAL_IRDA_CLEAR_IT(hirda, IRDA_CLEAR_FEF);

      hirda->ErrorCode |= HAL_IRDA_ERROR_FE;
    }

    /* IRDA noise error interrupt occurred --------------------------------------*/
    if(((isrflags & USART_ISR_NE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
    {
      __HAL_IRDA_CLEAR_IT(hirda, IRDA_CLEAR_NEF);

      hirda->ErrorCode |= HAL_IRDA_ERROR_NE;
    }

    /* IRDA Over-Run interrupt occurred -----------------------------------------*/
    if(((isrflags & USART_ISR_ORE) != RESET) &&
       (((cr1its & USART_CR1_RXNEIE) != RESET) || ((cr3its & USART_CR3_EIE) != RESET)))
    {
      __HAL_IRDA_CLEAR_IT(hirda, IRDA_CLEAR_OREF);

      hirda->ErrorCode |= HAL_IRDA_ERROR_ORE;
    }

    /* Call IRDA Error Call back function if need be --------------------------*/
    if(hirda->ErrorCode != HAL_IRDA_ERROR_NONE)
    {
      /* IRDA in mode Receiver ---------------------------------------------------*/
      if(((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
      {
        IRDA_Receive_IT(hirda);
      }

      /* If Overrun error occurs, or if any error occurs in DMA mode reception,
         consider error as blocking */
      if (((hirda->ErrorCode & HAL_IRDA_ERROR_ORE) != RESET) ||
          (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAR)))
      {  
        /* Blocking error : transfer is aborted
           Set the IRDA state ready to be able to start again the process,
           Disable Rx Interrupts, and disable Rx DMA request, if ongoing */
        IRDA_EndRxTransfer(hirda);

        /* Disable the IRDA DMA Rx request if enabled */
        if (HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAR))
        {
          CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAR);

          /* Abort the IRDA DMA Rx channel */
          if(hirda->hdmarx != NULL)
          {
            /* Set the IRDA DMA Abort callback : 
               will lead to call HAL_IRDA_ErrorCallback() at end of DMA abort procedure */
            hirda->hdmarx->XferAbortCallback = IRDA_DMAAbortOnError;

            /* Abort DMA RX */
            if(HAL_DMA_Abort_IT(hirda->hdmarx) != HAL_OK)
            {
              /* Call Directly hirda->hdmarx->XferAbortCallback function in case of error */
              hirda->hdmarx->XferAbortCallback(hirda->hdmarx);
            }
          }
          else
          {
            /* Call user error callback */
            HAL_IRDA_ErrorCallback(hirda);
          }
        }
        else
        {
          /* Call user error callback */
          HAL_IRDA_ErrorCallback(hirda);
        }
      }
      else
      {
        /* Non Blocking error : transfer could go on. 
           Error is notified to user through user error callback */
        HAL_IRDA_ErrorCallback(hirda);
        hirda->ErrorCode = HAL_IRDA_ERROR_NONE;
      }
    }
    return;

  } /* End if some error occurs */

  /* IRDA in mode Transmitter ------------------------------------------------*/
  if(((isrflags & USART_ISR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
  {
    IRDA_Transmit_IT(hirda);
    return;
  }

  /* IRDA in mode Transmitter (transmission end) -----------------------------*/
  if(((isrflags & USART_ISR_TC) != RESET) && ((cr1its & USART_CR1_TCIE) != RESET))
  {
    IRDA_EndTransmit_IT(hirda);
    return;
  }

}

/**
  * @brief  Tx Transfer completed callback.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval None
  */
__weak void HAL_IRDA_TxCpltCallback(IRDA_HandleTypeDef *hirda)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hirda);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_IRDA_TxCpltCallback can be implemented in the user file.
   */
}

/**
  * @brief  Tx Half Transfer completed callback.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified USART module.
  * @retval None
  */
__weak void HAL_IRDA_TxHalfCpltCallback(IRDA_HandleTypeDef *hirda)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hirda);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_IRDA_TxHalfCpltCallback can be implemented in the user file.
   */
}

/**
  * @brief  Rx Transfer completed callback.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval None
  */
__weak void HAL_IRDA_RxCpltCallback(IRDA_HandleTypeDef *hirda)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hirda);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_IRDA_RxCpltCallback can be implemented in the user file.
   */
}

/**
  * @brief  Rx Half Transfer complete callback.
  * @param  hirda: Pointer to a IRDA_HandleTypeDef structure that contains
  *                the configuration information for the specified IRDA module.
  * @retval None
  */
__weak void HAL_IRDA_RxHalfCpltCallback(IRDA_HandleTypeDef *hirda)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hirda);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_IRDA_RxHalfCpltCallback can be implemented in the user file.
   */
}

/**
  * @brief  IRDA error callback.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval None
  */
__weak void HAL_IRDA_ErrorCallback(IRDA_HandleTypeDef *hirda)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hirda);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_IRDA_ErrorCallback can be implemented in the user file.
   */
}

/**
  * @brief  IRDA Abort Complete callback.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval None
  */
__weak void HAL_IRDA_AbortCpltCallback (IRDA_HandleTypeDef *hirda)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hirda);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_IRDA_AbortCpltCallback can be implemented in the user file.
   */
}

/**
  * @brief  IRDA Abort Complete callback.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval None
  */
__weak void HAL_IRDA_AbortTransmitCpltCallback (IRDA_HandleTypeDef *hirda)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hirda);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_IRDA_AbortTransmitCpltCallback can be implemented in the user file.
   */
}

/**
  * @brief  IRDA Abort Receive Complete callback.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval None
  */
__weak void HAL_IRDA_AbortReceiveCpltCallback (IRDA_HandleTypeDef *hirda)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hirda);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_IRDA_AbortReceiveCpltCallback can be implemented in the user file.
   */
}

/**
  * @}
  */

/** @defgroup IRDA_Exported_Functions_Group4 Peripheral State and Error functions
  *  @brief   IRDA State and Errors functions
  *
@verbatim
  ==============================================================================
            ##### Peripheral State and Error functions #####
  ==============================================================================
  [..]
    This subsection provides a set of functions allowing to return the State of IrDA
    communication process and also return Peripheral Errors occurred during communication process
     (+) HAL_IRDA_GetState() API can be helpful to check in run-time the state
         of the IRDA peripheral handle.
     (+) HAL_IRDA_GetError() checks in run-time errors that could occur during
         communication.

@endverbatim
  * @{
  */

/**
  * @brief  Return the IRDA handle state.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval HAL state
  */
HAL_IRDA_StateTypeDef HAL_IRDA_GetState(IRDA_HandleTypeDef *hirda)
{
  /* Return IRDA handle state */
  uint32_t temp1= 0x00, temp2 = 0x00;
  temp1 = hirda->gState;
  temp2 = hirda->RxState;

  return (HAL_IRDA_StateTypeDef)(temp1 | temp2);
}

/**
  * @brief  Return the IRDA handle error code.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval IRDA Error Code
  */
uint32_t HAL_IRDA_GetError(IRDA_HandleTypeDef *hirda)
{
  return hirda->ErrorCode;
}

/**
  * @}
  */

/**
  * @}
  */

/** @defgroup IRDA_Private_Functions IRDA Private Functions
  * @{
  */

/**
  * @brief  Configure the IRDA peripheral.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval HAL status
  */
static HAL_StatusTypeDef IRDA_SetConfig(IRDA_HandleTypeDef *hirda)
{
  uint32_t tmpreg                     = 0x00000000;
  IRDA_ClockSourceTypeDef clocksource = IRDA_CLOCKSOURCE_UNDEFINED;
  HAL_StatusTypeDef ret               = HAL_OK;

  /* Check the communication parameters */
  assert_param(IS_IRDA_BAUDRATE(hirda->Init.BaudRate));
  assert_param(IS_IRDA_WORD_LENGTH(hirda->Init.WordLength));
  assert_param(IS_IRDA_PARITY(hirda->Init.Parity));
  assert_param(IS_IRDA_TX_RX_MODE(hirda->Init.Mode));
  assert_param(IS_IRDA_PRESCALER(hirda->Init.Prescaler));
  assert_param(IS_IRDA_POWERMODE(hirda->Init.PowerMode));

  /*-------------------------- USART CR1 Configuration -----------------------*/
  /* Configure the IRDA Word Length, Parity and transfer Mode:
     Set the M bits according to hirda->Init.WordLength value
     Set PCE and PS bits according to hirda->Init.Parity value
     Set TE and RE bits according to hirda->Init.Mode value */
  tmpreg = (uint32_t)hirda->Init.WordLength | hirda->Init.Parity | hirda->Init.Mode ;

  MODIFY_REG(hirda->Instance->CR1, IRDA_CR1_FIELDS, tmpreg);

  /*-------------------------- USART CR3 Configuration -----------------------*/
  MODIFY_REG(hirda->Instance->CR3, USART_CR3_IRLP, hirda->Init.PowerMode);

  /*-------------------------- USART GTPR Configuration ----------------------*/
  MODIFY_REG(hirda->Instance->GTPR, USART_GTPR_PSC, hirda->Init.Prescaler);

  /*-------------------------- USART BRR Configuration -----------------------*/
  IRDA_GETCLOCKSOURCE(hirda, clocksource);
  switch (clocksource)
  {
    case IRDA_CLOCKSOURCE_PCLK1:
      hirda->Instance->BRR = (uint16_t)((HAL_RCC_GetPCLK1Freq() + (hirda->Init.BaudRate/2)) / hirda->Init.BaudRate);
      break;
    case IRDA_CLOCKSOURCE_PCLK2:
      hirda->Instance->BRR = (uint16_t)((HAL_RCC_GetPCLK2Freq() + (hirda->Init.BaudRate/2)) / hirda->Init.BaudRate);
      break;
    case IRDA_CLOCKSOURCE_HSI:
      hirda->Instance->BRR = (uint16_t)((HSI_VALUE + (hirda->Init.BaudRate/2)) / hirda->Init.BaudRate);
      break;
    case IRDA_CLOCKSOURCE_SYSCLK:
      hirda->Instance->BRR = (uint16_t)((HAL_RCC_GetSysClockFreq() + (hirda->Init.BaudRate/2)) / hirda->Init.BaudRate);
      break;
    case IRDA_CLOCKSOURCE_LSE:
      hirda->Instance->BRR = (uint16_t)((LSE_VALUE  + (hirda->Init.BaudRate/2)) / hirda->Init.BaudRate);
      break;
    case IRDA_CLOCKSOURCE_UNDEFINED:
    default:
      ret = HAL_ERROR;
      break;
  }

  return ret;
}

/**
  * @brief  Check the IRDA Idle State.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval HAL status
  */
static HAL_StatusTypeDef IRDA_CheckIdleState(IRDA_HandleTypeDef *hirda)
{
  uint32_t tickstart = 0;

  /* Initialize the IRDA ErrorCode */
  hirda->ErrorCode = HAL_IRDA_ERROR_NONE;

  /* Init tickstart for timeout managment*/
  tickstart = HAL_GetTick();

  /* Check if the Transmitter is enabled */
  if((hirda->Instance->CR1 & USART_CR1_TE) == USART_CR1_TE)
  {
    /* Wait until TEACK flag is set */
    if(IRDA_WaitOnFlagUntilTimeout(hirda, USART_ISR_TEACK, RESET, tickstart, IRDA_TEACK_REACK_TIMEOUT) != HAL_OK)
    {
      /* Timeout occurred */
      return HAL_TIMEOUT;
    }
  }
  /* Check if the Receiver is enabled */
  if((hirda->Instance->CR1 & USART_CR1_RE) == USART_CR1_RE)
  {
    /* Wait until REACK flag is set */
    if(IRDA_WaitOnFlagUntilTimeout(hirda, USART_ISR_REACK, RESET, tickstart, IRDA_TEACK_REACK_TIMEOUT) != HAL_OK)
    {
      /* Timeout occurred */
      return HAL_TIMEOUT;
    }
  }

  /* Initialize the IRDA state*/
  hirda->gState  = HAL_IRDA_STATE_READY;
  hirda->RxState = HAL_IRDA_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hirda);

  return HAL_OK;
}

/**
  * @brief  Handle IRDA Communication Timeout.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @param  Flag Specifies the IRDA flag to check.
  * @param  Status Flag status (SET or RESET)
  * @param  Tickstart Tick start value
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
static HAL_StatusTypeDef IRDA_WaitOnFlagUntilTimeout(IRDA_HandleTypeDef *hirda, uint32_t Flag, FlagStatus Status, uint32_t Tickstart, uint32_t Timeout)
{
  /* Wait until flag is set */
  while((__HAL_IRDA_GET_FLAG(hirda, Flag) ? SET : RESET) == Status)
  {
    /* Check for the Timeout */
    if(Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0) || ((HAL_GetTick()-Tickstart) > Timeout))
      {
        /* Disable TXE, RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts for the interrupt process */
        CLEAR_BIT(hirda->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE | USART_CR1_TXEIE));
        CLEAR_BIT(hirda->Instance->CR3, USART_CR3_EIE);

        hirda->gState  = HAL_IRDA_STATE_READY;
        hirda->RxState = HAL_IRDA_STATE_READY;

        /* Process Unlocked */
        __HAL_UNLOCK(hirda);
        return HAL_TIMEOUT;
      }
    }
  }
  return HAL_OK;
}


/**
  * @brief  End ongoing Tx transfer on IRDA peripheral (following error detection or Transmit completion).
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval None
  */
static void IRDA_EndTxTransfer(IRDA_HandleTypeDef *hirda)
{
  /* Disable TXEIE and TCIE interrupts */
  CLEAR_BIT(hirda->Instance->CR1, (USART_CR1_TXEIE | USART_CR1_TCIE));

  /* At end of Tx process, restore hirda->gState to Ready */
  hirda->gState = HAL_IRDA_STATE_READY;
}


/**
  * @brief  End ongoing Rx transfer on UART peripheral (following error detection or Reception completion).
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval None
  */
static void IRDA_EndRxTransfer(IRDA_HandleTypeDef *hirda)
{
  /* Disable RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
  CLEAR_BIT(hirda->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
  CLEAR_BIT(hirda->Instance->CR3, USART_CR3_EIE);

  /* At end of Rx process, restore hirda->RxState to Ready */
  hirda->RxState = HAL_IRDA_STATE_READY;
}


/**
  * @brief  DMA IRDA transmit process complete callback.
  * @param  hdma Pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA module.
  * @retval None
  */
static void IRDA_DMATransmitCplt(DMA_HandleTypeDef *hdma)
{
  IRDA_HandleTypeDef* hirda = (IRDA_HandleTypeDef*)(hdma->Parent);

  /* DMA Normal mode */
  if ( HAL_IS_BIT_CLR(hdma->Instance->CCR, DMA_CCR_CIRC) )
  {
    hirda->TxXferCount = 0;

    /* Disable the DMA transfer for transmit request by resetting the DMAT bit
       in the IRDA CR3 register */
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAT);

    /* Enable the IRDA Transmit Complete Interrupt */
    SET_BIT(hirda->Instance->CR1, USART_CR1_TCIE);
  }
  /* DMA Circular mode */
  else
  {
    HAL_IRDA_TxCpltCallback(hirda);
  }

}

/**
  * @brief  DMA IRDA transmit process half complete callback.
  * @param  hdma Pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA module.
  * @retval None
  */
static void IRDA_DMATransmitHalfCplt(DMA_HandleTypeDef *hdma)
{
  IRDA_HandleTypeDef* hirda = (IRDA_HandleTypeDef*)(hdma->Parent);

  HAL_IRDA_TxHalfCpltCallback(hirda);
}

/**
  * @brief  DMA IRDA receive process complete callback.
  * @param  hdma: Pointer to a DMA_HandleTypeDef structure that contains
  *               the configuration information for the specified DMA module.
  * @retval None
  */
static void IRDA_DMAReceiveCplt(DMA_HandleTypeDef *hdma)
{
  IRDA_HandleTypeDef* hirda = (IRDA_HandleTypeDef*)(hdma->Parent);

  /* DMA Normal mode */
  if ( HAL_IS_BIT_CLR(hdma->Instance->CCR, DMA_CCR_CIRC) )
  {
    hirda->RxXferCount = 0;

    /* Disable PE and ERR (Frame error, noise error, overrun error) interrupts */
    CLEAR_BIT(hirda->Instance->CR1, USART_CR1_PEIE);
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_EIE);

    /* Disable the DMA transfer for the receiver request by resetting the DMAR bit
       in the IRDA CR3 register */
    CLEAR_BIT(hirda->Instance->CR3, USART_CR3_DMAR);

    /* At end of Rx process, restore hirda->RxState to Ready */
    hirda->RxState = HAL_IRDA_STATE_READY;
  }

  HAL_IRDA_RxCpltCallback(hirda);
}

/**
  * @brief DMA IRDA receive process half complete callback.
  * @param hdma: Pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA module.
  * @retval None
  */
static void IRDA_DMAReceiveHalfCplt(DMA_HandleTypeDef *hdma)
{
  IRDA_HandleTypeDef* hirda = (IRDA_HandleTypeDef*)(hdma->Parent);

  HAL_IRDA_RxHalfCpltCallback(hirda);
}

/**
  * @brief  DMA IRDA communication error callback.
  * @param  hdma Pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA module.
  * @retval None
  */
static void IRDA_DMAError(DMA_HandleTypeDef *hdma)
{
  IRDA_HandleTypeDef* hirda = (IRDA_HandleTypeDef*)(hdma->Parent);

  /* Stop IRDA DMA Tx request if ongoing */
  if (  (hirda->gState == HAL_IRDA_STATE_BUSY_TX)
      &&(HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAT)) )
  {
    hirda->TxXferCount = 0;
    IRDA_EndTxTransfer(hirda);
  }

  /* Stop IRDA DMA Rx request if ongoing */
  if (  (hirda->RxState == HAL_IRDA_STATE_BUSY_RX)
      &&(HAL_IS_BIT_SET(hirda->Instance->CR3, USART_CR3_DMAR)) )
  {
    hirda->RxXferCount = 0;
    IRDA_EndRxTransfer(hirda);
  }

  hirda->ErrorCode |= HAL_IRDA_ERROR_DMA;
  HAL_IRDA_ErrorCallback(hirda);
}

/**
  * @brief  DMA IRDA communication abort callback, when initiated by HAL services on Error
  *         (To be called at end of DMA Abort procedure following error occurrence).
  * @param  hdma DMA handle.
  * @retval None
  */
static void IRDA_DMAAbortOnError(DMA_HandleTypeDef *hdma)
{
  IRDA_HandleTypeDef* hirda = (IRDA_HandleTypeDef*)(hdma->Parent);
  hirda->RxXferCount = 0;
  hirda->TxXferCount = 0;

  HAL_IRDA_ErrorCallback(hirda);
}

/**
  * @brief  DMA IRDA Tx communication abort callback, when initiated by user
  *         (To be called at end of DMA Tx Abort procedure following user abort request).
  * @note   When this callback is executed, User Abort complete call back is called only if no
  *         Abort still ongoing for Rx DMA Handle.
  * @param  hdma DMA handle.
  * @retval None
  */
static void IRDA_DMATxAbortCallback(DMA_HandleTypeDef *hdma)
{
  IRDA_HandleTypeDef* hirda = (IRDA_HandleTypeDef* )(hdma->Parent);
  
  hirda->hdmatx->XferAbortCallback = NULL;

  /* Check if an Abort process is still ongoing */
  if(hirda->hdmarx != NULL)
  {
    if(hirda->hdmarx->XferAbortCallback != NULL)
    {
      return;
    }
  }
  
  /* No Abort process still ongoing : All DMA channels are aborted, call user Abort Complete callback */
  hirda->TxXferCount = 0;
  hirda->RxXferCount = 0;

  /* Reset errorCode */
  hirda->ErrorCode = HAL_IRDA_ERROR_NONE;

  /* Clear the Error flags in the ICR register */
  __HAL_IRDA_CLEAR_FLAG(hirda, IRDA_CLEAR_OREF | IRDA_CLEAR_NEF | IRDA_CLEAR_PEF | IRDA_CLEAR_FEF);

  /* Restore hirda->gState and hirda->RxState to Ready */
  hirda->gState  = HAL_IRDA_STATE_READY;
  hirda->RxState = HAL_IRDA_STATE_READY;

  /* Call user Abort complete callback */
  HAL_IRDA_AbortCpltCallback(hirda);
}


/**
  * @brief  DMA IRDA Rx communication abort callback, when initiated by user
  *         (To be called at end of DMA Rx Abort procedure following user abort request).
  * @note   When this callback is executed, User Abort complete call back is called only if no
  *         Abort still ongoing for Tx DMA Handle.
  * @param  hdma DMA handle.
  * @retval None
  */
static void IRDA_DMARxAbortCallback(DMA_HandleTypeDef *hdma)
{
  IRDA_HandleTypeDef* hirda = (IRDA_HandleTypeDef* )(hdma->Parent);
  
  hirda->hdmarx->XferAbortCallback = NULL;

  /* Check if an Abort process is still ongoing */
  if(hirda->hdmatx != NULL)
  {
    if(hirda->hdmatx->XferAbortCallback != NULL)
    {
      return;
    }
  }
  
  /* No Abort process still ongoing : All DMA channels are aborted, call user Abort Complete callback */
  hirda->TxXferCount = 0;
  hirda->RxXferCount = 0;

  /* Reset errorCode */
  hirda->ErrorCode = HAL_IRDA_ERROR_NONE;

  /* Clear the Error flags in the ICR register */
  __HAL_IRDA_CLEAR_FLAG(hirda, IRDA_CLEAR_OREF | IRDA_CLEAR_NEF | IRDA_CLEAR_PEF | IRDA_CLEAR_FEF);

  /* Restore hirda->gState and hirda->RxState to Ready */
  hirda->gState  = HAL_IRDA_STATE_READY;
  hirda->RxState = HAL_IRDA_STATE_READY;

  /* Call user Abort complete callback */
  HAL_IRDA_AbortCpltCallback(hirda);
}


/**
  * @brief  DMA IRDA Tx communication abort callback, when initiated by user by a call to
  *         HAL_IRDA_AbortTransmit_IT API (Abort only Tx transfer)
  *         (This callback is executed at end of DMA Tx Abort procedure following user abort request,
  *         and leads to user Tx Abort Complete callback execution).
  * @param  hdma DMA handle.
  * @retval None
  */
static void IRDA_DMATxOnlyAbortCallback(DMA_HandleTypeDef *hdma)
{
  IRDA_HandleTypeDef* hirda = (IRDA_HandleTypeDef*)(hdma->Parent);

  hirda->TxXferCount = 0;

  /* Restore hirda->gState to Ready */
  hirda->gState = HAL_IRDA_STATE_READY;

  /* Call user Abort complete callback */
  HAL_IRDA_AbortTransmitCpltCallback(hirda);
}

/**
  * @brief  DMA IRDA Rx communication abort callback, when initiated by user by a call to
  *         HAL_IRDA_AbortReceive_IT API (Abort only Rx transfer)
  *         (This callback is executed at end of DMA Rx Abort procedure following user abort request,
  *         and leads to user Rx Abort Complete callback execution).
  * @param  hdma DMA handle.
  * @retval None
  */
static void IRDA_DMARxOnlyAbortCallback(DMA_HandleTypeDef *hdma)
{
  IRDA_HandleTypeDef* hirda = ( IRDA_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;

  hirda->RxXferCount = 0;

  /* Clear the Error flags in the ICR register */
  __HAL_IRDA_CLEAR_FLAG(hirda, IRDA_CLEAR_OREF | IRDA_CLEAR_NEF | IRDA_CLEAR_PEF | IRDA_CLEAR_FEF);

  /* Restore hirda->RxState to Ready */
  hirda->RxState = HAL_IRDA_STATE_READY;

  /* Call user Abort complete callback */
  HAL_IRDA_AbortReceiveCpltCallback(hirda);
}

/**
  * @brief  Send an amount of data in interrupt mode.
  * @note   Function is called under interruption only, once
  *         interruptions have been enabled by HAL_IRDA_Transmit_IT().
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval HAL status
  */
static HAL_StatusTypeDef IRDA_Transmit_IT(IRDA_HandleTypeDef *hirda)
{
  uint16_t* tmp;

  /* Check that a Tx process is ongoing */
  if(hirda->gState == HAL_IRDA_STATE_BUSY_TX)
  {
    if(hirda->TxXferCount == 0)
    {
      /* Disable the IRDA Transmit Data Register Empty Interrupt */
      CLEAR_BIT(hirda->Instance->CR1, USART_CR1_TXEIE);

      /* Enable the IRDA Transmit Complete Interrupt */
      SET_BIT(hirda->Instance->CR1, USART_CR1_TCIE);

      return HAL_OK;
    }
    else
    {
      if ((hirda->Init.WordLength == IRDA_WORDLENGTH_9B) && (hirda->Init.Parity == IRDA_PARITY_NONE))
      {
        tmp = (uint16_t*) hirda->pTxBuffPtr;
        hirda->Instance->TDR = (*tmp & (uint16_t)0x01FF);
        hirda->pTxBuffPtr += 2;
      }
      else
      {
        hirda->Instance->TDR = (uint8_t)(*hirda->pTxBuffPtr++ & (uint8_t)0xFF);
      }
      hirda->TxXferCount--;

      return HAL_OK;
    }
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief  Wrap up transmission in non-blocking mode.
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval HAL status
  */
static HAL_StatusTypeDef IRDA_EndTransmit_IT(IRDA_HandleTypeDef *hirda)
{
  /* Disable the IRDA Transmit Complete Interrupt */
  CLEAR_BIT(hirda->Instance->CR1, USART_CR1_TCIE);

  /* Tx process is ended, restore hirda->gState to Ready */
  hirda->gState = HAL_IRDA_STATE_READY;

  HAL_IRDA_TxCpltCallback(hirda);

  return HAL_OK;
}

/**
  * @brief  Receive an amount of data in interrupt mode.
  * @note   Function is called under interruption only, once
  *         interruptions have been enabled by HAL_IRDA_Receive_IT()
  * @param  hirda Pointer to a IRDA_HandleTypeDef structure that contains
  *               the configuration information for the specified IRDA module.
  * @retval HAL status
  */
static HAL_StatusTypeDef IRDA_Receive_IT(IRDA_HandleTypeDef *hirda)
{
  uint16_t* tmp;
  uint16_t  uhMask = hirda->Mask;
  uint16_t  uhdata;

  /* Check that a Rx process is ongoing */
  if (hirda->RxState == HAL_IRDA_STATE_BUSY_RX)
  {
    uhdata = (uint16_t) READ_REG(hirda->Instance->RDR);
    if ((hirda->Init.WordLength == IRDA_WORDLENGTH_9B) && (hirda->Init.Parity == IRDA_PARITY_NONE))
    {
      tmp = (uint16_t*) hirda->pRxBuffPtr ;
      *tmp = (uint16_t)(uhdata & uhMask);
      hirda->pRxBuffPtr +=2;
    }
    else
    {
      *hirda->pRxBuffPtr++ = (uint8_t)(uhdata & (uint8_t)uhMask);
    }

    if(--hirda->RxXferCount == 0)
    {
      /* Disable the IRDA Parity Error Interrupt and RXNE interrupt */
      CLEAR_BIT(hirda->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));

      /* Disable the IRDA Error Interrupt: (Frame error, noise error, overrun error) */
      CLEAR_BIT(hirda->Instance->CR3, USART_CR3_EIE);

      /* Rx process is completed, restore hirda->RxState to Ready */
      hirda->RxState = HAL_IRDA_STATE_READY;

      HAL_IRDA_RxCpltCallback(hirda);

      return HAL_OK;
    }

    return HAL_OK;
  }
  else
  {
    /* Clear RXNE interrupt flag */
    __HAL_IRDA_SEND_REQ(hirda, IRDA_RXDATA_FLUSH_REQUEST);

    return HAL_BUSY;
  }
}

/**
  * @}
  */

#endif /* HAL_IRDA_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
