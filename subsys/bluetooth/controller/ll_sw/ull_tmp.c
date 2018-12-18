/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <errno.h>

#include <toolchain.h>
#include <zephyr/types.h>

#if defined(CONFIG_BT_CTLR_DEBUG_PINS)
#if defined(CONFIG_PRINTK)
#undef CONFIG_PRINTK
#endif
#endif

#include "hal/ccm.h"

#include "util/mem.h"
#include "util/mfifo.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll_conn.h"
#include "lll_tmp.h"
#include "ull_internal.h"
#include "ull_tmp.h"
#include "ull_tmp_internal.h"

#define LOG_MODULE_NAME bt_ctlr_llsw_ull_tmp
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

#define TMP_TICKER_TICKS_PERIOD   32768
#define TMP_TICKER_TICKS_SLOT     327

#define TMP_TX_POOL_SIZE ((CONFIG_BT_TMP_TX_SIZE_MAX) * \
			  (CONFIG_BT_TMP_TX_COUNT_MAX))

/* NOTE: structure accessed by Thread and ULL */
struct ull_tmp {
	struct ull_hdr hdr;

	u8_t is_enabled:1;
};

struct tmp {
	struct ull_tmp ull;
	struct lll_tmp lll;
};

static struct tmp tmp_inst[CONFIG_BT_TMP_MAX];

static MFIFO_DEFINE(tmp_tx, sizeof(struct lll_tx),
		    CONFIG_BT_TMP_TX_COUNT_MAX);

static struct {
	void *free;
	u8_t pool[TMP_TX_POOL_SIZE];
} mem_tmp_tx;

static struct {
	void *free;
	u8_t pool[sizeof(memq_link_t) * CONFIG_BT_TMP_TX_COUNT_MAX];
} mem_link_tx;

static int _init_reset(void);
static void _ticker_cb(u32_t ticks_at_expire, u32_t remainder,
		       u16_t lazy, void *param);
static void _tx_demux(void);

int ull_tmp_init(void)
{
	int err;

	err = _init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_tmp_reset(void)
{
	u16_t handle;
	int err;

	handle = CONFIG_BT_TMP_MAX;
	while (handle--) {
		ull_tmp_disable(handle);
	}

	/* Re-initialize the Tx mfifo */
	MFIFO_INIT(tmp_tx);

	err = _init_reset();
	if (err) {
		return err;
	}

	return 0;
}

u16_t ull_tmp_handle_get(struct lll_tmp *tmp)
{
	return ((u8_t *)CONTAINER_OF(tmp, struct tmp, lll) -
		(u8_t *)&tmp_inst[0]) / sizeof(struct tmp);

}

int ull_tmp_enable(u16_t handle)
{
	u32_t tmp_ticker_anchor;
	u8_t tmp_ticker_id;
	struct tmp *inst;
	int ret;

	if (handle >= CONFIG_BT_TMP_MAX) {
		return -EINVAL;
	}

	inst = &tmp_inst[handle];
	if (inst->ull.is_enabled) {
		return -EALREADY;
	}

	ull_hdr_init(&inst->ull.hdr);
	lll_hdr_init(&inst->lll, inst);

	if (!inst->lll.link_free) {
		inst->lll.link_free = &inst->lll._link;
	}

	memq_init(inst->lll.link_free, &inst->lll.memq_tx.head,
		  &inst->lll.memq_tx.tail);

	tmp_ticker_id = TICKER_ID_TMP_BASE + handle;
	tmp_ticker_anchor = ticker_ticks_now_get();

	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   tmp_ticker_id,
			   tmp_ticker_anchor,
			   0,
			   TMP_TICKER_TICKS_PERIOD,
			   TICKER_NULL_REMAINDER,
			   TICKER_NULL_LAZY,
			   TMP_TICKER_TICKS_SLOT,
			   _ticker_cb, inst,
			   NULL, NULL);
	if (ret) {
		goto enable_cleanup;
	}

	inst->lll.link_free = NULL;
	inst->ull.is_enabled = 1;

enable_cleanup:

	return ret;
}

int ull_tmp_disable(u16_t handle)
{
	u8_t tmp_ticker_id;
	struct tmp *inst;
	int ret;

	if (handle >= CONFIG_BT_TMP_MAX) {
		return -EINVAL;
	}

	inst = &tmp_inst[handle];
	if (!inst->ull.is_enabled) {
		return -EALREADY;
	}

	tmp_ticker_id = TICKER_ID_TMP_BASE + handle;

	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			  tmp_ticker_id,
			  NULL, NULL);
	if (ret) {
		return ret;
	}

	ret = ull_disable(&inst->lll);
	if (ret) {
		return ret;
	}

	inst->ull.is_enabled = 0;
	inst->lll.link_free = memq_deinit(&inst->lll.memq_tx.head,
					  &inst->lll.memq_tx.tail);

	return ret;
}

int ull_tmp_data_send(u16_t handle, u8_t size, u8_t *data)
{
	struct lll_tx *tx;
	struct node_tx *node_tx;
	struct tmp *inst;
	u8_t idx;

	if (handle >= CONFIG_BT_TMP_MAX) {
		return -EINVAL;
	}

	inst = &tmp_inst[handle];
	if (!inst->ull.is_enabled) {
		return -EINVAL;
	}

	if (size > CONFIG_BT_TMP_TX_SIZE_MAX) {
		return -EMSGSIZE;
	}

	idx = MFIFO_ENQUEUE_GET(tmp_tx, (void **) &tx);
	if (!tx) {
		return -ENOBUFS;
	}

	tx->node = mem_acquire(&mem_tmp_tx.free);
	if (!tx->node) {
		return -ENOMEM;
	}

	tx->handle = handle;

	node_tx = tx->node;
	memcpy(node_tx->pdu, data, size);

	MFIFO_ENQUEUE(tmp_tx, idx);

	return 0;
}

void ull_tmp_link_tx_release(memq_link_t *link)
{
	mem_release(link, &mem_link_tx.free);
}

static int _init_reset(void)
{
	/* Initialize tx pool. */
	mem_init(mem_tmp_tx.pool, CONFIG_BT_TMP_TX_SIZE_MAX,
		 CONFIG_BT_TMP_TX_COUNT_MAX, &mem_tmp_tx.free);

	/* Initialize tx link pool. */
	mem_init(mem_link_tx.pool, sizeof(memq_link_t),
		 CONFIG_BT_TMP_TX_COUNT_MAX, &mem_link_tx.free);

	return 0;
}

static void _ticker_cb(u32_t ticks_at_expire, u32_t remainder,
		       u16_t lazy, void *param)
{
	static memq_link_t _link;
	static struct mayfly _mfy = {0, 0, &_link, NULL, lll_tmp_prepare};
	static struct lll_prepare_param p;
	struct tmp *inst = param;
	u32_t ret;
	u8_t ref;

	printk("\t_ticker_cb (%p) enter: %u, %u, %u.\n", param,
	       ticks_at_expire, remainder, lazy);
	DEBUG_RADIO_PREPARE_A(1);

	/* Increment prepare reference count */
	ref = ull_ref_inc(&inst->ull.hdr);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.param = &inst->lll;
	_mfy.param = &p;

	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &_mfy);
	LL_ASSERT(!ret);

	/* De-mux tx FIFO */
	_tx_demux();

	DEBUG_RADIO_PREPARE_A(1);
	printk("\t_ticker_cb (%p) exit.\n", param);
}

static void _tx_demux(void)
{
	struct lll_tx *tx;

	tx = MFIFO_DEQUEUE_GET(tmp_tx);
	while (tx) {
		memq_link_t *link;
		struct tmp *inst;

		inst = &tmp_inst[tx->handle];

		printk("\t_ticker_cb (%p) tx_demux (%p): h = 0x%x, n=%p.\n",
		       inst, tx, tx->handle, tx->node);

		link = mem_acquire(&mem_link_tx.free);
		LL_ASSERT(link);

		memq_enqueue(link, tx->node, &inst->lll.memq_tx.tail);

		MFIFO_DEQUEUE(tmp_tx);

		tx = MFIFO_DEQUEUE_GET(tmp_tx);
	}
}
