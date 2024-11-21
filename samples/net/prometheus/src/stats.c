/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(main, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#include <zephyr/net/prometheus/formatter.h>
#include <zephyr/net/prometheus/collector.h>
#include <zephyr/net/prometheus/counter.h>
#include <zephyr/net/prometheus/gauge.h>
#include <zephyr/net/prometheus/histogram.h>
#include <zephyr/net/prometheus/summary.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct http_service_desc test_http_service;

static struct prometheus_counter *http_request_counter;
static struct prometheus_collector *stats_collector;
static struct prometheus_collector_walk_context walk_ctx;

static int stats_handler(struct http_client_ctx *client, enum http_data_status status,
			 uint8_t *buffer, size_t len, struct http_response_ctx *response_ctx,
			 void *user_data)
{
	int ret;
	static uint8_t prom_buffer[1024];

	if (status == HTTP_SERVER_DATA_FINAL) {

		/* incrase counter per request */
		prometheus_counter_inc(http_request_counter);

		(void)memset(prom_buffer, 0, sizeof(prom_buffer));

		ret = prometheus_collector_walk_metrics(user_data,
							prom_buffer,
							sizeof(prom_buffer));
		if (ret < 0 && ret != -EAGAIN) {
			LOG_ERR("Cannot format exposition data (%d)", ret);
			return ret;
		}

		response_ctx->body = prom_buffer;
		response_ctx->body_len = strlen(prom_buffer);

		if (ret == 0) {
			response_ctx->final_chunk = true;
			ret = prometheus_collector_walk_init(&walk_ctx, stats_collector);
			if (ret < 0) {
				LOG_ERR("Cannot initialize walk context (%d)", ret);
			}
		}
	}

	return 0;
}

struct http_resource_detail_dynamic stats_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_type = "text/plain",
	},
	.cb = stats_handler,
	.user_data = &walk_ctx,
};

HTTP_RESOURCE_DEFINE(stats_resource, test_http_service, "/statistics", &stats_resource_detail);

int init_stats(struct prometheus_counter *counter)
{
	/* Use a collector from default network interface */
	stats_collector = net_if_get_default()->collector;
	if (stats_collector == NULL) {
		LOG_ERR("Cannot get collector from default network interface");
		return -EINVAL;
	}

	(void)prometheus_collector_walk_init(&walk_ctx, stats_collector);

	http_request_counter = counter;

	return 0;
}
