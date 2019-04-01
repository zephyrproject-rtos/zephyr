/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
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

#ifndef NRFX_TEMP_H__
#define NRFX_TEMP_H__

#include <nrfx.h>
#include <hal/nrf_temp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_temp TEMP driver
 * @{
 * @ingroup nrf_temp
 * @brief   Temperature sensor (TEMP) driver.
 */

/** @brief Structure for TEMP configuration. */
typedef struct
{
    uint8_t interrupt_priority;    /**< Interrupt priority. */
} nrfx_temp_config_t;

/** @brief TEMP default configuration. */
#define NRFX_TEMP_DEFAULT_CONFIG                                        \
    {                                                                   \
        .interrupt_priority = NRFX_TEMP_DEFAULT_CONFIG_IRQ_PRIORITY,    \
    }

/**
 * @brief TEMP driver data ready handler type.
 *
 * @param temperature  Raw temperature in a 2's complement signed value
 *                     representation. This value can be converted to Celsius
 *                     scale using the @ref nrfx_temp_calculate() function.
 */
typedef void (* nrfx_temp_data_handler_t)(int32_t raw_temperature);

/**
 * @brief Function for initializing the TEMP driver.
 *
 * @param[in] p_config  Pointer to the structure with initial configuration.
 * @param[in] handler   Data handler provided by the user. If not provided,
 *                      the driver is initialized in blocking mode.
 *
 * @retval NRFX_SUCCESS                    Driver was successfully initialized.
 * @retval NRFX_ERROR_ALREADY_INITIALIZED  Driver was already initialized.
 */
nrfx_err_t nrfx_temp_init(nrfx_temp_config_t const * p_config, nrfx_temp_data_handler_t handler);

/** @brief Function for uninitializing the TEMP driver. */
void nrfx_temp_uninit(void);

/**
 * @brief Function for getting the temperature measurement in a 2's complement
 *        signed value representation.
 *
 * This function returns the last value prepared by the TEMP peripheral.
 * In blocking mode, it should be used after calling the @ref nrfx_temp_measure()
 * function. In non-blocking mode, it is called internally by the driver,
 * and the value it returns is passed to the data handler.
 *
 * @return Temperature measurement result in a 2's complement signed value
 *         representation.
 */
__STATIC_INLINE int32_t nrfx_temp_result_get(void);

/**
 * @brief Function for calculating the temperature value in Celsius scale from raw data.
 *
 * The returned temperature value is in Celsius scale, multiplied by 100
 * For example, the actual temperature of 25.75[C] will be returned as a 2575 signed integer.
 * Measurement accuracy is 0.25[C].
 *
 * @param[in] raw_measurement Temperature value in a 2's complement signed
 *                            value representation.
 *
 * @return Temperature measurement result.
 */
int32_t nrfx_temp_calculate(int32_t raw_measurement);

/**
 * @brief Function for starting the temperature measurement.
 *
 * Non-blocking mode:
 * This function returns immediately. After a measurement, the handler specified
 * during initialization is called, with measurement result as the parameter.
 *
 * Blocking mode:
 * This function waits until the measurement is finished. The value should be read
 * using the @ref nrfx_temp_result_get() function.
 *
 * @retval NRFX_SUCCESS        In non-blocking mode: Measurement was started.
 *                             An interrupt will be generated soon. <br>
 *                             In blocking mode:
 *                             Measurement was started and finished. Data can
 *                             be read using the @ref nrfx_temp_result_get() function.
 * @retval NRFX_ERROR_INTERNAL In non-blocking mode:
 *                             Not applicable. <br>
 *                             In blocking mode:
 *                             Measurement data ready event did not occur.
 */
nrfx_err_t nrfx_temp_measure(void);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE int32_t nrfx_temp_result_get(void)
{
    return nrf_temp_result_get(NRF_TEMP);
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

void nrfx_temp_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif // NRFX_TEMP_H__
