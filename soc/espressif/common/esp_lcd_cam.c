/*
 * Copyright (c) 2025 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/esp32_clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_lcd_cam, CONFIG_SOC_LOG_LEVEL);

#define ESP32_LCD_CAM_INST DT_COMPAT_GET_ANY_STATUS_OKAY(espressif_esp32_lcd_cam)

PINCTRL_DT_DEFINE(ESP32_LCD_CAM_INST);

static const struct pinctrl_dev_config *pcfg = PINCTRL_DT_DEV_CONFIG_GET(ESP32_LCD_CAM_INST);

static const struct device *clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(ESP32_LCD_CAM_INST));
static const clock_control_subsys_t clock_subsys =
	(clock_control_subsys_t)DT_CLOCKS_CELL(ESP32_LCD_CAM_INST, offset);

static int esp32_lcd_cam_init(void)
{
	int ret;

	ret = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("pinctrl setup failed (%d)", ret);
		return ret;
	}

	if (!device_is_ready(clock_dev)) {
		return -ENODEV;
	}

	return clock_control_on(clock_dev, clock_subsys);
}

SYS_INIT(esp32_lcd_cam_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
