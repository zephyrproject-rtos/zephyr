/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#define BMC_SERVER_ADDR "127.0.0.1"
#define BMC_SERVER_PORT 80
#define RESPONSE_BUFFER_SIZE 2048

static void *bmc_suite_setup(void)
{
	zassert_ok(bmc_init(), "bmc_init() failed");
	k_msleep(100);

	return NULL;
}

static int connect_to_bmc(void)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(BMC_SERVER_PORT),
	};
	int fd;
	int ret;

	ret = zsock_inet_pton(AF_INET, BMC_SERVER_ADDR, &addr.sin_addr);
	zassert_equal(ret, 1, "inet_pton() failed");

	fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_not_equal(fd, -1, "socket() failed (%d)", errno);

	ret = zsock_connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	zassert_ok(ret, "connect() failed (%d)", errno);

	return fd;
}

static void expect_get_response_contains(const char *path, const char *expected_1,
					 const char *expected_2)
{
	char request[128];
	char response[RESPONSE_BUFFER_SIZE];
	size_t response_len = 0;
	int fd;
	int ret;

	ret = snprintk(request, sizeof(request),
		       "GET %s HTTP/1.1\r\n"
		       "Host: 127.0.0.1\r\n"
		       "Connection: close\r\n"
		       "\r\n",
		       path);
	zassert_true(ret > 0 && ret < sizeof(request), "request formatting failed");

	fd = connect_to_bmc();

	ret = zsock_send(fd, request, ret, 0);
	zassert_equal(ret, strlen(request), "send() failed (%d)", errno);

	while (response_len < sizeof(response) - 1) {
		ret = zsock_recv(fd, response + response_len,
				 sizeof(response) - response_len - 1, 0);
		zassert_true(ret >= 0, "recv() failed (%d)", errno);
		if (ret == 0) {
			break;
		}

		response_len += ret;
	}
	zassert_true(response_len > 0, "empty response");
	response[response_len] = '\0';

	zassert_not_null(strstr(response, "HTTP/1.1 200"), "missing 200 response");
	zassert_not_null(strstr(response, expected_1), "missing expected response text");
	zassert_not_null(strstr(response, expected_2), "missing second expected response text");

	zsock_close(fd);
}

ZTEST(bmc_http_connection, test_root_page_is_reachable)
{
	expect_get_response_contains("/", "text/html", "HTTP/1.1 200");
}

ZTEST(bmc_http_connection, test_redfish_service_root_is_reachable)
{
	expect_get_response_contains("/redfish/v1", "application/json", "RootService");
}

ZTEST(bmc_http_connection, test_webui_features_match_test_configuration)
{
	expect_get_response_contains("/webui/features", "\"hostConsole\":false",
				     "\"bmcShell\":false");
}

ZTEST_SUITE(bmc_http_connection, NULL, bmc_suite_setup, NULL, NULL, NULL);
