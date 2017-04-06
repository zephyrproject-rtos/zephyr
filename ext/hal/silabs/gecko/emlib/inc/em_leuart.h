/***************************************************************************//**
 * @file em_leuart.h
 * @brief Low Energy Universal Asynchronous Receiver/Transmitter (LEUART)
 *   peripheral API
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

#ifndef EM_LEUART_H
#define EM_LEUART_H

#include "em_device.h"
#if defined(LEUART_COUNT) && (LEUART_COUNT > 0)

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup LEUART
 * @{
 ******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Databit selection. */
typedef enum
{
  leuartDatabits8 = LEUART_CTRL_DATABITS_EIGHT,     /**< 8 databits. */
  leuartDatabits9 = LEUART_CTRL_DATABITS_NINE       /**< 9 databits. */
} LEUART_Databits_TypeDef;


/** Enable selection. */
typedef enum
{
  /** Disable both receiver and transmitter. */
  leuartDisable  = 0x0,

  /** Enable receiver only, transmitter disabled. */
  leuartEnableRx = LEUART_CMD_RXEN,

  /** Enable transmitter only, receiver disabled. */
  leuartEnableTx = LEUART_CMD_TXEN,

  /** Enable both receiver and transmitter. */
  leuartEnable   = (LEUART_CMD_RXEN | LEUART_CMD_TXEN)
} LEUART_Enable_TypeDef;


/** Parity selection. */
typedef enum
{
  leuartNoParity   = LEUART_CTRL_PARITY_NONE,    /**< No parity. */
  leuartEvenParity = LEUART_CTRL_PARITY_EVEN,    /**< Even parity. */
  leuartOddParity  = LEUART_CTRL_PARITY_ODD      /**< Odd parity. */
} LEUART_Parity_TypeDef;


/** Stopbits selection. */
typedef enum
{
  leuartStopbits1 = LEUART_CTRL_STOPBITS_ONE,           /**< 1 stopbits. */
  leuartStopbits2 = LEUART_CTRL_STOPBITS_TWO            /**< 2 stopbits. */
} LEUART_Stopbits_TypeDef;


/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** Init structure. */
typedef struct
{
  /** Specifies whether TX and/or RX shall be enabled when init completed. */
  LEUART_Enable_TypeDef   enable;

  /**
   * LEUART reference clock assumed when configuring baudrate setup. Set
   * it to 0 if currently configurated reference clock shall be used.
   */
  uint32_t                refFreq;

  /** Desired baudrate. */
  uint32_t                baudrate;

  /** Number of databits in frame. */
  LEUART_Databits_TypeDef databits;

  /** Parity mode to use. */
  LEUART_Parity_TypeDef   parity;

  /** Number of stopbits to use. */
  LEUART_Stopbits_TypeDef stopbits;
} LEUART_Init_TypeDef;

/** Default config for LEUART init structure. */
#define LEUART_INIT_DEFAULT                                                                 \
{                                                                                           \
  leuartEnable,      /* Enable RX/TX when init completed. */                                \
  0,                 /* Use current configured reference clock for configuring baudrate. */ \
  9600,              /* 9600 bits/s. */                                                     \
  leuartDatabits8,   /* 8 databits. */                                                      \
  leuartNoParity,    /* No parity. */                                                       \
  leuartStopbits1    /* 1 stopbit. */                                                       \
}


/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

uint32_t LEUART_BaudrateCalc(uint32_t refFreq, uint32_t clkdiv);
uint32_t LEUART_BaudrateGet(LEUART_TypeDef *leuart);
void LEUART_BaudrateSet(LEUART_TypeDef *leuart,
                        uint32_t refFreq,
                        uint32_t baudrate);
void LEUART_Enable(LEUART_TypeDef *leuart, LEUART_Enable_TypeDef enable);
void LEUART_FreezeEnable(LEUART_TypeDef *leuart, bool enable);
void LEUART_Init(LEUART_TypeDef *leuart, LEUART_Init_TypeDef const *init);
void LEUART_TxDmaInEM2Enable(LEUART_TypeDef *leuart, bool enable);
void LEUART_RxDmaInEM2Enable(LEUART_TypeDef *leuart, bool enable);

/***************************************************************************//**
 * @brief
 *   Clear one or more pending LEUART interrupts.
 *
 * @param[in] leuart
 *   Pointer to LEUART peripheral register block.
 *
 * @param[in] flags
 *   Pending LEUART interrupt source to clear. Use a bitwise logic OR
 *   combination of valid interrupt flags for the LEUART module (LEUART_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void LEUART_IntClear(LEUART_TypeDef *leuart, uint32_t flags)
{
  leuart->IFC = flags;
}


/***************************************************************************//**
 * @brief
 *   Disable one or more LEUART interrupts.
 *
 * @param[in] leuart
 *   Pointer to LEUART peripheral register block.
 *
 * @param[in] flags
 *   LEUART interrupt sources to disable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the LEUART module (LEUART_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void LEUART_IntDisable(LEUART_TypeDef *leuart, uint32_t flags)
{
  leuart->IEN &= ~flags;
}


/***************************************************************************//**
 * @brief
 *   Enable one or more LEUART interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using LEUART_IntClear() prior to enabling
 *   if such a pending interrupt should be ignored.
 *
 * @param[in] leuart
 *   Pointer to LEUART peripheral register block.
 *
 * @param[in] flags
 *   LEUART interrupt sources to enable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the LEUART module (LEUART_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void LEUART_IntEnable(LEUART_TypeDef *leuart, uint32_t flags)
{
  leuart->IEN |= flags;
}


/***************************************************************************//**
 * @brief
 *   Get pending LEUART interrupt flags.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @param[in] leuart
 *   Pointer to LEUART peripheral register block.
 *
 * @return
 *   LEUART interrupt sources pending. A bitwise logic OR combination of valid
 *   interrupt flags for the LEUART module (LEUART_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t LEUART_IntGet(LEUART_TypeDef *leuart)
{
  return leuart->IF;
}


/***************************************************************************//**
 * @brief
 *   Get enabled and pending LEUART interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @param[in] leuart
 *   Pointer to LEUART peripheral register block.
 *
 * @note
 *   Interrupt flags are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled LEUART interrupt sources.
 *   The return value is the bitwise AND combination of
 *   - the OR combination of enabled interrupt sources in LEUARTx_IEN_nnn
 *     register (LEUARTx_IEN_nnn) and
 *   - the OR combination of valid interrupt flags of the LEUART module
 *     (LEUARTx_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t LEUART_IntGetEnabled(LEUART_TypeDef *leuart)
{
  uint32_t tmp;

  /* Store LEUARTx->IEN in temporary variable in order to define explicit order
   * of volatile accesses. */
  tmp = leuart->IEN;

  /* Bitwise AND of pending and enabled interrupts */
  return leuart->IF & tmp;
}


/***************************************************************************//**
 * @brief
 *   Set one or more pending LEUART interrupts from SW.
 *
 * @param[in] leuart
 *   Pointer to LEUART peripheral register block.
 *
 * @param[in] flags
 *   LEUART interrupt sources to set to pending. Use a bitwise logic OR
 *   combination of valid interrupt flags for the LEUART module (LEUART_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void LEUART_IntSet(LEUART_TypeDef *leuart, uint32_t flags)
{
  leuart->IFS = flags;
}


/***************************************************************************//**
 * @brief
 *   Get LEUART STATUS register.
 *
 * @param[in] leuart
 *   Pointer to LEUART peripheral register block.
 *
 * @return
 *  STATUS register value.
 *
 ******************************************************************************/
__STATIC_INLINE uint32_t LEUART_StatusGet(LEUART_TypeDef *leuart)
{
  return leuart->STATUS;
}

void LEUART_Reset(LEUART_TypeDef *leuart);
uint8_t LEUART_Rx(LEUART_TypeDef *leuart);
uint16_t LEUART_RxExt(LEUART_TypeDef *leuart);
void LEUART_Tx(LEUART_TypeDef *leuart, uint8_t data);
void LEUART_TxExt(LEUART_TypeDef *leuart, uint16_t data);


/***************************************************************************//**
 * @brief
 *   Receive one 8 bit frame, (or part of a 9 bit frame).
 *
 * @details
 *   This function is used to quickly receive one 8 bit frame by reading the
 *   RXDATA register directly, without checking the STATUS register for the
 *   RXDATAV flag. This can be useful from the RXDATAV interrupt handler,
 *   i.e. waiting is superfluous, in order to quickly read the received data.
 *   Please refer to @ref LEUART_RxDataXGet() for reception of 9 bit frames.
 *
 * @note
 *   Since this function does not check whether the RXDATA register actually
 *   holds valid data, it should only be used in situations when it is certain
 *   that there is valid data, ensured by some external program routine, e.g.
 *   like when handling an RXDATAV interrupt. The @ref LEUART_Rx() is normally a
 *   better choice if the validity of the RXDATA register is not certain.
 *
 * @note
 *   Notice that possible parity/stop bits are not
 *   considered part of specified frame bit length.
 *
 * @param[in] leuart
 *   Pointer to LEUART peripheral register block.
 *
 * @return
 *   Data received.
 ******************************************************************************/
__STATIC_INLINE uint8_t LEUART_RxDataGet(LEUART_TypeDef *leuart)
{
  return (uint8_t)leuart->RXDATA;
}


/***************************************************************************//**
 * @brief
 *   Receive one 8-9 bit frame, with extended information.
 *
 * @details
 *   This function is used to quickly receive one 8-9 bit frame with extended
 *   information by reading the RXDATAX register directly, without checking the
 *   STATUS register for the RXDATAV flag. This can be useful from the RXDATAV
 *   interrupt handler, i.e. waiting is superfluous, in order to quickly read
 *   the received data.
 *
 * @note
 *   Since this function does not check whether the RXDATAX register actually
 *   holds valid data, it should only be used in situations when it is certain
 *   that there is valid data, ensured by some external program routine, e.g.
 *   like when handling an RXDATAV interrupt. The @ref LEUART_RxExt() is normally
 *   a better choice if the validity of the RXDATAX register is not certain.
 *
 * @note
 *   Notice that possible parity/stop bits are not
 *   considered part of specified frame bit length.
 *
 * @param[in] leuart
 *   Pointer to LEUART peripheral register block.
 *
 * @return
 *   Data received.
 ******************************************************************************/
__STATIC_INLINE uint16_t LEUART_RxDataXGet(LEUART_TypeDef *leuart)
{
  return (uint16_t)leuart->RXDATAX;
}


/** @} (end addtogroup LEUART) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(LEUART_COUNT) && (LEUART_COUNT > 0) */
#endif /* EM_LEUART_H */
