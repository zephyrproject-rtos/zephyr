/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include <fake_hci.h>

/* SID of the most recent LE Set Extended Advertising Parameters command. */
static uint8_t last_hci_sid;

/* Store the SID that the Host sent to the Controller so that the test can verify it. The
 * Controller answers the command.
 */
static bool cmd_cb(uint16_t opcode, struct net_buf *buf)
{
	const struct bt_hci_cp_le_set_ext_adv_param *cp = (const void *)buf->data;

	if (opcode == BT_HCI_OP_LE_SET_EXT_ADV_PARAM) {
		zassert_true(buf->len >= sizeof(*cp), "Unexpected command length %u", buf->len);
		last_hci_sid = cp->sid;
	}

	return false;
}

static struct bt_le_adv_param adv_param_get(uint8_t sid, bool ext_adv)
{
	struct bt_le_adv_param param = {
		.id = BT_ID_DEFAULT,
		.sid = sid,
		.secondary_max_skip = 0U,
		.options = ext_adv ? BT_LE_ADV_OPT_EXT_ADV : BT_LE_ADV_OPT_NONE,
		.interval_min = BT_GAP_ADV_SLOW_INT_MIN,
		.interval_max = BT_GAP_ADV_SLOW_INT_MAX,
		.peer = NULL,
	};

	return param;
}

static uint8_t adv_sid_get(struct bt_le_ext_adv *adv)
{
	struct bt_le_ext_adv_info info;
	int err;

	err = bt_le_ext_adv_get_info(adv, &info);
	zassert_ok(err, "Failed to get advertising set info: %d", err);

	return info.sid;
}

static void *adv_sid_setup(void)
{
	static const struct fake_hci_cb cb = {
		.cmd = cmd_cb,
	};
	int err;

	/* Only report support for LE Extended Advertising */
	fake_hci.le_features[BT_LE_FEAT_BIT_EXT_ADV / 8] = BIT(BT_LE_FEAT_BIT_EXT_ADV % 8);
	fake_hci_cb_set(&cb);

	err = bt_enable(NULL);
	zassert_ok(err, "Bluetooth init failed: %d", err);

	return NULL;
}

static void adv_sid_before(void *f)
{
	ARG_UNUSED(f);

	last_hci_sid = BT_HCI_LE_EXT_ADV_SID_INVALID;
}

ZTEST_SUITE(bt_adv_sid, NULL, adv_sid_setup, adv_sid_before, NULL, NULL);

static ZTEST(bt_adv_sid, test_ext_adv_uses_param_sid)
{
	const uint8_t sids[] = {BT_GAP_SID_MIN, 5U, BT_GAP_SID_MAX};

	for (size_t i = 0U; i < ARRAY_SIZE(sids); i++) {
		struct bt_le_adv_param param = adv_param_get(sids[i], true);
		struct bt_le_ext_adv *adv;
		int err;

		err = bt_le_ext_adv_create(&param, NULL, &adv);
		zassert_ok(err, "Failed to create advertising set with SID %u: %d", sids[i], err);

		zassert_equal(adv_sid_get(adv), sids[i],
			      "SID %u was not set in the advertising set", sids[i]);
		zassert_equal(last_hci_sid, sids[i], "SID %u was not sent to the controller",
			      sids[i]);

		err = bt_le_ext_adv_delete(adv);
		zassert_ok(err, "Failed to delete advertising set: %d", err);
	}
}

static ZTEST(bt_adv_sid, test_legacy_adv_sid_is_invalid)
{
	struct bt_le_adv_param param = adv_param_get(BT_GAP_SID_MAX + 1U, false);
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_ext_adv_create(&param, NULL, &adv);
	zassert_ok(err, "Failed to create legacy advertising set: %d", err);

	zassert_equal(adv_sid_get(adv), BT_GAP_SID_INVALID,
		      "SID of a legacy advertising set shall be BT_GAP_SID_INVALID");
	zassert_equal(last_hci_sid, 0U, "SID sent to the controller shall be 0 for legacy");

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}

static ZTEST(bt_adv_sid, test_ext_adv_invalid_sid_rejected)
{
	const uint8_t sids[] = {BT_GAP_SID_MAX + 1U, BT_GAP_SID_INVALID};

	for (size_t i = 0U; i < ARRAY_SIZE(sids); i++) {
		struct bt_le_adv_param param = adv_param_get(sids[i], true);
		struct bt_le_ext_adv *adv;
		int err;

		err = bt_le_ext_adv_create(&param, NULL, &adv);
		zassert_equal(err, -EINVAL, "Creating advertising set with SID %u returned %d",
			      sids[i], err);
	}
}

static ZTEST(bt_adv_sid, test_update_param_updates_sid)
{
	struct bt_le_adv_param param = adv_param_get(3U, true);
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_ext_adv_create(&param, NULL, &adv);
	zassert_ok(err, "Failed to create advertising set: %d", err);
	zassert_equal(adv_sid_get(adv), 3U, "SID was not set in the advertising set");

	/* Updating with legacy parameters shall invalidate the SID */
	param = adv_param_get(BT_GAP_SID_MAX + 1U, false);
	err = bt_le_ext_adv_update_param(adv, &param);
	zassert_ok(err, "Failed to update advertising parameters: %d", err);
	zassert_equal(adv_sid_get(adv), BT_GAP_SID_INVALID,
		      "SID of a legacy advertising set shall be BT_GAP_SID_INVALID");

	/* Updating with extended parameters shall set the SID again */
	param = adv_param_get(7U, true);
	err = bt_le_ext_adv_update_param(adv, &param);
	zassert_ok(err, "Failed to update advertising parameters: %d", err);
	zassert_equal(adv_sid_get(adv), 7U, "SID was not set in the advertising set");

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}
