/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_service.h"
#include "server_functions.h"

#include <zephyr/net/http/service.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#define BUFFER_SIZE  256
#define STACK_SIZE   8192
#define MY_IPV4_ADDR "127.0.0.1"
#define SERVER_PORT  8000
#define TIMEOUT      1000

static struct k_sem server_sem;

static K_THREAD_STACK_DEFINE(server_stack, STACK_SIZE);

static struct k_thread server_thread;

static uint16_t test_http_service_port = htons(SERVER_PORT);
HTTP_SERVICE_DEFINE(test_http_service, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &test_http_service_port, 1,
		    10, NULL);

struct http_resource_detail_rest add_two_numbers_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_REST,
			.bitmask_of_supported_http_methods = POST,
		},
};

HTTP_RESOURCE_DEFINE(add_two_numbers, test_http_service, "/add", &add_two_numbers_detail);

static void server_thread_fn(void *arg0, void *arg1, void *arg2)
{
	struct http_server_ctx *ctx = (struct http_server_ctx *)arg0;

	k_thread_name_set(k_current_get(), "server");

	k_sem_give(&server_sem);

	http_server_start(ctx);
}

static void test_json(void)
{
	int r;
	int client_fd;
	int proto = IPPROTO_TCP;
	char *addrstrp;
	k_tid_t server_thread_id;
	struct sockaddr_in sa;
	char addrstr[256];

	k_sem_init(&server_sem, 0, 1);

	struct http_server_ctx ctx;

	int server_fd = http_server_init(&ctx);

	zassert_true(server_fd >= 0, "Failed to create server socket");

	server_thread_id =
		k_thread_create(&server_thread, server_stack, STACK_SIZE, server_thread_fn, &ctx,
				NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

	r = k_sem_take(&server_sem, K_MSEC(TIMEOUT));
	zassert_equal(0, r, "failed to synchronize with server thread (%d)", r);

	k_thread_name_set(k_current_get(), "client");

	r = socket(AF_INET, SOCK_STREAM, proto);
	zassert_not_equal(r, -1, "failed to create client socket (%d)", errno);
	client_fd = r;

	sa.sin_family = AF_INET;
	sa.sin_port = htons(SERVER_PORT);

	r = inet_pton(AF_INET, MY_IPV4_ADDR, &sa.sin_addr.s_addr);
	zassert_not_equal(-1, r, "inet_pton() failed (%d)", errno);
	zassert_not_equal(0, r, "%s is not a valid IPv4 address", MY_IPV4_ADDR);
	zassert_equal(1, r, "inet_pton() failed to convert %s", MY_IPV4_ADDR);

	memset(addrstr, '\0', sizeof(addrstr));
	addrstrp = (char *)inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr));
	zassert_not_equal(addrstrp, NULL, "inet_ntop() failed (%d)", errno);

	r = connect(client_fd, (struct sockaddr *)&sa, sizeof(sa));
	zassert_not_equal(r, -1, "failed to connect (%d)", errno);

	static const char http1_request[] = "POST /add HTTP/1.1\r\n"
				     "Host: 192.0.2.1:8080\r\n"
				     "User-Agent: curl/7.68.0\r\n"
				     "Accept: */*\r\n"
				     "Content-Type: application/json\r\n"
				     "Content-Length: 18\r\n"
				     "\r\n"
				     "{\"x\": 10, \"y\": 20}";

	r = send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(r, -1, "send() failed (%d)", errno);

	memset(addrstr, 0, sizeof(addrstr));
	r = recv(client_fd, addrstr, sizeof(addrstr), 0);
	zassert_not_equal(r, -1, "recv() failed (%d)", errno);

	char expected_response[] = "[{\"x\":10,\"y\":20,\"result\":30}]";

	memset(addrstr, 0, sizeof(addrstr));
	r = recv(client_fd, addrstr, sizeof(addrstr), 0);
	zassert_not_equal(r, -1, "recv() failed (%d)", errno);

	zassert_mem_equal(addrstr, expected_response, strlen(expected_response) + 1,
			  "Response does not match the expected JSON format");

	r = close(client_fd);
	zassert_not_equal(-1, r, "close() failed on the client fd (%d)", errno);

	http_server_stop(&ctx);

	r = k_thread_join(&server_thread, K_FOREVER);
	zassert_equal(0, r, "k_thread_join() failed (%d)", r);

	r = close(server_fd);
	zassert_not_equal(-1, r, "close() failed on the server fd (%d)", errno);
}

ZTEST(framework_tests_json, test_json)
{
	test_json();
}

ZTEST_SUITE(framework_tests_json, NULL, NULL, NULL, NULL, NULL);
