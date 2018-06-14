/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

typedef void (*radio_isr_fp) (void);

void isr_radio(void);
void radio_isr_set(radio_isr_fp fp_radio_isr);

void radio_setup(void);
void radio_reset(void);
void radio_phy_set(u8_t phy, u8_t flags);
void radio_tx_power_set(u32_t power);
void radio_tx_power_max_set(void);
void radio_freq_chan_set(u32_t chan);
void radio_whiten_iv_set(u32_t iv);
void radio_aa_set(u8_t *aa);
void radio_pkt_configure(u8_t bits_len, u8_t max_len, u8_t flags);
void radio_pkt_rx_set(void *rx_packet);
void radio_pkt_tx_set(void *tx_packet);
u32_t radio_tx_ready_delay_get(u8_t phy, u8_t flags);
u32_t radio_tx_chain_delay_get(u8_t phy, u8_t flags);
u32_t radio_rx_ready_delay_get(u8_t phy, u8_t flags);
u32_t radio_rx_chain_delay_get(u8_t phy, u8_t flags);
void radio_rx_enable(void);
void radio_tx_enable(void);
void radio_disable(void);

void radio_status_reset(void);
u32_t radio_is_ready(void);
u32_t radio_is_done(void);
u32_t radio_has_disabled(void);
u32_t radio_is_idle(void);

void radio_crc_configure(u32_t polynomial, u32_t iv);
u32_t radio_crc_is_valid(void);

void *radio_pkt_empty_get(void);
void *radio_pkt_scratch_get(void);

void radio_switch_complete_and_rx(u8_t phy_rx);
void radio_switch_complete_and_tx(u8_t phy_rx, u8_t flags_rx, u8_t phy_tx,
				  u8_t flags_tx);
void radio_switch_complete_and_disable(void);

void radio_rssi_measure(void);
u32_t radio_rssi_get(void);
void radio_rssi_status_reset(void);
u32_t radio_rssi_is_ready(void);

void radio_filter_configure(u8_t bitmask_enable, u8_t bitmask_addr_type,
			    u8_t *bdaddr);
void radio_filter_disable(void);
void radio_filter_status_reset(void);
u32_t radio_filter_has_match(void);
u32_t radio_filter_match_get(void);

void radio_bc_configure(u32_t n);
void radio_bc_status_reset(void);
u32_t radio_bc_has_match(void);

void radio_tmr_status_reset(void);
void radio_tmr_tifs_set(u32_t tifs);
u32_t radio_tmr_start(u8_t trx, u32_t ticks_start, u32_t remainder);
void radio_tmr_start_us(u8_t trx, u32_t us);
u32_t radio_tmr_start_now(u8_t trx);
void radio_tmr_stop(void);
void radio_tmr_hcto_configure(u32_t hcto);
void radio_tmr_aa_capture(void);
u32_t radio_tmr_aa_get(void);
void radio_tmr_aa_save(u32_t aa);
u32_t radio_tmr_aa_restore(void);
u32_t radio_tmr_ready_get(void);
void radio_tmr_end_capture(void);
u32_t radio_tmr_end_get(void);
u32_t radio_tmr_tifs_base_get(void);
void radio_tmr_sample(void);
u32_t radio_tmr_sample_get(void);

void radio_gpio_pa_setup(void);
void radio_gpio_lna_setup(void);
void radio_gpio_lna_on(void);
void radio_gpio_lna_off(void);
void radio_gpio_pa_lna_enable(u32_t trx_us);
void radio_gpio_pa_lna_disable(void);

void *radio_ccm_rx_pkt_set(struct ccm *ccm, u8_t phy, void *pkt);
void *radio_ccm_tx_pkt_set(struct ccm *ccm, void *pkt);
u32_t radio_ccm_is_done(void);
u32_t radio_ccm_mic_is_valid(void);

void radio_ar_configure(u32_t nirk, void *irk);
u32_t radio_ar_match_get(void);
void radio_ar_status_reset(void);
u32_t radio_ar_has_match(void);
