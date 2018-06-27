/**
  ******************************************************************************
  * @file    stm32f7xx_hal_usart.h
  * @author  MCD Application Team
  * @brief   Header file of USART HAL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
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
#ifndef __STM32F7xx_HAL_USART_H
#define __STM32F7xx_HAL_USART_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal_def.h"

/** @addtogroup STM32F7xx_HAL_Driver
  * @{
  */

/** @addtogroup USART
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup USART_Exported_Types USART Exported Types
  * @{
  */

/**
  * @brief USART Init Structure definition
  */
typedef struct
{
  uint32_t BaudRate;                  /*!< This member configures the Usart communication baud rate.
                                           The baud rate is computed using the following formula:
                                              Baud Rate Register = ((PCLKx) / ((huart->Init.BaudRate))) */

  uint32_t WordLength;                /*!< Specifies the number of data bits transmitted or received in a frame.
                                           This parameter can be a value of @ref USARTEx_Word_Length */

  uint32_t StopBits;                  /*!< Specifies the number of stop bits transmitted.
                                           This parameter can be a value of @ref USART_Stop_Bits */

  uint32_t Parity;                   /*!< Specifies the parity mode.
                                           This parameter can be a value of @ref USART_Parity
                                           @note When parity is enabled, the computed parity is inserted
                                                 at the MSB position of the transmitted data (9th bit when
                                                 the word length is set to 9 data bits; 8th bit when the
                                                 word length is set to 8 data bits). */

  uint32_t Mode;                      /*!< Specifies whether the Receive or Transmit mode is enabled or disabled.
                                           This parameter can be a value of @ref USART_Mode */

  uint32_t OverSampling;              /*!< Specifies whether the Over sampling 8 is enabled or disabled, to achieve higher speed (up to fPCLK/8).
                                           This parameter can be a value of @ref USART_Over_Sampling */                                                                                        

  uint32_t CLKPolarity;               /*!< Specifies the steady state of the serial clock.
                                           This parameter can be a value of @ref USART_Clock_Polarity */

  uint32_t CLKPhase;                  /*!< Specifies the clock transition on which the bit capture is made.
                                           This parameter can be a value of @ref USART_Clock_Phase */

  uint32_t CLKLastBit;                /*!< Specifies whether the clock pulse corresponding to the last transmitted
                                           data bit (MSB) has to be output on the SCLK pin in synchronous mode.
                                           This parameter can be a value of @ref USART_Last_Bit */
}USART_InitTypeDef;

/**
  * @brief HAL USART State structures definition
  */
typedef enum
{
  HAL_USART_STATE_RESET             = 0x00U,    /*!< Peripheral is not initialized   */
  HAL_USART_STATE_READY             = 0x01U,    /*!< Peripheral Initialized and ready for use */
  HAL_USART_STATE_BUSY              = 0x02U,    /*!< an internal process is ongoing */
  HAL_USART_STATE_BUSY_TX           = 0x12U,    /*!< Data Transmission process is ongoing */
  HAL_USART_STATE_BUSY_RX           = 0x22U,    /*!< Data Reception process is ongoing */
  HAL_USART_STATE_BUSY_TX_RX        = 0x32U,    /*!< Data Transmission Reception process is ongoing */
  HAL_USART_STATE_TIMEOUT           = 0x03U,    /*!< Timeout state */
  HAL_USART_STATE_ERROR             = 0x04U     /*!< Error */
}HAL_USART_StateTypeDef;


/**
  * @brief  USART clock sources definitions
  */
typedef enum
{
  USART_CLOCKSOURCE_PCLK1      = 0x00U,    /*!< PCLK1 clock source  */
  USART_CLOCKSOURCE_PCLK2      = 0x01U,    /*!< PCLK2 clock source  */
  USART_CLOCKSOURCE_HSI        = 0x02U,    /*!< HSI clock source    */
  USART_CLOCKSOURCE_SYSCLK     = 0x04U,    /*!< SYSCLK clock source */
  USART_CLOCKSOURCE_LSE        = 0x08U,    /*!< LSE clock source       */
  USART_CLOCKSOURCE_UNDEFINED  = 0x10U     /*!< Undefined clock source */
}USART_ClockSourceTypeDef;


/**
  * @brief  USART handle Structure definition
  */
typedef struct
{
  USART_TypeDef                 *Instance;        /*!<  USART registers base address        */

  USART_InitTypeDef             Init;             /*!< USART communication parameters      */

  uint8_t                       *pTxBuffPtr;      /*!< Pointer to USART Tx transfer Buffer */

  uint16_t                      TxXferSize;       /*!< USART Tx Transfer size              */

  __IO uint16_t                 TxXferCount;      /*!< USART Tx Transfer Counter           */

  uint8_t                       *pRxBuffPtr;      /*!< Pointer to USART Rx transfer Buffer */

  uint16_t                      RxXferSize;       /*!< USART Rx Transfer size              */

  __IO uint16_t                 RxXferCount;      /*!< USART Rx Transfer Counter           */

  uint16_t                      Mask;             /*!< USART Rx RDR register mask          */

  DMA_HandleTypeDef             *hdmatx;          /*!< USART Tx DMA Handle parameters      */

  DMA_HandleTypeDef             *hdmarx;          /*!< USART Rx DMA Handle parameters      */

  HAL_LockTypeDef               Lock;            /*!<  Locking object                      */

  HAL_USART_StateTypeDef        State;           /*!< USART communication state           */

  __IO uint32_t                 ErrorCode;       /*!< USART Error code                    */

}USART_HandleTypeDef;
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup USART_Exported_Constants USART Exported Constants
  * @{
  */

/** @defgroup USART_Error_Code USART Error Code
  * @brief    USART Error Code 
  * @{
  */ 
#define HAL_USART_ERROR_NONE         ((uint32_t)0x00000000U)   /*!< No error            */
#define HAL_USART_ERROR_PE           ((uint32_t)0x00000001U)   /*!< Parity error        */
#define HAL_USART_ERROR_NE           ((uint32_t)0x00000002U)   /*!< Noise error         */
#define HAL_USART_ERROR_FE           ((uint32_t)0x00000004U)   /*!< Frame error         */
#define HAL_USART_ERROR_ORE          ((uint32_t)0x00000008U)   /*!< Overrun error       */
#define HAL_USART_ERROR_DMA          ((uint32_t)0x00000010U)   /*!< DMA transfer error  */
/**
  * @}
  */

/** @defgroup USART_Stop_Bits  USART Number of Stop Bits
  * @{
  */
#define USART_STOPBITS_1                     ((uint32_t)0x0000U)
#define USART_STOPBITS_2                     ((uint32_t)USART_CR2_STOP_1)
#define USART_STOPBITS_1_5                   ((uint32_t)(USART_CR2_STOP_0 | USART_CR2_STOP_1))
/**
  * @}
  */

/** @defgroup USART_Parity    USART Parity
  * @{
  */
#define USART_PARITY_NONE                   ((uint32_t)0x0000U)
#define USART_PARITY_EVEN                   ((uint32_t)USART_CR1_PCE)
#define USART_PARITY_ODD                    ((uint32_t)(USART_CR1_PCE | USART_CR1_PS))
/**
  * @}
  */

/** @defgroup USART_Mode   USART Mode
  * @{
  */
#define USART_MODE_RX                       ((uint32_t)USART_CR1_RE)
#define USART_MODE_TX                       ((uint32_t)USART_CR1_TE)
#define USART_MODE_TX_RX                    ((uint32_t)(USART_CR1_TE |USART_CR1_RE))
/**
  * @}
  */

/** @defgroup USART_Over_Sampling USART Over Sampling
  * @{
  */
#define USART_OVERSAMPLING_16               ((uint32_t)0x0000U)
#define USART_OVERSAMPLING_8                ((uint32_t)USART_CR1_OVER8)
/**
  * @}
  */
/** @defgroup USART_Clock  USART Clock
  * @{
  */
#define USART_CLOCK_DISABLE                 ((uint32_t)0x0000U)
#define USART_CLOCK_ENABLE                  ((uint32_t)USART_CR2_CLKEN)
/**
  * @}
  */

/** @defgroup USART_Clock_Polarity  USART Clock Polarity
  * @{
  */
#define USART_POLARITY_LOW                  ((uint32_t)0x0000U)
#define USART_POLARITY_HIGH                 ((uint32_t)USART_CR2_CPOL)
/**
  * @}
  */

/** @defgroup USART_Clock_Phase   USART Clock Phase
  * @{
  */
#define USART_PHASE_1EDGE                   ((uint32_t)0x0000U)
#define USART_PHASE_2EDGE                   ((uint32_t)USART_CR2_CPHA)
/**
  * @}
  */

/** @defgroup USART_Last_Bit  USART Last Bit
  * @{
  */
#define USART_LASTBIT_DISABLE               ((uint32_t)0x0000U)
#define USART_LASTBIT_ENABLE                ((uint32_t)USART_CR2_LBCL)
/**
  * @}
  */

/** @defgroup USART_Request_Parameters  USART Request Parameters
  * @{
  */
#define USART_RXDATA_FLUSH_REQUEST        ((uint32_t)USART_RQR_RXFRQ)        /*!< Receive Data flush Request */ 
#define USART_TXDATA_FLUSH_REQUEST        ((uint32_t)USART_RQR_TXFRQ)        /*!< Transmit data flush Request */
/**
  * @}
  */

/** @defgroup USART_Flags      USART Flags
  *        Elements values convention: 0xXXXX
  *           - 0xXXXX  : Flag mask in the ISR register
  * @{
  */
#define USART_FLAG_REACK                     ((uint32_t)0x00400000U)
#define USART_FLAG_TEACK                     ((uint32_t)0x00200000U)  
#define USART_FLAG_BUSY                      ((uint32_t)0x00010000U)
#define USART_FLAG_CTS                       ((uint32_t)0x00000400U)
#define USART_FLAG_CTSIF                     ((uint32_t)0x00000200U)
#define USART_FLAG_LBDF                      ((uint32_t)0x00000100U)
#define USART_FLAG_TXE                       ((uint32_t)0x00000080U)
#define USART_FLAG_TC                        ((uint32_t)0x00000040U)
#define USART_FLAG_RXNE                      ((uint32_t)0x00000020U)
#define USART_FLAG_IDLE                      ((uint32_t)0x00000010U)
#define USART_FLAG_ORE                       ((uint32_t)0x00000008U)
#define USART_FLAG_NE                        ((uint32_t)0x00000004U)
#define USART_FLAG_FE                        ((uint32_t)0x00000002U)
#define USART_FLAG_PE                        ((uint32_t)0x00000001U)
/**
  * @}
  */

/** @defgroup USART_Interrupt_definition USART Interrupts Definition
  *        Elements values convention: 0000ZZZZ0XXYYYYYb
  *           - YYYYY  : Interrupt source position in the XX register (5bits)
  *           - XX  : Interrupt source register (2bits)
  *                 - 01: CR1 register
  *                 - 10: CR2 register
  *                 - 11: CR3 register
  *           - ZZZZ  : Flag position in the ISR register(4bits)
  * @{
  */

#define USART_IT_PE                          ((uint16_t)0x0028U)
#define USART_IT_TXE                         ((uint16_t)0x0727U)
#define USART_IT_TC                          ((uint16_t)0x0626U)
#define USART_IT_RXNE                        ((uint16_t)0x0525U)
#define USART_IT_IDLE                        ((uint16_t)0x0424U)
#define USART_IT_ERR                         ((uint16_t)0x0060U)

#define USART_IT_ORE                         ((uint16_t)0x0300U)
#define USART_IT_NE                          ((uint16_t)0x0200U)
#define USART_IT_FE                          ((uint16_t)0x0100U)
/**
  * @}
  */

/** @defgroup USART_IT_CLEAR_Flags    USART Interruption Clear Flags
  * @{
  */
#define USART_CLEAR_PEF                       USART_ICR_PECF            /*!< Parity Error Clear Flag */
#define USART_CLEAR_FEF                       USART_ICR_FECF            /*!< Framing Error Clear Flag */
#define USART_CLEAR_NEF                       USART_ICR_NCF             /*!< Noise detected Clear Flag */
#define USART_CLEAR_OREF                      USART_ICR_ORECF           /*!< OverRun Error Clear Flag */
#define USART_CLEAR_IDLEF                     USART_ICR_IDLECF          /*!< IDLE line detected Clear Flag */
#define USART_CLEAR_TCF                       USART_ICR_TCCF            /*!< Transmission Complete Clear Flag */
#define USART_CLEAR_CTSF                      USART_ICR_CTSCF           /*!< CTS Interrupt Clear Flag */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup USART_Exported_Macros USART Exported Macros
  * @{
  */

/** @brief Reset USART handle state
  * @param  __HANDLE__ USART handle.
  * @retval None
  */
#define __HAL_USART_RESET_HANDLE_STATE(__HANDLE__)  ((__HANDLE__)->State = HAL_USART_STATE_RESET)

/** @brief  Checks whether the specified USART flag is set or not.
  * @param  __HANDLE__ specifies the USART Handle
  * @param  __FLAG__ specifies the flag to check.
  *        This parameter can be one of the following values:
  *            @arg USART_FLAG_REACK: Receive enable acknowledge flag
  *            @arg USART_FLAG_TEACK: Transmit enable acknowledge flag
  *            @arg USART_FLAG_BUSY:  Busy flag
  *            @arg USART_FLAG_CTS:   CTS Change flag
  *            @arg USART_FLAG_TXE:   Transmit data register empty flag
  *            @arg USART_FLAG_TC:    Transmission Complete flag
  *            @arg USART_FLAG_RXNE:  Receive data register not empty flag
  *            @arg USART_FLAG_IDLE:  Idle Line detection flag
  *            @arg USART_FLAG_ORE:   OverRun Error flag
  *            @arg USART_FLAG_NE:    Noise Error flag
  *            @arg USART_FLAG_FE:    Framing Error flag
  *            @arg USART_FLAG_PE:    Parity Error flag
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_USART_GET_FLAG(__HANDLE__, __FLAG__) (((__HANDLE__)->Instance->ISR & (__FLAG__)) == (__FLAG__))


/** @brief  Enables the specified USART interrupt.
  * @param  __HANDLE__ specifies the USART Handle
  * @param  __INTERRUPT__ specifies the USART interrupt source to enable.
  *          This parameter can be one of the following values:
  *            @arg USART_IT_TXE:  Transmit Data Register empty interrupt
  *            @arg USART_IT_TC:   Transmission complete interrupt
  *            @arg USART_IT_RXNE: Receive Data register not empty interrupt
  *            @arg USART_IT_IDLE: Idle line detection interrupt
  *            @arg USART_IT_PE:   Parity Error interrupt
  *            @arg USART_IT_ERR:  Error interrupt(Frame error, noise error, overrun error)
  * @retval None
  */
#define __HAL_USART_ENABLE_IT(__HANDLE__, __INTERRUPT__)   (((((uint8_t)(__INTERRUPT__)) >> 5U) == 1)? ((__HANDLE__)->Instance->CR1 |= (1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                            ((((uint8_t)(__INTERRUPT__)) >> 5U) == 2)? ((__HANDLE__)->Instance->CR2 |= (1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                            ((__HANDLE__)->Instance->CR3 |= (1U << ((__INTERRUPT__) & USART_IT_MASK))))

/** @brief  Disables the specified USART interrupt.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __INTERRUPT__ specifies the USART interrupt source to disable.
  *          This parameter can be one of the following values:
  *            @arg USART_IT_TXE:  Transmit Data Register empty interrupt
  *            @arg USART_IT_TC:   Transmission complete interrupt
  *            @arg USART_IT_RXNE: Receive Data register not empty interrupt
  *            @arg USART_IT_IDLE: Idle line detection interrupt
  *            @arg USART_IT_PE:   Parity Error interrupt
  *            @arg USART_IT_ERR:  Error interrupt(Frame error, noise error, overrun error)
  * @retval None
  */
#define __HAL_USART_DISABLE_IT(__HANDLE__, __INTERRUPT__)  (((((uint8_t)(__INTERRUPT__)) >> 5U) == 1)? ((__HANDLE__)->Instance->CR1 &= ~ (1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                            ((((uint8_t)(__INTERRUPT__)) >> 5U) == 2)? ((__HANDLE__)->Instance->CR2 &= ~ (1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                            ((__HANDLE__)->Instance->CR3 &= ~ (1U << ((__INTERRUPT__) & USART_IT_MASK))))


/** @brief  Checks whether the specified USART interrupt has occurred or not.
  * @param  __HANDLE__ specifies the USART Handle
  * @param  __IT__ specifies the USART interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg USART_IT_TXE: Transmit Data Register empty interrupt
  *            @arg USART_IT_TC:  Transmission complete interrupt
  *            @arg USART_IT_RXNE: Receive Data register not empty interrupt
  *            @arg USART_IT_IDLE: Idle line detection interrupt
  *            @arg USART_IT_ORE: OverRun Error interrupt
  *            @arg USART_IT_NE: Noise Error interrupt
  *            @arg USART_IT_FE: Framing Error interrupt
  *            @arg USART_IT_PE: Parity Error interrupt
  * @retval The new state of __IT__ (TRUE or FALSE).
  */
#define __HAL_USART_GET_IT(__HANDLE__, __IT__) ((__HANDLE__)->Instance->ISR & ((uint32_t)1 << ((__IT__)>> 0x08)))

/** @brief  Checks whether the specified USART interrupt source is enabled.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __IT__ specifies the USART interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg USART_IT_TXE: Transmit Data Register empty interrupt
  *            @arg USART_IT_TC:  Transmission complete interrupt
  *            @arg USART_IT_RXNE: Receive Data register not empty interrupt
  *            @arg USART_IT_IDLE: Idle line detection interrupt
  *            @arg USART_IT_ORE: OverRun Error interrupt
  *            @arg USART_IT_NE: Noise Error interrupt
  *            @arg USART_IT_FE: Framing Error interrupt
  *            @arg USART_IT_PE: Parity Error interrupt
  * @retval The new state of __IT__ (TRUE or FALSE).
  */
#define __HAL_USART_GET_IT_SOURCE(__HANDLE__, __IT__) ((((((uint8_t)(__IT__)) >> 5) == 1)? (__HANDLE__)->Instance->CR1:(((((uint8_t)(__IT__)) >> 5) == 2)? \
                                                   (__HANDLE__)->Instance->CR2 : (__HANDLE__)->Instance->CR3)) & ((uint32_t)1 << \
                                                   (((uint16_t)(__IT__)) & USART_IT_MASK)))


/** @brief  Clears the specified USART ISR flag, in setting the proper ICR register flag.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __IT_CLEAR__ specifies the interrupt clear register flag that needs to be set
  *                       to clear the corresponding interrupt
  *          This parameter can be one of the following values:
  *            @arg USART_CLEAR_PEF: Parity Error Clear Flag
  *            @arg USART_CLEAR_FEF: Framing Error Clear Flag
  *            @arg USART_CLEAR_NEF: Noise detected Clear Flag
  *            @arg USART_CLEAR_OREF: OverRun Error Clear Flag
  *            @arg USART_CLEAR_IDLEF: IDLE line detected Clear Flag
  *            @arg USART_CLEAR_TCF: Transmission Complete Clear Flag
  *            @arg USART_CLEAR_CTSF: CTS Interrupt Clear Flag
  * @retval None
  */
#define __HAL_USART_CLEAR_IT(__HANDLE__, __IT_CLEAR__) ((__HANDLE__)->Instance->ICR = (uint32_t)(__IT_CLEAR__))

/** @brief  Set a specific USART request flag.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __REQ__ specifies the request flag to set
  *          This parameter can be one of the following values:
  *            @arg USART_RXDATA_FLUSH_REQUEST: Receive Data flush Request
  *            @arg USART_TXDATA_FLUSH_REQUEST: Transmit data flush Request
  *
  * @retval None
  */
#define __HAL_USART_SEND_REQ(__HANDLE__, __REQ__) ((__HANDLE__)->Instance->RQR |= (uint16_t)(__REQ__)) 

/** @brief  Enable USART
  * @param  __HANDLE__ specifies the USART Handle.
  * @retval None
  */
#define __HAL_USART_ENABLE(__HANDLE__)                 ((__HANDLE__)->Instance->CR1 |=  USART_CR1_UE)

/** @brief  Disable USART
  * @param  __HANDLE__ specifies the USART Handle.
  * @retval None
  */
#define __HAL_USART_DISABLE(__HANDLE__)                ((__HANDLE__)->Instance->CR1 &=  ~USART_CR1_UE)

/**
  * @}
  */
/* Include UART HAL Extension module */
#include "stm32f7xx_hal_usart_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @addtogroup USART_Exported_Functions
  * @{
  */

/** @addtogroup USART_Exported_Functions_Group1
  * @{
  */
/* Initialization/de-initialization functions  **********************************/
HAL_StatusTypeDef HAL_USART_Init(USART_HandleTypeDef *husart);
HAL_StatusTypeDef HAL_USART_DeInit(USART_HandleTypeDef *husart);
void HAL_USART_MspInit(USART_HandleTypeDef *husart);
void HAL_USART_MspDeInit(USART_HandleTypeDef *husart);
HAL_StatusTypeDef HAL_USART_CheckIdleState(USART_HandleTypeDef *husart);
/**
  * @}
  */

/** @addtogroup USART_Exported_Functions_Group2
  * @{
  */
/* IO operation functions *******************************************************/
HAL_StatusTypeDef HAL_USART_Transmit(USART_HandleTypeDef *husart, uint8_t *pTxData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_USART_Receive(USART_HandleTypeDef *husart, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_USART_TransmitReceive(USART_HandleTypeDef *husart, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_USART_Transmit_IT(USART_HandleTypeDef *husart, uint8_t *pTxData, uint16_t Size);
HAL_StatusTypeDef HAL_USART_Receive_IT(USART_HandleTypeDef *husart, uint8_t *pRxData, uint16_t Size);
HAL_StatusTypeDef HAL_USART_TransmitReceive_IT(USART_HandleTypeDef *husart, uint8_t *pTxData, uint8_t *pRxData,  uint16_t Size);
HAL_StatusTypeDef HAL_USART_Transmit_DMA(USART_HandleTypeDef *husart, uint8_t *pTxData, uint16_t Size);
HAL_StatusTypeDef HAL_USART_Receive_DMA(USART_HandleTypeDef *husart, uint8_t *pRxData, uint16_t Size);
HAL_StatusTypeDef HAL_USART_TransmitReceive_DMA(USART_HandleTypeDef *husart, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size);
HAL_StatusTypeDef HAL_USART_DMAPause(USART_HandleTypeDef *husart);
HAL_StatusTypeDef HAL_USART_DMAResume(USART_HandleTypeDef *husart);
HAL_StatusTypeDef HAL_USART_DMAStop(USART_HandleTypeDef *husart);
void HAL_USART_IRQHandler(USART_HandleTypeDef *husart);
void HAL_USART_TxHalfCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_TxCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_RxCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_RxHalfCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_TxRxCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_ErrorCallback(USART_HandleTypeDef *husart);

/**
  * @}
  */ 

/** @addtogroup USART_Exported_Functions_Group3
  * @{
  */
/* Peripheral State functions  ************************************************/
HAL_USART_StateTypeDef HAL_USART_GetState(USART_HandleTypeDef *husart);
uint32_t               HAL_USART_GetError(USART_HandleTypeDef *husart);

/**
  * @}
  */

/**
  * @}
  */
/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup USART_Private_Constants USART Private Constants
  * @{
  */
/** @brief USART interruptions flag mask
  * 
  */ 
#define USART_IT_MASK                             ((uint16_t)0x001FU)

/**
  * @}
  */
/* Private macros ------------------------------------------------------------*/
/** @defgroup USART_Private_Macros USART Private Macros
  * @{
  */
/** @brief  Reports the USART clock source.
  * @param  __HANDLE__ specifies the USART Handle
  * @param  __CLOCKSOURCE__  output variable
  * @retval the USART clocking source, written in __CLOCKSOURCE__.
  */
#define USART_GETCLOCKSOURCE(__HANDLE__, __CLOCKSOURCE__)\
  do {                                                         \
    if((__HANDLE__)->Instance == USART1)                       \
    {                                                          \
       switch(__HAL_RCC_GET_USART1_SOURCE())                   \
       {                                                       \
        case RCC_USART1CLKSOURCE_PCLK2:                        \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK2;         \
          break;                                               \
        case RCC_USART1CLKSOURCE_HSI:                          \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_HSI;           \
          break;                                               \
        case RCC_USART1CLKSOURCE_SYSCLK:                       \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_SYSCLK;        \
          break;                                               \
        case RCC_USART1CLKSOURCE_LSE:                          \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_LSE;           \
          break;                                               \
        default:                                               \
          break;                                               \
       }                                                       \
    }                                                          \
    else if((__HANDLE__)->Instance == USART2)                  \
    {                                                          \
       switch(__HAL_RCC_GET_USART2_SOURCE())                   \
       {                                                       \
        case RCC_USART2CLKSOURCE_PCLK1:                        \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;         \
          break;                                               \
        case RCC_USART2CLKSOURCE_HSI:                          \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_HSI;           \
          break;                                               \
        case RCC_USART2CLKSOURCE_SYSCLK:                       \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_SYSCLK;        \
          break;                                               \
        case RCC_USART2CLKSOURCE_LSE:                          \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_LSE;           \
          break;                                               \
        default:                                               \
          break;                                               \
       }                                                       \
    }                                                          \
    else if((__HANDLE__)->Instance == USART3)                  \
    {                                                          \
       switch(__HAL_RCC_GET_USART3_SOURCE())                   \
       {                                                       \
        case RCC_USART3CLKSOURCE_PCLK1:                        \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;         \
          break;                                               \
        case RCC_USART3CLKSOURCE_HSI:                          \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_HSI;           \
          break;                                               \
        case RCC_USART3CLKSOURCE_SYSCLK:                       \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_SYSCLK;        \
          break;                                               \
        case RCC_USART3CLKSOURCE_LSE:                          \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_LSE;           \
          break;                                               \
        default:                                               \
          break;                                               \
       }                                                       \
    }                                                          \
    else if((__HANDLE__)->Instance == USART6)                  \
    {                                                          \
       switch(__HAL_RCC_GET_USART6_SOURCE())                   \
       {                                                       \
        case RCC_USART6CLKSOURCE_PCLK2:                        \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK2;         \
          break;                                               \
        case RCC_USART6CLKSOURCE_HSI:                          \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_HSI;           \
          break;                                               \
        case RCC_USART6CLKSOURCE_SYSCLK:                       \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_SYSCLK;        \
          break;                                               \
        case RCC_USART6CLKSOURCE_LSE:                          \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_LSE;           \
          break;                                               \
        default:                                               \
          break;                                               \
       }                                                       \
    }                                                          \
 } while(0)
  

#define IS_USART_STOPBITS(__STOPBITS__) (((__STOPBITS__) == USART_STOPBITS_1) || \
                                         ((__STOPBITS__) == USART_STOPBITS_1_5) || \
                                         ((__STOPBITS__) == USART_STOPBITS_2))
#define IS_USART_PARITY(__PARITY__) (((__PARITY__) == USART_PARITY_NONE) || \
                                     ((__PARITY__) == USART_PARITY_EVEN) || \
                                     ((__PARITY__) == USART_PARITY_ODD))
#define IS_USART_MODE(__MODE__) ((((__MODE__) & (uint32_t)0xFFFFFFF3U) == 0x00U) && ((__MODE__) != (uint32_t)0x00U))
#define IS_USART_OVERSAMPLING(__SAMPLING__) (((__SAMPLING__) == USART_OVERSAMPLING_16) || \
                                             ((__SAMPLING__) == USART_OVERSAMPLING_8))
#define IS_USART_CLOCK(__CLOCK__)     (((__CLOCK__)== USART_CLOCK_DISABLE) || \
                                       ((__CLOCK__)== USART_CLOCK_ENABLE))
#define IS_USART_POLARITY(__CPOL__) (((__CPOL__) == USART_POLARITY_LOW) || ((__CPOL__) == USART_POLARITY_HIGH))
#define IS_USART_PHASE(__CPHA__) (((__CPHA__) == USART_PHASE_1EDGE) || ((__CPHA__) == USART_PHASE_2EDGE))
#define IS_USART_LASTBIT(__LASTBIT__) (((__LASTBIT__) == USART_LASTBIT_DISABLE) || \
                                       ((__LASTBIT__) == USART_LASTBIT_ENABLE))
#define IS_USART_REQUEST_PARAMETER(__PARAM__) (((__PARAM__) == USART_RXDATA_FLUSH_REQUEST) || \
                                               ((__PARAM__) == USART_TXDATA_FLUSH_REQUEST))   
#define IS_USART_BAUDRATE(__BAUDRATE__) ((__BAUDRATE__) < 9000001)

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/
/** @defgroup USART_Private_Functions USART Private Functions
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

#endif /* __STM32F7xx_HAL_USART_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
