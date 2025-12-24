/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/hwspinlock.h>
#include <zephyr/sys/printk.h>

static K_SEM_DEFINE(rx_sem, 0, 1);

static void callback(const struct device *dev, mbox_channel_id_t channel_id,
		     void *user_data, struct mbox_msg *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(user_data);
	ARG_UNUSED(data);

	/* ISR context: keep it minimal, just signal the main loop. */
	k_sem_give(&rx_sem);
}

int main(void)
{
	int ret;

	printk("Hello from REMOTE - %s\n", CONFIG_BOARD_TARGET);

	const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);
	const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
	struct hwspinlock_dt_spec gate =
			HWSPINLOCK_DT_SPEC_GET_BY_NAME(DT_PATH(hwspinlock_consumer), gate0);

	if (!device_is_ready(rx_channel.dev)) {
		printk("REMOTE: MBOX device not ready\n");
		return 0;
	}
	if (!device_is_ready(tx_channel.dev)) {
		printk("REMOTE: MBOX device not ready\n");
		return 0;
	}
	if (!device_is_ready(gate.dev)) {
		printk("REMOTE: HWSPINLOCK device not ready\n");
		return 0;
	}

	printk("Maximum RX channels: %d\n", mbox_max_channels_get_dt(&rx_channel));
	printk("Maximum TX channels: %d\n", mbox_max_channels_get_dt(&tx_channel));
	printk("HWSPINLOCK max id: %u\n", hw_spinlock_get_max_id_dt(&gate));

	ret = mbox_register_callback_dt(&rx_channel, callback, NULL);
	if (ret < 0) {
		printk("REMOTE: Could not register callback (%d)\n", ret);
		return 0;
	}

	ret = mbox_set_enabled_dt(&rx_channel, true);
	if (ret < 0) {
		printk("REMOTE: Could not enable RX channel %d (%d)\n", rx_channel.channel_id, ret);
		return 0;
	}

	printk("Maximum bytes of data in the TX message: %d\n", mbox_mtu_get_dt(&tx_channel));

	while (1) {
		/* Wait for HOST ping */
		(void)k_sem_take(&rx_sem, K_FOREVER);
		printk("REMOTE: got ping (rx_sem), checking gate contention...\n");

		/* Demonstrate contention: trylock should fail while HOST still holds the gate. */
		k_spinlock_key_t key;

		ret = hw_spin_trylock_dt(&gate, &key);
		if (ret == 0) {
			/* Unexpected: trylock succeeded (HOST must have released early). */
			printk("REMOTE: trylock unexpectedly succeeded, releasing immediately\n");
			hw_spin_unlock_dt(&gate, key);
			continue;
		}

		printk("REMOTE: trylock returned %d (expected if HOST holds the lock)\n", ret);

		/* Now take the HW spinlock (blocks until HOST releases). */
		printk("REMOTE: blocking on hw_spin_lock_dt()...\n");
		key = hw_spin_lock_dt(&gate);
		printk("REMOTE: acquired gate id=%u\n", gate.id);

		printk("REMOTE: locked gate id=%u, sending pong on channel %d\n",
		       gate.id, tx_channel.channel_id);

		k_busy_wait(400000);
		hw_spin_unlock_dt(&gate, key);

		/* Signal HOST that we're done with the protected region. */
		(void)mbox_send_dt(&tx_channel, NULL);
	}

	return 0;
}
