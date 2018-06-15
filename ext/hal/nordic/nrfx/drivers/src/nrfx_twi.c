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

#if NRFX_CHECK(NRFX_TWI_ENABLED)

#if !(NRFX_CHECK(NRFX_TWI0_ENABLED) || NRFX_CHECK(NRFX_TWI1_ENABLED))
#error "No enabled TWI instances. Check <nrfx_config.h>."
#endif

#include <nrfx_twi.h>
#include <hal/nrf_gpio.h>
#include "prs/nrfx_prs.h"

#define NRFX_LOG_MODULE TWI
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                      \
    (event == NRFX_TWI_EVT_DONE         ? "EVT_DONE"         : \
    (event == NRFX_TWI_EVT_ADDRESS_NACK ? "EVT_ADDRESS_NACK" : \
    (event == NRFX_TWI_EVT_DATA_NACK    ? "EVT_DATA_NACK"    : \
                                          "UNKNOWN ERROR")))

#define EVT_TO_STR_TWI(event)                                       \
    (event == NRF_TWI_EVENT_STOPPED   ? "NRF_TWI_EVENT_STOPPED"   : \
    (event == NRF_TWI_EVENT_RXDREADY  ? "NRF_TWI_EVENT_RXDREADY"  : \
    (event == NRF_TWI_EVENT_TXDSENT   ? "NRF_TWI_EVENT_TXDSENT"   : \
    (event == NRF_TWI_EVENT_ERROR     ? "NRF_TWI_EVENT_ERROR"     : \
    (event == NRF_TWI_EVENT_BB        ? "NRF_TWI_EVENT_BB"        : \
    (event == NRF_TWI_EVENT_SUSPENDED ? "NRF_TWI_EVENT_SUSPENDED" : \
                                        "UNKNOWN ERROR"))))))

#define TRANSFER_TO_STR(type)                   \
    (type == NRFX_TWI_XFER_TX   ? "XFER_TX"   : \
    (type == NRFX_TWI_XFER_RX   ? "XFER_RX"   : \
    (type == NRFX_TWI_XFER_TXRX ? "XFER_TXRX" : \
    (type == NRFX_TWI_XFER_TXTX ? "XFER_TXTX" : \
                                  "UNKNOWN TRANSFER TYPE"))))

#define TWI_PIN_INIT(_pin) nrf_gpio_cfg((_pin),                     \
                                        NRF_GPIO_PIN_DIR_INPUT,     \
                                        NRF_GPIO_PIN_INPUT_CONNECT, \
                                        NRF_GPIO_PIN_PULLUP,        \
                                        NRF_GPIO_PIN_S0D1,          \
                                        NRF_GPIO_PIN_NOSENSE)

#define HW_TIMEOUT      100000

// Control block - driver instance local data.
typedef struct
{
    nrfx_twi_evt_handler_t  handler;
    void *                  p_context;
    volatile uint32_t       int_mask;
    nrfx_twi_xfer_desc_t    xfer_desc;
    uint32_t                flags;
    uint8_t *               p_curr_buf;
    size_t                  curr_length;
    bool                    curr_no_stop;
    nrfx_drv_state_t        state;
    bool                    error;
    volatile bool           busy;
    bool                    repeated;
    size_t                  bytes_transferred;
    bool                    hold_bus_uninit;
} twi_control_block_t;

static twi_control_block_t m_cb[NRFX_TWI_ENABLED_COUNT];

static nrfx_err_t twi_process_error(uint32_t errorsrc)
{
    nrfx_err_t ret = NRFX_ERROR_INTERNAL;

    if (errorsrc & NRF_TWI_ERROR_OVERRUN)
    {
        ret = NRFX_ERROR_DRV_TWI_ERR_OVERRUN;
    }

    if (errorsrc & NRF_TWI_ERROR_ADDRESS_NACK)
    {
        ret = NRFX_ERROR_DRV_TWI_ERR_ANACK;
    }

    if (errorsrc & NRF_TWI_ERROR_DATA_NACK)
    {
        ret = NRFX_ERROR_DRV_TWI_ERR_DNACK;
    }

    return ret;
}



nrfx_err_t nrfx_twi_init(nrfx_twi_t const *        p_instance,
                         nrfx_twi_config_t const * p_config,
                         nrfx_twi_evt_handler_t    event_handler,
                         void *                    p_context)
{
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(p_config->scl != p_config->sda);
    twi_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
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
    static nrfx_irq_handler_t const irq_handlers[NRFX_TWI_ENABLED_COUNT] = {
        #if NRFX_CHECK(NRFX_TWI0_ENABLED)
        nrfx_twi_0_irq_handler,
        #endif
        #if NRFX_CHECK(NRFX_TWI1_ENABLED)
        nrfx_twi_1_irq_handler,
        #endif
    };
    if (nrfx_prs_acquire(p_instance->p_twi,
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

    /* To secure correct signal levels on the pins used by the TWI
       master when the system is in OFF mode, and when the TWI master is
       disabled, these pins must be configured in the GPIO peripheral.
    */
    TWI_PIN_INIT(p_config->scl);
    TWI_PIN_INIT(p_config->sda);

    NRF_TWI_Type * p_twi = p_instance->p_twi;
    nrf_twi_pins_set(p_twi, p_config->scl, p_config->sda);
    nrf_twi_frequency_set(p_twi,
        (nrf_twi_frequency_t)p_config->frequency);

    if (p_cb->handler)
    {
        NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(p_instance->p_twi),
            p_config->interrupt_priority);
        NRFX_IRQ_ENABLE(nrfx_get_irq_number(p_instance->p_twi));
    }

    p_cb->state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_twi_uninit(nrfx_twi_t const * p_instance)
{
    twi_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    if (p_cb->handler)
    {
        NRFX_IRQ_DISABLE(nrfx_get_irq_number(p_instance->p_twi));
    }
    nrfx_twi_disable(p_instance);

#if NRFX_CHECK(NRFX_PRS_ENABLED)
    nrfx_prs_release(p_instance->p_twi);
#endif

    if (!p_cb->hold_bus_uninit)
    {
        nrf_gpio_cfg_default(p_instance->p_twi->PSELSCL);
        nrf_gpio_cfg_default(p_instance->p_twi->PSELSDA);
    }

    p_cb->state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Instance uninitialized: %d.", p_instance->drv_inst_idx);
}

void nrfx_twi_enable(nrfx_twi_t const * p_instance)
{
    twi_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state == NRFX_DRV_STATE_INITIALIZED);

    NRF_TWI_Type * p_twi = p_instance->p_twi;
    nrf_twi_enable(p_twi);

    p_cb->state = NRFX_DRV_STATE_POWERED_ON;
    NRFX_LOG_INFO("Instance enabled: %d.", p_instance->drv_inst_idx);
}

void nrfx_twi_disable(nrfx_twi_t const * p_instance)
{
    twi_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    NRF_TWI_Type * p_twi = p_instance->p_twi;
    nrf_twi_int_disable(p_twi, NRF_TWI_ALL_INTS_MASK);
    nrf_twi_shorts_disable(p_twi, NRF_TWI_ALL_SHORTS_MASK);
    nrf_twi_disable(p_twi);

    p_cb->state = NRFX_DRV_STATE_INITIALIZED;
    NRFX_LOG_INFO("Instance disabled: %d.", p_instance->drv_inst_idx);
}

static bool twi_send_byte(NRF_TWI_Type  * p_twi,
                          uint8_t const * p_data,
                          size_t          length,
                          size_t        * p_bytes_transferred,
                          bool            no_stop)
{
    if (*p_bytes_transferred < length)
    {
        nrf_twi_txd_set(p_twi, p_data[*p_bytes_transferred]);
        ++(*p_bytes_transferred);
    }
    else
    {
        if (no_stop)
        {
            nrf_twi_task_trigger(p_twi, NRF_TWI_TASK_SUSPEND);
            return false;
        }
        else
        {
            nrf_twi_task_trigger(p_twi, NRF_TWI_TASK_STOP);
        }
    }
    return true;
}

static void twi_receive_byte(NRF_TWI_Type * p_twi,
                             uint8_t      * p_data,
                             size_t         length,
                             size_t       * p_bytes_transferred)
{
    if (*p_bytes_transferred < length)
    {
        p_data[*p_bytes_transferred] = nrf_twi_rxd_get(p_twi);

        ++(*p_bytes_transferred);

        if (*p_bytes_transferred == length - 1)
        {
            nrf_twi_shorts_set(p_twi, NRF_TWI_SHORT_BB_STOP_MASK);
        }
        else if (*p_bytes_transferred == length)
        {
            return;
        }

        nrf_twi_task_trigger(p_twi, NRF_TWI_TASK_RESUME);
    }
}

static bool twi_transfer(NRF_TWI_Type  * p_twi,
                         bool          * p_error,
                         size_t        * p_bytes_transferred,
                         uint8_t       * p_data,
                         size_t          length,
                         bool            no_stop)
{
    bool do_stop_check = ((*p_error) || ((*p_bytes_transferred) == length));

    if (*p_error)
    {
        nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_ERROR);
        nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_TXDSENT);
        nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_RXDREADY);
    }
    else if (nrf_twi_event_check(p_twi, NRF_TWI_EVENT_ERROR))
    {
        nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_ERROR);
        NRFX_LOG_DEBUG("TWI: Event: %s.", EVT_TO_STR_TWI(NRF_TWI_EVENT_ERROR));
        nrf_twi_task_trigger(p_twi, NRF_TWI_TASK_STOP);
        *p_error = true;
    }
    else
    {
        if (nrf_twi_event_check(p_twi, NRF_TWI_EVENT_TXDSENT))
        {
            nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_TXDSENT);
            NRFX_LOG_DEBUG("TWI: Event: %s.", EVT_TO_STR_TWI(NRF_TWI_EVENT_TXDSENT));
            if (nrf_twi_event_check(p_twi, NRF_TWI_EVENT_ERROR))
            {
                nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_ERROR);
                NRFX_LOG_DEBUG("TWI: Event: %s.", EVT_TO_STR_TWI(NRF_TWI_EVENT_ERROR));
                nrf_twi_task_trigger(p_twi, NRF_TWI_TASK_STOP);
                *p_error = true;
            }
            else
            {
                if (!twi_send_byte(p_twi, p_data, length, p_bytes_transferred, no_stop))
                {
                    return false;
                }
            }
        }
        else if (nrf_twi_event_check(p_twi, NRF_TWI_EVENT_RXDREADY))
        {
            nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_RXDREADY);
            NRFX_LOG_DEBUG("TWI: Event: %s.", EVT_TO_STR_TWI(NRF_TWI_EVENT_RXDREADY));
            if (nrf_twi_event_check(p_twi, NRF_TWI_EVENT_ERROR))
            {
                NRFX_LOG_DEBUG("TWI: Event: %s.", EVT_TO_STR_TWI(NRF_TWI_EVENT_ERROR));
                nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_ERROR);
                nrf_twi_task_trigger(p_twi, NRF_TWI_TASK_STOP);
                *p_error = true;
            }
            else
            {
                twi_receive_byte(p_twi, p_data, length, p_bytes_transferred);
            }
        }
    }

    if (do_stop_check && nrf_twi_event_check(p_twi, NRF_TWI_EVENT_STOPPED))
    {
        nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_STOPPED);
        NRFX_LOG_DEBUG("TWI: Event: %s.", EVT_TO_STR_TWI(NRF_TWI_EVENT_STOPPED));
        return false;
    }

    return true;
}

static nrfx_err_t twi_tx_start_transfer(twi_control_block_t * p_cb,
                                        NRF_TWI_Type *        p_twi,
                                        uint8_t const *       p_data,
                                        size_t                length,
                                        bool                  no_stop)
{
    nrfx_err_t ret_code = NRFX_SUCCESS;
    volatile int32_t hw_timeout;

    hw_timeout = HW_TIMEOUT;

    nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_STOPPED);
    nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_ERROR);
    nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_TXDSENT);
    nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_RXDREADY);
    nrf_twi_shorts_set(p_twi, 0);

    p_cb->bytes_transferred = 0;
    p_cb->error             = false;

    // In case TWI is suspended resume its operation.
    nrf_twi_task_trigger(p_twi, NRF_TWI_TASK_RESUME);
    nrf_twi_task_trigger(p_twi, NRF_TWI_TASK_STARTTX);

    (void)twi_send_byte(p_twi, p_data, length, &p_cb->bytes_transferred, no_stop);

    if (p_cb->handler)
    {
        p_cb->int_mask = NRF_TWI_INT_STOPPED_MASK   |
                         NRF_TWI_INT_ERROR_MASK     |
                         NRF_TWI_INT_TXDSENT_MASK   |
                         NRF_TWI_INT_RXDREADY_MASK;
        nrf_twi_int_enable(p_twi, p_cb->int_mask);
    }
    else
    {
        while ((hw_timeout > 0) &&
               twi_transfer(p_twi,
                            &p_cb->error,
                            &p_cb->bytes_transferred,
                            (uint8_t *)p_data,
                            length,
                            no_stop))
        {
            hw_timeout--;
        }

        if (p_cb->error)
        {
            uint32_t errorsrc =  nrf_twi_errorsrc_get_and_clear(p_twi);

            if (errorsrc)
            {
                ret_code = twi_process_error(errorsrc);
            }
        }

        if (hw_timeout <= 0)
        {
            nrf_twi_disable(p_twi);
            nrf_twi_enable(p_twi);
            ret_code = NRFX_ERROR_INTERNAL;
        }

    }
    return ret_code;
}

static nrfx_err_t twi_rx_start_transfer(twi_control_block_t * p_cb,
                                        NRF_TWI_Type *        p_twi,
                                        uint8_t const *       p_data,
                                        size_t                length)
{
    nrfx_err_t ret_code = NRFX_SUCCESS;
    volatile int32_t hw_timeout;

    hw_timeout = HW_TIMEOUT;

    nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_STOPPED);
    nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_ERROR);
    nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_TXDSENT);
    nrf_twi_event_clear(p_twi, NRF_TWI_EVENT_RXDREADY);

    p_cb->bytes_transferred = 0;
    p_cb->error             = false;

    if (length == 1)
    {
        nrf_twi_shorts_set(p_twi, NRF_TWI_SHORT_BB_STOP_MASK);
    }
    else
    {
        nrf_twi_shorts_set(p_twi, NRF_TWI_SHORT_BB_SUSPEND_MASK);
    }
    // In case TWI is suspended resume its operation.
    nrf_twi_task_trigger(p_twi, NRF_TWI_TASK_RESUME);
    nrf_twi_task_trigger(p_twi, NRF_TWI_TASK_STARTRX);

    if (p_cb->handler)
    {
        p_cb->int_mask = NRF_TWI_INT_STOPPED_MASK   |
                        NRF_TWI_INT_ERROR_MASK     |
                        NRF_TWI_INT_TXDSENT_MASK   |
                        NRF_TWI_INT_RXDREADY_MASK;
        nrf_twi_int_enable(p_twi, p_cb->int_mask);
    }
    else
    {
        while ((hw_timeout > 0) &&
               twi_transfer(p_twi,
                            &p_cb->error,
                            &p_cb->bytes_transferred,
                            (uint8_t*)p_data,
                            length,
                            false))
        {
               hw_timeout--;
        }

        if (p_cb->error)
        {
            uint32_t errorsrc =  nrf_twi_errorsrc_get_and_clear(p_twi);

            if (errorsrc)
            {
                ret_code = twi_process_error(errorsrc);
            }
        }
        if (hw_timeout <= 0)
        {
            nrf_twi_disable(p_twi);
            nrf_twi_enable(p_twi);
            ret_code = NRFX_ERROR_INTERNAL;
        }
    }
    return ret_code;
}

__STATIC_INLINE nrfx_err_t twi_xfer(twi_control_block_t        * p_cb,
                                    NRF_TWI_Type               * p_twi,
                                    nrfx_twi_xfer_desc_t const * p_xfer_desc,
                                    uint32_t                     flags)
{

    nrfx_err_t err_code = NRFX_SUCCESS;

    /* Block TWI interrupts to ensure that function is not interrupted by TWI interrupt. */
    nrf_twi_int_disable(p_twi, NRF_TWI_ALL_INTS_MASK);

    if (p_cb->busy)
    {
        nrf_twi_int_enable(p_twi, p_cb->int_mask);
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    else
    {
        p_cb->busy = (NRFX_TWI_FLAG_NO_XFER_EVT_HANDLER & flags) ? false : true;
    }

    p_cb->flags       = flags;
    p_cb->xfer_desc   = *p_xfer_desc;
    p_cb->curr_length = p_xfer_desc->primary_length;
    p_cb->p_curr_buf  = p_xfer_desc->p_primary_buf;
    nrf_twi_address_set(p_twi, p_xfer_desc->address);

    if (p_xfer_desc->type != NRFX_TWI_XFER_RX)
    {
        p_cb->curr_no_stop = ((p_xfer_desc->type == NRFX_TWI_XFER_TX) &&
                             !(flags & NRFX_TWI_FLAG_TX_NO_STOP)) ? false : true;

        err_code = twi_tx_start_transfer(p_cb,
                                         p_twi,
                                         p_xfer_desc->p_primary_buf,
                                         p_xfer_desc->primary_length,
                                         p_cb->curr_no_stop);
    }
    else
    {
        p_cb->curr_no_stop = false;

        err_code = twi_rx_start_transfer(p_cb,
                                         p_twi,
                                         p_xfer_desc->p_primary_buf,
                                         p_xfer_desc->primary_length);
    }
    if (p_cb->handler == NULL)
    {
        p_cb->busy = false;
    }
    return err_code;
}

bool nrfx_twi_is_busy(nrfx_twi_t const * p_instance)
{
    twi_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    return p_cb->busy;
}

nrfx_err_t nrfx_twi_xfer(nrfx_twi_t           const * p_instance,
                         nrfx_twi_xfer_desc_t const * p_xfer_desc,
                         uint32_t                     flags)
{

    nrfx_err_t err_code = NRFX_SUCCESS;
    twi_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    // TXRX and TXTX transfers are supported only in non-blocking mode.
    NRFX_ASSERT( !((p_cb->handler == NULL) && (p_xfer_desc->type == NRFX_TWI_XFER_TXRX)));
    NRFX_ASSERT( !((p_cb->handler == NULL) && (p_xfer_desc->type == NRFX_TWI_XFER_TXTX)));

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

    err_code = twi_xfer(p_cb, (NRF_TWI_Type  *)p_instance->p_twi, p_xfer_desc, flags);
    NRFX_LOG_WARNING("Function: %s, error code: %s.",
                     __func__,
                     NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_twi_tx(nrfx_twi_t const * p_instance,
                       uint8_t            address,
                       uint8_t    const * p_data,
                       size_t             length,
                       bool               no_stop)
{
    nrfx_twi_xfer_desc_t xfer = NRFX_TWI_XFER_DESC_TX(address, (uint8_t*)p_data, length);

    return nrfx_twi_xfer(p_instance, &xfer, no_stop ? NRFX_TWI_FLAG_TX_NO_STOP : 0);
}

nrfx_err_t nrfx_twi_rx(nrfx_twi_t const * p_instance,
                       uint8_t            address,
                       uint8_t *          p_data,
                       size_t             length)
{
    nrfx_twi_xfer_desc_t xfer = NRFX_TWI_XFER_DESC_RX(address, p_data, length);
    return nrfx_twi_xfer(p_instance, &xfer, 0);
}

size_t nrfx_twi_data_count_get(nrfx_twi_t const * const p_instance)
{
    return m_cb[p_instance->drv_inst_idx].bytes_transferred;
}

uint32_t nrfx_twi_stopped_event_get(nrfx_twi_t const * p_instance)
{
    return (uint32_t)nrf_twi_event_address_get(p_instance->p_twi, NRF_TWI_EVENT_STOPPED);
}

static void twi_irq_handler(NRF_TWI_Type * p_twi, twi_control_block_t * p_cb)
{
    NRFX_ASSERT(p_cb->handler);

    if (twi_transfer(p_twi,
                     &p_cb->error,
                     &p_cb->bytes_transferred,
                     p_cb->p_curr_buf,
                     p_cb->curr_length,
                     p_cb->curr_no_stop ))
    {
        return;
    }

    if (!p_cb->error &&
        ((p_cb->xfer_desc.type == NRFX_TWI_XFER_TXRX) ||
         (p_cb->xfer_desc.type == NRFX_TWI_XFER_TXTX)) &&
        p_cb->p_curr_buf == p_cb->xfer_desc.p_primary_buf)
    {
        p_cb->p_curr_buf   = p_cb->xfer_desc.p_secondary_buf;
        p_cb->curr_length  = p_cb->xfer_desc.secondary_length;
        p_cb->curr_no_stop = (p_cb->flags & NRFX_TWI_FLAG_TX_NO_STOP);

        if (p_cb->xfer_desc.type == NRFX_TWI_XFER_TXTX)
        {
            (void)twi_tx_start_transfer(p_cb,
                                        p_twi,
                                        p_cb->p_curr_buf,
                                        p_cb->curr_length,
                                        p_cb->curr_no_stop);
        }
        else
        {
            (void)twi_rx_start_transfer(p_cb, p_twi, p_cb->p_curr_buf, p_cb->curr_length);
        }
    }
    else
    {
        nrfx_twi_evt_t event;
        event.xfer_desc = p_cb->xfer_desc;

        if (p_cb->error)
        {
            uint32_t errorsrc = nrf_twi_errorsrc_get_and_clear(p_twi);
            if (errorsrc & NRF_TWI_ERROR_ADDRESS_NACK)
            {
                event.type = NRFX_TWI_EVT_ADDRESS_NACK;
                NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRFX_TWI_EVT_ADDRESS_NACK));
            }
            else if (errorsrc & NRF_TWI_ERROR_DATA_NACK)
            {
                event.type = NRFX_TWI_EVT_DATA_NACK;
                NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRFX_TWI_EVT_DATA_NACK));
            }
        }
        else
        {
            event.type = NRFX_TWI_EVT_DONE;
            NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRFX_TWI_EVT_DONE));
        }

        p_cb->busy = false;

        if (!(NRFX_TWI_FLAG_NO_XFER_EVT_HANDLER & p_cb->flags))
        {
            p_cb->handler(&event, p_cb->p_context);
        }
    }

}

#if NRFX_CHECK(NRFX_TWI0_ENABLED)
void nrfx_twi_0_irq_handler(void)
{
    twi_irq_handler(NRF_TWI0, &m_cb[NRFX_TWI0_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_TWI1_ENABLED)
void nrfx_twi_1_irq_handler(void)
{
    twi_irq_handler(NRF_TWI1, &m_cb[NRFX_TWI1_INST_IDX]);
}
#endif

#endif // NRFX_CHECK(NRFX_TWI_ENABLED)
