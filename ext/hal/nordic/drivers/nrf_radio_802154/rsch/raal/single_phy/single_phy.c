/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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
 *   This file implements the nrf 802.15.4 radio arbiter for single phy.
 *
 * This arbiter should be used when 802.15.4 is the only wireless protocol used by the application.
 *
 */

#include "nrf_raal_api.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

static bool m_continuous;

void nrf_raal_init(void)
{
    m_continuous = false;
}

void nrf_raal_uninit(void)
{
    // Intentionally empty.
}

void nrf_raal_continuous_mode_enter(void)
{
    assert(!m_continuous);

    m_continuous = true;
    nrf_raal_timeslot_started();
}

void nrf_raal_continuous_mode_exit(void)
{
    assert(m_continuous);

    m_continuous = false;
}

void nrf_raal_continuous_ended(void)
{
    // Intentionally empty.
}

bool nrf_raal_timeslot_request(uint32_t length_us)
{
    (void)length_us;

    assert(m_continuous);

    return true;
}

uint32_t nrf_raal_timeslot_us_left_get(void)
{
    return UINT32_MAX;
}
