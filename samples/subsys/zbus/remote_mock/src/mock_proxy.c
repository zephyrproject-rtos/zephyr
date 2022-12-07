/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <stdint.h>
#include <string.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/buf.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

const static struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

ZBUS_CHAN_DECLARE(sensor_data_chan);

ZBUS_CHAN_DEFINE(version_chan,	     /* Name */
		 struct version_msg, /* Message type */

		 NULL,		       /* Validator */
		 NULL,		       /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(.major = 0, .minor = 1,
			       .build = 1023) /* Initial value major 0, minor 1, build 1023 */
);

static bool start_measurement_from_bridge;
ZBUS_CHAN_DEFINE(start_measurement_chan, /* Name */
		 struct action_msg,	 /* Message type */

		 NULL,					    /* Validator */
		 &start_measurement_from_bridge,	    /* User data */
		 ZBUS_OBSERVERS(proxy_lis, peripheral_sub), /* observers */
		 ZBUS_MSG_INIT(false)			    /* Initial value */
);

static uint8_t encoder(const struct zbus_channel *chan)
{
	if (chan == &sensor_data_chan) {
		return 1;
	} else if (chan == &start_measurement_chan) {
		return 2;
	}
	return 0;
}

static const struct zbus_channel *decoder(uint8_t chan_idx)
{
	if (chan_idx == 1) {
		return &sensor_data_chan;
	} else if (chan_idx == 2) {
		return &start_measurement_chan;
	}
	return NULL;
}

static void proxy_callback(const struct zbus_channel *chan)
{
	bool *generated_by_the_bridge = zbus_chan_user_data(chan);

	if (*generated_by_the_bridge) {
		LOG_DBG("discard loopback event (channel %s)", zbus_chan_name(chan));

		*generated_by_the_bridge = false;
	} else {
		uart_poll_out(uart_dev, '$');

		uart_poll_out(uart_dev, encoder(chan));

		for (int i = 0; i < zbus_chan_msg_size(chan); ++i) {
			uart_poll_out(uart_dev, ((unsigned char *)zbus_chan_const_msg(chan))[i]);
		}

		uart_poll_out(uart_dev, '*');

		LOG_DBG("sending message to host (channel %s)", zbus_chan_name(chan));
	}
}

ZBUS_LISTENER_DEFINE(proxy_lis, proxy_callback);

void main(void)
{
	LOG_DBG("[Mock Proxy] Started.");
}

static void decode_sentence(struct net_buf_simple *rx_buf)
{
	if (rx_buf->len <= 1) {
		LOG_DBG("[Mock Proxy RX] Discard invalid sequence");
		/* '*' indicates the end of a sentence. Sometimes it is
		 * necessary to flush more than on ensure sending it from
		 * the python script. The code must discard when there is no
		 * other data at the buffer.
		 */
	} else {
		if ('$' == net_buf_simple_pull_u8(rx_buf)) {
			LOG_DBG("[Mock Proxy RX] Found sentence");

			const struct zbus_channel *chan = decoder(net_buf_simple_pull_u8(rx_buf));

			__ASSERT_NO_MSG(chan != NULL);

			if (!zbus_chan_claim(chan, K_MSEC(250))) {
				memcpy(zbus_chan_msg(chan),
				       net_buf_simple_pull_mem(rx_buf, zbus_chan_msg_size(chan)),
				       zbus_chan_msg_size(chan));

				bool *generated_by_the_bridge = zbus_chan_user_data(chan);
				*generated_by_the_bridge = true;

				zbus_chan_finish(chan);

				LOG_DBG("Publishing channel %s", zbus_chan_name(chan));

				zbus_chan_notify(chan, K_MSEC(500));
			}
		}
		net_buf_simple_init(rx_buf, 0);
	}
}

static void mock_proxy_rx_thread(void)
{
	LOG_DBG("[Mock Proxy RX] Started.");

	uint8_t byte;
	struct net_buf_simple *rx_buf = NET_BUF_SIMPLE(64);

	net_buf_simple_init(rx_buf, 0);

	while (1) {
		while (uart_poll_in(uart_dev, &byte) < 0) {
			/* Allow other thread/workqueue to work. */
			k_msleep(50);
		}
		if (byte == '*') {
			decode_sentence(rx_buf);
		} else {
			net_buf_simple_add_u8(rx_buf, byte);
		}
	}
}

K_THREAD_DEFINE(mock_proxy_rx_thread_tid, 2048, mock_proxy_rx_thread, NULL, NULL, NULL, 5, 0, 1500);
