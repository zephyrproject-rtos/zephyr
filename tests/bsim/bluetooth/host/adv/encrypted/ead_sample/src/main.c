/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/ead.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bstests.h"

#include <zephyr/logging/log.h>

#include "data.h"
#include "common.h"

LOG_MODULE_REGISTER(bt_bsim_ead_sample, CONFIG_BT_EAD_LOG_LEVEL);

#define FAIL(...)                                                                                  \
	do {                                                                                       \
		bst_result = Failed;                                                               \
		bs_trace_error_time_line(__VA_ARGS__);                                             \
	} while (0)

#define PASS(...)                                                                                  \
	do {                                                                                       \
		bst_result = Passed;                                                               \
		bs_trace_info_time(1, __VA_ARGS__);                                                \
	} while (0)

extern enum bst_result_t bst_result;

#define WAIT_TIME_S 60
#define WAIT_TIME   (WAIT_TIME_S * 1e6)

extern int run_peripheral_sample(int get_passkey_confirmation(struct bt_conn *conn));
extern int run_central_sample(int get_passkey_confirmation(struct bt_conn *conn),
			      uint8_t *received_data, size_t received_data_size,
			      struct key_material *received_keymat);

static int get_passkey_confirmation(struct bt_conn *conn)
{
	int err;

	err = bt_conn_auth_passkey_confirm(conn);
	if (err) {
		LOG_ERR("Failed to confirm passkey (err %d)", err);
		return -1;
	}

	printk("Passkey confirmed.\n");

	return 0;
}

static atomic_t failed_connections;

static void test_connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		(void)atomic_inc(&failed_connections);
	}
}

BT_CONN_CB_DEFINE(test_conn_callbacks) = {
	.connected = test_connected,
};

static void central_run(void)
{
	int err;

	size_t offset;
	size_t expected_data_size = bt_data_get_len(ad, ARRAY_SIZE(ad));
	uint8_t expected_data[expected_data_size];

	uint8_t received_data[expected_data_size];
	struct key_material received_keymat;

	offset = 0;
	for (size_t i = 0; i < ARRAY_SIZE(ad); i++) {
		offset += bt_data_serialize(&ad[i], &expected_data[offset]);
	}

	err = run_central_sample(get_passkey_confirmation, received_data, expected_data_size,
				 &received_keymat);

	LOG_DBG("Expected data size: %zu", expected_data_size);

	LOG_HEXDUMP_DBG(received_data, expected_data_size, "Received data");
	LOG_HEXDUMP_DBG(received_keymat.session_key, BT_EAD_KEY_SIZE, "Receveid key");
	LOG_HEXDUMP_DBG(received_keymat.iv, BT_EAD_IV_SIZE, "Received IV");

	if (err != 0) {
		FAIL("Central test failed. (sample err %d)\n", err);
	}

	if (memcmp(&received_keymat, &mk, BT_EAD_KEY_SIZE + BT_EAD_IV_SIZE) != 0) {
		FAIL("Received Key Material does not match expected one.");
	}

	if (memcmp(expected_data, received_data, expected_data_size) != 0) {
		FAIL("Received data does not match expected ones.\n");
	}
}

static void central_main(void)
{
	central_run();

	PASS("Central test passed.\n");
}

static void central_retry_main(void)
{
	central_run();

	/* The central has to have tried the decoy first */
	if (atomic_get(&failed_connections) == 0) {
		FAIL("No connection attempt failed\n");
	}

	PASS("Central test passed.\n");
}

static void peripheral_main(void)
{
	int err = run_peripheral_sample(get_passkey_confirmation);

	if (err) {
		FAIL("Peripheral test failed. (sample err %d)\n");
	}

	PASS("Peripheral test passed.\n");
}

static void decoy_main(void)
{
	/* Connectable, but with an empty filter accept list it ignores every connection request,
	 * so that the central's connection attempt to it times out
	 */
	const struct bt_le_adv_param *param = BT_LE_ADV_PARAM(
		BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_FILTER_CONN,
		BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL);
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
	}

	err = bt_le_ext_adv_create(param, NULL, &adv);
	if (err != 0) {
		FAIL("Failed to create advertiser (err %d)\n", err);
	}

	/* Stop after 2 s, before that attempt times out after 3 s, so that the next scan of the
	 * central finds only the peripheral
	 */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_PARAM(200, 0));
	if (err != 0) {
		FAIL("Failed to start advertiser (err %d)\n", err);
	}

	PASS("Decoy test passed.\n");
}

void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error_time_line("Test failed (not passed after %d seconds)\n",
					 WAIT_TIME_S);
	}
}

static void test_ead_sample_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central device",
		.test_pre_init_f = test_ead_sample_init,
		.test_tick_f = test_tick,
		.test_main_f = central_main,
	},
	{
		.test_id = "central_retry",
		.test_descr = "Central device whose first connection attempt fails",
		.test_pre_init_f = test_ead_sample_init,
		.test_tick_f = test_tick,
		.test_main_f = central_retry_main,
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral device",
		.test_pre_init_f = test_ead_sample_init,
		.test_tick_f = test_tick,
		.test_main_f = peripheral_main,
	},
	{
		.test_id = "decoy",
		.test_descr = "Advertiser that accepts no connection request",
		.test_main_f = decoy_main,
	},
	BSTEST_END_MARKER};

struct bst_test_list *test_ead_sample_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_ead_sample_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
