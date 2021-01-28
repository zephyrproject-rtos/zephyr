/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <sys/byteorder.h>

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
#include "lll_conn.h"

#include "ull_conn_llcp_internal.h"
#include "ull_tx_queue.h"
#include "ull_llcp.h"

#include "ull_conn_types.h"
#include "ull_internal.h"
#include "ull_slave_internal.h"
#include "ull_master_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_connections
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_USER_EXT)
#include "ull_vendor.h"
#endif /* CONFIG_BT_CTLR_USER_EXT */

/**
 *  User CPR Interval
 */
#if !defined(CONFIG_BT_CTLR_USER_CPR_INTERVAL_MIN)
/* Bluetooth defined CPR Interval Minimum (7.5ms) */
#define CONN_INTERVAL_MIN(x) (6)
#else /* CONFIG_BT_CTLR_USER_CPR_INTERVAL_MIN */
/* Proprietary user defined CPR Interval Minimum */
extern uint16_t ull_conn_interval_min_get(struct ll_conn *conn);
#define CONN_INTERVAL_MIN(x) (MAX(ull_conn_interval_min_get(x), 1))
#endif /* CONFIG_BT_CTLR_USER_CPR_INTERVAL_MIN */

static int init_reset(void);

static inline void disable(uint16_t handle);

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
static int empty_data_start_release(struct ll_conn *conn, struct node_tx *tx);
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

static inline int ctrl_rx(memq_link_t *link, struct node_rx_pdu **rx,
			  struct pdu_data *pdu_rx, struct ll_conn *conn);

#if !defined(BT_CTLR_USER_TX_BUFFER_OVERHEAD)
#define BT_CTLR_USER_TX_BUFFER_OVERHEAD 0
#endif /* BT_CTLR_USER_TX_BUFFER_OVERHEAD */

#define CONN_TX_BUF_SIZE MROUND(offsetof(struct node_tx, pdu) + \
				offsetof(struct pdu_data, lldata) + \
				(CONFIG_BT_CTLR_TX_BUFFER_SIZE + \
				BT_CTLR_USER_TX_BUFFER_OVERHEAD))

static MFIFO_DEFINE(conn_tx, sizeof(struct lll_tx), CONFIG_BT_CTLR_TX_BUFFERS);
static MFIFO_DEFINE(conn_ack, sizeof(struct lll_tx),CONFIG_BT_CTLR_TX_BUFFERS);


static struct {
	void *free;
	uint8_t pool[CONN_TX_BUF_SIZE * CONFIG_BT_CTLR_TX_BUFFERS];
} mem_conn_tx;

/* TODO(thoh): What is the correct size for this pool ? */
static struct {
	void *free;
	uint8_t pool[sizeof(memq_link_t) * CONFIG_BT_CTLR_TX_BUFFERS];
} mem_link_tx;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static uint16_t default_tx_octets;
static uint16_t default_tx_time;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static uint8_t default_phy_tx;
static uint8_t default_phy_rx;
#endif /* CONFIG_BT_CTLR_PHY */

static struct {
	void *free;
	struct ll_conn pool[CONFIG_BT_MAX_CONN];
} mem_conn;

static struct ll_conn *conn_upd_curr;

struct ll_conn *ll_conn_acquire(void)
{
	return mem_acquire(&mem_conn.free);
}

void ll_conn_release(struct ll_conn  *conn)
{
	mem_release(conn, &mem_conn.free);
}

uint16_t ll_conn_handle_get(struct ll_conn *conn)
{
	return mem_index_get(conn, mem_conn.pool, sizeof(struct ll_conn));
}

struct ll_conn *ll_conn_get(uint16_t handle)
{
	return mem_get(mem_conn.pool, sizeof(struct ll_conn), handle);
}

struct ll_conn  *ll_connected_get(uint16_t handle)
{
	struct ll_conn  *conn;

	if (handle >= CONFIG_BT_MAX_CONN) {
		return NULL;
	}

	conn = ll_conn_get(handle);
	/*
	 * the conn->lll.handle can be different from handle
	 * when the connection is not established,
	 * for ex. due to connection termination
	 */
	if (conn->lll.handle != handle) {
		LL_ASSERT(conn->lll.handle != ULL_HANDLE_NOT_CONNECTED);
		return NULL;
	}

	return conn;
}

void *ull_conn_tx_mem_acquire(void)
{
	return mem_acquire(&mem_conn_tx.free);
}

void ull_conn_tx_mem_release(void *tx)
{
	mem_release(tx, &mem_conn_tx.free);
}

uint8_t ull_conn_mfifo_get_tx(void *lll_tx)
{
	return MFIFO_ENQUEUE_GET(conn_tx, (void **) &lll_tx);
}

void ull_conn_mfifo_enqueue_tx(uint8_t idx)
{
	MFIFO_ENQUEUE(conn_tx, idx);
}

/*
 * EGON: following 2 functions need to be removed when implementing tx/rx path
 * ll_rx_enqueue needs to be replaced with calls to ll_rx_put/ll_rx_sched
 */

#ifdef ZTEST_UNITTEST
extern sys_slist_t ut_rx_q;
#else
sys_slist_t ut_rx_q;
#endif

void ll_rx_enqueue(struct node_rx_pdu *rx)
{
	sys_slist_append(&ut_rx_q, (sys_snode_t *) rx);
}



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

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
void ull_conn_default_tx_octets_set(uint16_t tx_octets)
{
       default_tx_octets = tx_octets;
}

#if defined(CONFIG_BT_CTLR_PHY)
void ull_conn_default_tx_time_set(uint16_t tx_time)
{
       default_tx_time = tx_time;
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

#if defined(CONFIG_BT_CTLR_PHY)
void ull_conn_default_phy_tx_set(uint8_t phy_tx)
{
       default_phy_tx = phy_tx;
}

void ull_conn_default_phy_rx_set(uint8_t phy_rx)
{
       default_phy_rx = phy_rx;
}
#endif /* CONFIG_BT_CTLR_PHY */


void ull_conn_setup(memq_link_t *link, struct node_rx_hdr *rx)
{
	struct node_rx_ftr *ftr;
	struct lll_conn *lll;

	ftr = &(rx->rx_ftr);

	lll = *((struct lll_conn **)((uint8_t *)ftr->param +
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
		(*rx)->hdr.type = NODE_RX_TYPE_RELEASE;

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
		/*
		 * EGON TODO: use correct data structure
		 */
		if (conn->llcp.enc.pause_rx) {
			conn->llcp.terminate.reason_peer =
				BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;

			/* Mark for buffer for release */
			(*rx)->hdr.type = NODE_RX_TYPE_RELEASE;
		}
#endif /* CONFIG_BT_CTLR_LE_ENC */
		break;

	case PDU_DATA_LLID_RESV:
	default:
#if defined(CONFIG_BT_CTLR_LE_ENC)
		/*
		 * EGON TODO: use correct data structure
		 * or more likely this is handled in ull_llcp.c
		 */
		if (conn->llcp.enc.pause_rx) {
			conn->llcp.terminate.reason_peer =
				BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;
		}
#endif /* CONFIG_BT_CTLR_LE_ENC */

		/* Invalid LL id, drop it. */

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_RELEASE;

		break;
	}


	return 0;
}



int ull_conn_llcp(struct ll_conn *conn, uint32_t ticks_at_expire, uint16_t lazy)
{
	LL_ASSERT(conn->lll.handle != ULL_HANDLE_NOT_CONNECTED);

	return 0;
}

void ull_conn_done(struct node_rx_event_done *done)
{
	return;
}

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

			/* TODO(thoh): Is this assignment needed? */
			tx->next = NULL;
			ull_tx_q_enqueue_data(&conn->tx_q, tx);
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

void ull_conn_tx_lll_enqueue(struct ll_conn *conn, uint8_t count)
{
	while (count--) {
		struct node_tx *tx;
		memq_link_t *link;

		tx = ull_tx_q_dequeue(&conn->tx_q);
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
	return 0;
}

memq_link_t *ull_conn_ack_peek(uint8_t *ack_last, uint16_t *handle,
			       struct node_tx **tx)
{
	return NULL;
}

memq_link_t *ull_conn_ack_by_last_peek(uint8_t last, uint16_t *handle,
				       struct node_tx **tx)
{
	return NULL;
}

void *ull_conn_ack_dequeue(void)
{
	return NULL;
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
	return;
}

uint8_t ull_conn_llcp_req(void *conn)
{
	return 0;
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
		max_tx_octets = (lll->max_tx_time >> 3) - 10;
		break;

	case PHY_2M:
		/* 2M PHY, 1us = 2 bits, hence divide by 4.
		 * Deduct 11 bytes for preamble (2), access address (4),
		 * header (2), and CRC (3).
		 */
		max_tx_octets = (lll->max_tx_time >> 2) - 11;
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
	mem_init(mem_conn.pool, sizeof(struct ll_conn),
		 sizeof(mem_conn.pool) / sizeof(struct ll_conn), &mem_conn.free);

	/* Initialize tx link pool. */
	mem_init(mem_link_tx.pool, sizeof(memq_link_t),
		 CONFIG_BT_CTLR_TX_BUFFERS, &mem_link_tx.free);

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	/* Initialize the DLE defaults */
	default_tx_octets = PDU_DC_PAYLOAD_SIZE_MIN;
	default_tx_time = PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, PHY_1M);
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

	return 0;
}

static inline void disable(uint16_t handle)
{
	struct ll_conn *conn;
	int err;

	conn = ll_conn_get(handle);

	err = ull_ticker_stop_with_mark(TICKER_ID_CONN_BASE + handle,
					conn, &conn->lll);
	LL_ASSERT(err == 0 || err == -EALREADY);

	/*
	 * EGON TODO: use proper variable when available
	conn->lll.link_tx_free = NULL;
	*/
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

static inline int ctrl_rx(memq_link_t *link, struct node_rx_pdu **rx,
			  struct pdu_data *pdu_rx, struct ll_conn *conn)
{
	/*
	 * EGON TODO: this is part of the receiving of PDUs,
	 * to be filled in later
	 */
	return 0;
}
