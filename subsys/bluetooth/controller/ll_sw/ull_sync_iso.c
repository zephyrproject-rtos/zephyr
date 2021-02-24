/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <sys/byteorder.h>

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll_clock.h"
#include "lll_scan.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"

#include "ull_scan_types.h"
#include "ull_sync_types.h"

#include "ull_internal.h"
#include "ull_scan_internal.h"
#include "ull_sync_internal.h"
#include "ull_sync_iso_internal.h"

#include "ll.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_sync_iso
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);
static inline struct ll_sync_iso *sync_iso_acquire(void);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param);
static void ticker_op_cb(uint32_t status, void *param);

static struct ll_sync_iso ll_sync_iso_pool[CONFIG_BT_CTLR_SCAN_SYNC_ISO_SET];
static void *sync_iso_free;

uint8_t ll_big_sync_create(uint8_t big_handle, uint16_t sync_handle,
			   uint8_t encryption, uint8_t *bcode, uint8_t mse,
			   uint16_t sync_timeout, uint8_t num_bis,
			   uint8_t *bis)
{
	struct ll_sync_iso *sync_iso;
	memq_link_t *link_sync_estab;
	memq_link_t *link_sync_lost;
	struct node_rx_hdr *node_rx;
	struct ll_sync_set *sync;

	sync = ull_sync_is_enabled_get(sync_handle);
	if (!sync || sync->iso.sync_iso) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	link_sync_estab = ll_rx_link_alloc();
	if (!link_sync_estab) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	link_sync_lost = ll_rx_link_alloc();
	if (!link_sync_lost) {
		ll_rx_link_release(link_sync_estab);

		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	node_rx = ll_rx_alloc();
	if (!node_rx) {
		ll_rx_link_release(link_sync_lost);
		ll_rx_link_release(link_sync_estab);

		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	sync_iso = sync_iso_acquire();
	if (!sync_iso) {
		ll_rx_release(node_rx);
		ll_rx_link_release(link_sync_lost);
		ll_rx_link_release(link_sync_estab);

		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Initialize the ISO sync ULL context */
	sync_iso->sync = sync;
	sync_iso->node_rx_lost.hdr.link = link_sync_lost;

	/* Setup the periodic sync to establish ISO sync */
	node_rx->link = link_sync_estab;
	sync->iso.node_rx_estab = node_rx;

	/* Initialize ULL and LLL headers */
	ull_hdr_init(&sync_iso->ull);
	lll_hdr_init(&sync_iso->lll, sync);

	/* Enable periodic advertising to establish ISO sync */
	sync->iso.sync_iso = sync_iso;

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_big_sync_terminate(uint8_t big_handle, void **rx)
{
	struct ll_sync_iso *sync_iso;
	struct node_rx_pdu *node_rx;
	memq_link_t *link_sync_estab;
	memq_link_t *link_sync_lost;
	struct ll_sync_set *sync;
	int err;

	sync_iso = ull_sync_iso_get(big_handle);
	if (!sync_iso) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	sync = sync_iso->sync;
	if (sync) {
		struct node_rx_sync_iso *se;

		if (sync->iso.sync_iso != sync_iso) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
		sync->iso.sync_iso = NULL;
		sync_iso->sync = NULL;

		node_rx = (void *)sync->iso.node_rx_estab;
		link_sync_estab = node_rx->hdr.link;
		link_sync_lost = sync_iso->node_rx_lost.hdr.link;

		ll_rx_link_release(link_sync_lost);
		ll_rx_link_release(link_sync_estab);
		ll_rx_release(node_rx);

		node_rx = (void *)&sync_iso->node_rx_lost;
		node_rx->hdr.type = NODE_RX_TYPE_SYNC_ISO;
		node_rx->hdr.handle = 0xffff;

		/* NOTE: struct node_rx_lost has uint8_t member following the
		 *       struct node_rx_hdr to store the reason.
		 */
		se = (void *)node_rx->pdu;
		se->status = BT_HCI_ERR_OP_CANCELLED_BY_HOST;

		/* NOTE: Since NODE_RX_TYPE_SYNC_ISO is only generated from ULL
		 *       context, pass ULL context as parameter.
		 */
		node_rx->hdr.rx_ftr.param = sync_iso;

		*rx = node_rx;

		return BT_HCI_ERR_SUCCESS;
	}

	err = ull_ticker_stop_with_mark((TICKER_ID_SCAN_SYNC_ISO_BASE +
					 big_handle), sync_iso, &sync_iso->lll);
	LL_ASSERT(err == 0 || err == -EALREADY);
	if (err) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	link_sync_lost = sync_iso->node_rx_lost.hdr.link;
	ll_rx_link_release(link_sync_lost);

	ull_sync_iso_release(sync_iso);

	return BT_HCI_ERR_SUCCESS;
}

int ull_sync_iso_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_sync_iso_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

struct ll_sync_iso *ull_sync_iso_get(uint8_t handle)
{
	if (handle >= CONFIG_BT_CTLR_SCAN_SYNC_ISO_SET) {
		return NULL;
	}

	return &ll_sync_iso_pool[handle];
}

uint8_t ull_sync_iso_handle_get(struct ll_sync_iso *sync)
{
	return mem_index_get(sync, ll_sync_iso_pool,
			     sizeof(struct ll_sync_iso));
}

uint8_t ull_sync_iso_lll_handle_get(struct lll_sync_iso *lll)
{
	return ull_sync_handle_get((void *)HDR_LLL2EVT(lll));
}

void ull_sync_iso_release(struct ll_sync_iso *sync_iso)
{
	mem_release(sync_iso, &sync_iso_free);
}

void ull_sync_iso_setup(struct ll_sync_iso *sync_iso,
			struct node_rx_hdr *node_rx,
			struct pdu_big_info *big_info)
{
	uint32_t ticks_slot_overhead;
	struct node_rx_sync_iso *se;
	uint32_t ticks_slot_offset;
	struct lll_sync_iso *lll;
	struct node_rx_ftr *ftr;
	uint32_t sync_offset_us;
	struct node_rx_pdu *rx;
	uint32_t interval_us;
	uint16_t interval;
	uint16_t handle;
	uint32_t ret;

	lll = &sync_iso->lll;
	handle = ull_sync_iso_handle_get(sync_iso);

	interval = sys_le16_to_cpu(big_info->iso_interval);
	interval_us = interval * CONN_INT_UNIT_US;

	/* TODO: Populate LLL with information from the BIGINFO */

	rx = (void *)sync_iso->sync->iso.node_rx_estab;
	rx->hdr.type = NODE_RX_TYPE_SYNC_ISO;
	rx->hdr.handle = handle;
	rx->hdr.rx_ftr.param = sync_iso;
	se = (void *)rx->pdu;
	se->status = BT_HCI_ERR_SUCCESS;
	se->interval = interval;

	ll_rx_put(rx->hdr.link, rx);
	ll_rx_sched();

	ftr = &node_rx->rx_ftr;

	/* TODO: Calculate offset */
	sync_offset_us = 0;
	ticks_slot_offset = 0;

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}

	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			   (TICKER_ID_SCAN_SYNC_ISO_BASE + handle),
			   ftr->ticks_anchor - ticks_slot_offset,
			   HAL_TICKER_US_TO_TICKS(sync_offset_us),
			   HAL_TICKER_US_TO_TICKS(interval_us),
			   HAL_TICKER_REMAINDER(interval_us),
			   TICKER_NULL_LAZY,
			   ticks_slot_overhead,
			   ticker_cb, sync_iso, ticker_op_cb, (void *)__LINE__);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

static int init_reset(void)
{
	/* Initialize sync pool. */
	mem_init(ll_sync_iso_pool, sizeof(struct ll_sync_iso),
		 sizeof(ll_sync_iso_pool) / sizeof(struct ll_sync_iso),
		 &sync_iso_free);

	return 0;
}

static inline struct ll_sync_iso *sync_iso_acquire(void)
{
	return mem_acquire(&sync_iso_free);
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param)
{
	/* TODO: Implement LLL handling */
#if 0
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_sync_prepare};
	static struct lll_prepare_param p;
	struct ll_sync_set *sync = param;
	struct lll_sync *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_O(1);

	lll = &sync->lll;

	/* Increment prepare reference count */
	ref = ull_ref_inc(&sync->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.param = lll;
	mfy.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
			     TICKER_USER_ID_LLL, 0, &mfy);
	LL_ASSERT(!ret);

	DEBUG_RADIO_PREPARE_O(1);
#endif
}

static void ticker_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}
