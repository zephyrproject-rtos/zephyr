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

#if NRFX_CHECK(NRFX_GPIOTE_ENABLED)

#include <nrfx_gpiote.h>
#include "nrf_bitmask.h"
#include <string.h>

#define NRFX_LOG_MODULE GPIOTE
#include <nrfx_log.h>


#define FORBIDDEN_HANDLER_ADDRESS ((nrfx_gpiote_evt_handler_t)UINT32_MAX)
#define PIN_NOT_USED              (-1)
#define PIN_USED                  (-2)
#define NO_CHANNELS               (-1)
#define SENSE_FIELD_POS           (6)
#define SENSE_FIELD_MASK          (0xC0)

/**
 * @brief Macro for converting task-event index to an address of an event register.
 *
 * Macro utilizes the fact that registers are grouped together in ascending order.
 */
#define TE_IDX_TO_EVENT_ADDR(idx)    (nrf_gpiote_events_t)((uint32_t)NRF_GPIOTE_EVENTS_IN_0 + \
                                                           (sizeof(uint32_t) * (idx)))

/**
 * @brief Macro for converting task-event index of OUT task to an address of a task register.
 *
 * Macro utilizes the fact that registers are grouped together in ascending order.
 */
#define TE_OUT_IDX_TO_TASK_ADDR(idx) (nrf_gpiote_tasks_t)((uint32_t)NRF_GPIOTE_TASKS_OUT_0 + \
                                                          (sizeof(uint32_t) * (idx)))

#if defined(GPIOTE_FEATURE_SET_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Macro for converting task-event index of SET task to an address of a task register.
 *
 * Macro utilizes the fact that registers are grouped together in ascending order.
 */
#define TE_SET_IDX_TO_TASK_ADDR(idx) (nrf_gpiote_tasks_t)((uint32_t)NRF_GPIOTE_TASKS_SET_0 + \
                                                          (sizeof(uint32_t) * (idx)))

#endif // defined(GPIOTE_FEATURE_SET_PRESENT) || defined(__NRFX_DOXYGEN__)

#if defined(GPIOTE_FEATURE_CLR_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Macro for converting task-event index of CLR task to an address of a task register.
 *
 * Macro utilizes the fact that registers are grouped together in ascending order.
 */
#define TE_CLR_IDX_TO_TASK_ADDR(idx) (nrf_gpiote_tasks_t)((uint32_t)NRF_GPIOTE_TASKS_CLR_0 + \
                                                          (sizeof(uint32_t) * (idx)))

#endif // defined(GPIOTE_FEATURE_CLR_PRESENT) || defined(__NRFX_DOXYGEN__)

/*lint -save -e571*/ /* Suppress "Warning 571: Suspicious cast" */
typedef struct
{
    nrfx_gpiote_evt_handler_t handlers[GPIOTE_CH_NUM + NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS];
    int8_t                    pin_assignments[NUMBER_OF_PINS];
    int8_t                    port_handlers_pins[NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS];
    nrfx_drv_state_t          state;
} gpiote_control_block_t;

static gpiote_control_block_t m_cb;

__STATIC_INLINE bool pin_in_use(uint32_t pin)
{
    return (m_cb.pin_assignments[pin] != PIN_NOT_USED);
}


__STATIC_INLINE bool pin_in_use_as_non_task_out(uint32_t pin)
{
    return (m_cb.pin_assignments[pin] == PIN_USED);
}


__STATIC_INLINE bool pin_in_use_by_te(uint32_t pin)
{
    return (m_cb.pin_assignments[pin] >= 0 && m_cb.pin_assignments[pin] < GPIOTE_CH_NUM) ?
            true : false;
}


__STATIC_INLINE bool pin_in_use_by_port(uint32_t pin)
{
    return (m_cb.pin_assignments[pin] >= GPIOTE_CH_NUM);
}


__STATIC_INLINE bool pin_in_use_by_gpiote(uint32_t pin)
{
    return (m_cb.pin_assignments[pin] >= 0);
}


__STATIC_INLINE void pin_in_use_by_te_set(uint32_t                  pin,
                                          uint32_t                  channel_id,
                                          nrfx_gpiote_evt_handler_t handler,
                                          bool                      is_channel)
{
    m_cb.pin_assignments[pin] = channel_id;
    m_cb.handlers[channel_id] = handler;
    if (!is_channel)
    {
        m_cb.port_handlers_pins[channel_id - GPIOTE_CH_NUM] = (int8_t)pin;
    }
}


__STATIC_INLINE void pin_in_use_set(uint32_t pin)
{
    m_cb.pin_assignments[pin] = PIN_USED;
}


__STATIC_INLINE void pin_in_use_clear(uint32_t pin)
{
    m_cb.pin_assignments[pin] = PIN_NOT_USED;
}


__STATIC_INLINE int8_t channel_port_get(uint32_t pin)
{
    return m_cb.pin_assignments[pin];
}


__STATIC_INLINE nrfx_gpiote_evt_handler_t channel_handler_get(uint32_t channel)
{
    return m_cb.handlers[channel];
}


static int8_t channel_port_alloc(uint32_t pin, nrfx_gpiote_evt_handler_t handler, bool channel)
{
    int8_t   channel_id = NO_CHANNELS;
    uint32_t i;

    uint32_t start_idx = channel ? 0 : GPIOTE_CH_NUM;
    uint32_t end_idx   =
        channel ? GPIOTE_CH_NUM : (GPIOTE_CH_NUM + NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS);

    // critical section

    for (i = start_idx; i < end_idx; i++)
    {
        if (m_cb.handlers[i] == FORBIDDEN_HANDLER_ADDRESS)
        {
            pin_in_use_by_te_set(pin, i, handler, channel);
            channel_id = i;
            break;
        }
    }
    // critical section
    return channel_id;
}


static void channel_free(uint8_t channel_id)
{
    m_cb.handlers[channel_id] = FORBIDDEN_HANDLER_ADDRESS;
    if (channel_id >= GPIOTE_CH_NUM)
    {
        m_cb.port_handlers_pins[channel_id - GPIOTE_CH_NUM] = (int8_t)PIN_NOT_USED;
    }
}


nrfx_err_t nrfx_gpiote_init(void)
{
    nrfx_err_t err_code;

    if (m_cb.state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    uint8_t i;

    for (i = 0; i < NUMBER_OF_PINS; i++)
    {
        pin_in_use_clear(i);
    }

    for (i = 0; i < (GPIOTE_CH_NUM + NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS); i++)
    {
        channel_free(i);
    }

    NRFX_IRQ_PRIORITY_SET(GPIOTE_IRQn, NRFX_GPIOTE_CONFIG_IRQ_PRIORITY);
    NRFX_IRQ_ENABLE(GPIOTE_IRQn);
    nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);
    nrf_gpiote_int_enable(GPIOTE_INTENSET_PORT_Msk);
    m_cb.state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


bool nrfx_gpiote_is_init(void)
{
    return (m_cb.state != NRFX_DRV_STATE_UNINITIALIZED) ? true : false;
}


void nrfx_gpiote_uninit(void)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    uint32_t i;

    for (i = 0; i < NUMBER_OF_PINS; i++)
    {
        if (pin_in_use_as_non_task_out(i))
        {
            nrfx_gpiote_out_uninit(i);
        }
        else if ( pin_in_use_by_gpiote(i))
        {
            /* Disable gpiote_in is having the same effect on out pin as gpiote_out_uninit on
             * so it can be called on all pins used by GPIOTE.
             */
            nrfx_gpiote_in_uninit(i);
        }
    }
    m_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Uninitialized.");
}


nrfx_err_t nrfx_gpiote_out_init(nrfx_gpiote_pin_t                pin,
                                nrfx_gpiote_out_config_t const * p_config)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(m_cb.state == NRFX_DRV_STATE_INITIALIZED);
    NRFX_ASSERT(p_config);

    nrfx_err_t err_code = NRFX_SUCCESS;

    if (pin_in_use(pin))
    {
        err_code = NRFX_ERROR_INVALID_STATE;
    }
    else
    {
        if (p_config->task_pin)
        {
            int8_t channel = channel_port_alloc(pin, NULL, true);

            if (channel != NO_CHANNELS)
            {
                nrf_gpiote_task_configure((uint32_t)channel,
                                          pin,
                                          p_config->action,
                                          p_config->init_state);
            }
            else
            {
                err_code = NRFX_ERROR_NO_MEM;
            }
        }
        else
        {
            pin_in_use_set(pin);
        }

        if (err_code == NRFX_SUCCESS)
        {
            if (p_config->init_state == NRF_GPIOTE_INITIAL_VALUE_HIGH)
            {
                nrf_gpio_pin_set(pin);
            }
            else
            {
                nrf_gpio_pin_clear(pin);
            }

            nrf_gpio_cfg_output(pin);
        }
    }

    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


void nrfx_gpiote_out_uninit(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use(pin));

    if (pin_in_use_by_te(pin))
    {
        channel_free((uint8_t)channel_port_get(pin));
        nrf_gpiote_te_default((uint32_t)channel_port_get(pin));
    }
    pin_in_use_clear(pin);

    nrf_gpio_cfg_default(pin);
}


void nrfx_gpiote_out_set(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use(pin));
    NRFX_ASSERT(!pin_in_use_by_te(pin))

    nrf_gpio_pin_set(pin);
}


void nrfx_gpiote_out_clear(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use(pin));
    NRFX_ASSERT(!pin_in_use_by_te(pin))

    nrf_gpio_pin_clear(pin);
}


void nrfx_gpiote_out_toggle(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use(pin));
    NRFX_ASSERT(!pin_in_use_by_te(pin))

    nrf_gpio_pin_toggle(pin);
}


void nrfx_gpiote_out_task_enable(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use(pin));
    NRFX_ASSERT(pin_in_use_by_te(pin))

    nrf_gpiote_task_enable((uint32_t)m_cb.pin_assignments[pin]);
}


void nrfx_gpiote_out_task_disable(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use(pin));
    NRFX_ASSERT(pin_in_use_by_te(pin))

    nrf_gpiote_task_disable((uint32_t)m_cb.pin_assignments[pin]);
}


uint32_t nrfx_gpiote_out_task_addr_get(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use_by_te(pin));

    nrf_gpiote_tasks_t task = TE_OUT_IDX_TO_TASK_ADDR((uint32_t)channel_port_get(pin));
    return nrf_gpiote_task_addr_get(task);
}


#if defined(GPIOTE_FEATURE_SET_PRESENT)
uint32_t nrfx_gpiote_set_task_addr_get(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use_by_te(pin));

    nrf_gpiote_tasks_t task = TE_SET_IDX_TO_TASK_ADDR((uint32_t)channel_port_get(pin));
    return nrf_gpiote_task_addr_get(task);
}


#endif // defined(GPIOTE_FEATURE_SET_PRESENT)

#if defined(GPIOTE_FEATURE_CLR_PRESENT)
uint32_t nrfx_gpiote_clr_task_addr_get(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use_by_te(pin));

    nrf_gpiote_tasks_t task = TE_CLR_IDX_TO_TASK_ADDR((uint32_t)channel_port_get(pin));
    return nrf_gpiote_task_addr_get(task);
}


#endif // defined(GPIOTE_FEATURE_CLR_PRESENT)

void nrfx_gpiote_out_task_force(nrfx_gpiote_pin_t pin, uint8_t state)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use(pin));
    NRFX_ASSERT(pin_in_use_by_te(pin));

    nrf_gpiote_outinit_t init_val =
        state ? NRF_GPIOTE_INITIAL_VALUE_HIGH : NRF_GPIOTE_INITIAL_VALUE_LOW;
    nrf_gpiote_task_force((uint32_t)m_cb.pin_assignments[pin], init_val);
}


void nrfx_gpiote_out_task_trigger(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use(pin));
    NRFX_ASSERT(pin_in_use_by_te(pin));

    nrf_gpiote_tasks_t task = TE_OUT_IDX_TO_TASK_ADDR((uint32_t)channel_port_get(pin));
    nrf_gpiote_task_set(task);
}


#if defined(GPIOTE_FEATURE_SET_PRESENT)
void nrfx_gpiote_set_task_trigger(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use(pin));
    NRFX_ASSERT(pin_in_use_by_te(pin));

    nrf_gpiote_tasks_t task = TE_SET_IDX_TO_TASK_ADDR((uint32_t)channel_port_get(pin));
    nrf_gpiote_task_set(task);
}


#endif // defined(GPIOTE_FEATURE_SET_PRESENT)

#if  defined(GPIOTE_FEATURE_CLR_PRESENT)
void nrfx_gpiote_clr_task_trigger(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use(pin));
    NRFX_ASSERT(pin_in_use_by_te(pin));

    nrf_gpiote_tasks_t task = TE_CLR_IDX_TO_TASK_ADDR((uint32_t)channel_port_get(pin));
    nrf_gpiote_task_set(task);
}


#endif // defined(GPIOTE_FEATURE_CLR_PRESENT)

nrfx_err_t nrfx_gpiote_in_init(nrfx_gpiote_pin_t               pin,
                               nrfx_gpiote_in_config_t const * p_config,
                               nrfx_gpiote_evt_handler_t       evt_handler)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    nrfx_err_t err_code = NRFX_SUCCESS;

    /* Only one GPIOTE channel can be assigned to one physical pin. */
    if (pin_in_use_by_gpiote(pin))
    {
        err_code = NRFX_ERROR_INVALID_STATE;
    }
    else
    {
        int8_t channel = channel_port_alloc(pin, evt_handler, p_config->hi_accuracy);
        if (channel != NO_CHANNELS)
        {
            if (p_config->is_watcher)
            {
                nrf_gpio_cfg_watcher(pin);
            }
            else
            {
                nrf_gpio_cfg_input(pin, p_config->pull);
            }

            if (p_config->hi_accuracy)
            {
                nrf_gpiote_event_configure((uint32_t)channel, pin, p_config->sense);
            }
            else
            {
                m_cb.port_handlers_pins[channel -
                                        GPIOTE_CH_NUM] |= (p_config->sense) << SENSE_FIELD_POS;
            }
        }
        else
        {
            err_code = NRFX_ERROR_NO_MEM;
        }
    }

    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


void nrfx_gpiote_in_event_enable(nrfx_gpiote_pin_t pin, bool int_enable)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use_by_gpiote(pin));
    if (pin_in_use_by_port(pin))
    {
        uint8_t pin_and_sense = (uint8_t)
            m_cb.port_handlers_pins[channel_port_get(pin) - GPIOTE_CH_NUM];
        nrf_gpiote_polarity_t polarity =
            (nrf_gpiote_polarity_t)(pin_and_sense >> SENSE_FIELD_POS);
        nrf_gpio_pin_sense_t sense;
        if (polarity == NRF_GPIOTE_POLARITY_TOGGLE)
        {
            /* read current pin state and set for next sense to oposit */
            sense = (nrf_gpio_pin_read(pin)) ?
                    NRF_GPIO_PIN_SENSE_LOW : NRF_GPIO_PIN_SENSE_HIGH;
        }
        else
        {
            sense = (polarity == NRF_GPIOTE_POLARITY_LOTOHI) ?
                    NRF_GPIO_PIN_SENSE_HIGH : NRF_GPIO_PIN_SENSE_LOW;
        }
        nrf_gpio_cfg_sense_set(pin, sense);
    }
    else if (pin_in_use_by_te(pin))
    {
        int32_t             channel = (int32_t)channel_port_get(pin);
        nrf_gpiote_events_t event   = TE_IDX_TO_EVENT_ADDR((uint32_t)channel);

        nrf_gpiote_event_enable((uint32_t)channel);

        nrf_gpiote_event_clear(event);
        if (int_enable)
        {
            nrfx_gpiote_evt_handler_t handler = channel_handler_get((uint32_t)channel_port_get(pin));
            // Enable the interrupt only if event handler was provided.
            if (handler)
            {
                nrf_gpiote_int_enable(1 << channel);
            }
        }
    }
}


void nrfx_gpiote_in_event_disable(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use_by_gpiote(pin));
    if (pin_in_use_by_port(pin))
    {
        nrf_gpio_cfg_sense_set(pin, NRF_GPIO_PIN_NOSENSE);
    }
    else if (pin_in_use_by_te(pin))
    {
        int32_t channel = (int32_t)channel_port_get(pin);
        nrf_gpiote_event_disable((uint32_t)channel);
        nrf_gpiote_int_disable(1 << channel);
    }
}


void nrfx_gpiote_in_uninit(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use_by_gpiote(pin));
    nrfx_gpiote_in_event_disable(pin);
    if (pin_in_use_by_te(pin))
    {
        nrf_gpiote_te_default((uint32_t)channel_port_get(pin));
    }
    nrf_gpio_cfg_default(pin);
    channel_free((uint8_t)channel_port_get(pin));
    pin_in_use_clear(pin);
}


bool nrfx_gpiote_in_is_set(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    return nrf_gpio_pin_read(pin) ? true : false;
}


uint32_t nrfx_gpiote_in_event_addr_get(nrfx_gpiote_pin_t pin)
{
    NRFX_ASSERT(pin < NUMBER_OF_PINS);
    NRFX_ASSERT(pin_in_use_by_port(pin) || pin_in_use_by_te(pin));

    nrf_gpiote_events_t event = NRF_GPIOTE_EVENTS_PORT;

    if (pin_in_use_by_te(pin))
    {
        event = TE_IDX_TO_EVENT_ADDR((uint32_t)channel_port_get(pin));
    }
    return nrf_gpiote_event_addr_get(event);
}


void nrfx_gpiote_irq_handler(void)
{
    uint32_t status            = 0;
    uint32_t input[GPIO_COUNT] = {0};

    /* collect status of all GPIOTE pin events. Processing is done once all are collected and cleared.*/
    uint32_t            i;
    nrf_gpiote_events_t event = NRF_GPIOTE_EVENTS_IN_0;
    uint32_t            mask  = (uint32_t)NRF_GPIOTE_INT_IN0_MASK;

    for (i = 0; i < GPIOTE_CH_NUM; i++)
    {
        if (nrf_gpiote_event_is_set(event) && nrf_gpiote_int_is_enabled(mask))
        {
            nrf_gpiote_event_clear(event);
            status |= mask;
        }
        mask <<= 1;
        /* Incrementing to next event, utilizing the fact that events are grouped together
         * in ascending order. */
        event = (nrf_gpiote_events_t)((uint32_t)event + sizeof(uint32_t));
    }

    /* collect PORT status event, if event is set read pins state. Processing is postponed to the
     * end of interrupt. */
    if (nrf_gpiote_event_is_set(NRF_GPIOTE_EVENTS_PORT))
    {
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);
        status |= (uint32_t)NRF_GPIOTE_INT_PORT_MASK;
        nrf_gpio_ports_read(0, GPIO_COUNT, input);
    }

    /* Process pin events. */
    if (status & NRF_GPIOTE_INT_IN_MASK)
    {
        mask = (uint32_t)NRF_GPIOTE_INT_IN0_MASK;

        for (i = 0; i < GPIOTE_CH_NUM; i++)
        {
            if (mask & status)
            {
                nrfx_gpiote_pin_t pin = nrf_gpiote_event_pin_get(i);
                NRFX_LOG_DEBUG("Event in number: %d.", i);
                nrf_gpiote_polarity_t        polarity = nrf_gpiote_event_polarity_get(i);
                nrfx_gpiote_evt_handler_t handler  = channel_handler_get(i);
                NRFX_LOG_DEBUG("Pin: %d, polarity: %d.", pin, polarity);
                if (handler)
                {
                    handler(pin, polarity);
                }
            }
            mask <<= 1;
        }
    }

    if (status & (uint32_t)NRF_GPIOTE_INT_PORT_MASK)
    {
        /* Process port event. */
        uint32_t port_idx;
        uint8_t  repeat                  = 0;
        uint32_t toggle_mask[GPIO_COUNT] = {0};
        uint32_t pins_to_check[GPIO_COUNT];

        // Faster way of doing memset because in interrupt context.
        for (port_idx = 0; port_idx < GPIO_COUNT; port_idx++)
        {
            pins_to_check[port_idx] = 0xFFFFFFFF;
        }

        do
        {
            repeat = 0;

            for (i = 0; i < NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS; i++)
            {
                uint8_t           pin_and_sense = (uint8_t)m_cb.port_handlers_pins[i];
                nrfx_gpiote_pin_t pin           = (pin_and_sense & ~SENSE_FIELD_MASK);

                if ((m_cb.port_handlers_pins[i] != PIN_NOT_USED)
                    && nrf_bitmask_bit_is_set(pin, pins_to_check))
                {
                    nrf_gpiote_polarity_t polarity =
                        (nrf_gpiote_polarity_t)((pin_and_sense &
                                                 SENSE_FIELD_MASK) >> SENSE_FIELD_POS);
                    nrfx_gpiote_evt_handler_t handler =
                        channel_handler_get((uint32_t)channel_port_get(pin));
                    if (handler || (polarity == NRF_GPIOTE_POLARITY_TOGGLE))
                    {
                        if (polarity == NRF_GPIOTE_POLARITY_TOGGLE)
                        {
                            nrf_bitmask_bit_set(pin, toggle_mask);
                        }
                        nrf_gpio_pin_sense_t sense     = nrf_gpio_pin_sense_get(pin);
                        uint32_t             pin_state = nrf_bitmask_bit_is_set(pin, input);
                        if ((pin_state && (sense == NRF_GPIO_PIN_SENSE_HIGH)) ||
                            (!pin_state && (sense == NRF_GPIO_PIN_SENSE_LOW))  )
                        {
                            NRFX_LOG_DEBUG("PORT event for pin: %d, polarity: %d.", pin, polarity);
                            if (polarity == NRF_GPIOTE_POLARITY_TOGGLE)
                            {
                                nrf_gpio_pin_sense_t next_sense =
                                    (sense == NRF_GPIO_PIN_SENSE_HIGH) ?
                                    NRF_GPIO_PIN_SENSE_LOW :
                                    NRF_GPIO_PIN_SENSE_HIGH;
                                nrf_gpio_cfg_sense_set(pin, next_sense);
                                ++repeat;

                            }
                            if (handler)
                            {
                                handler(pin, polarity);
                            }
                        }
                    }
                }
            }

            if (repeat)
            {
                // When one of the pins in low-accuracy and toggle mode becomes active,
                // it's sense mode is inverted to clear the internal SENSE signal.
                // State of any other enabled low-accuracy input in toggle mode must be checked
                // explicitly, because it does not trigger the interrput when SENSE signal is active.
                // For more information about SENSE functionality, refer to Product Specification.

                uint32_t new_input[GPIO_COUNT];
                bool     input_unchanged = true;
                nrf_gpio_ports_read(0, GPIO_COUNT, new_input);

                // Faster way of doing memcmp because in interrupt context.
                for (port_idx = 0; port_idx < GPIO_COUNT; port_idx++)
                {
                    if (new_input[port_idx] != input[port_idx])
                    {
                        input_unchanged = false;
                        break;
                    }
                }

                if (input_unchanged)
                {
                    // No change.
                    repeat = 0;
                }
                else
                {
                    // Faster way of doing memcpy because in interrupt context.
                    for (port_idx = 0; port_idx < GPIO_COUNT; port_idx++)
                    {
                        input[port_idx]         = new_input[port_idx];
                        pins_to_check[port_idx] = toggle_mask[port_idx];
                    }
                }
            }
        }
        while (repeat);
    }
}


/*lint -restore*/
#endif // NRFX_CHECK(NRFX_GPIOTE_ENABLED)
