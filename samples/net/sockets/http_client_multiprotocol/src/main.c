/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_http_client_sample, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/client_multiprotocol.h>

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)

#define CA_CERTIFICATE_TAG 1
#define TLS_PEER_HOSTNAME "localhost"
#define SERVER_PORT "4443"

/* The certificate of net-tools' https-server.pem. */
static const unsigned char ca_certificate[] = {
#include "https-cert.der.inc"
};

static const sec_tag_t sec_tag_list[] = {
	CA_CERTIFICATE_TAG,
};

#else
#define SERVER_PORT "8080"
#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */

#define SERVER_ADDR CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#define REQUEST_TIMEOUT_MS 5000
#define UPLOAD_CHUNK 128

/* One per request in flight, so that the callback can tell them apart and
 * the application can wait for them.
 */
struct transaction {
	const char *name;
	size_t body_len;
	struct k_sem done;
};

static uint8_t upload[1024];

static int response_cb(struct http_client *client,
		       const struct http_client_request *request,
		       struct http_client_response *response,
		       enum http_client_transaction_status status,
		       void *user_data)
{
	struct transaction *t = user_data;

	if (status == HTTP_CLIENT_TRANSACTION_ABORTED) {
		LOG_ERR("%s: aborted, err %d", t->name, response->error);
		k_sem_give(&t->done);
		return 0;
	}

	/* Each header block is reported in a callback carrying no body data.
	 * The fields are only valid for the duration of the callback.
	 */
	if (response->header_count > 0) {
		LOG_INF("%s: status %u, %zu headers", t->name, response->status_code,
			response->header_count);

		for (size_t i = 0; i < response->header_count; i++) {
			LOG_DBG("  %s: %s", response->headers[i].name, response->headers[i].value);
		}
	}

	t->body_len += response->body_len;

	if (status == HTTP_CLIENT_RESPONSE_DATA_FINAL) {
		LOG_INF("%s: complete, %zu body bytes", t->name, t->body_len);
		k_sem_give(&t->done);
	}

	return 0;
}

/* Hands out the upload in chunks, as the client asks for them. */
static int upload_cb(size_t offset, const uint8_t **payload, size_t *len, bool *more,
		     void *user_data)
{
	size_t chunk = MIN(UPLOAD_CHUNK, sizeof(upload) - offset);

	ARG_UNUSED(user_data);

	*payload = upload + offset;
	*len = chunk;
	*more = (offset + chunk < sizeof(upload));

	return 0;
}

static void transaction_init(struct transaction *t, const char *name)
{
	t->name = name;
	t->body_len = 0;
	k_sem_init(&t->done, 0, 1);
}

static int connect_session(struct http_client *client, bool async)
{
	struct http_client_session_config cfg = { 0 };
	struct zsock_addrinfo *res;
	int ret;

	ret = zsock_getaddrinfo(SERVER_ADDR, SERVER_PORT, NULL, &res);
	if (ret != 0) {
		LOG_ERR("Failed to resolve the server address, err %d", ret);
		return ret;
	}

	memcpy(&cfg.server_addr, res->ai_addr, res->ai_addrlen);
	cfg.addrlen = res->ai_addrlen;
	cfg.async = async;
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	cfg.host = TLS_PEER_HOSTNAME;
#endif

	zsock_freeaddrinfo(res);

	ret = http_client_connect(client, &cfg);
	if (ret != 0) {
		LOG_ERR("Failed to connect to the server, err %d", ret);
	}

	return ret;
}

/* A synchronous session: each request returns with its response. */
static int run_sync(struct http_client *client)
{
	struct http_client_header headers[] = { { "accept", "*/*" } };
	struct http_client_request request = { 0 };
	struct transaction t;
	int ret;

	ret = connect_session(client, false);
	if (ret != 0) {
		return ret;
	}

	transaction_init(&t, "sync GET /index.html");
	request.method = HTTP_GET;
	request.url = "/index.html";
	request.response_cb = response_cb;
	request.header_fields = headers;
	request.headers_count = ARRAY_SIZE(headers);

	ret = http_client_send_request(client, &request, REQUEST_TIMEOUT_MS, &t);
	if (ret != 0) {
		LOG_ERR("%s failed, err %d", t.name, ret);
	}

	/* An upload streamed through a callback: the length is not known up
	 * front, so no Content-Length header is sent.
	 */
	transaction_init(&t, "sync PUT /upload");
	memset(&request, 0, sizeof(request));
	request.method = HTTP_PUT;
	request.url = "/upload";
	request.response_cb = response_cb;
	request.content_type_value = "application/octet-stream";
	request.payload_cb = upload_cb;

	ret = http_client_send_request(client, &request, REQUEST_TIMEOUT_MS, &t);
	if (ret != 0) {
		LOG_ERR("%s failed, err %d", t.name, ret);
	}

	return http_client_disconnect(client);
}

/* An asynchronous session: the requests are sent back to back and their
 * responses arrive through the callback, interleaved on one connection.
 */
static int run_async(struct http_client *client)
{
	static const char *const urls[] = { "/index.html", "/", "/missing.html" };
	struct http_client_request requests[ARRAY_SIZE(urls)] = { 0 };
	struct transaction t[ARRAY_SIZE(urls)];
	int ret;

	ret = connect_session(client, true);
	if (ret != 0) {
		return ret;
	}

	/* The requests stay in use until their transactions complete. */
	for (size_t i = 0; i < ARRAY_SIZE(urls); i++) {
		transaction_init(&t[i], urls[i]);
		requests[i].method = HTTP_GET;
		requests[i].url = urls[i];
		requests[i].response_cb = response_cb;

		ret = http_client_send_request(client, &requests[i], REQUEST_TIMEOUT_MS, &t[i]);
		if (ret != 0) {
			LOG_ERR("async GET %s failed, err %d", urls[i], ret);
			k_sem_give(&t[i].done);
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(urls); i++) {
		k_sem_take(&t[i].done, K_FOREVER);
	}

	return http_client_disconnect(client);
}

int main(void)
{
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	struct http_client_transport_tls_ctx ctx = {
		.config = {
			.sec_tag_list = sec_tag_list,
			.sec_tag_count = ARRAY_SIZE(sec_tag_list),
			.hostname = TLS_PEER_HOSTNAME,
		},
	};
#else
	struct http_client_transport_tcp_ctx ctx = { 0 };
#endif
	/* Too large for the main thread's stack. */
	static struct http_client client;
	int ret;

	for (size_t i = 0; i < sizeof(upload); i++) {
		upload[i] = (uint8_t)i;
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	ret = tls_credential_add(CA_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate, sizeof(ca_certificate));
	if (ret < 0) {
		LOG_ERR("Failed to register the CA certificate, err %d", ret);
		return ret;
	}

	ret = http_client_init_tls(&client, HTTP_CLIENT_VERSION_2, &ctx);
#else
	ret = http_client_init_tcp(&client, HTTP_CLIENT_VERSION_2, &ctx);
#endif
	if (ret != 0) {
		LOG_ERR("Failed to initialize the HTTP client, err %d", ret);
		return ret;
	}

	ret = run_sync(&client);
	if (ret != 0) {
		return ret;
	}

	return run_async(&client);
}
