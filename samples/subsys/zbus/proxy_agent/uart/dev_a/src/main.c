/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_uart.h>

#include "common.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

ZBUS_PROXY_AGENT_DEFINE(
	proxy_agent,                                     /* Proxy agent name */
	ZBUS_PROXY_AGENT_TYPE_UART,                      /* Proxy agent type */
	DT_PROP(DT_PATH(zephyr_user), zbus_proxy_uart),  /* Backend node */
	10,                                              /* Initial ack timeout ms */
	5,                                               /* Maximum transmission attempts */
	4                                                /* Maximum concurrent tracked messages */
);

/*
 * Channel definition
 * Channel request_channel is origin channel, being forwarded to device B
 * Channel response_channel is shadow channels, shadowing channel from device B
 */
ZBUS_CHAN_DEFINE(
	request_channel,      /* Chan name */
	struct request_data,  /* Message type */

	NULL,                 /* Validator function */
	NULL,                 /* User data */
	ZBUS_OBSERVERS_EMPTY, /* Observers */
	ZBUS_MSG_INIT(0)      /* Initial value */
);

/* Add request channel to proxy agent forwarding */
ZBUS_PROXY_ADD_CHAN(proxy_agent, request_channel);

ZBUS_SHADOW_CHAN_DEFINE(
	response_channel,     /* Chan name */
	struct response_data, /* Message type */

	proxy_agent,          /* Proxy agent name */
	NULL,                 /* User data */
	ZBUS_OBSERVERS_EMPTY, /* Observers */
	ZBUS_MSG_INIT(0)      /* Initial value */
);

void uart_forwarder_listener_cb(const struct zbus_channel *chan)
{
	const struct response_data *data = zbus_chan_const_msg(chan);

	LOG_INF("Received message on channel %s", chan->name);
	LOG_INF("Response ID: %d, Value: %d", data->response_id, data->value);
}

ZBUS_LISTENER_DEFINE(uart_forwarder_listener, uart_forwarder_listener_cb);
ZBUS_CHAN_ADD_OBS(response_channel, uart_forwarder_listener, 0);

int main(void)
{
	LOG_INF("ZBUS Proxy agent UART Forwarder Sample Application");
	LOG_INF("Requesting on channel %s, listening on channel %s", request_channel.name,
		response_channel.name);

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
