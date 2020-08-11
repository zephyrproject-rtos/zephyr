/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <bluetooth/hci.h>

#include "hal/ccm.h"
#include "hal/radio.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"

#include "pdu.h"
#include "ll.h"

#include "lll.h"
#include "lll_scan.h"
#include "lll_sync.h"

#include "ull_scan_types.h"
#include "ull_sync_types.h"

#include "ull_scan_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_sync
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static inline struct ll_sync_set *sync_acquire(void);

static struct ll_sync_set ll_sync_pool[CONFIG_BT_CTLR_SCAN_SYNC_SET];
static void *sync_free;

uint8_t ll_sync_create(uint8_t options, uint8_t sid, uint8_t adv_addr_type,
			    uint8_t *adv_addr, uint16_t skip,
			    uint16_t sync_timeout, uint8_t sync_cte_type)
{
	struct ll_scan_set *scan_coded = NULL;
	struct ll_scan_set *scan = NULL;
	struct ll_sync_set *sync;

	scan = ull_scan_set_get(SCAN_HANDLE_1M);
	if (!scan || scan->sync) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
		if (!scan_coded || scan_coded->sync) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}

	sync = sync_acquire();
	if (!sync) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	sync->filter_policy = options & BIT(0);
	if (!sync->filter_policy) {
		sync->sid = sid;
		sync->adv_addr_type = adv_addr_type;
		memcpy(sync->adv_addr, adv_addr, BDADDR_SIZE);
	}

	sync->skip = skip;
	sync->skip_countdown = skip;

	sync->timeout = sync_timeout;

	/* TODO: Support for CTE type */

	/* Reporting initially enabled/disabled */
	sync->lll.is_enabled = options & BIT(1);

	/* Enable scanner to create sync */
	scan->sync = sync;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded->sync = sync;
	}

	return 0;
}

uint8_t ll_sync_create_cancel(void)
{
	/* TODO: */
	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_sync_terminate(uint16_t handle)
{
	/* TODO: */
	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_sync_recv_enable(uint16_t handle, uint8_t enable)
{
	/* TODO: */
	return BT_HCI_ERR_CMD_DISALLOWED;
}

int ull_sync_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_sync_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

static int init_reset(void)
{
	/* Initialize sync pool. */
	mem_init(ll_sync_pool, sizeof(struct ll_sync_set),
		 sizeof(ll_sync_pool) / sizeof(struct ll_sync_set),
		 &sync_free);

	return 0;
}

static inline struct ll_sync_set *sync_acquire(void)
{
	return mem_acquire(&sync_free);
}

static inline void sync_release(struct ll_sync_set *sync)
{
	mem_release(sync, &sync_free);
}

static inline uint8_t sync_handle_get(struct ll_sync_set *sync)
{
	return mem_index_get(sync, ll_sync_pool, sizeof(struct ll_sync_set));
}

