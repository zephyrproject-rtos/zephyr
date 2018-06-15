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

#if NRFX_CHECK(NRFX_ADC_ENABLED)

#include <nrfx_adc.h>

#define NRFX_LOG_MODULE ADC
#include <nrfx_log.h>

#define EVT_TO_STR(event)   (event == NRF_ADC_EVENT_END ? "NRF_ADC_EVENT_END" : "UNKNOWN EVENT")

typedef struct
{
    nrfx_adc_event_handler_t event_handler;
    nrfx_adc_channel_t     * p_head;
    nrfx_adc_channel_t     * p_current_conv;
    nrf_adc_value_t        * p_buffer;
    uint16_t                 size;
    uint16_t                 idx;
    nrfx_drv_state_t         state;
} adc_cb_t;

static adc_cb_t m_cb;

nrfx_err_t nrfx_adc_init(nrfx_adc_config_t const * p_config,
                         nrfx_adc_event_handler_t  event_handler)
{
    NRFX_ASSERT(p_config);
    nrfx_err_t err_code;

    if (m_cb.state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    nrf_adc_event_clear(NRF_ADC_EVENT_END);
    if (event_handler)
    {
        NRFX_IRQ_PRIORITY_SET(ADC_IRQn, p_config->interrupt_priority);
        NRFX_IRQ_ENABLE(ADC_IRQn);
    }
    m_cb.event_handler = event_handler;
    m_cb.state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_adc_uninit(void)
{
    NRFX_IRQ_DISABLE(ADC_IRQn);
    nrf_adc_int_disable(NRF_ADC_INT_END_MASK);
    nrf_adc_task_trigger(NRF_ADC_TASK_STOP);

    // Disable all channels. This must be done after the interrupt is disabled
    // because adc_sample_process() dereferences this pointer when it needs to
    // switch back to the first channel in the list (when the number of samples
    // to read is bigger than the number of enabled channels).
    m_cb.p_head = NULL;

    m_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
}

void nrfx_adc_channel_enable(nrfx_adc_channel_t * const p_channel)
{
    NRFX_ASSERT(!nrfx_adc_is_busy());

    p_channel->p_next = NULL;
    if (m_cb.p_head == NULL)
    {
        m_cb.p_head = p_channel;
    }
    else
    {
        nrfx_adc_channel_t * p_curr_channel = m_cb.p_head;
        while (p_curr_channel->p_next != NULL)
        {
            NRFX_ASSERT(p_channel != p_curr_channel);
            p_curr_channel = p_curr_channel->p_next;
        }
        p_curr_channel->p_next = p_channel;
    }

    NRFX_LOG_INFO("Enabled.");
}

void nrfx_adc_channel_disable(nrfx_adc_channel_t * const p_channel)
{
    NRFX_ASSERT(m_cb.p_head);
    NRFX_ASSERT(!nrfx_adc_is_busy());

    nrfx_adc_channel_t * p_curr_channel = m_cb.p_head;
    nrfx_adc_channel_t * p_prev_channel = NULL;
    while (p_curr_channel != p_channel)
    {
        p_prev_channel = p_curr_channel;
        p_curr_channel = p_curr_channel->p_next;
        NRFX_ASSERT(p_curr_channel != NULL);
    }
    if (p_prev_channel)
    {
        p_prev_channel->p_next = p_curr_channel->p_next;
    }
    else
    {
        m_cb.p_head = p_curr_channel->p_next;
    }

    NRFX_LOG_INFO("Disabled.");
}

void nrfx_adc_all_channels_disable(void)
{
    NRFX_ASSERT(!nrfx_adc_is_busy());

    m_cb.p_head = NULL;
}

void nrfx_adc_sample(void)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(!nrf_adc_busy_check());
    nrf_adc_task_trigger(NRF_ADC_TASK_START);
}

nrfx_err_t nrfx_adc_sample_convert(nrfx_adc_channel_t const * const p_channel,
                                   nrf_adc_value_t                * p_value)
{
    nrfx_err_t err_code;

    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    if (m_cb.state == NRFX_DRV_STATE_POWERED_ON)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    else
    {
        m_cb.state = NRFX_DRV_STATE_POWERED_ON;

        nrf_adc_init(&p_channel->config);
        nrf_adc_enable();
        nrf_adc_int_disable(NRF_ADC_INT_END_MASK);
        nrf_adc_task_trigger(NRF_ADC_TASK_START);
        if (p_value)
        {
            while (!nrf_adc_event_check(NRF_ADC_EVENT_END)) {}
            nrf_adc_event_clear(NRF_ADC_EVENT_END);
            *p_value = (nrf_adc_value_t)nrf_adc_result_get();
            nrf_adc_disable();

            m_cb.state = NRFX_DRV_STATE_INITIALIZED;
        }
        else
        {
            NRFX_ASSERT(m_cb.event_handler);
            m_cb.p_buffer = NULL;
            nrf_adc_int_enable(NRF_ADC_INT_END_MASK);
        }
        err_code = NRFX_SUCCESS;
        NRFX_LOG_INFO("Function: %s, error code: %s.",
                      __func__,
                      NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
}

static bool adc_sample_process()
{
    nrf_adc_event_clear(NRF_ADC_EVENT_END);
    nrf_adc_disable();
    m_cb.p_buffer[m_cb.idx] = (nrf_adc_value_t)nrf_adc_result_get();
    m_cb.idx++;
    if (m_cb.idx < m_cb.size)
    {
        bool task_trigger = false;
        if (m_cb.p_current_conv->p_next == NULL)
        {
            // Make sure the list of channels has not been somehow removed
            // (it is when all channels are disabled).
            NRFX_ASSERT(m_cb.p_head);

            m_cb.p_current_conv = m_cb.p_head;
        }
        else
        {
            m_cb.p_current_conv = m_cb.p_current_conv->p_next;
            task_trigger = true;
        }
        nrf_adc_init(&m_cb.p_current_conv->config);
        nrf_adc_enable();
        if (task_trigger)
        {
            nrf_adc_task_trigger(NRF_ADC_TASK_START);
        }
        return false;
    }
    else
    {
        return true;
    }
}

nrfx_err_t nrfx_adc_buffer_convert(nrf_adc_value_t * buffer, uint16_t size)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    nrfx_err_t err_code;

    NRFX_LOG_INFO("Number of samples requested to convert: %d.", size);

    if (m_cb.state == NRFX_DRV_STATE_POWERED_ON)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    else
    {
        m_cb.state          = NRFX_DRV_STATE_POWERED_ON;
        m_cb.p_current_conv = m_cb.p_head;
        m_cb.size           = size;
        m_cb.idx            = 0;
        m_cb.p_buffer       = buffer;
        nrf_adc_init(&m_cb.p_current_conv->config);
        nrf_adc_event_clear(NRF_ADC_EVENT_END);
        nrf_adc_enable();
        if (m_cb.event_handler)
        {
            nrf_adc_int_enable(NRF_ADC_INT_END_MASK);
        }
        else
        {
            while (1)
            {
                while (!nrf_adc_event_check(NRF_ADC_EVENT_END)){}

                if (adc_sample_process())
                {
                    m_cb.state = NRFX_DRV_STATE_INITIALIZED;
                    break;
                }
            }
        }
        err_code = NRFX_SUCCESS;
        NRFX_LOG_INFO("Function: %s, error code: %s.",
                      __func__,
                      NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
}

bool nrfx_adc_is_busy(void)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    return (m_cb.state == NRFX_DRV_STATE_POWERED_ON) ? true : false;
}

void nrfx_adc_irq_handler(void)
{
    if (m_cb.p_buffer == NULL)
    {
        nrf_adc_event_clear(NRF_ADC_EVENT_END);
        NRFX_LOG_DEBUG("Event: %s.",NRFX_LOG_ERROR_STRING_GET(NRF_ADC_EVENT_END));
        nrf_adc_int_disable(NRF_ADC_INT_END_MASK);
        nrf_adc_disable();
        nrfx_adc_evt_t evt;
        evt.type = NRFX_ADC_EVT_SAMPLE;
        evt.data.sample.sample = (nrf_adc_value_t)nrf_adc_result_get();
        NRFX_LOG_DEBUG("ADC data:");
        NRFX_LOG_HEXDUMP_DEBUG((uint8_t *)(&evt.data.sample.sample), sizeof(nrf_adc_value_t));
        m_cb.state = NRFX_DRV_STATE_INITIALIZED;
        m_cb.event_handler(&evt);
    }
    else if (adc_sample_process())
    {
        NRFX_LOG_DEBUG("Event: %s.", NRFX_LOG_ERROR_STRING_GET(NRF_ADC_EVENT_END));
        nrf_adc_int_disable(NRF_ADC_INT_END_MASK);
        nrfx_adc_evt_t evt;
        evt.type = NRFX_ADC_EVT_DONE;
        evt.data.done.p_buffer = m_cb.p_buffer;
        evt.data.done.size     = m_cb.size;
        m_cb.state = NRFX_DRV_STATE_INITIALIZED;
        NRFX_LOG_DEBUG("ADC data:");
        NRFX_LOG_HEXDUMP_DEBUG((uint8_t *)m_cb.p_buffer, m_cb.size * sizeof(nrf_adc_value_t));
        m_cb.event_handler(&evt);
    }
}

#endif // NRFX_CHECK(NRFX_ADC_ENABLED)
