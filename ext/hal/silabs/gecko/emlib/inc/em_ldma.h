/***************************************************************************//**
 * @file em_ldma.h
 * @brief Direct memory access (LDMA) API
 * @version 5.1.2
 *******************************************************************************
 * @section License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
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

#if defined( LDMA_PRESENT ) && ( LDMA_COUNT == 1 )

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
 * The LDMA API functions provide full support for the LDMA peripheral.
 *
 * The LDMA supports these DMA transfer types:
 *
 * @li Memory to memory.
 * @li Memory to peripheral.
 * @li Peripheral to memory.
 * @li Peripheral to peripheral.
 * @li Constant value to memory.
 *
 * The LDMA supports linked lists of DMA descriptors allowing:
 *
 * @li Circular and ping-pong buffer transfers.
 * @li Scatter-gather transfers.
 * @li Looped transfers.
 *
 * The LDMA has some advanced features:
 *
 * @li Intra-channel synchronization (SYNC), allowing hardware events to
 *     pause and restart a DMA sequence.
 * @li Immediate-write (WRI), allowing the DMA to write a constant anywhere
 *     in the memory map.
 * @li Complex flow control allowing if-else constructs.
 *
 * A basic understanding of the LDMA controller is assumed. Please refer to
 * the reference manual for further details. The LDMA examples described
 * in the reference manual are particularly helpful in understanding LDMA
 * operations.
 *
 * In order to use the DMA controller, the initialization function @ref
 * LDMA_Init() must have been executed once (normally during system init).
 *
 * DMA transfers are initiated by a call to @ref LDMA_StartTransfer(), the
 * transfer properties are controlled by the contents of @ref LDMA_TransferCfg_t
 * and @ref LDMA_Descriptor_t structure parameters.
 * The @htmlonly LDMA_Descriptor_t @endhtmlonly structure parameter may be a
 * pointer to an array of descriptors, the descriptors in the array should
 * be linked together as needed.
 *
 * Transfer and descriptor initialization macros are provided for the most common
 * transfer types. Due to the flexibility of the LDMA peripheral only a small
 * subset of all possible initializer macros are provided, the user should create
 * new one's when needed.
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
 * @n Ping pong DMA from serial port peripheral to memory:
 *
 * @include em_ldma_pingpong.c
 *
 * @note The LDMA module does not implement the LDMA interrupt handler. A
 * template for an LDMA IRQ handler is include here as an example.
 *
 * @include em_ldma_irq.c
 *
 * @{
 ******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/**
 * This value controls the number of unit data transfers per arbitration
 * cycle, providing a means to balance DMA channels' load on the controller.
 */
typedef enum
{
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
typedef enum
{
  ldmaCtrlStructTypeXfer  = _LDMA_CH_CTRL_STRUCTTYPE_TRANSFER,    /**< TRANSFER transfer type.    */
  ldmaCtrlStructTypeSync  = _LDMA_CH_CTRL_STRUCTTYPE_SYNCHRONIZE, /**< SYNCHRONIZE transfer type. */
  ldmaCtrlStructTypeWrite = _LDMA_CH_CTRL_STRUCTTYPE_WRITE        /**< WRITE transfer type.       */
} LDMA_CtrlStructType_t;

/** DMA transfer block or cycle selector. */
typedef enum
{
  ldmaCtrlReqModeBlock = _LDMA_CH_CTRL_REQMODE_BLOCK, /**< Each DMA request trigger transfer of one block.     */
  ldmaCtrlReqModeAll   = _LDMA_CH_CTRL_REQMODE_ALL    /**< A DMA request trigger transfer of a complete cycle. */
} LDMA_CtrlReqMode_t;

/** Source address increment unit size. */
typedef enum
{
  ldmaCtrlSrcIncOne  = _LDMA_CH_CTRL_SRCINC_ONE,  /**< Increment source address by one unit data size.   */
  ldmaCtrlSrcIncTwo  = _LDMA_CH_CTRL_SRCINC_TWO,  /**< Increment source address by two unit data sizes.  */
  ldmaCtrlSrcIncFour = _LDMA_CH_CTRL_SRCINC_FOUR, /**< Increment source address by four unit data sizes. */
  ldmaCtrlSrcIncNone = _LDMA_CH_CTRL_SRCINC_NONE  /**< Do not increment the source address.              */
} LDMA_CtrlSrcInc_t;

/** DMA transfer unit size. */
typedef enum
{
  ldmaCtrlSizeByte = _LDMA_CH_CTRL_SIZE_BYTE,     /**< Each unit transfer is a byte.      */
  ldmaCtrlSizeHalf = _LDMA_CH_CTRL_SIZE_HALFWORD, /**< Each unit transfer is a half-word. */
  ldmaCtrlSizeWord = _LDMA_CH_CTRL_SIZE_WORD      /**< Each unit transfer is a word.      */
} LDMA_CtrlSize_t;

/** Destination address increment unit size. */
typedef enum
{
  ldmaCtrlDstIncOne  = _LDMA_CH_CTRL_DSTINC_ONE,  /**< Increment destination address by one unit data size.   */
  ldmaCtrlDstIncTwo  = _LDMA_CH_CTRL_DSTINC_TWO,  /**< Increment destination address by two unit data sizes.  */
  ldmaCtrlDstIncFour = _LDMA_CH_CTRL_DSTINC_FOUR, /**< Increment destination address by four unit data sizes. */
  ldmaCtrlDstIncNone = _LDMA_CH_CTRL_DSTINC_NONE  /**< Do not increment the destination address.              */
} LDMA_CtrlDstInc_t;

/** Source addressing mode. */
typedef enum
{
  ldmaCtrlSrcAddrModeAbs = _LDMA_CH_CTRL_SRCMODE_ABSOLUTE, /**< Address fetched from a linked structure is absolute.  */
  ldmaCtrlSrcAddrModeRel = _LDMA_CH_CTRL_SRCMODE_RELATIVE  /**< Address fetched from a linked structure is relative.  */
} LDMA_CtrlSrcAddrMode_t;

/** Destination addressing mode. */
typedef enum
{
  ldmaCtrlDstAddrModeAbs = _LDMA_CH_CTRL_DSTMODE_ABSOLUTE, /**< Address fetched from a linked structure is absolute.  */
  ldmaCtrlDstAddrModeRel = _LDMA_CH_CTRL_DSTMODE_RELATIVE  /**< Address fetched from a linked structure is relative.  */
} LDMA_CtrlDstAddrMode_t;

/** DMA linkload address mode. */
typedef enum
{
  ldmaLinkModeAbs = _LDMA_CH_LINK_LINKMODE_ABSOLUTE, /**< Link address is an absolute address value.            */
  ldmaLinkModeRel = _LDMA_CH_LINK_LINKMODE_RELATIVE  /**< Link address is a two's complement releative address. */
} LDMA_LinkMode_t;

/** Insert extra arbitration slots to increase channel arbitration priority. */
typedef enum
{
  ldmaCfgArbSlotsAs1 = _LDMA_CH_CFG_ARBSLOTS_ONE,  /**< One arbitration slot selected.    */
  ldmaCfgArbSlotsAs2 = _LDMA_CH_CFG_ARBSLOTS_TWO,  /**< Two arbitration slots selected.   */
  ldmaCfgArbSlotsAs4 = _LDMA_CH_CFG_ARBSLOTS_FOUR, /**< Four arbitration slots selected.  */
  ldmaCfgArbSlotsAs8 = _LDMA_CH_CFG_ARBSLOTS_EIGHT /**< Eight arbitration slots selected. */
} LDMA_CfgArbSlots_t;

/** Source address increment sign. */
typedef enum
{
  ldmaCfgSrcIncSignPos = _LDMA_CH_CFG_SRCINCSIGN_POSITIVE, /**< Increment source address. */
  ldmaCfgSrcIncSignNeg = _LDMA_CH_CFG_SRCINCSIGN_NEGATIVE  /**< Decrement source address. */
} LDMA_CfgSrcIncSign_t;

/** Destination address increment sign. */
typedef enum
{
  ldmaCfgDstIncSignPos = _LDMA_CH_CFG_DSTINCSIGN_POSITIVE, /**< Increment destination address. */
  ldmaCfgDstIncSignNeg = _LDMA_CH_CFG_DSTINCSIGN_NEGATIVE  /**< Decrement destination address. */
} LDMA_CfgDstIncSign_t;

/** Peripherals that can trigger LDMA transfers. */
typedef enum
{
  ldmaPeripheralSignal_NONE = LDMA_CH_REQSEL_SOURCESEL_NONE,                                                                ///< No peripheral selected for DMA triggering.
  #if defined(LDMA_CH_REQSEL_SIGSEL_ADC0SCAN)
  ldmaPeripheralSignal_ADC0_SCAN = LDMA_CH_REQSEL_SIGSEL_ADC0SCAN | LDMA_CH_REQSEL_SOURCESEL_ADC0,                          ///< Trig on ADC0_SCAN.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_ADC0SINGLE)
  ldmaPeripheralSignal_ADC0_SINGLE = LDMA_CH_REQSEL_SIGSEL_ADC0SINGLE | LDMA_CH_REQSEL_SOURCESEL_ADC0,                      ///< Trig on ADC0_SINGLE.
  #endif
  #if defined( LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0RD )
  ldmaPeripheralSignal_CRYPTO_DATA0RD = LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO,              ///< Trig on CRYPTO_DATA0RD.
  #endif
  #if defined( LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0WR )
  ldmaPeripheralSignal_CRYPTO_DATA0WR = LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO,              ///< Trig on CRYPTO_DATA0WR.
  #endif
  #if defined( LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0XWR )
  ldmaPeripheralSignal_CRYPTO_DATA0XWR = LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0XWR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO,            ///< Trig on CRYPTO_DATA0XWR.
  #endif
  #if defined( LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1RD )
  ldmaPeripheralSignal_CRYPTO_DATA1RD = LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO,              ///< Trig on CRYPTO_DATA1RD.
  #endif
  #if defined( LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1WR )
  ldmaPeripheralSignal_CRYPTO_DATA1WR = LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO,              ///< Trig on CRYPTO_DATA1WR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0RD)
  ldmaPeripheralSignal_CRYPTO0_DATA0RD = LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO0,           ///< Trig on CRYPTO0_DATA0RD.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0WR)
  ldmaPeripheralSignal_CRYPTO0_DATA0WR = LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO0,           ///< Trig on CRYPTO0_DATA0WR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0XWR)
  ldmaPeripheralSignal_CRYPTO0_DATA0XWR = LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA0XWR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO0,         ///< Trig on CRYPTO0_DATA0XWR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA1RD)
  ldmaPeripheralSignal_CRYPTO0_DATA1RD = LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA1RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO0,           ///< Trig on CRYPTO0_DATA1RD.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA1WR)
  ldmaPeripheralSignal_CRYPTO0_DATA1WR = LDMA_CH_REQSEL_SIGSEL_CRYPTO0DATA1WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO0,           ///< Trig on CRYPTO0_DATA1WR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0RD)
  ldmaPeripheralSignal_CRYPTO1_DATA0RD = LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO1,           ///< Trig on CRYPTO1_DATA0RD.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0WR)
  ldmaPeripheralSignal_CRYPTO1_DATA0WR = LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO1,           ///< Trig on CRYPTO1_DATA0WR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0XWR)
  ldmaPeripheralSignal_CRYPTO1_DATA0XWR = LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA0XWR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO1,         ///< Trig on CRYPTO1_DATA0XWR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA1RD)
  ldmaPeripheralSignal_CRYPTO1_DATA1RD = LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA1RD | LDMA_CH_REQSEL_SOURCESEL_CRYPTO1,           ///< Trig on CRYPTO1_DATA1RD.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA1WR)
  ldmaPeripheralSignal_CRYPTO1_DATA1WR = LDMA_CH_REQSEL_SIGSEL_CRYPTO1DATA1WR | LDMA_CH_REQSEL_SOURCESEL_CRYPTO1,           ///< Trig on CRYPTO1_DATA1WR.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CSENBSLN)
  ldmaPeripheralSignal_CSEN_BSLN = LDMA_CH_REQSEL_SIGSEL_CSENBSLN | LDMA_CH_REQSEL_SOURCESEL_CSEN,                          ///< Trig on CSEN_BSLN.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_CSENDATA)
  ldmaPeripheralSignal_CSEN_DATA = LDMA_CH_REQSEL_SIGSEL_CSENDATA | LDMA_CH_REQSEL_SOURCESEL_CSEN,                          ///< Trig on CSEN_DATA.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_I2C0RXDATAV)
  ldmaPeripheralSignal_I2C0_RXDATAV = LDMA_CH_REQSEL_SIGSEL_I2C0RXDATAV | LDMA_CH_REQSEL_SOURCESEL_I2C0,                    ///< Trig on I2C0_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_I2C0TXBL)
  ldmaPeripheralSignal_I2C0_TXBL = LDMA_CH_REQSEL_SIGSEL_I2C0TXBL | LDMA_CH_REQSEL_SOURCESEL_I2C0,                          ///< Trig on I2C0_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_I2C1RXDATAV)
  ldmaPeripheralSignal_I2C1_RXDATAV = LDMA_CH_REQSEL_SIGSEL_I2C1RXDATAV | LDMA_CH_REQSEL_SOURCESEL_I2C1,                    ///< Trig on I2C1_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_I2C1TXBL)
  ldmaPeripheralSignal_I2C1_TXBL = LDMA_CH_REQSEL_SIGSEL_I2C1TXBL | LDMA_CH_REQSEL_SOURCESEL_I2C1,                          ///< Trig on I2C1_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_LESENSEBUFDATAV)
  ldmaPeripheralSignal_LESENSE_BUFDATAV = LDMA_CH_REQSEL_SIGSEL_LESENSEBUFDATAV | LDMA_CH_REQSEL_SOURCESEL_LESENSE,         ///< Trig on LESENSE_BUFDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_LEUART0RXDATAV)
  ldmaPeripheralSignal_LEUART0_RXDATAV = LDMA_CH_REQSEL_SIGSEL_LEUART0RXDATAV | LDMA_CH_REQSEL_SOURCESEL_LEUART0,           ///< Trig on LEUART0_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_LEUART0TXBL)
  ldmaPeripheralSignal_LEUART0_TXBL = LDMA_CH_REQSEL_SIGSEL_LEUART0TXBL | LDMA_CH_REQSEL_SOURCESEL_LEUART0,                 ///< Trig on LEUART0_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_LEUART0TXEMPTY)
  ldmaPeripheralSignal_LEUART0_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_LEUART0TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_LEUART0,           ///< Trig on LEUART0_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_MSCWDATA)
  ldmaPeripheralSignal_MSC_WDATA = LDMA_CH_REQSEL_SIGSEL_MSCWDATA | LDMA_CH_REQSEL_SOURCESEL_MSC,                           ///< Trig on MSC_WDATA.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_PRSREQ0)
  ldmaPeripheralSignal_PRS_REQ0 = LDMA_CH_REQSEL_SIGSEL_PRSREQ0 | LDMA_CH_REQSEL_SOURCESEL_PRS,                             ///< Trig on PRS_REQ0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_PRSREQ1)
  ldmaPeripheralSignal_PRS_REQ1 = LDMA_CH_REQSEL_SIGSEL_PRSREQ1 | LDMA_CH_REQSEL_SOURCESEL_PRS,                             ///< Trig on PRS_REQ1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER0CC0)
  ldmaPeripheralSignal_TIMER0_CC0 = LDMA_CH_REQSEL_SIGSEL_TIMER0CC0 | LDMA_CH_REQSEL_SOURCESEL_TIMER0,                      ///< Trig on TIMER0_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER0CC1)
  ldmaPeripheralSignal_TIMER0_CC1 = LDMA_CH_REQSEL_SIGSEL_TIMER0CC1 | LDMA_CH_REQSEL_SOURCESEL_TIMER0,                      ///< Trig on TIMER0_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER0CC2)
  ldmaPeripheralSignal_TIMER0_CC2 = LDMA_CH_REQSEL_SIGSEL_TIMER0CC2 | LDMA_CH_REQSEL_SOURCESEL_TIMER0,                      ///< Trig on TIMER0_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER0UFOF)
  ldmaPeripheralSignal_TIMER0_UFOF = LDMA_CH_REQSEL_SIGSEL_TIMER0UFOF | LDMA_CH_REQSEL_SOURCESEL_TIMER0,                    ///< Trig on TIMER0_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER1CC0)
  ldmaPeripheralSignal_TIMER1_CC0 = LDMA_CH_REQSEL_SIGSEL_TIMER1CC0 | LDMA_CH_REQSEL_SOURCESEL_TIMER1,                      ///< Trig on TIMER1_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER1CC1)
  ldmaPeripheralSignal_TIMER1_CC1 = LDMA_CH_REQSEL_SIGSEL_TIMER1CC1 | LDMA_CH_REQSEL_SOURCESEL_TIMER1,                      ///< Trig on TIMER1_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER1CC2)
  ldmaPeripheralSignal_TIMER1_CC2 = LDMA_CH_REQSEL_SIGSEL_TIMER1CC2 | LDMA_CH_REQSEL_SOURCESEL_TIMER1,                      ///< Trig on TIMER1_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER1CC3)
  ldmaPeripheralSignal_TIMER1_CC3 = LDMA_CH_REQSEL_SIGSEL_TIMER1CC3 | LDMA_CH_REQSEL_SOURCESEL_TIMER1,                      ///< Trig on TIMER1_CC3.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_TIMER1UFOF)
  ldmaPeripheralSignal_TIMER1_UFOF = LDMA_CH_REQSEL_SIGSEL_TIMER1UFOF | LDMA_CH_REQSEL_SOURCESEL_TIMER1,                    ///< Trig on TIMER1_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART0RXDATAV)
  ldmaPeripheralSignal_USART0_RXDATAV = LDMA_CH_REQSEL_SIGSEL_USART0RXDATAV | LDMA_CH_REQSEL_SOURCESEL_USART0,              ///< Trig on USART0_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART0TXBL)
  ldmaPeripheralSignal_USART0_TXBL = LDMA_CH_REQSEL_SIGSEL_USART0TXBL | LDMA_CH_REQSEL_SOURCESEL_USART0,                    ///< Trig on USART0_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART0TXEMPTY)
  ldmaPeripheralSignal_USART0_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_USART0TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_USART0,              ///< Trig on USART0_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART1RXDATAV)
  ldmaPeripheralSignal_USART1_RXDATAV = LDMA_CH_REQSEL_SIGSEL_USART1RXDATAV | LDMA_CH_REQSEL_SOURCESEL_USART1,              ///< Trig on USART1_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART1RXDATAVRIGHT)
  ldmaPeripheralSignal_USART1_RXDATAVRIGHT = LDMA_CH_REQSEL_SIGSEL_USART1RXDATAVRIGHT | LDMA_CH_REQSEL_SOURCESEL_USART1,    ///< Trig on USART1_RXDATAVRIGHT.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART1TXBL)
  ldmaPeripheralSignal_USART1_TXBL = LDMA_CH_REQSEL_SIGSEL_USART1TXBL | LDMA_CH_REQSEL_SOURCESEL_USART1,                    ///< Trig on USART1_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART1TXBLRIGHT)
  ldmaPeripheralSignal_USART1_TXBLRIGHT = LDMA_CH_REQSEL_SIGSEL_USART1TXBLRIGHT | LDMA_CH_REQSEL_SOURCESEL_USART1,          ///< Trig on USART1_TXBLRIGHT.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART1TXEMPTY)
  ldmaPeripheralSignal_USART1_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_USART1TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_USART1,              ///< Trig on USART1_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART2RXDATAV)
  ldmaPeripheralSignal_USART2_RXDATAV = LDMA_CH_REQSEL_SIGSEL_USART2RXDATAV | LDMA_CH_REQSEL_SOURCESEL_USART2,              ///< Trig on USART2_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART2TXBL)
  ldmaPeripheralSignal_USART2_TXBL = LDMA_CH_REQSEL_SIGSEL_USART2TXBL | LDMA_CH_REQSEL_SOURCESEL_USART2,                    ///< Trig on USART2_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART2TXEMPTY)
  ldmaPeripheralSignal_USART2_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_USART2TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_USART2,              ///< Trig on USART2_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART3RXDATAV)
  ldmaPeripheralSignal_USART3_RXDATAV = LDMA_CH_REQSEL_SIGSEL_USART3RXDATAV | LDMA_CH_REQSEL_SOURCESEL_USART3,              ///< Trig on USART3_RXDATAV.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART3RXDATAVRIGHT)
  ldmaPeripheralSignal_USART3_RXDATAVRIGHT = LDMA_CH_REQSEL_SIGSEL_USART3RXDATAVRIGHT | LDMA_CH_REQSEL_SOURCESEL_USART3,    ///< Trig on USART3_RXDATAVRIGHT.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART3TXBL)
  ldmaPeripheralSignal_USART3_TXBL = LDMA_CH_REQSEL_SIGSEL_USART3TXBL | LDMA_CH_REQSEL_SOURCESEL_USART3,                    ///< Trig on USART3_TXBL.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART3TXBLRIGHT)
  ldmaPeripheralSignal_USART3_TXBLRIGHT = LDMA_CH_REQSEL_SIGSEL_USART3TXBLRIGHT | LDMA_CH_REQSEL_SOURCESEL_USART3,          ///< Trig on USART3_TXBLRIGHT.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_USART3TXEMPTY)
  ldmaPeripheralSignal_USART3_TXEMPTY = LDMA_CH_REQSEL_SIGSEL_USART3TXEMPTY | LDMA_CH_REQSEL_SOURCESEL_USART3,              ///< Trig on USART3_TXEMPTY.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_VDAC0CH0)
  ldmaPeripheralSignal_VDAC0_CH0 = LDMA_CH_REQSEL_SIGSEL_VDAC0CH0 | LDMA_CH_REQSEL_SOURCESEL_VDAC0,                         ///< Trig on VDAC0_CH0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_VDAC0CH1)
  ldmaPeripheralSignal_VDAC0_CH1 = LDMA_CH_REQSEL_SIGSEL_VDAC0CH1 | LDMA_CH_REQSEL_SOURCESEL_VDAC0,                         ///< Trig on VDAC0_CH1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER0CC0)
  ldmaPeripheralSignal_WTIMER0_CC0 = LDMA_CH_REQSEL_SIGSEL_WTIMER0CC0 | LDMA_CH_REQSEL_SOURCESEL_WTIMER0,                   ///< Trig on WTIMER0_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER0CC1)
  ldmaPeripheralSignal_WTIMER0_CC1 = LDMA_CH_REQSEL_SIGSEL_WTIMER0CC1 | LDMA_CH_REQSEL_SOURCESEL_WTIMER0,                   ///< Trig on WTIMER0_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER0CC2)
  ldmaPeripheralSignal_WTIMER0_CC2 = LDMA_CH_REQSEL_SIGSEL_WTIMER0CC2 | LDMA_CH_REQSEL_SOURCESEL_WTIMER0,                   ///< Trig on WTIMER0_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER0UFOF)
  ldmaPeripheralSignal_WTIMER0_UFOF = LDMA_CH_REQSEL_SIGSEL_WTIMER0UFOF | LDMA_CH_REQSEL_SOURCESEL_WTIMER0,                 ///< Trig on WTIMER0_UFOF.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER1CC0)
  ldmaPeripheralSignal_WTIMER1_CC0 = LDMA_CH_REQSEL_SIGSEL_WTIMER1CC0 | LDMA_CH_REQSEL_SOURCESEL_WTIMER1,                   ///< Trig on WTIMER1_CC0.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER1CC1)
  ldmaPeripheralSignal_WTIMER1_CC1 = LDMA_CH_REQSEL_SIGSEL_WTIMER1CC1 | LDMA_CH_REQSEL_SOURCESEL_WTIMER1,                   ///< Trig on WTIMER1_CC1.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER1CC2)
  ldmaPeripheralSignal_WTIMER1_CC2 = LDMA_CH_REQSEL_SIGSEL_WTIMER1CC2 | LDMA_CH_REQSEL_SOURCESEL_WTIMER1,                   ///< Trig on WTIMER1_CC2.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER1CC3)
  ldmaPeripheralSignal_WTIMER1_CC3 = LDMA_CH_REQSEL_SIGSEL_WTIMER1CC3 | LDMA_CH_REQSEL_SOURCESEL_WTIMER1,                   ///< Trig on WTIMER1_CC3.
  #endif
  #if defined(LDMA_CH_REQSEL_SIGSEL_WTIMER1UFOF)
  ldmaPeripheralSignal_WTIMER1_UFOF = LDMA_CH_REQSEL_SIGSEL_WTIMER1UFOF | LDMA_CH_REQSEL_SOURCESEL_WTIMER1                  ///< Trig on WTIMER1_UFOF.
  #endif
} LDMA_PeripheralSignal_t;


/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/**
 * @brief
 *   DMA descriptor.
 * @details
 *   The LDMA DMA controller supports three different DMA descriptors. Each
 *   consist of four WORD's which map directly onto hw control registers for a
 *   given DMA channel. The three descriptor types are XFER, SYNC and WRI.
 *   Refer to the reference manual for further information.
 */
typedef union
{
  /**
   *  TRANSFER DMA descriptor, this is the only descriptor type which can be
   *  used to start a DMA transfer.
   */
  struct
  {
    uint32_t  structType : 2;   /**< Set to 0 to select XFER descriptor type. */
    uint32_t  reserved0  : 1;
    uint32_t  structReq  : 1;   /**< DMA transfer trigger during LINKLOAD.    */
    uint32_t  xferCnt    : 11;  /**< Transfer count minus one.                */
    uint32_t  byteSwap   : 1;   /**< Enable byte swapping transfers.          */
    uint32_t  blockSize  : 4;   /**< Number of unit transfers per arb. cycle. */
    uint32_t  doneIfs    : 1;   /**< Generate interrupt when done.            */
    uint32_t  reqMode    : 1;   /**< Block or cycle transfer selector.        */
    uint32_t  decLoopCnt : 1;   /**< Enable looped transfers.                 */
    uint32_t  ignoreSrec : 1;   /**< Ignore single requests.                  */
    uint32_t  srcInc     : 2;   /**< Source address increment unit size.      */
    uint32_t  size       : 2;   /**< DMA transfer unit size.                  */
    uint32_t  dstInc     : 2;   /**< Destination address increment unit size. */
    uint32_t  srcAddrMode: 1;   /**< Source addressing mode.                  */
    uint32_t  dstAddrMode: 1;   /**< Destination addressing mode.             */

    uint32_t  srcAddr;          /**< DMA source address.                      */
    uint32_t  dstAddr;          /**< DMA destination address.                 */

    uint32_t  linkMode   : 1;   /**< Select absolute or relative link address.*/
    uint32_t  link       : 1;   /**< Enable LINKLOAD when transfer is done.   */
    int32_t   linkAddr   : 30;  /**< Address of next (linked) descriptor.     */
  } xfer;

  /** SYNCHRONIZE DMA descriptor, used for intra channel transfer
  *   syncronization.
  */
  struct
  {
    uint32_t  structType : 2;   /**< Set to 1 to select SYNC descriptor type. */
    uint32_t  reserved0  : 1;
    uint32_t  structReq  : 1;   /**< DMA transfer trigger during LINKLOAD.    */
    uint32_t  xferCnt    : 11;  /**< Transfer count minus one.                */
    uint32_t  byteSwap   : 1;   /**< Enable byte swapping transfers.          */
    uint32_t  blockSize  : 4;   /**< Number of unit transfers per arb. cycle. */
    uint32_t  doneIfs    : 1;   /**< Generate interrupt when done.            */
    uint32_t  reqMode    : 1;   /**< Block or cycle transfer selector.        */
    uint32_t  decLoopCnt : 1;   /**< Enable looped transfers.                 */
    uint32_t  ignoreSrec : 1;   /**< Ignore single requests.                  */
    uint32_t  srcInc     : 2;   /**< Source address increment unit size.      */
    uint32_t  size       : 2;   /**< DMA transfer unit size.                  */
    uint32_t  dstInc     : 2;   /**< Destination address increment unit size. */
    uint32_t  srcAddrMode: 1;   /**< Source addressing mode.                  */
    uint32_t  dstAddrMode: 1;   /**< Destination addressing mode.             */

    uint32_t  syncSet    : 8;   /**< Set bits in LDMA_CTRL.SYNCTRIG register. */
    uint32_t  syncClr    : 8;   /**< Clear bits in LDMA_CTRL.SYNCTRIG register*/
    uint32_t  reserved3  : 16;
    uint32_t  matchVal   : 8;   /**< Sync trig match value.                   */
    uint32_t  matchEn    : 8;   /**< Sync trig match enable.                  */
    uint32_t  reserved4  : 16;

    uint32_t  linkMode   : 1;   /**< Select absolute or relative link address.*/
    uint32_t  link       : 1;   /**< Enable LINKLOAD when transfer is done.   */
    int32_t   linkAddr   : 30;  /**< Address of next (linked) descriptor.     */
  } sync;

  /** WRITE DMA descriptor, used for write immediate operations.              */
  struct
  {
    uint32_t  structType : 2;   /**< Set to 2 to select WRITE descriptor type.*/
    uint32_t  reserved0  : 1;
    uint32_t  structReq  : 1;   /**< DMA transfer trigger during LINKLOAD.    */
    uint32_t  xferCnt    : 11;  /**< Transfer count minus one.                */
    uint32_t  byteSwap   : 1;   /**< Enable byte swapping transfers.          */
    uint32_t  blockSize  : 4;   /**< Number of unit transfers per arb. cycle. */
    uint32_t  doneIfs    : 1;   /**< Generate interrupt when done.            */
    uint32_t  reqMode    : 1;   /**< Block or cycle transfer selector.        */
    uint32_t  decLoopCnt : 1;   /**< Enable looped transfers.                 */
    uint32_t  ignoreSrec : 1;   /**< Ignore single requests.                  */
    uint32_t  srcInc     : 2;   /**< Source address increment unit size.      */
    uint32_t  size       : 2;   /**< DMA transfer unit size.                  */
    uint32_t  dstInc     : 2;   /**< Destination address increment unit size. */
    uint32_t  srcAddrMode: 1;   /**< Source addressing mode.                  */
    uint32_t  dstAddrMode: 1;   /**< Destination addressing mode.             */

    uint32_t  immVal;           /**< Data to be written at dstAddr.           */
    uint32_t  dstAddr;          /**< DMA write destination address.           */

    uint32_t  linkMode   : 1;   /**< Select absolute or relative link address.*/
    uint32_t  link       : 1;   /**< Enable LINKLOAD when transfer is done.   */
    int32_t   linkAddr   : 30;  /**< Address of next (linked) descriptor.     */
  } wri;
} LDMA_Descriptor_t;

/** @brief LDMA initialization configuration structure. */
typedef struct
{
  uint8_t               ldmaInitCtrlNumFixed;     /**< Arbitration mode separator.*/
  uint8_t               ldmaInitCtrlSyncPrsClrEn; /**< PRS Synctrig clear enable. */
  uint8_t               ldmaInitCtrlSyncPrsSetEn; /**< PRS Synctrig set enable.   */
  uint8_t               ldmaInitIrqPriority;      /**< LDMA IRQ priority (0..7).  */
} LDMA_Init_t;

/**
 * @brief
 *   DMA transfer configuration structure.
 * @details
 *   This struct configures all aspects of a DMA transfer.
 */
typedef struct
{
  uint32_t              ldmaReqSel;            /**< Selects DMA trigger source.          */
  uint8_t               ldmaCtrlSyncPrsClrOff; /**< PRS Synctrig clear enables to clear. */
  uint8_t               ldmaCtrlSyncPrsClrOn;  /**< PRS Synctrig clear enables to set.   */
  uint8_t               ldmaCtrlSyncPrsSetOff; /**< PRS Synctrig set enables to clear.   */
  uint8_t               ldmaCtrlSyncPrsSetOn;  /**< PRS Synctrig set enables to set.     */
  bool                  ldmaReqDis;            /**< Mask the PRS trigger input.          */
  bool                  ldmaDbgHalt;           /**< Dis. DMA trig when cpu is halted.    */
  uint8_t               ldmaCfgArbSlots;       /**< Arbitration slot number.             */
  uint8_t               ldmaCfgSrcIncSign;     /**< Source addr. increment sign.         */
  uint8_t               ldmaCfgDstIncSign;     /**< Dest. addr. increment sign.          */
  uint8_t               ldmaLoopCnt;           /**< Counter for looped transfers.        */
} LDMA_TransferCfg_t;


/*******************************************************************************
 **************************   STRUCT INITIALIZERS   ****************************
 ******************************************************************************/


/** @brief Default DMA initialization structure. */
#define LDMA_INIT_DEFAULT                                                                   \
{                                                                                           \
  .ldmaInitCtrlNumFixed     = _LDMA_CTRL_NUMFIXED_DEFAULT, /* Fixed priority arbitration. */ \
  .ldmaInitCtrlSyncPrsClrEn = 0,                           /* No PRS Synctrig clear enable*/ \
  .ldmaInitCtrlSyncPrsSetEn = 0,                           /* No PRS Synctrig set enable. */ \
  .ldmaInitIrqPriority      = 3                            /* IRQ priority level 3.       */ \
}

/**
 * @brief
 *   Generic DMA transfer configuration for memory to memory transfers.
 */
#define LDMA_TRANSFER_CFG_MEMORY()              \
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
{                                               \
  0, 0, 0, 0, 0,                                \
  false, false, ldmaCfgArbSlotsAs1,             \
  ldmaCfgSrcIncSignPos, ldmaCfgDstIncSignPos,   \
  loopCnt                                       \
}

/**
 * @brief
 *   Generic DMA transfer configuration for memory to/from peripheral transfers.
 */
#define LDMA_TRANSFER_CFG_PERIPHERAL(signal)    \
{                                               \
  signal, 0, 0, 0, 0,                           \
  false, false, ldmaCfgArbSlotsAs1,             \
  ldmaCfgSrcIncSignPos, ldmaCfgDstIncSignPos, 0 \
}

/**
 * @brief
 *   Generic DMA transfer configuration for looped memory to/from peripheral transfers.
 */
#define LDMA_TRANSFER_CFG_PERIPHERAL_LOOP(signal, loopCnt)    \
{                                                             \
  signal, 0, 0, 0, 0,                                         \
  false, false, ldmaCfgArbSlotsAs1,                           \
  ldmaCfgSrcIncSignPos, ldmaCfgDstIncSignPos, loopCnt         \
}

/**
 * @brief
 *   DMA descriptor initializer for single memory to memory word transfer.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of words to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_M2M_WORD(src, dest, count)   \
{                                                           \
  .xfer =                                                   \
  {                                                         \
    .structType   = ldmaCtrlStructTypeXfer,                 \
    .structReq    = 1,                                      \
    .xferCnt      = ( count ) - 1,                          \
    .byteSwap     = 0,                                      \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                 \
    .doneIfs      = 1,                                      \
    .reqMode      = ldmaCtrlReqModeAll,                     \
    .decLoopCnt   = 0,                                      \
    .ignoreSrec   = 0,                                      \
    .srcInc       = ldmaCtrlSrcIncOne,                      \
    .size         = ldmaCtrlSizeWord,                       \
    .dstInc       = ldmaCtrlDstIncOne,                      \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                 \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                 \
    .srcAddr      = (uint32_t)(src),                        \
    .dstAddr      = (uint32_t)(dest),                       \
    .linkMode     = 0,                                      \
    .link         = 0,                                      \
    .linkAddr     = 0                                       \
  }                                                         \
}

/**
 * @brief
 *   DMA descriptor initializer for single memory to memory half-word transfer.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of half-words to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_M2M_HALF(src, dest, count)   \
{                                                           \
  .xfer =                                                   \
  {                                                         \
    .structType   = ldmaCtrlStructTypeXfer,                 \
    .structReq    = 1,                                      \
    .xferCnt      = ( count ) - 1,                          \
    .byteSwap     = 0,                                      \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                 \
    .doneIfs      = 1,                                      \
    .reqMode      = ldmaCtrlReqModeAll,                     \
    .decLoopCnt   = 0,                                      \
    .ignoreSrec   = 0,                                      \
    .srcInc       = ldmaCtrlSrcIncOne,                      \
    .size         = ldmaCtrlSizeHalf,                       \
    .dstInc       = ldmaCtrlDstIncOne,                      \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                 \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                 \
    .srcAddr      = (uint32_t)(src),                        \
    .dstAddr      = (uint32_t)(dest),                       \
    .linkMode     = 0,                                      \
    .link         = 0,                                      \
    .linkAddr     = 0                                       \
  }                                                         \
}

/**
 * @brief
 *   DMA descriptor initializer for single memory to memory byte transfer.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of bytes to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_M2M_BYTE(src, dest, count)   \
{                                                           \
  .xfer =                                                   \
  {                                                         \
    .structType   = ldmaCtrlStructTypeXfer,                 \
    .structReq    = 1,                                      \
    .xferCnt      = (count) - 1,                            \
    .byteSwap     = 0,                                      \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                 \
    .doneIfs      = 1,                                      \
    .reqMode      = ldmaCtrlReqModeAll,                     \
    .decLoopCnt   = 0,                                      \
    .ignoreSrec   = 0,                                      \
    .srcInc       = ldmaCtrlSrcIncOne,                      \
    .size         = ldmaCtrlSizeByte,                       \
    .dstInc       = ldmaCtrlDstIncOne,                      \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                 \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                 \
    .srcAddr      = (uint32_t)(src),                        \
    .dstAddr      = (uint32_t)(dest),                       \
    .linkMode     = 0,                                      \
    .link         = 0,                                      \
    .linkAddr     = 0                                       \
  }                                                         \
}

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory word transfer.
 *
 *   The link address must be an absolute address.
 * @note
 *   The linkAddr member of the transfer descriptor is not
 *   initialized.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of words to transfer.
 */
#define LDMA_DESCRIPTOR_LINKABS_M2M_WORD(src, dest, count)   \
{                                                            \
  .xfer =                                                    \
  {                                                          \
    .structType   = ldmaCtrlStructTypeXfer,                  \
    .structReq    = 1,                                       \
    .xferCnt      = (count) - 1,                             \
    .byteSwap     = 0,                                       \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                  \
    .doneIfs      = 0,                                       \
    .reqMode      = ldmaCtrlReqModeAll,                      \
    .decLoopCnt   = 0,                                       \
    .ignoreSrec   = 0,                                       \
    .srcInc       = ldmaCtrlSrcIncOne,                       \
    .size         = ldmaCtrlSizeWord,                        \
    .dstInc       = ldmaCtrlDstIncOne,                       \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                  \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                  \
    .srcAddr      = (uint32_t)(src),                         \
    .dstAddr      = (uint32_t)(dest),                        \
    .linkMode     = ldmaLinkModeAbs,                         \
    .link         = 1,                                       \
    .linkAddr     = 0   /* Must be set runtime ! */          \
  }                                                          \
}

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory half-word transfer.
 *
 *   The link address must be an absolute address.
 * @note
 *   The linkAddr member of the transfer descriptor is not
 *   initialized.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of half-words to transfer.
 */
#define LDMA_DESCRIPTOR_LINKABS_M2M_HALF(src, dest, count)   \
{                                                            \
  .xfer =                                                    \
  {                                                          \
    .structType   = ldmaCtrlStructTypeXfer,                  \
    .structReq    = 1,                                       \
    .xferCnt      = (count) - 1,                             \
    .byteSwap     = 0,                                       \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                  \
    .doneIfs      = 0,                                       \
    .reqMode      = ldmaCtrlReqModeAll,                      \
    .decLoopCnt   = 0,                                       \
    .ignoreSrec   = 0,                                       \
    .srcInc       = ldmaCtrlSrcIncOne,                       \
    .size         = ldmaCtrlSizeHalf,                        \
    .dstInc       = ldmaCtrlDstIncOne,                       \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                  \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                  \
    .srcAddr      = (uint32_t)(src),                         \
    .dstAddr      = (uint32_t)(dest),                        \
    .linkMode     = ldmaLinkModeAbs,                         \
    .link         = 1,                                       \
    .linkAddr     = 0   /* Must be set runtime ! */          \
  }                                                          \
}

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory byte transfer.
 *
 *   The link address must be an absolute address.
 * @note
 *   The linkAddr member of the transfer descriptor is not
 *   initialized.
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of bytes to transfer.
 */
#define LDMA_DESCRIPTOR_LINKABS_M2M_BYTE(src, dest, count)   \
{                                                            \
  .xfer =                                                    \
  {                                                          \
    .structType   = ldmaCtrlStructTypeXfer,                  \
    .structReq    = 1,                                       \
    .xferCnt      = (count) - 1,                             \
    .byteSwap     = 0,                                       \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                  \
    .doneIfs      = 0,                                       \
    .reqMode      = ldmaCtrlReqModeAll,                      \
    .decLoopCnt   = 0,                                       \
    .ignoreSrec   = 0,                                       \
    .srcInc       = ldmaCtrlSrcIncOne,                       \
    .size         = ldmaCtrlSizeByte,                        \
    .dstInc       = ldmaCtrlDstIncOne,                       \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                  \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                  \
    .srcAddr      = (uint32_t)(src),                         \
    .dstAddr      = (uint32_t)(dest),                        \
    .linkMode     = ldmaLinkModeAbs,                         \
    .link         = 1,                                       \
    .linkAddr     = 0   /* Must be set runtime ! */          \
  }                                                          \
}

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory word transfer.
 *
 *   The link address is a relative address.
 * @note
 *   The linkAddr member of the transfer descriptor is
 *   initialized to 4, assuming that the next descriptor immediately follows
 *   this descriptor (in memory).
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of words to transfer.
 * @param[in] linkjmp   Address of descriptor to link to expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_M2M_WORD(src, dest, count, linkjmp)   \
{                                                                     \
  .xfer =                                                             \
  {                                                                   \
    .structType   = ldmaCtrlStructTypeXfer,                           \
    .structReq    = 1,                                                \
    .xferCnt      = (count) - 1,                                      \
    .byteSwap     = 0,                                                \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                           \
    .doneIfs      = 0,                                                \
    .reqMode      = ldmaCtrlReqModeAll,                               \
    .decLoopCnt   = 0,                                                \
    .ignoreSrec   = 0,                                                \
    .srcInc       = ldmaCtrlSrcIncOne,                                \
    .size         = ldmaCtrlSizeWord,                                 \
    .dstInc       = ldmaCtrlDstIncOne,                                \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                           \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                           \
    .srcAddr      = (uint32_t)(src),                                  \
    .dstAddr      = (uint32_t)(dest),                                 \
    .linkMode     = ldmaLinkModeRel,                                  \
    .link         = 1,                                                \
    .linkAddr     = (linkjmp) * 4                                     \
  }                                                                   \
}

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory half-word transfer.
 *
 *   The link address is a relative address.
 * @note
 *   The linkAddr member of the transfer descriptor is
 *   initialized to 4, assuming that the next descriptor immediately follows
 *   this descriptor (in memory).
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of half-words to transfer.
 * @param[in] linkjmp   Address of descriptor to link to expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_M2M_HALF(src, dest, count, linkjmp)   \
{                                                                     \
  .xfer =                                                             \
  {                                                                   \
    .structType   = ldmaCtrlStructTypeXfer,                           \
    .structReq    = 1,                                                \
    .xferCnt      = (count) - 1,                                      \
    .byteSwap     = 0,                                                \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                           \
    .doneIfs      = 0,                                                \
    .reqMode      = ldmaCtrlReqModeAll,                               \
    .decLoopCnt   = 0,                                                \
    .ignoreSrec   = 0,                                                \
    .srcInc       = ldmaCtrlSrcIncOne,                                \
    .size         = ldmaCtrlSizeHalf,                                 \
    .dstInc       = ldmaCtrlDstIncOne,                                \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                           \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                           \
    .srcAddr      = (uint32_t)(src),                                  \
    .dstAddr      = (uint32_t)(dest),                                 \
    .linkMode     = ldmaLinkModeRel,                                  \
    .link         = 1,                                                \
    .linkAddr     = (linkjmp) * 4                                     \
  }                                                                   \
}

/**
 * @brief
 *   DMA descriptor initializer for linked memory to memory byte transfer.
 *
 *   The link address is a relative address.
 * @note
 *   The linkAddr member of the transfer descriptor is
 *   initialized to 4, assuming that the next descriptor immediately follows
 *   this descriptor (in memory).
 * @param[in] src       Source data address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of bytes to transfer.
 * @param[in] linkjmp   Address of descriptor to link to expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_M2M_BYTE(src, dest, count, linkjmp)   \
{                                                                     \
  .xfer =                                                             \
  {                                                                   \
    .structType   = ldmaCtrlStructTypeXfer,                           \
    .structReq    = 1,                                                \
    .xferCnt      = (count) - 1,                                      \
    .byteSwap     = 0,                                                \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                           \
    .doneIfs      = 0,                                                \
    .reqMode      = ldmaCtrlReqModeAll,                               \
    .decLoopCnt   = 0,                                                \
    .ignoreSrec   = 0,                                                \
    .srcInc       = ldmaCtrlSrcIncOne,                                \
    .size         = ldmaCtrlSizeByte,                                 \
    .dstInc       = ldmaCtrlDstIncOne,                                \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                           \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                           \
    .srcAddr      = (uint32_t)(src),                                  \
    .dstAddr      = (uint32_t)(dest),                                 \
    .linkMode     = ldmaLinkModeRel,                                  \
    .link         = 1,                                                \
    .linkAddr     = (linkjmp) * 4                                     \
  }                                                                   \
}

/**
 * @brief
 *   DMA descriptor initializer for byte transfers from a peripheral to memory.
 * @param[in] src       Peripheral data source register address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of bytes to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_P2M_BYTE(src, dest, count)   \
{                                                           \
  .xfer =                                                   \
  {                                                         \
    .structType   = ldmaCtrlStructTypeXfer,                 \
    .structReq    = 0,                                      \
    .xferCnt      = (count) - 1,                            \
    .byteSwap     = 0,                                      \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                 \
    .doneIfs      = 1,                                      \
    .reqMode      = ldmaCtrlReqModeBlock,                   \
    .decLoopCnt   = 0,                                      \
    .ignoreSrec   = 0,                                      \
    .srcInc       = ldmaCtrlSrcIncNone,                     \
    .size         = ldmaCtrlSizeByte,                       \
    .dstInc       = ldmaCtrlDstIncOne,                      \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                 \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                 \
    .srcAddr      = (uint32_t)(src),                        \
    .dstAddr      = (uint32_t)(dest),                       \
    .linkMode     = 0,                                      \
    .link         = 0,                                      \
    .linkAddr     = 0                                       \
  }                                                         \
}

/**
 * @brief
 *   DMA descriptor initializer for byte transfers from a peripheral to a peripheral.
 * @param[in] src       Peripheral data source register address.
 * @param[in] dest      Peripheral data destination register address.
 * @param[in] count     Number of bytes to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_P2P_BYTE(src, dest, count)   \
{                                                           \
  .xfer =                                                   \
  {                                                         \
    .structType   = ldmaCtrlStructTypeXfer,                 \
    .structReq    = 0,                                      \
    .xferCnt      = (count) - 1,                            \
    .byteSwap     = 0,                                      \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                 \
    .doneIfs      = 1,                                      \
    .reqMode      = ldmaCtrlReqModeBlock,                   \
    .decLoopCnt   = 0,                                      \
    .ignoreSrec   = 0,                                      \
    .srcInc       = ldmaCtrlSrcIncNone,                     \
    .size         = ldmaCtrlSizeByte,                       \
    .dstInc       = ldmaCtrlDstIncNone,                     \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                 \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                 \
    .srcAddr      = (uint32_t)(src),                        \
    .dstAddr      = (uint32_t)(dest),                       \
    .linkMode     = 0,                                      \
    .link         = 0,                                      \
    .linkAddr     = 0                                       \
  }                                                         \
}

/**
 * @brief
 *   DMA descriptor initializer for byte transfers from memory to a peripheral
 * @param[in] src       Source data address.
 * @param[in] dest      Peripheral data register destination address.
 * @param[in] count     Number of bytes to transfer.
 */
#define LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(src, dest, count)   \
{                                                           \
  .xfer =                                                   \
  {                                                         \
    .structType   = ldmaCtrlStructTypeXfer,                 \
    .structReq    = 0,                                      \
    .xferCnt      = (count) - 1,                            \
    .byteSwap     = 0,                                      \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                 \
    .doneIfs      = 1,                                      \
    .reqMode      = ldmaCtrlReqModeBlock,                   \
    .decLoopCnt   = 0,                                      \
    .ignoreSrec   = 0,                                      \
    .srcInc       = ldmaCtrlSrcIncOne,                      \
    .size         = ldmaCtrlSizeByte,                       \
    .dstInc       = ldmaCtrlDstIncNone,                     \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                 \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                 \
    .srcAddr      = (uint32_t)(src),                        \
    .dstAddr      = (uint32_t)(dest),                       \
    .linkMode     = 0,                                      \
    .link         = 0,                                      \
    .linkAddr     = 0                                       \
  }                                                         \
}

/**
 * @brief
 *   DMA descriptor initializer for byte transfers from a peripheral to memory.
 * @param[in] src       Peripheral data source register address.
 * @param[in] dest      Destination data address.
 * @param[in] count     Number of bytes to transfer.
 * @param[in] linkjmp   Address of descriptor to link to expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(src, dest, count, linkjmp)   \
{                                                                     \
  .xfer =                                                             \
  {                                                                   \
    .structType   = ldmaCtrlStructTypeXfer,                           \
    .structReq    = 0,                                                \
    .xferCnt      = (count) - 1,                                      \
    .byteSwap     = 0,                                                \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                           \
    .doneIfs      = 1,                                                \
    .reqMode      = ldmaCtrlReqModeBlock,                             \
    .decLoopCnt   = 0,                                                \
    .ignoreSrec   = 0,                                                \
    .srcInc       = ldmaCtrlSrcIncNone,                               \
    .size         = ldmaCtrlSizeByte,                                 \
    .dstInc       = ldmaCtrlDstIncOne,                                \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                           \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                           \
    .srcAddr      = (uint32_t)(src),                                  \
    .dstAddr      = (uint32_t)(dest),                                 \
    .linkMode     = ldmaLinkModeRel,                                  \
    .link         = 1,                                                \
    .linkAddr     = (linkjmp) * 4                                     \
  }                                                                   \
}

/**
 * @brief
 *   DMA descriptor initializer for byte transfers from memory to a peripheral
 * @param[in] src       Source data address.
 * @param[in] dest      Peripheral data register destination address.
 * @param[in] count     Number of bytes to transfer.
 * @param[in] linkjmp   Address of descriptor to link to expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(src, dest, count, linkjmp)   \
{                                                                     \
  .xfer =                                                             \
  {                                                                   \
    .structType   = ldmaCtrlStructTypeXfer,                           \
    .structReq    = 0,                                                \
    .xferCnt      = (count) - 1,                                      \
    .byteSwap     = 0,                                                \
    .blockSize    = ldmaCtrlBlockSizeUnit1,                           \
    .doneIfs      = 1,                                                \
    .reqMode      = ldmaCtrlReqModeBlock,                             \
    .decLoopCnt   = 0,                                                \
    .ignoreSrec   = 0,                                                \
    .srcInc       = ldmaCtrlSrcIncOne,                                \
    .size         = ldmaCtrlSizeByte,                                 \
    .dstInc       = ldmaCtrlDstIncNone,                               \
    .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                           \
    .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                           \
    .srcAddr      = (uint32_t)(src),                                  \
    .dstAddr      = (uint32_t)(dest),                                 \
    .linkMode     = ldmaLinkModeRel,                                  \
    .link         = 1,                                                \
    .linkAddr     = (linkjmp) * 4                                     \
  }                                                                   \
}

/**
 * @brief
 *   DMA descriptor initializer for Immediate WRITE transfer
 * @param[in] value     Immediate value to write.
 * @param[in] address   Write sddress.
 */
#define LDMA_DESCRIPTOR_SINGLE_WRITE(value, address)    \
{                                                       \
  .wri =                                                \
  {                                                     \
    .structType   = ldmaCtrlStructTypeWrite,            \
    .structReq    = 1,                                  \
    .xferCnt      = 0,                                  \
    .byteSwap     = 0,                                  \
    .blockSize    = 0,                                  \
    .doneIfs      = 1,                                  \
    .reqMode      = 0,                                  \
    .decLoopCnt   = 0,                                  \
    .ignoreSrec   = 0,                                  \
    .srcInc       = 0,                                  \
    .size         = 0,                                  \
    .dstInc       = 0,                                  \
    .srcAddrMode  = 0,                                  \
    .dstAddrMode  = 0,                                  \
    .immVal       = (value),                            \
    .dstAddr      = (uint32_t)(address),                \
    .linkMode     = 0,                                  \
    .link         = 0,                                  \
    .linkAddr     = 0                                   \
  }                                                     \
}

/**
 * @brief
 *   DMA descriptor initializer for Immediate WRITE transfer
 *
 *   The link address must be an absolute address.
 * @note
 *   The linkAddr member of the transfer descriptor is not
 *   initialized.
 * @param[in] value     Immediate value to write.
 * @param[in] address   Write sddress.
 */
#define LDMA_DESCRIPTOR_LINKABS_WRITE(value, address)    \
{                                                        \
  .wri =                                                 \
  {                                                      \
    .structType   = ldmaCtrlStructTypeWrite,             \
    .structReq    = 1,                                   \
    .xferCnt      = 0,                                   \
    .byteSwap     = 0,                                   \
    .blockSize    = 0,                                   \
    .doneIfs      = 0,                                   \
    .reqMode      = 0,                                   \
    .decLoopCnt   = 0,                                   \
    .ignoreSrec   = 0,                                   \
    .srcInc       = 0,                                   \
    .size         = 0,                                   \
    .dstInc       = 0,                                   \
    .srcAddrMode  = 0,                                   \
    .dstAddrMode  = 0,                                   \
    .immVal       = (value),                             \
    .dstAddr      = (uint32_t)(address),                 \
    .linkMode     = ldmaLinkModeAbs,                     \
    .link         = 1,                                   \
    .linkAddr     = 0   /* Must be set runtime ! */      \
  }                                                      \
}

/**
 * @brief
 *   DMA descriptor initializer for Immediate WRITE transfer
 * @param[in] value     Immediate value to write.
 * @param[in] address   Write sddress.
 * @param[in] linkjmp   Address of descriptor to link to expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_WRITE(value, address, linkjmp)    \
{                                                                 \
  .wri =                                                          \
  {                                                               \
    .structType   = ldmaCtrlStructTypeWrite,                      \
    .structReq    = 1,                                            \
    .xferCnt      = 0,                                            \
    .byteSwap     = 0,                                            \
    .blockSize    = 0,                                            \
    .doneIfs      = 0,                                            \
    .reqMode      = 0,                                            \
    .decLoopCnt   = 0,                                            \
    .ignoreSrec   = 0,                                            \
    .srcInc       = 0,                                            \
    .size         = 0,                                            \
    .dstInc       = 0,                                            \
    .srcAddrMode  = 0,                                            \
    .dstAddrMode  = 0,                                            \
    .immVal       = (value),                                      \
    .dstAddr      = (uint32_t)(address),                          \
    .linkMode     = ldmaLinkModeRel,                              \
    .link         = 1,                                            \
    .linkAddr     = (linkjmp) * 4                                 \
  }                                                               \
}

/**
 * @brief
 *   DMA descriptor initializer for SYNC transfer
 * @param[in] set          Sync pattern bits to set.
 * @param[in] clr          Sync pattern bits to clear.
 * @param[in] matchValue   Sync pattern to match.
 * @param[in] matchEnable  Sync pattern bits to enable for match.
 */
#define LDMA_DESCRIPTOR_SINGLE_SYNC(set, clr, matchValue, matchEnable)    \
{                                                                         \
  .sync =                                                                 \
  {                                                                       \
    .structType   = ldmaCtrlStructTypeSync,                               \
    .structReq    = 1,                                                    \
    .xferCnt      = 0,                                                    \
    .byteSwap     = 0,                                                    \
    .blockSize    = 0,                                                    \
    .doneIfs      = 1,                                                    \
    .reqMode      = 0,                                                    \
    .decLoopCnt   = 0,                                                    \
    .ignoreSrec   = 0,                                                    \
    .srcInc       = 0,                                                    \
    .size         = 0,                                                    \
    .dstInc       = 0,                                                    \
    .srcAddrMode  = 0,                                                    \
    .dstAddrMode  = 0,                                                    \
    .syncSet      = (set),                                                \
    .syncClr      = (clr),                                                \
    .matchVal     = (matchValue),                                         \
    .matchEn      = (matchEnable),                                        \
    .linkMode     = 0,                                                    \
    .link         = 0,                                                    \
    .linkAddr     = 0                                                     \
  }                                                                       \
}

/**
 * @brief
 *   DMA descriptor initializer for SYNC transfer
 *
 *   The link address must be an absolute address.
 * @note
 *   The linkAddr member of the transfer descriptor is not
 *   initialized.
 * @param[in] set          Sync pattern bits to set.
 * @param[in] clr          Sync pattern bits to clear.
 * @param[in] matchValue   Sync pattern to match.
 * @param[in] matchEnable  Sync pattern bits to enable for match.
 */
#define LDMA_DESCRIPTOR_LINKABS_SYNC(set, clr, matchValue, matchEnable)   \
{                                                                         \
  .sync =                                                                 \
  {                                                                       \
    .structType   = ldmaCtrlStructTypeSync,                               \
    .structReq    = 1,                                                    \
    .xferCnt      = 0,                                                    \
    .byteSwap     = 0,                                                    \
    .blockSize    = 0,                                                    \
    .doneIfs      = 0,                                                    \
    .reqMode      = 0,                                                    \
    .decLoopCnt   = 0,                                                    \
    .ignoreSrec   = 0,                                                    \
    .srcInc       = 0,                                                    \
    .size         = 0,                                                    \
    .dstInc       = 0,                                                    \
    .srcAddrMode  = 0,                                                    \
    .dstAddrMode  = 0,                                                    \
    .syncSet      = (set),                                                \
    .syncClr      = (clr),                                                \
    .matchVal     = (matchValue),                                         \
    .matchEn      = (matchEnable),                                        \
    .linkMode     = ldmaLinkModeAbs,                                      \
    .link         = 1,                                                    \
    .linkAddr     = 0   /* Must be set runtime ! */                       \
  }                                                                       \
}

/**
 * @brief
 *   DMA descriptor initializer for SYNC transfer
 * @param[in] set          Sync pattern bits to set.
 * @param[in] clr          Sync pattern bits to clear.
 * @param[in] matchValue   Sync pattern to match.
 * @param[in] matchEnable  Sync pattern bits to enable for match.
 * @param[in] linkjmp   Address of descriptor to link to expressed as a
 *                      signed number of descriptors from "here".
 *                      1=one descriptor forward in memory,
 *                      0=one this descriptor,
 *                      -1=one descriptor back in memory.
 */
#define LDMA_DESCRIPTOR_LINKREL_SYNC(set, clr, matchValue, matchEnable, linkjmp) \
{                                                                                \
  .sync =                                                                        \
  {                                                                              \
    .structType   = ldmaCtrlStructTypeSync,                                      \
    .structReq    = 1,                                                           \
    .xferCnt      = 0,                                                           \
    .byteSwap     = 0,                                                           \
    .blockSize    = 0,                                                           \
    .doneIfs      = 0,                                                           \
    .reqMode      = 0,                                                           \
    .decLoopCnt   = 0,                                                           \
    .ignoreSrec   = 0,                                                           \
    .srcInc       = 0,                                                           \
    .size         = 0,                                                           \
    .dstInc       = 0,                                                           \
    .srcAddrMode  = 0,                                                           \
    .dstAddrMode  = 0,                                                           \
    .syncSet      = (set),                                                       \
    .syncClr      = (clr),                                                       \
    .matchVal     = (matchValue),                                                \
    .matchEn      = (matchEnable),                                               \
    .linkMode     = ldmaLinkModeRel,                                             \
    .link         = 1,                                                           \
    .linkAddr     = (linkjmp) * 4                                                \
  }                                                                              \
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
 *   Clear one or more pending LDMA interrupts.
 *
 * @param[in] flags
 *   Pending LDMA interrupt sources to clear. Use one or more valid
 *   interrupt flags for the LDMA module. The flags are @ref LDMA_IFC_ERROR
 *   and one done flag for each channel.
 ******************************************************************************/
__STATIC_INLINE void LDMA_IntClear(uint32_t flags)
{
  LDMA->IFC = flags;
}


/***************************************************************************//**
 * @brief
 *   Disable one or more LDMA interrupts.
 *
 * @param[in] flags
 *   LDMA interrupt sources to disable. Use one or more valid
 *   interrupt flags for the LDMA module. The flags are @ref LDMA_IEN_ERROR
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
 *   enabling the interrupt. Consider using LDMA_IntClear() prior to enabling
 *   if such a pending interrupt should be ignored.
 *
 * @param[in] flags
 *   LDMA interrupt sources to enable. Use one or more valid
 *   interrupt flags for the LDMA module. The flags are @ref LDMA_IEN_ERROR
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
 *   The event bits are not cleared by the use of this function.
 *
 * @return
 *   LDMA interrupt sources pending. Returns one or more valid
 *   interrupt flags for the LDMA module. The flags are @ref LDMA_IF_ERROR and
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
 *   The return value is the bitwise AND of
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
 *   interrupt flags for the LDMA module. The flags are @ref LDMA_IFS_ERROR and
 *   one done flag for each LDMA channel.
 ******************************************************************************/
__STATIC_INLINE void LDMA_IntSet(uint32_t flags)
{
  LDMA->IFS = flags;
}

/** @} (end addtogroup LDMA) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined( LDMA_PRESENT ) && ( LDMA_COUNT == 1 ) */
#endif /* EM_LDMA_H */
