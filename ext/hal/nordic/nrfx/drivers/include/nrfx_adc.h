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

#ifndef NRFX_ADC_H__
#define NRFX_ADC_H__

#include <nrfx.h>
#include <hal/nrf_adc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_adc ADC driver
 * @{
 * @ingroup nrf_adc
 * @brief   Analog-to-Digital Converter (ADC) peripheral driver.
 */

/**
 * @brief Driver event types.
 */
typedef enum
{
    NRFX_ADC_EVT_DONE,    ///< Event generated when the buffer is filled with samples.
    NRFX_ADC_EVT_SAMPLE,  ///< Event generated when the requested channel is sampled.
} nrfx_adc_evt_type_t;

/**
 * @brief Analog-to-digital converter driver DONE event.
 */
typedef struct
{
    nrf_adc_value_t * p_buffer; ///< Pointer to the buffer with converted samples.
    uint16_t          size;     ///< Number of samples in the buffer.
} nrfx_adc_done_evt_t;

/**
 * @brief Analog-to-digital converter driver SAMPLE event.
 */
typedef struct
{
    nrf_adc_value_t   sample; ///< Converted sample.
} nrfx_adc_sample_evt_t;

/**
 * @brief Analog-to-digital converter driver event.
 */
typedef struct
{
    nrfx_adc_evt_type_t type;  ///< Event type.
    union
    {
        nrfx_adc_done_evt_t   done;   ///< Data for DONE event.
        nrfx_adc_sample_evt_t sample; ///< Data for SAMPLE event.
    } data;
} nrfx_adc_evt_t;

/**@brief Macro for initializing the ADC channel with the default configuration. */
#define NRFX_ADC_DEFAULT_CHANNEL(analog_input)                 \
 {                                                             \
     NULL,                                                     \
     {                                                         \
        .resolution = NRF_ADC_CONFIG_RES_10BIT,                \
        .scaling    = NRF_ADC_CONFIG_SCALING_INPUT_FULL_SCALE, \
        .reference  = NRF_ADC_CONFIG_REF_VBG,                  \
        .input      = (analog_input),                          \
        .extref     = NRF_ADC_CONFIG_EXTREFSEL_NONE            \
     }                                                         \
 }

// Forward declaration of the nrfx_adc_channel_t type.
typedef struct nrfx_adc_channel_s nrfx_adc_channel_t;

/**
 * @brief ADC channel.
 *
 * This structure is defined by the user and used by the driver. Therefore, it should
 * not be defined on the stack as a local variable.
 */
struct nrfx_adc_channel_s
{
    nrfx_adc_channel_t * p_next; ///< Pointer to the next enabled channel (for internal use).
    nrf_adc_config_t     config; ///< ADC configuration for the current channel.
};

/**
 * @brief ADC configuration.
 */
typedef struct
{
    uint8_t interrupt_priority; ///< Priority of ADC interrupt.
} nrfx_adc_config_t;

/** @brief ADC default configuration. */
#define NRFX_ADC_DEFAULT_CONFIG                        \
{                                                      \
    .interrupt_priority = NRFX_ADC_CONFIG_IRQ_PRIORITY \
}

/**
 * @brief User event handler prototype.
 *
 * This function is called when the requested number of samples has been processed.
 *
 * @param p_event Event.
 */
typedef void (*nrfx_adc_event_handler_t)(nrfx_adc_evt_t const * p_event);

/**
 * @brief Function for initializing the ADC.
 *
 * If a valid event handler is provided, the driver is initialized in non-blocking mode.
 * If event_handler is NULL, the driver works in blocking mode.
 *
 * @param[in] p_config      Pointer to the structure with initial configuration.
 * @param[in] event_handler Event handler provided by the user.
 *
 * @retval    NRFX_SUCCESS If initialization was successful.
 * @retval    NRFX_ERROR_INVALID_STATE If the driver is already initialized.
 */
nrfx_err_t nrfx_adc_init(nrfx_adc_config_t const * p_config,
                         nrfx_adc_event_handler_t  event_handler);

/**
 * @brief Function for uninitializing the ADC.
 *
 * This function stops all ongoing conversions and disables all channels.
 */
void nrfx_adc_uninit(void);

/**
 * @brief Function for enabling an ADC channel.
 *
 * This function configures and enables the channel. When @ref nrfx_adc_buffer_convert is
 * called, all channels that have been enabled with this function are sampled.
 *
 * This function can be called only when there is no conversion in progress
 * (the ADC is not busy).
 *
 * @note The channel instance variable @p p_channel is used by the driver as an item
 *       in a list. Therefore, it cannot be an automatic variable that is located on the stack.
 */
void nrfx_adc_channel_enable(nrfx_adc_channel_t * const p_channel);

/**
 * @brief Function for disabling an ADC channel.
 *
 * This function can be called only when there is no conversion in progress
 * (the ADC is not busy).
 */
void nrfx_adc_channel_disable(nrfx_adc_channel_t * const p_channel);

/**
 * @brief Function for disabling all ADC channels.
 *
 * This function can be called only when there is no conversion in progress
 * (the ADC is not busy).
 */
void nrfx_adc_all_channels_disable(void);

/**
 * @brief Function for starting ADC sampling.
 *
 * This function triggers single ADC sampling. If more than one channel is enabled, the driver
 * emulates scanning and all channels are sampled in the order they were enabled.
 */
void nrfx_adc_sample(void);

/**
 * @brief Function for executing a single ADC conversion.
 *
 * This function selects the desired input and starts a single conversion. If a valid pointer
 * is provided for the result, the function blocks until the conversion is completed. Otherwise, the
 * function returns when the conversion is started, and the result is provided in an event (driver
 * must be initialized in non-blocking mode, otherwise an assertion will fail). The function will
 * fail if ADC is busy. The channel does not need to be enabled to perform a single conversion.
 *
 * @param[in]  p_channel Channel.
 * @param[out] p_value   Pointer to the location where the result should be placed. Unless NULL is
 *                       provided, the function is blocking.
 *
 * @retval NRFX_SUCCESS    If conversion was successful.
 * @retval NRFX_ERROR_BUSY If the ADC driver is busy.
 */
nrfx_err_t nrfx_adc_sample_convert(nrfx_adc_channel_t const * const p_channel,
                                   nrf_adc_value_t                * p_value);

/**
 * @brief Function for converting data to the buffer.
 *
 * If the driver is initialized in non-blocking mode, this function returns when the first
 * conversion is set up. When the buffer is filled, the application is notified by the event
 * handler. If the driver is initialized in blocking mode, the function returns when the buffer is
 * filled.
 *
 * Conversion is done on all enabled channels, but it is not triggered by this
 * function. This function will prepare the ADC for sampling and then
 * wait for the SAMPLE task. Sampling can be triggered manually by the @ref
 * nrfx_adc_sample function or by PPI using the @ref NRF_ADC_TASK_START task.
 *
 * @note If more than one channel is enabled, the function emulates scanning, and
 * a single START task will trigger conversion on all enabled channels. For example:
 * If 3 channels are enabled and the user requests 6 samples, the completion event
 * handler will be called after 2 START tasks.
 *
 * @note The application must adjust the sampling frequency. The maximum frequency
 * depends on the sampling timer and the maximum latency of the ADC interrupt. If
 * an interrupt is not handled before the next sampling is triggered, the sample
 * will be lost.
 *
 * @param[in] buffer Result buffer.
 * @param[in] size   Buffer size in samples.
 *
 * @retval NRFX_SUCCESS    If conversion was successful.
 * @retval NRFX_ERROR_BUSY If the driver is busy.
 */
nrfx_err_t nrfx_adc_buffer_convert(nrf_adc_value_t * buffer, uint16_t size);

/**
 * @brief Function for retrieving the ADC state.
 *
 * @retval true  If the ADC is busy.
 * @retval false If the ADC is ready.
 */
bool nrfx_adc_is_busy(void);

/**
 * @brief Function for getting the address of the ADC START task.
 *
 * This function is used to get the address of the START task, which can be used to trigger ADC
 * conversion.
 *
 * @return Start task address.
 */
__STATIC_INLINE uint32_t nrfx_adc_start_task_get(void);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE uint32_t nrfx_adc_start_task_get(void)
{
    return nrf_adc_task_address_get(NRF_ADC_TASK_START);
}

#endif


void nrfx_adc_irq_handler(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_ADC_H__
