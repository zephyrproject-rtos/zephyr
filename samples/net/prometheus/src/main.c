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

extern int init_stats(struct prometheus_counter *counter);

struct app_context {

	struct prometheus_collector *collector;

	struct prometheus_counter *counter;

} prom_context;

#if defined(CONFIG_NET_SAMPLE_HTTP_SERVICE)
static uint16_t test_http_service_port = CONFIG_NET_SAMPLE_HTTP_SERVER_SERVICE_PORT;
HTTP_SERVICE_DEFINE(test_http_service, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &test_http_service_port, 1,
		    10, NULL);

static int dyn_handler(struct http_client_ctx *client, enum http_data_status status,
		       uint8_t *buffer, size_t len, struct http_response_ctx *response_ctx,
		       void *user_data)
{
	int ret;
	static uint8_t prom_buffer[256];

	if (status == HTTP_SERVER_DATA_FINAL) {

		/* incrase counter per request */
		prometheus_counter_inc(prom_context.counter);

		/* clear buffer */
		(void)memset(prom_buffer, 0, sizeof(prom_buffer));

		/* format exposition data */
		ret = prometheus_format_exposition(prom_context.collector, prom_buffer,
						   sizeof(prom_buffer));
		if (ret < 0) {
			LOG_ERR("Cannot format exposition data (%d)", ret);
			return ret;
		}

		response_ctx->body = prom_buffer;
		response_ctx->body_len = strlen(prom_buffer);
		response_ctx->final_chunk = true;
	}

	return 0;
}

struct http_resource_detail_dynamic dyn_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_type = "text/plain",
		},
	.cb = dyn_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(dyn_resource, test_http_service, "/metrics", &dyn_resource_detail);

#endif /* CONFIG_NET_SAMPLE_HTTP_SERVICE */

#if defined(CONFIG_NET_SAMPLE_HTTPS_SERVICE)
#include "certificate.h"

const sec_tag_t sec_tag_list_verify_none[] = {
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

PROMETHEUS_COUNTER_DEFINE(http_request_counter, "HTTP request counter",
			  ({ .key = "http_request", .value = "request_count" }), NULL);

PROMETHEUS_COLLECTOR_DEFINE(test_collector);

int main(void)
{
	/* Create a mock collector with different types of metrics */
	prom_context.collector = &test_collector;

	prom_context.counter = &http_request_counter;
	prometheus_counter_inc(prom_context.counter);

	prometheus_collector_register_metric(prom_context.collector, &prom_context.counter->base);

#if defined(CONFIG_NET_STATISTICS_VIA_PROMETHEUS)
	(void)init_stats(prom_context.counter);
#endif

	setup_tls();

	http_server_start();

	return 0;
}
