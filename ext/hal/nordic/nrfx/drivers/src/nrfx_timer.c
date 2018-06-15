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

#if NRFX_CHECK(NRFX_TIMER_ENABLED)

#if !(NRFX_CHECK(NRFX_TIMER0_ENABLED) || NRFX_CHECK(NRFX_TIMER1_ENABLED) || \
      NRFX_CHECK(NRFX_TIMER2_ENABLED) || NRFX_CHECK(NRFX_TIMER3_ENABLED) || \
      NRFX_CHECK(NRFX_TIMER4_ENABLED))
#error "No enabled TIMER instances. Check <nrfx_config.h>."
#endif

#include <nrfx_timer.h>

#define NRFX_LOG_MODULE TIMER
#include <nrfx_log.h>

/**@brief Timer control block. */
typedef struct
{
    nrfx_timer_event_handler_t handler;
    void *                     context;
    nrfx_drv_state_t           state;
} timer_control_block_t;

static timer_control_block_t m_cb[NRFX_TIMER_ENABLED_COUNT];

nrfx_err_t nrfx_timer_init(nrfx_timer_t const * const  p_instance,
                           nrfx_timer_config_t const * p_config,
                           nrfx_timer_event_handler_t  timer_event_handler)
{
    timer_control_block_t * p_cb = &m_cb[p_instance->instance_id];
#ifdef SOFTDEVICE_PRESENT
    NRFX_ASSERT(p_instance->p_reg != NRF_TIMER0);
#endif
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(timer_event_handler);

    nrfx_err_t err_code;

    if (p_cb->state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    /* Warning 685: Relational operator '<=' always evaluates to 'true'"
     * Warning in NRF_TIMER_IS_BIT_WIDTH_VALID macro. Macro validate timers resolution.
     * Not necessary in nRF52 based systems. Obligatory in nRF51 based systems.
     */

    /*lint -save -e685 */

    NRFX_ASSERT(NRF_TIMER_IS_BIT_WIDTH_VALID(p_instance->p_reg, p_config->bit_width));

    //lint -restore

    p_cb->handler = timer_event_handler;
    p_cb->context = p_config->p_context;

    uint8_t i;
    for (i = 0; i < p_instance->cc_channel_count; ++i)
    {
        nrf_timer_event_clear(p_instance->p_reg,
                              nrf_timer_compare_event_get(i));
    }

    NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(p_instance->p_reg),
        p_config->interrupt_priority);
    NRFX_IRQ_ENABLE(nrfx_get_irq_number(p_instance->p_reg));

    nrf_timer_mode_set(p_instance->p_reg, p_config->mode);
    nrf_timer_bit_width_set(p_instance->p_reg, p_config->bit_width);
    nrf_timer_frequency_set(p_instance->p_reg, p_config->frequency);

    p_cb->state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.",
                  __func__,
                  NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_timer_uninit(nrfx_timer_t const * const p_instance)
{
    NRFX_IRQ_DISABLE(nrfx_get_irq_number(p_instance->p_reg));

    #define DISABLE_ALL UINT32_MAX
    nrf_timer_shorts_disable(p_instance->p_reg, DISABLE_ALL);
    nrf_timer_int_disable(p_instance->p_reg, DISABLE_ALL);
    #undef DISABLE_ALL

    nrfx_timer_disable(p_instance);

    m_cb[p_instance->instance_id].state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Uninitialized instance: %d.", p_instance->instance_id);
}

void nrfx_timer_enable(nrfx_timer_t const * const p_instance)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state == NRFX_DRV_STATE_INITIALIZED);
    nrf_timer_task_trigger(p_instance->p_reg, NRF_TIMER_TASK_START);
    m_cb[p_instance->instance_id].state = NRFX_DRV_STATE_POWERED_ON;
    NRFX_LOG_INFO("Enabled instance: %d.", p_instance->instance_id);
}

void nrfx_timer_disable(nrfx_timer_t const * const p_instance)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);
    nrf_timer_task_trigger(p_instance->p_reg, NRF_TIMER_TASK_SHUTDOWN);
    m_cb[p_instance->instance_id].state = NRFX_DRV_STATE_INITIALIZED;
    NRFX_LOG_INFO("Disabled instance: %d.", p_instance->instance_id);
}

bool nrfx_timer_is_enabled(nrfx_timer_t const * const p_instance)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);
    return (m_cb[p_instance->instance_id].state == NRFX_DRV_STATE_POWERED_ON);
}

void nrfx_timer_resume(nrfx_timer_t const * const p_instance)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);
    nrf_timer_task_trigger(p_instance->p_reg, NRF_TIMER_TASK_START);
    NRFX_LOG_INFO("Resumed instance: %d.", p_instance->instance_id);
}

void nrfx_timer_pause(nrfx_timer_t const * const p_instance)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);
    nrf_timer_task_trigger(p_instance->p_reg, NRF_TIMER_TASK_STOP);
    NRFX_LOG_INFO("Paused instance: %d.", p_instance->instance_id);
}

void nrfx_timer_clear(nrfx_timer_t const * const p_instance)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);
    nrf_timer_task_trigger(p_instance->p_reg, NRF_TIMER_TASK_CLEAR);
}

void nrfx_timer_increment(nrfx_timer_t const * const p_instance)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(nrf_timer_mode_get(p_instance->p_reg) != NRF_TIMER_MODE_TIMER);

    nrf_timer_task_trigger(p_instance->p_reg, NRF_TIMER_TASK_COUNT);
}

uint32_t nrfx_timer_capture(nrfx_timer_t const * const p_instance,
                            nrf_timer_cc_channel_t     cc_channel)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(cc_channel < p_instance->cc_channel_count);

    nrf_timer_task_trigger(p_instance->p_reg,
        nrf_timer_capture_task_get(cc_channel));
    return nrf_timer_cc_read(p_instance->p_reg, cc_channel);
}

void nrfx_timer_compare(nrfx_timer_t const * const p_instance,
                        nrf_timer_cc_channel_t     cc_channel,
                        uint32_t                   cc_value,
                        bool                       enable_int)
{
    nrf_timer_int_mask_t timer_int = nrf_timer_compare_int_get(cc_channel);

    if (enable_int)
    {
        nrf_timer_event_clear(p_instance->p_reg, nrf_timer_compare_event_get(cc_channel));
        nrf_timer_int_enable(p_instance->p_reg, timer_int);
    }
    else
    {
        nrf_timer_int_disable(p_instance->p_reg, timer_int);
    }

    nrf_timer_cc_write(p_instance->p_reg, cc_channel, cc_value);
    NRFX_LOG_INFO("Timer id: %d, capture value set: %lu, channel: %d.",
                  p_instance->instance_id,
                  cc_value,
                  cc_channel);
}

void nrfx_timer_extended_compare(nrfx_timer_t const * const p_instance,
                                 nrf_timer_cc_channel_t     cc_channel,
                                 uint32_t                   cc_value,
                                 nrf_timer_short_mask_t     timer_short_mask,
                                 bool                       enable_int)
{
    nrf_timer_shorts_disable(p_instance->p_reg,
        (TIMER_SHORTS_COMPARE0_STOP_Msk  << cc_channel) |
        (TIMER_SHORTS_COMPARE0_CLEAR_Msk << cc_channel));

    nrf_timer_shorts_enable(p_instance->p_reg, timer_short_mask);

    nrfx_timer_compare(p_instance,
                       cc_channel,
                       cc_value,
                       enable_int);
    NRFX_LOG_INFO("Timer id: %d, capture value set: %lu, channel: %d.",
                  p_instance->instance_id,
                  cc_value,
                  cc_channel);
}

void nrfx_timer_compare_int_enable(nrfx_timer_t const * const p_instance,
                                   uint32_t                   channel)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(channel < p_instance->cc_channel_count);

    nrf_timer_event_clear(p_instance->p_reg,
        nrf_timer_compare_event_get(channel));
    nrf_timer_int_enable(p_instance->p_reg,
        nrf_timer_compare_int_get(channel));
}

void nrfx_timer_compare_int_disable(nrfx_timer_t const * const p_instance,
                                    uint32_t                   channel)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(channel < p_instance->cc_channel_count);

    nrf_timer_int_disable(p_instance->p_reg,
        nrf_timer_compare_int_get(channel));
}

static void irq_handler(NRF_TIMER_Type        * p_reg,
                        timer_control_block_t * p_cb,
                        uint8_t                 channel_count)
{
    uint8_t i;
    for (i = 0; i < channel_count; ++i)
    {
        nrf_timer_event_t event = nrf_timer_compare_event_get(i);
        nrf_timer_int_mask_t int_mask = nrf_timer_compare_int_get(i);

        if (nrf_timer_event_check(p_reg, event) &&
            nrf_timer_int_enable_check(p_reg, int_mask))
        {
            nrf_timer_event_clear(p_reg, event);
            NRFX_LOG_DEBUG("Compare event, channel: %d.", i);
            p_cb->handler(event, p_cb->context);
        }
    }
}

#if NRFX_CHECK(NRFX_TIMER0_ENABLED)
void nrfx_timer_0_irq_handler(void)
{
    irq_handler(NRF_TIMER0, &m_cb[NRFX_TIMER0_INST_IDX],
        NRF_TIMER_CC_CHANNEL_COUNT(0));
}
#endif

#if NRFX_CHECK(NRFX_TIMER1_ENABLED)
void nrfx_timer_1_irq_handler(void)
{
    irq_handler(NRF_TIMER1, &m_cb[NRFX_TIMER1_INST_IDX],
        NRF_TIMER_CC_CHANNEL_COUNT(1));
}
#endif

#if NRFX_CHECK(NRFX_TIMER2_ENABLED)
void nrfx_timer_2_irq_handler(void)
{
    irq_handler(NRF_TIMER2, &m_cb[NRFX_TIMER2_INST_IDX],
        NRF_TIMER_CC_CHANNEL_COUNT(2));
}
#endif

#if NRFX_CHECK(NRFX_TIMER3_ENABLED)
void nrfx_timer_3_irq_handler(void)
{
    irq_handler(NRF_TIMER3, &m_cb[NRFX_TIMER3_INST_IDX],
        NRF_TIMER_CC_CHANNEL_COUNT(3));
}
#endif

#if NRFX_CHECK(NRFX_TIMER4_ENABLED)
void nrfx_timer_4_irq_handler(void)
{
    irq_handler(NRF_TIMER4, &m_cb[NRFX_TIMER4_INST_IDX],
        NRF_TIMER_CC_CHANNEL_COUNT(4));
}
#endif

#endif // NRFX_CHECK(NRFX_TIMER_ENABLED)
