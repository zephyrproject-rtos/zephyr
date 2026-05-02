/*
 * Copyright (c) 2022 Andrei-Edward Popa
 * Copyright (c) 2025 Dmitrii Sharshakov <d3dx12.xx@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_reset

#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>

#include <hardware/resets.h>

static int reset_rpi_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	*status = !!(resets_hw->reset & (1u << id));

	return 0;
}

static int reset_rpi_line_assert(const struct device *dev, uint32_t id)
{
	reset_block_num(id);

	return 0;
}

static int reset_rpi_line_deassert(const struct device *dev, uint32_t id)
{
	unreset_block_num_wait_blocking(id);

	return 0;
}

static int reset_rpi_line_toggle(const struct device *dev, uint32_t id)
{
	int ret;

	ret = reset_rpi_line_assert(dev, id);
	if (ret) {
		return ret;
	}

	return reset_rpi_line_deassert(dev, id);
}

static DEVICE_API(reset, reset_rpi_driver_api) = {
	.status = reset_rpi_status,
	.line_assert = reset_rpi_line_assert,
	.line_deassert = reset_rpi_line_deassert,
	.line_toggle = reset_rpi_line_toggle,
};

#define RPI_RESET_INIT(idx)							\
										\
	DEVICE_DT_INST_DEFINE(idx, NULL,				\
			      NULL, NULL,					\
			      NULL, PRE_KERNEL_1,		\
			      CONFIG_RESET_INIT_PRIORITY,			\
			      &reset_rpi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RPI_RESET_INIT);
