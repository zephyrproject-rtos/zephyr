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
			u8_t state;
		} local;

		/* Remote Request */
		struct {
			sys_slist_t pend_proc_list;
			u8_t state;
			u8_t collision;
			u8_t incompat;
			u8_t reject_opcode;
		} remote;

		/* Version Exchange Procedure State */
		struct {
			u8_t sent;
			u8_t valid;
			struct pdu_data_llctrl_version_ind cached;
		} vex;

		/*
		 * As of today only 36 feature bits are in use,
		 * so some optimisation is possible
		 */
		struct {
			u8_t sent;
			u8_t valid;
			u64_t features;
		} fex;
	} llcp;

	struct mocked_lll_conn {
		/* Encryption State (temporary) */
		u8_t enc_tx;
		u8_t enc_rx;

		/**/
		 u16_t latency_prepare;
		 u16_t latency_event;
		 u16_t event_counter;

		 u8_t role;
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
void ull_cp_state_set(struct ull_cp_conn *conn, u8_t state);

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
 * @brief Handle received LL Control PDU.
 */
void ull_cp_rx(struct ull_cp_conn *conn, struct node_rx_pdu *rx);

/**
 * @brief Initiate a LE Ping Procedure.
 */
u8_t ull_cp_le_ping(struct ull_cp_conn *conn);

/**
 * @brief Initiate a Version Exchange Procedure.
 */
u8_t ull_cp_version_exchange(struct ull_cp_conn *conn);

/**
 * @brief Initiate a Featue Exchange Procedure.
 */
u8_t ull_cp_feature_exchange(struct ull_cp_conn *conn);

/**
 * @brief Initiate a Encryption Start Procedure.
 */
u8_t ull_cp_encryption_start(struct ull_cp_conn *conn);

/**
 */
void ull_cp_ltk_req_reply(struct ull_cp_conn *conn);

/**
 */
void ull_cp_ltk_req_neq_reply(struct ull_cp_conn *conn);

/**
 * @brief Initiate a PHY Update Procedure.
 */
u8_t ull_cp_phy_update(struct ull_cp_conn *conn);
