/**
  ******************************************************************************
  * @file    stm32g0xx_hal_i2s.h
  * @author  MCD Application Team
  * @brief   Header file of I2S HAL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
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
#ifndef STM32G0xx_HAL_I2S_H
#define STM32G0xx_HAL_I2S_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SPI_I2S_SUPPORT)
/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal_def.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @addtogroup I2S
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup I2S_Exported_Types I2S Exported Types
  * @{
  */

/**
  * @brief I2S Init structure definition
  */
typedef struct
{
  uint32_t Mode;                /*!< Specifies the I2S operating mode.
                                     This parameter can be a value of @ref I2S_Mode */

  uint32_t Standard;            /*!< Specifies the standard used for the I2S communication.
                                     This parameter can be a value of @ref I2S_Standard */

  uint32_t DataFormat;          /*!< Specifies the data format for the I2S communication.
                                     This parameter can be a value of @ref I2S_Data_Format */

  uint32_t MCLKOutput;          /*!< Specifies whether the I2S MCLK output is enabled or not.
                                     This parameter can be a value of @ref I2S_MCLK_Output */

  uint32_t AudioFreq;           /*!< Specifies the frequency selected for the I2S communication.
                                     This parameter can be a value of @ref I2S_Audio_Frequency */

  uint32_t CPOL;                /*!< Specifies the idle state of the I2S clock.
                                     This parameter can be a value of @ref I2S_Clock_Polarity */
} I2S_InitTypeDef;

/**
  * @brief  HAL State structures definition
  */
typedef enum
{
  HAL_I2S_STATE_RESET      = 0x00U,  /*!< I2S not yet initialized or disabled                */
  HAL_I2S_STATE_READY      = 0x01U,  /*!< I2S initialized and ready for use                  */
  HAL_I2S_STATE_BUSY       = 0x02U,  /*!< I2S internal process is ongoing                    */
  HAL_I2S_STATE_BUSY_TX    = 0x03U,  /*!< Data Transmission process is ongoing               */
  HAL_I2S_STATE_BUSY_RX    = 0x04U,  /*!< Data Reception process is ongoing                  */
  HAL_I2S_STATE_TIMEOUT    = 0x06U,  /*!< I2S timeout state                                  */
  HAL_I2S_STATE_ERROR      = 0x07U   /*!< I2S error state                                    */
} HAL_I2S_StateTypeDef;

/**
  * @brief I2S handle Structure definition
  */
typedef struct __I2S_HandleTypeDef
{
  SPI_TypeDef                *Instance;    /*!< I2S registers base address */

  I2S_InitTypeDef            Init;         /*!< I2S communication parameters */

  uint16_t                   *pTxBuffPtr;  /*!< Pointer to I2S Tx transfer buffer */

  __IO uint16_t              TxXferSize;   /*!< I2S Tx transfer size */

  __IO uint16_t              TxXferCount;  /*!< I2S Tx transfer Counter */

  uint16_t                   *pRxBuffPtr;  /*!< Pointer to I2S Rx transfer buffer */

  __IO uint16_t              RxXferSize;   /*!< I2S Rx transfer size */

  __IO uint16_t              RxXferCount;  /*!< I2S Rx transfer counter
                                              (This field is initialized at the
                                               same value as transfer size at the
                                               beginning of the transfer and
                                               decremented when a sample is received
                                               NbSamplesReceived = RxBufferSize-RxBufferCount) */
  DMA_HandleTypeDef          *hdmatx;      /*!< I2S Tx DMA handle parameters */

  DMA_HandleTypeDef          *hdmarx;      /*!< I2S Rx DMA handle parameters */

  __IO HAL_LockTypeDef       Lock;         /*!< I2S locking object */

  __IO HAL_I2S_StateTypeDef  State;        /*!< I2S communication state */

  __IO uint32_t              ErrorCode;    /*!< I2S Error code
                                                This parameter can be a value of @ref I2S_Error */

#if (USE_HAL_I2S_REGISTER_CALLBACKS == 1U)
  void (* TxCpltCallback)(struct __I2S_HandleTypeDef *hi2s);             /*!< I2S Tx Completed callback          */
  void (* RxCpltCallback)(struct __I2S_HandleTypeDef *hi2s);             /*!< I2S Rx Completed callback          */
  void (* TxHalfCpltCallback)(struct __I2S_HandleTypeDef *hi2s);         /*!< I2S Tx Half Completed callback     */
  void (* RxHalfCpltCallback)(struct __I2S_HandleTypeDef *hi2s);         /*!< I2S Rx Half Completed callback     */
  void (* ErrorCallback)(struct __I2S_HandleTypeDef *hi2s);              /*!< I2S Error callback                 */
  void (* MspInitCallback)(struct __I2S_HandleTypeDef *hi2s);            /*!< I2S Msp Init callback              */
  void (* MspDeInitCallback)(struct __I2S_HandleTypeDef *hi2s);          /*!< I2S Msp DeInit callback            */

#endif  /* USE_HAL_I2S_REGISTER_CALLBACKS */
} I2S_HandleTypeDef;

#if (USE_HAL_I2S_REGISTER_CALLBACKS == 1U)
/**
  * @brief  HAL I2S Callback ID enumeration definition
  */
typedef enum
{
  HAL_I2S_TX_COMPLETE_CB_ID             = 0x00U,    /*!< I2S Tx Completed callback ID         */
  HAL_I2S_RX_COMPLETE_CB_ID             = 0x01U,    /*!< I2S Rx Completed callback ID         */
  HAL_I2S_TX_HALF_COMPLETE_CB_ID        = 0x03U,    /*!< I2S Tx Half Completed callback ID    */
  HAL_I2S_RX_HALF_COMPLETE_CB_ID        = 0x04U,    /*!< I2S Rx Half Completed callback ID    */
  HAL_I2S_ERROR_CB_ID                   = 0x06U,    /*!< I2S Error callback ID                */
  HAL_I2S_MSPINIT_CB_ID                 = 0x07U,    /*!< I2S Msp Init callback ID             */
  HAL_I2S_MSPDEINIT_CB_ID               = 0x08U     /*!< I2S Msp DeInit callback ID           */

} HAL_I2S_CallbackIDTypeDef;

/**
  * @brief  HAL I2S Callback pointer definition
  */
typedef  void (*pI2S_CallbackTypeDef)(I2S_HandleTypeDef *hi2s); /*!< pointer to an I2S callback function */

#endif /* USE_HAL_I2S_REGISTER_CALLBACKS */
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup I2S_Exported_Constants I2S Exported Constants
  * @{
  */
/** @defgroup I2S_Error I2S Error
  * @{
  */
#define HAL_I2S_ERROR_NONE               (0x00000000U)  /*!< No error                    */
#define HAL_I2S_ERROR_TIMEOUT            (0x00000001U)  /*!< Timeout error               */
#define HAL_I2S_ERROR_OVR                (0x00000002U)  /*!< OVR error                   */
#define HAL_I2S_ERROR_UDR                (0x00000004U)  /*!< UDR error                   */
#define HAL_I2S_ERROR_DMA                (0x00000008U)  /*!< DMA transfer error          */
#define HAL_I2S_ERROR_PRESCALER          (0x00000010U)  /*!< Prescaler Calculation error */
#if (USE_HAL_I2S_REGISTER_CALLBACKS == 1U)
#define HAL_I2S_ERROR_INVALID_CALLBACK   (0x00000020U)  /*!< Invalid Callback error      */
#endif /* USE_HAL_I2S_REGISTER_CALLBACKS */
/**
  * @}
  */

/** @defgroup I2S_Mode I2S Mode
  * @{
  */
#define I2S_MODE_SLAVE_TX                (0x00000000U)
#define I2S_MODE_SLAVE_RX                (SPI_I2SCFGR_I2SCFG_0)
#define I2S_MODE_MASTER_TX               (SPI_I2SCFGR_I2SCFG_1)
#define I2S_MODE_MASTER_RX               ((SPI_I2SCFGR_I2SCFG_0 | SPI_I2SCFGR_I2SCFG_1))
/**
  * @}
  */

/** @defgroup I2S_Standard I2S Standard
  * @{
  */
#define I2S_STANDARD_PHILIPS             (0x00000000U)
#define I2S_STANDARD_MSB                 (SPI_I2SCFGR_I2SSTD_0)
#define I2S_STANDARD_LSB                 (SPI_I2SCFGR_I2SSTD_1)
#define I2S_STANDARD_PCM_SHORT           ((SPI_I2SCFGR_I2SSTD_0 | SPI_I2SCFGR_I2SSTD_1))
#define I2S_STANDARD_PCM_LONG            ((SPI_I2SCFGR_I2SSTD_0 | SPI_I2SCFGR_I2SSTD_1 | SPI_I2SCFGR_PCMSYNC))
/**
  * @}
  */

/** @defgroup I2S_Data_Format I2S Data Format
  * @{
  */
#define I2S_DATAFORMAT_16B               (0x00000000U)
#define I2S_DATAFORMAT_16B_EXTENDED      (SPI_I2SCFGR_CHLEN)
#define I2S_DATAFORMAT_24B               ((SPI_I2SCFGR_CHLEN | SPI_I2SCFGR_DATLEN_0))
#define I2S_DATAFORMAT_32B               ((SPI_I2SCFGR_CHLEN | SPI_I2SCFGR_DATLEN_1))
/**
  * @}
  */

/** @defgroup I2S_MCLK_Output I2S MCLK Output
  * @{
  */
#define I2S_MCLKOUTPUT_ENABLE            (SPI_I2SPR_MCKOE)
#define I2S_MCLKOUTPUT_DISABLE           (0x00000000U)
/**
  * @}
  */

/** @defgroup I2S_Audio_Frequency I2S Audio Frequency
  * @{
  */
#define I2S_AUDIOFREQ_192K               (192000U)
#define I2S_AUDIOFREQ_96K                (96000U)
#define I2S_AUDIOFREQ_48K                (48000U)
#define I2S_AUDIOFREQ_44K                (44100U)
#define I2S_AUDIOFREQ_32K                (32000U)
#define I2S_AUDIOFREQ_22K                (22050U)
#define I2S_AUDIOFREQ_16K                (16000U)
#define I2S_AUDIOFREQ_11K                (11025U)
#define I2S_AUDIOFREQ_8K                 (8000U)
#define I2S_AUDIOFREQ_DEFAULT            (2U)
/**
  * @}
  */

/** @defgroup I2S_Clock_Polarity I2S Clock Polarity
  * @{
  */
#define I2S_CPOL_LOW                     (0x00000000U)
#define I2S_CPOL_HIGH                    (SPI_I2SCFGR_CKPOL)
/**
  * @}
  */

/** @defgroup I2S_Interrupts_Definition I2S Interrupts Definition
  * @{
  */
#define I2S_IT_TXE                       SPI_CR2_TXEIE
#define I2S_IT_RXNE                      SPI_CR2_RXNEIE
#define I2S_IT_ERR                       SPI_CR2_ERRIE
/**
  * @}
  */

/** @defgroup I2S_Flags_Definition I2S Flags Definition
  * @{
  */
#define I2S_FLAG_TXE                     SPI_SR_TXE
#define I2S_FLAG_RXNE                    SPI_SR_RXNE

#define I2S_FLAG_UDR                     SPI_SR_UDR
#define I2S_FLAG_OVR                     SPI_SR_OVR
#define I2S_FLAG_FRE                     SPI_SR_FRE

#define I2S_FLAG_CHSIDE                  SPI_SR_CHSIDE
#define I2S_FLAG_BSY                     SPI_SR_BSY
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup I2S_Exported_macros I2S Exported Macros
  * @{
  */

/** @brief  Reset I2S handle state
  * @param  __HANDLE__ specifies the I2S Handle.
  * @retval None
  */
#if (USE_HAL_I2S_REGISTER_CALLBACKS == 1U)
#define __HAL_I2S_RESET_HANDLE_STATE(__HANDLE__)                do{                                                  \
                                                                    (__HANDLE__)->State = HAL_I2S_STATE_RESET;       \
                                                                    (__HANDLE__)->MspInitCallback = NULL;            \
                                                                    (__HANDLE__)->MspDeInitCallback = NULL;          \
                                                                  } while(0)
#else
#define __HAL_I2S_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = HAL_I2S_STATE_RESET)
#endif

/** @brief  Enable the specified SPI peripheral (in I2S mode).
  * @param  __HANDLE__ specifies the I2S Handle.
  * @retval None
  */
#define __HAL_I2S_ENABLE(__HANDLE__)    (SET_BIT((__HANDLE__)->Instance->I2SCFGR, SPI_I2SCFGR_I2SE))

/** @brief  Disable the specified SPI peripheral (in I2S mode).
  * @param  __HANDLE__ specifies the I2S Handle.
  * @retval None
  */
#define __HAL_I2S_DISABLE(__HANDLE__) (CLEAR_BIT((__HANDLE__)->Instance->I2SCFGR, SPI_I2SCFGR_I2SE))

/** @brief  Enable the specified I2S interrupts.
  * @param  __HANDLE__ specifies the I2S Handle.
  * @param  __INTERRUPT__ specifies the interrupt source to enable or disable.
  *         This parameter can be one of the following values:
  *            @arg I2S_IT_TXE: Tx buffer empty interrupt enable
  *            @arg I2S_IT_RXNE: RX buffer not empty interrupt enable
  *            @arg I2S_IT_ERR: Error interrupt enable
  * @retval None
  */
#define __HAL_I2S_ENABLE_IT(__HANDLE__, __INTERRUPT__)    (SET_BIT((__HANDLE__)->Instance->CR2,(__INTERRUPT__)))

/** @brief  Disable the specified I2S interrupts.
  * @param  __HANDLE__ specifies the I2S Handle.
  * @param  __INTERRUPT__ specifies the interrupt source to enable or disable.
  *         This parameter can be one of the following values:
  *            @arg I2S_IT_TXE: Tx buffer empty interrupt enable
  *            @arg I2S_IT_RXNE: RX buffer not empty interrupt enable
  *            @arg I2S_IT_ERR: Error interrupt enable
  * @retval None
  */
#define __HAL_I2S_DISABLE_IT(__HANDLE__, __INTERRUPT__) (CLEAR_BIT((__HANDLE__)->Instance->CR2,(__INTERRUPT__)))

/** @brief  Checks if the specified I2S interrupt source is enabled or disabled.
  * @param  __HANDLE__ specifies the I2S Handle.
  *         This parameter can be I2S where x: 1, 2, or 3 to select the I2S peripheral.
  * @param  __INTERRUPT__ specifies the I2S interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg I2S_IT_TXE: Tx buffer empty interrupt enable
  *            @arg I2S_IT_RXNE: RX buffer not empty interrupt enable
  *            @arg I2S_IT_ERR: Error interrupt enable
  * @retval The new state of __IT__ (TRUE or FALSE).
  */
#define __HAL_I2S_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__) ((((__HANDLE__)->Instance->CR2 & (__INTERRUPT__)) == (__INTERRUPT__)) ? SET : RESET)

/** @brief  Checks whether the specified I2S flag is set or not.
  * @param  __HANDLE__ specifies the I2S Handle.
  * @param  __FLAG__ specifies the flag to check.
  *         This parameter can be one of the following values:
  *            @arg I2S_FLAG_RXNE: Receive buffer not empty flag
  *            @arg I2S_FLAG_TXE: Transmit buffer empty flag
  *            @arg I2S_FLAG_UDR: Underrun flag
  *            @arg I2S_FLAG_OVR: Overrun flag
  *            @arg I2S_FLAG_FRE: Frame error flag
  *            @arg I2S_FLAG_CHSIDE: Channel Side flag
  *            @arg I2S_FLAG_BSY: Busy flag
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_I2S_GET_FLAG(__HANDLE__, __FLAG__) ((((__HANDLE__)->Instance->SR) & (__FLAG__)) == (__FLAG__))

/** @brief Clears the I2S OVR pending flag.
  * @param  __HANDLE__ specifies the I2S Handle.
  * @retval None
  */
#define __HAL_I2S_CLEAR_OVRFLAG(__HANDLE__) do{ \
                                               __IO uint32_t tmpreg_ovr = 0x00U; \
                                               tmpreg_ovr = (__HANDLE__)->Instance->DR; \
                                               tmpreg_ovr = (__HANDLE__)->Instance->SR; \
                                               UNUSED(tmpreg_ovr); \
                                              }while(0U)
/** @brief Clears the I2S UDR pending flag.
  * @param  __HANDLE__ specifies the I2S Handle.
  * @retval None
  */
#define __HAL_I2S_CLEAR_UDRFLAG(__HANDLE__) do{\
                                               __IO uint32_t tmpreg_udr = 0x00U;\
                                               tmpreg_udr = ((__HANDLE__)->Instance->SR);\
                                               UNUSED(tmpreg_udr); \
                                              }while(0U)
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup I2S_Exported_Functions
  * @{
  */

/** @addtogroup I2S_Exported_Functions_Group1
  * @{
  */
/* Initialization/de-initialization functions  ********************************/
HAL_StatusTypeDef HAL_I2S_Init(I2S_HandleTypeDef *hi2s);
HAL_StatusTypeDef HAL_I2S_DeInit(I2S_HandleTypeDef *hi2s);
void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s);
void HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s);

/* Callbacks Register/UnRegister functions  ***********************************/
#if (USE_HAL_I2S_REGISTER_CALLBACKS == 1U)
HAL_StatusTypeDef HAL_I2S_RegisterCallback(I2S_HandleTypeDef *hi2s, HAL_I2S_CallbackIDTypeDef CallbackID, pI2S_CallbackTypeDef pCallback);
HAL_StatusTypeDef HAL_I2S_UnRegisterCallback(I2S_HandleTypeDef *hi2s, HAL_I2S_CallbackIDTypeDef CallbackID);
#endif /* USE_HAL_I2S_REGISTER_CALLBACKS */
/**
  * @}
  */

/** @addtogroup I2S_Exported_Functions_Group2
  * @{
  */
/* I/O operation functions  ***************************************************/
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_I2S_Transmit(I2S_HandleTypeDef *hi2s, uint16_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2S_Receive(I2S_HandleTypeDef *hi2s, uint16_t *pData, uint16_t Size, uint32_t Timeout);

/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_I2S_Transmit_IT(I2S_HandleTypeDef *hi2s, uint16_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2S_Receive_IT(I2S_HandleTypeDef *hi2s, uint16_t *pData, uint16_t Size);
void HAL_I2S_IRQHandler(I2S_HandleTypeDef *hi2s);

/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_I2S_Transmit_DMA(I2S_HandleTypeDef *hi2s, uint16_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2S_Receive_DMA(I2S_HandleTypeDef *hi2s, uint16_t *pData, uint16_t Size);

HAL_StatusTypeDef HAL_I2S_DMAPause(I2S_HandleTypeDef *hi2s);
HAL_StatusTypeDef HAL_I2S_DMAResume(I2S_HandleTypeDef *hi2s);
HAL_StatusTypeDef HAL_I2S_DMAStop(I2S_HandleTypeDef *hi2s);

/* Callbacks used in non blocking modes (Interrupt and DMA) *******************/
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s);
/**
  * @}
  */

/** @addtogroup I2S_Exported_Functions_Group3
  * @{
  */
/* Peripheral Control and State functions  ************************************/
HAL_I2S_StateTypeDef HAL_I2S_GetState(I2S_HandleTypeDef *hi2s);
uint32_t HAL_I2S_GetError(I2S_HandleTypeDef *hi2s);
/**
  * @}
  */

/**
  * @}
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup I2S_Private_Constants I2S Private Constants
  * @{
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup I2S_Private_Macros I2S Private Macros
  * @{
  */
#define IS_I2S_MODE(MODE) (((MODE) == I2S_MODE_SLAVE_TX)  || \
                           ((MODE) == I2S_MODE_SLAVE_RX)  || \
                           ((MODE) == I2S_MODE_MASTER_TX) || \
                           ((MODE) == I2S_MODE_MASTER_RX))

#define IS_I2S_STANDARD(STANDARD) (((STANDARD) == I2S_STANDARD_PHILIPS)   || \
                                   ((STANDARD) == I2S_STANDARD_MSB)       || \
                                   ((STANDARD) == I2S_STANDARD_LSB)       || \
                                   ((STANDARD) == I2S_STANDARD_PCM_SHORT) || \
                                   ((STANDARD) == I2S_STANDARD_PCM_LONG))

#define IS_I2S_DATA_FORMAT(FORMAT) (((FORMAT) == I2S_DATAFORMAT_16B)          || \
                                    ((FORMAT) == I2S_DATAFORMAT_16B_EXTENDED) || \
                                    ((FORMAT) == I2S_DATAFORMAT_24B)          || \
                                    ((FORMAT) == I2S_DATAFORMAT_32B))

#define IS_I2S_MCLK_OUTPUT(OUTPUT) (((OUTPUT) == I2S_MCLKOUTPUT_ENABLE) || \
                                    ((OUTPUT) == I2S_MCLKOUTPUT_DISABLE))

#define IS_I2S_AUDIO_FREQ(FREQ) ((((FREQ) >= I2S_AUDIOFREQ_8K)    && \
                                  ((FREQ) <= I2S_AUDIOFREQ_192K)) || \
                                  ((FREQ) == I2S_AUDIOFREQ_DEFAULT))

#define IS_I2S_CPOL(CPOL) (((CPOL) == I2S_CPOL_LOW) || \
                           ((CPOL) == I2S_CPOL_HIGH))

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
#endif /* SPI_I2S_SUPPORT */

#ifdef __cplusplus
}
#endif

#endif /* STM32G0xx_HAL_I2S_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
