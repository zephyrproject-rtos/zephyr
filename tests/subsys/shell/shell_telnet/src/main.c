/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#define PROMPT CONFIG_SHELL_PROMPT_TELNET

static K_SEM_DEFINE(probe_sem, 0, 1);

static void probe_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&probe_sem);
}

static K_WORK_DEFINE(probe_work, probe_handler);

static int connect_client(void)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(CONFIG_SHELL_TELNET_PORT),
		.sin_addr = NET_INADDR_LOOPBACK_INIT,
	};
	struct zsock_timeval tv = { .tv_usec = 100 * USEC_PER_MSEC };
	int sock;

	sock = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_true(sock >= 0, "socket() failed (%d)", errno);
	zassert_ok(zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &tv, sizeof(tv)));
	zassert_ok(zsock_connect(sock, (struct net_sockaddr *)&addr, sizeof(addr)),
		   "connect() failed (%d)", errno);

	return sock;
}

/* Receive until the output so far contains `what`, the time is out or the
 * connection ends.
 */
static bool recv_until(int sock, const char *what, k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	static char buf[256];
	size_t len = 0;

	while (!sys_timepoint_expired(end)) {
		ssize_t ret = zsock_recv(sock, buf + len, sizeof(buf) - 1 - len, 0);

		if (ret == 0 || (ret < 0 && errno != EAGAIN)) {
			break;
		}
		if (ret > 0) {
			len += ret;
			buf[len] = '\0';
			if (strstr(buf, what) != NULL) {
				return true;
			}
			if (len == sizeof(buf) - 1) {
				len = 0;
			}
		}
	}

	return false;
}

static void send_line(int sock)
{
	zassert_equal(zsock_send(sock, "\n", 1, 0), 1, "send() failed (%d)", errno);
}

/* Take every network TX data buffer, as a busy stream would */
static struct net_buf *take_tx_buffers(void)
{
	struct net_buf *head = NULL;
	struct net_buf *buf;

	while ((buf = net_pkt_get_reserve_tx_data(1, K_NO_WAIT)) != NULL) {
		head = head == NULL ? buf : net_buf_frag_add(head, buf);
	}

	return head;
}

ZTEST(shell_telnet, test_failed_deferred_send)
{
	struct net_buf *taken;
	int sock;

	sock = connect_client();
	send_line(sock);
	zassert_true(recv_until(sock, PROMPT, K_SECONDS(2)), "no prompt");

	/* The line feed goes out at once, the prompt after it from the send
	 * work, which finds no buffer: the shell ends the connection there.
	 */
	send_line(sock);
	zassert_true(recv_until(sock, "\n", K_SECONDS(2)), "no line feed");
	k_msleep(10);
	taken = take_tx_buffers();
	zassert_not_null(taken);
	k_msleep(2 * CONFIG_SHELL_TELNET_SEND_TIMEOUT + CONFIG_NET_TCP_PKT_ALLOC_TIMEOUT);
	net_buf_unref(taken);

	k_work_submit(&probe_work);
	zassert_ok(k_sem_take(&probe_sem, K_SECONDS(1)), "the system work queue is stuck");
	zsock_close(sock);

	/* And the shell takes a new client */
	k_msleep(100);
	sock = connect_client();
	send_line(sock);
	zassert_true(recv_until(sock, PROMPT, K_SECONDS(2)), "no prompt for a new client");
	zsock_close(sock);
}

ZTEST_SUITE(shell_telnet, NULL, NULL, NULL, NULL, NULL);
