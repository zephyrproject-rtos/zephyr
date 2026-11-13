/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* End-to-end tests against the Zephyr HTTP server over loopback. */

#include <strings.h>

#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/ztest.h>

#include "common.h"

#define SERVER_ADDR "::1"
#define SERVER_PORT 8080
#define SERVER_HOST "zephyr.local"

#define BODY_BUF_SIZE (24 * 1024)
#define LARGE_BODY_SIZE (20 * 1024)
#define LARGE_BODY_CHUNK 1000
#define UPLOAD_SIZE 2048
#define UPLOAD_PATTERN_SIZE 1500
#define REQ_HEADERS_MAX 4

static uint16_t test_http_service_port = SERVER_PORT;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#define CA_CERTIFICATE_TAG 1
#define SERVER_CERTIFICATE_TAG 2

static const unsigned char ca[] = {
#include "ca.inc"
};

static const unsigned char server_cert[] = {
#include "server.inc"
};

static const unsigned char server_privkey[] = {
#include "server_privkey.inc"
};

static const sec_tag_t server_tag_list[] = {
	CA_CERTIFICATE_TAG,
	SERVER_CERTIFICATE_TAG,
};

static const sec_tag_t client_tag_list[] = {
	CA_CERTIFICATE_TAG,
};

HTTPS_SERVICE_DEFINE(test_http_service, SERVER_ADDR, &test_http_service_port, 1, 10, NULL, NULL,
		     NULL, server_tag_list, sizeof(server_tag_list));
#else
HTTP_SERVICE_DEFINE(test_http_service, SERVER_ADDR, &test_http_service_port, 1, 10, NULL, NULL,
		    NULL);
#endif

/* Static resource */

static const char static_payload[] = LOREM_IPSUM_SHORT;

static struct http_resource_detail_static static_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_type = "text/plain",
	},
	.static_data = static_payload,
	.static_data_len = sizeof(static_payload) - 1,
};

HTTP_RESOURCE_DEFINE(static_resource, test_http_service, "/static", &static_detail);

/* Dynamic resource, driven by per-test settings */

static struct {
	uint16_t status;
	const uint8_t *body;
	size_t body_len;
	size_t chunk;
	const struct http_header *headers;
	size_t header_count;
	uint8_t upload[UPLOAD_SIZE];
	size_t upload_len;
	struct test_header req_headers[REQ_HEADERS_MAX];
	size_t req_header_count;
	size_t offset;
} dyn;

HTTP_SERVER_REGISTER_HEADER_CAPTURE(capture_test_header, "Test-Header");
HTTP_SERVER_REGISTER_HEADER_CAPTURE(capture_test_header2, "Test-Header2");

static void dyn_reset(void)
{
	memset(&dyn, 0, sizeof(dyn));
	dyn.status = HTTP_200_OK;
}

static const char *dyn_request_header(const char *name)
{
	for (size_t i = 0; i < dyn.req_header_count; i++) {
		if (strcasecmp(dyn.req_headers[i].name, name) == 0) {
			return dyn.req_headers[i].value;
		}
	}

	return NULL;
}

static void dyn_capture_request_headers(const struct http_request_ctx *request_ctx)
{
	for (size_t i = 0; i < request_ctx->header_count; i++) {
		struct test_header *hdr;

		if (dyn.req_header_count >= REQ_HEADERS_MAX) {
			break;
		}

		hdr = &dyn.req_headers[dyn.req_header_count++];
		strncpy(hdr->name, request_ctx->headers[i].name, sizeof(hdr->name) - 1);
		strncpy(hdr->value, request_ctx->headers[i].value, sizeof(hdr->value) - 1);
	}
}

static int dynamic_cb(struct http_client_ctx *client, enum http_transaction_status status,
		      const struct http_request_ctx *request_ctx,
		      struct http_response_ctx *response_ctx, void *user_data)
{
	ARG_UNUSED(user_data);

	if (status == HTTP_SERVER_TRANSACTION_ABORTED ||
	    status == HTTP_SERVER_TRANSACTION_COMPLETE) {
		dyn.offset = 0;
		return 0;
	}

	if (request_ctx->header_count > 0) {
		dyn_capture_request_headers(request_ctx);
	}

	switch (client->method) {
	case HTTP_GET: {
		size_t remaining = dyn.body_len - dyn.offset;
		size_t len = (dyn.chunk > 0) ? MIN(dyn.chunk, remaining) : remaining;

		response_ctx->status = dyn.status;
		response_ctx->headers = dyn.headers;
		response_ctx->header_count = dyn.header_count;
		response_ctx->body = dyn.body + dyn.offset;
		response_ctx->body_len = len;
		dyn.offset += len;
		response_ctx->final_chunk = (dyn.offset >= dyn.body_len);
		break;
	}
	case HTTP_POST:
	case HTTP_PUT:
		if (dyn.offset + request_ctx->data_len > sizeof(dyn.upload)) {
			return -ENOMEM;
		}

		if (request_ctx->data_len > 0) {
			memcpy(dyn.upload + dyn.offset, request_ctx->data, request_ctx->data_len);
			dyn.offset += request_ctx->data_len;
		}

		if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
			dyn.upload_len = dyn.offset;
			dyn.offset = 0;
			response_ctx->status = dyn.status;
			response_ctx->headers = dyn.headers;
			response_ctx->header_count = dyn.header_count;
			response_ctx->final_chunk = true;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static struct http_resource_detail_dynamic dynamic_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST) | BIT(HTTP_PUT),
	},
	.cb = dynamic_cb,
};

HTTP_RESOURCE_DEFINE(dynamic_resource, test_http_service, "/dynamic", &dynamic_detail);

/* Client under test */

static struct http_client client;
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
static struct http_client_transport_tls_ctx transport_ctx;
#else
static struct http_client_transport_tcp_ctx transport_ctx;
#endif
static struct http_client_session_config session_cfg;
static struct test_response rsp;
static uint8_t body_buf[BODY_BUF_SIZE];
static uint8_t large_body[LARGE_BODY_SIZE];
static uint8_t upload_pattern[UPLOAD_PATTERN_SIZE];
static bool initialized;

static int client_init(void)
{
	memset(&transport_ctx, 0, sizeof(transport_ctx));

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	transport_ctx.config.sec_tag_list = client_tag_list;
	transport_ctx.config.sec_tag_count = ARRAY_SIZE(client_tag_list);
	transport_ctx.config.hostname = SERVER_HOST;

	return http_client_init_tls(&client, HTTP_CLIENT_VERSION_2, &transport_ctx);
#else
	return http_client_init_tcp(&client, HTTP_CLIENT_VERSION_2, &transport_ctx);
#endif
}

static void session_cfg_init(struct http_client_session_config *cfg, bool async)
{
	struct net_sockaddr_in6 *sa = (struct net_sockaddr_in6 *)&cfg->server_addr;
	int ret;

	memset(cfg, 0, sizeof(*cfg));
	sa->sin6_family = NET_AF_INET6;
	sa->sin6_port = net_htons(SERVER_PORT);
	ret = zsock_inet_pton(NET_AF_INET6, SERVER_ADDR, &sa->sin6_addr);
	zassert_equal(ret, 1, "inet_pton failed");
	cfg->addrlen = sizeof(*sa);
	cfg->host = SERVER_HOST;
	cfg->async = async;
}

static void client_connect(bool async)
{
	int ret;

	ret = client_init();
	zassert_ok(ret, "client init failed (%d)", ret);

	session_cfg_init(&session_cfg, async);

	ret = http_client_connect(&client, &session_cfg);
	zassert_ok(ret, "connect failed (%d)", ret);
}

static void ensure_initialized(void)
{
	int ret;

	if (initialized) {
		return;
	}

	ARG_UNUSED(ret);

	for (size_t i = 0; i < sizeof(large_body); i++) {
		large_body[i] = (uint8_t)(i * 7 + 1);
	}

	for (size_t i = 0; i < sizeof(upload_pattern); i++) {
		upload_pattern[i] = (uint8_t)(i * 3 + 5);
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	ret = tls_credential_add(CA_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, ca,
				 sizeof(ca));
	zassert_ok(ret, "failed to add CA certificate (%d)", ret);

	ret = tls_credential_add(SERVER_CERTIFICATE_TAG, TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
				 server_cert, sizeof(server_cert));
	zassert_ok(ret, "failed to add server certificate (%d)", ret);

	ret = tls_credential_add(SERVER_CERTIFICATE_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
				 server_privkey, sizeof(server_privkey));
	zassert_ok(ret, "failed to add server private key (%d)", ret);
#endif

	initialized = true;
}

static void common_before(bool async)
{
	int ret;

	ensure_initialized();
	dyn_reset();
	test_response_init(&rsp, body_buf, sizeof(body_buf));

	ret = http_server_start();
	zassert_ok(ret, "server start failed (%d)", ret);

	client_connect(async);
}

static void sync_before(void *fixture)
{
	ARG_UNUSED(fixture);

	common_before(false);
}

static void async_before(void *fixture)
{
	ARG_UNUSED(fixture);

	common_before(true);
}

static void common_after(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)http_client_disconnect(&client);
	(void)http_server_stop();

	k_yield();
}

static int do_request(struct http_client_request *req, enum http_method method, const char *url,
		      struct test_response *r)
{
	req->method = method;
	req->url = url;
	req->response_cb = test_response_cb;

	return http_client_send_request(&client, req, TEST_REQUEST_TIMEOUT_MS, r);
}

static int do_get(const char *url, struct test_response *r)
{
	struct http_client_request req = { 0 };

	return do_request(&req, HTTP_GET, url, r);
}

static void assert_complete(const struct test_response *r, uint16_t status)
{
	zassert_false(r->aborted, "transaction aborted, err %d", r->error);
	zassert_equal(r->final_count, 1, "final reported %d times", r->final_count);
	zassert_equal(r->status, status, "unexpected status %u", r->status);
	zassert_false(r->body_overflow, "body buffer too small");
}

/* Synchronous sessions */

ZTEST(http2_client, test_get_static)
{
	int ret;

	ret = do_get("/static", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	assert_complete(&rsp, HTTP_200_OK);
	zassert_equal(rsp.header_blocks, 1, "headers reported %d times", rsp.header_blocks);
	zassert_equal(rsp.body_len, sizeof(static_payload) - 1, "body length %zu",
		      rsp.body_len);
	zassert_mem_equal(rsp.body, static_payload, rsp.body_len, "body mismatch");
	zassert_false(rsp.content_len_present, "content length reported without header");
}

ZTEST(http2_client, test_get_dynamic_status)
{
	int ret;

	dyn.status = HTTP_404_NOT_FOUND;
	ret = do_get("/dynamic", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	assert_complete(&rsp, HTTP_404_NOT_FOUND);

	test_response_init(&rsp, body_buf, sizeof(body_buf));
	dyn.status = HTTP_500_INTERNAL_SERVER_ERROR;
	ret = do_get("/dynamic", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	assert_complete(&rsp, HTTP_500_INTERNAL_SERVER_ERROR);
}

ZTEST(http2_client, test_get_response_headers)
{
	static const char body[] = "dynamic body";
	static const struct http_header headers[] = {
		{ "test-header", "value1" },
		{ "content-length", "12" },
	};
	const char *value;
	int ret;

	dyn.body = body;
	dyn.body_len = sizeof(body) - 1;
	dyn.headers = headers;
	dyn.header_count = ARRAY_SIZE(headers);

	ret = do_get("/dynamic", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	assert_complete(&rsp, HTTP_200_OK);
	zassert_equal(rsp.header_blocks, 1, "headers reported %d times", rsp.header_blocks);

	value = test_response_header(&rsp, "test-header");
	zassert_not_null(value, "test-header missing");
	zassert_str_equal(value, "value1", "test-header value %s", value);

	zassert_true(rsp.content_len_present, "content length not reported");
	zassert_equal(rsp.content_len, 12, "content length %zu", rsp.content_len);
	zassert_equal(rsp.body_len, 12, "body length %zu", rsp.body_len);
	zassert_mem_equal(rsp.body, body, rsp.body_len, "body mismatch");
}

ZTEST(http2_client, test_method_not_allowed)
{
	struct http_client_request req = { 0 };
	int ret;

	ret = do_request(&req, HTTP_DELETE, "/static", &rsp);
	zassert_ok(ret, "DELETE failed (%d)", ret);
	assert_complete(&rsp, HTTP_405_METHOD_NOT_ALLOWED);
}

ZTEST(http2_client, test_put_payload)
{
	struct http_client_request req = { 0 };
	int ret;

	req.payload = upload_pattern;
	req.payload_len = sizeof(upload_pattern);

	ret = do_request(&req, HTTP_PUT, "/dynamic", &rsp);
	zassert_ok(ret, "PUT failed (%d)", ret);
	assert_complete(&rsp, HTTP_200_OK);
	zassert_equal(dyn.upload_len, sizeof(upload_pattern), "upload length %zu",
		      dyn.upload_len);
	zassert_mem_equal(dyn.upload, upload_pattern, dyn.upload_len, "upload mismatch");
}

static int payload_cb(size_t offset, const uint8_t **payload, size_t *len, bool *more,
		      void *user_data)
{
	size_t chunk = MIN(600, sizeof(upload_pattern) - offset);

	ARG_UNUSED(user_data);

	*payload = upload_pattern + offset;
	*len = chunk;
	*more = (offset + chunk < sizeof(upload_pattern));

	return 0;
}

ZTEST(http2_client, test_post_payload_cb)
{
	struct http_client_request req = { 0 };
	int ret;

	req.payload_cb = payload_cb;

	ret = do_request(&req, HTTP_POST, "/dynamic", &rsp);
	zassert_ok(ret, "POST failed (%d)", ret);
	assert_complete(&rsp, HTTP_200_OK);
	zassert_equal(dyn.upload_len, sizeof(upload_pattern), "upload length %zu",
		      dyn.upload_len);
	zassert_mem_equal(dyn.upload, upload_pattern, dyn.upload_len, "upload mismatch");
}

/* Reports the end of the payload with an empty final chunk. */
static int payload_cb_empty_final(size_t offset, const uint8_t **payload, size_t *len,
				  bool *more, void *user_data)
{
	size_t chunk = MIN(500, sizeof(upload_pattern) - offset);

	ARG_UNUSED(user_data);

	*payload = upload_pattern + offset;
	*len = chunk;
	*more = (chunk > 0);

	return 0;
}

ZTEST(http2_client, test_post_payload_cb_empty_final)
{
	struct http_client_request req = { 0 };
	int ret;

	req.payload_cb = payload_cb_empty_final;

	ret = do_request(&req, HTTP_POST, "/dynamic", &rsp);
	zassert_ok(ret, "POST failed (%d)", ret);
	assert_complete(&rsp, HTTP_200_OK);
	zassert_equal(dyn.upload_len, sizeof(upload_pattern), "upload length %zu",
		      dyn.upload_len);
	zassert_mem_equal(dyn.upload, upload_pattern, dyn.upload_len, "upload mismatch");
}

static int headers_cb(const struct http_client_header **headers, size_t *count, bool *more,
		      void *user_data)
{
	static const struct http_client_header extra[] = {
		{ "test-header2", "def" },
	};

	ARG_UNUSED(user_data);

	*headers = extra;
	*count = ARRAY_SIZE(extra);
	*more = false;

	return 0;
}

ZTEST(http2_client, test_request_headers)
{
	static const struct http_client_header fields[] = {
		{ "test-header", "abc" },
	};
	struct http_client_request req = { 0 };
	const char *value;
	int ret;

	req.header_fields = fields;
	req.headers_count = ARRAY_SIZE(fields);
	req.headers_cb = headers_cb;

	ret = do_request(&req, HTTP_GET, "/dynamic", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	assert_complete(&rsp, HTTP_200_OK);

	value = dyn_request_header("test-header");
	zassert_not_null(value, "test-header not received by the server");
	zassert_str_equal(value, "abc", "test-header value %s", value);

	value = dyn_request_header("test-header2");
	zassert_not_null(value, "test-header2 not received by the server");
	zassert_str_equal(value, "def", "test-header2 value %s", value);
}

ZTEST(http2_client, test_skip_response_headers)
{
	struct http_client_request req = { 0 };
	int ret;

	req.skip_response_headers = true;

	ret = do_request(&req, HTTP_GET, "/static", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	assert_complete(&rsp, HTTP_200_OK);
	zassert_equal(rsp.header_blocks, 0, "headers reported although skipped");
	zassert_equal(rsp.header_count, 0, "headers captured although skipped");
	zassert_equal(rsp.body_len, sizeof(static_payload) - 1, "body length %zu",
		      rsp.body_len);
}

ZTEST(http2_client, test_get_large_body)
{
	static const struct http_header headers[] = {
		{ "content-type", "application/octet-stream" },
	};
	int ret;

	dyn.body = large_body;
	dyn.body_len = sizeof(large_body);
	dyn.chunk = LARGE_BODY_CHUNK;
	dyn.headers = headers;
	dyn.header_count = ARRAY_SIZE(headers);

	ret = do_get("/dynamic", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	assert_complete(&rsp, HTTP_200_OK);
	zassert_equal(rsp.header_blocks, 1, "headers reported %d times", rsp.header_blocks);
	zassert_true(rsp.body_chunks > 1, "body delivered in one chunk");
	zassert_equal(rsp.body_len, sizeof(large_body), "body length %zu", rsp.body_len);
	zassert_mem_equal(rsp.body, large_body, rsp.body_len, "body mismatch");
}

ZTEST(http2_client, test_abort_from_callback)
{
	int ret;

	rsp.body_cb_ret = -ECANCELED;

	ret = do_get("/static", &rsp);
	zassert_equal(ret, -ECANCELED, "unexpected result %d", ret);
	zassert_true(rsp.aborted, "abort not reported");
	zassert_equal(rsp.error, -ECANCELED, "abort reason %d", rsp.error);
	zassert_equal(rsp.final_count, 0, "final reported after abort");
}

ZTEST(http2_client, test_request_on_closed_session)
{
	int ret;

	ret = http_client_disconnect(&client);
	zassert_ok(ret, "disconnect failed (%d)", ret);

	ret = do_get("/static", &rsp);
	zassert_equal(ret, -ENOTCONN, "unexpected result %d", ret);
	zassert_true(rsp.aborted, "abort not reported");
	zassert_equal(rsp.error, -ENOTCONN, "abort reason %d", rsp.error);
}

ZTEST(http2_client, test_reconnect)
{
	int ret;

	ret = do_get("/static", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	assert_complete(&rsp, HTTP_200_OK);

	ret = http_client_disconnect(&client);
	zassert_ok(ret, "disconnect failed (%d)", ret);

	ret = http_client_disconnect(&client);
	zassert_ok(ret, "second disconnect failed (%d)", ret);

	ret = http_client_connect(&client, &session_cfg);
	zassert_ok(ret, "reconnect failed (%d)", ret);

	ret = http_client_connect(&client, &session_cfg);
	zassert_equal(ret, -EALREADY, "connect on open session returned %d", ret);

	test_response_init(&rsp, body_buf, sizeof(body_buf));
	ret = do_get("/static", &rsp);
	zassert_ok(ret, "GET after reconnect failed (%d)", ret);
	assert_complete(&rsp, HTTP_200_OK);
}

ZTEST(http2_client, test_connect_invalid_addrlen)
{
	struct http_client_session_config cfg;
	int ret;

	ret = http_client_disconnect(&client);
	zassert_ok(ret, "disconnect failed (%d)", ret);

	session_cfg_init(&cfg, false);
	cfg.addrlen = 0;
	ret = http_client_connect(&client, &cfg);
	zassert_equal(ret, -EINVAL, "zero addrlen accepted (%d)", ret);

	cfg.addrlen = sizeof(cfg.server_addr) + 1;
	ret = http_client_connect(&client, &cfg);
	zassert_equal(ret, -EINVAL, "oversized addrlen accepted (%d)", ret);
}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
ZTEST(http2_client, test_tls_server_name_default)
{
	struct http_client_session_config cfg;
	int ret;

	ret = http_client_disconnect(&client);
	zassert_ok(ret, "disconnect failed (%d)", ret);

	/* With no TLS hostname configured, the session host is sent as the
	 * server name and the certificate is verified against it.
	 */
	memset(&transport_ctx, 0, sizeof(transport_ctx));
	transport_ctx.config.sec_tag_list = client_tag_list;
	transport_ctx.config.sec_tag_count = ARRAY_SIZE(client_tag_list);
	ret = http_client_init_tls(&client, HTTP_CLIENT_VERSION_2, &transport_ctx);
	zassert_ok(ret, "client init failed (%d)", ret);

	session_cfg_init(&cfg, false);
	ret = http_client_connect(&client, &cfg);
	zassert_ok(ret, "connect failed (%d)", ret);
	ret = do_get("/static", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	assert_complete(&rsp, HTTP_200_OK);

	ret = http_client_disconnect(&client);
	zassert_ok(ret, "disconnect failed (%d)", ret);

	cfg.host = "other.test";
	ret = http_client_connect(&client, &cfg);
	zassert_true(ret < 0, "certificate accepted for another host (%d)", ret);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);
}
#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */

ZTEST_SUITE(http2_client, NULL, NULL, sync_before, common_after, NULL);

/* Asynchronous sessions */

ZTEST(http2_client_async, test_async_get)
{
	struct http_client_request req = { 0 };
	int ret;

	ret = do_request(&req, HTTP_GET, "/static", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);

	ret = test_response_wait(&rsp, K_SECONDS(5));
	zassert_ok(ret, "response timed out");
	assert_complete(&rsp, HTTP_200_OK);
	zassert_equal(rsp.body_len, sizeof(static_payload) - 1, "body length %zu",
		      rsp.body_len);
	zassert_mem_equal(rsp.body, static_payload, rsp.body_len, "body mismatch");
}

ZTEST(http2_client_async, test_async_concurrent)
{
	static const char body[] = "dynamic body";
	static struct http_client_request reqs[3];
	static struct test_response rsps[3];
	static uint8_t bufs[3][2048];
	int ret;

	dyn.body = body;
	dyn.body_len = sizeof(body) - 1;

	for (int i = 0; i < 3; i++) {
		memset(&reqs[i], 0, sizeof(reqs[i]));
		test_response_init(&rsps[i], bufs[i], sizeof(bufs[i]));
	}

	ret = do_request(&reqs[0], HTTP_GET, "/static", &rsps[0]);
	zassert_ok(ret, "GET 1 failed (%d)", ret);
	ret = do_request(&reqs[1], HTTP_GET, "/dynamic", &rsps[1]);
	zassert_ok(ret, "GET 2 failed (%d)", ret);
	ret = do_request(&reqs[2], HTTP_GET, "/static", &rsps[2]);
	zassert_ok(ret, "GET 3 failed (%d)", ret);

	for (int i = 0; i < 3; i++) {
		ret = test_response_wait(&rsps[i], K_SECONDS(5));
		zassert_ok(ret, "response %d timed out", i);
		assert_complete(&rsps[i], HTTP_200_OK);
	}

	zassert_equal(rsps[0].body_len, sizeof(static_payload) - 1, "body 1 length");
	zassert_mem_equal(rsps[0].body, static_payload, rsps[0].body_len, "body 1 mismatch");
	zassert_equal(rsps[1].body_len, sizeof(body) - 1, "body 2 length");
	zassert_mem_equal(rsps[1].body, body, rsps[1].body_len, "body 2 mismatch");
	zassert_equal(rsps[2].body_len, sizeof(static_payload) - 1, "body 3 length");
	zassert_mem_equal(rsps[2].body, static_payload, rsps[2].body_len, "body 3 mismatch");
}

ZTEST(http2_client_async, test_async_abort_from_callback)
{
	struct http_client_request req = { 0 };
	int ret;

	rsp.body_cb_ret = -ECANCELED;

	ret = do_request(&req, HTTP_GET, "/static", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);

	ret = test_response_wait(&rsp, K_SECONDS(5));
	zassert_ok(ret, "abort timed out");
	zassert_true(rsp.aborted, "abort not reported");
	zassert_equal(rsp.error, -ECANCELED, "abort reason %d", rsp.error);
	zassert_equal(rsp.final_count, 0, "final reported after abort");
}

ZTEST_SUITE(http2_client_async, NULL, NULL, async_before, common_after, NULL);
