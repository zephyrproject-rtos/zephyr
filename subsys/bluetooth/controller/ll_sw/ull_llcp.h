/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Temporary data structure to avoid change ll_conn/lll_conn to much
 * and having to update all the dependencies
 */
struct ull_cp_conn {
	struct ull_tx_q tx_q;

	struct {
		/* Local Request */
		struct {
			sys_slist_t pend_proc_list;
			uint8_t state;
		} local;

		/* Remote Request */
		struct {
			sys_slist_t pend_proc_list;
			uint8_t state;
			uint8_t collision;
			uint8_t incompat;
			uint8_t reject_opcode;
		} remote;

		/* Version Exchange Procedure State */
		struct {
			uint8_t sent;
			uint8_t valid;
			struct pdu_data_llctrl_version_ind cached;
		} vex;

		/*
		 * As of today only 36 feature bits are in use,
		 * so some optimisation is possible
		 * we also need to keep track of the features of the
		 * other node, so that we can send a proper
		 * reply over HCI to the host
		 * see BT Core spec 5.2 Vol 6, Part B, sec. 5.1.4
		 */
		struct {
			uint8_t sent;
			uint8_t valid;
			uint64_t features_peer;
			uint64_t features_used;
		} fex;

		/* Minimum used channels procedure state */
		struct {
			uint8_t phys;
			uint8_t min_used_chans;
		} muc;
	} llcp;

	struct mocked_lll_conn {
		/* Encryption State (temporary) */
		uint8_t enc_tx;
		uint8_t enc_rx;

		/**/
		 uint16_t latency_prepare;
		 uint16_t latency_event;
		 uint16_t event_counter;

		 uint8_t role;
	} lll;
};

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
void ull_cp_conn_init(struct ull_cp_conn *conn);

/**
 * @brief XXX
 */
void ull_cp_state_set(struct ull_cp_conn *conn, uint8_t state);

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
void ull_cp_run(struct ull_cp_conn *conn);

/**
 * @brief Handle TX ack PDU.
 */
void ull_cp_tx_ack(struct ull_cp_conn *conn, struct node_tx *tx);

/**
 * @brief Handle received LL Control PDU.
 */
void ull_cp_rx(struct ull_cp_conn *conn, struct node_rx_pdu *rx);

/**
 * @brief Initiate a LE Ping Procedure.
 */
uint8_t ull_cp_le_ping(struct ull_cp_conn *conn);

/**
 * @brief Initiate a Version Exchange Procedure.
 */
uint8_t ull_cp_version_exchange(struct ull_cp_conn *conn);

/**
 * @brief Initiate a Featue Exchange Procedure.
 */
uint8_t ull_cp_feature_exchange(struct ull_cp_conn *conn);

/**
 * @brief Initiate a Minimum used channels Procedure.
 */
uint8_t ull_cp_min_used_chans(struct ull_cp_conn *conn, uint8_t phys, uint8_t min_used_chans);

/**
 * @brief Initiate a Encryption Start Procedure.
 */
uint8_t ull_cp_encryption_start(struct ull_cp_conn *conn);

/**
 */
void ull_cp_ltk_req_reply(struct ull_cp_conn *conn);

/**
 */
void ull_cp_ltk_req_neq_reply(struct ull_cp_conn *conn);

/**
 * @brief Initiate a PHY Update Procedure.
 */
uint8_t ull_cp_phy_update(struct ull_cp_conn *conn);

/**
 * @brief Initiate a Termination Procedure.
 */
uint8_t ull_cp_terminate(struct ull_cp_conn *conn, uint8_t error_code);
