/**
 * Copyright (c) 2015 - 2017, Nordic Semiconductor ASA
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

#if NRFX_CHECK(NRFX_QDEC_ENABLED)

#include <nrfx_qdec.h>
#include <hal/nrf_gpio.h>

#define NRFX_LOG_MODULE QDEC
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                             \
    (event == NRF_QDEC_EVENT_SAMPLERDY ? "NRF_QDEC_EVENT_SAMPLERDY" : \
    (event == NRF_QDEC_EVENT_REPORTRDY ? "NRF_QDEC_EVENT_REPORTRDY" : \
    (event == NRF_QDEC_EVENT_ACCOF     ? "NRF_QDEC_EVENT_ACCOF"     : \
                                         "UNKNOWN EVENT")))


static nrfx_qdec_event_handler_t m_qdec_event_handler = NULL;
static nrfx_drv_state_t m_state = NRFX_DRV_STATE_UNINITIALIZED;

void nrfx_qdec_irq_handler(void)
{
    nrfx_qdec_event_t event;
    if ( nrf_qdec_event_check(NRF_QDEC_EVENT_SAMPLERDY) &&
         nrf_qdec_int_enable_check(NRF_QDEC_INT_SAMPLERDY_MASK) )
    {
        nrf_qdec_event_clear(NRF_QDEC_EVENT_SAMPLERDY);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_QDEC_EVENT_SAMPLERDY));

        event.type = NRF_QDEC_EVENT_SAMPLERDY;
        event.data.sample.value = (int8_t)nrf_qdec_sample_get();
        m_qdec_event_handler(event);
    }

    if ( nrf_qdec_event_check(NRF_QDEC_EVENT_REPORTRDY) &&
         nrf_qdec_int_enable_check(NRF_QDEC_INT_REPORTRDY_MASK) )
    {
        nrf_qdec_event_clear(NRF_QDEC_EVENT_REPORTRDY);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_QDEC_EVENT_REPORTRDY));

        event.type = NRF_QDEC_EVENT_REPORTRDY;

        event.data.report.acc    = (int16_t)nrf_qdec_accread_get();
        event.data.report.accdbl = (uint16_t)nrf_qdec_accdblread_get();
        m_qdec_event_handler(event);
    }

    if ( nrf_qdec_event_check(NRF_QDEC_EVENT_ACCOF) &&
         nrf_qdec_int_enable_check(NRF_QDEC_INT_ACCOF_MASK) )
    {
        nrf_qdec_event_clear(NRF_QDEC_EVENT_ACCOF);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_QDEC_EVENT_ACCOF));

        event.type = NRF_QDEC_EVENT_ACCOF;
        m_qdec_event_handler(event);
    }
}


nrfx_err_t nrfx_qdec_init(nrfx_qdec_config_t const * p_config,
                          nrfx_qdec_event_handler_t  event_handler)
{
    NRFX_ASSERT(p_config);
    nrfx_err_t err_code;

    if (m_state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (event_handler)
    {
        m_qdec_event_handler = event_handler;
    }
    else
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    nrf_qdec_sampleper_set(p_config->sampleper);
    nrf_gpio_cfg_input(p_config->pselled, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(p_config->psela, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(p_config->pselb, NRF_GPIO_PIN_NOPULL);
    nrf_qdec_pio_assign(p_config->psela, p_config->pselb, p_config->pselled);
    nrf_qdec_ledpre_set(p_config->ledpre);
    nrf_qdec_ledpol_set(p_config->ledpol);
    nrf_qdec_shorts_enable(NRF_QDEC_SHORT_REPORTRDY_READCLRACC_MASK);

    if (p_config->dbfen)
    {
        nrf_qdec_dbfen_enable();
    }
    else
    {
        nrf_qdec_dbfen_disable();
    }

    uint32_t int_mask = NRF_QDEC_INT_ACCOF_MASK;

    if (p_config->reportper != NRF_QDEC_REPORTPER_DISABLED)
    {
        nrf_qdec_reportper_set(p_config->reportper);
        int_mask |= NRF_QDEC_INT_REPORTRDY_MASK;
    }

    if (p_config->sample_inten)
    {
        int_mask |= NRF_QDEC_INT_SAMPLERDY_MASK;
    }

    nrf_qdec_int_enable(int_mask);
    NRFX_IRQ_PRIORITY_SET(QDEC_IRQn, p_config->interrupt_priority);
    NRFX_IRQ_ENABLE(QDEC_IRQn);

    m_state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_qdec_uninit(void)
{
    NRFX_ASSERT(m_state != NRFX_DRV_STATE_UNINITIALIZED);
    nrfx_qdec_disable();
    NRFX_IRQ_DISABLE(QDEC_IRQn);
    m_state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Uninitialized.");
}

void nrfx_qdec_enable(void)
{
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_INITIALIZED);
    nrf_qdec_enable();
    nrf_qdec_task_trigger(NRF_QDEC_TASK_START);
    m_state = NRFX_DRV_STATE_POWERED_ON;
    NRFX_LOG_INFO("Enabled.");
}

void nrfx_qdec_disable(void)
{
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_POWERED_ON);
    nrf_qdec_task_trigger(NRF_QDEC_TASK_STOP);
    nrf_qdec_disable();
    m_state = NRFX_DRV_STATE_INITIALIZED;
    NRFX_LOG_INFO("Disabled.");
}

void nrfx_qdec_accumulators_read(int16_t * p_acc, int16_t * p_accdbl)
{
    NRFX_ASSERT(m_state == NRFX_DRV_STATE_POWERED_ON);
    nrf_qdec_task_trigger(NRF_QDEC_TASK_READCLRACC);

    *p_acc    = (int16_t)nrf_qdec_accread_get();
    *p_accdbl = (int16_t)nrf_qdec_accdblread_get();

    NRFX_LOG_DEBUG("Accumulators data, ACC register:");
    NRFX_LOG_HEXDUMP_DEBUG((uint8_t *)p_acc, sizeof(p_acc[0]));
    NRFX_LOG_DEBUG("Accumulators data, ACCDBL register:");
    NRFX_LOG_HEXDUMP_DEBUG((uint8_t *)p_accdbl, sizeof(p_accdbl[0]));
}

#endif // NRFX_CHECK(NRFX_QDEC_ENABLED)
