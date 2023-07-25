/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_SUBSCRIBER_DEFINE(bridge_sub, 4);

ZBUS_CHAN_DEFINE(finish_chan,	    /* Name */
		 struct action_msg, /* Message type */

		 NULL,			     /* Validator */
NULL,		       /* User data */
		 ZBUS_OBSERVERS(bridge_sub), /* observers */
		 ZBUS_MSG_INIT(false)	     /* Initial value */
);

const static struct device *bridge_uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

static void bridge_tx_thread(void)
{
	LOG_DBG("Bridge Started");

	const struct zbus_channel *chan;

	while (1) {
		if (!zbus_sub_wait(&bridge_sub, &chan, K_FOREVER)) {
			if (!zbus_chan_claim(chan, K_MSEC(500))) {
				bool *user_data = (bool *)zbus_chan_user_data(chan);
				bool generated_by_the_bridge = *user_data;
				*user_data = false;

				zbus_chan_finish(chan);

				/* true here means the package was published by the bridge and must
				 * be discarded
				 */
				if (!generated_by_the_bridge) {
					LOG_DBG("Bridge send %s", zbus_chan_name(chan));

					uart_poll_out(bridge_uart, '$');

					for (int i = 0; i < strlen(zbus_chan_name(chan)); ++i) {
						uart_poll_out(bridge_uart, zbus_chan_name(chan)[i]);
					}

					uart_poll_out(bridge_uart, ',');

					uart_poll_out(bridge_uart, zbus_chan_msg_size(chan));

					for (int i = 0; i < zbus_chan_msg_size(chan); ++i) {
						uart_poll_out(bridge_uart,
							      ((uint8_t *)zbus_chan_msg(chan))[i]);
					}

					uart_poll_out(bridge_uart, '\r');

					uart_poll_out(bridge_uart, '\n');
				}
			}
		}
	}
}

K_THREAD_DEFINE(bridge_thread_tid, 2048, bridge_tx_thread, NULL, NULL, NULL, 1, 0, 500);
