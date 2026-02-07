/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_ipc.h>

#include "common.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

ZBUS_PROXY_AGENT_DEFINE(
	proxy_agent,                                    /* Proxy agent name */
	ZBUS_PROXY_AGENT_TYPE_IPC,                      /* Proxy agent type */
	DT_PROP(DT_PATH(zephyr_user), zbus_proxy_ipc),  /* Backend node */
	1,                                              /* Initial ack timeout ms */
	1,                                              /* Max transmit attempts */
	4                                               /* Max concurrent tracked messages */
)

/*
 * Channel definition
 * Channel request_channel is shadow channels, shadowing channel from domain A
 * Channel response_channel is origin channel, being forwarded to domain A
 */

ZBUS_SHADOW_CHAN_DEFINE(
	request_channel,      /* Chan name */
	struct request_data,  /* Message type */

	proxy_agent,          /* Proxy agent name */
	NULL,                 /* User data */
	ZBUS_OBSERVERS_EMPTY, /* Observers */
	ZBUS_MSG_INIT(0)      /* Initial value */
);

ZBUS_CHAN_DEFINE(
	response_channel,     /* Chan name */
	struct response_data, /* Message type */

	NULL,                 /* Validator function */
	NULL,                 /* User data */
	ZBUS_OBSERVERS_EMPTY, /* Observers */
	ZBUS_MSG_INIT(0)      /* Initial value */
);

ZBUS_PROXY_ADD_CHAN(proxy_agent, response_channel);

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

int main(void)
{
	LOG_INF("ZBUS Proxy agent IPC Forwarder Sample Application");
	LOG_INF("Listening for requests on channel %s, responding on channel %s",
		request_channel.name, response_channel.name);

	return 0;
}
