/**
 * Copyright (c) 2016 - 2017, Nordic Semiconductor ASA
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
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nrfx.h>

#if NRFX_CHECK(NRFX_RNG_ENABLED)

#include <nrfx_rng.h>

#define NRFX_LOG_MODULE RNG
#include <nrfx_log.h>

/**
 * @brief Internal state of RNG driver.
 */
static nrfx_drv_state_t m_rng_state;

/**
 * @brief Pointer to handler calling from interrupt routine.
 */
static nrfx_rng_evt_handler_t m_rng_hndl;

nrfx_err_t nrfx_rng_init(nrfx_rng_config_t const * p_config, nrfx_rng_evt_handler_t handler)
{
    NRFX_ASSERT(p_config);
    if (m_rng_state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        return NRFX_ERROR_ALREADY_INITIALIZED;
    }

    if (handler == NULL)
    {
        return NRFX_ERROR_INVALID_PARAM;
    }

    m_rng_hndl = handler;

    if (p_config->error_correction)
    {
        nrf_rng_error_correction_enable();
    }
    nrf_rng_shorts_disable(NRF_RNG_SHORT_VALRDY_STOP_MASK);
    NRFX_IRQ_PRIORITY_SET(RNG_IRQn, p_config->interrupt_priority);
    NRFX_IRQ_ENABLE(RNG_IRQn);

    m_rng_state = NRFX_DRV_STATE_INITIALIZED;

    return NRFX_SUCCESS;
}

void nrfx_rng_start(void)
{
    NRFX_ASSERT(m_rng_state == NRFX_DRV_STATE_INITIALIZED);
    nrf_rng_event_clear(NRF_RNG_EVENT_VALRDY);
    nrf_rng_int_enable(NRF_RNG_INT_VALRDY_MASK);
    nrf_rng_task_trigger(NRF_RNG_TASK_START);
}

void nrfx_rng_stop(void)
{
    NRFX_ASSERT(m_rng_state == NRFX_DRV_STATE_INITIALIZED);
    nrf_rng_int_disable(NRF_RNG_INT_VALRDY_MASK);
    nrf_rng_task_trigger(NRF_RNG_TASK_STOP);
}

void nrfx_rng_uninit(void)
{
    NRFX_ASSERT(m_rng_state == NRFX_DRV_STATE_INITIALIZED);

    nrf_rng_int_disable(NRF_RNG_INT_VALRDY_MASK);
    nrf_rng_task_trigger(NRF_RNG_TASK_STOP);
    NRFX_IRQ_DISABLE(RNG_IRQn);

    m_rng_state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Uninitialized.");
}

void nrfx_rng_irq_handler(void)
{
    nrf_rng_event_clear(NRF_RNG_EVENT_VALRDY);

    uint8_t rng_value = nrf_rng_random_value_get();

    m_rng_hndl(rng_value);

    NRFX_LOG_DEBUG("Event: NRF_RNG_EVENT_VALRDY.");
}

#endif // NRFX_CHECK(NRFX_RNG_ENABLED)
