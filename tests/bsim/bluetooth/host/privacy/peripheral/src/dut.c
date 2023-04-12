/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"

#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/toolchain/gcc.h>

bool cb_rpa_expired(struct bt_le_ext_adv *adv)
{
	/* Return true to rotate the current RPA */
	return true;
}

void start_advertising(void)
{
	int err;
	static struct bt_le_ext_adv_cb cb_adv;
	struct bt_le_ext_adv_start_param start_params;
	struct bt_le_ext_adv *adv;

	/* Enable bluetooth */
	err = bt_enable(NULL);
	if (err) {
		FAIL("Failed to enable bluetooth (err %d\n)", err);
	}

	/* Start non-connectable extended advertising with private address */
	start_params.timeout = 0;
	start_params.num_events = 0;
	cb_adv.rpa_expired = cb_rpa_expired;

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN_NAME, &cb_adv, &adv);
	if (err) {
		FAIL("Failed to create advertising set (err %d)\n", err);
	}

	err = bt_le_ext_adv_start(adv, &start_params);
	if (err) {
		FAIL("Failed to enable periodic advertising (err %d)\n", err);
	}
}

void dut_procedure(void)
{
	start_advertising();

	/* Nothing to do */
	PASS("PASS\n");
}
