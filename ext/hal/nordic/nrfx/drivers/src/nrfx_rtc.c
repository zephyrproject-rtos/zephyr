/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
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

#if NRFX_CHECK(NRFX_RTC_ENABLED)

#if !(NRFX_CHECK(NRFX_RTC0_ENABLED) || NRFX_CHECK(NRFX_RTC1_ENABLED) || \
      NRFX_CHECK(NRFX_RTC2_ENABLED))
#error "No enabled RTC instances. Check <nrfx_config.h>."
#endif

#include <nrfx_rtc.h>

#define NRFX_LOG_MODULE RTC
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                           \
    (event == NRF_RTC_EVENT_TICK      ? "NRF_RTC_EVENT_TICK"      : \
    (event == NRF_RTC_EVENT_OVERFLOW  ? "NRF_RTC_EVENT_OVERFLOW"  : \
    (event == NRF_RTC_EVENT_COMPARE_0 ? "NRF_RTC_EVENT_COMPARE_0" : \
    (event == NRF_RTC_EVENT_COMPARE_1 ? "NRF_RTC_EVENT_COMPARE_1" : \
    (event == NRF_RTC_EVENT_COMPARE_2 ? "NRF_RTC_EVENT_COMPARE_2" : \
    (event == NRF_RTC_EVENT_COMPARE_3 ? "NRF_RTC_EVENT_COMPARE_3" : \
                                        "UNKNOWN EVENT"))))))


/**@brief RTC driver instance control block structure. */
typedef struct
{
    nrfx_drv_state_t state;        /**< Instance state. */
    bool             reliable;     /**< Reliable mode flag. */
    uint8_t          tick_latency; /**< Maximum length of interrupt handler in ticks (max 7.7 ms). */
} nrfx_rtc_cb_t;

// User callbacks local storage.
static nrfx_rtc_handler_t m_handlers[NRFX_RTC_ENABLED_COUNT];
static nrfx_rtc_cb_t      m_cb[NRFX_RTC_ENABLED_COUNT];

nrfx_err_t nrfx_rtc_init(nrfx_rtc_t const * const  p_instance,
                         nrfx_rtc_config_t const * p_config,
                         nrfx_rtc_handler_t        handler)
{
    NRFX_ASSERT(p_config);

    nrfx_err_t err_code;

    if (handler)
    {
        m_handlers[p_instance->instance_id] = handler;
    }
    else
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    NRFX_IRQ_PRIORITY_SET(p_instance->irq, p_config->interrupt_priority);
    NRFX_IRQ_ENABLE(p_instance->irq);
    nrf_rtc_prescaler_set(p_instance->p_reg, p_config->prescaler);
    m_cb[p_instance->instance_id].reliable     = p_config->reliable;
    m_cb[p_instance->instance_id].tick_latency = p_config->tick_latency;
    m_cb[p_instance->instance_id].state        = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_rtc_uninit(nrfx_rtc_t const * const p_instance)
{
    uint32_t mask = NRF_RTC_INT_TICK_MASK     |
                    NRF_RTC_INT_OVERFLOW_MASK |
                    NRF_RTC_INT_COMPARE0_MASK |
                    NRF_RTC_INT_COMPARE1_MASK |
                    NRF_RTC_INT_COMPARE2_MASK |
                    NRF_RTC_INT_COMPARE3_MASK;
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);

    NRFX_IRQ_DISABLE(p_instance->irq);

    nrf_rtc_task_trigger(p_instance->p_reg, NRF_RTC_TASK_STOP);
    nrf_rtc_event_disable(p_instance->p_reg, mask);
    nrf_rtc_int_disable(p_instance->p_reg, mask);

    m_cb[p_instance->instance_id].state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Uninitialized.");
}

void nrfx_rtc_enable(nrfx_rtc_t const * const p_instance)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state == NRFX_DRV_STATE_INITIALIZED);

    nrf_rtc_task_trigger(p_instance->p_reg, NRF_RTC_TASK_START);
    m_cb[p_instance->instance_id].state = NRFX_DRV_STATE_POWERED_ON;
    NRFX_LOG_INFO("Enabled.");
}

void nrfx_rtc_disable(nrfx_rtc_t const * const p_instance)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);

    nrf_rtc_task_trigger(p_instance->p_reg, NRF_RTC_TASK_STOP);
    m_cb[p_instance->instance_id].state = NRFX_DRV_STATE_INITIALIZED;
    NRFX_LOG_INFO("Disabled.");
}

nrfx_err_t nrfx_rtc_cc_disable(nrfx_rtc_t const * const p_instance, uint32_t channel)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(channel<p_instance->cc_channel_count);

    nrfx_err_t err_code;
    uint32_t int_mask = RTC_CHANNEL_INT_MASK(channel);
    nrf_rtc_event_t event    = RTC_CHANNEL_EVENT_ADDR(channel);

    nrf_rtc_event_disable(p_instance->p_reg,int_mask);
    if (nrf_rtc_int_is_enabled(p_instance->p_reg,int_mask))
    {
        nrf_rtc_int_disable(p_instance->p_reg,int_mask);
        if (nrf_rtc_event_pending(p_instance->p_reg,event))
        {
            nrf_rtc_event_clear(p_instance->p_reg,event);
            err_code = NRFX_ERROR_TIMEOUT;
            NRFX_LOG_WARNING("Function: %s, error code: %s.",
                             __func__,
                             NRFX_LOG_ERROR_STRING_GET(err_code));
            return err_code;
        }
    }
    NRFX_LOG_INFO("RTC id: %d, channel disabled: %d.", p_instance->instance_id, channel);
    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_rtc_cc_set(nrfx_rtc_t const * const p_instance,
                           uint32_t channel,
                           uint32_t val,
                           bool enable_irq)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(channel<p_instance->cc_channel_count);

    nrfx_err_t err_code;
    uint32_t int_mask = RTC_CHANNEL_INT_MASK(channel);
    nrf_rtc_event_t event    = RTC_CHANNEL_EVENT_ADDR(channel);

    nrf_rtc_event_disable(p_instance->p_reg, int_mask);
    nrf_rtc_int_disable(p_instance->p_reg, int_mask);

    val = RTC_WRAP(val);
    if (m_cb[p_instance->instance_id].reliable)
    {
        nrf_rtc_cc_set(p_instance->p_reg,channel,val);
        uint32_t cnt = nrf_rtc_counter_get(p_instance->p_reg);
        int32_t diff = cnt - val;
        if (cnt < val)
        {
            diff += RTC_COUNTER_COUNTER_Msk;
        }
        if (diff < m_cb[p_instance->instance_id].tick_latency)
        {
            err_code = NRFX_ERROR_TIMEOUT;
            NRFX_LOG_WARNING("Function: %s, error code: %s.",
                             __func__,
                             NRFX_LOG_ERROR_STRING_GET(err_code));
            return err_code;
        }
    }
    else
    {
        nrf_rtc_cc_set(p_instance->p_reg,channel,val);
    }

    if (enable_irq)
    {
        nrf_rtc_event_clear(p_instance->p_reg,event);
        nrf_rtc_int_enable(p_instance->p_reg, int_mask);
    }
    nrf_rtc_event_enable(p_instance->p_reg,int_mask);

    NRFX_LOG_INFO("RTC id: %d, channel enabled: %d, compare value: %d.",
                  p_instance->instance_id,
                  channel,
                  val);
    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_rtc_tick_enable(nrfx_rtc_t const * const p_instance, bool enable_irq)
{
    nrf_rtc_event_t event = NRF_RTC_EVENT_TICK;
    uint32_t mask = NRF_RTC_INT_TICK_MASK;

    nrf_rtc_event_clear(p_instance->p_reg, event);
    nrf_rtc_event_enable(p_instance->p_reg, mask);
    if (enable_irq)
    {
        nrf_rtc_int_enable(p_instance->p_reg, mask);
    }
    NRFX_LOG_INFO("Tick events enabled.");
}

void nrfx_rtc_tick_disable(nrfx_rtc_t const * const p_instance)
{
    uint32_t mask = NRF_RTC_INT_TICK_MASK;

    nrf_rtc_event_disable(p_instance->p_reg, mask);
    nrf_rtc_int_disable(p_instance->p_reg, mask);
    NRFX_LOG_INFO("Tick events disabled.");
}

void nrfx_rtc_overflow_enable(nrfx_rtc_t const * const p_instance, bool enable_irq)
{
    nrf_rtc_event_t event = NRF_RTC_EVENT_OVERFLOW;
    uint32_t mask = NRF_RTC_INT_OVERFLOW_MASK;

    nrf_rtc_event_clear(p_instance->p_reg, event);
    nrf_rtc_event_enable(p_instance->p_reg, mask);
    if (enable_irq)
    {
        nrf_rtc_int_enable(p_instance->p_reg, mask);
    }
}

void nrfx_rtc_overflow_disable(nrfx_rtc_t const * const p_instance)
{
    uint32_t mask = NRF_RTC_INT_OVERFLOW_MASK;
    nrf_rtc_event_disable(p_instance->p_reg, mask);
    nrf_rtc_int_disable(p_instance->p_reg, mask);
}

uint32_t nrfx_rtc_max_ticks_get(nrfx_rtc_t const * const p_instance)
{
    uint32_t ticks;
    if (m_cb[p_instance->instance_id].reliable)
    {
        ticks = RTC_COUNTER_COUNTER_Msk - m_cb[p_instance->instance_id].tick_latency;
    }
    else
    {
        ticks = RTC_COUNTER_COUNTER_Msk;
    }
    return ticks;
}

static void irq_handler(NRF_RTC_Type * p_reg,
                        uint32_t       instance_id,
                        uint32_t       channel_count)
{
    uint32_t i;
    uint32_t int_mask = (uint32_t)NRF_RTC_INT_COMPARE0_MASK;
    nrf_rtc_event_t event = NRF_RTC_EVENT_COMPARE_0;

    for (i = 0; i < channel_count; i++)
    {
        if (nrf_rtc_int_is_enabled(p_reg,int_mask) && nrf_rtc_event_pending(p_reg,event))
        {
            nrf_rtc_event_disable(p_reg,int_mask);
            nrf_rtc_int_disable(p_reg,int_mask);
            nrf_rtc_event_clear(p_reg,event);
            NRFX_LOG_DEBUG("Event: %s, instance id: %d.", EVT_TO_STR(event), instance_id);
            m_handlers[instance_id]((nrfx_rtc_int_type_t)i);
        }
        int_mask <<= 1;
        event    = (nrf_rtc_event_t)((uint32_t)event + sizeof(uint32_t));
    }
    event = NRF_RTC_EVENT_TICK;
    if (nrf_rtc_int_is_enabled(p_reg,NRF_RTC_INT_TICK_MASK) &&
        nrf_rtc_event_pending(p_reg, event))
    {
        nrf_rtc_event_clear(p_reg, event);
        NRFX_LOG_DEBUG("Event: %s, instance id: %d.", EVT_TO_STR(event), instance_id);
        m_handlers[instance_id](NRFX_RTC_INT_TICK);
    }

    event = NRF_RTC_EVENT_OVERFLOW;
    if (nrf_rtc_int_is_enabled(p_reg,NRF_RTC_INT_OVERFLOW_MASK) &&
        nrf_rtc_event_pending(p_reg, event))
    {
        nrf_rtc_event_clear(p_reg,event);
        NRFX_LOG_DEBUG("Event: %s, instance id: %d.", EVT_TO_STR(event), instance_id);
        m_handlers[instance_id](NRFX_RTC_INT_OVERFLOW);
    }
}

#if NRFX_CHECK(NRFX_RTC0_ENABLED)
void nrfx_rtc_0_irq_handler(void)
{
    irq_handler(NRF_RTC0, NRFX_RTC0_INST_IDX, NRF_RTC_CC_CHANNEL_COUNT(0));
}
#endif

#if NRFX_CHECK(NRFX_RTC1_ENABLED)
void nrfx_rtc_1_irq_handler(void)
{
    irq_handler(NRF_RTC1, NRFX_RTC1_INST_IDX, NRF_RTC_CC_CHANNEL_COUNT(1));
}
#endif

#if NRFX_CHECK(NRFX_RTC2_ENABLED)
void nrfx_rtc_2_irq_handler(void)
{
    irq_handler(NRF_RTC2, NRFX_RTC2_INST_IDX, NRF_RTC_CC_CHANNEL_COUNT(2));
}
#endif

#endif // NRFX_CHECK(NRFX_RTC_ENABLED)
