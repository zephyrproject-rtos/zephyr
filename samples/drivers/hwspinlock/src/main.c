/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/hwspinlock.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>

static K_SEM_DEFINE(rx_sem, 0, 1);

#define GATE_LOCK_TIMEOUT_MS 2000
#define GATE_LOCK_POLL_MS    1

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

	printk("Hello from HOST - %s\n", CONFIG_BOARD_TARGET);

	const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);
	const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
	struct hwspinlock_dt_spec gate =
			HWSPINLOCK_DT_SPEC_GET_BY_NAME(DT_PATH(hwspinlock_consumer), gate0);

	if (!device_is_ready(rx_channel.dev)) {
		printk("HOST: MBOX device not ready\n");
		return 0;
	}
	if (!device_is_ready(tx_channel.dev)) {
		printk("HOST: MBOX device not ready\n");
		return 0;
	}
	if (!device_is_ready(gate.dev)) {
		printk("HOST: HWSPINLOCK device not ready\n");
		return 0;
	}

	printk("Maximum RX channels: %d\n", mbox_max_channels_get_dt(&rx_channel));
	printk("Maximum TX channels: %d\n", mbox_max_channels_get_dt(&tx_channel));
	printk("HWSPINLOCK max id: %u\n", hw_spinlock_get_max_id_dt(&gate));

	ret = mbox_register_callback_dt(&rx_channel, callback, NULL);
	if (ret < 0) {
		printk("HOST: Could not register callback (%d)\n", ret);
		return 0;
	}

	ret = mbox_set_enabled_dt(&rx_channel, true);
	if (ret < 0) {
		printk("HOST: Could not enable RX channel %d (%d)\n", rx_channel.channel_id, ret);
		return 0;
	}

	printk("Maximum bytes of data in the TX message: %d\n", mbox_mtu_get_dt(&tx_channel));
	printk("Demo: HOST locks gate, sends ping while holding it; REMOTE blocks on same gate then replies.\n");

	while (1) {
		k_busy_wait(1500000);

		/* Discard any stale/late pong from a previous iteration. */
		k_sem_reset(&rx_sem);

		k_spinlock_key_t key;
		bool have_lock = false;
		int64_t start_ms = k_uptime_get();

		while (true) {
			ret = hw_spin_trylock_dt(&gate, &key);
			if (ret == 0) {
				have_lock = true;
				break;
			}

			if (ret != -EBUSY) {
				printk("HOST: trylock returned unexpected error %d\n", ret);
				break;
			}

			if ((k_uptime_get() - start_ms) > GATE_LOCK_TIMEOUT_MS) {
				printk("HOST: timeout waiting for gate (%d ms)\n",
					GATE_LOCK_TIMEOUT_MS);
				break;
			}

			k_sleep(K_MSEC(GATE_LOCK_POLL_MS));
		}

		if (!have_lock) {
			continue;
		}

		printk("HOST: locked gate id=%u, sending ping on channel %d\n",
		       gate.id, tx_channel.channel_id);

		ret = mbox_send_dt(&tx_channel, NULL);
		if (ret < 0) {
			printk("HOST: could not send ping (%d)\n", ret);
			hw_spin_unlock_dt(&gate, key);
			continue;
		}

		/* Hold the lock a bit so REMOTE will observe contention. */
		k_busy_wait(600000);
		hw_spin_unlock_dt(&gate, key);
		printk("HOST: unlocked gate, waiting for pong...\n");

		/* Wait until RX callback gives rx_sem */
		ret = k_sem_take(&rx_sem, K_SECONDS(2));
		if (ret < 0) {
			printk("HOST: timeout waiting for pong (%d)\n", ret);
		} else {
			printk("HOST: received pong\n");
		}
	}

	return 0;
}
