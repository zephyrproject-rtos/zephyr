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

#if NRFX_CHECK(NRFX_SAADC_ENABLED)
#include <nrfx_saadc.h>

#define NRFX_LOG_MODULE SAADC
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                                       \
    (event == NRF_SAADC_EVENT_STARTED       ? "NRF_SAADC_EVENT_STARTED"       : \
    (event == NRF_SAADC_EVENT_END           ? "NRF_SAADC_EVENT_END"           : \
    (event == NRF_SAADC_EVENT_DONE          ? "NRF_SAADC_EVENT_DONE"          : \
    (event == NRF_SAADC_EVENT_RESULTDONE    ? "NRF_SAADC_EVENT_RESULTDONE"    : \
    (event == NRF_SAADC_EVENT_CALIBRATEDONE ? "NRF_SAADC_EVENT_CALIBRATEDONE" : \
    (event == NRF_SAADC_EVENT_STOPPED       ? "NRF_SAADC_EVENT_STOPPED"       : \
                                              "UNKNOWN EVENT"))))))


typedef enum
{
    NRF_SAADC_STATE_IDLE        = 0,
    NRF_SAADC_STATE_BUSY        = 1,
    NRF_SAADC_STATE_CALIBRATION = 2
} nrf_saadc_state_t;


typedef struct
{
    nrf_saadc_input_t pselp;
    nrf_saadc_input_t pseln;
} nrf_saadc_psel_buffer;

/** @brief SAADC control block.*/
typedef struct
{
    nrfx_saadc_event_handler_t    event_handler;                 ///< Event handler function pointer.
    volatile nrf_saadc_value_t  * p_buffer;                      ///< Sample buffer.
    volatile uint16_t             buffer_size;                   ///< Size of the sample buffer.
    volatile nrf_saadc_value_t  * p_secondary_buffer;            ///< Secondary sample buffer.
    volatile nrf_saadc_state_t    adc_state;                     ///< State of the SAADC.
    uint32_t                      limits_enabled_flags;          ///< Enabled limits flags.
    uint16_t                      secondary_buffer_size;         ///< Size of the secondary buffer.
    uint16_t                      buffer_size_left;              ///< When low power mode is active indicates how many samples left to convert on current buffer.
    nrf_saadc_psel_buffer         psel[NRF_SAADC_CHANNEL_COUNT]; ///< Pin configurations of SAADC channels.
    nrfx_drv_state_t              state;                         ///< Driver initialization state.
    uint8_t                       active_channels;               ///< Number of enabled SAADC channels.
    bool                          low_power_mode;                ///< Indicates if low power mode is active.
    bool                          conversions_end;               ///< When low power mode is active indicates end of conversions on current buffer.
} nrfx_saadc_cb_t;

static nrfx_saadc_cb_t m_cb;

#define LOW_LIMIT_TO_FLAG(channel)      ((2 * channel + 1))
#define HIGH_LIMIT_TO_FLAG(channel)     ((2 * channel))
#define FLAG_IDX_TO_EVENT(idx)          ((nrf_saadc_event_t)((uint32_t)NRF_SAADC_EVENT_CH0_LIMITH + \
                                            4 * idx))
#define LIMIT_EVENT_TO_CHANNEL(event)   (uint8_t)(((uint32_t)event - \
                                            (uint32_t)NRF_SAADC_EVENT_CH0_LIMITH) / 8)
#define LIMIT_EVENT_TO_LIMIT_TYPE(event)((((uint32_t)event - (uint32_t)NRF_SAADC_EVENT_CH0_LIMITH) & 4) \
                                            ? NRF_SAADC_LIMIT_LOW : NRF_SAADC_LIMIT_HIGH)
#define HW_TIMEOUT 10000

void nrfx_saadc_irq_handler(void)
{
    if (nrf_saadc_event_check(NRF_SAADC_EVENT_END))
    {
        nrf_saadc_event_clear(NRF_SAADC_EVENT_END);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_SAADC_EVENT_END));

        if (!m_cb.low_power_mode || m_cb.conversions_end)
        {
            nrfx_saadc_evt_t evt;
            evt.type               = NRFX_SAADC_EVT_DONE;
            evt.data.done.p_buffer = (nrf_saadc_value_t *)m_cb.p_buffer;
            evt.data.done.size     = m_cb.buffer_size;

            if (m_cb.p_secondary_buffer == NULL)
            {
                m_cb.adc_state = NRF_SAADC_STATE_IDLE;
            }
            else
            {
                m_cb.buffer_size_left   = m_cb.secondary_buffer_size;
                m_cb.p_buffer           = m_cb.p_secondary_buffer;
                m_cb.buffer_size        = m_cb.secondary_buffer_size;
                m_cb.p_secondary_buffer = NULL;
                if (!m_cb.low_power_mode)
                {
                    nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
                }
            }
            m_cb.event_handler(&evt);
            m_cb.conversions_end = false;
        }
    }
    if (m_cb.low_power_mode && nrf_saadc_event_check(NRF_SAADC_EVENT_STARTED))
    {
        nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_SAADC_EVENT_STARTED));

        if (m_cb.buffer_size_left > m_cb.active_channels)
        {
            // More samples to convert than for single event.
            m_cb.buffer_size_left -= m_cb.active_channels;
            nrf_saadc_buffer_init((nrf_saadc_value_t *)&m_cb.p_buffer[m_cb.buffer_size -
                                                                      m_cb.buffer_size_left],
                                  m_cb.active_channels);
        }
        else if ((m_cb.buffer_size_left == m_cb.active_channels) &&

                 (m_cb.p_secondary_buffer != NULL))
        {
            // Samples to convert for one event, prepare next buffer.
            m_cb.conversions_end  = true;
            m_cb.buffer_size_left = 0;
            nrf_saadc_buffer_init((nrf_saadc_value_t *)m_cb.p_secondary_buffer,
                                  m_cb.active_channels);
        }
        else if (m_cb.buffer_size_left == m_cb.active_channels)
        {
            // Samples to convert for one event, but no second buffer.
            m_cb.conversions_end  = true;
            m_cb.buffer_size_left = 0;
        }
        nrf_saadc_event_clear(NRF_SAADC_EVENT_END);
        nrf_saadc_task_trigger(NRF_SAADC_TASK_SAMPLE);
    }
    if (nrf_saadc_event_check(NRF_SAADC_EVENT_CALIBRATEDONE))
    {
        nrf_saadc_event_clear(NRF_SAADC_EVENT_CALIBRATEDONE);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_SAADC_EVENT_CALIBRATEDONE));
        m_cb.adc_state = NRF_SAADC_STATE_IDLE;

        nrfx_saadc_evt_t evt;
        evt.type = NRFX_SAADC_EVT_CALIBRATEDONE;
        m_cb.event_handler(&evt);
    }
    if (nrf_saadc_event_check(NRF_SAADC_EVENT_STOPPED))
    {
        nrf_saadc_event_clear(NRF_SAADC_EVENT_STOPPED);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_SAADC_EVENT_STOPPED));
        m_cb.adc_state = NRF_SAADC_STATE_IDLE;
    }
    else
    {
        uint32_t          limit_flags = m_cb.limits_enabled_flags;
        uint32_t          flag_idx;
        nrf_saadc_event_t event;

        while (limit_flags)
        {
            flag_idx     = __CLZ(limit_flags);
            limit_flags &= ~((1UL << 31) >> flag_idx);
            event        = FLAG_IDX_TO_EVENT(flag_idx);
            if (nrf_saadc_event_check(event))
            {
                nrf_saadc_event_clear(event);
                nrfx_saadc_evt_t evt;
                evt.type                  = NRFX_SAADC_EVT_LIMIT;
                evt.data.limit.channel    = LIMIT_EVENT_TO_CHANNEL(event);
                evt.data.limit.limit_type = LIMIT_EVENT_TO_LIMIT_TYPE(event);
                NRFX_LOG_DEBUG("Event limit, channel: %d, limit type: %d.",
                               evt.data.limit.channel,
                               evt.data.limit.limit_type);
                m_cb.event_handler(&evt);
            }
        }
    }
}


nrfx_err_t nrfx_saadc_init(nrfx_saadc_config_t const * p_config,
                           nrfx_saadc_event_handler_t  event_handler)
{
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(event_handler);
    nrfx_err_t err_code;

    if (m_cb.state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    m_cb.event_handler = event_handler;
    nrf_saadc_resolution_set(p_config->resolution);
    nrf_saadc_oversample_set(p_config->oversample);
    m_cb.low_power_mode       = p_config->low_power_mode;
    m_cb.state                = NRFX_DRV_STATE_INITIALIZED;
    m_cb.adc_state            = NRF_SAADC_STATE_IDLE;
    m_cb.active_channels      = 0;
    m_cb.limits_enabled_flags = 0;
    m_cb.conversions_end      = false;

    nrf_saadc_int_disable(NRF_SAADC_INT_ALL);
    nrf_saadc_event_clear(NRF_SAADC_EVENT_END);
    nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
    NRFX_IRQ_PRIORITY_SET(SAADC_IRQn, p_config->interrupt_priority);
    NRFX_IRQ_ENABLE(SAADC_IRQn);
    nrf_saadc_int_enable(NRF_SAADC_INT_END);

    if (m_cb.low_power_mode)
    {
        nrf_saadc_int_enable(NRF_SAADC_INT_STARTED);
    }

    nrf_saadc_enable();

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));

    return err_code;
}


void nrfx_saadc_uninit(void)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    nrf_saadc_int_disable(NRF_SAADC_INT_ALL);
    NRFX_IRQ_DISABLE(SAADC_IRQn);
    nrf_saadc_task_trigger(NRF_SAADC_TASK_STOP);

    // Wait for ADC being stopped.
    bool result;
    NRFX_WAIT_FOR(nrf_saadc_event_check(NRF_SAADC_EVENT_STOPPED), HW_TIMEOUT, 0, result);
    NRFX_ASSERT(result);

    nrf_saadc_disable();
    m_cb.adc_state = NRF_SAADC_STATE_IDLE;

    for (uint32_t channel = 0; channel < NRF_SAADC_CHANNEL_COUNT; ++channel)
    {
        if (m_cb.psel[channel].pselp != NRF_SAADC_INPUT_DISABLED)
        {
            nrfx_err_t err_code = nrfx_saadc_channel_uninit(channel);
            NRFX_ASSERT(err_code == NRFX_SUCCESS);
        }
    }

    m_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
}


nrfx_err_t nrfx_saadc_channel_init(uint8_t                                  channel,
                                   nrf_saadc_channel_config_t const * const p_config)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(channel < NRF_SAADC_CHANNEL_COUNT);
    // Oversampling can be used only with one channel.
    NRFX_ASSERT((nrf_saadc_oversample_get() == NRF_SAADC_OVERSAMPLE_DISABLED) ||
                (m_cb.active_channels == 0));
    NRFX_ASSERT((p_config->pin_p <= NRF_SAADC_INPUT_VDD) &&
                (p_config->pin_p > NRF_SAADC_INPUT_DISABLED));
    NRFX_ASSERT(p_config->pin_n <= NRF_SAADC_INPUT_VDD);

    nrfx_err_t err_code;

    // A channel can only be initialized if the driver is in the idle state.
    if (m_cb.adc_state != NRF_SAADC_STATE_IDLE)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

#ifdef NRF52_PAN_74
    if ((p_config->acq_time == NRF_SAADC_ACQTIME_3US) ||
        (p_config->acq_time == NRF_SAADC_ACQTIME_5US))
    {
        nrf_saadc_disable();
    }
#endif //NRF52_PAN_74

    if (m_cb.psel[channel].pselp == NRF_SAADC_INPUT_DISABLED)
    {
        ++m_cb.active_channels;
    }
    m_cb.psel[channel].pselp = p_config->pin_p;
    m_cb.psel[channel].pseln = p_config->pin_n;
    nrf_saadc_channel_init(channel, p_config);

#ifdef NRF52_PAN_74
    if ((p_config->acq_time == NRF_SAADC_ACQTIME_3US) ||
        (p_config->acq_time == NRF_SAADC_ACQTIME_5US))
    {
        nrf_saadc_enable();
    }
#endif //NRF52_PAN_74

    NRFX_LOG_INFO("Channel initialized: %d.", channel);
    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_saadc_channel_uninit(uint8_t channel)
{
    NRFX_ASSERT(channel < NRF_SAADC_CHANNEL_COUNT);
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    nrfx_err_t err_code;

    // A channel can only be uninitialized if the driver is in the idle state.
    if (m_cb.adc_state != NRF_SAADC_STATE_IDLE)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (m_cb.psel[channel].pselp != NRF_SAADC_INPUT_DISABLED)
    {
        --m_cb.active_channels;
    }
    m_cb.psel[channel].pselp = NRF_SAADC_INPUT_DISABLED;
    m_cb.psel[channel].pseln = NRF_SAADC_INPUT_DISABLED;
    nrf_saadc_channel_input_set(channel, NRF_SAADC_INPUT_DISABLED, NRF_SAADC_INPUT_DISABLED);
    nrfx_saadc_limits_set(channel, NRFX_SAADC_LIMITL_DISABLED, NRFX_SAADC_LIMITH_DISABLED);
    NRFX_LOG_INFO("Channel denitialized: %d.", channel);

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


uint32_t nrfx_saadc_sample_task_get(void)
{
    return nrf_saadc_task_address_get(
                m_cb.low_power_mode ? NRF_SAADC_TASK_START : NRF_SAADC_TASK_SAMPLE);
}


nrfx_err_t nrfx_saadc_sample_convert(uint8_t channel, nrf_saadc_value_t * p_value)
{
    nrfx_err_t err_code;

    if (m_cb.adc_state != NRF_SAADC_STATE_IDLE)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    m_cb.adc_state = NRF_SAADC_STATE_BUSY;
    nrf_saadc_int_disable(NRF_SAADC_INT_STARTED | NRF_SAADC_INT_END);
    nrf_saadc_buffer_init(p_value, 1);
    if (m_cb.active_channels > 1)
    {
        for (uint32_t i = 0; i < NRF_SAADC_CHANNEL_COUNT; ++i)
        {
            nrf_saadc_channel_input_set(i, NRF_SAADC_INPUT_DISABLED, NRF_SAADC_INPUT_DISABLED);
        }
    }
    nrf_saadc_channel_input_set(channel, m_cb.psel[channel].pselp, m_cb.psel[channel].pseln);
    nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
    nrf_saadc_task_trigger(NRF_SAADC_TASK_SAMPLE);

    bool result;
    NRFX_WAIT_FOR(nrf_saadc_event_check(NRF_SAADC_EVENT_END), HW_TIMEOUT, 0, result);
    NRFX_ASSERT(result);

    nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
    nrf_saadc_event_clear(NRF_SAADC_EVENT_END);

    NRFX_LOG_INFO("Conversion value: %d, channel %d.", *p_value, channel);

    if (m_cb.active_channels > 1)
    {
        for (uint32_t i = 0; i < NRF_SAADC_CHANNEL_COUNT; ++i)
        {
            nrf_saadc_channel_input_set(i, m_cb.psel[i].pselp, m_cb.psel[i].pseln);
        }
    }

    if (m_cb.low_power_mode)
    {
        nrf_saadc_int_enable(NRF_SAADC_INT_STARTED | NRF_SAADC_INT_END);
    }
    else
    {
        nrf_saadc_int_enable(NRF_SAADC_INT_END);
    }

    m_cb.adc_state = NRF_SAADC_STATE_IDLE;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_WARNING("Function: %s, error code: %s.",
                     __func__,
                     NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_saadc_buffer_convert(nrf_saadc_value_t * p_buffer, uint16_t size)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT((size % m_cb.active_channels) == 0);
    nrfx_err_t err_code;

    nrf_saadc_int_disable(NRF_SAADC_INT_END | NRF_SAADC_INT_CALIBRATEDONE);
    if (m_cb.adc_state == NRF_SAADC_STATE_CALIBRATION)
    {
        nrf_saadc_int_enable(NRF_SAADC_INT_END | NRF_SAADC_INT_CALIBRATEDONE);
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    if (m_cb.adc_state == NRF_SAADC_STATE_BUSY)
    {
        if ( m_cb.p_secondary_buffer)
        {
            nrf_saadc_int_enable(NRF_SAADC_INT_END);
            err_code = NRFX_ERROR_BUSY;
            NRFX_LOG_WARNING("Function: %s, error code: %s.",
                             __func__,
                             NRFX_LOG_ERROR_STRING_GET(err_code));
            return err_code;
        }
        else
        {
            m_cb.p_secondary_buffer    = p_buffer;
            m_cb.secondary_buffer_size = size;
            if (!m_cb.low_power_mode)
            {
                while (nrf_saadc_event_check(NRF_SAADC_EVENT_STARTED) == 0);
                nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
                nrf_saadc_buffer_init(p_buffer, size);
            }
            nrf_saadc_int_enable(NRF_SAADC_INT_END);
            err_code = NRFX_SUCCESS;
            NRFX_LOG_WARNING("Function: %s, error code: %s.",
                             __func__,
                             NRFX_LOG_ERROR_STRING_GET(err_code));
            return err_code;
        }
    }
    nrf_saadc_int_enable(NRF_SAADC_INT_END);
    m_cb.adc_state = NRF_SAADC_STATE_BUSY;

    m_cb.p_buffer           = p_buffer;
    m_cb.buffer_size        = size;
    m_cb.p_secondary_buffer = NULL;

    NRFX_LOG_INFO("Function: %s, buffer length: %d, active channels: %d.",
                  __func__,
                  size,
                  m_cb.active_channels);

    if (m_cb.low_power_mode)
    {
        m_cb.buffer_size_left = size;
        nrf_saadc_buffer_init(p_buffer, m_cb.active_channels);
    }
    else
    {
        nrf_saadc_buffer_init(p_buffer, size);
        nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
        nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
    }

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_saadc_sample()
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    nrfx_err_t err_code = NRFX_SUCCESS;
    if (m_cb.adc_state != NRF_SAADC_STATE_BUSY)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
    }
    else if (m_cb.low_power_mode)
    {
        nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
    }
    else
    {
        nrf_saadc_task_trigger(NRF_SAADC_TASK_SAMPLE);
    }

    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_saadc_calibrate_offset()
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    nrfx_err_t err_code;

    if (m_cb.adc_state != NRF_SAADC_STATE_IDLE)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    m_cb.adc_state = NRF_SAADC_STATE_CALIBRATION;

    nrf_saadc_event_clear(NRF_SAADC_EVENT_CALIBRATEDONE);
    nrf_saadc_int_enable(NRF_SAADC_INT_CALIBRATEDONE);
    nrf_saadc_task_trigger(NRF_SAADC_TASK_CALIBRATEOFFSET);
    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.",
                  __func__,
                  NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


bool nrfx_saadc_is_busy(void)
{
    return (m_cb.adc_state != NRF_SAADC_STATE_IDLE);
}


void nrfx_saadc_abort(void)
{
    if (nrfx_saadc_is_busy())
    {
        nrf_saadc_event_clear(NRF_SAADC_EVENT_STOPPED);
        nrf_saadc_int_enable(NRF_SAADC_INT_STOPPED);
        nrf_saadc_task_trigger(NRF_SAADC_TASK_STOP);

        if (m_cb.adc_state == NRF_SAADC_STATE_CALIBRATION)
        {
            m_cb.adc_state = NRF_SAADC_STATE_IDLE;
        }
        else
        {
            // Wait for ADC being stopped.
            bool result;
            NRFX_WAIT_FOR((m_cb.adc_state != NRF_SAADC_STATE_IDLE), HW_TIMEOUT, 0, result);
            NRFX_ASSERT(result);
        }

        nrf_saadc_int_disable(NRF_SAADC_INT_STOPPED);

        m_cb.p_buffer           = 0;
        m_cb.p_secondary_buffer = 0;
        NRFX_LOG_INFO("Conversion aborted.");
    }
}


void nrfx_saadc_limits_set(uint8_t channel, int16_t limit_low, int16_t limit_high)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(m_cb.event_handler); // only non blocking mode supported
    NRFX_ASSERT(limit_low >= NRFX_SAADC_LIMITL_DISABLED);
    NRFX_ASSERT(limit_high <= NRFX_SAADC_LIMITH_DISABLED);
    NRFX_ASSERT(limit_low < limit_high);
    nrf_saadc_channel_limits_set(channel, limit_low, limit_high);

    uint32_t int_mask = nrf_saadc_limit_int_get(channel, NRF_SAADC_LIMIT_LOW);
    if (limit_low == NRFX_SAADC_LIMITL_DISABLED)
    {
        m_cb.limits_enabled_flags &= ~(0x80000000 >> LOW_LIMIT_TO_FLAG(channel));
        nrf_saadc_int_disable(int_mask);
    }
    else
    {
        m_cb.limits_enabled_flags |= (0x80000000 >> LOW_LIMIT_TO_FLAG(channel));
        nrf_saadc_int_enable(int_mask);
    }

    int_mask = nrf_saadc_limit_int_get(channel, NRF_SAADC_LIMIT_HIGH);
    if (limit_high == NRFX_SAADC_LIMITH_DISABLED)
    {
        m_cb.limits_enabled_flags &= ~(0x80000000 >> HIGH_LIMIT_TO_FLAG(channel));
        nrf_saadc_int_disable(int_mask);
    }
    else
    {
        m_cb.limits_enabled_flags |= (0x80000000 >> HIGH_LIMIT_TO_FLAG(channel));
        nrf_saadc_int_enable(int_mask);
    }
}
#endif // NRFX_CHECK(NRFX_SAADC_ENABLED)
