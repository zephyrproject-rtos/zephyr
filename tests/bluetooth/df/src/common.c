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

/* Example cte length value in allowed range, no particular meaning */
uint8_t g_cte_len = 0x14U;

static struct bt_le_per_adv_param per_param = {
	.interval_min = BT_GAP_ADV_SLOW_INT_MIN,
	.interval_max = BT_GAP_ADV_SLOW_INT_MAX,
	.options = BT_LE_ADV_OPT_USE_TX_POWER,
};

static struct bt_le_ext_adv_start_param ext_adv_start_param = {
	.timeout = 0,
	.num_events = 0,
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

void common_set_cl_cte_tx_params(void)
{
	uint8_t ant_ids[] = { 0x1, 0x2, 0x3, 0x4, 0x5};

	struct bt_hci_cp_le_set_cl_cte_tx_params *cp;
	uint8_t *dest_ant_ids;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_CL_CTE_TX_PARAMS,
				sizeof(*cp) + ARRAY_SIZE(ant_ids));
	zassert_not_null(buf, "Failed to create HCI cmd object");

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = g_adv->handle;
	cp->cte_len = g_cte_len;
	cp->cte_type = BT_HCI_LE_AOD_CTE_2US;
	cp->cte_count = 5;

	dest_ant_ids = net_buf_add(buf, ARRAY_SIZE(ant_ids));
	memcpy(dest_ant_ids, ant_ids, ARRAY_SIZE(ant_ids));

	cp->switch_pattern_len = ARRAY_SIZE(ant_ids);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CL_CTE_TX_PARAMS,
				   buf, NULL);
	zassert_equal(err, 0, "Failed to  set CTE parameters");
}

void common_set_adv_params(void)
{
	int err;

	err = bt_le_per_adv_set_param(g_adv, &per_param);
	zassert_equal(err, 0, "Failed to set periodic advertising params");
}

void common_per_adv_enable(void)
{
	int err;

	err = bt_le_per_adv_start(g_adv);
	zassert_equal(err, 0, "Failed to start periodic advertising");

	err = bt_le_ext_adv_start(g_adv, &ext_adv_start_param);
	zassert_equal(err, 0, "Failed to start extended advertising");
}

void common_per_adv_disable(void)
{
	int err;

	err = bt_le_per_adv_stop(g_adv);
	zassert_equal(err, 0, "Failed to stop periodic advertising");

	err = bt_le_ext_adv_stop(g_adv);
	zassert_equal(err, 0, "Failed to stop extended advertising");
}
