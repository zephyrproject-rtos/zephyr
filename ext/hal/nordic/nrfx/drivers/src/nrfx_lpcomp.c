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

#if NRFX_CHECK(NRFX_LPCOMP_ENABLED)

#include <nrfx_lpcomp.h>
#include "prs/nrfx_prs.h"

#define NRFX_LOG_MODULE LPCOMP
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                         \
    (event == NRF_LPCOMP_EVENT_READY ? "NRF_LPCOMP_EVENT_READY" : \
    (event == NRF_LPCOMP_EVENT_DOWN  ? "NRF_LPCOMP_EVENT_DOWN"  : \
    (event == NRF_LPCOMP_EVENT_UP    ? "NRF_LPCOMP_EVENT_UP"    : \
    (event == NRF_LPCOMP_EVENT_CROSS ? "NRF_LPCOMP_EVENT_CROSS" : \
                                       "UNKNOWN EVENT"))))


static nrfx_lpcomp_event_handler_t  m_lpcomp_event_handler = NULL;
static nrfx_drv_state_t             m_state = NRFX_DRV_STATE_UNINITIALIZED;

static void lpcomp_execute_handler(nrf_lpcomp_event_t event, uint32_t event_mask)
{
    if (nrf_lpcomp_event_check(event) && nrf_lpcomp_int_enable_check(event_mask))
    {
        nrf_lpcomp_event_clear(event);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(event));

        m_lpcomp_event_handler(event);
    }
}

void nrfx_lpcomp_irq_handler(void)
{
    lpcomp_execute_handler(NRF_LPCOMP_EVENT_READY, LPCOMP_INTENSET_READY_Msk);
    lpcomp_execute_handler(NRF_LPCOMP_EVENT_DOWN,  LPCOMP_INTENSET_DOWN_Msk);
    lpcomp_execute_handler(NRF_LPCOMP_EVENT_UP,    LPCOMP_INTENSET_UP_Msk);
    lpcomp_execute_handler(NRF_LPCOMP_EVENT_CROSS, LPCOMP_INTENSET_CROSS_Msk);
}

nrfx_err_t nrfx_lpcomp_init(nrfx_lpcomp_config_t const * p_config,
                            nrfx_lpcomp_event_handler_t  event_handler)
{
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(event_handler);
    nrfx_err_t err_code;

    if (m_state != NRFX_DRV_STATE_UNINITIALIZED)
    { // LPCOMP driver is already initialized
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    m_lpcomp_event_handler = event_handler;

#if NRFX_CHECK(NRFX_PRS_ENABLED)
    if (nrfx_prs_acquire(NRF_LPCOMP, nrfx_lpcomp_irq_handler) != NRFX_SUCCESS)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
#endif

    nrf_lpcomp_configure(&(p_config->hal));

    nrf_lpcomp_input_select(p_config->input);

    switch (p_config->hal.detection)
    {
        case NRF_LPCOMP_DETECT_UP:
            nrf_lpcomp_int_enable(LPCOMP_INTENSET_UP_Msk);
            break;

        case NRF_LPCOMP_DETECT_DOWN:
            nrf_lpcomp_int_enable(LPCOMP_INTENSET_DOWN_Msk);
            break;

        case NRF_LPCOMP_DETECT_CROSS:
            nrf_lpcomp_int_enable(LPCOMP_INTENSET_CROSS_Msk);
            break;

        default:
            break;
    }
    nrf_lpcomp_shorts_enable(NRF_LPCOMP_SHORT_READY_SAMPLE_MASK);

    NRFX_IRQ_PRIORITY_SET(LPCOMP_IRQn, p_config->interrupt_priority);
    NRFX_IRQ_ENABLE(LPCOMP_IRQn);

    m_state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_lpcomp_uninit(void)
{
    NRFX_ASSERT(m_state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_IRQ_DISABLE(LPCOMP_IRQn);
    nrfx_lpcomp_disable();
#if NRFX_CHECK(NRFX_PRS_ENABLED)
    nrfx_prs_release(NRF_LPCOMP);
#endif
    m_state = NRFX_DRV_STATE_UNINITIALIZED;
    m_lpcomp_event_handler = NULL;
    NRFX_LOG_INFO("Uninitialized.");
}

void nrfx_lpcomp_enable(void)
{
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_INITIALIZED);
    nrf_lpcomp_enable();
    nrf_lpcomp_task_trigger(NRF_LPCOMP_TASK_START);
    m_state = NRFX_DRV_STATE_POWERED_ON;
    NRFX_LOG_INFO("Enabled.");
}

void nrfx_lpcomp_disable(void)
{
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_POWERED_ON);
    nrf_lpcomp_disable();
    nrf_lpcomp_task_trigger(NRF_LPCOMP_TASK_STOP);
    m_state = NRFX_DRV_STATE_INITIALIZED;
    NRFX_LOG_INFO("Disabled.");
}

#endif // NRFX_CHECK(NRFX_LPCOMP_ENABLED)
