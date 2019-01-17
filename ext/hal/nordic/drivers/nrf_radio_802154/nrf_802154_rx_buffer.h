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
 * @brief This module contains buffer for frames received by nRF 802.15.4 radio driver.
 *
 */

#ifndef NRF_802154_RX_BUFFER_H_
#define NRF_802154_RX_BUFFER_H_

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_const.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure containing received frame.
 */
typedef struct
{
    uint8_t psdu[MAX_PACKET_SIZE + 1];
    bool    free; // If this buffer is free or contains a frame.
} rx_buffer_t;

/**
 * @brief Array containing all buffers used to receive frame.
 *
 * This array is in global scope to allow optimizations in Core module in case there is only
 * one buffer provided by this module.
 *
 */
extern rx_buffer_t nrf_802154_rx_buffers[];

/**
 * @brief Initialize buffer for received frames.
 */
void nrf_802154_rx_buffer_init(void);

/**
 * @brief Get free buffer to receive a frame.
 *
 * @return  Pointer to free buffer of NULL if no free buffer is available.
 */
rx_buffer_t * nrf_802154_rx_buffer_free_find(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_RX_BUFFER_H_ */
