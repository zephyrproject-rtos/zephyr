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
static int dynamic_status;

static int dynamic_cb(struct http_client_ctx *client, enum http_transaction_status status,
		      const struct http_request_ctx *request_ctx,
		      struct http_response_ctx *response_ctx, void *user_data)
{
	static size_t offset;

	if (status == HTTP_SERVER_TRANSACTION_ABORTED ||
	    status == HTTP_SERVER_TRANSACTION_COMPLETE) {
		offset = 0;
		return 0;
	}

	switch (client->method) {
	case HTTP_GET:
		response_ctx->status = dynamic_status;
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

		if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
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
static int response_status_code;

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

	response_status_code = parser->status_code;
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

ZTEST(http_client, test_http1_client_get_body_cb_error_500)
{
	struct http_parser_settings http_cb = {
		.on_body = test_common_cb,
	};
	struct http_request req = { 0 };
	int ret;

	common_request_init(&req);
	req.method = HTTP_GET;
	req.url = "/dynamic";
	req.http_cb = &http_cb;

	dynamic_len = LOREM_IPSUM_STRLEN;
	memcpy(dynamic_buf, LOREM_IPSUM, LOREM_IPSUM_STRLEN);
	dynamic_status = 500;

	ret = http_client_req(client_fd, &req, -1, NULL);
	zassert_true(ret > 0, "http_client_req() failed (%d)", ret);
	zassert_str_equal(response_buf, LOREM_IPSUM, "Wrong body payload");
	zassert_equal(response_status_code, 500, "Unexpected HTTP status code");
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
	struct net_sockaddr_in6 sa;
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

	ret = zsock_socket(NET_AF_INET6, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (ret < 0) {
		printk("Failed to create client socket (%d)\n", errno);
		return;
	}
	client_fd = ret;

	sa.sin6_family = NET_AF_INET6;
	sa.sin6_port = net_htons(SERVER_PORT);

	ret = zsock_inet_pton(NET_AF_INET6, SERVER_IPV6_ADDR, &sa.sin6_addr.s6_addr);
	if (ret != 1) {
		printk("inet_pton() failed to convert %s\n", SERVER_IPV6_ADDR);
		return;
	}

	ret = zsock_connect(client_fd, (struct net_sockaddr *)&sa, sizeof(sa));
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

/* Verify that body_frag_len equals the decoded body length when the server
 * uses Transfer-Encoding: chunked.  The response is a single 16-byte chunk
 * followed by the terminal chunk, all delivered in one recv call so the
 * chunk framing bytes land in the same buffer as the body.
 */

#define CHUNKED_SERVER_PORT 8082
#define CHUNKED_BODY_SIZE   16

static const char chunked_te_response[] =
	"HTTP/1.1 206 Partial Content\r\n"
	"Content-Range: bytes 0-15/16\r\n"
	"Transfer-Encoding: chunked\r\n"
	"\r\n"
	"10\r\n"
	"AAAAAAAAAAAAAAAA"
	"\r\n"
	"0\r\n"
	"\r\n";

static K_THREAD_STACK_DEFINE(chunked_srv_stack, 1024);
static struct k_thread chunked_srv_thread;
static K_SEM_DEFINE(chunked_srv_ready, 0, 1);

static void chunked_srv_fn(void *p1, void *p2, void *p3)
{
	struct net_sockaddr_in6 sa = { 0 };
	char req_buf[256];
	int srv, conn, ret;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	srv = zsock_socket(NET_AF_INET6, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (srv < 0) {
		return;
	}

	sa.sin6_family = NET_AF_INET6;
	sa.sin6_port = net_htons(CHUNKED_SERVER_PORT);
	ret = zsock_inet_pton(NET_AF_INET6, SERVER_IPV6_ADDR, &sa.sin6_addr.s6_addr);
	if (ret != 1) {
		zsock_close(srv);
		return;
	}

	ret = zsock_bind(srv, (struct net_sockaddr *)&sa, sizeof(sa));
	if (ret < 0) {
		zsock_close(srv);
		return;
	}

	zsock_listen(srv, 1);
	k_sem_give(&chunked_srv_ready);

	conn = zsock_accept(srv, NULL, NULL);
	if (conn >= 0) {
		(void)zsock_recv(conn, req_buf, sizeof(req_buf) - 1, 0);
		(void)zsock_send(conn, chunked_te_response,
				 sizeof(chunked_te_response) - 1, 0);
		zsock_close(conn);
	}

	zsock_close(srv);
}

static int chunked_client_fd = -1;
static uint8_t chunked_recv_buf[256];
static uint8_t chunked_response_buf[256];

static void chunked_tests_before(void *fixture)
{
	struct net_sockaddr_in6 sa = { 0 };
	int ret;

	ARG_UNUSED(fixture);

	memset(chunked_recv_buf, 0, sizeof(chunked_recv_buf));
	memset(chunked_response_buf, 0, sizeof(chunked_response_buf));

	k_thread_create(&chunked_srv_thread, chunked_srv_stack,
			K_THREAD_STACK_SIZEOF(chunked_srv_stack),
			chunked_srv_fn, NULL, NULL, NULL,
			K_PRIO_PREEMPT(7), 0, K_NO_WAIT);

	ret = k_sem_take(&chunked_srv_ready, K_SECONDS(5));
	zassert_equal(ret, 0, "chunked server did not start in time");

	chunked_client_fd = zsock_socket(NET_AF_INET6, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_true(chunked_client_fd >= 0, "client socket failed");

	sa.sin6_family = NET_AF_INET6;
	sa.sin6_port = net_htons(CHUNKED_SERVER_PORT);
	ret = zsock_inet_pton(NET_AF_INET6, SERVER_IPV6_ADDR, &sa.sin6_addr.s6_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	ret = zsock_connect(chunked_client_fd, (struct net_sockaddr *)&sa, sizeof(sa));
	zassert_equal(ret, 0, "connect failed (%d)", errno);
}

static void chunked_tests_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (chunked_client_fd >= 0) {
		(void)zsock_close(chunked_client_fd);
		chunked_client_fd = -1;
	}

	k_thread_join(&chunked_srv_thread, K_SECONDS(5));
}

ZTEST(http_client_chunked_te, test_body_frag_len_chunked_te)
{
	struct http_request req = { 0 };
	struct test_ctx ctx = { 0 };
	int ret;

	req.method = HTTP_GET;
	req.url = "/firmware/app.bin";
	req.host = SERVER_IPV6_ADDR;
	req.protocol = "HTTP/1.1";
	req.response = response_cb;
	req.recv_buf = chunked_recv_buf;
	req.recv_buf_len = sizeof(chunked_recv_buf);

	ctx.buf = chunked_response_buf;
	ctx.buflen = sizeof(chunked_response_buf);

	ret = http_client_req(chunked_client_fd, &req, -1, &ctx);
	zassert_true(ret > 0, "http_client_req() failed (%d)", ret);
	zassert_true(ctx.final, "No final event received");
	zassert_equal(ctx.status, 206, "Unexpected HTTP status code");

	zassert_equal(ctx.offset, CHUNKED_BODY_SIZE,
		      "body_frag_len overcount: got %zu, expected %d",
		      ctx.offset, CHUNKED_BODY_SIZE);
	zassert_mem_equal(chunked_response_buf, "AAAAAAAAAAAAAAAA",
			  CHUNKED_BODY_SIZE, "Body content mismatch");
}

ZTEST_SUITE(http_client_chunked_te, NULL, NULL,
	    chunked_tests_before, chunked_tests_after, NULL);
