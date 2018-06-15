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

#if NRFX_CHECK(NRFX_COMP_ENABLED)

#include <nrfx_comp.h>
#include "prs/nrfx_prs.h"

#define NRFX_LOG_MODULE COMP
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                     \
    (event == NRF_COMP_EVENT_READY ? "NRF_COMP_EVENT_READY" : \
    (event == NRF_COMP_EVENT_DOWN  ? "NRF_COMP_EVENT_DOWN"  : \
    (event == NRF_COMP_EVENT_UP    ? "NRF_COMP_EVENT_UP"    : \
    (event == NRF_COMP_EVENT_CROSS ? "NRF_COMP_EVENT_CROSS" : \
                                     "UNKNOWN ERROR"))))


static nrfx_comp_event_handler_t    m_comp_event_handler = NULL;
static nrfx_drv_state_t             m_state = NRFX_DRV_STATE_UNINITIALIZED;

static void comp_execute_handler(nrf_comp_event_t event, uint32_t event_mask)
{
    if (nrf_comp_event_check(event) && nrf_comp_int_enable_check(event_mask))
    {
        nrf_comp_event_clear(event);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(event));

        m_comp_event_handler(event);
    }
}

void nrfx_comp_irq_handler(void)
{
    comp_execute_handler(NRF_COMP_EVENT_READY, COMP_INTENSET_READY_Msk);
    comp_execute_handler(NRF_COMP_EVENT_DOWN,  COMP_INTENSET_DOWN_Msk);
    comp_execute_handler(NRF_COMP_EVENT_UP,    COMP_INTENSET_UP_Msk);
    comp_execute_handler(NRF_COMP_EVENT_CROSS, COMP_INTENSET_CROSS_Msk);
}


nrfx_err_t nrfx_comp_init(nrfx_comp_config_t const * p_config,
                          nrfx_comp_event_handler_t  event_handler)
{
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(event_handler);
    nrfx_err_t err_code;

    if (m_state != NRFX_DRV_STATE_UNINITIALIZED)
    { // COMP driver is already initialized
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    m_comp_event_handler = event_handler;

#if NRFX_CHECK(NRFX_PRS_ENABLED)
    if (nrfx_prs_acquire(NRF_COMP, nrfx_comp_irq_handler) != NRFX_SUCCESS)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
#endif

    nrf_comp_task_trigger(NRF_COMP_TASK_STOP);
    nrf_comp_enable();

    // Clear events to be sure there are no leftovers.
    nrf_comp_event_clear(NRF_COMP_EVENT_READY);
    nrf_comp_event_clear(NRF_COMP_EVENT_DOWN);
    nrf_comp_event_clear(NRF_COMP_EVENT_UP);
    nrf_comp_event_clear(NRF_COMP_EVENT_CROSS);

    nrf_comp_ref_set(p_config->reference);

    //If external source is chosen, write to appropriate register.
    if (p_config->reference == COMP_REFSEL_REFSEL_ARef)
    {
        nrf_comp_ext_ref_set(p_config->ext_ref);
    }

    nrf_comp_th_set(p_config->threshold);
    nrf_comp_main_mode_set(p_config->main_mode);
    nrf_comp_speed_mode_set(p_config->speed_mode);
    nrf_comp_hysteresis_set(p_config->hyst);
#if defined (COMP_ISOURCE_ISOURCE_Msk)
    nrf_comp_isource_set(p_config->isource);
#endif
    nrf_comp_shorts_disable(NRFX_COMP_SHORT_STOP_AFTER_CROSS_EVT |
                            NRFX_COMP_SHORT_STOP_AFTER_UP_EVT |
                            NRFX_COMP_SHORT_STOP_AFTER_DOWN_EVT);
    nrf_comp_int_disable(COMP_INTENCLR_CROSS_Msk |
                         COMP_INTENCLR_UP_Msk |
                         COMP_INTENCLR_DOWN_Msk |
                         COMP_INTENCLR_READY_Msk);

    nrf_comp_input_select(p_config->input);

    NRFX_IRQ_PRIORITY_SET(COMP_LPCOMP_IRQn, p_config->interrupt_priority);
    NRFX_IRQ_ENABLE(COMP_LPCOMP_IRQn);

    m_state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_comp_uninit(void)
{
    NRFX_ASSERT(m_state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_IRQ_DISABLE(COMP_LPCOMP_IRQn);
    nrf_comp_disable();
#if NRFX_CHECK(NRFX_PRS_ENABLED)
    nrfx_prs_release(NRF_COMP);
#endif
    m_state = NRFX_DRV_STATE_UNINITIALIZED;
    m_comp_event_handler = NULL;
    NRFX_LOG_INFO("Uninitialized.");
}

void nrfx_comp_pin_select(nrf_comp_input_t psel)
{
    bool comp_enable_state = nrf_comp_enable_check();
    nrf_comp_task_trigger(NRF_COMP_TASK_STOP);
    if (m_state == NRFX_DRV_STATE_POWERED_ON)
    {
        m_state = NRFX_DRV_STATE_INITIALIZED;
    }
    nrf_comp_disable();
    nrf_comp_input_select(psel);
    if (comp_enable_state == true)
    {
        nrf_comp_enable();
    }
}

void nrfx_comp_start(uint32_t comp_int_mask, uint32_t comp_shorts_mask)
{
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_INITIALIZED);
    nrf_comp_int_enable(comp_int_mask);
    nrf_comp_shorts_enable(comp_shorts_mask);
    nrf_comp_task_trigger(NRF_COMP_TASK_START);
    m_state = NRFX_DRV_STATE_POWERED_ON;
    NRFX_LOG_INFO("Enabled.");
}

void nrfx_comp_stop(void)
{
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_POWERED_ON);
    nrf_comp_shorts_disable(UINT32_MAX);
    nrf_comp_int_disable(UINT32_MAX);
    nrf_comp_task_trigger(NRF_COMP_TASK_STOP);
    m_state = NRFX_DRV_STATE_INITIALIZED;
    NRFX_LOG_INFO("Disabled.");
}

uint32_t nrfx_comp_sample()
{
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_POWERED_ON);
    nrf_comp_task_trigger(NRF_COMP_TASK_SAMPLE);
    return nrf_comp_result_get();
}

#endif // NRFX_CHECK(NRFX_COMP_ENABLED)
