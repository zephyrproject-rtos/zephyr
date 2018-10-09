/**
  ******************************************************************************
  * @file    stm32f0xx_hal_usart_ex.h
  * @author  MCD Application Team
  * @brief   Header file of USART HAL Extended module.
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
#ifndef __STM32F0xx_HAL_USART_EX_H
#define __STM32F0xx_HAL_USART_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal_def.h"

/** @addtogroup STM32F0xx_HAL_Driver
  * @{
  */

/** @addtogroup USARTEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup USARTEx_Exported_Constants USARTEx Exported Constants
  * @{
  */

/** @defgroup USARTEx_Word_Length USARTEx Word Length
  * @{
  */
#if defined (STM32F042x6) || defined (STM32F048xx) || defined (STM32F070x6) || \
    defined (STM32F071xB) || defined (STM32F072xB) || defined (STM32F078xx) || defined (STM32F070xB) || \
    defined (STM32F091xC) || defined (STM32F098xx) || defined (STM32F030xC)
#define USART_WORDLENGTH_7B                  ((uint32_t)USART_CR1_M1)   /*!< 7-bit long USART frame */
#define USART_WORDLENGTH_8B                  (0x00000000U)              /*!< 8-bit long USART frame */
#define USART_WORDLENGTH_9B                  ((uint32_t)USART_CR1_M0)   /*!< 9-bit long USART frame */
#else
#define USART_WORDLENGTH_8B                  (0x00000000U)              /*!< 8-bit long USART frame */
#define USART_WORDLENGTH_9B                  ((uint32_t)USART_CR1_M)    /*!< 9-bit long USART frame */
#endif /* defined (STM32F042x6) || defined (STM32F048xx) || defined (STM32F070x6) || defined (STM32F070xB) || \
          defined (STM32F071xB) || defined (STM32F072xB) || defined (STM32F078xx) || \
          defined (STM32F091xC) || defined (STM32F098xx) || defined (STM32F030xC) */
/**
  * @}
  */

/** @defgroup USART_Request_Parameters USARTEx Request Parameters
  * @{
  */
#define USART_RXDATA_FLUSH_REQUEST        ((uint32_t)USART_RQR_RXFRQ)        /*!< Receive Data flush Request  */
#if !defined(STM32F030x6) && !defined(STM32F030x8) && !defined(STM32F070x6)  && !defined(STM32F070xB)  && !defined(STM32F030xC)
#define USART_TXDATA_FLUSH_REQUEST        ((uint32_t)USART_RQR_TXFRQ)        /*!< Transmit data flush Request */
#else
#endif /* !defined(STM32F030x6) && !defined(STM32F030x8) && !defined(STM32F070x6)  && !defined(STM32F070xB)  && !defined(STM32F030xC) */
/**
  * @}
  */

/** @defgroup USART_Flags      USART Flags
  *        Elements values convention: 0xXXXX
  *           - 0xXXXX  : Flag mask in the ISR register
  * @{
  */
#if !defined(STM32F030x6) && !defined(STM32F030x8) && !defined(STM32F070x6)  && !defined(STM32F070xB)  && !defined(STM32F030xC)
#define USART_FLAG_REACK                     (0x00400000U)    /*!< USART receive enable acknowledge flag  */
#endif /* !defined(STM32F030x6) && !defined(STM32F030x8) && !defined(STM32F070x6)  && !defined(STM32F070xB)  && !defined(STM32F030xC) */
#define USART_FLAG_TEACK                     (0x00200000U)    /*!< USART transmit enable acknowledge flag */
#define USART_FLAG_BUSY                      (0x00010000U)    /*!< USART busy flag                        */
#define USART_FLAG_CTS                       (0x00000400U)    /*!< USART clear to send flag               */
#define USART_FLAG_CTSIF                     (0x00000200U)    /*!< USART clear to send interrupt flag     */
#define USART_FLAG_TXE                       (0x00000080U)    /*!< USART transmit data register empty     */
#define USART_FLAG_TC                        (0x00000040U)    /*!< USART transmission complete            */
#define USART_FLAG_RXNE                      (0x00000020U)    /*!< USART read data register not empty     */
#define USART_FLAG_IDLE                      (0x00000010U)    /*!< USART idle flag                        */
#define USART_FLAG_ORE                       (0x00000008U)    /*!< USART overrun error                    */
#define USART_FLAG_NE                        (0x00000004U)    /*!< USART noise error                      */
#define USART_FLAG_FE                        (0x00000002U)    /*!< USART frame error                      */
#define USART_FLAG_PE                        (0x00000001U)    /*!< USART parity error                     */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros ------------------------------------------------------------*/
/** @defgroup USARTEx_Exported_Macros USARTEx Exported Macros
  * @{
  */

/** @brief  Flush the USART Data registers.
  * @param  __HANDLE__ specifies the USART Handle.
  * @retval None
  */
#if !defined(STM32F030x6) && !defined(STM32F030x8) && !defined(STM32F070x6)  && !defined(STM32F070xB)  && !defined(STM32F030xC)
#define __HAL_USART_FLUSH_DRREGISTER(__HANDLE__)  \
  do{                \
      SET_BIT((__HANDLE__)->Instance->RQR, USART_RXDATA_FLUSH_REQUEST); \
      SET_BIT((__HANDLE__)->Instance->RQR, USART_TXDATA_FLUSH_REQUEST); \
    }  while(0)
#else
#define __HAL_USART_FLUSH_DRREGISTER(__HANDLE__)  \
  do{                \
      SET_BIT((__HANDLE__)->Instance->RQR, USART_RXDATA_FLUSH_REQUEST); \
    }  while(0)
#endif /* !defined(STM32F030x6) && !defined(STM32F030x8) && !defined(STM32F070x6)  && !defined(STM32F070xB)  && !defined(STM32F030xC) */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup USARTEx_Private_Macros USARTEx Private Macros
  * @{
  */

/** @brief  Report the USART clock source.
  * @param  __HANDLE__ specifies the USART Handle.
  * @param  __CLOCKSOURCE__ output variable.
  * @retval the USART clocking source, written in __CLOCKSOURCE__.
  */
#if defined(STM32F030x6) || defined(STM32F031x6) || defined(STM32F038xx)
#define USART_GETCLOCKSOURCE(__HANDLE__,__CLOCKSOURCE__) \
  do {                                                         \
     switch(__HAL_RCC_GET_USART1_SOURCE())                     \
     {                                                         \
      case RCC_USART1CLKSOURCE_PCLK1:                          \
        (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;           \
        break;                                                 \
      case RCC_USART1CLKSOURCE_HSI:                            \
        (__CLOCKSOURCE__) = USART_CLOCKSOURCE_HSI;             \
        break;                                                 \
      case RCC_USART1CLKSOURCE_SYSCLK:                         \
        (__CLOCKSOURCE__) = USART_CLOCKSOURCE_SYSCLK;          \
        break;                                                 \
      case RCC_USART1CLKSOURCE_LSE:                            \
        (__CLOCKSOURCE__) = USART_CLOCKSOURCE_LSE;             \
        break;                                                 \
      default:                                                 \
        (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;       \
        break;                                                 \
     }                                                         \
  } while(0)
#elif defined (STM32F030x8) || defined (STM32F070x6) ||        \
      defined (STM32F042x6) || defined (STM32F048xx) ||        \
      defined (STM32F051x8) || defined (STM32F058xx)
#define USART_GETCLOCKSOURCE(__HANDLE__,__CLOCKSOURCE__) \
  do {                                                         \
    if((__HANDLE__)->Instance == USART1)                       \
    {                                                          \
       switch(__HAL_RCC_GET_USART1_SOURCE())                   \
       {                                                       \
        case RCC_USART1CLKSOURCE_PCLK1:                        \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;         \
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
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;     \
          break;                                               \
       }                                                       \
    }                                                          \
    else if((__HANDLE__)->Instance == USART2)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else                                                       \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;         \
    }                                                          \
  } while(0)
#elif defined (STM32F070xB)
#define USART_GETCLOCKSOURCE(__HANDLE__,__CLOCKSOURCE__) \
  do {                                                         \
    if((__HANDLE__)->Instance == USART1)                       \
    {                                                          \
       switch(__HAL_RCC_GET_USART1_SOURCE())                   \
       {                                                       \
        case RCC_USART1CLKSOURCE_PCLK1:                        \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;         \
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
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;     \
          break;                                               \
       }                                                       \
    }                                                          \
    else if((__HANDLE__)->Instance == USART2)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else if((__HANDLE__)->Instance == USART3)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else if((__HANDLE__)->Instance == USART4)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else                                                       \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;         \
    }                                                          \
  } while(0)
#elif defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)
#define USART_GETCLOCKSOURCE(__HANDLE__,__CLOCKSOURCE__) \
  do {                                                         \
    if((__HANDLE__)->Instance == USART1)                       \
    {                                                          \
       switch(__HAL_RCC_GET_USART1_SOURCE())                   \
       {                                                       \
        case RCC_USART1CLKSOURCE_PCLK1:                        \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;         \
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
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;     \
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
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;     \
          break;                                               \
       }                                                       \
    }                                                          \
    else if((__HANDLE__)->Instance == USART3)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else if((__HANDLE__)->Instance == USART4)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else                                                       \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;         \
    }                                                          \
  } while(0)
#elif defined(STM32F091xC) || defined (STM32F098xx)
#define USART_GETCLOCKSOURCE(__HANDLE__,__CLOCKSOURCE__) \
  do {                                                         \
    if((__HANDLE__)->Instance == USART1)                       \
    {                                                          \
       switch(__HAL_RCC_GET_USART1_SOURCE())                   \
       {                                                       \
        case RCC_USART1CLKSOURCE_PCLK1:                        \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;         \
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
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;     \
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
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;     \
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
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;     \
          break;                                               \
       }                                                       \
    }                                                          \
    else if((__HANDLE__)->Instance == USART4)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else if((__HANDLE__)->Instance == USART5)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else if((__HANDLE__)->Instance == USART6)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else if((__HANDLE__)->Instance == USART7)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else if((__HANDLE__)->Instance == USART8)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else                                                       \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;         \
    }                                                          \
  } while(0)
#elif defined(STM32F030xC)
#define USART_GETCLOCKSOURCE(__HANDLE__,__CLOCKSOURCE__) \
  do {                                                         \
    if((__HANDLE__)->Instance == USART1)                       \
    {                                                          \
       switch(__HAL_RCC_GET_USART1_SOURCE())                   \
       {                                                       \
        case RCC_USART1CLKSOURCE_PCLK1:                        \
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;         \
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
          (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;     \
          break;                                               \
       }                                                       \
    }                                                          \
    else if((__HANDLE__)->Instance == USART2)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else if((__HANDLE__)->Instance == USART3)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else if((__HANDLE__)->Instance == USART4)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else if((__HANDLE__)->Instance == USART5)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else if((__HANDLE__)->Instance == USART6)                  \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_PCLK1;             \
    }                                                          \
    else                                                       \
    {                                                          \
      (__CLOCKSOURCE__) = USART_CLOCKSOURCE_UNDEFINED;         \
    }                                                          \
  } while(0)
#endif /* defined(STM32F030x6) || defined(STM32F031x6) || defined(STM32F038xx) */


/** @brief  Compute the USART mask to apply to retrieve the received data
  *         according to the word length and to the parity bits activation.
  * @note   If PCE = 1, the parity bit is not included in the data extracted
  *         by the reception API().
  *         This masking operation is not carried out in the case of
  *         DMA transfers.
  * @param  __HANDLE__ specifies the USART Handle.
  * @retval None, the mask to apply to USART RDR register is stored in (__HANDLE__)->Mask field.
  */
#if defined (STM32F042x6) || defined (STM32F048xx) || defined (STM32F070x6) || \
    defined (STM32F071xB) || defined (STM32F072xB) || defined (STM32F078xx) || defined (STM32F070xB) || \
    defined (STM32F091xC) || defined (STM32F098xx) || defined (STM32F030xC)
#define USART_MASK_COMPUTATION(__HANDLE__)                            \
  do {                                                                \
  if ((__HANDLE__)->Init.WordLength == USART_WORDLENGTH_9B)           \
  {                                                                   \
     if ((__HANDLE__)->Init.Parity == USART_PARITY_NONE)              \
     {                                                                \
        (__HANDLE__)->Mask = 0x01FFU;                                 \
     }                                                                \
     else                                                             \
     {                                                                \
        (__HANDLE__)->Mask = 0x00FFU;                                 \
     }                                                                \
  }                                                                   \
  else if ((__HANDLE__)->Init.WordLength == USART_WORDLENGTH_8B)      \
  {                                                                   \
     if ((__HANDLE__)->Init.Parity == USART_PARITY_NONE)              \
     {                                                                \
        (__HANDLE__)->Mask = 0x00FFU;                                 \
     }                                                                \
     else                                                             \
     {                                                                \
        (__HANDLE__)->Mask = 0x007FU;                                 \
     }                                                                \
  }                                                                   \
  else if ((__HANDLE__)->Init.WordLength == USART_WORDLENGTH_7B)      \
  {                                                                   \
     if ((__HANDLE__)->Init.Parity == USART_PARITY_NONE)              \
     {                                                                \
        (__HANDLE__)->Mask = 0x007FU;                                 \
     }                                                                \
     else                                                             \
     {                                                                \
        (__HANDLE__)->Mask = 0x003FU;                                 \
     }                                                                \
  }                                                                   \
} while(0)
#else
#define USART_MASK_COMPUTATION(__HANDLE__)                            \
  do {                                                                \
  if ((__HANDLE__)->Init.WordLength == USART_WORDLENGTH_9B)           \
  {                                                                   \
     if ((__HANDLE__)->Init.Parity == USART_PARITY_NONE)              \
     {                                                                \
        (__HANDLE__)->Mask = 0x01FFU;                                 \
     }                                                                \
     else                                                             \
     {                                                                \
        (__HANDLE__)->Mask = 0x00FFU;                                 \
     }                                                                \
  }                                                                   \
  else if ((__HANDLE__)->Init.WordLength == USART_WORDLENGTH_8B)      \
  {                                                                   \
     if ((__HANDLE__)->Init.Parity == USART_PARITY_NONE)              \
     {                                                                \
        (__HANDLE__)->Mask = 0x00FFU;                                 \
     }                                                                \
     else                                                             \
     {                                                                \
        (__HANDLE__)->Mask = 0x007FU;                                 \
     }                                                                \
  }                                                                   \
} while(0)
#endif /* defined (STM32F042x6) || defined (STM32F048xx) || defined (STM32F070x6) || \
          defined (STM32F071xB) || defined (STM32F072xB) || defined (STM32F078xx) || defined (STM32F070xB) || \
          defined (STM32F091xC) || defined (STM32F098xx) || defined (STM32F030xC) */


/**
  * @brief Ensure that USART frame length is valid.
  * @param __LENGTH__ USART frame length.
  * @retval SET (__LENGTH__ is valid) or RESET (__LENGTH__ is invalid)
  */
#if defined (STM32F042x6) || defined (STM32F048xx) || defined (STM32F070x6) || \
    defined (STM32F071xB) || defined (STM32F072xB) || defined (STM32F078xx) || defined (STM32F070xB) || \
    defined (STM32F091xC) || defined (STM32F098xx) || defined (STM32F030xC)
#define IS_USART_WORD_LENGTH(__LENGTH__) (((__LENGTH__) == USART_WORDLENGTH_7B) || \
                                          ((__LENGTH__) == USART_WORDLENGTH_8B) || \
                                          ((__LENGTH__) == USART_WORDLENGTH_9B))
#else
#define IS_USART_WORD_LENGTH(__LENGTH__) (((__LENGTH__) == USART_WORDLENGTH_8B) || \
                                          ((__LENGTH__) == USART_WORDLENGTH_9B))
#endif /* defined (STM32F042x6) || defined (STM32F048xx) || defined (STM32F070x6) || defined (STM32F070xB) || \
          defined (STM32F071xB) || defined (STM32F072xB) || defined (STM32F078xx) || \
          defined (STM32F091xC) || defined (STM32F098xx) || defined (STM32F030xC) */

/**
  * @brief Ensure that USART request parameter is valid.
  * @param __PARAM__ USART request parameter.
  * @retval SET (__PARAM__ is valid) or RESET (__PARAM__ is invalid)
  */
#if !defined(STM32F030x6) && !defined(STM32F030x8) && !defined(STM32F070x6)  && !defined(STM32F070xB)  && !defined(STM32F030xC)
#define IS_USART_REQUEST_PARAMETER(__PARAM__) (((__PARAM__) == USART_RXDATA_FLUSH_REQUEST) || \
                                               ((__PARAM__) == USART_TXDATA_FLUSH_REQUEST))
#else
#define IS_USART_REQUEST_PARAMETER(__PARAM__) ((__PARAM__) == USART_RXDATA_FLUSH_REQUEST)
#endif /* !defined(STM32F030x6) && !defined(STM32F030x8) && !defined(STM32F070x6)  && !defined(STM32F070xB)  && !defined(STM32F030xC) */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F0xx_HAL_USART_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

