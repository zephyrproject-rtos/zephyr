/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr.h>
#include <bluetooth/bluetooth.h>
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
#include "ll_feat.h"

#include "lll.h"
#include "lll_conn.h"

#include "ull_conn_llcp_internal.h"
#include "ull_tx_queue.h"
#include "ull_llcp.h"

#include "ull_conn_types.h"
#include "ull_llcp_features.h"
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

static void ticker_update_conn_op_cb(uint32_t status, void *param);
static void ticker_op_stop_cb(uint32_t status, void *param);

static inline void disable(uint16_t handle);
static void conn_cleanup(struct ll_conn *conn, uint8_t reason);
static struct node_tx *tx_ull_dequeue(struct ll_conn *conn);
static void tx_ull_flush(struct ll_conn *conn);
static void tx_lll_flush(void *param);

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
static int empty_data_start_release(struct ll_conn *conn, struct node_tx *tx);
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

#if !defined(BT_CTLR_USER_TX_BUFFER_OVERHEAD)
#define BT_CTLR_USER_TX_BUFFER_OVERHEAD 0
#endif /* BT_CTLR_USER_TX_BUFFER_OVERHEAD */

#define CONN_TX_BUF_SIZE MROUND(offsetof(struct node_tx, pdu) + \
				offsetof(struct pdu_data, lldata) + \
				(CONFIG_BT_BUF_ACL_TX_SIZE + \
				BT_CTLR_USER_TX_BUFFER_OVERHEAD))

static MFIFO_DEFINE(conn_tx, sizeof(struct lll_tx), CONFIG_BT_BUF_ACL_TX_COUNT);
static MFIFO_DEFINE(conn_ack, sizeof(struct lll_tx),CONFIG_BT_BUF_ACL_TX_COUNT);


static struct {
	void *free;
	uint8_t pool[CONN_TX_BUF_SIZE * CONFIG_BT_BUF_ACL_TX_COUNT];
} mem_conn_tx;

/* TODO(thoh): What is the correct size for this pool ? */
static struct {
	void *free;
	uint8_t pool[sizeof(memq_link_t) * CONFIG_BT_BUF_ACL_TX_COUNT];
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

uint8_t ull_conn_mfifo_get_tx(void **lll_tx)
{
	return MFIFO_ENQUEUE_GET(conn_tx, lll_tx);
}

void ull_conn_mfifo_enqueue_tx(uint8_t idx)
{
	MFIFO_ENQUEUE(conn_tx, idx);
}

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

/**
 * @brief Restart procedure timeout 'timer'
 */
void ull_conn_prt_reload(struct ll_conn *conn, uint16_t procedure_reload)
{
	conn->procedure_expire = procedure_reload;
}

/**
 * @brief Clear procedure timeout 'timer'
 */
void ull_conn_prt_clear(struct ll_conn *conn)
{
	conn->procedure_expire = 0U;
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
static inline void dle_max_time_get(struct ll_conn *conn,
				    uint16_t *max_rx_time, uint16_t *max_tx_time)
{
	uint16_t rx_time = 0;
	uint16_t tx_time = 0;
	uint8_t phy_select = PHY_1M;

#if defined(CONFIG_BT_CTLR_PHY)
	if (conn->llcp.fex.valid && feature_phy_coded(conn)) {
		/* If coded PHY is supported on the connection
		 * this will define the max times
		 */
		phy_select = PHY_CODED;
		/* If not, max times should be defined by 1M timing */
	}
#endif

	rx_time = PKT_US(LL_LENGTH_OCTETS_RX_MAX, phy_select);

#if defined(CONFIG_BT_CTLR_PHY)
	tx_time = MIN(conn->lll.dle.local.max_tx_time, PKT_US(LL_LENGTH_OCTETS_RX_MAX, phy_select));
#else /* !CONFIG_BT_CTLR_PHY */
	tx_time = PKT_US(conn->lll.dle.local.max_tx_octets, phy_select);
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

uint16_t ull_conn_default_tx_octets_get(void)
{
	return default_tx_octets;
}

uint16_t ull_conn_default_tx_time_get(void)
{
#if defined(CONFIG_BT_CTLR_PHY)
	return default_tx_time;
#else
	return PKT_US(default_tx_octets, PHY_1M);
#endif /* CONFIG_BT_CTLR_PHY */
}

uint8_t ull_dle_update_eff(struct ll_conn *conn)
{
	uint8_t dle_changed = 0;

	const uint16_t eff_tx_octets = MIN(conn->lll.dle.local.max_tx_octets, conn->lll.dle.remote.max_rx_octets);
	const uint16_t eff_rx_octets = MIN(conn->lll.dle.local.max_rx_octets, conn->lll.dle.remote.max_tx_octets);
#if defined(CONFIG_BT_CTLR_PHY)
	const uint16_t eff_tx_time = MIN(conn->lll.dle.local.max_tx_time, conn->lll.dle.remote.max_rx_time);
	const uint16_t eff_rx_time = MIN(conn->lll.dle.local.max_rx_time, conn->lll.dle.remote.max_tx_time);

	if (eff_tx_time != conn->lll.dle.eff.max_tx_time) {
		conn->lll.dle.eff.max_tx_time = eff_tx_time;
		dle_changed = 1;
	}
	if (eff_rx_time != conn->lll.dle.eff.max_rx_time) {
		conn->lll.dle.eff.max_rx_time = eff_rx_time;
		dle_changed = 1;
	}
#else
	conn->lll.dle.eff.max_rx_time = PKT_US(eff_rx_octets, PHY_1M);
	conn->lll.dle.eff.max_tx_time = PKT_US(eff_tx_octets, PHY_1M);
#endif

	if (eff_tx_octets != conn->lll.dle.eff.max_tx_octets) {
		conn->lll.dle.eff.max_tx_octets = eff_tx_octets;
		dle_changed = 1;
	}
	if (eff_rx_octets != conn->lll.dle.eff.max_rx_octets) {
		conn->lll.dle.eff.max_rx_octets = eff_rx_octets;
		dle_changed = 1;
	}

	return dle_changed;
}

void ull_dle_local_tx_update(struct ll_conn *conn, uint16_t tx_octets, uint16_t tx_time)
{
	conn->lll.dle.local.max_tx_octets = tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
	conn->lll.dle.local.max_tx_time = tx_time;
#endif /* CONFIG_BT_CTLR_PHY */

	dle_max_time_get(conn, &conn->lll.dle.local.max_rx_time, &conn->lll.dle.local.max_tx_time);
}

void ull_dle_init(struct ll_conn *conn, uint8_t phy)
{
#if defined(CONFIG_BT_CTLR_PHY)
	const uint16_t max_time_min = PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, phy);
	const uint16_t max_time_max = PKT_US(LL_LENGTH_OCTETS_RX_MAX, phy);
#endif

	/* Clear DLE data set */
	memset(&conn->lll.dle, 0, sizeof(conn->lll.dle));
	/* See BT. 5.2 Spec - Vol 6, Part B, Sect 4.5.10
	 * Default to locally max supported rx/tx length/time
	 */
	ull_dle_local_tx_update(conn, ull_conn_default_tx_octets_get(), ull_conn_default_tx_time_get());

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

	ull_dle_update_eff(conn);

	/* Check whether the controller should perform a data length update after
	 * connection is established
	 */
#if defined(CONFIG_BT_CTLR_PHY)
	if ((conn->lll.dle.local.max_rx_time != max_time_min ||
		 conn->lll.dle.local.max_tx_time != max_time_min)) {
		conn->lll.dle.update = 1;
	} else
#endif
	if (conn->lll.dle.local.max_tx_octets != PDU_DC_PAYLOAD_SIZE_MIN ||
		conn->lll.dle.local.max_rx_octets != PDU_DC_PAYLOAD_SIZE_MIN)
	{
		conn->lll.dle.update = 1;
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

	/*
	 * EGON Todo: this functions has changed in ull_conn.c;
	 * we need to update
	 */

	/* Store the link in the node rx so that when done event is
	 * processed it can be used to enqueue node rx towards LL context
	 */
	rx->link = link;
	ftr = &(rx->rx_ftr);

	lll = *((struct lll_conn **)((uint8_t *)ftr->param +
				     sizeof(struct lll_hdr)));
	switch (lll->role) {
#if defined(CONFIG_BT_CENTRAL)
	case 0:
		ull_master_setup(rx, ftr, lll);
		break;
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
	case 1:
		ull_slave_setup(rx, ftr, lll);
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
		ull_cp_rx(conn, *rx);
		/* Mark buffer for release */
		(*rx)->hdr.type = NODE_RX_TYPE_RELEASE;
		return 0;
	}

	case PDU_DATA_LLID_DATA_CONTINUE:
	case PDU_DATA_LLID_DATA_START:
#if defined(CONFIG_BT_CTLR_LE_ENC)
		if (conn->pause_rx_data) {
			/* Data PDU recieved during data pause */
			conn->terminate.reason =
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
			/* Data PDU recieved during data pause */
			conn->terminate.reason =
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

	ull_cp_run(conn);

	return 0;
}

void ull_conn_done(struct node_rx_event_done *done)
{
	uint32_t ticks_drift_minus;
	uint32_t ticks_drift_plus;
	uint16_t latency_event;
	uint16_t elapsed_event;
	struct lll_conn *lll;
	struct ll_conn *conn;
	uint8_t reason_final;
	uint16_t lazy;
	uint8_t force;

	/* Get reference to ULL context */
	conn = CONTAINER_OF(done->param, struct ll_conn, ull);
	lll = &conn->lll;

	/* Skip if connection terminated by local host */
	if (unlikely(lll->handle == 0xFFFF)) {
		return;
	}

#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* Check authenticated payload expiry or MIC failure */
	switch (done->extra.mic_state) {
	case LLL_CONN_MIC_NONE:
#if defined(CONFIG_BT_CTLR_LE_PING)
		if (lll->enc_rx || ull_cp_encryption_paused(conn)) {
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
		conn->terminate.reason =
			BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;
		break;
	}
#endif /* CONFIG_BT_CTLR_LE_ENC */

	/* Master transmitted ack for the received terminate ind or
	 * Slave received terminate ind or MIC failure
	 */
	reason_final = conn->terminate.reason;
	if (reason_final && (
#if defined(CONFIG_BT_PERIPHERAL)
			    lll->role ||
#else /* CONFIG_BT_PERIPHERAL */
			    0 ||
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CENTRAL)
			    conn->master.terminate_ack ||
			    (reason_final == BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL)
#else /* CONFIG_BT_CENTRAL */
			    1
#endif /* CONFIG_BT_CENTRAL */
			    )) {
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
			ull_drift_ticks_get(done, &ticks_drift_plus,
					    &ticks_drift_minus);

			/*
			** EGON TODO to be verified
			*/
			if (!ull_tx_q_peek(&conn->tx_q)) {
				ull_conn_tx_demux(UINT8_MAX);
			}

			if (ull_tx_q_peek(&conn->tx_q) || memq_peek(lll->memq_tx.head,
						       lll->memq_tx.tail,
						       NULL)) {
				lll->latency_event = 0;
			} else if (lll->slave.latency_enabled) {
				lll->latency_event = lll->latency;
			}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
		} else if (reason_final) {
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
			lll->latency_event = 0U;

			/* Force both master and slave when close to
			 * supervision timeout.
			 */
			if (conn->supervision_expire <= 6U) {
				force = 1U;
			}
#if defined(CONFIG_BT_CTLR_CONN_RANDOM_FORCE)
			/* use randomness to force slave role when anchor
			 * points are being missed.
			 */
			else if (lll->role) {
				if (latency_event) {
					force = 1U;
				} else {
					force = conn->slave.force & 0x01;

					/* rotate force bits */
					conn->slave.force >>= 1U;
					if (force) {
						conn->slave.force |= BIT(31);
					}
				}
			}
#endif /* CONFIG_BT_CTLR_CONN_RANDOM_FORCE */
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

			/* Initiate LE_PING procedure */
			ull_cp_le_ping(conn);
		}
	}
#endif /* CONFIG_BT_CTLR_LE_PING */

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
			ll_rx_put(rx->hdr.link, rx);
			ll_rx_sched();
		}
	}
#endif /* CONFIG_BT_CTLR_CONN_RSSI_EVENT */

	/*
	** EGON TODO: verify what we need to do here
	*/


	/* break latency based on ctrl procedure pending */
	/*
	** Original code
	if (((((conn->llcp_req - conn->llcp_ack) & 0x03) == 0x02) &&
	     ((conn->llcp_type == LLCP_CONN_UPD) ||
	      (conn->llcp_type == LLCP_CHAN_MAP))) ||
	    (conn->llcp_cu.req != conn->llcp_cu.ack)) {
		lll->latency_event = 0U;
	}
	*/

	/* check if latency needs update */
	lazy = 0U;
	if ((force) || (latency_event != lll->latency_event)) {
		lazy = lll->latency_event + 1U;
	}

	/* update conn ticker */
	if (ticks_drift_plus || ticks_drift_minus || lazy || force) {
		uint8_t ticker_id = TICKER_ID_CONN_BASE + lll->handle;
		struct ll_conn *conn = lll->hdr.parent;
		uint32_t ticker_status;

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

		tx = tx_ull_dequeue(conn);
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
	return mfifo_conn_ack.l;
}

memq_link_t *ull_conn_ack_peek(uint8_t *ack_last, uint16_t *handle,
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

memq_link_t *ull_conn_ack_by_last_peek(uint8_t last, uint16_t *handle,
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
		if (handle != 0xFFFF) {
			struct ll_conn *conn = ll_conn_get(handle);

			ull_cp_tx_ack(conn, tx);
		}

		/* If the link's next pointer points to the tx node itself,
		 * then the tx node belong to the ctrl pool and should be
		 * free'ed (see tx_ull_dequeue) */
		if (link->next == (void *)tx) {
			LL_ASSERT(link->next);
			ull_cp_release_tx(tx);
			return;
		} else {
			LL_ASSERT(!link->next);
		}
	} else if (handle == 0xFFFF) {
		pdu_tx->ll_id = PDU_DATA_LLID_RESV;
	} else {
		LL_ASSERT(handle != 0xFFFF);
	}

	ll_tx_ack_put(handle, tx);

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

uint8_t ull_conn_lll_phy_active(struct ll_conn *conn, uint8_t phys)
{
#if defined(CONFIG_BT_CTLR_PHY)
	if (!(phys & (conn->lll.phy_tx |
			 conn->lll.phy_rx))) {
#else /* !CONFIG_BT_CTLR_PHY */
	if (!(phys & 0x01)) {
#endif /* !CONFIG_BT_CTLR_PHY */
		return 0;
	}
	return 1;
}

static int init_reset(void)
{
	/* Initialize conn pool. */
	mem_init(mem_conn.pool, sizeof(struct ll_conn),
		 sizeof(mem_conn.pool) / sizeof(struct ll_conn), &mem_conn.free);

	/* Initialize tx pool. */
	mem_init(mem_conn_tx.pool, CONN_TX_BUF_SIZE, CONFIG_BT_BUF_ACL_TX_COUNT,
		 &mem_conn_tx.free);

	/* Initialize tx link pool. */
	mem_init(mem_link_tx.pool, sizeof(memq_link_t),
		 CONFIG_BT_BUF_ACL_TX_COUNT, &mem_link_tx.free);

	/* Initialize control procedure system. */
	ull_cp_init();

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

static void ticker_update_conn_op_cb(uint32_t status, void *param)
{
	/* Slave drift compensation succeeds, or it fails in a race condition
	 * when disconnecting or connection update (race between ticker_update
	 * and ticker_stop calls).
	 */
	LL_ASSERT(status == TICKER_STATUS_SUCCESS ||
		  param == ull_update_mark_get() ||
		  param == ull_disable_mark_get());
}

static void ticker_op_stop_cb(uint32_t status, void *param)
{
	uint32_t retval;
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, tx_lll_flush};

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	mfy.param = param;

	/* Flush pending tx PDUs in LLL (using a mayfly) */
	retval = mayfly_enqueue(TICKER_USER_ID_ULL_LOW, TICKER_USER_ID_LLL, 0,
				&mfy);
	LL_ASSERT(!retval);
}

static inline void disable(uint16_t handle)
{
	struct ll_conn *conn;
	int err;

	conn = ll_conn_get(handle);

	err = ull_ticker_stop_with_mark(TICKER_ID_CONN_BASE + handle,
					conn, &conn->lll);
	LL_ASSERT(err == 0 || err == -EALREADY);

	conn->lll.link_tx_free = NULL;
}

static void conn_cleanup(struct ll_conn *conn, uint8_t reason)
{
	struct lll_conn *lll = &conn->lll;
	struct node_rx_pdu *rx;
	uint32_t ticker_status;

	/* Only termination structure is populated here in ULL context
	 * but the actual enqueue happens in the LLL context in
	 * tx_lll_flush. The reason being to avoid passing the reason
	 * value and handle through the mayfly scheduling of the
	 * tx_lll_flush.
	 */
	rx = (void *)&conn->terminate.node_rx;
	rx->hdr.handle = conn->lll.handle;
	rx->hdr.type = NODE_RX_TYPE_TERMINATE;
	*((uint8_t *)rx->pdu) = reason;

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

static struct node_tx *tx_ull_dequeue(struct ll_conn *conn)
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

static void tx_ull_flush(struct ll_conn *conn)
{
	struct node_tx *tx;

	tx = tx_ull_dequeue(conn);
	while (tx) {
		memq_link_t *link;

		link = mem_acquire(&mem_link_tx.free);
		LL_ASSERT(link);

		/* Enqueue towards LLL */
		memq_enqueue(link, tx, &conn->lll.memq_tx.tail);

		tx = tx_ull_dequeue(conn);
	}
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
		struct lll_tx *lll_tx;
		uint8_t idx;

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
	rx = (void *)&conn->terminate.node_rx;
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
