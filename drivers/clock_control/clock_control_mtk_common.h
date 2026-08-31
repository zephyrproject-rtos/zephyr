/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_MTK_COMMON_H
#define DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_MTK_COMMON_H

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

typedef struct {
	uint32_t set_offs;
	uint32_t clr_offs;
	uint32_t status_offs;
	uint32_t bit_mask;
} clk_map_t;

#define MAKE_CLK_MAP(_set, _clr, _status, _mask)                                                   \
	{.set_offs = (uint32_t)_set,                                                               \
	 .clr_offs = (uint32_t)_clr,                                                               \
	 .status_offs = (uint32_t)_status,                                                         \
	 .bit_mask = (uint32_t)_mask}

typedef struct {
	DEVICE_MMIO_ROM; /* Must be first */

	const clk_map_t *clk_map;

	size_t clk_map_size;

	bool enable_is_set; /* If 'true' the 'set_offs' will be used to enable and 'clr_offs'  */
			    /* to disable the clock. If 'false' the 'clr_offs' will be used to */
			    /* enable and 'set_offs' to disable the clock.                     */
} clk_ctrl_mtk_config_t;

typedef struct {
	DEVICE_MMIO_RAM; /* Must be first */
} clk_ctrl_mtk_data_t;

int clk_ctrl_mtk_clock_on(const struct device *dev, clock_control_subsys_t sys);
int clk_ctrl_mtk_clock_off(const struct device *dev, clock_control_subsys_t sys);
enum clock_control_status clk_ctrl_mtk_clock_status(const struct device *dev,
						    clock_control_subsys_t sys);

int clk_ctrl_mtk_init(const struct device *dev);

#endif /* DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_MTK_COMMON_H */
