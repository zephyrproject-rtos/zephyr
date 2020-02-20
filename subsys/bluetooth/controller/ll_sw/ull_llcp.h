/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* LL Control Procedure connection related data */
struct ull_cp_conn {
	/* Tx Queue Reference
	 * TODO(thoh):
	 *	Temporary solution to contain the changes.
	 *	To be transitioned to ll_conn. */
	struct ull_tx_q *tx_q;

	/* Local Request */
	struct {
		sys_slist_t pend_proc_list;
		u8_t state;
	} local;

	/* Remote Request */
	struct {
		sys_slist_t pend_proc_list;
		u8_t state;
	} remote;

	/* Version Exchange Procedure State */
	struct {
		u8_t sent;
		u8_t valid;
		struct pdu_data_llctrl_version_ind cached;
	} vex;
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
 * @breif Run pending LL Control Procedures.
 */
void ull_cp_run(struct ull_cp_conn *conn);

/**
 * @brief Move to the connected state.
 */
void ull_cp_connect(struct ull_cp_conn *conn);

/**
 * @brief Move to the disconnected state.
 */
void ull_cp_disconnect(struct ull_cp_conn *conn);

/**
 * @brief Initiate a Version Exchange Procedure.
 */
u8_t ull_cp_version_exchange(struct ull_cp_conn *conn);

/**
 * @brief Handle received LL Control PDU.
 */
void ull_cp_rx(struct ull_cp_conn *conn, struct node_rx_pdu *rx);
