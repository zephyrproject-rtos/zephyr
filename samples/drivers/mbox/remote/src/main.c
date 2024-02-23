/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/sys/printk.h>

#if !defined(CONFIG_RX_ENABLED) && !defined(CONFIG_TX_ENABLED)
#error "At least one of CONFIG_RX_ENABLED or CONFIG_TX_ENABLED must be set"
#endif

#ifdef CONFIG_RX_ENABLED
static void callback(const struct device *dev, uint32_t channel,
		     void *user_data, struct mbox_msg *data)
{
	printk("Pong (on channel %d)\n", channel);
}
#endif /* CONFIG_RX_ENABLED */

int main(void)
{
	int ret;
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(mbox));

	printk("Hello from REMOTE\n");

#ifdef CONFIG_RX_ENABLED
	const struct mbox_channel rx_channel = MBOX_DT_CHANNEL_GET(DT_PATH(mbox_consumer), rx);

	mbox_init_channel(&rx_channel, dev, CONFIG_RX_CHANNEL_ID);

	ret = mbox_register_callback(&rx_channel, callback, NULL);
	if (ret < 0) {
		printk("Could not register callback (%d)\n", ret);
		return 0;
	}

	ret = mbox_set_enabled(&rx_channel, true);
	if (ret < 0) {
		printk("Could not enable RX channel %d (%d)\n", rx_channel.id, ret);
		return 0;
	}
#endif /* CONFIG_RX_ENABLED */

#ifdef CONFIG_TX_ENABLED
	const struct mbox_channel tx_channel = MBOX_DT_CHANNEL_GET(DT_PATH(mbox_consumer), tx);

	mbox_init_channel(&tx_channel, dev, CONFIG_TX_CHANNEL_ID);

	while (1) {
		printk("Ping (on channel %d)\n", tx_channel.id);

		ret = mbox_send(&tx_channel, NULL);
		if (ret < 0) {
			printk("Could not send (%d)\n", ret);
			return 0;
		}

		k_sleep(K_MSEC(3000));
	}
#endif /* CONFIG_TX_ENABLED */

	return 0;
}
