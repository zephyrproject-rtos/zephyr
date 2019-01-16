/**
  ******************************************************************************
  * @file    stm32g0xx_hal_dma.h
  * @author  MCD Application Team
  * @brief   Header file of DMA HAL module.
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
#ifndef STM32G0xx_HAL_DMA_H
#define STM32G0xx_HAL_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal_def.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @addtogroup DMA
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup DMA_Exported_Types DMA Exported Types
  * @{
  */

/**
  * @brief  DMA Configuration Structure definition
  */
typedef struct
{
  uint32_t Request;               /*!< Specifies the request selected for the specified channel.
                                       This parameter can be a value of @ref DMA_request */

  uint32_t Direction;             /*!< Specifies if the data will be transferred from memory to peripheral,
                                       from memory to memory or from peripheral to memory.
                                       This parameter can be a value of @ref DMA_Data_transfer_direction */

  uint32_t PeriphInc;             /*!< Specifies whether the Peripheral address register should be incremented or not.
                                       This parameter can be a value of @ref DMA_Peripheral_incremented_mode */

  uint32_t MemInc;                /*!< Specifies whether the memory address register should be incremented or not.
                                       This parameter can be a value of @ref DMA_Memory_incremented_mode */

  uint32_t PeriphDataAlignment;   /*!< Specifies the Peripheral data width.
                                       This parameter can be a value of @ref DMA_Peripheral_data_size */

  uint32_t MemDataAlignment;      /*!< Specifies the Memory data width.
                                       This parameter can be a value of @ref DMA_Memory_data_size */

  uint32_t Mode;                  /*!< Specifies the operation mode of the DMAy Channelx.
                                       This parameter can be a value of @ref DMA_mode
                                       @note The circular buffer mode cannot be used if the memory-to-memory
                                             data transfer is configured on the selected Channel */

  uint32_t Priority;              /*!< Specifies the software priority for the DMAy Channelx.
                                       This parameter can be a value of @ref DMA_Priority_level */
} DMA_InitTypeDef;

/**
  * @brief  HAL DMA State structures definition
  */
typedef enum
{
  HAL_DMA_STATE_RESET             = 0x00U,  /*!< DMA not yet initialized or disabled    */
  HAL_DMA_STATE_READY             = 0x01U,  /*!< DMA initialized and ready for use      */
  HAL_DMA_STATE_BUSY              = 0x02U,  /*!< DMA process is ongoing                 */
  HAL_DMA_STATE_TIMEOUT           = 0x03U,  /*!< DMA timeout state                      */
} HAL_DMA_StateTypeDef;

/**
  * @brief  HAL DMA Error Code structure definition
  */
typedef enum
{
  HAL_DMA_FULL_TRANSFER           = 0x00U,  /*!< Full transfer     */
  HAL_DMA_HALF_TRANSFER           = 0x01U   /*!< Half Transfer     */
} HAL_DMA_LevelCompleteTypeDef;

/**
  * @brief  HAL DMA Callback ID structure definition
  */
typedef enum
{
  HAL_DMA_XFER_CPLT_CB_ID          = 0x00U,  /*!< Full transfer    */
  HAL_DMA_XFER_HALFCPLT_CB_ID      = 0x01U,  /*!< Half transfer    */
  HAL_DMA_XFER_ERROR_CB_ID         = 0x02U,  /*!< Error            */
  HAL_DMA_XFER_ABORT_CB_ID         = 0x03U,  /*!< Abort            */
  HAL_DMA_XFER_ALL_CB_ID           = 0x04U   /*!< All              */

} HAL_DMA_CallbackIDTypeDef;

/**
  * @brief  DMA handle Structure definition
  */
typedef struct __DMA_HandleTypeDef
{
  DMA_Channel_TypeDef             *Instance;                          /*!< Register base address                 */

  DMA_InitTypeDef                 Init;                               /*!< DMA communication parameters          */

  HAL_LockTypeDef                 Lock;                               /*!< DMA locking object                    */

  __IO HAL_DMA_StateTypeDef       State;                              /*!< DMA transfer state                    */

  void   *Parent;                                                     /*!< Parent object state                   */

  void (* XferCpltCallback)(struct __DMA_HandleTypeDef *hdma);        /*!< DMA transfer complete callback        */

  void (* XferHalfCpltCallback)(struct __DMA_HandleTypeDef *hdma);    /*!< DMA Half transfer complete callback   */

  void (* XferErrorCallback)(struct __DMA_HandleTypeDef *hdma);       /*!< DMA transfer error callback           */

  void (* XferAbortCallback)(struct __DMA_HandleTypeDef *hdma);       /*!< DMA transfer abort callback           */

  __IO uint32_t                   ErrorCode;                          /*!< DMA Error code                        */

  uint32_t                        ChannelIndex;                       /*!< DMA Channel Index                     */

  DMAMUX_Channel_TypeDef           *DMAmuxChannel;                    /*!< Register base address                 */

  DMAMUX_ChannelStatus_TypeDef     *DMAmuxChannelStatus;              /*!< DMAMUX Channels Status Base Address   */

  uint32_t                         DMAmuxChannelStatusMask;           /*!< DMAMUX Channel Status Mask            */

  DMAMUX_RequestGen_TypeDef        *DMAmuxRequestGen;                 /*!< DMAMUX request generator Base Address */

  DMAMUX_RequestGenStatus_TypeDef  *DMAmuxRequestGenStatus;           /*!< DMAMUX request generator Address      */

  uint32_t                         DMAmuxRequestGenStatusMask;        /*!< DMAMUX request generator Status mask  */
} DMA_HandleTypeDef;
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup DMA_Exported_Constants DMA Exported Constants
  * @{
  */

/** @defgroup DMA_Error_Code DMA Error Code
  * @{
  */
#define HAL_DMA_ERROR_NONE           0x00000000U       /*!< No error                                */
#define HAL_DMA_ERROR_TE             0x00000001U       /*!< Transfer error                          */
#define HAL_DMA_ERROR_NO_XFER        0x00000004U       /*!< Abort requested with no Xfer ongoing    */
#define HAL_DMA_ERROR_TIMEOUT        0x00000020U       /*!< Timeout error                           */
#define HAL_DMA_ERROR_NOT_SUPPORTED  0x00000100U       /*!< Not supported mode                      */
#define HAL_DMA_ERROR_SYNC           0x00000200U       /*!< DMAMUX sync overrun  error              */
#define HAL_DMA_ERROR_REQGEN         0x00000400U       /*!< DMAMUX request generator overrun  error */

/**
  * @}
  */

/** @defgroup DMA_request DMA request
  * @{
  */
#define DMA_REQUEST_MEM2MEM           0U               /*!< memory to memory transfer     */


#define DMA_REQUEST_GENERATOR0        1U               /*!< DMAMUX request generator 0    */
#define DMA_REQUEST_GENERATOR1        2U               /*!< DMAMUX request generator 1    */
#define DMA_REQUEST_GENERATOR2        3U               /*!< DMAMUX request generator 2    */
#define DMA_REQUEST_GENERATOR3        4U               /*!< DMAMUX request generator 3    */


#define DMA_REQUEST_ADC1              5U               /*!< DMAMUX ADC1 request           */

#if defined(AES)
#define DMA_REQUEST_AES_IN            6U               /*!< DMAMUX AES IN request         */
#define DMA_REQUEST_AES_OUT           7U               /*!< DMAMUX AES OUT request        */
#endif

#if defined(DAC1)
#define DMA_REQUEST_DAC1_CHANNEL1     8U               /*!< DMAMUX SPI2 RX request        */
#define DMA_REQUEST_DAC1_CHANNEL2     9U               /*!< DMAMUX SPI2 TX request        */
#endif

#define DMA_REQUEST_I2C1_RX          10U               /*!< DMAMUX I2C1 RX request        */
#define DMA_REQUEST_I2C1_TX          11U               /*!< DMAMUX I2C1 TX request        */
#define DMA_REQUEST_I2C2_RX          12U               /*!< DMAMUX I2C2 RX request        */
#define DMA_REQUEST_I2C2_TX          13U               /*!< DMAMUX I2C2 TX request        */

#if defined(LPUART1)
#define DMA_REQUEST_LPUART1_RX       14U               /*!< DMAMUX LPUART1 RX request     */
#define DMA_REQUEST_LPUART1_TX       15U               /*!< DMAMUX LPUART1 TX request     */
#endif

#define DMA_REQUEST_SPI1_RX          16U               /*!< DMAMUX SPI1 RX request        */
#define DMA_REQUEST_SPI1_TX          17U               /*!< DMAMUX SPI1 TX request        */
#define DMA_REQUEST_SPI2_RX          18U               /*!< DMAMUX SPI2 RX request        */
#define DMA_REQUEST_SPI2_TX          19U               /*!< DMAMUX SPI2 TX request        */


#define DMA_REQUEST_TIM1_CH1         20U               /*!< DMAMUX TIM1 CH1 request       */
#define DMA_REQUEST_TIM1_CH2         21U               /*!< DMAMUX TIM1 CH2 request       */
#define DMA_REQUEST_TIM1_CH3         22U               /*!< DMAMUX TIM1 CH3 request       */
#define DMA_REQUEST_TIM1_CH4         23U               /*!< DMAMUX TIM1 CH4 request       */
#define DMA_REQUEST_TIM1_TRIG_COM    24U               /*!< DMAMUX TIM1 TRIG COM request  */
#define DMA_REQUEST_TIM1_UP          25U               /*!< DMAMUX TIM1 UP request        */

#if defined(TIM2)
#define DMA_REQUEST_TIM2_CH1         26U               /*!< DMAMUX TIM2 CH1 request       */
#define DMA_REQUEST_TIM2_CH2         27U               /*!< DMAMUX TIM2 CH2 request       */
#define DMA_REQUEST_TIM2_CH3         28U               /*!< DMAMUX TIM2 CH3 request       */
#define DMA_REQUEST_TIM2_CH4         29U               /*!< DMAMUX TIM2 CH4 request       */
#define DMA_REQUEST_TIM2_TRIG        30U               /*!< DMAMUX TIM2 TRIG request      */
#define DMA_REQUEST_TIM2_UP          31U               /*!< DMAMUX TIM2 UP request        */
#endif

#define DMA_REQUEST_TIM3_CH1         32U               /*!< DMAMUX TIM3 CH1 request       */
#define DMA_REQUEST_TIM3_CH2         33U               /*!< DMAMUX TIM3 CH2 request       */
#define DMA_REQUEST_TIM3_CH3         34U               /*!< DMAMUX TIM3 CH3 request       */
#define DMA_REQUEST_TIM3_CH4         35U               /*!< DMAMUX TIM3 CH4 request       */
#define DMA_REQUEST_TIM3_TRIG        36U               /*!< DMAMUX TIM3 TRIG request      */
#define DMA_REQUEST_TIM3_UP          37U               /*!< DMAMUX TIM3 UP request        */

#if defined(TIM6)
#define DMA_REQUEST_TIM6_UP          38U               /*!< DMAMUX TIM6 UP request        */
#endif
#if defined(TIM7)
#define DMA_REQUEST_TIM7_UP          39U               /*!< DMAMUX TIM7 UP request        */
#endif

#if defined(TIM15)
#define DMA_REQUEST_TIM15_CH1        40U               /*!< DMAMUX TIM15 CH1 request      */
#define DMA_REQUEST_TIM15_CH2        41U               /*!< DMAMUX TIM15 CH2 request      */
#define DMA_REQUEST_TIM15_TRIG_COM   42U               /*!< DMAMUX TIM15 TRIG COM request */
#define DMA_REQUEST_TIM15_UP         43U               /*!< DMAMUX TIM15 UP request       */
#endif

#define DMA_REQUEST_TIM16_CH1        44U               /*!< DMAMUX TIM16 CH1 request      */
#define DMA_REQUEST_TIM16_TRIG_COM   45U               /*!< DMAMUX TIM16 TRIG COM request */
#define DMA_REQUEST_TIM16_UP         46U               /*!< DMAMUX TIM16 UP request       */

#define DMA_REQUEST_TIM17_CH1        47U               /*!< DMAMUX TIM17 CH2 request      */
#define DMA_REQUEST_TIM17_TRIG_COM   48U               /*!< DMAMUX TIM17 TRIG COM request */
#define DMA_REQUEST_TIM17_UP         49U               /*!< DMAMUX TIM17 UP request       */

#define DMA_REQUEST_USART1_RX        50U               /*!< DMAMUX USART1 RX request      */
#define DMA_REQUEST_USART1_TX        51U               /*!< DMAMUX USART1 TX request      */
#define DMA_REQUEST_USART2_RX        52U               /*!< DMAMUX USART2 RX request      */
#define DMA_REQUEST_USART2_TX        53U               /*!< DMAMUX USART2 TX request      */
#if defined(USART3)
#define DMA_REQUEST_USART3_RX        54U               /*!< DMAMUX USART3 RX request      */
#define DMA_REQUEST_USART3_TX        55U               /*!< DMAMUX USART3 TX request      */
#endif
#if defined(USART4)
#define DMA_REQUEST_USART4_RX        56U               /*!< DMAMUX USART4 RX request      */
#define DMA_REQUEST_USART4_TX        57U               /*!< DMAMUX USART4 TX request      */
#endif

#if defined(UCPD1)
#define DMA_REQUEST_UCPD1_RX         58U               /*!< DMAMUX UCPD1 RX request       */
#define DMA_REQUEST_UCPD1_TX         59U               /*!< DMAMUX UCPD1 TX request       */
#endif
#if defined(UCPD2)
#define DMA_REQUEST_UCPD2_RX         60U               /*!< DMAMUX UCPD2 RX request       */
#define DMA_REQUEST_UCPD2_TX         61U               /*!< DMAMUX UCPD2 TX request       */
#endif
/**
  * @}
  */

/** @defgroup DMA_Data_transfer_direction DMA Data transfer direction
  * @{
  */
#define DMA_PERIPH_TO_MEMORY         0x00000000U       /*!< Peripheral to memory direction */
#define DMA_MEMORY_TO_PERIPH         DMA_CCR_DIR       /*!< Memory to peripheral direction */
#define DMA_MEMORY_TO_MEMORY         DMA_CCR_MEM2MEM   /*!< Memory to memory direction     */

/**
  * @}
  */

/** @defgroup DMA_Peripheral_incremented_mode DMA Peripheral incremented mode
  * @{
  */
#define DMA_PINC_ENABLE              DMA_CCR_PINC      /*!< Peripheral increment mode Enable  */
#define DMA_PINC_DISABLE             0x00000000U       /*!< Peripheral increment mode Disable */
/**
  * @}
  */

/** @defgroup DMA_Memory_incremented_mode DMA Memory incremented mode
  * @{
  */
#define DMA_MINC_ENABLE              DMA_CCR_MINC      /*!< Memory increment mode Enable  */
#define DMA_MINC_DISABLE             0x00000000U       /*!< Memory increment mode Disable */
/**
  * @}
  */

/** @defgroup DMA_Peripheral_data_size DMA Peripheral data size
  * @{
  */
#define DMA_PDATAALIGN_BYTE          0x00000000U       /*!< Peripheral data alignment : Byte     */
#define DMA_PDATAALIGN_HALFWORD      DMA_CCR_PSIZE_0   /*!< Peripheral data alignment : HalfWord */
#define DMA_PDATAALIGN_WORD          DMA_CCR_PSIZE_1   /*!< Peripheral data alignment : Word     */
/**
  * @}
  */

/** @defgroup DMA_Memory_data_size DMA Memory data size
  * @{
  */
#define DMA_MDATAALIGN_BYTE          0x00000000U       /*!< Memory data alignment : Byte     */
#define DMA_MDATAALIGN_HALFWORD      DMA_CCR_MSIZE_0   /*!< Memory data alignment : HalfWord */
#define DMA_MDATAALIGN_WORD          DMA_CCR_MSIZE_1   /*!< Memory data alignment : Word     */
/**
  * @}
  */

/** @defgroup DMA_mode DMA mode
  * @{
  */
#define DMA_NORMAL                   0x00000000U       /*!< Normal mode    */
#define DMA_CIRCULAR                 DMA_CCR_CIRC      /*!< Circular mode  */
/**
  * @}
  */

/** @defgroup DMA_Priority_level DMA Priority level
  * @{
  */
#define DMA_PRIORITY_LOW             0x00000000U       /*!< Priority level : Low       */
#define DMA_PRIORITY_MEDIUM          DMA_CCR_PL_0      /*!< Priority level : Medium    */
#define DMA_PRIORITY_HIGH            DMA_CCR_PL_1      /*!< Priority level : High      */
#define DMA_PRIORITY_VERY_HIGH       DMA_CCR_PL        /*!< Priority level : Very_High */
/**
  * @}
  */

/** @defgroup DMA_interrupt_enable_definitions DMA interrupt enable definitions
  * @{
  */
#define DMA_IT_TC                    DMA_CCR_TCIE
#define DMA_IT_HT                    DMA_CCR_HTIE
#define DMA_IT_TE                    DMA_CCR_TEIE
/**
  * @}
  */

/** @defgroup DMA_flag_definitions DMA flag definitions
  * @{
  */

#define DMA_FLAG_GI1                 DMA_ISR_GIF1
#define DMA_FLAG_TC1                 DMA_ISR_TCIF1
#define DMA_FLAG_HT1                 DMA_ISR_HTIF1
#define DMA_FLAG_TE1                 DMA_ISR_TEIF1
#define DMA_FLAG_GI2                 DMA_ISR_GIF2
#define DMA_FLAG_TC2                 DMA_ISR_TCIF2
#define DMA_FLAG_HT2                 DMA_ISR_HTIF2
#define DMA_FLAG_TE2                 DMA_ISR_TEIF2
#define DMA_FLAG_GI3                 DMA_ISR_GIF3
#define DMA_FLAG_TC3                 DMA_ISR_TCIF3
#define DMA_FLAG_HT3                 DMA_ISR_HTIF3
#define DMA_FLAG_TE3                 DMA_ISR_TEIF3
#define DMA_FLAG_GI4                 DMA_ISR_GIF4
#define DMA_FLAG_TC4                 DMA_ISR_TCIF4
#define DMA_FLAG_HT4                 DMA_ISR_HTIF4
#define DMA_FLAG_TE4                 DMA_ISR_TEIF4
#define DMA_FLAG_GI5                 DMA_ISR_GIF5
#define DMA_FLAG_TC5                 DMA_ISR_TCIF5
#define DMA_FLAG_HT5                 DMA_ISR_HTIF5
#define DMA_FLAG_TE5                 DMA_ISR_TEIF5
#if defined (DMA1_Channel6)
#define DMA_FLAG_GI6                 DMA_ISR_GIF6
#define DMA_FLAG_TC6                 DMA_ISR_TCIF6
#define DMA_FLAG_HT6                 DMA_ISR_HTIF6
#define DMA_FLAG_TE6                 DMA_ISR_TEIF6
#endif /* DMA1_Channel6 */
#if defined (DMA1_Channel7)
#define DMA_FLAG_GI7                 DMA_ISR_GIF7
#define DMA_FLAG_TC7                 DMA_ISR_TCIF7
#define DMA_FLAG_HT7                 DMA_ISR_HTIF7
#define DMA_FLAG_TE7                 DMA_ISR_TEIF7
#endif /* DMA1_Channel7 */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup DMA_Exported_Macros DMA Exported Macros
  * @{
  */

/** @brief  Reset DMA handle state
  * @param __HANDLE__ DMA handle
  * @retval None
  */
#define __HAL_DMA_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = HAL_DMA_STATE_RESET)

/**
  * @brief  Enable the specified DMA Channel.
  * @param __HANDLE__ DMA handle
  * @retval None
  */
#define __HAL_DMA_ENABLE(__HANDLE__)        ((__HANDLE__)->Instance->CCR |=  DMA_CCR_EN)

/**
  * @brief  Disable the specified DMA Channel.
  * @param __HANDLE__ DMA handle
  * @retval None
  */
#define __HAL_DMA_DISABLE(__HANDLE__)       ((__HANDLE__)->Instance->CCR &=  ~DMA_CCR_EN)

/* Interrupt & Flag management */

/**
  * @brief  Return the current DMA Channel transfer complete flag.
  * @param __HANDLE__ DMA handle
  * @retval The specified transfer complete flag index.
  */

#if defined (DMA1_Channel7)
#define __HAL_DMA_GET_TC_FLAG_INDEX(__HANDLE__) \
(((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel1))? DMA_FLAG_TC1 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel2))? DMA_FLAG_TC2 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel3))? DMA_FLAG_TC3 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel4))? DMA_FLAG_TC4 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel5))? DMA_FLAG_TC5 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel6))? DMA_FLAG_TC6 :\
   DMA_FLAG_TC7)
#elif defined (DMA1_Channel5)
#define __HAL_DMA_GET_TC_FLAG_INDEX(__HANDLE__) \
(((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel1))? DMA_FLAG_TC1 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel2))? DMA_FLAG_TC2 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel3))? DMA_FLAG_TC3 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel4))? DMA_FLAG_TC4 :\
   DMA_FLAG_TC5)
#endif /* DMA1_Channel7 */

/**
  * @brief  Return the current DMA Channel half transfer complete flag.
  * @param __HANDLE__ DMA handle
  * @retval The specified half transfer complete flag index.
  */
#if defined (DMA1_Channel7)
#define __HAL_DMA_GET_HT_FLAG_INDEX(__HANDLE__)\
(((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel1))? DMA_FLAG_HT1 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel2))? DMA_FLAG_HT2 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel3))? DMA_FLAG_HT3 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel4))? DMA_FLAG_HT4 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel5))? DMA_FLAG_HT5 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel6))? DMA_FLAG_HT6 :\
   DMA_FLAG_HT7)
#elif defined(DMA1_Channel5)
#define __HAL_DMA_GET_HT_FLAG_INDEX(__HANDLE__)\
(((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel1))? DMA_FLAG_HT1 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel2))? DMA_FLAG_HT2 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel3))? DMA_FLAG_HT3 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel4))? DMA_FLAG_HT4 :\
   DMA_FLAG_HT5)
#endif /* DMA1_Channel7 */

/**
  * @brief  Return the current DMA Channel transfer error flag.
  * @param  __HANDLE__ DMA handle
  * @retval The specified transfer error flag index.
  */
#if defined (DMA1_Channel7)
#define __HAL_DMA_GET_TE_FLAG_INDEX(__HANDLE__)\
(((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel1))? DMA_FLAG_TE1 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel2))? DMA_FLAG_TE2 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel3))? DMA_FLAG_TE3 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel4))? DMA_FLAG_TE4 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel5))? DMA_FLAG_TE5 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel6))? DMA_FLAG_TE6 :\
   DMA_FLAG_TE7)
#elif defined(DMA1_Channel5)
#define __HAL_DMA_GET_TE_FLAG_INDEX(__HANDLE__)\
(((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel1))? DMA_FLAG_TE1 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel2))? DMA_FLAG_TE2 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel3))? DMA_FLAG_TE3 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel4))? DMA_FLAG_TE4 :\
   DMA_FLAG_TE5)
#endif /* DMA1_Channel7 */

/**
  * @brief  Return the current DMA Channel Global interrupt flag.
  * @param  __HANDLE__ DMA handle
  * @retval The specified transfer error flag index.
  */
#if defined (DMA1_Channel7)
#define __HAL_DMA_GET_GI_FLAG_INDEX(__HANDLE__)\
(((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel1))? DMA_FLAG_GI1 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel2))? DMA_FLAG_GI2 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel3))? DMA_FLAG_GI3 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel4))? DMA_FLAG_GI4 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel5))? DMA_FLAG_GI5 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel6))? DMA_FLAG_GI6 :\
   DMA_ISR_GIF7)
#elif defined(DMA1_Channel5)
#define __HAL_DMA_GET_GI_FLAG_INDEX(__HANDLE__)\
(((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel1))? DMA_FLAG_GI1 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel2))? DMA_FLAG_GI2 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel3))? DMA_FLAG_GI3 :\
 ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA1_Channel4))? DMA_FLAG_GI4 :\
   DMA_ISR_GIF5)
#endif /* DMA1_Channel7 */

/**
  * @brief  Get the DMA Channel pending flags.
  * @param  __HANDLE__ DMA handle
  * @param  __FLAG__ Get the specified flag.
  *          This parameter can be any combination of the following values:
  *            @arg DMA_FLAG_TCIFx:  Transfer complete flag
  *            @arg DMA_FLAG_HTIFx:  Half transfer complete flag
  *            @arg DMA_FLAG_TEIFx:  Transfer error flag
  *            @arg DMA_FLAG_GIFx: Global interrupt flag
  *         Where x can be 1_7 to select the DMA Channel flag.
  * @retval The state of FLAG (SET or RESET).
  */
#define __HAL_DMA_GET_FLAG(__HANDLE__, __FLAG__)  (DMA1->ISR & (__FLAG__))

/**
  * @brief  Clear the DMA Channel pending flags.
  * @param  __HANDLE__ DMA handle
  * @param  __FLAG__ specifies the flag to clear.
  *          This parameter can be any combination of the following values:
  *            @arg DMA_FLAG_TCIFx:  Transfer complete flag
  *            @arg DMA_FLAG_HTIFx:  Half transfer complete flag
  *            @arg DMA_FLAG_TEIFx:  Transfer error flag
  *            @arg DMA_FLAG_GIFx: Global interrupt flag
  *         Where x can be 1_7 to select the DMA Channel flag.
  * @retval None
  */
#define __HAL_DMA_CLEAR_FLAG(__HANDLE__, __FLAG__) (DMA1->IFCR |= (__FLAG__))

/**
  * @brief  Enable the specified DMA Channel interrupts.
  * @param  __HANDLE__ DMA handle
  * @param __INTERRUPT__ specifies the DMA interrupt sources to be enabled or disabled.
  *          This parameter can be any combination of the following values:
  *            @arg DMA_IT_TC:  Transfer complete interrupt mask
  *            @arg DMA_IT_HT:  Half transfer complete interrupt mask
  *            @arg DMA_IT_TE:  Transfer error interrupt mask
  * @retval None
  */
#define __HAL_DMA_ENABLE_IT(__HANDLE__, __INTERRUPT__)   ((__HANDLE__)->Instance->CCR |= (__INTERRUPT__))

/**
  * @brief  Disable the specified DMA Channel interrupts.
  * @param  __HANDLE__ DMA handle
  * @param __INTERRUPT__ specifies the DMA interrupt sources to be enabled or disabled.
  *          This parameter can be any combination of the following values:
  *            @arg DMA_IT_TC:  Transfer complete interrupt mask
  *            @arg DMA_IT_HT:  Half transfer complete interrupt mask
  *            @arg DMA_IT_TE:  Transfer error interrupt mask
  * @retval None
  */
#define __HAL_DMA_DISABLE_IT(__HANDLE__, __INTERRUPT__)  ((__HANDLE__)->Instance->CCR &= ~(__INTERRUPT__))

/**
  * @brief  Check whether the specified DMA Channel interrupt is enabled or disabled.
  * @param  __HANDLE__ DMA handle
  * @param  __INTERRUPT__ specifies the DMA interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg DMA_IT_TC:  Transfer complete interrupt mask
  *            @arg DMA_IT_HT:  Half transfer complete interrupt mask
  *            @arg DMA_IT_TE:  Transfer error interrupt mask
  * @retval The state of DMA_IT (SET or RESET).
  */
#define __HAL_DMA_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)  (((__HANDLE__)->Instance->CCR & (__INTERRUPT__)))

/**
  * @brief  Returns the number of remaining data units in the current DMA Channel transfer.
  * @param  __HANDLE__ DMA handle
  * @retval The number of remaining data units in the current DMA Channel transfer.
  */
#define __HAL_DMA_GET_COUNTER(__HANDLE__) ((__HANDLE__)->Instance->CNDTR)

/**
  * @}
  */

/* Include DMA HAL Extension module */
#include "stm32g0xx_hal_dma_ex.h"

/* Exported functions --------------------------------------------------------*/

/** @addtogroup DMA_Exported_Functions
  * @{
  */

/** @addtogroup DMA_Exported_Functions_Group1
  * @{
  */
/* Initialization and de-initialization functions *****************************/
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_DMA_DeInit(DMA_HandleTypeDef *hdma);
/**
  * @}
  */

/** @addtogroup DMA_Exported_Functions_Group2
  * @{
  */
/* IO operation functions *****************************************************/
HAL_StatusTypeDef HAL_DMA_Start(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
HAL_StatusTypeDef HAL_DMA_Start_IT(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_DMA_Abort_IT(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_DMA_PollForTransfer(DMA_HandleTypeDef *hdma, HAL_DMA_LevelCompleteTypeDef CompleteLevel, uint32_t Timeout);
void HAL_DMA_IRQHandler(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_DMA_RegisterCallback(DMA_HandleTypeDef *hdma, HAL_DMA_CallbackIDTypeDef CallbackID, void (* pCallback)(DMA_HandleTypeDef *_hdma));
HAL_StatusTypeDef HAL_DMA_UnRegisterCallback(DMA_HandleTypeDef *hdma, HAL_DMA_CallbackIDTypeDef CallbackID);

/**
  * @}
  */

/** @addtogroup DMA_Exported_Functions_Group3
  * @{
  */
/* Peripheral State and Error functions ***************************************/
HAL_DMA_StateTypeDef HAL_DMA_GetState(DMA_HandleTypeDef *hdma);
uint32_t             HAL_DMA_GetError(DMA_HandleTypeDef *hdma);
/**
  * @}
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup DMA_Private_Macros DMA Private Macros
  * @{
  */

#define IS_DMA_DIRECTION(DIRECTION) (((DIRECTION) == DMA_PERIPH_TO_MEMORY ) || \
                                     ((DIRECTION) == DMA_MEMORY_TO_PERIPH)  || \
                                     ((DIRECTION) == DMA_MEMORY_TO_MEMORY))

#define IS_DMA_BUFFER_SIZE(SIZE) (((SIZE) >= 0x1U) && ((SIZE) < 0x10000U))

#define IS_DMA_PERIPHERAL_INC_STATE(STATE) (((STATE) == DMA_PINC_ENABLE) || \
                                            ((STATE) == DMA_PINC_DISABLE))

#define IS_DMA_MEMORY_INC_STATE(STATE) (((STATE) == DMA_MINC_ENABLE)  || \
                                        ((STATE) == DMA_MINC_DISABLE))

#if defined(STM32G081xx)||defined(STM32G071xx)||defined(STM32G070xx)
#define IS_DMA_ALL_REQUEST(REQUEST) ((REQUEST) <= DMA_REQUEST_UCPD2_TX)
#endif

#define IS_DMA_PERIPHERAL_DATA_SIZE(SIZE) (((SIZE) == DMA_PDATAALIGN_BYTE)     || \
                                           ((SIZE) == DMA_PDATAALIGN_HALFWORD) || \
                                           ((SIZE) == DMA_PDATAALIGN_WORD))

#define IS_DMA_MEMORY_DATA_SIZE(SIZE) (((SIZE) == DMA_MDATAALIGN_BYTE)     || \
                                       ((SIZE) == DMA_MDATAALIGN_HALFWORD) || \
                                       ((SIZE) == DMA_MDATAALIGN_WORD ))

#define IS_DMA_MODE(MODE) (((MODE) == DMA_NORMAL )  || \
                           ((MODE) == DMA_CIRCULAR))

#define IS_DMA_PRIORITY(PRIORITY) (((PRIORITY) == DMA_PRIORITY_LOW )   || \
                                   ((PRIORITY) == DMA_PRIORITY_MEDIUM) || \
                                   ((PRIORITY) == DMA_PRIORITY_HIGH)   || \
                                   ((PRIORITY) == DMA_PRIORITY_VERY_HIGH))

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32G0xx_HAL_DMA_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
