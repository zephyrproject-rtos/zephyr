/*
 * Copyright (c) 2015 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRF_TWIS_H__
#define NRF_TWIS_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_twis_hal TWIS HAL
 * @{
 * @ingroup nrf_twis
 * @brief   Hardware access layer for managing the Two Wire Interface Slave with EasyDMA
 *          (TWIS) peripheral.
 */

/**
 * @brief TWIS tasks
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_TWIS_TASK_STOP      = offsetof(NRF_TWIS_Type, TASKS_STOP),      /**< Stop TWIS transaction */
    NRF_TWIS_TASK_SUSPEND   = offsetof(NRF_TWIS_Type, TASKS_SUSPEND),   /**< Suspend TWIS transaction */
    NRF_TWIS_TASK_RESUME    = offsetof(NRF_TWIS_Type, TASKS_RESUME),    /**< Resume TWIS transaction */
    NRF_TWIS_TASK_PREPARERX = offsetof(NRF_TWIS_Type, TASKS_PREPARERX), /**< Prepare the TWIS slave to respond to a write command */
    NRF_TWIS_TASK_PREPARETX = offsetof(NRF_TWIS_Type, TASKS_PREPARETX)  /**< Prepare the TWIS slave to respond to a read command */
    /*lint -restore*/
} nrf_twis_task_t;

/**
 * @brief TWIS events
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_TWIS_EVENT_STOPPED   = offsetof(NRF_TWIS_Type, EVENTS_STOPPED),   /**< TWIS stopped */
    NRF_TWIS_EVENT_ERROR     = offsetof(NRF_TWIS_Type, EVENTS_ERROR),     /**< TWIS error */
    NRF_TWIS_EVENT_RXSTARTED = offsetof(NRF_TWIS_Type, EVENTS_RXSTARTED), /**< Receive sequence started */
    NRF_TWIS_EVENT_TXSTARTED = offsetof(NRF_TWIS_Type, EVENTS_TXSTARTED), /**< Transmit sequence started */
    NRF_TWIS_EVENT_WRITE     = offsetof(NRF_TWIS_Type, EVENTS_WRITE),     /**< Write command received */
    NRF_TWIS_EVENT_READ      = offsetof(NRF_TWIS_Type, EVENTS_READ)       /**< Read command received */
    /*lint -restore*/
} nrf_twis_event_t;

/**
 * @brief TWIS shortcuts
 */
typedef enum
{
    NRF_TWIS_SHORT_WRITE_SUSPEND_MASK   = TWIS_SHORTS_WRITE_SUSPEND_Msk,   /**< Shortcut between WRITE event and SUSPEND task */
    NRF_TWIS_SHORT_READ_SUSPEND_MASK    = TWIS_SHORTS_READ_SUSPEND_Msk,    /**< Shortcut between READ event and SUSPEND task */
} nrf_twis_short_mask_t;

/**
 * @brief TWIS interrupts
 */
typedef enum
{
    NRF_TWIS_INT_STOPPED_MASK   = TWIS_INTEN_STOPPED_Msk,   /**< Interrupt on STOPPED event */
    NRF_TWIS_INT_ERROR_MASK     = TWIS_INTEN_ERROR_Msk,     /**< Interrupt on ERROR event */
    NRF_TWIS_INT_RXSTARTED_MASK = TWIS_INTEN_RXSTARTED_Msk, /**< Interrupt on RXSTARTED event */
    NRF_TWIS_INT_TXSTARTED_MASK = TWIS_INTEN_TXSTARTED_Msk, /**< Interrupt on TXSTARTED event */
    NRF_TWIS_INT_WRITE_MASK     = TWIS_INTEN_WRITE_Msk,     /**< Interrupt on WRITE event */
    NRF_TWIS_INT_READ_MASK      = TWIS_INTEN_READ_Msk,      /**< Interrupt on READ event */
} nrf_twis_int_mask_t;

/**
 * @brief TWIS error source
 */
typedef enum
{
    NRF_TWIS_ERROR_OVERFLOW  = TWIS_ERRORSRC_OVERFLOW_Msk, /**< RX buffer overflow detected, and prevented */
    NRF_TWIS_ERROR_DATA_NACK = TWIS_ERRORSRC_DNACK_Msk,    /**< NACK sent after receiving a data byte */
    NRF_TWIS_ERROR_OVERREAD  = TWIS_ERRORSRC_OVERREAD_Msk  /**< TX buffer over-read detected, and prevented */
} nrf_twis_error_t;

/**
 * @brief TWIS address matching configuration
 */
typedef enum
{
    NRF_TWIS_CONFIG_ADDRESS0_MASK  = TWIS_CONFIG_ADDRESS0_Msk, /**< Enable or disable address matching on ADDRESS[0] */
    NRF_TWIS_CONFIG_ADDRESS1_MASK  = TWIS_CONFIG_ADDRESS1_Msk, /**< Enable or disable address matching on ADDRESS[1] */
    NRF_TWIS_CONFIG_ADDRESS01_MASK = TWIS_CONFIG_ADDRESS0_Msk | TWIS_CONFIG_ADDRESS1_Msk /**< Enable both address matching */
} nrf_twis_config_addr_mask_t;

/**
 * @brief Variable type to hold amount of data for EasyDMA
 *
 * Variable of the minimum size that can hold the amount of data to transfer.
 *
 * @note
 * Defined to make it simple to change if EasyDMA would be updated to support more data in
 * the future devices to.
 */
typedef uint8_t nrf_twis_amount_t;

/**
 * @brief Smallest variable type to hold TWI address
 *
 * Variable of the minimum size that can hold single TWI address.
 *
 * @note
 * Defined to make it simple to change if new TWI would support for example
 * 10 bit addressing mode.
 */
typedef uint8_t nrf_twis_address_t;


/**
 * @brief Function for activating a specific TWIS task.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param     task   Task.
 */
__STATIC_INLINE void nrf_twis_task_trigger(NRF_TWIS_Type * const p_reg, nrf_twis_task_t task);

/**
 * @brief Function for returning the address of a specific TWIS task register.
 *
 * @param[in]  p_reg Pointer to the peripheral registers structure.
 * @param      task   Task.
 *
 * @return Task address.
 */
__STATIC_INLINE uint32_t nrf_twis_task_address_get(
        NRF_TWIS_Type const * const p_reg,
        nrf_twis_task_t      task);

/**
 * @brief Function for clearing a specific event.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param     event  Event.
 */
__STATIC_INLINE void nrf_twis_event_clear(
        NRF_TWIS_Type     * const p_reg,
        nrf_twis_event_t   event);
/**
 * @brief Function for returning the state of a specific event.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param     event  Event.
 *
 * @retval true If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_twis_event_check(
        NRF_TWIS_Type const * const p_reg,
        nrf_twis_event_t     event);


/**
 * @brief Function for getting and clearing the state of specific event
 *
 * This function checks the state of the event and clears it.
 * @param[in,out] p_reg Pointer to the peripheral registers structure.
 * @param         event Event.
 *
 * @retval true If the event was set.
 * @retval false If the event was not set.
 */
__STATIC_INLINE bool nrf_twis_event_get_and_clear(
        NRF_TWIS_Type    * const p_reg,
        nrf_twis_event_t   event);


/**
 * @brief Function for returning the address of a specific TWIS event register.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param     event  Event.
 *
 * @return Address.
 */
__STATIC_INLINE uint32_t nrf_twis_event_address_get(
        NRF_TWIS_Type const * const p_reg,
        nrf_twis_event_t     event);

/**
 * @brief Function for setting a shortcut.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param     short_mask Shortcuts mask.
 */
__STATIC_INLINE void nrf_twis_shorts_enable(NRF_TWIS_Type * const p_reg, uint32_t short_mask);

/**
 * @brief Function for clearing shortcuts.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param     short_mask Shortcuts mask.
 */
__STATIC_INLINE void nrf_twis_shorts_disable(NRF_TWIS_Type * const p_reg, uint32_t short_mask);

/**
 * @brief Get the shorts mask
 *
 * Function returns shorts register.
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @return Flags of currently enabled shortcuts
 */
__STATIC_INLINE uint32_t nrf_twis_shorts_get(NRF_TWIS_Type * const p_reg);

/**
 * @brief Function for enabling selected interrupts.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param     int_mask Interrupts mask.
 */
__STATIC_INLINE void nrf_twis_int_enable(NRF_TWIS_Type * const p_reg, uint32_t int_mask);

/**
 * @brief Function for retrieving the state of selected interrupts.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param     int_mask Interrupts mask.
 *
 * @retval true If any of selected interrupts is enabled.
 * @retval false If none of selected interrupts is enabled.
 */
__STATIC_INLINE bool nrf_twis_int_enable_check(NRF_TWIS_Type const * const p_reg, uint32_t int_mask);

/**
 * @brief Function for disabling selected interrupts.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param     int_mask Interrupts mask.
 */
__STATIC_INLINE void nrf_twis_int_disable(NRF_TWIS_Type * const p_reg, uint32_t int_mask);

/**
 * @brief Function for retrieving and clearing the TWIS error source.
 *
 * @attention Error sources are cleared after read.
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @return Error source mask with values from @ref nrf_twis_error_t.
 */
__STATIC_INLINE uint32_t nrf_twis_error_source_get_and_clear(NRF_TWIS_Type * const p_reg);

/**
 * @brief Get information which of addresses matched
 *
 * Function returns index in the address table
 * that points to the address that already matched.
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @return Index of matched address
 */
__STATIC_INLINE uint_fast8_t nrf_twis_match_get(NRF_TWIS_Type const * p_reg);

/**
 * @brief Function for enabling TWIS.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_twis_enable(NRF_TWIS_Type * const p_reg);

/**
 * @brief Function for disabling TWIS.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_twis_disable(NRF_TWIS_Type * const p_reg);

/**
 * @brief Function for configuring TWIS pins.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param scl SCL pin number.
 * @param sda SDA pin number.
 */
__STATIC_INLINE void nrf_twis_pins_set(NRF_TWIS_Type * const p_reg, uint32_t scl, uint32_t sda);

/**
 * @brief Function for setting the receive buffer.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param     p_buf  Pointer to the buffer for received data.
 * @param     length Maximum number of data bytes to receive.
 */
__STATIC_INLINE void nrf_twis_rx_buffer_set(
        NRF_TWIS_Type     * const p_reg,
        uint8_t           * p_buf,
        nrf_twis_amount_t   length);

/**
 * @brief Function that prepares TWIS for receiving
 *
 * This function sets receive buffer and then sets NRF_TWIS_TASK_PREPARERX task.
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param     p_buf  Pointer to the buffer for received data.
 * @param     length Maximum number of data bytes to receive.
 */
__STATIC_INLINE void nrf_twis_rx_prepare(
        NRF_TWIS_Type     * const p_reg,
        uint8_t           * p_buf,
        nrf_twis_amount_t   length);

/**
 * @brief Function for getting number of bytes received in the last transaction.
 *
 * @param[in] p_reg TWIS instance.
 * @return Amount of bytes received.
 * */
__STATIC_INLINE nrf_twis_amount_t nrf_twis_rx_amount_get(NRF_TWIS_Type const * const p_reg);

/**
 * @brief Function for setting the transmit buffer.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param     p_buf  Pointer to the buffer with data to send.
 * @param     length Maximum number of data bytes to transmit.
 */
__STATIC_INLINE void nrf_twis_tx_buffer_set(
        NRF_TWIS_Type     * const p_reg,
        uint8_t const     * p_buf,
        nrf_twis_amount_t   length);

/**
 * @brief Function that prepares TWIS for transmitting
 *
 * This function sets transmit buffer and then sets NRF_TWIS_TASK_PREPARETX task.
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param     p_buf  Pointer to the buffer with data to send.
 * @param     length Maximum number of data bytes to transmit.
 */
__STATIC_INLINE void nrf_twis_tx_prepare(
        NRF_TWIS_Type     * const p_reg,
        uint8_t const     * p_buf,
        nrf_twis_amount_t   length);

/**
 * @brief Function for getting number of bytes transmitted in the last transaction.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @return Amount of bytes transmitted.
 */
__STATIC_INLINE nrf_twis_amount_t nrf_twis_tx_amount_get(NRF_TWIS_Type const * const p_reg);

/**
 * @brief Function for setting slave address
 *
 * Function sets the selected address for this TWI interface.
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param     n Index of address to set
 * @param     addr Addres to set
 * @sa nrf_twis_config_address_set
 * @sa nrf_twis_config_address_get
 */
__STATIC_INLINE void nrf_twis_address_set(
        NRF_TWIS_Type      * const p_reg,
        uint_fast8_t         n,
        nrf_twis_address_t   addr);

/**
 * @brief Function for retrieving configured slave address
 *
 * Function gets the selected address for this TWI interface.
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param n   Index of address to get
 */
__STATIC_INLINE nrf_twis_address_t nrf_twis_address_get(
        NRF_TWIS_Type const * const p_reg,
        uint_fast8_t          n);

/**
 * @brief Function for setting the device address configuration.
 *
 * @param[in] p_reg    Pointer to the peripheral registers structure.
 * @param     addr_mask Mask of address indexes of what device should answer to.
 *
 * @sa nrf_twis_address_set
 */
__STATIC_INLINE void nrf_twis_config_address_set(
        NRF_TWIS_Type              * const p_reg,
        nrf_twis_config_addr_mask_t        addr_mask);

/**
 * @brief Function for retrieving the device address configuration.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 *
 * @return Mask of address indexes of what device should answer to.
 */
__STATIC_INLINE nrf_twis_config_addr_mask_t nrf_twis_config_address_get(
        NRF_TWIS_Type const * const p_reg);

/**
 * @brief Function for setting the over-read character.
 *
 * @param[in] p_reg    Pointer to the peripheral registers structure.
 * @param[in] orc       Over-read character. Character clocked out in case of
 *                      over-read of the TXD buffer.
 */
__STATIC_INLINE void nrf_twis_orc_set(
        NRF_TWIS_Type * const p_reg,
        uint8_t         orc);

/**
 * @brief Function for setting the over-read character.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 *
 * @return Over-read character configured for selected instance.
 */
__STATIC_INLINE uint8_t nrf_twis_orc_get(NRF_TWIS_Type const * const p_reg);


/** @} */ /*  End of nrf_twis_hal */

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

/* ------------------------------------------------------------------------------------------------
 *  Internal functions
 */

/**
 * @internal
 * @brief Internal function for getting task/event register address
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @oaram     offset Offset of the register from the instance beginning
 *
 * @attention offset has to be modulo 4 value. In other case we can get hardware fault.
 * @return Pointer to the register
 */
__STATIC_INLINE volatile uint32_t* nrf_twis_getRegPtr(NRF_TWIS_Type * const p_reg, uint32_t offset)
{
    return (volatile uint32_t*)((uint8_t *)p_reg + (uint32_t)offset);
}

/**
 * @internal
 * @brief Internal function for getting task/event register address - constant version
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @oaram     offset Offset of the register from the instance beginning
 *
 * @attention offset has to be modulo 4 value. In other case we can get hardware fault.
 * @return Pointer to the register
 */
__STATIC_INLINE volatile const uint32_t* nrf_twis_getRegPtr_c(NRF_TWIS_Type const * const p_reg, uint32_t offset)
{
    return (volatile const uint32_t*)((uint8_t *)p_reg + (uint32_t)offset);
}


/* ------------------------------------------------------------------------------------------------
 *  Interface functions definitions
 */


void nrf_twis_task_trigger(NRF_TWIS_Type * const p_reg, nrf_twis_task_t task)
{
    *(nrf_twis_getRegPtr(p_reg, (uint32_t)task)) = 1UL;
}

uint32_t nrf_twis_task_address_get(
        NRF_TWIS_Type const * const p_reg,
        nrf_twis_task_t       task)
{
    return (uint32_t)nrf_twis_getRegPtr_c(p_reg, (uint32_t)task);
}

void nrf_twis_event_clear(
        NRF_TWIS_Type     * const p_reg,
        nrf_twis_event_t    event)
{
    *(nrf_twis_getRegPtr(p_reg, (uint32_t)event)) = 0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event));
    (void)dummy;
#endif
}

bool nrf_twis_event_check(
        NRF_TWIS_Type const * const p_reg,
        nrf_twis_event_t      event)
{
    return (bool)*nrf_twis_getRegPtr_c(p_reg, (uint32_t)event);
}

bool nrf_twis_event_get_and_clear(
        NRF_TWIS_Type    * const p_reg,
        nrf_twis_event_t   event)
{
    bool ret = nrf_twis_event_check(p_reg, event);
    if (ret)
    {
        nrf_twis_event_clear(p_reg, event);
    }
    return ret;
}

uint32_t nrf_twis_event_address_get(
        NRF_TWIS_Type const * const p_reg,
        nrf_twis_event_t      event)
{
    return (uint32_t)nrf_twis_getRegPtr_c(p_reg, (uint32_t)event);
}

void nrf_twis_shorts_enable(NRF_TWIS_Type * const p_reg, uint32_t short_mask)
{
    p_reg->SHORTS |= short_mask;
}

void nrf_twis_shorts_disable(NRF_TWIS_Type * const p_reg, uint32_t short_mask)
{
    if (~0U == short_mask)
    {
        /* Optimized version for "disable all" */
        p_reg->SHORTS = 0;
    }
    else
    {
        p_reg->SHORTS &= ~short_mask;
    }
}

uint32_t nrf_twis_shorts_get(NRF_TWIS_Type * const p_reg)
{
    return p_reg->SHORTS;
}

void nrf_twis_int_enable(NRF_TWIS_Type * const p_reg, uint32_t int_mask)
{
    p_reg->INTENSET = int_mask;
}

bool nrf_twis_int_enable_check(NRF_TWIS_Type const * const p_reg, uint32_t int_mask)
{
    return (bool)(p_reg->INTENSET & int_mask);
}

void nrf_twis_int_disable(NRF_TWIS_Type * const p_reg, uint32_t int_mask)
{
    p_reg->INTENCLR = int_mask;
}

uint32_t nrf_twis_error_source_get_and_clear(NRF_TWIS_Type * const p_reg)
{
    uint32_t ret = p_reg->ERRORSRC;
    p_reg->ERRORSRC = ret;
    return ret;
}

uint_fast8_t nrf_twis_match_get(NRF_TWIS_Type const * p_reg)
{
    return (uint_fast8_t)p_reg->MATCH;
}

void nrf_twis_enable(NRF_TWIS_Type * const p_reg)
{
    p_reg->ENABLE = (TWIS_ENABLE_ENABLE_Enabled << TWIS_ENABLE_ENABLE_Pos);
}

void nrf_twis_disable(NRF_TWIS_Type * const p_reg)
{
    p_reg->ENABLE = (TWIS_ENABLE_ENABLE_Disabled << TWIS_ENABLE_ENABLE_Pos);
}

void nrf_twis_pins_set(NRF_TWIS_Type * const p_reg, uint32_t scl, uint32_t sda)
{
    p_reg->PSEL.SCL = scl;
    p_reg->PSEL.SDA = sda;
}

void nrf_twis_rx_buffer_set(
        NRF_TWIS_Type     * const p_reg,
        uint8_t           * p_buf,
        nrf_twis_amount_t   length)
{
    p_reg->RXD.PTR    = (uint32_t)p_buf;
    p_reg->RXD.MAXCNT = length;
}

__STATIC_INLINE void nrf_twis_rx_prepare(
        NRF_TWIS_Type     * const p_reg,
        uint8_t           * p_buf,
        nrf_twis_amount_t   length)
{
    nrf_twis_rx_buffer_set(p_reg, p_buf, length);
    nrf_twis_task_trigger(p_reg, NRF_TWIS_TASK_PREPARERX);
}

nrf_twis_amount_t nrf_twis_rx_amount_get(NRF_TWIS_Type const * const p_reg)
{
    return (nrf_twis_amount_t)p_reg->RXD.AMOUNT;
}

void nrf_twis_tx_buffer_set(
        NRF_TWIS_Type     * const p_reg,
        uint8_t const     * p_buf,
        nrf_twis_amount_t   length)
{
    p_reg->TXD.PTR    = (uint32_t)p_buf;
    p_reg->TXD.MAXCNT = length;
}

__STATIC_INLINE void nrf_twis_tx_prepare(
        NRF_TWIS_Type     * const p_reg,
        uint8_t const     * p_buf,
        nrf_twis_amount_t   length)
{
    nrf_twis_tx_buffer_set(p_reg, p_buf, length);
    nrf_twis_task_trigger(p_reg, NRF_TWIS_TASK_PREPARETX);
}

nrf_twis_amount_t nrf_twis_tx_amount_get(NRF_TWIS_Type const * const p_reg)
{
    return (nrf_twis_amount_t)p_reg->TXD.AMOUNT;
}

void nrf_twis_address_set(
        NRF_TWIS_Type      * const p_reg,
        uint_fast8_t         n,
        nrf_twis_address_t   addr)
{
    p_reg->ADDRESS[n] = addr;
}

nrf_twis_address_t nrf_twis_address_get(
        NRF_TWIS_Type const * const p_reg,
        uint_fast8_t          n)
{
    return (nrf_twis_address_t)p_reg->ADDRESS[n];
}
void nrf_twis_config_address_set(
        NRF_TWIS_Type              * const p_reg,
        nrf_twis_config_addr_mask_t        addr_mask)
{
    /* This is the only configuration in TWIS - just write it without masking */
    p_reg->CONFIG = addr_mask;
}

nrf_twis_config_addr_mask_t nrf_twis_config_address_get(NRF_TWIS_Type const * const p_reg)
{
    return (nrf_twis_config_addr_mask_t)(p_reg->CONFIG & TWIS_ADDRESS_ADDRESS_Msk);
}

void nrf_twis_orc_set(
        NRF_TWIS_Type * const p_reg,
        uint8_t         orc)
{
    p_reg->ORC = orc;
}

uint8_t nrf_twis_orc_get(NRF_TWIS_Type const * const p_reg)
{
    return (uint8_t)p_reg->ORC;
}

#endif /* SUPPRESS_INLINE_IMPLEMENTATION */


#ifdef __cplusplus
}
#endif

#endif /* NRF_TWIS_H__ */
