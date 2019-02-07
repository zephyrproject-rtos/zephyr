/*
 * Copyright (c) 2015 - 2019, Nordic Semiconductor ASA
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

#if NRFX_CHECK(NRFX_PWM_ENABLED)

#if !(NRFX_CHECK(NRFX_PWM0_ENABLED) || NRFX_CHECK(NRFX_PWM1_ENABLED) || \
      NRFX_CHECK(NRFX_PWM2_ENABLED) || NRFX_CHECK(NRFX_PWM3_ENABLED))
#error "No enabled PWM instances. Check <nrfx_config.h>."
#endif

#include <nrfx_pwm.h>
#include <hal/nrf_gpio.h>

#define NRFX_LOG_MODULE PWM
#include <nrfx_log.h>

#if NRFX_CHECK(NRFX_PWM_NRF52_ANOMALY_109_WORKAROUND_ENABLED)
// The workaround uses interrupts to wake up the CPU and ensure it is active
// when PWM is about to start a DMA transfer. For initial transfer, done when
// a playback is started via PPI, a specific EGU instance is used to generate
// an interrupt. During the playback, the PWM interrupt triggered on SEQEND
// event of a preceding sequence is used to protect the transfer done for
// the next sequence to be played.
#include <hal/nrf_egu.h>
#define USE_DMA_ISSUE_WORKAROUND
#endif
#if defined(USE_DMA_ISSUE_WORKAROUND)
#define EGU_IRQn(i)         EGU_IRQn_(i)
#define EGU_IRQn_(i)        SWI##i##_EGU##i##_IRQn
#define EGU_IRQHandler(i)   EGU_IRQHandler_(i)
#define EGU_IRQHandler_(i)  nrfx_swi_##i##_irq_handler
#define DMA_ISSUE_EGU_IDX           NRFX_PWM_NRF52_ANOMALY_109_EGU_INSTANCE
#define DMA_ISSUE_EGU               NRFX_CONCAT_2(NRF_EGU, DMA_ISSUE_EGU_IDX)
#define DMA_ISSUE_EGU_IRQn          EGU_IRQn(DMA_ISSUE_EGU_IDX)
#define DMA_ISSUE_EGU_IRQHandler    EGU_IRQHandler(DMA_ISSUE_EGU_IDX)
#endif

// Control block - driver instance local data.
typedef struct
{
#if defined(USE_DMA_ISSUE_WORKAROUND)
    uint32_t                  starting_task_address;
#endif
    nrfx_pwm_handler_t        handler;
    nrfx_drv_state_t volatile state;
    uint8_t                   flags;
} pwm_control_block_t;
static pwm_control_block_t m_cb[NRFX_PWM_ENABLED_COUNT];

static void configure_pins(nrfx_pwm_t const * const p_instance,
                           nrfx_pwm_config_t const * p_config)
{
    uint32_t out_pins[NRF_PWM_CHANNEL_COUNT];
    uint8_t i;

    for (i = 0; i < NRF_PWM_CHANNEL_COUNT; ++i)
    {
        uint8_t output_pin = p_config->output_pins[i];
        if (output_pin != NRFX_PWM_PIN_NOT_USED)
        {
            bool inverted = output_pin &  NRFX_PWM_PIN_INVERTED;
            out_pins[i]   = output_pin & ~NRFX_PWM_PIN_INVERTED;

            if (inverted)
            {
                nrf_gpio_pin_set(out_pins[i]);
            }
            else
            {
                nrf_gpio_pin_clear(out_pins[i]);
            }

            nrf_gpio_cfg_output(out_pins[i]);
        }
        else
        {
            out_pins[i] = NRF_PWM_PIN_NOT_CONNECTED;
        }
    }

    nrf_pwm_pins_set(p_instance->p_registers, out_pins);
}


nrfx_err_t nrfx_pwm_init(nrfx_pwm_t const * const p_instance,
                         nrfx_pwm_config_t const * p_config,
                         nrfx_pwm_handler_t        handler)
{
    NRFX_ASSERT(p_config);

    nrfx_err_t err_code;

    pwm_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];

    if (p_cb->state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    p_cb->handler = handler;

    configure_pins(p_instance, p_config);

    nrf_pwm_enable(p_instance->p_registers);
    nrf_pwm_configure(p_instance->p_registers,
        p_config->base_clock, p_config->count_mode, p_config->top_value);
    nrf_pwm_decoder_set(p_instance->p_registers,
        p_config->load_mode, p_config->step_mode);

    nrf_pwm_shorts_set(p_instance->p_registers, 0);
    nrf_pwm_int_set(p_instance->p_registers, 0);
    nrf_pwm_event_clear(p_instance->p_registers, NRF_PWM_EVENT_LOOPSDONE);
    nrf_pwm_event_clear(p_instance->p_registers, NRF_PWM_EVENT_SEQEND0);
    nrf_pwm_event_clear(p_instance->p_registers, NRF_PWM_EVENT_SEQEND1);
    nrf_pwm_event_clear(p_instance->p_registers, NRF_PWM_EVENT_STOPPED);

    // The workaround for nRF52 Anomaly 109 "protects" DMA transfers by handling
    // interrupts generated on SEQEND0 and SEQEND1 events (this ensures that
    // the 64 MHz clock is ready when data for the next sequence to be played
    // is read). Therefore, the PWM interrupt must be enabled even if the event
    // handler is not used.
#if defined(USE_DMA_ISSUE_WORKAROUND)
    NRFX_IRQ_PRIORITY_SET(DMA_ISSUE_EGU_IRQn, p_config->irq_priority);
    NRFX_IRQ_ENABLE(DMA_ISSUE_EGU_IRQn);
#else
    if (p_cb->handler)
#endif
    {
        NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(p_instance->p_registers),
            p_config->irq_priority);
        NRFX_IRQ_ENABLE(nrfx_get_irq_number(p_instance->p_registers));
    }

    p_cb->state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


void nrfx_pwm_uninit(nrfx_pwm_t const * const p_instance)
{
    pwm_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    NRFX_IRQ_DISABLE(nrfx_get_irq_number(p_instance->p_registers));
#if defined(USE_DMA_ISSUE_WORKAROUND)
    NRFX_IRQ_DISABLE(DMA_ISSUE_EGU_IRQn);
#endif

    nrf_pwm_disable(p_instance->p_registers);

    p_cb->state = NRFX_DRV_STATE_UNINITIALIZED;
}


static uint32_t start_playback(nrfx_pwm_t const * const p_instance,
                               pwm_control_block_t * p_cb,
                               uint8_t               flags,
                               nrf_pwm_task_t        starting_task)
{
    p_cb->state = NRFX_DRV_STATE_POWERED_ON;
    p_cb->flags = flags;

    if (p_cb->handler)
    {
        // The notification about finished playback is by default enabled,
        // but this can be suppressed.
        // The notification that the peripheral has stopped is always enabled.
        uint32_t int_mask = NRF_PWM_INT_LOOPSDONE_MASK |
                            NRF_PWM_INT_STOPPED_MASK;

        // The workaround for nRF52 Anomaly 109 "protects" DMA transfers by
        // handling interrupts generated on SEQEND0 and SEQEND1 events (see
        // 'nrfx_pwm_init'), hence these events must be always enabled
        // to generate interrupts.
        // However, the user handler is called for them only when requested
        // (see 'irq_handler').
#if defined(USE_DMA_ISSUE_WORKAROUND)
        int_mask |= NRF_PWM_INT_SEQEND0_MASK | NRF_PWM_INT_SEQEND1_MASK;
#else
        if (flags & NRFX_PWM_FLAG_SIGNAL_END_SEQ0)
        {
            int_mask |= NRF_PWM_INT_SEQEND0_MASK;
        }
        if (flags & NRFX_PWM_FLAG_SIGNAL_END_SEQ1)
        {
            int_mask |= NRF_PWM_INT_SEQEND1_MASK;
        }
#endif
        if (flags & NRFX_PWM_FLAG_NO_EVT_FINISHED)
        {
            int_mask &= ~NRF_PWM_INT_LOOPSDONE_MASK;
        }

        nrf_pwm_int_set(p_instance->p_registers, int_mask);
    }
#if defined(USE_DMA_ISSUE_WORKAROUND)
    else
    {
        nrf_pwm_int_set(p_instance->p_registers,
            NRF_PWM_INT_SEQEND0_MASK | NRF_PWM_INT_SEQEND1_MASK);
    }
#endif

    nrf_pwm_event_clear(p_instance->p_registers, NRF_PWM_EVENT_STOPPED);

    if (flags & NRFX_PWM_FLAG_START_VIA_TASK)
    {
        uint32_t starting_task_address =
            nrf_pwm_task_address_get(p_instance->p_registers, starting_task);

#if defined(USE_DMA_ISSUE_WORKAROUND)
        // To "protect" the initial DMA transfer it is required to start
        // the PWM by triggering the proper task from EGU interrupt handler,
        // it is not safe to do it directly via PPI.
        p_cb->starting_task_address = starting_task_address;
        nrf_egu_int_enable(DMA_ISSUE_EGU,
            nrf_egu_int_get(DMA_ISSUE_EGU, p_instance->drv_inst_idx));
        return (uint32_t)nrf_egu_task_trigger_address_get(DMA_ISSUE_EGU,
            p_instance->drv_inst_idx);
#else
        return starting_task_address;
#endif
    }

    nrf_pwm_task_trigger(p_instance->p_registers, starting_task);
    return 0;
}


uint32_t nrfx_pwm_simple_playback(nrfx_pwm_t const * const p_instance,
                                  nrf_pwm_sequence_t const * p_sequence,
                                  uint16_t                   playback_count,
                                  uint32_t                   flags)
{
    pwm_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(playback_count > 0);
    NRFX_ASSERT(nrfx_is_in_ram(p_sequence->values.p_raw));

    // To take advantage of the looping mechanism, we need to use both sequences
    // (single sequence can be played back only once).
    nrf_pwm_sequence_set(p_instance->p_registers, 0, p_sequence);
    nrf_pwm_sequence_set(p_instance->p_registers, 1, p_sequence);
    bool odd = (playback_count & 1);
    nrf_pwm_loop_set(p_instance->p_registers,
        (playback_count / 2) + (odd ? 1 : 0));

    uint32_t shorts_mask;
    if (flags & NRFX_PWM_FLAG_STOP)
    {
        shorts_mask = NRF_PWM_SHORT_LOOPSDONE_STOP_MASK;
    }
    else if (flags & NRFX_PWM_FLAG_LOOP)
    {
        shorts_mask = odd ? NRF_PWM_SHORT_LOOPSDONE_SEQSTART1_MASK
                          : NRF_PWM_SHORT_LOOPSDONE_SEQSTART0_MASK;
    }
    else
    {
        shorts_mask = 0;
    }
    nrf_pwm_shorts_set(p_instance->p_registers, shorts_mask);

    NRFX_LOG_INFO("Function: %s, sequence length: %d.",
                  __func__,
                  p_sequence->length);
    NRFX_LOG_DEBUG("Sequence data:");
    NRFX_LOG_HEXDUMP_DEBUG((uint8_t *)p_sequence->values.p_raw,
                           p_sequence->length * sizeof(uint16_t));
    return start_playback(p_instance, p_cb, flags,
        odd ? NRF_PWM_TASK_SEQSTART1 : NRF_PWM_TASK_SEQSTART0);
}


uint32_t nrfx_pwm_complex_playback(nrfx_pwm_t const * const p_instance,
                                   nrf_pwm_sequence_t const * p_sequence_0,
                                   nrf_pwm_sequence_t const * p_sequence_1,
                                   uint16_t                   playback_count,
                                   uint32_t                   flags)
{
    pwm_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(playback_count > 0);
    NRFX_ASSERT(nrfx_is_in_ram(p_sequence_0->values.p_raw));
    NRFX_ASSERT(nrfx_is_in_ram(p_sequence_1->values.p_raw));

    nrf_pwm_sequence_set(p_instance->p_registers, 0, p_sequence_0);
    nrf_pwm_sequence_set(p_instance->p_registers, 1, p_sequence_1);
    nrf_pwm_loop_set(p_instance->p_registers, playback_count);

    uint32_t shorts_mask;
    if (flags & NRFX_PWM_FLAG_STOP)
    {
        shorts_mask = NRF_PWM_SHORT_LOOPSDONE_STOP_MASK;
    }
    else if (flags & NRFX_PWM_FLAG_LOOP)
    {
        shorts_mask = NRF_PWM_SHORT_LOOPSDONE_SEQSTART0_MASK;
    }
    else
    {
        shorts_mask = 0;
    }
    nrf_pwm_shorts_set(p_instance->p_registers, shorts_mask);

    NRFX_LOG_INFO("Function: %s, sequence 0 length: %d.",
                  __func__,
                  p_sequence_0->length);
    NRFX_LOG_INFO("Function: %s, sequence 1 length: %d.",
                  __func__,
                  p_sequence_1->length);
    NRFX_LOG_DEBUG("Sequence 0 data:");
    NRFX_LOG_HEXDUMP_DEBUG(p_sequence_0->values.p_raw,
                           p_sequence_0->length * sizeof(uint16_t));
    NRFX_LOG_DEBUG("Sequence 1 data:");
    NRFX_LOG_HEXDUMP_DEBUG(p_sequence_1->values.p_raw,
                           p_sequence_1->length * sizeof(uint16_t));
    return start_playback(p_instance, p_cb, flags, NRF_PWM_TASK_SEQSTART0);
}


bool nrfx_pwm_stop(nrfx_pwm_t const * const p_instance,
                   bool wait_until_stopped)
{
    NRFX_ASSERT(m_cb[p_instance->drv_inst_idx].state != NRFX_DRV_STATE_UNINITIALIZED);

    bool ret_val = false;

    if (nrfx_pwm_is_stopped(p_instance))
    {
        ret_val = true;
    }
    else
    {
        nrf_pwm_task_trigger(p_instance->p_registers, NRF_PWM_TASK_STOP);

        do {
            if (nrfx_pwm_is_stopped(p_instance))
            {
                ret_val = true;
                break;
            }
        } while (wait_until_stopped);
    }

    NRFX_LOG_INFO("%s returned %d.", __func__, ret_val);
    return ret_val;
}


bool nrfx_pwm_is_stopped(nrfx_pwm_t const * const p_instance)
{
    pwm_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    bool ret_val = false;

    // If the event handler is used (interrupts are enabled), the state will
    // be changed in interrupt handler when the STOPPED event occurs.
    if (p_cb->state != NRFX_DRV_STATE_POWERED_ON)
    {
        ret_val = true;
    }
    // If interrupts are disabled, we must check the STOPPED event here.
    if (nrf_pwm_event_check(p_instance->p_registers, NRF_PWM_EVENT_STOPPED))
    {
        p_cb->state = NRFX_DRV_STATE_INITIALIZED;
        NRFX_LOG_INFO("Disabled.");
        ret_val = true;
    }

    NRFX_LOG_INFO("%s returned %d.", __func__, ret_val);
    return ret_val;
}


static void irq_handler(NRF_PWM_Type * p_pwm, pwm_control_block_t * p_cb)
{
    // The user handler is called for SEQEND0 and SEQEND1 events only when the
    // user asks for it (by setting proper flags when starting the playback).
    if (nrf_pwm_event_check(p_pwm, NRF_PWM_EVENT_SEQEND0))
    {
        nrf_pwm_event_clear(p_pwm, NRF_PWM_EVENT_SEQEND0);
        if ((p_cb->flags & NRFX_PWM_FLAG_SIGNAL_END_SEQ0) && p_cb->handler)
        {
            p_cb->handler(NRFX_PWM_EVT_END_SEQ0);
        }
    }
    if (nrf_pwm_event_check(p_pwm, NRF_PWM_EVENT_SEQEND1))
    {
        nrf_pwm_event_clear(p_pwm, NRF_PWM_EVENT_SEQEND1);
        if ((p_cb->flags & NRFX_PWM_FLAG_SIGNAL_END_SEQ1) && p_cb->handler)
        {
            p_cb->handler(NRFX_PWM_EVT_END_SEQ1);
        }
    }
    // For LOOPSDONE the handler is called by default, but the user can disable
    // this (via flags).
    if (nrf_pwm_event_check(p_pwm, NRF_PWM_EVENT_LOOPSDONE))
    {
        nrf_pwm_event_clear(p_pwm, NRF_PWM_EVENT_LOOPSDONE);
        if (!(p_cb->flags & NRFX_PWM_FLAG_NO_EVT_FINISHED) && p_cb->handler)
        {
            p_cb->handler(NRFX_PWM_EVT_FINISHED);
        }
    }

    // The STOPPED event is always propagated to the user handler.
    if (nrf_pwm_event_check(p_pwm, NRF_PWM_EVENT_STOPPED))
    {
        nrf_pwm_event_clear(p_pwm, NRF_PWM_EVENT_STOPPED);

        p_cb->state = NRFX_DRV_STATE_INITIALIZED;
        if (p_cb->handler)
        {
            p_cb->handler(NRFX_PWM_EVT_STOPPED);
        }
    }
}


#if defined(USE_DMA_ISSUE_WORKAROUND)
// See 'start_playback' why this is needed.
void DMA_ISSUE_EGU_IRQHandler(void)
{
    int i;
    for (i = 0; i < NRFX_PWM_ENABLED_COUNT; ++i)
    {
        volatile uint32_t * p_event_reg =
            nrf_egu_event_triggered_address_get(DMA_ISSUE_EGU, i);
        if (*p_event_reg)
        {
            *p_event_reg = 0;
            *(volatile uint32_t *)(m_cb[i].starting_task_address) = 1;
        }
    }
}
#endif


#if NRFX_CHECK(NRFX_PWM0_ENABLED)
void nrfx_pwm_0_irq_handler(void)
{
    irq_handler(NRF_PWM0, &m_cb[NRFX_PWM0_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_PWM1_ENABLED)
void nrfx_pwm_1_irq_handler(void)
{
    irq_handler(NRF_PWM1, &m_cb[NRFX_PWM1_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_PWM2_ENABLED)
void nrfx_pwm_2_irq_handler(void)
{
    irq_handler(NRF_PWM2, &m_cb[NRFX_PWM2_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_PWM3_ENABLED)
void nrfx_pwm_3_irq_handler(void)
{
    irq_handler(NRF_PWM3, &m_cb[NRFX_PWM3_INST_IDX]);
}
#endif

#endif // NRFX_CHECK(NRFX_PWM_ENABLED)
