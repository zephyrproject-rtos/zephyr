/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

#include <zephyr/net/prometheus/formatter.h>
#include <zephyr/net/prometheus/collector.h>
#include <zephyr/net/prometheus/counter.h>
#include <zephyr/net/prometheus/gauge.h>
#include <zephyr/net/prometheus/histogram.h>
#include <zephyr/net/prometheus/summary.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

struct {

	struct prometheus_collector *collector;

	struct prometheus_counter *counter;

} prom_context;

int sendall(struct http_client_ctx *client, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = zsock_send(client->fd, buf, len, 0);

		if (out_len < 0) {
			return -errno;
		}

		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

#if defined(CONFIG_NET_SAMPLE_HTTP_SERVICE)
static uint16_t test_http_service_port = CONFIG_NET_SAMPLE_HTTP_SERVER_SERVICE_PORT;
HTTP_SERVICE_DEFINE(test_http_service, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &test_http_service_port, 1,
		    10, NULL);

static uint8_t recv_buffer[2048];

static int dyn_handler(struct http_client_ctx *client, enum http_data_status status,
		       uint8_t *buffer, size_t len, void *user_data)
{
#define MAX_TEMP_PRINT_LEN 32
	int rc;
	char hex_len[MAX_TEMP_PRINT_LEN];
	static char print_str[MAX_TEMP_PRINT_LEN];
	enum http_method method = client->method;
	static size_t processed;

	__ASSERT_NO_MSG(buffer != NULL);

	if (status == HTTP_SERVER_DATA_ABORTED) {
		LOG_DBG("Transaction aborted after %zd bytes.", processed);
		processed = 0;
		return 0;
	}

	processed += len;

	snprintf(print_str, sizeof(print_str), "%s received (%zd bytes)", http_method_str(method),
		 len);
	LOG_HEXDUMP_DBG(buffer, len, print_str);

	if (status == HTTP_SERVER_DATA_FINAL) {
		LOG_DBG("All data received (%zd bytes).", processed);
		processed = 0;
	}

	/* incrase counter per request */
	prometheus_counter_inc(prom_context.counter);

	(void)memset(recv_buffer, 0, sizeof(recv_buffer));

	prometheus_format_exposition(prom_context.collector, recv_buffer, sizeof(recv_buffer));

	/**
	 * The response status line indicates that the request was successful (200 OK).
	 * The response headers include Date, Server, Content-Type, and Transfer-Encoding.
	 * Transfer-Encoding is set to chunked, indicating that the response body is sent in a
	 * series of chunks. Each chunk begins with the hexadecimal length of the chunk, followed by
	 * a carriage return and line feed ("\r\n"), then the chunk data itself, and finally
	 * terminated by another carriage return and line feed ("\r\n"). The last chunk has a length
	 * of 0, indicating the end of the response body.
	 *
	 */

	rc = snprintk(hex_len, sizeof(hex_len), "%x\r\n", strlen(recv_buffer));
	if (rc < 0) {
		LOG_ERR("Cannot format data length (%d)", rc);
		return rc;
	}

	rc = sendall(client, hex_len, rc);
	if (rc < 0) {
		LOG_ERR("Cannot send data length to client (%d)", rc);
		return rc;
	}

	rc = sendall(client, recv_buffer, strlen(recv_buffer));
	if (rc < 0) {
		LOG_ERR("Cannot send data to client (%d)", rc);
		return rc;
	}

	rc = sendall(client, "\r\n", 2);
	if (rc < 0) {
		LOG_ERR("Cannot send cl/rf to client (%d)", rc);
		return rc;
	}

	return len;
}

struct http_resource_detail_dynamic dyn_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_type = "text/plain",
		},
	.cb = dyn_handler,
	.data_buffer = recv_buffer,
	.data_buffer_len = sizeof(recv_buffer),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(dyn_resource, test_http_service, "/metrics", &dyn_resource_detail);

#endif /* CONFIG_NET_SAMPLE_HTTP_SERVICE */

#if defined(CONFIG_NET_SAMPLE_HTTPS_SERVICE)
#include "certificate.h"

static const sec_tag_t sec_tag_list_verify_none[] = {
	HTTP_SERVER_CERTIFICATE_TAG,
#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
	PSK_TAG,
#endif
};

static uint16_t test_https_service_port = CONFIG_NET_SAMPLE_HTTPS_SERVER_SERVICE_PORT;
HTTPS_SERVICE_DEFINE(test_https_service, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &test_https_service_port,
		     1, 10, NULL, sec_tag_list_verify_none, sizeof(sec_tag_list_verify_none));

HTTP_RESOURCE_DEFINE(index_html_gz_resource_https, test_https_service, "/metrics",
		     &dyn_resource_detail);

#endif /* CONFIG_NET_SAMPLE_HTTPS_SERVICE */

static void setup_tls(void)
{
#if defined(CONFIG_NET_SAMPLE_HTTPS_SERVICE)
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	int err;

#if defined(CONFIG_NET_SAMPLE_CERTS_WITH_SC)
	err = tls_credential_add(HTTP_SERVER_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate, sizeof(ca_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register CA certificate: %d", err);
	}
#endif /* defined(CONFIG_NET_SAMPLE_CERTS_WITH_SC) */

	err = tls_credential_add(HTTP_SERVER_CERTIFICATE_TAG, TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 server_certificate, sizeof(server_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d", err);
	}

	err = tls_credential_add(HTTP_SERVER_CERTIFICATE_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
				 private_key, sizeof(private_key));
	if (err < 0) {
		LOG_ERR("Failed to register private key: %d", err);
	}

#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
	err = tls_credential_add(PSK_TAG, TLS_CREDENTIAL_PSK, psk, sizeof(psk));
	if (err < 0) {
		LOG_ERR("Failed to register PSK: %d", err);
	}

	err = tls_credential_add(PSK_TAG, TLS_CREDENTIAL_PSK_ID, psk_id, sizeof(psk_id) - 1);
	if (err < 0) {
		LOG_ERR("Failed to register PSK ID: %d", err);
	}
#endif /* defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED) */
#endif /* defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) */
#endif /* defined(CONFIG_NET_SAMPLE_HTTPS_SERVICE) */
}

struct prometheus_metric http_request_counter = {
	.type = PROMETHEUS_COUNTER,
	.name = "http_request_counter",
	.description = "HTTP request counter",
	.num_labels = 1,
	.labels = {{
		.key = "http_request",
		.value = "request_count",
	}},
};

PROMETHEUS_COUNTER_DEFINE(test_counter, &http_request_counter);

PROMETHEUS_COLLECTOR_DEFINE(test_collector);

int main(void)
{
	/* Create a mock collector with different types of metrics */
	prom_context.collector = &test_collector;

	prom_context.counter = &test_counter;
	prometheus_counter_inc(prom_context.counter);

	prometheus_collector_register_metric(prom_context.collector, prom_context.counter->base);

	setup_tls();
	http_server_start();

	return 0;
}
