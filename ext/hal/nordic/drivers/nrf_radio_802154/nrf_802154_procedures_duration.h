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
 * @brief This module contains calculations of 802.15.4 radio driver procedures duration.
 *
 */

#ifndef NRF_802154_PROCEDURES_DURATION_H_
#define NRF_802154_PROCEDURES_DURATION_H_

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"

#include "nrf_802154_const.h"

#define TX_RAMP_UP_TIME                   40 // us
#define RX_RAMP_UP_TIME                   40 // us
#define RX_RAMP_DOWN_TIME                 0  // us
#define MAX_RAMP_DOWN_TIME                6  // us
#define RX_TX_TURNAROUND_TIME             20 // us

#define A_CCA_DURATION_SYMBOLS            8  // sym
#define A_TURNAROUND_TIME_SYMBOLS         12 // sym
#define A_UNIT_BACKOFF_SYMBOLS            20 // sym

#define PHY_SYMBOLS_FROM_OCTETS(octets)   ((octets) * PHY_SYMBOLS_PER_OCTET)
#define PHY_US_TIME_FROM_SYMBOLS(symbols) ((symbols) * PHY_US_PER_SYMBOL)

#define IMM_ACK_SYMBOLS                   (PHY_SHR_SYMBOLS + \
                                           PHY_SYMBOLS_FROM_OCTETS(IMM_ACK_LENGTH + PHR_SIZE))
#define IMM_ACK_DURATION                  (PHY_US_TIME_FROM_SYMBOLS(IMM_ACK_SYMBOLS))

#define MAC_IMM_ACK_WAIT_SYMBOLS          (A_UNIT_BACKOFF_SYMBOLS +    \
                                           A_TURNAROUND_TIME_SYMBOLS + \
                                           IMM_ACK_SYMBOLS)

__STATIC_INLINE uint16_t nrf_802154_tx_duration_get(uint8_t psdu_length,
                                                    bool    cca,
                                                    bool    ack_requested);

__STATIC_INLINE uint16_t nrf_802154_cca_before_tx_duration_get(void);

__STATIC_INLINE uint16_t nrf_802154_rx_duration_get(uint8_t psdu_length, bool ack_requested);

__STATIC_INLINE uint16_t nrf_802154_cca_duration_get(void);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE uint16_t nrf_802154_frame_duration_get(uint8_t psdu_length,
                                                       bool    shr,
                                                       bool    phr)
{
    uint16_t us_time = PHY_US_TIME_FROM_SYMBOLS(PHY_SYMBOLS_FROM_OCTETS(psdu_length));

    if (phr)
    {
        us_time += PHY_US_TIME_FROM_SYMBOLS(PHY_SYMBOLS_FROM_OCTETS(PHR_SIZE));
    }

    if (shr)
    {
        us_time += PHY_US_TIME_FROM_SYMBOLS(PHY_SHR_SYMBOLS);
    }

    return us_time;
}

__STATIC_INLINE uint16_t nrf_802154_tx_duration_get(uint8_t psdu_length,
                                                    bool    cca,
                                                    bool    ack_requested)
{
    // ramp down
    // if CCA: + RX ramp up + CCA + RX ramp down
    // + TX ramp up + SHR + PHR + PSDU
    // if ACK: + macAckWaitDuration
    uint16_t us_time = MAX_RAMP_DOWN_TIME + TX_RAMP_UP_TIME + nrf_802154_frame_duration_get(
        psdu_length,
        true,
        true);

    if (ack_requested)
    {
        us_time += PHY_US_TIME_FROM_SYMBOLS(MAC_IMM_ACK_WAIT_SYMBOLS);
    }

    if (cca)
    {
        us_time += RX_RAMP_UP_TIME + RX_RAMP_DOWN_TIME + PHY_US_TIME_FROM_SYMBOLS(
            A_CCA_DURATION_SYMBOLS);
    }

    return us_time;
}

__STATIC_INLINE uint16_t nrf_802154_cca_before_tx_duration_get(void)
{
    // CCA + turnaround time
    uint16_t us_time = PHY_US_TIME_FROM_SYMBOLS(A_CCA_DURATION_SYMBOLS) + RX_TX_TURNAROUND_TIME;

    return us_time;
}

__STATIC_INLINE uint16_t nrf_802154_rx_duration_get(uint8_t psdu_length, bool ack_requested)
{
    // SHR + PHR + PSDU
    // if ACK: + aTurnaroundTime + ACK frame duration
    uint16_t us_time = nrf_802154_frame_duration_get(psdu_length, true, true);

    if (ack_requested)
    {
        us_time += PHY_US_TIME_FROM_SYMBOLS(A_TURNAROUND_TIME_SYMBOLS +
                                            PHY_SHR_SYMBOLS +
                                            PHY_SYMBOLS_FROM_OCTETS(IMM_ACK_LENGTH + PHR_SIZE));
    }

    return us_time;
}

__STATIC_INLINE uint16_t nrf_802154_cca_duration_get(void)
{
    // ramp down + rx ramp up + CCA
    uint16_t us_time = MAX_RAMP_DOWN_TIME +
                       RX_RAMP_UP_TIME +
                       PHY_US_TIME_FROM_SYMBOLS(A_CCA_DURATION_SYMBOLS);

    return us_time;
}

#endif /* SUPPRESS_INLINE_IMPLEMENTATION */

#endif /* NRF_802154_PROCEDURES_DURATION_H_ */
