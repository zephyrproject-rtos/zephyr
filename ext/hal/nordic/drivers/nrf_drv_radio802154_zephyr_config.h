/* Copyright (c) 2016, Nordic Semiconductor ASA
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

#ifndef NRF_RADIO802154_ZEPHYR_CONFIG_H_
#define NRF_RADIO802154_ZEPHYR_CONFIG_H_

// Number of slots containing short addresses of nodes for which
// pending data is stored.
#define RADIO_PENDING_SHORT_ADDRESSES 1

// Number of slots containing extended addresses of nodes for which
// pending data is stored.
#define RADIO_PENDING_EXTENDED_ADDRESSES 1

// Number of buffers in receive queue.
#define RADIO_RX_BUFFERS 1

// CCA mode
#if defined(CONFIG_IEEE802154_NRF5_CCA_MODE_ED)
#define RADIO_CCA_MODE NRF_RADIO_CCA_MODE_ED
#elif defined(CONFIG_IEEE802154_NRF5_CCA_MODE_CARRIER)
#define RADIO_CCA_MODE NRF_RADIO_CCA_MODE_CARRIER
#elif defined(CONFIG_IEEE802154_NRF5_CCA_MODE_CARRIER_AND_ED)
#define RADIO_CCA_MODE  NRF_RADIO_CCA_MODE_CARRIER_AND_ED
#elif defined(CONFIG_IEEE802154_NRF5_CCA_MODE_CARRIER_OR_ED)
#define RADIO_CCA_MODE  NRF_RADIO_CCA_MODE_CARRIER_OR_ED
#endif

// CCA mode options
#define RADIO_CCA_CORR_LIMIT     CONFIG_IEEE802154_NRF5_CCA_CORR_LIMIT
#define RADIO_CCA_CORR_THRESHOLD CONFIG_IEEE802154_NRF5_CCA_CORR_THRESHOLD
#define RADIO_CCA_ED_THRESHOLD   CONFIG_IEEE802154_NRF5_CCA_ED_THRESHOLD

#endif
