/**
  ******************************************************************************
  * @file    stm32wbxx_hal_cryp.h
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
#ifndef STM32WBxx_HAL_CRYP_H
#define STM32WBxx_HAL_CRYP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_def.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */



/** @defgroup CRYP CRYP
  * @brief CRYP HAL module driver.
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
                                        128 or 256 bit key length in TinyAES This parameter can be a value of @ref CRYP_Key_Size */
  uint32_t *pKey;                      /*!< The key used for encryption/decryption */
  uint32_t *pInitVect;                 /*!< The initialization vector used also as initialization
                                         counter in CTR mode */
  uint32_t Algorithm;                  /*!<  DES/ TDES Algorithm ECB/CBC
                                        AES Algorithm ECB/CBC/CTR/GCM or CCM
                                        This parameter can be a value of @ref CRYP_Algorithm_Mode */
  uint32_t *Header;                    /*!< used only in AES GCM and CCM Algorithm for authentication,
                                        GCM : also known as Additional Authentication Data
                                        CCM : named B1 composed of the associated data length and Associated Data. */
  uint32_t HeaderSize;                 /*!< The size of header buffer in word  */
  uint32_t *B0;                        /*!< B0 is first authentication block used only  in AES CCM mode */
  uint32_t DataWidthUnit;              /*!< Data With Unit, this parameter can be value of @ref CRYP_Data_Width_Unit*/
  uint32_t KeyIVConfigSkip;            /*!< CRYP peripheral Key and IV configuration skip, to config Key and Initialization
                                           Vector only once and to skip configuration for consecutive processings.
                                           This parameter can be a value of @ref CRYP_Configuration_Skip */

} CRYP_ConfigTypeDef;


/**
  * @brief  CRYP State Structure definition
  */

typedef enum
{
  HAL_CRYP_STATE_RESET             = 0x00U,  /*!< CRYP not yet initialized or disabled  */
  HAL_CRYP_STATE_READY             = 0x01U,  /*!< CRYP initialized and ready for use    */
  HAL_CRYP_STATE_BUSY              = 0x02U,  /*!< CRYP BUSY, internal processing is ongoing  */
#if (USE_HAL_CRYP_SUSPEND_RESUME == 1U)
  HAL_CRYP_STATE_SUSPENDED         = 0x03U,   /*!< CRYP suspended                        */
#endif /* USE_HAL_CRYP_SUSPEND_RESUME */
} HAL_CRYP_STATETypeDef;

#if (USE_HAL_CRYP_SUSPEND_RESUME == 1U)
/**
  * @brief HAL CRYP mode suspend definitions
  */
typedef enum
{
  HAL_CRYP_SUSPEND_NONE            = 0x00U,    /*!< CRYP processing suspension not requested */
  HAL_CRYP_SUSPEND                 = 0x01U     /*!< CRYP processing suspension requested     */
}HAL_SuspendTypeDef;
#endif /* USE_HAL_CRYP_SUSPEND_RESUME */

/**
  * @brief  CRYP handle Structure definition
  */
#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1)
typedef struct __CRYP_HandleTypeDef
#else
typedef struct
#endif
{
  AES_TypeDef                       *Instance;            /*!< AES Register base address */

  CRYP_ConfigTypeDef                Init;             /*!< CRYP required parameters */

  FunctionalState                   AutoKeyDerivation;   /*!< Used only in TinyAES to allow to bypass or not key write-up before decryption.
                                                         This parameter can be a value of ENABLE/DISABLE */

  uint32_t                          *pCrypInBuffPtr;  /*!< Pointer to CRYP processing (encryption, decryption,...) buffer */

  uint32_t                          *pCrypOutBuffPtr; /*!< Pointer to CRYP processing (encryption, decryption,...) buffer */

  __IO uint16_t                     CrypHeaderCount;   /*!< Counter of header data */

  __IO uint16_t                     CrypInCount;      /*!< Counter of input data */

  __IO uint16_t                     CrypOutCount;     /*!< Counter of output data */

  uint16_t                          Size;             /*!< length of input data in words */

  uint32_t                          Phase;            /*!< CRYP peripheral phase */

  DMA_HandleTypeDef                 *hdmain;          /*!< CRYP In DMA handle parameters */

  DMA_HandleTypeDef                 *hdmaout;         /*!< CRYP Out DMA handle parameters */

  HAL_LockTypeDef                   Lock;             /*!< CRYP locking object */

  __IO  HAL_CRYP_STATETypeDef       State;            /*!< CRYP peripheral state */

  __IO uint32_t                     ErrorCode;        /*!< CRYP peripheral error code */

  uint32_t                          KeyIVConfig;      /*!< CRYP peripheral Key and IV configuration flag, used when
                                                           configuration can be skipped */

  uint32_t                          SizesSum;         /*!< Sum of successive payloads lengths (in bytes), stored
                                                           for a single signature computation after several
                                                           messages processing */

#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1U)
  void (*InCpltCallback)(struct __CRYP_HandleTypeDef *hcryp);      /*!< CRYP Input FIFO transfer completed callback  */
  void (*OutCpltCallback)(struct __CRYP_HandleTypeDef *hcryp);     /*!< CRYP Output FIFO transfer completed callback */
  void (*ErrorCallback)(struct __CRYP_HandleTypeDef *hcryp);       /*!< CRYP Error callback */

  void (* MspInitCallback)(struct __CRYP_HandleTypeDef *hcryp);    /*!< CRYP Msp Init callback  */
  void (* MspDeInitCallback)(struct __CRYP_HandleTypeDef *hcryp);  /*!< CRYP Msp DeInit callback  */

#endif /* (USE_HAL_CRYP_REGISTER_CALLBACKS) */

#if (USE_HAL_CRYP_SUSPEND_RESUME == 1U)

  __IO HAL_SuspendTypeDef     SuspendRequest;          /*!< CRYP peripheral suspension request flag */

  CRYP_ConfigTypeDef          Init_saved;              /*!< copy of CRYP required parameters when processing is suspended */

  uint32_t                    *pCrypInBuffPtr_saved;   /*!< copy of CRYP input pointer when processing is suspended */

  uint32_t                    *pCrypOutBuffPtr_saved;  /*!< copy of CRYP output pointer when processing is suspended */

  uint32_t                    CrypInCount_saved;       /*!< copy of CRYP input data counter when processing is suspended */

  uint32_t                    CrypOutCount_saved;      /*!< copy of CRYP output data counter when processing is suspended */

  uint32_t                    Phase_saved;             /*!< copy of CRYP authentication phase when processing is suspended */

  __IO HAL_CRYP_STATETypeDef  State_saved;             /*!< copy of CRYP peripheral state when processing is suspended */

  uint32_t                    IV_saved[4];             /*!< copy of Initialisation Vector registers */

  uint32_t                    SUSPxR_saved[8];         /*!< copy of suspension registers */

  uint32_t                    CR_saved;                /*!< copy of CRYP control register  when processing is suspended*/

  uint32_t                    Key_saved[8];            /*!< copy of key registers */

  uint32_t                    Size_saved;              /*!< copy of input buffer size */

  uint16_t                    CrypHeaderCount_saved;   /*!< copy of CRYP header data counter when processing is suspended */

  uint32_t                    ResumingFlag;            /*!< resumption flag to bypass steps already carried out */

  FunctionalState             AutoKeyDerivation_saved; /*!< copy of CRYP handle auto key derivation parameter */

#endif /* USE_HAL_CRYP_SUSPEND_RESUME */

} CRYP_HandleTypeDef;

#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1U)
/** @defgroup HAL_CRYP_Callback_ID_enumeration_definition HAL CRYP Callback ID enumeration definition
  * @brief  HAL CRYP Callback ID enumeration definition
  * @{
  */
typedef enum
{
  HAL_CRYP_MSPINIT_CB_ID           = 0x00U,    /*!< CRYP MspInit callback ID                        */
  HAL_CRYP_MSPDEINIT_CB_ID         = 0x01U,     /*!< CRYP MspDeInit callback ID                      */
  HAL_CRYP_INPUT_COMPLETE_CB_ID    = 0x02U,    /*!< CRYP Input FIFO transfer completed callback ID  */
  HAL_CRYP_OUTPUT_COMPLETE_CB_ID   = 0x03U,    /*!< CRYP Output FIFO transfer completed callback ID */
  HAL_CRYP_ERROR_CB_ID             = 0x04U,    /*!< CRYP Error callback ID                          */
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

/**
  * @}
  */

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
#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1U)
#define  HAL_CRYP_ERROR_INVALID_CALLBACK ((uint32_t)0x00000080U)    /*!< Invalid Callback error  */
#endif /* USE_HAL_CRYP_REGISTER_CALLBACKS */

/**
  * @}
  */

/** @defgroup CRYP_Data_Width_Unit CRYP Data Width Unit
  * @{
  */

#define CRYP_DATAWIDTHUNIT_WORD   0x00000000U  /*!< By default, size unit is word */
#define CRYP_DATAWIDTHUNIT_BYTE   0x00000001U  /*!< By default, size unit is byte */

/**
  * @}
  */

/** @defgroup CRYP_Algorithm_Mode CRYP Algorithm Mode
  * @{
  */

#define CRYP_AES_ECB            0x00000000U                       /*!< Electronic codebook chaining algorithm                   */
#define CRYP_AES_CBC            AES_CR_CHMOD_0                    /*!< Cipher block chaining algorithm                          */
#define CRYP_AES_CTR            AES_CR_CHMOD_1                    /*!< Counter mode chaining algorithm                          */
#define CRYP_AES_GCM_GMAC       (AES_CR_CHMOD_0 | AES_CR_CHMOD_1) /*!< Galois counter mode - Galois message authentication code */
#define CRYP_AES_CCM            AES_CR_CHMOD_2                    /*!< Counter with Cipher Mode                                 */

/**
  * @}
  */

/** @defgroup CRYP_Key_Size CRYP Key Size
  * @{
  */

#define CRYP_KEYSIZE_128B         0x00000000U          /*!< 128-bit long key */
#define CRYP_KEYSIZE_256B         AES_CR_KEYSIZE       /*!< 256-bit long key */

/**
  * @}
  */

/** @defgroup CRYP_Data_Type CRYP Data Type
  * @{
  */

#define CRYP_DATATYPE_32B         0x00000000U  /*!< 32-bit data type (no swapping)        */
#define CRYP_DATATYPE_16B         AES_CR_DATATYPE_0       /*!< 16-bit data type (half-word swapping) */
#define CRYP_DATATYPE_8B          AES_CR_DATATYPE_1       /*!< 8-bit data type (byte swapping)       */
#define CRYP_DATATYPE_1B          AES_CR_DATATYPE         /*!< 1-bit data type (bit swapping)        */

/**
  * @}
  */

/** @defgroup CRYP_Interrupt  CRYP Interrupt
  * @{
  */

#define CRYP_IT_CCFIE     AES_CR_CCFIE /*!< Computation Complete interrupt enable */
#define CRYP_IT_ERRIE     AES_CR_ERRIE /*!< Error interrupt enable                */
#define CRYP_IT_WRERR     AES_SR_WRERR  /*!< Write Error           */
#define CRYP_IT_RDERR     AES_SR_RDERR  /*!< Read Error            */
#define CRYP_IT_CCF       AES_SR_CCF    /*!< Computation completed */

/**
  * @}
  */

/** @defgroup CRYP_Flags CRYP Flags
  * @{
  */

/* status flags */
#define CRYP_FLAG_BUSY    AES_SR_BUSY   /*!< GCM process suspension forbidden */
#define CRYP_FLAG_WRERR   AES_SR_WRERR  /*!< Write Error                      */
#define CRYP_FLAG_RDERR   AES_SR_RDERR  /*!< Read error                       */
#define CRYP_FLAG_CCF     AES_SR_CCF    /*!< Computation completed            */
/* clearing flags */
#define CRYP_CCF_CLEAR    AES_CR_CCFC   /*!< Computation Complete Flag Clear */
#define CRYP_ERR_CLEAR    AES_CR_ERRC   /*!< Error Flag Clear  */

/**
  * @}
  */

/** @defgroup CRYP_Configuration_Skip CRYP Key and IV Configuration Skip Mode
  * @{
  */

#define CRYP_KEYIVCONFIG_ALWAYS        0x00000000U            /*!< Peripheral Key and IV configuration to do systematically */
#define CRYP_KEYIVCONFIG_ONCE          0x00000001U            /*!< Peripheral Key and IV configuration to do only once      */

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
#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1U)
#define __HAL_CRYP_RESET_HANDLE_STATE(__HANDLE__) do{\
                                                      (__HANDLE__)->State = HAL_CRYP_STATE_RESET;\
                                                      (__HANDLE__)->MspInitCallback = NULL;\
                                                      (__HANDLE__)->MspDeInitCallback = NULL;\
                                                     }while(0U)
#else
#define __HAL_CRYP_RESET_HANDLE_STATE(__HANDLE__) ( (__HANDLE__)->State = HAL_CRYP_STATE_RESET)
#endif /* USE_HAL_CRYP_REGISTER_CALLBACKS */

/**
  * @brief  Enable/Disable the CRYP peripheral.
  * @param  __HANDLE__ specifies the CRYP handle.
  * @retval None
  */

#define __HAL_CRYP_ENABLE(__HANDLE__)  ((__HANDLE__)->Instance->CR |=  AES_CR_EN)
#define __HAL_CRYP_DISABLE(__HANDLE__) ((__HANDLE__)->Instance->CR &=  ~AES_CR_EN)


/** @brief  Check whether the specified CRYP status flag is set or not.
  * @param  __HANDLE__ specifies the CRYP handle.
  * @param  __FLAG__ specifies the flag to check.
  *         This parameter can be one of the following values for TinyAES:
  *            @arg @ref CRYP_FLAG_BUSY GCM process suspension forbidden
  *            @arg @ref CRYP_IT_WRERR Write Error
  *            @arg @ref CRYP_IT_RDERR Read Error
  *            @arg @ref CRYP_IT_CCF Computation Complete
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
#define __HAL_CRYP_GET_FLAG(__HANDLE__, __FLAG__) (((__HANDLE__)->Instance->SR & (__FLAG__)) == (__FLAG__))

/** @brief  Clear the CRYP pending status flag.
  * @param  __HANDLE__ specifies the CRYP handle.
  * @param  __FLAG__ specifies the flag to clear.
  *         This parameter can be one of the following values:
  *            @arg @ref CRYP_ERR_CLEAR Read (RDERR) or Write Error (WRERR) Flag Clear
  *            @arg @ref CRYP_CCF_CLEAR Computation Complete Flag (CCF) Clear
  * @retval None
  */

#define __HAL_CRYP_CLEAR_FLAG(__HANDLE__, __FLAG__) SET_BIT((__HANDLE__)->Instance->CR, (__FLAG__))


/** @brief  Check whether the specified CRYP interrupt source is enabled or not.
  * @param  __HANDLE__ specifies the CRYP handle.
  * @param __INTERRUPT__ CRYP interrupt source to check
  *         This parameter can be one of the following values for TinyAES:
  *            @arg @ref CRYP_IT_ERRIE Error interrupt (used for RDERR and WRERR)
  *            @arg @ref CRYP_IT_CCFIE Computation Complete interrupt
  * @retval State of interruption (TRUE or FALSE).
  */

#define __HAL_CRYP_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__) (((__HANDLE__)->Instance->CR & (__INTERRUPT__)) == (__INTERRUPT__))

/** @brief  Check whether the specified CRYP interrupt is set or not.
  * @param  __HANDLE__ specifies the CRYP handle.
  * @param  __INTERRUPT__ specifies the interrupt to check.
  *         This parameter can be one of the following values for TinyAES:
  *            @arg @ref CRYP_IT_WRERR Write Error
  *            @arg @ref CRYP_IT_RDERR Read Error
  *            @arg @ref CRYP_IT_CCF  Computation Complete
  *         This parameter can be one of the following values for CRYP:
  *            @arg CRYP_IT_INI: Input FIFO service masked interrupt status
  *            @arg CRYP_IT_OUTI: Output FIFO service masked interrupt status
  * @retval The state of __INTERRUPT__ (TRUE or FALSE).
  */

#define __HAL_CRYP_GET_IT(__HANDLE__, __INTERRUPT__) (((__HANDLE__)->Instance->SR & (__INTERRUPT__)) == (__INTERRUPT__))

/**
  * @brief  Enable the CRYP interrupt.
  * @param  __HANDLE__ specifies the CRYP handle.
  * @param  __INTERRUPT__ CRYP Interrupt.
  *         This parameter can be one of the following values for TinyAES:
  *            @arg @ref CRYP_IT_ERRIE Error interrupt (used for RDERR and WRERR)
  *            @arg @ref CRYP_IT_CCFIE Computation Complete interrupt
  *         This parameter can be one of the following values for CRYP:
  *            @ CRYP_IT_INI : Input FIFO service interrupt mask.
  *            @ CRYP_IT_OUTI : Output FIFO service interrupt mask.CRYP interrupt.
  * @retval None
  */

#define __HAL_CRYP_ENABLE_IT(__HANDLE__, __INTERRUPT__) (((__HANDLE__)->Instance->CR) |= (__INTERRUPT__))

/**
  * @brief  Disable the CRYP interrupt.
  * @param  __HANDLE__ specifies the CRYP handle.
  * @param  __INTERRUPT__ CRYP Interrupt.
  *         This parameter can be one of the following values for TinyAES:
  *            @arg @ref CRYP_IT_ERRIE Error interrupt (used for RDERR and WRERR)
  *            @arg @ref CRYP_IT_CCFIE Computation Complete interrupt
  *         This parameter can be one of the following values for CRYP:
  *            @ CRYP_IT_INI : Input FIFO service interrupt mask.
  *            @ CRYP_IT_OUTI : Output FIFO service interrupt mask.CRYP interrupt.
  * @retval None
  */

#define __HAL_CRYP_DISABLE_IT(__HANDLE__, __INTERRUPT__) (((__HANDLE__)->Instance->CR) &= ~(__INTERRUPT__))

/**
  * @}
  */

/* Include CRYP HAL Extended module */
#include "stm32wbxx_hal_cryp_ex.h"

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
#if (USE_HAL_CRYP_REGISTER_CALLBACKS == 1U)
HAL_StatusTypeDef HAL_CRYP_RegisterCallback(CRYP_HandleTypeDef *hcryp, HAL_CRYP_CallbackIDTypeDef CallbackID, pCRYP_CallbackTypeDef pCallback);
HAL_StatusTypeDef HAL_CRYP_UnRegisterCallback(CRYP_HandleTypeDef *hcryp, HAL_CRYP_CallbackIDTypeDef CallbackID);
#endif /* USE_HAL_CRYP_REGISTER_CALLBACKS */
#if (USE_HAL_CRYP_SUSPEND_RESUME == 1U)
void HAL_CRYP_ProcessSuspend(CRYP_HandleTypeDef *hcryp);
HAL_StatusTypeDef HAL_CRYP_Suspend(CRYP_HandleTypeDef *hcryp);
HAL_StatusTypeDef HAL_CRYP_Resume(CRYP_HandleTypeDef *hcryp);
#endif /* defined (USE_HAL_CRYP_SUSPEND_RESUME) */
/**
  * @}
  */

/** @addtogroup CRYP_Exported_Functions_Group2
  * @{
  */

/* encryption/decryption ***********************************/
HAL_StatusTypeDef HAL_CRYP_Encrypt(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYP_Decrypt(CRYP_HandleTypeDef *hcryp, uint32_t *Input, uint16_t Size, uint32_t *Output, uint32_t Timeout);
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
/** @defgroup CRYP_Private_Macros CRYP Private Macros
  * @{
  */

/** @defgroup CRYP_IS_CRYP_Definitions CRYP Private macros to check input parameters
  * @{
  */

#define IS_CRYP_ALGORITHM(ALGORITHM) (((ALGORITHM) == CRYP_AES_ECB)      || \
                                      ((ALGORITHM)  == CRYP_AES_CBC)     || \
                                      ((ALGORITHM)  == CRYP_AES_CTR)     || \
                                      ((ALGORITHM)  == CRYP_AES_GCM_GMAC)|| \
                                      ((ALGORITHM)  == CRYP_AES_CCM))


#define IS_CRYP_KEYSIZE(KEYSIZE)(((KEYSIZE) == CRYP_KEYSIZE_128B)   || \
                                 ((KEYSIZE) == CRYP_KEYSIZE_256B))

#define IS_CRYP_DATATYPE(DATATYPE)(((DATATYPE) == CRYP_DATATYPE_32B) || \
                                   ((DATATYPE) == CRYP_DATATYPE_16B) || \
                                   ((DATATYPE) == CRYP_DATATYPE_8B)  || \
                                   ((DATATYPE) == CRYP_DATATYPE_1B))

#define IS_CRYP_INIT(CONFIG)(((CONFIG) == CRYP_KEYIVCONFIG_ALWAYS) || \
                             ((CONFIG) == CRYP_KEYIVCONFIG_ONCE))

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


/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32WBxx_HAL_CRYP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
