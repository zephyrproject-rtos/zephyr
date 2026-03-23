/* Copyright (c) 2026 Silicon Signals Pvt. Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * GPU2D Zephyr driver for STM32 SoC family
 */

#define DT_DRV_COMPAT st_neochrom_gpu2d

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stm32_gpu2d, CONFIG_NEOCHROM_GPU2D_LOG_LEVEL);

struct stm32_gpu2d_cfg {
	struct stm32_pclken pclken_gpu2d;
	struct reset_dt_spec reset_gpu2d;
};

static int stm32_gpu2d_init(const struct device *dev)
{
	const struct stm32_gpu2d_cfg *cfg = dev->config;
	int ret;

	ret = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			       (clock_control_subsys_t)&cfg->pclken_gpu2d);
	if (ret < 0) {
		LOG_ERR("Failed to enable GPU2D clock, %d", ret);
		return ret;
	}

	if (!device_is_ready(cfg->reset_gpu2d.dev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->reset_gpu2d.dev);
		return -ENODEV;
	}

	ret = reset_line_toggle_dt(&cfg->reset_gpu2d);
	if (ret < 0) {
		LOG_ERR("Failed to reset GPU2D, %d", ret);
		return ret;
	}

	LOG_INF("GPU2D hardware enabled");

	return 0;
}

static const struct stm32_gpu2d_cfg stm32_gpu2d_cfg = {
	.pclken_gpu2d = STM32_DT_INST_CLOCK_INFO(0),
	.reset_gpu2d = RESET_DT_SPEC_INST_GET(0),
};

DEVICE_DT_INST_DEFINE(0,
		      stm32_gpu2d_init,
		      NULL,
		      NULL,
		      &stm32_gpu2d_cfg,
		      POST_KERNEL,
		      CONFIG_NEOCHROM_GPU2D_INIT_PRIORITY,
		      NULL);
