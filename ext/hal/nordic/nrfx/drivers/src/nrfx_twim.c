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

#if NRFX_CHECK(NRFX_TWIM_ENABLED)

#if !(NRFX_CHECK(NRFX_TWIM0_ENABLED) || NRFX_CHECK(NRFX_TWIM1_ENABLED))
#error "No enabled TWIM instances. Check <nrfx_config.h>."
#endif

#include <nrfx_twim.h>
#include <hal/nrf_gpio.h>
#include "prs/nrfx_prs.h"

#define NRFX_LOG_MODULE TWIM
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                       \
    (event == NRFX_TWIM_EVT_DONE         ? "EVT_DONE"         : \
    (event == NRFX_TWIM_EVT_ADDRESS_NACK ? "EVT_ADDRESS_NACK" : \
    (event == NRFX_TWIM_EVT_DATA_NACK    ? "EVT_DATA_NACK"    : \
                                           "UNKNOWN ERROR")))

#define EVT_TO_STR_TWIM(event)                                        \
    (event == NRF_TWIM_EVENT_STOPPED   ? "NRF_TWIM_EVENT_STOPPED"   : \
    (event == NRF_TWIM_EVENT_ERROR     ? "NRF_TWIM_EVENT_ERROR"     : \
    (event == NRF_TWIM_EVENT_SUSPENDED ? "NRF_TWIM_EVENT_SUSPENDED" : \
    (event == NRF_TWIM_EVENT_RXSTARTED ? "NRF_TWIM_EVENT_RXSTARTED" : \
    (event == NRF_TWIM_EVENT_TXSTARTED ? "NRF_TWIM_EVENT_TXSTARTED" : \
    (event == NRF_TWIM_EVENT_LASTRX    ? "NRF_TWIM_EVENT_LASTRX"    : \
    (event == NRF_TWIM_EVENT_LASTTX    ? "NRF_TWIM_EVENT_LASTTX"    : \
                                         "UNKNOWN ERROR")))))))

#define TRANSFER_TO_STR(type)                    \
    (type == NRFX_TWIM_XFER_TX   ? "XFER_TX"   : \
    (type == NRFX_TWIM_XFER_RX   ? "XFER_RX"   : \
    (type == NRFX_TWIM_XFER_TXRX ? "XFER_TXRX" : \
    (type == NRFX_TWIM_XFER_TXTX ? "XFER_TXTX" : \
                                   "UNKNOWN TRANSFER TYPE"))))

#define TWIM_PIN_INIT(_pin) nrf_gpio_cfg((_pin),                     \
                                         NRF_GPIO_PIN_DIR_INPUT,     \
                                         NRF_GPIO_PIN_INPUT_CONNECT, \
                                         NRF_GPIO_PIN_PULLUP,        \
                                         NRF_GPIO_PIN_S0D1,          \
                                         NRF_GPIO_PIN_NOSENSE)

#define TWIMX_LENGTH_VALIDATE(peripheral, drv_inst_idx, len1, len2)     \
    (((drv_inst_idx) == NRFX_CONCAT_3(NRFX_, peripheral, _INST_IDX)) && \
     NRFX_EASYDMA_LENGTH_VALIDATE(peripheral, len1, len2))

#if NRFX_CHECK(NRFX_TWIM0_ENABLED)
#define TWIM0_LENGTH_VALIDATE(...)  TWIMX_LENGTH_VALIDATE(TWIM0, __VA_ARGS__)
#else
#define TWIM0_LENGTH_VALIDATE(...)  0
#endif

#if NRFX_CHECK(NRFX_TWIM1_ENABLED)
#define TWIM1_LENGTH_VALIDATE(...)  TWIMX_LENGTH_VALIDATE(TWIM1, __VA_ARGS__)
#else
#define TWIM1_LENGTH_VALIDATE(...)  0
#endif

#define TWIM_LENGTH_VALIDATE(drv_inst_idx, len1, len2)  \
    (TWIM0_LENGTH_VALIDATE(drv_inst_idx, len1, len2) || \
     TWIM1_LENGTH_VALIDATE(drv_inst_idx, len1, len2))

// Control block - driver instance local data.
typedef struct
{
    nrfx_twim_evt_handler_t handler;
    void *                  p_context;
    volatile uint32_t       int_mask;
    nrfx_twim_xfer_desc_t   xfer_desc;
    uint32_t                flags;
    uint8_t *               p_curr_buf;
    size_t                  curr_length;
    bool                    curr_no_stop;
    nrfx_drv_state_t        state;
    bool                    error;
    volatile bool           busy;
    bool                    repeated;
    uint8_t                 bytes_transferred;
    bool                    hold_bus_uninit;
#if NRFX_CHECK(NRFX_TWIM_NRF52_ANOMALY_109_WORKAROUND_ENABLED)
    nrf_twim_frequency_t    bus_frequency;
#endif
} twim_control_block_t;

static twim_control_block_t m_cb[NRFX_TWIM_ENABLED_COUNT];

static nrfx_err_t twi_process_error(uint32_t errorsrc)
{
    nrfx_err_t ret = NRFX_ERROR_INTERNAL;

    if (errorsrc & NRF_TWIM_ERROR_ADDRESS_NACK)
    {
        ret = NRFX_ERROR_DRV_TWI_ERR_ANACK;
    }

    if (errorsrc & NRF_TWIM_ERROR_DATA_NACK)
    {
        ret = NRFX_ERROR_DRV_TWI_ERR_DNACK;
    }

    return ret;
}

nrfx_err_t nrfx_twim_init(nrfx_twim_t const *        p_instance,
                          nrfx_twim_config_t const * p_config,
                          nrfx_twim_evt_handler_t    event_handler,
                          void *                     p_context)
{
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(p_config->scl != p_config->sda);
    twim_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
    nrfx_err_t err_code;

    if (p_cb->state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

#if NRFX_CHECK(NRFX_PRS_ENABLED)
    static nrfx_irq_handler_t const irq_handlers[NRFX_TWIM_ENABLED_COUNT] = {
        #if NRFX_CHECK(NRFX_TWIM0_ENABLED)
        nrfx_twim_0_irq_handler,
        #endif
        #if NRFX_CHECK(NRFX_TWIM1_ENABLED)
        nrfx_twim_1_irq_handler,
        #endif
    };
    if (nrfx_prs_acquire(p_instance->p_twim,
            irq_handlers[p_instance->drv_inst_idx]) != NRFX_SUCCESS)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
#endif // NRFX_CHECK(NRFX_PRS_ENABLED)

    p_cb->handler         = event_handler;
    p_cb->p_context       = p_context;
    p_cb->int_mask        = 0;
    p_cb->repeated        = false;
    p_cb->busy            = false;
    p_cb->hold_bus_uninit = p_config->hold_bus_uninit;
#if NRFX_CHECK(NRFX_TWIM_NRF52_ANOMALY_109_WORKAROUND_ENABLED)
    p_cb->bus_frequency   = (nrf_twim_frequency_t)p_config->frequency;
#endif

    /* To secure correct signal levels on the pins used by the TWI
       master when the system is in OFF mode, and when the TWI master is
       disabled, these pins must be configured in the GPIO peripheral.
    */
    TWIM_PIN_INIT(p_config->scl);
    TWIM_PIN_INIT(p_config->sda);

    NRF_TWIM_Type * p_twim = p_instance->p_twim;
    nrf_twim_pins_set(p_twim, p_config->scl, p_config->sda);
    nrf_twim_frequency_set(p_twim,
        (nrf_twim_frequency_t)p_config->frequency);

    if (p_cb->handler)
    {
        NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(p_instance->p_twim),
            p_config->interrupt_priority);
        NRFX_IRQ_ENABLE(nrfx_get_irq_number(p_instance->p_twim));
    }

    p_cb->state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_twim_uninit(nrfx_twim_t const * p_instance)
{
    twim_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    if (p_cb->handler)
    {
        NRFX_IRQ_DISABLE(nrfx_get_irq_number(p_instance->p_twim));
    }
    nrfx_twim_disable(p_instance);

#if NRFX_CHECK(NRFX_PRS_ENABLED)
    nrfx_prs_release(p_instance->p_twim);
#endif

    if (!p_cb->hold_bus_uninit)
    {
        nrf_gpio_cfg_default(p_instance->p_twim->PSEL.SCL);
        nrf_gpio_cfg_default(p_instance->p_twim->PSEL.SDA);
    }

    p_cb->state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Instance uninitialized: %d.", p_instance->drv_inst_idx);
}

void nrfx_twim_enable(nrfx_twim_t const * p_instance)
{
    twim_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state == NRFX_DRV_STATE_INITIALIZED);

    nrf_twim_enable(p_instance->p_twim);

    p_cb->state = NRFX_DRV_STATE_POWERED_ON;
    NRFX_LOG_INFO("Instance enabled: %d.", p_instance->drv_inst_idx);
}

void nrfx_twim_disable(nrfx_twim_t const * p_instance)
{
    twim_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    NRF_TWIM_Type * p_twim = p_instance->p_twim;
    p_cb->int_mask = 0;
    nrf_twim_int_disable(p_twim, NRF_TWIM_ALL_INTS_MASK);
    nrf_twim_shorts_disable(p_twim, NRF_TWIM_ALL_SHORTS_MASK);
    nrf_twim_disable(p_twim);

    p_cb->state = NRFX_DRV_STATE_INITIALIZED;
    NRFX_LOG_INFO("Instance disabled: %d.", p_instance->drv_inst_idx);
}


bool nrfx_twim_is_busy(nrfx_twim_t const * p_instance)
{
    twim_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    return p_cb->busy;
}


__STATIC_INLINE void twim_list_enable_handle(NRF_TWIM_Type * p_twim, uint32_t flags)
{
    if (NRFX_TWIM_FLAG_TX_POSTINC & flags)
    {
        nrf_twim_tx_list_enable(p_twim);
    }
    else
    {
        nrf_twim_tx_list_disable(p_twim);
    }

    if (NRFX_TWIM_FLAG_RX_POSTINC & flags)
    {
        nrf_twim_rx_list_enable(p_twim);
    }
    else
    {
        nrf_twim_rx_list_disable(p_twim);
    }
}
__STATIC_INLINE nrfx_err_t twim_xfer(twim_control_block_t        * p_cb,
                                     NRF_TWIM_Type               * p_twim,
                                     nrfx_twim_xfer_desc_t const * p_xfer_desc,
                                     uint32_t                      flags)
{
    nrfx_err_t err_code = NRFX_SUCCESS;
    nrf_twim_task_t  start_task = NRF_TWIM_TASK_STARTTX;
    nrf_twim_event_t evt_to_wait = NRF_TWIM_EVENT_STOPPED;

    if (!nrfx_is_in_ram(p_xfer_desc->p_primary_buf))
    {
        err_code = NRFX_ERROR_INVALID_ADDR;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    /* Block TWI interrupts to ensure that function is not interrupted by TWI interrupt. */
    nrf_twim_int_disable(p_twim, NRF_TWIM_ALL_INTS_MASK);
    if (p_cb->busy)
    {
        nrf_twim_int_enable(p_twim, p_cb->int_mask);
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    else
    {
        p_cb->busy = ((NRFX_TWIM_FLAG_NO_XFER_EVT_HANDLER & flags) ||
                      (NRFX_TWIM_FLAG_REPEATED_XFER & flags)) ? false: true;
    }

    p_cb->xfer_desc = *p_xfer_desc;
    p_cb->repeated = (flags & NRFX_TWIM_FLAG_REPEATED_XFER) ? true : false;
    nrf_twim_address_set(p_twim, p_xfer_desc->address);

    nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_STOPPED);
    nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_ERROR);

    twim_list_enable_handle(p_twim, flags);
    switch (p_xfer_desc->type)
    {
    case NRFX_TWIM_XFER_TXTX:
        NRFX_ASSERT(!(flags & NRFX_TWIM_FLAG_REPEATED_XFER));
        NRFX_ASSERT(!(flags & NRFX_TWIM_FLAG_HOLD_XFER));
        NRFX_ASSERT(!(flags & NRFX_TWIM_FLAG_NO_XFER_EVT_HANDLER));
        if (!nrfx_is_in_ram(p_xfer_desc->p_secondary_buf))
        {
            err_code = NRFX_ERROR_INVALID_ADDR;
            NRFX_LOG_WARNING("Function: %s, error code: %s.",
                             __func__,
                             NRFX_LOG_ERROR_STRING_GET(err_code));
            return err_code;
        }
        nrf_twim_shorts_set(p_twim, NRF_TWIM_SHORT_LASTTX_SUSPEND_MASK);
        nrf_twim_tx_buffer_set(p_twim, p_xfer_desc->p_primary_buf, p_xfer_desc->primary_length);
        nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_TXSTARTED);
        nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_LASTTX);
        nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_SUSPENDED);
        nrf_twim_task_trigger(p_twim, NRF_TWIM_TASK_RESUME);
        nrf_twim_task_trigger(p_twim, NRF_TWIM_TASK_STARTTX);
        while (!nrf_twim_event_check(p_twim, NRF_TWIM_EVENT_TXSTARTED))
        {}
        NRFX_LOG_DEBUG("TWIM: Event: %s.", EVT_TO_STR_TWIM(NRF_TWIM_EVENT_TXSTARTED));
        nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_TXSTARTED);
        nrf_twim_tx_buffer_set(p_twim, p_xfer_desc->p_secondary_buf, p_xfer_desc->secondary_length);
        p_cb->int_mask = NRF_TWIM_INT_SUSPENDED_MASK | NRF_TWIM_INT_ERROR_MASK;
        break;
    case NRFX_TWIM_XFER_TXRX:
        nrf_twim_tx_buffer_set(p_twim, p_xfer_desc->p_primary_buf, p_xfer_desc->primary_length);
        if (!nrfx_is_in_ram(p_xfer_desc->p_secondary_buf))
        {
            err_code = NRFX_ERROR_INVALID_ADDR;
            NRFX_LOG_WARNING("Function: %s, error code: %s.",
                             __func__,
                             NRFX_LOG_ERROR_STRING_GET(err_code));
            return err_code;
        }
        nrf_twim_rx_buffer_set(p_twim, p_xfer_desc->p_secondary_buf, p_xfer_desc->secondary_length);
        nrf_twim_shorts_set(p_twim, NRF_TWIM_SHORT_LASTTX_STARTRX_MASK |
                                    NRF_TWIM_SHORT_LASTRX_STOP_MASK);
        p_cb->int_mask = NRF_TWIM_INT_STOPPED_MASK | NRF_TWIM_INT_ERROR_MASK;
        break;
    case NRFX_TWIM_XFER_TX:
        nrf_twim_tx_buffer_set(p_twim, p_xfer_desc->p_primary_buf, p_xfer_desc->primary_length);
        if (NRFX_TWIM_FLAG_TX_NO_STOP & flags)
        {
            nrf_twim_shorts_set(p_twim, NRF_TWIM_SHORT_LASTTX_SUSPEND_MASK);
            p_cb->int_mask = NRF_TWIM_INT_SUSPENDED_MASK | NRF_TWIM_INT_ERROR_MASK;
            nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_SUSPENDED);
            evt_to_wait = NRF_TWIM_EVENT_SUSPENDED;
        }
        else
        {
            nrf_twim_shorts_set(p_twim, NRF_TWIM_SHORT_LASTTX_STOP_MASK);
            p_cb->int_mask = NRF_TWIM_INT_STOPPED_MASK | NRF_TWIM_INT_ERROR_MASK;
        }
        nrf_twim_task_trigger(p_twim, NRF_TWIM_TASK_RESUME);
        break;
    case NRFX_TWIM_XFER_RX:
        nrf_twim_rx_buffer_set(p_twim, p_xfer_desc->p_primary_buf, p_xfer_desc->primary_length);
        nrf_twim_shorts_set(p_twim, NRF_TWIM_SHORT_LASTRX_STOP_MASK);
        p_cb->int_mask = NRF_TWIM_INT_STOPPED_MASK | NRF_TWIM_INT_ERROR_MASK;
        start_task = NRF_TWIM_TASK_STARTRX;
        nrf_twim_task_trigger(p_twim, NRF_TWIM_TASK_RESUME);
        break;
    default:
        err_code = NRFX_ERROR_INVALID_PARAM;
        break;
    }

    if (!(flags & NRFX_TWIM_FLAG_HOLD_XFER) && (p_xfer_desc->type != NRFX_TWIM_XFER_TXTX))
    {
        nrf_twim_task_trigger(p_twim, start_task);
    }

    if (p_cb->handler)
    {
        if (flags & NRFX_TWIM_FLAG_NO_XFER_EVT_HANDLER)
        {
            p_cb->int_mask = NRF_TWIM_INT_ERROR_MASK;
        }
        nrf_twim_int_enable(p_twim, p_cb->int_mask);

#if NRFX_CHECK(NRFX_TWIM_NRF52_ANOMALY_109_WORKAROUND_ENABLED)
        if ((flags & NRFX_TWIM_FLAG_HOLD_XFER) && ((p_xfer_desc->type == NRFX_TWIM_XFER_TX) ||
                                                   (p_xfer_desc->type == NRFX_TWIM_XFER_TXRX)))
        {
            p_cb->flags = flags;
            twim_list_enable_handle(p_twim, 0);
            p_twim->FREQUENCY = 0;
            nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_TXSTARTED);
            nrf_twim_int_enable(p_twim, NRF_TWIM_INT_TXSTARTED_MASK);
        }
#endif
    }
    else
    {
        while (!nrf_twim_event_check(p_twim, evt_to_wait))
        {
            if (nrf_twim_event_check(p_twim, NRF_TWIM_EVENT_ERROR))
            {
                NRFX_LOG_DEBUG("TWIM: Event: %s.", EVT_TO_STR_TWIM(NRF_TWIM_EVENT_ERROR));
                nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_ERROR);
                nrf_twim_task_trigger(p_twim, NRF_TWIM_TASK_RESUME);
                nrf_twim_task_trigger(p_twim, NRF_TWIM_TASK_STOP);
                evt_to_wait = NRF_TWIM_EVENT_STOPPED;
            }
        }

        uint32_t errorsrc =  nrf_twim_errorsrc_get_and_clear(p_twim);

        p_cb->busy = false;

        if (errorsrc)
        {
            err_code = twi_process_error(errorsrc);
        }
    }
    return err_code;
}


nrfx_err_t nrfx_twim_xfer(nrfx_twim_t           const * p_instance,
                          nrfx_twim_xfer_desc_t const * p_xfer_desc,
                          uint32_t                      flags)
{
    NRFX_ASSERT(TWIM_LENGTH_VALIDATE(p_instance->drv_inst_idx,
                                     p_xfer_desc->primary_length,
                                     p_xfer_desc->secondary_length));

    nrfx_err_t err_code = NRFX_SUCCESS;
    twim_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    // TXRX and TXTX transfers are supported only in non-blocking mode.
    NRFX_ASSERT( !((p_cb->handler == NULL) && (p_xfer_desc->type == NRFX_TWIM_XFER_TXRX)));
    NRFX_ASSERT( !((p_cb->handler == NULL) && (p_xfer_desc->type == NRFX_TWIM_XFER_TXTX)));

    NRFX_LOG_INFO("Transfer type: %s.", TRANSFER_TO_STR(p_xfer_desc->type));
    NRFX_LOG_INFO("Transfer buffers length: primary: %d, secondary: %d.",
                  p_xfer_desc->primary_length,
                  p_xfer_desc->secondary_length);
    NRFX_LOG_DEBUG("Primary buffer data:");
    NRFX_LOG_HEXDUMP_DEBUG(p_xfer_desc->p_primary_buf,
                           p_xfer_desc->primary_length * sizeof(p_xfer_desc->p_primary_buf[0]));
    NRFX_LOG_DEBUG("Secondary buffer data:");
    NRFX_LOG_HEXDUMP_DEBUG(p_xfer_desc->p_secondary_buf,
                           p_xfer_desc->secondary_length * sizeof(p_xfer_desc->p_secondary_buf[0]));

    err_code = twim_xfer(p_cb, (NRF_TWIM_Type *)p_instance->p_twim, p_xfer_desc, flags);
    NRFX_LOG_WARNING("Function: %s, error code: %s.",
                     __func__,
                     NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_twim_tx(nrfx_twim_t const * p_instance,
                        uint8_t             address,
                        uint8_t     const * p_data,
                        size_t              length,
                        bool                no_stop)
{
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(address, (uint8_t*)p_data, length);

    return nrfx_twim_xfer(p_instance, &xfer, no_stop ? NRFX_TWIM_FLAG_TX_NO_STOP : 0);
}

nrfx_err_t nrfx_twim_rx(nrfx_twim_t const * p_instance,
                        uint8_t             address,
                        uint8_t *           p_data,
                        size_t              length)
{
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_RX(address, p_data, length);
    return nrfx_twim_xfer(p_instance, &xfer, 0);
}

uint32_t nrfx_twim_start_task_get(nrfx_twim_t const * p_instance,
                                  nrfx_twim_xfer_type_t xfer_type)
{
    return (uint32_t)nrf_twim_task_address_get(p_instance->p_twim,
        (xfer_type != NRFX_TWIM_XFER_RX) ? NRF_TWIM_TASK_STARTTX : NRF_TWIM_TASK_STARTRX);
}

uint32_t nrfx_twim_stopped_event_get(nrfx_twim_t const * p_instance)
{
    return (uint32_t)nrf_twim_event_address_get(p_instance->p_twim, NRF_TWIM_EVENT_STOPPED);
}

static void twim_irq_handler(NRF_TWIM_Type * p_twim, twim_control_block_t * p_cb)
{

#if NRFX_CHECK(NRFX_TWIM_NRF52_ANOMALY_109_WORKAROUND_ENABLED)
    /* Handle only workaround case. Can be used without TWIM handler in IRQs. */
    if (nrf_twim_event_check(p_twim, NRF_TWIM_EVENT_TXSTARTED))
    {
        nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_TXSTARTED);
        nrf_twim_int_disable(p_twim, NRF_TWIM_INT_TXSTARTED_MASK);
        if (p_twim->FREQUENCY == 0)
        {
            // Set enable to zero to reset TWIM internal state.
            nrf_twim_disable(p_twim);
            nrf_twim_enable(p_twim);

            // Set proper frequency.
            nrf_twim_frequency_set(p_twim, p_cb->bus_frequency);
            twim_list_enable_handle(p_twim, p_cb->flags);

            // Start proper transmission.
            nrf_twim_task_trigger(p_twim, NRF_TWIM_TASK_STARTTX);
            return;
        }
    }
#endif

    NRFX_ASSERT(p_cb->handler);

    if (nrf_twim_event_check(p_twim, NRF_TWIM_EVENT_ERROR))
    {
        nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_ERROR);
        NRFX_LOG_DEBUG("TWIM: Event: %s.", EVT_TO_STR_TWIM(NRF_TWIM_EVENT_ERROR));
        if (!nrf_twim_event_check(p_twim, NRF_TWIM_EVENT_STOPPED))
        {
            nrf_twim_int_disable(p_twim, p_cb->int_mask);
            p_cb->int_mask = NRF_TWIM_INT_STOPPED_MASK;
            nrf_twim_int_enable(p_twim, p_cb->int_mask);

            nrf_twim_task_trigger(p_twim, NRF_TWIM_TASK_RESUME);
            nrf_twim_task_trigger(p_twim, NRF_TWIM_TASK_STOP);
            return;
        }
    }

    nrfx_twim_evt_t event;

    if (nrf_twim_event_check(p_twim, NRF_TWIM_EVENT_STOPPED))
    {
        NRFX_LOG_DEBUG("TWIM: Event: %s.", EVT_TO_STR_TWIM(NRF_TWIM_EVENT_STOPPED));
        nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_STOPPED);
        event.xfer_desc = p_cb->xfer_desc;
        if (p_cb->error)
        {

            event.xfer_desc.primary_length = (p_cb->xfer_desc.type == NRFX_TWIM_XFER_RX) ?
                nrf_twim_rxd_amount_get(p_twim) : nrf_twim_txd_amount_get(p_twim);
            event.xfer_desc.secondary_length = (p_cb->xfer_desc.type == NRFX_TWIM_XFER_TXRX) ?
                nrf_twim_rxd_amount_get(p_twim) : nrf_twim_txd_amount_get(p_twim);

        }
        nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_LASTTX);
        nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_LASTRX);
        if (!p_cb->repeated || p_cb->error)
        {
            nrf_twim_shorts_set(p_twim, 0);
            p_cb->int_mask = 0;
            nrf_twim_int_disable(p_twim, NRF_TWIM_ALL_INTS_MASK);
        }
    }
    else
    {
        nrf_twim_event_clear(p_twim, NRF_TWIM_EVENT_SUSPENDED);
        NRFX_LOG_DEBUG("TWIM: Event: %s.", EVT_TO_STR_TWIM(NRF_TWIM_EVENT_SUSPENDED));
        if (p_cb->xfer_desc.type == NRFX_TWIM_XFER_TX)
        {
            event.xfer_desc = p_cb->xfer_desc;
            if (!p_cb->repeated)
            {
                nrf_twim_shorts_set(p_twim, 0);
                p_cb->int_mask = 0;
                nrf_twim_int_disable(p_twim, NRF_TWIM_ALL_INTS_MASK);
            }
        }
        else
        {
            nrf_twim_shorts_set(p_twim, NRF_TWIM_SHORT_LASTTX_STOP_MASK);
            p_cb->int_mask = NRF_TWIM_INT_STOPPED_MASK | NRF_TWIM_INT_ERROR_MASK;
            nrf_twim_int_disable(p_twim, NRF_TWIM_ALL_INTS_MASK);
            nrf_twim_int_enable(p_twim, p_cb->int_mask);
            nrf_twim_task_trigger(p_twim, NRF_TWIM_TASK_STARTTX);
            nrf_twim_task_trigger(p_twim, NRF_TWIM_TASK_RESUME);
            return;
        }
    }

    uint32_t errorsrc = nrf_twim_errorsrc_get_and_clear(p_twim);
    if (errorsrc & NRF_TWIM_ERROR_ADDRESS_NACK)
    {
        event.type = NRFX_TWIM_EVT_ADDRESS_NACK;
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRFX_TWIM_EVT_ADDRESS_NACK));
    }
    else if (errorsrc & NRF_TWIM_ERROR_DATA_NACK)
    {
        event.type = NRFX_TWIM_EVT_DATA_NACK;
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRFX_TWIM_EVT_DATA_NACK));
    }
    else
    {
        event.type = NRFX_TWIM_EVT_DONE;
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRFX_TWIM_EVT_DONE));
    }

    if (!p_cb->repeated)
    {
        p_cb->busy = false;
    }
    p_cb->handler(&event, p_cb->p_context);
}

#if NRFX_CHECK(NRFX_TWIM0_ENABLED)
void nrfx_twim_0_irq_handler(void)
{
    twim_irq_handler(NRF_TWIM0, &m_cb[NRFX_TWIM0_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_TWIM1_ENABLED)
void nrfx_twim_1_irq_handler(void)
{
    twim_irq_handler(NRF_TWIM1, &m_cb[NRFX_TWIM1_INST_IDX]);
}
#endif

#endif // NRFX_CHECK(NRFX_TWIM_ENABLED)
