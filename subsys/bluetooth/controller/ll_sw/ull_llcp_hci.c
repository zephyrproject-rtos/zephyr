/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * EGON TODO:
 * before merging with master these functions need to be re-visited
 * and compared to their counterpart in ull_conn.c
 * to verify that any bug-fixes and new features there are included here
 */
#include <zephyr.h>
#include <soc.h>
#include <bluetooth/hci.h>
#include <sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll_clock.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_filter.h"

#include "ull_tx_queue.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_conn_types.h"
#include "ull_filter.h"

#include "ull_internal.h"
#include "ull_sched_internal.h"
#include "ull_slave_internal.h"
#include "ull_chan_internal.h"
#include "ull_scan_internal.h"
#include "ull_conn_llcp_internal.h"
#include "ull_master_internal.h"

#include "ull_llcp.h"
#include "ull_llcp_features.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_hci
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_USER_EXT)
#include "ull_vendor.h"
#endif /* CONFIG_BT_CTLR_USER_EXT */

static void tx_demux(void *param);

#if defined(CONFIG_BT_CENTRAL)
static inline void conn_release(struct ll_scan_set *scan);
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_CTLR_FORCE_MD_AUTO)
static uint8_t force_md_cnt_calc(struct lll_conn *lll_conn, uint32_t tx_rate);
#endif /* CONFIG_BT_CTLR_FORCE_MD_AUTO */

uint8_t ll_version_ind_send(uint16_t handle)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return ull_cp_version_exchange(conn);
}

#if defined(CONFIG_BT_CTLR_LE_PING)
uint8_t ll_apto_get(uint16_t handle, uint16_t *apto)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	/* apto unit is 10ms */
	*apto = conn->apto_reload * conn->lll.interval * (CONN_INT_UNIT_US / 10) / 1000;

	return 0;
}

uint8_t ll_apto_set(uint16_t handle, uint16_t apto)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	/* apto unit is 10ms, RADIO_CONN_EVENTS converts us to connection events */
	conn->apto_reload =
		RADIO_CONN_EVENTS(apto * 10U * 1000U, conn->lll.interval * CONN_INT_UNIT_US);

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_CTLR_SLAVE_FEAT_REQ)
uint8_t ll_feature_req_send(uint16_t handle)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return ull_cp_feature_exchange(conn);
}
#endif /* CONFIG_BT_CENTRAL || CONFIG_BT_CTLR_SLAVE_FEAT_REQ */

#if defined(CONFIG_BT_CTLR_PHY)
uint8_t ll_phy_default_set(uint8_t tx, uint8_t rx)
{
	/* TODO: validate against supported phy */

	ull_conn_default_phy_tx_set(tx);
	ull_conn_default_phy_rx_set(rx);

	return 0;
}

uint8_t ll_phy_get(uint16_t handle, uint8_t *tx, uint8_t *rx)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	/* TODO: context safe read */
	*tx = conn->lll.phy_tx;
	*rx = conn->lll.phy_rx;

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_phy_req_send(uint16_t handle, uint8_t tx, uint8_t flags, uint8_t rx)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (!feature_phy_2m(conn) && !feature_phy_coded(conn)) {
		return BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
	}

	return ull_cp_phy_update(conn, tx, flags, rx, 1U);
}
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
uint32_t ll_length_req_send(uint16_t handle, uint16_t tx_octets, uint16_t tx_time)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (!feature_dle(conn)) {
		return BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
	}

	return ull_cp_data_length_update(conn, tx_octets, tx_time);
}

void ll_length_max_get(uint16_t *max_tx_octets, uint16_t *max_tx_time, uint16_t *max_rx_octets,
		       uint16_t *max_rx_time)
{
#if defined(CONFIG_BT_CTLR_PHY) && defined(CONFIG_BT_CTLR_PHY_CODED)
	const uint8_t phy = PHY_CODED;
#else
	const uint8_t phy = PHY_1M;
#endif

	*max_tx_octets = LL_LENGTH_OCTETS_RX_MAX;
	*max_rx_octets = LL_LENGTH_OCTETS_RX_MAX;
	*max_tx_time = PDU_DC_MAX_US(LL_LENGTH_OCTETS_RX_MAX, phy);
	*max_rx_time = PDU_DC_MAX_US(LL_LENGTH_OCTETS_RX_MAX, phy);
}

void ll_length_default_get(uint16_t *max_tx_octets, uint16_t *max_tx_time)
{
	*max_tx_octets = ull_conn_default_tx_octets_get();
	*max_tx_time = ull_conn_default_tx_time_get();
}

uint32_t ll_length_default_set(uint16_t max_tx_octets, uint16_t max_tx_time)
{
	/* TODO: parameter check (for BT 5.0 compliance) */

	ull_conn_default_tx_octets_set(max_tx_octets);
	ull_conn_default_tx_time_set(max_tx_time);

	return 0;
}

#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

uint8_t ll_terminate_ind_send(uint16_t handle, uint8_t reason)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return ull_cp_terminate(conn, reason);
}

/* cmd and status are used to select control procedure:
 * cmd		status		CP
 * 0		x		ull_cp_conn_update()
 * 2		0		ull_cp_conn_param_req_reply()
 * 2		!0		ull_cp_conn_param_req_neg_reply()
 */
uint8_t ll_conn_update(uint16_t handle, uint8_t cmd, uint8_t status, uint16_t interval_min,
		       uint16_t interval_max, uint16_t latency, uint16_t timeout)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (cmd == 0U) {
		return ull_cp_conn_update(conn, interval_min, interval_max, latency, timeout);
	} else if (cmd == 2U) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		if (status == 0U) {
			ull_cp_conn_param_req_reply(conn);
		} else {
			ull_cp_conn_param_req_neg_reply(conn, status);
		}
		return BT_HCI_ERR_SUCCESS;
#else /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
		/* CPR feature not supported */
		return BT_HCI_ERR_CMD_DISALLOWED;
#endif /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
	} else {
		return BT_HCI_ERR_UNKNOWN_CMD;
	}
}

uint8_t ll_chm_get(uint16_t handle, uint8_t *chm)
{
	struct ll_conn *conn;
	const uint8_t *pending_chm;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	/*
	 * Core Spec 5.2 Vol4: 7.8.20:
	 * The HCI_LE_Read_Channel_Map command returns the current Channel_Map
	 * for the specified Connection_Handle. The returned value indicates the state of
	 * the Channel_Map specified by the last transmitted or received Channel_Map
	 * (in a CONNECT_IND or LL_CHANNEL_MAP_IND message) for the specified
	 * Connection_Handle, regardless of whether the Master has received an
	 * acknowledgment
	 */
	pending_chm = ull_cp_chan_map_update_pending(conn);
	if (pending_chm) {
		memcpy(chm, pending_chm, sizeof(conn->lll.data_chan_map));
	} else {
		memcpy(chm, conn->lll.data_chan_map, sizeof(conn->lll.data_chan_map));
	}

	return BT_HCI_ERR_SUCCESS;
}

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
uint8_t ll_rssi_get(uint16_t handle, uint8_t *rssi)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	/*
	 * EGON TODO: get RSSI information from lll
	 */

	return BT_HCI_ERR_UNKNOWN_CMD;
}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

/*
 * the following are only valid for slave configuration
 */
#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_PERIPHERAL)
uint8_t ll_start_enc_req_send(uint16_t handle, uint8_t error_code, uint8_t const *const ltk)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	/*
	 * EGON TODO: add info to the conn-structure
	 * - refresh
	 * - no procedure in progress
	 * - procedure type
	 * and use that info to decide if the cmd is allowed
	 * or if we should terminate the connection
	 * see BT 5.2 Vol. 6 part B chapter 5.1.3
	 * see also ull_slave.c line 395-439
	 *
	 * EGON TODO: the ull_cp_ltx_req* functions should return success/fail status
	 */
	if (error_code) {
		ull_cp_ltk_req_neq_reply(conn);
		return BT_HCI_ERR_SUCCESS;
	} else {
		ull_cp_ltk_req_reply(conn, ltk);
		return BT_HCI_ERR_SUCCESS;
	}

	return BT_HCI_ERR_UNKNOWN_CMD;
}
#endif /* CONFIG_BT_CTLR_LE_ENC && CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CENTRAL)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
uint8_t ll_create_connection(uint16_t scan_interval, uint16_t scan_window, uint8_t filter_policy,
			     uint8_t peer_addr_type, uint8_t const *const peer_addr,
			     uint8_t own_addr_type, uint16_t interval, uint16_t latency,
			     uint16_t timeout, uint8_t phy)
#else /* !CONFIG_BT_CTLR_ADV_EXT */
uint8_t ll_create_connection(uint16_t scan_interval, uint16_t scan_window, uint8_t filter_policy,
			     uint8_t peer_addr_type, uint8_t const *const peer_addr,
			     uint8_t own_addr_type, uint16_t interval, uint16_t latency,
			     uint16_t timeout)
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
{
	struct lll_conn *conn_lll;
	struct ll_scan_set *scan;
	uint32_t conn_interval_us;
	uint32_t ready_delay_us;
	struct lll_scan *lll;
	struct ll_conn *conn;
	memq_link_t *link;
	uint8_t hop;
	int err;

	scan = ull_scan_is_disabled_get(SCAN_HANDLE_1M);
	if (!scan) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

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

	ull_scan_params_set(lll, 0, scan_interval, scan_window, filter_policy);

	lll->adv_addr_type = peer_addr_type;
	memcpy(lll->adv_addr, peer_addr, BDADDR_SIZE);
	lll->conn_timeout = timeout;

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

	memq_init(conn_lll->link_tx_free, &conn_lll->memq_tx.head, &conn_lll->memq_tx.tail);
	conn_lll->link_tx_free = NULL;

	conn_lll->packet_tx_head_len = 0;
	conn_lll->packet_tx_head_offset = 0;

	conn_lll->sn = 0;
	conn_lll->nesn = 0;
	conn_lll->empty = 0;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	ull_dle_init(conn, PHY_1M);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	conn_lll->phy_tx = PHY_1M;
	conn_lll->phy_flags = 0;
	conn_lll->phy_tx_time = PHY_1M;
	conn_lll->phy_rx = PHY_1M;
#endif /* CONFIG_BT_CTLR_PHY */

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
	conn_lll->master.initiated = 0;
	conn_lll->master.cancelled = 0;
	/* FIXME: END: Move to ULL? */
#if defined(CONFIG_BT_CTLR_CONN_META)
	memset(&conn_lll->conn_meta, 0, sizeof(conn_lll->conn_meta));
#endif /* CONFIG_BT_CTLR_CONN_META */

	conn->connect_expire = 6U;
	conn->supervision_expire = 0U;
	conn_interval_us = (uint32_t)interval * CONN_INT_UNIT_US;
	conn->supervision_reload = RADIO_CONN_EVENTS(timeout * 10000U, conn_interval_us);

	conn->procedure_expire = 0U;
	conn->procedure_reload = RADIO_CONN_EVENTS(40000000, conn_interval_us);

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

	conn->master.terminate_ack = 0U;

	/* Re-initialize the control procedure data structures */
	ll_conn_init(conn);

	conn->terminate.reason = 0U;

	/* NOTE: use allocated link for generating dedicated
	 * terminate ind rx node
	 */
	conn->terminate.node_rx.hdr.link = link;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	conn_lll->enc_rx = conn_lll->enc_tx = 0U;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_PHY)
	conn->phy_pref_tx = ull_conn_default_phy_tx_get();
	conn->phy_pref_rx = ull_conn_default_phy_rx_get();
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_PHY)
	ready_delay_us = lll_radio_tx_ready_delay_get(conn_lll->phy_tx, conn_lll->phy_flags);
#else
	ready_delay_us = lll_radio_tx_ready_delay_get(0, 0);
#endif

	/* Re-initialize the Tx Q */
	ull_tx_q_init(&conn->tx_q);

	/* TODO: active_to_start feature port */
	conn->ull.ticks_active_to_start = 0U;
	conn->ull.ticks_prepare_to_start = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	conn->ull.ticks_preempt_to_start = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	conn->ull.ticks_slot = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US + ready_delay_us +
						      328 + EVENT_IFS_US + 328);

	lll->conn = conn_lll;

	ull_hdr_init(&conn->ull);
	lll_hdr_init(&conn->lll, conn);

conn_is_valid:
#if defined(CONFIG_BT_CTLR_PRIVACY)
	ull_filter_scan_update(filter_policy);

	lll->rl_idx = FILTER_IDX_NONE;
	lll->rpa_gen = 0;
	if (!filter_policy && ull_filter_lll_rl_enabled()) {
		/* Look up the resolving list */
		lll->rl_idx = ull_filter_rl_find(peer_addr_type, peer_addr, NULL);
	}

	if (own_addr_type == BT_ADDR_LE_PUBLIC_ID || own_addr_type == BT_ADDR_LE_RANDOM_ID) {
		/* Generate RPAs if required */
		ull_filter_rpa_update(false);
		own_addr_type &= 0x1;
		lll->rpa_gen = 1;
	}
#endif

	scan->own_addr_type = own_addr_type;

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

	if (!is_coded_included || (scan->lll.phy & PHY_1M)) {
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

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) && IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
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

	conn_lll = scan_lll->conn;
	if (!conn_lll) {
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
		struct ll_conn *conn;
		struct node_rx_pdu *node_rx;
		struct node_rx_cc *cc;
		memq_link_t *link;

		conn = HDR_LLL2ULL(conn_lll);
		node_rx = (void *)&conn->terminate.node_rx;
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

/* EGON TODO: see the implementation in ull_chan.c; this function needs updating */
/* FIXME: Refactor out this interface so that its usable by extended
 * advertising channel classification, and also master role connections can
 * perform channel map update control procedure.
 */
uint8_t ll_chm_update(uint8_t const *const chm)
{
	uint16_t handle;
	uint8_t count;

	/* RFU bits */
	if (chm[4] & 0xE0) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/*
	 * HCI requires at least 1 channel to be unknown but LL requires at
	 * least 2...
	 */
	count = util_ones_count_get(chm, 5);
	if (count < 2) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	ull_chan_map_set(chm);

	handle = CONFIG_BT_MAX_CONN;
	while (handle--) {
		struct ll_conn *conn;

		conn = ll_connected_get(handle);
		if (!conn || (conn->lll.role == BT_HCI_ROLE_PERIPHERAL)) {
			continue;
		}

		/*
		 * initiate procedure for updating peer channel map
		 *
		 * TODO should we check error here (could happen if out of
		 * free context)?
		 */
		ull_cp_chan_map_update(conn, chm);
	}

	return BT_HCI_ERR_SUCCESS;
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
uint8_t ll_enc_req_send(uint16_t handle, uint8_t const *const rand_num, uint8_t const *const ediv,
			uint8_t const *const ltk)
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
	} else {
		/* Encryption is in limbo */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
}
#endif /* CONFIG_BT_CTLR_LE_ENC */
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
uint8_t ll_set_min_used_chans(uint16_t handle, uint8_t const phys, uint8_t const min_used_chans)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (!conn->lll.role) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	return ull_cp_min_used_chans(conn, phys, min_used_chans);
}
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */

/*
 * EGON: the ll_tx_mem routines should go to their own module
 */
void *ll_tx_mem_acquire(void)
{
	return ull_conn_tx_mem_acquire();
}

void ll_tx_mem_release(void *tx)
{
	ull_conn_tx_mem_release(tx);
}

int ll_tx_mem_enqueue(uint16_t handle, void *tx)
{
#if defined(CONFIG_BT_CTLR_THROUGHPUT)
#define BT_CTLR_THROUGHPUT_PERIOD 1000000000UL
	static uint32_t tx_rate;
	static uint32_t tx_cnt;
#endif /* CONFIG_BT_CTLR_THROUGHPUT */
	struct lll_tx *lll_tx;
	struct ll_conn *conn;
	uint8_t idx;

	conn = ll_connected_get(handle);
	if (!conn) {
		return -EINVAL;
	}

	idx = ull_conn_mfifo_get_tx((void **)&lll_tx);
	if (!lll_tx) {
		return -ENOBUFS;
	}

	lll_tx->handle = handle;
	lll_tx->node = tx;

	ull_conn_mfifo_enqueue_tx(idx);

	if (ull_ref_get(&conn->ull)) {
		static memq_link_t link;
		static struct mayfly mfy = { 0, 0, &link, NULL, tx_demux };

#if defined(CONFIG_BT_CTLR_FORCE_MD_AUTO)
		if (tx_cnt >= CONFIG_BT_BUF_ACL_TX_COUNT) {
			uint8_t previous, force_md_cnt;

			force_md_cnt = force_md_cnt_calc(&conn->lll, tx_rate);
			previous = lll_conn_force_md_cnt_set(force_md_cnt);
			if (previous != force_md_cnt) {
				BT_INFO("force_md_cnt: old= %u, new= %u.", previous, force_md_cnt);
			}
		}
#endif /* CONFIG_BT_CTLR_FORCE_MD_AUTO */

		mfy.param = conn;

		mayfly_enqueue(TICKER_USER_ID_THREAD, TICKER_USER_ID_ULL_HIGH, 0, &mfy);

#if defined(CONFIG_BT_CTLR_FORCE_MD_AUTO)
	} else {
		lll_conn_force_md_cnt_set(0U);
#endif /* CONFIG_BT_CTLR_FORCE_MD_AUTO */
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn->lll.role) {
		ull_slave_latency_cancel(conn, handle);
	}

#if defined(CONFIG_BT_CTLR_THROUGHPUT)
	static uint32_t last_cycle_stamp;
	static uint32_t tx_len;
	struct pdu_data *pdu;
	uint32_t cycle_stamp;
	uint64_t delta;

	cycle_stamp = k_cycle_get_32();
	delta = k_cyc_to_ns_floor64(cycle_stamp - last_cycle_stamp);
	if (delta > BT_CTLR_THROUGHPUT_PERIOD) {
		BT_INFO("incoming Tx: count= %u, len= %u, rate= %u bps.", tx_cnt, tx_len, tx_rate);

		last_cycle_stamp = cycle_stamp;
		tx_cnt = 0U;
		tx_len = 0U;
	}

	pdu = (void *)((struct node_tx *)tx)->pdu;
	tx_len += pdu->len;
	tx_rate = ((uint64_t)tx_len << 3) * BT_CTLR_THROUGHPUT_PERIOD / delta;
	tx_cnt++;
#endif /* CONFIG_BT_CTLR_THROUGHPUT */

	return 0;
}

static void tx_demux(void *param)
{
	ull_conn_tx_demux(1);

	ull_conn_tx_lll_enqueue(param, 1);
}

#if defined(CONFIG_BT_CENTRAL)
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

	cc = (void *)&conn->terminate.node_rx;
	link = cc->hdr.link;
	LL_ASSERT(link);

	ll_rx_link_release(link);

	ll_conn_release(conn);
	scan->lll.conn = NULL;
}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_CTLR_FORCE_MD_AUTO)
static uint8_t force_md_cnt_calc(struct lll_conn *lll_conn, uint32_t tx_rate)
{
	uint32_t time_incoming, time_outgoing;
	uint8_t force_md_cnt;
	uint8_t mic_size;
	uint8_t phy;

#if defined(CONFIG_BT_CTLR_PHY)
	phy = lll_conn->phy_tx;
#else /* !CONFIG_BT_CTLR_PHY */
	phy = PHY_1M;
#endif /* !CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_LE_ENC)
	mic_size = PDU_MIC_SIZE * lll_conn->enc_tx;
#else /* !CONFIG_BT_CTLR_LE_ENC */
	mic_size = 0U;
#endif /* !CONFIG_BT_CTLR_LE_ENC */

	time_incoming = (LL_LENGTH_OCTETS_RX_MAX << 3) * 1000000UL / tx_rate;
	time_outgoing = PDU_MAX_US(LL_LENGTH_OCTETS_RX_MAX, mic_size, phy) +
		PDU_MAX_US(0U, 0U, phy) +
		(EVENT_IFS_US << 1);

	force_md_cnt = 0U;
	if (time_incoming > time_outgoing) {
		uint32_t delta;
		uint32_t time_keep_alive;

		delta = (time_incoming << 1) - time_outgoing;
		time_keep_alive = (PDU_MAX_US(0U, 0U, phy) + EVENT_IFS_US) << 1;
		force_md_cnt = (delta + (time_keep_alive - 1)) / time_keep_alive;
		BT_DBG("Time: incoming= %u, expected outgoing= %u, delta= %u, "
		       "keepalive= %u, force_md_cnt = %u.",
		       time_incoming, time_outgoing, delta, time_keep_alive, force_md_cnt);
	}

	return force_md_cnt;
}
#endif /* CONFIG_BT_CTLR_FORCE_MD_AUTO */
