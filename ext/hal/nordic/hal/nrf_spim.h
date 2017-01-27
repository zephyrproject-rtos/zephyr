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

/**
 * @defgroup nrf_spim_hal SPIM HAL
 * @{
 * @ingroup nrf_spi
 *
 * @brief Hardware access layer for accessing the SPIM peripheral.
 */

#ifndef NRF_SPIM_H__
#define NRF_SPIM_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief This value can be used as a parameter for the @ref nrf_spim_pins_set
 *        function to specify that a given SPI signal (SCK, MOSI, or MISO)
 *        shall not be connected to a physical pin.
 */
#define NRF_SPIM_PIN_NOT_CONNECTED  0xFFFFFFFF


/**
 * @brief SPIM tasks.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_SPIM_TASK_START   = offsetof(NRF_SPIM_Type, TASKS_START),   ///< Start SPI transaction.
    NRF_SPIM_TASK_STOP    = offsetof(NRF_SPIM_Type, TASKS_STOP),    ///< Stop SPI transaction.
    NRF_SPIM_TASK_SUSPEND = offsetof(NRF_SPIM_Type, TASKS_SUSPEND), ///< Suspend SPI transaction.
    NRF_SPIM_TASK_RESUME  = offsetof(NRF_SPIM_Type, TASKS_RESUME)   ///< Resume SPI transaction.
    /*lint -restore*/
} nrf_spim_task_t;

/**
 * @brief SPIM events.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_SPIM_EVENT_STOPPED = offsetof(NRF_SPIM_Type, EVENTS_STOPPED), ///< SPI transaction has stopped.
    NRF_SPIM_EVENT_ENDRX   = offsetof(NRF_SPIM_Type, EVENTS_ENDRX),   ///< End of RXD buffer reached.
    NRF_SPIM_EVENT_END     = offsetof(NRF_SPIM_Type, EVENTS_END),     ///< End of RXD buffer and TXD buffer reached.
    NRF_SPIM_EVENT_ENDTX   = offsetof(NRF_SPIM_Type, EVENTS_ENDTX),   ///< End of TXD buffer reached.
    NRF_SPIM_EVENT_STARTED = offsetof(NRF_SPIM_Type, EVENTS_STARTED)  ///< Transaction started.
    /*lint -restore*/
} nrf_spim_event_t;

/**
 * @brief SPIM shortcuts.
 */
typedef enum
{
    NRF_SPIM_SHORT_END_START_MASK = SPIM_SHORTS_END_START_Msk ///< Shortcut between END event and START task.
} nrf_spim_short_mask_t;

/**
 * @brief SPIM interrupts.
 */
typedef enum
{
    NRF_SPIM_INT_STOPPED_MASK = SPIM_INTENSET_STOPPED_Msk, ///< Interrupt on STOPPED event.
    NRF_SPIM_INT_ENDRX_MASK   = SPIM_INTENSET_ENDRX_Msk,   ///< Interrupt on ENDRX event.
    NRF_SPIM_INT_END_MASK     = SPIM_INTENSET_END_Msk,     ///< Interrupt on END event.
    NRF_SPIM_INT_ENDTX_MASK   = SPIM_INTENSET_ENDTX_Msk,   ///< Interrupt on ENDTX event.
    NRF_SPIM_INT_STARTED_MASK = SPIM_INTENSET_STARTED_Msk  ///< Interrupt on STARTED event.
} nrf_spim_int_mask_t;

/**
 * @brief SPI master data rates.
 */
typedef enum
{
    NRF_SPIM_FREQ_125K = SPIM_FREQUENCY_FREQUENCY_K125,   ///< 125 kbps.
    NRF_SPIM_FREQ_250K = SPIM_FREQUENCY_FREQUENCY_K250,   ///< 250 kbps.
    NRF_SPIM_FREQ_500K = SPIM_FREQUENCY_FREQUENCY_K500,   ///< 500 kbps.
    NRF_SPIM_FREQ_1M   = SPIM_FREQUENCY_FREQUENCY_M1,     ///< 1 Mbps.
    NRF_SPIM_FREQ_2M   = SPIM_FREQUENCY_FREQUENCY_M2,     ///< 2 Mbps.
    NRF_SPIM_FREQ_4M   = SPIM_FREQUENCY_FREQUENCY_M4,     ///< 4 Mbps.
    // [conversion to 'int' needed to prevent compilers from complaining
    //  that the provided value (0x80000000UL) is out of range of "int"]
    NRF_SPIM_FREQ_8M   = (int)SPIM_FREQUENCY_FREQUENCY_M8 ///< 8 Mbps.
} nrf_spim_frequency_t;

/**
 * @brief SPI modes.
 */
typedef enum
{
    NRF_SPIM_MODE_0, ///< SCK active high, sample on leading edge of clock.
    NRF_SPIM_MODE_1, ///< SCK active high, sample on trailing edge of clock.
    NRF_SPIM_MODE_2, ///< SCK active low, sample on leading edge of clock.
    NRF_SPIM_MODE_3  ///< SCK active low, sample on trailing edge of clock.
} nrf_spim_mode_t;

/**
 * @brief SPI bit orders.
 */
typedef enum
{
    NRF_SPIM_BIT_ORDER_MSB_FIRST = SPIM_CONFIG_ORDER_MsbFirst, ///< Most significant bit shifted out first.
    NRF_SPIM_BIT_ORDER_LSB_FIRST = SPIM_CONFIG_ORDER_LsbFirst  ///< Least significant bit shifted out first.
} nrf_spim_bit_order_t;


/**
 * @brief Function for activating a specific SPIM task.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] spim_task Task to activate.
 */
__STATIC_INLINE void nrf_spim_task_trigger(NRF_SPIM_Type * p_reg,
                                           nrf_spim_task_t spim_task);

/**
 * @brief Function for getting the address of a specific SPIM task register.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] spim_task Requested task.
 *
 * @return Address of the specified task register.
 */
__STATIC_INLINE uint32_t nrf_spim_task_address_get(NRF_SPIM_Type * p_reg,
                                                   nrf_spim_task_t spim_task);

/**
 * @brief Function for clearing a specific SPIM event.
 *
 * @param[in] p_reg      Pointer to the peripheral registers structure.
 * @param[in] spim_event Event to clear.
 */
__STATIC_INLINE void nrf_spim_event_clear(NRF_SPIM_Type * p_reg,
                                          nrf_spim_event_t spim_event);

/**
 * @brief Function for checking the state of a specific SPIM event.
 *
 * @param[in] p_reg      Pointer to the peripheral registers structure.
 * @param[in] spim_event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_spim_event_check(NRF_SPIM_Type * p_reg,
                                          nrf_spim_event_t spim_event);

/**
 * @brief Function for getting the address of a specific SPIM event register.
 *
 * @param[in] p_reg      Pointer to the peripheral registers structure.
 * @param[in] spim_event Requested event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t nrf_spim_event_address_get(NRF_SPIM_Type  * p_reg,
                                                    nrf_spim_event_t spim_event);
/**
 * @brief Function for enabling specified shortcuts.
 *
 * @param[in] p_reg            Pointer to the peripheral registers structure.
 * @param[in] spim_shorts_mask Shortcuts to enable.
 */
__STATIC_INLINE void nrf_spim_shorts_enable(NRF_SPIM_Type * p_reg,
                                            uint32_t spim_shorts_mask);

/**
 * @brief Function for disabling specified shortcuts.
 *
 * @param[in] p_reg            Pointer to the peripheral registers structure.
 * @param[in] spim_shorts_mask Shortcuts to disable.
 */
__STATIC_INLINE void nrf_spim_shorts_disable(NRF_SPIM_Type * p_reg,
                                             uint32_t spim_shorts_mask);

/**
 * @brief Function for getting shorts setting.
 *
 * @param[in] p_reg           Pointer to the peripheral registers structure.
 */
__STATIC_INLINE uint32_t nrf_spim_shorts_get(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg        Pointer to the peripheral registers structure.
 * @param[in] spim_int_mask Interrupts to enable.
 */
__STATIC_INLINE void nrf_spim_int_enable(NRF_SPIM_Type * p_reg,
                                         uint32_t spim_int_mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg        Pointer to the peripheral registers structure.
 * @param[in] spim_int_mask Interrupts to disable.
 */
__STATIC_INLINE void nrf_spim_int_disable(NRF_SPIM_Type * p_reg,
                                          uint32_t spim_int_mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param[in] spim_int Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_spim_int_enable_check(NRF_SPIM_Type * p_reg,
                                               nrf_spim_int_mask_t spim_int);

/**
 * @brief Function for enabling the SPIM peripheral.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_enable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for disabling the SPIM peripheral.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_disable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for configuring SPIM pins.
 *
 * If a given signal is not needed, pass the @ref NRF_SPIM_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg    Pointer to the peripheral registers structure.
 * @param[in] sck_pin   SCK pin number.
 * @param[in] mosi_pin  MOSI pin number.
 * @param[in] miso_pin  MISO pin number.
 */
__STATIC_INLINE void nrf_spim_pins_set(NRF_SPIM_Type * p_reg,
                                       uint32_t sck_pin,
                                       uint32_t mosi_pin,
                                       uint32_t miso_pin);

/**
 * @brief Function for setting the SPI master data rate.
 *
 * @param[in] p_reg    Pointer to the peripheral registers structure.
 * @param[in] frequency SPI frequency.
 */
__STATIC_INLINE void nrf_spim_frequency_set(NRF_SPIM_Type * p_reg,
                                            nrf_spim_frequency_t frequency);

/**
 * @brief Function for setting the transmit buffer.
 *
 * @param[in]  p_reg   Pointer to the peripheral registers structure.
 * @param[in]  p_buffer Pointer to the buffer with data to send.
 * @param[in]  length   Maximum number of data bytes to transmit.
 */
__STATIC_INLINE void nrf_spim_tx_buffer_set(NRF_SPIM_Type * p_reg,
                                            uint8_t const * p_buffer,
                                            uint8_t         length);

/**
 * @brief Function for setting the receive buffer.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param[in] p_buffer Pointer to the buffer for received data.
 * @param[in] length   Maximum number of data bytes to receive.
 */
__STATIC_INLINE void nrf_spim_rx_buffer_set(NRF_SPIM_Type * p_reg,
                                            uint8_t * p_buffer,
                                            uint8_t   length);

/**
 * @brief Function for setting the SPI configuration.
 *
 * @param[in] p_reg        Pointer to the peripheral registers structure.
 * @param[in] spi_mode      SPI mode.
 * @param[in] spi_bit_order SPI bit order.
 */
__STATIC_INLINE void nrf_spim_configure(NRF_SPIM_Type * p_reg,
                                        nrf_spim_mode_t spi_mode,
                                        nrf_spim_bit_order_t spi_bit_order);

/**
 * @brief Function for setting the over-read character.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] orc    Over-read character that is clocked out in case of
 *                   an over-read of the TXD buffer.
 */
__STATIC_INLINE void nrf_spim_orc_set(NRF_SPIM_Type * p_reg,
                                      uint8_t orc);

/**
 * @brief Function for enabling the TX list feature.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_tx_list_enable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for disabling the TX list feature.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_tx_list_disable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for enabling the RX list feature.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_rx_list_enable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for disabling the RX list feature.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_rx_list_disable(NRF_SPIM_Type * p_reg);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_spim_task_trigger(NRF_SPIM_Type * p_reg,
                                           nrf_spim_task_t spim_task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spim_task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_spim_task_address_get(NRF_SPIM_Type * p_reg,
                                                   nrf_spim_task_t spim_task)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)spim_task);
}

__STATIC_INLINE void nrf_spim_event_clear(NRF_SPIM_Type * p_reg,
                                          nrf_spim_event_t spim_event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spim_event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spim_event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_spim_event_check(NRF_SPIM_Type * p_reg,
                                          nrf_spim_event_t spim_event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spim_event);
}

__STATIC_INLINE uint32_t nrf_spim_event_address_get(NRF_SPIM_Type * p_reg,
                                                    nrf_spim_event_t spim_event)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)spim_event);
}

__STATIC_INLINE void nrf_spim_shorts_enable(NRF_SPIM_Type * p_reg,
                                            uint32_t spim_shorts_mask)
{
    p_reg->SHORTS |= spim_shorts_mask;
}

__STATIC_INLINE void nrf_spim_shorts_disable(NRF_SPIM_Type * p_reg,
                                             uint32_t spim_shorts_mask)
{
    p_reg->SHORTS &= ~(spim_shorts_mask);
}

__STATIC_INLINE uint32_t nrf_spim_shorts_get(NRF_SPIM_Type * p_reg)
{
    return p_reg->SHORTS;
}

__STATIC_INLINE void nrf_spim_int_enable(NRF_SPIM_Type * p_reg,
                                         uint32_t spim_int_mask)
{
    p_reg->INTENSET = spim_int_mask;
}

__STATIC_INLINE void nrf_spim_int_disable(NRF_SPIM_Type * p_reg,
                                          uint32_t spim_int_mask)
{
    p_reg->INTENCLR = spim_int_mask;
}

__STATIC_INLINE bool nrf_spim_int_enable_check(NRF_SPIM_Type * p_reg,
                                               nrf_spim_int_mask_t spim_int)
{
    return (bool)(p_reg->INTENSET & spim_int);
}

__STATIC_INLINE void nrf_spim_enable(NRF_SPIM_Type * p_reg)
{
    p_reg->ENABLE = (SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_spim_disable(NRF_SPIM_Type * p_reg)
{
    p_reg->ENABLE = (SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_spim_pins_set(NRF_SPIM_Type * p_reg,
                                       uint32_t sck_pin,
                                       uint32_t mosi_pin,
                                       uint32_t miso_pin)
{
    p_reg->PSEL.SCK  = sck_pin;
    p_reg->PSEL.MOSI = mosi_pin;
    p_reg->PSEL.MISO = miso_pin;
}

__STATIC_INLINE void nrf_spim_frequency_set(NRF_SPIM_Type * p_reg,
                                            nrf_spim_frequency_t frequency)
{
    p_reg->FREQUENCY = frequency;
}

__STATIC_INLINE void nrf_spim_tx_buffer_set(NRF_SPIM_Type * p_reg,
                                            uint8_t const * p_buffer,
                                            uint8_t         length)
{
    p_reg->TXD.PTR    = (uint32_t)p_buffer;
    p_reg->TXD.MAXCNT = length;
}

__STATIC_INLINE void nrf_spim_rx_buffer_set(NRF_SPIM_Type * p_reg,
                                            uint8_t * p_buffer,
                                            uint8_t   length)
{
    p_reg->RXD.PTR    = (uint32_t)p_buffer;
    p_reg->RXD.MAXCNT = length;
}

__STATIC_INLINE void nrf_spim_configure(NRF_SPIM_Type * p_reg,
                                        nrf_spim_mode_t spi_mode,
                                        nrf_spim_bit_order_t spi_bit_order)
{
    uint32_t config = (spi_bit_order == NRF_SPIM_BIT_ORDER_MSB_FIRST ?
        SPIM_CONFIG_ORDER_MsbFirst : SPIM_CONFIG_ORDER_LsbFirst);
    switch (spi_mode)
    {
    default:
    case NRF_SPIM_MODE_0:
        config |= (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Leading    << SPIM_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIM_MODE_1:
        config |= (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Trailing   << SPIM_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIM_MODE_2:
        config |= (SPIM_CONFIG_CPOL_ActiveLow  << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Leading    << SPIM_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIM_MODE_3:
        config |= (SPIM_CONFIG_CPOL_ActiveLow  << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Trailing   << SPIM_CONFIG_CPHA_Pos);
        break;
    }
    p_reg->CONFIG = config;
}

__STATIC_INLINE void nrf_spim_orc_set(NRF_SPIM_Type * p_reg,
                                      uint8_t orc)
{
    p_reg->ORC = orc;
}


__STATIC_INLINE void nrf_spim_tx_list_enable(NRF_SPIM_Type * p_reg)
{
    p_reg->TXD.LIST = 1;
}

__STATIC_INLINE void nrf_spim_tx_list_disable(NRF_SPIM_Type * p_reg)
{
    p_reg->TXD.LIST = 0;
}

__STATIC_INLINE void nrf_spim_rx_list_enable(NRF_SPIM_Type * p_reg)
{
    p_reg->RXD.LIST = 1;
}

__STATIC_INLINE void nrf_spim_rx_list_disable(NRF_SPIM_Type * p_reg)
{
    p_reg->RXD.LIST = 0;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION


#ifdef __cplusplus
}
#endif

#endif // NRF_SPIM_H__

/** @} */
