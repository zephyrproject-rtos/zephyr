/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <hal/nrf_ipc.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/timer/nrf_rtc_timer.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

static void sync_callback(void)
{
	int32_t offset = z_nrf_rtc_timer_nrf53net_offset_get();

	__ASSERT(offset >= 0, "Synchronization should be completed");

	uint32_t timestamp = sys_clock_tick_get_32() + offset;
	uint32_t shared_cell = 0x20070000;
	uint32_t app_timestamp = *(volatile uint32_t *)shared_cell;

	LOG_INF("Local timestamp: %u, application core timestamp: %u",
		timestamp, app_timestamp);
}

static void mbox_callback(const struct device *dev, uint32_t channel,
			  void *user_data, struct mbox_msg *data)
{
	sync_callback();
}

static int mbox_init(void)
{
	const struct device *dev;
	struct mbox_channel channel;
	int err;

	dev = COND_CODE_1(CONFIG_MBOX, (DEVICE_DT_GET(DT_NODELABEL(mbox))), (NULL));
	if (dev == NULL) {
		return -ENODEV;
	}

	mbox_init_channel(&channel, dev, 2);

	err = mbox_register_callback(&channel, mbox_callback, NULL);
	if (err < 0) {
		return err;
	}

	return mbox_set_enabled(&channel, true);
}

int main(void)
{
	int err;

	LOG_INF("Synchronization using mbox driver");
	err = mbox_init();
	if (err < 0) {
		LOG_ERR("Failed to initialize sync RTC listener (err:%d)", err);
	}
	return 0;
}
