/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

/*
 * Largest payload this sample is able to build. Drivers advertising an MTU
 * bigger than this are driven in signalling mode instead.
 */
#define MAX_MSG_SIZE 64

#if !defined(CONFIG_RX_ENABLED) && !defined(CONFIG_TX_ENABLED)
#error "At least one of CONFIG_RX_ENABLED or CONFIG_TX_ENABLED must be set"
#endif

#ifdef CONFIG_RX_ENABLED
static void callback(const struct device *dev, mbox_channel_id_t channel_id,
		     void *user_data, struct mbox_msg *data)
{
	if ((data != NULL) && (data->data != NULL) && (data->size > 0U)) {
		uint32_t counter = 0;

		memcpy(&counter, data->data, MIN(sizeof(counter), data->size));

		printk("Pong (on channel %d) counter %u in %u byte(s)\n", channel_id,
		       counter, (unsigned int)data->size);
	} else {
		printk("Pong (on channel %d)\n", channel_id);
	}
}
#endif /* CONFIG_RX_ENABLED */

#ifdef CONFIG_TX_ENABLED
static uint8_t tx_data[MAX_MSG_SIZE];
#endif /* CONFIG_TX_ENABLED */

int main(void)
{
	int ret;

	printk("Hello from REMOTE - %s\n", CONFIG_BOARD_TARGET);

#ifdef CONFIG_RX_ENABLED
	const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);

	printk("Maximum RX channels: %d\n", mbox_max_channels_get_dt(&rx_channel));

	ret = mbox_register_callback_dt(&rx_channel, callback, NULL);
	if (ret < 0) {
		printk("Could not register callback (%d)\n", ret);
		return 0;
	}

	ret = mbox_set_enabled_dt(&rx_channel, true);
	if (ret < 0) {
		printk("Could not enable RX channel %d (%d)\n", rx_channel.channel_id, ret);
		return 0;
	}
#endif /* CONFIG_RX_ENABLED */

#ifdef CONFIG_TX_ENABLED
	const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
	struct mbox_msg msg;
	struct mbox_msg *msg_ptr = NULL;
	uint32_t counter = 0;
	int mtu;

	mtu = mbox_mtu_get_dt(&tx_channel);

	printk("Maximum bytes of data in the TX message: %d\n", mtu);
	printk("Maximum TX channels: %d\n", mbox_max_channels_get_dt(&tx_channel));

	/*
	 * A non-zero MTU means the driver operates in data transfer mode, where
	 * mbox_send() must be given a message whose size matches the MTU. A zero
	 * (or unsupported) MTU means signalling mode, where the message must be
	 * NULL.
	 */
	if (mtu > (int)sizeof(tx_data)) {
		printk("MTU exceeds %u bytes, using signalling mode instead\n",
		       (unsigned int)sizeof(tx_data));
	} else if (mtu > 0) {
		msg.data = tx_data;
		msg.size = (size_t)mtu;
		msg_ptr = &msg;
	}

	while (1) {
#if defined(CONFIG_MULTITHREADING)
		k_sleep(K_MSEC(3000));
#else
		k_busy_wait(3000000);
#endif

		if (msg_ptr != NULL) {
			counter++;
			memset(tx_data, 0, msg.size);
			memcpy(tx_data, &counter, MIN(sizeof(counter), msg.size));
		}

		printk("Ping (on channel %d)\n", tx_channel.channel_id);

		ret = mbox_send_dt(&tx_channel, msg_ptr);
		if (ret < 0) {
			printk("Could not send (%d)\n", ret);
			return 0;
		}
	}
#endif /* CONFIG_TX_ENABLED */
	return 0;
}
