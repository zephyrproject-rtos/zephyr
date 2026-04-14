/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_uart.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define PEER_BOOT_GRACE_PERIOD_MS 2000

struct transfer_data {
	uint32_t test_value;
};

ZBUS_PROXY_AGENT_UART_DEFINE(proxy_agent,
			     DEVICE_DT_GET(DT_PHANDLE(DT_PATH(zephyr_user), zbus_proxy_uart)));

ZBUS_CHAN_DEFINE(request_channel, struct transfer_data, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

ZBUS_PROXY_ADD_CHAN(proxy_agent, request_channel);

ZBUS_SHADOW_CHAN_DEFINE(response_channel, struct transfer_data, proxy_agent, NULL,
			ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

static void proxy_forwarder_listener_cb(const struct zbus_channel *chan)
{
	const struct transfer_data *data = zbus_chan_const_msg(chan);

	LOG_INF("Received message on channel %s. Test Value=%d", chan->name, data->test_value);
}

ZBUS_LISTENER_DEFINE(proxy_forwarder_listener, proxy_forwarder_listener_cb);
ZBUS_CHAN_ADD_OBS(response_channel, proxy_forwarder_listener, 0);

int main(void)
{
	struct transfer_data data = {.test_value = 1};

	LOG_INF("ZBUS Proxy agent UART Forwarder Sample Application");
	LOG_INF("Requesting on channel %s, listening on channel %s", request_channel.name,
		response_channel.name);
	LOG_INF("Waiting %d ms for peer startup before first publish",
		PEER_BOOT_GRACE_PERIOD_MS);
	k_sleep(K_MSEC(PEER_BOOT_GRACE_PERIOD_MS));

	while (1) {
		int ret = zbus_chan_pub(&request_channel, &data, K_MSEC(100));

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
