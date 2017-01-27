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

#ifndef NRF_TWI_H__
#define NRF_TWI_H__

/**
 * @defgroup nrf_twi_hal TWI HAL
 * @{
 * @ingroup nrf_twi
 *
 * @brief Hardware access layer for managing the TWI peripheral.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "nrf_peripherals.h"
#include "nrf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TWI tasks.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_TWI_TASK_STARTRX = offsetof(NRF_TWI_Type, TASKS_STARTRX), ///< Start TWI receive sequence.
    NRF_TWI_TASK_STARTTX = offsetof(NRF_TWI_Type, TASKS_STARTTX), ///< Start TWI transmit sequence.
    NRF_TWI_TASK_STOP    = offsetof(NRF_TWI_Type, TASKS_STOP),    ///< Stop TWI transaction.
    NRF_TWI_TASK_SUSPEND = offsetof(NRF_TWI_Type, TASKS_SUSPEND), ///< Suspend TWI transaction.
    NRF_TWI_TASK_RESUME  = offsetof(NRF_TWI_Type, TASKS_RESUME)   ///< Resume TWI transaction.
    /*lint -restore*/
} nrf_twi_task_t;

/**
 * @brief TWI events.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_TWI_EVENT_STOPPED   = offsetof(NRF_TWI_Type, EVENTS_STOPPED),  ///< TWI stopped.
    NRF_TWI_EVENT_RXDREADY  = offsetof(NRF_TWI_Type, EVENTS_RXDREADY), ///< TWI RXD byte received.
    NRF_TWI_EVENT_TXDSENT   = offsetof(NRF_TWI_Type, EVENTS_TXDSENT),  ///< TWI TXD byte sent.
    NRF_TWI_EVENT_ERROR     = offsetof(NRF_TWI_Type, EVENTS_ERROR),    ///< TWI error.
    NRF_TWI_EVENT_BB        = offsetof(NRF_TWI_Type, EVENTS_BB),       ///< TWI byte boundary, generated before each byte that is sent or received.
    NRF_TWI_EVENT_SUSPENDED = offsetof(NRF_TWI_Type, EVENTS_SUSPENDED) ///< TWI entered the suspended state.
    /*lint -restore*/
} nrf_twi_event_t;

/**
 * @brief TWI shortcuts.
 */
typedef enum
{
    NRF_TWI_SHORT_BB_SUSPEND_MASK = TWI_SHORTS_BB_SUSPEND_Msk, ///< Shortcut between BB event and SUSPEND task.
    NRF_TWI_SHORT_BB_STOP_MASK    = TWI_SHORTS_BB_STOP_Msk,    ///< Shortcut between BB event and STOP task.
} nrf_twi_short_mask_t;

/**
 * @brief TWI interrupts.
 */
typedef enum
{
    NRF_TWI_INT_STOPPED_MASK    = TWI_INTENSET_STOPPED_Msk,  ///< Interrupt on STOPPED event.
    NRF_TWI_INT_RXDREADY_MASK   = TWI_INTENSET_RXDREADY_Msk, ///< Interrupt on RXDREADY event.
    NRF_TWI_INT_TXDSENT_MASK    = TWI_INTENSET_TXDSENT_Msk,  ///< Interrupt on TXDSENT event.
    NRF_TWI_INT_ERROR_MASK      = TWI_INTENSET_ERROR_Msk,    ///< Interrupt on ERROR event.
    NRF_TWI_INT_BB_MASK         = TWI_INTENSET_BB_Msk,       ///< Interrupt on BB event.
    NRF_TWI_INT_SUSPENDED_MASK  = TWI_INTENSET_SUSPENDED_Msk ///< Interrupt on SUSPENDED event.
} nrf_twi_int_mask_t;

/**
 * @brief TWI error source.
 */
typedef enum
{
    NRF_TWI_ERROR_ADDRESS_NACK = TWI_ERRORSRC_ANACK_Msk,  ///< NACK received after sending the address.
    NRF_TWI_ERROR_DATA_NACK    = TWI_ERRORSRC_DNACK_Msk,  ///< NACK received after sending a data byte.
    NRF_TWI_ERROR_OVERRUN      = TWI_ERRORSRC_OVERRUN_Msk ///< Overrun error.
                                                          /**< A new byte was received before the previous byte was read
                                                           *   from the RXD register (previous data is lost). */
} nrf_twi_error_t;

/**
 * @brief TWI master clock frequency.
 */
typedef enum
{
    NRF_TWI_FREQ_100K = TWI_FREQUENCY_FREQUENCY_K100, ///< 100 kbps.
    NRF_TWI_FREQ_250K = TWI_FREQUENCY_FREQUENCY_K250, ///< 250 kbps.
    NRF_TWI_FREQ_400K = TWI_FREQUENCY_FREQUENCY_K400  ///< 400 kbps.
} nrf_twi_frequency_t;


/**
 * @brief Function for activating a specific TWI task.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] task  Task to activate.
 */
__STATIC_INLINE void nrf_twi_task_trigger(NRF_TWI_Type * p_reg,
                                          nrf_twi_task_t task);

/**
 * @brief Function for getting the address of a specific TWI task register.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] task  Requested task.
 *
 * @return Address of the specified task register.
 */
__STATIC_INLINE uint32_t * nrf_twi_task_address_get(NRF_TWI_Type * p_reg,
                                                    nrf_twi_task_t task);

/**
 * @brief Function for clearing a specific TWI event.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] event Event to clear.
 */
__STATIC_INLINE void nrf_twi_event_clear(NRF_TWI_Type * p_reg,
                                         nrf_twi_event_t event);

/**
 * @brief Function for checking the state of a specific event.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] event Event to check.
 *
 * @retval true If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_twi_event_check(NRF_TWI_Type  * p_reg,
                                         nrf_twi_event_t event);

/**
 * @brief Function for getting the address of a specific TWI event register.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] event Requested event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t * nrf_twi_event_address_get(NRF_TWI_Type  * p_reg,
                                                     nrf_twi_event_t event);

/**
 * @brief Function for enabling specified shortcuts.
 *
 * @param[in] p_reg       Pointer to the peripheral registers structure.
 * @param[in] shorts_mask Shortcuts to enable.
 */
__STATIC_INLINE void nrf_twi_shorts_enable(NRF_TWI_Type * p_reg,
                                           uint32_t shorts_mask);

/**
 * @brief Function for disabling specified shortcuts.
 *
 * @param[in] p_reg       Pointer to the peripheral registers structure.
 * @param[in] shorts_mask Shortcuts to disable.
 */
__STATIC_INLINE void nrf_twi_shorts_disable(NRF_TWI_Type * p_reg,
                                            uint32_t shorts_mask);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg    Pointer to the peripheral registers structure.
 * @param[in] int_mask Interrupts to enable.
 */
__STATIC_INLINE void nrf_twi_int_enable(NRF_TWI_Type * p_reg,
                                        uint32_t int_mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg    Pointer to the peripheral registers structure.
 * @param[in] int_mask Interrupts to disable.
 */
__STATIC_INLINE void nrf_twi_int_disable(NRF_TWI_Type * p_reg,
                                         uint32_t int_mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] p_reg    Pointer to the peripheral registers structure.
 * @param[in] int_mask Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_twi_int_enable_check(NRF_TWI_Type * p_reg,
                                              nrf_twi_int_mask_t int_mask);

/**
 * @brief Function for enabling the TWI peripheral.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_twi_enable(NRF_TWI_Type * p_reg);

/**
 * @brief Function for disabling the TWI peripheral.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_twi_disable(NRF_TWI_Type * p_reg);

/**
 * @brief Function for configuring TWI pins.
 *
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param[in] scl_pin SCL pin number.
 * @param[in] sda_pin SDA pin number.
 */
__STATIC_INLINE void nrf_twi_pins_set(NRF_TWI_Type * p_reg,
                                      uint32_t scl_pin,
                                      uint32_t sda_pin);

/**
 * @brief Function for setting the TWI master clock frequency.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] frequency TWI frequency.
 */
__STATIC_INLINE void nrf_twi_frequency_set(NRF_TWI_Type * p_reg,
                                           nrf_twi_frequency_t frequency);

/**
 * @brief Function for checking the TWI error source.
 *
 * The error flags are cleared after reading.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 *
 * @return Mask with error source flags.
 */
__STATIC_INLINE uint32_t nrf_twi_errorsrc_get_and_clear(NRF_TWI_Type * p_reg);

/**
 * @brief Function for setting the address to be used in TWI transfers.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param[in] address Address to be used in transfers.
 */
__STATIC_INLINE void nrf_twi_address_set(NRF_TWI_Type * p_reg, uint8_t address);

/**
 * @brief Function for reading data received by TWI.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 *
 * @return Received data.
 */
__STATIC_INLINE uint8_t nrf_twi_rxd_get(NRF_TWI_Type * p_reg);

/**
 * @brief Function for writing data to be transmitted by TWI.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] data  Data to be transmitted.
 */
__STATIC_INLINE void nrf_twi_txd_set(NRF_TWI_Type * p_reg, uint8_t data);

__STATIC_INLINE void nrf_twi_shorts_set(NRF_TWI_Type * p_reg,
                                        uint32_t shorts_mask);

/**
 * @}
 */


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_twi_task_trigger(NRF_TWI_Type * p_reg,
                                          nrf_twi_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE uint32_t * nrf_twi_task_address_get(NRF_TWI_Type * p_reg,
                                                    nrf_twi_task_t task)
{
    return (uint32_t *)((uint8_t *)p_reg + (uint32_t)task);
}

__STATIC_INLINE void nrf_twi_event_clear(NRF_TWI_Type  * p_reg,
                                         nrf_twi_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_twi_event_check(NRF_TWI_Type  * p_reg,
                                         nrf_twi_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

__STATIC_INLINE uint32_t * nrf_twi_event_address_get(NRF_TWI_Type  * p_reg,
                                                     nrf_twi_event_t event)
{
    return (uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

__STATIC_INLINE void nrf_twi_shorts_enable(NRF_TWI_Type * p_reg,
                                           uint32_t shorts_mask)
{
    p_reg->SHORTS |= shorts_mask;
}

__STATIC_INLINE void nrf_twi_shorts_disable(NRF_TWI_Type * p_reg,
                                            uint32_t shorts_mask)
{
    p_reg->SHORTS &= ~(shorts_mask);
}

__STATIC_INLINE void nrf_twi_int_enable(NRF_TWI_Type * p_reg,
                                        uint32_t int_mask)
{
    p_reg->INTENSET = int_mask;
}

__STATIC_INLINE void nrf_twi_int_disable(NRF_TWI_Type * p_reg,
                                         uint32_t int_mask)
{
    p_reg->INTENCLR = int_mask;
}

__STATIC_INLINE bool nrf_twi_int_enable_check(NRF_TWI_Type * p_reg,
                                              nrf_twi_int_mask_t int_mask)
{
    return (bool)(p_reg->INTENSET & int_mask);
}

__STATIC_INLINE void nrf_twi_enable(NRF_TWI_Type * p_reg)
{
    p_reg->ENABLE = (TWI_ENABLE_ENABLE_Enabled << TWI_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_twi_disable(NRF_TWI_Type * p_reg)
{
    p_reg->ENABLE = (TWI_ENABLE_ENABLE_Disabled << TWI_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_twi_pins_set(NRF_TWI_Type * p_reg,
                                      uint32_t scl_pin,
                                      uint32_t sda_pin)
{
#if defined(TWI_PSEL_SCL_CONNECT_Pos)
    p_reg->PSEL.SCL = scl_pin;
#else
    p_reg->PSELSCL = scl_pin;
#endif

#if defined(TWI_PSEL_SDA_CONNECT_Pos)
    p_reg->PSEL.SDA = sda_pin;
#else
    p_reg->PSELSDA = sda_pin;
#endif
}

__STATIC_INLINE void nrf_twi_frequency_set(NRF_TWI_Type * p_reg,
                                           nrf_twi_frequency_t frequency)
{
    p_reg->FREQUENCY = frequency;
}

__STATIC_INLINE uint32_t nrf_twi_errorsrc_get_and_clear(NRF_TWI_Type * p_reg)
{
    uint32_t error_source = p_reg->ERRORSRC;

    // [error flags are cleared by writing '1' on their position]
    p_reg->ERRORSRC = error_source;

    return error_source;
}

__STATIC_INLINE void nrf_twi_address_set(NRF_TWI_Type * p_reg, uint8_t address)
{
    p_reg->ADDRESS = address;
}

__STATIC_INLINE uint8_t nrf_twi_rxd_get(NRF_TWI_Type * p_reg)
{
    return (uint8_t)p_reg->RXD;
}

__STATIC_INLINE void nrf_twi_txd_set(NRF_TWI_Type * p_reg, uint8_t data)
{
    p_reg->TXD = data;
}

__STATIC_INLINE void nrf_twi_shorts_set(NRF_TWI_Type * p_reg,
                                        uint32_t shorts_mask)
{
    p_reg->SHORTS = shorts_mask;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION


#ifdef __cplusplus
}
#endif

#endif // NRF_TWI_H__
