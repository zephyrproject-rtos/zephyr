/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>

static K_SEM_DEFINE(g_mbox_data_rx_sem, 0, 1);

static mbox_channel_id_t g_mbox_received_data;
static mbox_channel_id_t g_mbox_received_channel;

static void callback(const struct device *dev, mbox_channel_id_t channel_id, void *user_data,
		     struct mbox_msg *data)
{
	memcpy(&g_mbox_received_data, data->data, data->size);
	g_mbox_received_channel = channel_id;

	k_sem_give(&g_mbox_data_rx_sem);
}

int main(void)
{
	const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
	const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);
	struct mbox_msg msg = {0};
	uint32_t message = 0;

	printk("mbox_data Client demo started\n");

	const int max_transfer_size_bytes = mbox_mtu_get_dt(&tx_channel);
	/* Sample currently supports only transfer size up to 4 bytes */
	if ((max_transfer_size_bytes < 0) || (max_transfer_size_bytes > 4)) {
		printk("mbox_mtu_get() error\n");
		return 0;
	}

	if (mbox_register_callback_dt(&rx_channel, callback, NULL)) {
		printk("mbox_register_callback() error\n");
		return 0;
	}

	if (mbox_set_enabled_dt(&rx_channel, 1)) {
		printk("mbox_set_enable() error\n");
		return 0;
	}

	while (message < 100) {
		msg.data = &message;
		msg.size = max_transfer_size_bytes;

		printk("Client send (on channel %d) value: %d\n", tx_channel.channel_id, message);
		if (mbox_send_dt(&tx_channel, &msg) < 0) {
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
