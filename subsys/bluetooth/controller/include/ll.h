/*
 * Copyright (c) 2016-2021 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ll_init(struct k_sem *sem_rx);
int ll_deinit(void);
void ll_reset(void);

uint8_t ll_set_host_feature(uint8_t bit_number, uint8_t bit_value);
uint64_t ll_feat_get(void);

uint8_t ll_addr_set(uint8_t addr_type, uint8_t const *const p_bdaddr);
uint8_t *ll_addr_get(uint8_t addr_type);
uint8_t *ll_addr_read(uint8_t addr_type, uint8_t *const bdaddr);

#if defined(CONFIG_BT_CTLR_HCI_ADV_HANDLE_MAPPING)
uint8_t ll_adv_set_by_hci_handle_get(uint8_t hci_handle, uint8_t *handle);
uint8_t ll_adv_set_by_hci_handle_get_or_new(uint8_t hci_handle,
					    uint8_t *handle);
uint8_t ll_adv_set_hci_handle_get(uint8_t handle);
uint8_t ll_adv_iso_by_hci_handle_get(uint8_t hci_handle, uint8_t *handle);
uint8_t ll_adv_iso_by_hci_handle_new(uint8_t hci_handle, uint8_t *handle);
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

static inline uint8_t ll_adv_set_hci_handle_get(uint8_t handle)
{
	return handle;
}

static inline uint8_t ll_adv_iso_by_hci_handle_get(uint8_t hci_handle,
						   uint8_t *handle)
{
	*handle = hci_handle;
	return 0;
}

static inline uint8_t ll_adv_iso_by_hci_handle_new(uint8_t hci_handle,
						   uint8_t *handle)
{
	*handle = hci_handle;
	return 0;
}
#endif

#if defined(CONFIG_BT_CTLR_ADV_EXT)
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
#else /* !CONFIG_BT_CTLR_ADV_EXT */
uint8_t ll_adv_params_set(uint16_t interval, uint8_t adv_type,
		       uint8_t own_addr_type, uint8_t direct_addr_type,
		       uint8_t const *const direct_addr, uint8_t chan_map,
		       uint8_t filter_policy);
uint8_t ll_adv_data_set(uint8_t len, uint8_t const *const p_data);
uint8_t ll_adv_scan_rsp_set(uint8_t len, uint8_t const *const p_data);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

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

uint8_t ll_adv_disable_all(void);

uint8_t ll_big_create(uint8_t big_handle, uint8_t adv_handle, uint8_t num_bis,
		      uint32_t sdu_interval, uint16_t max_sdu,
		      uint16_t max_latency, uint8_t rtn, uint8_t phy,
		      uint8_t packing, uint8_t framing, uint8_t encryption,
		      uint8_t *bcode);
uint8_t ll_big_test_create(uint8_t big_handle, uint8_t adv_handle,
			   uint8_t num_bis, uint32_t sdu_interval,
			   uint16_t iso_interval, uint8_t nse, uint16_t max_sdu,
			   uint16_t max_pdu, uint8_t phy, uint8_t packing,
			   uint8_t framing, uint8_t bn, uint8_t irc,
			   uint8_t pto, uint8_t encryption, uint8_t *bcode);
uint8_t ll_big_terminate(uint8_t big_handle, uint8_t reason);

uint8_t ll_scan_params_set(uint8_t type, uint16_t interval, uint16_t window,
		uint8_t own_addr_type, uint8_t filter_policy);
#if defined(CONFIG_BT_CTLR_ADV_EXT)
uint8_t ll_scan_enable(uint8_t enable, uint16_t duration, uint16_t period);
#else /* !CONFIG_BT_CTLR_ADV_EXT */
uint8_t ll_scan_enable(uint8_t enable);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

uint8_t ll_sync_create(uint8_t options, uint8_t sid, uint8_t adv_addr_type,
		       uint8_t *adv_addr, uint16_t skip,
		       uint16_t sync_timeout, uint8_t sync_cte_type);
uint8_t ll_sync_create_cancel(void **rx);
uint8_t ll_sync_terminate(uint16_t handle);
uint8_t ll_sync_recv_enable(uint16_t handle, uint8_t enable);
uint8_t ll_big_sync_create(uint8_t big_handle, uint16_t sync_handle,
			   uint8_t encryption, uint8_t *bcode, uint8_t mse,
			   uint16_t sync_timeout, uint8_t num_bis,
			   uint8_t *bis);
uint8_t ll_big_sync_terminate(uint8_t big_handle, void **rx);

uint8_t ll_cig_parameters_open(uint8_t cig_id,
			       uint32_t c_interval, uint32_t p_interval,
			       uint8_t sca, uint8_t packing, uint8_t framing,
			       uint16_t c_latency, uint16_t p_latency,
			       uint8_t num_cis);
uint8_t ll_cis_parameters_set(uint8_t cis_id,
			      uint16_t c_sdu, uint16_t p_sdu,
			      uint8_t c_phy, uint8_t p_phy,
			      uint8_t c_rtn, uint8_t p_rtn);
uint8_t ll_cig_parameters_commit(uint8_t cig_id, uint16_t *handles);
uint8_t ll_cig_parameters_test_open(uint8_t cig_id,
				    uint32_t c_interval,
				    uint32_t p_interval,
				    uint8_t c_ft,
				    uint8_t p_ft,
				    uint16_t iso_interval,
				    uint8_t sca,
				    uint8_t packing,
				    uint8_t framing,
				    uint8_t num_cis);
uint8_t ll_cis_parameters_test_set(uint8_t cis_id, uint8_t nse,
				   uint16_t c_sdu, uint16_t p_sdu,
				   uint16_t c_pdu, uint16_t p_pdu,
				   uint8_t c_phy, uint8_t p_phy,
				   uint8_t c_bn, uint8_t p_bn);
/* Must be implemented by vendor if vendor-specific data path is supported */
uint8_t ll_configure_data_path(uint8_t data_path_dir,
			       uint8_t data_path_id,
			       uint8_t vs_config_len,
			       uint8_t  *vs_config);
uint8_t ll_read_iso_tx_sync(uint16_t handle, uint16_t *seq,
			    uint32_t *timestamp, uint32_t *offset);
uint8_t ll_read_iso_link_quality(uint16_t  handle,
				 uint32_t *tx_unacked_packets,
				 uint32_t *tx_flushed_packets,
				 uint32_t *tx_last_subevent_packets,
				 uint32_t *retransmitted_packets,
				 uint32_t *crc_error_packets,
				 uint32_t *rx_unreceived_packets,
				 uint32_t *duplicate_packets);
uint8_t ll_setup_iso_path(uint16_t handle, uint8_t path_dir, uint8_t path_id,
			  uint8_t coding_format, uint16_t company_id,
			  uint16_t vs_codec_id, uint32_t controller_delay,
			  uint8_t codec_config_len, uint8_t *codec_config);
uint8_t ll_remove_iso_path(uint16_t handle, uint8_t path_dir);
uint8_t ll_iso_receive_test(uint16_t handle, uint8_t payload_type);
uint8_t ll_iso_transmit_test(uint16_t handle, uint8_t payload_type);
uint8_t ll_iso_test_end(uint16_t handle, uint32_t *received_cnt,
			uint32_t *missed_cnt, uint32_t *failed_cnt);
uint8_t ll_iso_read_test_counters(uint16_t handle, uint32_t *received_cnt,
				  uint32_t *missed_cnt,
				  uint32_t *failed_cnt);

uint8_t ll_cig_remove(uint8_t cig_id);

uint8_t ll_cis_create_check(uint16_t cis_handle, uint16_t acl_handle);
void ll_cis_create(uint16_t cis_handle, uint16_t acl_handle);

uint8_t ll_cis_accept(uint16_t handle);
uint8_t ll_cis_reject(uint16_t handle, uint8_t reason);

uint8_t ll_fal_size_get(void);
uint8_t ll_fal_clear(void);
uint8_t ll_fal_add(bt_addr_le_t *addr);
uint8_t ll_fal_remove(bt_addr_le_t *addr);

uint8_t ll_pal_size_get(void);
uint8_t ll_pal_clear(void);
uint8_t ll_pal_add(const bt_addr_le_t *const addr, const uint8_t sid);
uint8_t ll_pal_remove(const bt_addr_le_t *const addr, const uint8_t sid);

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
		    uint16_t interval_max, uint16_t latency, uint16_t timeout, uint16_t *offset);
uint8_t ll_chm_update(uint8_t const *const chm);
uint8_t ll_chm_get(uint16_t handle, uint8_t *const chm);
uint8_t ll_enc_req_send(uint16_t handle, uint8_t const *const rand_num, uint8_t const *const ediv,
			uint8_t const *const ltk);
uint8_t ll_start_enc_req_send(uint16_t handle, uint8_t err_code,
			   uint8_t const *const ltk);
uint8_t ll_req_peer_sca(uint16_t handle);
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

uint8_t ll_set_min_used_chans(uint16_t handle, uint8_t const phys,
		     uint8_t const min_used_chans);

/* Direction Finding */
/* Sets CTE transmission parameters for periodic advertising */
uint8_t ll_df_set_cl_cte_tx_params(uint8_t adv_handle, uint8_t cte_len,
				   uint8_t cte_type, uint8_t cte_count,
				   uint8_t num_ant_ids, uint8_t *ant_ids);
/* Enables or disables CTE TX for periodic advertising */
uint8_t ll_df_set_cl_cte_tx_enable(uint8_t adv_handle, uint8_t cte_enable);
/* Provides information about antennae switching and sampling settings */
uint8_t ll_df_set_conn_cte_tx_params(uint16_t handle, uint8_t cte_types,
				     uint8_t switching_patterns_len, const uint8_t *ant_id);
/* Enables or disables CTE sampling in direction fingin connected mode. */
uint8_t ll_df_set_conn_cte_rx_params(uint16_t handle, uint8_t sampling_enable,
				     uint8_t slot_durations, uint8_t switch_pattern_len,
				     const uint8_t *ant_ids);
/* Enables or disables CTE request control procedure in direction fingin connected mode. */
uint8_t ll_df_set_conn_cte_req_enable(uint16_t handle, uint8_t enable,
				      uint16_t cte_request_interval, uint8_t requested_cte_length,
				      uint8_t requested_cte_type);
/* Enables or disables CTE response control procedure in direction fingin connected mode. */
uint8_t ll_df_set_conn_cte_rsp_enable(uint16_t handle, uint8_t enable);
/* Enables or disables CTE sampling in periodic advertising scan */
uint8_t ll_df_set_cl_iq_sampling_enable(uint16_t handle,
					uint8_t sampling_enable,
					uint8_t slot_durations,
					uint8_t max_cte_count,
					uint8_t switch_pattern_len,
					uint8_t *ant_ids);
/* Sets CTE transmission parameters for a connection */
void ll_df_read_ant_inf(uint8_t *switch_sample_rates,
			uint8_t *num_ant,
			uint8_t *max_switch_pattern_len,
			uint8_t *max_cte_len);

/* Downstream - Data */
void *ll_tx_mem_acquire(void);
void ll_tx_mem_release(void *node_tx);
int ll_tx_mem_enqueue(uint16_t handle, void *node_tx);

/* Upstream - Num. Completes, Events and Data */
uint8_t ll_rx_get(void **node_rx, uint16_t *handle);
void ll_rx_dequeue(void);
void ll_rx_mem_release(void **node_rx);
void ll_iso_rx_mem_release(void **node_rx);

/* Downstream - ISO Data */
void *ll_iso_tx_mem_acquire(void);
void ll_iso_tx_mem_release(void *tx);
int ll_iso_tx_mem_enqueue(uint16_t handle, void *tx, void *link);
void ll_iso_link_tx_release(void *link);

uint8_t ll_conn_iso_accept_timeout_get(uint16_t *timeout);
uint8_t ll_conn_iso_accept_timeout_set(uint16_t timeout);

/* External co-operation */
void ll_timeslice_ticker_id_get(uint8_t * const instance_index,
				uint8_t * const ticker_id);
void ll_coex_ticker_id_get(uint8_t * const instance_index,
				uint8_t * const ticker_id);
void ll_radio_state_abort(void);
uint32_t ll_radio_state_is_idle(void);
