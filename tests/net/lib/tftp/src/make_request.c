/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/net/tftp.h>
#include <zephyr/ztest.h>

#include "tftp_client.h"

#define REQUEST_BUF_PADDING 256

#define DEFAULT_MODE_LEN 5

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 69

#define REQUEST_WAIT_MS 100

static struct tftpc api_client;
static const uint8_t put_data[] = "data";

static void fill(char *dst, size_t len, char c)
{
	memset(dst, c, len);
	dst[len] = '\0';
}

/* Bind a socket where api_client sends its requests, so a test can see them. */
static int bind_server(void)
{
	struct net_sockaddr_in *addr = (struct net_sockaddr_in *)&api_client.server_addr;
	int fd;

	addr->sin_family = NET_AF_INET;
	addr->sin_port = net_htons(SERVER_PORT);
	zassert_equal(zsock_inet_pton(NET_AF_INET, SERVER_ADDR, &addr->sin_addr), 1,
		      "could not parse %s", SERVER_ADDR);

	fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(fd >= 0, "could not open a UDP socket");

	if (zsock_bind(fd, (struct net_sockaddr *)addr, sizeof(*addr)) < 0) {
		(void)zsock_close(fd);
		zassert_unreachable("could not bind to %s:%u", SERVER_ADDR, SERVER_PORT);
	}

	return fd;
}

/* Size of the first request that reached fd, or 0 if none did. */
static int first_request_size(int fd)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + 1];
	struct zsock_pollfd pfd = {
		.fd = fd,
		.events = ZSOCK_POLLIN,
	};

	if (zsock_poll(&pfd, 1, REQUEST_WAIT_MS) <= 0) {
		return 0;
	}

	return zsock_recv(fd, buf, sizeof(buf), 0);
}

ZTEST(tftp_client, test_make_request_filename_one_under_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	char remote_file[TFTP_MAX_FILENAME_SIZE];
	size_t req_size;

	fill(remote_file, TFTP_MAX_FILENAME_SIZE - 1, 'a');

	req_size = make_request(buf, READ_REQUEST, remote_file, NULL);

	zassert_equal(req_size, 2 + (TFTP_MAX_FILENAME_SIZE - 1) + 1 + DEFAULT_MODE_LEN + 1,
		      "unexpected request size %zu for a filename one byte under budget", req_size);
}

ZTEST(tftp_client, test_make_request_filename_at_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	char remote_file[TFTP_MAX_FILENAME_SIZE + 1];
	size_t req_size;

	fill(remote_file, TFTP_MAX_FILENAME_SIZE, 'a');

	req_size = make_request(buf, READ_REQUEST, remote_file, NULL);

	zassert_equal(req_size, 2 + TFTP_MAX_FILENAME_SIZE + 1 + DEFAULT_MODE_LEN + 1,
		      "unexpected request size %zu for a filename exactly at budget", req_size);
}

ZTEST(tftp_client, test_filename_one_over_budget_is_rejected)
{
	char remote_file[TFTP_MAX_FILENAME_SIZE + 2];
	int fd = bind_server();

	fill(remote_file, TFTP_MAX_FILENAME_SIZE + 1, 'a');

	zexpect_equal(tftp_get(&api_client, remote_file, NULL), -EINVAL,
		      "tftp_get() accepted a filename one byte over budget");
	zexpect_equal(tftp_put(&api_client, remote_file, NULL, put_data, sizeof(put_data)),
		      -EINVAL, "tftp_put() accepted a filename one byte over budget");
	zexpect_equal(first_request_size(fd), 0,
		      "a request was sent for a filename one byte over budget");

	(void)zsock_close(fd);
}

ZTEST(tftp_client, test_make_request_mode_one_under_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	const char *remote_file = "file.bin";
	char mode[TFTP_MAX_MODE_SIZE];
	size_t req_size;

	fill(mode, TFTP_MAX_MODE_SIZE - 1, 'b');

	req_size = make_request(buf, READ_REQUEST, remote_file, mode);

	zassert_equal(req_size, 2 + strlen(remote_file) + 1 + (TFTP_MAX_MODE_SIZE - 1) + 1,
		      "unexpected request size %zu for a mode one byte under budget", req_size);
}

ZTEST(tftp_client, test_make_request_mode_at_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	const char *remote_file = "file.bin";
	char mode[TFTP_MAX_MODE_SIZE + 1];
	size_t req_size;

	fill(mode, TFTP_MAX_MODE_SIZE, 'b');

	req_size = make_request(buf, READ_REQUEST, remote_file, mode);

	zassert_equal(req_size, 2 + strlen(remote_file) + 1 + TFTP_MAX_MODE_SIZE + 1,
		      "unexpected request size %zu for a mode exactly at budget", req_size);
}

ZTEST(tftp_client, test_mode_one_over_budget_is_rejected)
{
	const char *remote_file = "file.bin";
	char mode[TFTP_MAX_MODE_SIZE + 2];
	int fd = bind_server();

	fill(mode, TFTP_MAX_MODE_SIZE + 1, 'b');

	zexpect_equal(tftp_get(&api_client, remote_file, mode), -EINVAL,
		      "tftp_get() accepted a mode one byte over budget");
	zexpect_equal(tftp_put(&api_client, remote_file, mode, put_data, sizeof(put_data)),
		      -EINVAL, "tftp_put() accepted a mode one byte over budget");
	zexpect_equal(first_request_size(fd), 0,
		      "a request was sent for a mode one byte over budget");

	(void)zsock_close(fd);
}

ZTEST(tftp_client, test_make_request_filename_and_mode_at_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	char remote_file[TFTP_MAX_FILENAME_SIZE + 1];
	char mode[TFTP_MAX_MODE_SIZE + 1];
	size_t req_size;

	fill(remote_file, TFTP_MAX_FILENAME_SIZE, 'a');
	fill(mode, TFTP_MAX_MODE_SIZE, 'b');

	req_size = make_request(buf, READ_REQUEST, remote_file, mode);

	zassert_equal(req_size, TFTPC_MAX_BUF_SIZE,
		      "unexpected request size %zu with both filename and mode at budget",
		      req_size);
}

ZTEST(tftp_client, test_filename_and_mode_at_budget_are_sent_whole)
{
	char remote_file[TFTP_MAX_FILENAME_SIZE + 1];
	char mode[TFTP_MAX_MODE_SIZE + 1];
	int fd = bind_server();

	fill(remote_file, TFTP_MAX_FILENAME_SIZE, 'a');
	fill(mode, TFTP_MAX_MODE_SIZE, 'b');

	/* No server answers, so the transfer fails after the request went out. */
	zexpect_not_equal(tftp_get(&api_client, remote_file, mode), -EINVAL,
			  "tftp_get() rejected a filename and mode at budget");
	zexpect_equal(first_request_size(fd), TFTPC_MAX_BUF_SIZE,
		      "the request for a filename and mode at budget was not sent whole");

	(void)zsock_close(fd);
}

ZTEST(tftp_client, test_make_request_short_filename_unaffected)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE];
	const char *remote_file = "boot.bin";
	size_t req_size;

	req_size = make_request(buf, READ_REQUEST, remote_file, NULL);

	zassert_equal(req_size, 2 + strlen(remote_file) + 1 + DEFAULT_MODE_LEN + 1,
		      "unexpected request size %zu for a short filename", req_size);
}
