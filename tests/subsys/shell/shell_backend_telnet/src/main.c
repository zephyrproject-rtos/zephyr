/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_telnet.h>

#include <string.h>

#define IAC  0xFF
#define NOP  0xF1
#define WILL 0xFB
#define DO   0xFD
#define ECHO 0x01

/* Time to wait for the socket service thread to deliver a segment. */
#define RX_TIMEOUT_MS 2000
#define RX_POLL_MS    10

struct shell_telnet_fixture {
	const struct shell *sh;
	struct shell_telnet *ctx;
	int sock;
};

static struct shell_telnet_fixture fixture;

static void *telnet_setup(void)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(CONFIG_SHELL_TELNET_PORT),
	};
	int ret;

	fixture.sh = shell_backend_telnet_get_ptr();
	zassert_not_null(fixture.sh, "telnet backend not registered");

	fixture.ctx = (struct shell_telnet *)fixture.sh->iface->ctx;

	/* The shell thread drains rx_buf as soon as the backend reports
	 * data. Keep it suspended so the tests can inspect what the IAC
	 * filter left in the buffer, including segments that fill it.
	 */
	k_thread_suspend(fixture.sh->ctx->tid);

	ret = net_addr_pton(NET_AF_INET, "127.0.0.1", &addr.sin_addr);
	zassert_equal(ret, 0, "net_addr_pton failed");

	fixture.sock = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_true(fixture.sock >= 0, "socket failed: %d", errno);

	ret = zsock_connect(fixture.sock, (struct net_sockaddr *)&addr, sizeof(addr));
	zassert_equal(ret, 0, "connect failed: %d", errno);

	return &fixture;
}

static void telnet_after(void *data)
{
	struct shell_telnet_fixture *f = data;

	k_mutex_lock(&f->ctx->rx_lock, K_FOREVER);
	f->ctx->rx_len = 0;
	memset(f->ctx->rx_buf, 0, sizeof(f->ctx->rx_buf));
	k_mutex_unlock(&f->ctx->rx_lock);
}

static void telnet_teardown(void *data)
{
	struct shell_telnet_fixture *f = data;

	(void)zsock_close(f->sock);
	k_thread_resume(f->sh->ctx->tid);
}

static size_t rx_len_get(struct shell_telnet_fixture *f)
{
	size_t len;

	k_mutex_lock(&f->ctx->rx_lock, K_FOREVER);
	len = f->ctx->rx_len;
	k_mutex_unlock(&f->ctx->rx_lock);

	return len;
}

/* Send one segment and check the bytes the filter stored for the shell. */
static void send_and_check(struct shell_telnet_fixture *f, const uint8_t *tx, size_t tx_len,
			   const uint8_t *expected, size_t expected_len)
{
	uint8_t rx_buf[CONFIG_SHELL_CMD_BUFF_SIZE];
	size_t rx_len;
	uint8_t cmd_len;
	ssize_t sent;

	sent = zsock_send(f->sock, tx, tx_len, 0);
	zassert_equal(sent, (ssize_t)tx_len, "send returned %zd", sent);

	for (int i = 0; i < RX_TIMEOUT_MS / RX_POLL_MS; i++) {
		if (rx_len_get(f) == expected_len) {
			break;
		}
		k_msleep(RX_POLL_MS);
	}

	/* Let any trailing command bytes be processed too. */
	k_msleep(RX_POLL_MS * 5);

	k_mutex_lock(&f->ctx->rx_lock, K_FOREVER);
	rx_len = f->ctx->rx_len;
	cmd_len = f->ctx->cmd_len;
	memcpy(rx_buf, f->ctx->rx_buf, sizeof(rx_buf));
	k_mutex_unlock(&f->ctx->rx_lock);

	zassert_equal(rx_len, expected_len, "rx_len %zu, expected %zu", rx_len, expected_len);
	zassert_equal(cmd_len, 0, "command state not reset (%u)", cmd_len);
	zassert_mem_equal(rx_buf, expected, expected_len, "filtered data mismatch");
}

ZTEST_F(shell_telnet, test_data_only)
{
	static const uint8_t tx[] = "hello";

	send_and_check(fixture, tx, sizeof(tx) - 1, tx, sizeof(tx) - 1);
}

ZTEST_F(shell_telnet, test_iac_between_data)
{
	static const uint8_t tx[] = {'a', 'b', 'c', IAC, WILL, ECHO, 'd',
				     'e', 'f', IAC, NOP, 'g',  'h'};
	static const uint8_t expected[] = "abcdefgh";

	send_and_check(fixture, tx, sizeof(tx), expected, sizeof(expected) - 1);
}

ZTEST_F(shell_telnet, test_iac_at_start)
{
	static const uint8_t tx[] = {IAC, DO, ECHO, 'a', 'b', 'c'};
	static const uint8_t expected[] = "abc";

	send_and_check(fixture, tx, sizeof(tx), expected, sizeof(expected) - 1);
}

ZTEST_F(shell_telnet, test_iac_at_end)
{
	static const uint8_t tx[] = {'a', 'b', 'c', IAC, DO, ECHO};
	static const uint8_t expected[] = "abc";

	send_and_check(fixture, tx, sizeof(tx), expected, sizeof(expected) - 1);
}

/* A segment that fills rx_buf, with a command before its last data byte.
 * Removing the command must only move the bytes that follow it; moving
 * the whole logical length instead runs past rx_buf into the fields
 * after it.
 */
ZTEST_F(shell_telnet, test_iac_near_end_of_full_buffer)
{
	uint8_t tx[CONFIG_SHELL_CMD_BUFF_SIZE];
	uint8_t expected[CONFIG_SHELL_CMD_BUFF_SIZE - 3];
	const size_t prefix = sizeof(tx) - 4;

	memset(tx, 'A', prefix);
	tx[prefix] = IAC;
	tx[prefix + 1] = DO;
	tx[prefix + 2] = ECHO;
	tx[prefix + 3] = 'D';

	memset(expected, 'A', prefix);
	expected[prefix] = 'D';

	send_and_check(fixture, tx, sizeof(tx), expected, sizeof(expected));
}

ZTEST_SUITE(shell_telnet, NULL, telnet_setup, NULL, telnet_after, telnet_teardown);
