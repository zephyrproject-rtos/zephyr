/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_ipc.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

/**
 * @brief Structure for the message data
 *
 * @note Must be kept in sync with the corresponding structure in the other domain to ensure correct
 * message parsing
 */
struct transfer_data {
	uint32_t test_value;
};

ZBUS_PROXY_AGENT_DEFINE(
	proxy_agent,                                     /* Proxy agent name */
	ZBUS_PROXY_AGENT_BACKEND_IPC,                    /* Proxy agent type */
	DT_PHANDLE(DT_PATH(zephyr_user), zbus_proxy_ipc) /* Backend node */
);

/**
 * Channel definitions
 * Channel request_channel is shadow channels, shadowing channel from domain A
 * Channel response_channel is origin channel, being forwarded to domain A
 */

ZBUS_SHADOW_CHAN_DEFINE(
	request_channel,      /* Chan name */
	struct transfer_data, /* Message type */

	proxy_agent,          /* Proxy agent name */
	NULL,                 /* User data */
	ZBUS_OBSERVERS_EMPTY, /* Observers */
	ZBUS_MSG_INIT(0)      /* Initial value */
);

ZBUS_CHAN_DEFINE(
	response_channel,     /* Chan name */
	struct transfer_data, /* Message type */

	NULL,                 /* Validator function */
	NULL,                 /* User data */
	ZBUS_OBSERVERS_EMPTY, /* Observers */
	ZBUS_MSG_INIT(0)      /* Initial value */
);

ZBUS_PROXY_ADD_CHAN(proxy_agent, response_channel);

/* Listen for requests on the request channel, and respond on the response channel */
static void ipc_forwarder_listener_cb(const struct zbus_channel *chan)
{
	int ret;
	const struct transfer_data *data = zbus_chan_const_msg(chan);
	struct transfer_data response = {
		/* Increment the received value for the response */
		.test_value = data->test_value + 1,
	};

	LOG_INF("Received message on channel %s. Test Value=%d", chan->name, data->test_value);

	LOG_INF("Sending response: Test Value=%d", response.test_value);

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
