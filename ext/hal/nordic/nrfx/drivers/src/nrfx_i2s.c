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

#if NRFX_CHECK(NRFX_I2S_ENABLED)

#include <nrfx_i2s.h>
#include <hal/nrf_gpio.h>

#define NRFX_LOG_MODULE I2S
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                         \
    (event == NRF_I2S_EVENT_RXPTRUPD ? "NRF_I2S_EVENT_RXPTRUPD" : \
    (event == NRF_I2S_EVENT_TXPTRUPD ? "NRF_I2S_EVENT_TXPTRUPD" : \
    (event == NRF_I2S_EVENT_STOPPED  ? "NRF_I2S_EVENT_STOPPED"  : \
                                       "UNKNOWN EVENT")))

#if !defined(USE_WORKAROUND_FOR_ANOMALY_194) &&          \
    (defined(NRF52832_XXAA) || defined(NRF52832_XXAB) || \
     defined(NRF52840_XXAA))
// Enable workaround for nRF52832 and nRF52840 anomaly 194 (STOP task does not
// switch off all resources).
#define USE_WORKAROUND_FOR_ANOMALY_194 1
#endif

// Control block - driver instance local data.
typedef struct
{
    nrfx_i2s_data_handler_t handler;
    nrfx_drv_state_t        state;

    bool use_rx         : 1;
    bool use_tx         : 1;
    bool rx_ready       : 1;
    bool tx_ready       : 1;
    bool buffers_needed : 1;
    bool buffers_reused : 1;

    uint16_t            buffer_size;
    nrfx_i2s_buffers_t  next_buffers;
    nrfx_i2s_buffers_t  current_buffers;
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

    nrf_i2s_pins_set(NRF_I2S,
                     p_config->sck_pin,
                     p_config->lrck_pin,
                     mck_pin,
                     sdout_pin,
                     sdin_pin);
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

    if (!nrf_i2s_configure(NRF_I2S,
                           p_config->mode,
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

    NRFX_LOG_INFO("Initialized.");
    return NRFX_SUCCESS;
}


void nrfx_i2s_uninit(void)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    nrfx_i2s_stop();

    NRFX_IRQ_DISABLE(I2S_IRQn);

    nrf_i2s_pins_set(NRF_I2S,
                     NRF_I2S_PIN_NOT_CONNECTED,
                     NRF_I2S_PIN_NOT_CONNECTED,
                     NRF_I2S_PIN_NOT_CONNECTED,
                     NRF_I2S_PIN_NOT_CONNECTED,
                     NRF_I2S_PIN_NOT_CONNECTED);

    m_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Uninitialized.");
}


nrfx_err_t nrfx_i2s_start(nrfx_i2s_buffers_t const * p_initial_buffers,
                          uint16_t                   buffer_size,
                          uint8_t                    flags)
{
    NRFX_ASSERT(p_initial_buffers != NULL);
    NRFX_ASSERT(p_initial_buffers->p_rx_buffer != NULL ||
                p_initial_buffers->p_tx_buffer != NULL);
    NRFX_ASSERT(buffer_size != 0);
    (void)(flags);

    nrfx_err_t err_code;

    if (m_cb.state != NRFX_DRV_STATE_INITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (((p_initial_buffers->p_rx_buffer != NULL)
         && !nrfx_is_in_ram(p_initial_buffers->p_rx_buffer))
        ||
        ((p_initial_buffers->p_tx_buffer != NULL)
         && !nrfx_is_in_ram(p_initial_buffers->p_tx_buffer)))
    {
        err_code = NRFX_ERROR_INVALID_ADDR;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    m_cb.use_rx         = (p_initial_buffers->p_rx_buffer != NULL);
    m_cb.use_tx         = (p_initial_buffers->p_tx_buffer != NULL);
    m_cb.rx_ready       = false;
    m_cb.tx_ready       = false;
    m_cb.buffers_needed = false;
    m_cb.buffer_size    = buffer_size;

    // Set the provided initial buffers as next, they will become the current
    // ones after the IRQ handler is called for the first time, what will occur
    // right after the START task is triggered.
    m_cb.next_buffers = *p_initial_buffers;
    m_cb.current_buffers.p_rx_buffer = NULL;
    m_cb.current_buffers.p_tx_buffer = NULL;

    nrf_i2s_transfer_set(NRF_I2S,
                         m_cb.buffer_size,
                         m_cb.next_buffers.p_rx_buffer,
                         m_cb.next_buffers.p_tx_buffer);

    nrf_i2s_enable(NRF_I2S);

    m_cb.state = NRFX_DRV_STATE_POWERED_ON;

    nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);
    nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);
    nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_STOPPED);
    nrf_i2s_int_enable(NRF_I2S, (m_cb.use_rx ? NRF_I2S_INT_RXPTRUPD_MASK : 0) |
                                (m_cb.use_tx ? NRF_I2S_INT_TXPTRUPD_MASK : 0) |
                                NRF_I2S_INT_STOPPED_MASK);
    nrf_i2s_task_trigger(NRF_I2S, NRF_I2S_TASK_START);

    NRFX_LOG_INFO("Started.");
    return NRFX_SUCCESS;
}


nrfx_err_t nrfx_i2s_next_buffers_set(nrfx_i2s_buffers_t const * p_buffers)
{
    NRFX_ASSERT(m_cb.state == NRFX_DRV_STATE_POWERED_ON);
    NRFX_ASSERT(p_buffers);

    nrfx_err_t err_code;

    if (!m_cb.buffers_needed)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (((p_buffers->p_rx_buffer != NULL)
         && !nrfx_is_in_ram(p_buffers->p_rx_buffer))
        ||
        ((p_buffers->p_tx_buffer != NULL)
         && !nrfx_is_in_ram(p_buffers->p_tx_buffer)))
    {
        err_code = NRFX_ERROR_INVALID_ADDR;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (m_cb.use_tx)
    {
        NRFX_ASSERT(p_buffers->p_tx_buffer != NULL);
        nrf_i2s_tx_buffer_set(NRF_I2S, p_buffers->p_tx_buffer);
    }
    if (m_cb.use_rx)
    {
        NRFX_ASSERT(p_buffers->p_rx_buffer != NULL);
        nrf_i2s_rx_buffer_set(NRF_I2S, p_buffers->p_rx_buffer);
    }

    m_cb.next_buffers   = *p_buffers;
    m_cb.buffers_needed = false;

    return NRFX_SUCCESS;
}


void nrfx_i2s_stop(void)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    m_cb.buffers_needed = false;

    // First disable interrupts, then trigger the STOP task, so no spurious
    // RXPTRUPD and TXPTRUPD events (see nRF52 anomaly 55) are processed.
    nrf_i2s_int_disable(NRF_I2S, NRF_I2S_INT_RXPTRUPD_MASK |
                                 NRF_I2S_INT_TXPTRUPD_MASK);
    nrf_i2s_task_trigger(NRF_I2S, NRF_I2S_TASK_STOP);

#if USE_WORKAROUND_FOR_ANOMALY_194
    *((volatile uint32_t *)0x40025038) = 1;
    *((volatile uint32_t *)0x4002503C) = 1;
#endif
}


void nrfx_i2s_irq_handler(void)
{
    if (nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD))
    {
        nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);
        m_cb.tx_ready = true;
        if (m_cb.use_tx && m_cb.buffers_needed)
        {
            m_cb.buffers_reused = true;
        }
    }
    if (nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD))
    {
        nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);
        m_cb.rx_ready = true;
        if (m_cb.use_rx && m_cb.buffers_needed)
        {
            m_cb.buffers_reused = true;
        }
    }

    if (nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_STOPPED))
    {
        nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_STOPPED);
        nrf_i2s_int_disable(NRF_I2S, NRF_I2S_INT_STOPPED_MASK);
        nrf_i2s_disable(NRF_I2S);

        // When stopped, release all buffers, including these scheduled for
        // the next transfer.
        m_cb.handler(&m_cb.current_buffers, 0);
        m_cb.handler(&m_cb.next_buffers, 0);

        m_cb.state = NRFX_DRV_STATE_INITIALIZED;
        NRFX_LOG_INFO("Stopped.");
    }
    else
    {
        // Check if the requested transfer has been completed:
        // - full-duplex mode
        if ((m_cb.use_tx && m_cb.use_rx && m_cb.tx_ready && m_cb.rx_ready) ||
            // - TX only mode
            (!m_cb.use_rx && m_cb.tx_ready) ||
            // - RX only mode
            (!m_cb.use_tx && m_cb.rx_ready))
        {
            m_cb.tx_ready = false;
            m_cb.rx_ready = false;

            // If the application did not supply the buffers for the next
            // part of the transfer until this moment, the current buffers
            // cannot be released, since the I2S peripheral already started
            // using them. Signal this situation to the application by
            // passing NULL instead of the structure with released buffers.
            if (m_cb.buffers_reused)
            {
                m_cb.buffers_reused = false;
                // This will most likely be set at this point. However, there is
                // a small time window between TXPTRUPD and RXPTRUPD events,
                // and it is theoretically possible that next buffers will be
                // set in this window, so to be sure this flag is set to true,
                // set it explicitly.
                m_cb.buffers_needed = true;
                m_cb.handler(NULL,
                             NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
            }
            else
            {
                // Buffers that have been used by the I2S peripheral (current)
                // are now released and will be returned to the application,
                // and the ones scheduled to be used as next become the current
                // ones.
                nrfx_i2s_buffers_t released_buffers = m_cb.current_buffers;
                m_cb.current_buffers = m_cb.next_buffers;
                m_cb.next_buffers.p_rx_buffer = NULL;
                m_cb.next_buffers.p_tx_buffer = NULL;
                m_cb.buffers_needed = true;
                m_cb.handler(&released_buffers,
                             NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
            }

        }
    }
}

#endif // NRFX_CHECK(NRFX_I2S_ENABLED)
