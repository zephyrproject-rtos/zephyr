/**
  ******************************************************************************
  * @file    stm32f7xx_hal_smartcard.c
  * @author  MCD Application Team
  * @version V1.1.1
  * @date    01-July-2016
  * @brief   SMARTCARD HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the SMARTCARD peripheral:
  *           + Initialization and de-initialization functions
  *           + IO operation functions
  *           + Peripheral State and Errors functions 
  *           
  @verbatim       
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
  [..]
    The SMARTCARD HAL driver can be used as follow:
    
    (#) Declare a SMARTCARD_HandleTypeDef handle structure.
    (#) Associate a USART to the SMARTCARD handle hsc.
    (#) Initialize the SMARTCARD low level resources by implementing the HAL_SMARTCARD_MspInit() API:
        (##) Enable the USARTx interface clock.
        (##) SMARTCARD pins configuration:
            (+++) Enable the clock for the SMARTCARD GPIOs.
            (+++) Configure these SMARTCARD pins as alternate function pull-up.
        (##) NVIC configuration if you need to use interrupt process (HAL_SMARTCARD_Transmit_IT()
             and HAL_SMARTCARD_Receive_IT() APIs):
            (+++) Configure the USARTx interrupt priority.
            (+++) Enable the NVIC USART IRQ handle.
            (+++) The specific USART interrupts (Transmission complete interrupt, 
                  RXNE interrupt and Error Interrupts) will be managed using the macros
                  __HAL_SMARTCARD_ENABLE_IT() and __HAL_SMARTCARD_DISABLE_IT() inside the transmit and receive process.
        (##) DMA Configuration if you need to use DMA process (HAL_SMARTCARD_Transmit_DMA()
             and HAL_SMARTCARD_Receive_DMA() APIs):
            (+++) Declare a DMA handle structure for the Tx/Rx stream.
            (+++) Enable the DMAx interface clock.
            (+++) Configure the declared DMA handle structure with the required Tx/Rx parameters.                
            (+++) Configure the DMA Tx/Rx Stream.
            (+++) Associate the initialized DMA handle to the SMARTCARD DMA Tx/Rx handle.
            (+++) Configure the priority and enable the NVIC for the transfer complete interrupt on the DMA Tx/Rx Stream.

    (#) Program the Baud Rate, Parity, Mode(Receiver/Transmitter), clock enabling/disabling and accordingly,
        the clock parameters (parity, phase, last bit), prescaler value, guard time and NACK on transmission
        error enabling or disabling in the hsc Init structure.
        
    (#) If required, program SMARTCARD advanced features (TX/RX pins swap, TimeOut, auto-retry counter,...)
        in the hsc AdvancedInit structure.

    (#) Initialize the SMARTCARD associated USART registers by calling
        the HAL_SMARTCARD_Init() API. 
    
  [..]
    (@) HAL_SMARTCARD_Init() API also configure also the low level Hardware GPIO, CLOCK, CORTEX...etc) by 
        calling the customized HAL_SMARTCARD_MspInit() API.
          
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

/** @defgroup SMARTCARD SMARTCARD
  * @brief HAL USART SMARTCARD module driver
  * @{
  */
#ifdef HAL_SMARTCARD_MODULE_ENABLED
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup SMARTCARD_Private_Constants SMARTCARD Private Constants
 * @{
 */
#define TEACK_REACK_TIMEOUT               1000U
#define HAL_SMARTCARD_TXDMA_TIMEOUTVALUE  22000U
#define USART_CR1_FIELDS      ((uint32_t)(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | \
                                          USART_CR1_TE | USART_CR1_RE | USART_CR1_OVER8))
#define USART_CR2_CLK_FIELDS  ((uint32_t)(USART_CR2_CLKEN|USART_CR2_CPOL|USART_CR2_CPHA|USART_CR2_LBCL))   
#define USART_CR2_FIELDS      ((uint32_t)(USART_CR2_RTOEN|USART_CR2_CLK_FIELDS|USART_CR2_STOP))
#define USART_CR3_FIELDS      ((uint32_t)(USART_CR3_ONEBIT|USART_CR3_NACK|USART_CR3_SCARCNT))  
/**
  * @}
  */
/* Private macros -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup SMARTCARD_Private_Functions
  * @{
  */
static void SMARTCARD_DMATransmitCplt(DMA_HandleTypeDef *hdma);
static void SMARTCARD_DMAReceiveCplt(DMA_HandleTypeDef *hdma);
static void SMARTCARD_DMAError(DMA_HandleTypeDef *hdma);
static void SMARTCARD_DMAAbortOnError(DMA_HandleTypeDef *hdma);
static void SMARTCARD_SetConfig(SMARTCARD_HandleTypeDef *hsc);
static HAL_StatusTypeDef SMARTCARD_WaitOnFlagUntilTimeout(SMARTCARD_HandleTypeDef *hsc, uint32_t Flag, FlagStatus Status, uint32_t Tickstart, uint32_t Timeout);
static HAL_StatusTypeDef SMARTCARD_CheckIdleState(SMARTCARD_HandleTypeDef *hsc);
static HAL_StatusTypeDef SMARTCARD_Transmit_IT(SMARTCARD_HandleTypeDef *hsc);
static HAL_StatusTypeDef SMARTCARD_Receive_IT(SMARTCARD_HandleTypeDef *hsc);
static void SMARTCARD_EndTxTransfer(SMARTCARD_HandleTypeDef *hsc);
static void SMARTCARD_EndRxTransfer(SMARTCARD_HandleTypeDef *hsc);
static void SMARTCARD_AdvFeatureConfig(SMARTCARD_HandleTypeDef *hsc);
/**
  * @}
  */
/* Exported functions --------------------------------------------------------*/
/** @defgroup SMARTCARD_Exported_Functions SMARTCARD Exported Functions
  * @{
  */

/** @defgroup SMARTCARD_Exported_Functions_Group1 SmartCard Initialization and de-initialization functions
  *  @brief    Initialization and Configuration functions
  *
@verbatim
===============================================================================
            ##### Initialization and Configuration functions #####
 ===============================================================================
  [..]
  This subsection provides a set of functions allowing to initialize the USART
  associated to the SmartCard.
      (+) These parameters can be configured: 
        (++) Baud Rate
        (++) Parity: parity should be enabled,
             Frame Length is fixed to 8 bits plus parity.
        (++) Receiver/transmitter modes
        (++) Synchronous mode (and if enabled, phase, polarity and last bit parameters)
        (++) Prescaler value
        (++) Guard bit time 
        (++) NACK enabling or disabling on transmission error               

      (+) The following advanced features can be configured as well:
        (++) TX and/or RX pin level inversion
        (++) data logical level inversion
        (++) RX and TX pins swap
        (++) RX overrun detection disabling
        (++) DMA disabling on RX error
        (++) MSB first on communication line
        (++) Time out enabling (and if activated, timeout value)
        (++) Block length
        (++) Auto-retry counter       
        
    [..]                                                  
    The HAL_SMARTCARD_Init() API follow respectively the USART (a)synchronous configuration procedures 
    (details for the procedures are available in reference manual).

@endverbatim

  The USART frame format is given in the following table:

     +---------------------------------------------------------------+
     | M1M0 bits |  PCE bit  |            USART frame                |
     |-----------------------|---------------------------------------|
     |     01    |    1      |    | SB | 8 bit data | PB | STB |     |
     +---------------------------------------------------------------+

  * @{
  */

/**
  * @brief Initializes the SMARTCARD mode according to the specified
  *         parameters in the SMARTCARD_InitTypeDef and create the associated handle .
  * @param hsc: SMARTCARD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SMARTCARD_Init(SMARTCARD_HandleTypeDef *hsc)
{
  /* Check the SMARTCARD handle allocation */
  if(hsc == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_SMARTCARD_INSTANCE(hsc->Instance));
  
  if(hsc->gState == HAL_SMARTCARD_STATE_RESET)
  { 
    /* Allocate lock resource and initialize it */
    hsc->Lock = HAL_UNLOCKED; 
    /* Init the low level hardware : GPIO, CLOCK, CORTEX */
    HAL_SMARTCARD_MspInit(hsc);
  }
  
  hsc->gState = HAL_SMARTCARD_STATE_BUSY;

  /* Disable the Peripheral */
  __HAL_SMARTCARD_DISABLE(hsc);

  /* Set the SMARTCARD Communication parameters */
  SMARTCARD_SetConfig(hsc);

  if(hsc->AdvancedInit.AdvFeatureInit != SMARTCARD_ADVFEATURE_NO_INIT)
  {
    SMARTCARD_AdvFeatureConfig(hsc);
  }

  /* In SmartCard mode, the following bits must be kept cleared: 
  - LINEN in the USART_CR2 register,
  - HDSEL and IREN  bits in the USART_CR3 register.*/
  CLEAR_BIT(hsc->Instance->CR2, USART_CR2_LINEN);
  CLEAR_BIT(hsc->Instance->CR3, (USART_CR3_IREN | USART_CR3_HDSEL));

  /* set the USART in SMARTCARD mode */
  SET_BIT(hsc->Instance->CR3, USART_CR3_SCEN);
  
  /* Enable the Peripheral */
  __HAL_SMARTCARD_ENABLE(hsc);
  
  /* TEACK and/or REACK to check before moving hsc->State to Ready */
  return (SMARTCARD_CheckIdleState(hsc));
}

/**
  * @brief DeInitializes the SMARTCARD peripheral 
  * @param hsc: SMARTCARD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SMARTCARD_DeInit(SMARTCARD_HandleTypeDef *hsc)
{
  /* Check the SMARTCARD handle allocation */
  if(hsc == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_SMARTCARD_INSTANCE(hsc->Instance));

  hsc->gState = HAL_SMARTCARD_STATE_BUSY;

  /* DeInit the low level hardware */
  HAL_SMARTCARD_MspDeInit(hsc);

  hsc->ErrorCode = HAL_SMARTCARD_ERROR_NONE;
  hsc->gState = HAL_SMARTCARD_STATE_RESET;
  hsc->RxState = HAL_SMARTCARD_STATE_RESET;

  /* Release Lock */
  __HAL_UNLOCK(hsc);

  return HAL_OK;
}

/**
  * @brief SMARTCARD MSP Init
  * @param hsc: SMARTCARD handle
  * @retval None
  */
 __weak void HAL_SMARTCARD_MspInit(SMARTCARD_HandleTypeDef *hsc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsc);
 
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_SMARTCARD_MspInit could be implemented in the user file
   */
}

/**
  * @brief SMARTCARD MSP DeInit
  * @param hsc: SMARTCARD handle
  * @retval None
  */
 __weak void HAL_SMARTCARD_MspDeInit(SMARTCARD_HandleTypeDef *hsc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsc);
 
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_SMARTCARD_MspDeInit could be implemented in the user file
   */
}

/**
  * @}
  */

/** @defgroup SMARTCARD_Exported_Functions_Group2 IO operation functions 
  *  @brief   SMARTCARD Transmit and Receive functions 
  *
@verbatim   
 ===============================================================================
                      ##### IO operation functions #####
 ===============================================================================  
    This subsection provides a set of functions allowing to manage the SMARTCARD data transfers.

    (#) There are two modes of transfer:
       (+) Blocking mode: The communication is performed in polling mode. 
            The HAL status of all data processing is returned by the same function 
            after finishing transfer.  
       (+) No-Blocking mode: The communication is performed using Interrupts 
           or DMA, These API's return the HAL status.
           The end of the data processing will be indicated through the 
           dedicated SMARTCARD IRQ when using Interrupt mode or the DMA IRQ when 
           using DMA mode.
           The HAL_SMARTCARD_TxCpltCallback(), HAL_SMARTCARD_RxCpltCallback() user callbacks 
           will be executed respectively at the end of the Transmit or Receive process
           The HAL_SMARTCARD_ErrorCallback()user callback will be executed when a communication error is detected

    (#) Blocking mode API's are :
        (+) HAL_SMARTCARD_Transmit()
        (+) HAL_SMARTCARD_Receive() 
        
    (#) Non-Blocking mode API's with Interrupt are :
        (+) HAL_SMARTCARD_Transmit_IT()
        (+) HAL_SMARTCARD_Receive_IT()
        (+) HAL_SMARTCARD_IRQHandler()
        (+) SMARTCARD_Transmit_IT()
        (+) SMARTCARD_Receive_IT()    

    (#) No-Blocking mode functions with DMA are :
        (+) HAL_SMARTCARD_Transmit_DMA()
        (+) HAL_SMARTCARD_Receive_DMA()

    (#) A set of Transfer Complete Callbacks are provided in No_Blocking mode:
        (+) HAL_SMARTCARD_TxCpltCallback()
        (+) HAL_SMARTCARD_RxCpltCallback()
        (+) HAL_SMARTCARD_ErrorCallback()
      
@endverbatim
  * @{
  */

/**
  * @brief Send an amount of data in blocking mode 
  * @param hsc: SMARTCARD handle
  * @param pData: pointer to data buffer
  * @param Size: amount of data to be sent
  * @param Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SMARTCARD_Transmit(SMARTCARD_HandleTypeDef *hsc, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  uint32_t tickstart = 0U;
  
  if(hsc->gState == HAL_SMARTCARD_STATE_READY)
  {
    if((pData == NULL) || (Size == 0U)) 
    {
      return  HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hsc);
    hsc->ErrorCode = HAL_SMARTCARD_ERROR_NONE;    
    hsc->gState = HAL_SMARTCARD_STATE_BUSY_TX;
    
    /* Init tickstart for timeout managment*/
    tickstart = HAL_GetTick();
    
    hsc->TxXferSize = Size;
    hsc->TxXferCount = Size;
    while(hsc->TxXferCount > 0U)
    {
      hsc->TxXferCount--;
      if(SMARTCARD_WaitOnFlagUntilTimeout(hsc, SMARTCARD_FLAG_TXE, RESET, tickstart, Timeout) != HAL_OK)  
      { 
        return HAL_TIMEOUT;
      }
      hsc->Instance->TDR = (*pData++ & (uint8_t)0xFFU);
    }
    if(SMARTCARD_WaitOnFlagUntilTimeout(hsc, SMARTCARD_FLAG_TC, RESET, tickstart, Timeout) != HAL_OK)  
    { 
      return HAL_TIMEOUT;
    }
    
    /* At end of Tx process, restore hsc->gState to Ready */
    hsc->gState = HAL_SMARTCARD_STATE_READY;
    
    /* Process Unlocked */
    __HAL_UNLOCK(hsc);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief Receive an amount of data in blocking mode 
  * @param hsc: SMARTCARD handle
  * @param pData: pointer to data buffer
  * @param Size: amount of data to be received
  * @param Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SMARTCARD_Receive(SMARTCARD_HandleTypeDef *hsc, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  uint32_t tickstart = 0U;
  
  if(hsc->RxState == HAL_SMARTCARD_STATE_READY)
  {
    if((pData == NULL) || (Size == 0U)) 
    {
      return  HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hsc);
    
    hsc->ErrorCode = HAL_SMARTCARD_ERROR_NONE;
    hsc->RxState = HAL_SMARTCARD_STATE_BUSY_RX;
    
    /* Init tickstart for timeout managment*/
    tickstart = HAL_GetTick();

    hsc->RxXferSize = Size;
    hsc->RxXferCount = Size;
    /* Check the remain data to be received */
    while(hsc->RxXferCount > 0U)
    {
      hsc->RxXferCount--;
      if(SMARTCARD_WaitOnFlagUntilTimeout(hsc, SMARTCARD_FLAG_RXNE, RESET, tickstart, Timeout) != HAL_OK)  
      {
        return HAL_TIMEOUT;
      }
      *pData++ = (uint8_t)(hsc->Instance->RDR & (uint8_t)0x00FFU);
    }

    /* At end of Rx process, restore hsc->RxState to Ready */
    hsc->RxState = HAL_SMARTCARD_STATE_READY;

    /* Process Unlocked */
    __HAL_UNLOCK(hsc);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief Send an amount of data in interrupt mode 
  * @param hsc: SMARTCARD handle
  * @param pData: pointer to data buffer
  * @param Size: amount of data to be sent
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SMARTCARD_Transmit_IT(SMARTCARD_HandleTypeDef *hsc, uint8_t *pData, uint16_t Size)
{
  /* Check that a Tx process is not already ongoing */
  if(hsc->gState == HAL_SMARTCARD_STATE_READY)
  {
    if((pData == NULL) || (Size == 0U)) 
    {
      return HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hsc);

    hsc->pTxBuffPtr = pData;
    hsc->TxXferSize = Size;
    hsc->TxXferCount = Size;

    hsc->ErrorCode = HAL_SMARTCARD_ERROR_NONE;
    hsc->gState = HAL_SMARTCARD_STATE_BUSY_TX;
    
    /* Process Unlocked */
    __HAL_UNLOCK(hsc);

    /* Enable the SMARTCARD Error Interrupt: (Frame error, noise error, overrun error) */
    CLEAR_BIT(hsc->Instance->CR3, USART_CR3_EIE);

    /* Enable the SMARTCARD Transmit Complete Interrupt */
    CLEAR_BIT(hsc->Instance->CR1, USART_CR1_TCIE);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief Receive an amount of data in interrupt mode 
  * @param hsc: SMARTCARD handle
  * @param pData: pointer to data buffer
  * @param Size: amount of data to be received
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SMARTCARD_Receive_IT(SMARTCARD_HandleTypeDef *hsc, uint8_t *pData, uint16_t Size)
{
  /* Check that a Rx process is not already ongoing */ 
  if(hsc->RxState == HAL_SMARTCARD_STATE_READY) 
  {
    if((pData == NULL) || (Size == 0U)) 
    {
      return HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hsc);

    hsc->pRxBuffPtr = pData;
    hsc->RxXferSize = Size;
    hsc->RxXferCount = Size;

    hsc->ErrorCode = HAL_SMARTCARD_ERROR_NONE;
    hsc->RxState = HAL_SMARTCARD_STATE_BUSY_RX;
    
    /* Process Unlocked */
    __HAL_UNLOCK(hsc);
    
    /* Enable the SMARTCARD Parity Error Interrupt */
    SET_BIT(hsc->Instance->CR1, USART_CR1_PEIE);

    /* Enable the SMARTCARD Error Interrupt: (Frame error, noise error, overrun error) */
    SET_BIT(hsc->Instance->CR3, USART_CR3_EIE);

    /* Enable the SMARTCARD Data Register not empty Interrupt */
    SET_BIT(hsc->Instance->CR1, USART_CR1_RXNEIE);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief Send an amount of data in DMA mode 
  * @param hsc: SMARTCARD handle
  * @param pData: pointer to data buffer
  * @param Size: amount of data to be sent
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SMARTCARD_Transmit_DMA(SMARTCARD_HandleTypeDef *hsc, uint8_t *pData, uint16_t Size)
{
  uint32_t *tmp;
  
  /* Check that a Tx process is not already ongoing */
  if(hsc->gState == HAL_SMARTCARD_STATE_READY)
  {
    if((pData == NULL) || (Size == 0U)) 
    {
      return HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hsc);

    hsc->pTxBuffPtr = pData;
    hsc->TxXferSize = Size;
    hsc->TxXferCount = Size;

    hsc->ErrorCode = HAL_SMARTCARD_ERROR_NONE;
    hsc->gState = HAL_SMARTCARD_STATE_BUSY_TX;

    /* Set the SMARTCARD DMA transfer complete callback */
    hsc->hdmatx->XferCpltCallback = SMARTCARD_DMATransmitCplt;

    /* Set the SMARTCARD error callback */
    hsc->hdmatx->XferErrorCallback = SMARTCARD_DMAError;
    
    /* Set the DMA abort callback */
    hsc->hdmatx->XferAbortCallback = NULL;

    /* Enable the SMARTCARD transmit DMA Stream */
    tmp = (uint32_t*)&pData;
    HAL_DMA_Start_IT(hsc->hdmatx, *(uint32_t*)tmp, (uint32_t)&hsc->Instance->TDR, Size);
    
	/* Clear the TC flag in the SR register by writing 0 to it */
    __HAL_SMARTCARD_CLEAR_IT(hsc, SMARTCARD_FLAG_TC);
    
    /* Process Unlocked */
    __HAL_UNLOCK(hsc);

    /* Enable the DMA transfer for transmit request by setting the DMAT bit
       in the SMARTCARD associated USART CR3 register */
    SET_BIT(hsc->Instance->CR3, USART_CR3_DMAT);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief Receive an amount of data in DMA mode 
  * @param hsc: SMARTCARD handle
  * @param pData: pointer to data buffer
  * @param Size: amount of data to be received
  * @note   The SMARTCARD-associated USART parity is enabled (PCE = 1), 
  *         the received data contain the parity bit (MSB position)   
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SMARTCARD_Receive_DMA(SMARTCARD_HandleTypeDef *hsc, uint8_t *pData, uint16_t Size)
{
  uint32_t *tmp;
  
  /* Check that a Rx process is not already ongoing */
  if(hsc->RxState == HAL_SMARTCARD_STATE_READY)
  {
    if((pData == NULL) || (Size == 0U)) 
    {
      return HAL_ERROR;
    }

    /* Process Locked */
    __HAL_LOCK(hsc);

    hsc->pRxBuffPtr = pData;
    hsc->RxXferSize = Size;

    hsc->ErrorCode = HAL_SMARTCARD_ERROR_NONE;
    hsc->RxState = HAL_SMARTCARD_STATE_BUSY_RX;

    /* Set the SMARTCARD DMA transfer complete callback */
    hsc->hdmarx->XferCpltCallback = SMARTCARD_DMAReceiveCplt;

    /* Set the SMARTCARD DMA error callback */
    hsc->hdmarx->XferErrorCallback = SMARTCARD_DMAError;
    
    /* Set the DMA abort callback */
    hsc->hdmatx->XferAbortCallback = NULL;

    /* Enable the DMA Stream */
    tmp = (uint32_t*)&pData;
    HAL_DMA_Start_IT(hsc->hdmarx, (uint32_t)&hsc->Instance->RDR, *(uint32_t*)tmp, Size);
    
    /* Process Unlocked */
    __HAL_UNLOCK(hsc);
    
    /* Enable the SMARTCARD Parity Error Interrupt */
    SET_BIT(hsc->Instance->CR1, USART_CR1_PEIE);

    /* Enable the SMARTCARD Error Interrupt: (Frame error, noise error, overrun error) */
    SET_BIT(hsc->Instance->CR3, USART_CR3_EIE);

    /* Enable the DMA transfer for the receiver request by setting the DMAR bit 
    in the SMARTCARD associated USART CR3 register */
    SET_BIT(hsc->Instance->CR3, USART_CR3_DMAR);

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}
    
/**
  * @brief SMARTCARD interrupt requests handling.
  * @param hsc: SMARTCARD handle
  * @retval None
  */
void HAL_SMARTCARD_IRQHandler(SMARTCARD_HandleTypeDef *hsc)
{
  uint32_t isrflags   = READ_REG(hsc->Instance->ISR);
  uint32_t cr1its     = READ_REG(hsc->Instance->CR1);
  uint32_t cr3its     = READ_REG(hsc->Instance->CR3);
  uint32_t dmarequest = 0x00U;
  uint32_t errorflags = 0x00U;

  /* If no error occurs */
  errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
  if(errorflags == RESET)
  {
    /* SMARTCARD in mode Receiver -------------------------------------------------*/
    if(((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
    {
      SMARTCARD_Receive_IT(hsc);
      return;
    }
  }

  /* If some errors occur */
  if((errorflags != RESET) && ((cr3its & (USART_CR3_EIE | USART_CR1_PEIE)) != RESET))
  {
    /* SMARTCARD parity error interrupt occurred ---------------------------*/
    if(((isrflags & SMARTCARD_FLAG_PE) != RESET) && ((cr1its & USART_CR1_PEIE) != RESET))
    { 
      hsc->ErrorCode |= HAL_SMARTCARD_ERROR_PE;
    }

    /* SMARTCARD frame error interrupt occurred ----------------------------*/
    if(((isrflags & SMARTCARD_FLAG_FE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
    { 
      hsc->ErrorCode |= HAL_SMARTCARD_ERROR_FE;
    }

    /* SMARTCARD noise error interrupt occurred ----------------------------*/
    if(((isrflags & SMARTCARD_FLAG_NE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
    { 
      hsc->ErrorCode |= HAL_SMARTCARD_ERROR_NE;
    }

    /* SMARTCARD Over-Run interrupt occurred -------------------------------*/
    if(((isrflags & SMARTCARD_FLAG_ORE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
    { 
      hsc->ErrorCode |= HAL_SMARTCARD_ERROR_ORE;
    }
    /* Call the Error call Back in case of Errors */
    if(hsc->ErrorCode != HAL_SMARTCARD_ERROR_NONE)
    {
      /* SMARTCARD in mode Receiver -----------------------------------------------*/
      if(((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
      {
        SMARTCARD_Receive_IT(hsc);
      }

      /* If Overrun error occurs, or if any error occurs in DMA mode reception,
         consider error as blocking */
      dmarequest = HAL_IS_BIT_SET(hsc->Instance->CR3, USART_CR3_DMAR);
      if(((hsc->ErrorCode & HAL_SMARTCARD_ERROR_ORE) != RESET) || dmarequest)
      {
        /* Blocking error : transfer is aborted
          Set the SMARTCARD state ready to be able to start again the process,
          Disable Rx Interrupts, and disable Rx DMA request, if ongoing */
        SMARTCARD_EndRxTransfer(hsc);
        /* Disable the SMARTCARD DMA Rx request if enabled */
        if (HAL_IS_BIT_SET(hsc->Instance->CR3, USART_CR3_DMAR))
        {
          CLEAR_BIT(hsc->Instance->CR3, USART_CR3_DMAR);

          /* Abort the SMARTCARD DMA Rx channel */
          if(hsc->hdmarx != NULL)
          {
            /* Set the SMARTCARD DMA Abort callback : 
              will lead to call HAL_SMARTCARD_ErrorCallback() at end of DMA abort procedure */
            hsc->hdmarx->XferAbortCallback = SMARTCARD_DMAAbortOnError;

           if(HAL_DMA_Abort_IT(hsc->hdmarx) != HAL_OK)
            {
              /* Call Directly XferAbortCallback function in case of error */
              hsc->hdmarx->XferAbortCallback(hsc->hdmarx);
            }
          }
          else
          {
            /* Call user error callback */
            HAL_SMARTCARD_ErrorCallback(hsc);
          }
        }
        else
        {
          /* Call user error callback */
          HAL_SMARTCARD_ErrorCallback(hsc);
        }
      }
      else
      {
        /* Call user error callback */
        HAL_SMARTCARD_ErrorCallback(hsc);
        hsc->ErrorCode = HAL_SMARTCARD_ERROR_NONE;
      }
    }
    return;
  } /* End if some error occurs */
  
  /* SMARTCARD in mode Transmitter -------------------------------------------*/
  if(((isrflags & SMARTCARD_FLAG_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
  {
    SMARTCARD_Transmit_IT(hsc);
    return;
  }
  
  /* SMARTCARD in mode Transmitter (transmission end) ------------------------*/
  if(((isrflags & SMARTCARD_FLAG_TC) != RESET) && ((cr1its & USART_CR1_TCIE) != RESET))
  {
    /* Disable the SMARTCARD Transmit Complete Interrupt */   
    CLEAR_BIT(hsc->Instance->CR1, USART_CR1_TCIE);

    /* Disable the SMARTCARD Error Interrupt: (Frame error, noise error, overrun error) */
    CLEAR_BIT(hsc->Instance->CR3, USART_CR3_EIE);

    /* Tx process is ended, restore hsmartcard->gState to Ready */
    hsc->gState = HAL_SMARTCARD_STATE_READY;

    HAL_SMARTCARD_TxCpltCallback(hsc);
    
    return;
  }
}

/**
  * @brief Tx Transfer completed callbacks
  * @param hsc: SMARTCARD handle
  * @retval None
  */
 __weak void HAL_SMARTCARD_TxCpltCallback(SMARTCARD_HandleTypeDef *hsc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsc);
 
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_SMARTCARD_TxCpltCallback could be implemented in the user file
   */ 
}

/**
  * @brief Rx Transfer completed callbacks
  * @param hsc: SMARTCARD handle
  * @retval None
  */
__weak void HAL_SMARTCARD_RxCpltCallback(SMARTCARD_HandleTypeDef *hsc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsc);
 
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_SMARTCARD_TxCpltCallback could be implemented in the user file
   */
}

/**
  * @brief SMARTCARD error callbacks
  * @param hsc: SMARTCARD handle
  * @retval None
  */
 __weak void HAL_SMARTCARD_ErrorCallback(SMARTCARD_HandleTypeDef *hsc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsc);
 
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_SMARTCARD_ErrorCallback could be implemented in the user file
   */
}

/**
  * @}
  */

/** @defgroup SMARTCARD_Exported_Functions_Group3 Peripheral State functions 
  *  @brief   SMARTCARD State functions 
  *
@verbatim   
 ===============================================================================
                ##### Peripheral State and Errors functions #####
 ===============================================================================
    [..]
    This subsection provides a set of functions allowing to initialize the SMARTCARD.
     (+) HAL_SMARTCARD_GetState() API is helpful to check in run-time the state of the SMARTCARD peripheral 
     (+) SMARTCARD_SetConfig() API configures the SMARTCARD peripheral  
     (+) SMARTCARD_CheckIdleState() API ensures that TEACK and/or REACK are set after initialization 

@endverbatim
  * @{
  */


/**
  * @brief return the SMARTCARD state
  * @param hsc: SMARTCARD handle
  * @retval HAL state
  */
HAL_SMARTCARD_StateTypeDef HAL_SMARTCARD_GetState(SMARTCARD_HandleTypeDef *hsc)
{
  uint32_t temp1= 0x00U, temp2 = 0x00U;
  temp1 = hsc->gState;
  temp2 = hsc->RxState;
  
  return (HAL_SMARTCARD_StateTypeDef)(temp1 | temp2);
}

/**
  * @brief  Return the SMARTCARD error code
  * @param  hsc : pointer to a SMARTCARD_HandleTypeDef structure that contains
  *              the configuration information for the specified SMARTCARD.
  * @retval SMARTCARD Error Code
  */
uint32_t HAL_SMARTCARD_GetError(SMARTCARD_HandleTypeDef *hsc)
{
  return hsc->ErrorCode;
}

/**
  * @}
  */

/**
  * @brief Send an amount of data in non blocking mode 
  * @param hsc: SMARTCARD handle.
  *         Function called under interruption only, once
  *         interruptions have been enabled by HAL_SMARTCARD_Transmit_IT()      
  * @retval HAL status
  */
static HAL_StatusTypeDef SMARTCARD_Transmit_IT(SMARTCARD_HandleTypeDef *hsc)
{
  if(hsc->gState == HAL_SMARTCARD_STATE_BUSY_TX)
  {
    if(hsc->TxXferCount == 0)
    {
      /* Disable the SMARTCARD Transmit Complete Interrupt */
      CLEAR_BIT(hsc->Instance->CR1, USART_CR1_TXEIE);
      
      /* Disable the SMARTCARD Error Interrupt: (Frame error, noise error, overrun error) */
      CLEAR_BIT(hsc->Instance->CR3, USART_CR3_EIE);
      
      /* Tx process is ended, restore hsmartcard->gState to Ready */
      hsc->gState = HAL_SMARTCARD_STATE_READY;
    }
    
    HAL_SMARTCARD_TxCpltCallback(hsc);
    
    return HAL_OK;
  }
  else
  {    
    hsc->Instance->TDR = (*hsc->pTxBuffPtr++ & (uint8_t)0xFFU);
    hsc->TxXferCount--;
    
    return HAL_OK;
  }
}

/**
  * @brief Receive an amount of data in non blocking mode 
  * @param hsc: SMARTCARD handle.
  *         Function called under interruption only, once
  *         interruptions have been enabled by HAL_SMARTCARD_Receive_IT()      
  * @retval HAL status
  */
static HAL_StatusTypeDef SMARTCARD_Receive_IT(SMARTCARD_HandleTypeDef *hsc)
{
  /* Check that a Rx process is ongoing */
  if(hsc->RxState == HAL_SMARTCARD_STATE_BUSY_RX) 
  {
    *hsc->pRxBuffPtr++ = (uint8_t)(hsc->Instance->RDR & (uint8_t)0xFFU);
    
    if(--hsc->RxXferCount == 0)
    {
      CLEAR_BIT(hsc->Instance->CR1, USART_CR1_RXNEIE);
      
      /* Disable the SMARTCARD Parity Error Interrupt */
      CLEAR_BIT(hsc->Instance->CR1, USART_CR1_PEIE);
      
      /* Disable the SMARTCARD Error Interrupt: (Frame error, noise error, overrun error) */
      CLEAR_BIT(hsc->Instance->CR3, USART_CR3_EIE);
      
      hsc->RxState = HAL_SMARTCARD_STATE_READY;
      
      HAL_SMARTCARD_RxCpltCallback(hsc);
      
      return HAL_OK;
    }
    
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief Configure the SMARTCARD associated USART peripheral 
  * @param hsc: SMARTCARD handle
  * @retval None
  */
static void SMARTCARD_SetConfig(SMARTCARD_HandleTypeDef *hsc)
{
  uint32_t tmpreg      = 0x00000000U;
  uint32_t clocksource = 0x00000000U;
  
  /* Check the parameters */ 
  assert_param(IS_SMARTCARD_INSTANCE(hsc->Instance));
  assert_param(IS_SMARTCARD_BAUDRATE(hsc->Init.BaudRate)); 
  assert_param(IS_SMARTCARD_WORD_LENGTH(hsc->Init.WordLength));  
  assert_param(IS_SMARTCARD_STOPBITS(hsc->Init.StopBits));   
  assert_param(IS_SMARTCARD_PARITY(hsc->Init.Parity));
  assert_param(IS_SMARTCARD_MODE(hsc->Init.Mode));
  assert_param(IS_SMARTCARD_POLARITY(hsc->Init.CLKPolarity));
  assert_param(IS_SMARTCARD_PHASE(hsc->Init.CLKPhase));
  assert_param(IS_SMARTCARD_LASTBIT(hsc->Init.CLKLastBit));    
  assert_param(IS_SMARTCARD_ONE_BIT_SAMPLE(hsc->Init.OneBitSampling));
  assert_param(IS_SMARTCARD_NACK(hsc->Init.NACKState));
  assert_param(IS_SMARTCARD_TIMEOUT(hsc->Init.TimeOutEnable));
  assert_param(IS_SMARTCARD_AUTORETRY_COUNT(hsc->Init.AutoRetryCount)); 

  /*-------------------------- USART CR1 Configuration -----------------------*/
  /* In SmartCard mode, M and PCE are forced to 1 (8 bits + parity).
   * Oversampling is forced to 16 (OVER8 = 0).
   * Configure the Parity and Mode: 
   *  set PS bit according to hsc->Init.Parity value
   *  set TE and RE bits according to hsc->Init.Mode value */
  tmpreg = (uint32_t) hsc->Init.Parity | hsc->Init.Mode;
  /* in case of TX-only mode, if NACK is enabled, the USART must be able to monitor 
     the bidirectional line to detect a NACK signal in case of parity error. 
     Therefore, the receiver block must be enabled as well (RE bit must be set). */
  if((hsc->Init.Mode == SMARTCARD_MODE_TX) && (hsc->Init.NACKState == SMARTCARD_NACK_ENABLE))
  {
    tmpreg |= USART_CR1_RE;   
  }
  tmpreg |= (uint32_t) hsc->Init.WordLength;
  MODIFY_REG(hsc->Instance->CR1, USART_CR1_FIELDS, tmpreg);

  /*-------------------------- USART CR2 Configuration -----------------------*/
  /* Stop bits are forced to 1.5 (STOP = 11) */
  tmpreg = hsc->Init.StopBits;
  /* Synchronous mode is activated by default */
  tmpreg |= (uint32_t) USART_CR2_CLKEN | hsc->Init.CLKPolarity; 
  tmpreg |= (uint32_t) hsc->Init.CLKPhase | hsc->Init.CLKLastBit;
  tmpreg |= (uint32_t) hsc->Init.TimeOutEnable;
  MODIFY_REG(hsc->Instance->CR2, USART_CR2_FIELDS, tmpreg); 
    
  /*-------------------------- USART CR3 Configuration -----------------------*/
  /* Configure 
   * - one-bit sampling method versus three samples' majority rule 
   *   according to hsc->Init.OneBitSampling 
   * - NACK transmission in case of parity error according 
   *   to hsc->Init.NACKEnable   
   * - autoretry counter according to hsc->Init.AutoRetryCount     */
  tmpreg =  (uint32_t) hsc->Init.OneBitSampling | hsc->Init.NACKState;
  tmpreg |= (uint32_t) (hsc->Init.AutoRetryCount << SMARTCARD_CR3_SCARCNT_LSB_POS);
  MODIFY_REG(hsc->Instance-> CR3,USART_CR3_FIELDS, tmpreg);
  
  /*-------------------------- USART GTPR Configuration ----------------------*/
  tmpreg = (uint32_t) (hsc->Init.Prescaler | (hsc->Init.GuardTime << SMARTCARD_GTPR_GT_LSB_POS));
  MODIFY_REG(hsc->Instance->GTPR, (uint32_t)(USART_GTPR_GT|USART_GTPR_PSC), tmpreg); 
  
  /*-------------------------- USART RTOR Configuration ----------------------*/ 
  tmpreg =   (uint32_t) (hsc->Init.BlockLength << SMARTCARD_RTOR_BLEN_LSB_POS);
  if(hsc->Init.TimeOutEnable == SMARTCARD_TIMEOUT_ENABLE)
  {
    assert_param(IS_SMARTCARD_TIMEOUT_VALUE(hsc->Init.TimeOutValue));
    tmpreg |=  (uint32_t) hsc->Init.TimeOutValue;
  }
  MODIFY_REG(hsc->Instance->RTOR, (USART_RTOR_RTO|USART_RTOR_BLEN), tmpreg);
  
  /*-------------------------- USART BRR Configuration -----------------------*/
  SMARTCARD_GETCLOCKSOURCE(hsc, clocksource);
  switch (clocksource)
  {
  case SMARTCARD_CLOCKSOURCE_PCLK1: 
    hsc->Instance->BRR = (uint16_t)((HAL_RCC_GetPCLK1Freq() + (hsc->Init.BaudRate/2))/ hsc->Init.BaudRate);
    break;
  case SMARTCARD_CLOCKSOURCE_PCLK2: 
    hsc->Instance->BRR = (uint16_t)((HAL_RCC_GetPCLK2Freq() + (hsc->Init.BaudRate/2))/ hsc->Init.BaudRate);
    break;
  case SMARTCARD_CLOCKSOURCE_HSI: 
    hsc->Instance->BRR = (uint16_t)((HSI_VALUE + (hsc->Init.BaudRate/2))/ hsc->Init.BaudRate); 
    break; 
  case SMARTCARD_CLOCKSOURCE_SYSCLK:  
    hsc->Instance->BRR = (uint16_t)((HAL_RCC_GetSysClockFreq() + (hsc->Init.BaudRate/2))/ hsc->Init.BaudRate);
    break;  
  case SMARTCARD_CLOCKSOURCE_LSE:                
    hsc->Instance->BRR = (uint16_t)((LSE_VALUE + (hsc->Init.BaudRate/2))/ hsc->Init.BaudRate); 
    break;
  default:
    break;
  } 
}

/**
  * @brief Check the SMARTCARD Idle State
  * @param hsc: SMARTCARD handle
  * @retval HAL status
  */
static HAL_StatusTypeDef SMARTCARD_CheckIdleState(SMARTCARD_HandleTypeDef *hsc)
{
  uint32_t tickstart = 0U;
  
  /* Initialize the SMARTCARD ErrorCode */
  hsc->ErrorCode = HAL_SMARTCARD_ERROR_NONE;
  
  /* Init tickstart for timeout managment*/
  tickstart = HAL_GetTick();

  /* Check if the Transmitter is enabled */
  if((hsc->Instance->CR1 & USART_CR1_TE) == USART_CR1_TE)
  {
    /* Wait until TEACK flag is set */
    if(SMARTCARD_WaitOnFlagUntilTimeout(hsc, USART_ISR_TEACK, RESET, tickstart, TEACK_REACK_TIMEOUT) != HAL_OK)  
    { 
      return HAL_TIMEOUT;
    } 
  }
  /* Check if the Receiver is enabled */
  if((hsc->Instance->CR1 & USART_CR1_RE) == USART_CR1_RE)
  {
    /* Wait until REACK flag is set */
    if(SMARTCARD_WaitOnFlagUntilTimeout(hsc, USART_ISR_REACK, RESET, tickstart, TEACK_REACK_TIMEOUT) != HAL_OK)  
    { 
      return HAL_TIMEOUT;
    }
  }
  
  /* Process Unlocked */
  __HAL_UNLOCK(hsc);
        
  /* Initialize the SMARTCARD state*/
  hsc->gState= HAL_SMARTCARD_STATE_READY;
  hsc->RxState= HAL_SMARTCARD_STATE_READY;
  
  return HAL_OK;
}

/**
  * @brief Configure the SMARTCARD associated USART peripheral advanced features 
  * @param hsc: SMARTCARD handle  
  * @retval None
  */
static void SMARTCARD_AdvFeatureConfig(SMARTCARD_HandleTypeDef *hsc)
{  
  /* Check whether the set of advanced features to configure is properly set */ 
  assert_param(IS_SMARTCARD_ADVFEATURE_INIT(hsc->AdvancedInit.AdvFeatureInit));
  
  /* if required, configure TX pin active level inversion */
  if(HAL_IS_BIT_SET(hsc->AdvancedInit.AdvFeatureInit, SMARTCARD_ADVFEATURE_TXINVERT_INIT))
  {
    assert_param(IS_SMARTCARD_ADVFEATURE_TXINV(hsc->AdvancedInit.TxPinLevelInvert));
    MODIFY_REG(hsc->Instance->CR2, USART_CR2_TXINV, hsc->AdvancedInit.TxPinLevelInvert);
  }
  
  /* if required, configure RX pin active level inversion */
  if(HAL_IS_BIT_SET(hsc->AdvancedInit.AdvFeatureInit, SMARTCARD_ADVFEATURE_RXINVERT_INIT))
  {
    assert_param(IS_SMARTCARD_ADVFEATURE_RXINV(hsc->AdvancedInit.RxPinLevelInvert));
    MODIFY_REG(hsc->Instance->CR2, USART_CR2_RXINV, hsc->AdvancedInit.RxPinLevelInvert);
  }
  
  /* if required, configure data inversion */
  if(HAL_IS_BIT_SET(hsc->AdvancedInit.AdvFeatureInit, SMARTCARD_ADVFEATURE_DATAINVERT_INIT))
  {
    assert_param(IS_SMARTCARD_ADVFEATURE_DATAINV(hsc->AdvancedInit.DataInvert));
    MODIFY_REG(hsc->Instance->CR2, USART_CR2_DATAINV, hsc->AdvancedInit.DataInvert);
  }
  
  /* if required, configure RX/TX pins swap */
  if(HAL_IS_BIT_SET(hsc->AdvancedInit.AdvFeatureInit, SMARTCARD_ADVFEATURE_SWAP_INIT))
  {
    assert_param(IS_SMARTCARD_ADVFEATURE_SWAP(hsc->AdvancedInit.Swap));
    MODIFY_REG(hsc->Instance->CR2, USART_CR2_SWAP, hsc->AdvancedInit.Swap);
  }
  
  /* if required, configure RX overrun detection disabling */
  if(HAL_IS_BIT_SET(hsc->AdvancedInit.AdvFeatureInit, SMARTCARD_ADVFEATURE_RXOVERRUNDISABLE_INIT))
  {
    assert_param(IS_SMARTCARD_OVERRUN(hsc->AdvancedInit.OverrunDisable));  
    MODIFY_REG(hsc->Instance->CR3, USART_CR3_OVRDIS, hsc->AdvancedInit.OverrunDisable);
  }
  
  /* if required, configure DMA disabling on reception error */
  if(HAL_IS_BIT_SET(hsc->AdvancedInit.AdvFeatureInit, SMARTCARD_ADVFEATURE_DMADISABLEONERROR_INIT))
  {
    assert_param(IS_SMARTCARD_ADVFEATURE_DMAONRXERROR(hsc->AdvancedInit.DMADisableonRxError));   
    MODIFY_REG(hsc->Instance->CR3, USART_CR3_DDRE, hsc->AdvancedInit.DMADisableonRxError);
  }
  
  /* if required, configure MSB first on communication line */  
  if(HAL_IS_BIT_SET(hsc->AdvancedInit.AdvFeatureInit, SMARTCARD_ADVFEATURE_MSBFIRST_INIT))
  {
    assert_param(IS_SMARTCARD_ADVFEATURE_MSBFIRST(hsc->AdvancedInit.MSBFirst));   
    MODIFY_REG(hsc->Instance->CR2, USART_CR2_MSBFIRST, hsc->AdvancedInit.MSBFirst);
  }
}
  
/**
  * @brief  This function handles SMARTCARD Communication Timeout.
  * @param  hsc SMARTCARD handle
  * @param  Flag specifies the SMARTCARD flag to check.
  * @param  Status The new Flag status (SET or RESET).
  * @param  Tickstart Tick start value
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
static HAL_StatusTypeDef SMARTCARD_WaitOnFlagUntilTimeout(SMARTCARD_HandleTypeDef *hsc, uint32_t Flag, FlagStatus Status, uint32_t Tickstart, uint32_t Timeout)
{
  /* Wait until flag is set */
  while((__HAL_SMARTCARD_GET_FLAG(hsc, Flag) ? SET : RESET) == Status)
  {
    /* Check for the Timeout */
    if(Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0U)||((HAL_GetTick() - Tickstart ) > Timeout))
      {
        /* Disable TXE, RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts for the interrupt process */
        CLEAR_BIT(hsc->Instance->CR1, USART_CR1_TXEIE);
        CLEAR_BIT(hsc->Instance->CR1, USART_CR1_RXNEIE);
        __HAL_SMARTCARD_DISABLE_IT(hsc, SMARTCARD_IT_PE);
        __HAL_SMARTCARD_DISABLE_IT(hsc, SMARTCARD_IT_ERR);
        CLEAR_BIT(hsc->Instance->CR1, USART_CR1_PEIE);
        CLEAR_BIT(hsc->Instance->CR3, USART_CR3_EIE);
        
        hsc->gState= HAL_SMARTCARD_STATE_READY;
        hsc->RxState= HAL_SMARTCARD_STATE_READY;
        
        /* Process Unlocked */
        __HAL_UNLOCK(hsc);
        
        return HAL_TIMEOUT;
      }
    }
  }
  return HAL_OK;
}

/*
  * @brief  End ongoing Tx transfer on SMARTCARD peripheral (following error detection or Transmit completion).
  * @param  hsc: SMARTCARD handle.
  * @retval None
  */
static void SMARTCARD_EndTxTransfer(SMARTCARD_HandleTypeDef *hsc)
{
  /* Disable TXEIE and TCIE interrupts */
  CLEAR_BIT(hsc->Instance->CR1, (USART_CR1_TXEIE | USART_CR1_TCIE));
  
  /* At end of Tx process, restore hsc->gState to Ready */
  hsc->gState = HAL_SMARTCARD_STATE_READY;
}


/**
  * @brief  End ongoing Rx transfer on SMARTCARD peripheral (following error detection or Reception completion).
  * @param  hsc: SMARTCARD handle.
  * @retval None
  */
static void SMARTCARD_EndRxTransfer(SMARTCARD_HandleTypeDef *hsc)
{
  /* Disable RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
  CLEAR_BIT(hsc->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
  CLEAR_BIT(hsc->Instance->CR3, USART_CR3_EIE);
  
  /* At end of Rx process, restore hsc->RxState to Ready */
  hsc->RxState = HAL_SMARTCARD_STATE_READY;
}

/**
  * @brief DMA SMARTCARD transmit process complete callback 
  * @param hdma: DMA handle
  * @retval None
  */
static void SMARTCARD_DMATransmitCplt(DMA_HandleTypeDef *hdma)     
{ 
  SMARTCARD_HandleTypeDef* hsc = ( SMARTCARD_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
  hsc->TxXferCount = 0;
  
  /* Disable the DMA transfer for transmit request by setting the DMAT bit
  in the USART CR3 register */
  CLEAR_BIT(hsc->Instance->CR3, USART_CR3_DMAT);
  
  /* Enable the SMARTCARD Transmit Complete Interrupt */   
  SET_BIT(hsc->Instance->CR1, USART_CR1_TCIE);
}

/**
  * @brief DMA SMARTCARD receive process complete callback 
  * @param hdma: DMA handle
  * @retval None
  */
static void SMARTCARD_DMAReceiveCplt(DMA_HandleTypeDef *hdma)  
{
  SMARTCARD_HandleTypeDef* hsc = ( SMARTCARD_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
  hsc->RxXferCount = 0;
  
  /* Disable RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
  CLEAR_BIT(hsc->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
  CLEAR_BIT(hsc->Instance->CR3, USART_CR3_EIE);
  
  /* Disable the DMA transfer for the receiver request by setting the DMAR bit 
     in the SMARTCARD associated USART CR3 register */
  CLEAR_BIT(hsc->Instance->CR3, USART_CR3_DMAR);
  
  /* At end of Rx process, restore hsc->RxState to Ready */
  hsc->RxState = HAL_SMARTCARD_STATE_READY;
  
  HAL_SMARTCARD_RxCpltCallback(hsc);
}

/**
  * @brief DMA SMARTCARD communication error callback 
  * @param hdma: DMA handle
  * @retval None
  */
static void SMARTCARD_DMAError(DMA_HandleTypeDef *hdma)
{
  SMARTCARD_HandleTypeDef* hsc = ( SMARTCARD_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
  hsc->RxXferCount = 0U;
  hsc->TxXferCount = 0U;
  hsc->ErrorCode = HAL_SMARTCARD_ERROR_DMA;
  
  /* Stop SMARTCARD DMA Tx request if ongoing */
  if (  (hsc->gState == HAL_SMARTCARD_STATE_BUSY_TX)
      &&(HAL_IS_BIT_SET(hsc->Instance->CR3, USART_CR3_DMAT)) )
  {
    SMARTCARD_EndTxTransfer(hsc);
  }
  
  /* Stop SMARTCARD DMA Rx request if ongoing */
  if (  (hsc->RxState == HAL_SMARTCARD_STATE_BUSY_RX)
      &&(HAL_IS_BIT_SET(hsc->Instance->CR3, USART_CR3_DMAR)) )
  {
    SMARTCARD_EndRxTransfer(hsc);
  }
  
  HAL_SMARTCARD_ErrorCallback(hsc);
}

/**
  * @brief DMA SMARTCARD communication abort callback, when call by HAL services on Error
  *        (To be called at end of DMA Abort procedure following error occurrence).
  * @param hdma: DMA handle.
  * @retval None
  */
static void SMARTCARD_DMAAbortOnError(DMA_HandleTypeDef *hdma)
{
  SMARTCARD_HandleTypeDef* hsc = ( SMARTCARD_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
  hsc->RxXferCount = 0U;
  hsc->TxXferCount = 0U;

  HAL_SMARTCARD_ErrorCallback(hsc);
}

/**
  * @}
  */

#endif /* HAL_SMARTCARD_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
