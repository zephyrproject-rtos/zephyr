/**
  ******************************************************************************
  * @file    stm32l0xx_hal_usart.h
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
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
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L0xx_HAL_USART_H
#define __STM32L0xx_HAL_USART_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal_def.h"

/** @addtogroup STM32L0xx_HAL_Driver
  * @{
  */

/** @defgroup USART USART (Synchronous UART)
  * @{
  */ 

/******************************************************************************/
/* Exported types ------------------------------------------------------------*/
/******************************************************************************/

   /** @defgroup USART_Exported_Types USART Exported Types
  * @{
  */

/** @defgroup USART_Init_Configuration USART initialization configuration structure
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
 
  uint32_t Mode;                      /*!< Specifies wether the Receive or Transmit mode is enabled or disabled.
                                           This parameter can be a value of @ref USART_Mode */

  uint32_t CLKPolarity;               /*!< Specifies the steady state of the serial clock.
                                           This parameter can be a value of @ref USART_Clock_Polarity */

  uint32_t CLKPhase;                  /*!< Specifies the clock transition on which the bit capture is made.
                                           This parameter can be a value of @ref USART_Clock_Phase */

  uint32_t CLKLastBit;                /*!< Specifies whether the clock pulse corresponding to the last transmitted
                                           data bit (MSB) has to be output on the SCLK pin in synchronous mode.
                                           This parameter can be a value of @ref USART_Last_Bit */
}USART_InitTypeDef;

/**
  * @}
  */

/** @defgroup USART_State_Definition  USART state definition
  * @{
  */
/** 
  * @brief HAL State structures definition  
  */ 
typedef enum
{
  HAL_USART_STATE_RESET             = 0x00U,    /*!< Peripheral Reset state */  
  HAL_USART_STATE_READY             = 0x01U,    /*!< Peripheral Initialized and ready for use */
  HAL_USART_STATE_BUSY              = 0x02U,    /*!< an internal process is ongoing */   
  HAL_USART_STATE_BUSY_TX           = 0x12U,    /*!< Data Transmission process is ongoing */ 
  HAL_USART_STATE_BUSY_RX           = 0x22U,    /*!< Data Reception process is ongoing */
  HAL_USART_STATE_BUSY_TX_RX        = 0x32U,    /*!< Data Transmission Reception process is ongoing */
  HAL_USART_STATE_TIMEOUT           = 0x03U,    /*!< Timeout state */
  HAL_USART_STATE_ERROR             = 0x04U     /*!< Error */      
}HAL_USART_StateTypeDef;
/**
  * @}
  */
/** @defgroup USART_Error_Definition  USART error definition
  * @{
  */
/** 
  * @brief  HAL USART Error Code  definition
  */ 

#define   HAL_USART_ERROR_NONE      ((uint32_t)0x00U)    /*!< No error            */
#define   HAL_USART_ERROR_PE        ((uint32_t)0x01U)    /*!< Parity error        */
#define   HAL_USART_ERROR_NE        ((uint32_t)0x02U)    /*!< Noise error         */
#define   HAL_USART_ERROR_FE        ((uint32_t)0x04U)    /*!< frame error         */
#define   HAL_USART_ERROR_ORE       ((uint32_t)0x08U)    /*!< Overrun error       */
#define   HAL_USART_ERROR_DMA       ((uint32_t)0x10U)    /*!< DMA transfer error  */

/**
  * @}
  */
/** @defgroup USART_Clock_SourceDefinition  USART clock source definition
  * @{
  */
/** 
  * @brief  USART clock sources definitions
  */
typedef enum
{
  USART_CLOCKSOURCE_PCLK1      = 0x00U,    /*!< PCLK1 clock source    */
  USART_CLOCKSOURCE_PCLK2      = 0x01U,    /*!< PCLK2 clock source    */
  USART_CLOCKSOURCE_HSI        = 0x02U,    /*!< HSI clock source      */
  USART_CLOCKSOURCE_SYSCLK     = 0x04U,    /*!< SYSCLK clock source   */
  USART_CLOCKSOURCE_LSE        = 0x08U,    /*!< LSE clock source      */
  USART_CLOCKSOURCE_UNDEFINED  = 0x10U     /*!< Undefined clock source */
}USART_ClockSourceTypeDef;
/**
  * @}
  */
/** @defgroup USART_handle_Definition  Handle structure definition
  * @{
  */
/** 
  * @brief  USART handle Structure definition  
  */

typedef struct
{
  USART_TypeDef                 *Instance;        /*!<  USART registers base address        */

  USART_InitTypeDef             Init;             /*!<  Usart communication parameters      */
  
  uint8_t                       *pTxBuffPtr;      /*!<  Pointer to Usart Tx transfer Buffer */
  
  uint16_t                      TxXferSize;       /*!<  Usart Tx Transfer size              */
  
  __IO uint16_t                 TxXferCount;      /*!<  Usart Tx Transfer Counter           */
  
  uint8_t                       *pRxBuffPtr;      /*!<  Pointer to Usart Rx transfer Buffer */
  
  uint16_t                      RxXferSize;       /*!<  Usart Rx Transfer size              */
  
  __IO uint16_t                 RxXferCount;      /*!<  Usart Rx Transfer Counter           */

  uint16_t                      Mask;             /* USART Rx RDR register mask             */  
  
  DMA_HandleTypeDef             *hdmatx;          /*!<  Usart Tx DMA Handle parameters      */
    
  DMA_HandleTypeDef             *hdmarx;          /*!<  Usart Rx DMA Handle parameters      */
  
  HAL_LockTypeDef                Lock;            /*!<  Locking object                      */
  
  __IO HAL_USART_StateTypeDef    State;           /*!<  Usart communication state           */
  
  __IO uint32_t                  ErrorCode;       /*!<  USART Error code                    */
  
}USART_HandleTypeDef;
/**
  * @}
  */
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup USART_Exported_Constants USART Exported Constants
  * @{
  */

/** @defgroup USART_Stop_Bits USART stop bit definition
  * @{
  */
#define USART_STOPBITS_1                     ((uint32_t)0x0000U)
#define USART_STOPBITS_2                     ((uint32_t)USART_CR2_STOP_1)
#define USART_STOPBITS_1_5                   ((uint32_t)(USART_CR2_STOP_0 | USART_CR2_STOP_1))
#define IS_USART_STOPBITS(STOPBITS) (((STOPBITS) == USART_STOPBITS_1) || \
                                         ((STOPBITS) == USART_STOPBITS_1_5) || \
                                         ((STOPBITS) == USART_STOPBITS_2))
/**
  * @}
  */ 

/** @defgroup USART_Parity USART parity definition
  * @{
  */ 
#define USART_PARITY_NONE                    ((uint32_t)0x0000U)
#define USART_PARITY_EVEN                    ((uint32_t)USART_CR1_PCE)
#define USART_PARITY_ODD                     ((uint32_t)(USART_CR1_PCE | USART_CR1_PS)) 
#define IS_USART_PARITY(PARITY) (((PARITY) == USART_PARITY_NONE) || \
                                     ((PARITY) == USART_PARITY_EVEN) || \
                                     ((PARITY) == USART_PARITY_ODD))
/**
  * @}
  */ 

/** @defgroup USART_Mode USART mode definition
  * @{
  */ 
#define USART_MODE_RX                        ((uint32_t)USART_CR1_RE)
#define USART_MODE_TX                        ((uint32_t)USART_CR1_TE)
#define USART_MODE_TX_RX                     ((uint32_t)(USART_CR1_TE |USART_CR1_RE))
#define IS_USART_MODE(MODE)     (((MODE) == USART_MODE_RX) || \
                                 ((MODE) == USART_MODE_TX) || \
                                 ((MODE) == USART_MODE_TX_RX))
/**
  * @}
  */
    
/** @defgroup USART_Clock USART clock activation definition
  * @{
  */ 
#define USART_CLOCK_DISABLE                 ((uint32_t)0x0000U)
#define USART_CLOCK_ENABLE                  ((uint32_t)USART_CR2_CLKEN)
#define IS_USART_CLOCK(CLOCK)      (((CLOCK) == USART_CLOCK_DISABLE) || \
                                   ((CLOCK) == USART_CLOCK_ENABLE))
/**
  * @}
  */ 

/** @defgroup USART_Clock_Polarity USART polarity level definition
  * @{
  */
#define USART_POLARITY_LOW                   ((uint32_t)0x0000U)
#define USART_POLARITY_HIGH                  ((uint32_t)USART_CR2_CPOL)
#define IS_USART_POLARITY(CPOL) (((CPOL) == USART_POLARITY_LOW) || ((CPOL) == USART_POLARITY_HIGH))
/**
  * @}
  */ 

/** @defgroup USART_Clock_Phase USART clock phase definition
  * @{
  */
#define USART_PHASE_1EDGE                    ((uint32_t)0x0000U)
#define USART_PHASE_2EDGE                    ((uint32_t)USART_CR2_CPHA)
#define IS_USART_PHASE(CPHA) (((CPHA) == USART_PHASE_1EDGE) || ((CPHA) == USART_PHASE_2EDGE))
/**
  * @}
  */

/** @defgroup USART_Last_Bit USART last bit activation definition
  * @{
  */
#define USART_LASTBIT_DISABLE                ((uint32_t)0x0000U)
#define USART_LASTBIT_ENABLE                 ((uint32_t)USART_CR2_LBCL)
#define IS_USART_LASTBIT(LASTBIT) (((LASTBIT) == USART_LASTBIT_DISABLE) || \
                                       ((LASTBIT) == USART_LASTBIT_ENABLE))
/**
  * @}
  */


/** @defgroup USART_Flags USART flag definitions
  *        Elements values convention: 0xXXXX
  *           - 0xXXXX  : Flag mask in the ISR register
  * @{
  */
#define USART_FLAG_REACK                     USART_ISR_REACK /*!< Receive Enable Acknowledge Flag */
#define USART_FLAG_TEACK                     USART_ISR_TEACK /*!< Transmit Enable Acknowledge Flag */
#define USART_FLAG_BUSY                      USART_ISR_BUSY  /*!< Busy Flag */
#define USART_FLAG_CTS                       USART_ISR_CTS   /*!< CTS flag */
#define USART_FLAG_CTSIF                     USART_ISR_CTSIF /*!< CTS interrupt flag */
#define USART_FLAG_LBDF                      USART_ISR_LBDF  /*!< LIN Break Detection Flag */
#define USART_FLAG_TXE                       USART_ISR_TXE   /*!< Transmit Data Register Empty */
#define USART_FLAG_TC                        USART_ISR_TC    /*!< Transmission Complete */
#define USART_FLAG_RXNE                      USART_ISR_RXNE  /*!< Read Data Register Not Empty */
#define USART_FLAG_IDLE                      USART_ISR_IDLE  /*!< IDLE line detected */
#define USART_FLAG_ORE                       USART_ISR_ORE   /*!< OverRun Error */
#define USART_FLAG_NE                        USART_ISR_NE    /*!< Noise detected Flag */
#define USART_FLAG_FE                        USART_ISR_FE    /*!< Framing Error */
#define USART_FLAG_PE                        USART_ISR_PE    /*!< Parity Error */
/**
  * @}
  */

/** @defgroup USART_Interrupt_definition USART interrupt definition
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

/** @defgroup USART_IT_CLEAR_Flags USART interrupt clear flags definition
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

/** @defgroup USART_Request_Parameters USART request parameter definition
  * @{
  */
#define USART_RXDATA_FLUSH_REQUEST        ((uint32_t)USART_RQR_RXFRQ)        /*!< Receive Data flush Request */ 
#define USART_TXDATA_FLUSH_REQUEST        ((uint32_t)USART_RQR_TXFRQ)        /*!< Transmit data flush Request */
#define IS_USART_REQUEST_PARAMETER(PARAM) (((PARAM) == USART_RXDATA_FLUSH_REQUEST) || \
                                           ((PARAM) == USART_TXDATA_FLUSH_REQUEST))   
/**
  * @}
  */

/** @defgroup USART_Interruption_Mask USART interruption mask definition
  * @{
  */  
#define USART_IT_MASK                             ((uint16_t)0x001FU)  
/**
  * @}
  */

/**
  * @}
  */
  
    
/* Exported macro ------------------------------------------------------------*/

/** @defgroup USART_Exported_Macros USART Exported Macros
  * @{
  */
/** @brief Reset USART handle state
  * @param  __HANDLE__: specifies the USART Handle.
  *         The Handle Instance which can be USART1 or USART2.
  * @retval None
  */
#define __HAL_USART_RESET_HANDLE_STATE(__HANDLE__)  ((__HANDLE__)->State = HAL_USART_STATE_RESET)

/** @brief  Flush the USART Data registers
  * @param  __HANDLE__: specifies the USART Handle.
  */
#define __HAL_USART_FLUSH_DRREGISTER(__HANDLE__)  \
  do{                \
      SET_BIT((__HANDLE__)->Instance->RQR, USART_RXDATA_FLUSH_REQUEST); \
      SET_BIT((__HANDLE__)->Instance->RQR, USART_TXDATA_FLUSH_REQUEST); \
    } while(0)



/** @brief  Checks whether the specified USART flag is set or not.
  * @param  __HANDLE__: specifies the USART Handle which can be USART1 or USART2.
  * @param  __FLAG__: specifies the flag to check.
  *        This parameter can be one of the following values:
  *            @arg USART_FLAG_REACK: Receive enable ackowledge flag
  *            @arg USART_FLAG_TEACK: Transmit enable ackowledge flag
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

/** @brief  Clears the specified USART pending flag.
  * @param  __HANDLE__: specifies the USART Handle.
  * @param  __FLAG__: specifies the flag to check.
  *          This parameter can be any combination of the following values:
  *            @arg USART_CLEAR_PEF
  *            @arg USART_CLEAR_FEF
  *            @arg USART_CLEAR_NEF
  *            @arg USART_CLEAR_OREF
  *            @arg USART_CLEAR_IDLEF
  *            @arg USART_CLEAR_TCF
  *            @arg USART_CLEAR_LBDF
  *            @arg USART_CLEAR_CTSF
  *            @arg USART_CLEAR_RTOF
  *            @arg USART_CLEAR_EOBF
  *            @arg USART_CLEAR_CMF
  *            @arg USART_CLEAR_WUF
  * @retval None
  */
#define __HAL_USART_CLEAR_FLAG(__HANDLE__, __FLAG__) ((__HANDLE__)->Instance->ICR = (__FLAG__))

/** @brief  Clear the USART PE pending flag.
  * @param  __HANDLE__: specifies the UART Handle.
  * @retval None
  */
#define __HAL_USART_CLEAR_PEFLAG(__HANDLE__)   __HAL_USART_CLEAR_FLAG(__HANDLE__,USART_CLEAR_PEF)

/** @brief  Clear the USART FE pending flag.
  * @param  __HANDLE__: specifies the UART Handle.
  * @retval None
  */
#define __HAL_USART_CLEAR_FEFLAG(__HANDLE__)   __HAL_USART_CLEAR_FLAG(__HANDLE__,USART_CLEAR_FEF)

/** @brief  Clear the UART NE pending flag.
  * @param  __HANDLE__: specifies the UART Handle.
  * @retval None
  */
#define __HAL_USART_CLEAR_NEFLAG(__HANDLE__)  __HAL_USART_CLEAR_FLAG(__HANDLE__,USART_CLEAR_NEF)

/** @brief  Clear the UART ORE pending flag.
  * @param  __HANDLE__: specifies the UART Handle.
  * @retval None
  */
#define __HAL_USART_CLEAR_OREFLAG(__HANDLE__)   __HAL_USART_CLEAR_FLAG(__HANDLE__,USART_CLEAR_OREF)

/** @brief  Clear the UART IDLE pending flag.
  * @param  __HANDLE__: specifies the UART Handle.
  * @retval None
  */
#define __HAL_USART_CLEAR_IDLEFLAG(__HANDLE__)   __HAL_USART_CLEAR_FLAG(__HANDLE__,USART_CLEAR_IDLEF)

/** @brief  Enables the specified USART interrupt.
  * @param  __HANDLE__: specifies the USART Handle which can be USART1 or USART2.
  * @param  __INTERRUPT__: specifies the USART interrupt source to enable.
  *          This parameter can be one of the following values:
  *            @arg USART_IT_TXE:  Transmit Data Register empty interrupt
  *            @arg USART_IT_TC:   Transmission complete interrupt
  *            @arg USART_IT_RXNE: Receive Data register not empty interrupt
  *            @arg USART_IT_IDLE: Idle line detection interrupt
  *            @arg USART_IT_PE:   Parity Error interrupt
  *            @arg USART_IT_ERR:  Error interrupt(Frame error, noise error, overrun error)
  * @retval None
  */
#define __HAL_USART_ENABLE_IT(__HANDLE__, __INTERRUPT__)(((((uint8_t)(__INTERRUPT__)) >> 5U) == 1U)? ((__HANDLE__)->Instance->CR1 |= (1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                        ((((uint8_t)(__INTERRUPT__)) >> 5U) == 2U)? ((__HANDLE__)->Instance->CR2 |= (1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                        ((__HANDLE__)->Instance->CR3 |= (1U << ((__INTERRUPT__) & USART_IT_MASK))))

/** @brief  Disables the specified USART interrupt.
  * @param  __HANDLE__: specifies the USART Handle which can be USART1 or USART2.
  * @param  __INTERRUPT__: specifies the USART interrupt source to disable.
  *          This parameter can be one of the following values:
  *            @arg USART_IT_TXE:  Transmit Data Register empty interrupt
  *            @arg USART_IT_TC:   Transmission complete interrupt
  *            @arg USART_IT_RXNE: Receive Data register not empty interrupt
  *            @arg USART_IT_IDLE: Idle line detection interrupt
  *            @arg USART_IT_PE:   Parity Error interrupt
  *            @arg USART_IT_ERR:  Error interrupt(Frame error, noise error, overrun error)
  * @retval None
  */
#define __HAL_USART_DISABLE_IT(__HANDLE__, __INTERRUPT__)  (((((uint8_t)(__INTERRUPT__)) >> 5U) == 1U)? ((__HANDLE__)->Instance->CR1 &= ~ ((uint32_t)1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                        ((((uint8_t)(__INTERRUPT__)) >> 5U) == 2U)? ((__HANDLE__)->Instance->CR2 &= ~ ((uint32_t)1U << ((__INTERRUPT__) & USART_IT_MASK))): \
                                                        ((__HANDLE__)->Instance->CR3 &= ~ ((uint32_t)1U << ((__INTERRUPT__) & USART_IT_MASK))))

/** @brief  Checks whether the specified USART interrupt has occurred or not.
  * @param  __HANDLE__: specifies the USART Handle which can be USART1 or USART2.
  * @param  __IT__: specifies the USART interrupt source to check.
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
#define __HAL_USART_GET_IT(__HANDLE__, __IT__) ((__HANDLE__)->Instance->ISR & ((uint32_t)1U << ((__IT__)>> 0x08U))) 

/** @brief  Checks whether the specified USART interrupt source is enabled.
  * @param  __HANDLE__: specifies the USART Handle which can be USART1 or USART2.
  * @param  __IT__: specifies the USART interrupt source to check.
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
#define __HAL_USART_GET_IT_SOURCE(__HANDLE__, __IT__) ((((((uint8_t)(__IT__)) >> 5U) == 1U)? (__HANDLE__)->Instance->CR1:(((((uint8_t)(__IT__)) >> 5U) == 2U)? \
                                                   (__HANDLE__)->Instance->CR2 : (__HANDLE__)->Instance->CR3)) & ((uint32_t)1U << \
                                                   (((uint16_t)(__IT__)) & USART_IT_MASK)))


/** @brief  Clears the specified USART ISR flag, in setting the proper ICR register flag.
  * @param  __HANDLE__: specifies the USART Handle which can be USART1 or USART2.
  * @param  __IT_CLEAR__: specifies the interrupt clear register flag that needs to be set
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
  * @param  __HANDLE__: specifies the USART Handle which can be USART1 or USART2.
  * @param  __REQ__: specifies the request flag to set
  *          This parameter can be one of the following values:
  *            @arg USART_RXDATA_FLUSH_REQUEST: Receive Data flush Request 
  *            @arg USART_TXDATA_FLUSH_REQUEST: Transmit data flush Request 
  *
  * @retval None
  */ 
#define __HAL_USART_SEND_REQ(__HANDLE__, __REQ__) ((__HANDLE__)->Instance->RQR |= (uint32_t)(__REQ__))

/** @brief  Enables the USART one bit sample method
  * @param  __HANDLE__: specifies the USART Handle.
  * @retval None
  */
#define __HAL_USART_ONE_BIT_SAMPLE_ENABLE(__HANDLE__) ((__HANDLE__)->Instance->CR3|= USART_CR3_ONEBIT)

/** @brief  Disables the UART one bit sample method
  * @param  __HANDLE__: specifies the UART Handle.
  * @retval None
  */
#define __HAL_USART_ONE_BIT_SAMPLE_DISABLE(__HANDLE__) ((__HANDLE__)->Instance->CR3 &= (uint32_t)~((uint32_t)USART_CR3_ONEBIT))

/** @brief  Enable USART
  * @param  __HANDLE__: specifies the USART Handle.
  *         The Handle Instance which can be USART1 or USART2.
  * @retval None
  */ 
#define __HAL_USART_ENABLE(__HANDLE__)               ((__HANDLE__)->Instance->CR1 |=  USART_CR1_UE)

/** @brief  Disable USART
  * @param  __HANDLE__: specifies the USART Handle.
  *         The Handle Instance which can be USART1 or USART2.
  * @retval None
  */ 
#define __HAL_USART_DISABLE(__HANDLE__)              ((__HANDLE__)->Instance->CR1 &=  ~USART_CR1_UE)
  

/** @brief  Check USART Baud rate
  * @param  BAUDRATE: Baudrate specified by the user
  *         The maximum Baud Rate is derived from the maximum clock on L0 (i.e. 32 MHz) 
  *         divided by the smallest oversampling used on the USART (i.e. 8) 
  * @retval Test result (TRUE or FALSE).
  */
#define IS_USART_BAUDRATE(BAUDRATE) ((BAUDRATE) < 4000001U)

/**
  * @}
  */
      
/* Include USART HAL Extension module */
#include "stm32l0xx_hal_usart_ex.h"
/* Exported functions --------------------------------------------------------*/
/** @defgroup USART_Exported_Functions USART Exported Functions
  * @{
  */
/* Initialization/de-initialization functions  ********************************/
/** @defgroup USART_Exported_Functions_Group1 Initialization/de-initialization functions
 *  @{
 */
HAL_StatusTypeDef HAL_USART_Init(USART_HandleTypeDef *husart);
HAL_StatusTypeDef HAL_USART_DeInit(USART_HandleTypeDef *husart);
void HAL_USART_MspInit(USART_HandleTypeDef *husart);
void HAL_USART_MspDeInit(USART_HandleTypeDef *husart);
/**
  * @}
  */
/* IO operation functions *****************************************************/
/** @defgroup USART_Exported_Functions_Group2 IO operation functions
 *  @{
 */
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
void HAL_USART_TxCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_TxHalfCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_RxCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_RxHalfCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_TxRxCpltCallback(USART_HandleTypeDef *husart);
void HAL_USART_ErrorCallback(USART_HandleTypeDef *husart);
/**
  * @}
  */
/* IO operation functions *****************************************************/
/** @defgroup USART_Exported_Functions_Group3 Peripheral State functions
 *  @{
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

/* Define the private group ***********************************/
/**************************************************************/
/** @defgroup USART_Private USART Private
  * @{
  */
/**
  * @}
  */
/**************************************************************/

/**
  * @}
  */ 

/**
  * @}
  */
  
#ifdef __cplusplus
}
#endif

#endif /* __STM32L0xx_HAL_USART_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

