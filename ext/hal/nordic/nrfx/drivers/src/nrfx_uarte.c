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

#if NRFX_CHECK(NRFX_UARTE_ENABLED)

#if !(NRFX_CHECK(NRFX_UARTE0_ENABLED) || \
      NRFX_CHECK(NRFX_UARTE1_ENABLED) || \
      NRFX_CHECK(NRFX_UARTE2_ENABLED) || \
      NRFX_CHECK(NRFX_UARTE3_ENABLED))
#error "No enabled UARTE instances. Check <nrfx_config.h>."
#endif

#include <nrfx_uarte.h>
#include "prs/nrfx_prs.h"
#include <hal/nrf_gpio.h>

#define NRFX_LOG_MODULE UARTE
#include <nrfx_log.h>

#define EVT_TO_STR(event) \
    (event == NRF_UARTE_EVENT_ERROR ? "NRF_UARTE_EVENT_ERROR" : \
                                      "UNKNOWN EVENT")

#define UARTEX_LENGTH_VALIDATE(peripheral, drv_inst_idx, len1, len2)     \
    (((drv_inst_idx) == NRFX_CONCAT_3(NRFX_, peripheral, _INST_IDX)) && \
     NRFX_EASYDMA_LENGTH_VALIDATE(peripheral, len1, len2))

#if NRFX_CHECK(NRFX_UARTE0_ENABLED)
#define UARTE0_LENGTH_VALIDATE(...)  UARTEX_LENGTH_VALIDATE(UARTE0, __VA_ARGS__)
#else
#define UARTE0_LENGTH_VALIDATE(...)  0
#endif

#if NRFX_CHECK(NRFX_UARTE1_ENABLED)
#define UARTE1_LENGTH_VALIDATE(...)  UARTEX_LENGTH_VALIDATE(UARTE1, __VA_ARGS__)
#else
#define UARTE1_LENGTH_VALIDATE(...)  0
#endif

#if NRFX_CHECK(NRFX_UARTE2_ENABLED)
#define UARTE2_LENGTH_VALIDATE(...)  UARTEX_LENGTH_VALIDATE(UARTE2, __VA_ARGS__)
#else
#define UARTE2_LENGTH_VALIDATE(...)  0
#endif

#if NRFX_CHECK(NRFX_UARTE3_ENABLED)
#define UARTE3_LENGTH_VALIDATE(...)  UARTEX_LENGTH_VALIDATE(UARTE3, __VA_ARGS__)
#else
#define UARTE3_LENGTH_VALIDATE(...)  0
#endif

#define UARTE_LENGTH_VALIDATE(drv_inst_idx, length)     \
    (UARTE0_LENGTH_VALIDATE(drv_inst_idx, length, 0) || \
     UARTE1_LENGTH_VALIDATE(drv_inst_idx, length, 0) || \
     UARTE2_LENGTH_VALIDATE(drv_inst_idx, length, 0) || \
     UARTE3_LENGTH_VALIDATE(drv_inst_idx, length, 0))


typedef struct
{
    void                     * p_context;
    nrfx_uarte_event_handler_t handler;
    uint8_t            const * p_tx_buffer;
    uint8_t                  * p_rx_buffer;
    uint8_t                  * p_rx_secondary_buffer;
    volatile size_t            tx_buffer_length;
    size_t                     rx_buffer_length;
    size_t                     rx_secondary_buffer_length;
    nrfx_drv_state_t           state;
} uarte_control_block_t;
static uarte_control_block_t m_cb[NRFX_UARTE_ENABLED_COUNT];

static void apply_config(nrfx_uarte_t        const * p_instance,
                         nrfx_uarte_config_t const * p_config)
{
    if (p_config->pseltxd != NRF_UARTE_PSEL_DISCONNECTED)
    {
        nrf_gpio_pin_set(p_config->pseltxd);
        nrf_gpio_cfg_output(p_config->pseltxd);
    }
    if (p_config->pselrxd != NRF_UARTE_PSEL_DISCONNECTED)
    {
        nrf_gpio_cfg_input(p_config->pselrxd, NRF_GPIO_PIN_NOPULL);
    }

    nrf_uarte_baudrate_set(p_instance->p_reg, p_config->baudrate);
    nrf_uarte_configure(p_instance->p_reg, p_config->parity, p_config->hwfc);
    nrf_uarte_txrx_pins_set(p_instance->p_reg, p_config->pseltxd, p_config->pselrxd);
    if (p_config->hwfc == NRF_UARTE_HWFC_ENABLED)
    {
        if (p_config->pselcts != NRF_UARTE_PSEL_DISCONNECTED)
        {
            nrf_gpio_cfg_input(p_config->pselcts, NRF_GPIO_PIN_NOPULL);
        }
        if (p_config->pselrts != NRF_UARTE_PSEL_DISCONNECTED)
        {
            nrf_gpio_pin_set(p_config->pselrts);
            nrf_gpio_cfg_output(p_config->pselrts);
        }
        nrf_uarte_hwfc_pins_set(p_instance->p_reg, p_config->pselrts, p_config->pselcts);
    }
}

static void interrupts_enable(nrfx_uarte_t const * p_instance,
                              uint8_t              interrupt_priority)
{
    nrf_uarte_event_clear(p_instance->p_reg, NRF_UARTE_EVENT_ENDRX);
    nrf_uarte_event_clear(p_instance->p_reg, NRF_UARTE_EVENT_ENDTX);
    nrf_uarte_event_clear(p_instance->p_reg, NRF_UARTE_EVENT_ERROR);
    nrf_uarte_event_clear(p_instance->p_reg, NRF_UARTE_EVENT_RXTO);
    nrf_uarte_int_enable(p_instance->p_reg, NRF_UARTE_INT_ENDRX_MASK |
                                            NRF_UARTE_INT_ENDTX_MASK |
                                            NRF_UARTE_INT_ERROR_MASK |
                                            NRF_UARTE_INT_RXTO_MASK);
    NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number((void *)p_instance->p_reg),
                          interrupt_priority);
    NRFX_IRQ_ENABLE(nrfx_get_irq_number((void *)p_instance->p_reg));
}

static void interrupts_disable(nrfx_uarte_t const * p_instance)
{
    nrf_uarte_int_disable(p_instance->p_reg, NRF_UARTE_INT_ENDRX_MASK |
                                             NRF_UARTE_INT_ENDTX_MASK |
                                             NRF_UARTE_INT_ERROR_MASK |
                                             NRF_UARTE_INT_RXTO_MASK);
    NRFX_IRQ_DISABLE(nrfx_get_irq_number((void *)p_instance->p_reg));
}

static void pins_to_default(nrfx_uarte_t const * p_instance)
{
    /* Reset pins to default states */
    uint32_t txd;
    uint32_t rxd;
    uint32_t rts;
    uint32_t cts;

    txd = nrf_uarte_tx_pin_get(p_instance->p_reg);
    rxd = nrf_uarte_rx_pin_get(p_instance->p_reg);
    rts = nrf_uarte_rts_pin_get(p_instance->p_reg);
    cts = nrf_uarte_cts_pin_get(p_instance->p_reg);
    nrf_uarte_txrx_pins_disconnect(p_instance->p_reg);
    nrf_uarte_hwfc_pins_disconnect(p_instance->p_reg);

    if (txd != NRF_UARTE_PSEL_DISCONNECTED)
    {
        nrf_gpio_cfg_default(txd);
    }
    if (rxd != NRF_UARTE_PSEL_DISCONNECTED)
    {
        nrf_gpio_cfg_default(rxd);
    }
    if (cts != NRF_UARTE_PSEL_DISCONNECTED)
    {
        nrf_gpio_cfg_default(cts);
    }
    if (rts != NRF_UARTE_PSEL_DISCONNECTED)
    {
        nrf_gpio_cfg_default(rts);
    }
}

nrfx_err_t nrfx_uarte_init(nrfx_uarte_t const *        p_instance,
                           nrfx_uarte_config_t const * p_config,
                           nrfx_uarte_event_handler_t  event_handler)
{
    NRFX_ASSERT(p_config);
    uarte_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (p_cb->state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

#if NRFX_CHECK(NRFX_PRS_ENABLED)
    static nrfx_irq_handler_t const irq_handlers[NRFX_UARTE_ENABLED_COUNT] = {
        #if NRFX_CHECK(NRFX_UARTE0_ENABLED)
        nrfx_uarte_0_irq_handler,
        #endif
        #if NRFX_CHECK(NRFX_UARTE1_ENABLED)
        nrfx_uarte_1_irq_handler,
        #endif
        #if NRFX_CHECK(NRFX_UARTE2_ENABLED)
        nrfx_uarte_2_irq_handler,
        #endif
        #if NRFX_CHECK(NRFX_UARTE3_ENABLED)
        nrfx_uarte_3_irq_handler,
        #endif
    };
    if (nrfx_prs_acquire(p_instance->p_reg,
            irq_handlers[p_instance->drv_inst_idx]) != NRFX_SUCCESS)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
#endif // NRFX_CHECK(NRFX_PRS_ENABLED)

    apply_config(p_instance, p_config);

    p_cb->handler   = event_handler;
    p_cb->p_context = p_config->p_context;

    if (p_cb->handler)
    {
        interrupts_enable(p_instance, p_config->interrupt_priority);
    }

    nrf_uarte_enable(p_instance->p_reg);
    p_cb->rx_buffer_length           = 0;
    p_cb->rx_secondary_buffer_length = 0;
    p_cb->tx_buffer_length           = 0;
    p_cb->state                      = NRFX_DRV_STATE_INITIALIZED;
    NRFX_LOG_WARNING("Function: %s, error code: %s.",
                     __func__,
                     NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_uarte_uninit(nrfx_uarte_t const * p_instance)
{
    uarte_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    nrf_uarte_disable(p_instance->p_reg);

    if (p_cb->handler)
    {
        interrupts_disable(p_instance);
    }

    pins_to_default(p_instance);

#if NRFX_CHECK(NRFX_PRS_ENABLED)
    nrfx_prs_release(p_instance->p_reg);
#endif

    p_cb->state   = NRFX_DRV_STATE_UNINITIALIZED;
    p_cb->handler = NULL;
    NRFX_LOG_INFO("Instance uninitialized: %d.", p_instance->drv_inst_idx);
}

nrfx_err_t nrfx_uarte_tx(nrfx_uarte_t const * p_instance,
                         uint8_t const *      p_data,
                         size_t               length)
{
    uarte_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state == NRFX_DRV_STATE_INITIALIZED);
    NRFX_ASSERT(p_data);
    NRFX_ASSERT(length > 0);
    NRFX_ASSERT(UARTE_LENGTH_VALIDATE(p_instance->drv_inst_idx, length));

    nrfx_err_t err_code;

    // EasyDMA requires that transfer buffers are placed in DataRAM,
    // signal error if the are not.
    if (!nrfx_is_in_ram(p_data))
    {
        err_code = NRFX_ERROR_INVALID_ADDR;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (nrfx_uarte_tx_in_progress(p_instance))
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    p_cb->tx_buffer_length = length;
    p_cb->p_tx_buffer      = p_data;

    NRFX_LOG_INFO("Transfer tx_len: %d.", p_cb->tx_buffer_length);
    NRFX_LOG_DEBUG("Tx data:");
    NRFX_LOG_HEXDUMP_DEBUG(p_cb->p_tx_buffer,
                           p_cb->tx_buffer_length * sizeof(p_cb->p_tx_buffer[0]));

    err_code = NRFX_SUCCESS;

    nrf_uarte_event_clear(p_instance->p_reg, NRF_UARTE_EVENT_ENDTX);
    nrf_uarte_event_clear(p_instance->p_reg, NRF_UARTE_EVENT_TXSTOPPED);
    nrf_uarte_tx_buffer_set(p_instance->p_reg, p_cb->p_tx_buffer, p_cb->tx_buffer_length);
    nrf_uarte_task_trigger(p_instance->p_reg, NRF_UARTE_TASK_STARTTX);

    if (p_cb->handler == NULL)
    {
        bool endtx;
        bool txstopped;
        do
        {
            endtx     = nrf_uarte_event_check(p_instance->p_reg, NRF_UARTE_EVENT_ENDTX);
            txstopped = nrf_uarte_event_check(p_instance->p_reg, NRF_UARTE_EVENT_TXSTOPPED);
        }
        while ((!endtx) && (!txstopped));

        if (txstopped)
        {
            err_code = NRFX_ERROR_FORBIDDEN;
        }
        p_cb->tx_buffer_length = 0;
    }

    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

bool nrfx_uarte_tx_in_progress(nrfx_uarte_t const * p_instance)
{
    return (m_cb[p_instance->drv_inst_idx].tx_buffer_length != 0);
}

nrfx_err_t nrfx_uarte_rx(nrfx_uarte_t const * p_instance,
                         uint8_t *            p_data,
                         size_t               length)
{
    uarte_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    NRFX_ASSERT(m_cb[p_instance->drv_inst_idx].state == NRFX_DRV_STATE_INITIALIZED);
    NRFX_ASSERT(p_data);
    NRFX_ASSERT(length > 0);
    NRFX_ASSERT(UARTE_LENGTH_VALIDATE(p_instance->drv_inst_idx, length));

    nrfx_err_t err_code;

    // EasyDMA requires that transfer buffers are placed in DataRAM,
    // signal error if the are not.
    if (!nrfx_is_in_ram(p_data))
    {
        err_code = NRFX_ERROR_INVALID_ADDR;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    bool second_buffer = false;

    if (p_cb->handler)
    {
        nrf_uarte_int_disable(p_instance->p_reg, NRF_UARTE_INT_ERROR_MASK |
                                                 NRF_UARTE_INT_ENDRX_MASK);
    }
    if (p_cb->rx_buffer_length != 0)
    {
        if (p_cb->rx_secondary_buffer_length != 0)
        {
            if (p_cb->handler)
            {
                nrf_uarte_int_enable(p_instance->p_reg, NRF_UARTE_INT_ERROR_MASK |
                                                        NRF_UARTE_INT_ENDRX_MASK);
            }
            err_code = NRFX_ERROR_BUSY;
            NRFX_LOG_WARNING("Function: %s, error code: %s.",
                             __func__,
                             NRFX_LOG_ERROR_STRING_GET(err_code));
            return err_code;
        }
        second_buffer = true;
    }

    if (!second_buffer)
    {
        p_cb->rx_buffer_length = length;
        p_cb->p_rx_buffer      = p_data;
        p_cb->rx_secondary_buffer_length = 0;
    }
    else
    {
        p_cb->p_rx_secondary_buffer = p_data;
        p_cb->rx_secondary_buffer_length = length;
    }

    NRFX_LOG_INFO("Transfer rx_len: %d.", length);

    err_code = NRFX_SUCCESS;

    nrf_uarte_event_clear(p_instance->p_reg, NRF_UARTE_EVENT_ENDRX);
    nrf_uarte_event_clear(p_instance->p_reg, NRF_UARTE_EVENT_RXTO);
    nrf_uarte_rx_buffer_set(p_instance->p_reg, p_data, length);
    if (!second_buffer)
    {
        nrf_uarte_task_trigger(p_instance->p_reg, NRF_UARTE_TASK_STARTRX);
    }
    else
    {
        nrf_uarte_shorts_enable(p_instance->p_reg, NRF_UARTE_SHORT_ENDRX_STARTRX);
    }

    if (m_cb[p_instance->drv_inst_idx].handler == NULL)
    {
        bool endrx;
        bool rxto;
        bool error;
        do {
            endrx  = nrf_uarte_event_check(p_instance->p_reg, NRF_UARTE_EVENT_ENDRX);
            rxto   = nrf_uarte_event_check(p_instance->p_reg, NRF_UARTE_EVENT_RXTO);
            error  = nrf_uarte_event_check(p_instance->p_reg, NRF_UARTE_EVENT_ERROR);
        } while ((!endrx) && (!rxto) && (!error));

        m_cb[p_instance->drv_inst_idx].rx_buffer_length = 0;

        if (error)
        {
            err_code = NRFX_ERROR_INTERNAL;
        }

        if (rxto)
        {
            err_code = NRFX_ERROR_FORBIDDEN;
        }
    }
    else
    {
        nrf_uarte_int_enable(p_instance->p_reg, NRF_UARTE_INT_ERROR_MASK |
                                                NRF_UARTE_INT_ENDRX_MASK);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

bool nrfx_uarte_rx_ready(nrfx_uarte_t const * p_instance)
{
    return nrf_uarte_event_check(p_instance->p_reg, NRF_UARTE_EVENT_ENDRX);
}

uint32_t nrfx_uarte_errorsrc_get(nrfx_uarte_t const * p_instance)
{
    nrf_uarte_event_clear(p_instance->p_reg, NRF_UARTE_EVENT_ERROR);
    return nrf_uarte_errorsrc_get_and_clear(p_instance->p_reg);
}

static void rx_done_event(uarte_control_block_t * p_cb,
                          size_t                  bytes,
                          uint8_t *               p_data)
{
    nrfx_uarte_event_t event;

    event.type             = NRFX_UARTE_EVT_RX_DONE;
    event.data.rxtx.bytes  = bytes;
    event.data.rxtx.p_data = p_data;

    p_cb->handler(&event, p_cb->p_context);
}

static void tx_done_event(uarte_control_block_t * p_cb,
                          size_t                  bytes)
{
    nrfx_uarte_event_t event;

    event.type             = NRFX_UARTE_EVT_TX_DONE;
    event.data.rxtx.bytes  = bytes;
    event.data.rxtx.p_data = (uint8_t *)p_cb->p_tx_buffer;

    p_cb->tx_buffer_length = 0;

    p_cb->handler(&event, p_cb->p_context);
}

void nrfx_uarte_tx_abort(nrfx_uarte_t const * p_instance)
{
    uarte_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    nrf_uarte_event_clear(p_instance->p_reg, NRF_UARTE_EVENT_TXSTOPPED);
    nrf_uarte_task_trigger(p_instance->p_reg, NRF_UARTE_TASK_STOPTX);
    if (p_cb->handler == NULL)
    {
        while (!nrf_uarte_event_check(p_instance->p_reg, NRF_UARTE_EVENT_TXSTOPPED))
        {}
    }
    NRFX_LOG_INFO("TX transaction aborted.");
}

void nrfx_uarte_rx_abort(nrfx_uarte_t const * p_instance)
{
    uarte_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    // Short between ENDRX event and STARTRX task must be disabled before
    // aborting transmission.
    if (p_cb->rx_secondary_buffer_length != 0)
    {
        nrf_uarte_shorts_disable(p_instance->p_reg, NRF_UARTE_SHORT_ENDRX_STARTRX);
    }
    nrf_uarte_task_trigger(p_instance->p_reg, NRF_UARTE_TASK_STOPRX);
    NRFX_LOG_INFO("RX transaction aborted.");
}

static void uarte_irq_handler(NRF_UARTE_Type *        p_uarte,
                              uarte_control_block_t * p_cb)
{
    if (nrf_uarte_event_check(p_uarte, NRF_UARTE_EVENT_ERROR))
    {
        nrfx_uarte_event_t event;

        nrf_uarte_event_clear(p_uarte, NRF_UARTE_EVENT_ERROR);

        event.type                   = NRFX_UARTE_EVT_ERROR;
        event.data.error.error_mask  = nrf_uarte_errorsrc_get_and_clear(p_uarte);
        event.data.error.rxtx.bytes  = nrf_uarte_rx_amount_get(p_uarte);
        event.data.error.rxtx.p_data = p_cb->p_rx_buffer;

        // Abort transfer.
        p_cb->rx_buffer_length = 0;
        p_cb->rx_secondary_buffer_length = 0;

        p_cb->handler(&event, p_cb->p_context);
    }
    else if (nrf_uarte_event_check(p_uarte, NRF_UARTE_EVENT_ENDRX))
    {
        nrf_uarte_event_clear(p_uarte, NRF_UARTE_EVENT_ENDRX);
        size_t amount = nrf_uarte_rx_amount_get(p_uarte);
        // If the transfer was stopped before completion, amount of transfered bytes
        // will not be equal to the buffer length. Interrupted transfer is ignored.
        if (amount == p_cb->rx_buffer_length)
        {
            if (p_cb->rx_secondary_buffer_length != 0)
            {
                uint8_t * p_data = p_cb->p_rx_buffer;
                nrf_uarte_shorts_disable(p_uarte, NRF_UARTE_SHORT_ENDRX_STARTRX);
                p_cb->rx_buffer_length = p_cb->rx_secondary_buffer_length;
                p_cb->p_rx_buffer = p_cb->p_rx_secondary_buffer;
                p_cb->rx_secondary_buffer_length = 0;
                rx_done_event(p_cb, amount, p_data);
            }
            else
            {
                p_cb->rx_buffer_length = 0;
                rx_done_event(p_cb, amount, p_cb->p_rx_buffer);
            }
        }
    }

    if (nrf_uarte_event_check(p_uarte, NRF_UARTE_EVENT_RXTO))
    {
        nrf_uarte_event_clear(p_uarte, NRF_UARTE_EVENT_RXTO);

        if (p_cb->rx_buffer_length != 0)
        {
            p_cb->rx_buffer_length = 0;
            // In case of using double-buffered reception both variables storing buffer length
            // have to be cleared to prevent incorrect behaviour of the driver.
            p_cb->rx_secondary_buffer_length = 0;
            rx_done_event(p_cb, nrf_uarte_rx_amount_get(p_uarte), p_cb->p_rx_buffer);
        }
    }

    if (nrf_uarte_event_check(p_uarte, NRF_UARTE_EVENT_ENDTX))
    {
        nrf_uarte_event_clear(p_uarte, NRF_UARTE_EVENT_ENDTX);
        if (p_cb->tx_buffer_length != 0)
        {
            tx_done_event(p_cb, nrf_uarte_tx_amount_get(p_uarte));
        }
    }
}

#if NRFX_CHECK(NRFX_UARTE0_ENABLED)
void nrfx_uarte_0_irq_handler(void)
{
    uarte_irq_handler(NRF_UARTE0, &m_cb[NRFX_UARTE0_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_UARTE1_ENABLED)
void nrfx_uarte_1_irq_handler(void)
{
    uarte_irq_handler(NRF_UARTE1, &m_cb[NRFX_UARTE1_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_UARTE2_ENABLED)
void nrfx_uarte_2_irq_handler(void)
{
    uarte_irq_handler(NRF_UARTE2, &m_cb[NRFX_UARTE2_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_UARTE3_ENABLED)
void nrfx_uarte_3_irq_handler(void)
{
    uarte_irq_handler(NRF_UARTE3, &m_cb[NRFX_UARTE3_INST_IDX]);
}
#endif

#endif // NRFX_CHECK(NRFX_UARTE_ENABLED)
