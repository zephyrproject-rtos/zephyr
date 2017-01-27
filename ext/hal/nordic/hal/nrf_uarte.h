/* Copyright (c) 2015-2017 Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
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
 */

#ifndef NRF_UARTE_H__
#define NRF_UARTE_H__

#include "nrf.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NRF_UARTE_PSEL_DISCONNECTED 0xFFFFFFFF

/**
 * @defgroup nrf_uarte_hal UARTE HAL
 * @{
 * @ingroup nrf_uart
 *
 * @brief Hardware access layer for accessing the UARTE peripheral.
 */

/**
 * @enum  nrf_uarte_task_t
 * @brief UARTE tasks.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_UARTE_TASK_STARTRX   = offsetof(NRF_UARTE_Type, TASKS_STARTRX),///< Start UART receiver.
    NRF_UARTE_TASK_STOPRX    = offsetof(NRF_UARTE_Type, TASKS_STOPRX), ///< Stop UART receiver.
    NRF_UARTE_TASK_STARTTX   = offsetof(NRF_UARTE_Type, TASKS_STARTTX),///< Start UART transmitter.
    NRF_UARTE_TASK_STOPTX    = offsetof(NRF_UARTE_Type, TASKS_STOPTX), ///< Stop UART transmitter.
    NRF_UARTE_TASK_FLUSHRX   = offsetof(NRF_UARTE_Type, TASKS_FLUSHRX) ///< Flush RX FIFO in RX buffer.
    /*lint -restore*/
} nrf_uarte_task_t;

/**
 * @enum  nrf_uarte_event_t
 * @brief UARTE events.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_UARTE_EVENT_CTS       = offsetof(NRF_UARTE_Type, EVENTS_CTS),      ///< CTS is activated.
    NRF_UARTE_EVENT_NCTS      = offsetof(NRF_UARTE_Type, EVENTS_NCTS),     ///< CTS is deactivated.
    NRF_UARTE_EVENT_ENDRX     = offsetof(NRF_UARTE_Type, EVENTS_ENDRX),    ///< Receive buffer is filled up.
    NRF_UARTE_EVENT_ENDTX     = offsetof(NRF_UARTE_Type, EVENTS_ENDTX),    ///< Last TX byte transmitted.
    NRF_UARTE_EVENT_ERROR     = offsetof(NRF_UARTE_Type, EVENTS_ERROR),    ///< Error detected.
    NRF_UARTE_EVENT_RXTO      = offsetof(NRF_UARTE_Type, EVENTS_RXTO),     ///< Receiver timeout.
    NRF_UARTE_EVENT_RXSTARTED = offsetof(NRF_UARTE_Type, EVENTS_RXSTARTED),///< Receiver has started.
    NRF_UARTE_EVENT_TXSTARTED = offsetof(NRF_UARTE_Type, EVENTS_TXSTARTED),///< Transmitter has started.
    NRF_UARTE_EVENT_TXSTOPPED = offsetof(NRF_UARTE_Type, EVENTS_TXSTOPPED) ///< Transmitted stopped.
    /*lint -restore*/
} nrf_uarte_event_t;

/**
 * @brief Types of UARTE shortcuts.
 */
typedef enum
{
    NRF_UARTE_SHORT_ENDRX_STARTRX = UARTE_SHORTS_ENDRX_STARTRX_Msk,///< Shortcut between ENDRX event and STARTRX task.
    NRF_UARTE_SHORT_ENDRX_STOPRX  = UARTE_SHORTS_ENDRX_STOPRX_Msk, ///< Shortcut between ENDRX event and STOPRX task.
} nrf_uarte_short_t;


/**
 * @enum  nrf_uarte_int_mask_t
 * @brief UARTE interrupts.
 */
typedef enum
{
    NRF_UARTE_INT_CTS_MASK       = UARTE_INTENSET_CTS_Msk,      ///< Interrupt on CTS event.
    NRF_UARTE_INT_NCTSRX_MASK    = UARTE_INTENSET_NCTS_Msk,     ///< Interrupt on NCTS event.
    NRF_UARTE_INT_ENDRX_MASK     = UARTE_INTENSET_ENDRX_Msk,    ///< Interrupt on ENDRX event.
    NRF_UARTE_INT_ENDTX_MASK     = UARTE_INTENSET_ENDTX_Msk,    ///< Interrupt on ENDTX event.
    NRF_UARTE_INT_ERROR_MASK     = UARTE_INTENSET_ERROR_Msk,    ///< Interrupt on ERROR event.
    NRF_UARTE_INT_RXTO_MASK      = UARTE_INTENSET_RXTO_Msk,     ///< Interrupt on RXTO event.
    NRF_UARTE_INT_RXSTARTED_MASK = UARTE_INTENSET_RXSTARTED_Msk,///< Interrupt on RXSTARTED event.
    NRF_UARTE_INT_TXSTARTED_MASK = UARTE_INTENSET_TXSTARTED_Msk,///< Interrupt on TXSTARTED event.
    NRF_UARTE_INT_TXSTOPPED_MASK = UARTE_INTENSET_TXSTOPPED_Msk ///< Interrupt on TXSTOPPED event.
} nrf_uarte_int_mask_t;

/**
 * @enum nrf_uarte_baudrate_t
 * @brief Baudrates supported by UARTE.
 */
typedef enum
{
    NRF_UARTE_BAUDRATE_1200   =  UARTE_BAUDRATE_BAUDRATE_Baud1200,   ///< 1200 baud.
    NRF_UARTE_BAUDRATE_2400   =  UARTE_BAUDRATE_BAUDRATE_Baud2400,   ///< 2400 baud.
    NRF_UARTE_BAUDRATE_4800   =  UARTE_BAUDRATE_BAUDRATE_Baud4800,   ///< 4800 baud.
    NRF_UARTE_BAUDRATE_9600   =  UARTE_BAUDRATE_BAUDRATE_Baud9600,   ///< 9600 baud.
    NRF_UARTE_BAUDRATE_14400  =  UARTE_BAUDRATE_BAUDRATE_Baud14400,  ///< 14400 baud.
    NRF_UARTE_BAUDRATE_19200  =  UARTE_BAUDRATE_BAUDRATE_Baud19200,  ///< 19200 baud.
    NRF_UARTE_BAUDRATE_28800  =  UARTE_BAUDRATE_BAUDRATE_Baud28800,  ///< 28800 baud.
    NRF_UARTE_BAUDRATE_38400  =  UARTE_BAUDRATE_BAUDRATE_Baud38400,  ///< 38400 baud.
    NRF_UARTE_BAUDRATE_57600  =  UARTE_BAUDRATE_BAUDRATE_Baud57600,  ///< 57600 baud.
    NRF_UARTE_BAUDRATE_76800  =  UARTE_BAUDRATE_BAUDRATE_Baud76800,  ///< 76800 baud.
    NRF_UARTE_BAUDRATE_115200 =  UARTE_BAUDRATE_BAUDRATE_Baud115200, ///< 115200 baud.
    NRF_UARTE_BAUDRATE_230400 =  UARTE_BAUDRATE_BAUDRATE_Baud230400, ///< 230400 baud.
    NRF_UARTE_BAUDRATE_250000 =  UARTE_BAUDRATE_BAUDRATE_Baud250000, ///< 250000 baud.
    NRF_UARTE_BAUDRATE_460800 =  UARTE_BAUDRATE_BAUDRATE_Baud460800, ///< 460800 baud.
    NRF_UARTE_BAUDRATE_921600 =  UARTE_BAUDRATE_BAUDRATE_Baud921600, ///< 921600 baud.
    NRF_UARTE_BAUDRATE_1000000 =  UARTE_BAUDRATE_BAUDRATE_Baud1M,    ///< 1000000 baud.
} nrf_uarte_baudrate_t;

/**
 * @enum nrf_uarte_error_mask_t
 * @brief Types of UARTE error masks.
 */
typedef enum
{
    NRF_UARTE_ERROR_OVERRUN_MASK = UARTE_ERRORSRC_OVERRUN_Msk,   ///< Overrun error.
    NRF_UARTE_ERROR_PARITY_MASK  = UARTE_ERRORSRC_PARITY_Msk,    ///< Parity error.
    NRF_UARTE_ERROR_FRAMING_MASK = UARTE_ERRORSRC_FRAMING_Msk,   ///< Framing error.
    NRF_UARTE_ERROR_BREAK_MASK   = UARTE_ERRORSRC_BREAK_Msk,     ///< Break error.
} nrf_uarte_error_mask_t;

/**
 * @enum nrf_uarte_parity_t
 * @brief Types of UARTE parity modes.
 */
typedef enum
{
    NRF_UARTE_PARITY_EXCLUDED = UARTE_CONFIG_PARITY_Excluded << UARTE_CONFIG_PARITY_Pos, ///< Parity excluded.
    NRF_UARTE_PARITY_INCLUDED = UARTE_CONFIG_PARITY_Included << UARTE_CONFIG_PARITY_Pos, ///< Parity included.
} nrf_uarte_parity_t;

/**
 * @enum nrf_uarte_hwfc_t
 * @brief Types of UARTE flow control modes.
 */
typedef enum
{
    NRF_UARTE_HWFC_DISABLED = UARTE_CONFIG_HWFC_Disabled << UARTE_CONFIG_HWFC_Pos, ///< HW flow control disabled.
    NRF_UARTE_HWFC_ENABLED  = UARTE_CONFIG_HWFC_Enabled  << UARTE_CONFIG_HWFC_Pos, ///< HW flow control enabled.
} nrf_uarte_hwfc_t;


/**
 * @brief Function for clearing a specific UARTE event.
 *
 * @param[in] p_reg  Pointer to the peripheral registers structure.
 * @param[in] event  Event to clear.
 */
__STATIC_INLINE void nrf_uarte_event_clear(NRF_UARTE_Type * p_reg, nrf_uarte_event_t event);

/**
 * @brief Function for checking the state of a specific UARTE event.
 *
 * @param[in] p_reg  Pointer to the peripheral registers structure.
 * @param[in] event  Event to check.
 *
 * @retval True if event is set, False otherwise.
 */
__STATIC_INLINE bool nrf_uarte_event_check(NRF_UARTE_Type * p_reg, nrf_uarte_event_t event);

/**
 * @brief Function for returning the address of a specific UARTE event register.
 *
 * @param[in] p_reg  Pointer to the peripheral registers structure.
 * @param[in] event  Desired event.
 *
 * @retval Address of specified event register.
 */
__STATIC_INLINE uint32_t nrf_uarte_event_address_get(NRF_UARTE_Type  * p_reg,
                                                    nrf_uarte_event_t  event);

/**
 * @brief Function for enabling UARTE shortcuts.
 *
 * @param p_reg       Pointer to the peripheral registers structure.
 * @param shorts_mask Shortcuts to enable.
 */
__STATIC_INLINE void nrf_uarte_shorts_enable(NRF_UARTE_Type * p_reg, uint32_t shorts_mask);

/**
 * @brief Function for disabling UARTE shortcuts.
 *
 * @param p_reg       Pointer to the peripheral registers structure.
 * @param shorts_mask Shortcuts to disable.
 */
__STATIC_INLINE void nrf_uarte_shorts_disable(NRF_UARTE_Type * p_reg, uint32_t shorts_mask);

/**
 * @brief Function for enabling UARTE interrupts.
 *
 * @param p_reg     Pointer to the peripheral registers structure.
 * @param int_mask  Interrupts to enable.
 */
__STATIC_INLINE void nrf_uarte_int_enable(NRF_UARTE_Type * p_reg, uint32_t int_mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param p_reg     Pointer to the peripheral registers structure.
 * @param int_mask  Mask of interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_uarte_int_enable_check(NRF_UARTE_Type * p_reg, nrf_uarte_int_mask_t int_mask);

/**
 * @brief Function for disabling specific interrupts.
 *
 * @param p_reg    Instance.
 * @param int_mask Interrupts to disable.
 */
__STATIC_INLINE void nrf_uarte_int_disable(NRF_UARTE_Type * p_reg, uint32_t int_mask);

/**
 * @brief Function for getting error source mask. Function is clearing error source flags after reading.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 * @return         Mask with error source flags.
 */
__STATIC_INLINE uint32_t nrf_uarte_errorsrc_get_and_clear(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for enabling UARTE.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_uarte_enable(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for disabling UARTE.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_uarte_disable(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for configuring TX/RX pins.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 * @param pseltxd  TXD pin number.
 * @param pselrxd  RXD pin number.
 */
__STATIC_INLINE void nrf_uarte_txrx_pins_set(NRF_UARTE_Type * p_reg, uint32_t pseltxd, uint32_t pselrxd);

/**
 * @brief Function for disconnecting TX/RX pins.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_uarte_txrx_pins_disconnect(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for getting TX pin.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 */
__STATIC_INLINE uint32_t nrf_uarte_tx_pin_get(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for getting RX pin.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 */
__STATIC_INLINE uint32_t nrf_uarte_rx_pin_get(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for getting RTS pin.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 */
__STATIC_INLINE uint32_t nrf_uarte_rts_pin_get(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for getting CTS pin.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 */
__STATIC_INLINE uint32_t nrf_uarte_cts_pin_get(NRF_UARTE_Type * p_reg);


/**
 * @brief Function for configuring flow control pins.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 * @param pselrts  RTS pin number.
 * @param pselcts  CTS pin number.
 */
__STATIC_INLINE void nrf_uarte_hwfc_pins_set(NRF_UARTE_Type * p_reg,
                                                uint32_t        pselrts,
                                                uint32_t        pselcts);

/**
 * @brief Function for disconnecting flow control pins.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_uarte_hwfc_pins_disconnect(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for starting an UARTE task.
 *
 * @param p_reg    Pointer to the peripheral registers structure.
 * @param task     Task.
 */
__STATIC_INLINE void nrf_uarte_task_trigger(NRF_UARTE_Type * p_reg, nrf_uarte_task_t task);

/**
 * @brief Function for returning the address of a specific task register.
 *
 * @param p_reg Pointer to the peripheral registers structure.
 * @param task  Task.
 *
 * @return      Task address.
 */
__STATIC_INLINE uint32_t nrf_uarte_task_address_get(NRF_UARTE_Type * p_reg, nrf_uarte_task_t task);

/**
 * @brief Function for configuring UARTE.
 *
 * @param p_reg  Pointer to the peripheral registers structure.
 * @param hwfc   Hardware flow control. Enabled if true.
 * @param parity Parity. Included if true.
 */
__STATIC_INLINE void nrf_uarte_configure(NRF_UARTE_Type   * p_reg,
                                            nrf_uarte_parity_t parity,
                                            nrf_uarte_hwfc_t   hwfc);


/**
 * @brief Function for setting UARTE baudrate.
 *
 * @param p_reg    Instance.
 * @param baudrate Baudrate.
 */
__STATIC_INLINE void nrf_uarte_baudrate_set(NRF_UARTE_Type   * p_reg, nrf_uarte_baudrate_t baudrate);

/**
 * @brief Function for setting the transmit buffer.
 *
 * @param[in] p_reg     Instance.
 * @param[in] p_buffer  Pointer to the buffer with data to send.
 * @param[in] length    Maximum number of data bytes to transmit.
 */
__STATIC_INLINE void nrf_uarte_tx_buffer_set(NRF_UARTE_Type * p_reg,
                                             uint8_t  const * p_buffer,
                                             uint8_t          length);

/**
 * @brief Function for getting number of bytes transmitted in the last transaction.
 *
 * @param[in] p_reg     Instance.
 *
 * @retval Amount of bytes transmitted.
 */
__STATIC_INLINE uint32_t nrf_uarte_tx_amount_get(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for setting the receive buffer.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] p_buffer  Pointer to the buffer for received data.
 * @param[in] length    Maximum number of data bytes to receive.
 */
__STATIC_INLINE void nrf_uarte_rx_buffer_set(NRF_UARTE_Type * p_reg,
                                             uint8_t * p_buffer,
                                             uint8_t   length);

/**
 * @brief Function for getting number of bytes received in the last transaction.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 *
 * @retval Amount of bytes received.
 */
__STATIC_INLINE uint32_t nrf_uarte_rx_amount_get(NRF_UARTE_Type * p_reg);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION
__STATIC_INLINE void nrf_uarte_event_clear(NRF_UARTE_Type * p_reg, nrf_uarte_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event));
    (void)dummy;
#endif

}

__STATIC_INLINE bool nrf_uarte_event_check(NRF_UARTE_Type * p_reg, nrf_uarte_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

__STATIC_INLINE uint32_t nrf_uarte_event_address_get(NRF_UARTE_Type  * p_reg,
                                                    nrf_uarte_event_t  event)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)event);
}

__STATIC_INLINE void nrf_uarte_shorts_enable(NRF_UARTE_Type * p_reg, uint32_t shorts_mask)
{
    p_reg->SHORTS |= shorts_mask;
}

__STATIC_INLINE void nrf_uarte_shorts_disable(NRF_UARTE_Type * p_reg, uint32_t shorts_mask)
{
    p_reg->SHORTS &= ~(shorts_mask);
}

__STATIC_INLINE void nrf_uarte_int_enable(NRF_UARTE_Type * p_reg, uint32_t int_mask)
{
    p_reg->INTENSET = int_mask;
}

__STATIC_INLINE bool nrf_uarte_int_enable_check(NRF_UARTE_Type * p_reg, nrf_uarte_int_mask_t int_mask)
{
    return (bool)(p_reg->INTENSET & int_mask);
}

__STATIC_INLINE void nrf_uarte_int_disable(NRF_UARTE_Type * p_reg, uint32_t int_mask)
{
    p_reg->INTENCLR = int_mask;
}

__STATIC_INLINE uint32_t nrf_uarte_errorsrc_get_and_clear(NRF_UARTE_Type * p_reg)
{
    uint32_t errsrc_mask = p_reg->ERRORSRC;
    p_reg->ERRORSRC = errsrc_mask;
    return errsrc_mask;
}

__STATIC_INLINE void nrf_uarte_enable(NRF_UARTE_Type * p_reg)
{
    p_reg->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
}

__STATIC_INLINE void nrf_uarte_disable(NRF_UARTE_Type * p_reg)
{
    p_reg->ENABLE = UARTE_ENABLE_ENABLE_Disabled;
}

__STATIC_INLINE void nrf_uarte_txrx_pins_set(NRF_UARTE_Type * p_reg, uint32_t pseltxd, uint32_t pselrxd)
{
    p_reg->PSEL.TXD = pseltxd;
    p_reg->PSEL.RXD = pselrxd;
}

__STATIC_INLINE void nrf_uarte_txrx_pins_disconnect(NRF_UARTE_Type * p_reg)
{
    nrf_uarte_txrx_pins_set(p_reg, NRF_UARTE_PSEL_DISCONNECTED, NRF_UARTE_PSEL_DISCONNECTED);
}

__STATIC_INLINE uint32_t nrf_uarte_tx_pin_get(NRF_UARTE_Type * p_reg)
{
    return p_reg->PSEL.TXD;
}

__STATIC_INLINE uint32_t nrf_uarte_rx_pin_get(NRF_UARTE_Type * p_reg)
{
    return p_reg->PSEL.RXD;
}

__STATIC_INLINE uint32_t nrf_uarte_rts_pin_get(NRF_UARTE_Type * p_reg)
{
    return p_reg->PSEL.RTS;
}

__STATIC_INLINE uint32_t nrf_uarte_cts_pin_get(NRF_UARTE_Type * p_reg)
{
    return p_reg->PSEL.CTS;
}

__STATIC_INLINE void nrf_uarte_hwfc_pins_set(NRF_UARTE_Type * p_reg, uint32_t pselrts, uint32_t pselcts)
{
    p_reg->PSEL.RTS = pselrts;
    p_reg->PSEL.CTS = pselcts;
}

__STATIC_INLINE void nrf_uarte_hwfc_pins_disconnect(NRF_UARTE_Type * p_reg)
{
    nrf_uarte_hwfc_pins_set(p_reg, NRF_UARTE_PSEL_DISCONNECTED, NRF_UARTE_PSEL_DISCONNECTED);
}

__STATIC_INLINE void nrf_uarte_task_trigger(NRF_UARTE_Type * p_reg, nrf_uarte_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_uarte_task_address_get(NRF_UARTE_Type * p_reg, nrf_uarte_task_t task)
{
    return (uint32_t)p_reg + (uint32_t)task;
}

__STATIC_INLINE void nrf_uarte_configure(NRF_UARTE_Type   * p_reg,
                                            nrf_uarte_parity_t parity,
                                            nrf_uarte_hwfc_t   hwfc)
{
    p_reg->CONFIG = (uint32_t)parity | (uint32_t)hwfc;
}

__STATIC_INLINE void nrf_uarte_baudrate_set(NRF_UARTE_Type   * p_reg, nrf_uarte_baudrate_t baudrate)
{
    p_reg->BAUDRATE = baudrate;
}

__STATIC_INLINE void nrf_uarte_tx_buffer_set(NRF_UARTE_Type * p_reg,
                                             uint8_t  const * p_buffer,
                                             uint8_t          length)
{
    p_reg->TXD.PTR    = (uint32_t)p_buffer;
    p_reg->TXD.MAXCNT = length;
}

__STATIC_INLINE uint32_t nrf_uarte_tx_amount_get(NRF_UARTE_Type * p_reg)
{
    return p_reg->TXD.AMOUNT;
}

__STATIC_INLINE void nrf_uarte_rx_buffer_set(NRF_UARTE_Type * p_reg,
                                             uint8_t * p_buffer,
                                             uint8_t   length)
{
    p_reg->RXD.PTR    = (uint32_t)p_buffer;
    p_reg->RXD.MAXCNT = length;
}

__STATIC_INLINE uint32_t nrf_uarte_rx_amount_get(NRF_UARTE_Type * p_reg)
{
    return p_reg->RXD.AMOUNT;
}
#endif //SUPPRESS_INLINE_IMPLEMENTATION
/** @} */

#ifdef __cplusplus
}
#endif

#endif //NRF_UARTE_H__
