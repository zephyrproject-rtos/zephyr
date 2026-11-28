/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/server.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_websocket.h>

/* Expose a websocket shell console on the default HTTP port. */
WEBSOCKET_CONSOLE_DEFINE(ws_console, NULL, 0);

#define WS_PORT             CONFIG_SHELL_WEBSOCKET_PORT
#define WS_HTTP_TIMEOUT_MS  3000
#define WS_RX_CHUNK         256
#define WS_PRINTER_LINES    2000

static struct shell_websocket *const ws = &ws_ctx_ws_console;
static const struct shell *const sh = &shell_websocket_ws_console;

static K_THREAD_STACK_DEFINE(printer_stack, 4096);
static struct k_thread printer_thread_data;
static volatile bool printer_stop;
static K_SEM_DEFINE(printer_started, 0, 1);

/* Wait until the shell backend is closed on the server side. */
static void ws_wait_closed(int timeout_ms)
{
	int64_t deadline = k_uptime_get() + timeout_ms;

	while (ws->fds[0].fd >= 0 && k_uptime_get() < deadline) {
		k_msleep(20);
	}
}

#define WS_CONNECT_TIMEOUT_MS 2000

static int ws_connect(void)
{
	static const char request[] =
		"GET /" CONFIG_SHELL_WEBSOCKET_ENDPOINT_URL " HTTP/1.1\r\n"
		"Host: 127.0.0.1\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"\r\n";
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(WS_PORT),
	};
	char rsp[WS_RX_CHUNK];
	size_t len = 0;
	int rcvbuf = 512;
	int sock;
	int ret;

	int64_t connect_deadline = k_uptime_get() + WS_CONNECT_TIMEOUT_MS;
	int attempt = 0;

	sock = -1;
	do {
		int conn_err;

		attempt++;
		sock = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
		zassert_true(sock >= 0, "socket() failed (%d)", errno);

		(void)zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVBUF,
				       &rcvbuf, sizeof(rcvbuf));

		ret = net_addr_pton(NET_AF_INET, "127.0.0.1", &addr.sin_addr);
		zassert_equal(ret, 0, "inet_pton failed (%d)", ret);

		ret = zsock_connect(sock, (struct net_sockaddr *)&addr, sizeof(addr));
		if (ret == 0) {
			break;
		}

		conn_err = errno;
		(void)zsock_close(sock);
		sock = -1;

		/* http_server_start() returns before the listener is ready, and the
		 * server serves one client at a time, so retry until accepted.
		 */
		if (conn_err != ECONNREFUSED && conn_err != ETIMEDOUT) {
			break;
		}

		k_msleep(10);
	} while (k_uptime_get() < connect_deadline);

	zassert_equal(ret, 0, "connect failed (%d)", errno);

	ret = zsock_send(sock, request, sizeof(request) - 1, 0);
	zassert_equal(ret, (int)sizeof(request) - 1, "send failed (%d)", errno);

	while (len < sizeof(rsp) - 1) {
		struct zsock_pollfd pfd = { .fd = sock, .events = ZSOCK_POLLIN };
		ssize_t received;

		ret = zsock_poll(&pfd, 1, WS_HTTP_TIMEOUT_MS);
		zassert_true(ret > 0, "handshake timed out");

		received = zsock_recv(sock, rsp + len, sizeof(rsp) - 1 - len, 0);
		zassert_true(received > 0, "handshake recv failed (%d)", errno);
		len += received;
		rsp[len] = '\0';

		if (strstr(rsp, "\r\n\r\n") != NULL) {
			break;
		}
	}

	zassert_not_null(strstr(rsp, " 101 "), "no 101 response: %s", rsp);
	zassert_not_null(strstr(rsp, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="),
			 "wrong Sec-WebSocket-Accept: %s", rsp);

	return sock;
}

/* Read from the socket until @p needle shows up in the received data. */
static bool ws_read_until(int sock, const char *needle, int timeout_ms)
{
	char buf[WS_RX_CHUNK * 2];
	size_t len = 0;
	int64_t deadline = k_uptime_get() + timeout_ms;

	while (k_uptime_get() < deadline) {
		struct zsock_pollfd pfd = { .fd = sock, .events = ZSOCK_POLLIN };
		ssize_t received;
		int ret;

		ret = zsock_poll(&pfd, 1, (int)(deadline - k_uptime_get()));
		if (ret <= 0) {
			continue;
		}

		received = zsock_recv(sock, buf + len, sizeof(buf) - len - 1, 0);
		if (received <= 0) {
			continue;
		}

		len += received;
		buf[len] = '\0';

		if (strstr(buf, needle) != NULL) {
			return true;
		}

		if (len > sizeof(buf) - WS_RX_CHUNK) {
			memmove(buf, buf + len - WS_RX_CHUNK, WS_RX_CHUNK);
			len = WS_RX_CHUNK;
		}
	}

	return false;
}

/*
 * Generate a lot of output from a separate thread so the test thread can
 * probe the socket lock while the shell backend is trying to send.
 */
static void ws_printer(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_give(&printer_started);

	for (int i = 0; i < WS_PRINTER_LINES && !printer_stop; i++) {
		shell_print(sh, "%04d aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa "
			    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n", i);
	}
}

static void *ws_setup(void)
{
	WEBSOCKET_CONSOLE_ENABLE(ws_console);
	zassert_equal(shell_start(sh), 0, "shell_start() failed");
	zassert_equal(http_server_start(), 0, "http_server_start() failed");
	return NULL;
}

ZTEST(shell_websocket, test_output)
{
	int sock = ws_connect();

	shell_print(sh, "hello-over-websocket\n");
	zassert_true(ws_read_until(sock, "hello-over-websocket", WS_HTTP_TIMEOUT_MS),
		     "shell output was not delivered over the websocket");

	(void)zsock_close(sock);
	ws_wait_closed(WS_HTTP_TIMEOUT_MS);
}

ZTEST(shell_websocket, test_send_does_not_hold_socket_lock)
{
	int64_t deadline;
	int64_t start;
	int sock;

	sock = ws_connect();

	/* Let the server side finish setting up the transport for this socket. */
	deadline = k_uptime_get() + WS_HTTP_TIMEOUT_MS;
	while (ws->fds[0].fd < 0 && k_uptime_get() < deadline) {
		k_msleep(5);
	}
	k_msleep(50);

	/* The client never reads, so the server send window fills up. */
	printer_stop = false;
	(void)k_thread_create(&printer_thread_data, printer_stack,
			      K_THREAD_STACK_SIZEOF(printer_stack), ws_printer,
			      NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	start = k_uptime_get();
	deadline = start + 2000;

	/* Sample the lock instead of waiting on it, so a long hold shows up as a
	 * long run of consecutive failed K_NO_WAIT acquisitions.
	 */
	int64_t hold_start = 0;
	int64_t max_hold = 0;
	bool held = false;

	while (k_uptime_get() < deadline) {
		if (k_mutex_lock(&ws->socket_lock, K_NO_WAIT) == 0) {
			if (held) {
				int64_t dur = k_uptime_get() - hold_start;

				if (dur > max_hold) {
					max_hold = dur;
				}
				held = false;
			}
			k_mutex_unlock(&ws->socket_lock);
		} else if (!held) {
			held = true;
			hold_start = k_uptime_get();
		}
		k_msleep(1);
	}

	if (held) {
		int64_t dur = k_uptime_get() - hold_start;

		if (dur > max_hold) {
			max_hold = dur;
		}
	}

	zassert_true(max_hold < 50,
		     "socket_lock was held for %lld ms while a send was pending",
		     (long long)max_hold);

	(void)zsock_close(sock);
	ws_wait_closed(WS_HTTP_TIMEOUT_MS);
}

ZTEST(shell_websocket, test_reconnect)
{
	int sock = ws_connect();

	shell_print(sh, "before-disconnect\n");
	zassert_true(ws_read_until(sock, "before-disconnect", WS_HTTP_TIMEOUT_MS));

	(void)zsock_close(sock);
	ws_wait_closed(WS_HTTP_TIMEOUT_MS);
	zassert_true(ws->fds[0].fd < 0, "backend did not close the connection");

	sock = ws_connect();
	shell_print(sh, "after-reconnect\n");
	zassert_true(ws_read_until(sock, "after-reconnect", WS_HTTP_TIMEOUT_MS));

	(void)zsock_close(sock);
	ws_wait_closed(WS_HTTP_TIMEOUT_MS);
}


/* Check whether an output writer is currently inside ws_send(). */
static bool ws_send_in_progress(void)
{
	bool busy;

	if (k_mutex_lock(&ws->tx_lock, K_NO_WAIT) == 0) {
		k_mutex_unlock(&ws->tx_lock);
		busy = false;
	} else {
		busy = true;
	}

	return busy;
}

ZTEST(shell_websocket, test_delayed_flush)
{
	int sock = ws_connect();

	/* No line feed: the delayed send work must flush the line. */
	shell_fprintf(sh, SHELL_NORMAL, "delayed-flush");
	zassert_true(ws_read_until(sock, "delayed-flush", WS_HTTP_TIMEOUT_MS),
		     "line without LF was not flushed by the delayed work");

	(void)zsock_close(sock);
	ws_wait_closed(WS_HTTP_TIMEOUT_MS);
}

ZTEST(shell_websocket, test_slow_reader)
{
	int sock = ws_connect();
	char buf[64];
	size_t total = 0;
	int64_t deadline = k_uptime_get() + 4000;

	for (int i = 0; i < 5; i++) {
		shell_print(sh, "slow-%02d ------------------------------------\n", i);
	}

	/* Read slower than the writer produces data. */
	while (k_uptime_get() < deadline && total < 128) {
		struct zsock_pollfd pfd = { .fd = sock, .events = ZSOCK_POLLIN };
		ssize_t received;

		if (zsock_poll(&pfd, 1, 20) > 0) {
			received = zsock_recv(sock, buf, sizeof(buf), 0);
			if (received > 0) {
				total += received;
			}
		}
		k_msleep(10);
	}

	zassert_true(total > 0, "no data was received by the slow reader");
	zassert_true(ws->fds[0].fd >= 0,
		     "a slow reader caused the connection to be closed");

	(void)zsock_close(sock);
	ws_wait_closed(WS_HTTP_TIMEOUT_MS);
}

ZTEST(shell_websocket, test_disconnect_while_send_pending)
{
	int64_t deadline;
	int sock;

	sock = ws_connect();

	/* Let the server side finish setting up the transport for this socket. */
	deadline = k_uptime_get() + WS_HTTP_TIMEOUT_MS;
	while (ws->fds[0].fd < 0 && k_uptime_get() < deadline) {
		k_msleep(5);
	}
	k_msleep(50);

	k_sem_reset(&printer_started);
	printer_stop = false;
	(void)k_thread_create(&printer_thread_data, printer_stack,
			      K_THREAD_STACK_SIZEOF(printer_stack), ws_printer,
			      NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	zassert_equal(k_sem_take(&printer_started, K_MSEC(2000)), 0,
		      "printer thread did not start");

	/* Wait until a writer is really inside ws_send(). */
	deadline = k_uptime_get() + 5000;
	while (!ws_send_in_progress() && k_uptime_get() < deadline) {
		k_msleep(1);
	}
	zassert_true(ws_send_in_progress(), "no writer entered send() within 5 s");

	/* Disconnect while the send is pending: the cleanup must complete. */
	(void)zsock_close(sock);

	deadline = k_uptime_get() + CONFIG_SHELL_WEBSOCKET_TX_TIMEOUT + 3000;
	while (ws->fds[0].fd >= 0 && k_uptime_get() < deadline) {
		k_msleep(5);
	}
	printer_stop = true;

	zassert_true(ws->fds[0].fd < 0,
		     "connection cleanup did not complete within the agreed time");

	/* A new connection must work and must not inherit the old output. */
	sock = ws_connect();
	shell_print(sh, "after-pending-disconnect\n");
	zassert_true(ws_read_until(sock, "after-pending-disconnect", WS_HTTP_TIMEOUT_MS),
		     "no output after reconnecting");

	(void)zsock_close(sock);
	ws_wait_closed(WS_HTTP_TIMEOUT_MS);
}

ZTEST_SUITE(shell_websocket, NULL, ws_setup, NULL, NULL, NULL);
