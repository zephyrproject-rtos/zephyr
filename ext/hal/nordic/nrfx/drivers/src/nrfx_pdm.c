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

#if NRFX_CHECK(NRFX_PDM_ENABLED)

#include <nrfx_pdm.h>
#include <hal/nrf_gpio.h>

#define NRFX_LOG_MODULE PDM
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                       \
    (event == NRF_PDM_EVENT_STARTED ? "NRF_PDM_EVENT_STARTED" : \
    (event == NRF_PDM_EVENT_STOPPED ? "NRF_PDM_EVENT_STOPPED" : \
    (event == NRF_PDM_EVENT_END     ? "NRF_PDM_EVENT_END"     : \
                                      "UNKNOWN EVENT")))


/** @brief PDM interface status. */
typedef enum
{
    NRFX_PDM_STATE_IDLE,
    NRFX_PDM_STATE_RUNNING,
    NRFX_PDM_STATE_STARTING,
    NRFX_PDM_STATE_STOPPING
} nrfx_pdm_state_t;

/** @brief PDM interface control block.*/
typedef struct
{
    nrfx_pdm_event_handler_t  event_handler;    ///< Event handler function pointer.
    int16_t *                 buff_address[2];  ///< Sample buffers.
    uint16_t                  buff_length[2];   ///< Length of the sample buffers.
    nrfx_drv_state_t          drv_state;        ///< Driver state.
    volatile nrfx_pdm_state_t op_state;         ///< PDM peripheral operation state.
    uint8_t                   active_buffer;    ///< Number of currently active buffer.
    uint8_t                   error;            ///< Driver error flag.
    volatile uint8_t          irq_buff_request; ///< Request the next buffer in the ISR.
} nrfx_pdm_cb_t;

static nrfx_pdm_cb_t m_cb;


void nrfx_pdm_irq_handler(void)
{
    if (nrf_pdm_event_check(NRF_PDM_EVENT_STARTED))
    {
        nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_PDM_EVENT_STARTED));

        uint8_t finished_buffer = m_cb.active_buffer;

        // Check if the next buffer was set before.
        uint8_t next_buffer = (~m_cb.active_buffer) & 0x01;
        if (m_cb.buff_address[next_buffer] ||
            m_cb.op_state == NRFX_PDM_STATE_STARTING)
        {
            nrfx_pdm_evt_t evt;
            evt.error = NRFX_PDM_NO_ERROR;
            m_cb.error = 0;

            // Release the full buffer if ready and request the next one.
            if (m_cb.op_state == NRFX_PDM_STATE_STARTING)
            {
                evt.buffer_released = 0;
                m_cb.op_state = NRFX_PDM_STATE_RUNNING;
            }
            else
            {
                evt.buffer_released = m_cb.buff_address[finished_buffer];
                m_cb.buff_address[finished_buffer] = 0;
                m_cb.active_buffer = next_buffer;
            }
            evt.buffer_requested = true;
            m_cb.event_handler(&evt);
        }
        else
        {
            // No next buffer available. Report an error.
            // Do not request the new buffer as it was already done.
            if (m_cb.error == 0)
            {
                nrfx_pdm_evt_t const evt = {
                    .buffer_requested = false,
                    .buffer_released  = NULL,
                    .error = NRFX_PDM_ERROR_OVERFLOW
                };
                m_cb.error = 1;
                m_cb.event_handler(&evt);
            }
        }

        if (m_cb.op_state == NRFX_PDM_STATE_STARTING)
        {
            m_cb.op_state = NRFX_PDM_STATE_RUNNING;
        }
    }
    else if (nrf_pdm_event_check(NRF_PDM_EVENT_STOPPED))
    {
        nrf_pdm_event_clear(NRF_PDM_EVENT_STOPPED);
        NRFX_LOG_DEBUG("Event: %s.", EVT_TO_STR(NRF_PDM_EVENT_STOPPED));
        nrf_pdm_disable();
        m_cb.op_state = NRFX_PDM_STATE_IDLE;

        // Release the buffers.
        nrfx_pdm_evt_t evt;
        evt.error = NRFX_PDM_NO_ERROR;
        evt.buffer_requested = false;
        if (m_cb.buff_address[m_cb.active_buffer])
        {
            evt.buffer_released = m_cb.buff_address[m_cb.active_buffer];
            m_cb.buff_address[m_cb.active_buffer] = 0;
            m_cb.event_handler(&evt);
        }

        uint8_t second_buffer = (~m_cb.active_buffer) & 0x01;
        if (m_cb.buff_address[second_buffer])
        {
            evt.buffer_released = m_cb.buff_address[second_buffer];
            m_cb.buff_address[second_buffer] = 0;
            m_cb.event_handler(&evt);
        }
        m_cb.active_buffer = 0;
    }

    if (m_cb.irq_buff_request)
    {
        nrfx_pdm_evt_t const evt =
        {
            .buffer_requested = true,
            .buffer_released  = NULL,
            .error = NRFX_PDM_NO_ERROR,
        };
        m_cb.irq_buff_request = 0;
        m_cb.event_handler(&evt);
    }
}


nrfx_err_t nrfx_pdm_init(nrfx_pdm_config_t const * p_config,
                         nrfx_pdm_event_handler_t  event_handler)
{
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(event_handler);
    nrfx_err_t err_code;

    if (m_cb.drv_state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (p_config->gain_l > NRF_PDM_GAIN_MAXIMUM ||
        p_config->gain_r > NRF_PDM_GAIN_MAXIMUM)
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    m_cb.buff_address[0] = 0;
    m_cb.buff_address[1] = 0;
    m_cb.active_buffer = 0;
    m_cb.error = 0;
    m_cb.event_handler = event_handler;
    m_cb.op_state = NRFX_PDM_STATE_IDLE;

    nrf_pdm_clock_set(p_config->clock_freq);
    nrf_pdm_mode_set(p_config->mode, p_config->edge);
    nrf_pdm_gain_set(p_config->gain_l, p_config->gain_r);

    nrf_gpio_cfg_output(p_config->pin_clk);
    nrf_gpio_pin_clear(p_config->pin_clk);
    nrf_gpio_cfg_input(p_config->pin_din, NRF_GPIO_PIN_NOPULL);
    nrf_pdm_psel_connect(p_config->pin_clk, p_config->pin_din);

    nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);
    nrf_pdm_event_clear(NRF_PDM_EVENT_END);
    nrf_pdm_event_clear(NRF_PDM_EVENT_STOPPED);
    nrf_pdm_int_enable(NRF_PDM_INT_STARTED | NRF_PDM_INT_STOPPED);
    NRFX_IRQ_PRIORITY_SET(PDM_IRQn, p_config->interrupt_priority);
    NRFX_IRQ_ENABLE(PDM_IRQn);
    m_cb.drv_state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.",
                  __func__,
                  NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_pdm_uninit(void)
{
    nrf_pdm_disable();
    nrf_pdm_psel_disconnect();
    m_cb.drv_state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Uninitialized.");
}

static void pdm_start()
{
    m_cb.drv_state = NRFX_DRV_STATE_POWERED_ON;
    nrf_pdm_enable();
    nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);
    nrf_pdm_task_trigger(NRF_PDM_TASK_START);
}

static void pdm_buf_request()
{
    m_cb.irq_buff_request = 1;
    NRFX_IRQ_PENDING_SET(PDM_IRQn);
}

nrfx_err_t nrfx_pdm_start(void)
{
    NRFX_ASSERT(m_cb.drv_state != NRFX_DRV_STATE_UNINITIALIZED);
    nrfx_err_t err_code;

    if (m_cb.op_state != NRFX_PDM_STATE_IDLE)
    {
        if (m_cb.op_state == NRFX_PDM_STATE_RUNNING)
        {
            err_code = NRFX_SUCCESS;
            NRFX_LOG_INFO("Function: %s, error code: %s.",
                          __func__,
                          NRFX_LOG_ERROR_STRING_GET(err_code));
            return err_code;
        }
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    m_cb.op_state = NRFX_PDM_STATE_STARTING;
    pdm_buf_request();

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.",
                  __func__,
                  NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_pdm_buffer_set(int16_t * buffer, uint16_t buffer_length)
{
    if (m_cb.drv_state == NRFX_DRV_STATE_UNINITIALIZED)
    {
        return NRFX_ERROR_INVALID_STATE;
    }
    if (m_cb.op_state == NRFX_PDM_STATE_STOPPING)
    {
        return NRFX_ERROR_BUSY;
    }
    if ((buffer == NULL) || (buffer_length > NRFX_PDM_MAX_BUFFER_SIZE))
    {
        return NRFX_ERROR_INVALID_PARAM;
    }

    nrfx_err_t err_code = NRFX_SUCCESS;

    // Enter the PDM critical section.
    NRFX_IRQ_DISABLE(PDM_IRQn);

    uint8_t next_buffer = (~m_cb.active_buffer) & 0x01;
    if (m_cb.op_state == NRFX_PDM_STATE_STARTING)
    {
        next_buffer = 0;
    }

    if (m_cb.buff_address[next_buffer])
    {
        // Buffer already set.
        err_code = NRFX_ERROR_BUSY;
    }
    else
    {
        m_cb.buff_address[next_buffer] = buffer;
        m_cb.buff_length[next_buffer] = buffer_length;
        nrf_pdm_buffer_set((uint32_t *)buffer, buffer_length);

        if (m_cb.drv_state != NRFX_DRV_STATE_POWERED_ON)
        {
            pdm_start();
        }
    }

    NRFX_IRQ_ENABLE(PDM_IRQn);
    return err_code;
}

nrfx_err_t nrfx_pdm_stop(void)
{
    NRFX_ASSERT(m_cb.drv_state != NRFX_DRV_STATE_UNINITIALIZED);
    nrfx_err_t err_code;

    if (m_cb.op_state != NRFX_PDM_STATE_RUNNING)
    {
        if (m_cb.op_state == NRFX_PDM_STATE_IDLE ||
            m_cb.op_state == NRFX_PDM_STATE_STARTING)
        {
            nrf_pdm_disable();
            m_cb.op_state = NRFX_PDM_STATE_IDLE;
            err_code = NRFX_SUCCESS;
            NRFX_LOG_INFO("Function: %s, error code: %s.",
                          __func__,
                          NRFX_LOG_ERROR_STRING_GET(err_code));
            return err_code;
        }
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    m_cb.drv_state = NRFX_DRV_STATE_INITIALIZED;
    m_cb.op_state = NRFX_PDM_STATE_STOPPING;

    nrf_pdm_task_trigger(NRF_PDM_TASK_STOP);
    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

#endif // NRFX_CHECK(NRFX_PDM_ENABLED)
