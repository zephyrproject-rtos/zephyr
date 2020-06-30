/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LL_VERSION_NUMBER BT_HCI_VERSION_5_2

#define LL_ADV_CMDS_ANY    0 /* Any advertising cmd/evt allowed */
#define LL_ADV_CMDS_LEGACY 1 /* Only legacy advertising cmd/evt allowed */
#define LL_ADV_CMDS_EXT    2 /* Only extended advertising cmd/evt allowed */

int ll_init(struct k_sem *sem_rx);
void ll_reset(void);

uint8_t *ll_addr_get(uint8_t addr_type, uint8_t *p_bdaddr);
uint8_t ll_addr_set(uint8_t addr_type, uint8_t const *const p_bdaddr);

#if defined(CONFIG_BT_CTLR_HCI_ADV_HANDLE_MAPPING)
uint8_t ll_adv_set_by_hci_handle_get(uint8_t hci_handle, uint8_t *handle);
uint8_t ll_adv_set_by_hci_handle_get_or_new(uint8_t hci_handle,
					    uint8_t *handle);
#else
static inline uint8_t ll_adv_set_by_hci_handle_get(uint8_t hci_handle,
						   uint8_t *handle)
{
	*handle = hci_handle;
	return 0;
}

static inline uint8_t ll_adv_set_by_hci_handle_get_or_new(uint8_t hci_handle,
							  uint8_t *handle)
{
	*handle = hci_handle;
	return 0;
}
#endif

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_HCI_RAW)
int ll_adv_cmds_set(uint8_t adv_cmds);
int ll_adv_cmds_is_ext(void);
#else
static inline int ll_adv_cmds_is_ext(void)
{
	return 1;
}
#endif /* CONFIG_BT_HCI_RAW */

uint8_t ll_adv_params_set(uint8_t handle, uint16_t evt_prop, uint32_t interval,
		       uint8_t adv_type, uint8_t own_addr_type,
		       uint8_t direct_addr_type, uint8_t const *const direct_addr,
		       uint8_t chan_map, uint8_t filter_policy,
		       uint8_t *const tx_pwr, uint8_t phy_p, uint8_t skip,
		       uint8_t phy_s, uint8_t sid, uint8_t sreq);
uint8_t ll_adv_data_set(uint8_t handle, uint8_t len,
			uint8_t const *const p_data);
uint8_t ll_adv_scan_rsp_set(uint8_t handle, uint8_t len,
			    uint8_t const *const p_data);
uint8_t ll_adv_aux_random_addr_set(uint8_t handle, uint8_t const *const addr);
uint8_t ll_adv_aux_ad_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref,
			       uint8_t len, uint8_t const *const data);
uint8_t ll_adv_aux_sr_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref,
			       uint8_t len, uint8_t const *const data);
uint16_t ll_adv_aux_max_data_length_get(void);
uint8_t ll_adv_aux_set_count_get(void);
uint8_t ll_adv_aux_set_remove(uint8_t handle);
uint8_t ll_adv_aux_set_clear(void);
uint8_t ll_adv_sync_param_set(uint8_t handle, uint16_t interval,
			      uint16_t flags);
uint8_t ll_adv_sync_ad_data_set(uint8_t handle, uint8_t op, uint8_t len,
				uint8_t const *const data);
uint8_t ll_adv_sync_enable(uint8_t handle, uint8_t enable);
#else /* !CONFIG_BT_CTLR_ADV_EXT */
uint8_t ll_adv_params_set(uint16_t interval, uint8_t adv_type,
		       uint8_t own_addr_type, uint8_t direct_addr_type,
		       uint8_t const *const direct_addr, uint8_t chan_map,
		       uint8_t filter_policy);
uint8_t ll_adv_data_set(uint8_t len, uint8_t const *const p_data);
uint8_t ll_adv_scan_rsp_set(uint8_t len, uint8_t const *const p_data);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_ADV_EXT) || defined(CONFIG_BT_HCI_MESH_EXT)
#if defined(CONFIG_BT_HCI_MESH_EXT)
uint8_t ll_adv_enable(uint8_t handle, uint8_t enable,
		   uint8_t at_anchor, uint32_t ticks_anchor, uint8_t retry,
		   uint8_t scan_window, uint8_t scan_delay);
#else /* !CONFIG_BT_HCI_MESH_EXT */
uint8_t ll_adv_enable(uint8_t handle, uint8_t enable,
		   uint16_t duration, uint8_t max_ext_adv_evts);
#endif /* !CONFIG_BT_HCI_MESH_EXT */
#else /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */
uint8_t ll_adv_enable(uint8_t enable);
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */

uint8_t ll_scan_params_set(uint8_t type, uint16_t interval, uint16_t window,
			uint8_t own_addr_type, uint8_t filter_policy);
uint8_t ll_scan_enable(uint8_t enable);

uint8_t ll_wl_size_get(void);
uint8_t ll_wl_clear(void);
uint8_t ll_wl_add(bt_addr_le_t *addr);
uint8_t ll_wl_remove(bt_addr_le_t *addr);

void ll_rl_id_addr_get(uint8_t rl_idx, uint8_t *id_addr_type, uint8_t *id_addr);
uint8_t ll_rl_size_get(void);
uint8_t ll_rl_clear(void);
uint8_t ll_rl_add(bt_addr_le_t *id_addr, const uint8_t pirk[16],
		const uint8_t lirk[16]);
uint8_t ll_rl_remove(bt_addr_le_t *id_addr);
void ll_rl_crpa_set(uint8_t id_addr_type, uint8_t *id_addr, uint8_t rl_idx, uint8_t *crpa);
uint8_t ll_rl_crpa_get(bt_addr_le_t *id_addr, bt_addr_t *crpa);
uint8_t ll_rl_lrpa_get(bt_addr_le_t *id_addr, bt_addr_t *lrpa);
uint8_t ll_rl_enable(uint8_t enable);
void  ll_rl_timeout_set(uint16_t timeout);
uint8_t ll_priv_mode_set(bt_addr_le_t *id_addr, uint8_t mode);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
uint8_t ll_create_connection(uint16_t scan_interval, uint16_t scan_window,
			  uint8_t filter_policy, uint8_t peer_addr_type,
			  uint8_t const *const peer_addr, uint8_t own_addr_type,
			  uint16_t interval, uint16_t latency, uint16_t timeout,
			  uint8_t phy);
uint8_t ll_connect_enable(uint8_t is_coded_included);
#else /* !CONFIG_BT_CTLR_ADV_EXT */
uint8_t ll_create_connection(uint16_t scan_interval, uint16_t scan_window,
			  uint8_t filter_policy, uint8_t peer_addr_type,
			  uint8_t const *const peer_addr, uint8_t own_addr_type,
			  uint16_t interval, uint16_t latency, uint16_t timeout);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
uint8_t ll_connect_disable(void **rx);
uint8_t ll_conn_update(uint16_t handle, uint8_t cmd, uint8_t status, uint16_t interval_min,
		    uint16_t interval_max, uint16_t latency, uint16_t timeout);
uint8_t ll_chm_update(uint8_t const *const chm);
uint8_t ll_chm_get(uint16_t handle, uint8_t *const chm);
uint8_t ll_enc_req_send(uint16_t handle, uint8_t const *const rand,
		     uint8_t const *const ediv, uint8_t const *const ltk);
uint8_t ll_start_enc_req_send(uint16_t handle, uint8_t err_code,
			   uint8_t const *const ltk);
uint8_t ll_feature_req_send(uint16_t handle);
uint8_t ll_version_ind_send(uint16_t handle);
uint8_t ll_terminate_ind_send(uint16_t handle, uint8_t reason);
uint8_t ll_rssi_get(uint16_t handle, uint8_t *const rssi);
uint8_t ll_tx_pwr_lvl_get(uint8_t handle_type,
		       uint16_t handle, uint8_t type, int8_t *const tx_pwr_lvl);
void ll_tx_pwr_get(int8_t *const min, int8_t *const max);
uint8_t ll_tx_pwr_lvl_set(uint8_t handle_type, uint16_t handle,
			  int8_t *const tx_pwr_lvl);

uint8_t ll_apto_get(uint16_t handle, uint16_t *const apto);
uint8_t ll_apto_set(uint16_t handle, uint16_t apto);

uint32_t ll_length_req_send(uint16_t handle, uint16_t tx_octets, uint16_t tx_time);
void ll_length_default_get(uint16_t *const max_tx_octets,
			   uint16_t *const max_tx_time);
uint32_t ll_length_default_set(uint16_t max_tx_octets, uint16_t max_tx_time);
void ll_length_max_get(uint16_t *const max_tx_octets,
		       uint16_t *const max_tx_time,
		       uint16_t *const max_rx_octets,
		       uint16_t *const max_rx_time);

uint8_t ll_phy_get(uint16_t handle, uint8_t *const tx, uint8_t *const rx);
uint8_t ll_phy_default_set(uint8_t tx, uint8_t rx);
uint8_t ll_phy_req_send(uint16_t handle, uint8_t tx, uint8_t flags, uint8_t rx);

/* Downstream - Data */
void *ll_tx_mem_acquire(void);
void ll_tx_mem_release(void *node_tx);
int ll_tx_mem_enqueue(uint16_t handle, void *node_tx);

/* Upstream - Num. Completes, Events and Data */
uint8_t ll_rx_get(void **node_rx, uint16_t *handle);
void ll_rx_dequeue(void);
void ll_rx_mem_release(void **node_rx);

/* External co-operation */
void ll_timeslice_ticker_id_get(uint8_t * const instance_index, uint8_t * const user_id);
void ll_radio_state_abort(void);
uint32_t ll_radio_state_is_idle(void);
