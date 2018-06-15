/*
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
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

#ifndef NRFX_CLOCK_H__
#define NRFX_CLOCK_H__

#include <nrfx.h>
#include <hal/nrf_clock.h>
#include <nrfx_power_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_clock CLOCK driver
 * @{
 * @ingroup nrf_clock
 * @brief   CLOCK peripheral driver.
 */

/**
 * @brief Clock events.
 */
typedef enum
{
    NRFX_CLOCK_EVT_HFCLK_STARTED, ///< HFCLK has been started.
    NRFX_CLOCK_EVT_LFCLK_STARTED, ///< LFCLK has been started.
    NRFX_CLOCK_EVT_CTTO,          ///< Calibration timeout.
    NRFX_CLOCK_EVT_CAL_DONE       ///< Calibration has been done.
} nrfx_clock_evt_type_t;

/**
 * @brief Clock event handler.
 *
 * @param[in] event  Event.
 */
typedef void (*nrfx_clock_event_handler_t)(nrfx_clock_evt_type_t event);

/**
 * @brief Function for initializing internal structures in the nrfx_clock module.
 *
 * After initialization, the module is in power off state (clocks are not started).
 *
 * @param[in] event_handler Event handler provided by the user.
 *                          Must not be NULL.
 *
 * @retval NRFX_SUCCESS                   If the procedure was successful.
 * @retval NRFX_ERROR_ALREADY_INITIALIZED If the driver was already initialized.
 */
nrfx_err_t nrfx_clock_init(nrfx_clock_event_handler_t  event_handler);

/**
 * @brief Function for enabling interrupts in the clock module.
 */
void nrfx_clock_enable(void);

/**
 * @brief Function for disabling interrupts in the clock module.
 */
void nrfx_clock_disable(void);

/**
 * @brief Function for uninitializing the clock module.
 */
void nrfx_clock_uninit(void);

/**
 * @brief Function for starting the LFCLK.
 */
void nrfx_clock_lfclk_start(void);

/**
 * @brief Function for stoping the LFCLK.
 */
void nrfx_clock_lfclk_stop(void);

/**
 * @brief Function for checking the LFCLK state.
 *
 * @retval true If the LFCLK is running.
 * @retval false If the LFCLK is not running.
 */
__STATIC_INLINE bool nrfx_clock_lfclk_is_running(void);

/**
 * @brief Function for starting the high-accuracy source HFCLK.
 */
void nrfx_clock_hfclk_start(void);

/**
 * @brief Function for stoping external high-accuracy source HFCLK.
 */
void nrfx_clock_hfclk_stop(void);

/**
 * @brief Function for checking the HFCLK state.
 *
 * @retval true If the HFCLK is running (XTAL source).
 * @retval false If the HFCLK is not running.
 */
__STATIC_INLINE bool nrfx_clock_hfclk_is_running(void);

/**
 * @brief Function for starting calibration of internal LFCLK.
 *
 * This function starts the calibration process. The process cannot be aborted. LFCLK and HFCLK
 * must be running before this function is called.
 *
 * @retval     NRFX_SUCCESS                        If the procedure was successful.
 * @retval     NRFX_ERROR_INVALID_STATE            If the low-frequency of high-frequency clock is off.
 * @retval     NRFX_ERROR_BUSY                     If calibration is in progress.
 */
nrfx_err_t nrfx_clock_calibration_start(void);

/**
 * @brief Function for checking if calibration is in progress.
 *
 * This function indicates that the system is in calibration phase.
 *
 * @retval     NRFX_SUCCESS                        If the procedure was successful.
 * @retval     NRFX_ERROR_BUSY                     If calibration is in progress.
 */
nrfx_err_t nrfx_clock_is_calibrating(void);

/**
 * @brief Function for starting calibration timer.
 * @param interval Time after which the CTTO event and interrupt will be generated (in 0.25 s units).
 */
void nrfx_clock_calibration_timer_start(uint8_t interval);

/**
 * @brief Function for stoping calibration timer.
 */
void nrfx_clock_calibration_timer_stop(void);

/**@brief Function for returning a requested task address for the clock driver module.
 *
 * @param[in]  task                               One of the peripheral tasks.
 *
 * @return     Task address.
 */
__STATIC_INLINE uint32_t nrfx_clock_ppi_task_addr(nrf_clock_task_t task);

/**@brief Function for returning a requested event address for the clock driver module.
 *
 * @param[in]  event                              One of the peripheral events.
 *
 * @return     Event address.
 */
__STATIC_INLINE uint32_t nrfx_clock_ppi_event_addr(nrf_clock_event_t event);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION
__STATIC_INLINE uint32_t nrfx_clock_ppi_task_addr(nrf_clock_task_t task)
{
    return nrf_clock_task_address_get(task);
}

__STATIC_INLINE uint32_t nrfx_clock_ppi_event_addr(nrf_clock_event_t event)
{
    return nrf_clock_event_address_get(event);
}

__STATIC_INLINE bool nrfx_clock_hfclk_is_running(void)
{
    return nrf_clock_hf_is_running(NRF_CLOCK_HFCLK_HIGH_ACCURACY);
}

__STATIC_INLINE bool nrfx_clock_lfclk_is_running(void)
{
    return nrf_clock_lf_is_running();
}
#endif //SUPPRESS_INLINE_IMPLEMENTATION


void nrfx_clock_irq_handler(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_CLOCK_H__
