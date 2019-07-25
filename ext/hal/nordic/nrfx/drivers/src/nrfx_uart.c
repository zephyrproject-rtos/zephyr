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

#if NRFX_CHECK(NRFX_UART_ENABLED)

#if !NRFX_CHECK(NRFX_UART0_ENABLED)
#error "No enabled UART instances. Check <nrfx_config.h>."
#endif

#include <nrfx_uart.h>
#include "prs/nrfx_prs.h"
#include <hal/nrf_gpio.h>

#define NRFX_LOG_MODULE UART
#include <nrfx_log.h>

#define EVT_TO_STR(event) \
    (event == NRF_UART_EVENT_ERROR ? "NRF_UART_EVENT_ERROR" : \
                                     "UNKNOWN EVENT")


#define TX_COUNTER_ABORT_REQ_VALUE  UINT32_MAX

typedef struct
{
    void                    * p_context;
    nrfx_uart_event_handler_t handler;
    uint8_t           const * p_tx_buffer;
    uint8_t                 * p_rx_buffer;
    uint8_t                 * p_rx_secondary_buffer;
    volatile size_t           tx_buffer_length;
    size_t                    rx_buffer_length;
    size_t                    rx_secondary_buffer_length;
    volatile size_t           tx_counter;
    volatile size_t           rx_counter;
    volatile bool             tx_abort;
    bool                      rx_enabled;
    nrfx_drv_state_t          state;
} uart_control_block_t;
static uart_control_block_t m_cb[NRFX_UART_ENABLED_COUNT];

static void apply_config(nrfx_uart_t        const * p_instance,
                         nrfx_uart_config_t const * p_config)
{
    if (p_config->pseltxd != NRF_UART_PSEL_DISCONNECTED)
    {
        nrf_gpio_pin_set(p_config->pseltxd);
        nrf_gpio_cfg_output(p_config->pseltxd);
    }
    if (p_config->pselrxd != NRF_UART_PSEL_DISCONNECTED)
    {
        nrf_gpio_cfg_input(p_config->pselrxd, NRF_GPIO_PIN_NOPULL);
    }

    nrf_uart_baudrate_set(p_instance->p_reg, p_config->baudrate);
    nrf_uart_configure(p_instance->p_reg, p_config->parity, p_config->hwfc);
    nrf_uart_txrx_pins_set(p_instance->p_reg, p_config->pseltxd, p_config->pselrxd);
    if (p_config->hwfc == NRF_UART_HWFC_ENABLED)
    {
        if (p_config->pselcts != NRF_UART_PSEL_DISCONNECTED)
        {
            nrf_gpio_cfg_input(p_config->pselcts, NRF_GPIO_PIN_NOPULL);
        }
        if (p_config->pselrts != NRF_UART_PSEL_DISCONNECTED)
        {
            nrf_gpio_pin_set(p_config->pselrts);
            nrf_gpio_cfg_output(p_config->pselrts);
        }
        nrf_uart_hwfc_pins_set(p_instance->p_reg, p_config->pselrts, p_config->pselcts);
    }
}

static void interrupts_enable(nrfx_uart_t const * p_instance,
                              uint8_t             interrupt_priority)
{
    nrf_uart_event_clear(p_instance->p_reg, NRF_UART_EVENT_TXDRDY);
    nrf_uart_event_clear(p_instance->p_reg, NRF_UART_EVENT_RXTO);
    nrf_uart_int_enable(p_instance->p_reg, NRF_UART_INT_MASK_TXDRDY |
                                           NRF_UART_INT_MASK_RXTO);
    NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number((void *)p_instance->p_reg),
                          interrupt_priority);
    NRFX_IRQ_ENABLE(nrfx_get_irq_number((void *)p_instance->p_reg));
}

static void interrupts_disable(nrfx_uart_t const * p_instance)
{
    nrf_uart_int_disable(p_instance->p_reg, NRF_UART_INT_MASK_RXDRDY |
                                            NRF_UART_INT_MASK_TXDRDY |
                                            NRF_UART_INT_MASK_ERROR  |
                                            NRF_UART_INT_MASK_RXTO);
    NRFX_IRQ_DISABLE(nrfx_get_irq_number((void *)p_instance->p_reg));
}

static void pins_to_default(nrfx_uart_t const * p_instance)
{
    /* Reset pins to default states */
    uint32_t txd;
    uint32_t rxd;
    uint32_t rts;
    uint32_t cts;

    txd = nrf_uart_tx_pin_get(p_instance->p_reg);
    rxd = nrf_uart_rx_pin_get(p_instance->p_reg);
    rts = nrf_uart_rts_pin_get(p_instance->p_reg);
    cts = nrf_uart_cts_pin_get(p_instance->p_reg);
    nrf_uart_txrx_pins_disconnect(p_instance->p_reg);
    nrf_uart_hwfc_pins_disconnect(p_instance->p_reg);

    if (txd != NRF_UART_PSEL_DISCONNECTED)
    {
        nrf_gpio_cfg_default(txd);
    }
    if (rxd != NRF_UART_PSEL_DISCONNECTED)
    {
        nrf_gpio_cfg_default(rxd);
    }
    if (cts != NRF_UART_PSEL_DISCONNECTED)
    {
        nrf_gpio_cfg_default(cts);
    }
    if (rts != NRF_UART_PSEL_DISCONNECTED)
    {
        nrf_gpio_cfg_default(rts);
    }
}

nrfx_err_t nrfx_uart_init(nrfx_uart_t const *        p_instance,
                          nrfx_uart_config_t const * p_config,
                          nrfx_uart_event_handler_t  event_handler)
{
    NRFX_ASSERT(p_config);
    uart_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
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
    static nrfx_irq_handler_t const irq_handlers[NRFX_UART_ENABLED_COUNT] = {
        #if NRFX_CHECK(NRFX_UART0_ENABLED)
        nrfx_uart_0_irq_handler,
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

    nrf_uart_enable(p_instance->p_reg);
    p_cb->rx_buffer_length           = 0;
    p_cb->rx_secondary_buffer_length = 0;
    p_cb->rx_enabled                 = false;
    p_cb->tx_buffer_length           = 0;
    p_cb->state                      = NRFX_DRV_STATE_INITIALIZED;
    NRFX_LOG_WARNING("Function: %s, error code: %s.",
                     __func__,
                     NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_uart_uninit(nrfx_uart_t const * p_instance)
{
    uart_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    nrf_uart_disable(p_instance->p_reg);

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

static void tx_byte(NRF_UART_Type * p_uart, uart_control_block_t * p_cb)
{
    nrf_uart_event_clear(p_uart, NRF_UART_EVENT_TXDRDY);
    uint8_t txd = p_cb->p_tx_buffer[p_cb->tx_counter];
    p_cb->tx_counter++;
    nrf_uart_txd_set(p_uart, txd);
}

static bool tx_blocking(NRF_UART_Type * p_uart, uart_control_block_t * p_cb)
{
    // Use a local variable to avoid undefined order of accessing two volatile variables
    // in one statement.
    size_t const tx_buffer_length = p_cb->tx_buffer_length;
    while (p_cb->tx_counter < tx_buffer_length)
    {
        // Wait until the transmitter is ready to accept a new byte.
        // Exit immediately if the transfer has been aborted.
        while (!nrf_uart_event_check(p_uart, NRF_UART_EVENT_TXDRDY))
        {
            if (p_cb->tx_abort)
            {
                return false;
            }
        }

        tx_byte(p_uart, p_cb);
    }

    return true;
}

nrfx_err_t nrfx_uart_tx(nrfx_uart_t const * p_instance,
                        uint8_t const *     p_data,
                        size_t              length)
{
    uart_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state == NRFX_DRV_STATE_INITIALIZED);
    NRFX_ASSERT(p_data);
    NRFX_ASSERT(length > 0);

    nrfx_err_t err_code;

    if (nrfx_uart_tx_in_progress(p_instance))
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    p_cb->tx_buffer_length = length;
    p_cb->p_tx_buffer      = p_data;
    p_cb->tx_counter       = 0;
    p_cb->tx_abort         = false;

    NRFX_LOG_INFO("Transfer tx_len: %d.", p_cb->tx_buffer_length);
    NRFX_LOG_DEBUG("Tx data:");
    NRFX_LOG_HEXDUMP_DEBUG(p_cb->p_tx_buffer,
                           p_cb->tx_buffer_length * sizeof(p_cb->p_tx_buffer[0]));

    err_code = NRFX_SUCCESS;

    nrf_uart_event_clear(p_instance->p_reg, NRF_UART_EVENT_TXDRDY);
    nrf_uart_task_trigger(p_instance->p_reg, NRF_UART_TASK_STARTTX);

    tx_byte(p_instance->p_reg, p_cb);

    if (p_cb->handler == NULL)
    {
        if (!tx_blocking(p_instance->p_reg, p_cb))
        {
            // The transfer has been aborted.
            err_code = NRFX_ERROR_FORBIDDEN;
        }
        else
        {
            // Wait until the last byte is completely transmitted.
            while (!nrf_uart_event_check(p_instance->p_reg, NRF_UART_EVENT_TXDRDY))
            {}
            nrf_uart_task_trigger(p_instance->p_reg, NRF_UART_TASK_STOPTX);
        }
        p_cb->tx_buffer_length = 0;
    }

    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

bool nrfx_uart_tx_in_progress(nrfx_uart_t const * p_instance)
{
    return (m_cb[p_instance->drv_inst_idx].tx_buffer_length != 0);
}

static void rx_enable(nrfx_uart_t const * p_instance)
{
    nrf_uart_event_clear(p_instance->p_reg, NRF_UART_EVENT_ERROR);
    nrf_uart_event_clear(p_instance->p_reg, NRF_UART_EVENT_RXDRDY);
    nrf_uart_task_trigger(p_instance->p_reg, NRF_UART_TASK_STARTRX);
}

static void rx_byte(NRF_UART_Type * p_uart, uart_control_block_t * p_cb)
{
    if (!p_cb->rx_buffer_length)
    {
        nrf_uart_event_clear(p_uart, NRF_UART_EVENT_RXDRDY);
        // Byte received when buffer is not set - data lost.
        (void) nrf_uart_rxd_get(p_uart);
        return;
    }
    nrf_uart_event_clear(p_uart, NRF_UART_EVENT_RXDRDY);
    p_cb->p_rx_buffer[p_cb->rx_counter] = nrf_uart_rxd_get(p_uart);
    p_cb->rx_counter++;
}

nrfx_err_t nrfx_uart_rx(nrfx_uart_t const * p_instance,
                        uint8_t *           p_data,
                        size_t              length)
{
    uart_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    NRFX_ASSERT(m_cb[p_instance->drv_inst_idx].state == NRFX_DRV_STATE_INITIALIZED);
    NRFX_ASSERT(p_data);
    NRFX_ASSERT(length > 0);

    nrfx_err_t err_code;

    bool second_buffer = false;

    if (p_cb->handler)
    {
        nrf_uart_int_disable(p_instance->p_reg, NRF_UART_INT_MASK_RXDRDY |
                                                NRF_UART_INT_MASK_ERROR);
    }
    if (p_cb->rx_buffer_length != 0)
    {
        if (p_cb->rx_secondary_buffer_length != 0)
        {
            if (p_cb->handler)
            {
                nrf_uart_int_enable(p_instance->p_reg, NRF_UART_INT_MASK_RXDRDY |
                                                       NRF_UART_INT_MASK_ERROR);
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
        p_cb->rx_counter       = 0;
        p_cb->rx_secondary_buffer_length = 0;
    }
    else
    {
        p_cb->p_rx_secondary_buffer = p_data;
        p_cb->rx_secondary_buffer_length = length;
    }

    NRFX_LOG_INFO("Transfer rx_len: %d.", length);

    if ((!p_cb->rx_enabled) && (!second_buffer))
    {
        rx_enable(p_instance);
    }

    if (p_cb->handler == NULL)
    {
        nrf_uart_event_clear(p_instance->p_reg, NRF_UART_EVENT_RXTO);

        bool rxrdy;
        bool rxto;
        bool error;
        do
        {
            do
            {
                error = nrf_uart_event_check(p_instance->p_reg, NRF_UART_EVENT_ERROR);
                rxrdy = nrf_uart_event_check(p_instance->p_reg, NRF_UART_EVENT_RXDRDY);
                rxto  = nrf_uart_event_check(p_instance->p_reg, NRF_UART_EVENT_RXTO);
            } while ((!rxrdy) && (!rxto) && (!error));

            if (error || rxto)
            {
                break;
            }
            rx_byte(p_instance->p_reg, p_cb);
        } while (p_cb->rx_buffer_length > p_cb->rx_counter);

        p_cb->rx_buffer_length = 0;
        if (error)
        {
            err_code = NRFX_ERROR_INTERNAL;
            NRFX_LOG_WARNING("Function: %s, error code: %s.",
                             __func__,
                             NRFX_LOG_ERROR_STRING_GET(err_code));
            return err_code;
        }

        if (rxto)
        {
            err_code = NRFX_ERROR_FORBIDDEN;
            NRFX_LOG_WARNING("Function: %s, error code: %s.",
                             __func__,
                             NRFX_LOG_ERROR_STRING_GET(err_code));
            return err_code;
        }

        if (p_cb->rx_enabled)
        {
            nrf_uart_task_trigger(p_instance->p_reg, NRF_UART_TASK_STARTRX);
        }
        else
        {
            // Skip stopping RX if driver is forced to be enabled.
            nrf_uart_task_trigger(p_instance->p_reg, NRF_UART_TASK_STOPRX);
        }
    }
    else
    {
        nrf_uart_int_enable(p_instance->p_reg, NRF_UART_INT_MASK_RXDRDY |
                                               NRF_UART_INT_MASK_ERROR);
    }
    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

bool nrfx_uart_rx_ready(nrfx_uart_t const * p_instance)
{
    return nrf_uart_event_check(p_instance->p_reg, NRF_UART_EVENT_RXDRDY);
}

void nrfx_uart_rx_enable(nrfx_uart_t const * p_instance)
{
    if (!m_cb[p_instance->drv_inst_idx].rx_enabled)
    {
        rx_enable(p_instance);
        m_cb[p_instance->drv_inst_idx].rx_enabled = true;
    }
}

void nrfx_uart_rx_disable(nrfx_uart_t const * p_instance)
{
    nrf_uart_task_trigger(p_instance->p_reg, NRF_UART_TASK_STOPRX);
    m_cb[p_instance->drv_inst_idx].rx_enabled = false;
}

uint32_t nrfx_uart_errorsrc_get(nrfx_uart_t const * p_instance)
{
    nrf_uart_event_clear(p_instance->p_reg, NRF_UART_EVENT_ERROR);
    return nrf_uart_errorsrc_get_and_clear(p_instance->p_reg);
}

static void rx_done_event(uart_control_block_t * p_cb,
                          size_t                 bytes,
                          uint8_t *              p_data)
{
    nrfx_uart_event_t event;

    event.type             = NRFX_UART_EVT_RX_DONE;
    event.data.rxtx.bytes  = bytes;
    event.data.rxtx.p_data = p_data;

    p_cb->handler(&event, p_cb->p_context);
}

static void tx_done_event(uart_control_block_t * p_cb,
                          size_t                 bytes)
{
    nrfx_uart_event_t event;

    event.type             = NRFX_UART_EVT_TX_DONE;
    event.data.rxtx.bytes  = bytes;
    event.data.rxtx.p_data = (uint8_t *)p_cb->p_tx_buffer;

    p_cb->tx_buffer_length = 0;

    p_cb->handler(&event, p_cb->p_context);
}

void nrfx_uart_tx_abort(nrfx_uart_t const * p_instance)
{
    uart_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    p_cb->tx_abort = true;
    nrf_uart_task_trigger(p_instance->p_reg, NRF_UART_TASK_STOPTX);
    if (p_cb->handler)
    {
        tx_done_event(p_cb, p_cb->tx_counter);
    }

    NRFX_LOG_INFO("TX transaction aborted.");
}

void nrfx_uart_rx_abort(nrfx_uart_t const * p_instance)
{
    nrf_uart_int_disable(p_instance->p_reg, NRF_UART_INT_MASK_RXDRDY |
                                            NRF_UART_INT_MASK_ERROR);
    nrf_uart_task_trigger(p_instance->p_reg, NRF_UART_TASK_STOPRX);

    NRFX_LOG_INFO("RX transaction aborted.");
}

static void uart_irq_handler(NRF_UART_Type *        p_uart,
                             uart_control_block_t * p_cb)
{
    if (nrf_uart_int_enable_check(p_uart, NRF_UART_INT_MASK_ERROR) &&
        nrf_uart_event_check(p_uart, NRF_UART_EVENT_ERROR))
    {
        nrfx_uart_event_t event;
        nrf_uart_event_clear(p_uart, NRF_UART_EVENT_ERROR);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_UART_EVENT_ERROR));
        nrf_uart_int_disable(p_uart, NRF_UART_INT_MASK_RXDRDY |
                                     NRF_UART_INT_MASK_ERROR);
        if (!p_cb->rx_enabled)
        {
            nrf_uart_task_trigger(p_uart, NRF_UART_TASK_STOPRX);
        }
        event.type                   = NRFX_UART_EVT_ERROR;
        event.data.error.error_mask  = nrf_uart_errorsrc_get_and_clear(p_uart);
        event.data.error.rxtx.bytes  = p_cb->rx_buffer_length;
        event.data.error.rxtx.p_data = p_cb->p_rx_buffer;

        // Abort transfer.
        p_cb->rx_buffer_length = 0;
        p_cb->rx_secondary_buffer_length = 0;

        p_cb->handler(&event,p_cb->p_context);
    }
    else if (nrf_uart_int_enable_check(p_uart, NRF_UART_INT_MASK_RXDRDY) &&
             nrf_uart_event_check(p_uart, NRF_UART_EVENT_RXDRDY))
    {
        rx_byte(p_uart, p_cb);
        if (p_cb->rx_buffer_length == p_cb->rx_counter)
        {
            if (p_cb->rx_secondary_buffer_length)
            {
                uint8_t * p_data     = p_cb->p_rx_buffer;
                size_t    rx_counter = p_cb->rx_counter;

                // Switch to secondary buffer.
                p_cb->rx_buffer_length = p_cb->rx_secondary_buffer_length;
                p_cb->p_rx_buffer = p_cb->p_rx_secondary_buffer;
                p_cb->rx_secondary_buffer_length = 0;
                p_cb->rx_counter = 0;
                rx_done_event(p_cb, rx_counter, p_data);
            }
            else
            {
                if (!p_cb->rx_enabled)
                {
                    nrf_uart_task_trigger(p_uart, NRF_UART_TASK_STOPRX);
                }
                nrf_uart_int_disable(p_uart, NRF_UART_INT_MASK_RXDRDY |
                                             NRF_UART_INT_MASK_ERROR);
                p_cb->rx_buffer_length = 0;
                rx_done_event(p_cb, p_cb->rx_counter, p_cb->p_rx_buffer);
            }
        }
    }

    if (nrf_uart_event_check(p_uart, NRF_UART_EVENT_TXDRDY))
    {
        // Use a local variable to avoid undefined order of accessing two volatile variables
        // in one statement.
        size_t const tx_buffer_length = p_cb->tx_buffer_length;
        if (p_cb->tx_counter < tx_buffer_length && !p_cb->tx_abort)
        {
            tx_byte(p_uart, p_cb);
        }
        else
        {
            nrf_uart_event_clear(p_uart, NRF_UART_EVENT_TXDRDY);
            if (p_cb->tx_buffer_length)
            {
                tx_done_event(p_cb, p_cb->tx_buffer_length);
            }
        }
    }

    if (nrf_uart_event_check(p_uart, NRF_UART_EVENT_RXTO))
    {
        nrf_uart_event_clear(p_uart, NRF_UART_EVENT_RXTO);

        // RXTO event may be triggered as a result of abort call. In th
        if (p_cb->rx_enabled)
        {
            nrf_uart_task_trigger(p_uart, NRF_UART_TASK_STARTRX);
        }
        if (p_cb->rx_buffer_length)
        {
            p_cb->rx_buffer_length = 0;
            rx_done_event(p_cb, p_cb->rx_counter, p_cb->p_rx_buffer);
        }
    }
}

#if NRFX_CHECK(NRFX_UART0_ENABLED)
void nrfx_uart_0_irq_handler(void)
{
    uart_irq_handler(NRF_UART0, &m_cb[NRFX_UART0_INST_IDX]);
}
#endif

#endif // NRFX_CHECK(NRFX_UART_ENABLED)
