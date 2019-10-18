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

#define LOG_MODULE_NAME bt_ctlr_llsw_ull_conn
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static void ticker_update_conn_op_cb(u32_t status, void *param);
static void ticker_stop_conn_op_cb(u32_t status, void *param);
static void ticker_start_conn_op_cb(u32_t status, void *param);

static inline void disable(u16_t handle);
static void conn_cleanup(struct ll_conn *conn, u8_t reason);
static void tx_ull_flush(struct ll_conn *conn);
static void tx_lll_flush(void *param);

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
static int empty_data_start_release(struct ll_conn *conn, struct node_tx *tx);
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

static void ctrl_tx_enqueue(struct ll_conn *conn, struct node_tx *tx);
static inline void event_fex_prep(struct ll_conn *conn);
static inline void event_vex_prep(struct ll_conn *conn);
static inline int event_conn_upd_prep(struct ll_conn *conn, u16_t lazy,
				      u32_t ticks_at_expire);
static inline void event_ch_map_prep(struct ll_conn *conn,
				     u16_t event_counter);

#if defined(CONFIG_BT_CTLR_LE_ENC)
static bool is_enc_req_pause_tx(struct ll_conn *conn);
static inline void event_enc_prep(struct ll_conn *conn);
static int enc_rsp_send(struct ll_conn *conn);
static int start_enc_rsp_send(struct ll_conn *conn,
			      struct pdu_data *pdu_ctrl_tx);
static inline bool ctrl_is_unexpected(struct ll_conn *conn, u8_t opcode);
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static inline void event_conn_param_prep(struct ll_conn *conn,
					 u16_t event_counter,
					 u32_t ticks_at_expire);
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_LE_PING)
static inline void event_ping_prep(struct ll_conn *conn);
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static inline void event_len_prep(struct ll_conn *conn);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static inline void event_phy_req_prep(struct ll_conn *conn);
static inline void event_phy_upd_ind_prep(struct ll_conn *conn,
					  u16_t event_counter);
#endif /* CONFIG_BT_CTLR_PHY */

static inline void ctrl_tx_pre_ack(struct ll_conn *conn,
				   struct pdu_data *pdu_tx);
static inline void ctrl_tx_ack(struct ll_conn *conn, struct node_tx **tx,
			       struct pdu_data *pdu_tx);
static inline int ctrl_rx(memq_link_t *link, struct node_rx_pdu **rx,
			  struct pdu_data *pdu_rx, struct ll_conn *conn);
#define CONN_TX_BUF_SIZE MROUND(offsetof(struct node_tx, pdu) + \
				offsetof(struct pdu_data, lldata) + \
				CONFIG_BT_CTLR_TX_BUFFER_SIZE)

#define CONN_TX_CTRL_BUFFERS 2
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

	return 0;
}

u8_t ll_conn_update(u16_t handle, u8_t cmd, u8_t status, u16_t interval_min,
		    u16_t interval_max, u16_t latency, u16_t timeout)
{
	struct ll_conn *conn;
	u8_t ret;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (!cmd) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		if (!conn->llcp_conn_param.disabled &&
		    (!conn->common.fex_valid ||
		     (conn->llcp_features &
		      BIT(BT_LE_FEAT_BIT_CONN_PARAM_REQ)))) {
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

		ret = ull_conn_llcp_req(conn);
		if (ret) {
			return ret;
		}

		conn->llcp.conn_upd.win_size = 1U;
		conn->llcp.conn_upd.win_offset_us = 0U;
		conn->llcp.conn_upd.interval = interval_max;
		conn->llcp.conn_upd.latency = latency;
		conn->llcp.conn_upd.timeout = timeout;
		/* conn->llcp.conn_upd.instant     = 0; */
		conn->llcp.conn_upd.state = LLCP_CUI_STATE_USE;
		conn->llcp.conn_upd.is_internal = 0U;

		conn->llcp_type = LLCP_CONN_UPD;
		conn->llcp_req++;
	} else {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		cmd--;

		if (cmd) {
			if ((conn->llcp_conn_param.req ==
			     conn->llcp_conn_param.ack) ||
			    (conn->llcp_conn_param.state !=
			     LLCP_CPR_STATE_APP_WAIT)) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}

			conn->llcp_conn_param.status = status;
			conn->llcp_conn_param.state = cmd;
			conn->llcp_conn_param.cmd = 1U;
		} else {
			if (conn->llcp_conn_param.req !=
			    conn->llcp_conn_param.ack) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}

			conn->llcp_conn_param.status = 0U;
			conn->llcp_conn_param.interval_min = interval_min;
			conn->llcp_conn_param.interval_max = interval_max;
			conn->llcp_conn_param.latency = latency;
			conn->llcp_conn_param.timeout = timeout;
			conn->llcp_conn_param.state = cmd;
			conn->llcp_conn_param.cmd = 1U;
			conn->llcp_conn_param.req++;
		}

#else /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
		/* CPR feature not supported */
		return BT_HCI_ERR_CMD_DISALLOWED;
#endif /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
	}

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
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	conn->llcp_terminate.reason_own = reason;

	conn->llcp_terminate.req++;

	return 0;
}

u8_t ll_feature_req_send(u16_t handle)
{
	struct ll_conn *conn;
	u8_t ret;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ret = ull_conn_llcp_req(conn);
	if (ret) {
		return ret;
	}

	conn->llcp_type = LLCP_FEATURE_EXCHANGE;
	conn->llcp_req++;

	return 0;
}

u8_t ll_version_ind_send(u16_t handle)
{
	struct ll_conn *conn;
	u8_t ret;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ret = ull_conn_llcp_req(conn);
	if (ret) {
		return ret;
	}

	conn->llcp_type = LLCP_VERSION_EXCHANGE;
	conn->llcp_req++;

	return 0;
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
u32_t ll_length_req_send(u16_t handle, u16_t tx_octets, u16_t tx_time)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if ((conn->llcp_req != conn->llcp_ack) ||
	    (conn->llcp_length.req != conn->llcp_length.ack)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* TODO: parameter check tx_octets and tx_time */

	conn->llcp_length.state = LLCP_LENGTH_STATE_REQ;
	conn->llcp_length.tx_octets = tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
	conn->llcp_length.tx_time = tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

	conn->llcp_length.req++;

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
	*max_tx_time = PKT_US(LL_LENGTH_OCTETS_RX_MAX, BIT(2));
	*max_rx_time = PKT_US(LL_LENGTH_OCTETS_RX_MAX, BIT(2));
#else /* !CONFIG_BT_CTLR_PHY */
	/* Default is 1M packet timing */
	*max_tx_time = PKT_US(LL_LENGTH_OCTETS_RX_MAX, 0);
	*max_rx_time = PKT_US(LL_LENGTH_OCTETS_RX_MAX, 0);
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
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if ((conn->llcp_req != conn->llcp_ack) ||
	    (conn->llcp_phy.req != conn->llcp_phy.ack)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	conn->llcp_phy.state = LLCP_PHY_STATE_REQ;
	conn->llcp_phy.cmd = 1U;
	conn->llcp_phy.tx = tx;
	conn->llcp_phy.flags = flags;
	conn->llcp_phy.rx = rx;
	conn->llcp_phy.req++;

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

	entropy = device_get_binding(CONFIG_ENTROPY_NAME);
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
#if defined(CONFIG_BT_CTLR_LE_ENC)
		if (conn->llcp_enc.pause_rx) {
			conn->llcp_terminate.reason_peer =
				BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;

			/* Mark for buffer for release */
			(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
		}
#endif /* CONFIG_BT_CTLR_LE_ENC */
		break;

	case PDU_DATA_LLID_RESV:
	default:
#if defined(CONFIG_BT_CTLR_LE_ENC)
		if (conn->llcp_enc.pause_rx) {
			conn->llcp_terminate.reason_peer =
				BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;
		}
#endif /* CONFIG_BT_CTLR_LE_ENC */

		/* Invalid LL id, drop it. */

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		break;
	}


	return 0;
}

int ull_conn_llcp(struct ll_conn *conn, u32_t ticks_at_expire, u16_t lazy)
{
	LL_ASSERT(conn->lll.handle != 0xFFFF);

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ) || defined(CONFIG_BT_CTLR_PHY)
	/* Check if no other procedure with instant is requested and not in
	 * Encryption setup.
	 */
	if ((conn->llcp_ack == conn->llcp_req) &&
#if defined(CONFIG_BT_CTLR_LE_ENC)
	    !conn->llcp_enc.pause_rx) {
#else /* !CONFIG_BT_CTLR_LE_ENC */
	    1) {
#endif /* !CONFIG_BT_CTLR_LE_ENC */
		if (0) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		/* check if CPR procedure is requested */
		} else if (conn->llcp_conn_param.ack !=
			   conn->llcp_conn_param.req) {
			struct lll_conn *lll = &conn->lll;
			u16_t event_counter;

			/* Calculate current event counter */
			event_counter = lll->event_counter +
					lll->latency_prepare + lazy;

			/* handle CPR state machine */
			event_conn_param_prep(conn, event_counter,
					      ticks_at_expire);
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		/* check if procedure is requested */
		} else if (conn->llcp_length.ack != conn->llcp_length.req) {
			/* handle DLU state machine */
			event_len_prep(conn);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
		/* check if PHY Req procedure is requested */
		} else if (conn->llcp_phy.ack != conn->llcp_phy.req) {
			/* handle PHY Upd state machine */
			event_phy_req_prep(conn);
#endif /* CONFIG_BT_CTLR_PHY */
		}
	}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ || CONFIG_BT_CTLR_PHY */

	/* check if procedure is requested */
	if (((conn->llcp_req - conn->llcp_ack) & 0x03) == 0x02) {
		switch (conn->llcp_type) {
		case LLCP_CONN_UPD:
		{
			if (event_conn_upd_prep(conn, lazy,
						ticks_at_expire) == 0) {
				return -ECANCELED;
			}
		}
		break;

		case LLCP_CHAN_MAP:
		{
			struct lll_conn *lll = &conn->lll;
			u16_t event_counter;

			/* Calculate current event counter */
			event_counter = lll->event_counter +
					lll->latency_prepare + lazy;

			event_ch_map_prep(conn, event_counter);
		}
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
		case LLCP_ENCRYPTION:
			event_enc_prep(conn);
			break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

		case LLCP_FEATURE_EXCHANGE:
			event_fex_prep(conn);
			break;

		case LLCP_VERSION_EXCHANGE:
			event_vex_prep(conn);
			break;

#if defined(CONFIG_BT_CTLR_LE_PING)
		case LLCP_PING:
			event_ping_prep(conn);
			break;
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_PHY)
		case LLCP_PHY_UPD:
		{
			struct lll_conn *lll = &conn->lll;
			u16_t event_counter;

			/* Calculate current event counter */
			event_counter = lll->event_counter +
					lll->latency_prepare + lazy;

			event_phy_upd_ind_prep(conn, event_counter);
		}
		break;
#endif /* CONFIG_BT_CTLR_PHY */

		default:
			LL_ASSERT(0);
			break;
		}
	}

	/* Terminate Procedure Request */
	if (conn->llcp_terminate.ack != conn->llcp_terminate.req) {
		struct node_tx *tx;

		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (tx) {
			struct pdu_data *pdu_tx = (void *)tx->pdu;

			/* Terminate Procedure acked */
			conn->llcp_terminate.ack = conn->llcp_terminate.req;

			/* place the terminate ind packet in tx queue */
			pdu_tx->ll_id = PDU_DATA_LLID_CTRL;
			pdu_tx->len = offsetof(struct pdu_data_llctrl,
						    terminate_ind) +
				sizeof(struct pdu_data_llctrl_terminate_ind);
			pdu_tx->llctrl.opcode =
				PDU_DATA_LLCTRL_TYPE_TERMINATE_IND;
			pdu_tx->llctrl.terminate_ind.error_code =
				conn->llcp_terminate.reason_own;

			ctrl_tx_enqueue(conn, tx);
		}

		if (!conn->procedure_expire) {
			/* Terminate Procedure timeout is started, will
			 * replace any other timeout running
			 */
			conn->procedure_expire = conn->supervision_reload;

			/* NOTE: if supervision timeout equals connection
			 * interval, dont timeout in current event.
			 */
			if (conn->procedure_expire <= 1U) {
				conn->procedure_expire++;
			}
		}
	}

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
		if (lll->enc_rx || conn->llcp_enc.pause_rx) {
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
		conn->llcp_terminate.reason_peer =
			BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;
		break;
	}
#endif /* CONFIG_BT_CTLR_LE_ENC */

	/* Master transmitted ack for the received terminate ind or
	 * Slave received terminate ind or MIC failure
	 */
	reason_peer = conn->llcp_terminate.reason_peer;
	if (reason_peer && (lll->role || lll->master.terminate_ack)) {
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
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && lll->role) {
			ull_slave_done(done, &ticks_drift_plus,
				       &ticks_drift_minus);

			if (conn->tx_head || memq_peek(lll->memq_tx.head,
						       lll->memq_tx.tail,
						       NULL)) {
				lll->latency_event = 0;
			} else if (lll->slave.latency_enabled) {
				lll->latency_event = lll->latency;
			}
		} else if (reason_peer) {
			lll->master.terminate_ack = 1;
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
			    (conn->llcp_req == conn->llcp_ack)) {
				conn->llcp_type = LLCP_PING;
				conn->llcp_ack -= 2U;
			}
		}
	}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
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
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

	/* break latency based on ctrl procedure pending */
	if ((((conn->llcp_req - conn->llcp_ack) & 0x03) == 0x02) &&
	    ((conn->llcp_type == LLCP_CONN_UPD) ||
	     (conn->llcp_type == LLCP_CHAN_MAP))) {
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
	       ((
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		 !conn->llcp_length.pause_tx &&
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_PHY)
		 !conn->llcp_phy.pause_tx &&
#endif /* CONFIG_BT_CTLR_PHY */
#if defined(CONFIG_BT_CTLR_LE_ENC)
		 !conn->llcp_enc.pause_tx &&
		 !(pause_tx = is_enc_req_pause_tx(conn)) &&
#endif /* CONFIG_BT_CTLR_LE_ENC */
		 1) ||
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
	struct ll_conn * const conn_hdr = conn;
	if (conn_hdr->llcp_req != conn_hdr->llcp_ack) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	conn_hdr->llcp_req++;
	if (((conn_hdr->llcp_req - conn_hdr->llcp_ack) & 0x03) != 1) {
		conn_hdr->llcp_req--;
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	return 0;
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
	default_tx_time = PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, 0);
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

static void ticker_stop_conn_op_cb(u32_t status, void *param)
{
	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	void *p = ull_update_mark(param);

	LL_ASSERT(p == param);
}

static void ticker_start_conn_op_cb(u32_t status, void *param)
{
	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	void *p = ull_update_unmark(param);

	LL_ASSERT(p == param);
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
	rx = (void *)&conn->llcp_terminate.node_rx;
	rx->hdr.handle = conn->lll.handle;
	rx->hdr.type = NODE_RX_TYPE_TERMINATE;
	*((u8_t *)rx->pdu) = reason;

	/* release any llcp reserved rx node */
	rx = conn->llcp_rx;
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
	struct lll_conn *lll = param;
	struct node_rx_pdu *rx;
	struct node_tx *tx;
	memq_link_t *link;

	lll_conn_flush(lll);

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
	rx = (void *)&conn->llcp_terminate.node_rx;
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

static void ctrl_tx_last_enqueue(struct ll_conn *conn,
				      struct node_tx *tx)
{
	tx->next = conn->tx_ctrl_last->next;
	conn->tx_ctrl_last->next = tx;
	conn->tx_ctrl_last = tx;
}

static void ctrl_tx_enqueue(struct ll_conn *conn, struct node_tx *tx)
{
	/* check if a packet was tx-ed and not acked by peer */
	if (
	    /* data/ctrl packet is in the head */
	    conn->tx_head &&
#if defined(CONFIG_BT_CTLR_LE_ENC)
	    !conn->llcp_enc.pause_tx &&
#endif /* CONFIG_BT_CTLR_LE_ENC */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	    !conn->llcp_length.pause_tx &&
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_PHY)
	    !conn->llcp_phy.pause_tx &&
#endif /* CONFIG_BT_CTLR_PHY */
	    1) {
		/* data or ctrl may have been transmitted once, but not acked
		 * by peer, hence place this new ctrl after head
		 */

		/* if data transmitted once, keep it at head of the tx list,
		 * as we will insert a ctrl after it, hence advance the
		 * data pointer
		 */
		if (conn->tx_head == conn->tx_data) {
			conn->tx_data = conn->tx_data->next;
		}

		/* if no ctrl packet already queued, new ctrl added will be
		 * the ctrl pointer and is inserted after head.
		 */
		if (!conn->tx_ctrl) {
			tx->next = conn->tx_head->next;
			conn->tx_head->next = tx;
			conn->tx_ctrl = tx;
			conn->tx_ctrl_last = tx;
		} else {
			ctrl_tx_last_enqueue(conn, tx);
		}
	} else {
		/* No packet needing ACK. */

		/* If first ctrl packet then add it as head else add it to the
		 * tail of the ctrl packets.
		 */
		if (!conn->tx_ctrl) {
			tx->next = conn->tx_head;
			conn->tx_head = tx;
			conn->tx_ctrl = tx;
			conn->tx_ctrl_last = tx;
		} else {
			ctrl_tx_last_enqueue(conn, tx);
		}
	}

	/* Update last pointer if ctrl added at end of tx list */
	if (!tx->next) {
		conn->tx_data_last = tx;
	}
}

static void ctrl_tx_sec_enqueue(struct ll_conn *conn, struct node_tx *tx)
{
#if defined(CONFIG_BT_CTLR_LE_ENC)
	if (conn->llcp_enc.pause_tx) {
		if (!conn->tx_ctrl) {
			tx->next = conn->tx_head;
			conn->tx_head = tx;
		} else {
			tx->next = conn->tx_ctrl_last->next;
			conn->tx_ctrl_last->next = tx;
		}
	} else
#endif /* CONFIG_BT_CTLR_LE_ENC */

	{
		ctrl_tx_enqueue(conn, tx);
	}
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
static bool is_enc_req_pause_tx(struct ll_conn *conn)
{
	struct pdu_data *pdu_data_tx;

	pdu_data_tx = (void *)conn->tx_head->pdu;
	if ((pdu_data_tx->ll_id == PDU_DATA_LLID_CTRL) &&
	    (pdu_data_tx->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_ENC_REQ)) {
		if ((conn->llcp_req != conn->llcp_ack) ||
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		    (conn->llcp_conn_param.ack != conn->llcp_conn_param.req) ||
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		    (conn->llcp_length.ack != conn->llcp_length.req) ||
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_PHY)
		    (conn->llcp_phy.ack != conn->llcp_phy.req) ||
#endif /* CONFIG_BT_CTLR_PHY */
		    0) {
			struct node_tx *tx;

			/* if we have control packets enqueued after this PDU
			 * bring it ahead, and move the enc_req to last of
			 * ctrl queue.
			 */
			tx = conn->tx_head;
			if ((tx->next != NULL) &&
			    (tx->next == conn->tx_ctrl)) {
				conn->tx_head = tx->next;
				tx->next = conn->tx_ctrl_last->next;
				conn->tx_ctrl_last->next = tx;
				conn->tx_data = tx;
				if (!conn->tx_data_last) {
					conn->tx_data_last = tx;
				}

				/* Head now contains a control packet permitted
				 * to be transmitted to peer.
				 */
				return false;
			}

			/* Head contains ENC_REQ packet deferred due to another
			 * control procedure in progress.
			 */
			return true;
		}

		conn->llcp.encryption.initiate = 1U;

		conn->llcp_type = LLCP_ENCRYPTION;
		conn->llcp_ack -= 2U;
	}

	/* Head contains a permitted data or control packet. */
	return false;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

static inline void event_conn_upd_init(struct ll_conn *conn,
				       u16_t event_counter,
				       u32_t ticks_at_expire,
				       struct pdu_data *pdu_ctrl_tx,
				       struct mayfly *mfy_sched_offset,
				       void (*fp_mfy_select_or_use)(void *))
{
	/* move to in progress */
	conn->llcp.conn_upd.state = LLCP_CUI_STATE_INPROG;

	/* set instant */
	conn->llcp.conn_upd.instant = event_counter + conn->lll.latency + 6;

	/* place the conn update req packet as next in tx queue */
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, conn_update_ind) +
		sizeof(struct pdu_data_llctrl_conn_update_ind);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;
	pdu_ctrl_tx->llctrl.conn_update_ind.win_size =
		conn->llcp.conn_upd.win_size;
	pdu_ctrl_tx->llctrl.conn_update_ind.win_offset =
		sys_cpu_to_le16(conn->llcp.conn_upd.win_offset_us / 1250U);
	pdu_ctrl_tx->llctrl.conn_update_ind.interval =
		sys_cpu_to_le16(conn->llcp.conn_upd.interval);
	pdu_ctrl_tx->llctrl.conn_update_ind.latency =
		sys_cpu_to_le16(conn->llcp.conn_upd.latency);
	pdu_ctrl_tx->llctrl.conn_update_ind.timeout =
		sys_cpu_to_le16(conn->llcp.conn_upd.timeout);
	pdu_ctrl_tx->llctrl.conn_update_ind.instant =
		sys_cpu_to_le16(conn->llcp.conn_upd.instant);

#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
	{
		u32_t retval;

		/* calculate window offset that places the connection in the
		 * next available slot after existing masters.
		 */
		conn->llcp.conn_upd.ticks_anchor = ticks_at_expire;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
		if (conn->evt.ticks_xtal_to_start & XON_BITMASK) {
			u32_t ticks_prepare_to_start =
				MAX(conn->evt.ticks_active_to_start,
				    conn->evt.ticks_preempt_to_start);

			conn->llcp.conn_upd.ticks_anchor -=
				(conn->evt.ticks_xtal_to_start &
				 ~XON_BITMASK) - ticks_prepare_to_start;
		}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

		conn->llcp.conn_upd.pdu_win_offset = (u16_t *)
			&pdu_ctrl_tx->llctrl.conn_update_ind.win_offset;

		mfy_sched_offset->fp = fp_mfy_select_or_use;
		mfy_sched_offset->param = (void *)conn;

		retval = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
					TICKER_USER_ID_ULL_LOW, 1,
					mfy_sched_offset);
		LL_ASSERT(!retval);
	}
#else /* !CONFIG_BT_CTLR_SCHED_ADVANCED */
	ARG_UNUSED(ticks_at_expire);
	ARG_UNUSED(mfy_sched_offset);
	ARG_UNUSED(fp_mfy_select_or_use);
#endif /* !CONFIG_BT_CTLR_SCHED_ADVANCED */
}

static inline int event_conn_upd_prep(struct ll_conn *conn, u16_t lazy,
				      u32_t ticks_at_expire)
{
	struct lll_conn *lll = &conn->lll;
	struct ll_conn *conn_upd;
	u16_t instant_latency;
	u16_t event_counter;


	conn_upd = conn_upd_curr;

	/* set mutex */
	if (!conn_upd) {
		conn_upd_curr = conn;
	}

	/* Calculate current event counter */
	event_counter = lll->event_counter + lll->latency_prepare + lazy;

	instant_latency = (event_counter - conn->llcp.conn_upd.instant) &
			  0xffff;
	if (conn->llcp.conn_upd.state != LLCP_CUI_STATE_INPROG) {
#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
		static memq_link_t s_link;
		static struct mayfly s_mfy_sched_offset = {0, 0,
			&s_link, 0, 0 };
		void (*fp_mfy_select_or_use)(void *) = NULL;
#endif /* CONFIG_BT_CTLR_SCHED_ADVANCED */
		struct pdu_data *pdu_ctrl_tx;
		struct node_rx_pdu *rx;
		struct node_tx *tx;

		LL_ASSERT(!conn->llcp_rx);

		rx = ll_pdu_rx_alloc_peek(1);
		if (!rx) {
			return -ENOBUFS;
		}

		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (!tx) {
			return -ENOBUFS;
		}

		(void)ll_pdu_rx_alloc();
		conn->llcp_rx = rx;

		pdu_ctrl_tx = (void *)tx->pdu;

#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
		switch (conn->llcp.conn_upd.state) {
		case LLCP_CUI_STATE_USE:
			fp_mfy_select_or_use = ull_sched_mfy_win_offset_use;
			break;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		case LLCP_CUI_STATE_SELECT:
			fp_mfy_select_or_use = ull_sched_mfy_win_offset_select;
			break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

		default:
			LL_ASSERT(0);
			break;
		}

		event_conn_upd_init(conn, event_counter, ticks_at_expire,
				    pdu_ctrl_tx, &s_mfy_sched_offset,
				    fp_mfy_select_or_use);
#else /* !CONFIG_BT_CTLR_SCHED_ADVANCED */
		event_conn_upd_init(conn, event_counter, ticks_at_expire,
				    pdu_ctrl_tx, NULL, NULL);
#endif /* !CONFIG_BT_CTLR_SCHED_ADVANCED */

		ctrl_tx_enqueue(conn, tx);

	} else if (instant_latency <= 0x7FFF) {
		u32_t ticks_slot_overhead;
		u16_t conn_interval_old;
		u16_t conn_interval_new;
		u32_t ticks_win_offset;
		u32_t conn_interval_us;
		struct node_rx_pdu *rx;
		u8_t ticker_id_conn;
		u32_t ticker_status;
		u32_t periodic_us;
		u16_t latency;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		if ((conn->llcp_conn_param.req != conn->llcp_conn_param.ack) &&
		    (conn->llcp_conn_param.state == LLCP_CPR_STATE_UPD)) {
			conn->llcp_conn_param.ack = conn->llcp_conn_param.req;

			/* Stop procedure timeout */
			conn->procedure_expire = 0U;
		}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

		/* reset mutex */
		if (conn_upd_curr == conn) {
			conn_upd_curr = NULL;
		}

		lll = &conn->lll;

		/* Acquire Rx node */
		rx = conn->llcp_rx;
		LL_ASSERT(rx && rx->hdr.link);
		conn->llcp_rx = rx->hdr.link->mem;

		/* Prepare the rx packet structure */
		if ((conn->llcp.conn_upd.interval != lll->interval) ||
		    (conn->llcp.conn_upd.latency != lll->latency) ||
		    (RADIO_CONN_EVENTS(conn->llcp.conn_upd.timeout * 10000U,
				       lll->interval * 1250) !=
		     conn->supervision_reload)) {
			struct node_rx_cu *cu;

			rx->hdr.handle = lll->handle;
			rx->hdr.type = NODE_RX_TYPE_CONN_UPDATE;

			/* prepare connection update complete structure */
			cu = (void *)rx->pdu;
			cu->status = 0x00;
			cu->interval = conn->llcp.conn_upd.interval;
			cu->latency = conn->llcp.conn_upd.latency;
			cu->timeout = conn->llcp.conn_upd.timeout;
		} else {
			/* Mark for buffer for release */
			rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
		}

		/* enqueue rx node towards Thread */
		ll_rx_put(rx->hdr.link, rx);
		ll_rx_sched();

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
		/* restore to normal prepare */
		if (conn->evt.ticks_xtal_to_start & XON_BITMASK) {
			u32_t ticks_prepare_to_start =
				MAX(conn->evt.ticks_active_to_start,
				    conn->evt.ticks_preempt_to_start);

			conn->evt.ticks_xtal_to_start &= ~XON_BITMASK;
			ticks_at_expire -= (conn->evt.ticks_xtal_to_start -
					    ticks_prepare_to_start);
		}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

		/* compensate for instant_latency due to laziness */
		conn_interval_old = instant_latency * lll->interval;
		latency = conn_interval_old /
			conn->llcp.conn_upd.interval;
		conn_interval_new = latency *
			conn->llcp.conn_upd.interval;
		if (conn_interval_new > conn_interval_old) {
			ticks_at_expire += HAL_TICKER_US_TO_TICKS(
				(conn_interval_new - conn_interval_old) * 1250U);
		} else {
			ticks_at_expire -= HAL_TICKER_US_TO_TICKS(
				(conn_interval_old - conn_interval_new) * 1250U);
		}
		lll->latency_prepare += lazy;
		lll->latency_prepare -= (instant_latency - latency);

		/* calculate the offset */
		if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
			ticks_slot_overhead =
				MAX(conn->evt.ticks_active_to_start,
				    conn->evt.ticks_xtal_to_start);

		} else {
			ticks_slot_overhead = 0U;
		}

		/* calculate the window widening and interval */
		conn_interval_us = conn->llcp.conn_upd.interval * 1250U;
		periodic_us = conn_interval_us;
		if (lll->role) {
			lll->slave.window_widening_prepare_us -=
				lll->slave.window_widening_periodic_us *
				instant_latency;

			lll->slave.window_widening_periodic_us =
				(((lll_conn_ppm_local_get() +
				   lll_conn_ppm_get(lll->slave.sca)) *
				  conn_interval_us) + (1000000 - 1)) / 1000000U;
			lll->slave.window_widening_max_us =
				(conn_interval_us >> 1) - EVENT_IFS_US;
			lll->slave.window_size_prepare_us =
				conn->llcp.conn_upd.win_size * 1250U;
			conn->slave.ticks_to_offset = 0U;

			lll->slave.window_widening_prepare_us +=
				lll->slave.window_widening_periodic_us *
				latency;
			if (lll->slave.window_widening_prepare_us >
			    lll->slave.window_widening_max_us) {
				lll->slave.window_widening_prepare_us =
					lll->slave.window_widening_max_us;
			}

			ticks_at_expire -= HAL_TICKER_US_TO_TICKS(
				lll->slave.window_widening_periodic_us *
				latency);
			ticks_win_offset = HAL_TICKER_US_TO_TICKS(
				(conn->llcp.conn_upd.win_offset_us / 1250U) *
				1250U);
			periodic_us -= lll->slave.window_widening_periodic_us;
		} else {
			ticks_win_offset = HAL_TICKER_US_TO_TICKS(
				conn->llcp.conn_upd.win_offset_us);

			/* Workaround: Due to the missing remainder param in
			 * ticker_start function for first interval; add a
			 * tick so as to use the ceiled value.
			 */
			ticks_win_offset += 1U;
		}
		lll->interval = conn->llcp.conn_upd.interval;
		lll->latency = conn->llcp.conn_upd.latency;
		conn->supervision_reload =
			RADIO_CONN_EVENTS((conn->llcp.conn_upd.timeout
					   * 10U * 1000U), conn_interval_us);
		conn->procedure_reload =
			RADIO_CONN_EVENTS((40 * 1000 * 1000), conn_interval_us);

#if defined(CONFIG_BT_CTLR_LE_PING)
		/* APTO in no. of connection events */
		conn->apto_reload = RADIO_CONN_EVENTS((30 * 1000 * 1000),
						      conn_interval_us);
		/* Dispatch LE Ping PDU 6 connection events (that peer would
		 * listen to) before 30s timeout
		 * TODO: "peer listens to" is greater than 30s due to latency
		 */
		conn->appto_reload = (conn->apto_reload > (lll->latency + 6)) ?
				     (conn->apto_reload - (lll->latency + 6)) :
				     conn->apto_reload;
#endif /* CONFIG_BT_CTLR_LE_PING */

		if (!conn->llcp.conn_upd.is_internal) {
			conn->supervision_expire = 0U;
		}

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
		/* disable ticker job, in order to chain stop and start
		 * to avoid RTC being stopped if no tickers active.
		 */
		u32_t mayfly_was_enabled =
			mayfly_is_enabled(TICKER_USER_ID_ULL_HIGH,
					  TICKER_USER_ID_ULL_LOW);
		mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW,
			      0);
#endif

		/* start slave/master with new timings */
		ticker_id_conn = TICKER_ID_CONN_BASE + ll_conn_handle_get(conn);
		ticker_status =	ticker_stop(TICKER_INSTANCE_ID_CTLR,
					    TICKER_USER_ID_ULL_HIGH,
					    ticker_id_conn,
					    ticker_stop_conn_op_cb,
					    (void *)conn);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
		ticker_status =
			ticker_start(TICKER_INSTANCE_ID_CTLR,
				     TICKER_USER_ID_ULL_HIGH,
				     ticker_id_conn,
				     ticks_at_expire, ticks_win_offset,
				     HAL_TICKER_US_TO_TICKS(periodic_us),
				     HAL_TICKER_REMAINDER(periodic_us),
#if defined(CONFIG_BT_CTLR_CONN_META)
				     TICKER_LAZY_MUST_EXPIRE,
#else
				     TICKER_NULL_LAZY,
#endif /* CONFIG_BT_CTLR_CONN_META */
				     (ticks_slot_overhead +
				      conn->evt.ticks_slot),
#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_CENTRAL)
				     lll->role ? ull_slave_ticker_cb :
						 ull_master_ticker_cb,
#elif defined(CONFIG_BT_PERIPHERAL)
				     ull_slave_ticker_cb,
#else
				     ull_master_ticker_cb,
#endif
				     conn, ticker_start_conn_op_cb,
				     (void *)conn);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
		/* enable ticker job, if disabled in this function */
		if (mayfly_was_enabled) {
			mayfly_enable(TICKER_USER_ID_ULL_HIGH,
				      TICKER_USER_ID_ULL_LOW, 1);
		}
#endif

		return 0;
	}

	return -EINPROGRESS;
}

static inline void event_ch_map_prep(struct ll_conn *conn,
				     u16_t event_counter)
{
	if (conn->llcp.chan_map.initiate) {
		struct node_tx *tx;

		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (tx) {
			struct pdu_data *pdu_ctrl_tx = (void *)tx->pdu;

			/* reset initiate flag */
			conn->llcp.chan_map.initiate = 0U;

			/* set instant */
			conn->llcp.chan_map.instant = event_counter +
						      conn->lll.latency + 6;

			/* place the channel map req packet as next in
			 * tx queue
			 */
			pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
			pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl,
						    chan_map_ind) +
				sizeof(struct pdu_data_llctrl_chan_map_ind);
			pdu_ctrl_tx->llctrl.opcode =
				PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND;
			memcpy(&pdu_ctrl_tx->llctrl.chan_map_ind.chm[0],
			       &conn->llcp.chan_map.chm[0],
			       sizeof(pdu_ctrl_tx->llctrl.chan_map_ind.chm));
			pdu_ctrl_tx->llctrl.chan_map_ind.instant =
				sys_cpu_to_le16(conn->llcp.chan_map.instant);

			ctrl_tx_enqueue(conn, tx);
		}
	} else if (((event_counter - conn->llcp.chan_map.instant) & 0xFFFF)
			    <= 0x7FFF) {
		struct lll_conn *lll = &conn->lll;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		/* copy to active channel map */
		memcpy(&lll->data_chan_map[0],
		       &conn->llcp.chan_map.chm[0],
		       sizeof(lll->data_chan_map));
		lll->data_chan_count =
			util_ones_count_get(&lll->data_chan_map[0],
					    sizeof(lll->data_chan_map));
		conn->chm_updated = 1U;
	}

}

#if defined(CONFIG_BT_CTLR_LE_ENC)
static inline void event_enc_reject_prep(struct ll_conn *conn,
					 struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;

	if (conn->common.fex_valid &&
	    (conn->llcp_features & BIT(BT_LE_FEAT_BIT_EXT_REJ_IND))) {
		struct pdu_data_llctrl_reject_ext_ind *p;

		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;

		p = (void *)&pdu->llctrl.reject_ext_ind;
		p->reject_opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ;
		p->error_code = conn->llcp.encryption.error_code;

		pdu->len = sizeof(struct pdu_data_llctrl_reject_ext_ind);
	} else {
		struct pdu_data_llctrl_reject_ind *p;

		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_IND;

		p = (void *)&pdu->llctrl.reject_ind;
		p->error_code =	conn->llcp.encryption.error_code;

		pdu->len = sizeof(struct pdu_data_llctrl_reject_ind);
	}

	pdu->len += offsetof(struct pdu_data_llctrl, reject_ind);

	conn->llcp.encryption.error_code = 0U;
}

static inline void event_enc_prep(struct ll_conn *conn)
{
	struct pdu_data *pdu_ctrl_tx;
	struct node_tx *tx;
	struct lll_conn *lll;

	if (conn->llcp.encryption.initiate) {
		return;
	}

	tx = mem_acquire(&mem_conn_tx_ctrl.free);
	if (!tx) {
		return;
	}

	lll = &conn->lll;

	pdu_ctrl_tx = (void *)tx->pdu;

	/* master sends encrypted enc start rsp in control priority */
	if (!lll->role) {
		/* calc the Session Key */
		ecb_encrypt(&conn->llcp_enc.ltk[0],
			    &conn->llcp.encryption.skd[0], NULL,
			    &lll->ccm_rx.key[0]);

		/* copy the Session Key */
		memcpy(&lll->ccm_tx.key[0], &lll->ccm_rx.key[0],
		       sizeof(lll->ccm_tx.key));

		/* copy the IV */
		memcpy(&lll->ccm_tx.iv[0], &lll->ccm_rx.iv[0],
		       sizeof(lll->ccm_tx.iv));

		/* initialise counter */
		lll->ccm_rx.counter = 0;
		lll->ccm_tx.counter = 0;

		/* set direction: slave to master = 0,
		 * master to slave = 1
		 */
		lll->ccm_rx.direction = 0;
		lll->ccm_tx.direction = 1;

		/* enable receive encryption */
		lll->enc_rx = 1;

		/* send enc start resp */
		start_enc_rsp_send(conn, pdu_ctrl_tx);

		ctrl_tx_enqueue(conn, tx);
	}

	/* slave send reject ind or start enc req at control priority */

#if defined(CONFIG_BT_CTLR_FAST_ENC)
	else {
#else /* !CONFIG_BT_CTLR_FAST_ENC */
	else if (!conn->llcp_enc.pause_tx || conn->llcp_enc.refresh) {
#endif /* !CONFIG_BT_CTLR_FAST_ENC */

		/* place the reject ind packet as next in tx queue */
		if (conn->llcp.encryption.error_code) {
			event_enc_reject_prep(conn, pdu_ctrl_tx);

			ctrl_tx_enqueue(conn, tx);
		}
		/* place the start enc req packet as next in tx queue */
		else {

#if !defined(CONFIG_BT_CTLR_FAST_ENC)
			u8_t err;

			/* TODO BT Spec. text: may finalize the sending
			 * of additional data channel PDUs queued in the
			 * controller.
			 */
			err = enc_rsp_send(conn);
			if (err) {
				mem_release(tx, &mem_conn_tx_ctrl.free);

				return;
			}
#endif /* !CONFIG_BT_CTLR_FAST_ENC */

			/* calc the Session Key */
			ecb_encrypt(&conn->llcp_enc.ltk[0],
				    &conn->llcp.encryption.skd[0], NULL,
				    &lll->ccm_rx.key[0]);

			/* copy the Session Key */
			memcpy(&lll->ccm_tx.key[0],
			       &lll->ccm_rx.key[0],
			       sizeof(lll->ccm_tx.key));

			/* copy the IV */
			memcpy(&lll->ccm_tx.iv[0], &lll->ccm_rx.iv[0],
			       sizeof(lll->ccm_tx.iv));

			/* initialise counter */
			lll->ccm_rx.counter = 0U;
			lll->ccm_tx.counter = 0U;

			/* set direction: slave to master = 0,
			 * master to slave = 1
			 */
			lll->ccm_rx.direction = 1U;
			lll->ccm_tx.direction = 0U;

			/* enable receive encryption (transmit turned
			 * on when start enc resp from master is
			 * received)
			 */
			lll->enc_rx = 1U;

			/* prepare the start enc req */
			pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
			pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl,
						    start_enc_req) +
				sizeof(struct pdu_data_llctrl_start_enc_req);
			pdu_ctrl_tx->llctrl.opcode =
				PDU_DATA_LLCTRL_TYPE_START_ENC_REQ;

			ctrl_tx_enqueue(conn, tx);
		}

#if !defined(CONFIG_BT_CTLR_FAST_ENC)
	} else {
		start_enc_rsp_send(conn, pdu_ctrl_tx);

		ctrl_tx_enqueue(conn, tx);

		/* resume data packet rx and tx */
		conn->llcp_enc.pause_rx = 0U;
		conn->llcp_enc.pause_tx = 0U;
#endif /* !CONFIG_BT_CTLR_FAST_ENC */
	}

	/* procedure request acked */
	conn->llcp_ack = conn->llcp_req;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

static inline void event_fex_prep(struct ll_conn *conn)
{
	struct node_tx *tx;

	if (conn->common.fex_valid) {
		struct node_rx_pdu *rx;
		struct pdu_data *pdu;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		/* get a rx node for ULL->LL */
		rx = ll_pdu_rx_alloc();
		if (!rx) {
			return;
		}

		rx->hdr.handle = conn->lll.handle;
		rx->hdr.type = NODE_RX_TYPE_DC_PDU;

		/* prepare feature rsp structure */
		pdu = (void *)rx->pdu;
		pdu->ll_id = PDU_DATA_LLID_CTRL;
		pdu->len = offsetof(struct pdu_data_llctrl, feature_rsp) +
			   sizeof(struct pdu_data_llctrl_feature_rsp);
		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;
		(void)memset(&pdu->llctrl.feature_rsp.features[0], 0x00,
			sizeof(pdu->llctrl.feature_rsp.features));
		pdu->llctrl.feature_req.features[0] =
			conn->llcp_features & 0xFF;
		pdu->llctrl.feature_req.features[1] =
			(conn->llcp_features >> 8) & 0xFF;
		pdu->llctrl.feature_req.features[2] =
			(conn->llcp_features >> 16) & 0xFF;

		/* enqueue feature rsp structure into rx queue */
		ll_rx_put(rx->hdr.link, rx);
		ll_rx_sched();

		return;
	}

	tx = mem_acquire(&mem_conn_tx_ctrl.free);
	if (tx) {
		struct pdu_data *pdu = (void *)tx->pdu;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		/* use initial feature bitmap */
		conn->llcp_features = LL_FEAT;

		/* place the feature exchange req packet as next in tx queue */
		pdu->ll_id = PDU_DATA_LLID_CTRL;
		pdu->len = offsetof(struct pdu_data_llctrl, feature_req) +
			   sizeof(struct pdu_data_llctrl_feature_req);
		pdu->llctrl.opcode = !conn->lll.role ?
				     PDU_DATA_LLCTRL_TYPE_FEATURE_REQ :
				     PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ;
		(void)memset(&pdu->llctrl.feature_req.features[0],
			     0x00,
			     sizeof(pdu->llctrl.feature_req.features));
		pdu->llctrl.feature_req.features[0] =
			conn->llcp_features & 0xFF;
		pdu->llctrl.feature_req.features[1] =
			(conn->llcp_features >> 8) & 0xFF;
		pdu->llctrl.feature_req.features[2] =
			(conn->llcp_features >> 16) & 0xFF;

		ctrl_tx_enqueue(conn, tx);

		/* Start Procedure Timeout (TODO: this shall not replace
		 * terminate procedure)
		 */
		conn->procedure_expire = conn->procedure_reload;
	}

}

static inline void event_vex_prep(struct ll_conn *conn)
{
	if (conn->llcp_version.tx == 0U) {
		struct node_tx *tx;

		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (tx) {
			struct pdu_data *pdu = (void *)tx->pdu;
			u16_t cid;
			u16_t svn;

			/* procedure request acked */
			conn->llcp_ack = conn->llcp_req;

			/* set version ind tx-ed flag */
			conn->llcp_version.tx = 1U;

			/* place the version ind packet as next in tx queue */
			pdu->ll_id = PDU_DATA_LLID_CTRL;
			pdu->len =
				offsetof(struct pdu_data_llctrl, version_ind) +
				sizeof(struct pdu_data_llctrl_version_ind);
			pdu->llctrl.opcode =
				PDU_DATA_LLCTRL_TYPE_VERSION_IND;
			pdu->llctrl.version_ind.version_number =
				LL_VERSION_NUMBER;
			cid = sys_cpu_to_le16(ll_settings_company_id());
			svn = sys_cpu_to_le16(ll_settings_subversion_number());
			pdu->llctrl.version_ind.company_id = cid;
			pdu->llctrl.version_ind.sub_version_number = svn;

			ctrl_tx_enqueue(conn, tx);

			/* Start Procedure Timeout (TODO: this shall not
			 * replace terminate procedure)
			 */
			conn->procedure_expire = conn->procedure_reload;
		}
	} else if (conn->llcp_version.rx) {
		struct node_rx_pdu *rx;
		struct pdu_data *pdu;

		/* get a rx node for ULL->LL */
		rx = ll_pdu_rx_alloc();
		if (!rx) {
			return;
		};

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		rx->hdr.handle = conn->lll.handle;
		rx->hdr.type = NODE_RX_TYPE_DC_PDU;

		/* prepare version ind structure */
		pdu = (void *)rx->pdu;
		pdu->ll_id = PDU_DATA_LLID_CTRL;
		pdu->len = offsetof(struct pdu_data_llctrl, version_ind) +
			   sizeof(struct pdu_data_llctrl_version_ind);
		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
		pdu->llctrl.version_ind.version_number =
			conn->llcp_version.version_number;
		pdu->llctrl.version_ind.company_id =
			sys_cpu_to_le16(conn->llcp_version.company_id);
		pdu->llctrl.version_ind.sub_version_number =
			sys_cpu_to_le16(conn->llcp_version.sub_version_number);

		/* enqueue version ind structure into rx queue */
		ll_rx_put(rx->hdr.link, rx);
		ll_rx_sched();
	} else {
		/* tx-ed but no rx, and new request placed */
		LL_ASSERT(0);
	}
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static inline void event_conn_param_req(struct ll_conn *conn,
					u16_t event_counter,
					u32_t ticks_at_expire)
{
	struct pdu_data_llctrl_conn_param_req *p;
	struct pdu_data *pdu_ctrl_tx;
	struct node_tx *tx;

	tx = mem_acquire(&mem_conn_tx_ctrl.free);
	if (!tx) {
		return;
	}

	/* move to wait for conn_update/rsp/rej */
	conn->llcp_conn_param.state = LLCP_CPR_STATE_RSP_WAIT;

	/* place the conn param req packet as next in tx queue */
	pdu_ctrl_tx = (void *)tx->pdu;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, conn_param_req) +
		sizeof(struct pdu_data_llctrl_conn_param_req);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
	p = (void *)&pdu_ctrl_tx->llctrl.conn_param_req;
	p->interval_min = sys_cpu_to_le16(conn->llcp_conn_param.interval_min);
	p->interval_max = sys_cpu_to_le16(conn->llcp_conn_param.interval_max);
	p->latency = sys_cpu_to_le16(conn->llcp_conn_param.latency);
	p->timeout = sys_cpu_to_le16(conn->llcp_conn_param.timeout);
	p->preferred_periodicity = 0U;
	p->reference_conn_event_count = sys_cpu_to_le16(event_counter);
	p->offset0 = sys_cpu_to_le16(0x0000);
	p->offset1 = sys_cpu_to_le16(0xffff);
	p->offset2 = sys_cpu_to_le16(0xffff);
	p->offset3 = sys_cpu_to_le16(0xffff);
	p->offset4 = sys_cpu_to_le16(0xffff);
	p->offset5 = sys_cpu_to_le16(0xffff);

	ctrl_tx_enqueue(conn, tx);

	/* set CUI/CPR mutex */
	conn_upd_curr = conn;

	/* Start Procedure Timeout (TODO: this shall not replace
	 * terminate procedure).
	 */
	conn->procedure_expire = conn->procedure_reload;

#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
	{
		static memq_link_t s_link;
		static struct mayfly s_mfy_sched_offset = {0, 0, &s_link, NULL,
			ull_sched_mfy_free_win_offset_calc};
		u32_t retval;

		conn->llcp_conn_param.ticks_ref = ticks_at_expire;

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
		if (conn->evt.ticks_xtal_to_start & XON_BITMASK) {
			u32_t ticks_prepare_to_start =
				MAX(conn->evt.ticks_active_to_start,
				    conn->evt.ticks_preempt_to_start);

			conn->llcp_conn_param.ticks_ref -=
				(conn->evt.ticks_xtal_to_start &
				 ~XON_BITMASK) - ticks_prepare_to_start;
		}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

		conn->llcp_conn_param.pdu_win_offset0 = (u16_t *)&p->offset0;

		s_mfy_sched_offset.param = (void *)conn;

		retval = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
					TICKER_USER_ID_ULL_LOW, 1,
					&s_mfy_sched_offset);
		LL_ASSERT(!retval);
	}
#else /* !CONFIG_BT_CTLR_SCHED_ADVANCED */
	ARG_UNUSED(ticks_at_expire);
#endif /* !CONFIG_BT_CTLR_SCHED_ADVANCED */
}

static inline void event_conn_param_rsp(struct ll_conn *conn)
{
	struct pdu_data_llctrl_conn_param_rsp *rsp;
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* handle rejects */
	if (conn->llcp_conn_param.status) {
		struct pdu_data_llctrl_reject_ext_ind *rej;

		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (!tx) {
			return;
		}

		/* master/slave response with reject ext ind */
		pdu = (void *)tx->pdu;
		pdu->ll_id = PDU_DATA_LLID_CTRL;
		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
		pdu->len = offsetof(struct pdu_data_llctrl, reject_ext_ind) +
			   sizeof(struct pdu_data_llctrl_reject_ext_ind);

		rej = (void *)&pdu->llctrl.reject_ext_ind;
		rej->reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
		rej->error_code = conn->llcp_conn_param.status;

		ctrl_tx_enqueue(conn, tx);

		/* procedure request acked */
		conn->llcp_conn_param.ack = conn->llcp_conn_param.req;

		/* reset mutex */
		conn_upd_curr = NULL;

		return;
	}

	/* master respond with connection update */
	if (!conn->lll.role) {
		if (((conn->llcp_req - conn->llcp_ack) & 0x03) == 0x02) {
			return;
		}

		/* Move to waiting for connection update completion */
		conn->llcp_conn_param.state = LLCP_CPR_STATE_UPD;

		/* Initiate connection update procedure */
		conn->llcp.conn_upd.win_size = 1U;
		conn->llcp.conn_upd.win_offset_us = 0U;
		if (conn->llcp_conn_param.preferred_periodicity) {
			conn->llcp.conn_upd.interval =
				((conn->llcp_conn_param.interval_min /
				  conn->llcp_conn_param.preferred_periodicity) +
				 1) *
				conn->llcp_conn_param.preferred_periodicity;
		} else {
			conn->llcp.conn_upd.interval =
				conn->llcp_conn_param.interval_max;
		}
		conn->llcp.conn_upd.latency = conn->llcp_conn_param.latency;
		conn->llcp.conn_upd.timeout = conn->llcp_conn_param.timeout;
		/* conn->llcp.conn_upd.instant     = 0; */
		conn->llcp.conn_upd.state = LLCP_CUI_STATE_SELECT;
		conn->llcp.conn_upd.is_internal = !conn->llcp_conn_param.cmd;
		conn->llcp_type = LLCP_CONN_UPD;
		conn->llcp_ack -= 2U;

		return;
	}

	/* slave response with connection parameter response */
	tx = mem_acquire(&mem_conn_tx_ctrl.free);
	if (!tx) {
		return;
	}

	/* place the conn param rsp packet as next in tx queue */
	pdu = (void *)tx->pdu;
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_param_rsp) +
		sizeof(struct pdu_data_llctrl_conn_param_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP;
	rsp = (void *)&pdu->llctrl.conn_param_rsp;
	rsp->interval_min =
		sys_cpu_to_le16(conn->llcp_conn_param.interval_min);
	rsp->interval_max =
		sys_cpu_to_le16(conn->llcp_conn_param.interval_max);
	rsp->latency =
		sys_cpu_to_le16(conn->llcp_conn_param.latency);
	rsp->timeout =
		sys_cpu_to_le16(conn->llcp_conn_param.timeout);
	rsp->preferred_periodicity =
		conn->llcp_conn_param.preferred_periodicity;
	rsp->reference_conn_event_count =
		sys_cpu_to_le16(conn->llcp_conn_param.reference_conn_event_count);
	rsp->offset0 = sys_cpu_to_le16(conn->llcp_conn_param.offset0);
	rsp->offset1 = sys_cpu_to_le16(conn->llcp_conn_param.offset1);
	rsp->offset2 = sys_cpu_to_le16(conn->llcp_conn_param.offset2);
	rsp->offset3 = sys_cpu_to_le16(conn->llcp_conn_param.offset3);
	rsp->offset4 = sys_cpu_to_le16(conn->llcp_conn_param.offset4);
	rsp->offset5 = sys_cpu_to_le16(conn->llcp_conn_param.offset5);

	ctrl_tx_enqueue(conn, tx);

	/* procedure request acked */
	conn->llcp_conn_param.ack = conn->llcp_conn_param.req;

	/* reset mutex */
	conn_upd_curr = NULL;
}

static inline void event_conn_param_app_req(struct ll_conn *conn)
{
	struct pdu_data_llctrl_conn_param_req *p;
	struct node_rx_pdu *rx;
	struct pdu_data *pdu;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* defer until encryption setup is complete */
	if (conn->llcp_enc.pause_tx) {
		return;
	}
#endif /* CONFIG_BT_CTLR_LE_ENC */

	/* wait for free rx buffer */
	rx = ll_pdu_rx_alloc();
	if (!rx) {
		return;
	}

	/* move to wait for conn_update/rsp/rej */
	conn->llcp_conn_param.state = LLCP_CPR_STATE_APP_WAIT;

	/* Emulate as Rx-ed CPR data channel PDU */
	rx->hdr.handle = conn->lll.handle;
	rx->hdr.type = NODE_RX_TYPE_DC_PDU;

	/* place the conn param req packet as next in rx queue */
	pdu = (void *)rx->pdu;
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_param_req) +
		sizeof(struct pdu_data_llctrl_conn_param_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
	p = (void *) &pdu->llctrl.conn_param_req;
	p->interval_min = sys_cpu_to_le16(conn->llcp_conn_param.interval_min);
	p->interval_max = sys_cpu_to_le16(conn->llcp_conn_param.interval_max);
	p->latency = sys_cpu_to_le16(conn->llcp_conn_param.latency);
	p->timeout = sys_cpu_to_le16(conn->llcp_conn_param.timeout);

	/* enqueue connection parameter request into rx queue */
	ll_rx_put(rx->hdr.link, rx);
	ll_rx_sched();
}

static inline void event_conn_param_prep(struct ll_conn *conn,
					 u16_t event_counter,
					 u32_t ticks_at_expire)
{
	struct ll_conn *conn_upd;

	conn_upd = conn_upd_curr;
	if (conn_upd && (conn_upd != conn)) {
		return;
	}

	switch (conn->llcp_conn_param.state) {
	case LLCP_CPR_STATE_REQ:
		event_conn_param_req(conn, event_counter, ticks_at_expire);
		break;

	case LLCP_CPR_STATE_RSP:
		event_conn_param_rsp(conn);
		break;

	case LLCP_CPR_STATE_APP_REQ:
		event_conn_param_app_req(conn);
		break;

	case LLCP_CPR_STATE_APP_WAIT:
	case LLCP_CPR_STATE_RSP_WAIT:
	case LLCP_CPR_STATE_UPD:
		/* Do nothing */
		break;

	default:
		LL_ASSERT(0);
		break;
	}
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_LE_PING)
static inline void event_ping_prep(struct ll_conn *conn)
{
	struct node_tx *tx;

	tx = mem_acquire(&mem_conn_tx_ctrl.free);
	if (tx) {
		struct pdu_data *pdu_ctrl_tx = (void *)tx->pdu;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		/* place the ping req packet as next in tx queue */
		pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, ping_req) +
				   sizeof(struct pdu_data_llctrl_ping_req);
		pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PING_REQ;

		ctrl_tx_enqueue(conn, tx);

		/* Start Procedure Timeout (TODO: this shall not replace
		 * terminate procedure)
		 */
		conn->procedure_expire = conn->procedure_reload;
	}

}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static inline void event_len_prep(struct ll_conn *conn)
{
	switch (conn->llcp_length.state) {
	case LLCP_LENGTH_STATE_REQ:
	{
		struct pdu_data_llctrl_length_req *lr;
		struct pdu_data *pdu_ctrl_tx;
		struct node_tx *tx;

		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (!tx) {
			return;
		}

		/* wait for resp before completing the procedure */
		conn->llcp_length.state = LLCP_LENGTH_STATE_ACK_WAIT;

		/* set the default tx octets/time to requested value */
		conn->default_tx_octets = conn->llcp_length.tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
		conn->default_tx_time = conn->llcp_length.tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

		/* place the length req packet as next in tx queue */
		pdu_ctrl_tx = (void *) tx->pdu;
		pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_tx->len =
			offsetof(struct pdu_data_llctrl, length_req) +
			sizeof(struct pdu_data_llctrl_length_req);
		pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_REQ;

		lr = &pdu_ctrl_tx->llctrl.length_req;
		lr->max_rx_octets = sys_cpu_to_le16(LL_LENGTH_OCTETS_RX_MAX);
		lr->max_tx_octets = sys_cpu_to_le16(conn->default_tx_octets);
#if !defined(CONFIG_BT_CTLR_PHY)
		lr->max_rx_time =
			sys_cpu_to_le16(PKT_US(LL_LENGTH_OCTETS_RX_MAX, 0));
		lr->max_tx_time =
			sys_cpu_to_le16(PKT_US(conn->default_tx_octets, 0));
#else /* CONFIG_BT_CTLR_PHY */
		lr->max_rx_time =
			sys_cpu_to_le16(PKT_US(LL_LENGTH_OCTETS_RX_MAX,
					       BIT(2)));
		lr->max_tx_time =
			sys_cpu_to_le16(conn->default_tx_time);
#endif /* CONFIG_BT_CTLR_PHY */

		ctrl_tx_enqueue(conn, tx);

		/* Start Procedure Timeout (TODO: this shall not replace
		 * terminate procedure).
		 */
		conn->procedure_expire = conn->procedure_reload;
	}
	break;

	case LLCP_LENGTH_STATE_RESIZE:
	{
		struct pdu_data_llctrl_length_rsp *lr;
		struct pdu_data *pdu_ctrl_rx;
		struct node_rx_pdu *rx;
		struct lll_conn *lll;

		/* Procedure complete */
		conn->llcp_length.ack = conn->llcp_length.req;
		conn->llcp_length.pause_tx = 0U;
		conn->procedure_expire = 0U;

		lll = &conn->lll;

		/* Use the new rx octets/time in the connection */
		lll->max_rx_octets = conn->llcp_length.rx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
		lll->max_rx_time = conn->llcp_length.rx_time;
#endif /* CONFIG_BT_CTLR_PHY */

		/* Prepare the rx packet structure */
		rx = conn->llcp_rx;
		LL_ASSERT(rx && rx->hdr.link);
		conn->llcp_rx = rx->hdr.link->mem;

		rx->hdr.handle = conn->lll.handle;
		rx->hdr.type = NODE_RX_TYPE_DC_PDU;

		/* prepare length rsp structure */
		pdu_ctrl_rx = (void *)rx->pdu;
		pdu_ctrl_rx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_rx->len =
			offsetof(struct pdu_data_llctrl, length_rsp) +
			sizeof(struct pdu_data_llctrl_length_rsp);
		pdu_ctrl_rx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;

		lr = &pdu_ctrl_rx->llctrl.length_rsp;
		lr->max_rx_octets = sys_cpu_to_le16(lll->max_rx_octets);
		lr->max_tx_octets = sys_cpu_to_le16(lll->max_tx_octets);
#if !defined(CONFIG_BT_CTLR_PHY)
		lr->max_rx_time =
			sys_cpu_to_le16(PKT_US(lll->max_rx_octets, 0));
		lr->max_tx_time =
			sys_cpu_to_le16(PKT_US(lll->max_tx_octets, 0));
#else /* CONFIG_BT_CTLR_PHY */
		lr->max_rx_time = sys_cpu_to_le16(lll->max_rx_time);
		lr->max_tx_time = sys_cpu_to_le16(lll->max_tx_time);
#endif /* CONFIG_BT_CTLR_PHY */

		/* enqueue rx node towards Thread */
		ll_rx_put(rx->hdr.link, rx);
		ll_rx_sched();
	}
	break;

	case LLCP_LENGTH_STATE_ACK_WAIT:
	case LLCP_LENGTH_STATE_RSP_WAIT:
		/* no nothing */
		break;

	default:
		LL_ASSERT(0);
		break;
	}
}

static u16_t calc_eff_time(u8_t max_octets, u8_t phy, u16_t default_time)
{
	u16_t time = PKT_US(max_octets, phy);
	u16_t eff_time;

	if (time >= PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, 0)) {
		eff_time = MIN(time, default_time);
#if defined(CONFIG_BT_CTLR_PHY_CODED)
		eff_time = MAX(eff_time, PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, phy));
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	} else {
		eff_time = PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, 0);
	}

	return eff_time;
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static inline void event_phy_req_prep(struct ll_conn *conn)
{
	switch (conn->llcp_phy.state) {
	case LLCP_PHY_STATE_REQ:
	{
		struct pdu_data_llctrl_phy_req *pr;
		struct pdu_data *pdu_ctrl_tx;
		struct node_tx *tx;

		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (!tx) {
			break;
		}

		conn->llcp_phy.state = LLCP_PHY_STATE_ACK_WAIT;

		/* update preferred phy */
		conn->phy_pref_tx = conn->llcp_phy.tx;
		conn->phy_pref_rx = conn->llcp_phy.rx;
		conn->phy_pref_flags = conn->llcp_phy.flags;

		/* pause data packet tx */
		conn->llcp_phy.pause_tx = 1U;

		/* place the phy req packet as next in tx queue */
		pdu_ctrl_tx = (void *)tx->pdu;
		pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_tx->len =
			offsetof(struct pdu_data_llctrl, phy_req) +
			sizeof(struct pdu_data_llctrl_phy_req);
		pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ;

		pr = &pdu_ctrl_tx->llctrl.phy_req;
		pr->tx_phys = conn->llcp_phy.tx;
		pr->rx_phys = conn->llcp_phy.rx;

		ctrl_tx_enqueue(conn, tx);

		/* Start Procedure Timeout (TODO: this shall not replace
		 * terminate procedure).
		 */
		conn->procedure_expire = conn->procedure_reload;
	}
	break;

	case LLCP_PHY_STATE_UPD:
	{
		/* Defer if another procedure in progress */
		if (conn->llcp_ack != conn->llcp_req) {
			return;
		}

		/* Procedure complete */
		conn->llcp_phy.ack = conn->llcp_phy.req;

		/* select only one tx phy, prefer 2M */
		if (conn->llcp_phy.tx & BIT(1)) {
			conn->llcp_phy.tx = BIT(1);
		} else if (conn->llcp_phy.tx & BIT(0)) {
			conn->llcp_phy.tx = BIT(0);
		} else if (conn->llcp_phy.tx & BIT(2)) {
			conn->llcp_phy.tx = BIT(2);
		} else {
			conn->llcp_phy.tx = 0U;
		}

		/* select only one rx phy, prefer 2M */
		if (conn->llcp_phy.rx & BIT(1)) {
			conn->llcp_phy.rx = BIT(1);
		} else if (conn->llcp_phy.rx & BIT(0)) {
			conn->llcp_phy.rx = BIT(0);
		} else if (conn->llcp_phy.rx & BIT(2)) {
			conn->llcp_phy.rx = BIT(2);
		} else {
			conn->llcp_phy.rx = 0U;
		}

		/* Initiate PHY Update Ind */
		if (conn->llcp_phy.tx != conn->lll.phy_tx) {
			conn->llcp.phy_upd_ind.tx = conn->llcp_phy.tx;
		} else {
			conn->llcp.phy_upd_ind.tx = 0U;
		}
		if (conn->llcp_phy.rx != conn->lll.phy_rx) {
			conn->llcp.phy_upd_ind.rx = conn->llcp_phy.rx;
		} else {
			conn->llcp.phy_upd_ind.rx = 0U;
		}
		/* conn->llcp.phy_upd_ind.instant = 0; */
		conn->llcp.phy_upd_ind.initiate = 1U;
		conn->llcp.phy_upd_ind.cmd = conn->llcp_phy.cmd;

		conn->llcp_type = LLCP_PHY_UPD;
		conn->llcp_ack -= 2U;
	}
	break;

	case LLCP_PHY_STATE_ACK_WAIT:
	case LLCP_PHY_STATE_RSP_WAIT:
		/* no nothing */
		break;

	default:
		LL_ASSERT(0);
		break;
	}
}

static inline void event_phy_upd_ind_prep(struct ll_conn *conn,
					  u16_t event_counter)
{
	struct node_rx_pu *upd;

	if (conn->llcp.phy_upd_ind.initiate) {
		struct pdu_data_llctrl_phy_upd_ind *ind;
		struct pdu_data *pdu_ctrl_tx;
		struct node_rx_pdu *rx;
		struct node_tx *tx;

		LL_ASSERT(!conn->llcp_rx);

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		rx = ll_pdu_rx_alloc_peek(2);
#else /* !CONFIG_BT_CTLR_DATA_LENGTH */
		rx = ll_pdu_rx_alloc_peek(1);
#endif /* !CONFIG_BT_CTLR_DATA_LENGTH */
		if (!rx) {
			return;
		}

		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (!tx) {
			return;
		}

		/* reset initiate flag */
		conn->llcp.phy_upd_ind.initiate = 0U;

		/* Check if both tx and rx PHY unchanged */
		if (!((conn->llcp.phy_upd_ind.tx |
		       conn->llcp.phy_upd_ind.rx) & 0x07)) {
			/* Procedure complete */
			conn->llcp_ack = conn->llcp_req;

			/* 0 instant */
			conn->llcp.phy_upd_ind.instant = 0U;

			/* generate phy update event */
			if (conn->llcp.phy_upd_ind.cmd) {
				struct lll_conn *lll = &conn->lll;

				(void)ll_pdu_rx_alloc();

				rx->hdr.handle = lll->handle;
				rx->hdr.type = NODE_RX_TYPE_PHY_UPDATE;

				upd = (void *)rx->pdu;
				upd->status = 0U;
				upd->tx = lll->phy_tx;
				upd->rx = lll->phy_rx;

				/* Enqueue Rx node */
				ll_rx_put(rx->hdr.link, rx);
				ll_rx_sched();
			}
		} else {
			struct lll_conn *lll = &conn->lll;

			/* set instant */
			conn->llcp.phy_upd_ind.instant = event_counter +
							 lll->latency +
							 6;
			/* reserve rx node for event generation at instant */
			(void)ll_pdu_rx_alloc();
			rx->hdr.link->mem = conn->llcp_rx;
			conn->llcp_rx = rx;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
			/* reserve rx node for DLE event generation */
			rx = ll_pdu_rx_alloc();
			rx->hdr.link->mem = conn->llcp_rx;
			conn->llcp_rx = rx;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
		}

		/* place the phy update ind packet as next in
		 * tx queue
		 */
		pdu_ctrl_tx = (void *)tx->pdu;
		pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_tx->len =
			offsetof(struct pdu_data_llctrl, phy_upd_ind) +
			sizeof(struct pdu_data_llctrl_phy_upd_ind);
		pdu_ctrl_tx->llctrl.opcode =
			PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
		ind = &pdu_ctrl_tx->llctrl.phy_upd_ind;
		ind->m_to_s_phy = conn->llcp.phy_upd_ind.tx;
		ind->s_to_m_phy = conn->llcp.phy_upd_ind.rx;
		ind->instant = sys_cpu_to_le16(conn->llcp.phy_upd_ind.instant);

		ctrl_tx_enqueue(conn, tx);
	} else if (((event_counter - conn->llcp.phy_upd_ind.instant) &
		    0xFFFF) <= 0x7FFF) {
		struct lll_conn *lll = &conn->lll;
		struct node_rx_pdu *rx;
		u8_t old_tx, old_rx;

		/* procedure request acked */
		conn->llcp_ack = conn->llcp_req;

		/* apply new phy */
		old_tx = lll->phy_tx;
		old_rx = lll->phy_rx;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		u16_t eff_tx_time = lll->max_tx_time;
		u16_t eff_rx_time = lll->max_rx_time;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

		if (conn->llcp.phy_upd_ind.tx) {
			lll->phy_tx = conn->llcp.phy_upd_ind.tx;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
			eff_tx_time = calc_eff_time(lll->max_tx_octets,
						    lll->phy_tx,
						    conn->default_tx_time);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
		}
		if (conn->llcp.phy_upd_ind.rx) {
			lll->phy_rx = conn->llcp.phy_upd_ind.rx;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
			eff_rx_time =
				calc_eff_time(lll->max_rx_octets, lll->phy_rx,
					      PKT_US(LL_LENGTH_OCTETS_RX_MAX,
						     BIT(2)));
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
		}
		lll->phy_flags = conn->phy_pref_flags;

		/* Acquire Rx node */
		rx = conn->llcp_rx;
		LL_ASSERT(rx && rx->hdr.link);
		conn->llcp_rx = rx->hdr.link->mem;

		/* generate event if phy changed or initiated by cmd */
		if (!conn->llcp.phy_upd_ind.cmd && (lll->phy_tx == old_tx) &&
		    (lll->phy_rx == old_rx)) {
			/* Mark for buffer for release */
			rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

			/* enqueue rx node towards Thread */
			ll_rx_put(rx->hdr.link, rx);

			if (IS_ENABLED(CONFIG_BT_CTLR_DATA_LENGTH)) {
				/* get the DLE rx node reserved for ULL->LL */
				rx = conn->llcp_rx;
				LL_ASSERT(rx && rx->hdr.link);
				conn->llcp_rx = rx->hdr.link->mem;

				/* Mark for buffer for release */
				rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

				/* enqueue rx node towards Thread */
				ll_rx_put(rx->hdr.link, rx);
			}

			ll_rx_sched();

			return;
		}

		rx->hdr.handle = lll->handle;
		rx->hdr.type = NODE_RX_TYPE_PHY_UPDATE;

		upd = (void *)rx->pdu;
		upd->status = 0U;
		upd->tx = lll->phy_tx;
		upd->rx = lll->phy_rx;

		/* enqueue rx node towards Thread */
		ll_rx_put(rx->hdr.link, rx);

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		/* get a rx node for ULL->LL */
		rx = conn->llcp_rx;
		LL_ASSERT(rx && rx->hdr.link);
		conn->llcp_rx = rx->hdr.link->mem;

		/* Update max tx and/or max rx if changed */
		if ((eff_tx_time <= lll->max_tx_time) &&
		    (eff_rx_time <= lll->max_rx_time)) {
			/* Mark buffer for release */
			rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

			/* enqueue rx node towards Thread */
			ll_rx_put(rx->hdr.link, rx);
			ll_rx_sched();
			return;
		}
		lll->max_tx_time = eff_tx_time;
		lll->max_rx_time = eff_rx_time;

		/* prepare length rsp structure */
		rx->hdr.handle = lll->handle;
		rx->hdr.type = NODE_RX_TYPE_DC_PDU;

		struct pdu_data *pdu_rx = (void *)rx->pdu;

		pdu_rx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_rx->len = offsetof(struct pdu_data_llctrl, length_rsp) +
			      sizeof(struct pdu_data_llctrl_length_rsp);
		pdu_rx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;

		struct pdu_data_llctrl_length_req *lr =
			(void *)&pdu_rx->llctrl.length_rsp;

		lr->max_rx_octets = sys_cpu_to_le16(lll->max_rx_octets);
		lr->max_tx_octets = sys_cpu_to_le16(lll->max_tx_octets);
		lr->max_rx_time = sys_cpu_to_le16(lll->max_rx_time);
		lr->max_tx_time = sys_cpu_to_le16(lll->max_tx_time);

		/* enqueue rx node towards Thread */
		ll_rx_put(rx->hdr.link, rx);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

		ll_rx_sched();
	}
}
#endif /* CONFIG_BT_CTLR_PHY */

static u8_t conn_upd_recv(struct ll_conn *conn, memq_link_t *link,
			  struct node_rx_pdu **rx, struct pdu_data *pdu)
{
	u16_t instant;

	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);
	if (((instant - conn->lll.event_counter) & 0xFFFF) > 0x7FFF) {
		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		return BT_HCI_ERR_INSTANT_PASSED;
	}

	/* different transaction collision */
	if (((conn->llcp_req - conn->llcp_ack) & 0x03) == 0x02) {
		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		return BT_HCI_ERR_DIFF_TRANS_COLLISION;
	}

	/* set mutex, if only not already set. As a master the mutex shall
	 * be set, but a slave we accept it as new 'set' of mutex.
	 */
	if (!conn_upd_curr) {
		LL_ASSERT(conn->lll.role);

		conn_upd_curr = conn;
	}

	conn->llcp.conn_upd.win_size = pdu->llctrl.conn_update_ind.win_size;
	conn->llcp.conn_upd.win_offset_us =
		sys_le16_to_cpu(pdu->llctrl.conn_update_ind.win_offset) * 1250;
	conn->llcp.conn_upd.interval =
		sys_le16_to_cpu(pdu->llctrl.conn_update_ind.interval);
	conn->llcp.conn_upd.latency =
		sys_le16_to_cpu(pdu->llctrl.conn_update_ind.latency);
	conn->llcp.conn_upd.timeout =
		sys_le16_to_cpu(pdu->llctrl.conn_update_ind.timeout);
	conn->llcp.conn_upd.instant = instant;
	conn->llcp.conn_upd.state = LLCP_CUI_STATE_INPROG;
	conn->llcp.conn_upd.is_internal = 0U;

	link->mem = conn->llcp_rx;
	(*rx)->hdr.link = link;
	conn->llcp_rx = *rx;
	*rx = NULL;

	conn->llcp_type = LLCP_CONN_UPD;
	conn->llcp_ack -= 2U;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	if ((conn->llcp_conn_param.req != conn->llcp_conn_param.ack) &&
	    (conn->llcp_conn_param.state == LLCP_CPR_STATE_RSP_WAIT)) {
		conn->llcp_conn_param.ack = conn->llcp_conn_param.req;
	}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

	return 0;
}

static u8_t chan_map_upd_recv(struct ll_conn *conn, struct node_rx_pdu *rx,
			      struct pdu_data *pdu)
{
	u8_t err = 0U;
	u16_t instant;

	instant = sys_le16_to_cpu(pdu->llctrl.chan_map_ind.instant);
	if (((instant - conn->lll.event_counter) & 0xffff) > 0x7fff) {
		err = BT_HCI_ERR_INSTANT_PASSED;

		goto chan_map_upd_recv_exit;
	}

	/* different transaction collision */
	if (((conn->llcp_req - conn->llcp_ack) & 0x03) == 0x02) {
		err = BT_HCI_ERR_DIFF_TRANS_COLLISION;

		goto chan_map_upd_recv_exit;
	}


	memcpy(&conn->llcp.chan_map.chm[0], &pdu->llctrl.chan_map_ind.chm[0],
	       sizeof(conn->llcp.chan_map.chm));
	conn->llcp.chan_map.instant = instant;
	conn->llcp.chan_map.initiate = 0U;

	conn->llcp_type = LLCP_CHAN_MAP;
	conn->llcp_ack -= 2U;

chan_map_upd_recv_exit:
	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

	return err;
}

static void terminate_ind_recv(struct ll_conn *conn, struct node_rx_pdu *rx,
			      struct pdu_data *pdu)
{
	/* Ack and then terminate */
	conn->llcp_terminate.reason_peer = pdu->llctrl.terminate_ind.error_code;

	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
static void enc_req_reused_send(struct ll_conn *conn, struct node_tx **tx)
{
	struct pdu_data *pdu_ctrl_tx;

	pdu_ctrl_tx = (void *)(*tx)->pdu;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, enc_req) +
			   sizeof(struct pdu_data_llctrl_enc_req);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ;
	memcpy(&pdu_ctrl_tx->llctrl.enc_req.rand[0], &conn->llcp_enc.rand[0],
	       sizeof(pdu_ctrl_tx->llctrl.enc_req.rand));
	pdu_ctrl_tx->llctrl.enc_req.ediv[0] = conn->llcp_enc.ediv[0];
	pdu_ctrl_tx->llctrl.enc_req.ediv[1] = conn->llcp_enc.ediv[1];

	/*
	 * Take advantage of the fact that ivm and skdm fields, which both have
	 * to be filled with random data, are adjacent and use single call to
	 * the entropy driver.
	 */
	BUILD_ASSERT(offsetof(__typeof(pdu_ctrl_tx->llctrl.enc_req), ivm) ==
		     (offsetof(__typeof(pdu_ctrl_tx->llctrl.enc_req), skdm) +
		     sizeof(pdu_ctrl_tx->llctrl.enc_req.skdm)));

	/* NOTE: if not sufficient random numbers, ignore waiting */
	entropy_get_entropy_isr(entropy, pdu_ctrl_tx->llctrl.enc_req.skdm,
				sizeof(pdu_ctrl_tx->llctrl.enc_req.skdm) +
				sizeof(pdu_ctrl_tx->llctrl.enc_req.ivm), 0);

	ctrl_tx_enqueue(conn, *tx);

	/* dont release ctrl PDU memory */
	*tx = NULL;
}

static int enc_rsp_send(struct ll_conn *conn)
{
	struct pdu_data *pdu_ctrl_tx;
	struct node_tx *tx;

	/* acquire tx mem */
	tx = mem_acquire(&mem_conn_tx_ctrl.free);
	if (!tx) {
		return -ENOBUFS;
	}

	pdu_ctrl_tx = (void *)tx->pdu;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, enc_rsp) +
			   sizeof(struct pdu_data_llctrl_enc_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_RSP;

	/*
	 * Take advantage of the fact that ivs and skds fields, which both have
	 * to be filled with random data, are adjacent and use single call to
	 * the entropy driver.
	 */
	BUILD_ASSERT(offsetof(__typeof(pdu_ctrl_tx->llctrl.enc_rsp), ivs) ==
		     (offsetof(__typeof(pdu_ctrl_tx->llctrl.enc_rsp), skds) +
		     sizeof(pdu_ctrl_tx->llctrl.enc_rsp.skds)));

	/* NOTE: if not sufficient random numbers, ignore waiting */
	entropy_get_entropy_isr(entropy, pdu_ctrl_tx->llctrl.enc_rsp.skds,
				sizeof(pdu_ctrl_tx->llctrl.enc_rsp.skds) +
				sizeof(pdu_ctrl_tx->llctrl.enc_rsp.ivs), 0);

	/* things from slave stored for session key calculation */
	memcpy(&conn->llcp.encryption.skd[8],
	       &pdu_ctrl_tx->llctrl.enc_rsp.skds[0], 8);
	memcpy(&conn->lll.ccm_rx.iv[4],
	       &pdu_ctrl_tx->llctrl.enc_rsp.ivs[0], 4);

	ctrl_tx_enqueue(conn, tx);

	return 0;
}

static int start_enc_rsp_send(struct ll_conn *conn,
			      struct pdu_data *pdu_ctrl_tx)
{
	struct node_tx *tx = NULL;

	if (!pdu_ctrl_tx) {
		/* acquire tx mem */
		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (!tx) {
			return -ENOBUFS;
		}

		pdu_ctrl_tx = (void *)tx->pdu;
	}

	/* enable transmit encryption */
	conn->lll.enc_tx = 1;

	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, enc_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;

	if (tx) {
		ctrl_tx_enqueue(conn, tx);
	}

	return 0;
}

static inline bool ctrl_is_unexpected(struct ll_conn *conn, u8_t opcode)
{
	return (!conn->lll.role &&
		((!conn->llcp_enc.refresh &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_TERMINATE_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_REQ) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND)) ||
		 (conn->llcp_enc.refresh &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_TERMINATE_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_REQ) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND)))) ||
	       (conn->lll.role &&
		((!conn->llcp_enc.refresh &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_TERMINATE_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND)) ||
		 (conn->llcp_enc.refresh &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_TERMINATE_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_ENC_REQ) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_START_ENC_RSP) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_IND) &&
		  (opcode != PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND))));
}

#endif /* CONFIG_BT_CTLR_LE_ENC */

static int unknown_rsp_send(struct ll_conn *conn, struct node_rx_pdu *rx,
			    u8_t type)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* acquire ctrl tx mem */
	tx = mem_acquire(&mem_conn_tx_ctrl.free);
	if (!tx) {
		return -ENOBUFS;
	}

	pdu = (void *)tx->pdu;
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, unknown_rsp) +
			   sizeof(struct pdu_data_llctrl_unknown_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP;
	pdu->llctrl.unknown_rsp.type = type;

	ctrl_tx_enqueue(conn, tx);

	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

	return 0;
}

static inline u32_t feat_get(u8_t *features)
{
	u32_t feat;

	feat = ~LL_FEAT_BIT_MASK_VALID | features[0] |
	       (features[1] << 8) | (features[2] << 16);
	feat &= LL_FEAT_BIT_MASK;

	return feat;
}

static int feature_rsp_send(struct ll_conn *conn, struct node_rx_pdu *rx,
			    struct pdu_data *pdu_rx)
{
	struct pdu_data_llctrl_feature_req *req;
	struct node_tx *tx;
	struct pdu_data *pdu_tx;

	/* acquire tx mem */
	tx = mem_acquire(&mem_conn_tx_ctrl.free);
	if (!tx) {
		return -ENOBUFS;
	}

	/* AND the feature set to get Feature USED */
	req = &pdu_rx->llctrl.feature_req;
	conn->llcp_features &= feat_get(&req->features[0]);

	/* features exchanged */
	conn->common.fex_valid = 1U;

	/* Enqueue feature response */
	pdu_tx = (void *)tx->pdu;
	pdu_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_tx->len = offsetof(struct pdu_data_llctrl, feature_rsp) +
		      sizeof(struct pdu_data_llctrl_feature_rsp);
	pdu_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;
	(void)memset(&pdu_tx->llctrl.feature_rsp.features[0], 0x00,
		     sizeof(pdu_tx->llctrl.feature_rsp.features));
	pdu_tx->llctrl.feature_req.features[0] =
		conn->llcp_features & 0xFF;
	pdu_tx->llctrl.feature_req.features[1] =
		(conn->llcp_features >> 8) & 0xFF;
	pdu_tx->llctrl.feature_req.features[2] =
		(conn->llcp_features >> 16) & 0xFF;

	ctrl_tx_sec_enqueue(conn, tx);

	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

	return 0;
}

static void feature_rsp_recv(struct ll_conn *conn, struct pdu_data *pdu_rx)
{
	struct pdu_data_llctrl_feature_rsp *rsp;

	rsp = &pdu_rx->llctrl.feature_rsp;

	/* AND the feature set to get Feature USED */
	conn->llcp_features &= feat_get(&rsp->features[0]);

	/* features exchanged */
	conn->common.fex_valid = 1U;

	/* Procedure complete */
	conn->procedure_expire = 0U;
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
static int pause_enc_rsp_send(struct ll_conn *conn, struct node_rx_pdu *rx,
			      u8_t req)
{
	struct pdu_data *pdu_ctrl_tx;
	struct node_tx *tx;

	if (req) {
		/* acquire tx mem */
		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (!tx) {
			return -ENOBUFS;
		}

		/* key refresh */
		conn->llcp_enc.refresh = 1U;
	} else if (!conn->lll.role) {
		/* acquire tx mem */
		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (!tx) {
			return -ENOBUFS;
		}

		/* disable transmit encryption */
		conn->lll.enc_tx = 0;
	} else {
		/* disable transmit encryption */
		conn->lll.enc_tx = 0;

		goto pause_enc_rsp_send_exit;
	}

	/* pause data packet rx */
	conn->llcp_enc.pause_rx = 1U;

	/* disable receive encryption */
	conn->lll.enc_rx = 0;

	/* Enqueue pause enc rsp */
	pdu_ctrl_tx = (void *)tx->pdu;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, enc_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP;

	ctrl_tx_enqueue(conn, tx);

pause_enc_rsp_send_exit:
	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

static int version_ind_send(struct ll_conn *conn, struct node_rx_pdu *rx,
			    struct pdu_data *pdu_rx)
{
	struct pdu_data_llctrl_version_ind *v;
	struct pdu_data *pdu_tx;
	struct node_tx *tx;

	if (!conn->llcp_version.tx) {
		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (!tx) {
			return -ENOBUFS;
		}
		conn->llcp_version.tx = 1U;

		pdu_tx = (void *)tx->pdu;
		pdu_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_tx->len =
			offsetof(struct pdu_data_llctrl, version_ind) +
			sizeof(struct pdu_data_llctrl_version_ind);
		pdu_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
		v = &pdu_tx->llctrl.version_ind;
		v->version_number = LL_VERSION_NUMBER;
		v->company_id =	sys_cpu_to_le16(ll_settings_company_id());
		v->sub_version_number =
			sys_cpu_to_le16(ll_settings_subversion_number());

		ctrl_tx_sec_enqueue(conn, tx);

		/* Mark for buffer for release */
		rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
	} else if (!conn->llcp_version.rx) {
		/* Procedure complete */
		conn->procedure_expire = 0U;
	} else {
		/* Tx-ed and Rx-ed before, ignore this invalid Rx. */

		/* Mark for buffer for release */
		rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		return 0;
	}

	v = &pdu_rx->llctrl.version_ind;
	conn->llcp_version.version_number = v->version_number;
	conn->llcp_version.company_id = sys_le16_to_cpu(v->company_id);
	conn->llcp_version.sub_version_number =
		sys_le16_to_cpu(v->sub_version_number);
	conn->llcp_version.rx = 1U;

	return 0;
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ) || defined(CONFIG_BT_CTLR_PHY)
static int reject_ext_ind_send(struct ll_conn *conn, struct node_rx_pdu *rx,
			       u8_t reject_opcode, u8_t error_code)
{
	struct pdu_data *pdu_ctrl_tx;
	struct node_tx *tx;

	/* acquire tx mem */
	tx = mem_acquire(&mem_conn_tx_ctrl.free);
	if (!tx) {
		return -ENOBUFS;
	}

	pdu_ctrl_tx = (void *)tx->pdu;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, reject_ext_ind) +
		sizeof(struct pdu_data_llctrl_reject_ext_ind);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
	pdu_ctrl_tx->llctrl.reject_ext_ind.reject_opcode = reject_opcode;
	pdu_ctrl_tx->llctrl.reject_ext_ind.error_code = error_code;

	ctrl_tx_enqueue(conn, tx);

	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

	return 0;
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ  || PHY */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static inline void reject_ind_conn_upd_recv(struct ll_conn *conn,
					    struct node_rx_pdu *rx,
					    struct pdu_data *pdu_rx)
{
	struct pdu_data_llctrl_reject_ext_ind *rej_ext_ind;
	struct node_rx_cu *cu;
	struct lll_conn *lll;

	rej_ext_ind = (void *)&pdu_rx->llctrl.reject_ext_ind;
	if (rej_ext_ind->reject_opcode != PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ) {
		goto reject_ind_conn_upd_recv_exit;
	}

	/* Unsupported remote feature */
	lll = &conn->lll;
	if (!lll->role && (rej_ext_ind->error_code ==
			   BT_HCI_ERR_UNSUPP_REMOTE_FEATURE)) {
		LL_ASSERT(conn->llcp_req == conn->llcp_ack);

		conn->llcp_conn_param.state = LLCP_CPR_STATE_UPD;

		conn->llcp.conn_upd.win_size = 1U;
		conn->llcp.conn_upd.win_offset_us = 0U;
		conn->llcp.conn_upd.interval =
			conn->llcp_conn_param.interval_max;
		conn->llcp.conn_upd.latency = conn->llcp_conn_param.latency;
		conn->llcp.conn_upd.timeout = conn->llcp_conn_param.timeout;
		/* conn->llcp.conn_upd.instant     = 0; */
		conn->llcp.conn_upd.state = LLCP_CUI_STATE_USE;
		conn->llcp.conn_upd.is_internal = !conn->llcp_conn_param.cmd;
		conn->llcp_type = LLCP_CONN_UPD;
		conn->llcp_ack -= 2U;

		goto reject_ind_conn_upd_recv_exit;
	}
	/* FIXME: handle unsupported LL parameters error */
	else if (rej_ext_ind->error_code != BT_HCI_ERR_LL_PROC_COLLISION) {
		/* update to next ticks offset */
		if (lll->role) {
			conn->slave.ticks_to_offset =
			    conn->llcp_conn_param.ticks_to_offset_next;
		}
	}

	if (conn->llcp_conn_param.state == LLCP_CPR_STATE_RSP_WAIT) {
		LL_ASSERT(conn_upd_curr == conn);

		/* reset mutex */
		conn_upd_curr = NULL;

		/* Procedure complete */
		conn->llcp_conn_param.ack = conn->llcp_conn_param.req;

		/* Stop procedure timeout */
		conn->procedure_expire = 0U;
	}

	/* skip event generation if not cmd initiated */
	if (!conn->llcp_conn_param.cmd) {
		goto reject_ind_conn_upd_recv_exit;
	}

	/* generate conn update complete event with error code */
	rx->hdr.type = NODE_RX_TYPE_CONN_UPDATE;

	/* prepare connection update complete structure */
	cu = (void *)pdu_rx;
	cu->status = rej_ext_ind->error_code;
	cu->interval = lll->interval;
	cu->latency = lll->latency;
	cu->timeout = conn->supervision_reload *
		      lll->interval * 125U / 1000;

	return;

reject_ind_conn_upd_recv_exit:
	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static inline void reject_ind_dle_recv(struct ll_conn *conn,
				       struct node_rx_pdu *rx,
				       struct pdu_data *pdu_rx)
{
	struct pdu_data_llctrl_reject_ext_ind *rej_ext_ind;

	rej_ext_ind = (void *)&pdu_rx->llctrl.reject_ext_ind;
	if (rej_ext_ind->reject_opcode == PDU_DATA_LLCTRL_TYPE_LENGTH_REQ) {
		struct pdu_data_llctrl_length_req *lr;

		/* Procedure complete */
		conn->llcp_length.ack = conn->llcp_length.req;
		conn->llcp_length.pause_tx = 0U;
		conn->procedure_expire = 0U;

		/* prepare length rsp structure */
		pdu_rx->len = offsetof(struct pdu_data_llctrl, length_rsp) +
			      sizeof(struct pdu_data_llctrl_length_rsp);
		pdu_rx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;

		lr = (void *)&pdu_rx->llctrl.length_req;
		lr->max_rx_octets = sys_cpu_to_le16(conn->lll.max_rx_octets);
		lr->max_tx_octets = sys_cpu_to_le16(conn->lll.max_tx_octets);
#if !defined(CONFIG_BT_CTLR_PHY)
		lr->max_rx_time =
			sys_cpu_to_le16(PKT_US(conn->lll.max_rx_octets, 0));
		lr->max_tx_time =
			sys_cpu_to_le16(PKT_US(conn->lll.max_tx_octets, 0));
#else /* CONFIG_BT_CTLR_PHY */
		lr->max_rx_time = sys_cpu_to_le16(conn->lll.max_rx_time);
		lr->max_tx_time = sys_cpu_to_le16(conn->lll.max_tx_time);
#endif /* CONFIG_BT_CTLR_PHY */

		return;
	}

	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static inline void reject_ind_phy_upd_recv(struct ll_conn *conn,
					   struct node_rx_pdu *rx,
					   struct pdu_data *pdu_rx)
{
	struct pdu_data_llctrl_reject_ext_ind *rej_ext_ind;

	rej_ext_ind = (void *)&pdu_rx->llctrl.reject_ext_ind;
	if (rej_ext_ind->reject_opcode == PDU_DATA_LLCTRL_TYPE_PHY_REQ) {
		struct node_rx_pu *p;

		/* Same Procedure or Different Procedure Collision */

		/* If not same procedure, stop procedure timeout, else
		 * continue timer until phy upd ind is received.
		 */
		if (rej_ext_ind->error_code != BT_HCI_ERR_LL_PROC_COLLISION) {
			/* Procedure complete */
			conn->llcp_phy.ack = conn->llcp_phy.req;
			conn->llcp_phy.pause_tx = 0U;

			/* Reset packet timing restrictions */
			conn->lll.phy_tx_time = conn->lll.phy_tx;

			/* Stop procedure timeout */
			conn->procedure_expire = 0U;
		}

		/* skip event generation if not cmd initiated */
		if (!conn->llcp_phy.cmd) {
			goto reject_ind_phy_upd_recv_exit;
		}

		/* generate phy update complete event with error code */
		rx->hdr.type = NODE_RX_TYPE_PHY_UPDATE;

		p = (void *)pdu_rx;
		p->status = rej_ext_ind->error_code;
		p->tx = conn->lll.phy_tx;
		p->rx = conn->lll.phy_rx;

		return;
	}

reject_ind_phy_upd_recv_exit:
	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
}
#endif /* CONFIG_BT_CTLR_PHY */

static void reject_ext_ind_recv(struct ll_conn *conn, struct node_rx_pdu *rx,
				struct pdu_data *pdu_rx)
{
	if (0) {
#if defined(CONFIG_BT_CTLR_PHY)
	} else if (conn->llcp_phy.ack != conn->llcp_phy.req) {
		reject_ind_phy_upd_recv(conn, rx, pdu_rx);
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	} else if (conn->llcp_conn_param.ack != conn->llcp_conn_param.req) {
		reject_ind_conn_upd_recv(conn, rx, pdu_rx);
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	} else if (conn->llcp_length.ack != conn->llcp_length.req) {
		reject_ind_dle_recv(conn, rx, pdu_rx);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_LE_ENC)
	} else {
		struct pdu_data_llctrl_reject_ext_ind *rej_ext_ind;

		rej_ext_ind = (void *)&pdu_rx->llctrl.reject_ext_ind;

		switch (rej_ext_ind->reject_opcode) {
		case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
			/* resume data packet rx and tx */
			conn->llcp_enc.pause_rx = 0U;
			conn->llcp_enc.pause_tx = 0U;

			/* Procedure complete */
			conn->llcp_ack = conn->llcp_req;
			conn->procedure_expire = 0U;

			/* enqueue as if it were a reject ind */
			pdu_rx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_IND;
			pdu_rx->llctrl.reject_ind.error_code =
				rej_ext_ind->error_code;
			break;

		default:
			/* Ignore */

			/* Mark for buffer for release */
			rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
			break;
		}
#endif /* CONFIG_BT_CTLR_LE_ENC */
	}
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
#if !defined(CONFIG_BT_CTLR_PHY)
static void length_resp_send(struct ll_conn *conn, struct node_tx *tx,
			     u16_t eff_rx_octets, u16_t eff_tx_octets)
#else /* CONFIG_BT_CTLR_PHY */
static void length_resp_send(struct ll_conn *conn, struct node_tx *tx,
			     u16_t eff_rx_octets, u16_t eff_rx_time,
			     u16_t eff_tx_octets, u16_t eff_tx_time)
#endif /* CONFIG_BT_CTLR_PHY */
{
	struct pdu_data *pdu_tx;

	pdu_tx = (void *)tx->pdu;
	pdu_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_tx->len = offsetof(struct pdu_data_llctrl, length_rsp) +
		sizeof(struct pdu_data_llctrl_length_rsp);
	pdu_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;
	pdu_tx->llctrl.length_rsp.max_rx_octets =
		sys_cpu_to_le16(eff_rx_octets);
	pdu_tx->llctrl.length_rsp.max_tx_octets =
		sys_cpu_to_le16(eff_tx_octets);

#if !defined(CONFIG_BT_CTLR_PHY)
	pdu_tx->llctrl.length_rsp.max_rx_time =
		sys_cpu_to_le16(PKT_US(eff_rx_octets, 0));
	pdu_tx->llctrl.length_rsp.max_tx_time =
		sys_cpu_to_le16(PKT_US(eff_tx_octets, 0));
#else /* CONFIG_BT_CTLR_PHY */
	pdu_tx->llctrl.length_rsp.max_rx_time = sys_cpu_to_le16(eff_rx_time);
	pdu_tx->llctrl.length_rsp.max_tx_time = sys_cpu_to_le16(eff_tx_time);
#endif /* CONFIG_BT_CTLR_PHY */

	ctrl_tx_enqueue(conn, tx);
}

static inline int length_req_rsp_recv(struct ll_conn *conn, memq_link_t *link,
				      struct node_rx_pdu **rx,
				      struct pdu_data *pdu_rx)
{
	struct node_tx *tx = NULL;
	u16_t eff_rx_octets;
	u16_t eff_tx_octets;
#if defined(CONFIG_BT_CTLR_PHY)
	u16_t eff_rx_time;
	u16_t eff_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

	/* Check for free ctrl tx PDU */
	if (pdu_rx->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_LENGTH_REQ) {
		tx = mem_acquire(&mem_conn_tx_ctrl.free);
		if (!tx) {
			return -ENOBUFS;
		}
	}

	eff_rx_octets = conn->lll.max_rx_octets;
	eff_tx_octets = conn->lll.max_tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
	eff_rx_time = conn->lll.max_rx_time;
	eff_tx_time = conn->lll.max_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

	if (/* Local idle, and Peer request then complete the Peer procedure
	     * with response.
	     */
	    ((conn->llcp_length.req == conn->llcp_length.ack) &&
	     (pdu_rx->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_LENGTH_REQ)) ||
	    /* or Local has active... */
	    ((conn->llcp_length.req != conn->llcp_length.ack) &&
	     /* with Local requested and Peer request then complete the
	      * Peer procedure with response.
	      */
	     ((((conn->llcp_length.state == LLCP_LENGTH_STATE_REQ) ||
		(conn->llcp_length.state == LLCP_LENGTH_STATE_ACK_WAIT)) &&
	       (pdu_rx->llctrl.opcode ==
		PDU_DATA_LLCTRL_TYPE_LENGTH_REQ)) ||
	      /* with Local waiting for response, and Peer response then
	       * complete the Local procedure or Peer request then complete the
	       * Peer procedure with response.
	       */
	      ((conn->llcp_length.state == LLCP_LENGTH_STATE_RSP_WAIT) &&
	       ((pdu_rx->llctrl.opcode ==
		 PDU_DATA_LLCTRL_TYPE_LENGTH_RSP) ||
		(pdu_rx->llctrl.opcode ==
		 PDU_DATA_LLCTRL_TYPE_LENGTH_REQ)))))) {
		struct pdu_data_llctrl_length_req *lr;
		u16_t max_rx_octets;
		u16_t max_tx_octets;
		u16_t max_rx_time;
		u16_t max_tx_time;

		lr = &pdu_rx->llctrl.length_req;

		/* use the minimal of our default_tx_octets and
		 * peer max_rx_octets
		 */
		max_rx_octets = sys_le16_to_cpu(lr->max_rx_octets);
		if (max_rx_octets >= PDU_DC_PAYLOAD_SIZE_MIN) {
			eff_tx_octets = MIN(max_rx_octets,
					    conn->default_tx_octets);
		}

		/* use the minimal of our max supported and
		 * peer max_tx_octets
		 */
		max_tx_octets = sys_le16_to_cpu(lr->max_tx_octets);
		if (max_tx_octets >= PDU_DC_PAYLOAD_SIZE_MIN) {
			eff_rx_octets = MIN(max_tx_octets,
					    LL_LENGTH_OCTETS_RX_MAX);
		}

#if defined(CONFIG_BT_CTLR_PHY)
		/* use the minimal of our default_tx_time and
		 * peer max_rx_time
		 */
		max_rx_time = sys_le16_to_cpu(lr->max_rx_time);
		if (max_rx_time >= PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, 0)) {
			eff_tx_time = MIN(max_rx_time,
					  conn->default_tx_time);
#if defined(CONFIG_BT_CTLR_PHY_CODED)
			eff_tx_time = MAX(eff_tx_time,
					  PKT_US(PDU_DC_PAYLOAD_SIZE_MIN,
						 conn->lll.phy_tx));
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		}

		/* use the minimal of our max supported and
		 * peer max_tx_time
		 */
		max_tx_time = sys_le16_to_cpu(lr->max_tx_time);
		if (max_tx_time >= PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, 0)) {
#if defined(CONFIG_BT_CTLR_PHY_CODED)
			eff_rx_time = MIN(max_tx_time,
					  PKT_US(LL_LENGTH_OCTETS_RX_MAX,
						 BIT(2)));
			eff_rx_time = MAX(eff_rx_time,
					  PKT_US(PDU_DC_PAYLOAD_SIZE_MIN,
						 conn->lll.phy_rx));
#else /* !CONFIG_BT_CTLR_PHY_CODED */
			eff_rx_time = MIN(max_tx_time,
					  PKT_US(LL_LENGTH_OCTETS_RX_MAX, 0));
#endif /* !CONFIG_BT_CTLR_PHY_CODED */
		}
#endif /* CONFIG_BT_CTLR_PHY */

		/* check if change in rx octets */
		if (eff_rx_octets != conn->lll.max_rx_octets) {
			/* FIXME: If we want to resize Rx Pool, decide to
			 *        nack as required when implementing. Also,
			 *        closing the current event may be needed.
			 */

			/* accept the effective tx */
			conn->lll.max_tx_octets = eff_tx_octets;

			/* trigger or retain the ctrl procedure so as
			 * to resize the rx buffers.
			 */
			conn->llcp_length.rx_octets = eff_rx_octets;
			conn->llcp_length.tx_octets = eff_tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
			/* accept the effective tx time */
			conn->lll.max_tx_time = eff_tx_time;

			conn->llcp_length.rx_time = eff_rx_time;
			conn->llcp_length.tx_time = eff_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

			conn->llcp_length.ack = (conn->llcp_length.req -
						 1);
			conn->llcp_length.state =
				LLCP_LENGTH_STATE_RESIZE;

			link->mem = conn->llcp_rx;
			(*rx)->hdr.link = link;
			conn->llcp_rx = *rx;
			*rx = NULL;
		} else {
			/* Procedure complete */
			conn->llcp_length.ack = conn->llcp_length.req;
			conn->llcp_length.pause_tx = 0U;
			conn->procedure_expire = 0U;

			/* No change in effective octets or time */
			if (eff_tx_octets == conn->lll.max_tx_octets &&
#if defined(CONFIG_BT_CTLR_PHY)
			    eff_tx_time == conn->lll.max_tx_time &&
			    eff_rx_time == conn->lll.max_rx_time &&
#endif /* CONFIG_BT_CTLR_PHY */
			    (1)) {
				/* Mark for buffer for release */
				(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

				goto send_length_resp;
			}

			/* accept the effective tx */
			conn->lll.max_tx_octets = eff_tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
			/* accept the effective rx time */
			conn->lll.max_rx_time = eff_rx_time;

			/* accept the effective tx time */
			conn->lll.max_tx_time = eff_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

			/* prepare event params */
			lr->max_rx_octets = sys_cpu_to_le16(eff_rx_octets);
			lr->max_tx_octets = sys_cpu_to_le16(eff_tx_octets);

#if !defined(CONFIG_BT_CTLR_PHY)
			lr->max_rx_time =
				sys_cpu_to_le16(PKT_US(eff_rx_octets, 0));
			lr->max_tx_time =
				sys_cpu_to_le16(PKT_US(eff_tx_octets, 0));
#else /* CONFIG_BT_CTLR_PHY */
			lr->max_rx_time = sys_cpu_to_le16(eff_rx_time);
			lr->max_tx_time = sys_cpu_to_le16(eff_tx_time);
#endif /* CONFIG_BT_CTLR_PHY */
		}
	} else {
		/* Drop response with no Local initiated request. */
		LL_ASSERT(pdu_rx->llctrl.opcode ==
			  PDU_DATA_LLCTRL_TYPE_LENGTH_RSP);
	}

send_length_resp:
	if (tx) {
		/* FIXME: if nack-ing is implemented then release tx instead
		 *        of sending resp.
		 */
#if !defined(CONFIG_BT_CTLR_PHY)
		length_resp_send(conn, tx, eff_rx_octets,
				 eff_tx_octets);
#else /* CONFIG_BT_CTLR_PHY */
		length_resp_send(conn, tx, eff_rx_octets,
				 eff_rx_time, eff_tx_octets,
				 eff_tx_time);
#endif /* CONFIG_BT_CTLR_PHY */
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_LE_PING)
static int ping_resp_send(struct ll_conn *conn, struct node_rx_pdu *rx)
{
	struct node_tx *tx;
	struct pdu_data *pdu_tx;

	/* acquire tx mem */
	tx = mem_acquire(&mem_conn_tx_ctrl.free);
	if (!tx) {
		return -ENOBUFS;
	}

	pdu_tx = (void *)tx->pdu;
	pdu_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_tx->len = offsetof(struct pdu_data_llctrl, ping_rsp) +
		      sizeof(struct pdu_data_llctrl_ping_rsp);
	pdu_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PING_RSP;

	ctrl_tx_enqueue(conn, tx);

	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_PHY)
static int phy_rsp_send(struct ll_conn *conn, struct node_rx_pdu *rx,
			struct pdu_data *pdu_rx)
{
	struct pdu_data_llctrl_phy_req *p;
	struct pdu_data *pdu_ctrl_tx;
	struct node_tx *tx;

	/* acquire tx mem */
	tx = mem_acquire(&mem_conn_tx_ctrl.free);
	if (!tx) {
		return -ENOBUFS;
	}

	/* Wait for peer master to complete the procedure */
	conn->llcp_phy.state = LLCP_PHY_STATE_RSP_WAIT;
	if (conn->llcp_phy.ack ==
	    conn->llcp_phy.req) {
		conn->llcp_phy.ack--;

		conn->llcp_phy.cmd = 0U;

		conn->llcp_phy.tx =
			conn->phy_pref_tx;
		conn->llcp_phy.rx =
			conn->phy_pref_rx;

		/* Start Procedure Timeout (TODO: this shall not
		 * replace terminate procedure).
		 */
		conn->procedure_expire =
			conn->procedure_reload;
	}

	p = &pdu_rx->llctrl.phy_req;

	conn->llcp_phy.tx &= p->rx_phys;
	conn->llcp_phy.rx &= p->tx_phys;

	/* pause data packet tx */
	conn->llcp_phy.pause_tx = 1U;

	pdu_ctrl_tx = (void *)tx->pdu;
	pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
	pdu_ctrl_tx->len = offsetof(struct pdu_data_llctrl, phy_rsp) +
			   sizeof(struct pdu_data_llctrl_phy_rsp);
	pdu_ctrl_tx->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
	pdu_ctrl_tx->llctrl.phy_rsp.tx_phys = conn->phy_pref_tx;
	pdu_ctrl_tx->llctrl.phy_rsp.rx_phys = conn->phy_pref_rx;

	ctrl_tx_enqueue(conn, tx);

	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

	return 0;
}

static inline u8_t phy_upd_ind_recv(struct ll_conn *conn, memq_link_t *link,
				    struct node_rx_pdu **rx,
				    struct pdu_data *pdu_rx)
{
	struct pdu_data_llctrl_phy_upd_ind *ind = &pdu_rx->llctrl.phy_upd_ind;
	u16_t instant;

	/* Both tx and rx PHY unchanged */
	if (!((ind->m_to_s_phy | ind->s_to_m_phy) & 0x07)) {
		struct node_rx_pu *p;

		/* Not in PHY Update Procedure or PDU in wrong state */
		if ((conn->llcp_phy.ack == conn->llcp_phy.req) ||
		    (conn->llcp_phy.state != LLCP_PHY_STATE_RSP_WAIT)) {
			/* Mark for buffer for release */
			(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

			return 0;
		}

		/* Procedure complete */
		conn->llcp_phy.ack = conn->llcp_phy.req;
		conn->llcp_phy.pause_tx = 0U;
		conn->procedure_expire = 0U;

		/* Reset packet timing restrictions */
		conn->lll.phy_tx_time = conn->lll.phy_tx;

		/* Ignore event generation if not local cmd initiated */
		if (!conn->llcp_phy.cmd) {
			/* Mark for buffer for release */
			(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

			return 0;
		}

		/* generate phy update complete event */
		(*rx)->hdr.type = NODE_RX_TYPE_PHY_UPDATE;

		p = (void *)pdu_rx;
		p->status = 0U;
		p->tx = conn->lll.phy_tx;
		p->rx = conn->lll.phy_rx;

		return 0;
	}

	/* instant passed */
	instant = sys_le16_to_cpu(ind->instant);
	if (((instant - conn->lll.event_counter) & 0xffff) > 0x7fff) {
		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		return BT_HCI_ERR_INSTANT_PASSED;
	}

	/* different transaction collision */
	if (((conn->llcp_req - conn->llcp_ack) & 0x03) == 0x02) {
		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		return BT_HCI_ERR_DIFF_TRANS_COLLISION;
	}

	if ((conn->llcp_phy.ack != conn->llcp_phy.req) &&
	    (conn->llcp_phy.state == LLCP_PHY_STATE_RSP_WAIT)) {
		/* Procedure complete, just wait for instant */
		conn->llcp_phy.ack = conn->llcp_phy.req;
		conn->llcp_phy.pause_tx = 0U;
		conn->procedure_expire = 0U;

		conn->llcp.phy_upd_ind.cmd = conn->llcp_phy.cmd;
	}

	conn->llcp.phy_upd_ind.tx = ind->s_to_m_phy;
	conn->llcp.phy_upd_ind.rx = ind->m_to_s_phy;
	conn->llcp.phy_upd_ind.instant = instant;
	conn->llcp.phy_upd_ind.initiate = 0U;

	link->mem = conn->llcp_rx;
	(*rx)->hdr.link = link;
	conn->llcp_rx = *rx;
	*rx = NULL;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	/* reserve rx node for DLE event generation */
	struct node_rx_pdu *rx_dle = ll_pdu_rx_alloc();

	LL_ASSERT(rx_dle);
	rx_dle->hdr.link->mem = conn->llcp_rx;
	conn->llcp_rx = rx_dle;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

	conn->llcp_type = LLCP_PHY_UPD;
	conn->llcp_ack -= 2U;

	if (conn->llcp.phy_upd_ind.tx) {
		conn->lll.phy_tx_time = conn->llcp.phy_upd_ind.tx;
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_PHY */

static inline void ctrl_tx_pre_ack(struct ll_conn *conn,
				   struct pdu_data *pdu_tx)
{
	switch (pdu_tx->llctrl.opcode) {
#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP:
		if (!conn->lll.role) {
			break;
		}
		/* Pass Through */

	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
	case PDU_DATA_LLCTRL_TYPE_ENC_RSP:
	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ:
		/* pause data packet tx */
		conn->llcp_enc.pause_tx = 1U;
		break;

#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
		if ((conn->llcp_length.req != conn->llcp_length.ack) &&
		    (conn->llcp_length.state == LLCP_LENGTH_STATE_ACK_WAIT)) {
			/* pause data packet tx */
			conn->llcp_length.pause_tx = 1U;
		}
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

	default:
		/* Do nothing for other ctrl packet ack */
		break;
	}
}

static inline void ctrl_tx_ack(struct ll_conn *conn, struct node_tx **tx,
			       struct pdu_data *pdu_tx)
{
	switch (pdu_tx->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_TERMINATE_IND:
	{
		u8_t reason = (pdu_tx->llctrl.terminate_ind.error_code ==
			       BT_HCI_ERR_REMOTE_USER_TERM_CONN) ?
			      BT_HCI_ERR_LOCALHOST_TERM_CONN :
			      pdu_tx->llctrl.terminate_ind.error_code;

		conn_cleanup(conn, reason);
	}
	break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		/* things from master stored for session key calculation */
		memcpy(&conn->llcp.encryption.skd[0],
		       &pdu_tx->llctrl.enc_req.skdm[0], 8);
		memcpy(&conn->lll.ccm_rx.iv[0],
		       &pdu_tx->llctrl.enc_req.ivm[0], 4);

		/* pause data packet tx */
		conn->llcp_enc.pause_tx = 1U;

		/* Start Procedure Timeout (this will not replace terminate
		 * procedure which always gets place before any packets
		 * going out, hence safe by design).
		 */
		conn->procedure_expire = conn->procedure_reload;

		/* Reset enc req queued state */
		conn->llcp_enc.ack = conn->llcp_enc.req;
		break;

	case PDU_DATA_LLCTRL_TYPE_ENC_RSP:
		/* pause data packet tx */
		conn->llcp_enc.pause_tx = 1U;
		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_REQ:
		/* Nothing to do.
		 * Remember that we may have received encrypted START_ENC_RSP
		 * alongwith this tx ack at this point in time.
		 */
		break;

	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ:
		/* pause data packet tx */
		conn->llcp_enc.pause_tx = 1U;

		/* key refresh */
		conn->llcp_enc.refresh = 1U;

		/* Start Procedure Timeout (this will not replace terminate
		 * procedure which always gets place before any packets
		 * going out, hence safe by design).
		 */
		conn->procedure_expire = conn->procedure_reload;
		break;

	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP:
		if (!conn->lll.role) {
			/* reused tx-ed PDU and send enc req */
			enc_req_reused_send(conn, tx);
		} else {
			/* pause data packet tx */
			conn->llcp_enc.pause_tx = 1U;
		}
		break;

	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		if (pdu_tx->llctrl.reject_ext_ind.reject_opcode !=
		    PDU_DATA_LLCTRL_TYPE_ENC_REQ) {
			break;
		}
		/* Pass through */

	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		/* resume data packet rx and tx */
		conn->llcp_enc.pause_rx = 0U;
		conn->llcp_enc.pause_tx = 0U;

		/* Procedure complete */
		conn->procedure_expire = 0U;
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
		if ((conn->llcp_length.req != conn->llcp_length.ack) &&
		    (conn->llcp_length.state == LLCP_LENGTH_STATE_ACK_WAIT)) {
			/* pause data packet tx */
			conn->llcp_length.pause_tx = 1U;

			/* wait for response */
			conn->llcp_length.state = LLCP_LENGTH_STATE_RSP_WAIT;
		}
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
		conn->llcp_phy.state = LLCP_PHY_STATE_RSP_WAIT;
		/* fall through */

	case PDU_DATA_LLCTRL_TYPE_PHY_RSP:
		if (conn->lll.role) {
			/* select the probable PHY with longest Tx time, which
			 * will be restricted to fit current
			 * connEffectiveMaxTxTime.
			 */
			u8_t phy_tx_time[8] = {BIT(0), BIT(0), BIT(1), BIT(0),
					       BIT(2), BIT(2), BIT(2), BIT(2)};
			struct lll_conn *lll = &conn->lll;
			u8_t phys;

			phys = conn->llcp_phy.tx | lll->phy_tx;
			lll->phy_tx_time = phy_tx_time[phys];
		}

		/* resume data packet tx */
		conn->llcp_phy.pause_tx = 0U;
		break;

	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		conn->lll.phy_tx_time = conn->llcp.phy_upd_ind.tx;
		/* resume data packet tx */
		conn->llcp_phy.pause_tx = 0U;
		break;
#endif /* CONFIG_BT_CTLR_PHY */

	default:
		/* Do nothing for other ctrl packet ack */
		break;
	}
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
	int nack = 0;
	u8_t opcode;

	opcode = pdu_rx->llctrl.opcode;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* FIXME: do check in individual case to reduce CPU time */
	if (conn->llcp_enc.pause_rx && ctrl_is_unexpected(conn, opcode)) {
		conn->llcp_terminate.reason_peer =
			BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		return 0;
	}
#endif /* CONFIG_BT_CTLR_LE_ENC */

	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
	{
		u8_t err;

		if (!conn->lll.role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		err = conn_upd_recv(conn, link, rx, pdu_rx);
		if (err) {
			conn->llcp_terminate.reason_peer = err;
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		} else {
			/* conn param req procedure, if any, is complete */
			conn->procedure_expire = 0U;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
		}
	}
	break;

	case PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND:
	{
		u8_t err;

		if (!conn->lll.role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		err = chan_map_upd_recv(conn, *rx, pdu_rx);
		if (err) {
			conn->llcp_terminate.reason_peer = err;
		}
	}
	break;

	case PDU_DATA_LLCTRL_TYPE_TERMINATE_IND:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_TERMINATE_IND,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		terminate_ind_recv(conn, *rx, pdu_rx);
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		if (!conn->lll.role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_ENC_REQ, pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

#if defined(CONFIG_BT_CTLR_FAST_ENC)
		/* TODO: BT Spec. text: may finalize the sending of additional
		 * data channel PDUs queued in the controller.
		 */
		nack = enc_rsp_send(conn);
		if (nack) {
			break;
		}
#endif /* CONFIG_BT_CTLR_FAST_ENC */

		/* things from master stored for session key calculation */
		memcpy(&conn->llcp.encryption.skd[0],
		       &pdu_rx->llctrl.enc_req.skdm[0], 8);
		memcpy(&conn->lll.ccm_rx.iv[0],
		       &pdu_rx->llctrl.enc_req.ivm[0], 4);

		/* pause rx data packets */
		conn->llcp_enc.pause_rx = 1U;

		/* Start Procedure Timeout (TODO: this shall not replace
		 * terminate procedure).
		 */
		conn->procedure_expire = conn->procedure_reload;

		break;

	case PDU_DATA_LLCTRL_TYPE_ENC_RSP:
		if (conn->lll.role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_ENC_RSP, pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		/* things sent by slave stored for session key calculation */
		memcpy(&conn->llcp.encryption.skd[8],
		       &pdu_rx->llctrl.enc_rsp.skds[0], 8);
		memcpy(&conn->lll.ccm_rx.iv[4],
		       &pdu_rx->llctrl.enc_rsp.ivs[0], 4);

		/* pause rx data packets */
		conn->llcp_enc.pause_rx = 1U;

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_REQ:
		if (conn->lll.role ||
		    ((conn->llcp_req != conn->llcp_ack) &&
		     (conn->llcp_type != LLCP_ENCRYPTION)) ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_START_ENC_REQ,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		/* start enc rsp to be scheduled in master prepare */
		conn->llcp.encryption.initiate = 0U;
		if (conn->llcp_req == conn->llcp_ack) {
			conn->llcp_type = LLCP_ENCRYPTION;
			conn->llcp_ack -= 2U;
		}

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_START_ENC_RSP,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		if (conn->lll.role) {
#if !defined(CONFIG_BT_CTLR_FAST_ENC)
			if ((conn->llcp_req != conn->llcp_ack) &&
			    (conn->llcp_type != LLCP_ENCRYPTION)) {
				goto ull_conn_rx_unknown_rsp_send;
			}

			/* start enc rsp to be scheduled in slave  prepare */
			conn->llcp.encryption.initiate = 0U;
			if (conn->llcp_req == conn->llcp_ack) {
				conn->llcp_type = LLCP_ENCRYPTION;
				conn->llcp_ack -= 2U;
			}
#else /* CONFIG_BT_CTLR_FAST_ENC */
			nack = start_enc_rsp_send(conn, NULL);
			if (nack) {
				break;
			}

			/* resume data packet rx and tx */
			conn->llcp_enc.pause_rx = 0U;
			conn->llcp_enc.pause_tx = 0U;
#endif /* CONFIG_BT_CTLR_FAST_ENC */

		} else {
			/* resume data packet rx and tx */
			conn->llcp_enc.pause_rx = 0U;
			conn->llcp_enc.pause_tx = 0U;
		}

		/* enqueue the start enc resp (encryption change/refresh) */
		if (conn->llcp_enc.refresh) {
			conn->llcp_enc.refresh = 0U;

			/* key refresh event */
			(*rx)->hdr.type = NODE_RX_TYPE_ENC_REFRESH;
		}

		/* Procedure complete */
		conn->procedure_expire = 0U;

		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

	case PDU_DATA_LLCTRL_TYPE_FEATURE_REQ:
		if (!conn->lll.role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_FEATURE_REQ,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		nack = feature_rsp_send(conn, *rx, pdu_rx);
		break;

#if defined(CONFIG_BT_CTLR_SLAVE_FEAT_REQ)
	case PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ:
		if (conn->lll.role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		nack = feature_rsp_send(conn, *rx, pdu_rx);
		break;
#endif /* CONFIG_BT_CTLR_SLAVE_FEAT_REQ */

	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_FEATURE_RSP,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		feature_rsp_recv(conn, pdu_rx);
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ:
		if (!conn->lll.role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		nack = pause_enc_rsp_send(conn, *rx, 1);
		break;

	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		nack = pause_enc_rsp_send(conn, *rx, 0);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_VERSION_IND,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		nack = version_ind_send(conn, *rx, pdu_rx);
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_REJECT_IND, pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		/* resume data packet rx and tx */
		conn->llcp_enc.pause_rx = 0U;
		conn->llcp_enc.pause_tx = 0U;

		/* Procedure complete */
		conn->llcp_ack = conn->llcp_req;
		conn->procedure_expire = 0U;
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}


		/* check CUI/CPR mutex for other connections having CPR in
		 * progress.
		 */
		if (conn_upd_curr && (conn_upd_curr != conn)) {
			/* Unsupported LL Parameter Value */
			nack = reject_ext_ind_send(conn, *rx,
					PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
					BT_HCI_ERR_UNSUPP_LL_PARAM_VAL);
			break;
		}

		if (!conn->lll.role) {
			if ((conn->llcp_conn_param.req !=
					conn->llcp_conn_param.ack) &&
			    ((conn->llcp_conn_param.state ==
			      LLCP_CPR_STATE_REQ) ||
			     (conn->llcp_conn_param.state ==
			      LLCP_CPR_STATE_RSP_WAIT) ||
			     (conn->llcp_conn_param.state ==
			      LLCP_CPR_STATE_UPD))) {
				/* Same procedure collision  */
				nack = reject_ext_ind_send(conn, *rx,
					PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
					BT_HCI_ERR_LL_PROC_COLLISION);
#if defined(CONFIG_BT_CTLR_PHY)
#if defined(CONFIG_BT_CTLR_LE_ENC)
			} else if (((((conn->llcp_req - conn->llcp_ack) &
				      0x03) == 0x02) &&
				    (conn->llcp_type != LLCP_ENCRYPTION)) ||
				   (conn->llcp_phy.req != conn->llcp_phy.ack)) {
#else /* !CONFIG_BT_CTLR_LE_ENC */
			} else if ((((conn->llcp_req - conn->llcp_ack) &
				     0x03) == 0x02) &&
				   (conn->llcp_phy.req != conn->llcp_phy.ack)) {
#endif /* !CONFIG_BT_CTLR_LE_ENC */
#else /* !CONFIG_BT_CTLR_PHY */
#if defined(CONFIG_BT_CTLR_LE_ENC)
			} else if ((((conn->llcp_req - conn->llcp_ack) &
				     0x03) == 0x02) &&
				   (conn->llcp_type != LLCP_ENCRYPTION)) {
#else /* !CONFIG_BT_CTLR_LE_ENC */
			} else if (((conn->llcp_req - conn->llcp_ack) &
				      0x03) == 0x02) {
#endif /* !CONFIG_BT_CTLR_LE_ENC */
#endif /* !CONFIG_BT_CTLR_PHY */
				/* Different procedure collision */
				nack = reject_ext_ind_send(conn, *rx,
					PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
					BT_HCI_ERR_DIFF_TRANS_COLLISION);
			} else {
				struct pdu_data_llctrl_conn_param_req *cpr = (void *)
					&pdu_rx->llctrl.conn_param_req;
				struct lll_conn *lll = &conn->lll;

				/* Extract parameters */
				u16_t interval_min =
					sys_le16_to_cpu(cpr->interval_min);
				u16_t interval_max =
					sys_le16_to_cpu(cpr->interval_max);
				u16_t latency =
					sys_le16_to_cpu(cpr->latency);
				u16_t timeout =
					sys_le16_to_cpu(cpr->timeout);
				u16_t preferred_periodicity =
					cpr->preferred_periodicity;

				/* Invalid parameters */
				if ((interval_min < 6) ||
				    (interval_max > 3200) ||
				    (interval_min > interval_max) ||
				    (latency > 499) ||
				    (timeout < 10) ||
				    (timeout > 3200) ||
				    ((timeout * 4U) <=
				     ((latency + 1) * interval_max)) ||
				    (preferred_periodicity > interval_max)) {
					nack = reject_ext_ind_send(conn, *rx,
						PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
						BT_HCI_ERR_INVALID_LL_PARAM);
					break;
				}

				/* save parameters to be used to select offset
				 */
				conn->llcp_conn_param.interval_min =
					interval_min;
				conn->llcp_conn_param.interval_max =
					interval_max;
				conn->llcp_conn_param.latency =	latency;
				conn->llcp_conn_param.timeout =	timeout;
				conn->llcp_conn_param.preferred_periodicity =
					preferred_periodicity;
				conn->llcp_conn_param.reference_conn_event_count =
					sys_le16_to_cpu(cpr->reference_conn_event_count);
				conn->llcp_conn_param.offset0 =
					sys_le16_to_cpu(cpr->offset0);
				conn->llcp_conn_param.offset1 =
					sys_le16_to_cpu(cpr->offset1);
				conn->llcp_conn_param.offset2 =
					sys_le16_to_cpu(cpr->offset2);
				conn->llcp_conn_param.offset3 =
					sys_le16_to_cpu(cpr->offset3);
				conn->llcp_conn_param.offset4 =
					sys_le16_to_cpu(cpr->offset4);
				conn->llcp_conn_param.offset5 =
					sys_le16_to_cpu(cpr->offset5);

				/* enqueue the conn param req, if parameters
				 * changed, else respond.
				 */
				if ((conn->llcp_conn_param.interval_max !=
				     lll->interval) ||
				    (conn->llcp_conn_param.latency !=
				     lll->latency) ||
				    (RADIO_CONN_EVENTS(conn->llcp_conn_param.timeout *
						       10000U,
						       lll->interval *
						       1250) !=
				     conn->supervision_reload)) {
#if defined(CONFIG_BT_CTLR_LE_ENC)
					/* postpone CP request event if under
					 * encryption setup
					 */
					if (conn->llcp_enc.pause_tx) {
						conn->llcp_conn_param.state =
							LLCP_CPR_STATE_APP_REQ;

						/* Mark for buffer for release */
						(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
					} else
#endif /* CONFIG_BT_CTLR_LE_ENC */
					{
						conn->llcp_conn_param.state =
							LLCP_CPR_STATE_APP_WAIT;
					}
				} else {
					conn->llcp_conn_param.status = 0U;
					conn->llcp_conn_param.cmd = 0U;
					conn->llcp_conn_param.state =
						LLCP_CPR_STATE_RSP;

					/* Mark for buffer for release */
					(*rx)->hdr.type =
						NODE_RX_TYPE_DC_PDU_RELEASE;
				}

				conn->llcp_conn_param.ack--;

				/* set mutex */
				if (!conn_upd_curr) {
					conn_upd_curr = conn;
				}
			}
		} else if ((conn->llcp_conn_param.req ==
			    conn->llcp_conn_param.ack) ||
			   (conn->llcp_conn_param.state ==
			    LLCP_CPR_STATE_REQ) ||
			   (conn->llcp_conn_param.state ==
			    LLCP_CPR_STATE_RSP_WAIT)) {
			struct pdu_data_llctrl_conn_param_req *cpr = (void *)
				&pdu_rx->llctrl.conn_param_req;
			struct lll_conn *lll = &conn->lll;

			/* Extract parameters */
			u16_t interval_min = sys_le16_to_cpu(cpr->interval_min);
			u16_t interval_max = sys_le16_to_cpu(cpr->interval_max);
			u16_t latency = sys_le16_to_cpu(cpr->latency);
			u16_t timeout = sys_le16_to_cpu(cpr->timeout);
			u16_t preferred_periodicity =
				cpr->preferred_periodicity;

			/* Invalid parameters */
			if ((interval_min < 6) ||
			    (interval_max > 3200) ||
			    (interval_min > interval_max) ||
			    (latency > 499) ||
			    (timeout < 10) || (timeout > 3200) ||
			    ((timeout * 4U) <=
			     ((latency + 1) * interval_max)) ||
			    (preferred_periodicity > interval_max)) {
				nack = reject_ext_ind_send(conn, *rx,
					PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
					BT_HCI_ERR_INVALID_LL_PARAM);
				break;
			}

			/* resp to be generated by app, for now save
			 * parameters
			 */
			conn->llcp_conn_param.interval_min = interval_min;
			conn->llcp_conn_param.interval_max = interval_max;
			conn->llcp_conn_param.latency =	latency;
			conn->llcp_conn_param.timeout =	timeout;
			conn->llcp_conn_param.preferred_periodicity =
				preferred_periodicity;
			conn->llcp_conn_param.reference_conn_event_count =
				sys_le16_to_cpu(cpr->reference_conn_event_count);
			conn->llcp_conn_param.offset0 =
				sys_le16_to_cpu(cpr->offset0);
			conn->llcp_conn_param.offset1 =
				sys_le16_to_cpu(cpr->offset1);
			conn->llcp_conn_param.offset2 =
				sys_le16_to_cpu(cpr->offset2);
			conn->llcp_conn_param.offset3 =
				sys_le16_to_cpu(cpr->offset3);
			conn->llcp_conn_param.offset4 =
				sys_le16_to_cpu(cpr->offset4);
			conn->llcp_conn_param.offset5 =
				sys_le16_to_cpu(cpr->offset5);

			/* enqueue the conn param req, if parameters changed,
			 * else respond
			 */
			if ((conn->llcp_conn_param.interval_max !=
			     lll->interval) ||
			    (conn->llcp_conn_param.latency != lll->latency) ||
			    (RADIO_CONN_EVENTS(conn->llcp_conn_param.timeout *
					       10000U,
					       lll->interval *
					       1250) !=
			     conn->supervision_reload)) {
				conn->llcp_conn_param.state =
					LLCP_CPR_STATE_APP_WAIT;
			} else {
				conn->llcp_conn_param.status = 0U;
				conn->llcp_conn_param.cmd = 0U;
				conn->llcp_conn_param.state =
					LLCP_CPR_STATE_RSP;

				/* Mark for buffer for release */
				(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
			}

			conn->llcp_conn_param.ack--;

			/* set mutex */
			if (!conn_upd_curr) {
				conn_upd_curr = conn;
			}
		} else {
			LL_ASSERT(0);
		}
		break;

	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP:
		if (conn->lll.role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		if (!conn->lll.role &&
		    (conn->llcp_conn_param.req !=
		     conn->llcp_conn_param.ack) &&
		    (conn->llcp_conn_param.state ==
		     LLCP_CPR_STATE_RSP_WAIT)) {
			struct pdu_data_llctrl_conn_param_req *cpr = (void *)
				&pdu_rx->llctrl.conn_param_req;

			/* Extract parameters */
			u16_t interval_min = sys_le16_to_cpu(cpr->interval_min);
			u16_t interval_max = sys_le16_to_cpu(cpr->interval_max);
			u16_t latency = sys_le16_to_cpu(cpr->latency);
			u16_t timeout = sys_le16_to_cpu(cpr->timeout);
			u16_t preferred_periodicity =
				cpr->preferred_periodicity;

			/* Invalid parameters */
			if ((interval_min < 6) ||
			    (interval_max > 3200) ||
			    (interval_min > interval_max) ||
			    (latency > 499) ||
			    (timeout < 10) || (timeout > 3200) ||
			    ((timeout * 4U) <=
			     ((latency + 1) * interval_max)) ||
			    (preferred_periodicity > interval_max)) {
				nack = reject_ext_ind_send(conn, *rx,
					PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP,
					BT_HCI_ERR_INVALID_LL_PARAM);
				break;
			}

			/* Stop procedure timeout */
			conn->procedure_expire = 0U;

			/* save parameters to be used to select offset
			 */
			conn->llcp_conn_param.interval_min = interval_min;
			conn->llcp_conn_param.interval_max = interval_max;
			conn->llcp_conn_param.latency =	latency;
			conn->llcp_conn_param.timeout =	timeout;
			conn->llcp_conn_param.preferred_periodicity =
				preferred_periodicity;
			conn->llcp_conn_param.reference_conn_event_count =
				sys_le16_to_cpu(cpr->reference_conn_event_count);
			conn->llcp_conn_param.offset0 =
				sys_le16_to_cpu(cpr->offset0);
			conn->llcp_conn_param.offset1 =
				sys_le16_to_cpu(cpr->offset1);
			conn->llcp_conn_param.offset2 =
				sys_le16_to_cpu(cpr->offset2);
			conn->llcp_conn_param.offset3 =
				sys_le16_to_cpu(cpr->offset3);
			conn->llcp_conn_param.offset4 =
				sys_le16_to_cpu(cpr->offset4);
			conn->llcp_conn_param.offset5 =
				sys_le16_to_cpu(cpr->offset5);

			/* Perform connection update */
			conn->llcp_conn_param.state = LLCP_CPR_STATE_RSP;
		}

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		reject_ext_ind_recv(conn, *rx, pdu_rx);
		break;

#if defined(CONFIG_BT_CTLR_LE_PING)
	case PDU_DATA_LLCTRL_TYPE_PING_REQ:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PING_REQ, pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		nack = ping_resp_send(conn, *rx);
		break;

	case PDU_DATA_LLCTRL_TYPE_PING_RSP:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PING_RSP, pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		/* Procedure complete */
		conn->procedure_expire = 0U;

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		break;
#endif /* CONFIG_BT_CTLR_LE_PING */

	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		if (0) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		} else if (conn->llcp_conn_param.ack !=
			   conn->llcp_conn_param.req) {
			struct lll_conn *lll = &conn->lll;
			struct node_rx_cu *cu;

			/* Mark CPR as unsupported */
			conn->llcp_conn_param.disabled = 1U;

			/* TODO: check for unsupported remote feature reason */
			if (!conn->lll.role) {
				LL_ASSERT(conn->llcp_req == conn->llcp_ack);

				conn->llcp_conn_param.state =
					LLCP_CPR_STATE_UPD;

				conn->llcp.conn_upd.win_size = 1U;
				conn->llcp.conn_upd.win_offset_us = 0U;
				conn->llcp.conn_upd.interval =
					conn->llcp_conn_param.interval_max;
				conn->llcp.conn_upd.latency =
					conn->llcp_conn_param.latency;
				conn->llcp.conn_upd.timeout =
					conn->llcp_conn_param.timeout;
				/* conn->llcp.conn_upd.instant     = 0; */
				conn->llcp.conn_upd.state = LLCP_CUI_STATE_USE;
				conn->llcp.conn_upd.is_internal =
					!conn->llcp_conn_param.cmd;
				conn->llcp_type = LLCP_CONN_UPD;
				conn->llcp_ack -= 2U;

				/* Mark for buffer for release */
				(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

				break;
			}

			LL_ASSERT(conn_upd_curr == conn);

			/* reset mutex */
			conn_upd_curr = NULL;

			/* Procedure complete */
			conn->llcp_conn_param.ack = conn->llcp_conn_param.req;

			/* skip event generation if not cmd initiated */
			if (!conn->llcp_conn_param.cmd) {
				/* Mark for buffer for release */
				(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

				break;
			}

			/* generate conn upd complete event with error code */
			(*rx)->hdr.type = NODE_RX_TYPE_CONN_UPDATE;

			/* prepare connection update complete structure */
			cu = (void *)pdu_rx;
			cu->status = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
			cu->interval = lll->interval;
			cu->latency = lll->latency;
			cu->timeout = conn->supervision_reload *
				      lll->interval * 125U / 1000;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		} else if (conn->llcp_length.req != conn->llcp_length.ack) {
			/* Procedure complete */
			conn->llcp_length.ack = conn->llcp_length.req;
			conn->llcp_length.pause_tx = 0U;

			/* propagate the data length procedure to
			 * host
			 */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
		} else if (conn->llcp_phy.req !=
			   conn->llcp_phy.ack) {
			struct lll_conn *lll = &conn->lll;

			/* Procedure complete */
			conn->llcp_phy.ack = conn->llcp_phy.req;
			conn->llcp_phy.pause_tx = 0U;

			/* Reset packet timing restrictions */
			lll->phy_tx_time = lll->phy_tx;

			/* skip event generation is not cmd initiated */
			if (conn->llcp_phy.cmd) {
				struct node_rx_pu *p;

				/* generate phy update complete event */
				(*rx)->hdr.type = NODE_RX_TYPE_PHY_UPDATE;

				p = (void *)pdu_rx;
				p->status = 0U;
				p->tx = lll->phy_tx;
				p->rx = lll->phy_rx;
			} else {
				/* Mark for buffer for release */
				(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
			}
#endif /* CONFIG_BT_CTLR_PHY */

		} else {
			struct pdu_data_llctrl *llctrl;

			llctrl = (void *)&pdu_rx->llctrl;
			switch (llctrl->unknown_rsp.type) {

#if defined(CONFIG_BT_CTLR_LE_PING)
			case PDU_DATA_LLCTRL_TYPE_PING_REQ:
				/* unknown rsp to LE Ping Req completes the
				 * procedure; nothing to do here.
				 */

				/* Mark for buffer for release */
				(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
				break;
#endif /* CONFIG_BT_CTLR_LE_PING */

			default:
				/* TODO: enqueue the error and let HCI handle
				 *       it.
				 */
				break;
			}
		}

		/* Procedure complete */
		conn->procedure_expire = 0U;
		break;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PDU_DATA_LLCTRL_TYPE_LENGTH_RSP:
	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_LENGTH_REQ,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		nack = length_req_rsp_recv(conn, link, rx, pdu_rx);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
		if (!pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PHY_REQ, pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		if (!conn->lll.role) {
			if ((conn->llcp_phy.ack !=
			     conn->llcp_phy.req) &&
			    ((conn->llcp_phy.state ==
			      LLCP_PHY_STATE_ACK_WAIT) ||
			     (conn->llcp_phy.state ==
			      LLCP_PHY_STATE_RSP_WAIT) ||
			     (conn->llcp_phy.state ==
			      LLCP_PHY_STATE_UPD))) {
				/* Same procedure collision  */
				nack = reject_ext_ind_send(conn, *rx,
					PDU_DATA_LLCTRL_TYPE_PHY_REQ,
					BT_HCI_ERR_LL_PROC_COLLISION);
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
#if defined(CONFIG_BT_CTLR_LE_ENC)
			} else if (((((conn->llcp_req - conn->llcp_ack) &
				      0x03) == 0x02) &&
				    (conn->llcp_type !=
				     LLCP_ENCRYPTION)) ||
				   (conn->llcp_conn_param.req !=
				    conn->llcp_conn_param.ack)) {
#else /* !CONFIG_BT_CTLR_LE_ENC */
			} else if ((((conn->llcp_req - conn->llcp_ack) &
				     0x03) == 0x02) &&
				   (conn->llcp_conn_param.req !=
				    conn->llcp_conn_param.ack)) {
#endif /* !CONFIG_BT_CTLR_LE_ENC */
#else /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
#if defined(CONFIG_BT_CTLR_LE_ENC)
			} else if ((((conn->llcp_req - conn->llcp_ack) &
				     0x03) == 0x02) &&
				   (conn->llcp_type !=
				    LLCP_ENCRYPTION)) {
#else /* !CONFIG_BT_CTLR_LE_ENC */
			} else if (((conn->llcp_req - conn->llcp_ack) &
				    0x03) == 0x02) {
#endif /* !CONFIG_BT_CTLR_LE_ENC */
#endif /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
				/* Different procedure collision */
				nack = reject_ext_ind_send(conn, *rx,
					PDU_DATA_LLCTRL_TYPE_PHY_REQ,
					BT_HCI_ERR_DIFF_TRANS_COLLISION);
			} else {
				struct pdu_data_llctrl *c = &pdu_rx->llctrl;
				struct pdu_data_llctrl_phy_req *p =
					&c->phy_req;

				conn->llcp_phy.state =
					LLCP_PHY_STATE_UPD;

				if (conn->llcp_phy.ack ==
				    conn->llcp_phy.req) {
					conn->llcp_phy.ack--;

					conn->llcp_phy.cmd = 0U;

					conn->llcp_phy.tx =
						conn->phy_pref_tx;
					conn->llcp_phy.rx =
						conn->phy_pref_rx;
				}

				conn->llcp_phy.tx &= p->rx_phys;
				conn->llcp_phy.rx &= p->tx_phys;

				if (!conn->llcp_phy.tx || !conn->llcp_phy.rx) {
					conn->llcp_phy.tx = 0;
					conn->llcp_phy.rx = 0;
				}

				/* pause data packet tx */
				conn->llcp_phy.pause_tx = 1U;

				/* Mark for buffer for release */
				(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
			}
		} else {
			nack = phy_rsp_send(conn, *rx, pdu_rx);
		}
		break;

	case PDU_DATA_LLCTRL_TYPE_PHY_RSP:
		if (conn->lll.role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PHY_RSP, pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		if (!conn->lll.role &&
		    (conn->llcp_phy.ack != conn->llcp_phy.req) &&
		    (conn->llcp_phy.state == LLCP_PHY_STATE_RSP_WAIT)) {
			struct pdu_data_llctrl_phy_rsp *p =
				&pdu_rx->llctrl.phy_rsp;

			conn->llcp_phy.state = LLCP_PHY_STATE_UPD;

			conn->llcp_phy.tx &= p->rx_phys;
			conn->llcp_phy.rx &= p->tx_phys;

			if (!conn->llcp_phy.tx || !conn->llcp_phy.rx) {
				conn->llcp_phy.tx = 0;
				conn->llcp_phy.rx = 0;
			}

			/* pause data packet tx */
			conn->llcp_phy.pause_tx = 1U;

			/* Procedure timeout is stopped */
			conn->procedure_expire = 0U;
		}

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		break;

	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
	{
		u8_t err;

		if (!conn->lll.role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		err = phy_upd_ind_recv(conn, link, rx, pdu_rx);
		if (err) {
			conn->llcp_terminate.reason_peer = err;
		}
	}
	break;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	case PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND:
		if (conn->lll.role ||
		    !pdu_len_cmp(PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND,
				 pdu_rx->len)) {
			goto ull_conn_rx_unknown_rsp_send;
		}

		if (!conn->lll.role) {
			struct pdu_data_llctrl_min_used_chans_ind *p =
				&pdu_rx->llctrl.min_used_chans_ind;

#if defined(CONFIG_BT_CTLR_PHY)
			if (!(p->phys & (conn->lll.phy_tx |
					 conn->lll.phy_rx))) {
#else /* !CONFIG_BT_CTLR_PHY */
			if (!(p->phys & 0x01)) {
#endif /* !CONFIG_BT_CTLR_PHY */
				break;
			}

			if (((conn->llcp_req - conn->llcp_ack) & 0x03) ==
			    0x02) {
				break;
			}

			memcpy(&conn->llcp.chan_map.chm[0], data_chan_map,
			       sizeof(conn->llcp.chan_map.chm));
			/* conn->llcp.chan_map.instant     = 0; */
			conn->llcp.chan_map.initiate = 1U;

			conn->llcp_type = LLCP_CHAN_MAP;
			conn->llcp_ack -= 2U;
		}

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */

	default:
ull_conn_rx_unknown_rsp_send:
		nack = unknown_rsp_send(conn, *rx, opcode);
		break;
	}

	return nack;
}
