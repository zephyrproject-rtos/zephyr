/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_cryp.h
  * @author  MCD Application Team
  * @brief   Header file of CRYP HAL module.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32MP1xx_HAL_CRYP_H
#define STM32MP1xx_HAL_CRYP_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */
#if defined (CRYP1) || defined (CRYP2)
/** @addtogroup CRYP
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup CRYP_Exported_Types CRYP Exported Types
  * @{
  */

/**
  * @brief CRYP Init Structure definition
  */

typedef struct
{
  uint32_t DataType;                   /*!< 32-bit data, 16-bit data, 8-bit data or 1-bit string.
                                        This parameter can be a value of @ref CRYP_Data_Type */
  uint32_t KeySize;                    /*!< Used only in AES mode : 128, 192 or 256 bit key length in CRYP1.
                                            This parameter can be a value of @ref CRYP_Key_Size */
  uint32_t *pKey;                      /*!< The key used for encryption/decryption */
  uint32_t *pInitVect;                 /*!< The initialization vector used also as initialization
                                         counter in CTR mode */
  uint32_t Algorithm;                  /*!<  DES/ TDES Algorithm ECB/CBC
                                        AES Algorithm ECB/CBC/CTR/GCM or CCM
                                        This parameter can be a value of @ref CRYP_Algorithm_Mode */
  uint32_t *Header;                    /*!< used only in AES GCM and CCM Algorithm for authentication,
                                        GCM : also known as Additional Authentication Data
                                        CCM : named B1 composed of the associated data length and Associated Data. */
  uint32_t HeaderSize;                /*!< The size of header buffer in word  */
  uint32_t *B0;                       /*!< B0 is first authentication block used only  in AES CCM mode */
  uint32_t DataWidthUnit;             /*!< Data With Unit, this parameter can be value of @ref CRYP_Data_Width_Unit*/

} CRYP_ConfigTypeDef;


/**
  * @brief  CRYP State Structure definition
  */

typedef enum
{
  HAL_CRYP_STATE_RESET             = 0x00U,  /*!< CRYP not yet initialized or disabled  */
  HAL_CRYP_STATE_READY             = 0x01U,  /*!< CRYP initialized and ready for use    */
  HAL_CRYP_STATE_BUSY              = 0x02U  /*!< CRYP BUSY, internal processing is ongoing  */
} HAL_CRYP_STATETypeDef;


/**
  * @brief  CRYP handle Structure definition
  */

#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1)
typedef struct __CRYP_HandleTypeDef
#else
typedef struct
#endif /* (USE_HAL_CRYP_REGISTER_CALLBACKS) */
{

  CRYP_TypeDef                      *Instance;            /*!< CRYP registers base address */

  CRYP_ConfigTypeDef                Init;             /*!< CRYP required parameters */

  uint32_t                          *pCrypInBuffPtr;  /*!< Pointer to CRYP processing (encryption, decryption,...) buffer */

  uint32_t                          *pCrypOutBuffPtr; /*!< Pointer to CRYP processing (encryption, decryption,...) buffer */

  __IO uint16_t                     CrypHeaderCount;   /*!< Counter of header data */

  __IO uint16_t                     CrypInCount;      /*!< Counter of input data */

  __IO uint16_t                     CrypOutCount;     /*!< Counter of output data */

  uint16_t                          Size;           /*!< length of input data in word */

  uint32_t                          Phase;            /*!< CRYP peripheral phase */

  DMA_HandleTypeDef                 *hdmain;          /*!< CRYP In DMA handle parameters */

  DMA_HandleTypeDef                 *hdmaout;         /*!< CRYP Out DMA handle parameters */

  MDMA_HandleTypeDef                *hmdmain;         /*!< CRYP In MDMA handle parameters */

  MDMA_HandleTypeDef                *hmdmaout;        /*!< CRYP Out MDMA handle parameters */

  HAL_LockTypeDef                   Lock;             /*!< CRYP locking object */

  __IO  HAL_CRYP_STATETypeDef       State;            /*!< CRYP peripheral state */

  __IO uint32_t                     ErrorCode;        /*!< CRYP peripheral error code */

  uint32_t                          Version;          /*!< CRYP1 IP version*/

#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1)
  void (*InCpltCallback)(struct __CRYP_HandleTypeDef *hcryp);      /*!< CRYP Input FIFO transfer completed callback  */
  void (*OutCpltCallback)(struct __CRYP_HandleTypeDef *hcryp);     /*!< CRYP Output FIFO transfer completed callback */
  void (*ErrorCallback)(struct __CRYP_HandleTypeDef *hcryp);       /*!< CRYP Error callback */

  void (* MspInitCallback)(struct __CRYP_HandleTypeDef *hcryp);    /*!< CRYP Msp Init callback  */
  void (* MspDeInitCallback)(struct __CRYP_HandleTypeDef *hcryp);  /*!< CRYP Msp DeInit callback  */

#endif /* (USE_HAL_CRYP_REGISTER_CALLBACKS) */

} CRYP_HandleTypeDef;


/**
  * @}
  */

#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1)
/** @defgroup HAL_CRYP_Callback_ID_enumeration_definition HAL CRYP Callback ID enumeration definition
  * @brief  HAL CRYP Callback ID enumeration definition
  * @{
  */
typedef enum
{
  HAL_CRYP_INPUT_COMPLETE_CB_ID    = 0x01U,    /*!< CRYP Input FIFO transfer completed callback ID */
  HAL_CRYP_OUTPUT_COMPLETE_CB_ID   = 0x02U,    /*!< CRYP Output FIFO transfer completed callback ID */
  HAL_CRYP_ERROR_CB_ID             = 0x03U,    /*!< CRYP Error callback ID           */

  HAL_CRYP_MSPINIT_CB_ID        = 0x04U,    /*!< CRYP MspInit callback ID         */
  HAL_CRYP_MSPDEINIT_CB_ID      = 0x05U     /*!< CRYP MspDeInit callback ID       */

} HAL_CRYP_CallbackIDTypeDef;
/**
  * @}
  */

/** @defgroup HAL_CRYP_Callback_pointer_definition HAL CRYP Callback pointer definition
  * @brief  HAL CRYP Callback pointer definition
  * @{
  */

typedef  void (*pCRYP_CallbackTypeDef)(CRYP_HandleTypeDef *hcryp);    /*!< pointer to a common CRYP callback function */

/**
  * @}
  */

#endif /* USE_HAL_CRYP_REGISTER_CALLBACKS */

/* Exported constants --------------------------------------------------------*/
/** @defgroup CRYP_Exported_Constants CRYP Exported Constants
  * @{
  */

/** @defgroup CRYP_Error_Definition   CRYP Error Definition
  * @{
  */
#define HAL_CRYP_ERROR_NONE              0x00000000U  /*!< No error        */
#define HAL_CRYP_ERROR_WRITE             0x00000001U  /*!< Write error     */
#define HAL_CRYP_ERROR_READ              0x00000002U  /*!< Read error      */
#define HAL_CRYP_ERROR_DMA               0x00000004U  /*!< DMA error       */
#define HAL_CRYP_ERROR_BUSY              0x00000008U  /*!< Busy flag error */
#define HAL_CRYP_ERROR_TIMEOUT           0x00000010U  /*!< Timeout error */
#define HAL_CRYP_ERROR_NOT_SUPPORTED     0x00000020U  /*!< Not supported mode */
#define HAL_CRYP_ERROR_AUTH_TAG_SEQUENCE 0x00000040U  /*!< Sequence are not respected only for GCM or CCM */
#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1)
#define  HAL_CRYP_ERROR_INVALID_CALLBACK ((uint32_t)0x00000080U)    /*!< Invalid Callback error  */
#endif /* USE_HAL_CRYP_REGISTER_CALLBACKS */

/**
  * @}
  */


/** @defgroup CRYP_Data_Width_Unit CRYP Data Width Unit
  * @{
  */

#define CRYP_DATAWIDTHUNIT_WORD   0x00000000U  /*!< By default, size unit is word */
#define CRYP_DATAWIDTHUNIT_BYTE   0x00000001U  /*!< By default, size unit is word */

/**
  * @}
  */

/** @defgroup CRYP_Algorithm_Mode CRYP Algorithm Mode
  * @{
  */

#define CRYP_DES_ECB     CRYP_CR_ALGOMODE_DES_ECB
#define CRYP_DES_CBC     CRYP_CR_ALGOMODE_DES_CBC
#define CRYP_TDES_ECB    CRYP_CR_ALGOMODE_TDES_ECB
#define CRYP_TDES_CBC    CRYP_CR_ALGOMODE_TDES_CBC
#define CRYP_AES_ECB     CRYP_CR_ALGOMODE_AES_ECB
#define CRYP_AES_CBC     CRYP_CR_ALGOMODE_AES_CBC
#define CRYP_AES_CTR     CRYP_CR_ALGOMODE_AES_CTR
#define CRYP_AES_GCM     CRYP_CR_ALGOMODE_AES_GCM
#define CRYP_AES_CCM     CRYP_CR_ALGOMODE_AES_CCM

/**
  * @}
  */

/** @defgroup CRYP_Key_Size CRYP Key Size
  * @{
  */

#define CRYP_KEYSIZE_128B         0x00000000U
#define CRYP_KEYSIZE_192B         CRYP_CR_KEYSIZE_0
#define CRYP_KEYSIZE_256B         CRYP_CR_KEYSIZE_1

/**
  * @}
  */

/** @defgroup CRYP_Data_Type CRYP Data Type
  * @{
  */

#define CRYP_DATATYPE_32B         0x00000000U
#define CRYP_DATATYPE_16B         CRYP_CR_DATATYPE_0
#define CRYP_DATATYPE_8B          CRYP_CR_DATATYPE_1
#define CRYP_DATATYPE_1B          CRYP_CR_DATATYPE

/**
  * @}
  */

/** @defgroup CRYP_Interrupt  CRYP Interrupt
  * @{
  */

#define CRYP_IT_INI       CRYP_IMSCR_INIM   /*!< Input FIFO Interrupt */
#define CRYP_IT_OUTI      CRYP_IMSCR_OUTIM  /*!< Output FIFO Interrupt */

/**
  * @}
  */

/** @defgroup CRYP_Flags CRYP Flags
  * @{
  */

/* Flags in the SR register */
#define CRYP_FLAG_IFEM    CRYP_SR_IFEM  /*!< Input FIFO is empty */
#define CRYP_FLAG_IFNF    CRYP_SR_IFNF  /*!< Input FIFO is not Full */
#define CRYP_FLAG_OFNE    CRYP_SR_OFNE  /*!< Output FIFO is not empty */
#define CRYP_FLAG_OFFU    CRYP_SR_OFFU  /*!< Output FIFO is Full */
#define CRYP_FLAG_BUSY    CRYP_SR_BUSY  /*!< The CRYP core is currently processing a block of data
                                             or a key preparation (for AES decryption). */
/* Flags in the RISR register */
#define CRYP_FLAG_OUTRIS  0x01000002U  /*!< Output FIFO service raw interrupt status */
#define CRYP_FLAG_INRIS   0x01000001U  /*!< Input FIFO service raw interrupt status*/

/**
  * @}
  */


/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup CRYP_Exported_Macros CRYP Exported Macros
  * @{
  */

/** @brief Reset CRYP handle state
  * @param  __HANDLE__ specifies the CRYP handle.
  * @retval None
  */
#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1)
#define __HAL_CRYP_RESET_HANDLE_STATE(__HANDLE__) do{\
                                                      (__HANDLE__)->State = HAL_CRYP_STATE_RESET;\
                                                      (__HANDLE__)->MspInitCallback = NULL;\
                                                      (__HANDLE__)->MspDeInitCallback = NULL;\
                                                     }while(0)
#else
#define __HAL_CRYP_RESET_HANDLE_STATE(__HANDLE__) ( (__HANDLE__)->State = HAL_CRYP_STATE_RESET)
#endif /* USE_HAL_CRYP_REGISTER_CALLBACKS */

/**
  * @brief  Enable/Disable the CRYP peripheral.
  * @param  __HANDLE__: specifies the CRYP handle.
  * @retval None
  */

#define __HAL_CRYP_ENABLE(__HANDLE__)  ((__HANDLE__)->Instance->CR |=  CRYP_CR_CRYPEN)
#define __HAL_CRYP_DISABLE(__HANDLE__) ((__HANDLE__)->Instance->CR &=  ~CRYP_CR_CRYPEN)

/** @brief  Check whether the specified CRYP status flag is set or not.
  * @param  __FLAG__: specifies the flag to check.
  *         This parameter can be one of the following values for CRYP:
  *            @arg CRYP_FLAG_BUSY: The CRYP core is currently processing a block of data
  *                                 or a key preparation (for AES decryption).
  *            @arg CRYP_FLAG_IFEM: Input FIFO is empty
  *            @arg CRYP_FLAG_IFNF: Input FIFO is not full
  *            @arg CRYP_FLAG_INRIS: Input FIFO service raw interrupt is pending
  *            @arg CRYP_FLAG_OFNE: Output FIFO is not empty
  *            @arg CRYP_FLAG_OFFU: Output FIFO is full
  *            @arg CRYP_FLAG_OUTRIS: Input FIFO service raw interrupt is pending
  * @retval The state of __FLAG__ (TRUE or FALSE).
  */
#define CRYP_FLAG_MASK  0x0000001FU

#define __HAL_CRYP_GET_FLAG(__HANDLE__, __FLAG__) ((((uint8_t)((__FLAG__) >> 24)) == 0x01U)?((((__HANDLE__)->Instance->RISR) & ((__FLAG__) & CRYP_FLAG_MASK)) == ((__FLAG__) & CRYP_FLAG_MASK)): \
                                                   ((((__HANDLE__)->Instance->RISR) & ((__FLAG__) & CRYP_FLAG_MASK)) == ((__FLAG__) & CRYP_FLAG_MASK)))

/** @brief  Check whether the specified CRYP interrupt is set or not.
  * @param  __HANDLE__: specifies the CRYP handle.
  * @param  __INTERRUPT__: specifies the interrupt to check.
  *         This parameter can be one of the following values for CRYP:
  *            @arg CRYP_IT_INI: Input FIFO service masked interrupt status
  *            @arg CRYP_IT_OUTI: Output FIFO service masked interrupt status
  * @retval The state of __INTERRUPT__ (TRUE or FALSE).
  */

#define __HAL_CRYP_GET_IT(__HANDLE__, __INTERRUPT__) (((__HANDLE__)->Instance->MISR & (__INTERRUPT__)) == (__INTERRUPT__))

/**
  * @brief  Enable the CRYP interrupt.
  * @param  __HANDLE__: specifies the CRYP handle.
  * @param  __INTERRUPT__: CRYP Interrupt.
  *         This parameter can be one of the following values for CRYP:
  *            @ CRYP_IT_INI : Input FIFO service interrupt mask.
  *            @ CRYP_IT_OUTI : Output FIFO service interrupt mask.CRYP interrupt.
  * @retval None
  */

#define __HAL_CRYP_ENABLE_IT(__HANDLE__, __INTERRUPT__) (((__HANDLE__)->Instance->IMSCR) |= (__INTERRUPT__))

/**
  * @brief  Disable the CRYP interrupt.
  * @param  __HANDLE__: specifies the CRYP handle.
  * @param  __INTERRUPT__: CRYP Interrupt.
  *         This parameter can be one of the following values for CRYP:
  *            @ CRYP_IT_INI : Input FIFO service interrupt mask.
  *            @ CRYP_IT_OUTI : Output FIFO service interrupt mask.CRYP interrupt.
  * @retval None
  */

#define __HAL_CRYP_DISABLE_IT(__HANDLE__, __INTERRUPT__) (((__HANDLE__)->Instance->IMSCR) &= ~(__INTERRUPT__))

/**
  * @}
  */

/* Include CRYP HAL Extended module */
#include "stm32mp1xx_hal_cryp_ex.h"

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
HAL_StatusTypeDef HAL_CRYP_SetConfig(CRYP_HandleTypeDef *hcryp, CRYP_ConfigTypeDef *pConf);
HAL_StatusTypeDef HAL_CRYP_GetConfig(CRYP_HandleTypeDef *hcryp, CRYP_ConfigTypeDef *pConf);
#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1)
HAL_StatusTypeDef HAL_CRYP_RegisterCallback(CRYP_HandleTypeDef *hcryp, HAL_CRYP_CallbackIDTypeDef CallbackID,
                                            pCRYP_CallbackTypeDef pCallback);
HAL_StatusTypeDef HAL_CRYP_UnRegisterCallback(CRYP_HandleTypeDef *hcryp, HAL_CRYP_CallbackIDTypeDef CallbackID);
#endif /* USE_HAL_CRYP_REGISTER_CALLBACKS */
/**
  * @}
  */

/** @addtogroup CRYP_Exported_Functions_Group2
  * @{
  */

/* encryption/decryption ***********************************/
HAL_StatusTypeDef HAL_CRYP_Encrypt(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output,
                                   uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_Decrypt(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output,
                                   uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_Encrypt_IT(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output);
HAL_StatusTypeDef HAL_CRYP_Decrypt_IT(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output);
HAL_StatusTypeDef HAL_CRYP_Encrypt_DMA(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output);
HAL_StatusTypeDef HAL_CRYP_Decrypt_DMA(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output);

/**
  * @}
  */


/** @addtogroup CRYP_Exported_Functions_Group3
  * @{
  */
/* Interrupt Handler functions  **********************************************/
void HAL_CRYP_IRQHandler(CRYP_HandleTypeDef *hcryp);
HAL_CRYP_STATETypeDef HAL_CRYP_GetState(CRYP_HandleTypeDef *hcryp);
void HAL_CRYP_InCpltCallback(CRYP_HandleTypeDef *hcryp);
void HAL_CRYP_OutCpltCallback(CRYP_HandleTypeDef *hcryp);
void HAL_CRYP_ErrorCallback(CRYP_HandleTypeDef *hcryp);
uint32_t HAL_CRYP_GetError(CRYP_HandleTypeDef *hcryp);

/**
  * @}
  */

/**
  * @}
  */

/* Private macros --------------------------------------------------------*/
/** @defgroup CRYP_Private_Macros   CRYP Private Macros
  * @{
  */

/** @defgroup CRYP_IS_CRYP_Definitions CRYP Private macros to check input parameters
  * @{
  */

#define IS_CRYP_ALGORITHM(ALGORITHM) (((ALGORITHM) == CRYP_DES_ECB)   || \
                                      ((ALGORITHM)  == CRYP_DES_CBC)   || \
                                      ((ALGORITHM)  == CRYP_TDES_ECB)  || \
                                      ((ALGORITHM)  == CRYP_TDES_CBC)  || \
                                      ((ALGORITHM)  == CRYP_AES_ECB)   || \
                                      ((ALGORITHM)  == CRYP_AES_CBC)   || \
                                      ((ALGORITHM)  == CRYP_AES_CTR)   || \
                                      ((ALGORITHM)  == CRYP_AES_GCM)   || \
                                      ((ALGORITHM)  == CRYP_AES_CCM))

#define IS_CRYP_KEYSIZE(KEYSIZE)(((KEYSIZE) == CRYP_KEYSIZE_128B)   || \
                                 ((KEYSIZE) == CRYP_KEYSIZE_192B)   || \
                                 ((KEYSIZE) == CRYP_KEYSIZE_256B))

#define IS_CRYP_DATATYPE(DATATYPE)(((DATATYPE) == CRYP_DATATYPE_32B)   || \
                                   ((DATATYPE) == CRYP_DATATYPE_16B) || \
                                   ((DATATYPE) == CRYP_DATATYPE_8B) || \
                                   ((DATATYPE) == CRYP_DATATYPE_1B))

/**
  * @}
  */

/**
  * @}
  */


/* Private constants ---------------------------------------------------------*/
/** @defgroup CRYP_Private_Constants CRYP Private Constants
  * @{
  */

/**
  * @}
  */
/* Private defines -----------------------------------------------------------*/
/** @defgroup CRYP_Private_Defines CRYP Private Defines
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
/* Private functions prototypes ----------------------------------------------*/
/** @defgroup CRYP_Private_Functions_Prototypes CRYP Private Functions Prototypes
  * @{
  */

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


/**
  * @}
  */


#endif /* CRYP1 || CRYP2 */
/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32MP1xx_HAL_CRYP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
