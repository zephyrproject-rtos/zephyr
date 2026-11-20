/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The H:4 driver closed while its RX thread waits for a buffer. The test feeds the driver through
 * an emulated UART and uses the driver API without the Host.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/slist.h>
#include <zephyr/ztest.h>

/* Well above the time the UART emulator's work queue, which runs the driver's interrupt handler,
 * and the RX thread take to handle what the test puts into the UART: neither waits for time.
 */
#define SETTLE_TIME K_MSEC(100)

static const struct device *const hci_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci));
static const struct device *const uart_dev = DEVICE_DT_GET(DT_PARENT(DT_CHOSEN(zephyr_bt_hci)));

/* An event that the driver takes a buffer for with bt_buf_get_rx(BT_BUF_EVT, ...), waiting. */
static const uint8_t hw_error_evt[] = {BT_HCI_H4_EVT, BT_HCI_EVT_HARDWARE_ERROR, 1U, 0U};

static K_SEM_DEFINE(recv_sem, 0, 1);
static size_t recv_total;

/* Count the packets, and signal the ones that arrive intact. */
static int count_recv(const struct device *dev, struct net_buf *buf)
{
	ARG_UNUSED(dev);

	recv_total++;

	if ((buf->len == sizeof(hw_error_evt)) &&
	    (memcmp(buf->data, hw_error_evt, sizeof(hw_error_evt)) == 0)) {
		k_sem_give(&recv_sem);
	}

	net_buf_unref(buf);

	return 0;
}

/* Allocate every free RX event buffer and return how many there were. */
static size_t take_rx_bufs(sys_slist_t *bufs)
{
	struct net_buf *buf = bt_buf_get_rx(BT_BUF_EVT, K_NO_WAIT);
	size_t count = 0U;

	sys_slist_init(bufs);

	while (buf != NULL) {
		net_buf_slist_put(bufs, buf);
		count++;
		buf = bt_buf_get_rx(BT_BUF_EVT, K_NO_WAIT);
	}

	return count;
}

static void send_to_driver(const uint8_t *data, size_t len)
{
	zassert_equal(uart_emul_put_rx_data(uart_dev, data, len), len,
		      "The UART did not take the packet");
}

/* Have the RX thread wait for a buffer for an event, with all of them held in bufs. */
static void open_and_wait_for_buffer(sys_slist_t *bufs)
{
	zassert_ok(bt_hci_open(hci_dev, count_recv), "Opening failed");
	zassert_true(take_rx_bufs(bufs) > 0U, "No RX buffers");

	send_to_driver(hw_error_evt, sizeof(hw_error_evt));

	/* Nothing tells when the RX thread has started to wait */
	(void)k_sleep(SETTLE_TIME);
}

/* A buffer that the waiting RX thread has just been handed is not lost by bt_hci_close(). */
static ZTEST(hci_h4, test_close_with_buffer_handed_to_rx)
{
	sys_slist_t bufs;
	size_t taken;
	size_t count;

	open_and_wait_for_buffer(&bufs);
	count = sys_slist_len(&bufs);

	/* Being cooperative keeps the RX thread from running before the close. */
	zassert_false(k_is_preempt_thread(), "The test thread is preemptible");
	net_buf_unref(net_buf_slist_get(&bufs));
	zassert_ok(bt_hci_close(hci_dev), "Closing failed");
	zassert_equal(recv_total, 0U, "A packet was delivered");

	net_buf_slist_purge(&bufs);
	taken = take_rx_bufs(&bufs);
	net_buf_slist_purge(&bufs);
	zassert_equal(taken, count, "An RX buffer was lost");
}

/* bt_hci_close() does not depend on a buffer being freed for the waiting RX thread, and the
 * transport works when opened again.
 */
static ZTEST(hci_h4, test_close_with_rx_waiting_for_buffer)
{
	sys_slist_t bufs;

	open_and_wait_for_buffer(&bufs);

	zassert_ok(bt_hci_close(hci_dev), "Closing failed");
	zassert_equal(recv_total, 0U, "A packet was delivered");
	net_buf_slist_purge(&bufs);

	/* The rest of the event, which the driver had not read */
	(void)uart_emul_flush_rx_data(uart_dev);

	zassert_ok(bt_hci_open(hci_dev, count_recv), "Opening again failed");
	send_to_driver(hw_error_evt, sizeof(hw_error_evt));
	zassert_ok(k_sem_take(&recv_sem, SETTLE_TIME), "No event after opening again");
	zassert_ok(bt_hci_close(hci_dev), "Closing again failed");
}

static void hci_h4_after(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)uart_emul_flush_rx_data(uart_dev);
	k_sem_reset(&recv_sem);
	recv_total = 0U;
}

ZTEST_SUITE(hci_h4, NULL, NULL, NULL, hci_h4_after, NULL);
