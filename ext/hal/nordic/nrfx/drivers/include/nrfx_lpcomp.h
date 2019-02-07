/*
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
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

#ifndef NRFX_LPCOMP_H__
#define NRFX_LPCOMP_H__

#include <nrfx.h>
#include <hal/nrf_lpcomp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_lpcomp LPCOMP driver
 * @{
 * @ingroup nrf_lpcomp
 * @brief   Low Power Comparator (LPCOMP) peripheral driver.
 */

/**
 * @brief LPCOMP event handler function type.
 * @param[in] event  LPCOMP event.
 */
typedef void (* nrfx_lpcomp_event_handler_t)(nrf_lpcomp_event_t event);

/** @brief LPCOMP configuration. */
typedef struct
{
    nrf_lpcomp_config_t hal;                /**< LPCOMP HAL configuration. */
    nrf_lpcomp_input_t  input;              /**< Input to be monitored. */
    uint8_t             interrupt_priority; /**< LPCOMP interrupt priority. */
} nrfx_lpcomp_config_t;

/** @brief LPCOMP driver default configuration including the LPCOMP HAL configuration. */
#ifdef NRF52_SERIES
#define NRFX_LPCOMP_DEFAULT_CONFIG                                      \
    {                                                                   \
        .hal    = { (nrf_lpcomp_ref_t)NRFX_LPCOMP_CONFIG_REFERENCE ,    \
                    (nrf_lpcomp_detect_t)NRFX_LPCOMP_CONFIG_DETECTION,  \
                    (nrf_lpcomp_hysteresis_t)NRFX_LPCOMP_CONFIG_HYST }, \
        .input  = (nrf_lpcomp_input_t)NRFX_LPCOMP_CONFIG_INPUT,         \
        .interrupt_priority = NRFX_LPCOMP_CONFIG_IRQ_PRIORITY           \
    }
#else
#define NRFX_LPCOMP_DEFAULT_CONFIG                                       \
    {                                                                    \
        .hal    = { (nrf_lpcomp_ref_t)NRFX_LPCOMP_CONFIG_REFERENCE ,     \
                    (nrf_lpcomp_detect_t)NRFX_LPCOMP_CONFIG_DETECTION }, \
        .input  = (nrf_lpcomp_input_t)NRFX_LPCOMP_CONFIG_INPUT,          \
        .interrupt_priority = NRFX_LPCOMP_CONFIG_IRQ_PRIORITY            \
    }
#endif

/**
 * @brief Function for initializing the LPCOMP driver.
 *
 * This function initializes the LPCOMP driver, but does not enable the peripheral or any interrupts.
 * To start the driver, call the function nrfx_lpcomp_enable() after initialization.
 *
 * @param[in] p_config      Pointer to the structure with initial configuration.
 * @param[in] event_handler Event handler provided by the user.
 *                          Must not be NULL.
 *
 * @retval NRFX_SUCCESS             If initialization was successful.
 * @retval NRFX_ERROR_INVALID_STATE If the driver has already been initialized.
 * @retval NRFX_ERROR_BUSY          If the COMP peripheral is already in use.
 *                                  This is possible only if @ref nrfx_prs module
 *                                  is enabled.
 */
nrfx_err_t nrfx_lpcomp_init(nrfx_lpcomp_config_t const * p_config,
                            nrfx_lpcomp_event_handler_t  event_handler);

/**
 *  @brief Function for uninitializing the LCOMP driver.
 *
 *  This function uninitializes the LPCOMP driver. The LPCOMP peripheral and
 *  its interrupts are disabled, and local variables are cleaned. After this call, you must
 *  initialize the driver again by calling nrfx_lpcomp_init() if you want to use it.
 *
 *  @sa nrfx_lpcomp_disable()
 *  @sa nrfx_lpcomp_init()
 */
void  nrfx_lpcomp_uninit(void);

/**@brief Function for enabling the LPCOMP peripheral and interrupts.
 *
 * Before calling this function, the driver must be initialized. This function
 * enables the LPCOMP peripheral and its interrupts.
 *
 * @sa nrfx_lpcomp_disable()
 */
void nrfx_lpcomp_enable(void);

/**@brief Function for disabling the LPCOMP peripheral.
 *
 * Before calling this function, the driver must be initialized. This function disables the LPCOMP
 * peripheral and its interrupts.
 *
 * @sa nrfx_lpcomp_enable()
 */
void nrfx_lpcomp_disable(void);


void nrfx_lpcomp_irq_handler(void);

/** @} **/

#ifdef __cplusplus
}
#endif

#endif // NRFX_LPCOMP_H__
