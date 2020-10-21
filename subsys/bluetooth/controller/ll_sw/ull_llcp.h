/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Temporary data structure to avoid change ll_conn/lll_conn to much
 * and having to update all the dependencies
 */

enum {
	ULL_CP_CONNECTED,
	ULL_CP_DISCONNECTED
};

/**
 * @brief Initialize the LL Control Procedure system.
 */
void ull_cp_init(void);

/**
 * @brief Initialize the LL Control Procedure conenction data.
 */
void ll_conn_init(struct ll_conn *conn);

/**
 * @brief XXX
 */
void ull_cp_state_set(struct ll_conn *conn, uint8_t state);

/**
 *
 */
void ull_cp_release_tx(struct node_tx *tx);

/**
 *
 */
void ull_cp_release_ntf(struct node_rx_pdu *ntf);

/**
 * @brief Run pending LL Control Procedures.
 */
void ull_cp_run(struct ll_conn *conn);

/**
 * @brief Handle TX ack PDU.
 */
void ull_cp_tx_ack(struct ll_conn *conn, struct node_tx *tx);

/**
 * @brief Handle received LL Control PDU.
 */
void ull_cp_rx(struct ll_conn *conn, struct node_rx_pdu *rx);

/**
 * @brief Initiate a LE Ping Procedure.
 */
uint8_t ull_cp_le_ping(struct ll_conn *conn);

/**
 * @brief Initiate a Version Exchange Procedure.
 */
uint8_t ull_cp_version_exchange(struct ll_conn *conn);

/**
 * @brief Initiate a Featue Exchange Procedure.
 */
uint8_t ull_cp_feature_exchange(struct ll_conn *conn);

/**
 * @brief Initiate a Minimum used channels Procedure.
 */
uint8_t ull_cp_min_used_chans(struct ll_conn *conn, uint8_t phys, uint8_t min_used_chans);

/**
 * @brief Initiate a Encryption Start Procedure.
 */
uint8_t ull_cp_encryption_start(struct ll_conn *conn);

/**
 */
void ull_cp_ltk_req_reply(struct ll_conn *conn);

/**
 */
void ull_cp_ltk_req_neq_reply(struct ll_conn *conn);

/**
 * @brief Initiate a PHY Update Procedure.
 */
uint8_t ull_cp_phy_update(struct ll_conn *conn);

/**
 * @brief Initiate a Connection Parameter Request Procedure or Connection Update Procedure
 */
uint8_t ull_cp_conn_update(struct ull_cp_conn *conn, uint16_t interval_min, uint16_t interval_max, uint16_t latency, uint16_t timeout);

/**
 * @brief Accept the remote device’s request to change connection parameters.
 */
void ull_cp_conn_param_req_reply(struct ull_cp_conn *conn);

/**
 * @brief Reject the remote device’s request to change connection parameters.
 */
void ull_cp_conn_param_req_neg_reply(struct ull_cp_conn *conn);

/**
 * @brief Initiate a Termination Procedure.
 */
uint8_t ull_cp_terminate(struct ll_conn *conn, uint8_t error_code);
