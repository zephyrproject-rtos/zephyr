/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/hci.h>

#include "hal/cpu_vendor_hal.h"
#include "hal/ccm.h"

#include "util/mem.h"
#include "util/mfifo.h"
#include "util/memq.h"
#include "util/dbuf.h"
#include "util.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"
#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"
#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_scan.h"
#include "lll_sync.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"

#include "ull_conn_internal.h"

#define EVENT_DONE_MAX 3
/* Backing storage for elements in mfifo_done */
static struct {
	void *free;
	uint8_t pool[sizeof(struct node_rx_event_done) * EVENT_DONE_MAX];
} mem_done;

static struct {
	void *free;
	uint8_t pool[sizeof(memq_link_t) * EVENT_DONE_MAX];
} mem_link_done;

#if defined(CONFIG_BT_CTLR_PHY) && defined(CONFIG_BT_CTLR_DATA_LENGTH)
#define LL_PDU_RX_CNT (3 + 128)
#else
#define LL_PDU_RX_CNT (2 + 128)
#endif

#define PDU_RX_CNT (CONFIG_BT_CTLR_RX_BUFFERS + 3)
#define RX_CNT (PDU_RX_CNT + LL_PDU_RX_CNT)

static MFIFO_DEFINE(pdu_rx_free, sizeof(void *), PDU_RX_CNT);

#if defined(CONFIG_BT_RX_USER_PDU_LEN)
#define PDU_RX_USER_PDU_OCTETS_MAX (CONFIG_BT_RX_USER_PDU_LEN)
#else
#define PDU_RX_USER_PDU_OCTETS_MAX 0
#endif
#define NODE_RX_HEADER_SIZE (offsetof(struct node_rx_pdu, pdu))
#define NODE_RX_STRUCT_OVERHEAD (NODE_RX_HEADER_SIZE)

#define PDU_ADV_SIZE  MAX(PDU_AC_LL_SIZE_MAX, \
			  (PDU_AC_LL_HEADER_SIZE + LL_EXT_OCTETS_RX_MAX))

#define PDU_DATA_SIZE (PDU_DC_LL_HEADER_SIZE + LL_LENGTH_OCTETS_RX_MAX)

#define PDU_RX_NODE_POOL_ELEMENT_SIZE                                                              \
	MROUND(NODE_RX_STRUCT_OVERHEAD +                                                           \
	       MAX(MAX(PDU_ADV_SIZE, PDU_DATA_SIZE), PDU_RX_USER_PDU_OCTETS_MAX))

/*
 * just a big number
 */
#define PDU_RX_POOL_SIZE 16384

static struct {
	void *free;
	uint8_t pool[PDU_RX_POOL_SIZE];
} mem_pdu_rx;

/*
 * just a big number
 */
#define LINK_RX_POOL_SIZE 16384
static struct {
	uint8_t quota_pdu; /* Number of un-utilized buffers */

	void *free;
	uint8_t pool[LINK_RX_POOL_SIZE];
} mem_link_rx;

static MEMQ_DECLARE(ull_rx);
static MEMQ_DECLARE(ll_rx);

#if defined(CONFIG_BT_CONN)
static MFIFO_DEFINE(ll_pdu_rx_free, sizeof(void *), LL_PDU_RX_CNT);
#endif /* CONFIG_BT_CONN */

#ifdef ZTEST_UNITTEST
extern sys_slist_t ut_rx_q;
#else
sys_slist_t ut_rx_q;
#endif

static inline int init_reset(void);
static inline void rx_alloc(uint8_t max);
static inline void ll_rx_link_inc_quota(int8_t delta);

void ll_reset(void)
{
	MFIFO_INIT(ll_pdu_rx_free);
	init_reset();
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
		case NODE_RX_TYPE_DC_PDU:
		case NODE_RX_TYPE_CONN_UPDATE:
		case NODE_RX_TYPE_ENC_REFRESH:
		case NODE_RX_TYPE_PHY_UPDATE:
		case NODE_RX_TYPE_CIS_REQUEST:
		case NODE_RX_TYPE_CIS_ESTABLISHED:

			ll_rx_link_inc_quota(1);
			mem_release(rx_free, &mem_pdu_rx.free);
			break;
		default:
			__ASSERT(0, "Tried to release unknown rx node type");
			break;
		}
	}

	*node_rx = rx;

	rx_alloc(UINT8_MAX);
}

static inline void ll_rx_link_inc_quota(int8_t delta)
{
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
	if (((struct node_rx_hdr *)rx)->type != NODE_RX_TYPE_RELEASE) {
		/* Only put/sched if node was not marked for release */
		sys_slist_append(&ut_rx_q, (sys_snode_t *)rx);
	}
}

void ll_rx_sched(void)
{
}

void ll_rx_put_sched(memq_link_t *link, void *rx)
{
	ll_rx_put(link, rx);
	ll_rx_sched();
}

void *ll_pdu_rx_alloc_peek(uint8_t count)
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

void ll_tx_ack_put(uint16_t handle, struct node_tx *node)
{
}

void ull_ticker_status_give(uint32_t status, void *param)
{
}

uint32_t ull_ticker_status_take(uint32_t ret, uint32_t volatile *ret_cb)
{
	return *ret_cb;
}

void *ull_disable_mark(void *param)
{
	return NULL;
}

void *ull_disable_unmark(void *param)
{
	return NULL;
}

void *ull_disable_mark_get(void)
{
	return NULL;
}

int ull_ticker_stop_with_mark(uint8_t ticker_handle, void *param, void *lll_disable)
{
	return 0;
}

void *ull_update_mark(void *param)
{
	return NULL;
}

void *ull_update_unmark(void *param)
{
	return NULL;
}

void *ull_update_mark_get(void)
{
	return NULL;
}

int ull_disable(void *lll)
{
	return 0;
}

void *ull_pdu_rx_alloc(void)
{
	return NULL;
}

void ull_rx_put(memq_link_t *link, void *rx)
{
}

void ull_rx_sched(void)
{
}

void ull_rx_put_sched(memq_link_t *link, void *rx)
{
}

/* Forward declaration */
struct node_rx_event_done;
void ull_drift_ticks_get(struct node_rx_event_done *done, uint32_t *ticks_drift_plus,
			 uint32_t *ticks_drift_minus)
{
}

static inline int init_reset(void)
{
	memq_link_t *link;

	/* Initialize done pool. */
	mem_init(mem_done.pool, sizeof(struct node_rx_event_done), EVENT_DONE_MAX, &mem_done.free);

	/* Initialize done link pool. */
	mem_init(mem_link_done.pool, sizeof(memq_link_t), EVENT_DONE_MAX, &mem_link_done.free);

	/* Initialize rx pool. */
	mem_init(mem_pdu_rx.pool, (PDU_RX_NODE_POOL_ELEMENT_SIZE),
		 sizeof(mem_pdu_rx.pool) / (PDU_RX_NODE_POOL_ELEMENT_SIZE), &mem_pdu_rx.free);

	/* Initialize rx link pool. */
	mem_init(mem_link_rx.pool, sizeof(memq_link_t),
		 sizeof(mem_link_rx.pool) / sizeof(memq_link_t), &mem_link_rx.free);

	/* Acquire a link to initialize ull rx memq */
	link = mem_acquire(&mem_link_rx.free);

	/* Initialize ull rx memq */
	MEMQ_INIT(ull_rx, link);

	/* Acquire a link to initialize ll rx memq */
	link = mem_acquire(&mem_link_rx.free);

	/* Initialize ll rx memq */
	MEMQ_INIT(ll_rx, link);

	/* Allocate rx free buffers */
	mem_link_rx.quota_pdu = RX_CNT;
	rx_alloc(UINT8_MAX);

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	/* Reset CPR mutex */
	cpr_active_reset();
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

	return 0;
}

static inline void rx_alloc(uint8_t max)
{
	uint8_t idx;

#if defined(CONFIG_BT_CONN)
	while (mem_link_rx.quota_pdu && MFIFO_ENQUEUE_IDX_GET(ll_pdu_rx_free, &idx)) {
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

#if defined(CONFIG_BT_CTLR_ISO) || \
	defined(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER) || \
	defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
uint32_t ull_get_wrapped_time_us(uint32_t time_now_us, int32_t time_diff_us)
{
	return 0;
}
#endif /* CONFIG_BT_CTLR_ISO ||
	* CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER ||
	* CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER
	*/
