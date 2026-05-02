/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define RADIO_TXP_DEFAULT 0


typedef void (*radio_isr_cb_t) (void *param);

void isr_radio(void *arg);
void radio_isr_set(radio_isr_cb_t cb, void *param);

void radio_setup(void);
void radio_reset(void);
void radio_phy_set(uint8_t phy, uint8_t flags);
void radio_tx_power_set(uint32_t power);
void radio_tx_power_max_set(void);
void radio_freq_chan_set(uint32_t chan);
void radio_whiten_iv_set(uint32_t iv);
void radio_aa_set(const uint8_t *aa);
void radio_pkt_configure(uint8_t bits_len, uint8_t max_len, uint8_t flags);
void radio_pkt_rx_set(void *rx_packet);
void radio_pkt_tx_set(void *tx_packet);
uint32_t radio_tx_ready_delay_get(uint8_t phy, uint8_t flags);
uint32_t radio_tx_chain_delay_get(uint8_t phy, uint8_t flags);
uint32_t radio_rx_ready_delay_get(uint8_t phy, uint8_t flags);
uint32_t radio_rx_chain_delay_get(uint8_t phy, uint8_t flags);
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

void radio_switch_complete_and_rx(uint8_t phy_rx);
void radio_switch_complete_and_tx(uint8_t phy_rx, uint8_t flags_rx, uint8_t phy_tx,
				  uint8_t flags_tx);
void radio_switch_complete_and_disable(void);

void radio_rssi_measure(void);
uint32_t radio_rssi_get(void);
void radio_rssi_status_reset(void);
uint32_t radio_rssi_is_ready(void);

void radio_filter_configure(uint8_t bitmask_enable, uint8_t bitmask_addr_type,
			    uint8_t *bdaddr);
void radio_filter_disable(void);
void radio_filter_status_reset(void);
uint32_t radio_filter_has_match(void);
uint32_t radio_filter_match_get(void);

void radio_bc_configure(uint32_t n);
void radio_bc_status_reset(void);
uint32_t radio_bc_has_match(void);

void radio_tmr_status_reset(void);
void radio_tmr_tifs_set(uint32_t tifs);
uint32_t radio_tmr_start(uint8_t trx, uint32_t ticks_start, uint32_t remainder);
uint32_t radio_tmr_start_tick(uint8_t trx, uint32_t tick);
void radio_tmr_start_us(uint8_t trx, uint32_t us);
uint32_t radio_tmr_start_now(uint8_t trx);
uint32_t radio_tmr_start_get(void);
void radio_tmr_stop(void);
void radio_tmr_hcto_configure(uint32_t hcto);
void radio_tmr_aa_capture(void);
uint32_t radio_tmr_aa_get(void);
void radio_tmr_aa_save(uint32_t aa);
uint32_t radio_tmr_aa_restore(void);
uint32_t radio_tmr_ready_get(void);
void radio_tmr_end_capture(void);
uint32_t radio_tmr_end_get(void);
uint32_t radio_tmr_tifs_base_get(void);
void radio_tmr_sample(void);
uint32_t radio_tmr_sample_get(void);

void radio_gpio_pa_setup(void);
void radio_gpio_lna_setup(void);
void radio_gpio_lna_on(void);
void radio_gpio_lna_off(void);
void radio_gpio_pa_lna_enable(uint32_t trx_us);
void radio_gpio_pa_lna_disable(void);

void *radio_ccm_rx_pkt_set(struct ccm *ccm, uint8_t phy, void *pkt);
void *radio_ccm_tx_pkt_set(struct ccm *ccm, void *pkt);
uint32_t radio_ccm_is_done(void);
uint32_t radio_ccm_mic_is_valid(void);
uint32_t radio_ccm_is_available(void);

void radio_ar_configure(uint32_t nirk, void *irk);
uint32_t radio_ar_match_get(void);
void radio_ar_status_reset(void);
uint32_t radio_ar_has_match(void);

uint32_t radio_sleep(void);
uint32_t radio_wake(void);
uint32_t radio_is_off(void);
