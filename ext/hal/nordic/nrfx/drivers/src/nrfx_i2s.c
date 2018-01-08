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

#if NRFX_CHECK(NRFX_I2S_ENABLED)

#include <nrfx_i2s.h>
#include <hal/nrf_gpio.h>
#include <string.h>

#define NRFX_LOG_MODULE I2S
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                         \
    (event == NRF_I2S_EVENT_RXPTRUPD ? "NRF_I2S_EVENT_RXPTRUPD" : \
    (event == NRF_I2S_EVENT_TXPTRUPD ? "NRF_I2S_EVENT_TXPTRUPD" : \
    (event == NRF_I2S_EVENT_STOPPED  ? "NRF_I2S_EVENT_STOPPED"  : \
                                       "UNKNOWN EVENT")))

// Control block - driver instance local data.
typedef struct
{
    nrfx_i2s_data_handler_t handler;
    nrfx_drv_state_t        state;

    bool       synchronized_mode : 1;
    bool       rx_ready          : 1;
    bool       tx_ready          : 1;
    bool       just_started      : 1;
    uint16_t   buffer_half_size;
    uint32_t * p_rx_buffer;
    uint32_t * p_tx_buffer;
} i2s_control_block_t;
static i2s_control_block_t m_cb;

static void configure_pins(nrfx_i2s_config_t const * p_config)
{
    uint32_t mck_pin, sdout_pin, sdin_pin;

    // Configure pins used by the peripheral:

    // - SCK and LRCK (required) - depending on the mode of operation these
    //   pins are configured as outputs (in Master mode) or inputs (in Slave
    //   mode).
    if (p_config->mode == NRF_I2S_MODE_MASTER)
    {
        nrf_gpio_cfg_output(p_config->sck_pin);
        nrf_gpio_cfg_output(p_config->lrck_pin);
    }
    else
    {
        nrf_gpio_cfg_input(p_config->sck_pin,  NRF_GPIO_PIN_NOPULL);
        nrf_gpio_cfg_input(p_config->lrck_pin, NRF_GPIO_PIN_NOPULL);
    }

    // - MCK (optional) - always output,
    if (p_config->mck_pin != NRFX_I2S_PIN_NOT_USED)
    {
        mck_pin = p_config->mck_pin;
        nrf_gpio_cfg_output(mck_pin);
    }
    else
    {
        mck_pin = NRF_I2S_PIN_NOT_CONNECTED;
    }

    // - SDOUT (optional) - always output,
    if (p_config->sdout_pin != NRFX_I2S_PIN_NOT_USED)
    {
        sdout_pin = p_config->sdout_pin;
        nrf_gpio_cfg_output(sdout_pin);
    }
    else
    {
        sdout_pin = NRF_I2S_PIN_NOT_CONNECTED;
    }

    // - SDIN (optional) - always input.
    if (p_config->sdin_pin != NRFX_I2S_PIN_NOT_USED)
    {
        sdin_pin = p_config->sdin_pin;
        nrf_gpio_cfg_input(sdin_pin, NRF_GPIO_PIN_NOPULL);
    }
    else
    {
        sdin_pin = NRF_I2S_PIN_NOT_CONNECTED;
    }

    nrf_i2s_pins_set(NRF_I2S, p_config->sck_pin, p_config->lrck_pin,
        mck_pin, sdout_pin, sdin_pin);
}


nrfx_err_t nrfx_i2s_init(nrfx_i2s_config_t const * p_config,
                         nrfx_i2s_data_handler_t   handler)
{
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(handler);

    nrfx_err_t err_code;

    if (m_cb.state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (!nrf_i2s_configure(NRF_I2S, p_config->mode,
                                    p_config->format,
                                    p_config->alignment,
                                    p_config->sample_width,
                                    p_config->channels,
                                    p_config->mck_setup,
                                    p_config->ratio))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    configure_pins(p_config);

    m_cb.handler = handler;

    NRFX_IRQ_PRIORITY_SET(I2S_IRQn, p_config->irq_priority);
    NRFX_IRQ_ENABLE(I2S_IRQn);

    m_cb.state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


void nrfx_i2s_uninit(void)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    nrfx_i2s_stop();

    NRFX_IRQ_DISABLE(I2S_IRQn);

    m_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Initialized.");
}


nrfx_err_t nrfx_i2s_start(uint32_t * p_rx_buffer,
                          uint32_t * p_tx_buffer,
                          uint16_t   buffer_size,
                          uint8_t    flags)
{
    NRFX_ASSERT((p_rx_buffer != NULL) || (p_tx_buffer != NULL));

    uint16_t buffer_half_size = buffer_size / 2;
    NRFX_ASSERT(buffer_half_size != 0);

    if (m_cb.state != NRFX_DRV_STATE_INITIALIZED)
    {
        return NRFX_ERROR_INVALID_STATE;
    }

    nrfx_err_t err_code;

    if ((p_rx_buffer != NULL) && !nrfx_is_in_ram(p_rx_buffer))
    {
        err_code = NRFX_ERROR_INVALID_ADDR;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if ((p_tx_buffer != NULL) && !nrfx_is_in_ram(p_tx_buffer))
    {
        err_code = NRFX_ERROR_INVALID_ADDR;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    // Initially we set up the peripheral to use the first half of each buffer,
    // then in 'nrfx_i2s_irq_handler' we will switch to the second half.
    nrf_i2s_transfer_set(NRF_I2S, buffer_half_size, p_rx_buffer, p_tx_buffer);

    m_cb.p_rx_buffer      = p_rx_buffer;
    m_cb.p_tx_buffer      = p_tx_buffer;
    m_cb.buffer_half_size = buffer_half_size;
    m_cb.just_started     = true;

    if ((flags & NRFX_I2S_FLAG_SYNCHRONIZED_MODE) &&
        // [synchronized mode makes sense only when both RX and TX are enabled]
        (m_cb.p_rx_buffer != NULL) && (m_cb.p_tx_buffer != NULL))
    {
        m_cb.synchronized_mode = true;
        m_cb.rx_ready          = false;
        m_cb.tx_ready          = false;
    }
    else
    {
        m_cb.synchronized_mode = false;
    }

    nrf_i2s_enable(NRF_I2S);

    m_cb.state = NRFX_DRV_STATE_POWERED_ON;

    if (m_cb.p_tx_buffer != NULL)
    {
        // Get from the application the first portion of data to be sent - we
        // need to have it in the transmit buffer before we start the transfer.
        // Unless the synchronized mode is active. In this mode we must wait
        // with this until the first portion of data is received, so here we
        // just make sure that there will be silence on the SDOUT line prior
        // to that moment.
        if (m_cb.synchronized_mode)
        {
            memset(m_cb.p_tx_buffer, 0, m_cb.buffer_half_size * sizeof(uint32_t));
        }
        else
        {
            m_cb.handler(NULL, m_cb.p_tx_buffer, m_cb.buffer_half_size);
        }
    }

    nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);
    nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);
    nrf_i2s_int_enable(NRF_I2S,
        NRF_I2S_INT_RXPTRUPD_MASK | NRF_I2S_INT_TXPTRUPD_MASK);
    nrf_i2s_task_trigger(NRF_I2S, NRF_I2S_TASK_START);

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


void nrfx_i2s_stop(void)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    // First disable interrupts, then trigger the STOP task, so no spurious
    // RXPTRUPD and TXPTRUPD events (see FTPAN-55) will be processed.
    nrf_i2s_int_disable(NRF_I2S, NRF_I2S_INT_RXPTRUPD_MASK | NRF_I2S_INT_TXPTRUPD_MASK);

    nrf_i2s_task_trigger(NRF_I2S, NRF_I2S_TASK_STOP);

    nrf_i2s_disable(NRF_I2S);

    m_cb.state = NRFX_DRV_STATE_INITIALIZED;

    NRFX_LOG_INFO("Disabled.");
}


void nrfx_i2s_irq_handler(void)
{
    uint32_t const * p_data_received = NULL;
    uint32_t       * p_data_to_send  = NULL;

    if (nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD))
    {
        nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_I2S_EVENT_TXPTRUPD));

        // If transmission is not enabled, but for some reason the TXPTRUPD
        // event has been generated, just ignore it.
        if (m_cb.p_tx_buffer != NULL)
        {
            uint32_t * p_tx_buffer_next;
            if (nrf_i2s_tx_buffer_get(NRF_I2S) == m_cb.p_tx_buffer)
            {
                p_tx_buffer_next = m_cb.p_tx_buffer + m_cb.buffer_half_size;
            }
            else
            {
                p_tx_buffer_next = m_cb.p_tx_buffer;
            }
            nrf_i2s_tx_buffer_set(NRF_I2S, p_tx_buffer_next);

            m_cb.tx_ready = true;

            // Now the part of the buffer that we've configured as "next" should
            // be filled by the application with proper data to be sent;
            // the peripheral is sending data from the other part of the buffer
            // (but it will finish soon...).
            p_data_to_send = p_tx_buffer_next;

        }
    }

    if (nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD))
    {
        nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_I2S_EVENT_RXPTRUPD));

        // If reception is not enabled, but for some reason the RXPTRUPD event
        // has been generated, just ignore it.
        if (m_cb.p_rx_buffer != NULL)
        {
            uint32_t * p_rx_buffer_next;
            if (nrf_i2s_rx_buffer_get(NRF_I2S) == m_cb.p_rx_buffer)
            {
                p_rx_buffer_next = m_cb.p_rx_buffer + m_cb.buffer_half_size;
            }
            else
            {
                p_rx_buffer_next = m_cb.p_rx_buffer;
            }
            nrf_i2s_rx_buffer_set(NRF_I2S, p_rx_buffer_next);

            m_cb.rx_ready = true;

            // The RXPTRUPD event is generated for the first time right after
            // the transfer is started. Since there is no data received yet at
            // this point we only update the buffer pointer (it is done above),
            // there is no callback to the application.
            // [for synchronized mode this has to be handled differently -
            //  see below]
            if (m_cb.just_started && !m_cb.synchronized_mode)
            {
                m_cb.just_started = false;
            }
            else
            {
                // The RXPTRUPD event indicates that from now on the peripheral
                // will be filling the part of the buffer that was pointed at
                // the time the event has been generated, hence now we can let
                // the application process the data stored in the other part of
                // the buffer - the one that we've just set to be filled next.
                p_data_received = p_rx_buffer_next;
            }
        }
    }

    // Call the data handler passing received data to the application and/or
    // requesting data to be sent.
    if (!m_cb.synchronized_mode)
    {
        if ((p_data_received != NULL) || (p_data_to_send != NULL))
        {
            if (p_data_received != NULL)
            {
                NRFX_LOG_DEBUG("Rx data:");
                NRFX_LOG_HEXDUMP_DEBUG((uint8_t *)p_data_received,
                                       m_cb.buffer_half_size * sizeof(p_data_received[0]));
            }
            m_cb.handler(p_data_received, p_data_to_send,
                m_cb.buffer_half_size);
            if (p_data_to_send != NULL)
            {
                NRFX_LOG_DEBUG("Tx data:");
                NRFX_LOG_HEXDUMP_DEBUG((uint8_t *)p_data_to_send,
                                       m_cb.buffer_half_size * sizeof(p_data_to_send[0]));
            }
        }
    }
    // In the synchronized mode wait until the events for both RX and TX occur.
    // And ignore the initial occurrences of these events, since they only
    // indicate that the transfer has started - no data is received yet at
    // that moment, so we have got nothing to pass to the application.
    else
    {
        if (m_cb.rx_ready && m_cb.tx_ready)
        {
            m_cb.rx_ready = false;
            m_cb.tx_ready = false;

            if (m_cb.just_started)
            {
                m_cb.just_started = false;
            }
            else
            {
                NRFX_LOG_DEBUG("Rx data:");
                NRFX_LOG_HEXDUMP_DEBUG((uint8_t *)nrf_i2s_rx_buffer_get(NRF_I2S),
                                       m_cb.buffer_half_size * sizeof(p_data_received[0]));
                m_cb.handler(nrf_i2s_rx_buffer_get(NRF_I2S),
                             nrf_i2s_tx_buffer_get(NRF_I2S),
                             m_cb.buffer_half_size);
                NRFX_LOG_DEBUG("Tx data:");
                NRFX_LOG_HEXDUMP_DEBUG((uint8_t *)nrf_i2s_tx_buffer_get(NRF_I2S),
                                       m_cb.buffer_half_size * sizeof(p_data_to_send[0]));
            }
        }
    }
}
#endif // NRFX_CHECK(NRFX_I2S_ENABLED)
