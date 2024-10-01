/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/sys/printk.h>

#ifdef CONFIG_RX_ENABLED
static void callback(const struct device *dev, mbox_channel_id_t channel_id,
		     void *user_data, struct mbox_msg *data)
{
	printk("Pong (on channel %d)\n", channel_id);
}
#endif /* CONFIG_RX_ENABLED */

int main(void)
{
	int ret;

	printk("Hello from HOST - %s\n", CONFIG_BOARD_TARGET);

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

	printk("Maximum bytes of data in the TX message: %d\n", mbox_mtu_get_dt(&tx_channel));
	printk("Maximum TX channels: %d\n", mbox_max_channels_get_dt(&tx_channel));

	while (1) {
		k_sleep(K_MSEC(2000));

		printk("Ping (on channel %d)\n", tx_channel.channel_id);

		ret = mbox_send_dt(&tx_channel, NULL);
		if (ret < 0) {
			printk("Could not send (%d)\n", ret);
			return 0;
		}
	}
#endif /* CONFIG_TX_ENABLED */
	return 0;
}
