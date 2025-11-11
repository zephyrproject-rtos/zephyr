/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/http/service.h>
#include <zephyr/ztest.h>

#define SERVER_IPV6_ADDR "::1"
#define SERVER_PORT 8080
#define TEST_BUF_SIZE 1200

static uint16_t test_http_service_port = SERVER_PORT;
HTTP_SERVICE_DEFINE(test_http_service, SERVER_IPV6_ADDR,
		    &test_http_service_port, 1, 10, NULL, NULL, NULL);

static const char static_resource_payload[] = LOREM_IPSUM_SHORT;
struct http_resource_detail_static static_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.static_data = static_resource_payload,
	.static_data_len = sizeof(static_resource_payload) - 1,
};
HTTP_RESOURCE_DEFINE(static_resource, test_http_service, "/static",
		     &static_resource_detail);

static uint8_t dynamic_buf[TEST_BUF_SIZE];
static size_t dynamic_len;

static int dynamic_cb(struct http_client_ctx *client, enum http_data_status status,
		      const struct http_request_ctx *request_ctx,
		      struct http_response_ctx *response_ctx, void *user_data)
{
	static size_t offset;

	if (status == HTTP_SERVER_DATA_ABORTED) {
		offset = 0;
		return 0;
	}

	switch (client->method) {
	case HTTP_GET:
		response_ctx->body = dynamic_buf;
		response_ctx->body_len = dynamic_len;
		response_ctx->final_chunk = true;
		break;
	case HTTP_POST:
		if (request_ctx->data_len + offset > sizeof(dynamic_buf)) {
			return -ENOMEM;
		}

		if (request_ctx->data_len > 0) {
			memcpy(dynamic_buf + offset, request_ctx->data,
			       request_ctx->data_len);
			offset += request_ctx->data_len;
		}

		if (status == HTTP_SERVER_DATA_FINAL) {
			/* All data received, reset progress. */
			dynamic_len = offset;
			offset = 0;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

struct http_resource_detail_dynamic dynamic_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods =
			BIT(HTTP_GET) | BIT(HTTP_POST),
	},
	.cb = dynamic_cb,
	.user_data = NULL,
};
HTTP_RESOURCE_DEFINE(dynamic_resource, test_http_service, "/dynamic",
		     &dynamic_resource_detail);

struct test_ctx {
	uint8_t *buf;
	size_t buflen;
	size_t offset;
	uint16_t status;
	bool abort;
	bool final;
};

static int response_cb(struct http_response *rsp,
		       enum http_final_call final_data,
		       void *user_data)
{
	struct test_ctx *ctx = user_data;

	if (ctx == NULL) {
		return 0;
	}

	if (ctx->abort) {
		return -EBADMSG;
	}

	/* Final event should only be received once. */
	zassert_false(ctx->final);

	if (final_data == HTTP_DATA_FINAL) {
		ctx->final = true;
		ctx->status = rsp->http_status_code;
	}

	/* Copy response body */
	if (ctx->buf != NULL && rsp->body_frag_start != NULL &&
	    rsp->body_frag_len > 0) {
		zassert_true(ctx->offset + rsp->body_frag_len < ctx->buflen,
			     "Response too long");
		memcpy(ctx->buf + ctx->offset, rsp->body_frag_start,
		       rsp->body_frag_len);
		ctx->offset += rsp->body_frag_len;
	}

	return 0;
}

static int client_fd = -1;
static uint8_t recv_buf[64];
static uint8_t response_buf[TEST_BUF_SIZE];
static size_t resp_offset;

static void common_request_init(struct http_request *req)
{
	req->host = SERVER_IPV6_ADDR;
	req->protocol = "HTTP/1.1";
	req->response = response_cb;
	req->recv_buf = recv_buf;
	req->recv_buf_len = sizeof(recv_buf);
}

ZTEST(http_client, test_http1_client_get)
{
	struct http_request req = { 0 };
	struct test_ctx ctx = { 0 };
	int ret;

	common_request_init(&req);
	req.method = HTTP_GET;
	req.url = "/static";

	ctx.buf = response_buf;
	ctx.buflen = sizeof(response_buf);

	ret = http_client_req(client_fd, &req, -1, &ctx);
	zassert_true(ret > 0, "http_client_req() failed (%d)", ret);
	zassert_true(ctx.final, "No final event received");
	zassert_equal(ctx.status, 200, "Unexpected HTTP status code");
	zassert_equal(ctx.offset, strlen(static_resource_payload),
		      "Invalid payload length");
	zassert_mem_equal(response_buf, static_resource_payload, ctx.offset,
			  "Invalid payload");
}

static void test_http1_client_get_cb_common(struct http_parser_settings *http_cb)
{
	struct http_request req = { 0 };
	int ret;

	common_request_init(&req);
	req.method = HTTP_GET;
	req.url = "/static";
	req.http_cb = http_cb;

	ret = http_client_req(client_fd, &req, -1, NULL);
	zassert_true(ret > 0, "http_client_req() failed (%d)", ret);
}

int test_common_cb(struct http_parser *parser, const char *at, size_t length)
{
	zassert_true(resp_offset + length <= sizeof(response_buf),
		     "HTTP field too long");

	memcpy(response_buf + resp_offset, at, length);
	resp_offset += length;

	return 0;
}

ZTEST(http_client, test_http1_client_get_status_cb)
{
	struct http_parser_settings http_cb = {
		.on_status = test_common_cb,
	};

	test_http1_client_get_cb_common(&http_cb);

	zassert_str_equal(response_buf, "OK", "Wrong status");
}

ZTEST(http_client, test_http1_client_get_body_cb)
{
	struct http_parser_settings http_cb = {
		.on_body = test_common_cb,
	};

	test_http1_client_get_cb_common(&http_cb);

	zassert_str_equal(response_buf, LOREM_IPSUM_SHORT, "Wrong body payload");
}

static bool test_on_header_field;

int test_header_field_cb(struct http_parser *parser, const char *at, size_t length)
{
	if (resp_offset > 0 && !test_on_header_field) {
		response_buf[resp_offset++] = '\n';
	}

	zassert_true(resp_offset + length <= sizeof(response_buf),
		     "HTTP field too long");

	memcpy(response_buf + resp_offset, at, length);
	resp_offset += length;

	test_on_header_field = true;

	return 0;
}

int test_header_value_cb(struct http_parser *parser, const char *at, size_t length)
{
	if (test_on_header_field) {
		response_buf[resp_offset++] = ':';
		response_buf[resp_offset++] = ' ';

		test_on_header_field = false;
	}

	zassert_true(resp_offset + length <= sizeof(response_buf),
		     "HTTP field too long");

	memcpy(response_buf + resp_offset, at, length);
	resp_offset += length;

	return 0;
}

ZTEST(http_client, test_http1_client_get_headers_cb)
{
	struct http_parser_settings http_cb = {
		.on_header_field = test_header_field_cb,
		.on_header_value = test_header_value_cb,
	};
	uint8_t *header_field;

	test_http1_client_get_cb_common(&http_cb);

	header_field = strstr(response_buf, "Content-Type: text/html");
	zassert_not_null(header_field, "Header field not found");
	header_field = strstr(response_buf, "Content-Length: 445");
	zassert_not_null(header_field, "Header field not found");
}

ZTEST(http_client, test_http1_client_get_abort)
{
	struct http_request req = { 0 };
	struct test_ctx ctx = { 0 };
	int ret;

	common_request_init(&req);
	req.method = HTTP_GET;
	req.url = "/static";

	ctx.abort = true;

	ret = http_client_req(client_fd, &req, -1, &ctx);
	zassert_equal(ret, -ECONNABORTED,
		      "http_client_req() should've reported abort (%d)", ret);
}

ZTEST(http_client, test_http1_client_get_no_resource)
{
	struct http_request req = { 0 };
	struct test_ctx ctx = { 0 };
	int ret;

	common_request_init(&req);
	req.method = HTTP_GET;
	req.url = "/not_found";

	ret = http_client_req(client_fd, &req, -1, &ctx);
	zassert_true(ret > 0, "http_client_req() failed (%d)", ret);
	zassert_true(ctx.final, "No final event received");
	zassert_equal(ctx.status, 404, "Unexpected HTTP status code");
}

ZTEST(http_client, test_http1_client_post)
{
	struct http_request req = { 0 };
	struct test_ctx ctx = { 0 };
	int ret;

	common_request_init(&req);
	req.method = HTTP_POST;
	req.url = "/dynamic";
	req.payload = LOREM_IPSUM;
	req.payload_len = LOREM_IPSUM_STRLEN;

	ret = http_client_req(client_fd, &req, -1, &ctx);
	zassert_true(ret > 0, "http_client_req() failed (%d)", ret);
	zassert_true(ctx.final, "No final event received");
	zassert_equal(ctx.status, 200, "Unexpected HTTP status code");
	zassert_equal(dynamic_len, LOREM_IPSUM_STRLEN,
		      "Invalid payload length uploaded");
	zassert_mem_equal(dynamic_buf, LOREM_IPSUM, dynamic_len,
			  "Invalid payload uploaded");
}

static int test_payload_cb(int sock, struct http_request *req, void *user_data)
{
	int ret;

	ret = zsock_send(sock, LOREM_IPSUM_SHORT, LOREM_IPSUM_SHORT_STRLEN, 0);
	zassert_equal(ret, LOREM_IPSUM_SHORT_STRLEN,
		     "Failed to send payload (%d)", ret);

	return ret;
}

ZTEST(http_client, test_http1_client_post_payload_cb)
{
	static const char * const headers[] = {
		"Content-Length: " STRINGIFY(LOREM_IPSUM_SHORT_STRLEN) "\r\n",
		NULL
	};
	struct http_request req = { 0 };
	struct test_ctx ctx = { 0 };
	int ret;

	common_request_init(&req);
	req.method = HTTP_POST;
	req.url = "/dynamic";
	req.header_fields = headers;
	req.payload_cb = test_payload_cb;

	ret = http_client_req(client_fd, &req, -1, &ctx);
	zassert_true(ret > 0, "http_client_req() failed (%d)", ret);
	zassert_true(ctx.final, "No final event received");
	zassert_equal(ctx.status, 200, "Unexpected HTTP status code");
	zassert_equal(dynamic_len, LOREM_IPSUM_SHORT_STRLEN,
		      "Invalid payload length uploaded %d", dynamic_len);
	zassert_mem_equal(dynamic_buf, LOREM_IPSUM_SHORT, dynamic_len,
			  "Invalid payload uploaded %d", dynamic_len);
}

static void client_tests_before(void *fixture)
{
	struct sockaddr_in6 sa;
	int ret;

	ARG_UNUSED(fixture);

	dynamic_len = 0;
	resp_offset = 0;
	test_on_header_field = false;
	memset(recv_buf, 0, sizeof(recv_buf));
	memset(response_buf, 0, sizeof(response_buf));
	memset(dynamic_buf, 0, sizeof(dynamic_buf));

	ret = http_server_start();
	if (ret < 0) {
		printk("Failed to start the server\n");
		return;
	}

	ret = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if (ret < 0) {
		printk("Failed to create client socket (%d)\n", errno);
		return;
	}
	client_fd = ret;

	sa.sin6_family = AF_INET6;
	sa.sin6_port = htons(SERVER_PORT);

	ret = zsock_inet_pton(AF_INET6, SERVER_IPV6_ADDR, &sa.sin6_addr.s6_addr);
	if (ret != 1) {
		printk("inet_pton() failed to convert %s\n", SERVER_IPV6_ADDR);
		return;
	}

	ret = zsock_connect(client_fd, (struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0) {
		printk("Failed to connect (%d)\n", errno);
	}
}

static void client_tests_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (client_fd >= 0) {
		(void)zsock_close(client_fd);
		client_fd = -1;
	}

	(void)http_server_stop();

	k_yield();
}

ZTEST_SUITE(http_client, NULL, NULL, client_tests_before, client_tests_after, NULL);
