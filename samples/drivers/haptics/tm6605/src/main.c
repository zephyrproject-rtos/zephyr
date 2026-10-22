/*
 * Copyright (c) 2026 Anuj Deshpande
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/drivers/haptics/tm6605.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const enum tm6605_effect demo_effects[] = {
	TM6605_EFFECT_SHARP_CLICK,
	TM6605_EFFECT_DOUBLE_CLICK,
	TM6605_EFFECT_STRONG_ALERT,
	TM6605_EFFECT_ALERT,
	TM6605_EFFECT_FLASH_STRIKE,
	TM6605_EFFECT_SOFT_NOISE,
};

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(haptic));
	int ret;

	if (!device_is_ready(dev)) {
		LOG_ERR_DEVICE_NOT_READY(dev);
		return -ENODEV;
	}

	LOG_INF("Found TM6605 device %s", dev->name);

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(demo_effects); i++) {
			ret = tm6605_select_effect(dev, demo_effects[i]);
			if (ret < 0) {
				LOG_ERR("Failed to select effect %d: %d", demo_effects[i], ret);
				return ret;
			}

			LOG_INF("Playing effect %d", demo_effects[i]);

			ret = haptics_start_output(dev);
			if (ret < 0) {
				LOG_ERR("Failed to start playback: %d", ret);
				return ret;
			}

			k_msleep(1500);
		}
	}

	return 0;
}
