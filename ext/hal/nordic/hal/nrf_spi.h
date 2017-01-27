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
 * @defgroup nrf_spi_hal SPI HAL
 * @{
 * @ingroup nrf_spi
 *
 * @brief Hardware access layer for accessing the SPI peripheral.
 */

#ifndef NRF_SPI_H__
#define NRF_SPI_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief This value can be used as a parameter for the @ref nrf_spi_pins_set
 *        function to specify that a given SPI signal (SCK, MOSI, or MISO)
 *        shall not be connected to a physical pin.
 */
#define NRF_SPI_PIN_NOT_CONNECTED  0xFFFFFFFF


/**
 * @brief SPI events.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_SPI_EVENT_READY = offsetof(NRF_SPI_Type, EVENTS_READY) ///< TXD byte sent and RXD byte received.
    /*lint -restore*/
} nrf_spi_event_t;

/**
 * @brief SPI interrupts.
 */
typedef enum
{
    NRF_SPI_INT_READY_MASK = SPI_INTENSET_READY_Msk ///< Interrupt on READY event.
} nrf_spi_int_mask_t;

/**
 * @brief SPI data rates.
 */
typedef enum
{
    NRF_SPI_FREQ_125K = SPI_FREQUENCY_FREQUENCY_K125,   ///< 125 kbps.
    NRF_SPI_FREQ_250K = SPI_FREQUENCY_FREQUENCY_K250,   ///< 250 kbps.
    NRF_SPI_FREQ_500K = SPI_FREQUENCY_FREQUENCY_K500,   ///< 500 kbps.
    NRF_SPI_FREQ_1M   = SPI_FREQUENCY_FREQUENCY_M1,     ///< 1 Mbps.
    NRF_SPI_FREQ_2M   = SPI_FREQUENCY_FREQUENCY_M2,     ///< 2 Mbps.
    NRF_SPI_FREQ_4M   = SPI_FREQUENCY_FREQUENCY_M4,     ///< 4 Mbps.
    // [conversion to 'int' needed to prevent compilers from complaining
    //  that the provided value (0x80000000UL) is out of range of "int"]
    NRF_SPI_FREQ_8M   = (int)SPI_FREQUENCY_FREQUENCY_M8 ///< 8 Mbps.
} nrf_spi_frequency_t;

/**
 * @brief SPI modes.
 */
typedef enum
{
    NRF_SPI_MODE_0, ///< SCK active high, sample on leading edge of clock.
    NRF_SPI_MODE_1, ///< SCK active high, sample on trailing edge of clock.
    NRF_SPI_MODE_2, ///< SCK active low, sample on leading edge of clock.
    NRF_SPI_MODE_3  ///< SCK active low, sample on trailing edge of clock.
} nrf_spi_mode_t;

/**
 * @brief SPI bit orders.
 */
typedef enum
{
    NRF_SPI_BIT_ORDER_MSB_FIRST = SPI_CONFIG_ORDER_MsbFirst, ///< Most significant bit shifted out first.
    NRF_SPI_BIT_ORDER_LSB_FIRST = SPI_CONFIG_ORDER_LsbFirst  ///< Least significant bit shifted out first.
} nrf_spi_bit_order_t;


/**
 * @brief Function for clearing a specific SPI event.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] spi_event Event to clear.
 */
__STATIC_INLINE void nrf_spi_event_clear(NRF_SPI_Type * p_reg,
                                         nrf_spi_event_t spi_event);

/**
 * @brief Function for checking the state of a specific SPI event.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] spi_event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_spi_event_check(NRF_SPI_Type * p_reg,
                                         nrf_spi_event_t spi_event);

/**
 * @brief Function for getting the address of a specific SPI event register.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] spi_event Requested event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t * nrf_spi_event_address_get(NRF_SPI_Type  * p_reg,
                                                     nrf_spi_event_t spi_event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg         Pointer to the peripheral registers structure.
 * @param[in] spi_int_mask  Interrupts to enable.
 */
__STATIC_INLINE void nrf_spi_int_enable(NRF_SPI_Type * p_reg,
                                        uint32_t spi_int_mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg         Pointer to the peripheral registers structure.
 * @param[in] spi_int_mask  Interrupts to disable.
 */
__STATIC_INLINE void nrf_spi_int_disable(NRF_SPI_Type * p_reg,
                                         uint32_t spi_int_mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param[in] spi_int Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_spi_int_enable_check(NRF_SPI_Type * p_reg,
                                              nrf_spi_int_mask_t spi_int);

/**
 * @brief Function for enabling the SPI peripheral.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spi_enable(NRF_SPI_Type * p_reg);

/**
 * @brief Function for disabling the SPI peripheral.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spi_disable(NRF_SPI_Type * p_reg);

/**
 * @brief Function for configuring SPI pins.
 *
 * If a given signal is not needed, pass the @ref NRF_SPI_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] sck_pin   SCK pin number.
 * @param[in] mosi_pin  MOSI pin number.
 * @param[in] miso_pin  MISO pin number.
 */
__STATIC_INLINE void nrf_spi_pins_set(NRF_SPI_Type * p_reg,
                                      uint32_t sck_pin,
                                      uint32_t mosi_pin,
                                      uint32_t miso_pin);

/**
 * @brief Function for writing data to the SPI transmitter register.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] data  TX data to send.
 */
__STATIC_INLINE void nrf_spi_txd_set(NRF_SPI_Type * p_reg, uint8_t data);

/**
 * @brief Function for reading data from the SPI receiver register.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 *
 * @return RX data received.
 */
__STATIC_INLINE uint8_t nrf_spi_rxd_get(NRF_SPI_Type * p_reg);

/**
 * @brief Function for setting the SPI master data rate.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] frequency SPI frequency.
 */
__STATIC_INLINE void nrf_spi_frequency_set(NRF_SPI_Type * p_reg,
                                           nrf_spi_frequency_t frequency);

/**
 * @brief Function for setting the SPI configuration.
 *
 * @param[in] p_reg         Pointer to the peripheral registers structure.
 * @param[in] spi_mode      SPI mode.
 * @param[in] spi_bit_order SPI bit order.
 */
__STATIC_INLINE void nrf_spi_configure(NRF_SPI_Type * p_reg,
                                       nrf_spi_mode_t spi_mode,
                                       nrf_spi_bit_order_t spi_bit_order);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_spi_event_clear(NRF_SPI_Type * p_reg,
                                         nrf_spi_event_t spi_event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spi_event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spi_event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_spi_event_check(NRF_SPI_Type * p_reg,
                                         nrf_spi_event_t spi_event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spi_event);
}

__STATIC_INLINE uint32_t * nrf_spi_event_address_get(NRF_SPI_Type * p_reg,
                                                     nrf_spi_event_t spi_event)
{
    return (uint32_t *)((uint8_t *)p_reg + (uint32_t)spi_event);
}

__STATIC_INLINE void nrf_spi_int_enable(NRF_SPI_Type * p_reg,
                                        uint32_t spi_int_mask)
{
    p_reg->INTENSET = spi_int_mask;
}

__STATIC_INLINE void nrf_spi_int_disable(NRF_SPI_Type * p_reg,
                                         uint32_t spi_int_mask)
{
    p_reg->INTENCLR = spi_int_mask;
}

__STATIC_INLINE bool nrf_spi_int_enable_check(NRF_SPI_Type * p_reg,
                                              nrf_spi_int_mask_t spi_int)
{
    return (bool)(p_reg->INTENSET & spi_int);
}

__STATIC_INLINE void nrf_spi_enable(NRF_SPI_Type * p_reg)
{
    p_reg->ENABLE = (SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_spi_disable(NRF_SPI_Type * p_reg)
{
    p_reg->ENABLE = (SPI_ENABLE_ENABLE_Disabled << SPI_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_spi_pins_set(NRF_SPI_Type * p_reg,
                                      uint32_t sck_pin,
                                      uint32_t mosi_pin,
                                      uint32_t miso_pin)
{
    p_reg->PSELSCK  = sck_pin;
    p_reg->PSELMOSI = mosi_pin;
    p_reg->PSELMISO = miso_pin;
}

__STATIC_INLINE void nrf_spi_txd_set(NRF_SPI_Type * p_reg, uint8_t data)
{
    p_reg->TXD = data;
}

__STATIC_INLINE uint8_t nrf_spi_rxd_get(NRF_SPI_Type * p_reg)
{
    return p_reg->RXD;
}

__STATIC_INLINE void nrf_spi_frequency_set(NRF_SPI_Type * p_reg,
                                           nrf_spi_frequency_t frequency)
{
    p_reg->FREQUENCY = frequency;
}

__STATIC_INLINE void nrf_spi_configure(NRF_SPI_Type * p_reg,
                                       nrf_spi_mode_t spi_mode,
                                       nrf_spi_bit_order_t spi_bit_order)
{
    uint32_t config = (spi_bit_order == NRF_SPI_BIT_ORDER_MSB_FIRST ?
        SPI_CONFIG_ORDER_MsbFirst : SPI_CONFIG_ORDER_LsbFirst);
    switch (spi_mode)
    {
    default:
    case NRF_SPI_MODE_0:
        config |= (SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos) |
                  (SPI_CONFIG_CPHA_Leading    << SPI_CONFIG_CPHA_Pos);
        break;

    case NRF_SPI_MODE_1:
        config |= (SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos) |
                  (SPI_CONFIG_CPHA_Trailing   << SPI_CONFIG_CPHA_Pos);
        break;

    case NRF_SPI_MODE_2:
        config |= (SPI_CONFIG_CPOL_ActiveLow  << SPI_CONFIG_CPOL_Pos) |
                  (SPI_CONFIG_CPHA_Leading    << SPI_CONFIG_CPHA_Pos);
        break;

    case NRF_SPI_MODE_3:
        config |= (SPI_CONFIG_CPOL_ActiveLow  << SPI_CONFIG_CPOL_Pos) |
                  (SPI_CONFIG_CPHA_Trailing   << SPI_CONFIG_CPHA_Pos);
        break;
    }
    p_reg->CONFIG = config;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION


#ifdef __cplusplus
}
#endif

#endif // NRF_SPI_H__

/** @} */
