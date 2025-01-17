/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ecb.h"
#include "hal/ccm.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"
#include "lll/lll_vendor.h"

#include "ll_sw/ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"

#if defined(CONFIG_BT_CTLR_USER_EXT)
#include "ull_vendor.h"
#endif /* CONFIG_BT_CTLR_USER_EXT */

#include "ull_internal.h"
#include "ull_llcp_internal.h"
#include "ull_sched_internal.h"
#include "ull_chan_internal.h"
#include "ull_conn_internal.h"
#include "ull_peripheral_internal.h"
#include "ull_central_internal.h"

#include "ull_iso_internal.h"
#include "ull_conn_iso_internal.h"
#include "ull_peripheral_iso_internal.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "ull_adv_types.h"
#include "ull_adv_internal.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"
#include "ull_sync_types.h"
#include "lll_scan.h"
#include "ull_scan_types.h"
#include "ull_sync_internal.h"

#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#include "ll_sw/ull_llcp.h"
#include "ll_sw/ull_llcp_features.h"

#include "hal/debug.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_ctlr_ull_conn);

static int init_reset(void);
#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
static void tx_demux_sched(struct ll_conn *conn);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */
static void tx_demux(void *param);
static struct node_tx *tx_ull_dequeue(struct ll_conn *conn, struct node_tx *tx);

static void ticker_update_conn_op_cb(uint32_t status, void *param);
static void ticker_stop_conn_op_cb(uint32_t status, void *param);
static void ticker_start_conn_op_cb(uint32_t status, void *param);

static void conn_setup_adv_scan_disabled_cb(void *param);
static inline void disable(uint16_t handle);
static void conn_cleanup(struct ll_conn *conn, uint8_t reason);
static void conn_cleanup_finalize(struct ll_conn *conn);
static void tx_ull_flush(struct ll_conn *conn);
static void ticker_stop_op_cb(uint32_t status, void *param);
static void conn_disable(void *param);
static void disabled_cb(void *param);
static void tx_lll_flush(void *param);

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
static int empty_data_start_release(struct ll_conn *conn, struct node_tx *tx);
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
/* Connection context pointer used as CPR mutex to serialize connection
 * parameter requests procedures across simultaneous connections so that
 * offsets exchanged to the peer do not get changed.
 */
struct ll_conn *conn_upd_curr;
#endif /* defined(CONFIG_BT_CTLR_CONN_PARAM_REQ) */

#if defined(CONFIG_BT_CTLR_FORCE_MD_AUTO)
static uint8_t force_md_cnt_calc(struct lll_conn *lll_conn, uint32_t tx_rate);
#endif /* CONFIG_BT_CTLR_FORCE_MD_AUTO */

#if !defined(BT_CTLR_USER_TX_BUFFER_OVERHEAD)
#define BT_CTLR_USER_TX_BUFFER_OVERHEAD 0
#endif /* BT_CTLR_USER_TX_BUFFER_OVERHEAD */

#define CONN_TX_BUF_SIZE MROUND(offsetof(struct node_tx, pdu) + \
				offsetof(struct pdu_data, lldata) + \
				(LL_LENGTH_OCTETS_TX_MAX + \
				BT_CTLR_USER_TX_BUFFER_OVERHEAD))

#define CONN_DATA_BUFFERS CONFIG_BT_BUF_ACL_TX_COUNT

static MFIFO_DEFINE(conn_tx, sizeof(struct lll_tx), CONN_DATA_BUFFERS);
static MFIFO_DEFINE(conn_ack, sizeof(struct lll_tx),
		    (CONN_DATA_BUFFERS +
		     LLCP_TX_CTRL_BUF_COUNT));

static struct {
	void *free;
	uint8_t pool[CONN_TX_BUF_SIZE * CONN_DATA_BUFFERS];
} mem_conn_tx;

static struct {
	void *free;
	uint8_t pool[sizeof(memq_link_t) *
		     (CONN_DATA_BUFFERS +
		      LLCP_TX_CTRL_BUF_COUNT)];
} mem_link_tx;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static uint16_t default_tx_octets;
static uint16_t default_tx_time;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static uint8_t default_phy_tx;
static uint8_t default_phy_rx;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
static struct past_params default_past_params;
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER */

static struct ll_conn conn_pool[CONFIG_BT_MAX_CONN];
static void *conn_free;

struct ll_conn *ll_conn_acquire(void)
{
	return mem_acquire(&conn_free);
}

void ll_conn_release(struct ll_conn *conn)
{
	mem_release(conn, &conn_free);
}

uint16_t ll_conn_handle_get(struct ll_conn *conn)
{
	return mem_index_get(conn, conn_pool, sizeof(struct ll_conn));
}

struct ll_conn *ll_conn_get(uint16_t handle)
{
	return mem_get(conn_pool, sizeof(struct ll_conn), handle);
}

struct ll_conn *ll_connected_get(uint16_t handle)
{
	struct ll_conn *conn;

	if (handle >= CONFIG_BT_MAX_CONN) {
		return NULL;
	}

	conn = ll_conn_get(handle);
	if (conn->lll.handle != handle) {
		return NULL;
	}

	return conn;
}

uint16_t ll_conn_free_count_get(void)
{
	return mem_free_count_get(conn_free);
}

void *ll_tx_mem_acquire(void)
{
	return mem_acquire(&mem_conn_tx.free);
}

void ll_tx_mem_release(void *tx)
{
	mem_release(tx, &mem_conn_tx.free);
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

	idx = MFIFO_ENQUEUE_GET(conn_tx, (void **) &lll_tx);
	if (!lll_tx) {
		return -ENOBUFS;
	}

	lll_tx->handle = handle;
	lll_tx->node = tx;

	MFIFO_ENQUEUE(conn_tx, idx);

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	if (ull_ref_get(&conn->ull)) {
#if defined(CONFIG_BT_CTLR_FORCE_MD_AUTO)
		if (tx_cnt >= CONFIG_BT_BUF_ACL_TX_COUNT) {
			uint8_t previous, force_md_cnt;

			force_md_cnt = force_md_cnt_calc(&conn->lll, tx_rate);
			previous = lll_conn_force_md_cnt_set(force_md_cnt);
			if (previous != force_md_cnt) {
				LOG_INF("force_md_cnt: old= %u, new= %u.", previous, force_md_cnt);
			}
		}
#endif /* CONFIG_BT_CTLR_FORCE_MD_AUTO */

		tx_demux_sched(conn);

#if defined(CONFIG_BT_CTLR_FORCE_MD_AUTO)
	} else {
		lll_conn_force_md_cnt_set(0U);
#endif /* CONFIG_BT_CTLR_FORCE_MD_AUTO */
	}
#endif /* !CONFIG_BT_CTLR_LOW_LAT_ULL */

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn->lll.role) {
		ull_periph_latency_cancel(conn, handle);
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
		LOG_INF("incoming Tx: count= %u, len= %u, rate= %u bps.", tx_cnt, tx_len, tx_rate);

		last_cycle_stamp = cycle_stamp;
		tx_cnt = 0U;
		tx_len = 0U;
	}

	pdu = (void *)((struct node_tx *)tx)->pdu;
	tx_len += pdu->len;
	if (delta == 0) { /* Let's avoid a division by 0 if we happen to have a really fast HCI IF*/
		delta = 1;
	}
	tx_rate = ((uint64_t)tx_len << 3) * BT_CTLR_THROUGHPUT_PERIOD / delta;
	tx_cnt++;
#endif /* CONFIG_BT_CTLR_THROUGHPUT */

	return 0;
}

uint8_t ll_conn_update(uint16_t handle, uint8_t cmd, uint8_t status, uint16_t interval_min,
		    uint16_t interval_max, uint16_t latency, uint16_t timeout, uint16_t *offset)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (cmd == 0U) {
		uint8_t err;

		err = ull_cp_conn_update(conn, interval_min, interval_max, latency, timeout,
					 offset);
		if (err) {
			return err;
		}

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
		    conn->lll.role) {
			ull_periph_latency_cancel(conn, handle);
		}
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

	return 0;
}

uint8_t ll_chm_get(uint16_t handle, uint8_t *chm)
{
	struct ll_conn *conn;

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
	 * Connection_Handle, regardless of whether the Central has received an
	 * acknowledgment
	 */
	const uint8_t *pending_chm;

	pending_chm = ull_cp_chan_map_update_pending(conn);
	if (pending_chm) {
		memcpy(chm, pending_chm, sizeof(conn->lll.data_chan_map));
	} else {
		memcpy(chm, conn->lll.data_chan_map, sizeof(conn->lll.data_chan_map));
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
uint8_t ll_req_peer_sca(uint16_t handle)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return ull_cp_req_peer_sca(conn);
}
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

static bool is_valid_disconnect_reason(uint8_t reason)
{
	switch (reason) {
	case BT_HCI_ERR_AUTH_FAIL:
	case BT_HCI_ERR_REMOTE_USER_TERM_CONN:
	case BT_HCI_ERR_REMOTE_LOW_RESOURCES:
	case BT_HCI_ERR_REMOTE_POWER_OFF:
	case BT_HCI_ERR_UNSUPP_REMOTE_FEATURE:
	case BT_HCI_ERR_PAIRING_NOT_SUPPORTED:
	case BT_HCI_ERR_UNACCEPT_CONN_PARAM:
		return true;
	default:
		return false;
	}
}

uint8_t ll_terminate_ind_send(uint16_t handle, uint8_t reason)
{
	struct ll_conn *conn;
#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) || defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	struct ll_conn_iso_stream *cis;
#endif

	if (IS_ACL_HANDLE(handle)) {
		conn = ll_connected_get(handle);

		/* Is conn still connected? */
		if (!conn) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		if (!is_valid_disconnect_reason(reason)) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		uint8_t err;

		err = ull_cp_terminate(conn, reason);
		if (err) {
			return err;
		}

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn->lll.role) {
			ull_periph_latency_cancel(conn, handle);
		}
		return 0;
	}
#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) || defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	if (IS_CIS_HANDLE(handle)) {
		cis = ll_iso_stream_connected_get(handle);
		if (!cis) {
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
			/* CIS is not connected - get the unconnected instance */
			cis = ll_conn_iso_stream_get(handle);

			/* Sanity-check instance to make sure it's created but not connected */
			if (cis->group && cis->lll.handle == handle && !cis->established) {
				if (cis->group->state == CIG_STATE_CONFIGURABLE) {
					/* Disallow if CIG is still in configurable state */
					return BT_HCI_ERR_CMD_DISALLOWED;

				} else if (cis->group->state == CIG_STATE_INITIATING) {
					conn = ll_connected_get(cis->lll.acl_handle);

					/* CIS is not yet established - try to cancel procedure */
					if (ull_cp_cc_cancel(conn)) {
						/* Successfully canceled - complete disconnect */
						struct node_rx_pdu *node_terminate;

						node_terminate = ull_pdu_rx_alloc();
						LL_ASSERT(node_terminate);

						node_terminate->hdr.handle = handle;
						node_terminate->hdr.type = NODE_RX_TYPE_TERMINATE;
						*((uint8_t *)node_terminate->pdu) =
							BT_HCI_ERR_LOCALHOST_TERM_CONN;

						ll_rx_put_sched(node_terminate->hdr.link,
							node_terminate);

						/* We're no longer initiating a connection */
						cis->group->state = CIG_STATE_CONFIGURABLE;

						/* This is now a successful disconnection */
						return BT_HCI_ERR_SUCCESS;
					}

					/* Procedure could not be canceled in the current
					 * state - let it run its course and enqueue a
					 * terminate procedure.
					 */
					return ull_cp_cis_terminate(conn, cis, reason);
				}
			}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */
			/* Disallow if CIS is not connected */
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		conn = ll_connected_get(cis->lll.acl_handle);
		/* Disallow if ACL has disconnected */
		if (!conn) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		return ull_cp_cis_terminate(conn, cis, reason);
	}
#endif /* defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) || defined(CONFIG_BT_CTLR_CENTRAL_ISO) */

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
}

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG)
uint8_t ll_feature_req_send(uint16_t handle)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	uint8_t err;

	err = ull_cp_feature_exchange(conn, 1U);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    IS_ENABLED(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG) &&
	    conn->lll.role) {
		ull_periph_latency_cancel(conn, handle);
	}

	return 0;
}
#endif /* CONFIG_BT_CENTRAL || CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG */

uint8_t ll_version_ind_send(uint16_t handle)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	uint8_t err;

	err = ull_cp_version_exchange(conn);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn->lll.role) {
		ull_periph_latency_cancel(conn, handle);
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static bool ll_len_validate(uint16_t tx_octets, uint16_t tx_time)
{
	/* validate if within HCI allowed range */
	if (!IN_RANGE(tx_octets, PDU_DC_PAYLOAD_SIZE_MIN,
		      PDU_DC_PAYLOAD_SIZE_MAX)) {
		return false;
	}

	/* validate if within HCI allowed range */
	if (!IN_RANGE(tx_time, PDU_DC_PAYLOAD_TIME_MIN,
		      PDU_DC_PAYLOAD_TIME_MAX_CODED)) {
		return false;
	}

	return true;
}

uint32_t ll_length_req_send(uint16_t handle, uint16_t tx_octets,
			    uint16_t tx_time)
{
	struct ll_conn *conn;

	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK) &&
	    !ll_len_validate(tx_octets, tx_time)) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (!feature_dle(conn)) {
		return BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
	}

	uint8_t err;

	err = ull_cp_data_length_update(conn, tx_octets, tx_time);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn->lll.role) {
		ull_periph_latency_cancel(conn, handle);
	}

	return 0;
}

void ll_length_default_get(uint16_t *max_tx_octets, uint16_t *max_tx_time)
{
	*max_tx_octets = default_tx_octets;
	*max_tx_time = default_tx_time;
}

uint32_t ll_length_default_set(uint16_t max_tx_octets, uint16_t max_tx_time)
{
	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK) &&
	    !ll_len_validate(max_tx_octets, max_tx_time)) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	default_tx_octets = max_tx_octets;
	default_tx_time = max_tx_time;

	return 0;
}

void ll_length_max_get(uint16_t *max_tx_octets, uint16_t *max_tx_time,
		       uint16_t *max_rx_octets, uint16_t *max_rx_time)
{
#if defined(CONFIG_BT_CTLR_PHY) && defined(CONFIG_BT_CTLR_PHY_CODED)
#define PHY (PHY_CODED)
#else /* CONFIG_BT_CTLR_PHY && CONFIG_BT_CTLR_PHY_CODED */
#define PHY (PHY_1M)
#endif /* CONFIG_BT_CTLR_PHY && CONFIG_BT_CTLR_PHY_CODED */
	*max_tx_octets = LL_LENGTH_OCTETS_RX_MAX;
	*max_rx_octets = LL_LENGTH_OCTETS_RX_MAX;
	*max_tx_time = PDU_DC_MAX_US(LL_LENGTH_OCTETS_RX_MAX, PHY);
	*max_rx_time = PDU_DC_MAX_US(LL_LENGTH_OCTETS_RX_MAX, PHY);
#undef PHY
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
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

	return 0;
}

uint8_t ll_phy_default_set(uint8_t tx, uint8_t rx)
{
	/* TODO: validate against supported phy */

	default_phy_tx = tx;
	default_phy_rx = rx;

	return 0;
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

	uint8_t err;

	err = ull_cp_phy_update(conn, tx, flags, rx, 1U);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn->lll.role) {
		ull_periph_latency_cancel(conn, handle);
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
uint8_t ll_rssi_get(uint16_t handle, uint8_t *rssi)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	*rssi = conn->lll.rssi_latest;

	return 0;
}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_LE_PING)
uint8_t ll_apto_get(uint16_t handle, uint16_t *apto)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (conn->lll.interval >= BT_HCI_LE_INTERVAL_MIN) {
		*apto = conn->apto_reload * conn->lll.interval *
			CONN_INT_UNIT_US / (10U * USEC_PER_MSEC);
	} else {
		*apto = conn->apto_reload * (conn->lll.interval + 1U) *
			CONN_LOW_LAT_INT_UNIT_US / (10U * USEC_PER_MSEC);
	}

	return 0;
}

uint8_t ll_apto_set(uint16_t handle, uint16_t apto)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (conn->lll.interval >= BT_HCI_LE_INTERVAL_MIN) {
		conn->apto_reload =
			RADIO_CONN_EVENTS(apto * 10U * USEC_PER_MSEC,
					  conn->lll.interval *
					  CONN_INT_UNIT_US);
	} else {
		conn->apto_reload =
			RADIO_CONN_EVENTS(apto * 10U * USEC_PER_MSEC,
					  (conn->lll.interval + 1U) *
					  CONN_LOW_LAT_INT_UNIT_US);
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_PING */

int ull_conn_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_conn_reset(void)
{
	uint16_t handle;
	int err;

#if defined(CONFIG_BT_CENTRAL)
	/* Reset initiator */
	(void)ull_central_reset();
#endif /* CONFIG_BT_CENTRAL */

	for (handle = 0U; handle < CONFIG_BT_MAX_CONN; handle++) {
		disable(handle);
	}

	/* Re-initialize the Tx mfifo */
	MFIFO_INIT(conn_tx);

	/* Re-initialize the Tx Ack mfifo */
	MFIFO_INIT(conn_ack);

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

struct lll_conn *ull_conn_lll_get(uint16_t handle)
{
	struct ll_conn *conn;

	conn = ll_conn_get(handle);

	return &conn->lll;
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
uint16_t ull_conn_default_tx_octets_get(void)
{
	return default_tx_octets;
}

#if defined(CONFIG_BT_CTLR_PHY)
uint16_t ull_conn_default_tx_time_get(void)
{
	return default_tx_time;
}
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
uint8_t ull_conn_default_phy_tx_get(void)
{
	return default_phy_tx;
}

uint8_t ull_conn_default_phy_rx_get(void)
{
	return default_phy_rx;
}
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
void ull_conn_default_past_param_set(uint8_t mode, uint16_t skip, uint16_t timeout,
				     uint8_t cte_type)
{
	default_past_params.mode     = mode;
	default_past_params.skip     = skip;
	default_past_params.timeout  = timeout;
	default_past_params.cte_type = cte_type;
}

struct past_params ull_conn_default_past_param_get(void)
{
	return default_past_params;
}
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER */

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
bool ull_conn_peer_connected(uint8_t const own_id_addr_type,
			     uint8_t const *const own_id_addr,
			     uint8_t const peer_id_addr_type,
			     uint8_t const *const peer_id_addr)
{
	uint16_t handle;

	for (handle = 0U; handle < CONFIG_BT_MAX_CONN; handle++) {
		struct ll_conn *conn = ll_connected_get(handle);

		if (conn &&
		    conn->peer_id_addr_type == peer_id_addr_type &&
		    !memcmp(conn->peer_id_addr, peer_id_addr, BDADDR_SIZE) &&
		    conn->own_id_addr_type == own_id_addr_type &&
		    !memcmp(conn->own_id_addr, own_id_addr, BDADDR_SIZE)) {
			return true;
		}
	}

	return false;
}
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */

void ull_conn_setup(memq_link_t *rx_link, struct node_rx_pdu *rx)
{
	struct node_rx_ftr *ftr;
	struct ull_hdr *hdr;

	/* Store the link in the node rx so that when done event is
	 * processed it can be used to enqueue node rx towards LL context
	 */
	rx->hdr.link = rx_link;

	/* NOTE: LLL conn context SHALL be after lll_hdr in
	 *       struct lll_adv and struct lll_scan.
	 */
	ftr = &(rx->rx_ftr);

	/* Check for reference count and decide to setup connection
	 * here or when done event arrives.
	 */
	hdr = HDR_LLL2ULL(ftr->param);
	if (ull_ref_get(hdr)) {
		/* Setup connection in ULL disabled callback,
		 * pass the node rx as disabled callback parameter.
		 */
		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = rx;
		hdr->disabled_cb = conn_setup_adv_scan_disabled_cb;
	} else {
		conn_setup_adv_scan_disabled_cb(rx);
	}
}

void ull_conn_rx(memq_link_t *link, struct node_rx_pdu **rx)
{
	struct pdu_data *pdu_rx;
	struct ll_conn *conn;

	conn = ll_connected_get((*rx)->hdr.handle);
	if (!conn) {
		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_RELEASE;

		return;
	}

	ull_cp_tx_ntf(conn);

	pdu_rx = (void *)(*rx)->pdu;

	switch (pdu_rx->ll_id) {
	case PDU_DATA_LLID_CTRL:
	{
		/* Mark buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_RELEASE;

		ull_cp_rx(conn, link, *rx);

		return;
	}

	case PDU_DATA_LLID_DATA_CONTINUE:
	case PDU_DATA_LLID_DATA_START:
#if defined(CONFIG_BT_CTLR_LE_ENC)
		if (conn->pause_rx_data) {
			conn->llcp_terminate.reason_final =
				BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;

			/* Mark for buffer for release */
			(*rx)->hdr.type = NODE_RX_TYPE_RELEASE;
		}
#endif /* CONFIG_BT_CTLR_LE_ENC */
		break;

	case PDU_DATA_LLID_RESV:
	default:
#if defined(CONFIG_BT_CTLR_LE_ENC)
		if (conn->pause_rx_data) {
			conn->llcp_terminate.reason_final =
				BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;
		}
#endif /* CONFIG_BT_CTLR_LE_ENC */

		/* Invalid LL id, drop it. */

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_RELEASE;

		break;
	}
}

int ull_conn_llcp(struct ll_conn *conn, uint32_t ticks_at_expire,
		  uint32_t remainder, uint16_t lazy)
{
	LL_ASSERT(conn->lll.handle != LLL_HANDLE_INVALID);

	conn->llcp.prep.ticks_at_expire = ticks_at_expire;
	conn->llcp.prep.remainder = remainder;
	conn->llcp.prep.lazy = lazy;

	ull_cp_run(conn);

	if (conn->cancel_prepare) {
		/* Reset signal */
		conn->cancel_prepare = 0U;

		/* Cancel prepare */
		return -ECANCELED;
	}

	/* Continue prepare */
	return 0;
}

void ull_conn_done(struct node_rx_event_done *done)
{
	uint32_t ticks_drift_minus;
	uint32_t ticks_drift_plus;
	uint32_t ticks_slot_minus;
	uint32_t ticks_slot_plus;
	uint16_t latency_event;
	uint16_t elapsed_event;
	struct lll_conn *lll;
	struct ll_conn *conn;
	uint8_t reason_final;
	uint8_t force_lll;
	uint16_t lazy;
	uint8_t force;

	/* Get reference to ULL context */
	conn = CONTAINER_OF(done->param, struct ll_conn, ull);
	lll = &conn->lll;

	/* Skip if connection terminated by local host */
	if (unlikely(lll->handle == LLL_HANDLE_INVALID)) {
		return;
	}

	ull_cp_tx_ntf(conn);

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER)
	ull_lp_past_conn_evt_done(conn, done);
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER */

#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* Check authenticated payload expiry or MIC failure */
	switch (done->extra.mic_state) {
	case LLL_CONN_MIC_NONE:
#if defined(CONFIG_BT_CTLR_LE_PING)
		if (lll->enc_rx && lll->enc_tx) {
			uint16_t appto_reload_new;

			/* check for change in apto */
			appto_reload_new = (conn->apto_reload >
					    (lll->latency + 6)) ?
					   (conn->apto_reload -
					    (lll->latency + 6)) :
					   conn->apto_reload;
			if (conn->appto_reload != appto_reload_new) {
				conn->appto_reload = appto_reload_new;
				conn->apto_expire = 0U;
			}

			/* start authenticated payload (pre) timeout */
			if (conn->apto_expire == 0U) {
				conn->appto_expire = conn->appto_reload;
				conn->apto_expire = conn->apto_reload;
			}
		}
#endif /* CONFIG_BT_CTLR_LE_PING */
		break;

	case LLL_CONN_MIC_PASS:
#if defined(CONFIG_BT_CTLR_LE_PING)
		conn->appto_expire = conn->apto_expire = 0U;
#endif /* CONFIG_BT_CTLR_LE_PING */
		break;

	case LLL_CONN_MIC_FAIL:
		conn->llcp_terminate.reason_final =
			BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;
		break;
	}
#endif /* CONFIG_BT_CTLR_LE_ENC */

	reason_final = conn->llcp_terminate.reason_final;
	if (reason_final) {
		conn_cleanup(conn, reason_final);

		return;
	}

	/* Events elapsed used in timeout checks below */
#if defined(CONFIG_BT_CTLR_CONN_META)
	/* If event has shallow expiry do not add latency, but rely on
	 * accumulated lazy count.
	 */
	latency_event = conn->common.is_must_expire ? 0 : lll->latency_event;
#else
	latency_event = lll->latency_event;
#endif

	/* Peripheral drift compensation calc and new latency or
	 * central terminate acked
	 */
	ticks_drift_plus = 0U;
	ticks_drift_minus = 0U;
	ticks_slot_plus = 0U;
	ticks_slot_minus = 0U;

	if (done->extra.trx_cnt) {
		if (0) {
#if defined(CONFIG_BT_PERIPHERAL)
		} else if (lll->role) {
			if (!conn->periph.drift_skip) {
				ull_drift_ticks_get(done, &ticks_drift_plus,
						    &ticks_drift_minus);

				if (ticks_drift_plus || ticks_drift_minus) {
					conn->periph.drift_skip =
						ull_ref_get(&conn->ull);
				}
			} else {
				conn->periph.drift_skip--;
			}

			if (!ull_tx_q_peek(&conn->tx_q)) {
				ull_conn_tx_demux(UINT8_MAX);
			}

			if (ull_tx_q_peek(&conn->tx_q) ||
			    memq_peek(lll->memq_tx.head,
				      lll->memq_tx.tail, NULL)) {
				lll->latency_event = 0U;
			} else if (lll->periph.latency_enabled) {
				lll->latency_event = lll->latency;
			}
#endif /* CONFIG_BT_PERIPHERAL */
		}

		/* Reset connection failed to establish countdown */
		conn->connect_expire = 0U;
	}

	elapsed_event = latency_event + lll->lazy_prepare + 1U;

	/* Reset supervision countdown */
	if (done->extra.crc_valid && !done->extra.is_aborted) {
		conn->supervision_expire = 0U;
	}

	/* check connection failed to establish */
	else if (conn->connect_expire) {
		if (conn->connect_expire > elapsed_event) {
			conn->connect_expire -= elapsed_event;
		} else {
			conn_cleanup(conn, BT_HCI_ERR_CONN_FAIL_TO_ESTAB);

			return;
		}
	}

	/* if anchor point not sync-ed, start supervision timeout, and break
	 * latency if any.
	 */
	else {
		/* Start supervision timeout, if not started already */
		if (!conn->supervision_expire) {
			uint32_t conn_interval_us;

			if (conn->lll.interval >= BT_HCI_LE_INTERVAL_MIN) {
				conn_interval_us = conn->lll.interval *
						   CONN_INT_UNIT_US;
			} else {
				conn_interval_us = (conn->lll.interval + 1U) *
						   CONN_LOW_LAT_INT_UNIT_US;
			}

			conn->supervision_expire = RADIO_CONN_EVENTS(
				(conn->supervision_timeout * 10U * USEC_PER_MSEC),
				conn_interval_us);
		}
	}

	/* check supervision timeout */
	force = 0U;
	force_lll = 0U;
	if (conn->supervision_expire) {
		if (conn->supervision_expire > elapsed_event) {
			conn->supervision_expire -= elapsed_event;

			/* break latency */
			lll->latency_event = 0U;

			/* Force both central and peripheral when close to
			 * supervision timeout.
			 */
			if (conn->supervision_expire <= 6U) {
				force_lll = 1U;

				force = 1U;
			}
#if defined(CONFIG_BT_CTLR_CONN_RANDOM_FORCE)
			/* use randomness to force peripheral role when anchor
			 * points are being missed.
			 */
			else if (lll->role) {
				if (latency_event) {
					force = 1U;
				} else {
					force = conn->periph.force & 0x01;

					/* rotate force bits */
					conn->periph.force >>= 1U;
					if (force) {
						conn->periph.force |= BIT(31);
					}
				}
			}
#endif /* CONFIG_BT_CTLR_CONN_RANDOM_FORCE */
		} else {
			conn_cleanup(conn, BT_HCI_ERR_CONN_TIMEOUT);

			return;
		}
	}

	lll->forced = force_lll;

	/* check procedure timeout */
	uint8_t error_code;

	if (-ETIMEDOUT == ull_cp_prt_elapse(conn, elapsed_event, &error_code)) {
		conn_cleanup(conn, error_code);

		return;
	}

#if defined(CONFIG_BT_CTLR_LE_PING)
	/* check apto */
	if (conn->apto_expire != 0U) {
		if (conn->apto_expire > elapsed_event) {
			conn->apto_expire -= elapsed_event;
		} else {
			struct node_rx_hdr *rx;

			rx = ll_pdu_rx_alloc();
			if (rx) {
				conn->apto_expire = 0U;

				rx->handle = lll->handle;
				rx->type = NODE_RX_TYPE_APTO;

				/* enqueue apto event into rx queue */
				ll_rx_put_sched(rx->link, rx);
			} else {
				conn->apto_expire = 1U;
			}
		}
	}

	/* check appto */
	if (conn->appto_expire != 0U) {
		if (conn->appto_expire > elapsed_event) {
			conn->appto_expire -= elapsed_event;
		} else {
			conn->appto_expire = 0U;

			/* Initiate LE_PING procedure */
			ull_cp_le_ping(conn);
		}
	}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	/* Check if the CTE_REQ procedure is periodic and counter has been started.
	 * req_expire is set when: new CTE_REQ is started, after completion of last periodic run.
	 */
	if (conn->llcp.cte_req.req_interval != 0U && conn->llcp.cte_req.req_expire != 0U) {
		if (conn->llcp.cte_req.req_expire > elapsed_event) {
			conn->llcp.cte_req.req_expire -= elapsed_event;
		} else {
			uint8_t err;

			/* Set req_expire to zero to mark that new periodic CTE_REQ was started.
			 * The counter is re-started after completion of this run.
			 */
			conn->llcp.cte_req.req_expire = 0U;

			err = ull_cp_cte_req(conn, conn->llcp.cte_req.min_cte_len,
					     conn->llcp.cte_req.cte_type);

			if (err == BT_HCI_ERR_CMD_DISALLOWED) {
				/* Conditions has changed e.g. PHY was changed to CODED.
				 * New CTE REQ is not possible. Disable the periodic requests.
				 */
				ull_cp_cte_req_set_disable(conn);
			}
		}
	}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

#if defined(CONFIG_BT_CTLR_CONN_RSSI_EVENT)
	/* generate RSSI event */
	if (lll->rssi_sample_count == 0U) {
		struct node_rx_pdu *rx;
		struct pdu_data *pdu_data_rx;

		rx = ll_pdu_rx_alloc();
		if (rx) {
			lll->rssi_reported = lll->rssi_latest;
			lll->rssi_sample_count = LLL_CONN_RSSI_SAMPLE_COUNT;

			/* Prepare the rx packet structure */
			rx->hdr.handle = lll->handle;
			rx->hdr.type = NODE_RX_TYPE_RSSI;

			/* prepare connection RSSI structure */
			pdu_data_rx = (void *)rx->pdu;
			pdu_data_rx->rssi = lll->rssi_reported;

			/* enqueue connection RSSI structure into queue */
			ll_rx_put_sched(rx->hdr.link, rx);
		}
	}
#endif /* CONFIG_BT_CTLR_CONN_RSSI_EVENT */

	/* check if latency needs update */
	lazy = 0U;
	if ((force) || (latency_event != lll->latency_event)) {
		lazy = lll->latency_event + 1U;
	}

#if defined(CONFIG_BT_CTLR_SLOT_RESERVATION_UPDATE)
#if defined(CONFIG_BT_CTLR_DATA_LENGTH) || defined(CONFIG_BT_CTLR_PHY)
	if (lll->evt_len_upd) {
		uint32_t ready_delay, rx_time, tx_time, ticks_slot, slot_us;

		lll->evt_len_upd = 0;

#if defined(CONFIG_BT_CTLR_PHY)
		ready_delay = (lll->role) ?
			lll_radio_rx_ready_delay_get(lll->phy_rx, PHY_FLAGS_S8) :
			lll_radio_tx_ready_delay_get(lll->phy_tx, lll->phy_flags);

#if defined(CONFIG_BT_CTLR_PERIPHERAL_RESERVE_MAX)
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		tx_time = lll->dle.eff.max_tx_time;
		rx_time = lll->dle.eff.max_rx_time;

#else /* CONFIG_BT_CTLR_DATA_LENGTH */
		tx_time = MAX(PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, 0),
			      PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, lll->phy_tx));
		rx_time = MAX(PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, 0),
			      PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, lll->phy_rx));
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#else /* !CONFIG_BT_CTLR_PERIPHERAL_RESERVE_MAX */
		tx_time = PDU_MAX_US(0U, 0U, lll->phy_tx);
		rx_time = PDU_MAX_US(0U, 0U, lll->phy_rx);
#endif /* !CONFIG_BT_CTLR_PERIPHERAL_RESERVE_MAX */

#else /* CONFIG_BT_CTLR_PHY */
		ready_delay = (lll->role) ?
			lll_radio_rx_ready_delay_get(0, 0) :
			lll_radio_tx_ready_delay_get(0, 0);
#if defined(CONFIG_BT_CTLR_PERIPHERAL_RESERVE_MAX)
		tx_time = PDU_DC_MAX_US(lll->dle.eff.max_tx_octets, 0);
		rx_time = PDU_DC_MAX_US(lll->dle.eff.max_rx_octets, 0);

#else /* !CONFIG_BT_CTLR_PERIPHERAL_RESERVE_MAX */
		tx_time = PDU_MAX_US(0U, 0U, PHY_1M);
		rx_time = PDU_MAX_US(0U, 0U, PHY_1M);
#endif /* !CONFIG_BT_CTLR_PERIPHERAL_RESERVE_MAX */
#endif /* CONFIG_BT_CTLR_PHY */

		/* Calculate event time reservation */
		slot_us = tx_time + rx_time;
		slot_us += lll->tifs_rx_us + (EVENT_CLOCK_JITTER_US << 1);
		slot_us += ready_delay;

		if (IS_ENABLED(CONFIG_BT_CTLR_EVENT_OVERHEAD_RESERVE_MAX) ||
		    !conn->lll.role) {
			slot_us += EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;
		}

		ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(slot_us);
		if (ticks_slot > conn->ull.ticks_slot) {
			ticks_slot_plus = ticks_slot - conn->ull.ticks_slot;
		} else {
			ticks_slot_minus = conn->ull.ticks_slot - ticks_slot;
		}
		conn->ull.ticks_slot = ticks_slot;
	}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH || CONFIG_BT_CTLR_PHY */
#else /* CONFIG_BT_CTLR_SLOT_RESERVATION_UPDATE */
	ticks_slot_plus = 0;
	ticks_slot_minus = 0;
#endif /* CONFIG_BT_CTLR_SLOT_RESERVATION_UPDATE */

	/* update conn ticker */
	if (ticks_drift_plus || ticks_drift_minus ||
	    ticks_slot_plus || ticks_slot_minus ||
	    lazy || force) {
		uint8_t ticker_id = TICKER_ID_CONN_BASE + lll->handle;
		struct ll_conn *conn_ll = lll->hdr.parent;
		uint32_t ticker_status;

		/* Call to ticker_update can fail under the race
		 * condition where in the peripheral role is being stopped but
		 * at the same time it is preempted by peripheral event that
		 * gets into close state. Accept failure when peripheral role
		 * is being stopped.
		 */
		ticker_status = ticker_update(TICKER_INSTANCE_ID_CTLR,
					      TICKER_USER_ID_ULL_HIGH,
					      ticker_id,
					      ticks_drift_plus, ticks_drift_minus,
					      ticks_slot_plus, ticks_slot_minus,
					      lazy, force,
					      ticker_update_conn_op_cb,
					      conn_ll);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY) ||
			  ((void *)conn_ll == ull_disable_mark_get()));
	}
}

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
void ull_conn_lll_tx_demux_sched(struct lll_conn *lll)
{
	static memq_link_t link;
	static struct mayfly mfy = {0U, 0U, &link, NULL, tx_demux};

	mfy.param = HDR_LLL2ULL(lll);

	mayfly_enqueue(TICKER_USER_ID_LLL, TICKER_USER_ID_ULL_HIGH, 1U, &mfy);
}
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

void ull_conn_tx_demux(uint8_t count)
{
	do {
		struct lll_tx *lll_tx;
		struct ll_conn *conn;

		lll_tx = MFIFO_DEQUEUE_GET(conn_tx);
		if (!lll_tx) {
			break;
		}

		conn = ll_connected_get(lll_tx->handle);
		if (conn) {
			struct node_tx *tx = lll_tx->node;

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
			if (empty_data_start_release(conn, tx)) {
				goto ull_conn_tx_demux_release;
			}
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

			ull_tx_q_enqueue_data(&conn->tx_q, tx);
		} else {
			struct node_tx *tx = lll_tx->node;
			struct pdu_data *p = (void *)tx->pdu;

			p->ll_id = PDU_DATA_LLID_RESV;
			ll_tx_ack_put(LLL_HANDLE_INVALID, tx);
		}

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
ull_conn_tx_demux_release:
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

		MFIFO_DEQUEUE(conn_tx);
	} while (--count);
}

void ull_conn_tx_lll_enqueue(struct ll_conn *conn, uint8_t count)
{
	while (count--) {
		struct node_tx *tx;
		memq_link_t *link;

		tx = tx_ull_dequeue(conn, NULL);
		if (!tx) {
			/* No more tx nodes available */
			break;
		}

		link = mem_acquire(&mem_link_tx.free);
		LL_ASSERT(link);

		/* Enqueue towards LLL */
		memq_enqueue(link, tx, &conn->lll.memq_tx.tail);
	}
}

void ull_conn_link_tx_release(void *link)
{
	mem_release(link, &mem_link_tx.free);
}

uint8_t ull_conn_ack_last_idx_get(void)
{
	return mfifo_fifo_conn_ack.l;
}

memq_link_t *ull_conn_ack_peek(uint8_t *ack_last, uint16_t *handle,
			       struct node_tx **tx)
{
	struct lll_tx *lll_tx;

	lll_tx = MFIFO_DEQUEUE_GET(conn_ack);
	if (!lll_tx) {
		return NULL;
	}

	*ack_last = mfifo_fifo_conn_ack.l;

	*handle = lll_tx->handle;
	*tx = lll_tx->node;

	return (*tx)->link;
}

memq_link_t *ull_conn_ack_by_last_peek(uint8_t last, uint16_t *handle,
				       struct node_tx **tx)
{
	struct lll_tx *lll_tx;

	lll_tx = mfifo_dequeue_get(mfifo_fifo_conn_ack.m, mfifo_conn_ack.s,
				   mfifo_fifo_conn_ack.f, last);
	if (!lll_tx) {
		return NULL;
	}

	*handle = lll_tx->handle;
	*tx = lll_tx->node;

	return (*tx)->link;
}

void *ull_conn_ack_dequeue(void)
{
	return MFIFO_DEQUEUE(conn_ack);
}

void ull_conn_lll_ack_enqueue(uint16_t handle, struct node_tx *tx)
{
	struct lll_tx *lll_tx;
	uint8_t idx;

	idx = MFIFO_ENQUEUE_GET(conn_ack, (void **)&lll_tx);
	LL_ASSERT(lll_tx);

	lll_tx->handle = handle;
	lll_tx->node = tx;

	MFIFO_ENQUEUE(conn_ack, idx);
}

void ull_conn_tx_ack(uint16_t handle, memq_link_t *link, struct node_tx *tx)
{
	struct pdu_data *pdu_tx;

	pdu_tx = (void *)tx->pdu;
	LL_ASSERT(pdu_tx->len);

	if (pdu_tx->ll_id == PDU_DATA_LLID_CTRL) {
		if (handle != LLL_HANDLE_INVALID) {
			struct ll_conn *conn = ll_conn_get(handle);

			ull_cp_tx_ack(conn, tx);
		}

		/* release ctrl mem if points to itself */
		if (link->next == (void *)tx) {
			LL_ASSERT(link->next);

			struct ll_conn *conn = ll_connected_get(handle);

			ull_cp_release_tx(conn, tx);
			return;
		} else if (!tx) {
			/* Tx Node re-used to enqueue new ctrl PDU */
			return;
		}
		LL_ASSERT(!link->next);
	} else if (handle == LLL_HANDLE_INVALID) {
		pdu_tx->ll_id = PDU_DATA_LLID_RESV;
	} else {
		LL_ASSERT(handle != LLL_HANDLE_INVALID);
	}

	ll_tx_ack_put(handle, tx);
}

uint16_t ull_conn_lll_max_tx_octets_get(struct lll_conn *lll)
{
	uint16_t max_tx_octets;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
#if defined(CONFIG_BT_CTLR_PHY)
	switch (lll->phy_tx_time) {
	default:
	case PHY_1M:
		/* 1M PHY, 1us = 1 bit, hence divide by 8.
		 * Deduct 10 bytes for preamble (1), access address (4),
		 * header (2), and CRC (3).
		 */
		max_tx_octets = (lll->dle.eff.max_tx_time >> 3) - 10;
		break;

	case PHY_2M:
		/* 2M PHY, 1us = 2 bits, hence divide by 4.
		 * Deduct 11 bytes for preamble (2), access address (4),
		 * header (2), and CRC (3).
		 */
		max_tx_octets = (lll->dle.eff.max_tx_time >> 2) - 11;
		break;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case PHY_CODED:
		if (lll->phy_flags & 0x01) {
			/* S8 Coded PHY, 8us = 1 bit, hence divide by
			 * 64.
			 * Subtract time for preamble (80), AA (256),
			 * CI (16), TERM1 (24), CRC (192) and
			 * TERM2 (24), total 592 us.
			 * Subtract 2 bytes for header.
			 */
			max_tx_octets = ((lll->dle.eff.max_tx_time - 592) >>
					  6) - 2;
		} else {
			/* S2 Coded PHY, 2us = 1 bit, hence divide by
			 * 16.
			 * Subtract time for preamble (80), AA (256),
			 * CI (16), TERM1 (24), CRC (48) and
			 * TERM2 (6), total 430 us.
			 * Subtract 2 bytes for header.
			 */
			max_tx_octets = ((lll->dle.eff.max_tx_time - 430) >>
					  4) - 2;
		}
		break;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}

#if defined(CONFIG_BT_CTLR_LE_ENC)
	if (lll->enc_tx) {
		/* deduct the MIC */
		max_tx_octets -= 4U;
	}
#endif /* CONFIG_BT_CTLR_LE_ENC */

	if (max_tx_octets > lll->dle.eff.max_tx_octets) {
		max_tx_octets = lll->dle.eff.max_tx_octets;
	}

#else /* !CONFIG_BT_CTLR_PHY */
	max_tx_octets = lll->dle.eff.max_tx_octets;
#endif /* !CONFIG_BT_CTLR_PHY */
#else /* !CONFIG_BT_CTLR_DATA_LENGTH */
	max_tx_octets = PDU_DC_PAYLOAD_SIZE_MIN;
#endif /* !CONFIG_BT_CTLR_DATA_LENGTH */
	return max_tx_octets;
}

/**
 * @brief Initialize pdu_data members that are read only in lower link layer.
 *
 * @param pdu Pointer to pdu_data object to be initialized
 */
void ull_pdu_data_init(struct pdu_data *pdu)
{
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
	pdu->cp = 0U;
	pdu->octet3.resv[0] = 0U;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_TX || CONFIG_BT_CTLR_DF_CONN_CTE_RX */
}

static int init_reset(void)
{
	/* Initialize conn pool. */
	mem_init(conn_pool, sizeof(struct ll_conn),
		 sizeof(conn_pool) / sizeof(struct ll_conn), &conn_free);

	/* Initialize tx pool. */
	mem_init(mem_conn_tx.pool, CONN_TX_BUF_SIZE, CONN_DATA_BUFFERS,
		 &mem_conn_tx.free);

	/* Initialize tx link pool. */
	mem_init(mem_link_tx.pool, sizeof(memq_link_t),
		 (CONN_DATA_BUFFERS +
		  LLCP_TX_CTRL_BUF_COUNT),
		 &mem_link_tx.free);

	/* Initialize control procedure system. */
	ull_cp_init();

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	/* Reset CPR mutex */
	cpr_active_reset();
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	/* Initialize the DLE defaults */
	default_tx_octets = PDU_DC_PAYLOAD_SIZE_MIN;
	default_tx_time = PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, PHY_1M);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	/* Initialize the PHY defaults */
	default_phy_tx = PHY_1M;
	default_phy_rx = PHY_1M;

#if defined(CONFIG_BT_CTLR_PHY_2M)
	default_phy_tx |= PHY_2M;
	default_phy_rx |= PHY_2M;
#endif /* CONFIG_BT_CTLR_PHY_2M */

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	default_phy_tx |= PHY_CODED;
	default_phy_rx |= PHY_CODED;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
	memset(&default_past_params, 0, sizeof(struct past_params));
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER */

	return 0;
}

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
static void tx_demux_sched(struct ll_conn *conn)
{
	static memq_link_t link;
	static struct mayfly mfy = {0U, 0U, &link, NULL, tx_demux};

	mfy.param = conn;

	mayfly_enqueue(TICKER_USER_ID_THREAD, TICKER_USER_ID_ULL_HIGH, 0U, &mfy);
}
#endif /* !CONFIG_BT_CTLR_LOW_LAT_ULL */

static void tx_demux(void *param)
{
	ull_conn_tx_demux(1);

	ull_conn_tx_lll_enqueue(param, 1);
}

static struct node_tx *tx_ull_dequeue(struct ll_conn *conn, struct node_tx *unused)
{
	struct node_tx *tx = NULL;

	tx = ull_tx_q_dequeue(&conn->tx_q);
	if (tx) {
		struct pdu_data *pdu_tx;

		pdu_tx = (void *)tx->pdu;
		if (pdu_tx->ll_id == PDU_DATA_LLID_CTRL) {
			/* Mark the tx node as belonging to the ctrl pool */
			tx->next = tx;
		} else {
			/* Mark the tx node as belonging to the data pool */
			tx->next = NULL;
		}
	}
	return tx;
}

static void ticker_update_conn_op_cb(uint32_t status, void *param)
{
	/* Peripheral drift compensation succeeds, or it fails in a race condition
	 * when disconnecting or connection update (race between ticker_update
	 * and ticker_stop calls).
	 */
	LL_ASSERT(status == TICKER_STATUS_SUCCESS ||
		  param == ull_update_mark_get() ||
		  param == ull_disable_mark_get());
}

static void ticker_stop_conn_op_cb(uint32_t status, void *param)
{
	void *p;

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	p = ull_update_mark(param);
	LL_ASSERT(p == param);
}

static void ticker_start_conn_op_cb(uint32_t status, void *param)
{
	void *p;

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	p = ull_update_unmark(param);
	LL_ASSERT(p == param);
}

static void conn_setup_adv_scan_disabled_cb(void *param)
{
	struct node_rx_ftr *ftr;
	struct node_rx_pdu *rx;
	struct lll_conn *lll;

	/* NOTE: LLL conn context SHALL be after lll_hdr in
	 *       struct lll_adv and struct lll_scan.
	 */
	rx = param;
	ftr = &(rx->rx_ftr);
	lll = *((struct lll_conn **)((uint8_t *)ftr->param +
				     sizeof(struct lll_hdr)));

	if (IS_ENABLED(CONFIG_BT_CTLR_JIT_SCHEDULING)) {
		struct ull_hdr *hdr;

		/* Prevent fast ADV re-scheduling from re-triggering */
		hdr = HDR_LLL2ULL(ftr->param);
		hdr->disabled_cb = NULL;
	}

	switch (lll->role) {
#if defined(CONFIG_BT_CENTRAL)
	case 0:
		ull_central_setup(rx, ftr, lll);
		break;
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
	case 1:
		ull_periph_setup(rx, ftr, lll);
		break;
#endif /* CONFIG_BT_PERIPHERAL */

	default:
		LL_ASSERT(0);
		break;
	}
}

static inline void disable(uint16_t handle)
{
	struct ll_conn *conn;
	int err;

	conn = ll_conn_get(handle);

	err = ull_ticker_stop_with_mark(TICKER_ID_CONN_BASE + handle,
					conn, &conn->lll);
	LL_ASSERT_INFO2(err == 0 || err == -EALREADY, handle, err);

	conn->lll.handle = LLL_HANDLE_INVALID;
	conn->lll.link_tx_free = NULL;
}

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) || defined(CONFIG_BT_CTLR_CENTRAL_ISO)
static void conn_cleanup_iso_cis_released_cb(struct ll_conn *conn)
{
	struct ll_conn_iso_stream *cis;

	cis = ll_conn_iso_stream_get_by_acl(conn, NULL);
	if (cis) {
		struct node_rx_pdu *rx;
		uint8_t reason;

		/* More associated CISes - stop next */
		rx = (void *)&conn->llcp_terminate.node_rx;
		reason = *(uint8_t *)rx->pdu;

		ull_conn_iso_cis_stop(cis, conn_cleanup_iso_cis_released_cb,
				      reason);
	} else {
		/* No more CISes associated with conn - finalize */
		conn_cleanup_finalize(conn);
	}
}
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO || CONFIG_BT_CTLR_CENTRAL_ISO */

static void conn_cleanup_finalize(struct ll_conn *conn)
{
	struct lll_conn *lll = &conn->lll;
	uint32_t ticker_status;

	ull_cp_state_set(conn, ULL_CP_DISCONNECTED);

	/* Update tx buffer queue handling */
#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
	ull_cp_update_tx_buffer_queue(conn);
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */
	ull_cp_release_nodes(conn);

	/* flush demux-ed Tx buffer still in ULL context */
	tx_ull_flush(conn);

	/* Stop Central or Peripheral role ticker */
	ticker_status = ticker_stop(TICKER_INSTANCE_ID_CTLR,
				    TICKER_USER_ID_ULL_HIGH,
				    TICKER_ID_CONN_BASE + lll->handle,
				    ticker_stop_op_cb, conn);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

	/* Invalidate the connection context */
	lll->handle = LLL_HANDLE_INVALID;

	/* Demux and flush Tx PDUs that remain enqueued in thread context */
	ull_conn_tx_demux(UINT8_MAX);
}

static void conn_cleanup(struct ll_conn *conn, uint8_t reason)
{
	struct node_rx_pdu *rx;

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) || defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	struct ll_conn_iso_stream *cis;
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO || CONFIG_BT_CTLR_CENTRAL_ISO */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	/* Reset CPR mutex */
	cpr_active_check_and_reset(conn);
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

	/* Only termination structure is populated here in ULL context
	 * but the actual enqueue happens in the LLL context in
	 * tx_lll_flush. The reason being to avoid passing the reason
	 * value and handle through the mayfly scheduling of the
	 * tx_lll_flush.
	 */
	rx = (void *)&conn->llcp_terminate.node_rx.rx;
	rx->hdr.handle = conn->lll.handle;
	rx->hdr.type = NODE_RX_TYPE_TERMINATE;
	*((uint8_t *)rx->pdu) = reason;

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) || defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	cis = ll_conn_iso_stream_get_by_acl(conn, NULL);
	if (cis) {
		/* Stop CIS and defer cleanup to after teardown. */
		ull_conn_iso_cis_stop(cis, conn_cleanup_iso_cis_released_cb,
				      reason);
		return;
	}
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO || CONFIG_BT_CTLR_CENTRAL_ISO */

	conn_cleanup_finalize(conn);
}

static void tx_ull_flush(struct ll_conn *conn)
{
	struct node_tx *tx;

	ull_tx_q_resume_data(&conn->tx_q);

	tx = tx_ull_dequeue(conn, NULL);
	while (tx) {
		memq_link_t *link;

		link = mem_acquire(&mem_link_tx.free);
		LL_ASSERT(link);

		/* Enqueue towards LLL */
		memq_enqueue(link, tx, &conn->lll.memq_tx.tail);

		tx = tx_ull_dequeue(conn, NULL);
	}
}

static void ticker_stop_op_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, conn_disable};
	uint32_t ret;

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	/* Check if any pending LLL events that need to be aborted */
	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
			     TICKER_USER_ID_ULL_HIGH, 0, &mfy);
	LL_ASSERT(!ret);
}

static void conn_disable(void *param)
{
	struct ll_conn *conn;
	struct ull_hdr *hdr;

	/* Check ref count to determine if any pending LLL events in pipeline */
	conn = param;
	hdr = &conn->ull;
	if (ull_ref_get(hdr)) {
		static memq_link_t link;
		static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
		uint32_t ret;

		mfy.param = &conn->lll;

		/* Setup disabled callback to be called when ref count
		 * returns to zero.
		 */
		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = mfy.param;
		hdr->disabled_cb = disabled_cb;

		/* Trigger LLL disable */
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
				     TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!ret);
	} else {
		/* No pending LLL events */
		disabled_cb(&conn->lll);
	}
}

static void disabled_cb(void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, tx_lll_flush};
	uint32_t ret;

	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
			     TICKER_USER_ID_LLL, 0, &mfy);
	LL_ASSERT(!ret);
}

static void tx_lll_flush(void *param)
{
	struct node_rx_pdu *rx;
	struct lll_conn *lll;
	struct ll_conn *conn;
	struct node_tx *tx;
	memq_link_t *link;
	uint16_t handle;

	/* Get reference to ULL context */
	lll = param;
	conn = HDR_LLL2ULL(lll);
	handle = ll_conn_handle_get(conn);

	lll_conn_flush(handle, lll);

	link = memq_dequeue(lll->memq_tx.tail, &lll->memq_tx.head,
			    (void **)&tx);
	while (link) {
		uint8_t idx;
		struct lll_tx *tx_buf;

		idx = MFIFO_ENQUEUE_GET(conn_ack, (void **)&tx_buf);
		LL_ASSERT(tx_buf);

		tx_buf->handle = LLL_HANDLE_INVALID;
		tx_buf->node = tx;

		/* TX node UPSTREAM, i.e. Tx node ack path */
		link->next = tx->next; /* Indicates ctrl pool or data pool */
		tx->next = link;

		MFIFO_ENQUEUE(conn_ack, idx);

		link = memq_dequeue(lll->memq_tx.tail, &lll->memq_tx.head,
				    (void **)&tx);
	}

	/* Get the terminate structure reserved in the connection context.
	 * The terminate reason and connection handle should already be
	 * populated before this mayfly function was scheduled.
	 */
	rx = (void *)&conn->llcp_terminate.node_rx;
	LL_ASSERT(rx->hdr.link);
	link = rx->hdr.link;
	rx->hdr.link = NULL;

	/* Enqueue the terminate towards ULL context */
	ull_rx_put_sched(link, rx);
}

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
static int empty_data_start_release(struct ll_conn *conn, struct node_tx *tx)
{
	struct pdu_data *p = (void *)tx->pdu;

	if ((p->ll_id == PDU_DATA_LLID_DATA_START) && !p->len) {
		conn->start_empty = 1U;

		ll_tx_ack_put(conn->lll.handle, tx);

		return -EINVAL;
	} else if (p->len && conn->start_empty) {
		conn->start_empty = 0U;

		if (p->ll_id == PDU_DATA_LLID_DATA_CONTINUE) {
			p->ll_id = PDU_DATA_LLID_DATA_START;
		}
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

#if defined(CONFIG_BT_CTLR_FORCE_MD_AUTO)
static uint8_t force_md_cnt_calc(struct lll_conn *lll_connection, uint32_t tx_rate)
{
	uint32_t time_incoming, time_outgoing;
	uint8_t force_md_cnt;
	uint8_t phy_flags;
	uint8_t mic_size;
	uint8_t phy;

#if defined(CONFIG_BT_CTLR_PHY)
	phy = lll_connection->phy_tx;
	phy_flags = lll_connection->phy_flags;
#else /* !CONFIG_BT_CTLR_PHY */
	phy = PHY_1M;
	phy_flags = 0U;
#endif /* !CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_LE_ENC)
	mic_size = PDU_MIC_SIZE * lll_connection->enc_tx;
#else /* !CONFIG_BT_CTLR_LE_ENC */
	mic_size = 0U;
#endif /* !CONFIG_BT_CTLR_LE_ENC */

	time_incoming = (LL_LENGTH_OCTETS_RX_MAX << 3) *
			1000000UL / tx_rate;
	time_outgoing = PDU_DC_US(LL_LENGTH_OCTETS_RX_MAX, mic_size, phy,
				  phy_flags) +
			PDU_DC_US(0U, 0U, phy, PHY_FLAGS_S8) +
			(EVENT_IFS_US << 1);

	force_md_cnt = 0U;
	if (time_incoming > time_outgoing) {
		uint32_t delta;
		uint32_t time_keep_alive;

		delta = (time_incoming << 1) - time_outgoing;
		time_keep_alive = (PDU_DC_US(0U, 0U, phy, PHY_FLAGS_S8) +
				   EVENT_IFS_US) << 1;
		force_md_cnt = (delta + (time_keep_alive - 1)) /
			       time_keep_alive;
		LOG_DBG("Time: incoming= %u, expected outgoing= %u, delta= %u, "
		       "keepalive= %u, force_md_cnt = %u.",
		       time_incoming, time_outgoing, delta, time_keep_alive,
		       force_md_cnt);
	}

	return force_md_cnt;
}
#endif /* CONFIG_BT_CTLR_FORCE_MD_AUTO */

#if defined(CONFIG_BT_CTLR_LE_ENC)
/**
 * @brief Pause the data path of a rx queue.
 */
void ull_conn_pause_rx_data(struct ll_conn *conn)
{
	conn->pause_rx_data = 1U;
}

/**
 * @brief Resume the data path of a rx queue.
 */
void ull_conn_resume_rx_data(struct ll_conn *conn)
{
	conn->pause_rx_data = 0U;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

uint16_t ull_conn_event_counter(struct ll_conn *conn)
{
	struct lll_conn *lll;
	uint16_t event_counter;

	lll = &conn->lll;

	/* Calculate current event counter. If refcount is non-zero, we have called
	 * prepare and the LLL implementation has calculated and incremented the event
	 * counter (RX path). In this case we need to subtract one from the current
	 * event counter.
	 * Otherwise we are in the TX path, and we calculate the current event counter
	 * similar to LLL by taking the expected event counter value plus accumulated
	 * latency.
	 */
	if (ull_ref_get(&conn->ull)) {
		/* We are in post-prepare (RX path). Event counter is already
		 * calculated and incremented by 1 for next event.
		 */
		event_counter = lll->event_counter - 1;
	} else {
		event_counter = lll->event_counter + lll->latency_prepare +
				conn->llcp.prep.lazy;
	}

	return event_counter;
}
static void ull_conn_update_ticker(struct ll_conn *conn,
				   uint32_t ticks_win_offset,
				   uint32_t ticks_slot_overhead,
				   uint32_t periodic_us,
				   uint32_t ticks_at_expire)
{
#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* disable ticker job, in order to chain stop and start
	 * to avoid RTC being stopped if no tickers active.
	 */
	uint32_t mayfly_was_enabled =
		mayfly_is_enabled(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW);

	mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 0U);
#endif /* CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO */

	/* start periph/central with new timings */
	uint8_t ticker_id_conn = TICKER_ID_CONN_BASE + ll_conn_handle_get(conn);
	uint32_t ticker_status = ticker_stop_abs(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
						 ticker_id_conn, ticks_at_expire,
						 ticker_stop_conn_op_cb, (void *)conn);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));
	ticker_status = ticker_start(
		TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH, ticker_id_conn, ticks_at_expire,
		ticks_win_offset, HAL_TICKER_US_TO_TICKS(periodic_us),
		HAL_TICKER_REMAINDER(periodic_us),
#if defined(CONFIG_BT_TICKER_LOW_LAT)
		TICKER_NULL_LAZY,
#else /* !CONFIG_BT_TICKER_LOW_LAT */
		TICKER_LAZY_MUST_EXPIRE_KEEP,
#endif /* CONFIG_BT_TICKER_LOW_LAT */
		(ticks_slot_overhead + conn->ull.ticks_slot),
#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_CENTRAL)
		conn->lll.role == BT_HCI_ROLE_PERIPHERAL ?
		ull_periph_ticker_cb : ull_central_ticker_cb,
#elif defined(CONFIG_BT_PERIPHERAL)
		ull_periph_ticker_cb,
#else
		ull_central_ticker_cb,
#endif /* CONFIG_BT_PERIPHERAL && CONFIG_BT_CENTRAL */
		conn, ticker_start_conn_op_cb, (void *)conn);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* enable ticker job, if disabled in this function */
	if (mayfly_was_enabled) {
		mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1U);
	}
#endif /* CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO */
}

void ull_conn_update_parameters(struct ll_conn *conn, uint8_t is_cu_proc, uint8_t win_size,
				uint32_t win_offset_us, uint16_t interval, uint16_t latency,
				uint16_t timeout, uint16_t instant)
{
	uint16_t conn_interval_unit_old;
	uint16_t conn_interval_unit_new;
	uint32_t ticks_win_offset = 0U;
	uint16_t conn_interval_old_us;
	uint16_t conn_interval_new_us;
	uint32_t ticks_slot_overhead;
	uint16_t conn_interval_old;
	uint16_t conn_interval_new;
	uint32_t conn_interval_us;
	uint32_t ticks_at_expire;
	uint16_t instant_latency;
	uint32_t ready_delay_us;
	uint16_t event_counter;
	uint32_t periodic_us;
	uint16_t latency_upd;
	struct lll_conn *lll;

	lll = &conn->lll;

	/* Calculate current event counter */
	event_counter = ull_conn_event_counter(conn);

	instant_latency = (event_counter - instant) & 0xFFFF;


	ticks_at_expire = conn->llcp.prep.ticks_at_expire;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
	/* restore to normal prepare */
	if (conn->ull.ticks_prepare_to_start & XON_BITMASK) {
		uint32_t ticks_prepare_to_start =
			MAX(conn->ull.ticks_active_to_start, conn->ull.ticks_preempt_to_start);

		conn->ull.ticks_prepare_to_start &= ~XON_BITMASK;

		ticks_at_expire -= (conn->ull.ticks_prepare_to_start - ticks_prepare_to_start);
	}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

#if defined(CONFIG_BT_CTLR_PHY)
	ready_delay_us = lll_radio_tx_ready_delay_get(lll->phy_tx,
						      lll->phy_flags);
#else
	ready_delay_us = lll_radio_tx_ready_delay_get(0U, 0U);
#endif

	/* compensate for instant_latency due to laziness */
	if (lll->interval >= BT_HCI_LE_INTERVAL_MIN) {
		conn_interval_old = instant_latency * lll->interval;
		conn_interval_unit_old = CONN_INT_UNIT_US;
	} else {
		conn_interval_old = instant_latency * (lll->interval + 1U);
		conn_interval_unit_old = CONN_LOW_LAT_INT_UNIT_US;
	}

	if (interval >= BT_HCI_LE_INTERVAL_MIN) {
		uint16_t max_tx_time;
		uint16_t max_rx_time;
		uint32_t slot_us;

		conn_interval_new = interval;
		conn_interval_unit_new = CONN_INT_UNIT_US;
		lll->tifs_tx_us = EVENT_IFS_DEFAULT_US;
		lll->tifs_rx_us = EVENT_IFS_DEFAULT_US;
		lll->tifs_hcto_us = EVENT_IFS_DEFAULT_US;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH) && \
	defined(CONFIG_BT_CTLR_SLOT_RESERVATION_UPDATE)
		max_tx_time = lll->dle.eff.max_tx_time;
		max_rx_time = lll->dle.eff.max_rx_time;

#else /* !CONFIG_BT_CTLR_DATA_LENGTH ||
       * !CONFIG_BT_CTLR_SLOT_RESERVATION_UPDATE
       */
		max_tx_time = PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, PHY_1M);
		max_rx_time = PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, PHY_1M);
#if defined(CONFIG_BT_CTLR_PHY)
		max_tx_time = MAX(max_tx_time, PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, lll->phy_tx));
		max_rx_time = MAX(max_rx_time, PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, lll->phy_rx));
#endif /* !CONFIG_BT_CTLR_PHY */
#endif /* !CONFIG_BT_CTLR_DATA_LENGTH ||
	* !CONFIG_BT_CTLR_SLOT_RESERVATION_UPDATE
	*/

		/* Calculate event time reservation */
		slot_us = max_tx_time + max_rx_time;
		slot_us += lll->tifs_rx_us + (EVENT_CLOCK_JITTER_US << 1);
		slot_us += ready_delay_us;

		if (IS_ENABLED(CONFIG_BT_CTLR_EVENT_OVERHEAD_RESERVE_MAX) ||
		    (lll->role == BT_HCI_ROLE_CENTRAL)) {
			slot_us += EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;
		}

		conn->ull.ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(slot_us);

	} else {
		conn_interval_new = interval + 1U;
		conn_interval_unit_new = CONN_LOW_LAT_INT_UNIT_US;
		lll->tifs_tx_us = CONFIG_BT_CTLR_EVENT_IFS_LOW_LAT_US;
		lll->tifs_rx_us = CONFIG_BT_CTLR_EVENT_IFS_LOW_LAT_US;
		lll->tifs_hcto_us = CONFIG_BT_CTLR_EVENT_IFS_LOW_LAT_US;
		/* Reserve only the processing overhead, on overlap the
		 * is_abort_cb mechanism will ensure to continue the event so
		 * as to not loose anchor point sync.
		 */
		conn->ull.ticks_slot =
			HAL_TICKER_US_TO_TICKS_CEIL(EVENT_OVERHEAD_START_US);
	}

	conn_interval_us = conn_interval_new * conn_interval_unit_new;
	periodic_us = conn_interval_us;

	conn_interval_old_us = conn_interval_old * conn_interval_unit_old;
	latency_upd = conn_interval_old_us / conn_interval_us;
	conn_interval_new_us = latency_upd * conn_interval_us;
	if (conn_interval_new_us > conn_interval_old_us) {
		ticks_at_expire += HAL_TICKER_US_TO_TICKS(
			conn_interval_new_us - conn_interval_old_us);
	} else {
		ticks_at_expire -= HAL_TICKER_US_TO_TICKS(
			conn_interval_old_us - conn_interval_new_us);
	}

	lll->latency_prepare += conn->llcp.prep.lazy;
	lll->latency_prepare -= (instant_latency - latency_upd);

	/* calculate the offset */
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead =
			MAX(conn->ull.ticks_active_to_start,
			    conn->ull.ticks_prepare_to_start);

	} else {
		ticks_slot_overhead = 0U;
	}

	/* calculate the window widening and interval */
	switch (lll->role) {
#if defined(CONFIG_BT_PERIPHERAL)
	case BT_HCI_ROLE_PERIPHERAL:
		lll->periph.window_widening_prepare_us -=
			lll->periph.window_widening_periodic_us * instant_latency;

		lll->periph.window_widening_periodic_us =
			DIV_ROUND_UP(((lll_clock_ppm_local_get() +
					   lll_clock_ppm_get(conn->periph.sca)) *
					  conn_interval_us), 1000000U);
		lll->periph.window_widening_max_us = (conn_interval_us >> 1U) - EVENT_IFS_US;
		lll->periph.window_size_prepare_us = win_size * CONN_INT_UNIT_US;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		conn->periph.ticks_to_offset = 0U;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

		lll->periph.window_widening_prepare_us +=
			lll->periph.window_widening_periodic_us * latency_upd;
		if (lll->periph.window_widening_prepare_us > lll->periph.window_widening_max_us) {
			lll->periph.window_widening_prepare_us = lll->periph.window_widening_max_us;
		}

		ticks_at_expire -= HAL_TICKER_US_TO_TICKS(lll->periph.window_widening_periodic_us *
							  latency_upd);
		ticks_win_offset = HAL_TICKER_US_TO_TICKS((win_offset_us / CONN_INT_UNIT_US) *
							  CONN_INT_UNIT_US);
		periodic_us -= lll->periph.window_widening_periodic_us;
		break;
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CENTRAL)
	case BT_HCI_ROLE_CENTRAL:
		ticks_win_offset = HAL_TICKER_US_TO_TICKS(win_offset_us);

		/* Workaround: Due to the missing remainder param in
		 * ticker_start function for first interval; add a
		 * tick so as to use the ceiled value.
		 */
		ticks_win_offset += 1U;
		break;
#endif /*CONFIG_BT_CENTRAL */
	default:
		LL_ASSERT(0);
		break;
	}

	lll->interval = interval;
	lll->latency = latency;

	conn->supervision_timeout = timeout;
	ull_cp_prt_reload_set(conn, conn_interval_us);

#if defined(CONFIG_BT_CTLR_LE_PING)
	/* APTO in no. of connection events */
	conn->apto_reload = RADIO_CONN_EVENTS((30U * 1000U * 1000U), conn_interval_us);
	/* Dispatch LE Ping PDU 6 connection events (that peer would
	 * listen to) before 30s timeout
	 * TODO: "peer listens to" is greater than 30s due to latency
	 */
	conn->appto_reload = (conn->apto_reload > (lll->latency + 6U)) ?
					   (conn->apto_reload - (lll->latency + 6U)) :
					   conn->apto_reload;
#endif /* CONFIG_BT_CTLR_LE_PING */

	if (is_cu_proc) {
		conn->supervision_expire = 0U;
	}

	/* Update ACL ticker */
	ull_conn_update_ticker(conn, ticks_win_offset, ticks_slot_overhead, periodic_us,
			       ticks_at_expire);
	/* Signal that the prepare needs to be canceled */
	conn->cancel_prepare = 1U;
}

#if defined(CONFIG_BT_PERIPHERAL)
void ull_conn_update_peer_sca(struct ll_conn *conn)
{
	struct lll_conn *lll;

	uint32_t conn_interval_us;
	uint32_t periodic_us;

	lll = &conn->lll;

	/* calculate the window widening and interval */
	if (lll->interval >= BT_HCI_LE_INTERVAL_MIN) {
		conn_interval_us = lll->interval *
				   CONN_INT_UNIT_US;
	} else {
		conn_interval_us = (lll->interval + 1U) *
				   CONN_LOW_LAT_INT_UNIT_US;
	}
	periodic_us = conn_interval_us;

	lll->periph.window_widening_periodic_us =
		DIV_ROUND_UP(((lll_clock_ppm_local_get() +
				   lll_clock_ppm_get(conn->periph.sca)) *
				  conn_interval_us), 1000000U);

	periodic_us -= lll->periph.window_widening_periodic_us;

	/* Update ACL ticker */
	ull_conn_update_ticker(conn, HAL_TICKER_US_TO_TICKS(periodic_us), 0, periodic_us,
				   conn->llcp.prep.ticks_at_expire);

}
#endif /* CONFIG_BT_PERIPHERAL */

void ull_conn_chan_map_set(struct ll_conn *conn, const uint8_t chm[5])
{
	struct lll_conn *lll = &conn->lll;

	memcpy(lll->data_chan_map, chm, sizeof(lll->data_chan_map));
	lll->data_chan_count = util_ones_count_get(lll->data_chan_map, sizeof(lll->data_chan_map));
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static inline void dle_max_time_get(struct ll_conn *conn, uint16_t *max_rx_time,
				    uint16_t *max_tx_time)
{
	uint8_t phy_select = PHY_1M;
	uint16_t rx_time = 0U;
	uint16_t tx_time = 0U;

#if defined(CONFIG_BT_CTLR_PHY)
	if (conn->llcp.fex.valid && feature_phy_coded(conn)) {
		/* If coded PHY is supported on the connection
		 * this will define the max times
		 */
		phy_select = PHY_CODED;
		/* If not, max times should be defined by 1M timing */
	}
#endif

	rx_time = PDU_DC_MAX_US(LL_LENGTH_OCTETS_RX_MAX, phy_select);

#if defined(CONFIG_BT_CTLR_PHY)
	tx_time = MIN(conn->lll.dle.default_tx_time,
		      PDU_DC_MAX_US(LL_LENGTH_OCTETS_RX_MAX, phy_select));
#else /* !CONFIG_BT_CTLR_PHY */
	tx_time = PDU_DC_MAX_US(conn->lll.dle.default_tx_octets, phy_select);
#endif /* !CONFIG_BT_CTLR_PHY */

	/*
	 * see Vol. 6 Part B chapter 4.5.10
	 * minimum value for time is 328 us
	 */
	rx_time = MAX(PDU_DC_PAYLOAD_TIME_MIN, rx_time);
	tx_time = MAX(PDU_DC_PAYLOAD_TIME_MIN, tx_time);

	*max_rx_time = rx_time;
	*max_tx_time = tx_time;
}

void ull_dle_max_time_get(struct ll_conn *conn, uint16_t *max_rx_time,
				    uint16_t *max_tx_time)
{
	dle_max_time_get(conn, max_rx_time, max_tx_time);
}

/*
 * TODO: this probably can be optimised for ex. by creating a macro for the
 * ull_dle_update_eff function
 */
uint8_t ull_dle_update_eff(struct ll_conn *conn)
{
	uint8_t dle_changed = 0U;

	/* Note that we must use bitwise or and not logical or */
	dle_changed = ull_dle_update_eff_rx(conn);
	dle_changed |= ull_dle_update_eff_tx(conn);
#if defined(CONFIG_BT_CTLR_SLOT_RESERVATION_UPDATE)
	if (dle_changed) {
		conn->lll.evt_len_upd = 1U;
	}
#endif


	return dle_changed;
}

uint8_t ull_dle_update_eff_rx(struct ll_conn *conn)
{
	uint8_t dle_changed = 0U;

	const uint16_t eff_rx_octets =
		MAX(MIN(conn->lll.dle.local.max_rx_octets, conn->lll.dle.remote.max_tx_octets),
		    PDU_DC_PAYLOAD_SIZE_MIN);

#if defined(CONFIG_BT_CTLR_PHY)
	unsigned int min_eff_rx_time = (conn->lll.phy_rx == PHY_CODED) ?
			PDU_DC_PAYLOAD_TIME_MIN_CODED : PDU_DC_PAYLOAD_TIME_MIN;

	const uint16_t eff_rx_time =
		MAX(MIN(conn->lll.dle.local.max_rx_time, conn->lll.dle.remote.max_tx_time),
		    min_eff_rx_time);

	if (eff_rx_time != conn->lll.dle.eff.max_rx_time) {
		conn->lll.dle.eff.max_rx_time = eff_rx_time;
		dle_changed = 1U;
	}
#else
	conn->lll.dle.eff.max_rx_time = PDU_DC_MAX_US(eff_rx_octets, PHY_1M);
#endif

	if (eff_rx_octets != conn->lll.dle.eff.max_rx_octets) {
		conn->lll.dle.eff.max_rx_octets = eff_rx_octets;
		dle_changed = 1U;
	}
#if defined(CONFIG_BT_CTLR_SLOT_RESERVATION_UPDATE)
	/* we delay the update of event length to after the DLE procedure is finishede */
	if (dle_changed) {
		conn->lll.evt_len_upd_delayed = 1;
	}
#endif

	return dle_changed;
}

uint8_t ull_dle_update_eff_tx(struct ll_conn *conn)

{
	uint8_t dle_changed = 0U;

	const uint16_t eff_tx_octets =
		MAX(MIN(conn->lll.dle.local.max_tx_octets, conn->lll.dle.remote.max_rx_octets),
		    PDU_DC_PAYLOAD_SIZE_MIN);

#if defined(CONFIG_BT_CTLR_PHY)
	unsigned int min_eff_tx_time = (conn->lll.phy_tx == PHY_CODED) ?
			PDU_DC_PAYLOAD_TIME_MIN_CODED : PDU_DC_PAYLOAD_TIME_MIN;

	const uint16_t eff_tx_time =
		MAX(MIN(conn->lll.dle.local.max_tx_time, conn->lll.dle.remote.max_rx_time),
		    min_eff_tx_time);

	if (eff_tx_time != conn->lll.dle.eff.max_tx_time) {
		conn->lll.dle.eff.max_tx_time = eff_tx_time;
		dle_changed = 1U;
	}
#else
	conn->lll.dle.eff.max_tx_time = PDU_DC_MAX_US(eff_tx_octets, PHY_1M);
#endif

	if (eff_tx_octets != conn->lll.dle.eff.max_tx_octets) {
		conn->lll.dle.eff.max_tx_octets = eff_tx_octets;
		dle_changed = 1U;
	}

#if defined(CONFIG_BT_CTLR_SLOT_RESERVATION_UPDATE)
	if (dle_changed) {
		conn->lll.evt_len_upd = 1U;
	}
	conn->lll.evt_len_upd |= conn->lll.evt_len_upd_delayed;
	conn->lll.evt_len_upd_delayed = 0;
#endif

	return dle_changed;
}

static void ull_len_data_length_trim(uint16_t *tx_octets, uint16_t *tx_time)
{
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	uint16_t tx_time_max =
			PDU_DC_MAX_US(LL_LENGTH_OCTETS_TX_MAX, PHY_CODED);
#else /* !CONFIG_BT_CTLR_PHY_CODED */
	uint16_t tx_time_max =
			PDU_DC_MAX_US(LL_LENGTH_OCTETS_TX_MAX, PHY_1M);
#endif /* !CONFIG_BT_CTLR_PHY_CODED */

	/* trim to supported values */
	if (*tx_octets > LL_LENGTH_OCTETS_TX_MAX) {
		*tx_octets = LL_LENGTH_OCTETS_TX_MAX;
	}

	if (*tx_time > tx_time_max) {
		*tx_time = tx_time_max;
	}
}

void ull_dle_local_tx_update(struct ll_conn *conn, uint16_t tx_octets, uint16_t tx_time)
{
	/* Trim to supported values */
	ull_len_data_length_trim(&tx_octets, &tx_time);

	conn->lll.dle.default_tx_octets = tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
	conn->lll.dle.default_tx_time = tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

	dle_max_time_get(conn, &conn->lll.dle.local.max_rx_time, &conn->lll.dle.local.max_tx_time);
	conn->lll.dle.local.max_tx_octets = conn->lll.dle.default_tx_octets;
}

void ull_dle_init(struct ll_conn *conn, uint8_t phy)
{
#if defined(CONFIG_BT_CTLR_PHY)
	const uint16_t max_time_min = PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, phy);
	const uint16_t max_time_max = PDU_DC_MAX_US(LL_LENGTH_OCTETS_RX_MAX, phy);
#endif /* CONFIG_BT_CTLR_PHY */

	/* Clear DLE data set */
	memset(&conn->lll.dle, 0, sizeof(conn->lll.dle));
	/* See BT. 5.2 Spec - Vol 6, Part B, Sect 4.5.10
	 * Default to locally max supported rx/tx length/time
	 */
	ull_dle_local_tx_update(conn, default_tx_octets, default_tx_time);

	conn->lll.dle.local.max_rx_octets = LL_LENGTH_OCTETS_RX_MAX;
#if defined(CONFIG_BT_CTLR_PHY)
	conn->lll.dle.local.max_rx_time = max_time_max;
#endif /* CONFIG_BT_CTLR_PHY */

	/* Default to minimum rx/tx data length/time */
	conn->lll.dle.remote.max_tx_octets = PDU_DC_PAYLOAD_SIZE_MIN;
	conn->lll.dle.remote.max_rx_octets = PDU_DC_PAYLOAD_SIZE_MIN;

#if defined(CONFIG_BT_CTLR_PHY)
	conn->lll.dle.remote.max_tx_time = max_time_min;
	conn->lll.dle.remote.max_rx_time = max_time_min;
#endif /* CONFIG_BT_CTLR_PHY */

	/*
	 * ref. Bluetooth Core Specification version 5.3, Vol. 6,
	 * Part B, section 4.5.10 we can call ull_dle_update_eff
	 * for initialisation
	 */
	(void)ull_dle_update_eff(conn);

	/* Check whether the controller should perform a data length update after
	 * connection is established
	 */
#if defined(CONFIG_BT_CTLR_PHY)
	if ((conn->lll.dle.local.max_rx_time != max_time_min ||
	     conn->lll.dle.local.max_tx_time != max_time_min)) {
		conn->lll.dle.update = 1;
	} else
#endif
	{
		if (conn->lll.dle.local.max_tx_octets != PDU_DC_PAYLOAD_SIZE_MIN ||
		    conn->lll.dle.local.max_rx_octets != PDU_DC_PAYLOAD_SIZE_MIN) {
			conn->lll.dle.update = 1;
		}
	}
}

void ull_conn_default_tx_octets_set(uint16_t tx_octets)
{
	default_tx_octets = tx_octets;
}

void ull_conn_default_tx_time_set(uint16_t tx_time)
{
	default_tx_time = tx_time;
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER)
static bool ticker_op_id_match_func(uint8_t ticker_id, uint32_t ticks_slot,
				    uint32_t ticks_to_expire, void *op_context)
{
	ARG_UNUSED(ticks_slot);
	ARG_UNUSED(ticks_to_expire);

	uint8_t match_id = *(uint8_t *)op_context;

	return ticker_id == match_id;
}

static void ticker_get_offset_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}

static uint32_t get_ticker_offset(uint8_t ticker_id, uint16_t *lazy)
{
	uint32_t volatile ret_cb;
	uint32_t ticks_to_expire;
	uint32_t ticks_current;
	uint32_t sync_remainder_us;
	uint32_t remainder;
	uint32_t start_us;
	uint32_t ret;
	uint8_t id;

	id = TICKER_NULL;
	ticks_to_expire = 0U;
	ticks_current = 0U;

	ret_cb = TICKER_STATUS_BUSY;

	ret = ticker_next_slot_get_ext(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_LOW,
				       &id, &ticks_current, &ticks_to_expire, &remainder,
				       lazy, ticker_op_id_match_func, &ticker_id,
				       ticker_get_offset_op_cb, (void *)&ret_cb);

	if (ret == TICKER_STATUS_BUSY) {
		while (ret_cb == TICKER_STATUS_BUSY) {
			ticker_job_sched(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_LOW);
		}
	}

	LL_ASSERT(ret_cb == TICKER_STATUS_SUCCESS);

	/* Reduced a tick for negative remainder and return positive remainder
	 * value.
	 */
	hal_ticker_remove_jitter(&ticks_to_expire, &remainder);
	sync_remainder_us = remainder;

	/* Add a tick for negative remainder and return positive remainder
	 * value.
	 */
	hal_ticker_add_jitter(&ticks_to_expire, &remainder);
	start_us = remainder;

	return ull_get_wrapped_time_us(HAL_TICKER_TICKS_TO_US(ticks_to_expire),
					(sync_remainder_us - start_us));
}

static void mfy_past_sender_offset_get(void *param)
{
	uint16_t last_pa_event_counter;
	uint32_t ticker_offset_us;
	uint16_t pa_event_counter;
	uint8_t adv_sync_handle;
	uint16_t sync_handle;
	struct ll_conn *conn;
	uint16_t lazy;

	conn = param;

	/* Get handle to look for */
	ull_lp_past_offset_get_calc_params(conn, &adv_sync_handle, &sync_handle);

	if (adv_sync_handle == BT_HCI_ADV_HANDLE_INVALID &&
	    sync_handle == BT_HCI_SYNC_HANDLE_INVALID) {
		/* Procedure must have been aborted, do nothing */
		return;
	}

	if (adv_sync_handle != BT_HCI_ADV_HANDLE_INVALID) {
		const struct ll_adv_sync_set *adv_sync = ull_adv_sync_get(adv_sync_handle);

		LL_ASSERT(adv_sync);

		ticker_offset_us = get_ticker_offset(TICKER_ID_ADV_SYNC_BASE + adv_sync_handle,
						     &lazy);

		pa_event_counter = adv_sync->lll.event_counter;
		last_pa_event_counter = pa_event_counter - 1;
	} else {
		const struct ll_sync_set *sync = ull_sync_is_enabled_get(sync_handle);
		uint32_t interval_us = sync->interval * PERIODIC_INT_UNIT_US;
		uint32_t window_widening_event_us;

		LL_ASSERT(sync);

		ticker_offset_us = get_ticker_offset(TICKER_ID_SCAN_SYNC_BASE + sync_handle,
						     &lazy);

		if (lazy && ticker_offset_us > interval_us) {

			/* Figure out how many events we have actually skipped */
			lazy = lazy - (ticker_offset_us / interval_us);

			/* Correct offset to point to next event */
			ticker_offset_us = ticker_offset_us % interval_us;
		}

		/* Calculate window widening for next event */
		window_widening_event_us = sync->lll.window_widening_event_us +
					   sync->lll.window_widening_periodic_us * (lazy + 1U);

		/* Correct for window widening */
		ticker_offset_us += window_widening_event_us;

		pa_event_counter = sync->lll.event_counter + lazy;

		last_pa_event_counter = pa_event_counter - 1 - lazy;

		/* Handle unsuccessful events */
		if (sync->timeout_expire) {
			last_pa_event_counter -= sync->timeout_reload - sync->timeout_expire;
		}
	}

	ull_lp_past_offset_calc_reply(conn, ticker_offset_us, pa_event_counter,
				      last_pa_event_counter);
}

void ull_conn_past_sender_offset_request(struct ll_conn *conn)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mfy_past_sender_offset_get};
	uint32_t ret;

	mfy.param = conn;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1,
			     &mfy);
	LL_ASSERT(!ret);
}
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER */

uint8_t ull_conn_lll_phy_active(struct ll_conn *conn, uint8_t phys)
{
#if defined(CONFIG_BT_CTLR_PHY)
	if (!(phys & (conn->lll.phy_tx | conn->lll.phy_rx))) {
#else /* !CONFIG_BT_CTLR_PHY */
	if (!(phys & 0x01)) {
#endif /* !CONFIG_BT_CTLR_PHY */
		return 0;
	}
	return 1;
}

uint8_t ull_is_lll_tx_queue_empty(struct ll_conn *conn)
{
	return (memq_peek(conn->lll.memq_tx.head, conn->lll.memq_tx.tail, NULL) == NULL);
}
