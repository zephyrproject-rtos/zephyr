/*
 * Dummy HAS Lower Tester implementation of behavior invalid peer for BSIM HAS Client tests.
 *
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/has.h>

#include "common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(has_dummy_test, LOG_LEVEL_DBG);

extern enum bst_result_t bst_result;
static struct bt_gatt_service service;

static ssize_t write_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *data, uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("conn %p attr %p data %p len %d offset %d flags 0x%02x", (void *)conn, attr, data,
		len, offset, flags);

	return len;
}

static ssize_t read_active_preset_index(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					void *buf, uint16_t len, uint16_t offset)
{
	uint8_t active_index = 0;

	LOG_DBG("conn %p attr %p offset %d", (void *)conn, attr, offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &active_index, sizeof(active_index));
}

static void preset_cp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

static void active_preset_index_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

static struct bt_gatt_attr attrs_no_features_chrc[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_PRESET_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       NULL, write_control_point, NULL),
	BT_GATT_CCC(preset_cp_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_ACTIVE_PRESET_INDEX,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_active_preset_index, NULL, NULL),
	BT_GATT_CCC(active_preset_index_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
};

#define test_preamble(_attrs)                                                                      \
	do {                                                                                       \
		int err;                                                                           \
												   \
		service = (struct bt_gatt_service)BT_GATT_SERVICE(_attrs);                         \
		err = bt_gatt_service_register(&service);                                          \
		__ASSERT_NO_MSG(err == 0);                                                         \
												   \
		LOG_DBG("Service registered");                                                     \
												   \
		err = bt_enable(NULL);                                                             \
		__ASSERT_NO_MSG(err == 0);                                                         \
												   \
		LOG_DBG("Bluetooth initialized");                                                  \
												   \
		err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);                  \
		__ASSERT_NO_MSG(err == 0);                                                         \
												   \
		LOG_DBG("Advertising successfully started");                                       \
	} while (0)

static void test_main_no_features_chrc(void)
{
	test_preamble(attrs_no_features_chrc);

	PASS("HAS passed\n");
}

static ssize_t read_features(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	uint8_t features = 0;

	LOG_DBG("conn %p attr %p offset %d", (void *)conn, attr, offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &features, sizeof(features));
}

static void features_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

static struct bt_gatt_attr attrs_no_active_index_chrc[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_PRESET_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       NULL, write_control_point, NULL),
	BT_GATT_CCC(preset_cp_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_HEARING_AID_FEATURES,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_features, NULL, NULL),
	BT_GATT_CCC(features_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
};

static void test_main_no_active_index_chrc(void)
{
	test_preamble(attrs_no_active_index_chrc);

	PASS("HAS passed\n");
}

static struct bt_gatt_attr attrs_no_active_index_ccc[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_PRESET_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       NULL, write_control_point, NULL),
	BT_GATT_CCC(preset_cp_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_HEARING_AID_FEATURES,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_features, NULL, NULL),
	BT_GATT_CCC(features_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_ACTIVE_PRESET_INDEX,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_active_preset_index, NULL, NULL),
};

static void test_main_no_active_index_ccc(void)
{
	test_preamble(attrs_no_active_index_ccc);

	PASS("HAS passed\n");
}

typedef ssize_t (*ccc_cfg_write_t)(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   uint16_t value);

static ccc_cfg_write_t active_preset_index_ccc_cfg_write_func;

static ssize_t active_preset_index_ccc_cfg_write(struct bt_conn *conn,
						 const struct bt_gatt_attr *attr, uint16_t value)
{
	if (conn != NULL && active_preset_index_ccc_cfg_write_func != NULL) {
		return active_preset_index_ccc_cfg_write_func(conn, attr, value);
	}

	return sizeof(value);
}

static struct bt_gatt_attr attrs_complete[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_PRESET_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       NULL, write_control_point, NULL),
	BT_GATT_CCC(preset_cp_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_HEARING_AID_FEATURES,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_features, NULL, NULL),
	BT_GATT_CCC(features_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_ACTIVE_PRESET_INDEX,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_active_preset_index, NULL, NULL),
	BT_GATT_CCC_MANAGED(((struct _bt_gatt_ccc[]) {
			     BT_GATT_CCC_INITIALIZER(active_preset_index_cfg_changed,
						     active_preset_index_ccc_cfg_write, NULL)}),
			    (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)),
};

static ssize_t ccc_cfg_write_err_unlikely(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					  uint16_t value)
{
	return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
}

static void test_main_active_index_subscribe_err(void)
{
	active_preset_index_ccc_cfg_write_func = ccc_cfg_write_err_unlikely;

	test_preamble(attrs_complete);

	PASS("HAS passed\n");
}

static ssize_t ccc_cfg_write_disconnect(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					uint16_t value)
{
	int err;

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	__ASSERT_NO_MSG(err == 0);

	return sizeof(value);
}

static void test_main_unexpected_disconnection(void)
{
	/* Disconnect the link as soon as client tries to subscribe */
	active_preset_index_ccc_cfg_write_func = ccc_cfg_write_disconnect;

	test_preamble(attrs_complete);

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_UNSET_FLAG(flag_connected);

	/* Reset the function, so that client could subscribe successfully */
	active_preset_index_ccc_cfg_write_func = NULL;

	PASS("HAS passed\n");
}

static const struct bst_test_instance test_has_dummy[] = {
	{
		.test_id = "has_no_features_chrc",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_no_features_chrc,
	},
	{
		.test_id = "has_no_active_index_chrc",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_no_active_index_chrc,
	},
	{
		.test_id = "has_no_active_index_ccc",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_no_active_index_ccc,
	},
	{
		.test_id = "has_active_index_subscribe_err",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_active_index_subscribe_err,
	},
	{
		.test_id = "has_unexpected_disconnection",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_unexpected_disconnection,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_has_dummy_install(struct bst_test_list *tests)
{
	if (IS_ENABLED(CONFIG_BT_HAS)) {
		return bst_add_tests(tests, test_has_dummy);
	} else {
		return tests;
	}
}
