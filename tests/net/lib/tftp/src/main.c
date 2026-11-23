/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tftp.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(tftp_client_test, LOG_LEVEL_INF);

#include "net_private.h"

#define SERVER_PORT 69

#define DATA_OPCODE  3
#define ERROR_OPCODE 5

#define TFTP_ERROR_DISK_FULL 3

#define TFTP_BLOCK_SIZE 512

#define V4_SERVER      "127.0.0.1"
#define V4_SAME_PREFIX "127.0.0.2"
#define V4_OFF_PREFIX  "192.0.2.2"

#define V6_SERVER      "2001:db8::1"
#define V6_SAME_PREFIX "2001:db8::2"
#define V6_OFF_PREFIX  "2001:db8:1::2"

#define SERVER_WAIT_MS 300

#define FORGED_BURST     8
#define FORGED_RUNT_LEN  2
#define REQUEST_COUNT_MS 600

static K_THREAD_STACK_DEFINE(server_stack, 2048);
static struct k_thread server_thread;

static struct tftpc client;

static bool saw_data;
static bool saw_error;
static size_t seen_data_len;
static size_t seen_data_total;
static uint8_t seen_first_byte;
static int seen_error_code;

static int request_fd = -1;
static int reply_fd = -1;
static int attacker_fd = -1;

static net_sa_family_t family;
static const char *server_addr_str;

static bool zero_server_addr_on_request;

static bool forge_second_block;

static bool forge_before_connect;

static bool forge_burst_before_reply;
static int seen_requests;

#define MAX_ADDED 3
static struct {
	net_sa_family_t family;
	union {
		struct net_in_addr v4;
		struct net_in6_addr v6;
	} addr;
} added[MAX_ADDED];
static int added_count;

static void tftp_cb(const struct tftp_evt *evt)
{
	switch (evt->type) {
	case TFTP_EVT_DATA:
		saw_data = true;
		seen_data_len = evt->param.data.len;
		seen_data_total += evt->param.data.len;
		if (evt->param.data.len > 0) {
			seen_first_byte = evt->param.data.data_ptr[0];
		}
		break;
	case TFTP_EVT_ERROR:
		saw_error = true;
		seen_error_code = evt->param.error.code;
		break;
	default:
		break;
	}
}

static void parse_addr(const char *str, void *out)
{
	zassert_equal(zsock_inet_pton(family, str, out), 1, "could not parse %s", str);
}

static void iface_add_addr(const char *str)
{
	struct net_if *iface = net_if_get_default();

	zassert_not_null(iface, "no default network interface");
	zassert_true(added_count < MAX_ADDED, "too many interface addresses in one case");

	added[added_count].family = family;

	if (family == NET_AF_INET6) {
		parse_addr(str, &added[added_count].addr.v6);
		zassert_not_null(net_if_ipv6_addr_add(iface, &added[added_count].addr.v6,
						      NET_ADDR_MANUAL, 0),
				 "could not add %s to the interface", str);
	} else {
		parse_addr(str, &added[added_count].addr.v4);
		zassert_not_null(net_if_ipv4_addr_add(iface, &added[added_count].addr.v4,
						      NET_ADDR_MANUAL, 0),
				 "could not add %s to the interface", str);
	}

	added_count++;
}

static void iface_drop_addrs(void)
{
	struct net_if *iface = net_if_get_default();

	while (added_count > 0) {
		added_count--;

		if (iface == NULL) {
			continue;
		}

		if (added[added_count].family == NET_AF_INET6) {
			(void)net_if_ipv6_addr_rm(iface, &added[added_count].addr.v6);
		} else {
			(void)net_if_ipv4_addr_rm(iface, &added[added_count].addr.v4);
		}
	}
}

static int bind_loopback(const char *addr_str, uint16_t port)
{
	struct net_sockaddr_storage addr = {
		.ss_family = family,
	};
	net_socklen_t addr_len;
	int fd;

	if (family == NET_AF_INET6) {
		struct net_sockaddr_in6 *a6 = (struct net_sockaddr_in6 *)&addr;

		a6->sin6_port = net_htons(port);
		parse_addr(addr_str, &a6->sin6_addr);
		addr_len = sizeof(*a6);
	} else {
		struct net_sockaddr_in *a4 = (struct net_sockaddr_in *)&addr;

		a4->sin_port = net_htons(port);
		parse_addr(addr_str, &a4->sin_addr);
		addr_len = sizeof(*a4);
	}

	fd = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(fd >= 0, "could not open a UDP socket");

	if (zsock_bind(fd, (struct net_sockaddr *)&addr, addr_len) < 0) {
		int err = errno;

		(void)zsock_close(fd);
		zassert_unreachable("could not bind to [%s]:%u, err %d", addr_str, port, err);
	}

	return fd;
}

static void setup_server(net_sa_family_t af)
{
	family = af;

	if (family == NET_AF_INET6) {
		struct net_sockaddr_in6 *a6 = (struct net_sockaddr_in6 *)&client.server_addr;

		server_addr_str = V6_SERVER;
		iface_add_addr(server_addr_str);

		a6->sin6_family = NET_AF_INET6;
		a6->sin6_port = net_htons(SERVER_PORT);
		parse_addr(server_addr_str, &a6->sin6_addr);
	} else {
		struct net_sockaddr_in *a4 = (struct net_sockaddr_in *)&client.server_addr;

		server_addr_str = V4_SERVER;

		a4->sin_family = NET_AF_INET;
		a4->sin_port = net_htons(SERVER_PORT);
		parse_addr(server_addr_str, &a4->sin_addr);
	}

	request_fd = bind_loopback(server_addr_str, SERVER_PORT);
	reply_fd = bind_loopback(server_addr_str, 0);
}

static void setup_attacker(const char *addr_str)
{
	iface_add_addr(addr_str);
	attacker_fd = bind_loopback(addr_str, 0);
}

static void setup_attacker_on_server_address(void)
{
	attacker_fd = bind_loopback(server_addr_str, 0);
}

static void send_data_block(int fd, uint16_t block_no, uint8_t fill, size_t payload,
			    const struct net_sockaddr *to, net_socklen_t to_len)
{
	uint8_t buf[TFTP_HEADER_SIZE + TFTP_BLOCK_SIZE];

	sys_put_be16(DATA_OPCODE, buf);
	sys_put_be16(block_no, buf + 2);
	memset(buf + TFTP_HEADER_SIZE, fill, payload);

	(void)zsock_sendto(fd, buf, TFTP_HEADER_SIZE + payload, 0, to, to_len);
}

static void server_fn(void *a, void *b, void *c)
{
	struct net_sockaddr_storage from;
	net_socklen_t from_len = sizeof(from);
	struct net_sockaddr *from_sa = (struct net_sockaddr *)&from;
	uint8_t request[64];
	struct zsock_pollfd fds = {
		.fd = request_fd,
		.events = ZSOCK_POLLIN,
	};

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	if (zsock_poll(&fds, 1, SERVER_WAIT_MS) <= 0) {
		return;
	}

	if (zsock_recvfrom(request_fd, request, sizeof(request), 0, from_sa, &from_len) < 0) {
		return;
	}

	seen_requests++;

	if (zero_server_addr_on_request) {
		if (family == NET_AF_INET6) {
			struct net_sockaddr_in6 *a6 =
				(struct net_sockaddr_in6 *)&client.server_addr;

			memset(&a6->sin6_addr, 0, sizeof(a6->sin6_addr));
		} else {
			struct net_sockaddr_in *a4 = (struct net_sockaddr_in *)&client.server_addr;

			a4->sin_addr.s_addr = 0;
		}
	}

	if (forge_before_connect) {
		send_data_block(reply_fd, 1, 's', TFTP_BLOCK_SIZE, from_sa, from_len);
		send_data_block(attacker_fd, 2, 'a', 4, from_sa, from_len);

		fds.fd = reply_fd;
		if (zsock_poll(&fds, 1, SERVER_WAIT_MS) <= 0) {
			return;
		}

		from_len = sizeof(from);
		if (zsock_recvfrom(reply_fd, request, sizeof(request), 0, from_sa, &from_len) < 0) {
			return;
		}

		send_data_block(reply_fd, 2, 's', 4, from_sa, from_len);
		return;
	}

	if (forge_burst_before_reply) {
		static const uint8_t runt[FORGED_RUNT_LEN];
		int64_t until;

		for (int i = 0; i < FORGED_BURST; i++) {
			(void)zsock_sendto(attacker_fd, runt, sizeof(runt), 0, from_sa, from_len);
		}

		send_data_block(reply_fd, 1, 'x', 4, from_sa, from_len);

		until = k_uptime_get() + REQUEST_COUNT_MS;
		while (k_uptime_get() < until) {
			int64_t left = until - k_uptime_get();

			if (zsock_poll(&fds, 1, (int)left) <= 0) {
				break;
			}

			from_len = sizeof(from);
			if (zsock_recvfrom(request_fd, request, sizeof(request), 0, from_sa,
					   &from_len) < 0) {
				break;
			}

			seen_requests++;
		}

		return;
	}

	if (attacker_fd >= 0 && !forge_second_block) {
		uint8_t forged[TFTP_HEADER_SIZE];

		sys_put_be16(ERROR_OPCODE, forged);
		sys_put_be16(TFTP_ERROR_DISK_FULL, forged + 2);

		(void)zsock_sendto(attacker_fd, forged, sizeof(forged), 0, from_sa, from_len);
	}

	if (!forge_second_block) {
		send_data_block(reply_fd, 1, 'x', 4, from_sa, from_len);
		return;
	}

	send_data_block(reply_fd, 1, 's', TFTP_BLOCK_SIZE, from_sa, from_len);

	fds.fd = reply_fd;
	if (zsock_poll(&fds, 1, SERVER_WAIT_MS) <= 0) {
		return;
	}

	if (zsock_recvfrom(reply_fd, request, sizeof(request), 0, from_sa, &from_len) < 0) {
		return;
	}

	send_data_block(attacker_fd, 2, 'a', 4, from_sa, from_len);
	send_data_block(reply_fd, 2, 's', 4, from_sa, from_len);
}

static int run_transfer_ret(void)
{
	k_tid_t tid;
	int ret;

	tid = k_thread_create(&server_thread, server_stack, K_THREAD_STACK_SIZEOF(server_stack),
			      server_fn, NULL, NULL, NULL, K_PRIO_COOP(0), 0, K_NO_WAIT);

	ret = tftp_get(&client, "file", "octet");

	zassert_ok(k_thread_join(tid, K_SECONDS(5)), "the server thread did not finish");

	return ret;
}

static void run_transfer(void)
{
	zassert_equal(run_transfer_ret(), 4, "the transfer did not report 4 bytes");
}

static void expect_server_reply_won(void)
{
	zexpect_true(saw_data, "the real server's reply was not accepted");
	zexpect_equal(seen_data_len, 4, "wrong payload length");
	zexpect_false(saw_error, "an error was reported for a transfer that should have completed");
	zexpect_not_equal(seen_error_code, TFTP_ERROR_DISK_FULL,
			  "the attacker's error code was reported");
}

static void expect_burst_cost_nothing(void)
{
	int ret = run_transfer_ret();

	zexpect_equal(ret, 4, "tftp_get() returned %d, expected 4 bytes of data", ret);
	zexpect_equal(seen_requests, 1, "the client put %d requests on the wire, expected 1",
		      seen_requests);
	expect_server_reply_won();
}

static void before(void *f)
{
	ARG_UNUSED(f);

	loopback_enable_address_swap(false);

	memset(&client, 0, sizeof(client));
	client.callback = tftp_cb;

	saw_data = false;
	saw_error = false;
	seen_data_len = 0;
	seen_data_total = 0;
	seen_first_byte = 0;
	seen_error_code = -1;

	request_fd = -1;
	reply_fd = -1;
	attacker_fd = -1;
	added_count = 0;
	zero_server_addr_on_request = false;
	forge_second_block = false;
	forge_before_connect = false;
	forge_burst_before_reply = false;
	seen_requests = 0;
	family = NET_AF_INET;
}

static void after(void *f)
{
	ARG_UNUSED(f);

	if (request_fd >= 0) {
		(void)zsock_close(request_fd);
		request_fd = -1;
	}
	if (reply_fd >= 0) {
		(void)zsock_close(reply_fd);
		reply_fd = -1;
	}
	if (attacker_fd >= 0) {
		(void)zsock_close(attacker_fd);
		attacker_fd = -1;
	}

	iface_drop_addrs();

	loopback_enable_address_swap(true);
}

ZTEST(tftp_client, test_reply_from_a_different_port_is_accepted)
{
	setup_server(NET_AF_INET);

	run_transfer();

	expect_server_reply_won();
}

ZTEST(tftp_client, test_reply_from_a_different_port_is_accepted_v6)
{
	setup_server(NET_AF_INET6);

	run_transfer();

	expect_server_reply_won();
}

ZTEST(tftp_client, test_reply_from_a_different_address_is_rejected)
{
	setup_server(NET_AF_INET);
	setup_attacker(V4_OFF_PREFIX);

	run_transfer();

	expect_server_reply_won();
}

ZTEST(tftp_client, test_reply_from_a_different_address_is_rejected_v6)
{
	setup_server(NET_AF_INET6);
	setup_attacker(V6_OFF_PREFIX);

	run_transfer();

	expect_server_reply_won();
}

ZTEST(tftp_client, test_reply_from_a_same_prefix_address_is_rejected)
{
	setup_server(NET_AF_INET);
	setup_attacker(V4_SAME_PREFIX);

	run_transfer();

	expect_server_reply_won();
}

ZTEST(tftp_client, test_reply_from_a_same_prefix_address_is_rejected_v6)
{
	setup_server(NET_AF_INET6);
	setup_attacker(V6_SAME_PREFIX);

	run_transfer();

	expect_server_reply_won();
}

ZTEST(tftp_client, test_a_forged_block_mid_transfer_is_rejected)
{
	setup_server(NET_AF_INET);
	setup_attacker(V4_OFF_PREFIX);
	forge_second_block = true;

	zassert_equal(run_transfer_ret(), TFTP_BLOCK_SIZE + 4,
		      "the transfer did not report both of the server's blocks");

	zexpect_true(saw_data, "the callback saw no data event");
	zexpect_equal(seen_data_total, TFTP_BLOCK_SIZE + 4, "wrong total payload length");
	zexpect_equal(seen_first_byte, 's', "the attacker's block was reported to the caller");
	zexpect_false(saw_error, "an error was reported for a transfer that should have completed");
}

ZTEST(tftp_client, test_a_forged_block_mid_transfer_is_rejected_v6)
{
	setup_server(NET_AF_INET6);
	setup_attacker(V6_OFF_PREFIX);
	forge_second_block = true;

	zassert_equal(run_transfer_ret(), TFTP_BLOCK_SIZE + 4,
		      "the transfer did not report both of the server's blocks");

	zexpect_true(saw_data, "the callback saw no data event");
	zexpect_equal(seen_data_total, TFTP_BLOCK_SIZE + 4, "wrong total payload length");
	zexpect_equal(seen_first_byte, 's', "the attacker's block was reported to the caller");
	zexpect_false(saw_error, "an error was reported for a transfer that should have completed");
}

ZTEST(tftp_client, test_a_forged_block_from_the_server_address_is_rejected)
{
	setup_server(NET_AF_INET);
	setup_attacker_on_server_address();
	forge_second_block = true;

	zassert_equal(run_transfer_ret(), TFTP_BLOCK_SIZE + 4,
		      "the transfer did not report both of the server's blocks");

	zexpect_true(saw_data, "the callback saw no data event");
	zexpect_equal(seen_data_total, TFTP_BLOCK_SIZE + 4, "wrong total payload length");
	zexpect_equal(seen_first_byte, 's', "the attacker's block was reported to the caller");
	zexpect_false(saw_error, "an error was reported for a transfer that should have completed");
}

ZTEST(tftp_client, test_a_forged_block_from_the_server_address_is_rejected_v6)
{
	setup_server(NET_AF_INET6);
	setup_attacker_on_server_address();
	forge_second_block = true;

	zassert_equal(run_transfer_ret(), TFTP_BLOCK_SIZE + 4,
		      "the transfer did not report both of the server's blocks");

	zexpect_true(saw_data, "the callback saw no data event");
	zexpect_equal(seen_data_total, TFTP_BLOCK_SIZE + 4, "wrong total payload length");
	zexpect_equal(seen_first_byte, 's', "the attacker's block was reported to the caller");
	zexpect_false(saw_error, "an error was reported for a transfer that should have completed");
}

ZTEST(tftp_client, test_a_forged_block_racing_the_connect_is_rejected)
{
	setup_server(NET_AF_INET);
	setup_attacker_on_server_address();
	forge_before_connect = true;

	zassert_equal(run_transfer_ret(), TFTP_BLOCK_SIZE + 4,
		      "the transfer did not report both of the server's blocks");

	zexpect_true(saw_data, "the callback saw no data event");
	zexpect_equal(seen_data_total, TFTP_BLOCK_SIZE + 4, "wrong total payload length");
	zexpect_equal(seen_first_byte, 's', "the attacker's block was reported to the caller");
	zexpect_false(saw_error, "an error was reported for a transfer that should have completed");
}

ZTEST(tftp_client, test_a_forged_block_racing_the_connect_is_rejected_v6)
{
	setup_server(NET_AF_INET6);
	setup_attacker_on_server_address();
	forge_before_connect = true;

	zassert_equal(run_transfer_ret(), TFTP_BLOCK_SIZE + 4,
		      "the transfer did not report both of the server's blocks");

	zexpect_true(saw_data, "the callback saw no data event");
	zexpect_equal(seen_data_total, TFTP_BLOCK_SIZE + 4, "wrong total payload length");
	zexpect_equal(seen_first_byte, 's', "the attacker's block was reported to the caller");
	zexpect_false(saw_error, "an error was reported for a transfer that should have completed");
}

ZTEST(tftp_client, test_forged_datagrams_do_not_cost_a_retry)
{
	setup_server(NET_AF_INET);
	setup_attacker(V4_OFF_PREFIX);
	forge_burst_before_reply = true;

	expect_burst_cost_nothing();
}

ZTEST(tftp_client, test_forged_datagrams_do_not_cost_a_retry_v6)
{
	setup_server(NET_AF_INET6);
	setup_attacker(V6_OFF_PREFIX);
	forge_burst_before_reply = true;

	expect_burst_cost_nothing();
}

ZTEST(tftp_client, test_reply_when_server_address_is_unset_is_rejected)
{
	setup_server(NET_AF_INET);
	setup_attacker(V4_OFF_PREFIX);
	zero_server_addr_on_request = true;

	zexpect_not_equal(run_transfer_ret(), 4,
			  "an unset server address must not validate any reply");
	zexpect_false(saw_data, "a reply was accepted against an unset server address");
	zexpect_not_equal(seen_error_code, TFTP_ERROR_DISK_FULL,
			  "the attacker's error code was reported");
}

ZTEST(tftp_client, test_reply_when_server_address_is_unset_is_rejected_v6)
{
	setup_server(NET_AF_INET6);
	setup_attacker(V6_OFF_PREFIX);
	zero_server_addr_on_request = true;

	zexpect_not_equal(run_transfer_ret(), 4,
			  "an unset server address must not validate any reply");
	zexpect_false(saw_data, "a reply was accepted against an unset server address");
	zexpect_not_equal(seen_error_code, TFTP_ERROR_DISK_FULL,
			  "the attacker's error code was reported");
}

ZTEST_SUITE(tftp_client, NULL, NULL, before, after, NULL);
