/* Copyright (c) 2018, Nordic Semiconductor ASA
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
 *   This file implements RSSI calculations for 802.15.4 driver.
 *
 */

#include <stdint.h>

#include "platform/temperature/nrf_802154_temperature.h"

int8_t nrf_802154_rssi_sample_temp_corr_value_get(void)
{
    int8_t temp = nrf_802154_temperature_get();
    int8_t result;

    if (temp <= -30)
    {
        result = 3;
    }
    else if (temp <= -10)
    {
        result = 2;
    }
    else if (temp <= 10)
    {
        result = 1;
    }
    else if (temp <= 30)
    {
        result = 0;
    }
    else if (temp <= 50)
    {
        result = -1;
    }
    else if (temp <= 70)
    {
        result = -2;
    }
    else
    {
        result = -3;
    }

    return result;
}

uint8_t nrf_802154_rssi_sample_corrected_get(uint8_t rssi_sample)
{
    return rssi_sample + nrf_802154_rssi_sample_temp_corr_value_get();
}

uint8_t nrf_802154_rssi_lqi_corrected_get(uint8_t lqi)
{
    return lqi - nrf_802154_rssi_sample_temp_corr_value_get();
}

uint8_t nrf_802154_rssi_ed_corrected_get(uint8_t ed)
{
    return ed - nrf_802154_rssi_sample_temp_corr_value_get();
}

uint8_t nrf_802154_rssi_cca_ed_threshold_corrected_get(uint8_t cca_ed)
{
    return cca_ed - nrf_802154_rssi_sample_temp_corr_value_get();
}
