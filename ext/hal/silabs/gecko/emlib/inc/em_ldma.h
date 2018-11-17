/***************************************************************************//**
 * @file em_ldma.h
 * @brief Direct memory access (LDMA) API
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
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
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

#ifndef EM_LDMA_H
#define EM_LDMA_H

#include "em_device.h"

#if defined(LDMA_PRESENT) && (LDMA_COUNT == 1)

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup LDMA
 * @brief Linked Direct Memory Access (LDMA) Peripheral API
 *
 * @details
 * LDMA API functions provide full support for the LDMA peripheral.
 *
 * LDMA supports these DMA transfer types:
 *
 * @li Memory to memory.
 * @li Memory to peripheral.
 * @li Peripheral to memory.
 * @li Peripheral to peripheral.
 * @li Constant value to memory.
 *
 * LDMA supports linked lists of DMA descriptors allowing:
 *
 * @li Circular and ping-pong buffer transfers.
 * @li Scatter-gather transfers.
 * @li Looped transfers.
 *
 * LDMA has some advanced features:
 *
 * @li Intra-channel synchronization (SYNC), allowing hardware events to
 *     pause and restart a DMA sequence.
 * @li Immediate-write (WRI), allowing DMA to write a constant anywhere
 *     in the memory map.
 * @li Complex flow control allowing if-else constructs.
 *
 * Basic understanding of LDMA controller is assumed. Please refer to
 * the reference manual for further details. The LDMA examples described
 * in the reference manual are particularly helpful in understanding LDMA
 * operations.
 *
 * In order to use the DMA controller, the initialization function @ref
 * LDMA_Init() must have been executed once (normally during system initialization).
 *
 * DMA transfers are initiated by a call to @ref LDMA_StartTransfer(),
 * transfer properties are controlled by the contents of @ref LDMA_TransferCfg_t
 * and @ref LDMA_Descriptor_t structure parameters.
 * The @htmlonly LDMA_Descriptor_t @endhtmlonly structure parameter may be a
 * pointer to an array of descriptors, descriptors in array should
 * be linked together as needed.
 *
 * Transfer and descriptor initialization macros are provided for the most common
 * transfer types. Due to the flexibility of LDMA peripheral, only a small
 * subset of all possible initializer macros are provided, users should create
 * new ones when needed.
 *
 * <b> Examples of LDMA usage: </b>
 *
 * A simple memory to memory transfer:
 *
 * @include em_ldma_single.c
 *
 * @n A linked list of three memory to memory transfers:
 *
 * @include em_ldma_link_memory.c
 *
 * @n DMA from serial port peripheral to memory:
 *
 * @include em_ldma_peripheral.c
 *
 * @n Ping-pong DMA from serial port peripheral to memory:
 *
 * @include em_ldma_pingpong.c
 *
 * @note LDMA module does not implement LDMA interrupt handler. A
 * template for an LDMA IRQ handler is included here as an example.
 *
 * @include em_ldma_irq.c
 *
 * @{
 ******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/**
 * Controls the number of unit data transfers per arbitration
 * cycle, providing a means to balance DMA channels' load on the controller.
 */
typedef enum {
  ldmaCtrlBlockSizeUnit1    = _LDMA_CH_CTRL_BLOCKSIZE_UNIT1,    /**< One transfer per arbitration.     */
  ldmaCtrlBlockSizeUnit2    = _LDMA_CH_CTRL_BLOCKSIZE_UNIT2,    /**< Two transfers per arbitration.    */
  ldmaCtrlBlockSizeUnit3    = _LDMA_CH_CTRL_BLOCKSIZE_UNIT3,    /**< Three transfers per arbitration.  */
  ldmaCtrlBlockSizeUnit4    = _LDMA_CH_CTRL_BLOCKSIZE_UNIT4,    /**< Four transfers per arbitration.   */
  ldmaCtrlBlockSizeUnit6    = _LDMA_CH_CTRL_BLOCKSIZE_UNIT6,    /**< Six transfers per arbitration.    */
  ldmaCtrlBlockSizeUnit8    = _LDMA_CH_CTRL_BLOCKSIZE_UNIT8,    /**< Eight transfers per arbitration.  */
  ldmaCtrlBlockSizeUnit16   = _LDMA_CH_CTRL_BLOCKSIZE_UNIT16,   /**< 16 transfers per arbitration.     */
  ldmaCtrlBlockSizeUnit32   = _LDMA_CH_CTRL_BLOCKSIZE_UNIT32,   /**< 32 transfers per arbitration.     */
  ldmaCtrlBlockSizeUnit64   = _LDMA_CH_CTRL_BLOCKSIZE_UNIT64,   /**< 64 transfers per arbitration.     */
  ldmaCtrlBlockSizeUnit128  = _LDMA_CH_CTRL_BLOCKSIZE_UNIT128,  /**< 128 transfers per arbitration.    */
  ldmaCtrlBlockSizeUnit256  = _LDMA_CH_CTRL_BLOCKSIZE_UNIT256,  /**< 256 transfers per arbitration.    */
  ldmaCtrlBlockSizeUnit512  = _LDMA_CH_CTRL_BLOCKSIZE_UNIT512,  /**< 512 transfers per arbitration.    */
  ldmaCtrlBlockSizeUnit1024 = _LDMA_CH_CTRL_BLOCKSIZE_UNIT1024, /**< 1024 transfers per arbitration.   */
  ldmaCtrlBlockSizeAll      = _LDMA_CH_CTRL_BLOCKSIZE_ALL       /**< Lock arbitration during transfer. */
} LDMA_CtrlBlockSize_t;

/** DMA structure type. */
typedef enum {
  ldmaCtrlStructTypeXfer  = _LDMA_CH_CTRL_STRUCTTYPE_TRANSFER,    /**< TRANSFER transfer type.    */
  ldmaCtrlStructTypeSync  = _LDMA_CH_CTRL_STRUCTTYPE_SYNCHRONIZE, /**< SYNCHRONIZE transfer type. */
  ldmaCtrlStructTypeWrite = _LDMA_CH_CTRL_STRUCTTYPE_WRITE        /**< WRITE transfer type.       */
} LDMA_CtrlStructType_t;

/** DMA transfer block or cycle selector. */
typedef enum {
  ldmaCtrlReqModeBlock = _LDMA_CH_CTRL_REQMODE_BLOCK, /**< Each DMA request trigger transfer of one block.     */
  ldmaCtrlReqModeAll   = _LDMA_CH_CTRL_REQMODE_ALL    /**< A DMA request trigger transfer of a complete cycle. */
} LDMA_CtrlReqMode_t;

/** Source address increment unit size. */
typedef enum {
  ldmaCtrlSrcIncOne  = _LDMA_CH_CTRL_SRCINC_ONE,  /**< Increment source address by one unit data size.   */
  ldmaCtrlSrcIncTwo  = _LDMA_CH_CTRL_SRCINC_TWO,  /**< Increment source address by two unit data sizes.  */
  ldmaCtrlSrcIncFour = _LDMA_CH_CTRL_SRCINC_FOUR, /**< Increment source address by four unit data sizes. */
  ldmaCtrlSrcIncNone = _LDMA_CH_CTRL_SRCINC_NONE  /**< Do not increment source address.                  */
} LDMA_CtrlSrcInc_t;

/** DMA transfer unit size. */
typedef enum {
  ldmaCtrlSizeByte = _LDMA_CH_CTRL_SIZE_BYTE,     /**< Each unit transfer is a byte.      */
  ldmaCtrlSizeHalf = _LDMA_CH_CTRL_SIZE_HALFWORD, /**< Each unit transfer is a half-word. */
  ldmaCtrlSizeWord = _LDMA_CH_CTRL_SIZE_WORD      /**< Each unit transfer is a word.      */
} LDMA_CtrlSize_t;

/** Destination address increment unit size. */
typedef enum {
  ldmaCtrlDstIncOne  = _LDMA_CH_CTRL_DSTINC_ONE,  /**< Increment destination address by one unit data size.   */
  ldmaCtrlDstIncTwo  = _LDMA_CH_CTRL_DSTINC_TWO,  /**< Increment destination address by two unit data sizes.  */
  ldmaCtrlDstIncFour = _LDMA_CH_CTRL_DSTINC_FOUR, /**< Increment destination address by four unit data sizes. */
  ldmaCtrlDstIncNone = _LDMA_CH_CTRL_DSTINC_NONE  /**< Do not increment destination address.                  */
} LDMA_CtrlDstInc_t;

/** Source addressing mode. */
typedef enum {
  ldmaCtrlSrcAddrModeAbs = _LDMA_CH_CTRL_SRCMODE_ABSOLUTE, /**< Address fetched from a linked structure is absolute.  */
  ldmaCtrlSrcAddrModeRel = _LDMA_CH_CTRL_SRCMODE_RELATIVE  /**< Address fetched from a linked structure is relative.  */
} LDMA_CtrlSrcAddrMode_t;

/** Destination addressing mode. */
typedef enum {
  ldmaCtrlDstAddrModeAbs = _LDMA_CH_CTRL_DSTMODE_ABSOLUTE, /**< Address fetched from a linked structure is absolute.  */
  ldmaCtrlDstAddrModeRel = _LDMA_CH_CTRL_DSTMODE_RELATIVE  /**< Address fetched from a linked structure is relative.  */
} LDMA_CtrlDstAddrMode_t;

/** DMA link load address mode. */
typedef enum {
  ldmaLinkModeAbs = _LDMA_CH_LINK_LINKMODE_ABSOLUTE, /**< Link address is an absolute address value.            */
  ldmaLinkModeRel = _LDMA_CH_LINK_LINKMODE_RELATIVE  /**< Link address is a two's complement relative address.  */
} LDMA_LinkMode_t;

/** Insert extra arbitration slots to increase channel arbitration priority. */
typedef enum {
  ldmaCfgArbSlotsAs1 = _LDMA_CH_CFG_ARBSLOTS_ONE,  /**< One arbitration slot selected.    */
  ldmaCfgArbSlotsAs2 = _LDMA_CH_CFG_ARBSLOTS_TWO,  /**< Two arbitration slots selected.   */
  ldmaCfgArbSlotsAs4 = _LDMA_CH_CFG_ARBSLOTS_FOUR, /**< Four arbitration slots selected.  */
  ldmaCfgArbSlotsAs8 = _LDMA_CH_CFG_ARBSLOTS_EIGHT /**< Eight arbitration slots selected. */
} LDMA_CfgArbSlots_t;

/** Source address increment sign. */
typedef enum {
  ldmaCfgSrcIncSignPos = _LDMA_CH_CFG_SRCINCSIGN_POSITIVE, /**< Increment source address. */
  ldmaCfgSrcIncSignNeg = _LDMA_CH_CFG_SRCINCSIGN_NEGATIVE  /**< Decrement source address. */
} LDMA_CfgSrcIncSign_t;

/** Destination address increment sign. */
typedef enum {
  ldmaCfgDstIncSignPos = _LDMA_CH_CFG_DSTINCSIGN_POSITIVE, /**< Increment destination address. */
  ldmaCfgDstIncSignNeg = _LDMA_CH_CFG_DSTINCSIGN_NEGATIVE  /**< Decrement destination address. */
} LDMA_CfgDstIncSign_t;

#if defined(LDMAXBAR_COUNT) && (LDMAXBAR_COUNT > 0)
/** Peripherals that can trigger LDMA transfers. */
typedef enum {
  ldmaPeripheralSignal_NONE = LDMAXBAR_CH_REQSEL_SOURCESEL_NONE,                                                                ///< No peripheral selected for DMA triggering.
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER0CC0
  ldmaPeripheralSignal_TIMER0_CC0 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER0CC0 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER0CC1
  ldmaPeripheralSignal_TIMER0_CC1 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER0CC1 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER0CC2
  ldmaPeripheralSignal_TIMER0_CC2 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER0CC2 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER0UFOF
  ldmaPeripheralSignal_TIMER0_UFOF = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER0UFOF | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER1CC0
  ldmaPeripheralSignal_TIMER1_CC0 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER1CC0 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER1,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER1CC1
  ldmaPeripheralSignal_TIMER1_CC1 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER1CC1 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER1,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER1CC2
  ldmaPeripheralSignal_TIMER1_CC2 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER1CC2 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER1,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER1UFOF
  ldmaPeripheralSignal_TIMER1_UFOF = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER1UFOF | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER1,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART0RXDATAV
  ldmaPeripheralSignal_USART0_RXDATAV = LDMAXBAR_CH_REQSEL_SIGSEL_USART0RXDATAV | LDMAXBAR_CH_REQSEL_SOURCESEL_USART0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART0RXDATAVRIGHT
  ldmaPeripheralSignal_USART0_RXDATAVRIGHT = LDMAXBAR_CH_REQSEL_SIGSEL_USART0RXDATAVRIGHT | LDMAXBAR_CH_REQSEL_SOURCESEL_USART0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART0TXBL
  ldmaPeripheralSignal_USART0_TXBL = LDMAXBAR_CH_REQSEL_SIGSEL_USART0TXBL | LDMAXBAR_CH_REQSEL_SOURCESEL_USART0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART0TXBLRIGHT
  ldmaPeripheralSignal_USART0_TXBLRIGHT = LDMAXBAR_CH_REQSEL_SIGSEL_USART0TXBLRIGHT | LDMAXBAR_CH_REQSEL_SOURCESEL_USART0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART0TXEMPTY
  ldmaPeripheralSignal_USART0_TXEMPTY = LDMAXBAR_CH_REQSEL_SIGSEL_USART0TXEMPTY | LDMAXBAR_CH_REQSEL_SOURCESEL_USART0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART1RXDATAV
  ldmaPeripheralSignal_USART1_RXDATAV = LDMAXBAR_CH_REQSEL_SIGSEL_USART1RXDATAV | LDMAXBAR_CH_REQSEL_SOURCESEL_USART1,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART1RXDATAVRIGHT
  ldmaPeripheralSignal_USART1_RXDATAVRIGHT = LDMAXBAR_CH_REQSEL_SIGSEL_USART1RXDATAVRIGHT | LDMAXBAR_CH_REQSEL_SOURCESEL_USART1,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART1TXBL
  ldmaPeripheralSignal_USART1_TXBL = LDMAXBAR_CH_REQSEL_SIGSEL_USART1TXBL | LDMAXBAR_CH_REQSEL_SOURCESEL_USART1,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART1TXBLRIGHT
  ldmaPeripheralSignal_USART1_TXBLRIGHT = LDMAXBAR_CH_REQSEL_SIGSEL_USART1TXBLRIGHT | LDMAXBAR_CH_REQSEL_SOURCESEL_USART1,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART1TXEMPTY
  ldmaPeripheralSignal_USART1_TXEMPTY = LDMAXBAR_CH_REQSEL_SIGSEL_USART1TXEMPTY | LDMAXBAR_CH_REQSEL_SOURCESEL_USART1,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART2RXDATAV
  ldmaPeripheralSignal_USART2_RXDATAV = LDMAXBAR_CH_REQSEL_SIGSEL_USART2RXDATAV | LDMAXBAR_CH_REQSEL_SOURCESEL_USART2,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART2RXDATAVRIGHT
  ldmaPeripheralSignal_USART2_RXDATAVRIGHT = LDMAXBAR_CH_REQSEL_SIGSEL_USART2RXDATAVRIGHT | LDMAXBAR_CH_REQSEL_SOURCESEL_USART2,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART2TXBL
  ldmaPeripheralSignal_USART2_TXBL = LDMAXBAR_CH_REQSEL_SIGSEL_USART2TXBL | LDMAXBAR_CH_REQSEL_SOURCESEL_USART2,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART2TXBLRIGHT
  ldmaPeripheralSignal_USART2_TXBLRIGHT = LDMAXBAR_CH_REQSEL_SIGSEL_USART2TXBLRIGHT | LDMAXBAR_CH_REQSEL_SOURCESEL_USART2,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_USART2TXEMPTY
  ldmaPeripheralSignal_USART2_TXEMPTY = LDMAXBAR_CH_REQSEL_SIGSEL_USART2TXEMPTY | LDMAXBAR_CH_REQSEL_SOURCESEL_USART2,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_I2C0RXDATAV
  ldmaPeripheralSignal_I2C0_RXDATAV = LDMAXBAR_CH_REQSEL_SIGSEL_I2C0RXDATAV | LDMAXBAR_CH_REQSEL_SOURCESEL_I2C0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_I2C0TXBL
  ldmaPeripheralSignal_I2C0_TXBL = LDMAXBAR_CH_REQSEL_SIGSEL_I2C0TXBL | LDMAXBAR_CH_REQSEL_SOURCESEL_I2C0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_I2C1RXDATAV
  ldmaPeripheralSignal_I2C1_RXDATAV = LDMAXBAR_CH_REQSEL_SIGSEL_I2C1RXDATAV | LDMAXBAR_CH_REQSEL_SOURCESEL_I2C1,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_I2C1TXBL
  ldmaPeripheralSignal_I2C1_TXBL = LDMAXBAR_CH_REQSEL_SIGSEL_I2C1TXBL | LDMAXBAR_CH_REQSEL_SOURCESEL_I2C1,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_AGCRSSI
  ldmaPeripheralSignal_AGC_RSSI = LDMAXBAR_CH_REQSEL_SIGSEL_AGCRSSI | LDMAXBAR_CH_REQSEL_SOURCESEL_AGC,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERBOF
  ldmaPeripheralSignal_PROTIMER_BOF = LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERBOF | LDMAXBAR_CH_REQSEL_SOURCESEL_PROTIMER,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERCC0
  ldmaPeripheralSignal_PROTIMER_CC0 = LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERCC0 | LDMAXBAR_CH_REQSEL_SOURCESEL_PROTIMER,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERCC1
  ldmaPeripheralSignal_PROTIMER_CC1 = LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERCC1 | LDMAXBAR_CH_REQSEL_SOURCESEL_PROTIMER,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERCC2
  ldmaPeripheralSignal_PROTIMER_CC2 = LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERCC2 | LDMAXBAR_CH_REQSEL_SOURCESEL_PROTIMER,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERCC3
  ldmaPeripheralSignal_PROTIMER_CC3 = LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERCC3 | LDMAXBAR_CH_REQSEL_SOURCESEL_PROTIMER,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERCC4
  ldmaPeripheralSignal_PROTIMER_CC4 = LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERCC4 | LDMAXBAR_CH_REQSEL_SOURCESEL_PROTIMER,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERPOF
  ldmaPeripheralSignal_PROTIMER_POF = LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERPOF | LDMAXBAR_CH_REQSEL_SOURCESEL_PROTIMER,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERWOF
  ldmaPeripheralSignal_PROTIMER_WOF = LDMAXBAR_CH_REQSEL_SIGSEL_PROTIMERWOF | LDMAXBAR_CH_REQSEL_SOURCESEL_PROTIMER,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_MODEMDEBUG
  ldmaPeripheralSignal_MODEM_DEBUG = LDMAXBAR_CH_REQSEL_SIGSEL_MODEMDEBUG | LDMAXBAR_CH_REQSEL_SOURCESEL_MODEM,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_IADC0IADC_SCAN
  ldmaPeripheralSignal_IADC0_IADC_SCAN = LDMAXBAR_CH_REQSEL_SIGSEL_IADC0IADC_SCAN | LDMAXBAR_CH_REQSEL_SOURCESEL_IADC0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_IADC0IADC_SINGLE
  ldmaPeripheralSignal_IADC0_IADC_SINGLE = LDMAXBAR_CH_REQSEL_SIGSEL_IADC0IADC_SINGLE | LDMAXBAR_CH_REQSEL_SOURCESEL_IADC0,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_IMEMWDATA
  ldmaPeripheralSignal_IMEM_WDATA = LDMAXBAR_CH_REQSEL_SIGSEL_IMEMWDATA | LDMAXBAR_CH_REQSEL_SOURCESEL_IMEM,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER2CC0
  ldmaPeripheralSignal_TIMER2_CC0 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER2CC0 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER2,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER2CC1
  ldmaPeripheralSignal_TIMER2_CC1 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER2CC1 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER2,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER2CC2
  ldmaPeripheralSignal_TIMER2_CC2 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER2CC2 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER2,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER2UFOF
  ldmaPeripheralSignal_TIMER2_UFOF = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER2UFOF | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER2,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER3CC0
  ldmaPeripheralSignal_TIMER3_CC0 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER3CC0 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER3,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER3CC1
  ldmaPeripheralSignal_TIMER3_CC1 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER3CC1 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER3,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER3CC2
  ldmaPeripheralSignal_TIMER3_CC2 = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER3CC2 | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER3,
  #endif
  #if defined LDMAXBAR_CH_REQSEL_SIGSEL_TIMER3UFOF
  ldmaPeripheralSignal_TIMER3_UFOF = LDMAXBAR_CH_REQSEL_SIGSEL_TIMER3UFOF | LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER3,
  #endif
} LDMA_PeripheralSignal_t;

#else

typedef enum {
  ldmaPeripheralSignal_NONE = LDMA_CH_REQSEL_SOURCESEL_NONE,                                                                ///< No peripheral selected for DMA triggering.
  #if defined(LDMA_CH_REQSEL_SIGSEL_ADC0SCAN)
  ldmaPeripheralSignal_ADC0_SCAN = LDMA_CH_REQSEL_SIGSEL_ADC0SCAN | LDMA_CH_REQSEL_SOURCESEL_ADC0,                          ///< Trigger on ADC0_SCAN.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_ADC0SINGLE)
  ldmaPeripheralSignal_ADC0_SINGLE = LDMA_CH_REQSEL_SIGSEL_ADC0SINGLE | LDMA_CH_REQSEL_SOURCESEL_ADC0,                      ///< Trigger on ADC0_SINGLE.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_ADC1SCAN)
  ldmaPeripheralSignal_ADC1_SCAN = LDMA_CH_REQSEL_SIGSEL_ADC1SCAN | LDMA_CH_REQSEL_SOURCESEL_ADC1,                          ///< Trigger on ADC1_SCAN.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_ADC1SINGLE)
  ldmaPeripheralSignal_ADC1_SINGLE = LDMA_CH_REQSEL_SIGSEL_ADC1SINGLE | LDMA_CH_REQSEL_SOURCESEL_ADC1,                      ///< Trigger on ADC1_SINGLE.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0RD)
  ldmaPeripheralSignal_CRYPTO_DATA0RD = LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO,              ///< Trigger on CRYPTO_DATA0RD.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0WR)
  ldmaPeripheralSignal_CRYPTO_DATA0WR = LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO,              ///< Trigger on CRYPTO_DATA0WR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0XWR)
  ldmaPeripheralSignal_CRYPTO_DATA0XWR = LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0XWR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO,            ///< Trigger on CRYPTO_DATA0XWR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1RD)
  ldmaPeripheralSignal_CRYPTO_DATA1RD = LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO,              ///< Trigger on CRYPTO_DATA1RD.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1WR)
  ldmaPeripheralSignal_CRYPTO_DATA1WR = LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO,              ///< Trigger on CRYPTO_DATA1WR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0RD)
  ldmaPeripheralSignal_CRYPTO0_DATA0RD = LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO0,           ///< Trigger on CRYPTO0_DATA0RD.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0WR)
  ldmaPeripheralSignal_CRYPTO0_DATA0WR = LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO0,           ///< Trigger on CRYPTO0_DATA0WR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0XWR)
  ldmaPeripheralSignal_CRYPTO0_DATA0XWR = LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0XWR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO0,         ///< Trigger on CRYPTO0_DATA0XWR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA1RD)
  ldmaPeripheralSignal_CRYPTO0_DATA1RD = LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA1RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO0,           ///< Trigger on CRYPTO0_DATA1RD.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA1WR)
  ldmaPeripheralSignal_CRYPTO0_DATA1WR = LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA1WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO0,           ///< Trigger on CRYPTO0_DATA1WR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0RD)
  ldmaPeripheralSignal_CRYPTO1_DATA0RD = LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO1,           ///< Trigger on CRYPTO1_DATA0RD.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0WR)
  ldmaPeripheralSignal_CRYPTO1_DATA0WR = LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO1,           ///< Trigger on CRYPTO1_DATA0WR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0XWR)
  ldmaPeripheralSignal_CRYPTO1_DATA0XWR = LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0XWR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO1,         ///< Trigger on CRYPTO1_DATA0XWR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA1RD)
  ldmaPeripheralSignal_CRYPTO1_DATA1RD = LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA1RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO1,           ///< Trigger on CRYPTO1_DATA1RD.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA1WR)
  ldmaPeripheralSignal_CRYPTO1_DATA1WR = LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA1WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO1,           ///< Trigger on CRYPTO1_DATA1WR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CSENBSLN)
  ldmaPeripheralSignal_CSEN_BSLN = LDMA_CH_REQSEL_SIGSEL_CSENBSLN | LDMA_CH_REQSEL_SOURCESEL_CSEN,                          ///< Trigger on CSEN_BSLN.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CSENDATA)
  ldmaPeripheralSignal_CSEN_DATA = LDMA_CH_REQSEL_SIGSEL_CSENDATA | LDMA_CH_REQSEL_SOURCESEL_CSEN,                          ///< Trigger on CSEN_DATA.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_EBIPXL0EMPTY)
  ldmaPeripheralSignal_EBI_PXL0EMPTY = LDMA_CH_REQSEL_SIGSEL_EBIPXL0EMPTY | LDMA_CH_REQSEL_SOURCESEL_EBI,                   ///< Trigger on EBI_PXL0EMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_EBIPXL1EMPTY)
  ldmaPeripheralSignal_EBI_PXL1EMPTY = LDMA_CH_REQSEL_SIGSEL_EBIPXL1EMPTY | LDMA_CH_REQSEL_SOURCESEL_EBI,                   ///< Trigger on EBI_PXL1EMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_EBIPXLFULL)
  ldmaPeripheralSignal_EBI_PXLFULL = LDMA_CH_REQSEL_SIGSEL_EBIPXLFULL | LDMA_CH_REQSEL_SOURCESEL_EBI,                       ///< Trigger on EBI_PXLFULL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_EBIDDEMPTY)
  ldmaPeripheralSignal_EBI_DDEMPTY = LDMA_CH_REQSEL_SIGSEL_EBIDDEMPTY | LDMA_CH_REQSEL_SOURCESEL_EBI,                       ///< Trigger on EBI_DDEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_EBIVSYNC)
  ldmaPeripheralSignal_EBI_VSYNC = LDMA_CH_REQSEL_SIGSEL_EBIVSYNC | LDMA_CH_REQSEL_SOURCESEL_EBI,                           ///< Trigger on EBI_VSYNC.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_EBIHSYNC)
  ldmaPeripheralSignal_EBI_HSYNC = LDMA_CH_REQSEL_SIGSEL_EBIHSYNC | LDMA_CH_REQSEL_SOURCESEL_EBI,                           ///< Trigger on EBI_HSYNC.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_I2C0RXDATAV)
  ldmaPeripheralSignal_I2C0_RXDATAV = LDMA_CH_REQSEL_SIGSEL_I2C0RXDATAV | LDMA_CH_REQSEL_SOURCESEL_I2C0,                    ///< Trigger on I2C0_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_I2C0TXBL)
  ldmaPeripheralSignal_I2C0_TXBL = LDMA_CH_REQSEL_SIGSEL_I2C0TXBL | LDMA_CH_REQSEL_SOURCESEL_I2C0,                          ///< Trigger on I2C0_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_I2C1RXDATAV)
  ldmaPeripheralSignal_I2C1_RXDATAV = LDMA_CH_REQSEL_SIGSEL_I2C1RXDATAV | LDMA_CH_REQSEL_SOURCESEL_I2C1,                    ///< Trigger on I2C1_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_I2C1TXBL)
  ldmaPeripheralSignal_I2C1_TXBL = LDMA_CH_REQSEL_SIGSEL_I2C1TXBL | LDMA_CH_REQSEL_SOURCESEL_I2C1,                          ///< Trigger on I2C1_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_I2C2RXDATAV)
  ldmaPeripheralSignal_I2C2_RXDATAV = LDMA_CH_REQSEL_SIGSEL_I2C2RXDATAV | LDMA_CH_REQSEL_SOURCESEL_I2C2,                    ///< Trigger on I2C2_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_I2C2TXBL)
  ldmaPeripheralSignal_I2C2_TXBL = LDMA_CH_REQSEL_SIGSEL_I2C2TXBL | LDMA_CH_REQSEL_SOURCESEL_I2C2,                          ///< Trigger on I2C2_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_LESENSEBUFDATAV)
  ldmaPeripheralSignal_LESENSE_BUFDATAV = LDMA_CH_REQSEL_SIGSEL_LESENSEBUFDATAV | LDMA_CH_REQSEL_SOURCESEL_LESENSE,         ///< Trigger on LESENSE_BUFDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_LEUART0RXDATAV)
  ldmaPeripheralSignal_LEUART0_RXDATAV = LDMA_CH_REQSEL_SIGSEL_LEUART0RXDATAV | LDMA_CH_REQSEL_SOURCESEL_LEUART0,           ///< Trigger on LEUART0_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_LEUART0TXBL)
  ldmaPeripheralSignal_LEUART0_TXBL = LDMA_CH_REQSEL_SIGSEL_LEUART0TXBL | LDMA_CH_REQSEL_SOURCESEL_LEUART0,                 ///< Trigger on LEUART0_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_LEUART0TXEMPTY)
  ldmaPeripheralSignal_LEUART0_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_LEUART0TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_LEUART0,           ///< Trigger on LEUART0_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_LEUART1RXDATAV)
  ldmaPeripheralSignal_LEUART1_RXDATAV = LDMA_CH_REQSEL_SIGSEL_LEUART1RXDATAV | LDMA_CH_REQSEL_SOURCESEL_LEUART1,           ///< Trigger on LEUART1_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_LEUART1TXBL)
  ldmaPeripheralSignal_LEUART1_TXBL = LDMA_CH_REQSEL_SIGSEL_LEUART1TXBL | LDMA_CH_REQSEL_SOURCESEL_LEUART1,                 ///< Trigger on LEUART1_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_LEUART1TXEMPTY)
  ldmaPeripheralSignal_LEUART1_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_LEUART1TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_LEUART1,           ///< Trigger on LEUART1_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_MSCWDATA)
  ldmaPeripheralSignal_MSC_WDATA = LDMA_CH_REQSEL_SIGSEL_MSCWDATA | LDMA_CH_REQSEL_SOURCESEL_MSC,                           ///< Trigger on MSC_WDATA.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_PRSREQ0)
  ldmaPeripheralSignal_PRS_REQ0 = LDMA_CH_REQSEL_SIGSEL_PRSREQ0 | LDMA_CH_REQSEL_SOURCESEL_PRS,                             ///< Trigger on PRS_REQ0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_PRSREQ1)
  ldmaPeripheralSignal_PRS_REQ1 = LDMA_CH_REQSEL_SIGSEL_PRSREQ1 | LDMA_CH_REQSEL_SOURCESEL_PRS,                             ///< Trigger on PRS_REQ1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER0CC0)
  ldmaPeripheralSignal_TIMER0_CC0 = LDMA_CH_REQSEL_SIGSEL_TIMER0CC0 | LDMA_CH_REQSEL_SOURCESEL_TIMER0,                      ///< Trigger on TIMER0_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER0CC1)
  ldmaPeripheralSignal_TIMER0_CC1 = LDMA_CH_REQSEL_SIGSEL_TIMER0CC1 | LDMA_CH_REQSEL_SOURCESEL_TIMER0,                      ///< Trigger on TIMER0_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER0CC2)
  ldmaPeripheralSignal_TIMER0_CC2 = LDMA_CH_REQSEL_SIGSEL_TIMER0CC2 | LDMA_CH_REQSEL_SOURCESEL_TIMER0,                      ///< Trigger on TIMER0_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER0UFOF)
  ldmaPeripheralSignal_TIMER0_UFOF = LDMA_CH_REQSEL_SIGSEL_TIMER0UFOF | LDMA_CH_REQSEL_SOURCESEL_TIMER0,                    ///< Trigger on TIMER0_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER1CC0)
  ldmaPeripheralSignal_TIMER1_CC0 = LDMA_CH_REQSEL_SIGSEL_TIMER1CC0 | LDMA_CH_REQSEL_SOURCESEL_TIMER1,                      ///< Trigger on TIMER1_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER1CC1)
  ldmaPeripheralSignal_TIMER1_CC1 = LDMA_CH_REQSEL_SIGSEL_TIMER1CC1 | LDMA_CH_REQSEL_SOURCESEL_TIMER1,                      ///< Trigger on TIMER1_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER1CC2)
  ldmaPeripheralSignal_TIMER1_CC2 = LDMA_CH_REQSEL_SIGSEL_TIMER1CC2 | LDMA_CH_REQSEL_SOURCESEL_TIMER1,                      ///< Trigger on TIMER1_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER1CC3)
  ldmaPeripheralSignal_TIMER1_CC3 = LDMA_CH_REQSEL_SIGSEL_TIMER1CC3 | LDMA_CH_REQSEL_SOURCESEL_TIMER1,                      ///< Trigger on TIMER1_CC3.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER1UFOF)
  ldmaPeripheralSignal_TIMER1_UFOF = LDMA_CH_REQSEL_SIGSEL_TIMER1UFOF | LDMA_CH_REQSEL_SOURCESEL_TIMER1,                    ///< Trigger on TIMER1_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER2CC0)
  ldmaPeripheralSignal_TIMER2_CC0 = LDMA_CH_REQSEL_SIGSEL_TIMER2CC0 | LDMA_CH_REQSEL_SOURCESEL_TIMER2,                      ///< Trigger on TIMER2_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER2CC1)
  ldmaPeripheralSignal_TIMER2_CC1 = LDMA_CH_REQSEL_SIGSEL_TIMER2CC1 | LDMA_CH_REQSEL_SOURCESEL_TIMER2,                      ///< Trigger on TIMER2_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER2CC2)
  ldmaPeripheralSignal_TIMER2_CC2 = LDMA_CH_REQSEL_SIGSEL_TIMER2CC2 | LDMA_CH_REQSEL_SOURCESEL_TIMER2,                      ///< Trigger on TIMER2_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER2UFOF)
  ldmaPeripheralSignal_TIMER2_UFOF = LDMA_CH_REQSEL_SIGSEL_TIMER2UFOF | LDMA_CH_REQSEL_SOURCESEL_TIMER2,                    ///< Trigger on TIMER2_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER3CC0)
  ldmaPeripheralSignal_TIMER3_CC0 = LDMA_CH_REQSEL_SIGSEL_TIMER3CC0 | LDMA_CH_REQSEL_SOURCESEL_TIMER3,                      ///< Trigger on TIMER3_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER3CC1)
  ldmaPeripheralSignal_TIMER3_CC1 = LDMA_CH_REQSEL_SIGSEL_TIMER3CC1 | LDMA_CH_REQSEL_SOURCESEL_TIMER3,                      ///< Trigger on TIMER3_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER3CC2)
  ldmaPeripheralSignal_TIMER3_CC2 = LDMA_CH_REQSEL_SIGSEL_TIMER3CC2 | LDMA_CH_REQSEL_SOURCESEL_TIMER3,                      ///< Trigger on TIMER3_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER3UFOF)
  ldmaPeripheralSignal_TIMER3_UFOF = LDMA_CH_REQSEL_SIGSEL_TIMER3UFOF | LDMA_CH_REQSEL_SOURCESEL_TIMER3,                    ///< Trigger on TIMER3_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER4CC0)
  ldmaPeripheralSignal_TIMER4_CC0 = LDMA_CH_REQSEL_SIGSEL_TIMER4CC0 | LDMA_CH_REQSEL_SOURCESEL_TIMER4,                      ///< Trigger on TIMER4_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER4CC1)
  ldmaPeripheralSignal_TIMER4_CC1 = LDMA_CH_REQSEL_SIGSEL_TIMER4CC1 | LDMA_CH_REQSEL_SOURCESEL_TIMER4,                      ///< Trigger on TIMER4_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER4CC2)
  ldmaPeripheralSignal_TIMER4_CC2 = LDMA_CH_REQSEL_SIGSEL_TIMER4CC2 | LDMA_CH_REQSEL_SOURCESEL_TIMER4,                      ///< Trigger on TIMER4_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER4UFOF)
  ldmaPeripheralSignal_TIMER4_UFOF = LDMA_CH_REQSEL_SIGSEL_TIMER4UFOF | LDMA_CH_REQSEL_SOURCESEL_TIMER4,                    ///< Trigger on TIMER4_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER5CC0)
  ldmaPeripheralSignal_TIMER5_CC0 = LDMA_CH_REQSEL_SIGSEL_TIMER5CC0 | LDMA_CH_REQSEL_SOURCESEL_TIMER5,                      ///< Trigger on TIMER5_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER5CC1)
  ldmaPeripheralSignal_TIMER5_CC1 = LDMA_CH_REQSEL_SIGSEL_TIMER5CC1 | LDMA_CH_REQSEL_SOURCESEL_TIMER5,                      ///< Trigger on TIMER5_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER5CC2)
  ldmaPeripheralSignal_TIMER5_CC2 = LDMA_CH_REQSEL_SIGSEL_TIMER5CC2 | LDMA_CH_REQSEL_SOURCESEL_TIMER5,                      ///< Trigger on TIMER5_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER5UFOF)
  ldmaPeripheralSignal_TIMER5_UFOF = LDMA_CH_REQSEL_SIGSEL_TIMER5UFOF | LDMA_CH_REQSEL_SOURCESEL_TIMER5,                    ///< Trigger on TIMER5_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER6CC0)
  ldmaPeripheralSignal_TIMER6_CC0 = LDMA_CH_REQSEL_SIGSEL_TIMER6CC0 | LDMA_CH_REQSEL_SOURCESEL_TIMER6,                      ///< Trigger on TIMER6_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER6CC1)
  ldmaPeripheralSignal_TIMER6_CC1 = LDMA_CH_REQSEL_SIGSEL_TIMER6CC1 | LDMA_CH_REQSEL_SOURCESEL_TIMER6,                      ///< Trigger on TIMER6_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER6CC2)
  ldmaPeripheralSignal_TIMER6_CC2 = LDMA_CH_REQSEL_SIGSEL_TIMER6CC2 | LDMA_CH_REQSEL_SOURCESEL_TIMER6,                      ///< Trigger on TIMER6_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER6UFOF)
  ldmaPeripheralSignal_TIMER6_UFOF = LDMA_CH_REQSEL_SIGSEL_TIMER6UFOF | LDMA_CH_REQSEL_SOURCESEL_TIMER6,                    ///< Trigger on TIMER6_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_UART0RXDATAV)
  ldmaPeripheralSignal_UART0_RXDATAV = LDMA_CH_REQSEL_SIGSEL_UART0RXDATAV | LDMA_CH_REQSEL_SOURCESEL_UART0,                 ///< Trigger on UART0_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_UART0TXBL)
  ldmaPeripheralSignal_UART0_TXBL = LDMA_CH_REQSEL_SIGSEL_UART0TXBL | LDMA_CH_REQSEL_SOURCESEL_UART0,                       ///< Trigger on UART0_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_UART0TXEMPTY)
  ldmaPeripheralSignal_UART0_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_UART0TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_UART0,                 ///< Trigger on UART0_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_UART1RXDATAV)
  ldmaPeripheralSignal_UART1_RXDATAV = LDMA_CH_REQSEL_SIGSEL_UART1RXDATAV | LDMA_CH_REQSEL_SOURCESEL_UART1,                 ///< Trigger on UART1_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_UART1TXBL)
  ldmaPeripheralSignal_UART1_TXBL = LDMA_CH_REQSEL_SIGSEL_UART1TXBL | LDMA_CH_REQSEL_SOURCESEL_UART1,                       ///< Trigger on UART1_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_UART1TXEMPTY)
  ldmaPeripheralSignal_UART1_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_UART1TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_UART1,                 ///< Trigger on UART1_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART0RXDATAV)
  ldmaPeripheralSignal_USART0_RXDATAV = LDMA_CH_REQSEL_SIGSEL_USART0RXDATAV | LDMA_CH_REQSEL_SOURCESEL_USART0,              ///< Trigger on USART0_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART0TXBL)
  ldmaPeripheralSignal_USART0_TXBL = LDMA_CH_REQSEL_SIGSEL_USART0TXBL | LDMA_CH_REQSEL_SOURCESEL_USART0,                    ///< Trigger on USART0_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART0TXEMPTY)
  ldmaPeripheralSignal_USART0_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_USART0TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_USART0,              ///< Trigger on USART0_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART1RXDATAV)
  ldmaPeripheralSignal_USART1_RXDATAV = LDMA_CH_REQSEL_SIGSEL_USART1RXDATAV | LDMA_CH_REQSEL_SOURCESEL_USART1,              ///< Trigger on USART1_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART1RXDATAVRIGHT)
  ldmaPeripheralSignal_USART1_RXDATAVRIGHT = LDMA_CH_REQSEL_SIGSEL_USART1RXDATAVRIGHT | LDMA_CH_REQSEL_SOURCESEL_USART1,    ///< Trigger on USART1_RXDATAVRIGHT.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART1TXBL)
  ldmaPeripheralSignal_USART1_TXBL = LDMA_CH_REQSEL_SIGSEL_USART1TXBL | LDMA_CH_REQSEL_SOURCESEL_USART1,                    ///< Trigger on USART1_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART1TXBLRIGHT)
  ldmaPeripheralSignal_USART1_TXBLRIGHT = LDMA_CH_REQSEL_SIGSEL_USART1TXBLRIGHT | LDMA_CH_REQSEL_SOURCESEL_USART1,          ///< Trigger on USART1_TXBLRIGHT.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART1TXEMPTY)
  ldmaPeripheralSignal_USART1_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_USART1TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_USART1,              ///< Trigger on USART1_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART2RXDATAV)
  ldmaPeripheralSignal_USART2_RXDATAV = LDMA_CH_REQSEL_SIGSEL_USART2RXDATAV | LDMA_CH_REQSEL_SOURCESEL_USART2,              ///< Trigger on USART2_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART2TXBL)
  ldmaPeripheralSignal_USART2_TXBL = LDMA_CH_REQSEL_SIGSEL_USART2TXBL | LDMA_CH_REQSEL_SOURCESEL_USART2,                    ///< Trigger on USART2_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART2TXEMPTY)
  ldmaPeripheralSignal_USART2_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_USART2TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_USART2,              ///< Trigger on USART2_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART3RXDATAV)
  ldmaPeripheralSignal_USART3_RXDATAV = LDMA_CH_REQSEL_SIGSEL_USART3RXDATAV | LDMA_CH_REQSEL_SOURCESEL_USART3,              ///< Trigger on USART3_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART3RXDATAVRIGHT)
  ldmaPeripheralSignal_USART3_RXDATAVRIGHT = LDMA_CH_REQSEL_SIGSEL_USART3RXDATAVRIGHT | LDMA_CH_REQSEL_SOURCESEL_USART3,    ///< Trigger on USART3_RXDATAVRIGHT.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART3TXBL)
  ldmaPeripheralSignal_USART3_TXBL = LDMA_CH_REQSEL_SIGSEL_USART3TXBL | LDMA_CH_REQSEL_SOURCESEL_USART3,                    ///< Trigger on USART3_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART3TXBLRIGHT)
  ldmaPeripheralSignal_USART3_TXBLRIGHT = LDMA_CH_REQSEL_SIGSEL_USART3TXBLRIGHT | LDMA_CH_REQSEL_SOURCESEL_USART3,          ///< Trigger on USART3_TXBLRIGHT.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART3TXEMPTY)
  ldmaPeripheralSignal_USART3_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_USART3TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_USART3,              ///< Trigger on USART3_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART4RXDATAV)
  ldmaPeripheralSignal_USART4_RXDATAV = LDMA_CH_REQSEL_SIGSEL_USART4RXDATAV | LDMA_CH_REQSEL_SOURCESEL_USART4,              ///< Trigger on USART4_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART4RXDATAVRIGHT)
  ldmaPeripheralSignal_USART4_RXDATAVRIGHT = LDMA_CH_REQSEL_SIGSEL_USART4RXDATAVRIGHT | LDMA_CH_REQSEL_SOURCESEL_USART4,    ///< Trigger on USART4_RXDATAVRIGHT.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART4TXBL)
  ldmaPeripheralSignal_USART4_TXBL = LDMA_CH_REQSEL_SIGSEL_USART4TXBL | LDMA_CH_REQSEL_SOURCESEL_USART4,                    ///< Trigger on USART4_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART4TXBLRIGHT)
  ldmaPeripheralSignal_USART4_TXBLRIGHT = LDMA_CH_REQSEL_SIGSEL_USART4TXBLRIGHT | LDMA_CH_REQSEL_SOURCESEL_USART4,          ///< Trigger on USART4_TXBLRIGHT.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART4TXEMPTY)
  ldmaPeripheralSignal_USART4_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_USART4TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_USART4,              ///< Trigger on USART4_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART5RXDATAV)
  ldmaPeripheralSignal_USART5_RXDATAV = LDMA_CH_REQSEL_SIGSEL_USART5RXDATAV | LDMA_CH_REQSEL_SOURCESEL_USART5,              ///< Trigger on USART5_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART5TXBL)
  ldmaPeripheralSignal_USART5_TXBL = LDMA_CH_REQSEL_SIGSEL_USART5TXBL | LDMA_CH_REQSEL_SOURCESEL_USART5,                    ///< Trigger on USART5_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART5TXEMPTY)
  ldmaPeripheralSignal_USART5_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_USART5TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_USART5,              ///< Trigger on USART5_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_VDAC0CH0)
  ldmaPeripheralSignal_VDAC0_CH0 = LDMA_CH_REQSEL_SIGSEL_VDAC0CH0 | LDMA_CH_REQSEL_SOURCESEL_VDAC0,                         ///< Trigger on VDAC0_CH0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_VDAC0CH1)
  ldmaPeripheralSignal_VDAC0_CH1 = LDMA_CH_REQSEL_SIGSEL_VDAC0CH1 | LDMA_CH_REQSEL_SOURCESEL_VDAC0,                         ///< Trigger on VDAC0_CH1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER0CC0)
  ldmaPeripheralSignal_WTIMER0_CC0 = LDMA_CH_REQSEL_SIGSEL_WTIMER0CC0 | LDMA_CH_REQSEL_SOURCESEL_WTIMER0,                   ///< Trigger on WTIMER0_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER0CC1)
  ldmaPeripheralSignal_WTIMER0_CC1 = LDMA_CH_REQSEL_SIGSEL_WTIMER0CC1 | LDMA_CH_REQSEL_SOURCESEL_WTIMER0,                   ///< Trigger on WTIMER0_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER0CC2)
  ldmaPeripheralSignal_WTIMER0_CC2 = LDMA_CH_REQSEL_SIGSEL_WTIMER0CC2 | LDMA_CH_REQSEL_SOURCESEL_WTIMER0,                   ///< Trigger on WTIMER0_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER0UFOF)
  ldmaPeripheralSignal_WTIMER0_UFOF = LDMA_CH_REQSEL_SIGSEL_WTIMER0UFOF | LDMA_CH_REQSEL_SOURCESEL_WTIMER0,                 ///< Trigger on WTIMER0_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER1CC0)
  ldmaPeripheralSignal_WTIMER1_CC0 = LDMA_CH_REQSEL_SIGSEL_WTIMER1CC0 | LDMA_CH_REQSEL_SOURCESEL_WTIMER1,                   ///< Trigger on WTIMER1_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER1CC1)
  ldmaPeripheralSignal_WTIMER1_CC1 = LDMA_CH_REQSEL_SIGSEL_WTIMER1CC1 | LDMA_CH_REQSEL_SOURCESEL_WTIMER1,                   ///< Trigger on WTIMER1_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER1CC2)
  ldmaPeripheralSignal_WTIMER1_CC2 = LDMA_CH_REQSEL_SIGSEL_WTIMER1CC2 | LDMA_CH_REQSEL_SOURCESEL_WTIMER1,                   ///< Trigger on WTIMER1_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER1CC3)
  ldmaPeripheralSignal_WTIMER1_CC3 = LDMA_CH_REQSEL_SIGSEL_WTIMER1CC3 | LDMA_CH_REQSEL_SOURCESEL_WTIMER1,                   ///< Trigger on WTIMER1_CC3.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER1UFOF)
  ldmaPeripheralSignal_WTIMER1_UFOF = LDMA_CH_REQSEL_SIGSEL_WTIMER1UFOF | LDMA_CH_REQSEL_SOURCESEL_WTIMER1,                 ///< Trigger on WTIMER1_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER2CC0)
  ldmaPeripheralSignal_WTIMER2_CC0 = LDMA_CH_REQSEL_SIGSEL_WTIMER2CC0 | LDMA_CH_REQSEL_SOURCESEL_WTIMER2,                   ///< Trigger on WTIMER2_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER2CC1)
  ldmaPeripheralSignal_WTIMER2_CC1 = LDMA_CH_REQSEL_SIGSEL_WTIMER2CC1 | LDMA_CH_REQSEL_SOURCESEL_WTIMER2,                   ///< Trigger on WTIMER2_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER2CC2)
  ldmaPeripheralSignal_WTIMER2_CC2 = LDMA_CH_REQSEL_SIGSEL_WTIMER2CC2 | LDMA_CH_REQSEL_SOURCESEL_WTIMER2,                   ///< Trigger on WTIMER2_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER2UFOF)
  ldmaPeripheralSignal_WTIMER2_UFOF = LDMA_CH_REQSEL_SIGSEL_WTIMER2UFOF | LDMA_CH_REQSEL_SOURCESEL_WTIMER2,                 ///< Trigger on WTIMER2_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER3CC0)
  ldmaPeripheralSignal_WTIMER3_CC0 = LDMA_CH_REQSEL_SIGSEL_WTIMER3CC0 | LDMA_CH_REQSEL_SOURCESEL_WTIMER3,                   ///< Trigger on WTIMER3_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER3CC1)
  ldmaPeripheralSignal_WTIMER3_CC1 = LDMA_CH_REQSEL_SIGSEL_WTIMER3CC1 | LDMA_CH_REQSEL_SOURCESEL_WTIMER3,                   ///< Trigger on WTIMER3_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER3CC2)
  ldmaPeripheralSignal_WTIMER3_CC2 = LDMA_CH_REQSEL_SIGSEL_WTIMER3CC2 | LDMA_CH_REQSEL_SOURCESEL_WTIMER3,                   ///< Trigger on WTIMER3_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER3UFOF)
  ldmaPeripheralSignal_WTIMER3_UFOF = LDMA_CH_REQSEL_SIGSEL_WTIMER3UFOF | LDMA_CH_REQSEL_SOURCESEL_WTIMER3,                 ///< Trigger on WTIMER3_UFOF.
  #endif
} LDMA_PeripheralSignal_t;

#endif

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/**
 * @brief
 *   DMA descriptor.
 * @details
 *   The LDMA DMA controller supports three different DMA descriptors. Each
 *   consists of four WORDs which map directly onto HW control registers for a
 *   given DMA channel. The three descriptor types are XFER, SYNC and WRI.
 *   Refer to the reference manual for further information.
 */
typedef union {
  /**
   *  TRANSFER DMA descriptor, this is the only descriptor type which can be
   *  used to start a DMA transfer.
   */
  struct {
    uint32_t  structType : 2;   /**< Set to 0 to select XFER descriptor type.        */
    uint32_t  reserved0  : 1;
    uint32_t  structReq  : 1;   /**< DMA transfer trigger during LINKLOAD.           */
    uint32_t  xferCnt    : 11;  /**< Transfer count minus one.                       */
    uint32_t  byteSwap   : 1;   /**< Enable byte swapping transfers.                 */
    uint32_t  blockSize  : 4;   /**< Number of unit transfers per arbitration cycle. */
    uint32_t  doneIfs    : 1;   /**< Generate interrupt when done.                   */
    uint32_t  reqMode    : 1;   /**< Block or cycle transfer selector.               */
    uint32_t  decLoopCnt : 1;   /**< Enable looped transfers.                        */
    uint32_t  ignoreSrec : 1;   /**< Ignore single requests.                         */
    uint32_t  srcInc     : 2;   /**< Source address increment unit size.             */
    uint32_t  size       : 2;   /**< DMA transfer unit size.                         */
    uint32_t  dstInc     : 2;   /**< Destination address increment unit size.        */
    uint32_t  srcAddrMode : 1;  /**< Source addressing mode.                         */
    uint32_t  dstAddrMode : 1;  /**< Destination addressing mode.                    */

    uint32_t  srcAddr;          /**< DMA source address.                             */
    uint32_t  dstAddr;          /**< DMA destination address.                        */

    uint32_t  linkMode   : 1;   /**< Select absolute or relative link address.       */
    uint32_t  link       : 1;   /**< Enable LINKLOAD when transfer is done.          */
    int32_t   linkAddr   : 30;  /**< Address of next (linked) descriptor.            */
  } xfer;

  /** SYNCHRONIZE DMA descriptor, used for intra channel transfer
   *  synchronization.
   */
  struct {
    uint32_t  structType : 2;   /**< Set to 1 to select SYNC descriptor type.        */
    uint32_t  reserved0  : 1;
    uint32_t  structReq  : 1;   /**< DMA transfer trigger during LINKLOAD.           */
    uint32_t  xferCnt    : 11;  /**< Transfer count minus one.                       */
    uint32_t  byteSwap   : 1;   /**< Enable byte swapping transfers.                 */
    uint32_t  blockSize  : 4;   /**< Number of unit transfers per arbitration cycle. */
    uint32_t  doneIfs    : 1;   /**< Generate interrupt when done.                   */
    uint32_t  reqMode    : 1;   /**< Block or cycle transfer selector.               */
    uint32_t  decLoopCnt : 1;   /**< Enable looped transfers.                        */
    uint32_t  ignoreSrec : 1;   /**< Ignore single requests.                         */
    uint32_t  srcInc     : 2;   /**< Source address increment unit size.             */
    uint32_t  size       : 2;   /**< DMA transfer unit size.                         */
    uint32_t  dstInc     : 2;   /**< Destination address increment unit size.        */
    uint32_t  srcAddrMode : 1;  /**< Source addressing mode.                         */
    uint32_t  dstAddrMode : 1;  /**< Destination addressing mode.                    */

    uint32_t  syncSet    : 8;   /**< Set bits in LDMA_CTRL.SYNCTRIG register.        */
    uint32_t  syncClr    : 8;   /**< Clear bits in LDMA_CTRL.SYNCTRIG register.      */
    uint32_t  reserved3  : 16;
    uint32_t  matchVal   : 8;   /**< Sync trigger match value.                       */
    uint32_t  matchEn    : 8;   /**< Sync trigger match enable.                      */
    uint32_t  reserved4  : 16;

    uint32_t  linkMode   : 1;   /**< Select absolute or relative link address.       */
    uint32_t  link       : 1;   /**< Enable LINKLOAD when transfer is done.          */
    int32_t   linkAddr   : 30;  /**< Address of next (linked) descriptor.            */
  } sync;

  /** WRITE DMA descriptor, used for write immediate operations.                     */
  struct {
    uint32_t  structType : 2;   /**< Set to 2 to select WRITE descriptor type.       */
    uint32_t  reserved0  : 1;
    uint32_t  structReq  : 1;   /**< DMA transfer trigger during LINKLOAD.           */
    uint32_t  xferCnt    : 11;  /**< Transfer count minus one.                       */
    uint32_t  byteSwap   : 1;   /**< Enable byte swapping transfers.                 */
    uint32_t  blockSize  : 4;   /**< Number of unit transfers per arbitration cycle. */
    uint32_t  doneIfs    : 1;   /**< Generate interrupt when done.                   */
    uint32_t  reqMode    : 1;   /**< Block or cycle transfer selector.               */
    uint32_t  decLoopCnt : 1;   /**< Enable looped transfers.                        */
    uint32_t  ignoreSrec : 1;   /**< Ignore single requests.                         */
    uint32_t  srcInc     : 2;   /**< Source address increment unit size.             */
    uint32_t  size       : 2;   /**< DMA transfer unit size.                         */
    uint32_t  dstInc     : 2;   /**< Destination address increment unit size.        */
    uint32_t  srcAddrMode : 1;  /**< Source addressing mode.                         */
    uint32_t  dstAddrMode : 1;  /**< Destination addressing mode.                    */

    uint32_t  immVal;           /**< Data to be written at dstAddr.                  */
    uint32_t  dstAddr;          /**< DMA write destination address.                  */

    uint32_t  linkMode   : 1;   /**< Select absolute or relative link address.       */
    uint32_t  link       : 1;   /**< Enable LINKLOAD when transfer is done.          */
    int32_t   linkAddr   : 30;  /**< Address of next (linked) descriptor.            */
  } wri;
} LDMA_Descriptor_t;

/** @brief LDMA initialization configuration structure. */
typedef struct {
  uint8_t               ldmaInitCtrlNumFixed;     /**< Arbitration mode separator. */
  uint8_t               ldmaInitCtrlSyncPrsClrEn; /**< PRS Synctrig clear enable.  */
  uint8_t               ldmaInitCtrlSyncPrsSetEn; /**< PRS Synctrig set enable.    */
  uint8_t               ldmaInitIrqPriority;      /**< LDMA IRQ priority (0..7).   */
} LDMA_Init_t;

/**
 * @brief
 *   DMA transfer configuration structure.
 * @details
 *   This struct configures all aspects of a DMA transfer.
 */
typedef struct {
  uint32_t              ldmaReqSel;            /**< Selects DMA trigger source.                  */
  uint8_t               ldmaCtrlSyncPrsClrOff; /**< PRS Synctrig clear enables to clear.         */
  uint8_t               ldmaCtrlSyncPrsClrOn;  /**< PRS Synctrig clear enables to set.           */
  uint8_t               ldmaCtrlSyncPrsSetOff; /**< PRS Synctrig set enables to clear.           */
  uint8_t               ldmaCtrlSyncPrsSetOn;  /**< PRS Synctrig set enables to set.             */
  bool                  ldmaReqDis;            /**< Mask the PRS trigger input.                  */
  bool                  ldmaDbgHalt;           /**< Dis. DMA trig when CPU is halted.            */
  uint8_t               ldmaCfgArbSlots;       /**< Arbitration slot number.                     */
  uint8_t               ldmaCfgSrcIncSign;     /**< Source address increment sign.               */
  uint8_t               ldmaCfgDstIncSign;     /**< Destination address increment sign.          */
  uint8_t               ldmaLoopCnt;           /**< Counter for looped transfers.                */
} LDMA_TransferCfg_t;

/*******************************************************************************
 **************************   STRUCT INITIALIZERS   ****************************
 ******************************************************************************/

/** @brief Default DMA initialization structure. */
#define LDMA_INIT_DEFAULT                                                                    \
  {                                                                                          \
    .ldmaInitCtrlNumFixed     = _LDMA_CTRL_NUMFIXED_DEFAULT,/* Fixed priority arbitration.*/ \
    .ldmaInitCtrlSyncPrsClrEn = 0,                         /* No PRS Synctrig clear enable*/ \
    .ldmaInitCtrlSyncPrsSetEn = 0,                         /* No PRS Synctrig set enable. */ \
    .ldmaInitIrqPriority      = 3                          /* IRQ priority level 3.       */ \
  }

/**
 * @brief
 *   Generic DMA transfer configuration for memory to memory transfers.
 */
#define LDMA_TRANSFER_CFG_MEMORY()                \
  {                                               \
    0, 0, 0, 0, 0,                                \
    false, false, ldmaCfgArbSlotsAs1,             \
    ldmaCfgSrcIncSignPos, ldmaCfgDstIncSignPos, 0 \
  }

/**
 * @brief
 *   Generic DMA transfer configuration for looped memory to memory transfers.
 */
#define LDMA_TRANSFER_CFG_MEMORY_LOOP(loopCnt)  \
  {                                             \
    0, 0, 0, 0, 0,                              \
    false, false, ldmaCfgArbSlotsAs1,           \
    ldmaCfgSrcIncSignPos, ldmaCfgDstIncSignPos, \
    loopCnt                                     \
  }

/**
 * @brief
 *   Generic DMA transfer configuration for memory to/from peripheral transfers.
 */
#define LDMA_TRANSFER_CFG_PERIPHERAL(signal)      \
  {                                               \
    signal, 0, 0, 0, 0,                           \
    false, false, ldmaCfgArbSlotsAs1,             \
    ldmaCfgSrcIncSignPos, ldmaCfgDstIncSignPos, 0 \
  }

/**
 * @brief
 *   Generic DMA transfer configuration for looped memory to/from peripheral transfers.
 */
#define LDMA_TRANSFER_CFG_PERIPHERAL_LOOP(signal, loopCnt) \
  {                                                        \
    signal, 0, 0, 0, 0,                                    \
    false, false, ldmaCfgArbSlotsAs1,                      \
    ldmaCfgSrcIncSignPos, ldmaCfgDstIncSignPos, loopCnt    \
  }

/**
 * @brief
 *   DMA descriptor initializer for single memory to memory word transfer.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of words to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_M2M_WORD(src, dest, count) \
  {                                                       \
    .xfer =                                               \
    {                                                     \
      .structType   = ldmaCtrlStructTypeXfer,             \
      .structReq    = 1,                                  \
      .xferCnt      = (count) - 1,                        \
      .byteSwap     = 0,                                  \
      .blockSize    = ldmaCtrlBlockSizeUnit1,             \
      .doneIfs      = 1,                                  \
      .reqMode      = ldmaCtrlReqModeAll,                 \
      .decLoopCnt   = 0,                                  \
      .ignoreSrec   = 0,                                  \
      .srcInc       = ldmaCtrlSrcIncOne,                  \
      .size         = ldmaCtrlSizeWord,                   \
      .dstInc       = ldmaCtrlDstIncOne,                  \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,             \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,             \
      .srcAddr      = (uint32_t)(src),                    \
      .dstAddr      = (uint32_t)(dest),                   \
      .linkMode     = 0,                                  \
      .link         = 0,                                  \
      .linkAddr     = 0                                   \
    }                                                     \
  }

/**
 * @brief
 *   DMA descriptor initializer for single memory to memory half-word transfer.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of half-words to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_M2M_HALF(src, dest, count) \
  {                                                       \
    .xfer =                                               \
    {                                                     \
      .structType   = ldmaCtrlStructTypeXfer,             \
      .structReq    = 1,                                  \
      .xferCnt      = (count) - 1,                        \
      .byteSwap     = 0,                                  \
      .blockSize    = ldmaCtrlBlockSizeUnit1,             \
      .doneIfs      = 1,                                  \
      .reqMode      = ldmaCtrlReqModeAll,                 \
      .decLoopCnt   = 0,                                  \
      .ignoreSrec   = 0,                                  \
      .srcInc       = ldmaCtrlSrcIncOne,                  \
      .size         = ldmaCtrlSizeHalf,                   \
      .dstInc       = ldmaCtrlDstIncOne,                  \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,             \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,             \
      .srcAddr      = (uint32_t)(src),                    \
      .dstAddr      = (uint32_t)(dest),                   \
      .linkMode     = 0,                                  \
      .link         = 0,                                  \
      .linkAddr     = 0                                   \
    }                                                     \
  }

/**
 * @brief
 *   DMA descriptor initializer for single memory to memory byte transfer.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of bytes to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_M2M_BYTE(src, dest, count) \
  {                                                       \
    .xfer =                                               \
    {                                                     \
      .structType   = ldmaCtrlStructTypeXfer,             \
      .structReq    = 1,                                  \
      .xferCnt      = (count) - 1,                        \
      .byteSwap     = 0,                                  \
      .blockSize    = ldmaCtrlBlockSizeUnit1,             \
      .doneIfs      = 1,                                  \
      .reqMode      = ldmaCtrlReqModeAll,                 \
      .decLoopCnt   = 0,                                  \
      .ignoreSrec   = 0,                                  \
      .srcInc       = ldmaCtrlSrcIncOne,                  \
      .size         = ldmaCtrlSizeByte,                   \
      .dstInc       = ldmaCtrlDstIncOne,                  \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,             \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,             \
      .srcAddr      = (uint32_t)(src),                    \
      .dstAddr      = (uint32_t)(dest),                   \
      .linkMode     = 0,                                  \
      .link         = 0,                                  \
      .linkAddr     = 0                                   \
    }                                                     \
  }

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory word transfer.
 *
 *   Link address must be an absolute address.
 * @note
 *   The linkAddr member of the transfer descriptor is not
 *   initialized.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of words to transfer.
 */
#define LDMA_DESCRIPTOR_LINKABS_M2M_WORD(src, dest, count) \
  {                                                        \
    .xfer =                                                \
    {                                                      \
      .structType   = ldmaCtrlStructTypeXfer,              \
      .structReq    = 1,                                   \
      .xferCnt      = (count) - 1,                         \
      .byteSwap     = 0,                                   \
      .blockSize    = ldmaCtrlBlockSizeUnit1,              \
      .doneIfs      = 0,                                   \
      .reqMode      = ldmaCtrlReqModeAll,                  \
      .decLoopCnt   = 0,                                   \
      .ignoreSrec   = 0,                                   \
      .srcInc       = ldmaCtrlSrcIncOne,                   \
      .size         = ldmaCtrlSizeWord,                    \
      .dstInc       = ldmaCtrlDstIncOne,                   \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,              \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,              \
      .srcAddr      = (uint32_t)(src),                     \
      .dstAddr      = (uint32_t)(dest),                    \
      .linkMode     = ldmaLinkModeAbs,                     \
      .link         = 1,                                   \
      .linkAddr     = 0 /* Must be set runtime ! */        \
    }                                                      \
  }

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory half-word transfer.
 *
 *   Link address must be an absolute address.
 * @note
 *   The linkAddr member of the transfer descriptor is not
 *   initialized.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of half-words to transfer.
 */
#define LDMA_DESCRIPTOR_LINKABS_M2M_HALF(src, dest, count) \
  {                                                        \
    .xfer =                                                \
    {                                                      \
      .structType   = ldmaCtrlStructTypeXfer,              \
      .structReq    = 1,                                   \
      .xferCnt      = (count) - 1,                         \
      .byteSwap     = 0,                                   \
      .blockSize    = ldmaCtrlBlockSizeUnit1,              \
      .doneIfs      = 0,                                   \
      .reqMode      = ldmaCtrlReqModeAll,                  \
      .decLoopCnt   = 0,                                   \
      .ignoreSrec   = 0,                                   \
      .srcInc       = ldmaCtrlSrcIncOne,                   \
      .size         = ldmaCtrlSizeHalf,                    \
      .dstInc       = ldmaCtrlDstIncOne,                   \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,              \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,              \
      .srcAddr      = (uint32_t)(src),                     \
      .dstAddr      = (uint32_t)(dest),                    \
      .linkMode     = ldmaLinkModeAbs,                     \
      .link         = 1,                                   \
      .linkAddr     = 0 /* Must be set runtime ! */        \
    }                                                      \
  }

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory byte transfer.
 *
 *   Link address must be an absolute address.
 * @note
 *   The linkAddr member of the transfer descriptor is not
 *   initialized.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of bytes to transfer.
 */
#define LDMA_DESCRIPTOR_LINKABS_M2M_BYTE(src, dest, count) \
  {                                                        \
    .xfer =                                                \
    {                                                      \
      .structType   = ldmaCtrlStructTypeXfer,              \
      .structReq    = 1,                                   \
      .xferCnt      = (count) - 1,                         \
      .byteSwap     = 0,                                   \
      .blockSize    = ldmaCtrlBlockSizeUnit1,              \
      .doneIfs      = 0,                                   \
      .reqMode      = ldmaCtrlReqModeAll,                  \
      .decLoopCnt   = 0,                                   \
      .ignoreSrec   = 0,                                   \
      .srcInc       = ldmaCtrlSrcIncOne,                   \
      .size         = ldmaCtrlSizeByte,                    \
      .dstInc       = ldmaCtrlDstIncOne,                   \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,              \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,              \
      .srcAddr      = (uint32_t)(src),                     \
      .dstAddr      = (uint32_t)(dest),                    \
      .linkMode     = ldmaLinkModeAbs,                     \
      .link         = 1,                                   \
      .linkAddr     = 0 /* Must be set runtime ! */        \
    }                                                      \
  }

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory word transfer.
 *
 *   Link address is a relative address.
 * @note
 *   The linkAddr member of the transfer descriptor is initialized to 4,
 *   assuming that the next descriptor immediately follows
 *   this descriptor (in memory).
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of words to transfer.
 * @param[in] linkjmp   Address of descriptor to link to, expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_M2M_WORD(src, dest, count, linkjmp) \
  {                                                                 \
    .xfer =                                                         \
    {                                                               \
      .structType   = ldmaCtrlStructTypeXfer,                       \
      .structReq    = 1,                                            \
      .xferCnt      = (count) - 1,                                  \
      .byteSwap     = 0,                                            \
      .blockSize    = ldmaCtrlBlockSizeUnit1,                       \
      .doneIfs      = 0,                                            \
      .reqMode      = ldmaCtrlReqModeAll,                           \
      .decLoopCnt   = 0,                                            \
      .ignoreSrec   = 0,                                            \
      .srcInc       = ldmaCtrlSrcIncOne,                            \
      .size         = ldmaCtrlSizeWord,                             \
      .dstInc       = ldmaCtrlDstIncOne,                            \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                       \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                       \
      .srcAddr      = (uint32_t)(src),                              \
      .dstAddr      = (uint32_t)(dest),                             \
      .linkMode     = ldmaLinkModeRel,                              \
      .link         = 1,                                            \
      .linkAddr     = (linkjmp) * 4                                 \
    }                                                               \
  }

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory half-word transfer.
 *
 *   Link address is a relative address.
 * @note
 *   The linkAddr member of transfer descriptor is initialized to 4,
 *   assuming that the next descriptor immediately follows
 *   this descriptor (in memory).
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of half-words to transfer.
 * @param[in] linkjmp   Address of descriptor to link to, expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_M2M_HALF(src, dest, count, linkjmp) \
  {                                                                 \
    .xfer =                                                         \
    {                                                               \
      .structType   = ldmaCtrlStructTypeXfer,                       \
      .structReq    = 1,                                            \
      .xferCnt      = (count) - 1,                                  \
      .byteSwap     = 0,                                            \
      .blockSize    = ldmaCtrlBlockSizeUnit1,                       \
      .doneIfs      = 0,                                            \
      .reqMode      = ldmaCtrlReqModeAll,                           \
      .decLoopCnt   = 0,                                            \
      .ignoreSrec   = 0,                                            \
      .srcInc       = ldmaCtrlSrcIncOne,                            \
      .size         = ldmaCtrlSizeHalf,                             \
      .dstInc       = ldmaCtrlDstIncOne,                            \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                       \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                       \
      .srcAddr      = (uint32_t)(src),                              \
      .dstAddr      = (uint32_t)(dest),                             \
      .linkMode     = ldmaLinkModeRel,                              \
      .link         = 1,                                            \
      .linkAddr     = (linkjmp) * 4                                 \
    }                                                               \
  }

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory byte transfer.
 *
 *   Link address is a relative address.
 * @note
 *   The linkAddr member of transfer descriptor is initialized to 4,
 *   assuming that the next descriptor immediately follows
 *   this descriptor (in memory).
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of bytes to transfer.
 * @param[in] linkjmp   Address of descriptor to link to, expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_M2M_BYTE(src, dest, count, linkjmp) \
  {                                                                 \
    .xfer =                                                         \
    {                                                               \
      .structType   = ldmaCtrlStructTypeXfer,                       \
      .structReq    = 1,                                            \
      .xferCnt      = (count) - 1,                                  \
      .byteSwap     = 0,                                            \
      .blockSize    = ldmaCtrlBlockSizeUnit1,                       \
      .doneIfs      = 0,                                            \
      .reqMode      = ldmaCtrlReqModeAll,                           \
      .decLoopCnt   = 0,                                            \
      .ignoreSrec   = 0,                                            \
      .srcInc       = ldmaCtrlSrcIncOne,                            \
      .size         = ldmaCtrlSizeByte,                             \
      .dstInc       = ldmaCtrlDstIncOne,                            \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                       \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                       \
      .srcAddr      = (uint32_t)(src),                              \
      .dstAddr      = (uint32_t)(dest),                             \
      .linkMode     = ldmaLinkModeRel,                              \
      .link         = 1,                                            \
      .linkAddr     = (linkjmp) * 4                                 \
    }                                                               \
  }

/**
 * @brief
 *   DMA descriptor initializer for byte transfers from a peripheral to memory.
 * @param[in] src       Peripheral data source register address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of bytes to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_P2M_BYTE(src, dest, count) \
  {                                                       \
    .xfer =                                               \
    {                                                     \
      .structType   = ldmaCtrlStructTypeXfer,             \
      .structReq    = 0,                                  \
      .xferCnt      = (count) - 1,                        \
      .byteSwap     = 0,                                  \
      .blockSize    = ldmaCtrlBlockSizeUnit1,             \
      .doneIfs      = 1,                                  \
      .reqMode      = ldmaCtrlReqModeBlock,               \
      .decLoopCnt   = 0,                                  \
      .ignoreSrec   = 0,                                  \
      .srcInc       = ldmaCtrlSrcIncNone,                 \
      .size         = ldmaCtrlSizeByte,                   \
      .dstInc       = ldmaCtrlDstIncOne,                  \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,             \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,             \
      .srcAddr      = (uint32_t)(src),                    \
      .dstAddr      = (uint32_t)(dest),                   \
      .linkMode     = 0,                                  \
      .link         = 0,                                  \
      .linkAddr     = 0                                   \
    }                                                     \
  }

/**
 * @brief
 *   DMA descriptor initializer for byte transfers from a peripheral to a peripheral.
 * @param[in] src       Peripheral data source register address.
 * @param[in] dest      Peripheral data destination register address.
 * @param[in] count     Number of bytes to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_P2P_BYTE(src, dest, count) \
  {                                                       \
    .xfer =                                               \
    {                                                     \
      .structType   = ldmaCtrlStructTypeXfer,             \
      .structReq    = 0,                                  \
      .xferCnt      = (count) - 1,                        \
      .byteSwap     = 0,                                  \
      .blockSize    = ldmaCtrlBlockSizeUnit1,             \
      .doneIfs      = 1,                                  \
      .reqMode      = ldmaCtrlReqModeBlock,               \
      .decLoopCnt   = 0,                                  \
      .ignoreSrec   = 0,                                  \
      .srcInc       = ldmaCtrlSrcIncNone,                 \
      .size         = ldmaCtrlSizeByte,                   \
      .dstInc       = ldmaCtrlDstIncNone,                 \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,             \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,             \
      .srcAddr      = (uint32_t)(src),                    \
      .dstAddr      = (uint32_t)(dest),                   \
      .linkMode     = 0,                                  \
      .link         = 0,                                  \
      .linkAddr     = 0                                   \
    }                                                     \
  }

/**
 * @brief
 *   DMA descriptor initializer for byte transfers from memory to a peripheral
 * @param[in] src       Source data address.
 * @param[in] dest      Peripheral data register destination address.
 * @param[in] count     Number of bytes to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(src, dest, count) \
  {                                                       \
    .xfer =                                               \
    {                                                     \
      .structType   = ldmaCtrlStructTypeXfer,             \
      .structReq    = 0,                                  \
      .xferCnt      = (count) - 1,                        \
      .byteSwap     = 0,                                  \
      .blockSize    = ldmaCtrlBlockSizeUnit1,             \
      .doneIfs      = 1,                                  \
      .reqMode      = ldmaCtrlReqModeBlock,               \
      .decLoopCnt   = 0,                                  \
      .ignoreSrec   = 0,                                  \
      .srcInc       = ldmaCtrlSrcIncOne,                  \
      .size         = ldmaCtrlSizeByte,                   \
      .dstInc       = ldmaCtrlDstIncNone,                 \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,             \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,             \
      .srcAddr      = (uint32_t)(src),                    \
      .dstAddr      = (uint32_t)(dest),                   \
      .linkMode     = 0,                                  \
      .link         = 0,                                  \
      .linkAddr     = 0                                   \
    }                                                     \
  }

/**
 * @brief
 *   DMA descriptor initializer for byte transfers from a peripheral to memory.
 * @param[in] src       Peripheral data source register address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of bytes to transfer.
 * @param[in] linkjmp   Address of descriptor to link to, expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(src, dest, count, linkjmp) \
  {                                                                 \
    .xfer =                                                         \
    {                                                               \
      .structType   = ldmaCtrlStructTypeXfer,                       \
      .structReq    = 0,                                            \
      .xferCnt      = (count) - 1,                                  \
      .byteSwap     = 0,                                            \
      .blockSize    = ldmaCtrlBlockSizeUnit1,                       \
      .doneIfs      = 1,                                            \
      .reqMode      = ldmaCtrlReqModeBlock,                         \
      .decLoopCnt   = 0,                                            \
      .ignoreSrec   = 0,                                            \
      .srcInc       = ldmaCtrlSrcIncNone,                           \
      .size         = ldmaCtrlSizeByte,                             \
      .dstInc       = ldmaCtrlDstIncOne,                            \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                       \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                       \
      .srcAddr      = (uint32_t)(src),                              \
      .dstAddr      = (uint32_t)(dest),                             \
      .linkMode     = ldmaLinkModeRel,                              \
      .link         = 1,                                            \
      .linkAddr     = (linkjmp) * 4                                 \
    }                                                               \
  }

/**
 * @brief
 *   DMA descriptor initializer for byte transfers from memory to a peripheral
 * @param[in] src       Source data address.
 * @param[in] dest      Peripheral data register destination address.
 * @param[in] count     Number of bytes to transfer.
 * @param[in] linkjmp   Address of descriptor to link to, expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(src, dest, count, linkjmp) \
  {                                                                 \
    .xfer =                                                         \
    {                                                               \
      .structType   = ldmaCtrlStructTypeXfer,                       \
      .structReq    = 0,                                            \
      .xferCnt      = (count) - 1,                                  \
      .byteSwap     = 0,                                            \
      .blockSize    = ldmaCtrlBlockSizeUnit1,                       \
      .doneIfs      = 1,                                            \
      .reqMode      = ldmaCtrlReqModeBlock,                         \
      .decLoopCnt   = 0,                                            \
      .ignoreSrec   = 0,                                            \
      .srcInc       = ldmaCtrlSrcIncOne,                            \
      .size         = ldmaCtrlSizeByte,                             \
      .dstInc       = ldmaCtrlDstIncNone,                           \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                       \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                       \
      .srcAddr      = (uint32_t)(src),                              \
      .dstAddr      = (uint32_t)(dest),                             \
      .linkMode     = ldmaLinkModeRel,                              \
      .link         = 1,                                            \
      .linkAddr     = (linkjmp) * 4                                 \
    }                                                               \
  }

/**
 * @brief
 *   DMA descriptor initializer for Immediate WRITE transfer
 * @param[in] value     Immediate value to write.
 * @param[in] address   Write address.
 */
#define LDMA_DESCRIPTOR_SINGLE_WRITE(value, address) \
  {                                                  \
    .wri =                                           \
    {                                                \
      .structType   = ldmaCtrlStructTypeWrite,       \
      .structReq    = 1,                             \
      .xferCnt      = 0,                             \
      .byteSwap     = 0,                             \
      .blockSize    = 0,                             \
      .doneIfs      = 1,                             \
      .reqMode      = 0,                             \
      .decLoopCnt   = 0,                             \
      .ignoreSrec   = 0,                             \
      .srcInc       = 0,                             \
      .size         = 0,                             \
      .dstInc       = 0,                             \
      .srcAddrMode  = 0,                             \
      .dstAddrMode  = 0,                             \
      .immVal       = (value),                       \
      .dstAddr      = (uint32_t)(address),           \
      .linkMode     = 0,                             \
      .link         = 0,                             \
      .linkAddr     = 0                              \
    }                                                \
  }

/**
 * @brief
 *   DMA descriptor initializer for Immediate WRITE transfer
 *
 *   Link address must be an absolute address.
 * @note
 *   The linkAddr member of the transfer descriptor is not
 *   initialized.
 * @param[in] value     Immediate value to write.
 * @param[in] address   Write address.
 */
#define LDMA_DESCRIPTOR_LINKABS_WRITE(value, address) \
  {                                                   \
    .wri =                                            \
    {                                                 \
      .structType   = ldmaCtrlStructTypeWrite,        \
      .structReq    = 1,                              \
      .xferCnt      = 0,                              \
      .byteSwap     = 0,                              \
      .blockSize    = 0,                              \
      .doneIfs      = 0,                              \
      .reqMode      = 0,                              \
      .decLoopCnt   = 0,                              \
      .ignoreSrec   = 0,                              \
      .srcInc       = 0,                              \
      .size         = 0,                              \
      .dstInc       = 0,                              \
      .srcAddrMode  = 0,                              \
      .dstAddrMode  = 0,                              \
      .immVal       = (value),                        \
      .dstAddr      = (uint32_t)(address),            \
      .linkMode     = ldmaLinkModeAbs,                \
      .link         = 1,                              \
      .linkAddr     = 0 /* Must be set runtime ! */   \
    }                                                 \
  }

/**
 * @brief
 *   DMA descriptor initializer for Immediate WRITE transfer
 * @param[in] value     Immediate value to write.
 * @param[in] address   Write address.
 * @param[in] linkjmp   Address of descriptor to link to, expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_WRITE(value, address, linkjmp) \
  {                                                            \
    .wri =                                                     \
    {                                                          \
      .structType   = ldmaCtrlStructTypeWrite,                 \
      .structReq    = 1,                                       \
      .xferCnt      = 0,                                       \
      .byteSwap     = 0,                                       \
      .blockSize    = 0,                                       \
      .doneIfs      = 0,                                       \
      .reqMode      = 0,                                       \
      .decLoopCnt   = 0,                                       \
      .ignoreSrec   = 0,                                       \
      .srcInc       = 0,                                       \
      .size         = 0,                                       \
      .dstInc       = 0,                                       \
      .srcAddrMode  = 0,                                       \
      .dstAddrMode  = 0,                                       \
      .immVal       = (value),                                 \
      .dstAddr      = (uint32_t)(address),                     \
      .linkMode     = ldmaLinkModeRel,                         \
      .link         = 1,                                       \
      .linkAddr     = (linkjmp) * 4                            \
    }                                                          \
  }

/**
 * @brief
 *   DMA descriptor initializer for SYNC transfer
 * @param[in] set          Sync pattern bits to set.
 * @param[in] clr          Sync pattern bits to clear.
 * @param[in] matchValue   Sync pattern to match.
 * @param[in] matchEnable  Sync pattern bits to enable for match.
 */
#define LDMA_DESCRIPTOR_SINGLE_SYNC(set, clr, matchValue, matchEnable) \
  {                                                                    \
    .sync =                                                            \
    {                                                                  \
      .structType   = ldmaCtrlStructTypeSync,                          \
      .structReq    = 1,                                               \
      .xferCnt      = 0,                                               \
      .byteSwap     = 0,                                               \
      .blockSize    = 0,                                               \
      .doneIfs      = 1,                                               \
      .reqMode      = 0,                                               \
      .decLoopCnt   = 0,                                               \
      .ignoreSrec   = 0,                                               \
      .srcInc       = 0,                                               \
      .size         = 0,                                               \
      .dstInc       = 0,                                               \
      .srcAddrMode  = 0,                                               \
      .dstAddrMode  = 0,                                               \
      .syncSet      = (set),                                           \
      .syncClr      = (clr),                                           \
      .matchVal     = (matchValue),                                    \
      .matchEn      = (matchEnable),                                   \
      .linkMode     = 0,                                               \
      .link         = 0,                                               \
      .linkAddr     = 0                                                \
    }                                                                  \
  }

/**
 * @brief
 *   DMA descriptor initializer for SYNC transfer
 *
 *   Link address must be an absolute address.
 * @note
 *   The linkAddr member of the transfer descriptor is not
 *   initialized.
 * @param[in] set          Sync pattern bits to set.
 * @param[in] clr          Sync pattern bits to clear.
 * @param[in] matchValue   Sync pattern to match.
 * @param[in] matchEnable  Sync pattern bits to enable for match.
 */
#define LDMA_DESCRIPTOR_LINKABS_SYNC(set, clr, matchValue, matchEnable) \
  {                                                                     \
    .sync =                                                             \
    {                                                                   \
      .structType   = ldmaCtrlStructTypeSync,                           \
      .structReq    = 1,                                                \
      .xferCnt      = 0,                                                \
      .byteSwap     = 0,                                                \
      .blockSize    = 0,                                                \
      .doneIfs      = 0,                                                \
      .reqMode      = 0,                                                \
      .decLoopCnt   = 0,                                                \
      .ignoreSrec   = 0,                                                \
      .srcInc       = 0,                                                \
      .size         = 0,                                                \
      .dstInc       = 0,                                                \
      .srcAddrMode  = 0,                                                \
      .dstAddrMode  = 0,                                                \
      .syncSet      = (set),                                            \
      .syncClr      = (clr),                                            \
      .matchVal     = (matchValue),                                     \
      .matchEn      = (matchEnable),                                    \
      .linkMode     = ldmaLinkModeAbs,                                  \
      .link         = 1,                                                \
      .linkAddr     = 0 /* Must be set runtime ! */                     \
    }                                                                   \
  }

/**
 * @brief
 *   DMA descriptor initializer for SYNC transfer
 * @param[in] set          Sync pattern bits to set.
 * @param[in] clr          Sync pattern bits to clear.
 * @param[in] matchValue   Sync pattern to match.
 * @param[in] matchEnable  Sync pattern bits to enable for match.
 * @param[in] linkjmp   Address of descriptor to link to, expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_SYNC(set, clr, matchValue, matchEnable, linkjmp) \
  {                                                                              \
    .sync =                                                                      \
    {                                                                            \
      .structType   = ldmaCtrlStructTypeSync,                                    \
      .structReq    = 1,                                                         \
      .xferCnt      = 0,                                                         \
      .byteSwap     = 0,                                                         \
      .blockSize    = 0,                                                         \
      .doneIfs      = 0,                                                         \
      .reqMode      = 0,                                                         \
      .decLoopCnt   = 0,                                                         \
      .ignoreSrec   = 0,                                                         \
      .srcInc       = 0,                                                         \
      .size         = 0,                                                         \
      .dstInc       = 0,                                                         \
      .srcAddrMode  = 0,                                                         \
      .dstAddrMode  = 0,                                                         \
      .syncSet      = (set),                                                     \
      .syncClr      = (clr),                                                     \
      .matchVal     = (matchValue),                                              \
      .matchEn      = (matchEnable),                                             \
      .linkMode     = ldmaLinkModeRel,                                           \
      .link         = 1,                                                         \
      .linkAddr     = (linkjmp) * 4                                              \
    }                                                                            \
  }

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

void LDMA_DeInit(void);
void LDMA_EnableChannelRequest(int ch, bool enable);
void LDMA_Init(const LDMA_Init_t *init);
void LDMA_StartTransfer(int ch,
                        const LDMA_TransferCfg_t *transfer,
                        const LDMA_Descriptor_t  *descriptor);
void LDMA_StopTransfer(int ch);
bool LDMA_TransferDone(int ch);
uint32_t LDMA_TransferRemainingCount(int ch);

/***************************************************************************//**
 * @brief
 *   Check if a certain channel is enabled.
 *
 * @param[in] ch
 *   LDMA channel to check.
 *
 * @return
 *   return true if the LDMA channel is enabled and false if the channel is not
 *   enabled.
 ******************************************************************************/
__STATIC_INLINE bool LDMA_ChannelEnabled(int ch)
{
  if ((ch < 0) || (ch > 31)) {
    return false;
  }
#if defined(_LDMA_CHSTATUS_MASK)
  return LDMA->CHSTATUS & (1 << ch);
#else
  // We've already confirmed ch is between 0 and 31,
  // so it's now safe to cast it to uint8_t
  return LDMA->CHEN & (1 << (uint8_t)ch);
#endif
}

/***************************************************************************//**
 * @brief
 *   Clear one or more pending LDMA interrupts.
 *
 * @param[in] flags
 *   Pending LDMA interrupt sources to clear. Use one or more valid
 *   interrupt flags for the LDMA module. The flags are @ref LDMA_IFC_ERROR
 *   and one done flag for each channel.
 ******************************************************************************/
__STATIC_INLINE void LDMA_IntClear(uint32_t flags)
{
#if defined (LDMA_HAS_SET_CLEAR)
  LDMA->IF_CLR = flags;
#else
  LDMA->IFC = flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Disable one or more LDMA interrupts.
 *
 * @param[in] flags
 *   LDMA interrupt sources to disable. Use one or more valid
 *   interrupt flags for LDMA module. The flags are @ref LDMA_IEN_ERROR
 *   and one done flag for each channel.
 ******************************************************************************/
__STATIC_INLINE void LDMA_IntDisable(uint32_t flags)
{
  LDMA->IEN &= ~flags;
}

/***************************************************************************//**
 * @brief
 *   Enable one or more LDMA interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. To ignore a pending interrupt, consider using
 *   LDMA_IntClear() prior to enabling the interrupt.
 *
 * @param[in] flags
 *   LDMA interrupt sources to enable. Use one or more valid
 *   interrupt flags for LDMA module. The flags are @ref LDMA_IEN_ERROR
 *   and one done flag for each channel.
 ******************************************************************************/
__STATIC_INLINE void LDMA_IntEnable(uint32_t flags)
{
  LDMA->IEN |= flags;
}

/***************************************************************************//**
 * @brief
 *   Get pending LDMA interrupt flags.
 *
 * @note
 *   Event bits are not cleared by the use of this function.
 *
 * @return
 *   LDMA interrupt sources pending. Returns one or more valid
 *   interrupt flags for LDMA module. The flags are @ref LDMA_IF_ERROR and
 *   one flag for each LDMA channel.
 ******************************************************************************/
__STATIC_INLINE uint32_t LDMA_IntGet(void)
{
  return LDMA->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending LDMA interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   Interrupt flags are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled LDMA interrupt sources
 *   Return value is the bitwise AND of
 *   - the enabled interrupt sources in LDMA_IEN and
 *   - the pending interrupt flags LDMA_IF
 ******************************************************************************/
__STATIC_INLINE uint32_t LDMA_IntGetEnabled(void)
{
  uint32_t ien;

  ien = LDMA->IEN;
  return LDMA->IF & ien;
}

/***************************************************************************//**
 * @brief
 *   Set one or more pending LDMA interrupts
 *
 * @param[in] flags
 *   LDMA interrupt sources to set to pending. Use one or more valid
 *   interrupt flags for LDMA module. The flags are @ref LDMA_IFS_ERROR and
 *   one done flag for each LDMA channel.
 ******************************************************************************/
__STATIC_INLINE void LDMA_IntSet(uint32_t flags)
{
#if defined (LDMA_HAS_SET_CLEAR)
  LDMA->IF_SET = flags;
#else
  LDMA->IFS = flags;
#endif
}

/** @} (end addtogroup LDMA) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined( LDMA_PRESENT ) && ( LDMA_COUNT == 1 ) */
#endif /* EM_LDMA_H */
