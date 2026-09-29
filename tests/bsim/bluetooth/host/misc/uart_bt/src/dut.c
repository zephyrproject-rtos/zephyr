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

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "common.h"

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_tx_done);
DEFINE_FLAG_STATIC(flag_held);

static const struct device *const uart_dev = DEVICE_DT_GET(DT_NODELABEL(nus_uart));

static const uint8_t *tx_buf;
static size_t tx_len;
static size_t tx_pos;
static bool hold_after_fill;
static K_SEM_DEFINE(release_sem, 0, 1);

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);
		return;
	}

	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	UNSET_FLAG(flag_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

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
	tx_pos += n;

	/* This callback runs on the driver's work queue, so blocking here keeps
	 * tx_work from sending what was just queued.
	 */
	if (hold_after_fill) {
		hold_after_fill = false;
		SET_FLAG(flag_held);
		k_sem_take(&release_sem, K_FOREVER);
	}
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
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))
	};

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	TEST_ASSERT(err == 0, "Advertising failed to start (err %d)", err);

	WAIT_FOR_FLAG(flag_connected);
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

static void test_reconnect_main(void)
{
	dut_init();
	advertise_and_wait_for_connection();

	/* The first peer has subscribed. */
	bk_sync_wait();

	write_irq(FIRST_SESSION_MSG);
	WAIT_FOR_FLAG(flag_tx_done);

	/* The first peer has checked what it received. */
	bk_sync_wait();

	hold_after_fill = true;
	write_irq(HELD_MSG);
	WAIT_FOR_FLAG(flag_held);

	TEST_ASSERT(tx_pos == tx_len, "Driver took %zu of %zu bytes while subscribed",
		    tx_pos, tx_len);

	/* Ask the peer to disconnect. */
	bk_sync_send();
	WAIT_FOR_FLAG_UNSET(flag_connected);

	for (const char *c = DISCONNECTED_MSG; *c != '\0'; c++) {
		uart_poll_out(uart_dev, *c);
	}

	advertise_and_wait_for_connection();

	/* The second connection has subscribed. */
	bk_sync_wait();

	k_sem_give(&release_sem);
	WAIT_FOR_FLAG(flag_tx_done);

	write_irq(SECOND_SESSION_MSG);
	WAIT_FOR_FLAG(flag_tx_done);

	/* The peer has checked what it received. */
	bk_sync_wait();

	TEST_PASS("DUT passed");
}

static void fill_until_stalled(void)
{
	uint8_t buf[RESUBSCRIBED_LEN];
	int64_t start = k_uptime_get();
	int64_t last_taken = start;

	(void)memset(buf, STALLED_BYTE, sizeof(buf));

	while (k_uptime_get() - last_taken < 1000) {
		TEST_ASSERT(k_uptime_get() - start < 10000, "TX FIFO never stopped draining");

		if (uart_fifo_fill(uart_dev, buf, sizeof(buf)) > 0) {
			last_taken = k_uptime_get();
		}

		k_sleep(K_MSEC(10));
	}
}

static void test_resubscribe_main(void)
{
	uint8_t buf[RESUBSCRIBED_LEN];
	int n;

	dut_init();
	advertise_and_wait_for_connection();

	/* The peer has subscribed and stops reading after one notification. */
	bk_sync_wait();

	fill_until_stalled();
	bk_sync_send();

	/* The peer has unsubscribed and subscribed again. */
	bk_sync_wait();

	(void)memset(buf, RESUBSCRIBED_BYTE, sizeof(buf));
	n = uart_fifo_fill(uart_dev, buf, sizeof(buf));
	TEST_ASSERT(n == sizeof(buf), "Driver took %d of %zu bytes", n, sizeof(buf));

	/* A full FIFO takes nothing: the peer is subscribed again and the FIFO
	 * has not drained.
	 */
	n = uart_fifo_fill(uart_dev, buf, 1);
	TEST_ASSERT(n == 0, "Driver took %d bytes into a full FIFO", n);

	/* Let the peer read again. */
	bk_sync_send();

	/* The peer has checked what it received. */
	bk_sync_wait();

	TEST_PASS("DUT passed");
}

static const struct bst_test_instance test_dut[] = {
	{
		.test_id = "uart_bt_dut",
		.test_main_f = test_main,
	},
	{
		.test_id = "uart_bt_reconnect_dut",
		.test_main_f = test_reconnect_main,
	},
	{
		.test_id = "uart_bt_resubscribe_dut",
		.test_main_f = test_resubscribe_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_dut_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_dut);
}
