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

static struct ll_sync_iso ll_sync_iso_pool[CONFIG_BT_CTLR_SCAN_SYNC_ISO_MAX];
static void *sync_iso_free;

static int init_reset(void);
static inline struct ll_sync_iso *sync_iso_acquire(void);

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
	lll_sync->is_enabled = true;

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_big_sync_terminate(uint8_t big_handle)
{
	memq_link_t *big_sync_estab;
	memq_link_t *big_sync_lost;
	struct ll_sync_iso *sync_iso;
	void *mark;

	sync_iso = ull_sync_iso_get(big_handle);
	if (!sync_iso) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	if (!sync_iso->lll.is_enabled) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	mark = ull_disable_mark(sync_iso);
	LL_ASSERT(mark == sync_iso);

	/* TODO: Stop ticker */

	mark = ull_disable_unmark(sync_iso);
	LL_ASSERT(mark == sync_iso);

	big_sync_lost = sync_iso->node_rx_lost.link;
	ll_rx_link_release(big_sync_lost);

	big_sync_estab = sync_iso->node_rx_estab.link;
	ll_rx_link_release(big_sync_estab);

	sync_iso->lll.is_enabled = false;
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
	if (handle >= CONFIG_BT_CTLR_SCAN_SYNC_ISO_MAX) {
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
	/* TODO: Implement and start ticker.
	 * Depends on ACAD (biginfo) support
	 */
	ARG_UNUSED(sync_iso);
	ARG_UNUSED(node_rx);
	ARG_UNUSED(biginfo);
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
