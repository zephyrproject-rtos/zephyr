/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stddef.h>
#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <sys/byteorder.h>

#include "hal/ecb.h"
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "lll.h"
#include "lll_conn.h"
#include "ull_conn_types.h"
#include "ull_internal.h"
#include "ull_sched_internal.h"
#include "ull_slave_internal.h"
#include "ull_master_internal.h"

#include "lll_scan.h"
#include "ull_scan_types.h"
#include "ull_scan_internal.h"

#include "ull_conn_llcp_internal.h"
#include "ull_tx_queue.h"
#include "ull_llcp.h"

#include "ll.h"
#include "bluetooth/hci.h"
#include "ll_feat.h"
#include "ull_llcp_features.h"
#include "ull_connections.h"
#include "ll_settings.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_hci
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_USER_EXT)
#include "ull_vendor.h"
#endif /* CONFIG_BT_CTLR_USER_EXT */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static u16_t default_tx_octets;
static u16_t default_tx_time;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static u8_t default_phy_tx;
static u8_t default_phy_rx;
#endif /* CONFIG_BT_CTLR_PHY */




u8_t ll_version_ind_send(u16_t handle)
{
	struct ull_cp_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return ull_cp_version_exchange(conn);
}

#if defined(CONFIG_BT_CTLR_LE_PING)
u8_t ll_apto_get(u16_t handle, u16_t *apto)
{
	struct ull_cp_conn *conn;

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

u8_t ll_apto_set(u16_t handle, u16_t apto)
{
	struct ull_cp_conn *conn;

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

u8_t ll_feature_req_send(u16_t handle)
{
	struct ull_cp_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return ull_cp_feature_exchange(conn);
}



#if defined(CONFIG_BT_CTLR_PHY)
u8_t ll_phy_default_set(u8_t tx, u8_t rx)
{
	/* TODO: validate against supported phy */

	default_phy_tx = tx;
	default_phy_rx = rx;

	return 0;
}

/*
 * the next 2 functions are not called by HCI, but
 * provide data given by an HCI call to ll_phy_default_set
 */
u8_t ull_conn_default_phy_tx_get(void)
{
	return default_phy_tx;
}

u8_t ull_conn_default_phy_rx_get(void)
{
	return default_phy_rx;
}

u8_t ll_phy_get(u16_t handle, u8_t *tx, u8_t *rx)
{
	struct ull_cp_conn *conn;

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



u8_t ll_phy_req_send(u16_t handle, u8_t tx, u8_t flags, u8_t rx)
{
	struct ull_cp_conn *conn;

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
u32_t ll_length_req_send(u16_t handle, u16_t tx_octets, u16_t tx_time)
{
	struct ull_cp_conn *conn;

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

void ll_length_max_get(u16_t *max_tx_octets, u16_t *max_tx_time,
		       u16_t *max_rx_octets, u16_t *max_rx_time)
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


void ll_length_default_get(u16_t *max_tx_octets, u16_t *max_tx_time)
{
	*max_tx_octets = default_tx_octets;
	*max_tx_time = default_tx_time;
}

u32_t ll_length_default_set(u16_t max_tx_octets, u16_t max_tx_time)
{
	/* TODO: parameter check (for BT 5.0 compliance) */

	default_tx_octets = max_tx_octets;
	default_tx_time = max_tx_time;

	return 0;
}

u16_t ull_conn_default_tx_octets_get(void)
{
	return default_tx_octets;
}

#if defined(CONFIG_BT_CTLR_PHY)
u16_t ull_conn_default_tx_time_get(void)
{
	return default_tx_time;
}
#endif /* CONFIG_BT_CTLR_PHY */


#endif /* CONFIG_BT_CTLR_DATA_LENGTH */


u8_t ll_terminate_ind_send(u16_t handle, u8_t reason)
{
	struct ull_cp_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	/*
	 * EGON: TODO: fill connection structure with place for reason
	 */

	return BT_HCI_ERR_UNKNOWN_CMD;
}

u8_t ll_conn_update(u16_t handle, u8_t cmd, u8_t status, u16_t interval_min,
		    u16_t interval_max, u16_t latency, u16_t timeout)
{
	struct ull_cp_conn *conn;

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
		/* replace with call to correct ull_cp_conn procedure */
		return BT_HCI_ERR_UNKNOWN_CMD;
	} else {
		return BT_HCI_ERR_UNKNOWN_CMD;
	}
}



u8_t ll_chm_get(u16_t handle, u8_t *chm)
{
	struct ull_cp_conn *conn;

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
u8_t ll_rssi_get(u16_t handle, u8_t *rssi)
{
	struct ull_cp_conn *conn;

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
u8_t ll_start_enc_req_send(u16_t handle, u8_t error_code,
			    u8_t const *const ltk)
{
	struct ull_cp_conn *conn;

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

#if defined(CONFIG_BT_CENTRAL)
/** @brief Prepare access address as per BT Spec.
 *
 * - It shall have no more than six consecutive zeros or ones.
 * - It shall not be the advertising channel packets' Access Address.
 * - It shall not be a sequence that differs from the advertising channel
 *   packets Access Address by only one bit.
 * - It shall not have all four octets equal.
 * - It shall have no more than 24 transitions.
 * - It shall have a minimum of two transitions in the most significant six
 *   bits.
 *
 * LE Coded PHY requirements:
 * - It shall have at least three ones in the least significant 8 bits.
 * - It shall have no more than eleven transitions in the least significant 16
 *   bits.
 */
static inline void access_addr_get(u8_t access_addr[])
{
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	u8_t transitions_lsb16;
	u8_t ones_count_lsb8;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	u8_t consecutive_cnt;
	u8_t consecutive_bit;
	u32_t adv_aa_check;
	u32_t aa;
	u8_t transitions;
	u8_t bit_idx;
	u8_t retry;

	retry = 3U;
again:
	LL_ASSERT(retry);
	retry--;

	util_rand(access_addr, 4);
	aa = sys_get_le32(access_addr);

	bit_idx = 31U;
	transitions = 0U;
	consecutive_cnt = 1U;
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	ones_count_lsb8 = 0U;
	transitions_lsb16 = 0U;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	consecutive_bit = (aa >> bit_idx) & 0x01;
	while (bit_idx--) {
#if defined(CONFIG_BT_CTLR_PHY_CODED)
		u8_t transitions_lsb16_prev = transitions_lsb16;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		u8_t consecutive_cnt_prev = consecutive_cnt;
		u8_t transitions_prev = transitions;
		u8_t bit;

		bit = (aa >> bit_idx) & 0x01;
		if (bit == consecutive_bit) {
			consecutive_cnt++;
		} else {
			consecutive_cnt = 1U;
			consecutive_bit = bit;
			transitions++;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
			if (bit_idx < 15) {
				transitions_lsb16++;
			}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		}

#if defined(CONFIG_BT_CTLR_PHY_CODED)
		if ((bit_idx < 8) && consecutive_bit) {
			ones_count_lsb8++;
		}
#endif /* CONFIG_BT_CTLR_PHY_CODED */

		/* It shall have no more than six consecutive zeros or ones. */
		/* It shall have a minimum of two transitions in the most
		 * significant six bits.
		 */
		if ((consecutive_cnt > 6) ||
#if defined(CONFIG_BT_CTLR_PHY_CODED)
		    (!consecutive_bit && (((bit_idx < 6) &&
					   (ones_count_lsb8 < 1)) ||
					  ((bit_idx < 5) &&
					   (ones_count_lsb8 < 2)) ||
					  ((bit_idx < 4) &&
					   (ones_count_lsb8 < 3)))) ||
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		    ((consecutive_cnt < 6) &&
		     (((bit_idx < 29) && (transitions < 1)) ||
		      ((bit_idx < 28) && (transitions < 2))))) {
			if (consecutive_bit) {
				consecutive_bit = 0U;
				aa &= ~BIT(bit_idx);
#if defined(CONFIG_BT_CTLR_PHY_CODED)
				if (bit_idx < 8) {
					ones_count_lsb8--;
				}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
			} else {
				consecutive_bit = 1U;
				aa |= BIT(bit_idx);
#if defined(CONFIG_BT_CTLR_PHY_CODED)
				if (bit_idx < 8) {
					ones_count_lsb8++;
				}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
			}

			if (transitions != transitions_prev) {
				consecutive_cnt = consecutive_cnt_prev;
				transitions = transitions_prev;
			} else {
				consecutive_cnt = 1U;
				transitions++;
			}

#if defined(CONFIG_BT_CTLR_PHY_CODED)
			if (bit_idx < 15) {
				if (transitions_lsb16 !=
				    transitions_lsb16_prev) {
					transitions_lsb16 =
						transitions_lsb16_prev;
				} else {
					transitions_lsb16++;
				}
			}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		}

		/* It shall have no more than 24 transitions
		 * It shall have no more than eleven transitions in the least
		 * significant 16 bits.
		 */
		if ((transitions > 24) ||
#if defined(CONFIG_BT_CTLR_PHY_CODED)
		    (transitions_lsb16 > 11) ||
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		    0) {
			if (consecutive_bit) {
				aa &= ~(BIT(bit_idx + 1) - 1);
			} else {
				aa |= (BIT(bit_idx + 1) - 1);
			}

			break;
		}
	}

	/* It shall not be the advertising channel packets Access Address.
	 * It shall not be a sequence that differs from the advertising channel
	 * packets Access Address by only one bit.
	 */
	adv_aa_check = aa ^ PDU_AC_ACCESS_ADDR;
	if (util_ones_count_get((u8_t *)&adv_aa_check,
				sizeof(adv_aa_check)) <= 1) {
		goto again;
	}

	/* It shall not have all four octets equal. */
	if (!((aa & 0xFFFF) ^ (aa >> 16)) &&
	    !((aa & 0xFF) ^ (aa >> 24))) {
		goto again;
	}

	sys_put_le32(aa, access_addr);
}


u8_t ll_create_connection(u16_t scan_interval, u16_t scan_window,
			  u8_t filter_policy, u8_t peer_addr_type,
			  u8_t *peer_addr, u8_t own_addr_type,
			  u16_t interval, u16_t latency, u16_t timeout)
{
	/*
	 * disabled for now
	 */
#if 0
	struct ull_cp_conn *conn_lll;
	struct ll_scan_set *scan;
	u32_t conn_interval_us;
	struct lll_scan *lll;
	struct ull_cp_conn *conn;
	memq_link_t *link;
	u8_t access_addr[4];
	u8_t hop;
	int err;

	scan = ull_scan_is_disabled_get(0);
	if (!scan) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lll = &scan->lll;
	if (lll->conn) {
		return BT_HCI_ERR_CMD_DISALLOWED;
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
	lll->conn_ticks_slot = 0; /* TODO: */

	conn_lll = &conn->lll;

	access_addr_get(access_addr);
	memcpy(conn_lll->access_addr, &access_addr,
	       sizeof(conn_lll->access_addr));
	util_rand(&conn_lll->crc_init[0], 3);

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

	conn_lll->data_chan_count =
		ull_conn_chan_map_cpy(conn_lll->data_chan_map);
	util_rand(&hop, sizeof(u8_t));
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
	conn_interval_us = (u32_t)interval * 1250U;
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

	conn->common.fex_valid = 0U;
	conn->master.terminate_ack = 0U;

	conn->llcp_req = conn->llcp_ack = conn->llcp_type = 0U;
	conn->llcp_rx = NULL;
	conn->llcp_cu.req = conn->llcp_cu.ack = 0;
	conn->llcp_feature.req = conn->llcp_feature.ack = 0;
	conn->llcp_feature.features_conn = LL_FEAT;
	conn->llcp_feature.features_peer = 0;
	conn->llcp_version.req = conn->llcp_version.ack = 0;
	conn->llcp_version.tx = conn->llcp_version.rx = 0U;
	conn->llcp_terminate.reason_peer = 0U;
	/* NOTE: use allocated link for generating dedicated
	 * terminate ind rx node
	 */
	conn->llcp_terminate.node_rx.hdr.link = link;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	conn_lll->enc_rx = conn_lll->enc_tx = 0U;
	conn->llcp_enc.req = conn->llcp_enc.ack = 0U;
	conn->llcp_enc.pause_tx = conn->llcp_enc.pause_rx = 0U;
	conn->llcp_enc.refresh = 0U;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	conn->llcp_conn_param.req = 0U;
	conn->llcp_conn_param.ack = 0U;
	conn->llcp_conn_param.disabled = 0U;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	conn->llcp_length.req = conn->llcp_length.ack = 0U;
	conn->llcp_length.disabled = 0U;
	conn->llcp_length.cache.tx_octets = 0U;
	conn->default_tx_octets = ull_conn_default_tx_octets_get();

#if defined(CONFIG_BT_CTLR_PHY)
	conn->default_tx_time = ull_conn_default_tx_time_get();
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	conn->llcp_phy.req = conn->llcp_phy.ack = 0U;
	conn->llcp_phy.disabled = 0U;
	conn->llcp_phy.pause_tx = 0U;
	conn->phy_pref_tx = ull_conn_default_phy_tx_get();
	conn->phy_pref_rx = ull_conn_default_phy_rx_get();
	conn->phy_pref_flags = 0U;
#endif /* CONFIG_BT_CTLR_PHY */

	conn->tx_head = conn->tx_ctrl = conn->tx_ctrl_last =
	conn->tx_data = conn->tx_data_last = 0;

	lll->conn = conn_lll;

	ull_hdr_init(&conn->ull);
	lll_hdr_init(&conn->lll, conn);

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

	/* wait for stable clocks */
	err = lll_clock_wait();
	if (err) {
		conn_release(scan);

		return BT_HCI_ERR_HW_FAILURE;
	}

	return ull_scan_enable(scan);
#else
	return 0;
#endif /* 0 */
}

u8_t ll_connect_disable(void **rx)
{
	/*
	 * disable for now
	 */
#if 0
	struct lll_conn *conn_lll;
	struct ll_scan_set *scan;
	u8_t status;

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

		cc = (void *)&conn->llcp_terminate.node_rx;
		link = cc->hdr.link;
		LL_ASSERT(link);

		/* free the memq link early, as caller could overwrite it */
		ll_rx_link_release(link);

		cc->hdr.type = NODE_RX_TYPE_CONNECTION;
		cc->hdr.handle = 0xffff;
		*((u8_t *)cc->pdu) = BT_HCI_ERR_UNKNOWN_CONN_ID;

		ftr = &(cc->hdr.rx_ftr);
		ftr->param = &scan->lll;

		*rx = cc;
	}

	return status;
#else
	return 0;
#endif /* 0 */
}

u8_t ll_chm_update(u8_t *chm)
{
	u16_t handle;

	ull_conn_chan_map_set(chm);

	handle = CONFIG_BT_MAX_CONN;
	while (handle--) {
		struct ull_cp_conn *conn;

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

u8_t ll_enc_req_send(u16_t handle, u8_t *rand, u8_t *ediv, u8_t *ltk)
{
	struct ull_cp_conn *conn;

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
#if defined(CONFIG_BT_CTLR_LE_ENC)

#endif /* CONFIG_BT_CTLR_LE_ENC */
#endif /* CONFIG_BT_CENTRAL */

/*
 * EGON: the ll_tx_mem routines should go to their own module
 */
void *ll_tx_mem_acquire(void)
{
	return ll_tx_mem_acquire();
}

void ll_tx_mem_release(void *tx)
{
	ll_tx_mem_release(tx);
}

int ll_tx_mem_enqueue(u16_t handle, void *tx)
{
	return ll_tx_mem_enqueue(handle, tx);
}

uint8_t ull_llcp_hci_init(void)
{
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	/* Initialize the DLE defaults */
	default_tx_octets = PDU_DC_PAYLOAD_SIZE_MIN;
	default_tx_time = PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, PHY_1M);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	/* Initialize the PHY defaults */
	default_phy_tx = BIT(0);
	default_phy_rx = BIT(0);

#if defined(CONFIG_BT_CTLR_PHY_2M)
	default_phy_tx |= BIT(1);
	default_phy_rx |= BIT(1);
#endif /* CONFIG_BT_CTLR_PHY_2M */

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	default_phy_tx |= BIT(2);
	default_phy_rx |= BIT(2);
#endif /* CONFIG_BT_CTLR_PHY_CODED */
#endif /* CONFIG_BT_CTLR_PHY */

	return 0;

}
