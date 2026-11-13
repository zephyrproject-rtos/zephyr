/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The HCI user channel driver against a peer that goes away, and when it is closed at awkward
 * moments. The test thread is the peer: it holds the other end of the socket. It never answers
 * the Host, so every bt_enable() ends in the HCI command timeout, after which it can be called
 * again. The cases that need packets delivered use the driver API without the Host.
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "hci_core.h"
#include "nsi_host_trampolines.h"
#include "peer_bottom.h"

/* The same as --bt-dev in prj.conf. */
#define HCI_SOCKET_PATH "/proc/self/cwd/hci.sock"

/* Well above the time it takes for HCI Reset to go out and the RX thread to poll the socket. */
#define SETTLE_TIME_MS 100U
#define SETTLE_TIME    K_MSEC(SETTLE_TIME_MS)

/* The driver's RX thread waits for an event buffer this long at a time. */
#define RX_BUF_WAIT_MS 1000U

/* Events that arrive while the RX thread waits for a buffer for the first of them. */
#define WAITING_EVT_COUNT 4U

static const struct device *const hci_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci));

/* An event that the driver takes a buffer for with bt_buf_get_rx(BT_BUF_EVT, ...), waiting. */
static const uint8_t hw_error_evt[] = {BT_HCI_H4_EVT, BT_HCI_EVT_HARDWARE_ERROR, 1U, 0U};

/* An ACL packet, which the driver takes a buffer for without waiting. */
static const uint8_t acl_pkt[] = {BT_HCI_H4_ACL, 0U, 0U, 1U, 0U, 0U};

static K_SEM_DEFINE(ready_sem, 0, 1);
static int ready_err;

static size_t recv_count;
static size_t recv_total;
static bool slow_recv_entered;
static bool slow_recv_done;

static void ready_cb(int err)
{
	ready_err = err;
	k_sem_give(&ready_sem);
}

/* Count the packets that arrive intact. */
static int count_recv(const struct device *dev, struct net_buf *buf)
{
	ARG_UNUSED(dev);

	recv_total++;

	if ((buf->len == sizeof(hw_error_evt)) &&
	    (memcmp(buf->data, hw_error_evt, sizeof(hw_error_evt)) == 0)) {
		recv_count++;
	}

	net_buf_unref(buf);

	return 0;
}

static int slow_recv(const struct device *dev, struct net_buf *buf)
{
	ARG_UNUSED(dev);

	slow_recv_entered = true;
	/* Well above SETTLE_TIME, so that the test thread closes the driver during this. */
	(void)k_sleep(K_MSEC(5U * SETTLE_TIME_MS));
	slow_recv_done = true;

	net_buf_unref(buf);

	return 0;
}

static int start_listening(void)
{
	int listen_fd = peer_listen(HCI_SOCKET_PATH);

	zassert_true(listen_fd >= 0, "Listening on the socket failed");

	return listen_fd;
}

static int accept_driver(int listen_fd)
{
	int conn_fd = peer_accept(listen_fd);

	zassert_true(conn_fd >= 0, "The driver did not connect");
	peer_stop_listening(listen_fd, HCI_SOCKET_PATH);

	return conn_fd;
}

/* Start bt_enable() and return the peer's end of the connection, with HCI Reset sent to it. */
static int enable_and_accept(void)
{
	int listen_fd = start_listening();
	int conn_fd;

	zassert_ok(bt_enable(ready_cb), "bt_enable() failed");
	conn_fd = accept_driver(listen_fd);

	(void)k_sleep(SETTLE_TIME);

	return conn_fd;
}

/* Open the driver without the Host and return the peer's end of the connection. */
static int open_and_accept(bt_hci_recv_t recv)
{
	int listen_fd = start_listening();

	zassert_ok(bt_hci_open(hci_dev, recv), "bt_hci_open() failed");

	return accept_driver(listen_fd);
}

static void read_cmd(int conn_fd)
{
	uint8_t cmd[sizeof(uint8_t) + BT_HCI_CMD_HDR_SIZE];

	zassert_equal(nsi_host_read(conn_fd, cmd, sizeof(cmd)), sizeof(cmd),
		      "HCI Reset did not arrive");
}

static void send_to_driver(int conn_fd, const uint8_t *data, size_t len)
{
	zassert_equal(nsi_host_write(conn_fd, data, len), len, "Sending to the driver failed");
}

/* Once the RX thread has seen the loss, the driver has no socket left to send to. */
static void check_connection_lost(void)
{
	struct net_buf *buf;

	(void)k_sleep(SETTLE_TIME);

	buf = bt_buf_get_rx(BT_BUF_EVT, K_NO_WAIT);
	zassert_not_null(buf, "No buffer to send");

	zassert_equal(bt_hci_send(hci_dev, buf), -EIO, "The driver did not notice the loss");
	net_buf_unref(buf);
}

static void check_enable_timed_out(void)
{
	zassert_ok(k_sem_take(&ready_sem, K_TIMEOUT_SUM(HCI_CMD_TIMEOUT, SETTLE_TIME)),
		   "The ready callback was not called");
	zassert_equal(ready_err, -ETIMEDOUT, "The ready callback gave %d", ready_err);
	zassert_false(bt_is_ready(), "Bluetooth is ready without a controller");
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

/* The driver reads end-of-file. */
static ZTEST(hci_userchan, test_peer_closes)
{
	int conn_fd = enable_and_accept();

	read_cmd(conn_fd);
	(void)nsi_host_close(conn_fd);

	check_connection_lost();
	check_enable_timed_out();
}

/* As above, with part of a packet left in the driver's buffer. */
static ZTEST(hci_userchan, test_peer_closes_mid_packet)
{
	int conn_fd = enable_and_accept();

	read_cmd(conn_fd);
	send_to_driver(conn_fd, hw_error_evt, sizeof(hw_error_evt) - 1U);
	(void)nsi_host_close(conn_fd);

	check_connection_lost();
	check_enable_timed_out();
}

/* Closing with HCI Reset unread makes the driver's read fail with ECONNRESET. */
static ZTEST(hci_userchan, test_peer_resets)
{
	int conn_fd = enable_and_accept();

	(void)nsi_host_close(conn_fd);

	check_connection_lost();
	check_enable_timed_out();
}

/* The peer stays, so the driver is closed with its RX thread running, and opened again. */
static ZTEST(hci_userchan, test_reopen_after_close)
{
	int conn_fd[2];

	ARRAY_FOR_EACH(conn_fd, i) {
		conn_fd[i] = enable_and_accept();
		check_enable_timed_out();
	}

	ARRAY_FOR_EACH(conn_fd, i) {
		(void)nsi_host_close(conn_fd[i]);
	}
}

/* A send before the RX thread has noticed that the peer is gone fails, and raises no SIGPIPE. */
static ZTEST(hci_userchan, test_send_after_peer_closes)
{
	int conn_fd = enable_and_accept();
	struct net_buf *buf;
	int err;

	read_cmd(conn_fd);

	buf = bt_buf_get_rx(BT_BUF_EVT, K_NO_WAIT);
	zassert_not_null(buf, "No buffer to send");

	(void)nsi_host_close(conn_fd);

	err = bt_hci_send(hci_dev, buf);
	zassert_equal(err, -EPIPE, "bt_hci_send() gave %d (!= %d)", err, -EPIPE);
	net_buf_unref(buf);

	check_enable_timed_out();
}

/* What a lost connection leaves in the driver's buffer does not reach the next connection. */
static ZTEST(hci_userchan, test_packet_after_partial_packet)
{
	int conn_fd = open_and_accept(count_recv);

	send_to_driver(conn_fd, hw_error_evt, sizeof(hw_error_evt) - 1U);
	(void)nsi_host_close(conn_fd);

	check_connection_lost();
	zassert_ok(bt_hci_close(hci_dev), "Closing after the loss failed");

	conn_fd = open_and_accept(count_recv);

	send_to_driver(conn_fd, hw_error_evt, sizeof(hw_error_evt));
	(void)k_sleep(SETTLE_TIME);
	zassert_equal(recv_count, 1U, "%zu packets arrived intact", recv_count);

	zassert_ok(bt_hci_close(hci_dev), "Closing failed");
	(void)nsi_host_close(conn_fd);
}

/* A recv callback in progress is neither abandoned nor outlives bt_hci_close(). */
static ZTEST(hci_userchan, test_close_during_recv)
{
	int conn_fd = open_and_accept(slow_recv);

	send_to_driver(conn_fd, hw_error_evt, sizeof(hw_error_evt));
	(void)k_sleep(SETTLE_TIME);
	zassert_true(slow_recv_entered, "The packet did not reach the callback");
	zassert_false(slow_recv_done, "The callback is not in progress");

	zassert_ok(bt_hci_close(hci_dev), "Closing failed");
	zassert_true(slow_recv_done, "The callback did not finish");

	(void)nsi_host_close(conn_fd);
}

/* A buffer that the waiting RX thread has just been handed is not lost by bt_hci_close(), and the
 * packets behind the one it delivers are not decoded any more.
 */
static ZTEST(hci_userchan, test_close_with_buffer_handed_to_rx)
{
	sys_slist_t bufs;
	int conn_fd = open_and_accept(count_recv);
	size_t count = take_rx_bufs(&bufs);

	zassert_true(count > 0U, "No RX buffers");

	send_to_driver(conn_fd, hw_error_evt, sizeof(hw_error_evt));
	send_to_driver(conn_fd, acl_pkt, sizeof(acl_pkt));
	(void)k_sleep(SETTLE_TIME);

	/* Being cooperative keeps the RX thread from running before the close. */
	zassert_false(k_is_preempt_thread(), "The test thread is preemptible");
	net_buf_unref(net_buf_slist_get(&bufs));
	zassert_ok(bt_hci_close(hci_dev), "Closing failed");

	zassert_equal(recv_count, 1U, "%zu packets arrived intact", recv_count);
	zassert_equal(recv_total, 1U, "A packet was decoded after the close");

	net_buf_slist_purge(&bufs);
	zassert_equal(take_rx_bufs(&bufs), count, "An RX buffer was lost");
	net_buf_slist_purge(&bufs);

	(void)nsi_host_close(conn_fd);
}

/* bt_hci_close() does not depend on a buffer being freed for the waiting RX thread, nor wait for
 * one for each of the events it has received.
 */
static ZTEST(hci_userchan, test_close_with_rx_waiting_for_buffer)
{
	sys_slist_t bufs;
	int conn_fd = open_and_accept(count_recv);
	int64_t start;

	zassert_true(take_rx_bufs(&bufs) > 0U, "No RX buffers");

	for (size_t i = 0U; i < WAITING_EVT_COUNT; i++) {
		send_to_driver(conn_fd, hw_error_evt, sizeof(hw_error_evt));
	}
	(void)k_sleep(SETTLE_TIME);

	start = k_uptime_get();
	zassert_ok(bt_hci_close(hci_dev), "Closing failed");
	zassert_true(k_uptime_delta(&start) < (2U * RX_BUF_WAIT_MS),
		     "Closing waited for a buffer for each event");
	zassert_equal(recv_count, 0U, "%zu packets arrived intact", recv_count);

	net_buf_slist_purge(&bufs);
	(void)nsi_host_close(conn_fd);
}

static void hci_userchan_after(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&ready_sem);
	recv_count = 0U;
	recv_total = 0U;
	slow_recv_entered = false;
	slow_recv_done = false;
}

ZTEST_SUITE(hci_userchan, NULL, NULL, NULL, hci_userchan_after, NULL);
