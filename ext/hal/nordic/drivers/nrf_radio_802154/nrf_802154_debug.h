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
 * @brief This module contains debug helpers for 802.15.4 radio driver for nRF SoC devices.
 *
 */

#ifndef NRF_802154_DEBUG_H_
#define NRF_802154_DEBUG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NRF_802154_DEBUG_LOG_BUFFER_LEN        1024

#define EVENT_TRACE_ENTER                      0x0001UL
#define EVENT_TRACE_EXIT                       0x0002UL
#define EVENT_SET_STATE                        0x0005UL
#define EVENT_RADIO_RESET                      0x0006UL
#define EVENT_TIMESLOT_REQUEST                 0x0007UL
#define EVENT_TIMESLOT_REQUEST_RESULT          0x0008UL

#define FUNCTION_SLEEP                         0x0001UL
#define FUNCTION_RECEIVE                       0x0002UL
#define FUNCTION_TRANSMIT                      0x0003UL
#define FUNCTION_ENERGY_DETECTION              0x0004UL
#define FUNCTION_BUFFER_FREE                   0x0005UL
#define FUNCTION_CCA                           0x0006UL
#define FUNCTION_CONTINUOUS_CARRIER            0x0007UL
#define FUNCTION_CSMACA                        0x0008UL
#define FUNCTION_TRANSMIT_AT                   0x0009UL
#define FUNCTION_RECEIVE_AT                    0x000AUL

#define FUNCTION_IRQ_HANDLER                   0x0100UL
#define FUNCTION_EVENT_FRAMESTART              0x0101UL
#define FUNCTION_EVENT_BCMATCH                 0x0102UL
#define FUNCTION_EVENT_END                     0x0103UL
#define FUNCTION_EVENT_DISABLED                0x0104UL
#define FUNCTION_EVENT_READY                   0x0105UL
#define FUNCTION_EVENT_CCAIDLE                 0x0106UL
#define FUNCTION_EVENT_CCABUSY                 0x0107UL
#define FUNCTION_EVENT_EDEND                   0x0108UL
#define FUNCTION_EVENT_PHYEND                  0x0109UL
#define FUNCTION_EVENT_CRCOK                   0x010AUL
#define FUNCTION_EVENT_CRCERROR                0x010BUL

#define FUNCTION_AUTO_ACK_ABORT                0x0201UL
#define FUNCTION_TIMESLOT_STARTED              0x0202UL
#define FUNCTION_TIMESLOT_ENDED                0x0203UL
#define FUNCTION_CRIT_SECT_ENTER               0x0204UL
#define FUNCTION_CRIT_SECT_EXIT                0x0205UL

#define FUNCTION_RAAL_CRIT_SECT_ENTER          0x0301UL
#define FUNCTION_RAAL_CRIT_SECT_EXIT           0x0302UL
#define FUNCTION_RAAL_CONTINUOUS_ENTER         0x0303UL
#define FUNCTION_RAAL_CONTINUOUS_EXIT          0x0304UL

#define FUNCTION_RAAL_SIG_HANDLER              0x0400UL
#define FUNCTION_RAAL_SIG_EVENT_START          0x0401UL
#define FUNCTION_RAAL_SIG_EVENT_MARGIN         0x0402UL
#define FUNCTION_RAAL_SIG_EVENT_EXTEND         0x0403UL
#define FUNCTION_RAAL_SIG_EVENT_ENDED          0x0404UL
#define FUNCTION_RAAL_SIG_EVENT_RADIO          0x0405UL
#define FUNCTION_RAAL_SIG_EVENT_EXTEND_SUCCESS 0x0406UL
#define FUNCTION_RAAL_SIG_EVENT_EXTEND_FAIL    0x0407UL
#define FUNCTION_RAAL_EVT_BLOCKED              0x0408UL
#define FUNCTION_RAAL_EVT_SESSION_IDLE         0x0409UL
#define FUNCTION_RAAL_EVT_HFCLK_READY          0x040AUL
#define FUNCTION_RAAL_SIG_EVENT_MARGIN_MOVE    0x040BUL

#define FUNCTION_RSCH_CONTINUOUS_ENTER         0x0480UL
#define FUNCTION_RSCH_CONTINUOUS_EXIT          0x0481UL
#define FUNCTION_RSCH_CRITICAL_SECTION_ENTER   0x0482UL
#define FUNCTION_RSCH_CRITICAL_SECTION_EXIT    0x0483UL
#define FUNCTION_RSCH_TIMESLOT_STARTED         0x0484UL
#define FUNCTION_RSCH_TIMESLOT_ENDED           0x0485UL
#define FUNCTION_RSCH_PEND_GRANTED             0x0486UL
#define FUNCTION_RSCH_PEND_REVOKED             0x0487UL
#define FUNCTION_RSCH_NOTIFY_GRANTED           0x0488UL
#define FUNCTION_RSCH_NOTIFY_REVOKED           0x0489UL
#define FUNCTION_RSCH_NOTIFY_IF_PENDING        0x048AUL
#define FUNCTION_RSCH_DELAYED_TIMESLOT_REQ     0x048BUL
#define FUNCTION_RSCH_TIMER_DELAYED_PREC       0x048CUL
#define FUNCTION_RSCH_TIMER_DELAYED_START      0x048DUL

#define FUNCTION_CSMA_ABORT                    0x0500UL
#define FUNCTION_CSMA_TX_FAILED                0x0501UL
#define FUNCTION_CSMA_TX_STARTED               0x0502UL
#define FUNCTION_CSMA_CHANNEL_BUSY             0x0503UL
#define FUNCTION_CSMA_FRAME_TRANSMIT           0x0504UL

#define FUNCTION_TSCH_ADD                      0x0600UL
#define FUNCTION_TSCH_FIRED                    0x0601UL

#define PIN_DBG_RADIO_EVT_END                  11
#define PIN_DBG_RADIO_EVT_DISABLED             12
#define PIN_DBG_RADIO_EVT_READY                13
#define PIN_DBG_RADIO_EVT_FRAMESTART           14
#define PIN_DBG_RADIO_EVT_EDEND                25
#define PIN_DBG_RADIO_EVT_PHYEND               24

#define PIN_DBG_TIMESLOT_ACTIVE                3
#define PIN_DBG_TIMESLOT_EXTEND_REQ            4
#define PIN_DBG_TIMESLOT_SESSION_IDLE          16
#define PIN_DBG_TIMESLOT_RADIO_IRQ             28
#define PIN_DBG_TIMESLOT_FAILED                29
#define PIN_DBG_TIMESLOT_BLOCKED               30
#define PIN_DBG_RAAL_CRITICAL_SECTION          15

#define PIN_DBG_RTC0_EVT_REM                   31

#if ENABLE_DEBUG_LOG
extern volatile uint32_t nrf_802154_debug_log_buffer[
    NRF_802154_DEBUG_LOG_BUFFER_LEN];
extern volatile uint32_t nrf_802154_debug_log_ptr;

#define nrf_802154_log(EVENT_CODE, EVENT_ARG)                                    \
    do                                                                           \
    {                                                                            \
        uint32_t ptr = nrf_802154_debug_log_ptr;                                 \
                                                                                 \
        nrf_802154_debug_log_buffer[ptr] = ((EVENT_CODE) | ((EVENT_ARG) << 16)); \
        nrf_802154_debug_log_ptr         =                                       \
            ptr < (NRF_802154_DEBUG_LOG_BUFFER_LEN - 1) ? ptr + 1 : 0;           \
    }                                                                            \
    while (0)

#else // ENABLE_DEBUG_LOG

#define nrf_802154_log(EVENT_CODE, EVENT_ARG) (void)(EVENT_ARG)

#endif // ENABLE_DEBUG_LOG

#if ENABLE_DEBUG_GPIO

#define nrf_802154_pin_set(pin) NRF_P0->OUTSET = (1UL << (pin))
#define nrf_802154_pin_clr(pin) NRF_P0->OUTCLR = (1UL << (pin))
#define nrf_802154_pin_tgl(pin)                  \
    do                                           \
    {                                            \
        volatile uint32_t ps = NRF_P0->OUT;      \
                                                 \
        NRF_P0->OUTSET = (~ps & (1UL << (pin))); \
        NRF_P0->OUTCLR = (ps & (1UL << (pin)));  \
    }                                            \
    while (0);

#else // ENABLE_DEBUG_GPIO

#define nrf_802154_pin_set(pin)
#define nrf_802154_pin_clr(pin)
#define nrf_802154_pin_tgl(pin)

#endif // ENABLE_DEBUG_GPIO

/**
 * @brief Initialize debug helpers for nRF 802.15.4 driver.
 */
void nrf_802154_debug_init(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_DEBUG_H_ */
