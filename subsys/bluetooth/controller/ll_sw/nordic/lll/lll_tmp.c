/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>

#include <toolchain.h>
#include <zephyr/types.h>

#if defined(CONFIG_BT_CTLR_DEBUG_PINS)
#if defined(CONFIG_PRINTK)
#undef CONFIG_PRINTK
#endif
#endif

#include "hal/ccm.h"

#include "util/mfifo.h"
#include "util/memq.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll_conn.h"
#include "lll_tmp.h"
#include "lll_internal.h"

#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static MFIFO_DEFINE(tmp_ack, sizeof(struct lll_tx),
		    CONFIG_BT_TMP_TX_COUNT_MAX);

static int _init_reset(void);
static int _prepare_cb(struct lll_prepare_param *prepare_param);
static int _is_abort_cb(void *next, int prio, void *curr,
			lll_prepare_cb_t *resume_cb, int *resume_prio);
static void _abort_cb(struct lll_prepare_param *prepare_param, void *param);
static int _emulate_tx_rx(void *param);

int lll_tmp_init(void)
{
	int err;

	err = _init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_tmp_reset(void)
{
	int err;

	MFIFO_INIT(tmp_ack);

	err = _init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_tmp_prepare(void *param)
{
	struct lll_prepare_param *p = param;
	int err;

	printk("\t\tlll_tmp_prepare (%p) enter.\n", p->param);

	err = lll_clk_on();
	printk("\t\tlll_clk_on: %d.\n", err);

	err = lll_prepare(_is_abort_cb, _abort_cb, _prepare_cb, 0, p);

	printk("\t\tlll_tmp_prepare (%p) exit (%d).\n", p->param, err);
}

u8_t lll_tmp_ack_last_idx_get(void)
{
	return mfifo_tmp_ack.l;
}

memq_link_t *lll_tmp_ack_peek(u16_t *handle, struct node_tx **node_tx)
{
	struct lll_tx *tx;

	tx = MFIFO_DEQUEUE_GET(tmp_ack);
	if (!tx) {
		return NULL;
	}

	*handle = tx->handle;
	*node_tx = tx->node;

	return (*node_tx)->link;
}

memq_link_t *lll_tmp_ack_by_last_peek(u8_t last, u16_t *handle,
				      struct node_tx **node_tx)
{
	struct lll_tx *tx;

	tx = mfifo_dequeue_get(mfifo_tmp_ack.m, mfifo_tmp_ack.s,
			       mfifo_tmp_ack.f, last);
	if (!tx) {
		return NULL;
	}

	*handle = tx->handle;
	*node_tx = tx->node;

	return (*node_tx)->link;
}

void *lll_tmp_ack_dequeue(void)
{
	return MFIFO_DEQUEUE(tmp_ack);
}

static int _init_reset(void)
{
	return 0;
}

static int _prepare_cb(struct lll_prepare_param *prepare_param)
{
	int err;

	printk("\t\t_prepare (%p) enter: expected %u, actual %u.\n",
	       prepare_param->param, prepare_param->ticks_at_expire,
	       ticker_ticks_now_get());
	DEBUG_RADIO_PREPARE_A(1);

	err = _emulate_tx_rx(prepare_param);

	DEBUG_RADIO_PREPARE_A(1);
	printk("\t\t_prepare (%p) exit (%d).\n", prepare_param->param, err);

	return err;
}

static int _is_abort_cb(void *next, int prio, void *curr,
			lll_prepare_cb_t *resume_cb, int *resume_prio)
{
	static u8_t toggle;

	toggle++;

	return toggle & 0x01;
}

static void _abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	int err;

	printk("\t\t_abort (%p) enter.\n", param);

	/* NOTE: This is not a prepare being cancelled */
	if (!prepare_param) {
		/* Perform event abort here.
		 * After event has been cleanly aborted, clean up resources
		 * and dispatch event done.
		 */

		/* Current event is done, pass NULL to lll_done(). */
		param = NULL;
	}

	/* NOTE: Else clean the top half preparations of the aborted event
	 * currently in preparation pipeline.
	 */
	err = lll_clk_off();
	printk("\t\tlll_clk_off: %d.\n", err);

	lll_done(param);
	printk("\t\tlll_done (%p).\n", param);

	printk("\t\t_abort (%p) exit.\n", param);
}

static int _emulate_tx_rx(void *param)
{
	struct lll_prepare_param *prepare_param = param;
	struct lll_tmp *tmp = prepare_param->param;
	struct node_tx *node_tx;
	bool is_ull_rx = false;
	memq_link_t *link;
	void *free;

	/* Tx */
	link = memq_dequeue(tmp->memq_tx.tail, &tmp->memq_tx.head,
			    (void **)&node_tx);
	while (link) {
		struct lll_tx *tx;
		u8_t idx;

		idx = MFIFO_ENQUEUE_GET(tmp_ack, (void **)&tx);
		LL_ASSERT(tx);

		tx->handle = ull_tmp_handle_get(tmp);
		tx->node = node_tx;

		node_tx->link = link;

		printk("\t\t_emulate_tx_rx: h= %u.\n", tx->handle);

		MFIFO_ENQUEUE(tmp_ack, idx);

		link = memq_dequeue(tmp->memq_tx.tail, &tmp->memq_tx.head,
				    (void **)&node_tx);
	}

	/* Rx */
	free = ull_pdu_rx_alloc_peek(2);
	if (free) {
		struct node_rx_hdr *hdr = free;
		void *_free;

		_free = ull_pdu_rx_alloc();
		LL_ASSERT(free == _free);

		hdr->type = NODE_RX_TYPE_DC_PDU;

		ull_rx_put(hdr->link, hdr);

		is_ull_rx = true;
	} else {
		printk("\t\tOUT OF PDU RX MEMORY.\n");
	}

	if (is_ull_rx) {
		ull_rx_sched();
	}

	return 0;
}
