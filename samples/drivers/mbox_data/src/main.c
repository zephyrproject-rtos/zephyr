/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>

static K_SEM_DEFINE(g_mbox_data_rx_sem, 0, 1);

static uint32_t g_mbox_received_data;
static uint32_t g_mbox_received_channel;

#define TX_ID (3)
#define RX_ID (2)

static void callback(const struct device *dev, uint32_t channel, void *user_data,
		     struct mbox_msg *data)
{
	memcpy(&g_mbox_received_data, data->data, data->size);
	g_mbox_received_channel = channel;

	k_sem_give(&g_mbox_data_rx_sem);
}

int main(void)
{
	struct mbox_channel tx_channel;
	struct mbox_channel rx_channel;
	const struct device *dev;
	struct mbox_msg msg = {0};
	uint32_t message = 0;

	printk("mbox_data Client demo started\n");

	dev = DEVICE_DT_GET(DT_NODELABEL(mbox));

	mbox_init_channel(&tx_channel, dev, TX_ID);
	mbox_init_channel(&rx_channel, dev, RX_ID);

	if (mbox_register_callback(&rx_channel, callback, NULL)) {
		printk("mbox_register_callback() error\n");
		return 0;
	}

	if (mbox_set_enabled(&rx_channel, 1)) {
		printk("mbox_set_enable() error\n");
		return 0;
	}

	while (message < 100) {
		msg.data = &message;
		msg.size = 4;

		printk("Client send (on channel %d) value: %d\n", tx_channel.id, message);
		if (mbox_send(&tx_channel, &msg) < 0) {
			printk("mbox_send() error\n");
			return 0;
		}

		k_sem_take(&g_mbox_data_rx_sem, K_FOREVER);
		message = g_mbox_received_data;

		printk("Client received (on channel %d) value: %d\n", g_mbox_received_channel,
		       message);
		message++;
	}

	printk("mbox_data Client demo ended\n");
	return 0;
}
