/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Temporary data structure to avoid change ll_conn/lll_conn to much
 * and having to update all the dependencies
 */

enum { ULL_CP_CONNECTED, ULL_CP_DISCONNECTED };

/**
 * @brief Initialize the LL Control Procedure system.
 */
void ull_cp_init(void);

/**
 * @brief Initialize the LL Control Procedure connection data.
 */
void ull_llcp_init(struct ll_conn *conn);

/**
 * @brief XXX
 */
void ull_cp_state_set(struct ll_conn *conn, uint8_t state);

void ull_cp_release_nodes(struct ll_conn *conn);

/*
 * @brief Update 'global' tx buffer allowance
 */
void ull_cp_update_tx_buffer_queue(struct ll_conn *conn);

/**
 *
 */
void ull_cp_release_tx(struct ll_conn *conn, struct node_tx *tx);

/**
 * @brief Procedure Response Timeout Check
 * @param elapsed_event The number of elapsed events.
 * @param[out] error_code The error code for this timeout.
 * @return 0 on success, -ETIMEDOUT if timer expired.
 */
int ull_cp_prt_elapse(struct ll_conn *conn, uint16_t elapsed_event, uint8_t *error_code);

void ull_cp_prt_reload_set(struct ll_conn *conn, uint32_t conn_intv);

/**
 * @brief Run pending LL Control Procedures.
 */
void ull_cp_run(struct ll_conn *conn);

/**
 * @brief Handle TX ack PDU.
 */
void ull_cp_tx_ack(struct ll_conn *conn, struct node_tx *tx);

/**
 * @brief Handle TX procedures notifications towards Host.
 */
void ull_cp_tx_ntf(struct ll_conn *conn);

/**
 * @brief Handle received LL Control PDU.
 */
void ull_cp_rx(struct ll_conn *conn, memq_link_t *link, struct node_rx_pdu *rx);

#if defined(CONFIG_BT_CTLR_LE_PING)
/**
 * @brief Initiate a LE Ping Procedure.
 */
uint8_t ull_cp_le_ping(struct ll_conn *conn);
#endif /* CONFIG_BT_CTLR_LE_PING */

/**
 * @brief Initiate a Version Exchange Procedure.
 */
uint8_t ull_cp_version_exchange(struct ll_conn *conn);

/**
 * @brief Initiate a Feature Exchange Procedure.
 */
uint8_t ull_cp_feature_exchange(struct ll_conn *conn, uint8_t host_initiated);

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
/**
 * @brief Initiate a Minimum used channels Procedure.
 */
uint8_t ull_cp_min_used_chans(struct ll_conn *conn, uint8_t phys, uint8_t min_used_chans);
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */

/**
 * @brief Initiate a Encryption Start Procedure.
 */
uint8_t ull_cp_encryption_start(struct ll_conn *conn, const uint8_t rand[8], const uint8_t ediv[2],
				const uint8_t ltk[16]);

/**
 * @brief Initiate a Encryption Pause Procedure.
 */
uint8_t ull_cp_encryption_pause(struct ll_conn *conn, const uint8_t rand[8], const uint8_t ediv[2],
				const uint8_t ltk[16]);

/**
 * @brief Check if an encryption pause procedure is active.
 */
uint8_t ull_cp_encryption_paused(struct ll_conn *conn);

/**
 */
uint8_t ull_cp_ltk_req_reply(struct ll_conn *conn, const uint8_t ltk[16]);

/**
 */
uint8_t ull_cp_ltk_req_neq_reply(struct ll_conn *conn);

/**
 * @brief Initiate a PHY Update Procedure.
 */
uint8_t ull_cp_phy_update(struct ll_conn *conn, uint8_t tx, uint8_t flags, uint8_t rx,
			  uint8_t host_initiated);

/**
 * @brief Initiate a Connection Parameter Request Procedure or Connection Update Procedure
 */
uint8_t ull_cp_conn_update(struct ll_conn *conn, uint16_t interval_min, uint16_t interval_max,
			   uint16_t latency, uint16_t timeout, uint16_t *offsets);

/**
 * @brief Accept the remote device’s request to change connection parameters.
 */
void ull_cp_conn_param_req_reply(struct ll_conn *conn);

/**
 * @brief Reject the remote device’s request to change connection parameters.
 */
void ull_cp_conn_param_req_neg_reply(struct ll_conn *conn, uint8_t error_code);

/**
 * @brief Check if a remote data length update is in the works.
 */
uint8_t ull_cp_remote_dle_pending(struct ll_conn *conn);

/**
 * @brief Check if a remote connection param reg is in the
 *        works.
 */
uint8_t ull_cp_remote_cpr_pending(struct ll_conn *conn);

/**
 * @brief Check if a remote connection param reg is expecting an
 *        anchor point move response.
 */
bool ull_cp_remote_cpr_apm_awaiting_reply(struct ll_conn *conn);

/**
 * @brief Respond to anchor point move of remote connection
 *        param reg.
 */
void ull_cp_remote_cpr_apm_reply(struct ll_conn *conn, uint16_t *offsets);

/**
 * @brief Reject anchor point move of remote connection param
 *        reg.
 */
void ull_cp_remote_cpr_apm_neg_reply(struct ll_conn *conn, uint8_t error_code);

/**
 * @brief Initiate a Termination Procedure.
 */
uint8_t ull_cp_terminate(struct ll_conn *conn, uint8_t error_code);

/**
 * @brief Initiate a CIS Termination Procedure.
 */
uint8_t ull_cp_cis_terminate(struct ll_conn *conn, struct ll_conn_iso_stream *cis,
			     uint8_t error_code);

/**
 * @brief Initiate a CIS Create Procedure.
 */
uint8_t ull_cp_cis_create(struct ll_conn *conn, struct ll_conn_iso_stream *cis);

/**
 * @brief Resume CIS create after CIS offset calculation.
 */
void ull_cp_cc_offset_calc_reply(struct ll_conn *conn, uint32_t cis_offset_min,
				 uint32_t cis_offset_max);

/**
 * @brief Is ongoing create cis procedure expecting a reply?
 */
bool ull_cp_cc_awaiting_reply(struct ll_conn *conn);

/**
 * @brief Is ongoing create cis procedure expecting an established event?
 */
bool ull_cp_cc_awaiting_established(struct ll_conn *conn);

/**
 * @brief Cancel ongoing create cis procedure
 */
bool ull_cp_cc_cancel(struct ll_conn *conn);

/**
 * @brief Get handle of ongoing create cis procedure.
 * @return 0xFFFF if none
 */
uint16_t ull_cp_cc_ongoing_handle(struct ll_conn *conn);

/**
 * @brief Accept the remote device’s request to create cis.
 */
void ull_cp_cc_accept(struct ll_conn *conn, uint32_t cis_offset_min);

/**
 * @brief Reject the remote device’s request to create cis.
 */
void ull_cp_cc_reject(struct ll_conn *conn, uint8_t error_code);

/**
 * @brief CIS was established.
 */
void ull_cp_cc_established(struct ll_conn *conn, uint8_t error_code);

/**
 * @brief CIS creation ongoing.
 */
bool ull_lp_cc_is_active(struct ll_conn *conn);

/**
 * @brief CIS creation ongoing or enqueued.
 */
bool ull_lp_cc_is_enqueued(struct ll_conn *conn);

/**
 * @brief Initiate a Channel Map Update Procedure.
 */
uint8_t ull_cp_chan_map_update(struct ll_conn *conn, const uint8_t chm[5]);

/**
 * @brief Check if Channel Map Update Procedure is pending
 */
const uint8_t *ull_cp_chan_map_update_pending(struct ll_conn *conn);

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
/**
 * @brief Initiate a Data Length Update Procedure.
 */
uint8_t ull_cp_data_length_update(struct ll_conn *conn, uint16_t max_tx_octets,
				  uint16_t max_tx_time);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
/**
 * @brief Initiate a CTE Request Procedure.
 */
uint8_t ull_cp_cte_req(struct ll_conn *conn, uint8_t min_cte_len, uint8_t cte_type);

/**
 * @brief Set a CTE Request Procedure disabled.
 */
void ull_cp_cte_req_set_disable(struct ll_conn *conn);

/**
 * @brief Enable or disable response to CTE Request Procedure.
 */
void ull_cp_cte_rsp_enable(struct ll_conn *conn, bool enable, uint8_t max_cte_len,
			   uint8_t cte_types);

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
/**
 * @brief Initiate a Sleep Clock Accuracy Update Procedure.
 */
uint8_t ull_cp_req_peer_sca(struct ll_conn *conn);
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER)
struct ll_adv_sync_set;
struct ll_sync_set;

/**
 * @brief Initiate a Periodic Advertising Sync Transfer Procedure.
 */
uint8_t ull_cp_periodic_sync(struct ll_conn *conn, struct ll_sync_set *sync,
			     struct ll_adv_sync_set *adv_sync, uint16_t service_data);

void ull_lp_past_offset_get_calc_params(struct ll_conn *conn,
					uint8_t *adv_sync_handle, uint16_t *sync_handle);
void ull_lp_past_offset_calc_reply(struct ll_conn *conn, uint32_t offset_us,
				   uint16_t pa_event_counter, uint16_t last_pa_event_counter);
void ull_lp_past_conn_evt_done(struct ll_conn *conn, struct node_rx_event_done *done);
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER */

/**
 * @brief Validation of PHY, checking if exactly one bit set, and no bit set is rfu's
 */
bool phy_valid(uint8_t phy);
