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

#if NRFX_CHECK(NRFX_SPI_ENABLED)

#if !(NRFX_CHECK(NRFX_SPI0_ENABLED) || NRFX_CHECK(NRFX_SPI1_ENABLED) || \
      NRFX_CHECK(NRFX_SPI2_ENABLED))
#error "No enabled SPI instances. Check <nrfx_config.h>."
#endif

#include <nrfx_spi.h>
#include "prs/nrfx_prs.h"
#include <hal/nrf_gpio.h>

#define NRFX_LOG_MODULE SPI
#include <nrfx_log.h>

// Control block - driver instance local data.
typedef struct
{
    nrfx_spi_evt_handler_t  handler;
    void *                  p_context;
    nrfx_spi_evt_t          evt;  // Keep the struct that is ready for event handler. Less memcpy.
    nrfx_drv_state_t        state;
    volatile bool           transfer_in_progress;

    // [no need for 'volatile' attribute for the following members, as they
    //  are not concurrently used in IRQ handlers and main line code]
    uint8_t     ss_pin;
    uint8_t     miso_pin;
    uint8_t     orc;
    size_t      bytes_transferred;

    bool        abort;
} spi_control_block_t;
static spi_control_block_t m_cb[NRFX_SPI_ENABLED_COUNT];


nrfx_err_t nrfx_spi_init(nrfx_spi_t const * const  p_instance,
                         nrfx_spi_config_t const * p_config,
                         nrfx_spi_evt_handler_t    handler,
                         void *                    p_context)
{
    NRFX_ASSERT(p_config);
    spi_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
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
    static nrfx_irq_handler_t const irq_handlers[NRFX_SPI_ENABLED_COUNT] = {
        #if NRFX_CHECK(NRFX_SPI0_ENABLED)
        nrfx_spi_0_irq_handler,
        #endif
        #if NRFX_CHECK(NRFX_SPI1_ENABLED)
        nrfx_spi_1_irq_handler,
        #endif
        #if NRFX_CHECK(NRFX_SPI2_ENABLED)
        nrfx_spi_2_irq_handler,
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

    p_cb->handler = handler;
    p_cb->p_context = p_context;

    uint32_t mosi_pin;
    uint32_t miso_pin;
    // Configure pins used by the peripheral:
    // - SCK - output with initial value corresponding with the SPI mode used:
    //   0 - for modes 0 and 1 (CPOL = 0), 1 - for modes 2 and 3 (CPOL = 1);
    //   according to the reference manual guidelines this pin and its input
    //   buffer must always be connected for the SPI to work.
    if (p_config->mode <= NRF_SPI_MODE_1)
    {
        nrf_gpio_pin_clear(p_config->sck_pin);
    }
    else
    {
        nrf_gpio_pin_set(p_config->sck_pin);
    }
    nrf_gpio_cfg(p_config->sck_pin,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_CONNECT,
                 NRF_GPIO_PIN_NOPULL,
                 NRF_GPIO_PIN_S0S1,
                 NRF_GPIO_PIN_NOSENSE);
    // - MOSI (optional) - output with initial value 0,
    if (p_config->mosi_pin != NRFX_SPI_PIN_NOT_USED)
    {
        mosi_pin = p_config->mosi_pin;
        nrf_gpio_pin_clear(mosi_pin);
        nrf_gpio_cfg_output(mosi_pin);
    }
    else
    {
        mosi_pin = NRF_SPI_PIN_NOT_CONNECTED;
    }
    // - MISO (optional) - input,
    if (p_config->miso_pin != NRFX_SPI_PIN_NOT_USED)
    {
        miso_pin = p_config->miso_pin;
        nrf_gpio_cfg_input(miso_pin, (nrf_gpio_pin_pull_t)NRFX_SPI_MISO_PULL_CFG);
    }
    else
    {
        miso_pin = NRF_SPI_PIN_NOT_CONNECTED;
    }
    m_cb[p_instance->drv_inst_idx].miso_pin = p_config->miso_pin;
    // - Slave Select (optional) - output with initial value 1 (inactive).
    if (p_config->ss_pin != NRFX_SPI_PIN_NOT_USED)
    {
        nrf_gpio_pin_set(p_config->ss_pin);
        nrf_gpio_cfg_output(p_config->ss_pin);
    }
    m_cb[p_instance->drv_inst_idx].ss_pin = p_config->ss_pin;

    NRF_SPI_Type * p_spi = p_instance->p_reg;
    nrf_spi_pins_set(p_spi, p_config->sck_pin, mosi_pin, miso_pin);
    nrf_spi_frequency_set(p_spi, p_config->frequency);
    nrf_spi_configure(p_spi, p_config->mode, p_config->bit_order);

    m_cb[p_instance->drv_inst_idx].orc = p_config->orc;

    if (p_cb->handler)
    {
        nrf_spi_int_enable(p_spi, NRF_SPI_INT_READY_MASK);
    }

    nrf_spi_enable(p_spi);

    if (p_cb->handler)
    {
        NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(p_instance->p_reg),
            p_config->irq_priority);
        NRFX_IRQ_ENABLE(nrfx_get_irq_number(p_instance->p_reg));
    }

    p_cb->transfer_in_progress = false;
    p_cb->state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_spi_uninit(nrfx_spi_t const * const p_instance)
{
    spi_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    if (p_cb->handler)
    {
        NRFX_IRQ_DISABLE(nrfx_get_irq_number(p_instance->p_reg));
    }

    NRF_SPI_Type * p_spi = p_instance->p_reg;
    if (p_cb->handler)
    {
        nrf_spi_int_disable(p_spi, NRF_SPI_ALL_INTS_MASK);
    }

    if (p_cb->miso_pin != NRFX_SPI_PIN_NOT_USED)
    {
        nrf_gpio_cfg_default(p_cb->miso_pin);
    }
    nrf_spi_disable(p_spi);

#if NRFX_CHECK(NRFX_PRS_ENABLED)
    nrfx_prs_release(p_instance->p_reg);
#endif

    p_cb->state = NRFX_DRV_STATE_UNINITIALIZED;
}

static void finish_transfer(spi_control_block_t * p_cb)
{
    // If Slave Select signal is used, this is the time to deactivate it.
    if (p_cb->ss_pin != NRFX_SPI_PIN_NOT_USED)
    {
        nrf_gpio_pin_set(p_cb->ss_pin);
    }

    // By clearing this flag before calling the handler we allow subsequent
    // transfers to be started directly from the handler function.
    p_cb->transfer_in_progress = false;

    p_cb->evt.type = NRFX_SPI_EVENT_DONE;
    p_cb->handler(&p_cb->evt, p_cb->p_context);
}

// This function is called from the IRQ handler or, in blocking mode, directly
// from the 'spi_xfer' function.
// It returns true as long as the transfer should be continued, otherwise (when
// there is nothing more to send/receive) it returns false.
static bool transfer_byte(NRF_SPI_Type * p_spi, spi_control_block_t * p_cb)
{
    // Read the data byte received in this transfer (always, because no further
    // READY event can be generated until the current byte is read out from the
    // RXD register), and store it in the RX buffer (only when needed).
    volatile uint8_t rx_data = nrf_spi_rxd_get(p_spi);
    if (p_cb->bytes_transferred < p_cb->evt.xfer_desc.rx_length)
    {
        p_cb->evt.xfer_desc.p_rx_buffer[p_cb->bytes_transferred] = rx_data;
    }

    ++p_cb->bytes_transferred;

    // Check if there are more bytes to send or receive and write proper data
    // byte (next one from TX buffer or over-run character) to the TXD register
    // when needed.
    // NOTE - we've already used 'p_cb->bytes_transferred + 1' bytes from our
    //        buffers, because we take advantage of double buffering of TXD
    //        register (so in effect one byte is still being transmitted now);
    //        see how the transfer is started in the 'spi_xfer' function.
    size_t bytes_used = p_cb->bytes_transferred + 1;

    if (p_cb->abort)
    {
        if (bytes_used < p_cb->evt.xfer_desc.tx_length)
        {
            p_cb->evt.xfer_desc.tx_length = bytes_used;
        }
        if (bytes_used < p_cb->evt.xfer_desc.rx_length)
        {
            p_cb->evt.xfer_desc.rx_length = bytes_used;
        }
    }

    if (bytes_used < p_cb->evt.xfer_desc.tx_length)
    {
        nrf_spi_txd_set(p_spi, p_cb->evt.xfer_desc.p_tx_buffer[bytes_used]);
        return true;
    }
    else if (bytes_used < p_cb->evt.xfer_desc.rx_length)
    {
        nrf_spi_txd_set(p_spi, p_cb->orc);
        return true;
    }

    return (p_cb->bytes_transferred < p_cb->evt.xfer_desc.tx_length ||
            p_cb->bytes_transferred < p_cb->evt.xfer_desc.rx_length);
}

static void spi_xfer(NRF_SPI_Type               * p_spi,
                     spi_control_block_t        * p_cb,
                     nrfx_spi_xfer_desc_t const * p_xfer_desc)
{
    p_cb->bytes_transferred = 0;
    nrf_spi_int_disable(p_spi, NRF_SPI_INT_READY_MASK);

    nrf_spi_event_clear(p_spi, NRF_SPI_EVENT_READY);

    // Start the transfer by writing some byte to the TXD register;
    // if TX buffer is not empty, take the first byte from this buffer,
    // otherwise - use over-run character.
    nrf_spi_txd_set(p_spi,
        (p_xfer_desc->tx_length > 0 ? p_xfer_desc->p_tx_buffer[0] : p_cb->orc));

    // TXD register is double buffered, so next byte to be transmitted can
    // be written immediately, if needed, i.e. if TX or RX transfer is to
    // be more that 1 byte long. Again - if there is something more in TX
    // buffer send it, otherwise use over-run character.
    if (p_xfer_desc->tx_length > 1)
    {
        nrf_spi_txd_set(p_spi, p_xfer_desc->p_tx_buffer[1]);
    }
    else if (p_xfer_desc->rx_length > 1)
    {
        nrf_spi_txd_set(p_spi, p_cb->orc);
    }

    // For blocking mode (user handler not provided) wait here for READY
    // events (indicating that the byte from TXD register was transmitted
    // and a new incoming byte was moved to the RXD register) and continue
    // transaction until all requested bytes are transferred.
    // In non-blocking mode - IRQ service routine will do this stuff.
    if (p_cb->handler)
    {
        nrf_spi_int_enable(p_spi, NRF_SPI_INT_READY_MASK);
    }
    else
    {
        do {
            while (!nrf_spi_event_check(p_spi, NRF_SPI_EVENT_READY)) {}
            nrf_spi_event_clear(p_spi, NRF_SPI_EVENT_READY);
            NRFX_LOG_DEBUG("SPI: Event: NRF_SPI_EVENT_READY.");
        } while (transfer_byte(p_spi, p_cb));
        if (p_cb->ss_pin != NRFX_SPI_PIN_NOT_USED)
        {
            nrf_gpio_pin_set(p_cb->ss_pin);
        }
    }
}

nrfx_err_t nrfx_spi_xfer(nrfx_spi_t     const * const p_instance,
                         nrfx_spi_xfer_desc_t const * p_xfer_desc,
                         uint32_t                     flags)
{
    spi_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(p_xfer_desc->p_tx_buffer != NULL || p_xfer_desc->tx_length == 0);
    NRFX_ASSERT(p_xfer_desc->p_rx_buffer != NULL || p_xfer_desc->rx_length == 0);

    nrfx_err_t err_code = NRFX_SUCCESS;

    if (p_cb->transfer_in_progress)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    else
    {
        if (p_cb->handler)
        {
            p_cb->transfer_in_progress = true;
        }
    }

    p_cb->evt.xfer_desc = *p_xfer_desc;
    p_cb->abort = false;

    if (p_cb->ss_pin != NRFX_SPI_PIN_NOT_USED)
    {
        nrf_gpio_pin_clear(p_cb->ss_pin);
    }
    if (flags)
    {
        p_cb->transfer_in_progress = false;
        err_code = NRFX_ERROR_NOT_SUPPORTED;
    }
    else
    {
        spi_xfer(p_instance->p_reg, p_cb, p_xfer_desc);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.",
                  __func__,
                  NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_spi_abort(nrfx_spi_t const * p_instance)
{
    spi_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    p_cb->abort = true;
}

static void irq_handler(NRF_SPI_Type * p_spi, spi_control_block_t * p_cb)
{
    NRFX_ASSERT(p_cb->handler);

    nrf_spi_event_clear(p_spi, NRF_SPI_EVENT_READY);
    NRFX_LOG_DEBUG("Event: NRF_SPI_EVENT_READY.");

    if (!transfer_byte(p_spi, p_cb))
    {
        finish_transfer(p_cb);
    }
}

#if NRFX_CHECK(NRFX_SPI0_ENABLED)
void nrfx_spi_0_irq_handler(void)
{
    irq_handler(NRF_SPI0, &m_cb[NRFX_SPI0_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_SPI1_ENABLED)
void nrfx_spi_1_irq_handler(void)
{
    irq_handler(NRF_SPI1, &m_cb[NRFX_SPI1_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_SPI2_ENABLED)
void nrfx_spi_2_irq_handler(void)
{
    irq_handler(NRF_SPI2, &m_cb[NRFX_SPI2_INST_IDX]);
}
#endif

#endif // NRFX_CHECK(NRFX_SPI_ENABLED)
