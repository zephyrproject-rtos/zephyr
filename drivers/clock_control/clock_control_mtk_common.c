/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>

#include "clock_control_mtk_common.h"

int clk_ctrl_mtk_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	size_t idx = (size_t)sys;
	const clk_map_t *clk_map;
	const clk_ctrl_mtk_config_t *clk_ctrl_config = dev->config;

	if (idx >= clk_ctrl_config->clk_map_size) {
		return -EINVAL;
	}

	clk_map = &(clk_ctrl_config->clk_map[idx]);

	if (clk_ctrl_config->enable_is_set) {
		sys_write32(clk_map->bit_mask, (DEVICE_MMIO_GET(dev) + clk_map->set_offs));
	} else {
		sys_write32(clk_map->bit_mask, (DEVICE_MMIO_GET(dev) + clk_map->clr_offs));
	}

	return 0;
}

int clk_ctrl_mtk_clock_off(const struct device *dev, clock_control_subsys_t sys)
{
	size_t idx = (size_t)sys;
	const clk_map_t *clk_map;
	const clk_ctrl_mtk_config_t *clk_ctrl_config = dev->config;

	if (idx >= clk_ctrl_config->clk_map_size) {
		return -EINVAL;
	}

	clk_map = &(clk_ctrl_config->clk_map[idx]);

	if (clk_ctrl_config->enable_is_set) {
		sys_write32(clk_map->bit_mask, (DEVICE_MMIO_GET(dev) + clk_map->clr_offs));
	} else {
		sys_write32(clk_map->bit_mask, (DEVICE_MMIO_GET(dev) + clk_map->set_offs));
	}

	return 0;
}

enum clock_control_status clk_ctrl_mtk_clock_status(const struct device *dev,
						    clock_control_subsys_t sys)
{
	uint32_t clk_status;
	size_t idx = (size_t)sys;
	const clk_map_t *clk_map;
	const clk_ctrl_mtk_config_t *clk_ctrl_config = dev->config;

	if (idx >= clk_ctrl_config->clk_map_size) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	clk_map = &(clk_ctrl_config->clk_map[idx]);

	clk_status = sys_read32(DEVICE_MMIO_GET(dev) + clk_map->status_offs);

	if (clk_ctrl_config->enable_is_set) {
		if ((clk_status & clk_map->bit_mask) != 0) {
			return CLOCK_CONTROL_STATUS_ON;
		}
	} else {
		if ((clk_status & clk_map->bit_mask) == 0) {
			return CLOCK_CONTROL_STATUS_ON;
		}
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

int clk_ctrl_mtk_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	return 0;
}
