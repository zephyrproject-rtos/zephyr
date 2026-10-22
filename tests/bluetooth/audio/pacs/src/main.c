/* main.c - Application main entry point */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

DEFINE_FFF_GLOBALS;

static void pacs_test_suite_after(void *f)
{
	ARG_UNUSED(f);

	/* attempt to clean up after any failures */
	(void)bt_pacs_unregister();
}

ZTEST_SUITE(pacs_test_suite, NULL, NULL, NULL, pacs_test_suite_after, NULL);

/* Helper macro to define parameters ignoring unsupported features */
#define PACS_REGISTER_PARAM(_snk_pac, _snk_loc, _src_pac, _src_loc)                                \
	(struct bt_pacs_register_param)                                                            \
	{                                                                                          \
		IF_ENABLED(CONFIG_BT_PAC_SNK, (.snk_pac = (_snk_pac),))                            \
		IF_ENABLED(CONFIG_BT_PAC_SNK_LOC, (.snk_loc = (_snk_loc),))                        \
		IF_ENABLED(CONFIG_BT_PAC_SRC, (.src_pac = (_src_pac),))                            \
		IF_ENABLED(CONFIG_BT_PAC_SRC_LOC, (.src_loc = (_src_loc),))                        \
	}

static ZTEST(pacs_test_suite, test_pacs_register)
{
	const struct bt_pacs_register_param pacs_params[] = {
#if defined(CONFIG_BT_PAC_SNK)
		/* valid snk_pac combinations */
		PACS_REGISTER_PARAM(true, true, true, true),
		PACS_REGISTER_PARAM(true, true, true, false),
		PACS_REGISTER_PARAM(true, true, false, false),
		PACS_REGISTER_PARAM(true, false, true, true),
		PACS_REGISTER_PARAM(true, false, true, false),
		PACS_REGISTER_PARAM(true, false, false, false),
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
		/* valid src_pac combinations */
		PACS_REGISTER_PARAM(true, true, true, true),
		PACS_REGISTER_PARAM(true, false, true, true),
		PACS_REGISTER_PARAM(false, false, true, true),
		PACS_REGISTER_PARAM(true, true, true, false),
		PACS_REGISTER_PARAM(true, false, true, false),
		PACS_REGISTER_PARAM(false, false, true, false),
#endif /* CONFIG_BT_PAC_SRC */
	};

	for (size_t i = 0U; i < ARRAY_SIZE(pacs_params); i++) {
		struct bt_gatt_attr *attr;
		int err;

		err = bt_pacs_register(&pacs_params[i]);
		zassert_equal(err, 0, "[%zu]: Unexpected return value %d", i, err);

#if defined(CONFIG_BT_PAC_SNK)
		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SNK);
		if (pacs_params[i].snk_pac) {
			zassert_not_null(attr, "[%zu]: Could not find sink PAC", i);
		} else {
			zassert_is_null(attr, "[%zu]: Found unexpected sink PAC", i);
		}
#endif /*CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SNK_LOC)
		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SNK_LOC);
		if (pacs_params[i].snk_loc) {
			zassert_not_null(attr, "[%zu]: Could not find sink loc", i);
		} else {
			zassert_is_null(attr, "[%zu]: Found unexpected sink loc", i);
		}
#endif /*CONFIG_BT_PAC_SNK_LOC */
#if defined(CONFIG_BT_PAC_SRC)
		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC);
		if (pacs_params[i].src_pac) {
			zassert_not_null(attr, "[%zu]: Could not find source PAC", i);
		} else {
			zassert_is_null(attr, "[%zu]: Found unexpected source PAC", i);
		}
#endif /*CONFIG_BT_PAC_SRC */
#if defined(CONFIG_BT_PAC_SRC_LOC)
		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC_LOC);
		if (pacs_params[i].src_loc) {
			zassert_not_null(attr, "[%zu]: Could not find source loc", i);
		} else {
			zassert_is_null(attr, "[%zu]: Found unexpected source loc", i);
		}
#endif /*CONFIG_BT_PAC_SRC_LOC */

		err = bt_pacs_unregister();
		zassert_equal(err, 0, "[%zu]: Unexpected return value %d", i, err);

		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SNK);
		zassert_is_null(attr, "[%zu]: Unexpected find of sink PAC", i);

		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SNK_LOC);
		zassert_is_null(attr, "[%zu]: Unexpected find of sink loc", i);

		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC);
		zassert_is_null(attr, "[%zu]: Unexpected find of source PAC", i);

		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC_LOC);
		zassert_is_null(attr, "[%zu]: Unexpected find of source loc", i);
	}
}

struct chrc_search {
	const struct bt_uuid *uuid;
	const struct bt_gatt_attr *decl;
};

static uint8_t chrc_search_cb(const struct bt_gatt_attr *attr, uint16_t handle, void *user_data)
{
	struct chrc_search *search = user_data;
	const struct bt_gatt_chrc *chrc = attr->user_data;

	if (bt_uuid_cmp(chrc->uuid, search->uuid) == 0) {
		search->decl = attr;

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void check_notify_property(const struct bt_uuid *uuid, bool notifiable)
{
	struct chrc_search search = {.uuid = uuid};
	const struct bt_gatt_attr *value;
	const struct bt_gatt_attr *next;
	const struct bt_gatt_chrc *chrc;
	bool has_cccd;

	bt_gatt_foreach_attr_type(BT_ATT_FIRST_ATTRIBUTE_HANDLE, BT_ATT_LAST_ATTRIBUTE_HANDLE,
				  BT_UUID_GATT_CHRC, NULL, 0, chrc_search_cb, &search);
	zassert_not_null(search.decl, "Could not find characteristic declaration");

	chrc = search.decl->user_data;
	value = bt_gatt_attr_next(search.decl);
	zassert_not_null(value, "Could not find characteristic value");
	next = bt_gatt_attr_next(value);
	has_cccd = next != NULL && bt_uuid_cmp(next->uuid, BT_UUID_GATT_CCC) == 0;

	zassert_equal(has_cccd, notifiable, "Unexpected CCCD presence %d", has_cccd);
	zassert_equal((chrc->properties & BT_GATT_CHRC_NOTIFY) != 0U, notifiable,
		      "Unexpected properties 0x%02x", chrc->properties);
}

static ZTEST(pacs_test_suite, test_pacs_pac_notify_property)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, true, true);
	int err;

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	if (IS_ENABLED(CONFIG_BT_PAC_SNK)) {
		check_notify_property(BT_UUID_PACS_SNK, IS_ENABLED(CONFIG_BT_PAC_SNK_NOTIFIABLE));
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC)) {
		check_notify_property(BT_UUID_PACS_SRC, IS_ENABLED(CONFIG_BT_PAC_SRC_NOTIFIABLE));
	}
}

static ZTEST(pacs_test_suite, test_pacs_register_inval_null_param)
{
	int err;

	err = bt_pacs_register(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_register_inval_double_register)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, true, true);
	int err;

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, -EALREADY, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_register_inval_snk_loc_without_snk_pac)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(false, true, true, true);
	int err;

	if (!(IS_ENABLED(CONFIG_BT_PAC_SNK) && IS_ENABLED(CONFIG_BT_PAC_SNK_LOC))) {
		ztest_test_skip();
	}

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_register_inval_src_loc_without_src_pac)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, false, true);
	int err;

	if (!(IS_ENABLED(CONFIG_BT_PAC_SRC) && IS_ENABLED(CONFIG_BT_PAC_SRC_LOC))) {
		ztest_test_skip();
	}

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_register_inval_no_pac)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(false, false, false, false);
	int err;

	if (!(IS_ENABLED(CONFIG_BT_PAC_SNK) && IS_ENABLED(CONFIG_BT_PAC_SNK_LOC))) {
		ztest_test_skip();
	}

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}
