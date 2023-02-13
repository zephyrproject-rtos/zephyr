/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>

#define TX_ID (1)
#define RX_ID (0)

static void callback(const struct device *dev, uint32_t channel,
		     void *user_data, struct mbox_msg *data)
{
	printk("Pong (on channel %d)\n", channel);
}

void main(void)
{
	struct mbox_channel tx_channel;
	struct mbox_channel rx_channel;
	const struct device *dev;

	printk("Hello from APP\n");

	dev = DEVICE_DT_GET(DT_NODELABEL(mbox));

	mbox_init_channel(&tx_channel, dev, TX_ID);
	mbox_init_channel(&rx_channel, dev, RX_ID);

	if (mbox_register_callback(&rx_channel, callback, NULL)) {
		printk("mbox_register_callback() error\n");
		return;
	}

	if (mbox_set_enabled(&rx_channel, 1)) {
		printk("mbox_set_enable() error\n");
		return;
	}

	while (1) {
		k_sleep(K_MSEC(2000));

		printk("Ping (on channel %d)\n", tx_channel.id);

		if (mbox_send(&tx_channel, NULL) < 0) {
			printk("mbox_send() error\n");
			return;
		}
	}
}
