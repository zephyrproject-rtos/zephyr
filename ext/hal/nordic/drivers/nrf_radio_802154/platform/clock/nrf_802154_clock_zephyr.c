/* Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 *   This file implements the nrf 802.15.4 HF Clock abstraction with Zephyr API.
 *
 * This implementation uses Zephyr API for clock management.
 */

#include "nrf_802154_clock.h"

#include <stddef.h>

#include <compiler_abstraction.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <clock_control.h>

static bool hfclk_is_running;
static bool lfclk_is_running;

void nrf_802154_clock_init(void)
{
    /* Intentionally empty. */
}

void nrf_802154_clock_deinit(void)
{
    /* Intentionally empty. */
}

void nrf_802154_clock_hfclk_start(void)
{
    struct device *clk_m16;

    clk_m16 = device_get_binding(DT_NORDIC_NRF_CLOCK_0_LABEL "_16M");
    __ASSERT_NO_MSG(clk_m16 != NULL);

    clock_control_on(clk_m16, (void *)1); /* Blocking call. */

    hfclk_is_running = true;

    nrf_802154_clock_hfclk_ready();
}

void nrf_802154_clock_hfclk_stop(void)
{
    struct device *clk_m16;

    clk_m16 = device_get_binding(DT_NORDIC_NRF_CLOCK_0_LABEL "_16M");
    __ASSERT_NO_MSG(clk_m16 != NULL);

    hfclk_is_running = false;

    clock_control_off(clk_m16, NULL);
}

bool nrf_802154_clock_hfclk_is_running(void)
{
    return hfclk_is_running;
}

void nrf_802154_clock_lfclk_start(void)
{
    struct device *clk_k32;

    clk_k32 = device_get_binding(DT_NORDIC_NRF_CLOCK_0_LABEL "_32K");
    __ASSERT_NO_MSG(clk_k32 != NULL);

    clock_control_on(clk_k32, (void *)CLOCK_CONTROL_NRF_K32SRC);

    lfclk_is_running = true;

    nrf_802154_clock_lfclk_ready();
}

void nrf_802154_clock_lfclk_stop(void)
{
    struct device *clk_k32;

    clk_k32 = device_get_binding(DT_NORDIC_NRF_CLOCK_0_LABEL "_32K");
    __ASSERT_NO_MSG(clk_k32 != NULL);

    lfclk_is_running = false;

    clock_control_off(clk_k32, NULL);
}

bool nrf_802154_clock_lfclk_is_running(void)
{
    return lfclk_is_running;
}

__WEAK void nrf_802154_clock_hfclk_ready(void)
{
    /* Intentionally empty. */
}

__WEAK void nrf_802154_clock_lfclk_ready(void)
{
    /* Intentionally empty. */
}
