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

#include <stddef.h>
#include <zephyr.h>
#include <soc.h>
#include <bluetooth/bluetooth.h>
#include <sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ecb.h"
#include "hal/ccm.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"
#include "ull_tx_queue.h"

#include "pdu.h"
#include "lll.h"
#include "lll_conn.h"
#include "lll_filter.h"
#include "lll_adv.h"
#include "ull_adv_types.h"
#include "ull_filter.h"
#include "ull_conn_types.h"
#include "ull_internal.h"
#include "ull_sched_internal.h"
#include "ull_slave_internal.h"
#include "ull_master_internal.h"

#include "ull_chan_internal.h"

#include "lll_clock.h"
#include "lll_scan.h"
#include "ull_scan_types.h"
#include "ull_scan_internal.h"

#include "ull_conn_llcp_internal.h"
#include "ull_llcp.h"

#include "ll.h"
#include "bluetooth/hci.h"
#include "ll_feat.h"
#include "ull_llcp_features.h"
#include "ll_settings.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_hci
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_USER_EXT)
#include "ull_vendor.h"
#endif /* CONFIG_BT_CTLR_USER_EXT */


#if defined(CONFIG_BT_PERIPHERAL)
static void ticker_update_latency_cancel_op_cb(uint32_t ticker_status,
					       void *params);
#endif /* CONFIG_BT_PERIPHERAL */
static inline void conn_release(struct ll_scan_set *scan);

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

	return BT_HCI_ERR_CMD_DISALLOWED;
	/* EGON: for future reference
	 *
	 *	*apto = conn->apto_reload * conn->lll.interval * 125U / 1000;
	 */
}

uint8_t ll_apto_set(uint16_t handle, uint16_t apto)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return BT_HCI_ERR_CMD_DISALLOWED;

	/* EGON: for future reference
	 *
	 *	conn->apto_reload = RADIO_CONN_EVENTS(apto * 10U * 1000U,
	 *				      conn->lll.interval * 1250);
	 */
}
#endif /* CONFIG_BT_CTLR_LE_PING */

uint8_t ll_feature_req_send(uint16_t handle)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return ull_cp_feature_exchange(conn);
}



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
	/* TODO: get correct data
	 *	*tx = conn->lll.phy_tx;
	 *	*rx = conn->lll.phy_rx;
	 */
	*tx = 0;
	*rx = 0;

	return BT_HCI_ERR_SUCCESS;
}



uint8_t ll_phy_req_send(uint16_t handle, uint8_t tx, uint8_t flags, uint8_t rx)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}
	/* TODO: get correct info in conn structure
	 *	if (conn->llcp_phy.disabled ||
	 */
	if (
	    (!feature_phy_2m(conn) && !feature_phy_coded(conn))) {
		return BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
	}

	/*
	 * EGON TODO: pass correct parameters
	 */
	/*
	 * conn->llcp_phy.state = LLCP_PHY_STATE_REQ;
	 * conn->llcp_phy.cmd = 1U;
	 * conn->llcp_phy.tx = tx;
	 * conn->llcp_phy.flags = flags;
	 * conn->llcp_phy.rx = rx;
	 * conn->llcp_phy.req++;
	 */

	return ull_cp_phy_update(conn);
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

	/* TODO: get correct info in conn structure
	 *	if (conn->llcp_length.disabled ||
	 */
	if (
	    !feature_dle(conn)) {
		return BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
	}

	return BT_HCI_ERR_UNKNOWN_CMD;
}

void ll_length_max_get(uint16_t *max_tx_octets, uint16_t *max_tx_time,
		       uint16_t *max_rx_octets, uint16_t *max_rx_time)
{
	*max_tx_octets = LL_LENGTH_OCTETS_RX_MAX;
	*max_rx_octets = LL_LENGTH_OCTETS_RX_MAX;
#if defined(CONFIG_BT_CTLR_PHY)
	*max_tx_time = PKT_US(LL_LENGTH_OCTETS_RX_MAX, PHY_CODED);
	*max_rx_time = PKT_US(LL_LENGTH_OCTETS_RX_MAX, PHY_CODED);
#else /* !CONFIG_BT_CTLR_PHY */
	/* Default is 1M packet timing */
	*max_tx_time = PKT_US(LL_LENGTH_OCTETS_RX_MAX, PHY_1M);
	*max_rx_time = PKT_US(LL_LENGTH_OCTETS_RX_MAX, PHY_1M);
#endif /* !CONFIG_BT_CTLR_PHY */
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

	/*
	 * EGON: TODO: fill connection structure with place for reason
	 */

	return BT_HCI_ERR_UNKNOWN_CMD;
}


/*
 * EGON TODO: the use of cmd in the following function is unclear;
 * to be re-evaluated before merging into master
 */
uint8_t ll_conn_update(uint16_t handle, uint8_t cmd, uint8_t status, uint16_t interval_min,
		    uint16_t interval_max, uint16_t latency, uint16_t timeout)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (!cmd) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		if (feature_conn_param_req(conn)) {
			cmd++;
		} else if (conn->lll.role) {
			return BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
		}
#else /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
		if (conn->lll.role) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
#endif /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
	}

	if (!cmd) {
		/*
		 * EGON TODO: fill correct info in the connection structure
		 * as for example:
		 *		conn->llcp_cu.win_size = 1U;
		 *		conn->llcp_cu.win_offset_us = 0U;
		 *		conn->llcp_cu.interval = interval_max;
		 *		conn->llcp_cu.latency = latency;
		 *		conn->llcp_cu.timeout = timeout;
		 *		conn->llcp_cu.state = LLCP_CUI_STATE_USE;
		 *		conn->llcp_cu.cmd = 1U;
		 */
	} else {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		cmd--;

		if (cmd) {
			/*
			 * EGON TODO: fill correct info in
			 * the connection structure
			 * as for example:
			 *	conn->llcp_conn_param.status = status;
			 *	conn->llcp_conn_param.state = cmd;
			 *	conn->llcp_conn_param.cmd = 1U;
			 */
		} else {
			/*
			 * EGON TODO: fill correct info in
			 * the connection structure
			 * as for example:
			 *  conn->llcp_conn_param.status = 0U;
			 *  conn->llcp_conn_param.interval_min = interval_min;
			 *  conn->llcp_conn_param.interval_max = interval_max;
			 *  conn->llcp_conn_param.latency = latency;
			 *  conn->llcp_conn_param.timeout = timeout;
			 *  conn->llcp_conn_param.state = cmd;
			 *  conn->llcp_conn_param.cmd = 1U;
			 *  conn->llcp_conn_param.req++;
			 */
		}

#else /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
		/* CPR feature not supported */
		return BT_HCI_ERR_CMD_DISALLOWED;
#endif /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
	}

	if (!cmd) {
		/* replace with call to correct ll_conn procedure */
		return BT_HCI_ERR_UNKNOWN_CMD;
	} else {
		return BT_HCI_ERR_UNKNOWN_CMD;
	}
}



uint8_t ll_chm_get(uint16_t handle, uint8_t *chm)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	/*
	 * EGON TODO: copy channelmap in connection structure, as:
	 *	memcpy(chm, conn->lll.data_chan_map,
	 *	       sizeof(conn->lll.data_chan_map));
	 */
	return BT_HCI_ERR_UNKNOWN_CMD;
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
uint8_t ll_start_enc_req_send(uint16_t handle, uint8_t error_code,
			    uint8_t const *const ltk)
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
		ull_cp_ltk_req_reply(conn);
		return BT_HCI_ERR_SUCCESS;
	}

	return BT_HCI_ERR_UNKNOWN_CMD;
}
#endif /* CONFIG_BT_CTLR_LE_ENC && CONFIG_BT_PERIPHERAL */

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
	struct ll_scan_set *scan;
	uint32_t conn_interval_us;
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

	memq_init(conn_lll->link_tx_free, &conn_lll->memq_tx.head,
		  &conn_lll->memq_tx.tail);
	conn_lll->link_tx_free = NULL;

	conn_lll->packet_tx_head_len = 0;
	conn_lll->packet_tx_head_offset = 0;

	conn_lll->sn = 0;
	conn_lll->nesn = 0;
	conn_lll->empty = 0;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	conn_lll->max_tx_octets = PDU_DC_PAYLOAD_SIZE_MIN;
	conn_lll->max_rx_octets = PDU_DC_PAYLOAD_SIZE_MIN;

#if defined(CONFIG_BT_CTLR_PHY)
	conn_lll->max_tx_time = PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, PHY_1M);
	conn_lll->max_rx_time = PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, PHY_1M);
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	conn_lll->phy_tx = BIT(0);
	conn_lll->phy_flags = 0;
	conn_lll->phy_tx_time = BIT(0);
	conn_lll->phy_rx = BIT(0);
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	conn_lll->rssi_latest = 0x7F;
#if defined(CONFIG_BT_CTLR_CONN_RSSI_EVENT)
	conn_lll->rssi_reported = 0x7F;
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
	/* FIXME: END: Move to ULL? */
#if defined(CONFIG_BT_CTLR_CONN_META)
	memset(&conn_lll->conn_meta, 0, sizeof(conn_lll->conn_meta));
#endif /* CONFIG_BT_CTLR_CONN_META */

	conn->connect_expire = 6U;
	conn->supervision_expire = 0U;
	conn_interval_us = (uint32_t)interval * 1250U;
	conn->supervision_reload = RADIO_CONN_EVENTS(timeout * 10000U,
							 conn_interval_us);

	conn->procedure_expire = 0U;
	conn->procedure_reload = RADIO_CONN_EVENTS(40000000,
						       conn_interval_us);

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

	conn->llcp.fex.valid = 0U;

	/* NOTE: use allocated link for generating dedicated
	 * terminate ind rx node
	 */
	conn->llcp.terminate.node_rx.hdr.link = link;

	conn->llcp.fex.features_used = LL_FEAT;
	conn->llcp.fex.features_peer = 0;

	conn->default_tx_octets = ull_conn_default_tx_octets_get();

#if defined(CONFIG_BT_CTLR_LE_ENC)
	conn_lll->enc_rx = conn_lll->enc_tx = 0U;
#endif /* CONFIG_BT_CTLR_LE_ENC */


#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	conn->default_tx_octets = ull_conn_default_tx_octets_get();

#if defined(CONFIG_BT_CTLR_PHY)
	conn->default_tx_time = ull_conn_default_tx_time_get();
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	conn->phy_pref_tx = ull_conn_default_phy_tx_get();
	conn->phy_pref_rx = ull_conn_default_phy_rx_get();
	conn->phy_pref_flags = 0U;
#endif /* CONFIG_BT_CTLR_PHY */

	conn->tx_head = conn->tx_ctrl = conn->tx_ctrl_last =
	conn->tx_data = conn->tx_data_last = 0;

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
	    (scan->lll.phy & BIT(0))) {
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
	struct lll_conn *conn_lll;
	struct ll_scan_set *scan;
	uint8_t status;

	scan = ull_scan_is_enabled_get(0);
	if (!scan) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	conn_lll = scan->lll.conn;
	if (!conn_lll) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	status = ull_scan_disable(0, scan);
	if (!status) {
		struct ll_conn *conn = (void *)HDR_LLL2EVT(conn_lll);
		struct node_rx_ftr *ftr;
		struct node_rx_pdu *cc;
		memq_link_t *link;

		cc = (void *)&conn->llcp.terminate.node_rx;
		link = cc->hdr.link;
		LL_ASSERT(link);

		/* free the memq link early, as caller could overwrite it */
		ll_rx_link_release(link);

		cc->hdr.type = NODE_RX_TYPE_CONNECTION;
		cc->hdr.handle = 0xffff;
		*((uint8_t *)cc->pdu) = BT_HCI_ERR_UNKNOWN_CONN_ID;

		ftr = &(cc->hdr.rx_ftr);
		ftr->param = &scan->lll;

		*rx = cc;
	}

	return status;
}

uint8_t ll_chm_update(uint8_t const *const chm)
{
	uint16_t handle;

	ull_chan_map_set(chm);

	handle = CONFIG_BT_MAX_CONN;
	while (handle--) {
		struct ll_conn *conn;

		conn = ll_connected_get(handle);
		if (!conn || conn->lll.role) {
			continue;
		}

		/*
		 * initiate procedure for updating peer channel map
		 */
	}

	return BT_HCI_ERR_UNKNOWN_CMD;
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
uint8_t ll_enc_req_send(uint16_t handle, uint8_t const *const rand, uint8_t const *const ediv, uint8_t const *const ltk)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}
	/*
	 * we need to enhance the conn structure so that parameters
	 * are passed on
	 */
	return ull_cp_encryption_start(conn);
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

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

	struct lll_tx *lll_tx;
	struct ll_conn *conn;
	uint8_t idx;

	conn = ll_connected_get(handle);
	if (!conn) {
		return -EINVAL;
	}

	idx = ull_conn_mfifo_get_tx((void **) &lll_tx);
	if (!lll_tx) {
		return -ENOBUFS;
	}

	lll_tx->handle = handle;
	lll_tx->node = tx;

	ull_conn_mfifo_enqueue_tx(idx);

#if defined(CONFIG_BT_PERIPHERAL)
	/* break slave latency */
	if (conn->lll.role && conn->lll.latency_event &&
	    !conn->slave.latency_cancel) {
		uint32_t ticker_status;

		conn->slave.latency_cancel = 1U;

		ticker_status =
			ticker_update(TICKER_INSTANCE_ID_CTLR,
				      TICKER_USER_ID_THREAD,
				      (TICKER_ID_CONN_BASE + handle),
				      0, 0, 0, 0, 1, 0,
				      ticker_update_latency_cancel_op_cb,
				      (void *)conn);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	}
#endif /* CONFIG_BT_PERIPHERAL */
	return 0;
}

#if defined(CONFIG_BT_PERIPHERAL)
static void ticker_update_latency_cancel_op_cb(uint32_t ticker_status,
					       void *params)
{
	struct ll_conn *conn = params;

	LL_ASSERT(ticker_status == TICKER_STATUS_SUCCESS);

	conn->slave.latency_cancel = 0U;
}
#endif /* CONFIG_BT_PERIPHERAL */

static inline void conn_release(struct ll_scan_set *scan)
{
	struct lll_conn *lll = scan->lll.conn;
	struct node_rx_pdu *cc;
	struct ll_conn *conn;
	memq_link_t *link;

	LL_ASSERT(!lll->link_tx_free);
	link = memq_deinit(&lll->memq_tx.head, &lll->memq_tx.tail);
	LL_ASSERT(link);
	lll->link_tx_free = link;

	conn = (void *)HDR_LLL2EVT(lll);

	cc = (void *)&conn->llcp.terminate.node_rx;
	link = cc->hdr.link;
	LL_ASSERT(link);

	ll_rx_link_release(link);

	ll_conn_release(conn);
	scan->lll.conn = NULL;
}
