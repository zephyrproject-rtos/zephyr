/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/gatt.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bsim_mtu_update, LOG_LEVEL_DBG);

extern void run_central_sample(bt_gatt_notify_func_t notif_cb);
extern void run_peripheral_sample(uint8_t *notify_data, size_t notify_data_size, uint16_t seconds);

#define FAIL(...)					\
	do {						\
		bst_result = Failed;			\
		bs_trace_error_time_line(__VA_ARGS__);	\
	} while (0)

#define PASS(...)					\
	do {						\
		bst_result = Passed;			\
		bs_trace_info_time(1, __VA_ARGS__);	\
	} while (0)

extern enum bst_result_t bst_result;

#define WAIT_TIME (20e6) /* 20 seconds */
#define PERIPHERAL_WAIT_TIME ((WAIT_TIME - 10e6) / 1e6)

static volatile size_t central_mtu_count;
static uint16_t central_mtu_tx;
static uint16_t central_mtu_rx;

static void central_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("[CENTRAL] Updated MTU: Count: %u TX: %u RX: %u bytes\n",
		central_mtu_count, tx, rx);

	central_mtu_rx = rx;
	central_mtu_tx = tx;
	central_mtu_count++;
}

static struct bt_gatt_cb central_gatt_callbacks = {.att_mtu_updated = central_mtu_updated};

static void test_central_main(void)
{
	uint16_t expected_mtu = CONFIG_BT_L2CAP_TX_MTU;

	bt_gatt_cb_register(&central_gatt_callbacks);

	run_central_sample(NULL);

	/** Wait for MTU exchange to occur */
	while (central_mtu_count == 0) {
		k_sleep(K_MSEC(1));
	}

	if (central_mtu_count == 1 &&
		central_mtu_tx == expected_mtu &&
		central_mtu_rx == expected_mtu) {
		PASS("MTU Update test passed [Central]\n");
	} else {
		FAIL("MTU Update test failed [Central] - Count: %u, MTU RX: %u, MTU TX: %u\n",
			central_mtu_count, central_mtu_rx, central_mtu_tx);
	}
}

static volatile size_t peripheral_mtu_count;
static uint16_t peripheral_mtu_tx;
static uint16_t peripheral_mtu_rx;

static void peripheral_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("[PERIPHERAL] Updated MTU: Count: %u TX: %u RX: %u bytes\n",
		peripheral_mtu_count, tx, rx);

	peripheral_mtu_rx = rx;
	peripheral_mtu_tx = tx;
	peripheral_mtu_count++;
}

static struct bt_gatt_cb peripheral_gatt_callbacks = {.att_mtu_updated = peripheral_mtu_updated};

static void test_peripheral_main(void)
{
	uint16_t expected_mtu = CONFIG_BT_L2CAP_TX_MTU;

	bt_gatt_cb_register(&peripheral_gatt_callbacks);

	run_peripheral_sample(NULL, 0, PERIPHERAL_WAIT_TIME);

	/** Wait for MTU exchange to occur */
	while (peripheral_mtu_count == 0) {
		k_sleep(K_MSEC(1));
	}

	if (peripheral_mtu_count == 1 &&
		peripheral_mtu_tx == expected_mtu &&
		peripheral_mtu_rx == expected_mtu) {
		PASS("MTU Update test passed [Peripheral]\n");
	} else {
		FAIL("MTU Update test failed [Peripheral] - Count: %u, MTU RX: %u, MTU TX: %u\n",
			peripheral_mtu_count, peripheral_mtu_rx, peripheral_mtu_tx);
	}
}

void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("Test failed (not passed after %i seconds)\n", WAIT_TIME);
	}
}

static void test_mtu_update_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
	bst_result = In_progress;
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central GATT MTU Update",
		.test_post_init_f = test_mtu_update_init,
		.test_tick_f = test_tick,
		.test_main_f = test_central_main
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral GATT MTU Update",
		.test_post_init_f = test_mtu_update_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_mtu_update_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_mtu_update_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
