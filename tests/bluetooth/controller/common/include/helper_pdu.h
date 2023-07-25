/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void helper_pdu_encode_ping_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_ping_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_encode_feature_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_peripheral_feature_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_feature_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_encode_min_used_chans_ind(struct pdu_data *pdu, void *param);

void helper_pdu_encode_version_ind(struct pdu_data *pdu, void *param);

void helper_pdu_encode_enc_req(struct pdu_data *pdu, void *param);

void helper_pdu_encode_enc_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_encode_start_enc_req(struct pdu_data *pdu, void *param);

void helper_pdu_encode_start_enc_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_encode_pause_enc_req(struct pdu_data *pdu, void *param);

void helper_pdu_encode_pause_enc_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_encode_reject_ext_ind(struct pdu_data *pdu, void *param);

void helper_pdu_encode_reject_ind(struct pdu_data *pdu, void *param);

void helper_pdu_encode_phy_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_phy_rsp(struct pdu_data *pdu, void *param);
void helper_pdu_encode_phy_update_ind(struct pdu_data *pdu, void *param);
void helper_pdu_encode_unknown_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_encode_conn_param_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_conn_param_rsp(struct pdu_data *pdu, void *param);
void helper_pdu_encode_conn_update_ind(struct pdu_data *pdu, void *param);

void helper_pdu_encode_terminate_ind(struct pdu_data *pdu, void *param);

void helper_pdu_encode_channel_map_update_ind(struct pdu_data *pdu, void *param);

void helper_pdu_encode_length_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_length_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_encode_cte_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_cte_rsp(struct pdu_data *pdu, void *param);
void helper_node_encode_cte_rsp(struct node_rx_pdu *rx, void *param);

void helper_pdu_encode_zero(struct pdu_data *pdu, void *param);

void helper_pdu_encode_cis_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_cis_rsp(struct pdu_data *pdu, void *param);
void helper_pdu_encode_cis_ind(struct pdu_data *pdu, void *param);
void helper_pdu_encode_cis_terminate_ind(struct pdu_data *pdu, void *param);

void helper_pdu_encode_sca_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_sca_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_verify_ping_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_ping_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param);

void helper_pdu_verify_feature_req(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param);
void helper_pdu_verify_peripheral_feature_req(const char *file, uint32_t line, struct pdu_data *pdu,
					      void *param);
void helper_pdu_verify_feature_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param);

void helper_pdu_verify_min_used_chans_ind(const char *file, uint32_t line, struct pdu_data *pdu,
					  void *param);

void helper_pdu_verify_version_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param);

void helper_pdu_verify_enc_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param);

void helper_pdu_ntf_verify_enc_req(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param);

void helper_pdu_verify_enc_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param);

void helper_pdu_verify_start_enc_req(const char *file, uint32_t line, struct pdu_data *pdu,
				     void *param);

void helper_pdu_verify_start_enc_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				     void *param);

void helper_pdu_verify_pause_enc_req(const char *file, uint32_t line, struct pdu_data *pdu,
				     void *param);

void helper_pdu_verify_pause_enc_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				     void *param);

void helper_node_verify_enc_refresh(const char *file, uint32_t line, struct node_rx_pdu *rx,
				    void *param);

void helper_pdu_verify_reject_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				  void *param);

void helper_pdu_verify_reject_ext_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				      void *param);

void helper_pdu_verify_phy_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_phy_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_phy_update_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				      void *param);
void helper_pdu_verify_unknown_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param);

void helper_pdu_verify_terminate_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				     void *param);

void helper_pdu_verify_channel_map_update_ind(const char *file, uint32_t line, struct pdu_data *pdu,
					      void *param);

void helper_node_verify_phy_update(const char *file, uint32_t line, struct node_rx_pdu *rx,
				   void *param);
void helper_pdu_verify_conn_param_req(const char *file, uint32_t line, struct pdu_data *pdu,
				      void *param);
void helper_pdu_verify_conn_param_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				      void *param);
void helper_pdu_verify_conn_update_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				       void *param);
void helper_node_verify_conn_update(const char *file, uint32_t line, struct node_rx_pdu *rx,
				    void *param);

void helper_pdu_verify_length_req(const char *file, uint32_t line, struct pdu_data *pdu,
				  void *param);
void helper_pdu_verify_length_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				  void *param);

void helper_pdu_verify_cte_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_cte_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param);
void helper_node_verify_cte_rsp(const char *file, uint32_t line, struct node_rx_pdu *rx,
				void *param);
void helper_pdu_ntf_verify_cte_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param);

void helper_node_verify_cis_request(const char *file, uint32_t line, struct node_rx_pdu *rx,
				void *param);
void helper_node_verify_cis_established(const char *file, uint32_t line, struct node_rx_pdu *rx,
				void *param);
void helper_pdu_verify_cis_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_cis_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_cis_ind(const char *file, uint32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_cis_terminate_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				     void *param);

void helper_pdu_verify_sca_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_sca_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param);

void helper_node_verify_peer_sca_update(const char *file, uint32_t line, struct node_rx_pdu *rx,
				   void *param);

enum helper_pdu_opcode {
	LL_VERSION_IND,
	LL_LE_PING_REQ,
	LL_LE_PING_RSP,
	LL_FEATURE_REQ,
	LL_PERIPH_FEAT_XCHG,
	LL_FEATURE_RSP,
	LL_MIN_USED_CHANS_IND,
	LL_REJECT_IND,
	LL_REJECT_EXT_IND,
	LL_ENC_REQ,
	LL_ENC_RSP,
	LL_START_ENC_REQ,
	LL_START_ENC_RSP,
	LL_PAUSE_ENC_REQ,
	LL_PAUSE_ENC_RSP,
	LL_PHY_REQ,
	LL_PHY_RSP,
	LL_PHY_UPDATE_IND,
	LL_UNKNOWN_RSP,
	LL_CONNECTION_UPDATE_IND,
	LL_CONNECTION_PARAM_REQ,
	LL_CONNECTION_PARAM_RSP,
	LL_TERMINATE_IND,
	LL_CHAN_MAP_UPDATE_IND,
	LL_LENGTH_REQ,
	LL_LENGTH_RSP,
	LL_CTE_REQ,
	LL_CTE_RSP,
	LL_CLOCK_ACCURACY_REQ,
	LL_CLOCK_ACCURACY_RSP,
	LL_CIS_REQ,
	LL_CIS_RSP,
	LL_CIS_IND,
	LL_CIS_TERMINATE_IND,
	LL_ZERO,
};

enum helper_node_opcode {
	NODE_PHY_UPDATE,
	NODE_CONN_UPDATE,
	NODE_ENC_REFRESH,
	NODE_CTE_RSP,
	NODE_CIS_REQUEST,
	NODE_CIS_ESTABLISHED,
	NODE_PEER_SCA_UPDATE,
};

typedef void(helper_pdu_encode_func_t)(struct pdu_data *data, void *param);
typedef void(helper_pdu_verify_func_t)(const char *file, uint32_t line, struct pdu_data *data,
				       void *param);
typedef void(helper_pdu_ntf_verify_func_t)(const char *file, uint32_t line, struct pdu_data *data,
					   void *param);
typedef void(helper_node_encode_func_t)(struct node_rx_pdu *rx, void *param);
typedef void(helper_node_verify_func_t)(const char *file, uint32_t line, struct node_rx_pdu *rx,
					void *param);
