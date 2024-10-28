/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Set of macros related with Radio packet configuration flags */
/* PDU type, 2 bit field*/
#define RADIO_PKT_CONF_PDU_TYPE_POS (0U)
#define RADIO_PKT_CONF_PDU_TYPE_MSK (BIT_MASK(2U))
#define RADIO_PKT_CONF_PDU_TYPE_AC  (0U)
#define RADIO_PKT_CONF_PDU_TYPE_DC  (1U)
#define RADIO_PKT_CONF_PDU_TYPE_BIS (2U)
#define RADIO_PKT_CONF_PDU_TYPE_CIS (3U)
/* PHY type, three bit field */
#define RADIO_PKT_CONF_PHY_POS      (2U)
#define RADIO_PKT_CONF_PHY_MSK      (BIT_MASK(3U))
#define RADIO_PKT_CONF_PHY_LEGACY   (0U)
#define RADIO_PKT_CONF_PHY_1M       (BIT(0U))
#define RADIO_PKT_CONF_PHY_2M       (BIT(1U))
#define RADIO_PKT_CONF_PHY_CODED    (BIT(2U))
/* CTE enabled, 1 bit field */
#define RADIO_PKT_CONF_CTE_POS      (5U)
#define RADIO_PKT_CONF_CTE_MSK      (BIT_MASK(1U))
#define RADIO_PKT_CONF_CTE_DISABLED (0U)
#define RADIO_PKT_CONF_CTE_ENABLED  (1U)

/* Macro to define length of the BLE packet length field in bits */
#define RADIO_PKT_CONF_LENGTH_8BIT (8U)
#define RADIO_PKT_CONF_LENGTH_5BIT (5U)

/* Macro to define length of the BLE packet S1 field in bits */
#define RADIO_PKT_CONF_S1_8BIT (8U)

/* Helper macro to create bitfield with PDU type only*/
#define RADIO_PKT_CONF_PDU_TYPE(phy) ((uint8_t)((phy) << RADIO_PKT_CONF_PDU_TYPE_POS))
/* Helper macro to get PDU type from radio packet configuration bitfield */
#define RADIO_PKT_CONF_PDU_TYPE_GET(flags)                                                         \
	((uint8_t)(((flags) >> RADIO_PKT_CONF_PDU_TYPE_POS) & RADIO_PKT_CONF_PDU_TYPE_MSK))
/* Helper macro to create bitfield with PHY type only */
#define RADIO_PKT_CONF_PHY(phy) ((uint8_t)((phy) << RADIO_PKT_CONF_PHY_POS))
/* Helper macro to get PHY type from radio packet configuration bitfield */
#define RADIO_PKT_CONF_PHY_GET(flags)                                                              \
	((uint8_t)((((flags) >> RADIO_PKT_CONF_PHY_POS)) & RADIO_PKT_CONF_PHY_MSK))
/* Helper macro to create bitfield with CTE type only */
#define RADIO_PKT_CONF_CTE(phy) ((uint8_t)((phy) << RADIO_PKT_CONF_CTE_POS))
/* Helper macro to get CTE enable field value from radio packet configuration bitfield */
#define RADIO_PKT_CONF_CTE_GET(flags)                                                              \
	((uint8_t)((((flags) >> RADIO_PKT_CONF_CTE_POS)) & RADIO_PKT_CONF_CTE_MSK))
/* Helper macro to create a radio packet configure bitfield */
#define RADIO_PKT_CONF_FLAGS(pdu, phy, cte)                                                        \
	(RADIO_PKT_CONF_PDU_TYPE((pdu)) | RADIO_PKT_CONF_PHY((phy)) | RADIO_PKT_CONF_CTE((cte)))

enum radio_end_evt_delay_state { END_EVT_DELAY_DISABLED, END_EVT_DELAY_ENABLED };

typedef void (*radio_isr_cb_t) (void *param);

void isr_radio(void);
void radio_isr_set(radio_isr_cb_t cb, void *param);

void radio_setup(void);
void radio_reset(void);
void radio_stop(void);
void radio_phy_set(uint8_t phy, uint8_t flags);
void radio_tx_power_set(int8_t power);
void radio_tx_power_max_set(void);
int8_t radio_tx_power_min_get(void);
int8_t radio_tx_power_max_get(void);
int8_t radio_tx_power_floor(int8_t power);
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
uint32_t radio_is_address(void);
uint32_t radio_is_done(void);
uint32_t radio_is_tx_done(void);
uint32_t radio_has_disabled(void);
uint32_t radio_is_idle(void);

void radio_crc_configure(uint32_t polynomial, uint32_t iv);
uint32_t radio_crc_is_valid(void);

void *radio_pkt_empty_get(void);
void *radio_pkt_scratch_get(void);
void *radio_pkt_decrypt_get(void);
void *radio_pkt_big_ctrl_get(void);

void radio_switch_complete_and_rx(uint8_t phy_rx);
void radio_switch_complete_and_tx(uint8_t phy_rx, uint8_t flags_rx, uint8_t phy_tx,
				  uint8_t flags_tx);
void radio_switch_complete_with_delay_compensation_and_tx(
	uint8_t phy_rx, uint8_t flags_rx, uint8_t phy_tx, uint8_t flags_tx,
	enum radio_end_evt_delay_state end_evt_delay_en);
void radio_switch_complete_and_b2b_tx(uint8_t phy_curr, uint8_t flags_curr,
				      uint8_t phy_next, uint8_t flags_next);
void radio_switch_complete_and_b2b_rx(uint8_t phy_curr, uint8_t flags_curr,
				      uint8_t phy_next, uint8_t flags_next);
void radio_switch_complete_and_b2b_tx_disable(void);
void radio_switch_complete_and_b2b_rx_disable(void);
void radio_switch_complete_and_disable(void);

uint8_t radio_phy_flags_rx_get(void);

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

void isr_radio_tmr(void);
uint32_t radio_tmr_isr_set(uint32_t start_us, radio_isr_cb_t cb, void *param);

void radio_tmr_status_reset(void);
void radio_tmr_tx_status_reset(void);
void radio_tmr_rx_status_reset(void);
void radio_tmr_tx_enable(void);
void radio_tmr_rx_enable(void);
void radio_tmr_tx_disable(void);
void radio_tmr_rx_disable(void);
void radio_tmr_tifs_set(uint32_t tifs);
uint32_t radio_tmr_start(uint8_t trx, uint32_t ticks_start, uint32_t remainder);
uint32_t radio_tmr_start_tick(uint8_t trx, uint32_t ticks_start);
uint32_t radio_tmr_start_us(uint8_t trx, uint32_t us);
uint32_t radio_tmr_start_now(uint8_t trx);
uint32_t radio_tmr_start_get(void);
void radio_tmr_stop(void);
void radio_tmr_hcto_configure(uint32_t hcto);
void radio_tmr_aa_capture(void);
uint32_t radio_tmr_aa_get(void);
void radio_tmr_aa_save(uint32_t aa);
uint32_t radio_tmr_aa_restore(void);
uint32_t radio_tmr_ready_get(void);
void radio_tmr_ready_save(uint32_t ready);
uint32_t radio_tmr_ready_restore(void);
void radio_tmr_end_capture(void);
uint32_t radio_tmr_end_get(void);
uint32_t radio_tmr_tifs_base_get(void);
void radio_tmr_sample(void);
uint32_t radio_tmr_sample_get(void);

int radio_gpio_pa_lna_init(void);
void radio_gpio_pa_lna_deinit(void);
void radio_gpio_pa_setup(void);
void radio_gpio_lna_setup(void);
void radio_gpio_pdn_setup(void);
void radio_gpio_lna_on(void);
void radio_gpio_lna_off(void);
void radio_gpio_pa_lna_enable(uint32_t trx_us);
void radio_gpio_pa_lna_disable(void);

void *radio_ccm_rx_pkt_set(struct ccm *ccm, uint8_t phy, void *pkt);
void *radio_ccm_iso_rx_pkt_set(struct ccm *ccm, uint8_t phy, uint8_t pdu_type, void *pkt);
void *radio_ccm_tx_pkt_set(struct ccm *ccm, void *pkt);
void *radio_ccm_iso_tx_pkt_set(struct ccm *ccm, uint8_t pdu_type, void *pkt);
uint32_t radio_ccm_is_done(void);
uint32_t radio_ccm_mic_is_valid(void);

void radio_ar_configure(uint32_t nirk, void *irk, uint8_t flags);
uint32_t radio_ar_match_get(void);
void radio_ar_status_reset(void);
uint32_t radio_ar_has_match(void);
uint8_t radio_ar_resolve(const uint8_t *addr);

/* Enables CTE inline configuration to automatically setup sampling and
 * switching according to CTEInfo in received PDU.
 */
void radio_df_cte_inline_set_enabled(bool cte_info_in_s1);
