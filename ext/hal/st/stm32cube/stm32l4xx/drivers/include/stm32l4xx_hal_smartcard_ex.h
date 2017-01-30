/**
  ******************************************************************************
  * @file    stm32l4xx_hal_smartcard_ex.h
  * @author  MCD Application Team
  * @version V1.6.0
  * @date    28-October-2016
  * @brief   Header file of SMARTCARD HAL Extended module.
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
#ifndef __STM32L4xx_HAL_SMARTCARD_EX_H
#define __STM32L4xx_HAL_SMARTCARD_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal_def.h"

/** @addtogroup STM32L4xx_HAL_Driver
  * @{
  */

/** @addtogroup SMARTCARDEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/  
/* Exported constants --------------------------------------------------------*/

/** @addtogroup SMARTCARDEx_Exported_Constants  SMARTCARD Extended Exported Constants
  * @{
  */
  
/** @defgroup SMARTCARDEx_Transmission_Completion_Indication SMARTCARD Transmission Completion Indication
  * @{
  */
#if defined(USART_TCBGT_SUPPORT)
#define SMARTCARD_TCBGT      SMARTCARD_IT_TCBGT /*!< SMARTCARD transmission complete before guard time */
#endif /* USART_TCBGT_SUPPORT */  
#define SMARTCARD_TC         SMARTCARD_IT_TC    /*!< SMARTCARD transmission complete (flag raised when guard time has elapsed) */
/**
  * @}
  */    
  
/** @defgroup SMARTCARDEx_Advanced_Features_Initialization_Type SMARTCARD advanced feature initialization type
  * @{
  */
#define SMARTCARD_ADVFEATURE_NO_INIT                 ((uint32_t)0x00000000)    /*!< No advanced feature initialization                  */ 
#define SMARTCARD_ADVFEATURE_TXINVERT_INIT           ((uint32_t)0x00000001)    /*!< TX pin active level inversion                       */
#define SMARTCARD_ADVFEATURE_RXINVERT_INIT           ((uint32_t)0x00000002)    /*!< RX pin active level inversion                       */
#define SMARTCARD_ADVFEATURE_DATAINVERT_INIT         ((uint32_t)0x00000004)    /*!< Binary data inversion                               */
#define SMARTCARD_ADVFEATURE_SWAP_INIT               ((uint32_t)0x00000008)    /*!< TX/RX pins swap                                     */
#define SMARTCARD_ADVFEATURE_RXOVERRUNDISABLE_INIT   ((uint32_t)0x00000010)    /*!< RX overrun disable                                  */
#define SMARTCARD_ADVFEATURE_DMADISABLEONERROR_INIT  ((uint32_t)0x00000020)    /*!< DMA disable on Reception Error                      */
#define SMARTCARD_ADVFEATURE_MSBFIRST_INIT           ((uint32_t)0x00000080)    /*!< Most significant bit sent/received first            */
#if defined(USART_TCBGT_SUPPORT)
#define SMARTCARD_ADVFEATURE_TXCOMPLETION            ((uint32_t)0x00000100)    /*!< TX completion indication before of after guard time */
#endif /* USART_TCBGT_SUPPORT */   
/**
  * @}
  */


  
  
/** @defgroup SMARTCARDEx_Flags SMARTCARD Flags
  *        Elements values convention: 0xXXXX
  *           - 0xXXXX  : Flag mask in the ISR register
  * @{
  */
#if defined(USART_TCBGT_SUPPORT)
#define SMARTCARD_FLAG_TCBGT          USART_ISR_TCBGT      /*!< SMARTCARD transmission complete before guard time completion */
#endif /* USART_TCBGT_SUPPORT */ 
#define SMARTCARD_FLAG_REACK          USART_ISR_REACK      /*!< SMARTCARD receive enable acknowledge flag  */
#define SMARTCARD_FLAG_TEACK          USART_ISR_TEACK      /*!< SMARTCARD transmit enable acknowledge flag */
#define SMARTCARD_FLAG_BUSY           USART_ISR_BUSY       /*!< SMARTCARD busy flag                        */
#define SMARTCARD_FLAG_EOBF           USART_ISR_EOBF       /*!< SMARTCARD end of block flag                */
#define SMARTCARD_FLAG_RTOF           USART_ISR_RTOF       /*!< SMARTCARD receiver timeout flag            */
#define SMARTCARD_FLAG_TXE            USART_ISR_TXE        /*!< SMARTCARD transmit data register empty     */
#define SMARTCARD_FLAG_TC             USART_ISR_TC         /*!< SMARTCARD transmission complete            */
#define SMARTCARD_FLAG_RXNE           USART_ISR_RXNE       /*!< SMARTCARD read data register not empty     */
#define SMARTCARD_FLAG_IDLE           USART_ISR_IDLE       /*!< SMARTCARD idle line detection              */
#define SMARTCARD_FLAG_ORE            USART_ISR_ORE        /*!< SMARTCARD overrun error                    */
#define SMARTCARD_FLAG_NE             USART_ISR_NE         /*!< SMARTCARD noise error                      */
#define SMARTCARD_FLAG_FE             USART_ISR_FE         /*!< SMARTCARD frame error                      */
#define SMARTCARD_FLAG_PE             USART_ISR_PE         /*!< SMARTCARD parity error                     */
/**
  * @}
  */
  
/** @defgroup SMARTCARDEx_Interrupt_definition SMARTCARD Interrupts Definition
  *        Elements values convention: 000ZZZZZ0XXYYYYYb
  *           - YYYYY  : Interrupt source position in the XX register (5 bits)
  *           - XX  : Interrupt source register (2 bits)
  *                 - 01: CR1 register
  *                 - 10: CR2 register
  *                 - 11: CR3 register
  *           - ZZZZZ  : Flag position in the ISR register(5 bits)
  * @{
  */
#define SMARTCARD_IT_PE                     ((uint16_t)0x0028)        /*!< SMARTCARD parity error interruption                 */
#define SMARTCARD_IT_TXE                    ((uint16_t)0x0727)        /*!< SMARTCARD transmit data register empty interruption */
#define SMARTCARD_IT_TC                     ((uint16_t)0x0626)        /*!< SMARTCARD transmission complete interruption        */
#define SMARTCARD_IT_RXNE                   ((uint16_t)0x0525)        /*!< SMARTCARD read data register not empty interruption */
#define SMARTCARD_IT_IDLE                   ((uint16_t)0x0424)        /*!< SMARTCARD idle line detection interruption          */
                                                                      
#define SMARTCARD_IT_ERR                    ((uint16_t)0x0060)        /*!< SMARTCARD error interruption         */
#define SMARTCARD_IT_ORE                    ((uint16_t)0x0300)        /*!< SMARTCARD overrun error interruption */
#define SMARTCARD_IT_NE                     ((uint16_t)0x0200)        /*!< SMARTCARD noise error interruption   */
#define SMARTCARD_IT_FE                     ((uint16_t)0x0100)        /*!< SMARTCARD frame error interruption   */

#define SMARTCARD_IT_EOB                    ((uint16_t)0x0C3B)        /*!< SMARTCARD end of block interruption     */ 
#define SMARTCARD_IT_RTO                    ((uint16_t)0x0B3A)        /*!< SMARTCARD receiver timeout interruption */

#if defined(USART_TCBGT_SUPPORT)
#define SMARTCARD_IT_TCBGT                  ((uint16_t)0x1978)        /*!< SMARTCARD transmission complete before guard time completion interruption */
#endif /* USART_TCBGT_SUPPORT */ 
/**
  * @}
  */
  
/** @defgroup SMARTCARDEx_IT_CLEAR_Flags SMARTCARD Interruption Clear Flags
  * @{
  */
#define SMARTCARD_CLEAR_PEF                 USART_ICR_PECF    /*!< SMARTCARD parity error clear flag          */
#define SMARTCARD_CLEAR_FEF                 USART_ICR_FECF    /*!< SMARTCARD framing error clear flag         */
#define SMARTCARD_CLEAR_NEF                 USART_ICR_NCF     /*!< SMARTCARD noise detected clear flag        */
#define SMARTCARD_CLEAR_OREF                USART_ICR_ORECF   /*!< SMARTCARD overrun error clear flag         */
#define SMARTCARD_CLEAR_IDLEF               USART_ICR_IDLECF  /*!< SMARTCARD idle line detected clear flag    */
#define SMARTCARD_CLEAR_TCF                 USART_ICR_TCCF    /*!< SMARTCARD transmission complete clear flag */
#if defined(USART_TCBGT_SUPPORT)
#define SMARTCARD_CLEAR_TCBGTF              USART_ICR_TCBGTCF /*!< SMARTCARD transmission complete before guard time completion clear flag */
#endif /* USART_TCBGT_SUPPORT */ 
#define SMARTCARD_CLEAR_RTOF                USART_ICR_RTOCF   /*!< SMARTCARD receiver time out clear flag     */
#define SMARTCARD_CLEAR_EOBF                USART_ICR_EOBCF   /*!< SMARTCARD end of block clear flag          */
/**
  * @}
  */  
    
/**
  * @}
  */ 
/* Exported macros -----------------------------------------------------------*/  
/* Private macros ------------------------------------------------------------*/
/** @defgroup SMARTCARDEx_Private_Macros SMARTCARD Extended Private Macros
  * @{
  */
  
/** @brief  Report the SMARTCARD clock source.
  * @param  __HANDLE__: specifies the SMARTCARD Handle.
  * @param  __CLOCKSOURCE__: output variable.
  * @retval the SMARTCARD clocking source, written in __CLOCKSOURCE__.
  */
#if defined (STM32L432xx) || defined (STM32L442xx)
#define SMARTCARD_GETCLOCKSOURCE(__HANDLE__,__CLOCKSOURCE__)   \
  do {                                                         \
    if((__HANDLE__)->Instance == USART1)                       \
    {                                                          \
       switch(__HAL_RCC_GET_USART1_SOURCE())                   \
       {                                                       \
        case RCC_USART1CLKSOURCE_PCLK2:                        \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_PCLK2;     \
          break;                                               \
        case RCC_USART1CLKSOURCE_HSI:                          \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_HSI;       \
          break;                                               \
        case RCC_USART1CLKSOURCE_SYSCLK:                       \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_SYSCLK;    \
          break;                                               \
        case RCC_USART1CLKSOURCE_LSE:                          \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_LSE;       \
          break;                                               \
        default:                                               \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_UNDEFINED; \
          break;                                               \
       }                                                       \
    }                                                          \
    else if((__HANDLE__)->Instance == USART2)                  \
    {                                                          \
       switch(__HAL_RCC_GET_USART2_SOURCE())                   \
       {                                                       \
        case RCC_USART2CLKSOURCE_PCLK1:                        \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_PCLK1;     \
          break;                                               \
        case RCC_USART2CLKSOURCE_HSI:                          \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_HSI;       \
          break;                                               \
        case RCC_USART2CLKSOURCE_SYSCLK:                       \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_SYSCLK;    \
          break;                                               \
        case RCC_USART2CLKSOURCE_LSE:                          \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_LSE;       \
          break;                                               \
        default:                                               \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_UNDEFINED; \
          break;                                               \
       }                                                       \
    }                                                          \
  } while(0)
#else
#define SMARTCARD_GETCLOCKSOURCE(__HANDLE__,__CLOCKSOURCE__)   \
  do {                                                         \
    if((__HANDLE__)->Instance == USART1)                       \
    {                                                          \
       switch(__HAL_RCC_GET_USART1_SOURCE())                   \
       {                                                       \
        case RCC_USART1CLKSOURCE_PCLK2:                        \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_PCLK2;     \
          break;                                               \
        case RCC_USART1CLKSOURCE_HSI:                          \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_HSI;       \
          break;                                               \
        case RCC_USART1CLKSOURCE_SYSCLK:                       \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_SYSCLK;    \
          break;                                               \
        case RCC_USART1CLKSOURCE_LSE:                          \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_LSE;       \
          break;                                               \
        default:                                               \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_UNDEFINED; \
          break;                                               \
       }                                                       \
    }                                                          \
    else if((__HANDLE__)->Instance == USART2)                  \
    {                                                          \
       switch(__HAL_RCC_GET_USART2_SOURCE())                   \
       {                                                       \
        case RCC_USART2CLKSOURCE_PCLK1:                        \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_PCLK1;     \
          break;                                               \
        case RCC_USART2CLKSOURCE_HSI:                          \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_HSI;       \
          break;                                               \
        case RCC_USART2CLKSOURCE_SYSCLK:                       \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_SYSCLK;    \
          break;                                               \
        case RCC_USART2CLKSOURCE_LSE:                          \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_LSE;       \
          break;                                               \
        default:                                               \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_UNDEFINED; \
          break;                                               \
       }                                                       \
    }                                                          \
    else if((__HANDLE__)->Instance == USART3)                  \
    {                                                          \
       switch(__HAL_RCC_GET_USART3_SOURCE())                   \
       {                                                       \
        case RCC_USART3CLKSOURCE_PCLK1:                        \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_PCLK1;     \
          break;                                               \
        case RCC_USART3CLKSOURCE_HSI:                          \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_HSI;       \
          break;                                               \
        case RCC_USART3CLKSOURCE_SYSCLK:                       \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_SYSCLK;    \
          break;                                               \
        case RCC_USART3CLKSOURCE_LSE:                          \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_LSE;       \
          break;                                               \
        default:                                               \
          (__CLOCKSOURCE__) = SMARTCARD_CLOCKSOURCE_UNDEFINED; \
          break;                                               \
       }                                                       \
    }                                                          \
  } while(0)
#endif /* STM32L432xx || STM32L442xx */

/** @brief  Set the Transmission Completion flag
  * @param  __HANDLE__: specifies the SMARTCARD Handle.
  * @note  If TCBGT (Transmission Complete Before Guard Time) flag is not available or if 
  *        AdvancedInit.TxCompletionIndication is not already filled, the latter is forced 
  *        to SMARTCARD_TC (transmission completion indication when guard time has elapsed).     
  * @retval None
  */
#if defined(USART_TCBGT_SUPPORT)
#define SMARTCARD_TRANSMISSION_COMPLETION_SETTING(__HANDLE__)                                                \
  do {                                                                                                       \
    if (HAL_IS_BIT_CLR((__HANDLE__)->AdvancedInit.AdvFeatureInit, SMARTCARD_ADVFEATURE_TXCOMPLETION))        \
    {                                                                                                        \
     (__HANDLE__)->AdvancedInit.TxCompletionIndication = SMARTCARD_TC;                                       \
    }                                                                                                        \
    else                                                                                                     \
    {                                                                                                        \
      assert_param(IS_SMARTCARD_TRANSMISSION_COMPLETION((__HANDLE__)->AdvancedInit.TxCompletionIndication)); \
    }                                                                                                        \
  } while(0)
#else
#define SMARTCARD_TRANSMISSION_COMPLETION_SETTING(__HANDLE__)         \
  do {                                                                \
    (__HANDLE__)->AdvancedInit.TxCompletionIndication = SMARTCARD_TC; \
  } while(0)  
#endif 

/** @brief  Return the transmission completion flag.
  * @param  __HANDLE__: specifies the SMARTCARD Handle.
  * @note  Based on AdvancedInit.TxCompletionIndication setting, return TC or TCBGT flag.
  *        When TCBGT flag (Transmission Complete Before Guard Time) is not available, TC flag is
  *        reported.       
  * @retval Transmission completion flag
  */
#if defined(USART_TCBGT_SUPPORT)
#define SMARTCARD_TRANSMISSION_COMPLETION_FLAG(__HANDLE__) \
  (((__HANDLE__)->AdvancedInit.TxCompletionIndication == SMARTCARD_TC) ? (SMARTCARD_FLAG_TC) :  (SMARTCARD_FLAG_TCBGT))
#else
#define SMARTCARD_TRANSMISSION_COMPLETION_FLAG(__HANDLE__)    (SMARTCARD_FLAG_TC)
#endif
  
/**
  * @brief Ensure that SMARTCARD frame transmission completion used flag is valid.
  * @param __TXCOMPLETE__: SMARTCARD frame transmission completion used flag. 
  * @retval SET (__TXCOMPLETE__ is valid) or RESET (__TXCOMPLETE__ is invalid)
  */ 
#if defined(USART_TCBGT_SUPPORT)
#define IS_SMARTCARD_TRANSMISSION_COMPLETION(__TXCOMPLETE__) (((__TXCOMPLETE__) == SMARTCARD_TCBGT) ||\
                                                              ((__TXCOMPLETE__) == SMARTCARD_TC))
#else
#define IS_SMARTCARD_TRANSMISSION_COMPLETION(__TXCOMPLETE__) ((__TXCOMPLETE__) == SMARTCARD_TC)
#endif

/**
  * @}
  */      

/* Exported functions --------------------------------------------------------*/
/** @addtogroup SMARTCARDEx_Exported_Functions
  * @{
  */

/* Initialization and de-initialization functions  ****************************/
/* IO operation methods *******************************************************/

/** @addtogroup SMARTCARDEx_Exported_Functions_Group1
  * @{
  */

/* Peripheral Control functions ***********************************************/
void              HAL_SMARTCARDEx_BlockLength_Config(SMARTCARD_HandleTypeDef *hsmartcard, uint8_t BlockLength);
void              HAL_SMARTCARDEx_TimeOut_Config(SMARTCARD_HandleTypeDef *hsmartcard, uint32_t TimeOutValue);
HAL_StatusTypeDef HAL_SMARTCARDEx_EnableReceiverTimeOut(SMARTCARD_HandleTypeDef *hsmartcard);
HAL_StatusTypeDef HAL_SMARTCARDEx_DisableReceiverTimeOut(SMARTCARD_HandleTypeDef *hsmartcard);

/**
  * @}
  */

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

#endif /* __STM32L4xx_HAL_SMARTCARD_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
