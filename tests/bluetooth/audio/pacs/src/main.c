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
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(mock_pacs_location_cb, struct bt_conn *, enum bt_audio_dir,
	       enum bt_audio_location);

static struct bt_pacs_cb mock_pacs_cb = {
	.location = mock_pacs_location_cb,
};

static void self_unregistering_location_cb(struct bt_conn *conn, enum bt_audio_dir dir,
					   enum bt_audio_location location);

static struct bt_pacs_cb self_unregistering_pacs_cb = {
	.location = self_unregistering_location_cb,
};

static void self_unregistering_location_cb(struct bt_conn *conn, enum bt_audio_dir dir,
					   enum bt_audio_location location)
{
	(void)bt_pacs_unregister_cb(&self_unregistering_pacs_cb);
}

static void pacs_test_suite_before(void *f)
{
	ARG_UNUSED(f);

	RESET_FAKE(mock_pacs_location_cb);
}

static void pacs_test_suite_after(void *f)
{
	ARG_UNUSED(f);

	/* attempt to clean up after any failures */
	(void)bt_pacs_unregister_cb(&mock_pacs_cb);
	(void)bt_pacs_unregister_cb(&self_unregistering_pacs_cb);
	(void)bt_pacs_unregister();
}

ZTEST_SUITE(pacs_test_suite, NULL, NULL, pacs_test_suite_before, pacs_test_suite_after, NULL);

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

static ZTEST(pacs_test_suite, test_pacs_register_cb)
{
	int err;

	err = bt_pacs_register_cb(&mock_pacs_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_unregister_cb(&mock_pacs_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_register_cb_inval_null)
{
	int err;

	err = bt_pacs_register_cb(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_register_cb_inval_double_register)
{
	int err;

	err = bt_pacs_register_cb(&mock_pacs_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_register_cb(&mock_pacs_cb);
	zassert_equal(err, -EEXIST, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_unregister_cb_inval_null)
{
	int err;

	err = bt_pacs_unregister_cb(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_unregister_cb_inval_not_registered)
{
	int err;

	err = bt_pacs_unregister_cb(&mock_pacs_cb);
	zassert_equal(err, -ENOENT, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_get_location_inval)
{
	enum bt_audio_location location;
	int err;

	err = bt_pacs_get_location(BT_AUDIO_DIR_SINK, NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	err = bt_pacs_get_location((enum bt_audio_dir)0, &location);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	if (!IS_ENABLED(CONFIG_BT_PAC_SNK_LOC)) {
		err = bt_pacs_get_location(BT_AUDIO_DIR_SINK, &location);
		zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
	}

	if (!IS_ENABLED(CONFIG_BT_PAC_SRC_LOC)) {
		err = bt_pacs_get_location(BT_AUDIO_DIR_SOURCE, &location);
		zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
	}
}

static void test_get_location(enum bt_audio_dir dir)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, true, true);
	enum bt_audio_location location;
	int err;

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_set_location(dir, BT_AUDIO_LOCATION_FRONT_RIGHT);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_get_location(dir, &location);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	zassert_equal(location, BT_AUDIO_LOCATION_FRONT_RIGHT, "Unexpected location 0x%08x",
		      location);

	zassert_equal(mock_pacs_location_cb_fake.call_count, 0);
}

static ZTEST(pacs_test_suite, test_pacs_get_location_snk)
{
	if (!IS_ENABLED(CONFIG_BT_PAC_SNK_LOC)) {
		ztest_test_skip();
	}

	zassert_equal(bt_pacs_register_cb(&mock_pacs_cb), 0);
	test_get_location(BT_AUDIO_DIR_SINK);
}

static ZTEST(pacs_test_suite, test_pacs_get_location_src)
{
	if (!IS_ENABLED(CONFIG_BT_PAC_SRC_LOC)) {
		ztest_test_skip();
	}

	zassert_equal(bt_pacs_register_cb(&mock_pacs_cb), 0);
	test_get_location(BT_AUDIO_DIR_SOURCE);
}

static ZTEST(pacs_test_suite, test_pacs_get_location_inval_not_registered)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, false, true, false);
	enum bt_audio_location location = BT_AUDIO_LOCATION_FRONT_LEFT;
	int err;

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_get_location(BT_AUDIO_DIR_SINK, &location);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	err = bt_pacs_get_location(BT_AUDIO_DIR_SOURCE, &location);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_get_location_inval_unregistered)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, true, true);
	enum bt_audio_location location;
	int err;

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	if (IS_ENABLED(CONFIG_BT_PAC_SNK_LOC)) {
		err = bt_pacs_get_location(BT_AUDIO_DIR_SINK, &location);
		zassert_equal(err, 0, "Unexpected return value %d", err);
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC_LOC)) {
		err = bt_pacs_get_location(BT_AUDIO_DIR_SOURCE, &location);
		zassert_equal(err, 0, "Unexpected return value %d", err);
	}

	err = bt_pacs_unregister();
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_get_location(BT_AUDIO_DIR_SINK, &location);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	err = bt_pacs_get_location(BT_AUDIO_DIR_SOURCE, &location);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE) || defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
static ssize_t write_location(const struct bt_uuid *uuid, uint32_t location)
{
	const struct bt_gatt_attr *attr = bt_gatt_find_by_uuid(NULL, 0, uuid);
	uint8_t buf[sizeof(location)];

	zassert_not_null(attr, "Could not find location characteristic");
	zassert_not_null(attr->write, "Location characteristic is not writable");

	sys_put_le32(location, buf);

	return attr->write(NULL, attr, buf, sizeof(buf), 0, 0);
}

static void test_location_write(enum bt_audio_dir dir, const struct bt_uuid *uuid)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, true, true);
	enum bt_audio_location location;
	ssize_t ret;
	int err;

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_set_location(dir, BT_AUDIO_LOCATION_FRONT_LEFT);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_register_cb(&mock_pacs_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	ret = write_location(uuid, BT_AUDIO_LOCATION_FRONT_RIGHT);
	zassert_equal(ret, sizeof(uint32_t), "Unexpected return value %zd", ret);

	zassert_equal(mock_pacs_location_cb_fake.call_count, 1);
	zassert_is_null(mock_pacs_location_cb_fake.arg0_val);
	zassert_equal(mock_pacs_location_cb_fake.arg1_val, dir);
	zassert_equal(mock_pacs_location_cb_fake.arg2_val, BT_AUDIO_LOCATION_FRONT_RIGHT);

	err = bt_pacs_get_location(dir, &location);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	zassert_equal(location, BT_AUDIO_LOCATION_FRONT_RIGHT, "Unexpected location 0x%08x",
		      location);

	ret = write_location(uuid, BT_AUDIO_LOCATION_FRONT_RIGHT);
	zassert_equal(ret, sizeof(uint32_t), "Unexpected return value %zd", ret);
	zassert_equal(mock_pacs_location_cb_fake.call_count, 1);

	ret = write_location(uuid, BIT(28));
	zassert_equal(ret, BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED),
		      "Unexpected return value %zd", ret);
	zassert_equal(mock_pacs_location_cb_fake.call_count, 1);

	err = bt_pacs_unregister_cb(&mock_pacs_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	ret = write_location(uuid, BT_AUDIO_LOCATION_FRONT_LEFT);
	zassert_equal(ret, sizeof(uint32_t), "Unexpected return value %zd", ret);
	zassert_equal(mock_pacs_location_cb_fake.call_count, 1);
}

static void test_location_write_unregister_in_cb(enum bt_audio_dir dir, const struct bt_uuid *uuid)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, true, true);
	ssize_t ret;
	int err;

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_set_location(dir, BT_AUDIO_LOCATION_FRONT_LEFT);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_register_cb(&self_unregistering_pacs_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_register_cb(&mock_pacs_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	ret = write_location(uuid, BT_AUDIO_LOCATION_FRONT_RIGHT);
	zassert_equal(ret, sizeof(uint32_t), "Unexpected return value %zd", ret);
	zassert_equal(mock_pacs_location_cb_fake.call_count, 1);

	err = bt_pacs_unregister_cb(&self_unregistering_pacs_cb);
	zassert_equal(err, -ENOENT, "Unexpected return value %d", err);
}
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE || CONFIG_BT_PAC_SRC_LOC_WRITEABLE */

#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE)
static ZTEST(pacs_test_suite, test_pacs_snk_loc_write_calls_cb)
{
	test_location_write(BT_AUDIO_DIR_SINK, BT_UUID_PACS_SNK_LOC);
}

static ZTEST(pacs_test_suite, test_pacs_snk_loc_write_cb_unregisters_itself)
{
	test_location_write_unregister_in_cb(BT_AUDIO_DIR_SINK, BT_UUID_PACS_SNK_LOC);
}
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE */

#if defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
static ZTEST(pacs_test_suite, test_pacs_src_loc_write_calls_cb)
{
	test_location_write(BT_AUDIO_DIR_SOURCE, BT_UUID_PACS_SRC_LOC);
}

static ZTEST(pacs_test_suite, test_pacs_src_loc_write_cb_unregisters_itself)
{
	test_location_write_unregister_in_cb(BT_AUDIO_DIR_SOURCE, BT_UUID_PACS_SRC_LOC);
}
#endif /* CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
