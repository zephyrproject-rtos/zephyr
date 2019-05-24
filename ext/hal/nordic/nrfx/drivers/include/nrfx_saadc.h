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

#ifndef NRFX_SAADC_H__
#define NRFX_SAADC_H__

#include <nrfx.h>
#include <hal/nrf_saadc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_saadc SAADC driver
 * @{
 * @ingroup nrf_saadc
 * @brief   Successive Approximation Analog-to-Digital Converter (SAADC) peripheral driver.
 */

/** @brief Value to be set as high limit to disable limit detection. */
#define NRFX_SAADC_LIMITH_DISABLED (2047)
/** @brief Value to be set as low limit to disable limit detection. */
#define NRFX_SAADC_LIMITL_DISABLED (-2048)

/** @brief Macro for setting @ref nrfx_saadc_config_t to default settings. */
#define NRFX_SAADC_DEFAULT_CONFIG                                               \
{                                                                               \
    .resolution         = (nrf_saadc_resolution_t)NRFX_SAADC_CONFIG_RESOLUTION, \
    .oversample         = (nrf_saadc_oversample_t)NRFX_SAADC_CONFIG_OVERSAMPLE, \
    .interrupt_priority = NRFX_SAADC_CONFIG_IRQ_PRIORITY,                       \
    .low_power_mode     = NRFX_SAADC_CONFIG_LP_MODE                             \
}

/**
 * @brief Macro for setting @ref nrf_saadc_channel_config_t to default settings
 *        in single-ended mode.
 *
 * @param PIN_P Analog input.
 */
#define NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(PIN_P) \
{                                                   \
    .resistor_p = NRF_SAADC_RESISTOR_DISABLED,      \
    .resistor_n = NRF_SAADC_RESISTOR_DISABLED,      \
    .gain       = NRF_SAADC_GAIN1_6,                \
    .reference  = NRF_SAADC_REFERENCE_INTERNAL,     \
    .acq_time   = NRF_SAADC_ACQTIME_10US,           \
    .mode       = NRF_SAADC_MODE_SINGLE_ENDED,      \
    .burst      = NRF_SAADC_BURST_DISABLED,         \
    .pin_p      = (nrf_saadc_input_t)(PIN_P),       \
    .pin_n      = NRF_SAADC_INPUT_DISABLED          \
}

/**
 * @brief Macro for setting @ref nrf_saadc_channel_config_t to default settings
 *        in differential mode.
 *
 * @param PIN_P Positive analog input.
 * @param PIN_N Negative analog input.
 */
#define NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_DIFFERENTIAL(PIN_P, PIN_N) \
{                                                                    \
    .resistor_p = NRF_SAADC_RESISTOR_DISABLED,                       \
    .resistor_n = NRF_SAADC_RESISTOR_DISABLED,                       \
    .gain       = NRF_SAADC_GAIN1_6,                                 \
    .reference  = NRF_SAADC_REFERENCE_INTERNAL,                      \
    .acq_time   = NRF_SAADC_ACQTIME_10US,                            \
    .mode       = NRF_SAADC_MODE_DIFFERENTIAL,                       \
    .burst      = NRF_SAADC_BURST_DISABLED,                          \
    .pin_p      = (nrf_saadc_input_t)(PIN_P),                        \
    .pin_n      = (nrf_saadc_input_t)(PIN_N)                         \
}

/** @brief SAADC driver configuration structure. */
typedef struct
{
    nrf_saadc_resolution_t resolution;         ///< Resolution configuration.
    nrf_saadc_oversample_t oversample;         ///< Oversampling configuration.
    uint8_t                interrupt_priority; ///< Interrupt priority.
    bool                   low_power_mode;     ///< Indicates if low power mode is active.
} nrfx_saadc_config_t;

/** @brief SAADC driver event types. */
typedef enum
{
    NRFX_SAADC_EVT_DONE,         ///< Event generated when the buffer is filled with samples.
    NRFX_SAADC_EVT_LIMIT,        ///< Event generated after one of the limits is reached.
    NRFX_SAADC_EVT_CALIBRATEDONE ///< Event generated when the calibration is complete.
} nrfx_saadc_evt_type_t;

/** @brief SAADC driver done event data. */
typedef struct
{
    nrf_saadc_value_t * p_buffer; ///< Pointer to buffer with converted samples.
    uint16_t            size;     ///< Number of samples in the buffer.
} nrfx_saadc_done_evt_t;

/** @brief SAADC driver limit event data. */
typedef struct
{
    uint8_t           channel;    ///< Channel on which the limit was detected.
    nrf_saadc_limit_t limit_type; ///< Type of limit detected.
} nrfx_saadc_limit_evt_t;

/** @brief SAADC driver event structure. */
typedef struct
{
    nrfx_saadc_evt_type_t type; ///< Event type.
    union
    {
        nrfx_saadc_done_evt_t  done;  ///< Data for @ref NRFX_SAADC_EVT_DONE event.
        nrfx_saadc_limit_evt_t limit; ///< Data for @ref NRFX_SAADC_EVT_LIMIT event.
    } data;                           ///< Union to store event data.
} nrfx_saadc_evt_t;

/**
 * @brief SAADC driver event handler.
 *
 * @param[in] p_event Pointer to an SAADC driver event. The event structure is allocated on
 *                    the stack, so it is valid only within the context of the event handler.
 */
typedef void (* nrfx_saadc_event_handler_t)(nrfx_saadc_evt_t const * p_event);

/**
 * @brief Function for initializing the SAADC.
 *
 * @param[in] p_config      Pointer to the structure with initial configuration.
 * @param[in] event_handler Event handler provided by the user.
 *                          Must not be NULL.
 *
 * @retval NRFX_SUCCESS             Initialization was successful.
 * @retval NRFX_ERROR_INVALID_STATE The driver is already initialized.
 */
nrfx_err_t nrfx_saadc_init(nrfx_saadc_config_t const * p_config,
                           nrfx_saadc_event_handler_t  event_handler);

/**
 * @brief Function for uninitializing the SAADC.
 *
 * This function stops all ongoing conversions and disables all channels.
 */
void nrfx_saadc_uninit(void);


/**
 * @brief Function for getting the address of a SAMPLE SAADC task.
 *
 * @return Task address.
 */
uint32_t nrfx_saadc_sample_task_get(void);

/**
 * @brief Function for initializing an SAADC channel.
 *
 * This function configures and enables the channel.
 *
 * @param[in] channel  Channel index.
 * @param[in] p_config Pointer to the structure with the initial configuration.
 *
 * @retval NRFX_SUCCESS             Initialization was successful.
 * @retval NRFX_ERROR_INVALID_STATE The SAADC was not initialized.
 * @retval NRFX_ERROR_NO_MEM        The specified channel was already allocated.
 */
nrfx_err_t nrfx_saadc_channel_init(uint8_t                                  channel,
                                   nrf_saadc_channel_config_t const * const p_config);

/**
 * @brief Function for uninitializing an SAADC channel.
 *
 * @param[in] channel Channel index.
 *
 * @retval NRFX_SUCCESS    Uninitialization was successful.
 * @retval NRFX_ERROR_BUSY The SAADC is busy.
 */
nrfx_err_t nrfx_saadc_channel_uninit(uint8_t channel);

/**
 * @brief Function for starting the SAADC sampling.
 *
 * @retval NRFX_SUCCESS             The SAADC sampling was triggered.
 * @retval NRFX_ERROR_INVALID_STATE The SAADC is in idle state.
 */
nrfx_err_t nrfx_saadc_sample(void);

/**
 * @brief Blocking function for executing a single SAADC conversion.
 *
 * This function selects the desired input, starts a single conversion,
 * waits for it to finish, and returns the result.
 *
 * The function fails if the SAADC is busy.
 *
 * @param[in]  channel Channel.
 * @param[out] p_value Pointer to the location where the result is to be placed.
 *
 * @retval NRFX_SUCCESS    The conversion was successful.
 * @retval NRFX_ERROR_BUSY The SAADC driver is busy.
 */
nrfx_err_t nrfx_saadc_sample_convert(uint8_t channel, nrf_saadc_value_t * p_value);

/**
 * @brief Function for issuing conversion of data to the buffer.
 *
 * This function is non-blocking. The application is notified about filling the buffer by the event
 * handler. Conversion will be done on all enabled channels. If the SAADC is in idle state, the
 * function will set up EasyDMA for the conversion. The SAADC will be ready for sampling and wait
 * for the SAMPLE task. It can be triggered manually by the @ref nrfx_saadc_sample function
 * or by PPI using the @ref NRF_SAADC_TASK_SAMPLE task. If one buffer is already set and the
 * conversion is ongoing, calling this function will result in queuing the given buffer.
 * The driver will start filling the issued buffer when the first one is completed.
 * If the function is called again before the first buffer is filled or calibration
 * is in progress, it will return with error.
 *
 * @param[in] buffer Result buffer.
 * @param[in] size   Buffer size in words.
 *
 * @retval NRFX_SUCCESS    The conversion was successful.
 * @retval NRFX_ERROR_BUSY The driver already has two buffers set or the calibration is in progress.
 */
nrfx_err_t nrfx_saadc_buffer_convert(nrf_saadc_value_t * buffer, uint16_t size);

/**
 * @brief Function for triggering the SAADC offset calibration.
 *
 * This function is non-blocking. The application is notified about completion by the event handler.
 * Calibration will also trigger DONE and RESULTDONE events.
 *
 * The function will fail if the SAADC is busy or calibration is already in progress.
 *
 * @retval NRFX_SUCCESS    The calibration was started successfully.
 * @retval NRFX_ERROR_BUSY The SAADC driver is busy.
 */
nrfx_err_t nrfx_saadc_calibrate_offset(void);

/**
 * @brief Function for retrieving the SAADC state.
 *
 * @retval true  The SAADC is busy.
 * @retval false The SAADC is ready.
 */
bool nrfx_saadc_is_busy(void);

/**
 * @brief Function for aborting the ongoing and buffered conversions.
 *
 * @note @ref NRFX_SAADC_EVT_DONE event will be generated if there is a conversion in progress.
 *       Event will contain number of words in the sample buffer.
 */
void nrfx_saadc_abort(void);

/**
 * @brief Function for setting the SAADC channel limits.
 *        When limits are enabled and the result exceeds the defined bounds, the limit handler
 *        function is called.
 *
 * @param[in] channel    SAADC channel number.
 * @param[in] limit_low  Lower limit (valid values from @ref NRFX_SAADC_LIMITL_DISABLED to
 *                       @ref NRFX_SAADC_LIMITH_DISABLED). Conversion results below this value will
 *                       trigger the handler function. Set to @ref NRFX_SAADC_LIMITL_DISABLED
 *                       to disable this limit.
 * @param[in] limit_high Upper limit (valid values from @ref NRFX_SAADC_LIMITL_DISABLED to
 *                       @ref NRFX_SAADC_LIMITH_DISABLED). Conversion results above this value will
 *                       trigger the handler function. Set to @ref NRFX_SAADC_LIMITH_DISABLED
 *                       to disable this limit.
 */
void nrfx_saadc_limits_set(uint8_t channel, int16_t limit_low, int16_t limit_high);

/** @} */


void nrfx_saadc_irq_handler(void);


#ifdef __cplusplus
}
#endif

#endif // NRFX_SAADC_H__

