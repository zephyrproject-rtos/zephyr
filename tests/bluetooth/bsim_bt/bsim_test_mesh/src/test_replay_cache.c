/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "settings_test_backend.h"
#include "mesh/mesh.h"
#include "mesh/net.h"

#define LOG_MODULE_NAME test_rpc

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define WAIT_TIME 60 /*seconds*/
#define TEST_DATA_WAITING_TIME 5 /* seconds */
#define TEST_DATA_SIZE 20

static const struct bt_mesh_test_cfg tx_cfg = {
	.addr = 0x0001,
	.dev_key = { 0x01 },
};
static const struct bt_mesh_test_cfg rx_cfg = {
	.addr = 0x0002,
	.dev_key = { 0x02 },
};

static uint8_t test_data[TEST_DATA_SIZE];
static uint8_t rx_cnt;
static bool is_tx_succeeded;

static void test_tx_init(void)
{
	bt_mesh_test_cfg_set(&tx_cfg, WAIT_TIME);
}

static void test_rx_init(void)
{
	bt_mesh_test_cfg_set(&rx_cfg, WAIT_TIME);
}

static void tx_started(uint16_t dur, int err, void *data)
{
	if (err) {
		FAIL("Couldn't start sending (err: %d)", err);
	}

	LOG_INF("Sending started");
}

static void tx_ended(int err, void *data)
{
	struct k_sem *sem = data;

	if (err) {
		is_tx_succeeded = false;
		LOG_INF("Sending failed (%d)", err);
	} else {
		is_tx_succeeded = true;
		LOG_INF("Sending succeeded");
	}

	k_sem_give(sem);
}

static void rx_ended(uint8_t *data, size_t len)
{
	memset(test_data, rx_cnt++, sizeof(test_data));

	if (memcmp(test_data, data, len)) {
		FAIL("Unexpected rx data");
	}

	LOG_INF("Receiving succeeded");
}

static void test_tx_immediate_replay_attack(void)
{
	settings_test_backend_clear();
	bt_mesh_test_setup();

	static const struct bt_mesh_send_cb send_cb = {
		.start = tx_started,
		.end = tx_ended,
	};
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);

	uint32_t seq = bt_mesh.seq;

	for (int i = 0; i < 3; i++) {
		is_tx_succeeded = false;

		memset(test_data, i, sizeof(test_data));
		ASSERT_OK(bt_mesh_test_send_ra(rx_cfg.addr, test_data,
			sizeof(test_data), &send_cb, &sem));

		if (k_sem_take(&sem, K_SECONDS(TEST_DATA_WAITING_TIME))) {
			LOG_ERR("Send timed out");
		}

		ASSERT_TRUE(is_tx_succeeded);
	}

	bt_mesh.seq = seq;

	for (int i = 0; i < 3; i++) {
		is_tx_succeeded = true;

		memset(test_data, i, sizeof(test_data));
		ASSERT_OK(bt_mesh_test_send_ra(rx_cfg.addr, test_data,
			sizeof(test_data), &send_cb, &sem));

		if (k_sem_take(&sem, K_SECONDS(TEST_DATA_WAITING_TIME))) {
			LOG_ERR("Send timed out");
		}

		ASSERT_TRUE(!is_tx_succeeded);
	}

	PASS();
}

static void test_rx_immediate_replay_attack(void)
{
	settings_test_backend_clear();
	bt_mesh_test_setup();
	bt_mesh_test_ra_cb_setup(rx_ended);

	k_sleep(K_SECONDS(6 * TEST_DATA_WAITING_TIME));

	ASSERT_TRUE(rx_cnt == 3, "Device didn't receive expected data");

	PASS();
}

static void test_tx_power_replay_attack(void)
{
	settings_test_backend_clear();
	bt_mesh_test_setup();

	static const struct bt_mesh_send_cb send_cb = {
		.start = tx_started,
		.end = tx_ended,
	};
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);

	for (int i = 0; i < 3; i++) {
		is_tx_succeeded = true;

		memset(test_data, i, sizeof(test_data));
		ASSERT_OK(bt_mesh_test_send_ra(rx_cfg.addr, test_data,
			sizeof(test_data), &send_cb, &sem));

		if (k_sem_take(&sem, K_SECONDS(TEST_DATA_WAITING_TIME))) {
			LOG_ERR("Send timed out");
		}

		ASSERT_TRUE(!is_tx_succeeded);
	}

	for (int i = 0; i < 3; i++) {
		is_tx_succeeded = false;

		memset(test_data, i, sizeof(test_data));
		ASSERT_OK(bt_mesh_test_send_ra(rx_cfg.addr, test_data,
			sizeof(test_data), &send_cb, &sem));

		if (k_sem_take(&sem, K_SECONDS(TEST_DATA_WAITING_TIME))) {
			LOG_ERR("Send timed out");
		}

		ASSERT_TRUE(is_tx_succeeded);
	}

	PASS();
}

static void test_rx_power_replay_attack(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_ra_cb_setup(rx_ended);

	k_sleep(K_SECONDS(6 * TEST_DATA_WAITING_TIME));

	ASSERT_TRUE(rx_cnt == 3, "Device didn't receive expected data");

	PASS();
}

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "rpc_" #role "_" #name,             \
		.test_descr = description,                     \
		.test_post_init_f = test_##role##_init,        \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_rpc[] = {
	TEST_CASE(tx, immediate_replay_attack, "RPC: perform replay attack immediately"),
	TEST_CASE(tx, power_replay_attack,     "RPC: perform replay attack after power cycle"),

	TEST_CASE(rx, immediate_replay_attack, "RPC: device under immediate attack"),
	TEST_CASE(rx, power_replay_attack,     "RPC: device under power cycle reply attack"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_rpc_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_rpc);
	return tests;
}
