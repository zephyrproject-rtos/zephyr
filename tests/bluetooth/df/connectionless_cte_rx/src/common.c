/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stddef.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <host/hci_core.h>

#include <util/util.h>
#include <util/memq.h>
#include <util/dbuf.h>

#include <pdu_df.h>
#include <lll/pdu_vendor.h>
#include <pdu.h>
#include <lll.h>
#include <lll_scan.h>
#include <lll/lll_df_types.h>
#include <lll_sync.h>
#include <lll_conn.h>
#include <ull_tx_queue.h>
#include <ull_scan_types.h>
#include <ull_scan_internal.h>
#include <ull_conn_types.h>
#include <ull_conn_internal.h>
#include <ull_sync_types.h>
#include <ull_sync_internal.h>

struct bt_le_per_adv_sync *g_per_sync;

static struct bt_le_per_adv_sync_param g_sync_create_param;

void common_create_per_sync_set(void)
{
	struct ll_scan_set *scan;
	struct ll_sync_set *sync;
	int err;

	bt_addr_le_copy(&g_sync_create_param.addr, BT_ADDR_LE_NONE);
	g_sync_create_param.options = 0;
	g_sync_create_param.sid = 0;
	g_sync_create_param.skip = 0;
	g_sync_create_param.timeout = 0xa;

	err = bt_le_per_adv_sync_create(&g_sync_create_param, &g_per_sync);
	zassert_equal(err, 0, "Failed to create periodic sync set");

	/* Below code makes fake sync enable and provides appropriate handle value to
	 * g_per_sync->handle. There is no complete sync established procedure,
	 * because it is not required to test DF functionality.
	 */
	scan = ull_scan_set_get(SCAN_HANDLE_1M);
	sync = scan->periodic.sync;
	g_per_sync->handle = ull_sync_handle_get(sync);
	sync->lll.phy = PHY_2M;
	/* timeout_reload member is used by controller to check if sync was established. */
	sync->timeout_reload = 1;
}
