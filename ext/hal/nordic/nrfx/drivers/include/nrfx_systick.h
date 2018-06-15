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

#ifndef NRFX_SYSTICK_H__
#define NRFX_SYSTICK_H__

#include <nrfx.h>
#include <hal/nrf_systick.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_systick ARM(R) SysTick driver
 * @{
 * @ingroup nrf_systick
 *
 * @brief   ARM(R) SysTick driver.
 *
 * This driver configures ARM(R) SysTick as a free-running timer.
 * This timer is used to generate delays and pool for timeouts.
 * Only relatively short timeouts are supported.
 * The SysTick works on 64MHz and is 24-bits wide.
 * It means that it overflows around 4 times per second and around 250&nbsp;ms
 * would be the highest supported time in the library.
 * But it would be really hard to detect if overflow was generated without
 * using interrupts. For safety we would limit the maximum delay range by half.
 */

/**
 * @brief The value type that holds the SysTick state
 *
 * This variable is used to count the requested timeout.
 * @sa nrfx_systick_get
 */
typedef struct {
    uint32_t time; //!< Registered time value
} nrfx_systick_state_t;

/**
 * @brief Configure and start the timer
 *
 * Function configures SysTick as a free-running timer without interrupt.
 */
void nrfx_systick_init(void);

/**
 * @brief Get current SysTick state
 *
 * Function gets current state of the SysTick timer.
 * It can be used to check time-out by @ref nrfx_systick_test.
 *
 * @param[out] p_state The pointer to the state variable to be filled
 */
void nrfx_systick_get(nrfx_systick_state_t * p_state);

/**
 * @brief Test if specified time is up in relation to remembered state
 *
 * @param[in] p_state Remembered state set by @ref nrfx_systick_get
 * @param[in] us      Required time-out.
 *
 * @retval true  If current time is higher than specified state plus given time-out.
 * @retval false If current time is lower than specified state plus given time-out
 */
bool nrfx_systick_test(nrfx_systick_state_t const * p_state, uint32_t us);

/**
 * @brief Blocking delay in CPU ticks
 *
 * @param[in] ticks Number of CPU ticks to delay.
 */
void nrfx_systick_delay_ticks(uint32_t ticks);

/**
 * @brief Blocking delay in us
 *
 * @param[in] us Number of microseconds to delay.
 */
void nrfx_systick_delay_us(uint32_t us);

/**
 * @brief Blocking delay in ms
 *
 * This delay function removes the limits of the highest possible delay value.
 *
 * @param[in] ms Number of milliseconds to delay.
 */
void nrfx_systick_delay_ms(uint32_t ms);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRFX_SYSTICK_H__ */
