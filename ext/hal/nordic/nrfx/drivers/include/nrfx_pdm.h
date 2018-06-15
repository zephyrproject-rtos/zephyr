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

#ifndef NRFX_PDM_H__
#define NRFX_PDM_H__

#include <nrfx.h>
#include <hal/nrf_pdm.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_pdm PDM driver
 * @{
 * @ingroup nrf_pdm
 * @brief   Pulse Density Modulation (PDM) peripheral driver.
 */


#define NRFX_PDM_MAX_BUFFER_SIZE 32767


/**
 * @brief PDM error type.
 */
typedef enum
{
    NRFX_PDM_NO_ERROR = 0,
    NRFX_PDM_ERROR_OVERFLOW = 1
} nrfx_pdm_error_t;

/**
 * @brief PDM event structure.
 */
typedef struct
{
    bool             buffer_requested;  ///< Buffer request flag.
    int16_t *        buffer_released;   ///< Pointer to the released buffer. Can be NULL.
    nrfx_pdm_error_t error;             ///< Error type.
} nrfx_pdm_evt_t;

/**
 * @brief PDM interface driver configuration structure.
 */
typedef struct
{
    nrf_pdm_mode_t mode;               ///< Interface operation mode.
    nrf_pdm_edge_t edge;               ///< Sampling mode.
    uint8_t        pin_clk;            ///< CLK pin.
    uint8_t        pin_din;            ///< DIN pin.
    nrf_pdm_freq_t clock_freq;         ///< Clock frequency.
    nrf_pdm_gain_t gain_l;             ///< Left channel gain.
    nrf_pdm_gain_t gain_r;             ///< Right channel gain.
    uint8_t        interrupt_priority; ///< Interrupt priority.
} nrfx_pdm_config_t;

/**
 * @brief Macro for setting @ref nrfx_pdm_config_t to default settings
 *        in single ended mode.
 *
 * @param _pin_clk  CLK output pin.
 * @param _pin_din  DIN input pin.
 */
#define NRFX_PDM_DEFAULT_CONFIG(_pin_clk, _pin_din)                   \
{                                                                     \
    .mode               = (nrf_pdm_mode_t)NRFX_PDM_CONFIG_MODE,       \
    .edge               = (nrf_pdm_edge_t)NRFX_PDM_CONFIG_EDGE,       \
    .pin_clk            = _pin_clk,                                   \
    .pin_din            = _pin_din,                                   \
    .clock_freq         = (nrf_pdm_freq_t)NRFX_PDM_CONFIG_CLOCK_FREQ, \
    .gain_l             = NRF_PDM_GAIN_DEFAULT,                       \
    .gain_r             = NRF_PDM_GAIN_DEFAULT,                       \
    .interrupt_priority = NRFX_PDM_CONFIG_IRQ_PRIORITY                \
}

/**
 * @brief Handler for PDM interface ready events.
 *
 * This event handler is called on a buffer request, an error or when a buffer
 * is full and ready to be processed.
 *
 * @param[in] p_evt Pointer to the PDM event structure.
 */
typedef void (*nrfx_pdm_event_handler_t)(nrfx_pdm_evt_t const * const p_evt);


/**
 * @brief Function for initializing the PDM interface.
 *
 * @param[in] p_config      Pointer to the structure with initial configuration.
 * @param[in] event_handler Event handler provided by the user. Cannot be NULL.
 *
 * @retval    NRFX_SUCCESS If initialization was successful.
 * @retval    NRFX_ERROR_INVALID_STATE If the driver is already initialized.
 * @retval    NRFX_ERROR_INVALID_PARAM If invalid configuration was specified.
 */
nrfx_err_t nrfx_pdm_init(nrfx_pdm_config_t const * p_config,
                         nrfx_pdm_event_handler_t  event_handler);

/**
 * @brief Function for uninitializing the PDM interface.
 *
 * This function stops PDM sampling, if it is in progress.
 */
void nrfx_pdm_uninit(void);

/**
 * @brief Function for getting the address of a PDM interface task.
 *
 * @param[in]  task Task.
 *
 * @return     Task address.
 */
__STATIC_INLINE uint32_t nrfx_pdm_task_address_get(nrf_pdm_task_t task)
{
    return nrf_pdm_task_address_get(task);
}

/**
 * @brief Function for getting the state of the PDM interface.
 *
 * @retval true  If the PDM interface is enabled.
 * @retval false If the PDM interface is disabled.
 */
__STATIC_INLINE bool nrfx_pdm_enable_check(void)
{
    return nrf_pdm_enable_check();
}

/**
 * @brief Function for starting PDM sampling.
 *
 * @retval NRFX_SUCCESS    If sampling was started successfully or was already in progress.
 * @retval NRFX_ERROR_BUSY If a previous start/stop operation is in progress.
 */
nrfx_err_t nrfx_pdm_start(void);

/**
 * @brief   Function for stopping PDM sampling.
 *
 * When this function is called, the PDM interface is stopped after finishing
 * the current frame.
 * The event handler function might be called once more after calling this function.
 *
 * @retval NRFX_SUCCESS    If sampling was stopped successfully or was already stopped before.
 * @retval NRFX_ERROR_BUSY If a previous start/stop operation is in progress.
 */
nrfx_err_t nrfx_pdm_stop(void);

/**
 * @brief   Function for supplying the sample buffer.
 *
 * Call this function after every buffer request event.
 *
 * @param[in]  buffer        Pointer to the receive buffer. Cannot be NULL.
 * @param[in]  buffer_length Length of the receive buffer in 16-bit words.
 *
 * @retval NRFX_SUCCESS             If the buffer was applied successfully.
 * @retval NRFX_ERROR_BUSY          If the buffer was already supplied or the peripheral is currently being stopped.
 * @retval NRFX_ERROR_INVALID_STATE If the driver was not initialized.
 * @retval NRFX_ERROR_INVALID_PARAM If invalid parameters were provided.
 */
nrfx_err_t nrfx_pdm_buffer_set(int16_t * buffer, uint16_t buffer_length);


void nrfx_pdm_irq_handler(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_PDM_H__
