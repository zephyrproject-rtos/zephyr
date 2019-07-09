/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LL_VERSION_NUMBER BT_HCI_VERSION_5_1

int ll_init(struct k_sem *sem_rx);
void ll_reset(void);

u8_t *ll_addr_get(u8_t addr_type, u8_t *p_bdaddr);
u8_t ll_addr_set(u8_t addr_type, u8_t const *const p_bdaddr);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
u8_t ll_adv_params_set(u8_t handle, u16_t evt_prop, u32_t interval,
		       u8_t adv_type, u8_t own_addr_type,
		       u8_t direct_addr_type, u8_t const *const direct_addr,
		       u8_t chan_map, u8_t filter_policy, u8_t *tx_pwr,
		       u8_t phy_p, u8_t skip, u8_t phy_s, u8_t sid, u8_t sreq);
u8_t ll_adv_data_set(u16_t handle, u8_t len, u8_t const *const p_data);
u8_t ll_adv_scan_rsp_set(u16_t handle, u8_t len, u8_t const *const p_data);
#else /* !CONFIG_BT_CTLR_ADV_EXT */
u8_t ll_adv_params_set(u16_t interval, u8_t adv_type,
		       u8_t own_addr_type, u8_t direct_addr_type,
		       u8_t const *const direct_addr, u8_t chan_map,
		       u8_t filter_policy);
u8_t ll_adv_data_set(u8_t len, u8_t const *const p_data);
u8_t ll_adv_scan_rsp_set(u8_t len, u8_t const *const p_data);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_ADV_EXT) || defined(CONFIG_BT_HCI_MESH_EXT)
#if defined(CONFIG_BT_HCI_MESH_EXT)
u8_t ll_adv_enable(u16_t handle, u8_t enable,
		   u8_t at_anchor, u32_t ticks_anchor, u8_t retry,
		   u8_t scan_window, u8_t scan_delay);
#else /* !CONFIG_BT_HCI_MESH_EXT */
u8_t ll_adv_enable(u16_t handle, u8_t enable);
#endif /* !CONFIG_BT_HCI_MESH_EXT */
#else /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */
u8_t ll_adv_enable(u8_t enable);
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */

u8_t ll_scan_params_set(u8_t type, u16_t interval, u16_t window,
			u8_t own_addr_type, u8_t filter_policy);
u8_t ll_scan_enable(u8_t enable);

u8_t ll_wl_size_get(void);
u8_t ll_wl_clear(void);
u8_t ll_wl_add(bt_addr_le_t *addr);
u8_t ll_wl_remove(bt_addr_le_t *addr);

void ll_rl_id_addr_get(u8_t rl_idx, u8_t *id_addr_type, u8_t *id_addr);
u8_t ll_rl_size_get(void);
u8_t ll_rl_clear(void);
u8_t ll_rl_add(bt_addr_le_t *id_addr, const u8_t pirk[16],
		const u8_t lirk[16]);
u8_t ll_rl_remove(bt_addr_le_t *id_addr);
void ll_rl_crpa_set(u8_t id_addr_type, u8_t *id_addr, u8_t rl_idx, u8_t *crpa);
u8_t ll_rl_crpa_get(bt_addr_le_t *id_addr, bt_addr_t *crpa);
u8_t ll_rl_lrpa_get(bt_addr_le_t *id_addr, bt_addr_t *lrpa);
u8_t ll_rl_enable(u8_t enable);
void  ll_rl_timeout_set(u16_t timeout);
u8_t ll_priv_mode_set(bt_addr_le_t *id_addr, u8_t mode);

u8_t ll_create_connection(u16_t scan_interval, u16_t scan_window,
			  u8_t filter_policy, u8_t peer_addr_type,
			  u8_t *peer_addr, u8_t own_addr_type,
			  u16_t interval, u16_t latency, u16_t timeout);
u8_t ll_connect_disable(void **rx);
u8_t ll_conn_update(u16_t handle, u8_t cmd, u8_t status, u16_t interval_min,
		    u16_t interval_max, u16_t latency, u16_t timeout);
u8_t ll_chm_update(u8_t *chm);
u8_t ll_chm_get(u16_t handle, u8_t *chm);
u8_t ll_enc_req_send(u16_t handle, u8_t *rand, u8_t *ediv,
		     u8_t *ltk);
u8_t ll_start_enc_req_send(u16_t handle, u8_t err_code,
			   u8_t const *const ltk);
u8_t ll_feature_req_send(u16_t handle);
u8_t ll_version_ind_send(u16_t handle);
u8_t ll_terminate_ind_send(u16_t handle, u8_t reason);
u8_t ll_rssi_get(u16_t handle, u8_t *rssi);
u8_t ll_tx_pwr_lvl_get(u16_t handle, u8_t type, s8_t *tx_pwr_lvl);
void ll_tx_pwr_get(s8_t *min, s8_t *max);

u8_t ll_apto_get(u16_t handle, u16_t *apto);
u8_t ll_apto_set(u16_t handle, u16_t apto);

u32_t ll_length_req_send(u16_t handle, u16_t tx_octets, u16_t tx_time);
void ll_length_default_get(u16_t *max_tx_octets, u16_t *max_tx_time);
u32_t ll_length_default_set(u16_t max_tx_octets, u16_t max_tx_time);
void ll_length_max_get(u16_t *max_tx_octets, u16_t *max_tx_time,
		       u16_t *max_rx_octets, u16_t *max_rx_time);

u8_t ll_phy_get(u16_t handle, u8_t *tx, u8_t *rx);
u8_t ll_phy_default_set(u8_t tx, u8_t rx);
u8_t ll_phy_req_send(u16_t handle, u8_t tx, u8_t flags, u8_t rx);

/* Downstream - Data */
void *ll_tx_mem_acquire(void);
void ll_tx_mem_release(void *node_tx);
int ll_tx_mem_enqueue(u16_t handle, void *node_tx);

/* Upstream - Num. Completes, Events and Data */
u8_t ll_rx_get(void **node_rx, u16_t *handle);
void ll_rx_dequeue(void);
void ll_rx_mem_release(void **node_rx);

/* External co-operation */
void ll_timeslice_ticker_id_get(u8_t * const instance_index, u8_t * const user_id);
void ll_radio_state_abort(void);
u32_t ll_radio_state_is_idle(void);
