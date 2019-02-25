/**
  ******************************************************************************
  * @file    stm32wbxx_ll_ipcc.h
  * @author  MCD Application Team
  * @brief   Header file of IPCC LL module.
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
#ifndef STM32WBxx_LL_IPCC_H
#define STM32WBxx_LL_IPCC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx.h"

/** @addtogroup STM32WBxx_LL_Driver
  * @{
  */

#if defined(IPCC)

/** @defgroup IPCC_LL IPCC
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup IPCC_LL_Exported_Constants IPCC Exported Constants
  * @{
  */

/** @defgroup IPCC_LL_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_IPCC_ReadReg function
  * @{
  */
#define LL_IPCC_C1TOC2SR_CH1F IPCC_C1TOC2SR_CH1F_Msk /*!< C1 transmit to C2 receive Channel1 status flag before masking */
#define LL_IPCC_C1TOC2SR_CH2F IPCC_C1TOC2SR_CH2F_Msk /*!< C1 transmit to C2 receive Channel2 status flag before masking */
#define LL_IPCC_C1TOC2SR_CH3F IPCC_C1TOC2SR_CH3F_Msk /*!< C1 transmit to C2 receive Channel3 status flag before masking */
#define LL_IPCC_C1TOC2SR_CH4F IPCC_C1TOC2SR_CH4F_Msk /*!< C1 transmit to C2 receive Channel4 status flag before masking */
#define LL_IPCC_C1TOC2SR_CH5F IPCC_C1TOC2SR_CH5F_Msk /*!< C1 transmit to C2 receive Channel5 status flag before masking */
#define LL_IPCC_C1TOC2SR_CH6F IPCC_C1TOC2SR_CH6F_Msk /*!< C1 transmit to C2 receive Channel6 status flag before masking */
#define LL_IPCC_C2TOC1SR_CH1F IPCC_C2TOC1SR_CH1F_Msk /*!< C2 transmit to C1 receive Channel1 status flag before masking */
#define LL_IPCC_C2TOC1SR_CH2F IPCC_C2TOC1SR_CH2F_Msk /*!< C2 transmit to C1 receive Channel2 status flag before masking */
#define LL_IPCC_C2TOC1SR_CH3F IPCC_C2TOC1SR_CH3F_Msk /*!< C2 transmit to C1 receive Channel3 status flag before masking */
#define LL_IPCC_C2TOC1SR_CH4F IPCC_C2TOC1SR_CH4F_Msk /*!< C2 transmit to C1 receive Channel4 status flag before masking */
#define LL_IPCC_C2TOC1SR_CH5F IPCC_C2TOC1SR_CH5F_Msk /*!< C2 transmit to C1 receive Channel5 status flag before masking */
#define LL_IPCC_C2TOC1SR_CH6F IPCC_C2TOC1SR_CH6F_Msk /*!< C2 transmit to C1 receive Channel6 status flag before masking */

/**
  * @}
  */

/** @defgroup IPCC_LL_EC_Channel Channel
  * @{
  */
#define LL_IPCC_CHANNEL_1 (0x00000001U) /*!< IPCC Channel 1 */
#define LL_IPCC_CHANNEL_2 (0x00000002U) /*!< IPCC Channel 2 */
#define LL_IPCC_CHANNEL_3 (0x00000004U) /*!< IPCC Channel 3 */
#define LL_IPCC_CHANNEL_4 (0x00000008U) /*!< IPCC Channel 4 */
#define LL_IPCC_CHANNEL_5 (0x00000010U) /*!< IPCC Channel 5 */
#define LL_IPCC_CHANNEL_6 (0x00000020U) /*!< IPCC Channel 6 */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup IPCC_LL_Exported_Macros IPCC Exported Macros
  * @{
  */

/** @defgroup IPCC_LL_EM_WRITE_READ Common Write and read registers Macros
  * @{
  */

/**
  * @brief  Write a value in IPCC register
  * @param  __INSTANCE__ IPCC Instance
  * @param  __REG__ Register to be written
  * @param  __VALUE__ Value to be written in the register
  * @retval None
  */
#define LL_IPCC_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
  * @brief  Read a value in IPCC register
  * @param  __INSTANCE__ IPCC Instance
  * @param  __REG__ Register to be read
  * @retval Register value
  */
#define LL_IPCC_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
  * @}
  */

/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/
/** @defgroup IPCC_LL_Exported_Functions IPCC Exported Functions
  * @{
  */

/** @defgroup IPCC_LL_EF_IT_Management IT_Management
  * @{
  */

/**
  * @brief  Enable Transmit channel free interrupt for processor 1.
  * @rmtoll C1CR          TXFIE         LL_C1_IPCC_EnableIT_TXF
  * @param  IPCCx IPCC Instance.
  * @retval None
  */
__STATIC_INLINE void LL_C1_IPCC_EnableIT_TXF(IPCC_TypeDef *IPCCx)
{
  SET_BIT(IPCCx->C1CR, IPCC_C1CR_TXFIE);
}

/**
  * @brief  Disable Transmit channel free interrupt for processor 1.
  * @rmtoll C1CR          TXFIE         LL_C1_IPCC_DisableIT_TXF
  * @param  IPCCx IPCC Instance.
  * @retval None
  */
__STATIC_INLINE void LL_C1_IPCC_DisableIT_TXF(IPCC_TypeDef *IPCCx)
{
  CLEAR_BIT(IPCCx->C1CR, IPCC_C1CR_TXFIE);
}

/**
  * @brief  Check if Transmit channel free interrupt for processor 1 is enabled.
  * @rmtoll C1CR          TXFIE         LL_C1_IPCC_IsEnabledIT_TXF
  * @param  IPCCx IPCC Instance.
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C1_IPCC_IsEnabledIT_TXF(IPCC_TypeDef const *const IPCCx)
{
  return ((READ_BIT(IPCCx->C1CR, IPCC_C1CR_TXFIE) == (IPCC_C1CR_TXFIE)) ? 1UL : 0UL);
}

/**
  * @brief  Enable Receive channel occupied interrupt for processor 1.
  * @rmtoll C1CR          RXOIE         LL_C1_IPCC_EnableIT_RXO
  * @param  IPCCx IPCC Instance.
  * @retval None
  */
__STATIC_INLINE void LL_C1_IPCC_EnableIT_RXO(IPCC_TypeDef *IPCCx)
{
  SET_BIT(IPCCx->C1CR, IPCC_C1CR_RXOIE);
}

/**
  * @brief  Disable Receive channel occupied interrupt for processor 1.
  * @rmtoll C1CR          RXOIE         LL_C1_IPCC_DisableIT_RXO
  * @param  IPCCx IPCC Instance.
  * @retval None
  */
__STATIC_INLINE void LL_C1_IPCC_DisableIT_RXO(IPCC_TypeDef *IPCCx)
{
  CLEAR_BIT(IPCCx->C1CR, IPCC_C1CR_RXOIE);
}

/**
  * @brief  Check if Receive channel occupied interrupt for processor 1 is enabled.
  * @rmtoll C1CR          RXOIE         LL_C1_IPCC_IsEnabledIT_RXO
  * @param  IPCCx IPCC Instance.
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C1_IPCC_IsEnabledIT_RXO(IPCC_TypeDef  const *const IPCCx)
{
  return ((READ_BIT(IPCCx->C1CR, IPCC_C1CR_RXOIE) == (IPCC_C1CR_RXOIE)) ? 1UL : 0UL);
}

/**
  * @brief  Enable Transmit channel free interrupt for processor 2.
  * @rmtoll C2CR          TXFIE         LL_C2_IPCC_EnableIT_TXF
  * @param  IPCCx IPCC Instance.
  * @retval None
  */
__STATIC_INLINE void LL_C2_IPCC_EnableIT_TXF(IPCC_TypeDef *IPCCx)
{
  SET_BIT(IPCCx->C2CR, IPCC_C2CR_TXFIE);
}

/**
  * @brief  Disable Transmit channel free interrupt for processor 2.
  * @rmtoll C2CR          TXFIE         LL_C2_IPCC_DisableIT_TXF
  * @param  IPCCx IPCC Instance.
  * @retval None
  */
__STATIC_INLINE void LL_C2_IPCC_DisableIT_TXF(IPCC_TypeDef *IPCCx)
{
  CLEAR_BIT(IPCCx->C2CR, IPCC_C2CR_TXFIE);
}

/**
  * @brief  Check if Transmit channel free interrupt for processor 2 is enabled.
  * @rmtoll C2CR          TXFIE         LL_C2_IPCC_IsEnabledIT_TXF
  * @param  IPCCx IPCC Instance.
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C2_IPCC_IsEnabledIT_TXF(IPCC_TypeDef  const *const IPCCx)
{
  return ((READ_BIT(IPCCx->C2CR, IPCC_C2CR_TXFIE) == (IPCC_C2CR_TXFIE)) ? 1UL : 0UL);
}

/**
  * @brief  Enable Receive channel occupied interrupt for processor 2.
  * @rmtoll C2CR          RXOIE         LL_C2_IPCC_EnableIT_RXO
  * @param  IPCCx IPCC Instance.
  * @retval None
  */
__STATIC_INLINE void LL_C2_IPCC_EnableIT_RXO(IPCC_TypeDef *IPCCx)
{
  SET_BIT(IPCCx->C2CR, IPCC_C2CR_RXOIE);
}

/**
  * @brief  Disable Receive channel occupied interrupt for processor 2.
  * @rmtoll C2CR          RXOIE         LL_C2_IPCC_DisableIT_RXO
  * @param  IPCCx IPCC Instance.
  * @retval None
  */
__STATIC_INLINE void LL_C2_IPCC_DisableIT_RXO(IPCC_TypeDef *IPCCx)
{
  CLEAR_BIT(IPCCx->C2CR, IPCC_C2CR_RXOIE);
}

/**
  * @brief  Check if Receive channel occupied interrupt for processor 2 is enabled.
  * @rmtoll C2CR          RXOIE         LL_C2_IPCC_IsEnabledIT_RXO
  * @param  IPCCx IPCC Instance.
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C2_IPCC_IsEnabledIT_RXO(IPCC_TypeDef const *const IPCCx)
{
  return ((READ_BIT(IPCCx->C2CR, IPCC_C2CR_RXOIE) == (IPCC_C2CR_RXOIE)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup IPCC_LL_EF_Configuration Configuration
  * @{
  */

/**
  * @brief  Unmask transmit channel free interrupt for processor 1.
  * @rmtoll C1MR        CH1FM           LL_C1_IPCC_EnableTransmitChannel\n
  *         C1MR        CH2FM           LL_C1_IPCC_EnableTransmitChannel\n
  *         C1MR        CH3FM           LL_C1_IPCC_EnableTransmitChannel\n
  *         C1MR        CH4FM           LL_C1_IPCC_EnableTransmitChannel\n
  *         C1MR        CH5FM           LL_C1_IPCC_EnableTransmitChannel\n
  *         C1MR        CH6FM           LL_C1_IPCC_EnableTransmitChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C1_IPCC_EnableTransmitChannel(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  CLEAR_BIT(IPCCx->C1MR, Channel << IPCC_C1MR_CH1FM_Pos);
}

/**
  * @brief  Mask transmit channel free interrupt for processor 1.
  * @rmtoll C1MR        CH1FM           LL_C1_IPCC_DisableTransmitChannel\n
  *         C1MR        CH2FM           LL_C1_IPCC_DisableTransmitChannel\n
  *         C1MR        CH3FM           LL_C1_IPCC_DisableTransmitChannel\n
  *         C1MR        CH4FM           LL_C1_IPCC_DisableTransmitChannel\n
  *         C1MR        CH5FM           LL_C1_IPCC_DisableTransmitChannel\n
  *         C1MR        CH6FM           LL_C1_IPCC_DisableTransmitChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C1_IPCC_DisableTransmitChannel(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  SET_BIT(IPCCx->C1MR, Channel << IPCC_C1MR_CH1FM_Pos);
}

/**
  * @brief  Check if Transmit channel free interrupt for processor 1 is masked.
  * @rmtoll C1MR        CH1FM           LL_C1_IPCC_IsEnabledTransmitChannel\n
  *         C1MR        CH2FM           LL_C1_IPCC_IsEnabledTransmitChannel\n
  *         C1MR        CH3FM           LL_C1_IPCC_IsEnabledTransmitChannel\n
  *         C1MR        CH4FM           LL_C1_IPCC_IsEnabledTransmitChannel\n
  *         C1MR        CH5FM           LL_C1_IPCC_IsEnabledTransmitChannel\n
  *         C1MR        CH6FM           LL_C1_IPCC_IsEnabledTransmitChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C1_IPCC_IsEnabledTransmitChannel(IPCC_TypeDef const *const IPCCx, uint32_t Channel)
{
  return ((READ_BIT(IPCCx->C1MR, Channel << IPCC_C1MR_CH1FM_Pos) != (Channel << IPCC_C1MR_CH1FM_Pos)) ? 1UL : 0UL);
}

/**
  * @brief  Unmask receive channel occupied interrupt for processor 1.
  * @rmtoll C1MR        CH1OM           LL_C1_IPCC_EnableReceiveChannel\n
  *         C1MR        CH2OM           LL_C1_IPCC_EnableReceiveChannel\n
  *         C1MR        CH3OM           LL_C1_IPCC_EnableReceiveChannel\n
  *         C1MR        CH4OM           LL_C1_IPCC_EnableReceiveChannel\n
  *         C1MR        CH5OM           LL_C1_IPCC_EnableReceiveChannel\n
  *         C1MR        CH6OM           LL_C1_IPCC_EnableReceiveChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C1_IPCC_EnableReceiveChannel(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  CLEAR_BIT(IPCCx->C1MR, Channel);
}

/**
  * @brief  Mask receive channel occupied interrupt for processor 1.
  * @rmtoll C1MR        CH1OM           LL_C1_IPCC_DisableReceiveChannel\n
  *         C1MR        CH2OM           LL_C1_IPCC_DisableReceiveChannel\n
  *         C1MR        CH3OM           LL_C1_IPCC_DisableReceiveChannel\n
  *         C1MR        CH4OM           LL_C1_IPCC_DisableReceiveChannel\n
  *         C1MR        CH5OM           LL_C1_IPCC_DisableReceiveChannel\n
  *         C1MR        CH6OM           LL_C1_IPCC_DisableReceiveChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C1_IPCC_DisableReceiveChannel(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  SET_BIT(IPCCx->C1MR, Channel);
}

/**
  * @brief  Check if Receive channel occupied interrupt for processor 1 is masked.
  * @rmtoll C1MR        CH1OM           LL_C1_IPCC_IsEnabledReceiveChannel\n
  *         C1MR        CH2OM           LL_C1_IPCC_IsEnabledReceiveChannel\n
  *         C1MR        CH3OM           LL_C1_IPCC_IsEnabledReceiveChannel\n
  *         C1MR        CH4OM           LL_C1_IPCC_IsEnabledReceiveChannel\n
  *         C1MR        CH5OM           LL_C1_IPCC_IsEnabledReceiveChannel\n
  *         C1MR        CH6OM           LL_C1_IPCC_IsEnabledReceiveChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C1_IPCC_IsEnabledReceiveChannel(IPCC_TypeDef const *const IPCCx, uint32_t Channel)
{
  return ((READ_BIT(IPCCx->C1MR, Channel) != (Channel)) ? 1UL : 0UL);
}

/**
  * @brief  Unmask transmit channel free interrupt for processor 2.
  * @rmtoll C2MR        CH1FM           LL_C2_IPCC_EnableTransmitChannel\n
  *         C2MR        CH2FM           LL_C2_IPCC_EnableTransmitChannel\n
  *         C2MR        CH3FM           LL_C2_IPCC_EnableTransmitChannel\n
  *         C2MR        CH4FM           LL_C2_IPCC_EnableTransmitChannel\n
  *         C2MR        CH5FM           LL_C2_IPCC_EnableTransmitChannel\n
  *         C2MR        CH6FM           LL_C2_IPCC_EnableTransmitChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C2_IPCC_EnableTransmitChannel(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  CLEAR_BIT(IPCCx->C2MR, Channel << IPCC_C2MR_CH1FM_Pos);
}

/**
  * @brief  Mask transmit channel free interrupt for processor 2.
  * @rmtoll C2MR        CH1FM           LL_C2_IPCC_DisableTransmitChannel\n
  *         C2MR        CH2FM           LL_C2_IPCC_DisableTransmitChannel\n
  *         C2MR        CH3FM           LL_C2_IPCC_DisableTransmitChannel\n
  *         C2MR        CH4FM           LL_C2_IPCC_DisableTransmitChannel\n
  *         C2MR        CH5FM           LL_C2_IPCC_DisableTransmitChannel\n
  *         C2MR        CH6FM           LL_C2_IPCC_DisableTransmitChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C2_IPCC_DisableTransmitChannel(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  SET_BIT(IPCCx->C2MR, Channel << (IPCC_C2MR_CH1FM_Pos));
}

/**
  * @brief  Check if Transmit channel free interrupt for processor 2 is masked.
  * @rmtoll C2MR        CH1FM           LL_C2_IPCC_IsEnabledTransmitChannel\n
  *         C2MR        CH2FM           LL_C2_IPCC_IsEnabledTransmitChannel\n
  *         C2MR        CH3FM           LL_C2_IPCC_IsEnabledTransmitChannel\n
  *         C2MR        CH4FM           LL_C2_IPCC_IsEnabledTransmitChannel\n
  *         C2MR        CH5FM           LL_C2_IPCC_IsEnabledTransmitChannel\n
  *         C2MR        CH6FM           LL_C2_IPCC_IsEnabledTransmitChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C2_IPCC_IsEnabledTransmitChannel(IPCC_TypeDef const *const IPCCx, uint32_t Channel)
{
  return ((READ_BIT(IPCCx->C2MR, Channel << IPCC_C2MR_CH1FM_Pos) != (Channel << IPCC_C2MR_CH1FM_Pos)) ? 1UL : 0UL);
}

/**
  * @brief  Unmask receive channel occupied interrupt for processor 2.
  * @rmtoll C2MR        CH1OM           LL_C2_IPCC_EnableReceiveChannel\n
  *         C2MR        CH2OM           LL_C2_IPCC_EnableReceiveChannel\n
  *         C2MR        CH3OM           LL_C2_IPCC_EnableReceiveChannel\n
  *         C2MR        CH4OM           LL_C2_IPCC_EnableReceiveChannel\n
  *         C2MR        CH5OM           LL_C2_IPCC_EnableReceiveChannel\n
  *         C2MR        CH6OM           LL_C2_IPCC_EnableReceiveChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C2_IPCC_EnableReceiveChannel(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  CLEAR_BIT(IPCCx->C2MR, Channel);
}

/**
  * @brief  Mask receive channel occupied interrupt for processor 1.
  * @rmtoll C2MR        CH1OM           LL_C2_IPCC_DisableReceiveChannel\n
  *         C2MR        CH2OM           LL_C2_IPCC_DisableReceiveChannel\n
  *         C2MR        CH3OM           LL_C2_IPCC_DisableReceiveChannel\n
  *         C2MR        CH4OM           LL_C2_IPCC_DisableReceiveChannel\n
  *         C2MR        CH5OM           LL_C2_IPCC_DisableReceiveChannel\n
  *         C2MR        CH6OM           LL_C2_IPCC_DisableReceiveChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C2_IPCC_DisableReceiveChannel(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  SET_BIT(IPCCx->C2MR, Channel);
}

/**
  * @brief  Check if Receive channel occupied interrupt for processor 2 is masked.
  * @rmtoll C2MR        CH1OM           LL_C2_IPCC_IsEnabledReceiveChannel\n
  *         C2MR        CH2OM           LL_C2_IPCC_IsEnabledReceiveChannel\n
  *         C2MR        CH3OM           LL_C2_IPCC_IsEnabledReceiveChannel\n
  *         C2MR        CH4OM           LL_C2_IPCC_IsEnabledReceiveChannel\n
  *         C2MR        CH5OM           LL_C2_IPCC_IsEnabledReceiveChannel\n
  *         C2MR        CH6OM           LL_C2_IPCC_IsEnabledReceiveChannel
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C2_IPCC_IsEnabledReceiveChannel(IPCC_TypeDef const *const IPCCx, uint32_t Channel)
{
  return ((READ_BIT(IPCCx->C2MR, Channel) != (Channel)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup IPCC_LL_EF_FLAG_Management FLAG_Management
  * @{
  */

/**
  * @brief  Clear IPCC receive channel status for processor 1.
  * @note   Associated with IPCC_C2TOC1SR.CHxF
  * @rmtoll C1SCR        CH1C           LL_C1_IPCC_ClearFlag_CHx\n
  *         C1SCR        CH2C           LL_C1_IPCC_ClearFlag_CHx\n
  *         C1SCR        CH3C           LL_C1_IPCC_ClearFlag_CHx\n
  *         C1SCR        CH4C           LL_C1_IPCC_ClearFlag_CHx\n
  *         C1SCR        CH5C           LL_C1_IPCC_ClearFlag_CHx\n
  *         C1SCR        CH6C           LL_C1_IPCC_ClearFlag_CHx
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C1_IPCC_ClearFlag_CHx(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  WRITE_REG(IPCCx->C1SCR, Channel);
}

/**
  * @brief  Set IPCC transmit channel status for processor 1.
  * @note   Associated with IPCC_C1TOC2SR.CHxF
  * @rmtoll C1SCR        CH1S           LL_C1_IPCC_SetFlag_CHx\n
  *         C1SCR        CH2S           LL_C1_IPCC_SetFlag_CHx\n
  *         C1SCR        CH3S           LL_C1_IPCC_SetFlag_CHx\n
  *         C1SCR        CH4S           LL_C1_IPCC_SetFlag_CHx\n
  *         C1SCR        CH5S           LL_C1_IPCC_SetFlag_CHx\n
  *         C1SCR        CH6S           LL_C1_IPCC_SetFlag_CHx
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C1_IPCC_SetFlag_CHx(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  WRITE_REG(IPCCx->C1SCR, Channel << IPCC_C1SCR_CH1S_Pos);
}

/**
  * @brief  Get channel status for processor 1.
  * @rmtoll C1TOC2SR        CH1F           LL_C1_IPCC_IsActiveFlag_CHx\n
  *         C1TOC2SR        CH2F           LL_C1_IPCC_IsActiveFlag_CHx\n
  *         C1TOC2SR        CH3F           LL_C1_IPCC_IsActiveFlag_CHx\n
  *         C1TOC2SR        CH4F           LL_C1_IPCC_IsActiveFlag_CHx\n
  *         C1TOC2SR        CH5F           LL_C1_IPCC_IsActiveFlag_CHx\n
  *         C1TOC2SR        CH6F           LL_C1_IPCC_IsActiveFlag_CHx
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C1_IPCC_IsActiveFlag_CHx(IPCC_TypeDef  const *const IPCCx, uint32_t Channel)
{
  return ((READ_BIT(IPCCx->C1TOC2SR, Channel) == (Channel)) ? 1UL : 0UL);
}

/**
  * @brief  Clear IPCC receive channel status for processor 2.
  * @note   Associated with IPCC_C1TOC2SR.CHxF
  * @rmtoll C2SCR        CH1C           LL_C2_IPCC_ClearFlag_CHx\n
  *         C2SCR        CH2C           LL_C2_IPCC_ClearFlag_CHx\n
  *         C2SCR        CH3C           LL_C2_IPCC_ClearFlag_CHx\n
  *         C2SCR        CH4C           LL_C2_IPCC_ClearFlag_CHx\n
  *         C2SCR        CH5C           LL_C2_IPCC_ClearFlag_CHx\n
  *         C2SCR        CH6C           LL_C2_IPCC_ClearFlag_CHx
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C2_IPCC_ClearFlag_CHx(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  WRITE_REG(IPCCx->C2SCR, Channel);
}

/**
  * @brief  Set IPCC transmit channel status for processor 2.
  * @note   Associated with IPCC_C2TOC1SR.CHxF
  * @rmtoll C2SCR        CH1S           LL_C2_IPCC_SetFlag_CHx\n
  *         C2SCR        CH2S           LL_C2_IPCC_SetFlag_CHx\n
  *         C2SCR        CH3S           LL_C2_IPCC_SetFlag_CHx\n
  *         C2SCR        CH4S           LL_C2_IPCC_SetFlag_CHx\n
  *         C2SCR        CH5S           LL_C2_IPCC_SetFlag_CHx\n
  *         C2SCR        CH6S           LL_C2_IPCC_SetFlag_CHx
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_C2_IPCC_SetFlag_CHx(IPCC_TypeDef *IPCCx, uint32_t Channel)
{
  WRITE_REG(IPCCx->C2SCR, Channel << IPCC_C2SCR_CH1S_Pos);
}

/**
  * @brief  Get channel status for processor 2.
  * @rmtoll C2TOC1SR        CH1F           LL_C2_IPCC_IsActiveFlag_CHx\n
  *         C2TOC1SR        CH2F           LL_C2_IPCC_IsActiveFlag_CHx\n
  *         C2TOC1SR        CH3F           LL_C2_IPCC_IsActiveFlag_CHx\n
  *         C2TOC1SR        CH4F           LL_C2_IPCC_IsActiveFlag_CHx\n
  *         C2TOC1SR        CH5F           LL_C2_IPCC_IsActiveFlag_CHx\n
  *         C2TOC1SR        CH6F           LL_C2_IPCC_IsActiveFlag_CHx
  * @param  IPCCx IPCC Instance.
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_IPCC_CHANNEL_1
  *         @arg @ref LL_IPCC_CHANNEL_2
  *         @arg @ref LL_IPCC_CHANNEL_3
  *         @arg @ref LL_IPCC_CHANNEL_4
  *         @arg @ref LL_IPCC_CHANNEL_5
  *         @arg @ref LL_IPCC_CHANNEL_6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_C2_IPCC_IsActiveFlag_CHx(IPCC_TypeDef  const *const IPCCx, uint32_t Channel)
{
  return ((READ_BIT(IPCCx->C2TOC1SR, Channel) == (Channel)) ? 1UL : 0UL);
}

/**
  * @}
  */


/**
  * @}
  */

/**
  * @}
  */

#endif /* defined(IPCC) */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32WBxx_LL_IPCC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
