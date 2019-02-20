/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_ipcc.h
  * @author  MCD Application Team
  * @brief   Header file of Mailbox HAL module.
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
#ifndef STM32MP1xx_HAL_IPCC_H
#define STM32MP1xx_HAL_IPCC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"


/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @defgroup IPCC IPCC
  * @brief IPCC HAL module driver
  * @{
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup IPCC_Exported_Constants IPCC Exported Constants
  * @{
  */

/** @defgroup IPCC_Channel IPCC Channel
  * @{
  */
#define IPCC_CHANNEL_1 0x00000000U
#define IPCC_CHANNEL_2 0x00000001U
#define IPCC_CHANNEL_3 0x00000002U
#define IPCC_CHANNEL_4 0x00000003U
#define IPCC_CHANNEL_5 0x00000004U
#define IPCC_CHANNEL_6 0x00000005U
/**
  * @}
  */

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup IPCC_Exported_Types IPCC Exported Types
  * @{
  */

/**
  * @brief HAL IPCC State structures definition
  */
typedef enum
{
  HAL_IPCC_STATE_RESET             = 0x00U,  /*!< IPCC not yet initialized or disabled  */
  HAL_IPCC_STATE_READY             = 0x01U,  /*!< IPCC initialized and ready for use    */
  HAL_IPCC_STATE_BUSY              = 0x02U   /*!< IPCC internal processing is ongoing   */
} HAL_IPCC_StateTypeDef;

/**
  * @brief  IPCC channel direction structure definition
  */
typedef enum
{
  IPCC_CHANNEL_DIR_TX  = 0x00U,  /*!< Channel direction Tx is used by an MCU to transmit */
  IPCC_CHANNEL_DIR_RX  = 0x01U   /*!< Channel direction Rx is used by an MCU to receive */
} IPCC_CHANNELDirTypeDef;

/**
  * @brief  IPCC channel status structure definition
  */
typedef enum
{
  IPCC_CHANNEL_STATUS_FREE       = 0x00U,  /*!< Means that a new msg can be posted on that channel */
  IPCC_CHANNEL_STATUS_OCCUPIED   = 0x01U   /*!< An MCU has posted a msg the other MCU hasn't retrieved */
} IPCC_CHANNELStatusTypeDef;

/**
  * @brief  IPCC handle structure definition
  */
typedef struct __IPCC_HandleTypeDef
{
  IPCC_TypeDef                   *Instance;     /*!< IPCC registers base address */
  void (* ChannelCallbackRx[IPCC_CHANNEL_NUMBER])(struct __IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir);                            /*!< Rx Callback registration table */
  void (* ChannelCallbackTx[IPCC_CHANNEL_NUMBER])(struct __IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir);                            /*!< Tx Callback registration table */
  uint32_t                       callbackRequest; /*!< Store information about callback notification by channel */
  __IO HAL_IPCC_StateTypeDef      State;         /*!< IPCC State: initialized or not */
} IPCC_HandleTypeDef;

/**
  * @brief  IPCC callback typedef
  */
typedef void ChannelCb(IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir);

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup IPCC_Exported_Macros IPCC Exported Macros
  * @{
  */

/**
  * @brief  Enable the specified interrupt.
  * @param  __HANDLE__ specifies the IPCC Handle
  * @param  __CHDIRECTION__ specifies the channels Direction
  *          This parameter can be one of the following values:
  *            @arg @ref IPCC_CHANNEL_DIR_TX Transmit channel free interrupt enable
  *            @arg @ref IPCC_CHANNEL_DIR_RX Receive channel occupied interrupt enable
  */
#if defined(CORE_CM4)
#define __HAL_IPCC_ENABLE_IT(__HANDLE__, __CHDIRECTION__) \
            (((__CHDIRECTION__) == IPCC_CHANNEL_DIR_RX) ? \
                ((__HANDLE__)->Instance->C2CR |= IPCC_C2CR_RXOIE) : \
                ((__HANDLE__)->Instance->C2CR |= IPCC_C2CR_TXFIE))
#else
#define __HAL_IPCC_ENABLE_IT(__HANDLE__, __CHDIRECTION__) \
            (((__CHDIRECTION__) == IPCC_CHANNEL_DIR_RX) ? \
                ((__HANDLE__)->Instance->C1CR |= IPCC_C1CR_RXOIE) : \
                ((__HANDLE__)->Instance->C1CR |= IPCC_C1CR_TXFIE))
#endif

/**
  * @brief  Disable the specified interrupt.
  * @param  __HANDLE__ specifies the IPCC Handle
  * @param  __CHDIRECTION__ specifies the channels Direction
  *          This parameter can be one of the following values:
  *            @arg @ref IPCC_CHANNEL_DIR_TX Transmit channel free interrupt enable
  *            @arg @ref IPCC_CHANNEL_DIR_RX Receive channel occupied interrupt enable
  */
#if defined(CORE_CM4)
#define __HAL_IPCC_DISABLE_IT(__HANDLE__, __CHDIRECTION__) \
            (((__CHDIRECTION__) == IPCC_CHANNEL_DIR_RX) ? \
                ((__HANDLE__)->Instance->C2CR &= ~IPCC_C2CR_RXOIE) : \
                ((__HANDLE__)->Instance->C2CR &= ~IPCC_C2CR_TXFIE))
#else
#define __HAL_IPCC_DISABLE_IT(__HANDLE__, __CHDIRECTION__) \
            (((__CHDIRECTION__) == IPCC_CHANNEL_DIR_RX) ? \
                ((__HANDLE__)->Instance->C1CR &= ~IPCC_C1CR_RXOIE) : \
                ((__HANDLE__)->Instance->C1CR &= ~IPCC_C1CR_TXFIE))
#endif

/**
  * @brief  Mask the specified interrupt.
  * @param  __HANDLE__ specifies the IPCC Handle
  * @param  __CHDIRECTION__ specifies the channels Direction
  *          This parameter can be one of the following values:
  *            @arg @ref IPCC_CHANNEL_DIR_TX Transmit channel free interrupt enable
  *            @arg @ref IPCC_CHANNEL_DIR_RX Receive channel occupied interrupt enable
  * @param  __CHINDEX__ specifies the channels number:
  *         This parameter can be one of the following values:
  *            @arg IPCC_CHANNEL_1: IPCC Channel 1
  *            @arg IPCC_CHANNEL_2: IPCC Channel 2
  *            @arg IPCC_CHANNEL_3: IPCC Channel 3
  *            @arg IPCC_CHANNEL_4: IPCC Channel 4
  *            @arg IPCC_CHANNEL_5: IPCC Channel 5
  *            @arg IPCC_CHANNEL_6: IPCC Channel 6
  */
#if defined(CORE_CM4)
#define __HAL_IPCC_MASK_CHANNEL_IT(__HANDLE__, __CHDIRECTION__, __CHINDEX__) \
            (((__CHDIRECTION__) == IPCC_CHANNEL_DIR_RX) ? \
                ((__HANDLE__)->Instance->C2MR |= (IPCC_C1MR_CH1OM_Msk << (__CHINDEX__))) : \
                ((__HANDLE__)->Instance->C2MR |= (IPCC_C1MR_CH1FM_Msk << (__CHINDEX__))))
#else
#define __HAL_IPCC_MASK_CHANNEL_IT(__HANDLE__, __CHDIRECTION__, __CHINDEX__) \
            (((__CHDIRECTION__) == IPCC_CHANNEL_DIR_RX) ? \
                ((__HANDLE__)->Instance->C1MR |= (IPCC_C1MR_CH1OM_Msk << (__CHINDEX__))) : \
                ((__HANDLE__)->Instance->C1MR |= (IPCC_C1MR_CH1FM_Msk << (__CHINDEX__))))
#endif

/**
  * @brief  Unmask the specified interrupt.
  * @param  __HANDLE__ specifies the IPCC Handle
  * @param  __CHDIRECTION__ specifies the channels Direction
  *          This parameter can be one of the following values:
  *            @arg @ref IPCC_CHANNEL_DIR_TX Transmit channel free interrupt enable
  *            @arg @ref IPCC_CHANNEL_DIR_RX Receive channel occupied interrupt enable
  * @param  __CHINDEX__ specifies the channels number:
  *         This parameter can be one of the following values:
  *            @arg IPCC_CHANNEL_1: IPCC Channel 1
  *            @arg IPCC_CHANNEL_2: IPCC Channel 2
  *            @arg IPCC_CHANNEL_3: IPCC Channel 3
  *            @arg IPCC_CHANNEL_4: IPCC Channel 4
  *            @arg IPCC_CHANNEL_5: IPCC Channel 5
  *            @arg IPCC_CHANNEL_6: IPCC Channel 6
  */
#if defined(CORE_CM4)
#define __HAL_IPCC_UNMASK_CHANNEL_IT(__HANDLE__, __CHDIRECTION__, __CHINDEX__) \
            (((__CHDIRECTION__) == IPCC_CHANNEL_DIR_RX) ? \
                ((__HANDLE__)->Instance->C2MR &= ~(IPCC_C1MR_CH1OM_Msk << (__CHINDEX__))) : \
                ((__HANDLE__)->Instance->C2MR &= ~(IPCC_C1MR_CH1FM_Msk << (__CHINDEX__))))
#else
#define __HAL_IPCC_UNMASK_CHANNEL_IT(__HANDLE__, __CHDIRECTION__, __CHINDEX__) \
            (((__CHDIRECTION__) == IPCC_CHANNEL_DIR_RX) ? \
                ((__HANDLE__)->Instance->C1MR &= ~(IPCC_C1MR_CH1OM_Msk << (__CHINDEX__))) : \
                ((__HANDLE__)->Instance->C1MR &= ~(IPCC_C1MR_CH1FM_Msk << (__CHINDEX__))))
#endif

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup IPCC_Exported_Functions IPCC Exported Functions
  * @{
  */

/* Initialization and de-initialization functions *******************************/
/** @defgroup IPCC_Exported_Functions_Group1 Initialization and deinitialization functions
 *  @{
 */
HAL_StatusTypeDef HAL_IPCC_Init(IPCC_HandleTypeDef *hipcc);
HAL_StatusTypeDef HAL_IPCC_DeInit(IPCC_HandleTypeDef *hipcc);
void HAL_IPCC_MspInit(IPCC_HandleTypeDef *hipcc);
void HAL_IPCC_MspDeInit(IPCC_HandleTypeDef *hipcc);
/**
  * @}
  */

/** @defgroup IPCC_Exported_Functions_Group2 Communication functions
 *  @{
 */
/* IO operation functions  *****************************************************/
HAL_StatusTypeDef HAL_IPCC_ActivateNotification(IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir, ChannelCb cb);
HAL_StatusTypeDef HAL_IPCC_DeActivateNotification(IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir);
IPCC_CHANNELStatusTypeDef HAL_IPCC_GetChannelStatus(IPCC_HandleTypeDef const *const hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir);
HAL_StatusTypeDef HAL_IPCC_NotifyCPU(IPCC_HandleTypeDef const *const hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir);
/**
  * @}
  */

/** @defgroup IPCC_Exported_Functions_Group3 Peripheral State and Error functions
 *  @{
 */
/* Peripheral State and Error functions ****************************************/
HAL_IPCC_StateTypeDef HAL_IPCC_GetState(IPCC_HandleTypeDef const *const hipcc);
/**
  * @}
  */

/** @defgroup IPCC_IRQ_Handler_and_Callbacks Peripheral IRQ Handler and Callbacks
 *  @{
 */
/* IRQHandler and Callbacks used in non blocking modes  ************************/
void HAL_IPCC_TX_IRQHandler(IPCC_HandleTypeDef   *const hipcc);
void HAL_IPCC_RX_IRQHandler(IPCC_HandleTypeDef *const hipcc);
void HAL_IPCC_TxCallback(IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir);
void HAL_IPCC_RxCallback(IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir);
/**
  * @}
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

#endif /* STM32MP1xx_HAL_IPCC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
