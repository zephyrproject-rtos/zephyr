/**
  ******************************************************************************
  * @file    stm32f4xx_hal_cryp.h
  * @author  MCD Application Team
  * @version V1.5.1
  * @date    01-July-2016
  * @brief   Header file of CRYP HAL module.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_HAL_CRYP_H
#define __STM32F4xx_HAL_CRYP_H

#ifdef __cplusplus
 extern "C" {
#endif

#if defined(STM32F415xx) || defined(STM32F417xx) || defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F479xx)
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal_def.h"

/** @addtogroup STM32F4xx_HAL_Driver
  * @{
  */

/** @addtogroup CRYP
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup CRYP_Exported_Types CRYP Exported Types
  * @{
  */

/** @defgroup CRYP_Exported_Types_Group1 CRYP Configuration Structure definition
  * @{
  */

typedef struct
{
  uint32_t DataType;    /*!< 32-bit data, 16-bit data, 8-bit data or 1-bit string.
                             This parameter can be a value of @ref CRYP_Data_Type */

  uint32_t KeySize;     /*!< Used only in AES mode only : 128, 192 or 256 bit key length. 
                             This parameter can be a value of @ref CRYP_Key_Size */

  uint8_t* pKey;        /*!< The key used for encryption/decryption */

  uint8_t* pInitVect;   /*!< The initialization vector used also as initialization
                             counter in CTR mode */

  uint8_t IVSize;       /*!< The size of initialization vector. 
                             This parameter (called nonce size in CCM) is used only 
                             in AES-128/192/256 encryption/decryption CCM mode */

  uint8_t TagSize;      /*!< The size of returned authentication TAG. 
                             This parameter is used only in AES-128/192/256 
                             encryption/decryption CCM mode */

  uint8_t* Header;      /*!< The header used in GCM and CCM modes */

  uint32_t HeaderSize;  /*!< The size of header buffer in bytes */

  uint8_t* pScratch;    /*!< Scratch buffer used to append the header. It's size must be equal to header size + 21 bytes.
                             This parameter is used only in AES-128/192/256 encryption/decryption CCM mode */
}CRYP_InitTypeDef;

/** 
  * @}
  */

/** @defgroup CRYP_Exported_Types_Group2 CRYP State structures definition
  * @{
  */
    

typedef enum
{
  HAL_CRYP_STATE_RESET             = 0x00U,  /*!< CRYP not yet initialized or disabled  */
  HAL_CRYP_STATE_READY             = 0x01U,  /*!< CRYP initialized and ready for use    */
  HAL_CRYP_STATE_BUSY              = 0x02U,  /*!< CRYP internal processing is ongoing   */
  HAL_CRYP_STATE_TIMEOUT           = 0x03U,  /*!< CRYP timeout state                    */
  HAL_CRYP_STATE_ERROR             = 0x04U   /*!< CRYP error state                      */
}HAL_CRYP_STATETypeDef;

/** 
  * @}
  */
  
/** @defgroup CRYP_Exported_Types_Group3 CRYP phase structures definition
  * @{
  */
    

typedef enum
{
  HAL_CRYP_PHASE_READY             = 0x01U,    /*!< CRYP peripheral is ready for initialization. */
  HAL_CRYP_PHASE_PROCESS           = 0x02U,    /*!< CRYP peripheral is in processing phase */
  HAL_CRYP_PHASE_FINAL             = 0x03U     /*!< CRYP peripheral is in final phase
                                                   This is relevant only with CCM and GCM modes */
}HAL_PhaseTypeDef;

/** 
  * @}
  */
  
/** @defgroup CRYP_Exported_Types_Group4 CRYP handle Structure definition
  * @{
  */
  
typedef struct
{
      CRYP_TypeDef             *Instance;        /*!< CRYP registers base address */

      CRYP_InitTypeDef         Init;             /*!< CRYP required parameters */

      uint8_t                  *pCrypInBuffPtr;  /*!< Pointer to CRYP processing (encryption, decryption,...) buffer */

      uint8_t                  *pCrypOutBuffPtr; /*!< Pointer to CRYP processing (encryption, decryption,...) buffer */

      __IO uint16_t            CrypInCount;      /*!< Counter of inputed data */

      __IO uint16_t            CrypOutCount;     /*!< Counter of output data */

      HAL_StatusTypeDef        Status;           /*!< CRYP peripheral status */

      HAL_PhaseTypeDef         Phase;            /*!< CRYP peripheral phase */

      DMA_HandleTypeDef        *hdmain;          /*!< CRYP In DMA handle parameters */

      DMA_HandleTypeDef        *hdmaout;         /*!< CRYP Out DMA handle parameters */

      HAL_LockTypeDef          Lock;             /*!< CRYP locking object */

   __IO  HAL_CRYP_STATETypeDef State;            /*!< CRYP peripheral state */
}CRYP_HandleTypeDef;

/** 
  * @}
  */

/** 
  * @}
  */
    
/* Exported constants --------------------------------------------------------*/
/** @defgroup CRYP_Exported_Constants CRYP Exported Constants
  * @{
  */

/** @defgroup CRYP_Key_Size CRYP Key Size
  * @{
  */
#define CRYP_KEYSIZE_128B         ((uint32_t)0x00000000U)
#define CRYP_KEYSIZE_192B         CRYP_CR_KEYSIZE_0
#define CRYP_KEYSIZE_256B         CRYP_CR_KEYSIZE_1
/**
  * @}
  */

/** @defgroup CRYP_Data_Type CRYP Data Type
  * @{
  */
#define CRYP_DATATYPE_32B         ((uint32_t)0x00000000U)
#define CRYP_DATATYPE_16B         CRYP_CR_DATATYPE_0
#define CRYP_DATATYPE_8B          CRYP_CR_DATATYPE_1
#define CRYP_DATATYPE_1B          CRYP_CR_DATATYPE
/**
  * @}
  */

/** @defgroup CRYP_Exported_Constants_Group3 CRYP CRYP_AlgoModeDirection
  * @{
  */
#define CRYP_CR_ALGOMODE_DIRECTION         ((uint32_t)0x0008003CU)
#define CRYP_CR_ALGOMODE_TDES_ECB_ENCRYPT  ((uint32_t)0x00000000U)
#define CRYP_CR_ALGOMODE_TDES_ECB_DECRYPT  ((uint32_t)0x00000004U)
#define CRYP_CR_ALGOMODE_TDES_CBC_ENCRYPT  ((uint32_t)0x00000008U)
#define CRYP_CR_ALGOMODE_TDES_CBC_DECRYPT  ((uint32_t)0x0000000CU)
#define CRYP_CR_ALGOMODE_DES_ECB_ENCRYPT   ((uint32_t)0x00000010U)
#define CRYP_CR_ALGOMODE_DES_ECB_DECRYPT   ((uint32_t)0x00000014U)
#define CRYP_CR_ALGOMODE_DES_CBC_ENCRYPT   ((uint32_t)0x00000018U)
#define CRYP_CR_ALGOMODE_DES_CBC_DECRYPT   ((uint32_t)0x0000001CU)
#define CRYP_CR_ALGOMODE_AES_ECB_ENCRYPT   ((uint32_t)0x00000020U)
#define CRYP_CR_ALGOMODE_AES_ECB_DECRYPT   ((uint32_t)0x00000024U)
#define CRYP_CR_ALGOMODE_AES_CBC_ENCRYPT   ((uint32_t)0x00000028U)
#define CRYP_CR_ALGOMODE_AES_CBC_DECRYPT   ((uint32_t)0x0000002CU)
#define CRYP_CR_ALGOMODE_AES_CTR_ENCRYPT   ((uint32_t)0x00000030U)
#define CRYP_CR_ALGOMODE_AES_CTR_DECRYPT   ((uint32_t)0x00000034U)
/**
  * @}
  */
  
/** @defgroup CRYP_Exported_Constants_Group4 CRYP CRYP_Interrupt
  * @{
  */
#define CRYP_IT_INI               ((uint32_t)CRYP_IMSCR_INIM)   /*!< Input FIFO Interrupt */
#define CRYP_IT_OUTI              ((uint32_t)CRYP_IMSCR_OUTIM)  /*!< Output FIFO Interrupt */
/**
  * @}
  */

/** @defgroup CRYP_Exported_Constants_Group5 CRYP CRYP_Flags
  * @{
  */
#define CRYP_FLAG_BUSY   ((uint32_t)0x00000010U)  /*!< The CRYP core is currently 
                                                     processing a block of data 
                                                     or a key preparation (for 
                                                     AES decryption). */
#define CRYP_FLAG_IFEM   ((uint32_t)0x00000001U)  /*!< Input FIFO is empty */
#define CRYP_FLAG_IFNF   ((uint32_t)0x00000002U)  /*!< Input FIFO is not Full */
#define CRYP_FLAG_OFNE   ((uint32_t)0x00000004U)  /*!< Output FIFO is not empty */
#define CRYP_FLAG_OFFU   ((uint32_t)0x00000008U)  /*!< Output FIFO is Full */
#define CRYP_FLAG_OUTRIS ((uint32_t)0x01000002U)  /*!< Output FIFO service raw 
                                                      interrupt status */
#define CRYP_FLAG_INRIS  ((uint32_t)0x01000001U)  /*!< Input FIFO service raw 
                                                      interrupt status */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup CRYP_Exported_Macros CRYP Exported Macros
  * @{
  */
  
/** @brief Reset CRYP handle state
  * @param  __HANDLE__: specifies the CRYP handle.
  * @retval None
  */
#define __HAL_CRYP_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = HAL_CRYP_STATE_RESET)

/**
  * @brief  Enable/Disable the CRYP peripheral.
  * @param  __HANDLE__: specifies the CRYP handle.
  * @retval None
  */
#define __HAL_CRYP_ENABLE(__HANDLE__)  ((__HANDLE__)->Instance->CR |=  CRYP_CR_CRYPEN)
#define __HAL_CRYP_DISABLE(__HANDLE__) ((__HANDLE__)->Instance->CR &=  ~CRYP_CR_CRYPEN)

/**
  * @brief  Flush the data FIFO.
  * @param  __HANDLE__: specifies the CRYP handle.
  * @retval None
  */
#define __HAL_CRYP_FIFO_FLUSH(__HANDLE__) ((__HANDLE__)->Instance->CR |=  CRYP_CR_FFLUSH)

/**
  * @brief  Set the algorithm mode: AES-ECB, AES-CBC, AES-CTR, DES-ECB, DES-CBC.
  * @param  __HANDLE__: specifies the CRYP handle.
  * @param  MODE: The algorithm mode.
  * @retval None
  */
#define __HAL_CRYP_SET_MODE(__HANDLE__, MODE)  ((__HANDLE__)->Instance->CR |= (uint32_t)(MODE))

/** @brief  Check whether the specified CRYP flag is set or not.
  * @param  __HANDLE__: specifies the CRYP handle.
  * @param  __FLAG__: specifies the flag to check.
  *         This parameter can be one of the following values:
  *            @arg CRYP_FLAG_BUSY: The CRYP core is currently processing a block of data 
  *                                 or a key preparation (for AES decryption). 
  *            @arg CRYP_FLAG_IFEM: Input FIFO is empty
  *            @arg CRYP_FLAG_IFNF: Input FIFO is not full
  *            @arg CRYP_FLAG_INRIS: Input FIFO service raw interrupt is pending
  *            @arg CRYP_FLAG_OFNE: Output FIFO is not empty
  *            @arg CRYP_FLAG_OFFU: Output FIFO is full
  *            @arg CRYP_FLAG_OUTRIS: Input FIFO service raw interrupt is pending
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */

#define __HAL_CRYP_GET_FLAG(__HANDLE__, __FLAG__) ((((uint8_t)((__FLAG__) >> 24U)) == 0x01U)?((((__HANDLE__)->Instance->RISR) & ((__FLAG__) & CRYP_FLAG_MASK)) == ((__FLAG__) & CRYP_FLAG_MASK)): \
                                                 ((((__HANDLE__)->Instance->RISR) & ((__FLAG__) & CRYP_FLAG_MASK)) == ((__FLAG__) & CRYP_FLAG_MASK)))

/** @brief  Check whether the specified CRYP interrupt is set or not.
  * @param  __HANDLE__: specifies the CRYP handle.
  * @param  __INTERRUPT__: specifies the interrupt to check.
  *         This parameter can be one of the following values:
  *            @arg CRYP_IT_INRIS: Input FIFO service raw interrupt is pending
  *            @arg CRYP_IT_OUTRIS: Output FIFO service raw interrupt is pending
  * @retval The new state of __INTERRUPT__ (TRUE or FALSE).
  */
#define __HAL_CRYP_GET_IT(__HANDLE__, __INTERRUPT__) (((__HANDLE__)->Instance->MISR & (__INTERRUPT__)) == (__INTERRUPT__))

/**
  * @brief  Enable the CRYP interrupt.
  * @param  __HANDLE__: specifies the CRYP handle.
  * @param  __INTERRUPT__: CRYP Interrupt.
  * @retval None
  */
#define __HAL_CRYP_ENABLE_IT(__HANDLE__, __INTERRUPT__) (((__HANDLE__)->Instance->IMSCR) |= (__INTERRUPT__))

/**
  * @brief  Disable the CRYP interrupt.
  * @param  __HANDLE__: specifies the CRYP handle.
  * @param  __INTERRUPT__: CRYP interrupt.
  * @retval None
  */
#define __HAL_CRYP_DISABLE_IT(__HANDLE__, __INTERRUPT__) (((__HANDLE__)->Instance->IMSCR) &= ~(__INTERRUPT__))

/**
  * @}
  */ 
  
/* Include CRYP HAL Extension module */
#include "stm32f4xx_hal_cryp_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @defgroup CRYP_Exported_Functions CRYP Exported Functions
  * @{
  */

/** @addtogroup CRYP_Exported_Functions_Group1
  * @{
  */    
HAL_StatusTypeDef HAL_CRYP_Init(CRYP_HandleTypeDef *hcryp);
HAL_StatusTypeDef HAL_CRYP_DeInit(CRYP_HandleTypeDef *hcryp);
void HAL_CRYP_MspInit(CRYP_HandleTypeDef *hcryp);
void HAL_CRYP_MspDeInit(CRYP_HandleTypeDef *hcryp);
/**
  * @}
  */ 

/** @addtogroup CRYP_Exported_Functions_Group2
  * @{
  */  
/* AES encryption/decryption using polling  ***********************************/
HAL_StatusTypeDef HAL_CRYP_AESECB_Encrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_AESECB_Decrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_AESCBC_Encrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_AESCBC_Decrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_AESCTR_Encrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_AESCTR_Decrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData, uint32_t Timeout);

/* AES encryption/decryption using interrupt  *********************************/
HAL_StatusTypeDef HAL_CRYP_AESECB_Encrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_AESCBC_Encrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_AESCTR_Encrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_AESECB_Decrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);
HAL_StatusTypeDef HAL_CRYP_AESCTR_Decrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);
HAL_StatusTypeDef HAL_CRYP_AESCBC_Decrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);

/* AES encryption/decryption using DMA  ***************************************/
HAL_StatusTypeDef HAL_CRYP_AESECB_Encrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_AESECB_Decrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);
HAL_StatusTypeDef HAL_CRYP_AESCBC_Encrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_AESCBC_Decrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);
HAL_StatusTypeDef HAL_CRYP_AESCTR_Encrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_AESCTR_Decrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);
/**
  * @}
  */ 

/** @addtogroup CRYP_Exported_Functions_Group3
  * @{
  */  
/* DES encryption/decryption using polling  ***********************************/
HAL_StatusTypeDef HAL_CRYP_DESECB_Encrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_DESCBC_Encrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_DESECB_Decrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_DESCBC_Decrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData, uint32_t Timeout);

/* DES encryption/decryption using interrupt  *********************************/
HAL_StatusTypeDef HAL_CRYP_DESECB_Encrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_DESECB_Decrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);
HAL_StatusTypeDef HAL_CRYP_DESCBC_Encrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_DESCBC_Decrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);

/* DES encryption/decryption using DMA  ***************************************/
HAL_StatusTypeDef HAL_CRYP_DESECB_Encrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_DESECB_Decrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);
HAL_StatusTypeDef HAL_CRYP_DESCBC_Encrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_DESCBC_Decrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);
/**
  * @}
  */ 

/** @addtogroup CRYP_Exported_Functions_Group4
  * @{
  */  
/* TDES encryption/decryption using polling  **********************************/
HAL_StatusTypeDef HAL_CRYP_TDESECB_Encrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_TDESCBC_Encrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_TDESECB_Decrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_TDESCBC_Decrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData, uint32_t Timeout);

/* TDES encryption/decryption using interrupt  ********************************/
HAL_StatusTypeDef HAL_CRYP_TDESECB_Encrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_TDESECB_Decrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);
HAL_StatusTypeDef HAL_CRYP_TDESCBC_Encrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_TDESCBC_Decrypt_IT(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);

/* TDES encryption/decryption using DMA  **************************************/
HAL_StatusTypeDef HAL_CRYP_TDESECB_Encrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_TDESECB_Decrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);
HAL_StatusTypeDef HAL_CRYP_TDESCBC_Encrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size, uint8_t *pCypherData);
HAL_StatusTypeDef HAL_CRYP_TDESCBC_Decrypt_DMA(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size, uint8_t *pPlainData);
/**
  * @}
  */ 

/** @addtogroup CRYP_Exported_Functions_Group5
  * @{
  */  
void HAL_CRYP_InCpltCallback(CRYP_HandleTypeDef *hcryp);
void HAL_CRYP_OutCpltCallback(CRYP_HandleTypeDef *hcryp);
void HAL_CRYP_ErrorCallback(CRYP_HandleTypeDef *hcryp);
/**
  * @}
  */ 

/** @addtogroup CRYP_Exported_Functions_Group6
  * @{
  */  
void HAL_CRYP_IRQHandler(CRYP_HandleTypeDef *hcryp);
/**
  * @}
  */ 

/** @addtogroup CRYP_Exported_Functions_Group7
  * @{
  */  
HAL_CRYP_STATETypeDef HAL_CRYP_GetState(CRYP_HandleTypeDef *hcryp);
/**
  * @}
  */ 
  
/**
  * @}
  */

/* Private types -------------------------------------------------------------*/
/** @defgroup CRYP_Private_Types CRYP Private Types
  * @{
  */

/**
  * @}
  */ 

/* Private variables ---------------------------------------------------------*/
/** @defgroup CRYP_Private_Variables CRYP Private Variables
  * @{
  */

/**
  * @}
  */ 

/* Private constants ---------------------------------------------------------*/
/** @defgroup CRYP_Private_Constants CRYP Private Constants
  * @{
  */
#define CRYP_FLAG_MASK  ((uint32_t)0x0000001FU)
/**
  * @}
  */ 

/* Private macros ------------------------------------------------------------*/
/** @defgroup CRYP_Private_Macros CRYP Private Macros
  * @{
  */

#define IS_CRYP_KEYSIZE(__KEYSIZE__)  (((__KEYSIZE__) == CRYP_KEYSIZE_128B)  || \
                                       ((__KEYSIZE__) == CRYP_KEYSIZE_192B)  || \
                                       ((__KEYSIZE__) == CRYP_KEYSIZE_256B))


#define IS_CRYP_DATATYPE(__DATATYPE__) (((__DATATYPE__) == CRYP_DATATYPE_32B) || \
                                        ((__DATATYPE__) == CRYP_DATATYPE_16B) || \
                                        ((__DATATYPE__) == CRYP_DATATYPE_8B)  || \
                                        ((__DATATYPE__) == CRYP_DATATYPE_1B))  


 /**
  * @}
  */ 
  
/* Private functions ---------------------------------------------------------*/
/** @defgroup CRYP_Private_Functions CRYP Private Functions
  * @{
  */

/**
  * @}
  */
     
#endif /* STM32F415xx || STM32F417xx || STM32F437xx || STM32F439xx || STM32F479xx */

/**
  * @}
  */ 

/**
  * @}
  */ 
  
#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_CRYP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
