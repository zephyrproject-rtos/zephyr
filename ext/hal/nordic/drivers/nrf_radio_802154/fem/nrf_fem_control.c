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
 *   This file implements common function for Front End Module control of the nRF 802.15.4 radio driver.
 *
 */

#include "nrf_fem_control_api.h"

#include <assert.h>

#include "compiler_abstraction.h"
#include "nrf_fem_control_config.h"
#include "nrf_802154_config.h"
#include "nrf.h"
#include "hal/nrf_gpio.h"
#include "hal/nrf_gpiote.h"
#include "hal/nrf_ppi.h"
#include "hal/nrf_radio.h"
#include "hal/nrf_timer.h"

#define NRF_FEM_TIMER_INSTANCE NRF_802154_TIMER_INSTANCE

#if ENABLE_FEM

static nrf_fem_control_cfg_t m_nrf_fem_control_cfg; /**< FEM controller configuration. */

/** Check whether pin is valid and enabled. */
static bool pin_is_enabled(nrf_fem_control_pin_t pin)
{
    switch (pin)
    {
        case NRF_FEM_CONTROL_LNA_PIN:
            return m_nrf_fem_control_cfg.lna_cfg.enable;

        case NRF_FEM_CONTROL_PA_PIN:
            return m_nrf_fem_control_cfg.pa_cfg.enable;

        case NRF_FEM_CONTROL_ANY_PIN:
            return m_nrf_fem_control_cfg.lna_cfg.enable || m_nrf_fem_control_cfg.pa_cfg.enable;

        default:
            assert(false);
            return false;
    }
}

/**
 * @section GPIO control.
 */

/** Initialize GPIO according to configuration provided. */
static void gpio_init(void)
{
    if (m_nrf_fem_control_cfg.pa_cfg.enable)
    {
        nrf_gpio_cfg_output(m_nrf_fem_control_cfg.pa_cfg.gpio_pin);
        nrf_gpio_pin_write(m_nrf_fem_control_cfg.pa_cfg.gpio_pin,
                           !m_nrf_fem_control_cfg.pa_cfg.active_high);
    }

    if (m_nrf_fem_control_cfg.lna_cfg.enable)
    {
        nrf_gpio_cfg_output(m_nrf_fem_control_cfg.lna_cfg.gpio_pin);
        nrf_gpio_pin_write(m_nrf_fem_control_cfg.lna_cfg.gpio_pin,
                           !m_nrf_fem_control_cfg.lna_cfg.active_high);
    }
}

/** Configure GPIOTE module. */
static void gpiote_configure(void)
{
    nrf_gpiote_task_configure(m_nrf_fem_control_cfg.lna_gpiote_ch_id,
                              m_nrf_fem_control_cfg.lna_cfg.gpio_pin,
                              (nrf_gpiote_polarity_t)GPIOTE_CONFIG_POLARITY_None,
                              (nrf_gpiote_outinit_t) !m_nrf_fem_control_cfg.lna_cfg.active_high);

    nrf_gpiote_task_enable(m_nrf_fem_control_cfg.lna_gpiote_ch_id);

    nrf_gpiote_task_configure(m_nrf_fem_control_cfg.pa_gpiote_ch_id,
                              m_nrf_fem_control_cfg.pa_cfg.gpio_pin,
                              (nrf_gpiote_polarity_t)GPIOTE_CONFIG_POLARITY_None,
                              (nrf_gpiote_outinit_t) !m_nrf_fem_control_cfg.pa_cfg.active_high);

    nrf_gpiote_task_enable(m_nrf_fem_control_cfg.pa_gpiote_ch_id);
}

/**
 * @section PPI control.
 */

/** Initialize PPI according to configuration provided. */
static void ppi_init(void)
{
    /* RADIO DISABLED --> clr LNA & clr PA PPI */
    nrf_ppi_channel_and_fork_endpoint_setup(
        (nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_clr,
        (uint32_t)(&NRF_RADIO->EVENTS_DISABLED),
        (m_nrf_fem_control_cfg.lna_cfg.active_high ?
         (uint32_t)(&NRF_GPIOTE->TASKS_CLR[m_nrf_fem_control_cfg.lna_gpiote_ch_id]) :
         (uint32_t)(&NRF_GPIOTE->TASKS_SET[m_nrf_fem_control_cfg.lna_gpiote_ch_id])),
        (m_nrf_fem_control_cfg.pa_cfg.active_high ?
         (uint32_t)(&NRF_GPIOTE->TASKS_CLR[m_nrf_fem_control_cfg.pa_gpiote_ch_id]) :
         (uint32_t)(&NRF_GPIOTE->TASKS_SET[m_nrf_fem_control_cfg.pa_gpiote_ch_id])));
}

/** Setup PPI to set LNA pin on a timer event. */
static void ppi_lna_enable_setup(nrf_timer_cc_channel_t timer_cc_channel)
{
    /* TIMER0->COMPARE --> set LNA PPI */
    nrf_ppi_channel_endpoint_setup(
        (nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_set,
        (uint32_t)(&NRF_FEM_TIMER_INSTANCE->EVENTS_COMPARE[timer_cc_channel]),
        (m_nrf_fem_control_cfg.lna_cfg.active_high ?
         (uint32_t)(&NRF_GPIOTE->TASKS_SET[m_nrf_fem_control_cfg.lna_gpiote_ch_id]) :
         (uint32_t)(&NRF_GPIOTE->TASKS_CLR[m_nrf_fem_control_cfg.lna_gpiote_ch_id])));
}

/** Setup PPI to set PA pin on a timer event. */
static void ppi_pa_enable_setup(nrf_timer_cc_channel_t timer_cc_channel)
{
    /* TIMER2->COMPARE --> set PA PPI */
    nrf_ppi_channel_endpoint_setup(
        (nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_set,
        (uint32_t)(&NRF_FEM_TIMER_INSTANCE->EVENTS_COMPARE[timer_cc_channel]),
        (m_nrf_fem_control_cfg.pa_cfg.active_high ?
         (uint32_t)(&NRF_GPIOTE->TASKS_SET[m_nrf_fem_control_cfg.pa_gpiote_ch_id]) :
         (uint32_t)(&NRF_GPIOTE->TASKS_CLR[m_nrf_fem_control_cfg.pa_gpiote_ch_id])));
}

/**
 * @section FEM API functions.
 */

void nrf_fem_control_cfg_set(const nrf_fem_control_cfg_t * p_cfg)
{
    m_nrf_fem_control_cfg = *p_cfg;

    if (m_nrf_fem_control_cfg.pa_cfg.enable || m_nrf_fem_control_cfg.lna_cfg.enable)
    {
        gpio_init();
        gpiote_configure();
        ppi_init();

        nrf_ppi_channel_enable((nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_clr);
    }
}

void nrf_fem_control_cfg_get(nrf_fem_control_cfg_t * p_cfg)
{
    *p_cfg = m_nrf_fem_control_cfg;
}

void nrf_fem_control_activate(void)
{
    if (m_nrf_fem_control_cfg.pa_cfg.enable || m_nrf_fem_control_cfg.lna_cfg.enable)
    {
        nrf_ppi_channel_enable((nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_clr);
    }
}

void nrf_fem_control_deactivate(void)
{
    if (m_nrf_fem_control_cfg.pa_cfg.enable || m_nrf_fem_control_cfg.lna_cfg.enable)
    {
        nrf_ppi_channel_disable((nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_clr);
        nrf_ppi_channel_disable((nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_set);
    }
}

void nrf_fem_control_ppi_enable(nrf_fem_control_pin_t pin, nrf_timer_cc_channel_t timer_cc_channel)
{
    if (pin_is_enabled(pin))
    {
        switch (pin)
        {
            case NRF_FEM_CONTROL_LNA_PIN:
                ppi_lna_enable_setup(timer_cc_channel);
                nrf_ppi_channel_enable((nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_set);
                break;

            case NRF_FEM_CONTROL_PA_PIN:
                ppi_pa_enable_setup(timer_cc_channel);
                nrf_ppi_channel_enable((nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_set);
                break;

            default:
                assert(false);
                break;
        }
    }
}

void nrf_fem_control_ppi_disable(nrf_fem_control_pin_t pin)
{
    if (pin_is_enabled(pin))
    {
        nrf_ppi_channel_disable((nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_set);
    }
}

uint32_t nrf_fem_control_delay_get(nrf_fem_control_pin_t pin)
{
    uint32_t target_time = 1;

    if (pin_is_enabled(pin))
    {
        switch (pin)
        {
            case NRF_FEM_CONTROL_LNA_PIN:
                target_time = NRF_FEM_RADIO_RX_STARTUP_LATENCY_US - NRF_FEM_LNA_TIME_IN_ADVANCE;
                break;

            case NRF_FEM_CONTROL_PA_PIN:
                target_time = NRF_FEM_RADIO_TX_STARTUP_LATENCY_US - NRF_FEM_PA_TIME_IN_ADVANCE;
                break;

            default:
                assert(false);
                break;
        }
    }

    return target_time;
}

void nrf_fem_control_pin_clear(void)
{
    if (pin_is_enabled(NRF_FEM_CONTROL_PA_PIN))
    {
        nrf_gpiote_task_force(m_nrf_fem_control_cfg.pa_gpiote_ch_id,
                              (nrf_gpiote_outinit_t) !m_nrf_fem_control_cfg.pa_cfg.active_high);
    }

    if (pin_is_enabled(NRF_FEM_CONTROL_LNA_PIN))
    {
        nrf_gpiote_task_force(m_nrf_fem_control_cfg.lna_gpiote_ch_id,
                              (nrf_gpiote_outinit_t) !m_nrf_fem_control_cfg.lna_cfg.active_high);
    }
}

void nrf_fem_control_timer_set(nrf_fem_control_pin_t  pin,
                               nrf_timer_cc_channel_t timer_cc_channel,
                               nrf_timer_short_mask_t short_mask)
{
    if (pin_is_enabled(pin))
    {
        uint32_t target_time = nrf_fem_control_delay_get(pin);

        nrf_timer_shorts_enable(NRF_FEM_TIMER_INSTANCE, short_mask);
        nrf_timer_cc_write(NRF_FEM_TIMER_INSTANCE, timer_cc_channel, target_time);
    }
}

void nrf_fem_control_timer_reset(nrf_fem_control_pin_t pin, nrf_timer_short_mask_t short_mask)
{
    if (pin_is_enabled(pin))
    {
        // Anomaly 78: use SHUTDOWN instead of STOP and CLEAR.
        nrf_timer_task_trigger(NRF_FEM_TIMER_INSTANCE, NRF_TIMER_TASK_SHUTDOWN);
        nrf_timer_shorts_disable(NRF_FEM_TIMER_INSTANCE, short_mask);
    }
}

void nrf_fem_control_ppi_fork_setup(nrf_fem_control_pin_t pin,
                                    nrf_ppi_channel_t     ppi_channel,
                                    uint32_t              task_addr)
{
    if (pin_is_enabled(pin))
    {
        nrf_ppi_fork_endpoint_setup(ppi_channel, task_addr);
    }
}

void nrf_fem_control_ppi_task_setup(nrf_fem_control_pin_t pin,
                                    nrf_ppi_channel_t     ppi_channel,
                                    uint32_t              event_addr,
                                    uint32_t              task_addr)
{
    if (pin_is_enabled(pin))
    {
        nrf_ppi_channel_endpoint_setup(ppi_channel, event_addr, task_addr);
        nrf_ppi_channel_enable(ppi_channel);
    }
}

void nrf_fem_control_ppi_fork_clear(nrf_fem_control_pin_t pin, nrf_ppi_channel_t ppi_channel)
{
    if (pin_is_enabled(pin))
    {
        nrf_ppi_fork_endpoint_setup(ppi_channel, 0);
    }
}

void nrf_fem_control_ppi_pin_task_setup(nrf_ppi_channel_t ppi_channel,
                                        uint32_t          event_addr,
                                        bool              lna_pin_set,
                                        bool              pa_pin_set)
{
    uint32_t lna_task_addr = 0;
    uint32_t pa_task_addr  = 0;

    if (m_nrf_fem_control_cfg.lna_cfg.enable)
    {
        if ((lna_pin_set && m_nrf_fem_control_cfg.lna_cfg.active_high) ||
            (!lna_pin_set && !m_nrf_fem_control_cfg.lna_cfg.active_high))
        {
            lna_task_addr =
                (uint32_t)(&NRF_GPIOTE->TASKS_SET[m_nrf_fem_control_cfg.lna_gpiote_ch_id]);
        }
        else
        {
            lna_task_addr =
                (uint32_t)(&NRF_GPIOTE->TASKS_CLR[m_nrf_fem_control_cfg.lna_gpiote_ch_id]);
        }
    }

    if (m_nrf_fem_control_cfg.pa_cfg.enable)
    {
        if ((pa_pin_set && m_nrf_fem_control_cfg.pa_cfg.active_high) ||
            (!pa_pin_set && !m_nrf_fem_control_cfg.pa_cfg.active_high))
        {
            pa_task_addr =
                (uint32_t)(&NRF_GPIOTE->TASKS_SET[m_nrf_fem_control_cfg.pa_gpiote_ch_id]);
        }
        else
        {
            pa_task_addr =
                (uint32_t)(&NRF_GPIOTE->TASKS_CLR[m_nrf_fem_control_cfg.pa_gpiote_ch_id]);
        }
    }

    if (lna_task_addr != 0 || pa_task_addr != 0)
    {
        nrf_ppi_channel_and_fork_endpoint_setup(ppi_channel, event_addr, lna_task_addr,
                                                pa_task_addr);
        nrf_ppi_channel_enable(ppi_channel);
    }
}

#endif // ENABLE_FEM
