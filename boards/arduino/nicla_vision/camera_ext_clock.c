/*
 * Copyright 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(camera_ext_clock, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

int camera_ext_clock_enable(void)
{
	int ret;
	uint32_t rate;
	const struct device *cam_ext_clk_dev = DEVICE_DT_GET(DT_NODELABEL(pwmclock));

	if (!device_is_ready(cam_ext_clk_dev)) {
		LOG_ERR("Camera external clock source device is not ready!");
		return -ENODEV;
	}

	ret = clock_control_on(cam_ext_clk_dev, (clock_control_subsys_t)0);
	if (ret < 0) {
		LOG_ERR("Failed to enable camera external clock error: (%d)", ret);
		return ret;
	}

	ret = clock_control_get_rate(cam_ext_clk_dev, (clock_control_subsys_t)0, &rate);
	if (ret < 0) {
		LOG_ERR("Failed to get camera external clock rate, error: (%d)", ret);
		return ret;
	}

	LOG_INF("Camera external clock rate: (%u) Hz", rate);

	return 0;
}

SYS_INIT(camera_ext_clock_enable, POST_KERNEL, CONFIG_CLOCK_CONTROL_PWM_INIT_PRIORITY);
