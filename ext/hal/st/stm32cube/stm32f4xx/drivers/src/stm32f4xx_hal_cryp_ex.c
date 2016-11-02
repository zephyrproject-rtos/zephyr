/**
  ******************************************************************************
  * @file    stm32f4xx_hal_cryp_ex.c
  * @author  MCD Application Team
  * @version V1.5.1
  * @date    01-July-2016
  * @brief   Extended CRYP HAL module driver
  *          This file provides firmware functions to manage the following 
  *          functionalities of CRYP extension peripheral:
  *           + Extended AES processing functions     
  *  
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
    [..]
    The CRYP Extension HAL driver can be used as follows:
    (#)Initialize the CRYP low level resources by implementing the HAL_CRYP_MspInit():
        (##) Enable the CRYP interface clock using __HAL_RCC_CRYP_CLK_ENABLE()
        (##) In case of using interrupts (e.g. HAL_CRYPEx_AESGCM_Encrypt_IT())
            (+++) Configure the CRYP interrupt priority using HAL_NVIC_SetPriority()
            (+++) Enable the CRYP IRQ handler using HAL_NVIC_EnableIRQ()
            (+++) In CRYP IRQ handler, call HAL_CRYP_IRQHandler()
        (##) In case of using DMA to control data transfer (e.g. HAL_AES_ECB_Encrypt_DMA())
            (+++) Enable the DMAx interface clock using __DMAx_CLK_ENABLE()
            (+++) Configure and enable two DMA streams one for managing data transfer from
                memory to peripheral (input stream) and another stream for managing data
                transfer from peripheral to memory (output stream)
            (+++) Associate the initialized DMA handle to the CRYP DMA handle
                using  __HAL_LINKDMA()
            (+++) Configure the priority and enable the NVIC for the transfer complete
                interrupt on the two DMA Streams. The output stream should have higher
                priority than the input stream HAL_NVIC_SetPriority() and HAL_NVIC_EnableIRQ()
    (#)Initialize the CRYP HAL using HAL_CRYP_Init(). This function configures mainly:
        (##) The data type: 1-bit, 8-bit, 16-bit and 32-bit
        (##) The key size: 128, 192 and 256. This parameter is relevant only for AES
        (##) The encryption/decryption key. Its size depends on the algorithm
                used for encryption/decryption
        (##) The initialization vector (counter). It is not used ECB mode.
    (#)Three processing (encryption/decryption) functions are available:
        (##) Polling mode: encryption and decryption APIs are blocking functions
             i.e. they process the data and wait till the processing is finished
             e.g. HAL_CRYPEx_AESGCM_Encrypt()
        (##) Interrupt mode: encryption and decryption APIs are not blocking functions
                i.e. they process the data under interrupt
                e.g. HAL_CRYPEx_AESGCM_Encrypt_IT()
        (##) DMA mode: encryption and decryption APIs are not blocking functions
                i.e. the data transfer is ensured by DMA
                e.g. HAL_CRYPEx_AESGCM_Encrypt_DMA()
    (#)When the processing function is called at first time after HAL_CRYP_Init()
       the CRYP peripheral is initialized and processes the buffer in input.
       At second call, the processing function performs an append of the already
       processed buffer.
       When a new data block is to be processed, call HAL_CRYP_Init() then the
       processing function.
    (#)In AES-GCM and AES-CCM modes are an authenticated encryption algorithms
       which provide authentication messages.
       HAL_AES_GCM_Finish() and HAL_AES_CCM_Finish() are used to provide those
       authentication messages.
       Call those functions after the processing ones (polling, interrupt or DMA).
       e.g. in AES-CCM mode call HAL_CRYPEx_AESCCM_Encrypt() to encrypt the plain data
            then call HAL_CRYPEx_AESCCM_Finish() to get the authentication message
     -@- For CCM Encrypt/Decrypt API's, only DataType = 8-bit is supported by this version.
     -@- The HAL_CRYPEx_AESGCM_xxxx() implementation is limited to 32bits inputs data length 
           (Plain/Cyphertext, Header) compared with GCM standards specifications (800-38D).
    (#)Call HAL_CRYP_DeInit() to deinitialize the CRYP peripheral.

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
#include "stm32f4xx_hal.h"

/** @addtogroup STM32F4xx_HAL_Driver
  * @{
  */

/** @defgroup CRYPEx CRYPEx
  * @brief CRYP Extension HAL module driver.
  * @{
  */

#ifdef HAL_CRYP_MODULE_ENABLED

#if defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F479xx)

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @addtogroup CRYPEx_Private_define
  * @{
  */
#define CRYPEx_TIMEOUT_VALUE  1U
/**
  * @}
  */ 
  
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @defgroup CRYPEx_Private_Functions_prototypes  CRYP Private Functions Prototypes
  * @{
  */
static void CRYPEx_GCMCCM_SetInitVector(CRYP_HandleTypeDef *hcryp, uint8_t *InitVector);
static void CRYPEx_GCMCCM_SetKey(CRYP_HandleTypeDef *hcryp, uint8_t *Key, uint32_t KeySize);
static HAL_StatusTypeDef CRYPEx_GCMCCM_ProcessData(CRYP_HandleTypeDef *hcryp, uint8_t *Input, uint16_t Ilength, uint8_t *Output, uint32_t Timeout);
static HAL_StatusTypeDef CRYPEx_GCMCCM_SetHeaderPhase(CRYP_HandleTypeDef *hcryp, uint8_t* Input, uint16_t Ilength, uint32_t Timeout);
static void CRYPEx_GCMCCM_DMAInCplt(DMA_HandleTypeDef *hdma);
static void CRYPEx_GCMCCM_DMAOutCplt(DMA_HandleTypeDef *hdma);
static void CRYPEx_GCMCCM_DMAError(DMA_HandleTypeDef *hdma);
static void CRYPEx_GCMCCM_SetDMAConfig(CRYP_HandleTypeDef *hcryp, uint32_t inputaddr, uint16_t Size, uint32_t outputaddr);
/**
  * @}
  */ 
  
/* Private functions ---------------------------------------------------------*/
/** @addtogroup CRYPEx_Private_Functions
  * @{
  */

/**
  * @brief  DMA CRYP Input Data process complete callback. 
  * @param  hdma: DMA handle
  * @retval None
  */
static void CRYPEx_GCMCCM_DMAInCplt(DMA_HandleTypeDef *hdma)  
{
  CRYP_HandleTypeDef* hcryp = ( CRYP_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
  
  /* Disable the DMA transfer for input Fifo request by resetting the DIEN bit
     in the DMACR register */
  hcryp->Instance->DMACR &= (uint32_t)(~CRYP_DMACR_DIEN);
  
  /* Call input data transfer complete callback */
  HAL_CRYP_InCpltCallback(hcryp);
}

/**
  * @brief  DMA CRYP Output Data process complete callback.
  * @param  hdma: DMA handle
  * @retval None
  */
static void CRYPEx_GCMCCM_DMAOutCplt(DMA_HandleTypeDef *hdma)
{
  CRYP_HandleTypeDef* hcryp = ( CRYP_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
  
  /* Disable the DMA transfer for output Fifo request by resetting the DOEN bit
     in the DMACR register */
  hcryp->Instance->DMACR &= (uint32_t)(~CRYP_DMACR_DOEN);
  
  /* Enable the CRYP peripheral */
  __HAL_CRYP_DISABLE(hcryp);
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_READY;
  
  /* Call output data transfer complete callback */
  HAL_CRYP_OutCpltCallback(hcryp);
}

/**
  * @brief  DMA CRYP communication error callback. 
  * @param  hdma: DMA handle
  * @retval None
  */
static void CRYPEx_GCMCCM_DMAError(DMA_HandleTypeDef *hdma)
{
  CRYP_HandleTypeDef* hcryp = ( CRYP_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
  hcryp->State= HAL_CRYP_STATE_READY;
  HAL_CRYP_ErrorCallback(hcryp);
}

/**
  * @brief  Writes the Key in Key registers. 
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  Key: Pointer to Key buffer
  * @param  KeySize: Size of Key
  * @retval None
  */
static void CRYPEx_GCMCCM_SetKey(CRYP_HandleTypeDef *hcryp, uint8_t *Key, uint32_t KeySize)
{
  uint32_t keyaddr = (uint32_t)Key;
  
  switch(KeySize)
  {
  case CRYP_KEYSIZE_256B:
    /* Key Initialisation */
    hcryp->Instance->K0LR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K0RR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K1LR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K1RR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K2LR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K2RR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K3LR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K3RR = __REV(*(uint32_t*)(keyaddr));
    break;
  case CRYP_KEYSIZE_192B:
    hcryp->Instance->K1LR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K1RR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K2LR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K2RR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K3LR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K3RR = __REV(*(uint32_t*)(keyaddr));
    break;
  case CRYP_KEYSIZE_128B:       
    hcryp->Instance->K2LR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K2RR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K3LR = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4U;
    hcryp->Instance->K3RR = __REV(*(uint32_t*)(keyaddr));
    break;
  default:
    break;
  }
}

/**
  * @brief  Writes the InitVector/InitCounter in IV registers.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  InitVector: Pointer to InitVector/InitCounter buffer
  * @retval None
  */
static void CRYPEx_GCMCCM_SetInitVector(CRYP_HandleTypeDef *hcryp, uint8_t *InitVector)
{
  uint32_t ivaddr = (uint32_t)InitVector;
  
  hcryp->Instance->IV0LR = __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4U;
  hcryp->Instance->IV0RR = __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4U;
  hcryp->Instance->IV1LR = __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4U;
  hcryp->Instance->IV1RR = __REV(*(uint32_t*)(ivaddr));
}

/**
  * @brief  Process Data: Writes Input data in polling mode and read the Output data.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  Input: Pointer to the Input buffer.
  * @param  Ilength: Length of the Input buffer, must be a multiple of 16
  * @param  Output: Pointer to the returned buffer
  * @param  Timeout: Timeout value 
  * @retval None
  */
static HAL_StatusTypeDef CRYPEx_GCMCCM_ProcessData(CRYP_HandleTypeDef *hcryp, uint8_t *Input, uint16_t Ilength, uint8_t *Output, uint32_t Timeout)
{
  uint32_t tickstart = 0U;   
  uint32_t i = 0U;
  uint32_t inputaddr  = (uint32_t)Input;
  uint32_t outputaddr = (uint32_t)Output;
  
  for(i=0U; (i < Ilength); i+=16U)
  {
    /* Write the Input block in the IN FIFO */
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR  = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    
    /* Get tick */
    tickstart = HAL_GetTick();
 
    while(HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_OFNE))
    {
      /* Check for the Timeout */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
        }
      }
    }
    /* Read the Output block from the OUT FIFO */
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
  }
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Sets the header phase
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  Input: Pointer to the Input buffer.
  * @param  Ilength: Length of the Input buffer, must be a multiple of 16
  * @param  Timeout: Timeout value   
  * @retval None
  */
static HAL_StatusTypeDef CRYPEx_GCMCCM_SetHeaderPhase(CRYP_HandleTypeDef *hcryp, uint8_t* Input, uint16_t Ilength, uint32_t Timeout)
{
  uint32_t tickstart = 0U;   
  uint32_t loopcounter = 0U;
  uint32_t headeraddr = (uint32_t)Input;
  
  /***************************** Header phase *********************************/
  if(hcryp->Init.HeaderSize != 0U)
  {
    /* Select header phase */
    __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_HEADER);
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    for(loopcounter = 0U; (loopcounter < hcryp->Init.HeaderSize); loopcounter+=16U)
    {
      /* Get tick */
      tickstart = HAL_GetTick();
      
      while(HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_IFEM))
      {
        /* Check for the Timeout */
        if(Timeout != HAL_MAX_DELAY)
        {
          if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
          {
            /* Change state */
            hcryp->State = HAL_CRYP_STATE_TIMEOUT;
            
            /* Process Unlocked */
            __HAL_UNLOCK(hcryp);
            
            return HAL_TIMEOUT;
          }
        }
      }
      /* Write the Input block in the IN FIFO */
      hcryp->Instance->DR = *(uint32_t*)(headeraddr);
      headeraddr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(headeraddr);
      headeraddr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(headeraddr);
      headeraddr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(headeraddr);
      headeraddr+=4U;
    }
    
    /* Wait until the complete message has been processed */

    /* Get tick */
    tickstart = HAL_GetTick();

    while((hcryp->Instance->SR & CRYP_FLAG_BUSY) == CRYP_FLAG_BUSY)
    {
      /* Check for the Timeout */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
        }
      }
    }
  }
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Sets the DMA configuration and start the DMA transfer.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  inputaddr: Address of the Input buffer
  * @param  Size: Size of the Input buffer, must be a multiple of 16
  * @param  outputaddr: Address of the Output buffer
  * @retval None
  */
static void CRYPEx_GCMCCM_SetDMAConfig(CRYP_HandleTypeDef *hcryp, uint32_t inputaddr, uint16_t Size, uint32_t outputaddr)
{
  /* Set the CRYP DMA transfer complete callback */
  hcryp->hdmain->XferCpltCallback = CRYPEx_GCMCCM_DMAInCplt;
  /* Set the DMA error callback */
  hcryp->hdmain->XferErrorCallback = CRYPEx_GCMCCM_DMAError;
  
  /* Set the CRYP DMA transfer complete callback */
  hcryp->hdmaout->XferCpltCallback = CRYPEx_GCMCCM_DMAOutCplt;
  /* Set the DMA error callback */
  hcryp->hdmaout->XferErrorCallback = CRYPEx_GCMCCM_DMAError;
  
  /* Enable the CRYP peripheral */
  __HAL_CRYP_ENABLE(hcryp);
  
  /* Enable the DMA In DMA Stream */
  HAL_DMA_Start_IT(hcryp->hdmain, inputaddr, (uint32_t)&hcryp->Instance->DR, Size/4U);
  
  /* Enable In DMA request */
  hcryp->Instance->DMACR = CRYP_DMACR_DIEN;
  
  /* Enable the DMA Out DMA Stream */
  HAL_DMA_Start_IT(hcryp->hdmaout, (uint32_t)&hcryp->Instance->DOUT, outputaddr, Size/4U);
  
  /* Enable Out DMA request */
  hcryp->Instance->DMACR |= CRYP_DMACR_DOEN;
}

/**
  * @}
  */ 

/* Exported functions---------------------------------------------------------*/
/** @addtogroup CRYPEx_Exported_Functions
  * @{
  */

/** @defgroup CRYPEx_Exported_Functions_Group1 Extended AES processing functions 
 *  @brief   Extended processing functions. 
 *
@verbatim   
  ==============================================================================
              ##### Extended AES processing functions #####
  ==============================================================================  
    [..]  This section provides functions allowing to:
      (+) Encrypt plaintext using AES-128/192/256 using GCM and CCM chaining modes
      (+) Decrypt cyphertext using AES-128/192/256 using GCM and CCM chaining modes
      (+) Finish the processing. This function is available only for GCM and CCM
    [..]  Three processing methods are available:
      (+) Polling mode
      (+) Interrupt mode
      (+) DMA mode

@endverbatim
  * @{
  */


/**
  * @brief  Initializes the CRYP peripheral in AES CCM encryption mode then 
  *         encrypt pPlainData. The cypher data are available in pCypherData.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pPlainData: Pointer to the plaintext buffer
  * @param  Size: Length of the plaintext buffer, must be a multiple of 16
  * @param  pCypherData: Pointer to the cyphertext buffer
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESCCM_Encrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData, uint32_t Timeout)
{
  uint32_t tickstart = 0U;
  uint32_t headersize = hcryp->Init.HeaderSize;
  uint32_t headeraddr = (uint32_t)hcryp->Init.Header;
  uint32_t loopcounter = 0U;
  uint32_t bufferidx = 0U;
  uint8_t blockb0[16U] = {0U};/* Block B0 */
  uint8_t ctr[16U] = {0U}; /* Counter */
  uint32_t b0addr = (uint32_t)blockb0;
  
  /* Process Locked */
  __HAL_LOCK(hcryp);
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_BUSY;
  
  /* Check if initialization phase has already been performed */
  if(hcryp->Phase == HAL_CRYP_PHASE_READY)
  {
    /************************ Formatting the header block *********************/
    if(headersize != 0U)
    {
      /* Check that the associated data (or header) length is lower than 2^16 - 2^8 = 65536 - 256 = 65280 */
      if(headersize < 65280U)
      {
        hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize >> 8U) & 0xFFU);
        hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize) & 0xFFU);
        headersize += 2U;
      }
      else
      {
        /* Header is encoded as 0xff || 0xfe || [headersize]32, i.e., six octets */
        hcryp->Init.pScratch[bufferidx++] = 0xFFU;
        hcryp->Init.pScratch[bufferidx++] = 0xFEU;
        hcryp->Init.pScratch[bufferidx++] = headersize & 0xff000000U;
        hcryp->Init.pScratch[bufferidx++] = headersize & 0x00ff0000U;
        hcryp->Init.pScratch[bufferidx++] = headersize & 0x0000ff00U;
        hcryp->Init.pScratch[bufferidx++] = headersize & 0x000000ffU;
        headersize += 6U;
      }
      /* Copy the header buffer in internal buffer "hcryp->Init.pScratch" */
      for(loopcounter = 0U; loopcounter < headersize; loopcounter++)
      {
        hcryp->Init.pScratch[bufferidx++] = hcryp->Init.Header[loopcounter];
      }
      /* Check if the header size is modulo 16 */
      if ((headersize % 16U) != 0U)
      {
        /* Padd the header buffer with 0s till the hcryp->Init.pScratch length is modulo 16 */
        for(loopcounter = headersize; loopcounter <= ((headersize/16U) + 1U) * 16U; loopcounter++)
        {
          hcryp->Init.pScratch[loopcounter] = 0U;
        }
        /* Set the header size to modulo 16 */
        headersize = ((headersize/16U) + 1U) * 16U;
      }
      /* Set the pointer headeraddr to hcryp->Init.pScratch */
      headeraddr = (uint32_t)hcryp->Init.pScratch;
    }
    /*********************** Formatting the block B0 **************************/
    if(headersize != 0U)
    {
      blockb0[0U] = 0x40U;
    }
    /* Flags byte */
    /* blockb0[0] |= 0u | (((( (uint8_t) hcryp->Init.TagSize - 2) / 2) & 0x07 ) << 3 ) | ( ( (uint8_t) (15 - hcryp->Init.IVSize) - 1) & 0x07U) */
    blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)(((uint8_t)(hcryp->Init.TagSize - (uint8_t)(2U))) >> 1U) & (uint8_t)0x07U) << 3U);
    blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)((uint8_t)(15U) - hcryp->Init.IVSize) - (uint8_t)1U) & (uint8_t)0x07U);
 
    for (loopcounter = 0U; loopcounter < hcryp->Init.IVSize; loopcounter++)
    {
      blockb0[loopcounter+1U] = hcryp->Init.pInitVect[loopcounter];
    }
    for ( ; loopcounter < 13U; loopcounter++)
    {
      blockb0[loopcounter+1U] = 0U;
    }
    
    blockb0[14U] = (Size >> 8U);
    blockb0[15U] = (Size & 0xFFU);
    
    /************************* Formatting the initial counter *****************/
    /* Byte 0:
       Bits 7 and 6 are reserved and shall be set to 0
       Bits 3, 4, and 5 shall also be set to 0, to ensure that all the counter blocks
       are distinct from B0
       Bits 0, 1, and 2 contain the same encoding of q as in B0
    */
    ctr[0U] = blockb0[0U] & 0x07U;
    /* byte 1 to NonceSize is the IV (Nonce) */
    for(loopcounter = 1U; loopcounter < hcryp->Init.IVSize + 1U; loopcounter++)
    {
      ctr[loopcounter] = blockb0[loopcounter];
    }
    /* Set the LSB to 1 */
    ctr[15U] |= 0x01U;
    
    /* Set the key */
    CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
    
    /* Set the CRYP peripheral in AES CCM mode */
    __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_CCM_ENCRYPT);
    
    /* Set the Initialization Vector */
    CRYPEx_GCMCCM_SetInitVector(hcryp, ctr);
    
    /* Select init phase */
    __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_INIT);
    
    b0addr = (uint32_t)blockb0;
    /* Write the blockb0 block in the IN FIFO */
    hcryp->Instance->DR = *(uint32_t*)(b0addr);
    b0addr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(b0addr);
    b0addr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(b0addr);
    b0addr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(b0addr);
    
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    /* Get tick */
    tickstart = HAL_GetTick();

    while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
    {
      /* Check for the Timeout */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
        
          return HAL_TIMEOUT;
        }
      }
    }
    /***************************** Header phase *******************************/
    if(headersize != 0U)
    {
      /* Select header phase */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_HEADER);
      
      /* Enable the CRYP peripheral */
      __HAL_CRYP_ENABLE(hcryp);
      
      for(loopcounter = 0U; (loopcounter < headersize); loopcounter+=16U)
      {
        /* Get tick */
        tickstart = HAL_GetTick();

        while(HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_IFEM))
        {
          {
            /* Check for the Timeout */
            if(Timeout != HAL_MAX_DELAY)
            {
              if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
              {
                /* Change state */
                hcryp->State = HAL_CRYP_STATE_TIMEOUT;
                
                /* Process Unlocked */
                __HAL_UNLOCK(hcryp);
                
                return HAL_TIMEOUT;
              }
            }
          }
        }
        /* Write the header block in the IN FIFO */
        hcryp->Instance->DR = *(uint32_t*)(headeraddr);
        headeraddr+=4U;
        hcryp->Instance->DR = *(uint32_t*)(headeraddr);
        headeraddr+=4U;
        hcryp->Instance->DR = *(uint32_t*)(headeraddr);
        headeraddr+=4U;
        hcryp->Instance->DR = *(uint32_t*)(headeraddr);
        headeraddr+=4U;
      }
      
      /* Get tick */
      tickstart = HAL_GetTick();

      while((hcryp->Instance->SR & CRYP_FLAG_BUSY) == CRYP_FLAG_BUSY)
      {
        /* Check for the Timeout */
        if(Timeout != HAL_MAX_DELAY)
        {
          if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
          {
            /* Change state */
            hcryp->State = HAL_CRYP_STATE_TIMEOUT;
            
            /* Process Unlocked */
            __HAL_UNLOCK(hcryp);
            
            return HAL_TIMEOUT;
          }
        }
      }
    }
    /* Save formatted counter into the scratch buffer pScratch */
    for(loopcounter = 0U; (loopcounter < 16U); loopcounter++)
    {
      hcryp->Init.pScratch[loopcounter] = ctr[loopcounter];
    }
    /* Reset bit 0 */
    hcryp->Init.pScratch[15U] &= 0xFEU;
    
    /* Select payload phase once the header phase is performed */
    __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
    
    /* Flush FIFO */
    __HAL_CRYP_FIFO_FLUSH(hcryp);
    
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    /* Set the phase */
    hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
  }
  
  /* Write Plain Data and Get Cypher Data */
  if(CRYPEx_GCMCCM_ProcessData(hcryp,pPlainData, Size, pCypherData, Timeout) != HAL_OK)
  {
    return HAL_TIMEOUT;
  }
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_READY;
  
  /* Process Unlocked */
  __HAL_UNLOCK(hcryp);
  
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Initializes the CRYP peripheral in AES GCM encryption mode then 
  *         encrypt pPlainData. The cypher data are available in pCypherData.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pPlainData: Pointer to the plaintext buffer
  * @param  Size: Length of the plaintext buffer, must be a multiple of 16
  * @param  pCypherData: Pointer to the cyphertext buffer
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESGCM_Encrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData, uint32_t Timeout)
{
  uint32_t tickstart = 0U;
  
  /* Process Locked */
  __HAL_LOCK(hcryp);
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_BUSY;
  
  /* Check if initialization phase has already been performed */
  if(hcryp->Phase == HAL_CRYP_PHASE_READY)
  {
    /* Set the key */
    CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
    
    /* Set the CRYP peripheral in AES GCM mode */
    __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_GCM_ENCRYPT);
    
    /* Set the Initialization Vector */
    CRYPEx_GCMCCM_SetInitVector(hcryp, hcryp->Init.pInitVect);
    
    /* Flush FIFO */
    __HAL_CRYP_FIFO_FLUSH(hcryp);
    
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    /* Get tick */
    tickstart = HAL_GetTick();

    while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
    {
      /* Check for the Timeout */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
        }
      }
    }
    
    /* Set the header phase */
    if(CRYPEx_GCMCCM_SetHeaderPhase(hcryp, hcryp->Init.Header, hcryp->Init.HeaderSize, Timeout) != HAL_OK)
    {
      return HAL_TIMEOUT;
    }
    
    /* Disable the CRYP peripheral */
    __HAL_CRYP_DISABLE(hcryp);
    
    /* Select payload phase once the header phase is performed */
    __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
    
    /* Flush FIFO */
    __HAL_CRYP_FIFO_FLUSH(hcryp);
    
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    /* Set the phase */
    hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
  }
  
  /* Write Plain Data and Get Cypher Data */
  if(CRYPEx_GCMCCM_ProcessData(hcryp, pPlainData, Size, pCypherData, Timeout) != HAL_OK)
  {
    return HAL_TIMEOUT;
  }
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_READY;
  
  /* Process Unlocked */
  __HAL_UNLOCK(hcryp);
  
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Initializes the CRYP peripheral in AES GCM decryption mode then
  *         decrypted pCypherData. The cypher data are available in pPlainData.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pCypherData: Pointer to the cyphertext buffer
  * @param  Size: Length of the cyphertext buffer, must be a multiple of 16
  * @param  pPlainData: Pointer to the plaintext buffer 
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESGCM_Decrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData, uint32_t Timeout)
{
  uint32_t tickstart = 0U;   
  
  /* Process Locked */
  __HAL_LOCK(hcryp);
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_BUSY;
  
  /* Check if initialization phase has already been performed */
  if(hcryp->Phase == HAL_CRYP_PHASE_READY)
  {
    /* Set the key */
    CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
    
    /* Set the CRYP peripheral in AES GCM decryption mode */
    __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_GCM_DECRYPT);
    
    /* Set the Initialization Vector */
    CRYPEx_GCMCCM_SetInitVector(hcryp, hcryp->Init.pInitVect);
    
    /* Flush FIFO */
    __HAL_CRYP_FIFO_FLUSH(hcryp);
    
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    /* Get tick */
    tickstart = HAL_GetTick();

    while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
    {
      /* Check for the Timeout */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
        }
      }
    }
    
    /* Set the header phase */
    if(CRYPEx_GCMCCM_SetHeaderPhase(hcryp, hcryp->Init.Header, hcryp->Init.HeaderSize, Timeout) != HAL_OK)
    {
      return HAL_TIMEOUT;
    }
    /* Disable the CRYP peripheral */
    __HAL_CRYP_DISABLE(hcryp);
    
    /* Select payload phase once the header phase is performed */
    __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
    
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    /* Set the phase */
    hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
  }
  
  /* Write Plain Data and Get Cypher Data */
  if(CRYPEx_GCMCCM_ProcessData(hcryp, pCypherData, Size, pPlainData, Timeout) != HAL_OK)
  {
    return HAL_TIMEOUT;
  }
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_READY;
  
  /* Process Unlocked */
  __HAL_UNLOCK(hcryp);
  
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Computes the authentication TAG.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  Size: Total length of the plain/cyphertext buffer
  * @param  AuthTag: Pointer to the authentication buffer
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESGCM_Finish(CRYP_HandleTypeDef *hcryp, uint32_t Size, uint8_t *AuthTag, uint32_t Timeout)
{
  uint32_t tickstart = 0U;   
  uint64_t headerlength = hcryp->Init.HeaderSize * 8U; /* Header length in bits */
  uint64_t inputlength = Size * 8U; /* input length in bits */
  uint32_t tagaddr = (uint32_t)AuthTag;
  
  /* Process Locked */
  __HAL_LOCK(hcryp);
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_BUSY;
  
  /* Check if initialization phase has already been performed */
  if(hcryp->Phase == HAL_CRYP_PHASE_PROCESS)
  {
    /* Change the CRYP phase */
    hcryp->Phase = HAL_CRYP_PHASE_FINAL;
    
    /* Disable CRYP to start the final phase */
    __HAL_CRYP_DISABLE(hcryp);
    
    /* Select final phase */
    __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_FINAL);
    
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    /* Write the number of bits in header (64 bits) followed by the number of bits
       in the payload */
    if(hcryp->Init.DataType == CRYP_DATATYPE_1B)
    {
      hcryp->Instance->DR = __RBIT(headerlength >> 32U);
      hcryp->Instance->DR = __RBIT(headerlength);
      hcryp->Instance->DR = __RBIT(inputlength >> 32U);
      hcryp->Instance->DR = __RBIT(inputlength);
    }
    else if(hcryp->Init.DataType == CRYP_DATATYPE_8B)
    {
      hcryp->Instance->DR = __REV(headerlength >> 32U);
      hcryp->Instance->DR = __REV(headerlength);
      hcryp->Instance->DR = __REV(inputlength >> 32U);
      hcryp->Instance->DR = __REV(inputlength);
    }
    else if(hcryp->Init.DataType == CRYP_DATATYPE_16B)
    {
      hcryp->Instance->DR = __ROR((uint32_t)(headerlength >> 32U), 16U);
      hcryp->Instance->DR = __ROR((uint32_t)headerlength, 16U);
      hcryp->Instance->DR = __ROR((uint32_t)(inputlength >> 32U), 16U);
      hcryp->Instance->DR = __ROR((uint32_t)inputlength, 16U);
    }
    else if(hcryp->Init.DataType == CRYP_DATATYPE_32B)
    {
      hcryp->Instance->DR = (uint32_t)(headerlength >> 32U);
      hcryp->Instance->DR = (uint32_t)(headerlength);
      hcryp->Instance->DR = (uint32_t)(inputlength >> 32U);
      hcryp->Instance->DR = (uint32_t)(inputlength);
    }
    /* Get tick */
    tickstart = HAL_GetTick();

    while(HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_OFNE))
    {
      /* Check for the Timeout */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
        
          return HAL_TIMEOUT;
        }
      }
    }
    
    /* Read the Auth TAG in the IN FIFO */
    *(uint32_t*)(tagaddr) = hcryp->Instance->DOUT;
    tagaddr+=4U;
    *(uint32_t*)(tagaddr) = hcryp->Instance->DOUT;
    tagaddr+=4U;
    *(uint32_t*)(tagaddr) = hcryp->Instance->DOUT;
    tagaddr+=4U;
    *(uint32_t*)(tagaddr) = hcryp->Instance->DOUT;
  }
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_READY;
  
  /* Process Unlocked */
  __HAL_UNLOCK(hcryp);
  
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Computes the authentication TAG for AES CCM mode.
  * @note   This API is called after HAL_AES_CCM_Encrypt()/HAL_AES_CCM_Decrypt()   
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  AuthTag: Pointer to the authentication buffer
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESCCM_Finish(CRYP_HandleTypeDef *hcryp, uint8_t *AuthTag, uint32_t Timeout)
{
  uint32_t tickstart = 0U;   
  uint32_t tagaddr = (uint32_t)AuthTag;
  uint32_t ctraddr = (uint32_t)hcryp->Init.pScratch;
  uint32_t temptag[4U] = {0U}; /* Temporary TAG (MAC) */
  uint32_t loopcounter;
  
  /* Process Locked */
  __HAL_LOCK(hcryp);
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_BUSY;
  
  /* Check if initialization phase has already been performed */
  if(hcryp->Phase == HAL_CRYP_PHASE_PROCESS)
  {
    /* Change the CRYP phase */
    hcryp->Phase = HAL_CRYP_PHASE_FINAL;
    
    /* Disable CRYP to start the final phase */
    __HAL_CRYP_DISABLE(hcryp);
    
    /* Select final phase */
    __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_FINAL);
    
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    /* Write the counter block in the IN FIFO */
    hcryp->Instance->DR = *(uint32_t*)ctraddr;
    ctraddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)ctraddr;
    ctraddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)ctraddr;
    ctraddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)ctraddr;
    
    /* Get tick */
    tickstart = HAL_GetTick();

    while(HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_OFNE))
    {
      /* Check for the Timeout */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
        }
      }
    }
    
    /* Read the Auth TAG in the IN FIFO */
    temptag[0U] = hcryp->Instance->DOUT;
    temptag[1U] = hcryp->Instance->DOUT;
    temptag[2U] = hcryp->Instance->DOUT;
    temptag[3U] = hcryp->Instance->DOUT;
  }
  
  /* Copy temporary authentication TAG in user TAG buffer */
  for(loopcounter = 0U; loopcounter < hcryp->Init.TagSize ; loopcounter++)
  {
    /* Set the authentication TAG buffer */
    *((uint8_t*)tagaddr+loopcounter) = *((uint8_t*)temptag+loopcounter);
  }
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_READY;
  
  /* Process Unlocked */
  __HAL_UNLOCK(hcryp);
  
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Initializes the CRYP peripheral in AES CCM decryption mode then
  *         decrypted pCypherData. The cypher data are available in pPlainData.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pPlainData: Pointer to the plaintext buffer
  * @param  Size: Length of the plaintext buffer, must be a multiple of 16
  * @param  pCypherData: Pointer to the cyphertext buffer
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESCCM_Decrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData, uint32_t Timeout)
{
  uint32_t tickstart = 0U;   
  uint32_t headersize = hcryp->Init.HeaderSize;
  uint32_t headeraddr = (uint32_t)hcryp->Init.Header;
  uint32_t loopcounter = 0U;
  uint32_t bufferidx = 0U;
  uint8_t blockb0[16U] = {0U};/* Block B0 */
  uint8_t ctr[16U] = {0U}; /* Counter */
  uint32_t b0addr = (uint32_t)blockb0;
  
  /* Process Locked */
  __HAL_LOCK(hcryp);
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_BUSY;
  
  /* Check if initialization phase has already been performed */
  if(hcryp->Phase == HAL_CRYP_PHASE_READY)
  {
    /************************ Formatting the header block *********************/
    if(headersize != 0U)
    {
      /* Check that the associated data (or header) length is lower than 2^16 - 2^8 = 65536 - 256 = 65280 */
      if(headersize < 65280U)
      {
        hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize >> 8U) & 0xFFU);
        hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize) & 0xFFU);
        headersize += 2U;
      }
      else
      {
        /* Header is encoded as 0xff || 0xfe || [headersize]32, i.e., six octets */
        hcryp->Init.pScratch[bufferidx++] = 0xFFU;
        hcryp->Init.pScratch[bufferidx++] = 0xFEU;
        hcryp->Init.pScratch[bufferidx++] = headersize & 0xff000000U;
        hcryp->Init.pScratch[bufferidx++] = headersize & 0x00ff0000U;
        hcryp->Init.pScratch[bufferidx++] = headersize & 0x0000ff00U;
        hcryp->Init.pScratch[bufferidx++] = headersize & 0x000000ffU;
        headersize += 6U;
      }
      /* Copy the header buffer in internal buffer "hcryp->Init.pScratch" */
      for(loopcounter = 0U; loopcounter < headersize; loopcounter++)
      {
        hcryp->Init.pScratch[bufferidx++] = hcryp->Init.Header[loopcounter];
      }
      /* Check if the header size is modulo 16 */
      if ((headersize % 16U) != 0U)
      {
        /* Padd the header buffer with 0s till the hcryp->Init.pScratch length is modulo 16 */
        for(loopcounter = headersize; loopcounter <= ((headersize/16U) + 1U) * 16U; loopcounter++)
        {
          hcryp->Init.pScratch[loopcounter] = 0U;
        }
        /* Set the header size to modulo 16 */
        headersize = ((headersize/16U) + 1U) * 16U;
      }
      /* Set the pointer headeraddr to hcryp->Init.pScratch */
      headeraddr = (uint32_t)hcryp->Init.pScratch;
    }
    /*********************** Formatting the block B0 **************************/
    if(headersize != 0U)
    {
      blockb0[0U] = 0x40U;
    }
    /* Flags byte */
    /* blockb0[0] |= 0u | (((( (uint8_t) hcryp->Init.TagSize - 2) / 2) & 0x07 ) << 3 ) | ( ( (uint8_t) (15 - hcryp->Init.IVSize) - 1) & 0x07U) */
    blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)(((uint8_t)(hcryp->Init.TagSize - (uint8_t)(2U))) >> 1U) & (uint8_t)0x07U) << 3U);
    blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)((uint8_t)(15U) - hcryp->Init.IVSize) - (uint8_t)1U) & (uint8_t)0x07U);
    
    for (loopcounter = 0U; loopcounter < hcryp->Init.IVSize; loopcounter++)
    {
      blockb0[loopcounter+1U] = hcryp->Init.pInitVect[loopcounter];
    }
    for ( ; loopcounter < 13U; loopcounter++)
    {
      blockb0[loopcounter+1U] = 0U;
    }
    
    blockb0[14U] = (Size >> 8U);
    blockb0[15U] = (Size & 0xFFU);
    
    /************************* Formatting the initial counter *****************/
    /* Byte 0:
       Bits 7 and 6 are reserved and shall be set to 0
       Bits 3, 4, and 5 shall also be set to 0, to ensure that all the counter 
       blocks are distinct from B0
       Bits 0, 1, and 2 contain the same encoding of q as in B0
    */
    ctr[0U] = blockb0[0U] & 0x07U;
    /* byte 1 to NonceSize is the IV (Nonce) */
    for(loopcounter = 1U; loopcounter < hcryp->Init.IVSize + 1U; loopcounter++)
    {
      ctr[loopcounter] = blockb0[loopcounter];
    }
    /* Set the LSB to 1 */
    ctr[15U] |= 0x01U;
    
    /* Set the key */
    CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
    
    /* Set the CRYP peripheral in AES CCM mode */
    __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_CCM_DECRYPT);
    
    /* Set the Initialization Vector */
    CRYPEx_GCMCCM_SetInitVector(hcryp, ctr);
    
    /* Select init phase */
    __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_INIT);
    
    b0addr = (uint32_t)blockb0;
    /* Write the blockb0 block in the IN FIFO */
    hcryp->Instance->DR = *(uint32_t*)(b0addr);
    b0addr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(b0addr);
    b0addr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(b0addr);
    b0addr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(b0addr);
    
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    /* Get tick */
    tickstart = HAL_GetTick();
 
    while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
    {
      /* Check for the Timeout */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
        
          return HAL_TIMEOUT;
        }
      }
    }
    /***************************** Header phase *******************************/
    if(headersize != 0U)
    {
      /* Select header phase */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_HEADER);
      
      /* Enable Crypto processor */
      __HAL_CRYP_ENABLE(hcryp);
      
      for(loopcounter = 0U; (loopcounter < headersize); loopcounter+=16U)
      {
        /* Get tick */
        tickstart = HAL_GetTick();

        while(HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_IFEM))
        {
          /* Check for the Timeout */
          if(Timeout != HAL_MAX_DELAY)
          {
            if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
            {
              /* Change state */
              hcryp->State = HAL_CRYP_STATE_TIMEOUT;
              
              /* Process Unlocked */
              __HAL_UNLOCK(hcryp);
              
              return HAL_TIMEOUT;
            }
          }
        }
        /* Write the header block in the IN FIFO */
        hcryp->Instance->DR = *(uint32_t*)(headeraddr);
        headeraddr+=4U;
        hcryp->Instance->DR = *(uint32_t*)(headeraddr);
        headeraddr+=4U;
        hcryp->Instance->DR = *(uint32_t*)(headeraddr);
        headeraddr+=4U;
        hcryp->Instance->DR = *(uint32_t*)(headeraddr);
        headeraddr+=4U;
      }
      
      /* Get tick */
      tickstart = HAL_GetTick();

      while((hcryp->Instance->SR & CRYP_FLAG_BUSY) == CRYP_FLAG_BUSY)
      {
      /* Check for the Timeout */
        if(Timeout != HAL_MAX_DELAY)
        {
          if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
          {
            /* Change state */
            hcryp->State = HAL_CRYP_STATE_TIMEOUT;
            
            /* Process Unlocked */
            __HAL_UNLOCK(hcryp);
            
            return HAL_TIMEOUT;
          }
        }
      }
    }
    /* Save formatted counter into the scratch buffer pScratch */
    for(loopcounter = 0U; (loopcounter < 16U); loopcounter++)
    {
      hcryp->Init.pScratch[loopcounter] = ctr[loopcounter];
    }
    /* Reset bit 0 */
    hcryp->Init.pScratch[15U] &= 0xFEU;
    /* Select payload phase once the header phase is performed */
    __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
    
    /* Flush FIFO */
    __HAL_CRYP_FIFO_FLUSH(hcryp);
    
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    /* Set the phase */
    hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
  }
  
  /* Write Plain Data and Get Cypher Data */
  if(CRYPEx_GCMCCM_ProcessData(hcryp, pCypherData, Size, pPlainData, Timeout) != HAL_OK)
  {
    return HAL_TIMEOUT;
  }
  
  /* Change the CRYP peripheral state */
  hcryp->State = HAL_CRYP_STATE_READY;
  
  /* Process Unlocked */
  __HAL_UNLOCK(hcryp);
  
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Initializes the CRYP peripheral in AES GCM encryption mode using IT.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pPlainData: Pointer to the plaintext buffer
  * @param  Size: Length of the plaintext buffer, must be a multiple of 16
  * @param  pCypherData: Pointer to the cyphertext buffer
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESGCM_Encrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData)
{
  uint32_t tickstart = 0U;   
  uint32_t inputaddr;
  uint32_t outputaddr;
  
  if(hcryp->State == HAL_CRYP_STATE_READY)
  {
    /* Process Locked */
    __HAL_LOCK(hcryp);
    
    /* Get the buffer addresses and sizes */    
    hcryp->CrypInCount = Size;
    hcryp->pCrypInBuffPtr = pPlainData;
    hcryp->pCrypOutBuffPtr = pCypherData;
    hcryp->CrypOutCount = Size;
    
    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_BUSY;
    
    /* Check if initialization phase has already been performed */
    if(hcryp->Phase == HAL_CRYP_PHASE_READY)
    {
      /* Set the key */
      CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
      
      /* Set the CRYP peripheral in AES GCM mode */
      __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_GCM_ENCRYPT);
      
      /* Set the Initialization Vector */
      CRYPEx_GCMCCM_SetInitVector(hcryp, hcryp->Init.pInitVect);
      
      /* Flush FIFO */
      __HAL_CRYP_FIFO_FLUSH(hcryp);
      
      /* Enable CRYP to start the init phase */
      __HAL_CRYP_ENABLE(hcryp);
      
     /* Get tick */
     tickstart = HAL_GetTick();

      while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
      {
        /* Check for the Timeout */
        
        if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
          
        }
      }
      
      /* Set the header phase */
      if(CRYPEx_GCMCCM_SetHeaderPhase(hcryp, hcryp->Init.Header, hcryp->Init.HeaderSize, 1U) != HAL_OK)
      {
        return HAL_TIMEOUT;
      }
      /* Disable the CRYP peripheral */
      __HAL_CRYP_DISABLE(hcryp);
      
      /* Select payload phase once the header phase is performed */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
      
      /* Flush FIFO */
      __HAL_CRYP_FIFO_FLUSH(hcryp);
      
      /* Set the phase */
      hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
    }
    
    if(Size != 0U)
    {
      /* Enable Interrupts */
      __HAL_CRYP_ENABLE_IT(hcryp, CRYP_IT_INI | CRYP_IT_OUTI);
      /* Enable the CRYP peripheral */
      __HAL_CRYP_ENABLE(hcryp);
    }
    else
    {
      /* Process Locked */
      __HAL_UNLOCK(hcryp);
      /* Change the CRYP state and phase */
      hcryp->State = HAL_CRYP_STATE_READY;
    }
    /* Return function status */
    return HAL_OK;
  }
  else if (__HAL_CRYP_GET_IT(hcryp, CRYP_IT_INI))
  {
    inputaddr = (uint32_t)hcryp->pCrypInBuffPtr;
    /* Write the Input block in the IN FIFO */
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR  = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    hcryp->pCrypInBuffPtr += 16U;
    hcryp->CrypInCount -= 16U;
    if(hcryp->CrypInCount == 0U)
    {
      __HAL_CRYP_DISABLE_IT(hcryp, CRYP_IT_INI);
      /* Call the Input data transfer complete callback */
      HAL_CRYP_InCpltCallback(hcryp);
    }
  }
  else if (__HAL_CRYP_GET_IT(hcryp, CRYP_IT_OUTI))
  {
    outputaddr = (uint32_t)hcryp->pCrypOutBuffPtr;
    /* Read the Output block from the Output FIFO */
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    hcryp->pCrypOutBuffPtr += 16U;
    hcryp->CrypOutCount -= 16U;
    if(hcryp->CrypOutCount == 0U)
    {
      __HAL_CRYP_DISABLE_IT(hcryp, CRYP_IT_OUTI);
      /* Process Unlocked */
      __HAL_UNLOCK(hcryp);
      /* Change the CRYP peripheral state */
      hcryp->State = HAL_CRYP_STATE_READY;
      /* Call Input transfer complete callback */
      HAL_CRYP_OutCpltCallback(hcryp);
    }
  }
  
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Initializes the CRYP peripheral in AES CCM encryption mode using interrupt.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pPlainData: Pointer to the plaintext buffer
  * @param  Size: Length of the plaintext buffer, must be a multiple of 16
  * @param  pCypherData: Pointer to the cyphertext buffer
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESCCM_Encrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData)
{
  uint32_t tickstart = 0U;   
  uint32_t inputaddr;
  uint32_t outputaddr;
  
  uint32_t headersize = hcryp->Init.HeaderSize;
  uint32_t headeraddr = (uint32_t)hcryp->Init.Header;
  uint32_t loopcounter = 0U;
  uint32_t bufferidx = 0U;
  uint8_t blockb0[16U] = {0U};/* Block B0 */
  uint8_t ctr[16U] = {0U}; /* Counter */
  uint32_t b0addr = (uint32_t)blockb0;
  
  if(hcryp->State == HAL_CRYP_STATE_READY)
  {
    /* Process Locked */
    __HAL_LOCK(hcryp);
    
    hcryp->CrypInCount = Size;
    hcryp->pCrypInBuffPtr = pPlainData;
    hcryp->pCrypOutBuffPtr = pCypherData;
    hcryp->CrypOutCount = Size;
    
    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_BUSY;
    
    /* Check if initialization phase has already been performed */
    if(hcryp->Phase == HAL_CRYP_PHASE_READY)
    {    
      /************************ Formatting the header block *******************/
      if(headersize != 0U)
      {
        /* Check that the associated data (or header) length is lower than 2^16 - 2^8 = 65536 - 256 = 65280 */
        if(headersize < 65280U)
        {
          hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize >> 8U) & 0xFFU);
          hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize) & 0xFFU);
          headersize += 2U;
        }
        else
        {
          /* Header is encoded as 0xff || 0xfe || [headersize]32, i.e., six octets */
          hcryp->Init.pScratch[bufferidx++] = 0xFFU;
          hcryp->Init.pScratch[bufferidx++] = 0xFEU;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0xff000000U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x00ff0000U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x0000ff00U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x000000ffU;
          headersize += 6U;
        }
        /* Copy the header buffer in internal buffer "hcryp->Init.pScratch" */
        for(loopcounter = 0U; loopcounter < headersize; loopcounter++)
        {
          hcryp->Init.pScratch[bufferidx++] = hcryp->Init.Header[loopcounter];
        }
        /* Check if the header size is modulo 16 */
        if ((headersize % 16U) != 0U)
        {
          /* Padd the header buffer with 0s till the hcryp->Init.pScratch length is modulo 16 */
          for(loopcounter = headersize; loopcounter <= ((headersize/16U) + 1U) * 16U; loopcounter++)
          {
            hcryp->Init.pScratch[loopcounter] = 0U;
          }
          /* Set the header size to modulo 16 */
          headersize = ((headersize/16U) + 1U) * 16U;
        }
        /* Set the pointer headeraddr to hcryp->Init.pScratch */
        headeraddr = (uint32_t)hcryp->Init.pScratch;
      }
      /*********************** Formatting the block B0 ************************/
      if(headersize != 0U)
      {
        blockb0[0U] = 0x40U;
      }
      /* Flags byte */
      /* blockb0[0] |= 0u | (((( (uint8_t) hcryp->Init.TagSize - 2) / 2) & 0x07 ) << 3 ) | ( ( (uint8_t) (15 - hcryp->Init.IVSize) - 1) & 0x07U) */
      blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)(((uint8_t)(hcryp->Init.TagSize - (uint8_t)(2U))) >> 1U) & (uint8_t)0x07U) << 3U);
      blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)((uint8_t)(15U) - hcryp->Init.IVSize) - (uint8_t)1U) & (uint8_t)0x07U);
      
      for (loopcounter = 0U; loopcounter < hcryp->Init.IVSize; loopcounter++)
      {
        blockb0[loopcounter+1U] = hcryp->Init.pInitVect[loopcounter];
      }
      for ( ; loopcounter < 13U; loopcounter++)
      {
        blockb0[loopcounter+1U] = 0U;
      }
      
      blockb0[14U] = (Size >> 8U);
      blockb0[15U] = (Size & 0xFFU);
      
      /************************* Formatting the initial counter ***************/
      /* Byte 0:
         Bits 7 and 6 are reserved and shall be set to 0
         Bits 3, 4, and 5 shall also be set to 0, to ensure that all the counter 
         blocks are distinct from B0
         Bits 0, 1, and 2 contain the same encoding of q as in B0
      */
      ctr[0U] = blockb0[0U] & 0x07U;
      /* byte 1 to NonceSize is the IV (Nonce) */
      for(loopcounter = 1; loopcounter < hcryp->Init.IVSize + 1U; loopcounter++)
      {
        ctr[loopcounter] = blockb0[loopcounter];
      }
      /* Set the LSB to 1 */
      ctr[15U] |= 0x01U;
      
      /* Set the key */
      CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
      
      /* Set the CRYP peripheral in AES CCM mode */
      __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_CCM_ENCRYPT);
      
      /* Set the Initialization Vector */
      CRYPEx_GCMCCM_SetInitVector(hcryp, ctr);
      
      /* Select init phase */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_INIT);
      
      b0addr = (uint32_t)blockb0;
      /* Write the blockb0 block in the IN FIFO */
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      
      /* Enable the CRYP peripheral */
      __HAL_CRYP_ENABLE(hcryp);
      
     /* Get tick */
     tickstart = HAL_GetTick();

      while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
      {
        /* Check for the Timeout */
        if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
        }
      }
      /***************************** Header phase *****************************/
      if(headersize != 0U)
      {
        /* Select header phase */
        __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_HEADER);
        
        /* Enable Crypto processor */
        __HAL_CRYP_ENABLE(hcryp);
        
        for(loopcounter = 0U; (loopcounter < headersize); loopcounter+=16U)
        {
         /* Get tick */
         tickstart = HAL_GetTick();

          while(HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_IFEM))
          {
            /* Check for the Timeout */
            if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
            {
              /* Change state */
              hcryp->State = HAL_CRYP_STATE_TIMEOUT;
              
              /* Process Unlocked */
              __HAL_UNLOCK(hcryp);
              
              return HAL_TIMEOUT;
            }
          }
          /* Write the header block in the IN FIFO */
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
        }

        /* Get tick */
        tickstart = HAL_GetTick();

        while((hcryp->Instance->SR & CRYP_FLAG_BUSY) == CRYP_FLAG_BUSY)
        {
          /* Check for the Timeout */
          if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
          {
            /* Change state */
            hcryp->State = HAL_CRYP_STATE_TIMEOUT;
            
            /* Process Unlocked */
            __HAL_UNLOCK(hcryp);
            
            return HAL_TIMEOUT;
          }
        }
      }
      /* Save formatted counter into the scratch buffer pScratch */
      for(loopcounter = 0U; (loopcounter < 16U); loopcounter++)
      {
        hcryp->Init.pScratch[loopcounter] = ctr[loopcounter];
      }
      /* Reset bit 0 */
      hcryp->Init.pScratch[15U] &= 0xFEU;
      
      /* Select payload phase once the header phase is performed */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
      
      /* Flush FIFO */
      __HAL_CRYP_FIFO_FLUSH(hcryp);
      
      /* Set the phase */
      hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
    }
    
    if(Size != 0U)
    {
      /* Enable Interrupts */
      __HAL_CRYP_ENABLE_IT(hcryp, CRYP_IT_INI | CRYP_IT_OUTI);
      /* Enable the CRYP peripheral */
      __HAL_CRYP_ENABLE(hcryp);
    }
    else
    {
      /* Change the CRYP state and phase */
      hcryp->State = HAL_CRYP_STATE_READY;
    }
    
    /* Return function status */
    return HAL_OK;
  }
  else if (__HAL_CRYP_GET_IT(hcryp, CRYP_IT_INI))
  {
    inputaddr = (uint32_t)hcryp->pCrypInBuffPtr;
    /* Write the Input block in the IN FIFO */
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR  = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    hcryp->pCrypInBuffPtr += 16U;
    hcryp->CrypInCount -= 16U;
    if(hcryp->CrypInCount == 0U)
    {
      __HAL_CRYP_DISABLE_IT(hcryp, CRYP_IT_INI);
      /* Call Input transfer complete callback */
      HAL_CRYP_InCpltCallback(hcryp);
    }
  }
  else if (__HAL_CRYP_GET_IT(hcryp, CRYP_IT_OUTI))
  {
    outputaddr = (uint32_t)hcryp->pCrypOutBuffPtr;
    /* Read the Output block from the Output FIFO */
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    hcryp->pCrypOutBuffPtr += 16U;
    hcryp->CrypOutCount -= 16U;
    if(hcryp->CrypOutCount == 0U)
    {
      __HAL_CRYP_DISABLE_IT(hcryp, CRYP_IT_OUTI);
      /* Process Unlocked */
      __HAL_UNLOCK(hcryp);
      /* Change the CRYP peripheral state */
      hcryp->State = HAL_CRYP_STATE_READY;
      /* Call Input transfer complete callback */
      HAL_CRYP_OutCpltCallback(hcryp);
    }
  }
  
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Initializes the CRYP peripheral in AES GCM decryption mode using IT.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pCypherData: Pointer to the cyphertext buffer
  * @param  Size: Length of the cyphertext buffer, must be a multiple of 16
  * @param  pPlainData: Pointer to the plaintext buffer
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESGCM_Decrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData)
{
  uint32_t tickstart = 0U;   
  uint32_t inputaddr;
  uint32_t outputaddr;
  
  if(hcryp->State == HAL_CRYP_STATE_READY)
  {
    /* Process Locked */
    __HAL_LOCK(hcryp);
    
    /* Get the buffer addresses and sizes */    
    hcryp->CrypInCount = Size;
    hcryp->pCrypInBuffPtr = pCypherData;
    hcryp->pCrypOutBuffPtr = pPlainData;
    hcryp->CrypOutCount = Size;
    
    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_BUSY;
    
    /* Check if initialization phase has already been performed */
    if(hcryp->Phase == HAL_CRYP_PHASE_READY)
    {
      /* Set the key */
      CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
      
      /* Set the CRYP peripheral in AES GCM decryption mode */
      __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_GCM_DECRYPT);
      
      /* Set the Initialization Vector */
      CRYPEx_GCMCCM_SetInitVector(hcryp, hcryp->Init.pInitVect);
      
      /* Flush FIFO */
      __HAL_CRYP_FIFO_FLUSH(hcryp);
      
      /* Enable CRYP to start the init phase */
      __HAL_CRYP_ENABLE(hcryp);

        /* Get tick */
        tickstart = HAL_GetTick();

      while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
      {
        /* Check for the Timeout */
        if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
        }
      }
      
      /* Set the header phase */
      if(CRYPEx_GCMCCM_SetHeaderPhase(hcryp, hcryp->Init.Header, hcryp->Init.HeaderSize, 1U) != HAL_OK)
      {
        return HAL_TIMEOUT;
      }
      /* Disable the CRYP peripheral */
      __HAL_CRYP_DISABLE(hcryp);
      
      /* Select payload phase once the header phase is performed */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
      
      /* Set the phase */
      hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
    }
    
    if(Size != 0U)
    {
      /* Enable Interrupts */
      __HAL_CRYP_ENABLE_IT(hcryp, CRYP_IT_INI | CRYP_IT_OUTI);
      /* Enable the CRYP peripheral */
      __HAL_CRYP_ENABLE(hcryp);
    }
    else
    {
      /* Process Locked */
      __HAL_UNLOCK(hcryp);
      /* Change the CRYP state and phase */
      hcryp->State = HAL_CRYP_STATE_READY;
    }
    
    /* Return function status */
    return HAL_OK;
  }
  else if (__HAL_CRYP_GET_IT(hcryp, CRYP_IT_INI))
  {
    inputaddr = (uint32_t)hcryp->pCrypInBuffPtr;
    /* Write the Input block in the IN FIFO */
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR  = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    hcryp->pCrypInBuffPtr += 16U;
    hcryp->CrypInCount -= 16U;
    if(hcryp->CrypInCount == 0U)
    {
      __HAL_CRYP_DISABLE_IT(hcryp, CRYP_IT_INI);
      /* Call the Input data transfer complete callback */
      HAL_CRYP_InCpltCallback(hcryp);
    }
  }
  else if (__HAL_CRYP_GET_IT(hcryp, CRYP_IT_OUTI))
  {
    outputaddr = (uint32_t)hcryp->pCrypOutBuffPtr;
    /* Read the Output block from the Output FIFO */
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    hcryp->pCrypOutBuffPtr += 16U;
    hcryp->CrypOutCount -= 16U;
    if(hcryp->CrypOutCount == 0U)
    {
      __HAL_CRYP_DISABLE_IT(hcryp, CRYP_IT_OUTI);
      /* Process Unlocked */
      __HAL_UNLOCK(hcryp);
      /* Change the CRYP peripheral state */
      hcryp->State = HAL_CRYP_STATE_READY;
      /* Call Input transfer complete callback */
      HAL_CRYP_OutCpltCallback(hcryp);
    }
  }
  
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Initializes the CRYP peripheral in AES CCM decryption mode using interrupt
  *         then decrypted pCypherData. The cypher data are available in pPlainData.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pCypherData: Pointer to the cyphertext buffer 
  * @param  Size: Length of the plaintext buffer, must be a multiple of 16
  * @param  pPlainData: Pointer to the plaintext buffer  
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESCCM_Decrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData)
{
  uint32_t inputaddr;
  uint32_t outputaddr;
  uint32_t tickstart = 0U;
  uint32_t headersize = hcryp->Init.HeaderSize;
  uint32_t headeraddr = (uint32_t)hcryp->Init.Header;
  uint32_t loopcounter = 0U;
  uint32_t bufferidx = 0U;
  uint8_t blockb0[16U] = {0U};/* Block B0 */
  uint8_t ctr[16U] = {0U}; /* Counter */
  uint32_t b0addr = (uint32_t)blockb0;
  
  if(hcryp->State == HAL_CRYP_STATE_READY)
  {
    /* Process Locked */
    __HAL_LOCK(hcryp);
    
    hcryp->CrypInCount = Size;
    hcryp->pCrypInBuffPtr = pCypherData;
    hcryp->pCrypOutBuffPtr = pPlainData;
    hcryp->CrypOutCount = Size;
    
    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_BUSY;
    
    /* Check if initialization phase has already been performed */
    if(hcryp->Phase == HAL_CRYP_PHASE_READY)
    {
      /************************ Formatting the header block *******************/
      if(headersize != 0U)
      {
        /* Check that the associated data (or header) length is lower than 2^16 - 2^8 = 65536 - 256 = 65280 */
        if(headersize < 65280U)
        {
          hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize >> 8U) & 0xFFU);
          hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize) & 0xFFU);
          headersize += 2U;
        }
        else
        {
          /* Header is encoded as 0xff || 0xfe || [headersize]32, i.e., six octets */
          hcryp->Init.pScratch[bufferidx++] = 0xFFU;
          hcryp->Init.pScratch[bufferidx++] = 0xFEU;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0xff000000U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x00ff0000U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x0000ff00U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x000000ffU;
          headersize += 6U;
        }
        /* Copy the header buffer in internal buffer "hcryp->Init.pScratch" */
        for(loopcounter = 0U; loopcounter < headersize; loopcounter++)
        {
          hcryp->Init.pScratch[bufferidx++] = hcryp->Init.Header[loopcounter];
        }
        /* Check if the header size is modulo 16 */
        if ((headersize % 16U) != 0U)
        {
          /* Padd the header buffer with 0s till the hcryp->Init.pScratch length is modulo 16 */
          for(loopcounter = headersize; loopcounter <= ((headersize/16U) + 1U) * 16U; loopcounter++)
          {
            hcryp->Init.pScratch[loopcounter] = 0U;
          }
          /* Set the header size to modulo 16 */
          headersize = ((headersize/16U) + 1U) * 16U;
        }
        /* Set the pointer headeraddr to hcryp->Init.pScratch */
        headeraddr = (uint32_t)hcryp->Init.pScratch;
      }
      /*********************** Formatting the block B0 ************************/
      if(headersize != 0U)
      {
        blockb0[0U] = 0x40U;
      }
      /* Flags byte */
      /* blockb0[0] |= 0u | (((( (uint8_t) hcryp->Init.TagSize - 2) / 2) & 0x07 ) << 3 ) | ( ( (uint8_t) (15 - hcryp->Init.IVSize) - 1) & 0x07U) */
      blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)(((uint8_t)(hcryp->Init.TagSize - (uint8_t)(2U))) >> 1U) & (uint8_t)0x07U) << 3U);
      blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)((uint8_t)(15U) - hcryp->Init.IVSize) - (uint8_t)1U) & (uint8_t)0x07U);
      
      for (loopcounter = 0U; loopcounter < hcryp->Init.IVSize; loopcounter++)
      {
        blockb0[loopcounter+1U] = hcryp->Init.pInitVect[loopcounter];
      }
      for ( ; loopcounter < 13U; loopcounter++)
      {
        blockb0[loopcounter+1U] = 0U;
      }
      
      blockb0[14U] = (Size >> 8U);
      blockb0[15U] = (Size & 0xFFU);
      
      /************************* Formatting the initial counter ***************/
      /* Byte 0:
         Bits 7 and 6 are reserved and shall be set to 0
         Bits 3, 4, and 5 shall also be set to 0, to ensure that all the counter 
         blocks are distinct from B0
         Bits 0, 1, and 2 contain the same encoding of q as in B0
      */
      ctr[0U] = blockb0[0U] & 0x07U;
      /* byte 1 to NonceSize is the IV (Nonce) */
      for(loopcounter = 1U; loopcounter < hcryp->Init.IVSize + 1U; loopcounter++)
      {
        ctr[loopcounter] = blockb0[loopcounter];
      }
      /* Set the LSB to 1 */
      ctr[15U] |= 0x01U;
      
      /* Set the key */
      CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
      
      /* Set the CRYP peripheral in AES CCM mode */
      __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_CCM_DECRYPT);
      
      /* Set the Initialization Vector */
      CRYPEx_GCMCCM_SetInitVector(hcryp, ctr);
      
      /* Select init phase */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_INIT);
      
      b0addr = (uint32_t)blockb0;
      /* Write the blockb0 block in the IN FIFO */
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      
      /* Enable the CRYP peripheral */
      __HAL_CRYP_ENABLE(hcryp);

      /* Get tick */
      tickstart = HAL_GetTick();

      while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
      {
        /* Check for the Timeout */
        if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
        }
      }
      /***************************** Header phase *****************************/
      if(headersize != 0U)
      {
        /* Select header phase */
        __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_HEADER);
        
        /* Enable Crypto processor */
        __HAL_CRYP_ENABLE(hcryp);
        
        for(loopcounter = 0U; (loopcounter < headersize); loopcounter+=16U)
        {
         /* Get tick */
         tickstart = HAL_GetTick();

          while(HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_IFEM))
          {
            /* Check for the Timeout */
            if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
            {
              /* Change state */
              hcryp->State = HAL_CRYP_STATE_TIMEOUT;
              
              /* Process Unlocked */
              __HAL_UNLOCK(hcryp);
              
              return HAL_TIMEOUT;
            }
          }
          /* Write the header block in the IN FIFO */
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
        }

        /* Get tick */
        tickstart = HAL_GetTick();

        while((hcryp->Instance->SR & CRYP_FLAG_BUSY) == CRYP_FLAG_BUSY)
        {
          /* Check for the Timeout */
          if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
          {
            /* Change state */
            hcryp->State = HAL_CRYP_STATE_TIMEOUT;
            
            /* Process Unlocked */
            __HAL_UNLOCK(hcryp);
            
            return HAL_TIMEOUT;
          }
        }
      }
      /* Save formatted counter into the scratch buffer pScratch */
      for(loopcounter = 0U; (loopcounter < 16U); loopcounter++)
      {
        hcryp->Init.pScratch[loopcounter] = ctr[loopcounter];
      }
      /* Reset bit 0 */
      hcryp->Init.pScratch[15U] &= 0xFEU;
      /* Select payload phase once the header phase is performed */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
      
      /* Flush FIFO */
      __HAL_CRYP_FIFO_FLUSH(hcryp);
      
      /* Set the phase */
      hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
    }
    
    /* Enable Interrupts */
    __HAL_CRYP_ENABLE_IT(hcryp, CRYP_IT_INI | CRYP_IT_OUTI);
    
    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);
    
    /* Return function status */
    return HAL_OK;
  }
  else if (__HAL_CRYP_GET_IT(hcryp, CRYP_IT_INI))
  {
    inputaddr = (uint32_t)hcryp->pCrypInBuffPtr;
    /* Write the Input block in the IN FIFO */
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR  = *(uint32_t*)(inputaddr);
    inputaddr+=4U;
    hcryp->Instance->DR = *(uint32_t*)(inputaddr);
    hcryp->pCrypInBuffPtr += 16U;
    hcryp->CrypInCount -= 16U;
    if(hcryp->CrypInCount == 0U)
    {
      __HAL_CRYP_DISABLE_IT(hcryp, CRYP_IT_INI);
      /* Call the Input data transfer complete callback */
      HAL_CRYP_InCpltCallback(hcryp);
    }
  }
  else if (__HAL_CRYP_GET_IT(hcryp, CRYP_IT_OUTI))
  {
    outputaddr = (uint32_t)hcryp->pCrypOutBuffPtr;
    /* Read the Output block from the Output FIFO */
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    outputaddr+=4U;
    *(uint32_t*)(outputaddr) = hcryp->Instance->DOUT;
    hcryp->pCrypOutBuffPtr += 16U;
    hcryp->CrypOutCount -= 16U;
    if(hcryp->CrypOutCount == 0U)
    {
      __HAL_CRYP_DISABLE_IT(hcryp, CRYP_IT_OUTI);
      /* Process Unlocked */
      __HAL_UNLOCK(hcryp);
      /* Change the CRYP peripheral state */
      hcryp->State = HAL_CRYP_STATE_READY;
      /* Call Input transfer complete callback */
      HAL_CRYP_OutCpltCallback(hcryp);
    }
  }
  
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Initializes the CRYP peripheral in AES GCM encryption mode using DMA.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pPlainData: Pointer to the plaintext buffer
  * @param  Size: Length of the plaintext buffer, must be a multiple of 16
  * @param  pCypherData: Pointer to the cyphertext buffer
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESGCM_Encrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData)
{
  uint32_t tickstart = 0U;
  uint32_t inputaddr;
  uint32_t outputaddr;
  
  if((hcryp->State == HAL_CRYP_STATE_READY) || (hcryp->Phase == HAL_CRYP_PHASE_PROCESS))
  {
    /* Process Locked */
    __HAL_LOCK(hcryp);
    
    inputaddr  = (uint32_t)pPlainData;
    outputaddr = (uint32_t)pCypherData;
    
    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_BUSY;
    
    /* Check if initialization phase has already been performed */
    if(hcryp->Phase == HAL_CRYP_PHASE_READY)
    {
      /* Set the key */
      CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
      
      /* Set the CRYP peripheral in AES GCM mode */
      __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_GCM_ENCRYPT);
      
      /* Set the Initialization Vector */
      CRYPEx_GCMCCM_SetInitVector(hcryp, hcryp->Init.pInitVect);
      
      /* Flush FIFO */
      __HAL_CRYP_FIFO_FLUSH(hcryp);
      
      /* Enable CRYP to start the init phase */
      __HAL_CRYP_ENABLE(hcryp);
      
      /* Get tick */
      tickstart = HAL_GetTick();

      while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
      {
        /* Check for the Timeout */
        if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
        }
      }
      /* Flush FIFO */
      __HAL_CRYP_FIFO_FLUSH(hcryp);
      
      /* Set the header phase */
      if(CRYPEx_GCMCCM_SetHeaderPhase(hcryp, hcryp->Init.Header, hcryp->Init.HeaderSize, 1U) != HAL_OK)
      {
        return HAL_TIMEOUT;
      }
      /* Disable the CRYP peripheral */
      __HAL_CRYP_DISABLE(hcryp);
      
      /* Select payload phase once the header phase is performed */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
      
      /* Flush FIFO */
      __HAL_CRYP_FIFO_FLUSH(hcryp);
      
      /* Set the phase */
      hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
    }
    
    /* Set the input and output addresses and start DMA transfer */ 
    CRYPEx_GCMCCM_SetDMAConfig(hcryp, inputaddr, Size, outputaddr);
    
    /* Unlock process */
    __HAL_UNLOCK(hcryp);
    
    /* Return function status */
    return HAL_OK;
  }
  else
  {
    return HAL_ERROR;   
  }
}

/**
  * @brief  Initializes the CRYP peripheral in AES CCM encryption mode using interrupt.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pPlainData: Pointer to the plaintext buffer
  * @param  Size: Length of the plaintext buffer, must be a multiple of 16
  * @param  pCypherData: Pointer to the cyphertext buffer
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESCCM_Encrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData)
{
  uint32_t tickstart = 0U;   
  uint32_t inputaddr;
  uint32_t outputaddr;
  uint32_t headersize;
  uint32_t headeraddr;
  uint32_t loopcounter = 0U;
  uint32_t bufferidx = 0U;
  uint8_t blockb0[16U] = {0U};/* Block B0 */
  uint8_t ctr[16U] = {0U}; /* Counter */
  uint32_t b0addr = (uint32_t)blockb0;
  
  if((hcryp->State == HAL_CRYP_STATE_READY) || (hcryp->Phase == HAL_CRYP_PHASE_PROCESS))
  {
    /* Process Locked */
    __HAL_LOCK(hcryp);
    
    inputaddr  = (uint32_t)pPlainData;
    outputaddr = (uint32_t)pCypherData;
    
    headersize = hcryp->Init.HeaderSize;
    headeraddr = (uint32_t)hcryp->Init.Header;
    
    hcryp->CrypInCount = Size;
    hcryp->pCrypInBuffPtr = pPlainData;
    hcryp->pCrypOutBuffPtr = pCypherData;
    hcryp->CrypOutCount = Size;
    
    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_BUSY;
    
    /* Check if initialization phase has already been performed */
    if(hcryp->Phase == HAL_CRYP_PHASE_READY)
    {
      /************************ Formatting the header block *******************/
      if(headersize != 0U)
      {
        /* Check that the associated data (or header) length is lower than 2^16 - 2^8 = 65536 - 256 = 65280 */
        if(headersize < 65280U)
        {
          hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize >> 8U) & 0xFFU);
          hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize) & 0xFFU);
          headersize += 2U;
        }
        else
        {
          /* Header is encoded as 0xff || 0xfe || [headersize]32, i.e., six octets */
          hcryp->Init.pScratch[bufferidx++] = 0xFFU;
          hcryp->Init.pScratch[bufferidx++] = 0xFEU;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0xff000000U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x00ff0000U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x0000ff00U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x000000ffU;
          headersize += 6U;
        }
        /* Copy the header buffer in internal buffer "hcryp->Init.pScratch" */
        for(loopcounter = 0U; loopcounter < headersize; loopcounter++)
        {
          hcryp->Init.pScratch[bufferidx++] = hcryp->Init.Header[loopcounter];
        }
        /* Check if the header size is modulo 16 */
        if ((headersize % 16U) != 0U)
        {
          /* Padd the header buffer with 0s till the hcryp->Init.pScratch length is modulo 16 */
          for(loopcounter = headersize; loopcounter <= ((headersize/16U) + 1U) * 16U; loopcounter++)
          {
            hcryp->Init.pScratch[loopcounter] = 0U;
          }
          /* Set the header size to modulo 16 */
          headersize = ((headersize/16U) + 1U) * 16U;
        }
        /* Set the pointer headeraddr to hcryp->Init.pScratch */
        headeraddr = (uint32_t)hcryp->Init.pScratch;
      }
      /*********************** Formatting the block B0 ************************/
      if(headersize != 0U)
      {
        blockb0[0U] = 0x40U;
      }
      /* Flags byte */
      /* blockb0[0] |= 0u | (((( (uint8_t) hcryp->Init.TagSize - 2) / 2) & 0x07 ) << 3 ) | ( ( (uint8_t) (15 - hcryp->Init.IVSize) - 1) & 0x07U) */
      blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)(((uint8_t)(hcryp->Init.TagSize - (uint8_t)(2U))) >> 1U) & (uint8_t)0x07U) << 3U);
      blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)((uint8_t)(15U) - hcryp->Init.IVSize) - (uint8_t)1U) & (uint8_t)0x07U);
      
      for (loopcounter = 0U; loopcounter < hcryp->Init.IVSize; loopcounter++)
      {
        blockb0[loopcounter+1U] = hcryp->Init.pInitVect[loopcounter];
      }
      for ( ; loopcounter < 13U; loopcounter++)
      {
        blockb0[loopcounter+1U] = 0U;
      }
      
      blockb0[14U] = (Size >> 8U);
      blockb0[15U] = (Size & 0xFFU);
      
      /************************* Formatting the initial counter ***************/
      /* Byte 0:
         Bits 7 and 6 are reserved and shall be set to 0
         Bits 3, 4, and 5 shall also be set to 0, to ensure that all the counter 
         blocks are distinct from B0
         Bits 0, 1, and 2 contain the same encoding of q as in B0
      */
      ctr[0U] = blockb0[0U] & 0x07U;
      /* byte 1 to NonceSize is the IV (Nonce) */
      for(loopcounter = 1U; loopcounter < hcryp->Init.IVSize + 1U; loopcounter++)
      {
        ctr[loopcounter] = blockb0[loopcounter];
      }
      /* Set the LSB to 1 */
      ctr[15U] |= 0x01U;
      
      /* Set the key */
      CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
      
      /* Set the CRYP peripheral in AES CCM mode */
      __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_CCM_ENCRYPT);
      
      /* Set the Initialization Vector */
      CRYPEx_GCMCCM_SetInitVector(hcryp, ctr);
      
      /* Select init phase */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_INIT);
      
      b0addr = (uint32_t)blockb0;
      /* Write the blockb0 block in the IN FIFO */
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      
      /* Enable the CRYP peripheral */
      __HAL_CRYP_ENABLE(hcryp);
      
      /* Get tick */
      tickstart = HAL_GetTick();
 
      while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
      {
        /* Check for the Timeout */
        if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
        }
      }
      /***************************** Header phase *****************************/
      if(headersize != 0U)
      {
        /* Select header phase */
        __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_HEADER);
        
        /* Enable Crypto processor */
        __HAL_CRYP_ENABLE(hcryp);
        
        for(loopcounter = 0U; (loopcounter < headersize); loopcounter+=16U)
        {
         /* Get tick */
         tickstart = HAL_GetTick();

          while(HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_IFEM))
          {
            /* Check for the Timeout */
            if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
            {
              /* Change state */
              hcryp->State = HAL_CRYP_STATE_TIMEOUT;
              
              /* Process Unlocked */
              __HAL_UNLOCK(hcryp);
              
              return HAL_TIMEOUT;
            }
          }
          /* Write the header block in the IN FIFO */
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
        }
        
        /* Get tick */
        tickstart = HAL_GetTick();

        while((hcryp->Instance->SR & CRYP_FLAG_BUSY) == CRYP_FLAG_BUSY)
        {
          /* Check for the Timeout */
          if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
          {
            /* Change state */
            hcryp->State = HAL_CRYP_STATE_TIMEOUT;
            
            /* Process Unlocked */
            __HAL_UNLOCK(hcryp);
            
            return HAL_TIMEOUT;
          }
        }
      }
      /* Save formatted counter into the scratch buffer pScratch */
      for(loopcounter = 0U; (loopcounter < 16U); loopcounter++)
      {
        hcryp->Init.pScratch[loopcounter] = ctr[loopcounter];
      }
      /* Reset bit 0 */
      hcryp->Init.pScratch[15U] &= 0xFEU;
      
      /* Select payload phase once the header phase is performed */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
      
      /* Flush FIFO */
      __HAL_CRYP_FIFO_FLUSH(hcryp);
      
      /* Set the phase */
      hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
    }
    
    /* Set the input and output addresses and start DMA transfer */ 
    CRYPEx_GCMCCM_SetDMAConfig(hcryp, inputaddr, Size, outputaddr);
    
    /* Unlock process */
    __HAL_UNLOCK(hcryp);
    
    /* Return function status */
    return HAL_OK;
  }
  else
  {
    return HAL_ERROR;   
  }
}

/**
  * @brief  Initializes the CRYP peripheral in AES GCM decryption mode using DMA.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pCypherData: Pointer to the cyphertext buffer.
  * @param  Size: Length of the cyphertext buffer, must be a multiple of 16
  * @param  pPlainData: Pointer to the plaintext buffer
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESGCM_Decrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData)
{
  uint32_t tickstart = 0U;   
  uint32_t inputaddr;
  uint32_t outputaddr;
  
  if((hcryp->State == HAL_CRYP_STATE_READY) || (hcryp->Phase == HAL_CRYP_PHASE_PROCESS))
  {
    /* Process Locked */
    __HAL_LOCK(hcryp);
    
    inputaddr  = (uint32_t)pCypherData;
    outputaddr = (uint32_t)pPlainData;
    
    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_BUSY;
    
    /* Check if initialization phase has already been performed */
    if(hcryp->Phase == HAL_CRYP_PHASE_READY)
    {
      /* Set the key */
      CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
      
      /* Set the CRYP peripheral in AES GCM decryption mode */
      __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_GCM_DECRYPT);
      
      /* Set the Initialization Vector */
      CRYPEx_GCMCCM_SetInitVector(hcryp, hcryp->Init.pInitVect);
      
      /* Enable CRYP to start the init phase */
      __HAL_CRYP_ENABLE(hcryp);
      
      /* Get tick */
      tickstart = HAL_GetTick();

      while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
      {
        /* Check for the Timeout */
        if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
        }
      }
      
      /* Set the header phase */
      if(CRYPEx_GCMCCM_SetHeaderPhase(hcryp, hcryp->Init.Header, hcryp->Init.HeaderSize, 1U) != HAL_OK)
      {
        return HAL_TIMEOUT;
      }
      /* Disable the CRYP peripheral */
      __HAL_CRYP_DISABLE(hcryp);
      
      /* Select payload phase once the header phase is performed */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
      
      /* Set the phase */
      hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
    }
    
    /* Set the input and output addresses and start DMA transfer */ 
    CRYPEx_GCMCCM_SetDMAConfig(hcryp, inputaddr, Size, outputaddr);
    
    /* Unlock process */
    __HAL_UNLOCK(hcryp);
    
    /* Return function status */
    return HAL_OK;
  }
  else
  {
    return HAL_ERROR;   
  }
}

/**
  * @brief  Initializes the CRYP peripheral in AES CCM decryption mode using DMA
  *         then decrypted pCypherData. The cypher data are available in pPlainData.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  pCypherData: Pointer to the cyphertext buffer  
  * @param  Size: Length of the plaintext buffer, must be a multiple of 16
  * @param  pPlainData: Pointer to the plaintext buffer  
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESCCM_Decrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData)
{
  uint32_t tickstart = 0U;   
  uint32_t inputaddr;
  uint32_t outputaddr;
  uint32_t headersize;
  uint32_t headeraddr;
  uint32_t loopcounter = 0U;
  uint32_t bufferidx = 0U;
  uint8_t blockb0[16U] = {0U};/* Block B0 */
  uint8_t ctr[16U] = {0U}; /* Counter */
  uint32_t b0addr = (uint32_t)blockb0;
  
  if((hcryp->State == HAL_CRYP_STATE_READY) || (hcryp->Phase == HAL_CRYP_PHASE_PROCESS))
  {
    /* Process Locked */
    __HAL_LOCK(hcryp);
    
    inputaddr  = (uint32_t)pCypherData;
    outputaddr = (uint32_t)pPlainData;
    
    headersize = hcryp->Init.HeaderSize;
    headeraddr = (uint32_t)hcryp->Init.Header;
    
    hcryp->CrypInCount = Size;
    hcryp->pCrypInBuffPtr = pCypherData;
    hcryp->pCrypOutBuffPtr = pPlainData;
    hcryp->CrypOutCount = Size;
    
    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_BUSY;
    
    /* Check if initialization phase has already been performed */
    if(hcryp->Phase == HAL_CRYP_PHASE_READY)
    {
      /************************ Formatting the header block *******************/
      if(headersize != 0U)
      {
        /* Check that the associated data (or header) length is lower than 2^16 - 2^8 = 65536 - 256 = 65280 */
        if(headersize < 65280U)
        {
          hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize >> 8U) & 0xFFU);
          hcryp->Init.pScratch[bufferidx++] = (uint8_t) ((headersize) & 0xFFU);
          headersize += 2U;
        }
        else
        {
          /* Header is encoded as 0xff || 0xfe || [headersize]32, i.e., six octets */
          hcryp->Init.pScratch[bufferidx++] = 0xFFU;
          hcryp->Init.pScratch[bufferidx++] = 0xFEU;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0xff000000U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x00ff0000U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x0000ff00U;
          hcryp->Init.pScratch[bufferidx++] = headersize & 0x000000ffU;
          headersize += 6U;
        }
        /* Copy the header buffer in internal buffer "hcryp->Init.pScratch" */
        for(loopcounter = 0U; loopcounter < headersize; loopcounter++)
        {
          hcryp->Init.pScratch[bufferidx++] = hcryp->Init.Header[loopcounter];
        }
        /* Check if the header size is modulo 16 */
        if ((headersize % 16U) != 0U)
        {
          /* Padd the header buffer with 0s till the hcryp->Init.pScratch length is modulo 16 */
          for(loopcounter = headersize; loopcounter <= ((headersize/16U) + 1U) * 16U; loopcounter++)
          {
            hcryp->Init.pScratch[loopcounter] = 0U;
          }
          /* Set the header size to modulo 16 */
          headersize = ((headersize/16U) + 1U) * 16U;
        }
        /* Set the pointer headeraddr to hcryp->Init.pScratch */
        headeraddr = (uint32_t)hcryp->Init.pScratch;
      }
      /*********************** Formatting the block B0 ************************/
      if(headersize != 0U)
      {
        blockb0[0U] = 0x40U;
      }
      /* Flags byte */
      /* blockb0[0] |= 0u | (((( (uint8_t) hcryp->Init.TagSize - 2) / 2) & 0x07 ) << 3 ) | ( ( (uint8_t) (15 - hcryp->Init.IVSize) - 1) & 0x07U) */
      blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)(((uint8_t)(hcryp->Init.TagSize - (uint8_t)(2U))) >> 1U) & (uint8_t)0x07U) << 3U);
      blockb0[0U] |= (uint8_t)((uint8_t)((uint8_t)((uint8_t)(15U) - hcryp->Init.IVSize) - (uint8_t)1U) & (uint8_t)0x07U);
      
      for (loopcounter = 0U; loopcounter < hcryp->Init.IVSize; loopcounter++)
      {
        blockb0[loopcounter+1U] = hcryp->Init.pInitVect[loopcounter];
      }
      for ( ; loopcounter < 13U; loopcounter++)
      {
        blockb0[loopcounter+1U] = 0U;
      }
      
      blockb0[14U] = (Size >> 8U);
      blockb0[15U] = (Size & 0xFFU);
      
      /************************* Formatting the initial counter ***************/
      /* Byte 0:
         Bits 7 and 6 are reserved and shall be set to 0
         Bits 3, 4, and 5 shall also be set to 0, to ensure that all the counter 
         blocks are distinct from B0
         Bits 0, 1, and 2 contain the same encoding of q as in B0
      */
      ctr[0U] = blockb0[0U] & 0x07U;
      /* byte 1 to NonceSize is the IV (Nonce) */
      for(loopcounter = 1U; loopcounter < hcryp->Init.IVSize + 1U; loopcounter++)
      {
        ctr[loopcounter] = blockb0[loopcounter];
      }
      /* Set the LSB to 1 */
      ctr[15U] |= 0x01U;
      
      /* Set the key */
      CRYPEx_GCMCCM_SetKey(hcryp, hcryp->Init.pKey, hcryp->Init.KeySize);
      
      /* Set the CRYP peripheral in AES CCM mode */
      __HAL_CRYP_SET_MODE(hcryp, CRYP_CR_ALGOMODE_AES_CCM_DECRYPT);
      
      /* Set the Initialization Vector */
      CRYPEx_GCMCCM_SetInitVector(hcryp, ctr);
      
      /* Select init phase */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_INIT);
      
      b0addr = (uint32_t)blockb0;
      /* Write the blockb0 block in the IN FIFO */
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      b0addr+=4U;
      hcryp->Instance->DR = *(uint32_t*)(b0addr);
      
      /* Enable the CRYP peripheral */
      __HAL_CRYP_ENABLE(hcryp);
      
      /* Get tick */
      tickstart = HAL_GetTick();
 
      while((CRYP->CR & CRYP_CR_CRYPEN) == CRYP_CR_CRYPEN)
      {
        /* Check for the Timeout */
        
        if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
        {
          /* Change state */
          hcryp->State = HAL_CRYP_STATE_TIMEOUT;
          
          /* Process Unlocked */
          __HAL_UNLOCK(hcryp);
          
          return HAL_TIMEOUT;
          
        }
      }
      /***************************** Header phase *****************************/
      if(headersize != 0U)
      {
        /* Select header phase */
        __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_HEADER);
        
        /* Enable Crypto processor */
        __HAL_CRYP_ENABLE(hcryp);
        
        for(loopcounter = 0U; (loopcounter < headersize); loopcounter+=16U)
        {
         /* Get tick */
         tickstart = HAL_GetTick();
 
          while(HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_IFEM))
          {
            /* Check for the Timeout */
            if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
            {
              /* Change state */
              hcryp->State = HAL_CRYP_STATE_TIMEOUT;
              
              /* Process Unlocked */
              __HAL_UNLOCK(hcryp);
              
              return HAL_TIMEOUT;
            }
          }
          /* Write the header block in the IN FIFO */
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
          hcryp->Instance->DR = *(uint32_t*)(headeraddr);
          headeraddr+=4U;
        }
        
        /* Get tick */
        tickstart = HAL_GetTick();

        while((hcryp->Instance->SR & CRYP_FLAG_BUSY) == CRYP_FLAG_BUSY)
        {
          /* Check for the Timeout */
          if((HAL_GetTick() - tickstart ) > CRYPEx_TIMEOUT_VALUE)
          {
            /* Change state */
            hcryp->State = HAL_CRYP_STATE_TIMEOUT;
            
            /* Process Unlocked */
            __HAL_UNLOCK(hcryp);
            
            return HAL_TIMEOUT;
          }
        }
      }
      /* Save formatted counter into the scratch buffer pScratch */
      for(loopcounter = 0U; (loopcounter < 16U); loopcounter++)
      {
        hcryp->Init.pScratch[loopcounter] = ctr[loopcounter];
      }
      /* Reset bit 0 */
      hcryp->Init.pScratch[15U] &= 0xFEU;
      /* Select payload phase once the header phase is performed */
      __HAL_CRYP_SET_PHASE(hcryp, CRYP_PHASE_PAYLOAD);
      
      /* Flush FIFO */
      __HAL_CRYP_FIFO_FLUSH(hcryp);
      
      /* Set the phase */
      hcryp->Phase = HAL_CRYP_PHASE_PROCESS;
    }
    /* Set the input and output addresses and start DMA transfer */ 
    CRYPEx_GCMCCM_SetDMAConfig(hcryp, inputaddr, Size, outputaddr);
    
    /* Unlock process */
    __HAL_UNLOCK(hcryp);
    
    /* Return function status */
    return HAL_OK;
  }
  else
  {
    return HAL_ERROR;   
  }
}

/**
  * @}
  */
  
/** @defgroup CRYPEx_Exported_Functions_Group2 CRYPEx IRQ handler management  
 *  @brief   CRYPEx IRQ handler.
 *
@verbatim   
  ==============================================================================
                ##### CRYPEx IRQ handler management #####
  ==============================================================================  
[..]  This section provides CRYPEx IRQ handler function.

@endverbatim
  * @{
  */

/**
  * @brief  This function handles CRYPEx interrupt request.
  * @param  hcryp: pointer to a CRYPEx_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @retval None
  */

void HAL_CRYPEx_GCMCCM_IRQHandler(CRYP_HandleTypeDef *hcryp)
{
  switch(CRYP->CR & CRYP_CR_ALGOMODE_DIRECTION)
  {    
  case CRYP_CR_ALGOMODE_AES_GCM_ENCRYPT:
    HAL_CRYPEx_AESGCM_Encrypt_IT(hcryp, NULL, 0U, NULL);
    break;
    
  case CRYP_CR_ALGOMODE_AES_GCM_DECRYPT:
    HAL_CRYPEx_AESGCM_Decrypt_IT(hcryp, NULL, 0U, NULL);
    break;
    
  case CRYP_CR_ALGOMODE_AES_CCM_ENCRYPT:
    HAL_CRYPEx_AESCCM_Encrypt_IT(hcryp, NULL, 0U, NULL);
    break;
    
  case CRYP_CR_ALGOMODE_AES_CCM_DECRYPT:
    HAL_CRYPEx_AESCCM_Decrypt_IT(hcryp, NULL, 0U, NULL);
    break;
    
  default:
    break;
  }
}

/**
  * @}
  */

/**
  * @}
  */
#endif /* STM32F437xx || STM32F439xx || STM32F479xx */

#endif /* HAL_CRYP_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
