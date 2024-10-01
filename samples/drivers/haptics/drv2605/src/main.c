/*
 * Copyright (c) 2024 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/drivers/haptics/drv2605.h>
#include <zephyr/sys/util.h>
#include <sys/_stdint.h>

#define LOG_LEVEL 4

LOG_MODULE_REGISTER(main);

static struct drv2605_rom_data rom_data = {
	.library = DRV2605_LIBRARY_LRA,
	.brake_time = 0,
	.overdrive_time = 0,
	.sustain_neg_time = 0,
	.sustain_pos_time = 0,
	.trigger = DRV2605_MODE_INTERNAL_TRIGGER,
	.seq_regs[0] = 1,
	.seq_regs[1] = 10 | 0x80,
	.seq_regs[2] = 2,
	.seq_regs[3] = 10 | 0x80,
	.seq_regs[4] = 3,
	.seq_regs[5] = 10 | 0x80,
	.seq_regs[6] = 4,
};

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(haptic));
	union drv2605_config_data config_data = {};

	int ret;

	if (!dev) {
		LOG_ERR("DRV2605 device not found");
		return -ENODEV;
	} else if (!device_is_ready(dev)) {
		LOG_ERR("DRV2605 device %s is not ready", dev->name);
		return -EIO;
	}

	LOG_INF("Found DRV2605 device %s", dev->name);

	config_data.rom_data = &rom_data;

	ret = drv2605_haptic_config(dev, DRV2605_HAPTICS_SOURCE_ROM, &config_data);
	if (ret < 0) {
		LOG_ERR("Failed to configure ROM event: %d", ret);
		return ret;
	}

	ret = haptics_start_output(dev);
	if (ret < 0) {
		LOG_ERR("Failed to start ROM event: %d", ret);
		return ret;
	}

	return 0;
}
