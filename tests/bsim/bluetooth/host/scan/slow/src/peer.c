/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/atomic_builtin.h>

#include "testlib/log_utils.h"
#include "babblekit/flags.h"
#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(peer, LOG_LEVEL_DBG);

extern unsigned long runtime_log_level;
extern bool use_chain_adv;

/* bt_data::data_len is uint8_t (max 255), so we use multiple entries
 * sharing the same backing buffer.
 */
static uint8_t mfg_data[250];

void entrypoint_peer(void)
{
	int err;

	TEST_START("peer");

	/* Set the log level given by the `log_level` CLI argument */
	bt_testlib_log_level_set("peer", runtime_log_level);

	err = bt_enable(NULL);
	TEST_ASSERT(!err, "Bluetooth init failed (err %d)\n", err);

	LOG_DBG("Bluetooth initialised");

	struct bt_le_ext_adv *adv;

	struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
		BT_LE_ADV_OPT_EXT_ADV, BT_GAP_ADV_FAST_INT_MIN_1, BT_GAP_ADV_FAST_INT_MAX_1, NULL);

	err = bt_le_ext_adv_create(&adv_param, NULL, &adv);
	TEST_ASSERT(!err, "Failed to create advertising set: %d", err);
	LOG_DBG("Created extended advertising set.");

	if (use_chain_adv) {
		(void)memset(mfg_data, 0xAA, sizeof(mfg_data));

		const struct bt_data ad_chain[] = {
			BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
			BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
			BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
			BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
		};

		err = bt_le_ext_adv_set_data(adv, ad_chain, ARRAY_SIZE(ad_chain), NULL, 0U);
		TEST_ASSERT(err == 0, "Failed to set advertising data: %d", err);
		LOG_DBG("Set chain advertising data.");
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Failed to start extended advertising: %d", err);
	LOG_DBG("Started extended advertising.");

	TEST_PASS("Tester done");
}
