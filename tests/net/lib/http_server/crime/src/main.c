/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "server_internal.h"

#include <string.h>

#include <zephyr/net/http/service.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#define BUFFER_SIZE  256
#define MY_IPV4_ADDR "127.0.0.1"
#define SERVER_PORT  8080
#define TIMEOUT      1000

static const unsigned char index_html_gz[] = {
#include "index.html.gz.inc"
};

static const unsigned char compressed_inc_file[] = {
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0xff, 0x35, 0x8e, 0xc1, 0x0a, 0xc2, 0x30,
	0x0c, 0x86, 0xef, 0x7d, 0x8a, 0xec, 0x05, 0x2c,
	0xbb, 0x87, 0x5c, 0x54, 0xf0, 0xe0, 0x50, 0x58,
	0x41, 0x3c, 0x4e, 0x17, 0x69, 0x21, 0xa5, 0x65,
	0x2d, 0x42, 0xdf, 0xde, 0xba, 0x6e, 0x21, 0x10,
	0xf8, 0xf9, 0xbe, 0x9f, 0x60, 0x77, 0xba, 0x1d,
	0xcd, 0xf3, 0x7e, 0x06, 0x9b, 0xbd, 0x90, 0xc2,
	0xfd, 0xf0, 0x34, 0x93, 0x82, 0x3a, 0x98, 0x5d,
	0x16, 0xa6, 0xa1, 0xc0, 0xe8, 0x7c, 0x14, 0x86,
	0x91, 0x97, 0x2f, 0x2f, 0xa8, 0x5b, 0xae, 0x50,
	0x37, 0x16, 0x5f, 0x61, 0x2e, 0x9b, 0x62, 0x7b,
	0x7a, 0xb0, 0xbc, 0x83, 0x67, 0xc8, 0x01, 0x7c,
	0x81, 0xd4, 0xd4, 0xb4, 0xaa, 0x5d, 0x55, 0xfa,
	0x8d, 0x8c, 0x64, 0xac, 0x4b, 0x50, 0x77, 0xda,
	0xa1, 0x8b, 0x19, 0xae, 0xf0, 0x71, 0xc2, 0x07,
	0xd4, 0xf1, 0xdf, 0xdf, 0x8a, 0xab, 0xb4, 0xbe,
	0xf6, 0x03, 0xea, 0x2d, 0x11, 0x5c, 0xb2, 0x00,
	0x00, 0x00,
};

static uint16_t test_http_service_port = SERVER_PORT;
HTTP_SERVICE_DEFINE(test_http_service, MY_IPV4_ADDR,
		    &test_http_service_port, 1,
		    10, NULL);

struct http_resource_detail_static index_html_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

HTTP_RESOURCE_DEFINE(index_html_gz_resource, test_http_service, "/",
		     &index_html_gz_resource_detail);

static void test_crime(void)
{
	int ret, recv_len;
	int client_fd;
	int proto = IPPROTO_TCP;
	char *ptr;
	const char *data;
	size_t len;
	struct sockaddr_in sa;
	static unsigned char buf[512];

	zassert_ok(http_server_start(), "Failed to start the server");

	ret = zsock_socket(AF_INET, SOCK_STREAM, proto);
	zassert_not_equal(ret, -1, "failed to create client socket (%d)", errno);
	client_fd = ret;

	sa.sin_family = AF_INET;
	sa.sin_port = htons(SERVER_PORT);

	ret = zsock_inet_pton(AF_INET, MY_IPV4_ADDR, &sa.sin_addr.s_addr);
	zassert_not_equal(-1, ret, "inet_pton() failed (%d)", errno);
	zassert_not_equal(0, ret, "%s is not a valid IPv4 address", MY_IPV4_ADDR);
	zassert_equal(1, ret, "inet_pton() failed to convert %s", MY_IPV4_ADDR);

	memset(buf, '\0', sizeof(buf));
	ptr = (char *)zsock_inet_ntop(AF_INET, &sa.sin_addr, buf, sizeof(buf));
	zassert_not_equal(ptr, NULL, "inet_ntop() failed (%d)", errno);

	ret = zsock_connect(client_fd, (struct sockaddr *)&sa, sizeof(sa));
	zassert_not_equal(ret, -1, "failed to connect (%s/%d)", strerror(errno), errno);

	char *http1_request = "GET / HTTP/1.1\r\n"
			      "Host: 127.0.0.1:8080\r\n"
			      "Accept: */*\r\n"
			      "Accept-Encoding: deflate, gzip, br\r\n"
			      "\r\n";

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));
	recv_len = zsock_recv(client_fd, buf, sizeof(buf), 0);
	zassert_not_equal(recv_len, -1, "recv() failed (%d)", errno);

	len = sizeof(index_html_gz);

	while (recv_len < len) {
		ret = zsock_recv(client_fd, buf + recv_len, sizeof(buf) - recv_len, 0);
		zassert_not_equal(ret, -1, "recv() failed (%d)", errno);

		recv_len += ret;
	}

	data = strstr(buf, "\r\n\r\n");
	zassert_not_null(data, "Header not found");

	data += 4;

	zassert_equal(len, sizeof(compressed_inc_file), "Invalid compressed file size");

	ret = memcmp(data, compressed_inc_file, len);
	zassert_equal(ret, 0,
		      "inc_file and compressed_inc_file contents are not identical (%d)", ret);

	ret = zsock_close(client_fd);
	zassert_not_equal(-1, ret, "close() failed on the client fd (%d)", errno);

	zassert_ok(http_server_stop(), "Failed to stop the server");
}

ZTEST(framework_tests_crime, test_gen_gz_inc_file)
{
	test_crime();
}

ZTEST_SUITE(framework_tests_crime, NULL, NULL, NULL, NULL, NULL);
