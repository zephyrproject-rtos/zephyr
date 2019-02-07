/*
 * Copyright (c) 2015 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_SPIS_H__
#define NRF_SPIS_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_spis_hal SPIS HAL
 * @{
 * @ingroup nrf_spis
 * @brief   Hardware access layer for managing the SPIS peripheral.
 */

/**
 * @brief This value can be used as a parameter for the @ref nrf_spis_pins_set
 *        function to specify that a given SPI signal (SCK, MOSI, or MISO)
 *        shall not be connected to a physical pin.
 */
#define NRF_SPIS_PIN_NOT_CONNECTED  0xFFFFFFFF


/**
 * @brief SPIS tasks.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_SPIS_TASK_ACQUIRE = offsetof(NRF_SPIS_Type, TASKS_ACQUIRE), ///< Acquire SPI semaphore.
    NRF_SPIS_TASK_RELEASE = offsetof(NRF_SPIS_Type, TASKS_RELEASE), ///< Release SPI semaphore, enabling the SPI slave to acquire it.
    /*lint -restore*/
} nrf_spis_task_t;

/**
 * @brief SPIS events.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_SPIS_EVENT_END      = offsetof(NRF_SPIS_Type, EVENTS_END),     ///< Granted transaction completed.
    NRF_SPIS_EVENT_ACQUIRED = offsetof(NRF_SPIS_Type, EVENTS_ACQUIRED) ///< Semaphore acquired.
    /*lint -restore*/
} nrf_spis_event_t;

/**
 * @brief SPIS shortcuts.
 */
typedef enum
{
    NRF_SPIS_SHORT_END_ACQUIRE = SPIS_SHORTS_END_ACQUIRE_Msk ///< Shortcut between END event and ACQUIRE task.
} nrf_spis_short_mask_t;

/**
 * @brief SPIS interrupts.
 */
typedef enum
{
    NRF_SPIS_INT_END_MASK      = SPIS_INTENSET_END_Msk,     ///< Interrupt on END event.
    NRF_SPIS_INT_ACQUIRED_MASK = SPIS_INTENSET_ACQUIRED_Msk ///< Interrupt on ACQUIRED event.
} nrf_spis_int_mask_t;

/**
 * @brief SPI modes.
 */
typedef enum
{
    NRF_SPIS_MODE_0, ///< SCK active high, sample on leading edge of clock.
    NRF_SPIS_MODE_1, ///< SCK active high, sample on trailing edge of clock.
    NRF_SPIS_MODE_2, ///< SCK active low, sample on leading edge of clock.
    NRF_SPIS_MODE_3  ///< SCK active low, sample on trailing edge of clock.
} nrf_spis_mode_t;

/**
 * @brief SPI bit orders.
 */
typedef enum
{
    NRF_SPIS_BIT_ORDER_MSB_FIRST = SPIS_CONFIG_ORDER_MsbFirst, ///< Most significant bit shifted out first.
    NRF_SPIS_BIT_ORDER_LSB_FIRST = SPIS_CONFIG_ORDER_LsbFirst  ///< Least significant bit shifted out first.
} nrf_spis_bit_order_t;

/**
 * @brief SPI semaphore status.
 */
typedef enum
{
    NRF_SPIS_SEMSTAT_FREE       = 0, ///< Semaphore is free.
    NRF_SPIS_SEMSTAT_CPU        = 1, ///< Semaphore is assigned to the CPU.
    NRF_SPIS_SEMSTAT_SPIS       = 2, ///< Semaphore is assigned to the SPI slave.
    NRF_SPIS_SEMSTAT_CPUPENDING = 3  ///< Semaphore is assigned to the SPI, but a handover to the CPU is pending.
} nrf_spis_semstat_t;

/**
 * @brief SPIS status.
 */
typedef enum
{
    NRF_SPIS_STATUS_OVERREAD = SPIS_STATUS_OVERREAD_Msk, ///< TX buffer over-read detected and prevented.
    NRF_SPIS_STATUS_OVERFLOW = SPIS_STATUS_OVERFLOW_Msk  ///< RX buffer overflow detected and prevented.
} nrf_spis_status_mask_t;

/**
 * @brief Function for activating a specific SPIS task.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] spis_task Task to activate.
 */
__STATIC_INLINE void nrf_spis_task_trigger(NRF_SPIS_Type * p_reg,
                                           nrf_spis_task_t spis_task);

/**
 * @brief Function for getting the address of a specific SPIS task register.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] spis_task Requested task.
 *
 * @return Address of the specified task register.
 */
__STATIC_INLINE uint32_t nrf_spis_task_address_get(NRF_SPIS_Type const * p_reg,
                                                   nrf_spis_task_t spis_task);

/**
 * @brief Function for clearing a specific SPIS event.
 *
 * @param[in] p_reg      Pointer to the peripheral registers structure.
 * @param[in] spis_event Event to clear.
 */
__STATIC_INLINE void nrf_spis_event_clear(NRF_SPIS_Type * p_reg,
                                          nrf_spis_event_t spis_event);

/**
 * @brief Function for checking the state of a specific SPIS event.
 *
 * @param[in] p_reg      Pointer to the peripheral registers structure.
 * @param[in] spis_event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_spis_event_check(NRF_SPIS_Type const * p_reg,
                                          nrf_spis_event_t spis_event);

/**
 * @brief Function for getting the address of a specific SPIS event register.
 *
 * @param[in] p_reg      Pointer to the peripheral registers structure.
 * @param[in] spis_event Requested event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t nrf_spis_event_address_get(NRF_SPIS_Type const * p_reg,
                                                    nrf_spis_event_t spis_event);

/**
 * @brief Function for enabling specified shortcuts.
 *
 * @param[in] p_reg            Pointer to the peripheral registers structure.
 * @param[in] spis_shorts_mask Shortcuts to enable.
 */
__STATIC_INLINE void nrf_spis_shorts_enable(NRF_SPIS_Type * p_reg,
                                            uint32_t spis_shorts_mask);

/**
 * @brief Function for disabling specified shortcuts.
 *
 * @param[in] p_reg            Pointer to the peripheral registers structure.
 * @param[in] spis_shorts_mask Shortcuts to disable.
 */
__STATIC_INLINE void nrf_spis_shorts_disable(NRF_SPIS_Type * p_reg,
                                             uint32_t spis_shorts_mask);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg         Pointer to the peripheral registers structure.
 * @param[in] spis_int_mask Interrupts to enable.
 */
__STATIC_INLINE void nrf_spis_int_enable(NRF_SPIS_Type * p_reg,
                                         uint32_t spis_int_mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg         Pointer to the peripheral registers structure.
 * @param[in] spis_int_mask Interrupts to disable.
 */
__STATIC_INLINE void nrf_spis_int_disable(NRF_SPIS_Type * p_reg,
                                          uint32_t spis_int_mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] p_reg    Pointer to the peripheral registers structure.
 * @param[in] spis_int Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_spis_int_enable_check(NRF_SPIS_Type const * p_reg,
                                               nrf_spis_int_mask_t spis_int);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        SPIS task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
__STATIC_INLINE void nrf_spis_subscribe_set(NRF_SPIS_Type * p_reg,
                                            nrf_spis_task_t task,
                                            uint8_t         channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        SPIS task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
__STATIC_INLINE void nrf_spis_subscribe_clear(NRF_SPIS_Type * p_reg,
                                              nrf_spis_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        SPIS event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
__STATIC_INLINE void nrf_spis_publish_set(NRF_SPIS_Type *  p_reg,
                                          nrf_spis_event_t event,
                                          uint8_t          channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        SPIS event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
__STATIC_INLINE void nrf_spis_publish_clear(NRF_SPIS_Type *  p_reg,
                                            nrf_spis_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for enabling the SPIS peripheral.
 *
 * @param[in] p_reg  Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spis_enable(NRF_SPIS_Type * p_reg);

/**
 * @brief Function for disabling the SPIS peripheral.
 *
 * @param[in] p_reg  Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spis_disable(NRF_SPIS_Type * p_reg);

/**
 * @brief Function for retrieving the SPIS semaphore status.
 *
 * @param[in] p_reg  Pointer to the peripheral registers structure.
 *
 * @returns Current semaphore status.
 */
__STATIC_INLINE nrf_spis_semstat_t nrf_spis_semaphore_status_get(NRF_SPIS_Type * p_reg);

/**
 * @brief Function for retrieving the SPIS status.
 *
 * @param[in] p_reg  Pointer to the peripheral registers structure.
 *
 * @returns Current SPIS status.
 */
__STATIC_INLINE nrf_spis_status_mask_t nrf_spis_status_get(NRF_SPIS_Type * p_reg);

/**
 * @brief Function for configuring SPIS pins.
 *
 * If a given signal is not needed, pass the @ref NRF_SPIS_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] sck_pin   SCK pin number.
 * @param[in] mosi_pin  MOSI pin number.
 * @param[in] miso_pin  MISO pin number.
 * @param[in] csn_pin   CSN pin number.
 */
__STATIC_INLINE void nrf_spis_pins_set(NRF_SPIS_Type * p_reg,
                                       uint32_t sck_pin,
                                       uint32_t mosi_pin,
                                       uint32_t miso_pin,
                                       uint32_t csn_pin);

/**
 * @brief Function for setting the transmit buffer.
 *
 * @param[in]  p_reg    Pointer to the peripheral registers structure.
 * @param[in]  p_buffer Pointer to the buffer that contains the data to send.
 * @param[in]  length   Maximum number of data bytes to transmit.
 */
__STATIC_INLINE void nrf_spis_tx_buffer_set(NRF_SPIS_Type * p_reg,
                                            uint8_t const * p_buffer,
                                            size_t          length);

/**
 * @brief Function for setting the receive buffer.
 *
 * @param[in] p_reg    Pointer to the peripheral registers structure.
 * @param[in] p_buffer Pointer to the buffer for received data.
 * @param[in] length   Maximum number of data bytes to receive.
 */
__STATIC_INLINE void nrf_spis_rx_buffer_set(NRF_SPIS_Type * p_reg,
                                            uint8_t * p_buffer,
                                            size_t    length);

/**
 * @brief Function for getting the number of bytes transmitted
 *        in the last granted transaction.
 *
 * @param[in]  p_reg    Pointer to the peripheral registers structure.
 *
 * @returns Number of bytes transmitted.
 */
__STATIC_INLINE size_t nrf_spis_tx_amount_get(NRF_SPIS_Type const * p_reg);

/**
 * @brief Function for getting the number of bytes received
 *        in the last granted transaction.
 *
 * @param[in]  p_reg    Pointer to the peripheral registers structure.
 *
 * @returns Number of bytes received.
 */
__STATIC_INLINE size_t nrf_spis_rx_amount_get(NRF_SPIS_Type const * p_reg);

/**
 * @brief Function for setting the SPI configuration.
 *
 * @param[in] p_reg         Pointer to the peripheral registers structure.
 * @param[in] spi_mode      SPI mode.
 * @param[in] spi_bit_order SPI bit order.
 */
__STATIC_INLINE void nrf_spis_configure(NRF_SPIS_Type * p_reg,
                                        nrf_spis_mode_t spi_mode,
                                        nrf_spis_bit_order_t spi_bit_order);

/**
 * @brief Function for setting the default character.
 *
 * @param[in] p_reg  Pointer to the peripheral registers structure.
 * @param[in] def    Default character that is clocked out in case of
 *                   an overflow of the RXD buffer.
 */
__STATIC_INLINE void nrf_spis_def_set(NRF_SPIS_Type * p_reg,
                                      uint8_t def);

/**
 * @brief Function for setting the over-read character.
 *
 * @param[in] p_reg  Pointer to the peripheral registers structure.
 * @param[in] orc    Over-read character that is clocked out in case of
 *                   an over-read of the TXD buffer.
 */
__STATIC_INLINE void nrf_spis_orc_set(NRF_SPIS_Type * p_reg,
                                      uint8_t orc);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_spis_task_trigger(NRF_SPIS_Type * p_reg,
                                           nrf_spis_task_t spis_task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spis_task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_spis_task_address_get(NRF_SPIS_Type const * p_reg,
                                                   nrf_spis_task_t spis_task)
{
    return (uint32_t)p_reg + (uint32_t)spis_task;
}

__STATIC_INLINE void nrf_spis_event_clear(NRF_SPIS_Type *  p_reg,
                                          nrf_spis_event_t spis_event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spis_event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spis_event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_spis_event_check(NRF_SPIS_Type const * p_reg,
                                          nrf_spis_event_t spis_event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spis_event);
}

__STATIC_INLINE uint32_t nrf_spis_event_address_get(NRF_SPIS_Type const * p_reg,
                                                    nrf_spis_event_t spis_event)
{
    return (uint32_t)p_reg + (uint32_t)spis_event;
}

__STATIC_INLINE void nrf_spis_shorts_enable(NRF_SPIS_Type * p_reg,
                                            uint32_t spis_shorts_mask)
{
    p_reg->SHORTS |= spis_shorts_mask;
}

__STATIC_INLINE void nrf_spis_shorts_disable(NRF_SPIS_Type * p_reg,
                                             uint32_t spis_shorts_mask)
{
    p_reg->SHORTS &= ~(spis_shorts_mask);
}

__STATIC_INLINE void nrf_spis_int_enable(NRF_SPIS_Type * p_reg,
                                         uint32_t spis_int_mask)
{
    p_reg->INTENSET = spis_int_mask;
}

__STATIC_INLINE void nrf_spis_int_disable(NRF_SPIS_Type * p_reg,
                                          uint32_t spis_int_mask)
{
    p_reg->INTENCLR = spis_int_mask;
}

__STATIC_INLINE bool nrf_spis_int_enable_check(NRF_SPIS_Type const * p_reg,
                                               nrf_spis_int_mask_t spis_int)
{
    return (bool)(p_reg->INTENSET & spis_int);
}

#if defined(DPPI_PRESENT)
__STATIC_INLINE void nrf_spis_subscribe_set(NRF_SPIS_Type * p_reg,
                                            nrf_spis_task_t task,
                                            uint8_t        channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | SPIS_SUBSCRIBE_ACQUIRE_EN_Msk);
}

__STATIC_INLINE void nrf_spis_subscribe_clear(NRF_SPIS_Type * p_reg,
                                              nrf_spis_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

__STATIC_INLINE void nrf_spis_publish_set(NRF_SPIS_Type *  p_reg,
                                          nrf_spis_event_t event,
                                          uint8_t         channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | SPIS_PUBLISH_END_EN_Msk);
}

__STATIC_INLINE void nrf_spis_publish_clear(NRF_SPIS_Type *  p_reg,
                                            nrf_spis_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

__STATIC_INLINE void nrf_spis_enable(NRF_SPIS_Type * p_reg)
{
    p_reg->ENABLE = (SPIS_ENABLE_ENABLE_Enabled << SPIS_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_spis_disable(NRF_SPIS_Type * p_reg)
{
    p_reg->ENABLE = (SPIS_ENABLE_ENABLE_Disabled << SPIS_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE nrf_spis_semstat_t nrf_spis_semaphore_status_get(NRF_SPIS_Type * p_reg)
{
    return (nrf_spis_semstat_t) ((p_reg->SEMSTAT & SPIS_SEMSTAT_SEMSTAT_Msk)
                                 >> SPIS_SEMSTAT_SEMSTAT_Pos);
}

__STATIC_INLINE nrf_spis_status_mask_t nrf_spis_status_get(NRF_SPIS_Type * p_reg)
{
    return (nrf_spis_status_mask_t) p_reg->STATUS;
}

__STATIC_INLINE void nrf_spis_pins_set(NRF_SPIS_Type * p_reg,
                                       uint32_t sck_pin,
                                       uint32_t mosi_pin,
                                       uint32_t miso_pin,
                                       uint32_t csn_pin)
{
#if defined (NRF51)
    p_reg->PSELSCK  = sck_pin;
    p_reg->PSELMOSI = mosi_pin;
    p_reg->PSELMISO = miso_pin;
    p_reg->PSELCSN  = csn_pin;
#else
    p_reg->PSEL.SCK  = sck_pin;
    p_reg->PSEL.MOSI = mosi_pin;
    p_reg->PSEL.MISO = miso_pin;
    p_reg->PSEL.CSN  = csn_pin;
#endif
}

__STATIC_INLINE void nrf_spis_tx_buffer_set(NRF_SPIS_Type * p_reg,
                                            uint8_t const * p_buffer,
                                            size_t          length)
{
#if defined (NRF51)
    p_reg->TXDPTR = (uint32_t)p_buffer;
    p_reg->MAXTX  = length;
#else
    p_reg->TXD.PTR    = (uint32_t)p_buffer;
    p_reg->TXD.MAXCNT = length;
#endif
}

__STATIC_INLINE void nrf_spis_rx_buffer_set(NRF_SPIS_Type * p_reg,
                                            uint8_t * p_buffer,
                                            size_t    length)
{
#if defined (NRF51)
    p_reg->RXDPTR = (uint32_t)p_buffer;
    p_reg->MAXRX  = length;
#else
    p_reg->RXD.PTR    = (uint32_t)p_buffer;
    p_reg->RXD.MAXCNT = length;
#endif
}

__STATIC_INLINE size_t nrf_spis_tx_amount_get(NRF_SPIS_Type const * p_reg)
{
#if defined (NRF51)
    return p_reg->AMOUNTTX;
#else
    return p_reg->TXD.AMOUNT;
#endif
}

__STATIC_INLINE size_t nrf_spis_rx_amount_get(NRF_SPIS_Type const * p_reg)
{
#if defined (NRF51)
    return p_reg->AMOUNTRX;
#else
    return p_reg->RXD.AMOUNT;
#endif
}

__STATIC_INLINE void nrf_spis_configure(NRF_SPIS_Type * p_reg,
                                        nrf_spis_mode_t spi_mode,
                                        nrf_spis_bit_order_t spi_bit_order)
{
    uint32_t config = (spi_bit_order == NRF_SPIS_BIT_ORDER_MSB_FIRST ?
        SPIS_CONFIG_ORDER_MsbFirst : SPIS_CONFIG_ORDER_LsbFirst);

    switch (spi_mode)
    {
    default:
    case NRF_SPIS_MODE_0:
        config |= (SPIS_CONFIG_CPOL_ActiveHigh << SPIS_CONFIG_CPOL_Pos) |
                  (SPIS_CONFIG_CPHA_Leading    << SPIS_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIS_MODE_1:
        config |= (SPIS_CONFIG_CPOL_ActiveHigh << SPIS_CONFIG_CPOL_Pos) |
                  (SPIS_CONFIG_CPHA_Trailing   << SPIS_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIS_MODE_2:
        config |= (SPIS_CONFIG_CPOL_ActiveLow  << SPIS_CONFIG_CPOL_Pos) |
                  (SPIS_CONFIG_CPHA_Leading    << SPIS_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIS_MODE_3:
        config |= (SPIS_CONFIG_CPOL_ActiveLow  << SPIS_CONFIG_CPOL_Pos) |
                  (SPIS_CONFIG_CPHA_Trailing   << SPIS_CONFIG_CPHA_Pos);
        break;
    }
    p_reg->CONFIG = config;
}

__STATIC_INLINE void nrf_spis_orc_set(NRF_SPIS_Type * p_reg,
                                      uint8_t orc)
{
    p_reg->ORC = orc;
}

__STATIC_INLINE void nrf_spis_def_set(NRF_SPIS_Type * p_reg,
                                      uint8_t def)
{
    p_reg->DEF = def;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_SPIS_H__
