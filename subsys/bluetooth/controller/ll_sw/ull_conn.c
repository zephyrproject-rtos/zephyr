/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/entropy.h>
#include <bluetooth/bluetooth.h>
#include <sys/byteorder.h>

#include "hal/ecb.h"
#include "hal/ccm.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "lll.h"
#include "lll_tim_internal.h"
#include "lll_conn.h"
#include "ull_conn_types.h"
#include "ull_internal.h"
#include "ull_sched_internal.h"
#include "ull_conn_internal.h"
#include "ull_slave_internal.h"
#include "ull_master_internal.h"

#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_conn
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_USER_EXT)
#include "ull_vendor.h"
#endif /* CONFIG_BT_CTLR_USER_EXT */

static int init_reset(void);

#if defined(CONFIG_BT_PERIPHERAL)
static void ticker_update_latency_cancel_op_cb(u32_t ticker_status,
					       void *params);
#endif /* CONFIG_BT_PERIPHERAL */

static void ticker_update_conn_op_cb(u32_t status, void *param);

static inline void disable(u16_t handle);
static void conn_cleanup(struct ll_conn *conn, u8_t reason);
static void tx_ull_flush(struct ll_conn *conn);
static void tx_lll_flush(void *param);

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
static int empty_data_start_release(struct ll_conn *conn, struct node_tx *tx);
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

static inline void ctrl_tx_pre_ack(struct ll_conn *conn,
				   struct pdu_data *pdu_tx);
static inline void ctrl_tx_ack(struct ll_conn *conn, struct node_tx **tx,
			       struct pdu_data *pdu_tx);
static inline int ctrl_rx(memq_link_t *link, struct node_rx_pdu **rx,
			  struct pdu_data *pdu_rx, struct ll_conn *conn);

#if !defined(BT_CTLR_USER_TX_BUFFER_OVERHEAD)
#define BT_CTLR_USER_TX_BUFFER_OVERHEAD 0
#endif /* BT_CTLR_USER_TX_BUFFER_OVERHEAD */

#define CONN_TX_BUF_SIZE MROUND(offsetof(struct node_tx, pdu) + \
				offsetof(struct pdu_data, lldata) + \
				(CONFIG_BT_CTLR_TX_BUFFER_SIZE + \
				BT_CTLR_USER_TX_BUFFER_OVERHEAD))

/**
 * One connection may take up to 4 TX buffers for procedures
 * simultaneously, for example 2 for encryption, 1 for termination,
 * and 1 one that is in flight and has not been returned to the pool
 */
#define CONN_TX_CTRL_BUFFERS (4 * CONFIG_BT_CTLR_LLCP_CONN)
#define CONN_TX_CTRL_BUF_SIZE MROUND(offsetof(struct node_tx, pdu) + \
				     offsetof(struct pdu_data, llctrl) + \
				     sizeof(struct pdu_data_llctrl))

static MFIFO_DEFINE(conn_tx, sizeof(struct lll_tx), CONFIG_BT_CTLR_TX_BUFFERS);
static MFIFO_DEFINE(conn_ack, sizeof(struct lll_tx),
		    (CONFIG_BT_CTLR_TX_BUFFERS + CONN_TX_CTRL_BUFFERS));


static struct {
	void *free;
	u8_t pool[CONN_TX_BUF_SIZE * CONFIG_BT_CTLR_TX_BUFFERS];
} mem_conn_tx;

static struct {
	void *free;
	u8_t pool[CONN_TX_CTRL_BUF_SIZE * CONN_TX_CTRL_BUFFERS];
} mem_conn_tx_ctrl;

static struct {
	void *free;
	u8_t pool[sizeof(memq_link_t) *
		  (CONFIG_BT_CTLR_TX_BUFFERS + CONN_TX_CTRL_BUFFERS)];
} mem_link_tx;

static u8_t data_chan_map[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
static u8_t data_chan_count = 37U;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static u16_t default_tx_octets;
static u16_t default_tx_time;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static u8_t default_phy_tx;
static u8_t default_phy_rx;
#endif /* CONFIG_BT_CTLR_PHY */

static struct ll_conn conn_pool[CONFIG_BT_MAX_CONN];
static struct ll_conn *conn_upd_curr;
static void *conn_free;

static struct device *entropy;

struct ll_conn *ll_conn_acquire(void)
{
	return mem_acquire(&conn_free);
}

void ll_conn_release(struct ll_conn *conn)
{
	mem_release(conn, &conn_free);
}

u16_t ll_conn_handle_get(struct ll_conn *conn)
{
	return mem_index_get(conn, conn_pool, sizeof(struct ll_conn));
}

struct ll_conn *ll_conn_get(u16_t handle)
{
	return mem_get(conn_pool, sizeof(struct ll_conn), handle);
}

struct ll_conn *ll_connected_get(u16_t handle)
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

void *ll_tx_mem_acquire(void)
{
	return mem_acquire(&mem_conn_tx.free);
}

void ll_tx_mem_release(void *tx)
{
	mem_release(tx, &mem_conn_tx.free);
}

int ll_tx_mem_enqueue(u16_t handle, void *tx)
{
	struct lll_tx *lll_tx;
	struct ll_conn *conn;
	u8_t idx;

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

#if defined(CONFIG_BT_PERIPHERAL)
	/* break slave latency */
	if (conn->lll.role && conn->lll.latency_event &&
	    !conn->slave.latency_cancel) {
		u32_t ticker_status;

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

u8_t ll_conn_update(u16_t handle, u8_t cmd, u8_t status, u16_t interval_min,
		    u16_t interval_max, u16_t latency, u16_t timeout)
{
	// TODO(LLCP))
	return 0;
}

u8_t ll_chm_get(u16_t handle, u8_t *chm)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Iterate until we are sure the ISR did not modify the value while
	 * we were reading it from memory.
	 */
	do {
		conn->chm_updated = 0U;
		memcpy(chm, conn->lll.data_chan_map,
		       sizeof(conn->lll.data_chan_map));
	} while (conn->chm_updated);

	return 0;
}

u8_t ll_terminate_ind_send(u16_t handle, u8_t reason)
{
	// TODO(LLCP))
	return 0;
}

u8_t ll_feature_req_send(u16_t handle)
{
	// TODO(LLCP))
	return 0;
}

u8_t ll_version_ind_send(u16_t handle)
{
	// TODO(LLCP))
	return 0;
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
u32_t ll_length_req_send(u16_t handle, u16_t tx_octets, u16_t tx_time)
{

	// TODO(LLCP))
	return 0;
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
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
u8_t ll_phy_get(u16_t handle, u8_t *tx, u8_t *rx)
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

u8_t ll_phy_default_set(u8_t tx, u8_t rx)
{
	/* TODO: validate against supported phy */

	default_phy_tx = tx;
	default_phy_rx = rx;

	return 0;
}

u8_t ll_phy_req_send(u16_t handle, u8_t tx, u8_t flags, u8_t rx)
{
	// TODO(LLCP))
	return 0;
}
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
u8_t ll_rssi_get(u16_t handle, u8_t *rssi)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	*rssi = conn->lll.rssi_latest;

	return 0;
}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_LE_PING)
u8_t ll_apto_get(u16_t handle, u16_t *apto)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	*apto = conn->apto_reload * conn->lll.interval * 125U / 1000;

	return 0;
}

u8_t ll_apto_set(u16_t handle, u16_t apto)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	conn->apto_reload = RADIO_CONN_EVENTS(apto * 10U * 1000U,
					      conn->lll.interval * 1250);

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_PING */

int ull_conn_init(void)
{
	int err;

	entropy = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
	if (!entropy) {
		return -ENODEV;
	}

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_conn_reset(void)
{
	u16_t handle;
	int err;

	for (handle = 0U; handle < CONFIG_BT_MAX_CONN; handle++) {
		disable(handle);
	}

	/* initialise connection channel map */
	data_chan_map[0] = 0xFF;
	data_chan_map[1] = 0xFF;
	data_chan_map[2] = 0xFF;
	data_chan_map[3] = 0xFF;
	data_chan_map[4] = 0x1F;
	data_chan_count = 37U;

	/* Re-initialize the Tx mfifo */
	MFIFO_INIT(conn_tx);

	/* Re-initialize the Tx Ack mfifo */
	MFIFO_INIT(conn_ack);

	/* Reset the current conn update conn context pointer */
	conn_upd_curr = NULL;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

u8_t ull_conn_chan_map_cpy(u8_t *chan_map)
{
	memcpy(chan_map, data_chan_map, sizeof(data_chan_map));

	return data_chan_count;
}

void ull_conn_chan_map_set(u8_t *chan_map)
{
	memcpy(data_chan_map, chan_map, sizeof(data_chan_map));
	data_chan_count = util_ones_count_get(data_chan_map,
					      sizeof(data_chan_map));
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
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

#if defined(CONFIG_BT_CTLR_PHY)
u8_t ull_conn_default_phy_tx_get(void)
{
	return default_phy_tx;
}

u8_t ull_conn_default_phy_rx_get(void)
{
	return default_phy_rx;
}
#endif /* CONFIG_BT_CTLR_PHY */

void ull_conn_setup(memq_link_t *link, struct node_rx_hdr *rx)
{
	struct node_rx_ftr *ftr;
	struct lll_conn *lll;

	ftr = &(rx->rx_ftr);

	lll = *((struct lll_conn **)((u8_t *)ftr->param +
				     sizeof(struct lll_hdr)));
	switch (lll->role) {
#if defined(CONFIG_BT_CENTRAL)
	case 0:
		ull_master_setup(link, rx, ftr, lll);
		break;
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
	case 1:
		ull_slave_setup(link, rx, ftr, lll);
		break;
#endif /* CONFIG_BT_PERIPHERAL */

	default:
		LL_ASSERT(0);
		break;
	}
}

int ull_conn_rx(memq_link_t *link, struct node_rx_pdu **rx)
{
	struct pdu_data *pdu_rx;
	struct ll_conn *conn;

	conn = ll_connected_get((*rx)->hdr.handle);
	if (!conn) {
		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		return 0;
	}

	pdu_rx = (void *)(*rx)->pdu;

	switch (pdu_rx->ll_id) {
	case PDU_DATA_LLID_CTRL:
	{
		int nack;

		nack = ctrl_rx(link, rx, pdu_rx, conn);
		return nack;
	}

	case PDU_DATA_LLID_DATA_CONTINUE:
	case PDU_DATA_LLID_DATA_START:
		// TODO(LLCP))
		break;

	case PDU_DATA_LLID_RESV:
	default:
		// TODO(LLCP))

		/* Invalid LL id, drop it. */

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		break;
	}


	return 0;
}

int ull_conn_llcp(struct ll_conn *conn, u32_t ticks_at_expire, u16_t lazy)
{
  // TODO(LLCP))
  return 0;
}

void ull_conn_done(struct node_rx_event_done *done)
{
	struct lll_conn *lll = (void *)HDR_ULL2LLL(done->param);
	struct ll_conn *conn = (void *)HDR_LLL2EVT(lll);
	u32_t ticks_drift_minus;
	u32_t ticks_drift_plus;
	u16_t latency_event;
	u16_t elapsed_event;
	u8_t reason_peer;
	u16_t lazy;
	u8_t force;

	/* Skip if connection terminated by local host */
	if (lll->handle == 0xFFFF) {
		return;
	}

#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* Check authenticated payload expiry or MIC failure */
	switch (done->extra.mic_state) {
	case LLL_CONN_MIC_NONE:
#if defined(CONFIG_BT_CTLR_LE_PING)
		if (lll->enc_rx || 1 /* TODO(LLCP) */) {
			u16_t appto_reload_new;

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
		// TODO(LLCP))
		break;
	}
#endif /* CONFIG_BT_CTLR_LE_ENC */

	/* Master transmitted ack for the received terminate ind or
	 * Slave received terminate ind or MIC failure
	 */
	reason_peer = 0; /* TODO(LLCP) */
	if (reason_peer && (
#if defined(CONFIG_BT_PERIPHERAL)
			    lll->role ||
#else /* CONFIG_BT_PERIPHERAL */
			    0 ||
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CENTRAL)
			    conn->master.terminate_ack ||
			    (reason_peer == BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL)
#else /* CONFIG_BT_CENTRAL */
			    1
#endif /* CONFIG_BT_CENTRAL */
			    )) {
		conn_cleanup(conn, reason_peer);

		return;
	}

	/* Events elapsed used in timeout checks below */
	latency_event = lll->latency_event;
	elapsed_event = latency_event + 1;

	/* Slave drift compensation calc and new latency or
	 * master terminate acked
	 */
	ticks_drift_plus = 0U;
	ticks_drift_minus = 0U;
	if (done->extra.trx_cnt) {
		if (0) {
#if defined(CONFIG_BT_PERIPHERAL)
		} else if (lll->role) {
			ull_slave_done(done, &ticks_drift_plus,
				       &ticks_drift_minus);

			if (conn->tx_head || memq_peek(lll->memq_tx.head,
						       lll->memq_tx.tail,
						       NULL)) {
				lll->latency_event = 0;
			} else if (lll->slave.latency_enabled) {
				lll->latency_event = lll->latency;
			}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
		} else if (reason_peer) {
			conn->master.terminate_ack = 1;
#endif /* CONFIG_BT_CENTRAL */

		}

		/* Reset connection failed to establish countdown */
		conn->connect_expire = 0U;
	}

	/* Reset supervision countdown */
	if (done->extra.crc_valid) {
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
			conn->supervision_expire = conn->supervision_reload;
		}
	}

	/* check supervision timeout */
	force = 0U;
	if (conn->supervision_expire) {
		if (conn->supervision_expire > elapsed_event) {
			conn->supervision_expire -= elapsed_event;

			/* break latency */
			lll->latency_event = 0;

			/* Force both master and slave when close to
			 * supervision timeout.
			 */
			if (conn->supervision_expire <= 6U) {
				force = 1U;
			}
#if defined(CONFIG_BT_PERIPHERAL)
			/* use randomness to force slave role when anchor
			 * points are being missed.
			 */
			else if (lll->role) {
				if (latency_event) {
					force = 1U;
				} else {
					force = conn->slave.force & 0x01;

					/* rotate force bits */
					conn->slave.force >>= 1;
					if (force) {
						conn->slave.force |= BIT(31);
					}
				}
			}
#endif /* CONFIG_BT_PERIPHERAL */
		} else {
			conn_cleanup(conn, BT_HCI_ERR_CONN_TIMEOUT);

			return;
		}
	}

	/* check procedure timeout */
	if (conn->procedure_expire != 0U) {
		if (conn->procedure_expire > elapsed_event) {
			conn->procedure_expire -= elapsed_event;
		} else {
			conn_cleanup(conn, BT_HCI_ERR_LL_RESP_TIMEOUT);

			return;
		}
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
				ll_rx_put(rx->link, rx);
				ll_rx_sched();
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

			if ((conn->procedure_expire == 0U) &&
			    0 /* TODO(LLCP) */) {
			    // TODO(LLCP))
			}
		}
	}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_CONN_RSSI_EVENT)
	/* generate RSSI event */
	if (lll->rssi_sample_count == 0) {
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
			ll_rx_put(rx->hdr.link, rx);
			ll_rx_sched();
		}
	}
#endif /* CONFIG_BT_CTLR_CONN_RSSI_EVENT */

	/* break latency based on ctrl procedure pending */
	if (1 /* TODO(LLCP) */) {
		lll->latency_event = 0;
	}

	/* check if latency needs update */
	lazy = 0U;
	if ((force) || (latency_event != lll->latency_event)) {
		lazy = lll->latency_event + 1;
	}

	/* update conn ticker */
	if ((ticks_drift_plus != 0U) || (ticks_drift_minus != 0U) ||
	    (lazy != 0U) || (force != 0U)) {
		u8_t ticker_id = TICKER_ID_CONN_BASE + lll->handle;
		struct ll_conn *conn = lll->hdr.parent;
		u32_t ticker_status;

		/* Call to ticker_update can fail under the race
		 * condition where in the Slave role is being stopped but
		 * at the same time it is preempted by Slave event that
		 * gets into close state. Accept failure when Slave role
		 * is being stopped.
		 */
		ticker_status = ticker_update(TICKER_INSTANCE_ID_CTLR,
					      TICKER_USER_ID_ULL_HIGH,
					      ticker_id,
					      ticks_drift_plus,
					      ticks_drift_minus, 0, 0,
					      lazy, force,
					      ticker_update_conn_op_cb,
					      conn);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY) ||
			  ((void *)conn == ull_disable_mark_get()));
	}
}

void ull_conn_tx_demux(u8_t count)
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

			tx->next = NULL;
			if (!conn->tx_data) {
				conn->tx_data = tx;
				if (!conn->tx_head) {
					conn->tx_head = tx;
					conn->tx_data_last = NULL;
				}
			}

			if (conn->tx_data_last) {
				conn->tx_data_last->next = tx;
			}

			conn->tx_data_last = tx;
		} else {
			struct node_tx *tx = lll_tx->node;
			struct pdu_data *p = (void *)tx->pdu;

			p->ll_id = PDU_DATA_LLID_RESV;
			ll_tx_ack_put(0xFFFF, tx);
		}

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
ull_conn_tx_demux_release:
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

		MFIFO_DEQUEUE(conn_tx);
	} while (--count);
}

static struct node_tx *tx_ull_dequeue(struct ll_conn *conn,
					  struct node_tx *tx)
{
	if (!conn->tx_ctrl && (conn->tx_head != conn->tx_data)) {
		struct pdu_data *pdu_data_tx;

		pdu_data_tx = (void *)conn->tx_head->pdu;
		if ((pdu_data_tx->ll_id != PDU_DATA_LLID_CTRL) ||
		    ((pdu_data_tx->llctrl.opcode !=
		      PDU_DATA_LLCTRL_TYPE_ENC_REQ) &&
		     (pdu_data_tx->llctrl.opcode !=
		      PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ))) {
			conn->tx_ctrl = conn->tx_ctrl_last = conn->tx_head;
		}
	}

	if (conn->tx_head == conn->tx_ctrl) {
		conn->tx_head = conn->tx_head->next;
		if (conn->tx_ctrl == conn->tx_ctrl_last) {
			conn->tx_ctrl = NULL;
			conn->tx_ctrl_last = NULL;
		} else {
			conn->tx_ctrl = conn->tx_head;
		}

		/* point to self to indicate a control PDU mem alloc */
		tx->next = tx;
	} else {
		if (conn->tx_head == conn->tx_data) {
			conn->tx_data = conn->tx_data->next;
		}
		conn->tx_head = conn->tx_head->next;

		/* point to NULL to indicate a Data PDU mem alloc */
		tx->next = NULL;
	}

	return tx;
}

void ull_conn_tx_lll_enqueue(struct ll_conn *conn, u8_t count)
{
	bool pause_tx = false;

	while (conn->tx_head &&
	       ((0 /* TODO(LLCP) */) ||
		(!pause_tx && (conn->tx_head == conn->tx_ctrl))) && count--) {
		struct pdu_data *pdu_tx;
		struct node_tx *tx;
		memq_link_t *link;

		tx = tx_ull_dequeue(conn, conn->tx_head);

		pdu_tx = (void *)tx->pdu;
		if (pdu_tx->ll_id == PDU_DATA_LLID_CTRL) {
			ctrl_tx_pre_ack(conn, pdu_tx);
		}

		link = mem_acquire(&mem_link_tx.free);
		LL_ASSERT(link);

		memq_enqueue(link, tx, &conn->lll.memq_tx.tail);
	}
}

void ull_conn_link_tx_release(void *link)
{
	mem_release(link, &mem_link_tx.free);
}

u8_t ull_conn_ack_last_idx_get(void)
{
	return mfifo_conn_ack.l;
}

memq_link_t *ull_conn_ack_peek(u8_t *ack_last, u16_t *handle,
			       struct node_tx **tx)
{
	struct lll_tx *lll_tx;

	lll_tx = MFIFO_DEQUEUE_GET(conn_ack);
	if (!lll_tx) {
		return NULL;
	}

	*ack_last = mfifo_conn_ack.l;

	*handle = lll_tx->handle;
	*tx = lll_tx->node;

	return (*tx)->link;
}

memq_link_t *ull_conn_ack_by_last_peek(u8_t last, u16_t *handle,
				       struct node_tx **tx)
{
	struct lll_tx *lll_tx;

	lll_tx = mfifo_dequeue_get(mfifo_conn_ack.m, mfifo_conn_ack.s,
				   mfifo_conn_ack.f, last);
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

void ull_conn_lll_ack_enqueue(u16_t handle, struct node_tx *tx)
{
	struct lll_tx *lll_tx;
	u8_t idx;

	idx = MFIFO_ENQUEUE_GET(conn_ack, (void **)&lll_tx);
	LL_ASSERT(lll_tx);

	lll_tx->handle = handle;
	lll_tx->node = tx;

	MFIFO_ENQUEUE(conn_ack, idx);
}

struct ll_conn *ull_conn_tx_ack(u16_t handle, memq_link_t *link,
				struct node_tx *tx)
{
	struct ll_conn *conn = NULL;
	struct pdu_data *pdu_tx;

	pdu_tx = (void *)tx->pdu;
	LL_ASSERT(pdu_tx->len);

	if (pdu_tx->ll_id == PDU_DATA_LLID_CTRL) {
		if (handle != 0xFFFF) {
			conn = ll_conn_get(handle);

			ctrl_tx_ack(conn, &tx, pdu_tx);
		}

		/* release mem if points to itself */
		if (link->next == (void *)tx) {

			LL_ASSERT(link->next);

			mem_release(tx, &mem_conn_tx_ctrl.free);
			return conn;
		} else if (!tx) {
			return conn;
		} else {
			LL_ASSERT(!link->next);
		}
	} else if (handle != 0xFFFF) {
		conn = ll_conn_get(handle);
	} else {
		pdu_tx->ll_id = PDU_DATA_LLID_RESV;
	}

	ll_tx_ack_put(handle, tx);

	return conn;
}

u8_t ull_conn_llcp_req(void *conn)
{
	// TODO(LLCP))
	return 0;
}

u16_t ull_conn_lll_max_tx_octets_get(struct lll_conn *lll)
{
	u16_t max_tx_octets;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
#if defined(CONFIG_BT_CTLR_PHY)
	switch (lll->phy_tx_time) {
	default:
	case BIT(0):
		/* 1M PHY, 1us = 1 bit, hence divide by 8.
		 * Deduct 10 bytes for preamble (1), access address (4),
		 * header (2), and CRC (3).
		 */
		max_tx_octets = (lll->max_tx_time >> 3) - 10;
		break;

	case BIT(1):
		/* 2M PHY, 1us = 2 bits, hence divide by 4.
		 * Deduct 11 bytes for preamble (2), access address (4),
		 * header (2), and CRC (3).
		 */
		max_tx_octets = (lll->max_tx_time >> 2) - 11;
		break;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case BIT(2):
		if (lll->phy_flags & 0x01) {
			/* S8 Coded PHY, 8us = 1 bit, hence divide by
			 * 64.
			 * Subtract time for preamble (80), AA (256),
			 * CI (16), TERM1 (24), CRC (192) and
			 * TERM2 (24), total 592 us.
			 * Subtract 2 bytes for header.
			 */
			max_tx_octets = ((lll->max_tx_time - 592) >>
					  6) - 2;
		} else {
			/* S2 Coded PHY, 2us = 1 bit, hence divide by
			 * 16.
			 * Subtract time for preamble (80), AA (256),
			 * CI (16), TERM1 (24), CRC (48) and
			 * TERM2 (6), total 430 us.
			 * Subtract 2 bytes for header.
			 */
			max_tx_octets = ((lll->max_tx_time - 430) >>
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

	if (max_tx_octets > lll->max_tx_octets) {
		max_tx_octets = lll->max_tx_octets;
	}
#else /* !CONFIG_BT_CTLR_PHY */
	max_tx_octets = lll->max_tx_octets;
#endif /* !CONFIG_BT_CTLR_PHY */
#else /* !CONFIG_BT_CTLR_DATA_LENGTH */
	max_tx_octets = PDU_DC_PAYLOAD_SIZE_MIN;
#endif /* !CONFIG_BT_CTLR_DATA_LENGTH */
	return max_tx_octets;
}

static int init_reset(void)
{
	/* Initialize conn pool. */
	mem_init(conn_pool, sizeof(struct ll_conn),
		 sizeof(conn_pool) / sizeof(struct ll_conn), &conn_free);

	/* Initialize tx pool. */
	mem_init(mem_conn_tx.pool, CONN_TX_BUF_SIZE, CONFIG_BT_CTLR_TX_BUFFERS,
		 &mem_conn_tx.free);

	/* Initialize tx ctrl pool. */
	mem_init(mem_conn_tx_ctrl.pool, CONN_TX_CTRL_BUF_SIZE,
		 CONN_TX_CTRL_BUFFERS, &mem_conn_tx_ctrl.free);

	/* Initialize tx link pool. */
	mem_init(mem_link_tx.pool, sizeof(memq_link_t),
		 CONFIG_BT_CTLR_TX_BUFFERS + CONN_TX_CTRL_BUFFERS,
		 &mem_link_tx.free);

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

#if defined(CONFIG_BT_PERIPHERAL)
static void ticker_update_latency_cancel_op_cb(u32_t ticker_status,
					       void *params)
{
	struct ll_conn *conn = params;

	LL_ASSERT(ticker_status == TICKER_STATUS_SUCCESS);

	conn->slave.latency_cancel = 0U;
}
#endif /* CONFIG_BT_PERIPHERAL */

static void ticker_update_conn_op_cb(u32_t status, void *param)
{
	/* Slave drift compensation succeeds, or it fails in a race condition
	 * when disconnecting or connection update (race between ticker_update
	 * and ticker_stop calls).
	 */
	LL_ASSERT(status == TICKER_STATUS_SUCCESS ||
		  param == ull_update_mark_get() ||
		  param == ull_disable_mark_get());
}

static void ticker_op_stop_cb(u32_t status, void *param)
{
	u32_t retval;
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, tx_lll_flush};

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	mfy.param = param;

	/* Flush pending tx PDUs in LLL (using a mayfly) */
	retval = mayfly_enqueue(TICKER_USER_ID_ULL_LOW, TICKER_USER_ID_LLL, 0,
				&mfy);
	LL_ASSERT(!retval);
}

static inline void disable(u16_t handle)
{
	volatile u32_t ret_cb = TICKER_STATUS_BUSY;
	struct ll_conn *conn;
	void *mark;
	u32_t ret;

	conn = ll_conn_get(handle);

	mark = ull_disable_mark(conn);
	LL_ASSERT(mark == conn);

	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			  TICKER_ID_CONN_BASE + handle,
			  ull_ticker_status_give, (void *)&ret_cb);

	ret = ull_ticker_status_take(ret, &ret_cb);
	if (!ret) {
		ret = ull_disable(&conn->lll);
		LL_ASSERT(!ret);
	}

	conn->lll.link_tx_free = NULL;

	mark = ull_disable_unmark(conn);
	LL_ASSERT(mark == conn);
}

static void conn_cleanup(struct ll_conn *conn, u8_t reason)
{
	struct lll_conn *lll = &conn->lll;
	struct node_rx_pdu *rx;
	u32_t ticker_status;

	/* Only termination structure is populated here in ULL context
	 * but the actual enqueue happens in the LLL context in
	 * tx_lll_flush. The reason being to avoid passing the reason
	 * value and handle through the mayfly scheduling of the
	 * tx_lll_flush.
	 */
	rx = 0; // TODO(LLCP))
	rx->hdr.handle = conn->lll.handle;
	rx->hdr.type = NODE_RX_TYPE_TERMINATE;
	*((u8_t *)rx->pdu) = reason;

	/* release any llcp reserved rx node */
	// TODO(LLCP))
	while (rx) {
		struct node_rx_hdr *hdr;

		/* traverse to next rx node */
		hdr = &rx->hdr;
		rx = hdr->link->mem;

		/* Mark for buffer for release */
		hdr->type = NODE_RX_TYPE_DC_PDU_RELEASE;

		/* enqueue rx node towards Thread */
		ll_rx_put(hdr->link, hdr);
	}

	/* flush demux-ed Tx buffer still in ULL context */
	tx_ull_flush(conn);

	/* Stop Master or Slave role ticker */
	ticker_status = ticker_stop(TICKER_INSTANCE_ID_CTLR,
				    TICKER_USER_ID_ULL_HIGH,
				    TICKER_ID_CONN_BASE + lll->handle,
				    ticker_op_stop_cb, (void *)lll);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

	/* Invalidate the connection context */
	lll->handle = 0xFFFF;

	/* Demux and flush Tx PDUs that remain enqueued in thread context */
	ull_conn_tx_demux(UINT8_MAX);
}

static void tx_ull_flush(struct ll_conn *conn)
{
	while (conn->tx_head) {
		struct node_tx *tx;
		memq_link_t *link;

		tx = tx_ull_dequeue(conn, conn->tx_head);

		link = mem_acquire(&mem_link_tx.free);
		LL_ASSERT(link);

		memq_enqueue(link, tx, &conn->lll.memq_tx.tail);
	}
}

static void tx_lll_flush(void *param)
{
	struct ll_conn *conn = (void *)HDR_LLL2EVT(param);
	u16_t handle = ll_conn_handle_get(conn);
	struct lll_conn *lll = param;
	struct node_rx_pdu *rx;
	struct node_tx *tx;
	memq_link_t *link;

	lll_conn_flush(handle, lll);

	link = memq_dequeue(lll->memq_tx.tail, &lll->memq_tx.head,
			    (void **)&tx);
	while (link) {
		struct lll_tx *lll_tx;
		u8_t idx;

		idx = MFIFO_ENQUEUE_GET(conn_ack, (void **)&lll_tx);
		LL_ASSERT(lll_tx);

		lll_tx->handle = 0xFFFF;
		lll_tx->node = tx;

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
	rx = 0; // TODO(LLCP))
	LL_ASSERT(rx->hdr.link);
	link = rx->hdr.link;
	rx->hdr.link = NULL;

	/* Enqueue the terminate towards ULL context */
	ull_rx_put(link, rx);
	ull_rx_sched();
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

static inline u32_t feat_get(u8_t *features)
{
	u32_t feat;

	feat = ~LL_FEAT_BIT_MASK_VALID | features[0] |
	       (features[1] << 8) | (features[2] << 16);
	feat &= LL_FEAT_BIT_MASK;

	return feat;
}

static inline void ctrl_tx_pre_ack(struct ll_conn *conn,
				   struct pdu_data *pdu_tx)
{
	// TODO(LLCP))
}

static inline void ctrl_tx_ack(struct ll_conn *conn, struct node_tx **tx,
			       struct pdu_data *pdu_tx)
{
	// TODO(LLCP)
}

static inline bool pdu_len_cmp(u8_t opcode, u8_t len)
{
	const u8_t ctrl_len_lut[] = {
		(offsetof(struct pdu_data_llctrl, conn_update_ind) +
		 sizeof(struct pdu_data_llctrl_conn_update_ind)),
		(offsetof(struct pdu_data_llctrl, chan_map_ind) +
		 sizeof(struct pdu_data_llctrl_chan_map_ind)),
		(offsetof(struct pdu_data_llctrl, terminate_ind) +
		 sizeof(struct pdu_data_llctrl_terminate_ind)),
		(offsetof(struct pdu_data_llctrl, enc_req) +
		 sizeof(struct pdu_data_llctrl_enc_req)),
		(offsetof(struct pdu_data_llctrl, enc_rsp) +
		 sizeof(struct pdu_data_llctrl_enc_rsp)),
		(offsetof(struct pdu_data_llctrl, start_enc_req) +
		 sizeof(struct pdu_data_llctrl_start_enc_req)),
		(offsetof(struct pdu_data_llctrl, start_enc_rsp) +
		 sizeof(struct pdu_data_llctrl_start_enc_rsp)),
		(offsetof(struct pdu_data_llctrl, unknown_rsp) +
		 sizeof(struct pdu_data_llctrl_unknown_rsp)),
		(offsetof(struct pdu_data_llctrl, feature_req) +
		 sizeof(struct pdu_data_llctrl_feature_req)),
		(offsetof(struct pdu_data_llctrl, feature_rsp) +
		 sizeof(struct pdu_data_llctrl_feature_rsp)),
		(offsetof(struct pdu_data_llctrl, pause_enc_req) +
		 sizeof(struct pdu_data_llctrl_pause_enc_req)),
		(offsetof(struct pdu_data_llctrl, pause_enc_rsp) +
		 sizeof(struct pdu_data_llctrl_pause_enc_rsp)),
		(offsetof(struct pdu_data_llctrl, version_ind) +
		 sizeof(struct pdu_data_llctrl_version_ind)),
		(offsetof(struct pdu_data_llctrl, reject_ind) +
		 sizeof(struct pdu_data_llctrl_reject_ind)),
		(offsetof(struct pdu_data_llctrl, slave_feature_req) +
		 sizeof(struct pdu_data_llctrl_slave_feature_req)),
		(offsetof(struct pdu_data_llctrl, conn_param_req) +
		 sizeof(struct pdu_data_llctrl_conn_param_req)),
		(offsetof(struct pdu_data_llctrl, conn_param_rsp) +
		 sizeof(struct pdu_data_llctrl_conn_param_rsp)),
		(offsetof(struct pdu_data_llctrl, reject_ext_ind) +
		 sizeof(struct pdu_data_llctrl_reject_ext_ind)),
		(offsetof(struct pdu_data_llctrl, ping_req) +
		 sizeof(struct pdu_data_llctrl_ping_req)),
		(offsetof(struct pdu_data_llctrl, ping_rsp) +
		 sizeof(struct pdu_data_llctrl_ping_rsp)),
		(offsetof(struct pdu_data_llctrl, length_req) +
		 sizeof(struct pdu_data_llctrl_length_req)),
		(offsetof(struct pdu_data_llctrl, length_rsp) +
		 sizeof(struct pdu_data_llctrl_length_rsp)),
		(offsetof(struct pdu_data_llctrl, phy_req) +
		 sizeof(struct pdu_data_llctrl_phy_req)),
		(offsetof(struct pdu_data_llctrl, phy_rsp) +
		 sizeof(struct pdu_data_llctrl_phy_rsp)),
		(offsetof(struct pdu_data_llctrl, phy_upd_ind) +
		 sizeof(struct pdu_data_llctrl_phy_upd_ind)),
		(offsetof(struct pdu_data_llctrl, min_used_chans_ind) +
		 sizeof(struct pdu_data_llctrl_min_used_chans_ind)),
	};

	return ctrl_len_lut[opcode] == len;
}

static inline int ctrl_rx(memq_link_t *link, struct node_rx_pdu **rx,
			  struct pdu_data *pdu_rx, struct ll_conn *conn)
{
	// TODO(LLCP)
	return 0;
}
