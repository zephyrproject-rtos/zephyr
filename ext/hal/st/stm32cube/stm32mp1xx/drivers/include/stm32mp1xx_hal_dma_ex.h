/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_dma_ex.h
  * @author  MCD Application Team
  * @brief   Header file of DMA HAL extension module.
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
#ifndef STM32MP1xx_HAL_DMA_EX_H
#define STM32MP1xx_HAL_DMA_EX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @addtogroup DMAEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup DMAEx_Exported_Types DMAEx Exported Types
  * @brief DMAEx Exported types
  * @{
  */

/**
  * @brief  HAL DMA Memory definition
  */
typedef enum
{
  MEMORY0      = 0x00U,    /*!< Memory 0     */
  MEMORY1      = 0x01U,    /*!< Memory 1     */

} HAL_DMA_MemoryTypeDef;

/**
  * @brief  HAL DMAMUX Synchronization configuration structure definition
  */
typedef struct
{
  uint32_t SyncSignalID;  /*!< Specifies the synchronization signal gating the DMA request in periodic mode.
                              This parameter can be a value of @ref DMAEx_MUX_SyncSignalID_selection */

  uint32_t SyncPolarity;  /*!< Specifies the polarity of the signal on which the DMA request is synchronized.
                              This parameter can be a value of @ref DMAEx_MUX_SyncPolarity_selection */

  FunctionalState SyncEnable;  /*!< Specifies if the synchronization shall be enabled or disabled
                                    This parameter can take the value ENABLE or DISABLE*/


  FunctionalState EventEnable;    /*!< Specifies if an event shall be generated once the RequestNumber is reached.
                                       This parameter can take the value ENABLE or DISABLE */

  uint32_t RequestNumber; /*!< Specifies the number of DMA request that will be authorized after a sync event.
                               This parameters can be in the range 1 to 32 */

} HAL_DMA_MuxSyncConfigTypeDef;


/**
  * @brief  HAL DMAMUX request generator parameters structure definition
  */
typedef struct
{
  uint32_t SignalID;      /*!< Specifies the ID of the signal used for DMAMUX request generator
                              This parameter can be a value of @ref DMAEx_MUX_SignalGeneratorID_selection */

  uint32_t Polarity;       /*!< Specifies the polarity of the signal on which the request is generated.
                             This parameter can be a value of @ref DMAEx_MUX_RequestGeneneratorPolarity_selection */

  uint32_t RequestNumber;  /*!< Specifies the number of DMA request that will be generated after a signal event.
                                This parameters can be in the range 1 to 32 */

} HAL_DMA_MuxRequestGeneratorConfigTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup DMAEx_Exported_Constants DMA Exported Constants
  * @brief    DMAEx Exported constants
  * @{
  */

/** @defgroup DMAEx_MUX_SyncSignalID_selection DMAEx MUX SyncSignalID selection
  * @brief    DMAEx MUX SyncSignalID selection
  * @{
  */
#define HAL_DMAMUX1_SYNC_DMAMUX1_CH0_EVT   0U   /*!< Domain synchronization Signal is DMAMUX1 Channel0 Event */
#define HAL_DMAMUX1_SYNC_DMAMUX1_CH1_EVT   1U   /*!< Domain synchronization Signal is DMAMUX1 Channel1 Event */
#define HAL_DMAMUX1_SYNC_DMAMUX1_CH2_EVT   2U   /*!< Domain synchronization Signal is DMAMUX1 Channel2 Event */
#define HAL_DMAMUX1_SYNC_LPTIM1_OUT        3U   /*!< Domain synchronization Signal is LPTIM1 OUT             */
#define HAL_DMAMUX1_SYNC_LPTIM2_OUT        4U   /*!< Domain synchronization Signal is LPTIM2 OUT             */
#define HAL_DMAMUX1_SYNC_LPTIM3_OUT        5U   /*!< Domain synchronization Signal is LPTIM3 OUT             */
#define HAL_DMAMUX1_SYNC_EXTI0             6U   /*!< Domain synchronization Signal is EXTI0 IT               */
#define HAL_DMAMUX1_SYNC_TIM12_TRGO        7U   /*!< Domain synchronization Signal is TIM12 TRGO             */

/**
  * @}
  */

/** @defgroup DMAEx_MUX_SyncPolarity_selection DMAEx MUX SyncPolarity selection
  * @brief    DMAEx MUX SyncPolarity selection
  * @{
  */
#define HAL_DMAMUX_SYNC_NO_EVENT        0x00000000U             /*!< block synchronization events                    */
#define HAL_DMAMUX_SYNC_RISING          DMAMUX_CxCR_SPOL_0      /*!< synchronize with rising edge events             */
#define HAL_DMAMUX_SYNC_FALLING         DMAMUX_CxCR_SPOL_1      /*!< synchronize with falling edge events            */
#define HAL_DMAMUX_SYNC_RISING_FALLING  DMAMUX_CxCR_SPOL        /*!< synchronize with rising and falling edge events */

/**
  * @}
  */


/** @defgroup DMAEx_MUX_SignalGeneratorID_selection DMAEx MUX SignalGeneratorID selection
  * @brief    DMAEx MUX SignalGeneratorID selection
  * @{
  */
#define HAL_DMAMUX1_REQ_GEN_DMAMUX1_CH0_EVT   0U   /*!< Domain Request generator Signal is DMAMUX1 Channel0 Event */
#define HAL_DMAMUX1_REQ_GEN_DMAMUX1_CH1_EVT   1U   /*!< Domain Request generator Signal is DMAMUX1 Channel1 Event */
#define HAL_DMAMUX1_REQ_GEN_DMAMUX1_CH2_EVT   2U   /*!< Domain Request generator Signal is DMAMUX1 Channel2 Event */
#define HAL_DMAMUX1_REQ_GEN_LPTIM1_OUT        3U   /*!< Domain Request generator Signal is LPTIM1 OUT             */
#define HAL_DMAMUX1_REQ_GEN_LPTIM2_OUT        4U   /*!< Domain Request generator Signal is LPTIM2 OUT             */
#define HAL_DMAMUX1_REQ_GEN_LPTIM3_OUT        5U   /*!< Domain Request generator Signal is LPTIM3 OUT             */
#define HAL_DMAMUX1_REQ_GEN_EXTI0             6U   /*!< Domain Request generator Signal is EXTI0 IT               */
#define HAL_DMAMUX1_REQ_GEN_TIM12_TRGO        7U   /*!< Domain Request generator Signal is TIM12 TRGO             */


/**
  * @}
  */

/** @defgroup DMAEx_MUX_RequestGeneneratorPolarity_selection DMAEx MUX RequestGeneneratorPolarity selection
  * @brief    DMAEx MUX RequestGeneneratorPolarity selection
  * @{
  */
#define HAL_DMAMUX_REQ_GEN_NO_EVENT        0x00000000U           /*!< block request generator events                     */
#define HAL_DMAMUX_REQ_GEN_RISING          DMAMUX_RGxCR_GPOL_0  /*!< generate request on rising edge events             */
#define HAL_DMAMUX_REQ_GEN_FALLING         DMAMUX_RGxCR_GPOL_1  /*!< generate request on falling edge events            */
#define HAL_DMAMUX_REQ_GEN_RISING_FALLING  DMAMUX_RGxCR_GPOL    /*!< generate request on rising and falling edge events */

/**
  * @}
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup DMAEx_Exported_Functions DMAEx Exported Functions
  * @brief   DMAEx Exported functions
  * @{
  */

/** @defgroup DMAEx_Exported_Functions_Group1 Extended features functions
  * @brief   Extended features functions
  * @{
  */

/* IO operation functions *******************************************************/
HAL_StatusTypeDef HAL_DMAEx_MultiBufferStart(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t SecondMemAddress, uint32_t DataLength);
HAL_StatusTypeDef HAL_DMAEx_MultiBufferStart_IT(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t SecondMemAddress, uint32_t DataLength);
HAL_StatusTypeDef HAL_DMAEx_ChangeMemory(DMA_HandleTypeDef *hdma, uint32_t Address, HAL_DMA_MemoryTypeDef memory);
HAL_StatusTypeDef HAL_DMAEx_ConfigMuxSync(DMA_HandleTypeDef *hdma, HAL_DMA_MuxSyncConfigTypeDef *pSyncConfig);
HAL_StatusTypeDef HAL_DMAEx_ConfigMuxRequestGenerator(DMA_HandleTypeDef *hdma, HAL_DMA_MuxRequestGeneratorConfigTypeDef *pRequestGeneratorConfig);
HAL_StatusTypeDef HAL_DMAEx_EnableMuxRequestGenerator(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_DMAEx_DisableMuxRequestGenerator(DMA_HandleTypeDef *hdma);

void HAL_DMAEx_MUX_IRQHandler(DMA_HandleTypeDef *hdma);
/**
  * @}
  */
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup DMAEx_Private_Macros DMA Private Macros
  * @brief    DMAEx private macros
  * @{
  */

#define IS_DMAMUX_SYNC_SIGNAL_ID(SIGNAL_ID) ((SIGNAL_ID) <= HAL_DMAMUX1_SYNC_TIM12_TRGO)

#define IS_DMAMUX_SYNC_REQUEST_NUMBER(REQUEST_NUMBER) (((REQUEST_NUMBER) > 0) && ((REQUEST_NUMBER) <= 32))

#define IS_DMAMUX_SYNC_POLARITY(POLARITY) (((POLARITY) == HAL_DMAMUX_SYNC_NO_EVENT)    || \
                                           ((POLARITY) == HAL_DMAMUX_SYNC_RISING)   || \
                                           ((POLARITY) == HAL_DMAMUX_SYNC_FALLING)  || \
                                           ((POLARITY) == HAL_DMAMUX_SYNC_RISING_FALLING))

#define IS_DMAMUX_SYNC_STATE(SYNC) (((SYNC) == DISABLE)   || ((SYNC) == ENABLE))

#define IS_DMAMUX_SYNC_EVENT(EVENT) (((EVENT) == DISABLE)   || \
                                     ((EVENT) == ENABLE))

#define IS_DMAMUX_REQUEST_GEN_SIGNAL_ID(SIGNAL_ID) ((SIGNAL_ID) <= HAL_DMAMUX1_REQ_GEN_TIM12_TRGO)

#define IS_DMAMUX_REQUEST_GEN_REQUEST_NUMBER(REQUEST_NUMBER) (((REQUEST_NUMBER) > 0) && ((REQUEST_NUMBER) <= 32))

#define IS_DMAMUX_REQUEST_GEN_POLARITY(POLARITY) (((POLARITY) == HAL_DMAMUX_REQ_GEN_NO_EVENT) || \
                                                  ((POLARITY) == HAL_DMAMUX_REQ_GEN_RISING)   || \
                                                  ((POLARITY) == HAL_DMAMUX_REQ_GEN_FALLING)   || \
                                                  ((POLARITY) == HAL_DMAMUX_REQ_GEN_RISING_FALLING))

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/
/** @defgroup DMAEx_Private_Functions DMAEx Private Functions
  * @brief DMAEx Private functions
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

#endif /* STM32H7xx_HAL_DMA_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
