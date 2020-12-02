/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <sys/byteorder.h>

#include "test_set_cl_cte_tx_params.h"

struct bt_le_ext_adv *g_adv;

static void common_setup(void)
{
	static struct bt_le_adv_param param =
			BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_EXT_ADV |
					     BT_LE_ADV_OPT_NOTIFY_SCAN_REQ,
					     BT_GAP_ADV_FAST_INT_MIN_2,
					     BT_GAP_ADV_FAST_INT_MAX_2,
					     NULL);
	struct bt_le_per_adv_param per_param = {
		.interval_min = BT_GAP_ADV_SLOW_INT_MIN,
		.interval_max = BT_GAP_ADV_SLOW_INT_MAX,
		.options = BT_LE_ADV_OPT_USE_TX_POWER,
	};
	int err;

	/* Initialize bluetooth subsystem */
	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth subsystem initialization failed");

	err = bt_le_ext_adv_create(&param, NULL, &g_adv);
	zassert_equal(err, 0, "Failed to create advertiser set");


	err = bt_le_per_adv_set_param(g_adv, &per_param);
	zassert_equal(err, 0, "Failed to set periodic advertising params");
}

/*test case main entry*/
void test_main(void)
{
	common_setup();
	run_set_cl_cte_tx_params_tests();
}
