/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <sys/byteorder.h>

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/mayfly.h"

#include "hal/ccm.h"
#include "hal/debug.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "lll.h"
#include "lll_conn.h"
#include "ull_internal.h"
#include "ull_tx_queue.h"
#include "ull_llcp.h"
#include "ull_conn_llcp_internal.h"
#include "ull_conn_types.h"
#include "ull_master_internal.h"
#include "ull_slave_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_connections
#include "common/log.h"

#if !defined(BT_CTLR_USER_TX_BUFFER_OVERHEAD)
#define BT_CTLR_USER_TX_BUFFER_OVERHEAD 0
#endif /* BT_CTLR_USER_TX_BUFFER_OVERHEAD */


#define CONN_TX_BUF_SIZE                                                       \
	MROUND(offsetof(struct node_tx, pdu) +                                 \
	       offsetof(struct pdu_data, lldata) +                             \
	       (CONFIG_BT_CTLR_TX_BUFFER_SIZE +                                \
		BT_CTLR_USER_TX_BUFFER_OVERHEAD))

#define CONN_TX_CTRL_BUFFERS (4 * CONFIG_BT_CTLR_LLCP_CONN)


static MFIFO_DEFINE(conn_tx, sizeof(struct lll_tx), CONFIG_BT_CTLR_TX_BUFFERS);
static MFIFO_DEFINE(conn_ack, sizeof(struct lll_tx),
		    (CONFIG_BT_CTLR_TX_BUFFERS + CONN_TX_CTRL_BUFFERS));



static struct {
	void *free;
	uint8_t pool[CONN_TX_BUF_SIZE * CONFIG_BT_CTLR_TX_BUFFERS];
} mem_conn_tx;

static struct ull_cp_conn conn_pool[CONFIG_BT_MAX_CONN];
static struct ll_conn *conn_upd_curr;
static void *conn_free;

static uint8_t data_chan_map[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
static uint8_t data_chan_count = 37U;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static uint16_t default_tx_octets;
static uint16_t default_tx_time;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static uint8_t default_phy_tx;
static uint8_t default_phy_rx;
#endif /* CONFIG_BT_CTLR_PHY */


static uint16_t init_reset(void);
static inline void disable(uint16_t handle);
static inline int ctrl_rx(memq_link_t *link, struct node_rx_pdu **rx,
			  struct pdu_data *pdu_rx, struct ull_cp_conn *conn);


uint16_t ull_conn_init(void)
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

struct ull_cp_conn *ll_conn_acquire(void)
{
	return mem_acquire(&conn_free);
}


struct ull_cp_conn *ll_conn_get(uint16_t handle)
{
	return mem_get(conn_pool, sizeof(struct ull_cp_conn), handle);
}

uint16_t ll_conn_handle_get(struct ull_cp_conn *conn)
{
	return mem_index_get(conn, conn_pool, sizeof(struct ull_cp_conn));
}


struct ull_cp_conn  *ll_connected_get(uint16_t handle)
{
	struct ull_cp_conn  *conn;

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

void ll_conn_release(struct ull_cp_conn  *conn)
{
	mem_release(conn, &conn_free);
}

uint16_t ll_conn_free(void)
{
	return mem_free_count_get(conn_free);
}


uint8_t ull_conn_chan_map_cpy(uint8_t *chan_map)
{
	memcpy(chan_map, data_chan_map, sizeof(data_chan_map));

	return data_chan_count;
}

void ull_conn_chan_map_set(uint8_t const *const chan_map)
{

	memcpy(data_chan_map, chan_map, sizeof(data_chan_map));
	data_chan_count = util_ones_count_get(data_chan_map,
					      sizeof(data_chan_map));
}

#if defined(CONFIG_BT_CTLR_PHY)
uint8_t ull_conn_default_phy_tx_get(void)
{
	return default_phy_tx;
}

uint8_t ull_conn_default_phy_rx_get(void)
{
	return default_phy_rx;
}

void ull_conn_default_phy_tx_set(uint8_t phy_tx)
{
	default_phy_tx = phy_tx;
}

void ull_conn_default_phy_rx_set(uint8_t phy_rx)
{
	default_phy_rx = phy_rx;
}
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
uint16_t ull_conn_default_tx_octets_get(void)
{
	return default_tx_octets;
}

void ull_conn_default_tx_octets_set(uint16_t tx_octets)
{
	default_tx_octets = tx_octets;
}

#if defined(CONFIG_BT_CTLR_PHY)
uint16_t ull_conn_default_tx_time_get(void)
{
	return default_tx_time;
}
void ull_conn_default_tx_time_set(uint16_t tx_time)
{
	default_tx_time = tx_time;
}
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

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
	struct ull_cp_conn *conn;

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
		/*
		 * EGON TODO: use correct data structure
		 */
#if 0
		if (conn->llcp_enc.pause_rx) {
			conn->llcp_terminate.reason_peer =
				BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;

			/* Mark for buffer for release */
			(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;
		}
#endif /* 0 */
#endif /* CONFIG_BT_CTLR_LE_ENC */
		break;

	case PDU_DATA_LLID_RESV:
	default:
#if defined(CONFIG_BT_CTLR_LE_ENC)
		/*
		 * EGON TODO: use correct data structure
		 * or more likely this is handled in ull_llcp.c
		 */
#if 0
		if (conn->llcp_enc.pause_rx) {
			conn->llcp_terminate.reason_peer =
				BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;
		}
#endif /* 0 */
#endif /* CONFIG_BT_CTLR_LE_ENC */

		/* Invalid LL id, drop it. */

		/* Mark for buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_DC_PDU_RELEASE;

		break;
	}


	return 0;
}

uint8_t ull_conn_llcp_req(void *conn)
{
	return 0;
}

int ull_conn_llcp(struct ull_cp_conn *conn, uint32_t ticks_at_expire, uint16_t lazy)
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
	return;
}

void ull_conn_tx_lll_enqueue(struct ull_cp_conn *conn, uint8_t count)
{
	return;
}

void ull_conn_link_tx_release(void *link)
{
	return;
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

struct ull_cp_conn *ull_conn_tx_ack(uint16_t handle, memq_link_t *link,
				struct node_tx *tx)
{
	return NULL;
}
static uint16_t init_reset(void)
{
	/* Initialize conn pool. */
	mem_init(conn_pool, sizeof(struct ull_cp_conn),
		 sizeof(conn_pool) / sizeof(struct ull_cp_conn), &conn_free);

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

static inline void disable(uint16_t handle)
{
	volatile uint32_t ret_cb = TICKER_STATUS_BUSY;
	struct ull_cp_conn *conn;
	void *mark;
	uint32_t ret;

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

	/*
	 * EGON TODO: add proper assignment when data structure
	 * is in place
	 */
#if 0
	conn->lll.link_tx_free = NULL;
#endif

	mark = ull_disable_unmark(conn);
	LL_ASSERT(mark == conn);
}

static inline int ctrl_rx(memq_link_t *link, struct node_rx_pdu **rx,
			  struct pdu_data *pdu_rx, struct ull_cp_conn *conn)
{
	/*
	 * EGON TODO: this is part of the receiving of PDUs,
	 * to be filled in later
	 */
	return 0;
}
