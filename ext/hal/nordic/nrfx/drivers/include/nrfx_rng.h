/**
 * Copyright (c) 2016 - 2017, Nordic Semiconductor ASA
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
#ifndef NRFX_RNG_H__
#define NRFX_RNG_H__

#include <nrfx.h>
#include <hal/nrf_rng.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_rng RNG driver
 * @{
 * @ingroup nrf_rng
 * @brief   Random Number Generator (RNG) peripheral driver.
 */

/**
 * @brief Struct for RNG configuration.
 */
typedef struct
{
    bool     error_correction : 1;  /**< Error correction flag. */
    uint8_t  interrupt_priority;    /**< interrupt priority */
} nrfx_rng_config_t;

/**
 * @brief RNG default configuration.
 *        Basic usage:
 *        @code
 *            nrfx_rng_config_t config = NRFX_RNG_DEFAULT_CONFIG;
 *            if (nrfx_rng_init(&config, handler)
 *            { ...
 *        @endcode
 */
#define NRFX_RNG_DEFAULT_CONFIG                                 \
    {                                                           \
        .error_correction   = NRFX_RNG_CONFIG_ERROR_CORRECTION, \
        .interrupt_priority = NRFX_RNG_CONFIG_IRQ_PRIORITY,     \
    }

/**
 * @brief RNG driver event handler type.
 */
typedef void (* nrfx_rng_evt_handler_t)(uint8_t rng_data);

/**
 * @brief Function for initializing the nrfx_rng module.
 *
 * @param[in]  p_config Pointer to the structure with initial configuration.
 * @param[in]  handler  Event handler provided by the user. Handler is required
 *                      to work with the module. NULL value is not supported.
 *
 * @retval  NRFX_SUCCESS                   Driver was successfully initialized.
 * @retval  NRFX_ERROR_ALREADY_INITIALIZED Driver was already initialized.
 * @retval  NRFX_ERROR_INVALID_PARAM       Handler value is NULL.
 */
nrfx_err_t nrfx_rng_init(nrfx_rng_config_t const * p_config, nrfx_rng_evt_handler_t handler);

/**
 * @brief Function for starting the random value generation.
 *
 * Function enables interrupts in perihperal and start them.
 */
void nrfx_rng_start(void);

/**
 * @brief Function for stoping the random value generation.
 *
 * Function disables interrupts in perihperal and stop generation of new random values.
 */
void nrfx_rng_stop(void);

/**
 * @brief Function for uninitializing the nrfx_rng module.
 */
void nrfx_rng_uninit(void);


void nrfx_rng_irq_handler(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_RNG_H__
