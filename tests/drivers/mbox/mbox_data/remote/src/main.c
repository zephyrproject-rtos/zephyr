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

static uint64_t g_mbox_received_data;
static uint32_t g_mbox_received_channel;

#define TX_CHANNEL_INDEX 0
#define RX_CHANNEL_INDEX 1

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_mbox_consumer)
#define MBOX_CONSUMER_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_mbox_consumer)
#elif DT_HAS_COMPAT_STATUS_OKAY(vnd_mbox_consumer)
#define MBOX_CONSUMER_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(vnd_mbox_consumer)
#else
#error "No zephyr,mbox-consumer or vnd,mbox-consumer node in the devicetree"
#endif

BUILD_ASSERT(DT_PROP_LEN(MBOX_CONSUMER_NODE, mboxes) % 2 == 0,
	     "mboxes must contain tx/rx channel pairs");

#define CHANNELS_TO_TEST (DT_PROP_LEN(MBOX_CONSUMER_NODE, mboxes) / 2)

#if CHANNELS_TO_TEST > 4
#error "The test suite covers at most 4 tx/rx channel pairs"
#endif

#define CHANNEL_ENTRY(_i)                                                                          \
	{                                                                                          \
		MBOX_DT_SPEC_GET(MBOX_CONSUMER_NODE, CONCAT(tx, _i)),                              \
		MBOX_DT_SPEC_GET(MBOX_CONSUMER_NODE, CONCAT(rx, _i)),                              \
	}

static const struct mbox_dt_spec channels[CHANNELS_TO_TEST][2] = {
	CHANNEL_ENTRY(0),
#if CHANNELS_TO_TEST >= 2
	CHANNEL_ENTRY(1),
#endif
#if CHANNELS_TO_TEST >= 3
	CHANNEL_ENTRY(2),
#endif
#if CHANNELS_TO_TEST >= 4
	CHANNEL_ENTRY(3),
#endif
};

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
	struct mbox_msg msg = {0};
	uint64_t message = 0;

	for (int i = 0; i < ARRAY_SIZE(channels); i++) {
		const struct mbox_dt_spec *tx_channel = &channels[i][TX_CHANNEL_INDEX];
		const struct mbox_dt_spec *rx_channel = &channels[i][RX_CHANNEL_INDEX];

		const int max_transfer_size_bytes = mbox_mtu_get_dt(tx_channel);
		/* Sample currently supports only transfer size up to 8 bytes */
		if ((max_transfer_size_bytes <= 0) || (max_transfer_size_bytes > 8)) {
			printk("mbox_mtu_get() error\n");
			return 0;
		}

		if (mbox_register_callback_dt(rx_channel, callback, NULL)) {
			printk("mbox_register_callback() error\n");
			return 0;
		}

		if (mbox_set_enabled_dt(rx_channel, 1)) {
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

			if (mbox_send_dt(tx_channel, &msg) < 0) {
				printk("mbox_send() error\n");
				return 0;
			}
		}

		/* Disable current rx channel after channel loop */
		mbox_set_enabled_dt(rx_channel, 0);
	}
}
