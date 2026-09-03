/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tftp.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define SERVER_PORT 69

#define DATA_OPCODE  3
#define ACK_OPCODE   4
#define ERROR_OPCODE 5

#define TFTP_ERROR_NO_FILE   1
#define TFTP_ERROR_DISK_FULL 3

#define SERVER_QUIET_MS     300
#define SERVER_MAX_REQUESTS 8

static int server_fd = -1;

static K_THREAD_STACK_DEFINE(server_stack, 2048);
static struct k_thread server_thread;

#define MAX_REPLIES 2

static uint8_t server_reply[MAX_REPLIES][TFTPC_MAX_BUF_SIZE];
static size_t server_reply_len[MAX_REPLIES];
static int server_replies;

static bool saw_error;
static bool saw_data;
static int seen_code;
static size_t seen_msg_len;
static char seen_msg[64];
static size_t seen_data_len;

static struct {
	struct tftpc client;
	uint8_t poison[1024];
} fixture;

static void tftp_cb(const struct tftp_evt *evt)
{
	switch (evt->type) {
	case TFTP_EVT_ERROR:
		saw_error = true;
		seen_code = evt->param.error.code;
		seen_msg_len = strlen(evt->param.error.msg);
		strncpy(seen_msg, evt->param.error.msg, sizeof(seen_msg) - 1);
		seen_msg[sizeof(seen_msg) - 1] = '\0';
		break;
	case TFTP_EVT_DATA:
		saw_data = true;
		seen_data_len = evt->param.data.len;
		break;
	default:
		break;
	}
}

static void server_fn(void *a, void *b, void *c)
{
	struct net_sockaddr_storage from;
	uint8_t request[TFTPC_MAX_BUF_SIZE];

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	for (int i = 0; i < SERVER_MAX_REQUESTS; i++) {
		struct zsock_pollfd fds = {
			.fd = server_fd,
			.events = ZSOCK_POLLIN,
		};
		net_socklen_t from_len = sizeof(from);
		int idx = MIN(i, server_replies - 1);

		if (zsock_poll(&fds, 1, SERVER_QUIET_MS) <= 0) {
			return;
		}

		if (zsock_recvfrom(server_fd, request, sizeof(request), 0,
				   (struct net_sockaddr *)&from, &from_len) < 0) {
			return;
		}

		(void)zsock_sendto(server_fd, server_reply[idx], server_reply_len[idx], 0,
				   (struct net_sockaddr *)&from, from_len);
	}
}

static void prepare_client(void)
{
	struct net_sockaddr_in *addr;

	memset(&fixture, 0xAA, sizeof(fixture));
	fixture.poison[sizeof(fixture.poison) - 1] = '\0';

	memset(&fixture.client.server_addr, 0, sizeof(fixture.client.server_addr));
	addr = (struct net_sockaddr_in *)&fixture.client.server_addr;
	addr->sin_family = NET_AF_INET;
	addr->sin_port = net_htons(SERVER_PORT);
	addr->sin_addr.s4_addr[0] = 127;
	addr->sin_addr.s4_addr[3] = 1;

	fixture.client.callback = tftp_cb;

	saw_error = false;
	saw_data = false;
	seen_code = -1;
	seen_msg_len = 0;
	seen_msg[0] = '\0';
	seen_data_len = 0;
}

static int run_transfer(bool put)
{
	static const uint8_t payload[4] = {1, 2, 3, 4};
	k_tid_t tid;
	int ret;

	tid = k_thread_create(&server_thread, server_stack, K_THREAD_STACK_SIZEOF(server_stack),
			      server_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	if (put) {
		ret = tftp_put(&fixture.client, "file", "octet", payload, sizeof(payload));
	} else {
		ret = tftp_get(&fixture.client, "file", "octet");
	}

	zassert_ok(k_thread_join(tid, K_SECONDS(5)), "the server thread did not finish");

	return ret;
}

static int run_get(void)
{
	return run_transfer(false);
}

static int run_put(void)
{
	return run_transfer(true);
}

static void set_error_reply(int slot, uint16_t code, const char *msg, size_t msg_len,
			    bool terminate)
{
	if (terminate) {
		size_t max_msg_len = TFTPC_MAX_BUF_SIZE - TFTP_HEADER_SIZE - 1;

		if (msg_len > max_msg_len) {
			msg_len = max_msg_len;
		}
	}

	sys_put_be16(ERROR_OPCODE, server_reply[slot]);
	sys_put_be16(code, server_reply[slot] + 2);

	if (msg != NULL) {
		memcpy(server_reply[slot] + TFTP_HEADER_SIZE, msg, msg_len);
	} else {
		memset(server_reply[slot] + TFTP_HEADER_SIZE, 'A', msg_len);
	}

	server_reply_len[slot] = TFTP_HEADER_SIZE + msg_len;

	if (terminate) {
		server_reply[slot][server_reply_len[slot]] = '\0';
		server_reply_len[slot]++;
	}
}

static void set_ack_reply(int slot, uint16_t block)
{
	sys_put_be16(ACK_OPCODE, server_reply[slot]);
	sys_put_be16(block, server_reply[slot] + 2);
	server_reply_len[slot] = TFTP_HEADER_SIZE;
}

ZTEST(tftp_client_error_response, test_unterminated_error_message_stays_in_the_buffer)
{
	prepare_client();
	server_replies = 1;
	set_error_reply(0, TFTP_ERROR_NO_FILE, NULL, TFTP_BLOCK_SIZE, false);

	zassert_equal(run_get(), TFTPC_REMOTE_ERROR, "the error was not reported");
	zassert_true(saw_error, "the callback saw no error event");
	zassert_true(seen_msg_len < TFTP_BLOCK_SIZE,
		     "the message runs %zu bytes past the end of tftp_buf",
		     seen_msg_len - TFTP_BLOCK_SIZE + 1);
}

ZTEST(tftp_client_error_response, test_terminated_error_message_is_passed_through)
{
	static const char msg[] = "file not found";

	prepare_client();
	server_replies = 1;
	set_error_reply(0, TFTP_ERROR_NO_FILE, msg, strlen(msg), true);

	zassert_equal(run_get(), TFTPC_REMOTE_ERROR, "the error was not reported");
	zassert_true(saw_error, "the callback saw no error event");
	zassert_equal(seen_msg_len, strlen(msg), "the message was truncated");
	zassert_str_equal(seen_msg, msg, "the message was altered");
	zassert_equal(seen_code, TFTP_ERROR_NO_FILE, "wrong error code reported");
}

ZTEST(tftp_client_error_response, test_terminated_error_message_is_clamped_to_fit)
{
	prepare_client();
	server_replies = 1;
	memset(server_reply[1], 0xAA, sizeof(server_reply[1]));
	set_error_reply(0, TFTP_ERROR_NO_FILE, NULL, TFTP_BLOCK_SIZE, true);

	zassert_equal(server_reply[1][0], 0xAA, "set_error_reply() wrote past server_reply[0]");

	zassert_equal(run_get(), TFTPC_REMOTE_ERROR, "the error was not reported");
	zassert_true(saw_error, "the callback saw no error event");
	zassert_equal(seen_msg_len, TFTPC_MAX_BUF_SIZE - TFTP_HEADER_SIZE - 1,
		      "the clamp let %zu bytes through", seen_msg_len);
	zassert_equal(seen_code, TFTP_ERROR_NO_FILE, "wrong error code reported");
}

ZTEST(tftp_client_error_response, test_error_without_a_message)
{
	prepare_client();
	server_replies = 1;
	set_error_reply(0, TFTP_ERROR_NO_FILE, NULL, 0, false);

	zassert_equal(run_get(), TFTPC_REMOTE_ERROR, "the error was not reported");
	zassert_true(saw_error, "the callback saw no error event");
	zassert_equal(seen_msg_len, 0, "an empty message was reported as %zu bytes", seen_msg_len);
}

ZTEST(tftp_client_error_response, test_short_data_block_ends_the_transfer)
{
	prepare_client();

	server_replies = 1;
	sys_put_be16(DATA_OPCODE, server_reply[0]);
	sys_put_be16(1, server_reply[0] + 2);
	memset(server_reply[0] + TFTP_HEADER_SIZE, 'x', 4);
	server_reply_len[0] = TFTP_HEADER_SIZE + 4;

	zassert_equal(run_get(), 4, "the transfer did not report 4 bytes");
	zassert_true(saw_data, "the callback saw no data event");
	zassert_equal(seen_data_len, 4, "wrong payload length");
}

ZTEST(tftp_client_error_response, test_data_block_rejected_reports_the_servers_code)
{
	prepare_client();

	server_replies = 2;
	set_ack_reply(0, 0);
	set_error_reply(1, TFTP_ERROR_DISK_FULL, NULL, 0, false);

	(void)run_put();

	zassert_true(saw_error, "the callback saw no error event");
	zassert_equal(seen_code, TFTP_ERROR_DISK_FULL,
		      "reported error code %d, not the server's %d", seen_code,
		      TFTP_ERROR_DISK_FULL);
	zassert_equal(seen_msg_len, 0, "an absent message was reported as %zu bytes", seen_msg_len);
}

ZTEST(tftp_client_error_response, test_data_block_rejected_with_a_message_reports_it)
{
	static const char msg[] = "disk full";

	prepare_client();

	server_replies = 2;
	set_ack_reply(0, 0);
	set_error_reply(1, TFTP_ERROR_DISK_FULL, msg, strlen(msg), true);

	(void)run_put();

	zassert_true(saw_error, "the callback saw no error event");
	zassert_equal(seen_code, TFTP_ERROR_DISK_FULL,
		      "reported error code %d, not the server's %d", seen_code,
		      TFTP_ERROR_DISK_FULL);
	zassert_equal(seen_msg_len, strlen(msg), "the message was truncated");
	zassert_str_equal(seen_msg, msg, "the message was altered");
}

ZTEST(tftp_client_error_response, test_put_completes)
{
	prepare_client();

	server_replies = 2;
	set_ack_reply(0, 0);
	set_ack_reply(1, 1);

	zassert_equal(run_put(), 4, "the transfer did not report 4 bytes sent");
	zassert_false(saw_error, "an error was reported for a clean transfer");
}

static void before(void *f)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(SERVER_PORT),
	};

	ARG_UNUSED(f);

	addr.sin_addr.s4_addr[0] = 127;
	addr.sin_addr.s4_addr[3] = 1;

	server_fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(server_fd >= 0, "could not open the server socket");

	zassert_ok(zsock_bind(server_fd, (struct net_sockaddr *)&addr, sizeof(addr)),
		   "could not bind the server socket");
}

static void after(void *f)
{
	ARG_UNUSED(f);

	(void)zsock_close(server_fd);
	server_fd = -1;
}

ZTEST_SUITE(tftp_client_error_response, NULL, NULL, before, after, NULL);
