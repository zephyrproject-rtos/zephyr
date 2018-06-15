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
#include <nrfx.h>

#if NRFX_CHECK(NRFX_SYSTICK_ENABLED)
#include <nrfx_systick.h>

/**
 * @brief Maximum number of ticks to delay
 *
 * The maximum number of ticks should be much lower than
 * Physical maximum count of the SysTick timer.
 * It is dictated by the fact that it would be impossible to detect delay
 * properly when the timer value warps around the starting point.
 */
#define NRFX_SYSTICK_TICKS_MAX (NRF_SYSTICK_VAL_MASK / 2UL)

/**
 * @brief Number of milliseconds in a second
 */
#define NRFX_SYSTICK_MS (1000UL)

/**
 * @brief Number of microseconds in a second
 */
#define NRFX_SYSTICK_US (1000UL * NRFX_SYSTICK_MS)

/**
 * @brief Number of milliseconds to wait in single loop
 *
 * Constant used by @ref nrd_drv_systick_delay_ms function
 * to split waiting into loops and rest.
 *
 * It describes the number of milliseconds to wait in single loop.
 *
 * See @ref nrfx_systick_delay_ms source code for details.
 */
#define NRFX_SYSTICK_MS_STEP (64U)

/**
 * @brief Checks if the given time is in correct range
 *
 * Function tests given time is not to big for this library.
 * Assertion is used for testing.
 *
 * @param us Time in microseconds to check
 */
#define NRFX_SYSTICK_ASSERT_TIMEOUT(us) \
    NRFX_ASSERT(us <= (NRFX_SYSTICK_TICKS_MAX / ((SystemCoreClock) / NRFX_SYSTICK_US)));

/**
 * @brief Function that converts microseconds to ticks
 *
 * Function converts from microseconds to CPU ticks.
 *
 * @param us Number of microseconds
 *
 * @return Number of ticks
 *
 * @sa nrfx_systick_ms_tick
 */
static inline uint32_t nrfx_systick_us_tick(uint32_t us)
{
    return us * ((SystemCoreClock) / NRFX_SYSTICK_US);
}

/**
 * @brief Function that converts milliseconds to ticks
 *
 * Function converts from milliseconds to CPU ticks.
 *
 * @param us Number of milliseconds
 *
 * @return Number of ticks
 *
 * @sa nrfx_systick_us_tick
 */
static inline uint32_t nrfx_systick_ms_tick(uint32_t ms)
{
    return ms * ((SystemCoreClock) / NRFX_SYSTICK_MS);
}

void nrfx_systick_init(void)
{
    nrf_systick_load_set(NRF_SYSTICK_VAL_MASK);
    nrf_systick_csr_set(
        NRF_SYSTICK_CSR_CLKSOURCE_CPU |
        NRF_SYSTICK_CSR_TICKINT_DISABLE |
        NRF_SYSTICK_CSR_ENABLE);
}

void nrfx_systick_get(nrfx_systick_state_t * p_state)
{
    p_state->time = nrf_systick_val_get();
}

bool nrfx_systick_test(nrfx_systick_state_t const * p_state, uint32_t us)
{
    NRFX_SYSTICK_ASSERT_TIMEOUT(us);

    const uint32_t diff = NRF_SYSTICK_VAL_MASK & ((p_state->time) - nrf_systick_val_get());
    return (diff >= nrfx_systick_us_tick(us));
}

void nrfx_systick_delay_ticks(uint32_t ticks)
{
    NRFX_ASSERT(ticks <= NRFX_SYSTICK_TICKS_MAX);

    const uint32_t start = nrf_systick_val_get();
    while ((NRF_SYSTICK_VAL_MASK & (start - nrf_systick_val_get())) < ticks)
    {
        /* Nothing to do */
    }
}

void nrfx_systick_delay_us(uint32_t us)
{
    NRFX_SYSTICK_ASSERT_TIMEOUT(us);
    nrfx_systick_delay_ticks(nrfx_systick_us_tick(us));
}

void nrfx_systick_delay_ms(uint32_t ms)
{
    uint32_t n = ms / NRFX_SYSTICK_MS_STEP;
    uint32_t r = ms % NRFX_SYSTICK_MS_STEP;
    while (0 != (n--))
    {
        nrfx_systick_delay_ticks(nrfx_systick_ms_tick(NRFX_SYSTICK_MS_STEP));
    }
    nrfx_systick_delay_ticks(nrfx_systick_ms_tick(r));
}

#endif // NRFX_CHECK(NRFX_SYSTICK_ENABLED)
