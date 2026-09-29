/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include "testlib/adv.h"
#include "testlib/conn.h"

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "common.h"

DEFINE_FLAG_STATIC(flag_tx_done);

static const struct device *const uart_dev = DEVICE_DT_GET(DT_NODELABEL(nus_uart));

static const uint8_t *tx_buf;
static size_t tx_len;
static size_t tx_pos;

static void uart_cb(const struct device *dev, void *user_data)
{
	int n;

	if (uart_irq_tx_ready(dev) == 0) {
		return;
	}

	if (tx_pos == tx_len) {
		uart_irq_tx_disable(dev);
		SET_FLAG(flag_tx_done);
		return;
	}

	n = uart_fifo_fill(dev, &tx_buf[tx_pos], tx_len - tx_pos);
	if (n < 0) {
		TEST_FAIL("Failed to fill the TX FIFO (err %d)", n);
		return;
	}

	tx_pos += (size_t)n;
}

static void write_irq(const char *msg)
{
	tx_buf = (const uint8_t *)msg;
	tx_len = strlen(msg);
	tx_pos = 0;
	UNSET_FLAG(flag_tx_done);

	uart_irq_tx_enable(uart_dev);
}

static void dut_init(void)
{
	int err;

	TEST_ASSERT(device_is_ready(uart_dev), "UART over NUS device not ready");
	TEST_ASSERT(bk_sync_init() == 0, "Failed to open backchannel");

	err = uart_irq_callback_user_data_set(uart_dev, uart_cb, NULL);
	TEST_ASSERT(err == 0, "Failed to set UART callback (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);
}

static void advertise_and_wait_for_connection(void)
{
	struct bt_conn *conn = NULL;
	int err;

	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, bt_get_name());
	TEST_ASSERT(err == 0, "Failed to advertise and connect (err %d)", err);

	bt_testlib_conn_unref(&conn);
}

static void test_main(void)
{
	dut_init();

	write_irq(UNSUBSCRIBED_MSG);
	WAIT_FOR_FLAG(flag_tx_done);

	advertise_and_wait_for_connection();

	/* The peer has subscribed to the NUS TX characteristic. */
	bk_sync_wait();

	write_irq(SUBSCRIBED_MSG);
	WAIT_FOR_FLAG(flag_tx_done);

	/* The peer has checked what it received. */
	bk_sync_wait();

	TEST_PASS("DUT passed");
}

static const struct bst_test_instance test_dut[] = {
	{
		.test_id = "uart_bt_dut",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_dut_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_dut);
}
