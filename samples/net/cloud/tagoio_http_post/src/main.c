/*
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tagoio_http_post, CONFIG_TAGOIO_HTTP_POST_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/random/random.h>
#include <stdio.h>

#include "wifi.h"
#include "sockets.h"

static struct tagoio_context ctx;

static void response_cb(struct http_response *rsp,
			enum http_final_call final_data,
			void *user_data)
{
	if (final_data == HTTP_DATA_MORE) {
		LOG_DBG("Partial data received (%zd bytes)", rsp->data_len);
	} else if (final_data == HTTP_DATA_FINAL) {
		LOG_DBG("All the data received (%zd bytes)", rsp->data_len);
	}

	LOG_DBG("Response status %s", rsp->http_status);
}

static int collect_data(void)
{
	#define lower 20000
	#define upper 100000
	#define base  1000.00f

	float temp;

	/* Generate a temperature between 20 and 100 celsius degree */
	temp = ((sys_rand32_get() % (upper - lower + 1)) + lower);
	temp /= base;

	(void)snprintf(ctx.payload, sizeof(ctx.payload),
		       "{\"variable\": \"temperature\","
		       "\"unit\": \"c\",\"value\": %f}",
		       (double)temp);

	/* LOG doesn't print float #18351 */
	LOG_INF("Temp: %d", (int) temp);

	return 0;
}

static void next_turn(void)
{
	if (collect_data() < 0) {
		LOG_INF("Error collecting data.");
		return;
	}

	if (tagoio_connect(&ctx) < 0) {
		LOG_INF("No connection available.");
		return;
	}

	if (tagoio_http_push(&ctx, response_cb) < 0) {
		LOG_INF("Error pushing data.");
		return;
	}
}

int main(void)
{
	LOG_INF("TagoIO IoT - HTTP Client - Temperature demo");

	wifi_connect();

	while (true) {
		next_turn();

		k_sleep(K_SECONDS(CONFIG_TAGOIO_HTTP_PUSH_INTERVAL));
	}
	return 0;
}
