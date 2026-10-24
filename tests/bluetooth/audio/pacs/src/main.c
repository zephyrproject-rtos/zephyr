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
#include <zephyr/kernel.h>
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
	int err;

	err = bt_pacs_unregister_cb(&self_unregistering_pacs_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);
}

#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE) || defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
FAKE_VOID_FUNC(mock_pacs_location_cb_2, struct bt_conn *, enum bt_audio_dir,
	       enum bt_audio_location);

static struct bt_pacs_cb mock_pacs_cb_2 = {
	.location = mock_pacs_location_cb_2,
};

static void blocking_location_cb(struct bt_conn *conn, enum bt_audio_dir dir,
				 enum bt_audio_location location);

static struct bt_pacs_cb blocking_pacs_cb = {
	.location = blocking_location_cb,
};

static K_SEM_DEFINE(in_cb, 0, 1);
static K_SEM_DEFINE(release_cb, 0, 1);
static K_SEM_DEFINE(op_done, 0, 1);

static K_THREAD_STACK_DEFINE(writer_stack, CONFIG_ZTEST_STACK_SIZE);
static K_THREAD_STACK_DEFINE(op_stack, CONFIG_ZTEST_STACK_SIZE);
static struct k_thread writer_thread;
static struct k_thread op_thread;
static k_tid_t writer_tid;
static k_tid_t op_tid;

static const struct bt_gatt_attr *writer_attr;
static uint8_t writer_buf[sizeof(uint32_t)];
static ssize_t writer_ret;
static int op_ret;
static int blocking_cb_calls;
static int blocking_cb_release_ret;

static void blocking_location_cb(struct bt_conn *conn, enum bt_audio_dir dir,
				 enum bt_audio_location location)
{
	blocking_cb_calls++;
	k_sem_give(&in_cb);
	blocking_cb_release_ret = k_sem_take(&release_cb, K_SECONDS(1));
}
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE || CONFIG_BT_PAC_SRC_LOC_WRITEABLE */

static void pacs_test_suite_before(void *f)
{
	ARG_UNUSED(f);

	RESET_FAKE(mock_pacs_location_cb);
#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE) || defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
	RESET_FAKE(mock_pacs_location_cb_2);
	k_sem_reset(&in_cb);
	k_sem_reset(&release_cb);
	k_sem_reset(&op_done);
	writer_ret = 0;
	op_ret = 0;
	blocking_cb_calls = 0;
	blocking_cb_release_ret = 0;
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE || CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
}

static void pacs_test_suite_after(void *f)
{
	int err;

	ARG_UNUSED(f);

#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE) || defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
	k_sem_give(&release_cb);

	if (writer_tid != NULL) {
		err = k_thread_join(writer_tid, K_SECONDS(1));
		writer_tid = NULL;
		if (err != 0) {
			zassert_unreachable("Writer thread did not finish: %d", err);
			return;
		}
	}

	if (op_tid != NULL) {
		err = k_thread_join(op_tid, K_SECONDS(1));
		op_tid = NULL;
		if (err != 0) {
			zassert_unreachable("Op thread did not finish: %d", err);
			return;
		}
	}

	err = bt_pacs_unregister_cb(&mock_pacs_cb_2);
	zassert_true(err == 0 || err == -ENOENT, "Unexpected return value %d", err);

	err = bt_pacs_unregister_cb(&blocking_pacs_cb);
	zassert_true(err == 0 || err == -ENOENT, "Unexpected return value %d", err);
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE || CONFIG_BT_PAC_SRC_LOC_WRITEABLE */

	/* attempt to clean up after any failures */
	err = bt_pacs_unregister_cb(&mock_pacs_cb);
	zassert_true(err == 0 || err == -ENOENT, "Unexpected return value %d", err);

	err = bt_pacs_unregister_cb(&self_unregistering_pacs_cb);
	zassert_true(err == 0 || err == -ENOENT, "Unexpected return value %d", err);

	err = bt_pacs_unregister();
	zassert_true(err == 0 || err == -EALREADY, "Unexpected return value %d", err);
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
	zassert_equal(err, IS_ENABLED(CONFIG_BT_PAC_SNK_LOC) ? -ENOENT : -EINVAL,
		      "Unexpected return value %d", err);

	err = bt_pacs_get_location(BT_AUDIO_DIR_SOURCE, &location);
	zassert_equal(err, IS_ENABLED(CONFIG_BT_PAC_SRC_LOC) ? -ENOENT : -EINVAL,
		      "Unexpected return value %d", err);
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
	zassert_equal(err, IS_ENABLED(CONFIG_BT_PAC_SNK_LOC) ? -ENOENT : -EINVAL,
		      "Unexpected return value %d", err);

	err = bt_pacs_get_location(BT_AUDIO_DIR_SOURCE, &location);
	zassert_equal(err, IS_ENABLED(CONFIG_BT_PAC_SRC_LOC) ? -ENOENT : -EINVAL,
		      "Unexpected return value %d", err);
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

static void writer_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	writer_ret = writer_attr->write(NULL, writer_attr, writer_buf, sizeof(writer_buf), 0, 0);
}

static void unregister_op_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	op_ret = bt_pacs_unregister_cb(&blocking_pacs_cb);
	k_sem_give(&op_done);
}

static void register_op_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	op_ret = bt_pacs_register_cb(&mock_pacs_cb_2);
	k_sem_give(&op_done);
}

static void start_blocked_write(enum bt_audio_dir dir, const struct bt_uuid *uuid)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, true, true);
	int err;

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_set_location(dir, BT_AUDIO_LOCATION_FRONT_LEFT);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_register_cb(&blocking_pacs_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	writer_attr = bt_gatt_find_by_uuid(NULL, 0, uuid);
	zassert_not_null(writer_attr, "Could not find location characteristic");
	zassert_not_null(writer_attr->write, "Location characteristic is not writable");

	sys_put_le32(BT_AUDIO_LOCATION_FRONT_RIGHT, writer_buf);

	writer_tid = k_thread_create(&writer_thread, writer_stack,
				     K_THREAD_STACK_SIZEOF(writer_stack), writer_entry, NULL, NULL,
				     NULL, K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1), 0,
				     K_NO_WAIT);

	err = k_sem_take(&in_cb, K_SECONDS(1));
	zassert_equal(err, 0, "Callback was not called: %d", err);
}

static void run_op_while_blocked(k_thread_entry_t op_entry)
{
	int err;

	op_tid = k_thread_create(&op_thread, op_stack, K_THREAD_STACK_SIZEOF(op_stack), op_entry,
				 NULL, NULL, NULL,
				 K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1), 0, K_NO_WAIT);

	err = k_sem_take(&op_done, K_MSEC(100));
	zassert_equal(err, -EAGAIN, "Operation did not wait for the callback: %d", err);

	k_sem_give(&release_cb);

	err = k_sem_take(&op_done, K_SECONDS(1));
	zassert_equal(err, 0, "Operation did not finish: %d", err);

	err = k_thread_join(writer_tid, K_SECONDS(1));
	zassert_equal(err, 0, "Writer did not finish: %d", err);
	writer_tid = NULL;

	err = k_thread_join(op_tid, K_SECONDS(1));
	zassert_equal(err, 0, "Op thread did not finish: %d", err);
	op_tid = NULL;

	zassert_equal(blocking_cb_release_ret, 0, "Callback was not released: %d",
		      blocking_cb_release_ret);
	zassert_equal(writer_ret, sizeof(uint32_t), "Unexpected return value %zd", writer_ret);
	zassert_equal(blocking_cb_calls, 1, "Unexpected call count %d", blocking_cb_calls);
}

static void test_location_write_unregister_waits_for_cb(enum bt_audio_dir dir,
							const struct bt_uuid *uuid)
{
	ssize_t ret;

	start_blocked_write(dir, uuid);
	run_op_while_blocked(unregister_op_entry);

	zassert_equal(op_ret, 0, "Unexpected return value %d", op_ret);

	ret = write_location(uuid, BT_AUDIO_LOCATION_FRONT_LEFT);
	zassert_equal(ret, sizeof(uint32_t), "Unexpected return value %zd", ret);
	zassert_equal(blocking_cb_calls, 1, "Unexpected call count %d", blocking_cb_calls);
}

static void test_location_write_register_waits_for_cb(enum bt_audio_dir dir,
						      const struct bt_uuid *uuid)
{
	ssize_t ret;
	int err;

	start_blocked_write(dir, uuid);
	run_op_while_blocked(register_op_entry);

	zassert_equal(op_ret, 0, "Unexpected return value %d", op_ret);
	zassert_equal(mock_pacs_location_cb_2_fake.call_count, 0);

	err = bt_pacs_unregister_cb(&blocking_pacs_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	ret = write_location(uuid, BT_AUDIO_LOCATION_FRONT_LEFT);
	zassert_equal(ret, sizeof(uint32_t), "Unexpected return value %zd", ret);
	zassert_equal(mock_pacs_location_cb_2_fake.call_count, 1);
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

static ZTEST(pacs_test_suite, test_pacs_snk_loc_write_unregister_cb_waits_for_cb)
{
	test_location_write_unregister_waits_for_cb(BT_AUDIO_DIR_SINK, BT_UUID_PACS_SNK_LOC);
}

static ZTEST(pacs_test_suite, test_pacs_snk_loc_write_register_cb_waits_for_cb)
{
	test_location_write_register_waits_for_cb(BT_AUDIO_DIR_SINK, BT_UUID_PACS_SNK_LOC);
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

static ZTEST(pacs_test_suite, test_pacs_src_loc_write_unregister_cb_waits_for_cb)
{
	test_location_write_unregister_waits_for_cb(BT_AUDIO_DIR_SOURCE, BT_UUID_PACS_SRC_LOC);
}

static ZTEST(pacs_test_suite, test_pacs_src_loc_write_register_cb_waits_for_cb)
{
	test_location_write_register_waits_for_cb(BT_AUDIO_DIR_SOURCE, BT_UUID_PACS_SRC_LOC);
}
#endif /* CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
