/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

DEFINE_FLAG_STATIC(flag_adv_sent);

static bool keep_rpa(struct bt_le_ext_adv *adv)
{
	ARG_UNUSED(adv);

	return false;
}

static void adv_sent(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info)
{
	ARG_UNUSED(adv);
	ARG_UNUSED(info);

	SET_FLAG(flag_adv_sent);
}

/* The end of limited advertising asks the application whether to keep the RPA: once when the
 * application stops the advertising, once when its timeout ends it.
 */
void dut_rpa_expired_limited_procedure(void)
{
	static const struct bt_le_ext_adv_cb adv_cb = {
		.sent = adv_sent,
		.rpa_expired = keep_rpa,
	};
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Failed to enable Bluetooth (err %d)", err);

	err = settings_load();
	TEST_ASSERT(err == 0, "Failed to load settings (err %d)", err);

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, &adv_cb, &adv);
	TEST_ASSERT(err == 0, "Failed to create advertising set (err %d)", err);

	/* The timeout only makes the advertising limited: the application stops it first */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_PARAM(500, 0));
	TEST_ASSERT(err == 0, "Failed to start advertising (err %d)", err);

	err = bt_le_ext_adv_stop(adv);
	TEST_ASSERT(err == 0, "Failed to stop advertising (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_PARAM(100, 0));
	TEST_ASSERT(err == 0, "Failed to start advertising (err %d)", err);

	WAIT_FOR_FLAG(flag_adv_sent);

	TEST_PASS_AND_EXIT("Limited advertising stopped and timed out");
}
