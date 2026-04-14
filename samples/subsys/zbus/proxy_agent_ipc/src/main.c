/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
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

ZBUS_PROXY_AGENT_IPC_DEFINE(proxy_agent,
			    DEVICE_DT_GET(DT_PHANDLE(DT_PATH(zephyr_user), zbus_proxy_ipc)));

/**
 * Channel definitions
 * Channel request_channel is origin channel, being forwarded to domain B
 * Channel response_channel is shadow channels, shadowing channel from domain B
 */
ZBUS_CHAN_DEFINE(request_channel, struct transfer_data, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

/* Add request channel to proxy agent forwarding */
ZBUS_PROXY_ADD_CHAN(proxy_agent, request_channel);

ZBUS_SHADOW_CHAN_DEFINE(response_channel, struct transfer_data, proxy_agent, NULL,
			ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

void ipc_forwarder_listener_cb(const struct zbus_channel *chan)
{
	const struct transfer_data *data = zbus_chan_const_msg(chan);

	LOG_INF("Received message on channel %s. Test Value=%d", chan->name, data->test_value);
}

ZBUS_LISTENER_DEFINE(ipc_forwarder_listener, ipc_forwarder_listener_cb);
ZBUS_CHAN_ADD_OBS(response_channel, ipc_forwarder_listener, 0);

int main(void)
{
	LOG_INF("ZBUS Proxy agent IPC Forwarder Sample Application");
	LOG_INF("Requesting on channel %s, listening on channel %s", request_channel.name,
		response_channel.name);

	struct transfer_data data = {.test_value = 1};

	while (1) {
		int ret;

		ret = zbus_chan_pub(&request_channel, &data, K_MSEC(100));
		if (ret < 0) {
			LOG_ERR("Failed to publish on channel %s: %d", request_channel.name, ret);
		} else {
			LOG_INF("Published on channel %s. Test Value=%d", request_channel.name,
				data.test_value);
		}

		data.test_value++;
		k_sleep(K_SECONDS(10));
	}
	return 0;
}
