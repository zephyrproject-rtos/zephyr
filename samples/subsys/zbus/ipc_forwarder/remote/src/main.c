/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <string.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/multidomain/zbus_multidomain.h>

#include "common.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

/* NOTE: Shared channels request_channel and response_channel are defined in common.h */

/* Set up proxy agent for ipc0 and add response channel to be forwarded */
#define IPC0_NODE DT_NODELABEL(ipc0)
ZBUS_PROXY_AGENT_DEFINE(ipc_proxy, ZBUS_MULTIDOMAIN_TYPE_IPC, IPC0_NODE);
ZBUS_PROXY_ADD_CHANNEL(ipc_proxy, response_channel);

int get_random_value(int min, int max)
{
	return min + (sys_rand32_get() % (max - min + 1));
}

/* Listen for requests on the request channel, and respond on the response channel */
static void ipc_forwarder_listener_cb(const struct zbus_channel *chan)
{
	int ret;
	const struct request_data *data = zbus_chan_const_msg(chan);
	struct response_data response = {
		.response_id = data->request_id,
		.value = get_random_value(data->min_value, data->max_value)};

	LOG_INF("Received message on channel %s", chan->name);
	LOG_INF("Request ID: %d, Min: %d, Max: %d", data->request_id, data->min_value,
		data->max_value);

	LOG_INF("Sending response: ID=%d, Value=%d", response.response_id, response.value);

	ret = zbus_chan_pub(&response_channel, &response, K_MSEC(100));
	if (ret < 0) {
		LOG_ERR("Failed to publish response on channel %s: %d", response_channel.name, ret);
	} else {
		LOG_INF("Response published on channel %s", response_channel.name);
	}
}

ZBUS_LISTENER_DEFINE(ipc_forwarder_listener, ipc_forwarder_listener_cb);
ZBUS_CHAN_ADD_OBS(request_channel, ipc_forwarder_listener, 0);

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

	return 0;
}
