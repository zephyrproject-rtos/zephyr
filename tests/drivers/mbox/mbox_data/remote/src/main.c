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

static uint32_t g_mbox_received_data;
static uint32_t g_mbox_received_channel;

#define TX_ID0 (2)
#define RX_ID0 (3)
#define TX_ID1 (0)
#define RX_ID1 (1)
#define TX_ID2 (3)
#define RX_ID2 (2)
#define TX_ID3 (1)
#define RX_ID3 (0)

#define CHANNELS_TO_TEST (4)
#define TX_CHANNEL_INDEX (0)
#define RX_CHANNEL_INDEX (1)
const static uint32_t TEST_CHANNELS[CHANNELS_TO_TEST][2] = {
	{TX_ID0, RX_ID0}, {TX_ID1, RX_ID1}, {TX_ID2, RX_ID2}, {TX_ID3, RX_ID3}};

static void callback(const struct device *dev, uint32_t channel, void *user_data,
		     struct mbox_msg *data)
{
	if (data != NULL) {
		memcpy(&g_mbox_received_data, data->data, data->size);
		g_mbox_received_channel = channel;
	}

	k_sem_give(&g_mbox_data_rx_sem);
}

int main(void)
{
	struct mbox_channel tx_channel;
	struct mbox_channel rx_channel;
	const struct device *dev;
	struct mbox_msg msg = {0};
	uint32_t message = 0;

	dev = DEVICE_DT_GET(DT_NODELABEL(mbox));

	const int max_transfer_size_bytes = mbox_mtu_get(dev);
	/* Sample currently supports only transfer size up to 4 bytes */
	if ((max_transfer_size_bytes <= 0) || (max_transfer_size_bytes > 4)) {
		printk("mbox_mtu_get() error\n");
		return 0;
	}

	for (int i_test_channel = 0; i_test_channel < CHANNELS_TO_TEST; i_test_channel++) {
		mbox_init_channel(&tx_channel, dev,
				  TEST_CHANNELS[i_test_channel][TX_CHANNEL_INDEX]);
		mbox_init_channel(&rx_channel, dev,
				  TEST_CHANNELS[i_test_channel][RX_CHANNEL_INDEX]);

		if (mbox_register_callback(&rx_channel, callback, NULL)) {
			printk("mbox_register_callback() error\n");
			return 0;
		}

		if (mbox_set_enabled(&rx_channel, 1)) {
			printk("mbox_set_enable() error\n");
			return 0;
		}

		int test_count = 0;

		while (test_count < 100) {
			test_count++;

			k_sem_take(&g_mbox_data_rx_sem, K_FOREVER);
			message = g_mbox_received_data;

			message++;

			msg.data = &message;
			msg.size = max_transfer_size_bytes;

			if (mbox_send(&tx_channel, &msg) < 0) {
				printk("mbox_send() error\n");
				return 0;
			}
		}

		/* Disable current rx channel after channel loop */
		mbox_set_enabled(&rx_channel, 0);
	}
}
