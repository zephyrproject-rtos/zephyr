/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/direction.h>
#include <host/hci_core.h>

#include "common.h"

struct bt_le_ext_adv *g_adv;
struct bt_le_adv_param g_param =
		BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_EXT_ADV |
				     BT_LE_ADV_OPT_NOTIFY_SCAN_REQ,
				     BT_GAP_ADV_FAST_INT_MIN_2,
				     BT_GAP_ADV_FAST_INT_MAX_2,
				     NULL);

static struct bt_le_per_adv_param per_param = {
	.interval_min = BT_GAP_ADV_SLOW_INT_MIN,
	.interval_max = BT_GAP_ADV_SLOW_INT_MAX,
	.options = BT_LE_ADV_OPT_USE_TX_POWER,
};

void common_setup(void)
{
	int err;

	/* Initialize bluetooth subsystem */
	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth subsystem initialization failed");
}

void common_create_adv_set(void)
{
	int err;

	err = bt_le_ext_adv_create(&g_param, NULL, &g_adv);
	zassert_equal(err, 0, "Failed to create advertiser set");
}

void common_delete_adv_set(void)
{
	int err;

	err = bt_le_ext_adv_delete(g_adv);
	zassert_equal(err, 0, "Failed to delete advertiser set");
}
