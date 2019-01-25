/***************************************************************************//**
 * @file em_prs.h
 * @brief Peripheral Reflex System (PRS) peripheral API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Laboratories, Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

#ifndef EM_PRS_H
#define EM_PRS_H

#include "em_device.h"
#include "em_gpio.h"

#include <stdbool.h>
#include <stddef.h>

#if defined(PRS_COUNT) && (PRS_COUNT > 0)

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup PRS
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#if defined(_SILICON_LABS_32B_SERIES_2)
  #define PRS_SYNC_CHAN_COUNT    PRS_SYNC_CH_NUM
  #define PRS_ASYNC_CHAN_COUNT   PRS_ASYNC_CH_NUM
#elif defined(_EFM32_GECKO_FAMILY)
  #define PRS_SYNC_CHAN_COUNT    PRS_CHAN_COUNT
  #define PRS_ASYNC_CHAN_COUNT   0
#else
  #define PRS_SYNC_CHAN_COUNT    PRS_CHAN_COUNT
  #define PRS_ASYNC_CHAN_COUNT   PRS_CHAN_COUNT
#endif

#if !defined(_EFM32_GECKO_FAMILY)
#define PRS_ASYNC_SUPPORTED      1
#endif

/* Some devices have renamed signals so we map some of these signals to
   common names. */
#if defined(PRS_USART0_RXDATAV)
#define PRS_USART0_RXDATA        PRS_USART0_RXDATAV
#endif
#if defined(PRS_USART1_RXDATAV)
#define PRS_USART1_RXDATA        PRS_USART1_RXDATAV
#endif
#if defined(PRS_USART2_RXDATAV)
#define PRS_USART2_RXDATA        PRS_USART2_RXDATAV
#endif
#if defined(PRS_BURTC_OVERFLOW)
#define PRS_BURTC_OF             PRS_BURTC_OVERFLOW
#endif
#if defined(PRS_BURTC_COMP0)
#define PRS_BURTC_COMP           PRS_BURTC_COMP0
#endif

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** PRS Channel type. */
typedef enum {
  prsTypeAsync,  /**< Asynchronous channel type. */
  prsTypeSync    /**< Synchronous channel type.*/
} PRS_ChType_t;

/** Edge detection type. */
typedef enum {
  prsEdgeOff,  /**< Leave signal as is. */
  prsEdgePos,  /**< Generate pulses on positive edge. */
  prsEdgeNeg,  /**< Generate pulses on negative edge. */
  prsEdgeBoth  /**< Generate pulses on both edges. */
} PRS_Edge_TypeDef;

#if defined(_PRS_ASYNC_CH_CTRL_FNSEL_MASK)
/** Logic functions that can be used when combining two PRS channels. */
typedef enum {
  prsLogic_Zero        = _PRS_ASYNC_CH_CTRL_FNSEL_LOGICAL_ZERO,
  prsLogic_A_NOR_B     = _PRS_ASYNC_CH_CTRL_FNSEL_A_NOR_B,
  prsLogic_NOT_A_AND_B = _PRS_ASYNC_CH_CTRL_FNSEL_NOT_A_AND_B,
  prsLogic_NOT_A       = _PRS_ASYNC_CH_CTRL_FNSEL_NOT_A,
  prsLogic_A_AND_NOT_B = _PRS_ASYNC_CH_CTRL_FNSEL_A_AND_NOT_B,
  prsLogic_NOT_B       = _PRS_ASYNC_CH_CTRL_FNSEL_NOT_B,
  prsLogic_A_XOR_B     = _PRS_ASYNC_CH_CTRL_FNSEL_A_XOR_B,
  prsLogic_A_NAND_B    = _PRS_ASYNC_CH_CTRL_FNSEL_A_NAND_B,
  prsLogic_A_AND_B     = _PRS_ASYNC_CH_CTRL_FNSEL_A_AND_B,
  prsLogic_A_XNOR_B    = _PRS_ASYNC_CH_CTRL_FNSEL_A_XNOR_B,
  prsLogic_B           = _PRS_ASYNC_CH_CTRL_FNSEL_B,
  prsLogic_A_OR_NOT_B  = _PRS_ASYNC_CH_CTRL_FNSEL_A_OR_NOT_B,
  prsLogic_A           = _PRS_ASYNC_CH_CTRL_FNSEL_A,
  prsLogic_NOT_A_OR_B  = _PRS_ASYNC_CH_CTRL_FNSEL_NOT_A_OR_B,
  prsLogic_A_OR_B      = _PRS_ASYNC_CH_CTRL_FNSEL_A_OR_B,
  prsLogic_One         = _PRS_ASYNC_CH_CTRL_FNSEL_LOGICAL_ONE,
} PRS_Logic_t;
#endif

/** PRS Signal. */
typedef enum {
  prsSignalNone       = 0x0,
  /* Timer Signals */
#if defined(TIMER0)
  prsSignalTIMER0_UF  = PRS_TIMER0_UF,  /**< TIMER0 underflow Signal. */
  prsSignalTIMER0_OF  = PRS_TIMER0_OF,  /**< TIMER0 overflow Signal. */
  prsSignalTIMER0_CC0 = PRS_TIMER0_CC0, /**< TIMER0 capture/compare channel 0 Signal. */
  prsSignalTIMER0_CC1 = PRS_TIMER0_CC1, /**< TIMER0 capture/compare channel 1 Signal. */
  prsSignalTIMER0_CC2 = PRS_TIMER0_CC2, /**< TIMER0 capture/compare channel 2 Signal. */
#endif
#if defined(TIMER1)
  prsSignalTIMER1_UF  = PRS_TIMER1_UF,  /**< TIMER1 underflow Signal. */
  prsSignalTIMER1_OF  = PRS_TIMER1_OF,  /**< TIMER1 overflow Signal. */
  prsSignalTIMER1_CC0 = PRS_TIMER1_CC0, /**< TIMER1 capture/compare channel 0 Signal. */
  prsSignalTIMER1_CC1 = PRS_TIMER1_CC1, /**< TIMER1 capture/compare channel 1 Signal. */
  prsSignalTIMER1_CC2 = PRS_TIMER1_CC2, /**< TIMER1 capture/compare channel 2 Signal. */
#endif
#if defined(TIMER2)
  prsSignalTIMER2_UF  = PRS_TIMER2_UF,  /**< TIMER2 underflow Signal. */
  prsSignalTIMER2_OF  = PRS_TIMER2_OF,  /**< TIMER2 overflow Signal. */
  prsSignalTIMER2_CC0 = PRS_TIMER2_CC0, /**< TIMER2 capture/compare channel 0 Signal. */
  prsSignalTIMER2_CC1 = PRS_TIMER2_CC1, /**< TIMER2 capture/compare channel 1 Signal. */
  prsSignalTIMER2_CC2 = PRS_TIMER2_CC2, /**< TIMER2 capture/compare channel 2 Signal. */
#endif
#if defined(TIMER3)
  prsSignalTIMER3_UF  = PRS_TIMER3_UF,  /**< TIMER3 underflow Signal. */
  prsSignalTIMER3_OF  = PRS_TIMER3_OF,  /**< TIMER3 overflow Signal. */
  prsSignalTIMER3_CC0 = PRS_TIMER3_CC0, /**< TIMER3 capture/compare channel 0 Signal. */
  prsSignalTIMER3_CC1 = PRS_TIMER3_CC1, /**< TIMER3 capture/compare channel 1 Signal. */
  prsSignalTIMER3_CC2 = PRS_TIMER3_CC2, /**< TIMER3 capture/compare channel 2 Signal. */
#endif
#if defined(PRS_LETIMER0_CH0)
  prsSignalLETIMER0_CH0  = PRS_LETIMER0_CH0, /**< LETIMER0 channel 0 Signal. */
  prsSignalLETIMER0_CH1  = PRS_LETIMER0_CH1, /**< LETIMER0 channel 1 Signal. */
#endif

  /* RTC/RTCC/BURTC Signals */
#if defined(RTCC)
  prsSignalRTCC_CCV0  = PRS_RTCC_CCV0, /**< RTCC capture/compare channel 0 Signal. */
  prsSignalRTCC_CCV1  = PRS_RTCC_CCV1, /**< RTCC capture/compare channel 1 Signal. */
  prsSignalRTCC_CCV2  = PRS_RTCC_CCV2, /**< RTCC capture/compare channel 2 Signal. */
#endif
#if defined(BURTC)
  prsSignalBURTC_COMP = PRS_BURTC_COMP, /**< BURTC compare Signal. */
  prsSignalBURTC_OF   = PRS_BURTC_OF,   /**< BURTC overflow Signal. */
#endif

  /* ACMP Signals */
#if defined(ACMP0)
  prsSignalACMP0_OUT     = PRS_ACMP0_OUT, /**< ACMP0 Signal. */
#endif
#if defined(ACMP1)
  prsSignalACMP1_OUT     = PRS_ACMP1_OUT, /**< ACMP1 output Signal. */
#endif

  /* USART Signals */
#if defined(USART0)
  prsSignalUSART0_IRTX    = PRS_USART0_IRTX,   /**< USART0 IR tx Signal. */
  prsSignalUSART0_TXC     = PRS_USART0_TXC,    /**< USART0 tx complete Signal. */
  prsSignalUSART0_RXDATA  = PRS_USART0_RXDATA, /**< USART0 rx data available Signal. */
#if (_SILICON_LABS_32B_SERIES > 0)
  prsSignalUSART0_RTS     = PRS_USART0_RTS,    /**< USART0 RTS Signal. */
  prsSignalUSART0_TX      = PRS_USART0_TX,     /**< USART0 tx Signal. */
  prsSignalUSART0_CS      = PRS_USART0_CS,     /**< USART0 chip select Signal. */
#endif
#endif
#if defined(USART1)
  prsSignalUSART1_TXC     = PRS_USART1_TXC,    /**< USART1 tx complete Signal. */
  prsSignalUSART1_RXDATA  = PRS_USART1_RXDATA, /**< USART1 rx data available Signal. */
#if defined(PRS_USART1_IRTX)
  prsSignalUSART1_IRTX    = PRS_USART1_IRTX,   /**< USART1 IR tx Signal. */
#endif
#if (_SILICON_LABS_32B_SERIES > 0)
  prsSignalUSART1_RTS     = PRS_USART1_RTS,    /**< USART1 RTS Signal. */
  prsSignalUSART1_TX      = PRS_USART1_TX,     /**< USART1 tx Signal. */
  prsSignalUSART1_CS      = PRS_USART1_CS,     /**< USART1 chip select Signal. */
#endif
#endif
#if defined(USART2)
  prsSignalUSART2_TXC     = PRS_USART2_TXC,    /**< USART2 tx complete Signal. */
  prsSignalUSART2_RXDATA  = PRS_USART2_RXDATA, /**< USART2 rx data available Signal. */
#if defined(PRS_USART2_IRTX)
  prsSignalUSART2_IRTX    = PRS_USART2_IRTX,   /**< USART2 IR tx Signal. */
#endif
#if (_SILICON_LABS_32B_SERIES > 0)
  prsSignalUSART2_RTS     = PRS_USART2_RTS,    /**< USART2 RTS Signal. */
  prsSignalUSART2_TX      = PRS_USART2_TX,     /**< USART2 tx Signal. */
  prsSignalUSART2_CS      = PRS_USART2_CS,     /**< USART2 chip select Signal. */
#endif
#endif

  /* ADC Signals */
#if defined(IADC0)
  prsSignalIADC0_SCANENTRY = PRS_IADC0_SCANENTRYDONE, /**< IADC0 scan entry Signal. */
  prsSignalIADC0_SCANTABLE = PRS_IADC0_SCANTABLEDONE, /**< IADC0 scan table Signal. */
  prsSignalIADC0_SINGLE    = PRS_IADC0_SINGLEDONE,    /**< IADC0 single Signal. */
#endif

  /* GPIO pin Signals */
  prsSignalGPIO_PIN0  = PRS_GPIO_PIN0, /**< GPIO Pin 0 Signal. */
  prsSignalGPIO_PIN1  = PRS_GPIO_PIN1, /**< GPIO Pin 1 Signal. */
  prsSignalGPIO_PIN2  = PRS_GPIO_PIN2, /**< GPIO Pin 2 Signal. */
  prsSignalGPIO_PIN3  = PRS_GPIO_PIN3, /**< GPIO Pin 3 Signal. */
  prsSignalGPIO_PIN4  = PRS_GPIO_PIN4, /**< GPIO Pin 4 Signal. */
  prsSignalGPIO_PIN5  = PRS_GPIO_PIN5, /**< GPIO Pin 5 Signal. */
  prsSignalGPIO_PIN6  = PRS_GPIO_PIN6, /**< GPIO Pin 6 Signal. */
  prsSignalGPIO_PIN7  = PRS_GPIO_PIN7, /**< GPIO Pin 7 Signal. */
#if defined(PRS_GPIO_PIN15)
  prsSignalGPIO_PIN8  = PRS_GPIO_PIN8,  /**< GPIO Pin 8 Signal. */
  prsSignalGPIO_PIN9  = PRS_GPIO_PIN9,  /**< GPIO Pin 9 Signal. */
  prsSignalGPIO_PIN10 = PRS_GPIO_PIN10, /**< GPIO Pin 10 Signal. */
  prsSignalGPIO_PIN11 = PRS_GPIO_PIN11, /**< GPIO Pin 11 Signal. */
  prsSignalGPIO_PIN12 = PRS_GPIO_PIN12, /**< GPIO Pin 12 Signal. */
  prsSignalGPIO_PIN13 = PRS_GPIO_PIN13, /**< GPIO Pin 13 Signal. */
  prsSignalGPIO_PIN14 = PRS_GPIO_PIN14, /**< GPIO Pin 14 Signal. */
  prsSignalGPIO_PIN15 = PRS_GPIO_PIN15, /**< GPIO Pin 15 Signal. */
#endif
} PRS_Signal_t;

#if defined(_SILICON_LABS_32B_SERIES_2)
/** PRS Consumers. */
typedef enum {
  prsConsumerNone                = 0x000,                                               /**< No PRS consumer */
  prsConsumerCMU_CALDN           = offsetof(PRS_TypeDef, CONSUMER_CMU_CALDN),           /**< CMU calibration down consumer. */
  prsConsumerCMU_CALUP           = offsetof(PRS_TypeDef, CONSUMER_CMU_CALUP),           /**< CMU calibration up consumer. */
  prsConsumerIADC0_SCANTRIGGER   = offsetof(PRS_TypeDef, CONSUMER_IADC0_SCANTRIGGER),   /**< IADC0 scan trigger consumer. */
  prsConsumerIADC0_SINGLETRIGGER = offsetof(PRS_TypeDef, CONSUMER_IADC0_SINGLETRIGGER), /**< IADC0 single trigger consumer. */
  prsConsumerLDMA_REQUEST0       = offsetof(PRS_TypeDef, CONSUMER_LDMAXBAR_DMAREQ0),    /**< LDMA Request 0 consumer. */
  prsConsumerLDMA_REQUEST1       = offsetof(PRS_TypeDef, CONSUMER_LDMAXBAR_DMAREQ1),    /**< LDMA Request 1 consumer. */
  prsConsumerLETIMER0_CLEAR      = offsetof(PRS_TypeDef, CONSUMER_LETIMER_CLEAR),       /**< LETIMER0 clear consumer. */
  prsConsumerLETIMER0_START      = offsetof(PRS_TypeDef, CONSUMER_LETIMER_START),       /**< LETIMER0 start consumer. */
  prsConsumerLETIMER0_STOP       = offsetof(PRS_TypeDef, CONSUMER_LETIMER_STOP),        /**< LETIMER0 stop consumer. */
  prsConsumerTIMER0_CC0          = offsetof(PRS_TypeDef, CONSUMER_TIMER0_CC0),          /**< TIMER0 capture/compare channel 0 consumer. */
  prsConsumerTIMER0_CC1          = offsetof(PRS_TypeDef, CONSUMER_TIMER0_CC1),          /**< TIMER0 capture/compare channel 1 consumer. */
  prsConsumerTIMER0_CC2          = offsetof(PRS_TypeDef, CONSUMER_TIMER0_CC2),          /**< TIMER0 capture/compare channel 2 consumer. */
  prsConsumerTIMER1_CC0          = offsetof(PRS_TypeDef, CONSUMER_TIMER1_CC0),          /**< TIMER1 capture/compare channel 0 consumer. */
  prsConsumerTIMER1_CC1          = offsetof(PRS_TypeDef, CONSUMER_TIMER1_CC1),          /**< TIMER1 capture/compare channel 1 consumer. */
  prsConsumerTIMER1_CC2          = offsetof(PRS_TypeDef, CONSUMER_TIMER1_CC2),          /**< TIMER1 capture/compare channel 2 consumer. */
  prsConsumerTIMER2_CC0          = offsetof(PRS_TypeDef, CONSUMER_TIMER2_CC0),          /**< TIMER2 capture/compare channel 0 consumer. */
  prsConsumerTIMER2_CC1          = offsetof(PRS_TypeDef, CONSUMER_TIMER2_CC1),          /**< TIMER2 capture/compare channel 1 consumer. */
  prsConsumerTIMER2_CC2          = offsetof(PRS_TypeDef, CONSUMER_TIMER2_CC2),          /**< TIMER2 capture/compare channel 2 consumer. */
  prsConsumerTIMER3_CC0          = offsetof(PRS_TypeDef, CONSUMER_TIMER3_CC0),          /**< TIMER3 capture/compare channel 0 consumer. */
  prsConsumerTIMER3_CC1          = offsetof(PRS_TypeDef, CONSUMER_TIMER3_CC1),          /**< TIMER3 capture/compare channel 1 consumer. */
  prsConsumerTIMER3_CC2          = offsetof(PRS_TypeDef, CONSUMER_TIMER3_CC2),          /**< TIMER3 capture/compare channel 2 consumer. */
  prsConsumerUSART0_CLK          = offsetof(PRS_TypeDef, CONSUMER_USART0_CLK),          /**< USART0 clock consumer. */
  prsConsumerUSART0_IR           = offsetof(PRS_TypeDef, CONSUMER_USART0_IR),           /**< USART0 IR consumer. */
  prsConsumerUSART0_RX           = offsetof(PRS_TypeDef, CONSUMER_USART0_RX),           /**< USART0 rx consumer. */
  prsConsumerUSART0_TRIGGER      = offsetof(PRS_TypeDef, CONSUMER_USART0_TRIGGER),      /**< USART0 trigger consumer. */
  prsConsumerUSART1_CLK          = offsetof(PRS_TypeDef, CONSUMER_USART1_CLK),          /**< USART1 clock consumer. */
  prsConsumerUSART1_IR           = offsetof(PRS_TypeDef, CONSUMER_USART1_IR),           /**< USART1 IR consumer. */
  prsConsumerUSART1_RX           = offsetof(PRS_TypeDef, CONSUMER_USART1_RX),           /**< USART1 rx consumer. */
  prsConsumerUSART1_TRIGGER      = offsetof(PRS_TypeDef, CONSUMER_USART1_TRIGGER),      /**< USART1 trigger consumer. */
  prsConsumerUSART2_CLK          = offsetof(PRS_TypeDef, CONSUMER_USART2_CLK),          /**< USART2 clock consumer. */
  prsConsumerUSART2_IR           = offsetof(PRS_TypeDef, CONSUMER_USART2_IR),           /**< USART2 IR consumer. */
  prsConsumerUSART2_RX           = offsetof(PRS_TypeDef, CONSUMER_USART2_RX),           /**< USART2 rx consumer. */
  prsConsumerUSART2_TRIGGER      = offsetof(PRS_TypeDef, CONSUMER_USART2_TRIGGER),      /**< USART2 trigger consumer. */
  prsConsumerWDOG0_SRC0          = offsetof(PRS_TypeDef, CONSUMER_WDOG0_SRC0),          /**< WDOG0 source 0 consumer. */
  prsConsumerWDOG0_SRC1          = offsetof(PRS_TypeDef, CONSUMER_WDOG0_SRC1),          /**< WDOG0 source 1 consumer. */
  prsConsumerWDOG1_SRC0          = offsetof(PRS_TypeDef, CONSUMER_WDOG1_SRC0),          /**< WDOG1 source 0 consumer. */
  prsConsumerWDOG1_SRC1          = offsetof(PRS_TypeDef, CONSUMER_WDOG1_SRC1),          /**< WDOG1 source 1 consumer. */
} PRS_Consumer_t;
#endif

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Set level control bit for one or more channels.
 *
 * @details
 *   The level value for a channel is XORed with both the pulse possibly issued
 *   by PRS_PulseTrigger() and the PRS input signal selected for the channel(s).
 *
 * @cond DOXYDOC_S2_DEVICE
 * @note
 *   Note that software level control is only available for asynchronous
 *   channels on Series 2 devices.
 * @endcond
 *
 * @param[in] level
 *   Level to use for channels indicated by @p mask. Use logical OR combination
 *   of PRS_SWLEVEL_CHnLEVEL defines for channels to set high level, otherwise 0.
 *
 * @param[in] mask
 *   Mask indicating which channels to set level for. Use logical OR combination
 *   of PRS_SWLEVEL_CHnLEVEL defines.
 ******************************************************************************/
__STATIC_INLINE void PRS_LevelSet(uint32_t level, uint32_t mask)
{
#if defined(_PRS_SWLEVEL_MASK)
  PRS->SWLEVEL = (PRS->SWLEVEL & ~mask) | (level & mask);
#else
  PRS->ASYNC_SWLEVEL = (PRS->ASYNC_SWLEVEL & ~mask) | (level & mask);
#endif
}

/***************************************************************************//**
 * @brief
 *   Get level control bit for all channels.
 *
 * @return
 *   The current software level configuration.
 ******************************************************************************/
__STATIC_INLINE uint32_t PRS_LevelGet(void)
{
#if defined(_PRS_SWLEVEL_MASK)
  return PRS->SWLEVEL;
#else
  return PRS->ASYNC_SWLEVEL;
#endif
}

#if defined(_PRS_ASYNC_PEEK_MASK) || defined(_PRS_PEEK_MASK)
/***************************************************************************//**
 * @brief
 *   Get the PRS channel values for all channels.
 *
 * @param[in] type
 *   PRS channel type. This can be either @ref prsTypeAsync or
 *   @ref prsTypeSync.
 *
 * @return
 *   The current PRS channel output values for all channels as a bitset.
 ******************************************************************************/
__STATIC_INLINE uint32_t PRS_Values(PRS_ChType_t type)
{
#if defined(_PRS_ASYNC_PEEK_MASK)
  if (type == prsTypeAsync) {
    return PRS->ASYNC_PEEK;
  } else {
    return PRS->SYNC_PEEK;
  }
#else
  (void) type;
  return PRS->PEEK;
#endif
}

/***************************************************************************//**
 * @brief
 *   Get the PRS channel value for a single channel.
 *
 * @param[in] ch
 *   PRS channel number.
 *
 * @param[in] type
 *   PRS channel type. This can be either @ref prsTypeAsync or
 *   @ref prsTypeSync.
 *
 * @return
 *   The current PRS channel output value. This is either 0 or 1.
 ******************************************************************************/
__STATIC_INLINE bool PRS_ChannelValue(unsigned int ch, PRS_ChType_t type)
{
  return (PRS_Values(type) >> ch) & 0x1U;
}
#endif

/***************************************************************************//**
 * @brief
 *   Trigger a high pulse (one HFPERCLK) for one or more channels.
 *
 * @details
 *   Setting a bit for a channel causes the bit in the register to remain high
 *   for one HFPERCLK cycle. Pulse is XORed with both the corresponding bit
 *   in PRS SWLEVEL register and the PRS input signal selected for the
 *   channel(s).
 *
 * @param[in] channels
 *   Logical ORed combination of channels to trigger a pulse for. Use
 *   PRS_SWPULSE_CHnPULSE defines.
 ******************************************************************************/
__STATIC_INLINE void PRS_PulseTrigger(uint32_t channels)
{
#if defined(_PRS_SWPULSE_MASK)
  PRS->SWPULSE = channels & _PRS_SWPULSE_MASK;
#else
  PRS->ASYNC_SWPULSE = channels & _PRS_ASYNC_SWPULSE_MASK;
#endif
}

/***************************************************************************//**
 * @brief
 *   Set the PRS channel level for one asynchronous PRS channel.
 *
 * @param[in] ch
 *   PRS channel number.
 *
 * @param[in] level
 *   true to set the level high (1) and false to set the level low (0).
 ******************************************************************************/
__STATIC_INLINE void PRS_ChannelLevelSet(unsigned int ch, bool level)
{
  PRS_LevelSet(level << ch, 0x1U << ch);
}

/***************************************************************************//**
 * @brief
 *   Trigger a pulse on one PRS channel.
 *
 * @param[in] ch
 *   PRS channel number.
 ******************************************************************************/
__STATIC_INLINE void PRS_ChannelPulse(unsigned int ch)
{
  PRS_PulseTrigger(0x1U << ch);
}

void PRS_SourceSignalSet(unsigned int ch,
                         uint32_t source,
                         uint32_t signal,
                         PRS_Edge_TypeDef edge);

#if defined(PRS_ASYNC_SUPPORTED)
void PRS_SourceAsyncSignalSet(unsigned int ch,
                              uint32_t source,
                              uint32_t signal);
#endif
#if defined(_PRS_ROUTELOC0_MASK) || (_PRS_ROUTE_MASK)
void PRS_GpioOutputLocation(unsigned int ch,
                            unsigned int location);
#endif

int PRS_GetFreeChannel(PRS_ChType_t type);
void PRS_Reset(void);
void PRS_ConnectSignal(unsigned int ch, PRS_ChType_t type, PRS_Signal_t signal);
#if defined(_SILICON_LABS_32B_SERIES_2)
void PRS_ConnectConsumer(unsigned int ch, PRS_ChType_t type, PRS_Consumer_t consumer);
void PRS_PinOutput(unsigned int ch, PRS_ChType_t type, GPIO_Port_TypeDef port, uint8_t pin);
void PRS_Combine(unsigned int chA, unsigned int chB, PRS_Logic_t logic);
#endif

/** @} (end addtogroup PRS) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(PRS_COUNT) && (PRS_COUNT > 0) */
#endif /* EM_PRS_H */
