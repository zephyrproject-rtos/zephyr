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
 * @breif Prepare pending LL Control Procedures.
 */
void ull_cp_prepare(struct ull_cp_conn *conn);

/**
 * @brief Handle received LL Control PDU.
 */
void ull_cp_rx(struct ull_cp_conn *conn, struct node_rx_pdu *rx);
