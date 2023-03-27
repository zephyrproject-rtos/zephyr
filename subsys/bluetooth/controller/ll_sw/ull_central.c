/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/mem.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll/lll_vendor.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_chan.h"
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_central.h"
#include "lll_filter.h"
#include "lll_conn_iso.h"

#include "ll_sw/ull_tx_queue.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_conn_types.h"
#include "ull_filter.h"

#include "ull_internal.h"
#include "ull_chan_internal.h"
#include "ull_scan_internal.h"
#include "ull_conn_internal.h"
#include "ull_central_internal.h"

#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#include "ll_sw/isoal.h"
#include "ll_sw/ull_iso_types.h"
#include "ll_sw/ull_conn_iso_types.h"

#include "ll_sw/ull_llcp.h"

#include "hal/debug.h"

static void ticker_op_stop_scan_cb(uint32_t status, void *param);
#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CTLR_PHY_CODED)
static void ticker_op_stop_scan_other_cb(uint32_t status, void *param);
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_CTLR_PHY_CODED */
static void ticker_op_cb(uint32_t status, void *param);
static inline void conn_release(struct ll_scan_set *scan);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
uint8_t ll_create_connection(uint16_t scan_interval, uint16_t scan_window,
			  uint8_t filter_policy, uint8_t peer_addr_type,
			  uint8_t const *const peer_addr, uint8_t own_addr_type,
			  uint16_t interval, uint16_t latency, uint16_t timeout,
			  uint8_t phy)
#else /* !CONFIG_BT_CTLR_ADV_EXT */
uint8_t ll_create_connection(uint16_t scan_interval, uint16_t scan_window,
			  uint8_t filter_policy, uint8_t peer_addr_type,
			  uint8_t const *const peer_addr, uint8_t own_addr_type,
			  uint16_t interval, uint16_t latency, uint16_t timeout)
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
{
	struct lll_conn *conn_lll;
	uint32_t conn_interval_us;
	uint8_t own_id_addr_type;
	struct ll_scan_set *scan;
	uint32_t ready_delay_us;
	uint8_t *own_id_addr;
	struct lll_scan *lll;
	struct ll_conn *conn;
	uint16_t max_tx_time;
	uint16_t max_rx_time;
	memq_link_t *link;
	uint8_t hop;
	int err;

	scan = ull_scan_is_disabled_get(SCAN_HANDLE_1M);
	if (!scan) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Check if random address has been set */
	own_id_addr_type = (own_addr_type & 0x01);
	own_id_addr = ll_addr_get(own_id_addr_type);
	if (own_id_addr_type && !mem_nz((void *)own_id_addr, BDADDR_SIZE)) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
	/* Do not connect twice to the same peer */
	if (ull_conn_peer_connected(own_id_addr_type, own_id_addr,
				    peer_addr_type, peer_addr)) {
		return BT_HCI_ERR_CONN_ALREADY_EXISTS;
	}
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	struct ll_scan_set *scan_coded;
	struct lll_scan *lll_coded;

	scan_coded = ull_scan_is_disabled_get(SCAN_HANDLE_PHY_CODED);
	if (!scan_coded) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lll = &scan->lll;
	lll_coded = &scan_coded->lll;

	/* NOTE: When coded PHY is supported, and connection establishment
	 *       over coded PHY is selected by application then look for
	 *       a connection context already assigned to 1M PHY scanning
	 *       context. Use the same connection context in the coded PHY
	 *       scanning context.
	 */
	if (phy & BT_HCI_LE_EXT_SCAN_PHY_CODED) {
		if (!lll_coded->conn) {
			lll_coded->conn = lll->conn;
		}
		scan = scan_coded;
		lll = lll_coded;
	} else {
		if (!lll->conn) {
			lll->conn = lll_coded->conn;
		}
	}

#else /* !CONFIG_BT_CTLR_PHY_CODED */
	if (phy & ~BT_HCI_LE_EXT_SCAN_PHY_1M) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lll = &scan->lll;

#endif /* !CONFIG_BT_CTLR_PHY_CODED */

	/* NOTE: non-zero PHY value enables initiating connection on that PHY */
	lll->phy = phy;

#else /* !CONFIG_BT_CTLR_ADV_EXT */
	lll = &scan->lll;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	if (lll->conn) {
		conn_lll = lll->conn;
		conn = HDR_LLL2ULL(conn_lll);

		goto conn_is_valid;
	}

	link = ll_rx_link_alloc();
	if (!link) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	conn = ll_conn_acquire();
	if (!conn) {
		ll_rx_link_release(link);

		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	conn_lll = &conn->lll;

	err = util_aa_le32(conn_lll->access_addr);
	LL_ASSERT(!err);

	lll_csrand_get(conn_lll->crc_init, sizeof(conn_lll->crc_init));

	conn_lll->handle = 0xFFFF;
	conn_lll->interval = interval;
	conn_lll->latency = latency;

	if (!conn_lll->link_tx_free) {
		conn_lll->link_tx_free = &conn_lll->link_tx;
	}

	memq_init(conn_lll->link_tx_free, &conn_lll->memq_tx.head,
		  &conn_lll->memq_tx.tail);
	conn_lll->link_tx_free = NULL;

	conn_lll->packet_tx_head_len = 0;
	conn_lll->packet_tx_head_offset = 0;

	conn_lll->sn = 0;
	conn_lll->nesn = 0;
	conn_lll->empty = 0;

#if defined(CONFIG_BT_CTLR_PHY)
	/* Use the default 1M PHY, extended connection initiation in LLL will
	 * update this with the correct PHY.
	 */
	conn_lll->phy_tx = PHY_1M;
	conn_lll->phy_flags = 0;
	conn_lll->phy_tx_time = PHY_1M;
	conn_lll->phy_rx = PHY_1M;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	ull_dle_init(conn, PHY_1M);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	conn_lll->rssi_latest = BT_HCI_LE_RSSI_NOT_AVAILABLE;
#if defined(CONFIG_BT_CTLR_CONN_RSSI_EVENT)
	conn_lll->rssi_reported = BT_HCI_LE_RSSI_NOT_AVAILABLE;
	conn_lll->rssi_sample_count = 0;
#endif /* CONFIG_BT_CTLR_CONN_RSSI_EVENT */
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	conn_lll->tx_pwr_lvl = RADIO_TXP_DEFAULT;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	/* FIXME: BEGIN: Move to ULL? */
	conn_lll->latency_prepare = 0;
	conn_lll->latency_event = 0;
	conn_lll->event_counter = 0;

	conn_lll->data_chan_count = ull_chan_map_get(conn_lll->data_chan_map);
	lll_csrand_get(&hop, sizeof(uint8_t));
	conn_lll->data_chan_hop = 5 + (hop % 12);
	conn_lll->data_chan_sel = 0;
	conn_lll->data_chan_use = 0;
	conn_lll->role = 0;
	conn_lll->central.initiated = 0;
	conn_lll->central.cancelled = 0;
	/* FIXME: END: Move to ULL? */
#if defined(CONFIG_BT_CTLR_CONN_META)
	memset(&conn_lll->conn_meta, 0, sizeof(conn_lll->conn_meta));
#endif /* CONFIG_BT_CTLR_CONN_META */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
	conn_lll->df_rx_cfg.is_initialized = 0U;
	conn_lll->df_rx_cfg.hdr.elem_size = sizeof(struct lll_df_conn_rx_params);
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX)
	conn_lll->df_tx_cfg.is_initialized = 0U;
	conn_lll->df_tx_cfg.cte_rsp_en = 0U;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_TX */

	conn->connect_expire = CONN_ESTAB_COUNTDOWN;
	conn->supervision_expire = 0U;
	conn_interval_us = (uint32_t)interval * CONN_INT_UNIT_US;
	conn->supervision_timeout = timeout;

#if defined(CONFIG_BT_CTLR_LE_PING)
	conn->apto_expire = 0U;
	/* APTO in no. of connection events */
	conn->apto_reload = RADIO_CONN_EVENTS((30000000), conn_interval_us);
	conn->appto_expire = 0U;
	/* Dispatch LE Ping PDU 6 connection events (that peer would listen to)
	 * before 30s timeout
	 * TODO: "peer listens to" is greater than 30s due to latency
	 */
	conn->appto_reload = (conn->apto_reload > (conn_lll->latency + 6)) ?
			     (conn->apto_reload - (conn_lll->latency + 6)) :
			     conn->apto_reload;
#endif /* CONFIG_BT_CTLR_LE_PING */

	/* Re-initialize the control procedure data structures */
	ull_llcp_init(conn);

	/* Setup the PRT reload */
	ull_cp_prt_reload_set(conn, conn_interval_us);

	conn->central.terminate_ack = 0U;

	conn->llcp_terminate.reason_final = 0U;
	/* NOTE: use allocated link for generating dedicated
	 * terminate ind rx node
	 */
	conn->llcp_terminate.node_rx.hdr.link = link;

#if defined(CONFIG_BT_CTLR_PHY)
	conn->phy_pref_tx = ull_conn_default_phy_tx_get();
	conn->phy_pref_rx = ull_conn_default_phy_rx_get();
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_LE_ENC)
	conn->pause_rx_data = 0U;
#endif /* CONFIG_BT_CTLR_LE_ENC */

	/* Re-initialize the Tx Q */
	ull_tx_q_init(&conn->tx_q);

	/* TODO: active_to_start feature port */
	conn->ull.ticks_active_to_start = 0U;
	conn->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	conn->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
	/* Remember peer and own identity address */
	conn->peer_id_addr_type = peer_addr_type;
	(void)memcpy(conn->peer_id_addr, peer_addr, sizeof(conn->peer_id_addr));
	conn->own_id_addr_type = own_id_addr_type;
	(void)memcpy(conn->own_id_addr, own_id_addr, sizeof(conn->own_id_addr));
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */

	lll->conn = conn_lll;

	ull_hdr_init(&conn->ull);
	lll_hdr_init(&conn->lll, conn);

conn_is_valid:
#if defined(CONFIG_BT_CTLR_PHY)
	ready_delay_us = lll_radio_tx_ready_delay_get(conn_lll->phy_tx,
						      conn_lll->phy_flags);
#else
	ready_delay_us = lll_radio_tx_ready_delay_get(0, 0);
#endif

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	conn_lll->dle.eff.max_tx_time = MAX(conn_lll->dle.eff.max_tx_time,
					    PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN,
							  lll->phy));
	conn_lll->dle.eff.max_rx_time = MAX(conn_lll->dle.eff.max_rx_time,
					    PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN,
							  lll->phy));
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	max_tx_time = conn_lll->dle.eff.max_tx_time;
	max_rx_time = conn_lll->dle.eff.max_rx_time;
#else /* CONFIG_BT_CTLR_DATA_LENGTH */
	max_tx_time = PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, PHY_1M);
	max_rx_time = PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, PHY_1M);
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	max_tx_time = MAX(max_tx_time,
			  PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, lll->phy));
	max_rx_time = MAX(max_rx_time,
			  PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, lll->phy));
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

	conn->ull.ticks_slot =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US +
				       EVENT_OVERHEAD_END_US +
				       ready_delay_us +
				       max_tx_time +
				       EVENT_IFS_US +
				       max_rx_time);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	ull_filter_scan_update(filter_policy);

	lll->rl_idx = FILTER_IDX_NONE;
	lll->rpa_gen = 0;
	if (!filter_policy && ull_filter_lll_rl_enabled()) {
		/* Look up the resolving list */
		lll->rl_idx = ull_filter_rl_find(peer_addr_type, peer_addr,
						 NULL);
	}

	if (own_addr_type == BT_ADDR_LE_PUBLIC_ID ||
	    own_addr_type == BT_ADDR_LE_RANDOM_ID) {

		/* Generate RPAs if required */
		ull_filter_rpa_update(false);
		own_addr_type &= 0x1;
		lll->rpa_gen = 1;
	}
#endif

	scan->own_addr_type = own_addr_type;
	lll->adv_addr_type = peer_addr_type;
	memcpy(lll->adv_addr, peer_addr, BDADDR_SIZE);
	lll->conn_timeout = timeout;

	scan->ticks_window = ull_scan_params_set(lll, 0U, scan_interval,
						 scan_window, filter_policy);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	return 0;
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	/* wait for stable clocks */
	err = lll_clock_wait();
	if (err) {
		conn_release(scan);

		return BT_HCI_ERR_HW_FAILURE;
	}

	return ull_scan_enable(scan);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
uint8_t ll_connect_enable(uint8_t is_coded_included)
{
	uint8_t err = BT_HCI_ERR_CMD_DISALLOWED;
	struct ll_scan_set *scan;

	scan = ull_scan_set_get(SCAN_HANDLE_1M);

	/* wait for stable clocks */
	err = lll_clock_wait();
	if (err) {
		conn_release(scan);

		return BT_HCI_ERR_HW_FAILURE;
	}

	if (!is_coded_included ||
	    (scan->lll.phy & PHY_1M)) {
		err = ull_scan_enable(scan);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED) && is_coded_included) {
		scan = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
		err = ull_scan_enable(scan);
		if (err) {
			return err;
		}
	}

	return err;
}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

uint8_t ll_connect_disable(void **rx)
{
	struct ll_scan_set *scan_coded;
	struct lll_scan *scan_lll;
	struct lll_conn *conn_lll;
	struct ll_scan_set *scan;
	uint8_t err;

	scan = ull_scan_is_enabled_get(SCAN_HANDLE_1M);

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
	    IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded = ull_scan_is_enabled_get(SCAN_HANDLE_PHY_CODED);
	} else {
		scan_coded = NULL;
	}

	if (!scan) {
		if (!scan_coded) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		scan_lll = &scan_coded->lll;
	} else {
		scan_lll = &scan->lll;
	}

	/* Check if initiator active */
	conn_lll = scan_lll->conn;
	if (!conn_lll) {
		/* Scanning not associated with initiation of a connection or
		 * connection setup already complete (was set to NULL in
		 * ull_central_setup), but HCI event not processed by host.
		 */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Indicate to LLL that a cancellation is requested */
	conn_lll->central.cancelled = 1U;
	cpu_dmb();

	/* Check if connection was established under race condition, i.e.
	 * before the cancelled flag was set.
	 */
	conn_lll = scan_lll->conn;
	if (!conn_lll) {
		/* Connection setup completed on race condition with cancelled
		 * flag, before it was set.
		 */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (scan) {
		err = ull_scan_disable(SCAN_HANDLE_1M, scan);
	} else {
		err = 0U;
	}

	if (!err && scan_coded) {
		err = ull_scan_disable(SCAN_HANDLE_PHY_CODED, scan_coded);
	}

	if (!err) {
		struct node_rx_pdu *node_rx;
		struct node_rx_cc *cc;
		struct ll_conn *conn;
		memq_link_t *link;

		conn = HDR_LLL2ULL(conn_lll);
		node_rx = (void *)&conn->llcp_terminate.node_rx;
		link = node_rx->hdr.link;
		LL_ASSERT(link);

		/* free the memq link early, as caller could overwrite it */
		ll_rx_link_release(link);

		node_rx->hdr.type = NODE_RX_TYPE_CONNECTION;
		node_rx->hdr.handle = 0xffff;

		/* NOTE: struct llcp_terminate.node_rx has uint8_t member
		 *       following the struct node_rx_hdr to store the reason.
		 */
		cc = (void *)node_rx->pdu;
		cc->status = BT_HCI_ERR_UNKNOWN_CONN_ID;

		/* NOTE: Since NODE_RX_TYPE_CONNECTION is also generated from
		 *       LLL context for other cases, pass LLL context as
		 *       parameter.
		 */
		node_rx->hdr.rx_ftr.param = scan_lll;

		*rx = node_rx;
	}

	return err;
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
uint8_t ll_enc_req_send(uint16_t handle, uint8_t const *const rand_num,
		     uint8_t const *const ediv, uint8_t const *const ltk)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (!conn->lll.enc_tx && !conn->lll.enc_rx) {
		/* Encryption is fully disabled */
		return ull_cp_encryption_start(conn, rand_num, ediv, ltk);
	} else if (conn->lll.enc_tx && conn->lll.enc_rx) {
		/* Encryption is fully enabled */
		return ull_cp_encryption_pause(conn, rand_num, ediv, ltk);
	}

	return BT_HCI_ERR_CMD_DISALLOWED;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

int ull_central_reset(void)
{
	int err;
	void *rx;

	err = ll_connect_disable(&rx);
	if (!err) {
		struct ll_scan_set *scan;

		scan = ull_scan_is_enabled_get(SCAN_HANDLE_1M);

		if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
		    IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
			struct ll_scan_set *scan_other;

			scan_other =
				ull_scan_is_enabled_get(SCAN_HANDLE_PHY_CODED);
			if (scan_other) {
				if (scan) {
					scan->is_enabled = 0U;
					scan->lll.conn = NULL;
				}

				scan = scan_other;
			}
		}

		LL_ASSERT(scan);

		scan->is_enabled = 0U;
		scan->lll.conn = NULL;
	}

	ARG_UNUSED(rx);

	return err;
}

void ull_central_cleanup(struct node_rx_hdr *rx_free)
{
	struct lll_conn *conn_lll;
	struct ll_scan_set *scan;
	struct ll_conn *conn;
	memq_link_t *link;

	/* NOTE: `scan` variable can be 1M PHY or coded PHY scanning context.
	 *       Single connection context is allocated in both the 1M PHY and
	 *       coded PHY scanning context, hence releasing only this one
	 *       connection context.
	 */
	scan = HDR_LLL2ULL(rx_free->rx_ftr.param);
	conn_lll = scan->lll.conn;
	LL_ASSERT(conn_lll);
	scan->lll.conn = NULL;

	LL_ASSERT(!conn_lll->link_tx_free);
	link = memq_deinit(&conn_lll->memq_tx.head,
			   &conn_lll->memq_tx.tail);
	LL_ASSERT(link);
	conn_lll->link_tx_free = link;

	conn = HDR_LLL2ULL(conn_lll);
	ll_conn_release(conn);

	/* 1M PHY is disabled here if both 1M and coded PHY was enabled for
	 * connection establishment.
	 */
	scan->is_enabled = 0U;

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CTLR_PHY_CODED)
	scan->lll.phy = 0U;

	/* Determine if coded PHY was also enabled, if so, reset the assigned
	 * connection context, enabled flag and phy value.
	 */
	struct ll_scan_set *scan_coded =
				ull_scan_is_enabled_get(SCAN_HANDLE_PHY_CODED);
	if (scan_coded && scan_coded != scan) {
		conn_lll = scan_coded->lll.conn;
		LL_ASSERT(conn_lll);
		scan_coded->lll.conn = NULL;

		scan_coded->is_enabled = 0U;
		scan_coded->lll.phy = 0U;
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_CTLR_PHY_CODED */
}

void ull_central_setup(struct node_rx_hdr *rx, struct node_rx_ftr *ftr,
		      struct lll_conn *lll)
{
	uint32_t conn_offset_us, conn_interval_us;
	uint8_t ticker_id_scan, ticker_id_conn;
	uint8_t peer_addr[BDADDR_SIZE];
	uint32_t ticks_slot_overhead;
	uint32_t ticks_slot_offset;
	struct ll_scan_set *scan;
	struct pdu_adv *pdu_tx;
	uint8_t peer_addr_type;
	uint32_t ticker_status;
	uint32_t ticks_at_stop;
	struct node_rx_cc *cc;
	struct ll_conn *conn;
	memq_link_t *link;
	uint8_t chan_sel;
	void *node;

	/* Get reference to Tx-ed CONNECT_IND PDU */
	pdu_tx = (void *)((struct node_rx_pdu *)rx)->pdu;

	/* Backup peer addr and type, as we reuse the Tx-ed PDU to generate
	 * event towards LL
	 */
	peer_addr_type = pdu_tx->rx_addr;
	memcpy(peer_addr, &pdu_tx->connect_ind.adv_addr[0], BDADDR_SIZE);

	/* This is the chan sel bit from the received adv pdu */
	chan_sel = pdu_tx->chan_sel;

	/* Check for pdu field being aligned before populating connection
	 * complete event.
	 */
	node = pdu_tx;
	LL_ASSERT(IS_PTR_ALIGNED(node, struct node_rx_cc));

	/* Populate the fields required for connection complete event */
	cc = node;
	cc->status = 0U;
	cc->role = 0U;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t rl_idx = ftr->rl_idx;

	if (ftr->lrpa_used) {
		memcpy(&cc->local_rpa[0], &pdu_tx->connect_ind.init_addr[0],
		       BDADDR_SIZE);
	} else {
		memset(&cc->local_rpa[0], 0x0, BDADDR_SIZE);
	}

	if (rl_idx != FILTER_IDX_NONE) {
		/* Store identity address */
		ll_rl_id_addr_get(rl_idx, &cc->peer_addr_type,
				  &cc->peer_addr[0]);
		/* Mark it as identity address from RPA (0x02, 0x03) */
		cc->peer_addr_type += 2;

		/* Store peer RPA */
		memcpy(&cc->peer_rpa[0], &peer_addr[0], BDADDR_SIZE);
	} else {
		memset(&cc->peer_rpa[0], 0x0, BDADDR_SIZE);
#else
	if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
		cc->peer_addr_type = peer_addr_type;
		memcpy(cc->peer_addr, &peer_addr[0], BDADDR_SIZE);
	}

	scan = HDR_LLL2ULL(ftr->param);

	cc->interval = lll->interval;
	cc->latency = lll->latency;
	cc->timeout = scan->lll.conn_timeout;
	cc->sca = lll_clock_sca_local_get();

	conn = lll->hdr.parent;
	lll->handle = ll_conn_handle_get(conn);
	rx->handle = lll->handle;

	/* Set LLCP as connection-wise connected */
	ull_cp_state_set(conn, ULL_CP_CONNECTED);

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	lll->tx_pwr_lvl = RADIO_TXP_DEFAULT;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	/* Use the link stored in the node rx to enqueue connection
	 * complete node rx towards LL context.
	 */
	link = rx->link;

	/* Use Channel Selection Algorithm #2 if peer too supports it */
	if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
		struct node_rx_pdu *rx_csa;
		struct node_rx_cs *cs;

		/* pick the rx node instance stored within the connection
		 * rx node.
		 */
		rx_csa = (void *)ftr->extra;

		/* Enqueue the connection event */
		ll_rx_put(link, rx);

		/* use the rx node for CSA event */
		rx = (void *)rx_csa;
		link = rx->link;

		rx->handle = lll->handle;
		rx->type = NODE_RX_TYPE_CHAN_SEL_ALGO;

		cs = (void *)rx_csa->pdu;

		if (chan_sel) {
			lll->data_chan_sel = 1;
			lll->data_chan_id = lll_chan_id(lll->access_addr);

			cs->csa = 0x01;
		} else {
			cs->csa = 0x00;
		}
	}

	ll_rx_put_sched(link, rx);

	ticks_slot_offset = MAX(conn->ull.ticks_active_to_start,
				conn->ull.ticks_prepare_to_start);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}
	ticks_slot_offset += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	conn_interval_us = lll->interval * CONN_INT_UNIT_US;
	conn_offset_us = ftr->radio_end_us;
	conn_offset_us += EVENT_TICKER_RES_MARGIN_US;

#if defined(CONFIG_BT_CTLR_PHY)
	conn_offset_us -= lll_radio_tx_ready_delay_get(lll->phy_tx,
						      lll->phy_flags);
#else
	conn_offset_us -= lll_radio_tx_ready_delay_get(0, 0);
#endif


#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* disable ticker job, in order to chain stop and start to avoid RTC
	 * being stopped if no tickers active.
	 */
	mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 0);
#endif

	/* Stop Scanner */
	ticker_id_scan = TICKER_ID_SCAN_BASE + ull_scan_handle_get(scan);
	ticks_at_stop = ftr->ticks_anchor +
			HAL_TICKER_US_TO_TICKS(conn_offset_us) -
			ticks_slot_offset;
	ticker_status = ticker_stop_abs(TICKER_INSTANCE_ID_CTLR,
					TICKER_USER_ID_ULL_HIGH,
					ticker_id_scan, ticks_at_stop,
					ticker_op_stop_scan_cb, scan);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CTLR_PHY_CODED)
	/* Determine if coded PHY was also enabled, if so, reset the assigned
	 * connection context.
	 */
	struct ll_scan_set *scan_other =
				ull_scan_is_enabled_get(SCAN_HANDLE_PHY_CODED);
	if (scan_other) {
		if (scan_other == scan) {
			scan_other = ull_scan_is_enabled_get(SCAN_HANDLE_1M);
		}

		if (scan_other) {
			ticker_id_scan = TICKER_ID_SCAN_BASE +
					 ull_scan_handle_get(scan_other);
			ticker_status = ticker_stop(TICKER_INSTANCE_ID_CTLR,
						    TICKER_USER_ID_ULL_HIGH,
						    ticker_id_scan,
						    ticker_op_stop_scan_other_cb,
						    scan_other);
			LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
				  (ticker_status == TICKER_STATUS_BUSY));
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_CTLR_PHY_CODED */

	/* Scanner stop can expire while here in this ISR.
	 * Deferred attempt to stop can fail as it would have
	 * expired, hence ignore failure.
	 */
	ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
		    TICKER_ID_SCAN_STOP, NULL, NULL);

	/* Start central */
	ticker_id_conn = TICKER_ID_CONN_BASE + ll_conn_handle_get(conn);
	ticker_status = ticker_start(TICKER_INSTANCE_ID_CTLR,
				     TICKER_USER_ID_ULL_HIGH,
				     ticker_id_conn,
				     ftr->ticks_anchor - ticks_slot_offset,
				     HAL_TICKER_US_TO_TICKS(conn_offset_us),
				     HAL_TICKER_US_TO_TICKS(conn_interval_us),
				     HAL_TICKER_REMAINDER(conn_interval_us),
				     TICKER_NULL_LAZY,
				     (conn->ull.ticks_slot +
				      ticks_slot_overhead),
				     ull_central_ticker_cb, conn, ticker_op_cb,
				     (void *)__LINE__);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* enable ticker job, irrespective of disabled in this function so
	 * first connection event can be scheduled as soon as possible.
	 */
	mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1);
#endif
}

void ull_central_ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			  uint32_t remainder, uint16_t lazy, uint8_t force,
			  void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_central_prepare};
	static struct lll_prepare_param p;
	struct ll_conn *conn;
	uint32_t err;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_M(1);

	conn = param;

	/* Check if stopping ticker (on disconnection, race with ticker expiry)
	 */
	if (unlikely(conn->lll.handle == 0xFFFF)) {
		DEBUG_RADIO_CLOSE_M(0);
		return;
	}

#if defined(CONFIG_BT_CTLR_CONN_META)
	conn->common.is_must_expire = (lazy == TICKER_LAZY_MUST_EXPIRE);
#endif
	/* If this is a must-expire callback, LLCP state machine does not need
	 * to know. Will be called with lazy > 0 when scheduled in air.
	 */
	if (!IS_ENABLED(CONFIG_BT_CTLR_CONN_META) ||
	    (lazy != TICKER_LAZY_MUST_EXPIRE)) {
		int ret;

		/* Handle any LL Control Procedures */
		ret = ull_conn_llcp(conn, ticks_at_expire, lazy);
		if (ret) {
			/* NOTE: Under BT_CTLR_LOW_LAT, ULL_LOW context is
			 *       disabled inside radio events, hence, abort any
			 *       active radio event which will re-enable
			 *       ULL_LOW context that permits ticker job to run.
			 */
			if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) &&
			    (CONFIG_BT_CTLR_LLL_PRIO ==
			     CONFIG_BT_CTLR_ULL_LOW_PRIO)) {
				ll_radio_state_abort();
			}

			DEBUG_RADIO_CLOSE_M(0);
			return;
		}
	}

	/* Increment prepare reference count */
	ref = ull_ref_inc(&conn->ull);
	LL_ASSERT(ref);

	/* De-mux 2 tx node from FIFO, sufficient to be able to set MD bit */
	ull_conn_tx_demux(2);

	/* Enqueue towards LLL */
	ull_conn_tx_lll_enqueue(conn, 2);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.force = force;
	p.param = &conn->lll;
	mfy.param = &p;

	/* Kick LLL prepare */
	err = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!err);

	/* De-mux remaining tx nodes from FIFO */
	ull_conn_tx_demux(UINT8_MAX);

	/* Enqueue towards LLL */
	ull_conn_tx_lll_enqueue(conn, UINT8_MAX);

	DEBUG_RADIO_PREPARE_M(1);
}

uint8_t ull_central_chm_update(void)
{
	uint16_t handle;

	handle = CONFIG_BT_MAX_CONN;
	while (handle--) {
		struct ll_conn *conn;
		uint8_t ret;
		uint8_t chm[5];

		conn = ll_connected_get(handle);
		if (!conn || conn->lll.role) {
			continue;
		}

		ull_chan_map_get(chm);
		ret = ull_cp_chan_map_update(conn, chm);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static void ticker_op_stop_scan_cb(uint32_t status, void *param)
{
	/* NOTE: Nothing to do here, present here to add debug code if required
	 */
}

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CTLR_PHY_CODED)
static void ticker_op_stop_scan_other_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, NULL};
	struct ll_scan_set *scan;
	struct ull_hdr *hdr;

	/* Ignore if race between thread and ULL */
	if (status != TICKER_STATUS_SUCCESS) {
		/* TODO: detect race */

		return;
	}

	/* NOTE: We are in ULL_LOW which can be pre-empted by ULL_HIGH.
	 *       As we are in the callback after successful stop of the
	 *       ticker, the ULL reference count will not be modified
	 *       further hence it is safe to check and act on either the need
	 *       to call lll_disable or not.
	 */
	scan = param;
	hdr = &scan->ull;
	mfy.param = &scan->lll;
	if (ull_ref_get(hdr)) {
		uint32_t ret;

		mfy.fp = lll_disable;
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
				     TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!ret);
	}
}
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_CTLR_PHY_CODED */

static void ticker_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

static inline void conn_release(struct ll_scan_set *scan)
{
	struct node_rx_pdu *cc;
	struct lll_conn *lll;
	struct ll_conn *conn;
	memq_link_t *link;

	lll = scan->lll.conn;
	LL_ASSERT(!lll->link_tx_free);
	link = memq_deinit(&lll->memq_tx.head, &lll->memq_tx.tail);
	LL_ASSERT(link);
	lll->link_tx_free = link;

	conn = HDR_LLL2ULL(lll);

	cc = (void *)&conn->llcp_terminate.node_rx;
	link = cc->hdr.link;
	LL_ASSERT(link);

	ll_rx_link_release(link);

	ll_conn_release(conn);
	scan->lll.conn = NULL;
}
