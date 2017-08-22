/**
  ******************************************************************************
  * @file    stm32f0xx_hal_usart.h
  * @author  MCD Application Team
  * @brief   Header file of USART HAL module.
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
#ifndef __STM32F0xx_HAL_USART_H
#define __STM32F0xx_HAL_USART_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal_def.h"

/** @addtogroup STM32F0xx_HAL_Driver
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
                                              Baud Rate Register = ((PCLKx) / ((huart->Init.BaudRate))). */

  uint32_t WordLength;                /*!< Specifies the number of data bits transmitted or received in a frame.
                                           This parameter can be a value of @ref USARTEx_Word_Length. */

  uint32_t StopBits;                  /*!< Specifies the number of stop bits transmitted.
                                           This parameter can be a value of @ref USART_Stop_Bits. */

  uint32_t Parity;                   /*!< Specifies the parity mode.
                                           This parameter can be a value of @ref USART_Parity
                                           @note When parity is enabled, the computed parity is inserted
                                                 at the MSB position of the transmitted data (9th bit when
                                                 the word length is set to 9 data bits; 8th bit when the
                                                 word length is set to 8 data bits). */

  uint32_t Mode;                      /*!< Specifies whether the Receive or Transmit mode is enabled or disabled.
                                           This parameter can be a value of @ref USART_Mode. */

  uint32_t CLKPolarity;               /*!< Specifies the steady state of the serial clock.
                                           This parameter can be a value of @ref USART_Clock_Polarity. */

  uint32_t CLKPhase;                  /*!< Specifies the clock transition on which the bit capture is made.
                                           This parameter can be a value of @ref USART_Clock_Phase. */

  uint32_t CLKLastBit;                /*!< Specifies whether the clock pulse corresponding to the last transmitted
                                           data bit (MSB) has to be output on the SCLK pin in synchronous mode.
                                           This parameter can be a value of @ref USART_Last_Bit. */
}USART_InitTypeDef;

/**
  * @brief HAL USART State structures definition
  */
typedef enum
{
  HAL_USART_STATE_RESET             = 0x00U,    /*!< Peripheral is not initialized                  */
  HAL_USART_STATE_READY             = 0x01U,    /*!< Peripheral Initialized and ready for use       */
  HAL_USART_STATE_BUSY              = 0x02U,    /*!< an internal process is ongoing                 */
  HAL_USART_STATE_BUSY_TX           = 0x12U,    /*!< Data Transmission process is ongoing           */
  HAL_USART_STATE_BUSY_RX           = 0x22U,    /*!< Data Reception process is ongoing              */
  HAL_USART_STATE_BUSY_TX_RX        = 0x32U,    /*!< Data Transmission Reception process is ongoing */
  HAL_USART_STATE_TIMEOUT           = 0x03U,    /*!< Timeout state                                  */
  HAL_USART_STATE_ERROR             = 0x04U     /*!< Error                                          */
}HAL_USART_StateTypeDef;

/**
  * @brief  USART clock sources definitions
  */
typedef enum
{
  USART_CLOCKSOURCE_PCLK1      = 0x00U,    /*!< PCLK1 clock source     */
  USART_CLOCKSOURCE_HSI        = 0x02U,    /*!< HSI clock source       */
  USART_CLOCKSOURCE_SYSCLK     = 0x04U,    /*!< SYSCLK clock source    */
  USART_CLOCKSOURCE_LSE        = 0x08U,    /*!< LSE clock source       */
  USART_CLOCKSOURCE_UNDEFINED  = 0x10U     /*!< Undefined clock source */
}USART_ClockSourceTypeDef;


/**
  * @brief  USART handle Structure definition
  */
typedef struct
{
  USART_TypeDef                 *Instance;        /*!< USART registers base address        */

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

  HAL_LockTypeDef               Lock;             /*!< Locking object                      */

  __IO HAL_USART_StateTypeDef   State;            /*!< USART communication state           */

  __IO uint32_t                 ErrorCode;        /*!< USART Error code                    */

}USART_HandleTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup USART_Exported_Constants USART Exported Constants
  * @{
  */

/** @defgroup USART_Error USART Error
  * @{
  */
#define HAL_USART_ERROR_NONE      (0x00000000U)    /*!< No error            */
#define HAL_USART_ERROR_PE        (0x00000001U)    /*!< Parity error        */
#define HAL_USART_ERROR_NE        (0x00000002U)    /*!< Noise error         */
#define HAL_USART_ERROR_FE        (0x00000004U)    /*!< frame error         */
#define HAL_USART_ERROR_ORE       (0x00000008U)    /*!< Overrun error       */
#define HAL_USART_ERROR_DMA       (0x00000010U)    /*!< DMA transfer error  */
/**
  * @}
  */ 

/** @defgroup USART_Stop_Bits  USART Number of Stop Bits
  * @{
  */
#ifdef USART_SMARTCARD_SUPPORT
#define USART_STOPBITS_0_5                  ((uint32_t)USART_CR2_STOP_0)                      /*!< USART frame with 0.5 stop bit  */
#define USART_STOPBITS_1                    (0x00000000U)                                     /*!< USART frame with 1 stop bit    */
#define USART_STOPBITS_1_5                  ((uint32_t)(USART_CR2_STOP_0 | USART_CR2_STOP_1)) /*!< USART frame with 1.5 stop bits */
#define USART_STOPBITS_2                    ((uint32_t)USART_CR2_STOP_1)                      /*!< USART frame with 2 stop bits   */
#else
#define USART_STOPBITS_1                    (0x00000000U)                                     /*!< USART frame with 1 stop bit    */
#define USART_STOPBITS_2                    ((uint32_t)USART_CR2_STOP_1)                      /*!< USART frame with 2 stop bits   */
#endif
/**
  * @}
  */

/** @defgroup USART_Parity    USART Parity
  * @{
  */
#define USART_PARITY_NONE                   (0x00000000U)                               /*!< No parity   */
#define USART_PARITY_EVEN                   ((uint32_t)USART_CR1_PCE)                   /*!< Even parity */
#define USART_PARITY_ODD                    ((uint32_t)(USART_CR1_PCE | USART_CR1_PS))  /*!< Odd parity  */
/**
  * @}
  */

/** @defgroup USART_Mode   USART Mode
  * @{
  */
#define USART_MODE_RX                       ((uint32_t)USART_CR1_RE)                   /*!< RX mode        */
#define USART_MODE_TX                       ((uint32_t)USART_CR1_TE)                   /*!< TX mode        */
#define USART_MODE_TX_RX                    ((uint32_t)(USART_CR1_TE |USART_CR1_RE))   /*!< RX and TX mode */
/**
  * @}
  */

/** @defgroup USART_Clock  USART Clock
  * @{
  */
#define USART_CLOCK_DISABLE                 (0x00000000U)                 /*!< USART clock disable */
#define USART_CLOCK_ENABLE                  ((uint32_t)USART_CR2_CLKEN)   /*!< USART clock enable  */
/**
  * @}
  */

/** @defgroup USART_Clock_Polarity  USART Clock Polarity
  * @{
  */
#define USART_POLARITY_LOW                  (0x00000000U)                /*!< USART Clock signal is steady Low  */
#define USART_POLARITY_HIGH                 ((uint32_t)USART_CR2_CPOL)   /*!< USART Clock signal is steady High */
/**
  * @}
  */

/** @defgroup USART_Clock_Phase   USART Clock Phase
  * @{
  */
#define USART_PHASE_1EDGE                   (0x00000000U)                /*!< USART frame phase on first clock transition  */
#define USART_PHASE_2EDGE                   ((uint32_t)USART_CR2_CPHA)   /*!< USART frame phase on second clock transition */
/**
  * @}
  */

/** @defgroup USART_Last_Bit  USART Last Bit
  * @{
  */
#define USART_LASTBIT_DISABLE               (0x00000000U)                /*!< USART frame last data bit clock pulse not output to SCLK pin */
#define USART_LASTBIT_ENABLE                ((uint32_t)USART_CR2_LBCL)   /*!< USART frame last data bit clock pulse output to SCLK pin     */
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

#define USART_IT_PE                          ((uint16_t)0x0028U)     /*!< USART parity error interruption                 */   
#define USART_IT_TXE                         ((uint16_t)0x0727U)     /*!< USART transmit data register empty interruption */   
#define USART_IT_TC                          ((uint16_t)0x0626U)     /*!< USART transmission complete interruption        */   
#define USART_IT_RXNE                        ((uint16_t)0x0525U)     /*!< USART read data register not empty interruption */   
#define USART_IT_IDLE                        ((uint16_t)0x0424U)     /*!< USART idle interruption                         */   
#define USART_IT_ERR                         ((uint16_t)0x0060U)     /*!< USART error interruption                        */
#define USART_IT_ORE                         ((uint16_t)0x0300U)     /*!< USART overrun error interruption                */ 
#define USART_IT_NE                          ((uint16_t)0x0200U)     /*!< USART noise error interruption                  */ 
#define USART_IT_FE                          ((uint16_t)0x0100U)     /*!< USART frame error interruption                  */ 
/**
  * @}
  */

/** @defgroup USART_IT_CLEAR_Flags    USART Interruption Clear Flags
  * @{
  */
#define USART_CLEAR_PEF                       USART_ICR_PECF            /*!< Parity Error Clear Flag          */
#define USART_CLEAR_FEF                       USART_ICR_FECF            /*!< Framing Error Clear Flag         */
#define USART_CLEAR_NEF                       USART_ICR_NCF             /*!< Noise detected Clear Flag        */
#define USART_CLEAR_OREF                      USART_ICR_ORECF           /*!< OverRun Error Clear Flag         */
#define USART_CLEAR_IDLEF                     USART_ICR_IDLECF          /*!< IDLE line detected Clear Flag    */
#define USART_CLEAR_TCF                       USART_ICR_TCCF            /*!< Transmission Complete Clear Flag */
#define USART_CLEAR_CTSF                      USART_ICR_CTSCF           /*!< CTS Interrupt Clear Flag         */
/**
  * @}
  */

/** @defgroup USART_Interruption_Mask    USART Interruption Flags Mask
  * @{
  */
#define USART_IT_MASK                             ((uint16_t)0x001FU)     /*!< USART interruptions flags mask */
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

/** @brief  Reset USART handle state.
  * @param  __HANDLE__ USART handle.
  * @retval None
  */
#define __HAL_USART_RESET_HANDLE_STATE(__HANDLE__)  ((__HANDLE__)->State = HAL_USART_STATE_RESET)

/** @brief  Check whether the specified USART flag is set or not.
  * @param  __HANDLE__ specifies the USART Handle
  * @param  __FLAG__ specifies the flag to check.
  *        This parameter can be one of the following values:
  @if STM32F030x6
  @elseif STM32F030x8
  @elseif STM32F030xC
  @elseif STM32F070x6
  @elseif STM32F070xB
  @else
  *            @arg @ref USART_FLAG_REACK Receive enable acknowledge flag
  @endif
  *            @arg @ref USART_FLAG_TEACK Transmit enable acknowledge flag
  *            @arg @ref USART_FLAG_BUSY  Busy flag
  *            @arg @ref USART_FLAG_CTS   CTS Change flag
  *            @arg @ref USART_FLAG_TXE   Transmit data register empty flag
  *            @arg @ref USART_FLAG_TC    Transmission Complete flag
  *            @arg @ref USART_FLAG_RXNE  Receive data register not empty flag
  *            @arg @ref USART_FLAG_IDLE  Idle Line detection flag
  *            @arg @ref USART_FLAG_ORE   OverRun Error flag
  *            @arg @ref USART_FLAG_NE    Noise Error flag
  *            @arg @ref USART_FLAG_FE    Framing Error flag
  *            @arg @ref USART_FLAG_PE    Parity Error flag
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_USART_GET_FLAG(__HANDLE__, __FLAG__) (((__HANDLE__)->Instance->ISR & (__FLAG__)) == (__FLAG__))

/** @brief  Clear the specified USART pending flag.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __FLAG__ specifies the flag to check.
  *          This parameter can be any combination of the following values:
  *            @arg @ref USART_CLEAR_PEF
  *            @arg @ref USART_CLEAR_FEF
  *            @arg @ref USART_CLEAR_NEF
  *            @arg @ref USART_CLEAR_OREF
  *            @arg @ref USART_CLEAR_IDLEF
  *            @arg @ref USART_CLEAR_TCF
  *            @arg @ref USART_CLEAR_CTSF
  * @retval None
  */
#define __HAL_USART_CLEAR_FLAG(__HANDLE__, __FLAG__) ((__HANDLE__)->Instance->ICR = (__FLAG__))

/** @brief  Clear the USART PE pending flag.
  * @param  __HANDLE__ specifies the USART Handle.
  * @retval None
  */
#define __HAL_USART_CLEAR_PEFLAG(__HANDLE__)   __HAL_USART_CLEAR_FLAG((__HANDLE__), USART_CLEAR_PEF)

/** @brief  Clear the USART FE pending flag.
  * @param  __HANDLE__ specifies the USART Handle.
  * @retval None
  */
#define __HAL_USART_CLEAR_FEFLAG(__HANDLE__)   __HAL_USART_CLEAR_FLAG((__HANDLE__), USART_CLEAR_FEF)

/** @brief  Clear the USART NE pending flag.
  * @param  __HANDLE__ specifies the USART Handle.
  * @retval None
  */
#define __HAL_USART_CLEAR_NEFLAG(__HANDLE__)  __HAL_USART_CLEAR_FLAG((__HANDLE__), USART_CLEAR_NEF)

/** @brief  Clear the USART ORE pending flag.
  * @param  __HANDLE__ specifies the USART Handle.
  * @retval None
  */
#define __HAL_USART_CLEAR_OREFLAG(__HANDLE__)   __HAL_USART_CLEAR_FLAG((__HANDLE__), USART_CLEAR_OREF)

/** @brief  Clear the USART IDLE pending flag.
  * @param  __HANDLE__ specifies the USART Handle.
  * @retval None
  */
#define __HAL_USART_CLEAR_IDLEFLAG(__HANDLE__)   __HAL_USART_CLEAR_FLAG((__HANDLE__), USART_CLEAR_IDLEF)

/** @brief  Enable the specified USART interrupt.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __INTERRUPT__ specifies the USART interrupt source to enable.
  *          This parameter can be one of the following values:
  *            @arg @ref USART_IT_TXE  Transmit Data Register empty interrupt
  *            @arg @ref USART_IT_TC   Transmission complete interrupt
  *            @arg @ref USART_IT_RXNE Receive Data register not empty interrupt
  *            @arg @ref USART_IT_IDLE Idle line detection interrupt
  *            @arg @ref USART_IT_PE   Parity Error interrupt
  *            @arg @ref USART_IT_ERR  Error interrupt(Frame error, noise error, overrun error)
  * @retval None
  */
#define __HAL_USART_ENABLE_IT(__HANDLE__, __INTERRUPT__)   (((((__INTERRUPT__) & 0xFF) >> 5U) == 1U)? ((__HANDLE__)->Instance->CR1 |= (1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                            ((((__INTERRUPT__) & 0xFF) >> 5U) == 2U)? ((__HANDLE__)->Instance->CR2 |= (1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                            ((__HANDLE__)->Instance->CR3 |= (1U << ((__INTERRUPT__) & USART_IT_MASK))))

/** @brief  Disable the specified USART interrupt.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __INTERRUPT__ specifies the USART interrupt source to disable.
  *          This parameter can be one of the following values:
  *            @arg @ref USART_IT_TXE  Transmit Data Register empty interrupt
  *            @arg @ref USART_IT_TC   Transmission complete interrupt
  *            @arg @ref USART_IT_RXNE Receive Data register not empty interrupt
  *            @arg @ref USART_IT_IDLE Idle line detection interrupt
  *            @arg @ref USART_IT_PE   Parity Error interrupt
  *            @arg @ref USART_IT_ERR  Error interrupt(Frame error, noise error, overrun error)
  * @retval None
  */
#define __HAL_USART_DISABLE_IT(__HANDLE__, __INTERRUPT__)  (((((__INTERRUPT__) & 0xFF) >> 5U) == 1U)? ((__HANDLE__)->Instance->CR1 &= ~ (1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                            ((((__INTERRUPT__) & 0xFF) >> 5U) == 2U)? ((__HANDLE__)->Instance->CR2 &= ~ (1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                            ((__HANDLE__)->Instance->CR3 &= ~ (1U << ((__INTERRUPT__) & USART_IT_MASK))))


/** @brief  Check whether the specified USART interrupt has occurred or not.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __IT__ specifies the USART interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg @ref USART_IT_TXE Transmit Data Register empty interrupt
  *            @arg @ref USART_IT_TC  Transmission complete interrupt
  *            @arg @ref USART_IT_RXNE Receive Data register not empty interrupt
  *            @arg @ref USART_IT_IDLE Idle line detection interrupt
  *            @arg @ref USART_IT_ORE OverRun Error interrupt
  *            @arg @ref USART_IT_NE Noise Error interrupt
  *            @arg @ref USART_IT_FE Framing Error interrupt
  *            @arg @ref USART_IT_PE Parity Error interrupt
  * @retval The new state of __IT__ (TRUE or FALSE).
  */
#define __HAL_USART_GET_IT(__HANDLE__, __IT__) ((__HANDLE__)->Instance->ISR & (1U << ((__IT__)>> 0x08U)))

/** @brief  Check whether the specified USART interrupt source is enabled or not.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __IT__ specifies the USART interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg @ref USART_IT_TXE Transmit Data Register empty interrupt
  *            @arg @ref USART_IT_TC  Transmission complete interrupt
  *            @arg @ref USART_IT_RXNE Receive Data register not empty interrupt
  *            @arg @ref USART_IT_IDLE Idle line detection interrupt
  *            @arg @ref USART_IT_ORE OverRun Error interrupt
  *            @arg @ref USART_IT_NE Noise Error interrupt
  *            @arg @ref USART_IT_FE Framing Error interrupt
  *            @arg @ref USART_IT_PE Parity Error interrupt
  * @retval The new state of __IT__ (TRUE or FALSE).
  */
#define __HAL_USART_GET_IT_SOURCE(__HANDLE__, __IT__) ((((((uint8_t)(__IT__)) >> 5U) == 1U)? (__HANDLE__)->Instance->CR1:(((((uint8_t)(__IT__)) >> 5U) == 2U)? \
                                                   (__HANDLE__)->Instance->CR2 : (__HANDLE__)->Instance->CR3)) & (1U << \
                                                   (((uint16_t)(__IT__)) & USART_IT_MASK)))


/** @brief  Clear the specified USART ISR flag, in setting the proper ICR register flag.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __IT_CLEAR__ specifies the interrupt clear register flag that needs to be set
  *                       to clear the corresponding interrupt.
  *          This parameter can be one of the following values:
  *            @arg @ref USART_CLEAR_PEF Parity Error Clear Flag
  *            @arg @ref USART_CLEAR_FEF Framing Error Clear Flag
  *            @arg @ref USART_CLEAR_NEF Noise detected Clear Flag
  *            @arg @ref USART_CLEAR_OREF OverRun Error Clear Flag
  *            @arg @ref USART_CLEAR_IDLEF IDLE line detected Clear Flag
  *            @arg @ref USART_CLEAR_TCF Transmission Complete Clear Flag
  *            @arg @ref USART_CLEAR_CTSF CTS Interrupt Clear Flag
  * @retval None
  */
#define __HAL_USART_CLEAR_IT(__HANDLE__, __IT_CLEAR__) ((__HANDLE__)->Instance->ICR = (uint32_t)(__IT_CLEAR__))

/** @brief  Set a specific USART request flag.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __REQ__ specifies the request flag to set.
  *          This parameter can be one of the following values:
  *            @arg @ref USART_RXDATA_FLUSH_REQUEST Receive Data flush Request
  @if STM32F030x6
  @elseif STM32F030x8
  @elseif STM32F030xC
  @elseif STM32F070x6
  @elseif STM32F070xB
  @else
  *            @arg @ref USART_TXDATA_FLUSH_REQUEST Transmit data flush Request
  @endif
  *
  * @retval None
  */
#define __HAL_USART_SEND_REQ(__HANDLE__, __REQ__)      ((__HANDLE__)->Instance->RQR |= (__REQ__))

/** @brief  Enable the USART one bit sample method.
  * @param  __HANDLE__ specifies the USART Handle.  
  * @retval None
  */
#define __HAL_USART_ONE_BIT_SAMPLE_ENABLE(__HANDLE__) ((__HANDLE__)->Instance->CR3|= USART_CR3_ONEBIT)

/** @brief  Disable the USART one bit sample method.
  * @param  __HANDLE__ specifies the USART Handle.  
  * @retval None
  */
#define __HAL_USART_ONE_BIT_SAMPLE_DISABLE(__HANDLE__) ((__HANDLE__)->Instance->CR3 &= (uint32_t)~((uint32_t)USART_CR3_ONEBIT))

/** @brief  Enable USART.
  * @param  __HANDLE__ specifies the USART Handle.
  * @retval None
  */
#define __HAL_USART_ENABLE(__HANDLE__)                 ((__HANDLE__)->Instance->CR1 |=  USART_CR1_UE)

/** @brief  Disable USART.
  * @param  __HANDLE__ specifies the USART Handle.
  * @retval None
  */
#define __HAL_USART_DISABLE(__HANDLE__)                ((__HANDLE__)->Instance->CR1 &=  ~USART_CR1_UE)

/**
  * @}
  */

/* Private macros --------------------------------------------------------*/
/** @defgroup USART_Private_Macros   USART Private Macros
  * @{
  */

/** @brief  Check USART Baud rate.
  * @param  __BAUDRATE__ Baudrate specified by the user.
  *         The maximum Baud Rate is derived from the maximum clock on F0 (i.e. 48 MHz)
  *         divided by the smallest oversampling used on the USART (i.e. 8)
  * @retval Test result (TRUE or FALSE).
  */
#define IS_USART_BAUDRATE(__BAUDRATE__) ((__BAUDRATE__) < 6000001U)

/**
  * @brief Ensure that USART frame number of stop bits is valid.
  * @param __STOPBITS__ USART frame number of stop bits. 
  * @retval SET (__STOPBITS__ is valid) or RESET (__STOPBITS__ is invalid)
  */
#ifdef USART_SMARTCARD_SUPPORT
#define IS_USART_STOPBITS(__STOPBITS__) (((__STOPBITS__) == USART_STOPBITS_0_5) || \
                                         ((__STOPBITS__) == USART_STOPBITS_1)   || \
                                         ((__STOPBITS__) == USART_STOPBITS_1_5) || \
                                         ((__STOPBITS__) == USART_STOPBITS_2))
#else
#define IS_USART_STOPBITS(__STOPBITS__) (((__STOPBITS__) == USART_STOPBITS_1)   || \
                                         ((__STOPBITS__) == USART_STOPBITS_2))
#endif

/**
  * @brief Ensure that USART frame parity is valid.
  * @param __PARITY__ USART frame parity. 
  * @retval SET (__PARITY__ is valid) or RESET (__PARITY__ is invalid)
  */ 
#define IS_USART_PARITY(__PARITY__) (((__PARITY__) == USART_PARITY_NONE) || \
                                     ((__PARITY__) == USART_PARITY_EVEN) || \
                                     ((__PARITY__) == USART_PARITY_ODD))

/**
  * @brief Ensure that USART communication mode is valid.
  * @param __MODE__ USART communication mode. 
  * @retval SET (__MODE__ is valid) or RESET (__MODE__ is invalid)
  */ 
#define IS_USART_MODE(__MODE__) ((((__MODE__) & 0xFFFFFFF3U) == 0x00U) && ((__MODE__) != 0x00U))

/**
  * @brief Ensure that USART clock state is valid.
  * @param __CLOCK__ USART clock state. 
  * @retval SET (__CLOCK__ is valid) or RESET (__CLOCK__ is invalid)
  */ 
#define IS_USART_CLOCK(__CLOCK__) (((__CLOCK__) == USART_CLOCK_DISABLE) || \
                                   ((__CLOCK__) == USART_CLOCK_ENABLE))

/**
  * @brief Ensure that USART frame polarity is valid.
  * @param __CPOL__ USART frame polarity. 
  * @retval SET (__CPOL__ is valid) or RESET (__CPOL__ is invalid)
  */ 
#define IS_USART_POLARITY(__CPOL__) (((__CPOL__) == USART_POLARITY_LOW) || ((__CPOL__) == USART_POLARITY_HIGH))

/**
  * @brief Ensure that USART frame phase is valid.
  * @param __CPHA__ USART frame phase. 
  * @retval SET (__CPHA__ is valid) or RESET (__CPHA__ is invalid)
  */
#define IS_USART_PHASE(__CPHA__) (((__CPHA__) == USART_PHASE_1EDGE) || ((__CPHA__) == USART_PHASE_2EDGE))

/**
  * @brief Ensure that USART frame last bit clock pulse setting is valid.
  * @param __LASTBIT__ USART frame last bit clock pulse setting. 
  * @retval SET (__LASTBIT__ is valid) or RESET (__LASTBIT__ is invalid)
  */
#define IS_USART_LASTBIT(__LASTBIT__) (((__LASTBIT__) == USART_LASTBIT_DISABLE) || \
                                       ((__LASTBIT__) == USART_LASTBIT_ENABLE))

/**
  * @}
  */

/* Include USART HAL Extended module */
#include "stm32f0xx_hal_usart_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @addtogroup USART_Exported_Functions USART Exported Functions
  * @{
  */

/** @addtogroup USART_Exported_Functions_Group1 Initialization and de-initialization functions
  * @{
  */

/* Initialization and de-initialization functions  ****************************/
HAL_StatusTypeDef HAL_USART_Init(USART_HandleTypeDef *husart);
HAL_StatusTypeDef HAL_USART_DeInit(USART_HandleTypeDef *husart);
void HAL_USART_MspInit(USART_HandleTypeDef *husart);
void HAL_USART_MspDeInit(USART_HandleTypeDef *husart);

/**
  * @}
  */

/** @addtogroup USART_Exported_Functions_Group2 IO operation functions
  * @{
  */

/* IO operation functions *****************************************************/
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
/* Transfer Abort functions */
HAL_StatusTypeDef HAL_USART_Abort(USART_HandleTypeDef *husart);
HAL_StatusTypeDef HAL_USART_Abort_IT(USART_HandleTypeDef *husart);

void HAL_USART_IRQHandler(USART_HandleTypeDef *husart);
void HAL_USART_TxCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_RxCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_TxHalfCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_RxHalfCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_TxRxCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_ErrorCallback(USART_HandleTypeDef *husart);
void HAL_USART_AbortCpltCallback (USART_HandleTypeDef *husart);

/**
  * @}
  */

/* Peripheral Control functions ***********************************************/

/** @addtogroup USART_Exported_Functions_Group3 Peripheral State and Error functions
  * @{
  */

/* Peripheral State and Error functions ***************************************/
HAL_USART_StateTypeDef HAL_USART_GetState(USART_HandleTypeDef *husart);
uint32_t               HAL_USART_GetError(USART_HandleTypeDef *husart);

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

#endif /* __STM32F0xx_HAL_USART_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

