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

#include "ull_internal.h"
#include "ull_scan_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_sync
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static inline struct ll_sync_set *sync_acquire(void);
static struct ll_sync_set *is_enabled_get(uint8_t handle);

static struct ll_sync_set ll_sync_pool[CONFIG_BT_CTLR_SCAN_SYNC_SET];
static void *sync_free;

uint8_t ll_sync_create(uint8_t options, uint8_t sid, uint8_t adv_addr_type,
			    uint8_t *adv_addr, uint16_t skip,
			    uint16_t sync_timeout, uint8_t sync_cte_type)
{
	struct ll_scan_set *scan_coded;
	memq_link_t *link_sync_estab;
	memq_link_t *link_sync_lost;
	struct node_rx_hdr *node_rx;
	struct ll_scan_set *scan;
	struct ll_sync_set *sync;

	scan = ull_scan_set_get(SCAN_HANDLE_1M);
	if (!scan || scan->per_scan.sync) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
		if (!scan_coded || scan_coded->per_scan.sync) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
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

	sync = sync_acquire();
	if (!sync) {
		ll_rx_release(node_rx);
		ll_rx_link_release(link_sync_lost);
		ll_rx_link_release(link_sync_estab);

		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	scan->per_scan.filter_policy = options & BIT(0);
	if (!scan->per_scan.filter_policy) {
		scan->per_scan.sid = sid;
		scan->per_scan.adv_addr_type = adv_addr_type;
		memcpy(scan->per_scan.adv_addr, adv_addr, BDADDR_SIZE);
	}

	/* TODO: Support for skip */

	/* TODO: Support for timeout */

	/* TODO: Support for CTE type */

	/* Reporting initially enabled/disabled */
	sync->lll.is_enabled = options & BIT(1);

	/* Initialise state */
	scan->per_scan.state = LL_SYNC_STATE_IDLE;

	/* established and sync_lost node_rx */
	node_rx->link = link_sync_estab;
	scan->per_scan.node_rx_estab = node_rx;
	scan->per_scan.node_rx_lost.link = link_sync_lost;

	/* Initialise ULL and LLL headers */
	ull_hdr_init(&sync->ull);
	lll_hdr_init(&sync->lll, sync);

	/* Enable scanner to create sync */
	scan->per_scan.sync = sync;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded->per_scan.sync = sync;
	}

	return 0;
}

uint8_t ll_sync_create_cancel(void **rx)
{
	struct ll_scan_set *scan_coded;
	memq_link_t *link_sync_estab;
	memq_link_t *link_sync_lost;
	struct node_rx_pdu *node_rx;
	struct ll_scan_set *scan;
	struct ll_sync_set *sync;
	struct node_rx_sync *se;

	scan = ull_scan_set_get(SCAN_HANDLE_1M);
	if (!scan || !scan->per_scan.sync) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
		if (!scan_coded || !scan_coded->per_scan.sync) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}

	sync = scan->per_scan.sync;
	scan->per_scan.sync = NULL;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded->per_scan.sync = NULL;
	}

	/* Check for race condition where in sync is established when sync
	 * context was set to NULL.
	 */
	if (sync->is_enabled) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	node_rx = (void *)scan->per_scan.node_rx_estab;
	link_sync_estab = node_rx->hdr.link;
	link_sync_lost = scan->per_scan.node_rx_lost.link;

	ll_rx_link_release(link_sync_lost);
	ll_rx_link_release(link_sync_estab);
	ll_rx_release(node_rx);

	node_rx = (void *)&scan->per_scan.node_rx_lost;
	node_rx->hdr.type = NODE_RX_TYPE_SYNC;
	node_rx->hdr.handle = 0xffff;
	node_rx->hdr.rx_ftr.param = sync;
	se = (void *)node_rx->pdu;
	se->status = BT_HCI_ERR_OP_CANCELLED_BY_HOST;

	*rx = node_rx;

	return 0;
}

uint8_t ll_sync_terminate(uint16_t handle)
{
	struct ll_sync_set *sync;

	sync = is_enabled_get(handle);
	if (!sync) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* TODO: Stop periodic sync events */

	return 0;
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

struct ll_sync_set *ull_sync_set_get(uint8_t handle)
{
	if (handle >= CONFIG_BT_CTLR_SCAN_SYNC_SET) {
		return NULL;
	}

	return &ll_sync_pool[handle];
}

uint16_t ull_sync_handle_get(struct ll_sync_set *sync)
{
	return mem_index_get(sync, ll_sync_pool, sizeof(struct ll_sync_set));
}

void ull_sync_release(struct ll_sync_set *sync)
{
	mem_release(sync, &sync_free);
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

static struct ll_sync_set *is_enabled_get(uint8_t handle)
{
	struct ll_sync_set *sync;

	sync = ull_sync_set_get(handle);
	if (!sync || !sync->is_enabled) {
		return NULL;
	}

	return sync;
}
