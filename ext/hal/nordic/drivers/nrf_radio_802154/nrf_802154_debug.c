/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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
 * @file
 *   This file implements debug helpers for the nRF 802.15.4 radio driver.
 *
 */

#include "nrf_802154_debug.h"

#include <stdint.h>

#include "hal/nrf_gpio.h"
#include "hal/nrf_gpiote.h"
#include "hal/nrf_ppi.h"
#include "nrf.h"

#if ENABLE_DEBUG_LOG
/// Buffer used to store debug log messages.
volatile uint32_t nrf_802154_debug_log_buffer[NRF_802154_DEBUG_LOG_BUFFER_LEN];
/// Index of the log buffer pointing to the element that should be filled with next log message.
volatile uint32_t nrf_802154_debug_log_ptr = 0;

#endif

#if ENABLE_DEBUG_GPIO
/**
 * @brief Initialize PPI to toggle GPIO pins on radio events.
 */
static void radio_event_gpio_toggle_init(void)
{
    nrf_gpio_cfg_output(PIN_DBG_RADIO_EVT_END);
    nrf_gpio_cfg_output(PIN_DBG_RADIO_EVT_DISABLED);
    nrf_gpio_cfg_output(PIN_DBG_RADIO_EVT_READY);
    nrf_gpio_cfg_output(PIN_DBG_RADIO_EVT_FRAMESTART);
    nrf_gpio_cfg_output(PIN_DBG_RADIO_EVT_EDEND);
    nrf_gpio_cfg_output(PIN_DBG_RADIO_EVT_PHYEND);

    nrf_gpiote_task_configure(0,
                              PIN_DBG_RADIO_EVT_END,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_configure(1,
                              PIN_DBG_RADIO_EVT_DISABLED,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_configure(2,
                              PIN_DBG_RADIO_EVT_READY,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_configure(3,
                              PIN_DBG_RADIO_EVT_FRAMESTART,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_configure(4,
                              PIN_DBG_RADIO_EVT_EDEND,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_configure(5,
                              PIN_DBG_RADIO_EVT_PHYEND,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);

    nrf_gpiote_task_enable(0);
    nrf_gpiote_task_enable(1);
    nrf_gpiote_task_enable(2);
    nrf_gpiote_task_enable(3);
    nrf_gpiote_task_enable(4);
    nrf_gpiote_task_enable(5);

    nrf_ppi_channel_endpoint_setup(NRF_PPI_CHANNEL0,
                                   (uint32_t)&NRF_RADIO->EVENTS_END,
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_0));
    nrf_ppi_channel_endpoint_setup(NRF_PPI_CHANNEL1,
                                   (uint32_t)&NRF_RADIO->EVENTS_DISABLED,
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_1));
    nrf_ppi_channel_endpoint_setup(NRF_PPI_CHANNEL2,
                                   (uint32_t)&NRF_RADIO->EVENTS_READY,
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_2));
    nrf_ppi_channel_endpoint_setup(NRF_PPI_CHANNEL3,
                                   (uint32_t)&NRF_RADIO->EVENTS_FRAMESTART,
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_3));
    nrf_ppi_channel_endpoint_setup(NRF_PPI_CHANNEL4,
                                   (uint32_t)&NRF_RADIO->EVENTS_EDEND,
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_4));
    nrf_ppi_channel_endpoint_setup(NRF_PPI_CHANNEL5,
                                   (uint32_t)&NRF_RADIO->EVENTS_PHYEND,
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_5));

    nrf_ppi_channel_enable(NRF_PPI_CHANNEL0);
    nrf_ppi_channel_enable(NRF_PPI_CHANNEL1);
    nrf_ppi_channel_enable(NRF_PPI_CHANNEL2);
    nrf_ppi_channel_enable(NRF_PPI_CHANNEL3);
    nrf_ppi_channel_enable(NRF_PPI_CHANNEL4);
    nrf_ppi_channel_enable(NRF_PPI_CHANNEL5);
}

/**
 * @brief Initialize GPIO to set it simulated arbiter events.
 */
static void raal_simulator_gpio_init(void)
{
#if RAAL_SIMULATOR
    nrf_gpio_cfg_output(PIN_DBG_TIMESLOT_ACTIVE);
    nrf_gpio_cfg_output(PIN_DBG_RAAL_CRITICAL_SECTION);
#endif
}

/**
 * @brief Initialize PPI to toggle GPIO pins on Softdevice events. Initialize GPIO to set it
 *        according to Softdevice arbiter client events.
 */
static void raal_softdevice_event_gpio_toggle_init(void)
{
#if RAAL_SOFTDEVICE
    nrf_gpio_cfg_output(PIN_DBG_TIMESLOT_ACTIVE);
    nrf_gpio_cfg_output(PIN_DBG_TIMESLOT_EXTEND_REQ);
    nrf_gpio_cfg_output(PIN_DBG_TIMESLOT_SESSION_IDLE);
    nrf_gpio_cfg_output(PIN_DBG_TIMESLOT_RADIO_IRQ);
    nrf_gpio_cfg_output(PIN_DBG_TIMESLOT_FAILED);
    nrf_gpio_cfg_output(PIN_DBG_TIMESLOT_BLOCKED);
    nrf_gpio_cfg_output(PIN_DBG_RTC0_EVT_REM);

    nrf_gpiote_task_configure(5,
                              PIN_DBG_RTC0_EVT_REM,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);

    nrf_gpiote_task_enable(5);

    nrf_ppi_channel_endpoint_setup(NRF_PPI_CHANNEL5,
                                   (uint32_t)&NRF_RTC0->EVENTS_COMPARE[1],
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_5));

    nrf_ppi_channel_enable(NRF_PPI_CHANNEL5);
#endif // RAAL_SOFTDEVICE
}

#endif // ENABLE_DEBUG_GPIO

void nrf_802154_debug_init(void)
{
#if ENABLE_DEBUG_GPIO
    radio_event_gpio_toggle_init();
    raal_simulator_gpio_init();
    raal_softdevice_event_gpio_toggle_init();
#endif // ENABLE_DEBUG_GPIO
}

#if ENABLE_DEBUG_ASSERT
void __assert_func(const char * file, int line, const char * func, const char * cond)
{
    (void)file;
    (void)line;
    (void)func;
    (void)cond;

    __disable_irq();

    while (1)
        ;
}

#endif // ENABLE_DEBUG_ASSERT
