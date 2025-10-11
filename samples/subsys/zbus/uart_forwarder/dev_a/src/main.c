/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/multidomain/zbus_multidomain.h>

#include "common.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

/* NOTE: Shared channels request_channel and response_channel are defined in common.h */

/* Set up proxy agent for UART1 and add response channel to be forwarded */
#define UART_NODE DT_ALIAS(zbus_uart)
ZBUS_PROXY_AGENT_DEFINE(uart1_proxy, ZBUS_MULTIDOMAIN_TYPE_UART, UART_NODE);
ZBUS_PROXY_ADD_CHANNEL(uart1_proxy, request_channel);

/* Log response data coming from the response channel */
void uart_forwarder_listener_cb(const struct zbus_channel *chan)
{
	const struct response_data *data = zbus_chan_const_msg(chan);

	LOG_INF("Received message on channel %s", chan->name);
	LOG_INF("Response ID: %d, Value: %d", data->response_id, data->value);
}

ZBUS_LISTENER_DEFINE(uart_forwarder_listener, uart_forwarder_listener_cb);
ZBUS_CHAN_ADD_OBS(response_channel, uart_forwarder_listener, 0);

bool print_channel_info(const struct zbus_channel *chan)
{
	if (!chan) {
		LOG_ERR("Channel is NULL");
		return false;
	}
	LOG_INF("Channel %s is a %s channel", chan->name,
		ZBUS_CHANNEL_IS_SHADOW(chan) ? "shadow" : "master");

	return true;
}

int main(void)
{
	LOG_INF("ZBUS Multidomain UART Forwarder Sample Application");
	zbus_iterate_over_channels(print_channel_info);

	struct request_data data = {.request_id = 1, .min_value = -1, .max_value = 1};

	while (1) {
		int ret = zbus_chan_pub(&request_channel, &data, K_MSEC(100));

		if (ret < 0) {
			LOG_ERR("Failed to publish on channel %s: %d", request_channel.name, ret);
		} else {
			LOG_INF("Published on channel %s. Request ID=%d, Min=%d, Max=%d",
				request_channel.name, data.request_id, data.min_value,
				data.max_value);
		}

		data.request_id++;
		data.min_value -= 1;
		data.max_value += 1;
		k_sleep(K_SECONDS(5));
	}
	return 0;
}
