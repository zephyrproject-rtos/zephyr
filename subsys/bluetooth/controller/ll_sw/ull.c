/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdbool.h>
#include <errno.h>

#include <zephyr/types.h>
#include <device.h>
#include <drivers/entropy.h>
#include <bluetooth/hci.h>

#include "hal/cntr.h"
#include "hal/ccm.h"
#include "hal/ticker.h"

#if defined(CONFIG_SOC_FAMILY_NRF)
#include "hal/radio.h"
#endif /* CONFIG_SOC_FAMILY_NRF */

#include "util/util.h"
#include "util/mem.h"
#include "util/mfifo.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "ll.h"
#include "ll_feat.h"
#include "lll.h"
#include "lll_adv.h"
#include "lll_scan.h"
#include "lll_conn.h"
#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_conn_types.h"
#include "ull_filter.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_scan_internal.h"
#include "ull_conn_internal.h"

#if defined(CONFIG_BT_CTLR_USER_EXT)
#include "ull_vendor.h"
#endif /* CONFIG_BT_CTLR_USER_EXT */

#define LOG_MODULE_NAME bt_ctlr_llsw_ull
#include "common/log.h"
#include "hal/debug.h"

/* Define ticker nodes and user operations */
#if defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
#define TICKER_USER_LLL_OPS      (3 + 1)
#else
#define TICKER_USER_LLL_OPS      (2 + 1)
#endif /* CONFIG_BT_CTLR_LOW_LAT */

#if !defined(TICKER_USER_ULL_HIGH_VENDOR_OPS)
#define TICKER_USER_ULL_HIGH_VENDOR_OPS 0
#endif /* TICKER_USER_ULL_HIGH_VENDOR_OPS */

#define TICKER_USER_ULL_HIGH_OPS (3 + TICKER_USER_ULL_HIGH_VENDOR_OPS + 1)
#define TICKER_USER_ULL_LOW_OPS  (1 + 1)
#define TICKER_USER_THREAD_OPS   (1 + 1)

#if defined(CONFIG_BT_BROADCASTER)
#define BT_ADV_TICKER_NODES ((TICKER_ID_ADV_LAST) - (TICKER_ID_ADV_STOP) + 1)
#else
#define BT_ADV_TICKER_NODES 0
#endif

#if defined(CONFIG_BT_OBSERVER)
#define BT_SCAN_TICKER_NODES ((TICKER_ID_SCAN_LAST) - (TICKER_ID_SCAN_STOP) + 1)
#else
#define BT_SCAN_TICKER_NODES 0
#endif

#if defined(CONFIG_BT_CONN)
#define BT_CONN_TICKER_NODES ((TICKER_ID_CONN_LAST) - (TICKER_ID_CONN_BASE) + 1)
#else
#define BT_CONN_TICKER_NODES 0
#endif

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
#define FLASH_TICKER_NODES        1 /* No. of tickers reserved for flashing */
#define FLASH_TICKER_USER_APP_OPS 1 /* No. of additional ticker operations */
#else
#define FLASH_TICKER_NODES        0
#define FLASH_TICKER_USER_APP_OPS 0
#endif

#define TICKER_NODES              (TICKER_ID_ULL_BASE + \
				   BT_ADV_TICKER_NODES + \
				   BT_SCAN_TICKER_NODES + \
				   BT_CONN_TICKER_NODES + \
				   FLASH_TICKER_NODES)
#define TICKER_USER_APP_OPS       (TICKER_USER_THREAD_OPS + \
				   FLASH_TICKER_USER_APP_OPS)
#define TICKER_USER_OPS           (TICKER_USER_LLL_OPS + \
				   TICKER_USER_ULL_HIGH_OPS + \
				   TICKER_USER_ULL_LOW_OPS + \
				   TICKER_USER_THREAD_OPS + \
				   FLASH_TICKER_USER_APP_OPS)

/* Memory for ticker nodes/instances */
static u8_t MALIGN(4) ticker_nodes[TICKER_NODES][TICKER_NODE_T_SIZE];

/* Memory for users/contexts operating on ticker module */
static u8_t MALIGN(4) ticker_users[MAYFLY_CALLER_COUNT][TICKER_USER_T_SIZE];

/* Memory for user/context simultaneous API operations */
static u8_t MALIGN(4) ticker_user_ops[TICKER_USER_OPS][TICKER_USER_OP_T_SIZE];

/* Semaphire to wakeup thread on ticker API callback */
static struct k_sem sem_ticker_api_cb;

/* Semaphore to wakeup thread on Rx-ed objects */
static struct k_sem *sem_recv;

/* Declare prepare-event FIFO: mfifo_prep.
 * Queue of struct node_rx_event_done
 */
static MFIFO_DEFINE(prep, sizeof(struct lll_event), EVENT_PIPELINE_MAX);

/* Declare done-event FIFO: mfifo_done.
 * Queue of pointers to struct node_rx_event_done.
 * The actual backing behind these pointers is mem_done
 */
static MFIFO_DEFINE(done, sizeof(struct node_rx_event_done *), EVENT_DONE_MAX);

/* Backing storage for elements in mfifo_done */
static struct {
	void *free;
	u8_t pool[sizeof(struct node_rx_event_done) * EVENT_DONE_MAX];
} mem_done;

static struct {
	void *free;
	u8_t pool[sizeof(memq_link_t) * EVENT_DONE_MAX];
} mem_link_done;

#if defined(CONFIG_BT_CTLR_PHY) && defined(CONFIG_BT_CTLR_DATA_LENGTH)
#define LL_PDU_RX_CNT 3
#else
#define LL_PDU_RX_CNT 2
#endif

#define PDU_RX_CNT    (CONFIG_BT_CTLR_RX_BUFFERS + 3)
#define RX_CNT        (PDU_RX_CNT + LL_PDU_RX_CNT)

static MFIFO_DEFINE(pdu_rx_free, sizeof(void *), PDU_RX_CNT);

#if defined(CONFIG_BT_RX_USER_PDU_LEN)
#define PDU_RX_USER_PDU_OCTETS_MAX (CONFIG_BT_RX_USER_PDU_LEN)
#else
#define PDU_RX_USER_PDU_OCTETS_MAX 0
#endif
#define NODE_RX_HEADER_SIZE      (offsetof(struct node_rx_pdu, pdu))
#define NODE_RX_STRUCT_OVERHEAD  (NODE_RX_HEADER_SIZE)

#define PDU_ADVERTIZE_SIZE (PDU_AC_SIZE_MAX + PDU_AC_SIZE_EXTRA)
#define PDU_DATA_SIZE      (PDU_DC_LL_HEADER_SIZE + LL_LENGTH_OCTETS_RX_MAX)

#define PDU_RX_NODE_POOL_ELEMENT_SIZE                         \
	MROUND(                                               \
		NODE_RX_STRUCT_OVERHEAD                       \
		+ MAX(MAX(PDU_ADVERTIZE_SIZE,                 \
			  PDU_DATA_SIZE),                     \
		      PDU_RX_USER_PDU_OCTETS_MAX)              \
	)

/* When both central and peripheral are supported, one each Rx node will be
 * needed by connectable advertising and the initiator to generate connection
 * complete event, hence conditionally set the count.
 */
#if defined(CONFIG_BT_MAX_CONN)
#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_PERIPHERAL)
#define BT_CTLR_MAX_CONNECTABLE 2
#else
#define BT_CTLR_MAX_CONNECTABLE 1
#endif
#define BT_CTLR_MAX_CONN        CONFIG_BT_MAX_CONN
#else
#define BT_CTLR_MAX_CONNECTABLE 0
#define BT_CTLR_MAX_CONN        0
#endif

#define PDU_RX_POOL_SIZE (PDU_RX_NODE_POOL_ELEMENT_SIZE * \
			  (RX_CNT + BT_CTLR_MAX_CONNECTABLE))

static struct {
	void *free;
	u8_t pool[PDU_RX_POOL_SIZE];
} mem_pdu_rx;

#define LINK_RX_POOL_SIZE (sizeof(memq_link_t) * (RX_CNT + 2 + \
						  BT_CTLR_MAX_CONN))
static struct {
	u8_t quota_pdu; /* Number of un-utilized buffers */

	void *free;
	u8_t pool[LINK_RX_POOL_SIZE];
} mem_link_rx;

static MEMQ_DECLARE(ull_rx);
static MEMQ_DECLARE(ll_rx);

#if defined(CONFIG_BT_CONN)
static MFIFO_DEFINE(ll_pdu_rx_free, sizeof(void *), LL_PDU_RX_CNT);
static MFIFO_DEFINE(tx_ack, sizeof(struct lll_tx),
		    CONFIG_BT_CTLR_TX_BUFFERS);

static void *mark_update;
#endif /* CONFIG_BT_CONN */

static void *mark_disable;

static inline int init_reset(void);
static void perform_lll_reset(void *param);
static inline void *mark_set(void **m, void *param);
static inline void *mark_unset(void **m, void *param);
static inline void *mark_get(void *m);
static inline void done_alloc(void);
static inline void rx_alloc(u8_t max);
static void rx_demux(void *param);
static inline int rx_demux_rx(memq_link_t *link, struct node_rx_hdr *rx);
static inline void rx_demux_event_done(memq_link_t *link,
				       struct node_rx_hdr *rx);
static inline void ll_rx_link_inc_quota(int8_t delta);
static void disabled_cb(void *param);

#if defined(CONFIG_BT_CONN)
static u8_t tx_cmplt_get(u16_t *handle, u8_t *first, u8_t last);
#endif /* CONFIG_BT_CONN */

int ll_init(struct k_sem *sem_rx)
{
	int err;

	/* Store the semaphore to be used to wakeup Thread context */
	sem_recv = sem_rx;

	/* Initialize counter */
	/* TODO: Bind and use counter driver? */
	cntr_init();

	/* Initialize Mayfly */
	mayfly_init();

	/* Initialize Ticker */
	ticker_users[MAYFLY_CALL_ID_0][0] = TICKER_USER_LLL_OPS;
	ticker_users[MAYFLY_CALL_ID_1][0] = TICKER_USER_ULL_HIGH_OPS;
	ticker_users[MAYFLY_CALL_ID_2][0] = TICKER_USER_ULL_LOW_OPS;
	ticker_users[MAYFLY_CALL_ID_PROGRAM][0] = TICKER_USER_APP_OPS;

	err = ticker_init(TICKER_INSTANCE_ID_CTLR,
			  TICKER_NODES, &ticker_nodes[0],
			  MAYFLY_CALLER_COUNT, &ticker_users[0],
			  TICKER_USER_OPS, &ticker_user_ops[0],
			  hal_ticker_instance0_caller_id_get,
			  hal_ticker_instance0_sched,
			  hal_ticker_instance0_trigger_set);
	LL_ASSERT(!err);

	/* Initialize semaphore for ticker API blocking wait */
	k_sem_init(&sem_ticker_api_cb, 0, 1);

	/* Initialize LLL */
	err = lll_init();
	if (err) {
		return err;
	}

	/* Initialize ULL internals */
	/* TODO: globals? */

	/* Common to init and reset */
	err = init_reset();
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_BROADCASTER)
	err = lll_adv_init();
	if (err) {
		return err;
	}

	err = ull_adv_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	err = lll_scan_init();
	if (err) {
		return err;
	}

	err = ull_scan_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
	err = lll_conn_init();
	if (err) {
		return err;
	}

	err = ull_conn_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_USER_EXT)
	err = ull_user_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CTLR_USER_EXT */

	/* reset whitelist, resolving list and initialise RPA timeout*/
	if (IS_ENABLED(CONFIG_BT_CTLR_FILTER)) {
		ull_filter_reset(true);
	}

	return  0;
}

void ll_reset(void)
{
	int err;

#if defined(CONFIG_BT_BROADCASTER)
	/* Reset adv state */
	err = ull_adv_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	/* Reset scan state */
	err = ull_scan_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CENTRAL)
	/* Reset initiator */
	{
		void *rx;

		err = ll_connect_disable(&rx);
		if (!err) {
			struct ll_scan_set *scan;

			scan = ull_scan_is_enabled_get(0);
			LL_ASSERT(scan);

			scan->is_enabled = 0U;
			scan->lll.conn = NULL;
		}

		ARG_UNUSED(rx);
	}
#endif /* CONFIG_BT_CENTRAL */

	/* Reset conn role */
	err = ull_conn_reset();
	LL_ASSERT(!err);

	MFIFO_INIT(tx_ack);
#endif /* CONFIG_BT_CONN */

	/* reset whitelist and resolving list */
	if (IS_ENABLED(CONFIG_BT_CTLR_FILTER)) {
		ull_filter_reset(false);
	}

	/* Re-initialize ULL internals */

	/* Re-initialize the prep mfifo */
	MFIFO_INIT(prep);

	/* Re-initialize the free done mfifo */
	MFIFO_INIT(done);

	/* Re-initialize the free rx mfifo */
	MFIFO_INIT(pdu_rx_free);

#if defined(CONFIG_BT_CONN)
	/* Re-initialize the free ll rx mfifo */
	MFIFO_INIT(ll_pdu_rx_free);
#endif /* CONFIG_BT_CONN */

	/* Reset LLL via mayfly */
	{
		u32_t retval;
		static memq_link_t link;
		static struct mayfly mfy = {0, 0, &link, NULL,
					    perform_lll_reset};

		mfy.param = NULL;
		retval = mayfly_enqueue(TICKER_USER_ID_THREAD,
					TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!retval);
	}

	/* Common to init and reset */
	err = init_reset();
	LL_ASSERT(!err);
}

/**
 * @brief Peek the next node_rx to send up to Host
 * @details Tightly coupled with prio_recv_thread()
 *   Execution context: Controller thread
 *
 * @param node_rx[out]   Pointer to rx node at head of queue
 * @param handle[out]    Connection handle
 * @return TX completed
 */
u8_t ll_rx_get(void **node_rx, u16_t *handle)
{
	struct node_rx_hdr *rx;
	memq_link_t *link;
	u8_t cmplt = 0U;

#if defined(CONFIG_BT_CONN)
ll_rx_get_again:
#endif /* CONFIG_BT_CONN */

	*node_rx = NULL;

	link = memq_peek(memq_ll_rx.head, memq_ll_rx.tail, (void **)&rx);
	if (link) {
#if defined(CONFIG_BT_CONN)
		cmplt = tx_cmplt_get(handle, &mfifo_tx_ack.f, rx->ack_last);
		if (!cmplt) {
			u8_t f, cmplt_prev, cmplt_curr;
			u16_t h;

			cmplt_curr = 0U;
			f = mfifo_tx_ack.f;
			do {
				cmplt_prev = cmplt_curr;
				cmplt_curr = tx_cmplt_get(&h, &f,
							  mfifo_tx_ack.l);
			} while ((cmplt_prev != 0U) ||
				 (cmplt_prev != cmplt_curr));

			/* Do not send up buffers to Host thread that are
			 * marked for release
			 */
			if (rx->type == NODE_RX_TYPE_DC_PDU_RELEASE) {
				(void)memq_dequeue(memq_ll_rx.tail,
						   &memq_ll_rx.head, NULL);
				mem_release(link, &mem_link_rx.free);

				ll_rx_link_inc_quota(1);

				mem_release(rx, &mem_pdu_rx.free);

				rx_alloc(1);

				goto ll_rx_get_again;
			}
#endif /* CONFIG_BT_CONN */

			*node_rx = rx;

#if defined(CONFIG_BT_CONN)
		}
	} else {
		cmplt = tx_cmplt_get(handle, &mfifo_tx_ack.f, mfifo_tx_ack.l);
#endif /* CONFIG_BT_CONN */
	}

	return cmplt;
}

/**
 * @brief Commit the dequeue from memq_ll_rx, where ll_rx_get() did the peek
 * @details Execution context: Controller thread
 */
void ll_rx_dequeue(void)
{
	struct node_rx_hdr *rx = NULL;
	memq_link_t *link;

	link = memq_dequeue(memq_ll_rx.tail, &memq_ll_rx.head,
			    (void **)&rx);
	LL_ASSERT(link);

	mem_release(link, &mem_link_rx.free);

	/* handle object specific clean up */
	switch (rx->type) {
#if defined(CONFIG_BT_CONN)
	case NODE_RX_TYPE_CONNECTION:
	{
		struct node_rx_cc *cc = (void *)((struct node_rx_pdu *)rx)->pdu;
		struct node_rx_ftr *ftr = &(rx->rx_ftr);

		if (0) {

#if defined(CONFIG_BT_PERIPHERAL)
		} else if ((cc->status == BT_HCI_ERR_ADV_TIMEOUT) || cc->role) {
			struct lll_adv *lll = ftr->param;
			struct ll_adv_set *adv = (void *)HDR_LLL2EVT(lll);

			if (cc->status == BT_HCI_ERR_ADV_TIMEOUT) {
				struct lll_conn *conn_lll;
				struct ll_conn *conn;
				memq_link_t *link;

				conn_lll = lll->conn;
				LL_ASSERT(conn_lll);

				LL_ASSERT(!conn_lll->link_tx_free);
				link = memq_deinit(&conn_lll->memq_tx.head,
						   &conn_lll->memq_tx.tail);
				LL_ASSERT(link);
				conn_lll->link_tx_free = link;

				conn = (void *)HDR_LLL2EVT(conn_lll);
				ll_conn_release(conn);

				lll->conn = NULL;
			} else {
				/* Release un-utilized node rx */
				if (adv->node_rx_cc_free) {
					void *rx_free;

					rx_free = adv->node_rx_cc_free;
					adv->node_rx_cc_free = NULL;

					ll_rx_release(rx_free);
				}
			}

			adv->is_enabled = 0U;
#else /* !CONFIG_BT_PERIPHERAL */
			ARG_UNUSED(cc);
#endif /* !CONFIG_BT_PERIPHERAL */

		} else if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			struct lll_scan *lll = ftr->param;
			struct ll_scan_set *scan = (void *)HDR_LLL2EVT(lll);

			scan->is_enabled = 0U;
		} else {
			LL_ASSERT(0);
		}

		if (IS_ENABLED(CONFIG_BT_CTLR_PRIVACY)) {
			u8_t bm;

			bm = (IS_ENABLED(CONFIG_BT_OBSERVER) &&
			      ull_scan_is_enabled(0) << 1) |
			     (IS_ENABLED(CONFIG_BT_BROADCASTER) &&
			      ull_adv_is_enabled(0));

			if (!bm) {
				ull_filter_adv_scan_state_cb(0);
			}
		}
	}
	break;

	case NODE_RX_TYPE_TERMINATE:
	case NODE_RX_TYPE_DC_PDU:
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_OBSERVER)
	case NODE_RX_TYPE_REPORT:
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	case NODE_RX_TYPE_EXT_1M_REPORT:
	case NODE_RX_TYPE_EXT_CODED_REPORT:
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CONN)
	case NODE_RX_TYPE_CONN_UPDATE:
	case NODE_RX_TYPE_ENC_REFRESH:

#if defined(CONFIG_BT_CTLR_LE_PING)
	case NODE_RX_TYPE_APTO:
#endif /* CONFIG_BT_CTLR_LE_PING */

	case NODE_RX_TYPE_CHAN_SEL_ALGO:

#if defined(CONFIG_BT_CTLR_PHY)
	case NODE_RX_TYPE_PHY_UPDATE:
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	case NODE_RX_TYPE_RSSI:
#endif /* CONFIG_BT_CTLR_CONN_RSSI */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	case NODE_RX_TYPE_PROFILE:
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
	case NODE_RX_TYPE_SCAN_INDICATION:
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	case NODE_RX_TYPE_MESH_ADV_CPLT:
	case NODE_RX_TYPE_MESH_REPORT:
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CTLR_USER_EXT)
	case NODE_RX_TYPE_USER_START ... NODE_RX_TYPE_USER_END:
#endif /* CONFIG_BT_CTLR_USER_EXT */

	/* fall through */

	/* Ensure that at least one 'case' statement is present for this
	 * code block.
	 */
	case NODE_RX_TYPE_NONE:
		LL_ASSERT(rx->type != NODE_RX_TYPE_NONE);
		break;

	default:
		LL_ASSERT(0);
		break;
	}

	/* FIXME: clean up when porting Mesh Ext. */
	if (0) {
#if defined(CONFIG_BT_HCI_MESH_EXT)
	} else if (rx->type == NODE_RX_TYPE_MESH_ADV_CPLT) {
		struct ll_adv_set *adv;
		struct ll_scan_set *scan;

		adv = ull_adv_is_enabled_get(0);
		LL_ASSERT(adv);
		adv->is_enabled = 0U;

		scan = ull_scan_is_enabled_get(0);
		LL_ASSERT(scan);

		scan->is_enabled = 0U;

		ll_adv_scan_state_cb(0);
#endif /* CONFIG_BT_HCI_MESH_EXT */
	}
}

void ll_rx_mem_release(void **node_rx)
{
	struct node_rx_hdr *rx;

	rx = *node_rx;
	while (rx) {
		struct node_rx_hdr *rx_free;

		rx_free = rx;
		rx = rx->next;

		switch (rx_free->type) {
#if defined(CONFIG_BT_CONN)
		case NODE_RX_TYPE_CONNECTION:
#if defined(CONFIG_BT_CENTRAL)
		{
			struct node_rx_pdu *rx = (void *)rx_free;

			if (*((u8_t *)rx->pdu) ==
			    BT_HCI_ERR_UNKNOWN_CONN_ID) {
				struct lll_conn *conn_lll;
				struct ll_scan_set *scan;
				struct ll_conn *conn;
				memq_link_t *link;

				scan = ull_scan_is_enabled_get(0);
				LL_ASSERT(scan);

				conn_lll = scan->lll.conn;
				LL_ASSERT(conn_lll);

				LL_ASSERT(!conn_lll->link_tx_free);
				link = memq_deinit(&conn_lll->memq_tx.head,
						   &conn_lll->memq_tx.tail);
				LL_ASSERT(link);
				conn_lll->link_tx_free = link;

				conn = (void *)HDR_LLL2EVT(conn_lll);
				ll_conn_release(conn);

				scan->is_enabled = 0U;

				scan->lll.conn = NULL;

#if defined(CONFIG_BT_CTLR_PRIVACY)
#if defined(CONFIG_BT_BROADCASTER)
				if (!ull_adv_is_enabled_get(0))
#endif
				{
					ull_filter_adv_scan_state_cb(0);
				}
#endif
				break;
			}
		}
#endif /* CONFIG_BT_CENTRAL */
		/* passthrough */
		case NODE_RX_TYPE_DC_PDU:
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_OBSERVER)
		case NODE_RX_TYPE_REPORT:
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		case NODE_RX_TYPE_EXT_1M_REPORT:
		case NODE_RX_TYPE_EXT_CODED_REPORT:
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CONN)
		case NODE_RX_TYPE_CONN_UPDATE:
		case NODE_RX_TYPE_ENC_REFRESH:

#if defined(CONFIG_BT_CTLR_LE_PING)
		case NODE_RX_TYPE_APTO:
#endif /* CONFIG_BT_CTLR_LE_PING */

		case NODE_RX_TYPE_CHAN_SEL_ALGO:

#if defined(CONFIG_BT_CTLR_PHY)
		case NODE_RX_TYPE_PHY_UPDATE:
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		case NODE_RX_TYPE_RSSI:
#endif /* CONFIG_BT_CTLR_CONN_RSSI */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
		case NODE_RX_TYPE_PROFILE:
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
		case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
		case NODE_RX_TYPE_SCAN_INDICATION:
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */

#if defined(CONFIG_BT_HCI_MESH_EXT)
		case NODE_RX_TYPE_MESH_ADV_CPLT:
		case NODE_RX_TYPE_MESH_REPORT:
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CTLR_USER_EXT)
		case NODE_RX_TYPE_USER_START ... NODE_RX_TYPE_USER_END:
#endif /* CONFIG_BT_CTLR_USER_EXT */

		/* Ensure that at least one 'case' statement is present for this
		 * code block.
		 */
		case NODE_RX_TYPE_NONE:
			LL_ASSERT(rx_free->type != NODE_RX_TYPE_NONE);
			ll_rx_link_inc_quota(1);
			mem_release(rx_free, &mem_pdu_rx.free);
			break;

#if defined(CONFIG_BT_CONN)
		case NODE_RX_TYPE_TERMINATE:
		{
			struct ll_conn *conn;
			memq_link_t *link;

			conn = ll_conn_get(rx_free->handle);

			LL_ASSERT(!conn->lll.link_tx_free);
			link = memq_deinit(&conn->lll.memq_tx.head,
					   &conn->lll.memq_tx.tail);
			LL_ASSERT(link);
			conn->lll.link_tx_free = link;

			ll_conn_release(conn);
		}
		break;
#endif /* CONFIG_BT_CONN */

		case NODE_RX_TYPE_EVENT_DONE:
		default:
			LL_ASSERT(0);
			break;
		}
	}

	*node_rx = rx;

	rx_alloc(UINT8_MAX);
}

static inline void ll_rx_link_inc_quota(int8_t delta)
{
	LL_ASSERT(delta <= 0 || mem_link_rx.quota_pdu < RX_CNT);
	mem_link_rx.quota_pdu += delta;
}

void *ll_rx_link_alloc(void)
{
	return mem_acquire(&mem_link_rx.free);
}

void ll_rx_link_release(void *link)
{
	mem_release(link, &mem_link_rx.free);
}

void *ll_rx_alloc(void)
{
	return mem_acquire(&mem_pdu_rx.free);
}

void ll_rx_release(void *node_rx)
{
	mem_release(node_rx, &mem_pdu_rx.free);
}

void ll_rx_put(memq_link_t *link, void *rx)
{
	struct node_rx_hdr *rx_hdr = rx;

	/* Serialize Tx ack with Rx enqueue by storing reference to
	 * last element index in Tx ack FIFO.
	 */
#if defined(CONFIG_BT_CONN)
	rx_hdr->ack_last = mfifo_tx_ack.l;
#else /* !CONFIG_BT_CONN */
	ARG_UNUSED(rx_hdr);
#endif /* !CONFIG_BT_CONN */

	/* Enqueue the Rx object */
	memq_enqueue(link, rx, &memq_ll_rx.tail);
}

/**
 * @brief Permit another loop in the controller thread (prio_recv_thread)
 * @details Execution context: ULL mayfly
 */
void ll_rx_sched(void)
{
	/* sem_recv references the same semaphore (sem_prio_recv)
	 * in prio_recv_thread
	 */
	k_sem_give(sem_recv);
}

#if defined(CONFIG_BT_CONN)
void *ll_pdu_rx_alloc_peek(u8_t count)
{
	if (count > MFIFO_AVAIL_COUNT_GET(ll_pdu_rx_free)) {
		return NULL;
	}

	return MFIFO_DEQUEUE_PEEK(ll_pdu_rx_free);
}

void *ll_pdu_rx_alloc(void)
{
	return MFIFO_DEQUEUE(ll_pdu_rx_free);
}

void ll_tx_ack_put(u16_t handle, struct node_tx *node_tx)
{
	struct lll_tx *tx;
	u8_t idx;

	idx = MFIFO_ENQUEUE_GET(tx_ack, (void **)&tx);
	LL_ASSERT(tx);

	tx->handle = handle;
	tx->node = node_tx;

	MFIFO_ENQUEUE(tx_ack, idx);
}
#endif /* CONFIG_BT_CONN */

void ll_timeslice_ticker_id_get(u8_t * const instance_index,
				u8_t * const user_id)
{
	*instance_index = TICKER_INSTANCE_ID_CTLR;
	*user_id = (TICKER_NODES - FLASH_TICKER_NODES);
}

void ll_radio_state_abort(void)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
	u32_t ret;

	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL, 0,
			     &mfy);
	LL_ASSERT(!ret);
}

u32_t ll_radio_state_is_idle(void)
{
	return lll_radio_is_idle();
}

void ull_ticker_status_give(u32_t status, void *param)
{
	*((u32_t volatile *)param) = status;

	k_sem_give(&sem_ticker_api_cb);
}

u32_t ull_ticker_status_take(u32_t ret, u32_t volatile *ret_cb)
{
	if (ret == TICKER_STATUS_BUSY) {
		/* TODO: Enable ticker job in case of CONFIG_BT_CTLR_LOW_LAT */
	}

	k_sem_take(&sem_ticker_api_cb, K_FOREVER);

	return *ret_cb;
}

void *ull_disable_mark(void *param)
{
	return mark_set(&mark_disable, param);
}

void *ull_disable_unmark(void *param)
{
	return mark_unset(&mark_disable, param);
}

void *ull_disable_mark_get(void)
{
	return mark_get(mark_disable);
}

#if defined(CONFIG_BT_CONN)
void *ull_update_mark(void *param)
{
	return mark_set(&mark_update, param);
}

void *ull_update_unmark(void *param)
{
	return mark_unset(&mark_update, param);
}

void *ull_update_mark_get(void)
{
	return mark_get(mark_update);
}
#endif /* CONFIG_BT_CONN */

int ull_disable(void *lll)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
	struct ull_hdr *hdr;
	struct k_sem sem;
	u32_t ret;

	hdr = HDR_ULL(((struct lll_hdr *)lll)->parent);
	if (!hdr || !hdr->ref) {
		return ULL_STATUS_SUCCESS;
	}

	k_sem_init(&sem, 0, 1);
	hdr->disabled_param = &sem;
	hdr->disabled_cb = disabled_cb;

	mfy.param = lll;
	ret = mayfly_enqueue(TICKER_USER_ID_THREAD, TICKER_USER_ID_LLL, 0,
			     &mfy);
	LL_ASSERT(!ret);

	return k_sem_take(&sem, K_FOREVER);
}

void *ull_pdu_rx_alloc_peek(u8_t count)
{
	if (count > MFIFO_AVAIL_COUNT_GET(pdu_rx_free)) {
		return NULL;
	}

	return MFIFO_DEQUEUE_PEEK(pdu_rx_free);
}

void *ull_pdu_rx_alloc_peek_iter(u8_t *idx)
{
	return *(void **)MFIFO_DEQUEUE_ITER_GET(pdu_rx_free, idx);
}

void *ull_pdu_rx_alloc(void)
{
	return MFIFO_DEQUEUE(pdu_rx_free);
}

void ull_rx_put(memq_link_t *link, void *rx)
{
	struct node_rx_hdr *rx_hdr = rx;

	/* Serialize Tx ack with Rx enqueue by storing reference to
	 * last element index in Tx ack FIFO.
	 */
#if defined(CONFIG_BT_CONN)
	rx_hdr->ack_last = ull_conn_ack_last_idx_get();
#else
	ARG_UNUSED(rx_hdr);
#endif

	/* Enqueue the Rx object */
	memq_enqueue(link, rx, &memq_ull_rx.tail);
}

void ull_rx_sched(void)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, rx_demux};

	/* Kick the ULL (using the mayfly, tailchain it) */
	mayfly_enqueue(TICKER_USER_ID_LLL, TICKER_USER_ID_ULL_HIGH, 1, &mfy);
}

int ull_prepare_enqueue(lll_is_abort_cb_t is_abort_cb,
			lll_abort_cb_t abort_cb,
			struct lll_prepare_param *prepare_param,
			lll_prepare_cb_t prepare_cb, int prio,
			u8_t is_resume)
{
	struct lll_event *e;
	u8_t idx;

	idx = MFIFO_ENQUEUE_GET(prep, (void **)&e);
	if (!e) {
		return -ENOBUFS;
	}

	memcpy(&e->prepare_param, prepare_param, sizeof(e->prepare_param));
	e->prepare_cb = prepare_cb;
	e->is_abort_cb = is_abort_cb;
	e->abort_cb = abort_cb;
	e->prio = prio;
	e->is_resume = is_resume;
	e->is_aborted = 0U;

	MFIFO_ENQUEUE(prep, idx);

	return 0;
}

void *ull_prepare_dequeue_get(void)
{
	return MFIFO_DEQUEUE_GET(prep);
}

void *ull_prepare_dequeue_iter(u8_t *idx)
{
	return MFIFO_DEQUEUE_ITER_GET(prep, idx);
}

void *ull_event_done_extra_get(void)
{
	struct node_rx_event_done *evdone;

	evdone = MFIFO_DEQUEUE_PEEK(done);
	if (!evdone) {
		return NULL;
	}

	return &evdone->extra;
}

void *ull_event_done(void *param)
{
	struct node_rx_event_done *evdone;
	memq_link_t *link;

	/* Obtain new node that signals "Done of an RX-event".
	 * Obtain this by dequeuing from the global 'mfifo_done' queue.
	 * Note that 'mfifo_done' is a queue of pointers, not of
	 * struct node_rx_event_done
	 */
	evdone = MFIFO_DEQUEUE(done);
	if (!evdone) {
		/* Not fatal if we can not obtain node, though
		 * we will loose the packets in software stack.
		 * If this happens during Conn Upd, this could cause LSTO
		 */
		return NULL;
	}

	link = evdone->hdr.link;
	evdone->hdr.link = NULL;

	evdone->hdr.type = NODE_RX_TYPE_EVENT_DONE;
	evdone->param = param;

	ull_rx_put(link, evdone);
	ull_rx_sched();

	return evdone;
}

static inline int init_reset(void)
{
	memq_link_t *link;

	/* Initialize done pool. */
	mem_init(mem_done.pool, sizeof(struct node_rx_event_done),
		 EVENT_DONE_MAX, &mem_done.free);

	/* Initialize done link pool. */
	mem_init(mem_link_done.pool, sizeof(memq_link_t), EVENT_DONE_MAX,
		 &mem_link_done.free);

	/* Allocate done buffers */
	done_alloc();

	/* Initialize rx pool. */
	mem_init(mem_pdu_rx.pool, (PDU_RX_NODE_POOL_ELEMENT_SIZE),
		 sizeof(mem_pdu_rx.pool) / (PDU_RX_NODE_POOL_ELEMENT_SIZE),
		 &mem_pdu_rx.free);

	/* Initialize rx link pool. */
	mem_init(mem_link_rx.pool, sizeof(memq_link_t),
		 sizeof(mem_link_rx.pool) / sizeof(memq_link_t),
		 &mem_link_rx.free);

	/* Acquire a link to initialize ull rx memq */
	link = mem_acquire(&mem_link_rx.free);
	LL_ASSERT(link);

	/* Initialize ull rx memq */
	MEMQ_INIT(ull_rx, link);

	/* Acquire a link to initialize ll rx memq */
	link = mem_acquire(&mem_link_rx.free);
	LL_ASSERT(link);

	/* Initialize ll rx memq */
	MEMQ_INIT(ll_rx, link);

	/* Allocate rx free buffers */
	mem_link_rx.quota_pdu = RX_CNT;
	rx_alloc(UINT8_MAX);

	return 0;
}

static void perform_lll_reset(void *param)
{
	int err;

	/* Reset LLL */
	err = lll_reset();
	LL_ASSERT(!err);

#if defined(CONFIG_BT_BROADCASTER)
	/* Reset adv state */
	err = lll_adv_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	/* Reset scan state */
	err = lll_scan_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
	/* Reset conn role */
	err = lll_conn_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CONN */
}

static inline void *mark_set(void **m, void *param)
{
	if (!*m) {
		*m = param;
	}

	return *m;
}

static inline void *mark_unset(void **m, void *param)
{
	if (*m && *m == param) {
		*m = NULL;

		return param;
	}

	return NULL;
}

static inline void *mark_get(void *m)
{
	return m;
}

/**
 * @brief Allocate buffers for done events
 */
static inline void done_alloc(void)
{
	u8_t idx;

	/* mfifo_done is a queue of pointers */
	while (MFIFO_ENQUEUE_IDX_GET(done, &idx)) {
		memq_link_t *link;
		struct node_rx_hdr *rx;

		link = mem_acquire(&mem_link_done.free);
		if (!link) {
			break;
		}

		rx = mem_acquire(&mem_done.free);
		if (!rx) {
			mem_release(link, &mem_link_done.free);
			break;
		}

		rx->link = link;

		MFIFO_BY_IDX_ENQUEUE(done, idx, rx);
	}
}

static inline void *done_release(memq_link_t *link,
				 struct node_rx_event_done *done)
{
	u8_t idx;

	if (!MFIFO_ENQUEUE_IDX_GET(done, &idx)) {
		return NULL;
	}

	done->hdr.link = link;

	MFIFO_BY_IDX_ENQUEUE(done, idx, done);

	return done;
}

static inline void rx_alloc(u8_t max)
{
	u8_t idx;

#if defined(CONFIG_BT_CONN)
	while (mem_link_rx.quota_pdu &&
	       MFIFO_ENQUEUE_IDX_GET(ll_pdu_rx_free, &idx)) {
		memq_link_t *link;
		struct node_rx_hdr *rx;

		link = mem_acquire(&mem_link_rx.free);
		if (!link) {
			break;
		}

		rx = mem_acquire(&mem_pdu_rx.free);
		if (!rx) {
			mem_release(link, &mem_link_rx.free);
			break;
		}

		link->mem = NULL;
		rx->link = link;

		MFIFO_BY_IDX_ENQUEUE(ll_pdu_rx_free, idx, rx);

		ll_rx_link_inc_quota(-1);
	}
#endif /* CONFIG_BT_CONN */

	if (max > mem_link_rx.quota_pdu) {
		max = mem_link_rx.quota_pdu;
	}

	while ((max--) && MFIFO_ENQUEUE_IDX_GET(pdu_rx_free, &idx)) {
		memq_link_t *link;
		struct node_rx_hdr *rx;

		link = mem_acquire(&mem_link_rx.free);
		if (!link) {
			break;
		}

		rx = mem_acquire(&mem_pdu_rx.free);
		if (!rx) {
			mem_release(link, &mem_link_rx.free);
			break;
		}

		rx->link = link;

		MFIFO_BY_IDX_ENQUEUE(pdu_rx_free, idx, rx);

		ll_rx_link_inc_quota(-1);
	}
}

#if defined(CONFIG_BT_CONN)
static u8_t tx_cmplt_get(u16_t *handle, u8_t *first, u8_t last)
{
	struct lll_tx *tx;
	u8_t cmplt;

	tx = mfifo_dequeue_iter_get(mfifo_tx_ack.m, mfifo_tx_ack.s,
				    mfifo_tx_ack.n, mfifo_tx_ack.f, last,
				    first);
	if (!tx) {
		return 0;
	}

	*handle = tx->handle;
	cmplt = 0U;
	do {
		struct node_tx *node_tx;
		struct pdu_data *p;

		node_tx = tx->node;
		p = (void *)node_tx->pdu;
		if (!node_tx || (node_tx == (void *)1) ||
		    (((u32_t)node_tx & ~3) &&
		     (p->ll_id == PDU_DATA_LLID_DATA_START ||
		      p->ll_id == PDU_DATA_LLID_DATA_CONTINUE))) {
			/* data packet, hence count num cmplt */
			tx->node = (void *)1;
			cmplt++;
		} else {
			/* ctrl packet or flushed, hence dont count num cmplt */
			tx->node = (void *)2;
		}

		if (((u32_t)node_tx & ~3)) {
			ll_tx_mem_release(node_tx);
		}

		tx = mfifo_dequeue_iter_get(mfifo_tx_ack.m, mfifo_tx_ack.s,
					    mfifo_tx_ack.n, mfifo_tx_ack.f,
					    last, first);
	} while (tx && tx->handle == *handle);

	return cmplt;
}

static inline void rx_demux_conn_tx_ack(u8_t ack_last, u16_t handle,
					memq_link_t *link,
					struct node_tx *node_tx)
{
#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	do {
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */
		struct ll_conn *conn;

		/* Dequeue node */
		ull_conn_ack_dequeue();

		/* Process Tx ack */
		conn = ull_conn_tx_ack(handle, link, node_tx);

		/* Release link mem */
		ull_conn_link_tx_release(link);

		/* De-mux 1 tx node from FIFO */
		ull_conn_tx_demux(1);

		/* Enqueue towards LLL */
		if (conn) {
			ull_conn_tx_lll_enqueue(conn, 1);
		}

		/* check for more rx ack */
		link = ull_conn_ack_by_last_peek(ack_last, &handle, &node_tx);

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
		if (!link)
#else /* CONFIG_BT_CTLR_LOW_LAT_ULL */
	} while (link);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

		{
			/* trigger thread to call ll_rx_get() */
			ll_rx_sched();
		}
}
#endif /* CONFIG_BT_CONN */

static void rx_demux(void *param)
{
	memq_link_t *link;

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	do {
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */
		struct node_rx_hdr *rx;

		link = memq_peek(memq_ull_rx.head, memq_ull_rx.tail,
				 (void **)&rx);
		if (link) {
#if defined(CONFIG_BT_CONN)
			struct node_tx *node_tx;
			memq_link_t *link_tx;
			u16_t handle; /* Handle to Ack TX */
#endif /* CONFIG_BT_CONN */
			int nack = 0;

			LL_ASSERT(rx);

#if defined(CONFIG_BT_CONN)
			link_tx = ull_conn_ack_by_last_peek(rx->ack_last,
							    &handle, &node_tx);
			if (link_tx) {
				rx_demux_conn_tx_ack(rx->ack_last, handle,
						     link_tx, node_tx);
			} else
#endif /* CONFIG_BT_CONN */
			{
				nack = rx_demux_rx(link, rx);
			}

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
			if (!nack) {
				ull_rx_sched();
			}
#else /* !CONFIG_BT_CTLR_LOW_LAT_ULL */
			if (nack) {
				break;
			}
#endif /* !CONFIG_BT_CTLR_LOW_LAT_ULL */

#if defined(CONFIG_BT_CONN)
		} else {
			struct node_tx *node_tx;
			u8_t ack_last;
			u16_t handle;

			link = ull_conn_ack_peek(&ack_last, &handle, &node_tx);
			if (link) {
				rx_demux_conn_tx_ack(ack_last, handle,
						      link, node_tx);

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
				ull_rx_sched();
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

			}
#endif /* CONFIG_BT_CONN */
		}

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	} while (link);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */
}

/**
 * @brief Dispatch rx objects
 * @details Rx objects are only peeked, not dequeued yet.
 *   Execution context: ULL high priority Mayfly
 */
static inline int rx_demux_rx(memq_link_t *link, struct node_rx_hdr *rx)
{
	/* Demux Rx objects */
	switch (rx->type) {
	case NODE_RX_TYPE_EVENT_DONE:
	{
		memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		rx_demux_event_done(link, rx);
	}
	break;

#if defined(CONFIG_BT_CONN)
	case NODE_RX_TYPE_CONNECTION:
	{
		memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		ull_conn_setup(link, rx);
	}
	break;

	case NODE_RX_TYPE_DC_PDU:
	{
		int nack;

		nack = ull_conn_rx(link, (void *)&rx);
		if (nack) {
			return nack;
		}

		memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);

		if (rx) {
			ll_rx_put(link, rx);
			ll_rx_sched();
		}
	}
	break;

	case NODE_RX_TYPE_TERMINATE:
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_OBSERVER) || \
	defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY) || \
	defined(CONFIG_BT_CTLR_PROFILE_ISR) || \
	defined(CONFIG_BT_CTLR_ADV_INDICATION) || \
	defined(CONFIG_BT_CTLR_SCAN_INDICATION) || \
	defined(CONFIG_BT_CONN)

#if defined(CONFIG_BT_OBSERVER)
	case NODE_RX_TYPE_REPORT:
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	case NODE_RX_TYPE_EXT_1M_REPORT:
	case NODE_RX_TYPE_EXT_CODED_REPORT:
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	case NODE_RX_TYPE_PROFILE:
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
	case NODE_RX_TYPE_SCAN_INDICATION:
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */
	{
		memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		ll_rx_put(link, rx);
		ll_rx_sched();
	}
	break;
#endif /* CONFIG_BT_OBSERVER ||
	* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY ||
	* CONFIG_BT_CTLR_PROFILE_ISR ||
	* CONFIG_BT_CTLR_ADV_INDICATION ||
	* CONFIG_BT_CTLR_SCAN_INDICATION ||
	* CONFIG_BT_CONN
	*/

	default:
	{
#if defined(CONFIG_BT_CTLR_USER_EXT)
		/* Try proprietary demuxing */
		rx_demux_rx_proprietary(link, rx, memq_ull_rx.tail,
					&memq_ull_rx.head);
#else
		LL_ASSERT(0);
#endif /* CONFIG_BT_CTLR_USER_EXT */
	}
	break;
	}

	return 0;
}

static inline void rx_demux_event_done(memq_link_t *link,
				       struct node_rx_hdr *rx)
{
	struct node_rx_event_done *done = (void *)rx;
	struct ull_hdr *ull_hdr;
	struct lll_event *next;
	void *release;

	/* Get the ull instance */
	ull_hdr = done->param;

	/* Process role dependent event done */
	switch (done->extra.type) {
#if defined(CONFIG_BT_CONN)
	case EVENT_DONE_EXTRA_TYPE_CONN:
		ull_conn_done(done);
		break;
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_USER_EXT)
	case EVENT_DONE_EXTRA_TYPE_USER_START
		... EVENT_DONE_EXTRA_TYPE_USER_END:
		ull_proprietary_done(done);
		break;
#endif /* CONFIG_BT_CTLR_USER_EXT */

	case EVENT_DONE_EXTRA_TYPE_NONE:
		/* ignore */
		break;

	default:
		LL_ASSERT(0);
		break;
	}

	/* release done */
	done->extra.type = 0U;
	release = done_release(link, done);
	LL_ASSERT(release == done);

	/* dequeue prepare pipeline */
	next = ull_prepare_dequeue_get();
	while (next) {
		u8_t is_aborted = next->is_aborted;
		u8_t is_resume = next->is_resume;

		if (!is_aborted) {
			static memq_link_t link;
			static struct mayfly mfy = {0, 0, &link, NULL,
						    lll_resume};
			u32_t ret;

			mfy.param = next;
			ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
					     TICKER_USER_ID_LLL, 0, &mfy);
			LL_ASSERT(!ret);
		}

		MFIFO_DEQUEUE(prep);

		next = ull_prepare_dequeue_get();

		if (!next || (!is_aborted && (!is_resume || next->is_resume))) {
			break;
		}
	}

	/* ull instance will resume, dont decrement ref */
	if (!ull_hdr) {
		return;
	}

	/* Decrement prepare reference */
	LL_ASSERT(ull_hdr->ref);
	ull_ref_dec(ull_hdr);

	/* If disable initiated, signal the semaphore */
	if (!ull_hdr->ref && ull_hdr->disabled_cb) {
		ull_hdr->disabled_cb(ull_hdr->disabled_param);
	}
}

static void disabled_cb(void *param)
{
	k_sem_give(param);
}
