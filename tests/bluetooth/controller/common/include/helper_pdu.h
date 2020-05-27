/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void helper_pdu_encode_ping_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_ping_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_encode_feature_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_slave_feature_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_feature_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_encode_min_used_chans_ind(struct pdu_data *pdu, void *param);

void helper_pdu_encode_version_ind(struct pdu_data *pdu, void *param);

void helper_pdu_encode_enc_req(struct pdu_data *pdu, void *param);

void helper_pdu_encode_enc_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_encode_start_enc_req(struct pdu_data *pdu, void *param);

void helper_pdu_encode_start_enc_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_encode_reject_ext_ind(struct pdu_data *pdu, void *param);

void helper_pdu_encode_phy_req(struct pdu_data *pdu, void *param);
void helper_pdu_encode_phy_rsp(struct pdu_data *pdu, void *param);
void helper_pdu_encode_phy_update_ind(struct pdu_data *pdu, void *param);
void helper_pdu_encode_unknown_rsp(struct pdu_data *pdu, void *param);

void helper_pdu_verify_ping_req(const char *file, u32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_ping_rsp(const char *file, u32_t line, struct pdu_data *pdu, void *param);


void helper_pdu_verify_feature_req(const char *file, u32_t line,
				   struct pdu_data *pdu, void *param);
void helper_pdu_verify_slave_feature_req(const char *file, u32_t line,
					 struct pdu_data *pdu, void *param);
void helper_pdu_verify_feature_rsp(const char *file, u32_t line,
				   struct pdu_data *pdu, void *param);

void helper_pdu_verify_min_used_chans_ind(const char *file, u32_t line, struct pdu_data *pdu, void *param);

void helper_pdu_verify_version_ind(const char *file, u32_t line, struct pdu_data *pdu, void *param);

void helper_pdu_verify_enc_req(const char *file, u32_t line, struct pdu_data *pdu, void *param);

void helper_pdu_verify_enc_rsp(const char *file, u32_t line, struct pdu_data *pdu, void *param);

void helper_pdu_verify_start_enc_req(const char *file, u32_t line, struct pdu_data *pdu, void *param);

void helper_pdu_verify_start_enc_rsp(const char *file, u32_t line, struct pdu_data *pdu, void *param);

void helper_pdu_verify_reject_ind(const char *file, u32_t line, struct pdu_data *pdu, void *param);

void helper_pdu_verify_reject_ext_ind(const char *file, u32_t line, struct pdu_data *pdu, void *param);

void helper_pdu_verify_phy_req(const char *file, u32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_phy_rsp(const char *file, u32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_phy_update_ind(const char *file, u32_t line, struct pdu_data *pdu, void *param);
void helper_pdu_verify_unknown_rsp(const char *file, u32_t line, struct pdu_data *pdu, void *param);

void helper_node_verify_phy_update(const char *file, u32_t line,
				   struct node_rx_pdu *rx, void *param);



typedef enum {
	LL_VERSION_IND,
	LL_LE_PING_REQ,
	LL_LE_PING_RSP,
	LL_FEATURE_REQ,
	LL_SLAVE_FEATURE_REQ,
	LL_FEATURE_RSP,
	LL_MIN_USED_CHANS_IND,
	LL_REJECT_IND,
	LL_REJECT_EXT_IND,
	LL_ENC_REQ,
	LL_ENC_RSP,
	LL_START_ENC_REQ,
	LL_START_ENC_RSP,
	LL_PHY_REQ,
	LL_PHY_RSP,
	LL_PHY_UPDATE_IND,
	LL_UNKNOWN_RSP,
} helper_pdu_opcode_t;

typedef enum {
	NODE_PHY_UPDATE
} helper_node_opcode_t;

typedef void (helper_pdu_encode_func_t) (struct pdu_data *data, void *param);
typedef void (helper_pdu_verify_func_t) (const char *file, u32_t line, struct pdu_data *data, void *param);

typedef void (helper_node_verify_func_t) (const char *file, u32_t line, struct node_rx_pdu *rx, void *param);
typedef void (helper_func_t) (const char *file, u32_t line, struct pdu_data *data, void *param);
