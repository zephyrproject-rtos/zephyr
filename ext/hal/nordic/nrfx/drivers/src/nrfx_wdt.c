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

#include <nrfx.h>

#if NRFX_CHECK(NRFX_WDT_ENABLED)
#include <nrfx_wdt.h>

#define NRFX_LOG_MODULE WDT
#include <nrfx_log.h>


/**@brief WDT event handler. */
static nrfx_wdt_event_handler_t m_wdt_event_handler;

/**@brief WDT state. */
static nrfx_drv_state_t m_state;

/**@brief WDT alloc table. */
static uint32_t m_alloc_index;

/**@brief WDT interrupt handler. */
void nrfx_wdt_irq_handler(void)
{
    if (nrf_wdt_int_enable_check(NRF_WDT_INT_TIMEOUT_MASK) == true)
    {
        m_wdt_event_handler();
        nrf_wdt_event_clear(NRF_WDT_EVENT_TIMEOUT);
    }
}


nrfx_err_t nrfx_wdt_init(nrfx_wdt_config_t const * p_config,
                         nrfx_wdt_event_handler_t  wdt_event_handler)
{
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(wdt_event_handler != NULL);
    nrfx_err_t err_code;
    m_wdt_event_handler = wdt_event_handler;

    if (m_state == NRFX_DRV_STATE_UNINITIALIZED)
    {
        m_state = NRFX_DRV_STATE_INITIALIZED;
    }
    else
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    nrf_wdt_behaviour_set(p_config->behaviour);

    nrf_wdt_reload_value_set((p_config->reload_value * 32768) / 1000);

    NRFX_IRQ_PRIORITY_SET(WDT_IRQn, p_config->interrupt_priority);
    NRFX_IRQ_ENABLE(WDT_IRQn);

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


void nrfx_wdt_enable(void)
{
    NRFX_ASSERT(m_alloc_index != 0);
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_INITIALIZED);
    nrf_wdt_int_enable(NRF_WDT_INT_TIMEOUT_MASK);
    nrf_wdt_task_trigger(NRF_WDT_TASK_START);
    m_state = NRFX_DRV_STATE_POWERED_ON;
    NRFX_LOG_INFO("Enabled.");
}


void nrfx_wdt_feed(void)
{
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_POWERED_ON);
    for (uint32_t i = 0; i < m_alloc_index; i++)
    {
        nrf_wdt_reload_request_set((nrf_wdt_rr_register_t)(NRF_WDT_RR0 + i));
    }
}

nrfx_err_t nrfx_wdt_channel_alloc(nrfx_wdt_channel_id * p_channel_id)
{
    nrfx_err_t result;
    NRFX_ASSERT(p_channel_id);
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_INITIALIZED);

    NRFX_CRITICAL_SECTION_ENTER();
    if (m_alloc_index < NRF_WDT_CHANNEL_NUMBER)
    {
        *p_channel_id = (nrfx_wdt_channel_id)(NRF_WDT_RR0 + m_alloc_index);
        m_alloc_index++;
        nrf_wdt_reload_request_enable(*p_channel_id);
        result = NRFX_SUCCESS;
    }
    else
    {
        result = NRFX_ERROR_NO_MEM;
    }
    NRFX_CRITICAL_SECTION_EXIT();
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(result));
    return result;
}

void nrfx_wdt_channel_feed(nrfx_wdt_channel_id channel_id)
{
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_POWERED_ON);
    nrf_wdt_reload_request_set(channel_id);
}

#endif // NRFX_CHECK(NRFX_WDT_ENABLED)
