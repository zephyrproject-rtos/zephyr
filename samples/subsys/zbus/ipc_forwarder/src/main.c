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

/* Set up proxy agent for ipc0 and add response channel to be forwarded */
#define IPC0_NODE DT_NODELABEL(ipc0)
ZBUS_PROXY_AGENT_DEFINE(ipc_proxy, ZBUS_MULTIDOMAIN_TYPE_IPC, IPC0_NODE);
ZBUS_PROXY_ADD_CHANNEL(ipc_proxy, request_channel);

/* Log response data coming from the response channel */
void ipc_forwarder_listener_cb(const struct zbus_channel *chan)
{
	const struct response_data *data = zbus_chan_const_msg(chan);

	LOG_INF("Received message on channel %s", chan->name);
	LOG_INF("Response ID: %d, Value: %d", data->response_id, data->value);
}

ZBUS_LISTENER_DEFINE(ipc_forwarder_listener, ipc_forwarder_listener_cb);
ZBUS_CHAN_ADD_OBS(response_channel, ipc_forwarder_listener, 0);

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
	LOG_INF("ZBUS Multidomain IPC Forwarder Sample Application");
	zbus_iterate_over_channels(print_channel_info);

	struct request_data data = {.request_id = 1, .min_value = -1, .max_value = 1};

	while (1) {
		int ret;

		ret = zbus_chan_pub(&request_channel, &data, K_MSEC(100));
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
