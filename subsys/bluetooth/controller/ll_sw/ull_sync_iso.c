/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_sync_iso
#include "common/log.h"
#include "hal/debug.h"

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
#include "ll.h"

#include "lll.h"
#include "lll_vendor.h"
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
	memq_link_t *big_sync_estab;
	memq_link_t *big_sync_lost;
	struct lll_sync_iso *lll_sync;
	struct ll_sync_set *sync;
	struct ll_sync_iso *sync_iso;

	sync = ull_sync_is_enabled_get(sync_handle);
	if (!sync || sync->sync_iso) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	sync_iso = sync_iso_acquire();
	if (!sync_iso) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	big_sync_estab = ll_rx_link_alloc();
	if (!big_sync_estab) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	big_sync_lost = ll_rx_link_alloc();
	if (!big_sync_lost) {
		ll_rx_link_release(big_sync_estab);

		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	sync_iso->node_rx_lost.link = big_sync_lost;
	sync_iso->node_rx_estab.link = big_sync_estab;

	lll_sync = &sync_iso->lll;

	/* Initialise ULL and LLL headers */
	ull_hdr_init(&sync_iso->ull);
	lll_hdr_init(lll_sync, sync);

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_big_sync_terminate(uint8_t big_handle)
{
	memq_link_t *big_sync_estab;
	memq_link_t *big_sync_lost;
	struct ll_sync_iso *sync_iso;
	int err;

	sync_iso = ull_sync_iso_get(big_handle);
	if (!sync_iso) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	err = ull_ticker_stop_with_mark(
		TICKER_ID_SCAN_SYNC_ISO_BASE + big_handle,
		sync_iso, &sync_iso->lll);
	LL_ASSERT(err == 0 || err == -EALREADY);
	if (err) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	big_sync_lost = sync_iso->node_rx_lost.link;
	ll_rx_link_release(big_sync_lost);

	big_sync_estab = sync_iso->node_rx_estab.link;
	ll_rx_link_release(big_sync_estab);

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
			struct pdu_biginfo *biginfo)
{
	uint16_t handle;
	struct node_rx_sync_iso *se;
	uint16_t interval;
	uint32_t interval_us;
	uint32_t ticks_slot_overhead;
	uint32_t ticks_slot_offset;
	uint32_t ret;
	struct node_rx_ftr *ftr;
	uint32_t sync_offset_us;
	struct node_rx_pdu *rx;
	struct lll_sync_iso *lll;

	lll = &sync_iso->lll;
	handle = ull_sync_iso_handle_get(sync_iso);

	interval = sys_le16_to_cpu(biginfo->iso_interval);
	interval_us = interval * CONN_INT_UNIT_US;

	/* TODO: Populate LLL with information from the BIGINFO */

	rx = (void *)sync_iso->node_rx_estab.link;
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
