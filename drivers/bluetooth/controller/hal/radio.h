/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _RADIO_H_
#define _RADIO_H_

/* Ramp up times from OPS.
 */
#define RADIO_TX_READY_DELAY_US 140
#define RADIO_RX_READY_DELAY_US	138

/* Chain delays from OPS.
 * nRF51: Tx= 1us, Rx= 3us;
 * nRF52: Tx= 0.6us, Rx= 9.4us.
 */
#define RADIO_TX_CHAIN_DELAY_US 1
#define RADIO_RX_CHAIN_DELAY_US	10

/* Radio Pkt Size */
#define RADIO_EMPDU_SIZE_MAX 3
#define RADIO_ACPDU_SIZE_OVERHEAD 3
#define RADIO_ACPDU_SIZE_MAX (37 + RADIO_ACPDU_SIZE_OVERHEAD)

typedef void (*radio_isr_fp) (void);

void radio_isr(void);
void radio_isr_set(radio_isr_fp fp_radio_isr);

void radio_reset(void);
void radio_phy_set(uint8_t phy);
void radio_tx_power_set(uint32_t power);
void radio_freq_chnl_set(uint32_t chnl);
void radio_whiten_iv_set(uint32_t iv);
void radio_aa_set(uint8_t *aa);
void radio_pkt_configure(uint8_t preamble16, uint8_t bits_len, uint8_t max_len);
void radio_pkt_rx_set(void *rx_packet);
void radio_pkt_tx_set(void *tx_packet);
void radio_rx_enable(void);
void radio_tx_enable(void);
void radio_disable(void);

void radio_status_reset(void);
uint32_t radio_is_ready(void);
uint32_t radio_is_done(void);
uint32_t radio_has_disabled(void);
uint32_t radio_is_idle(void);

void radio_crc_configure(uint32_t polynomial, uint32_t iv);
uint32_t radio_crc_is_valid(void);

void *radio_pkt_empty_get(void);
void *radio_pkt_scratch_get(void);

void radio_switch_complete_and_rx(void);
void radio_switch_complete_and_tx(void);
void radio_switch_complete_and_disable(void);

void radio_rssi_measure(void);
uint32_t radio_rssi_get(void);
void radio_rssi_status_reset(void);
uint32_t radio_rssi_is_ready(void);

void radio_filter_configure(uint8_t bitmask_enable,
				uint8_t bitmask_addr_type,
				uint8_t *bdaddr);
void radio_filter_disable(void);
void radio_filter_status_reset(void);
uint32_t radio_filter_has_match(void);

void radio_bc_configure(uint32_t n);
void radio_bc_status_reset(void);
uint32_t radio_bc_has_match(void);

void radio_tmr_status_reset(void);
void radio_tmr_tifs_set(uint32_t tifs);
uint32_t radio_tmr_start(uint8_t trx, uint32_t ticks_start, uint32_t remainder);
void radio_tmr_stop(void);
void radio_tmr_hcto_configure(uint32_t hcto);
void radio_tmr_aa_capture(void);
uint32_t radio_tmr_aa_get(void);
void radio_tmr_end_capture(void);
uint32_t radio_tmr_end_get(void);

void *radio_ccm_rx_pkt_set(struct ccm *ccm, void *pkt);
void *radio_ccm_tx_pkt_set(struct ccm *ccm, void *pkt);
uint32_t radio_ccm_is_done(void);
uint32_t radio_ccm_mic_is_valid(void);

void radio_ar_configure(uint32_t nirk, void *irk);
uint32_t radio_ar_match_get(void);
void radio_ar_status_reset(void);
uint32_t radio_ar_has_match(void);

#endif
