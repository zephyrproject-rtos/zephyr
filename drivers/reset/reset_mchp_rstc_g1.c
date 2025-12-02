/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file reset_mchp_rstc_g1.c
 * @brief Zephyr reset driver for Microchip G1 peripherals
 *
 * This file implements the driver for the Microchip RSTC g1 reset controller,
 * providing APIs to assert, deassert, toggle, and query the status of reset lines.
 *
 */

#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>

#define DT_DRV_COMPAT microchip_rstc_g1_reset

/* Maximum number of reset lines supported by the controller */
#define MCHP_RST_LINE_MAX 8

struct reset_mchp_config {
	rstc_registers_t *regs;
};

/**
 * @brief Get the status of a reset line.
 *
 * This function checks if the specified reset line is currently asserted.
 *
 * @param[in]  dev	Pointer to the device structure for the driver instance.
 * @param[in]  id	 Reset line ID (0-7).
 * @param[out] status Pointer to a variable to store the status (1 = asserted, 0 = not asserted).
 *
 * @retval 0		On success.
 * @retval -EINVAL  If the reset line ID is invalid.
 */
static int reset_mchp_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	int ret = 0;
	uint8_t rcause = 0;

	if (id >= MCHP_RST_LINE_MAX) {
		ret = -EINVAL;
	} else {
		rcause = (((const struct reset_mchp_config *)((dev)->config))->regs)->RSTC_RCAUSE;
		*status = (rcause & BIT(id)) ? 1 : 0;
	}

	return ret;
}

/**
 * @brief Assert (activate) a reset line.
 *
 * This function asserts the specified reset line.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] id  Reset line ID.
 *
 * @retval -ENOTSUP Operation not supported by hardware.
 */
static int reset_mchp_line_assert(const struct device *dev, uint32_t id)
{
	return -ENOTSUP;
}

/**
 * @brief Deassert (deactivate) a reset line.
 *
 * This function deasserts the specified reset line.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] id  Reset line ID.
 *
 * @retval -ENOTSUP Operation not supported by hardware.
 */
static int reset_mchp_line_deassert(const struct device *dev, uint32_t id)
{
	return -ENOTSUP;
}

/**
 * @brief Toggle a reset line (assert then deassert).
 *
 * This function asserts and then deasserts the specified reset line, with a short delay in between.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] id  Reset line ID.
 *
 * @retval -ENOTSUP Operation not supported by hardware.
 */
static int reset_mchp_line_toggle(const struct device *dev, uint32_t id)
{
	return -ENOTSUP;
}

static DEVICE_API(reset, reset_mchp_api) = {
	.status = reset_mchp_status,
	.line_assert = reset_mchp_line_assert,
	.line_deassert = reset_mchp_line_deassert,
	.line_toggle = reset_mchp_line_toggle,
};

/* Configuration instance for the Microchip RSTC g1 reset controller */
static const struct reset_mchp_config reset_mchp_config = {
	.regs = (rstc_registers_t *)DT_INST_REG_ADDR(0),
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Only one Microchip RSTC g1 instance is supported.");

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &reset_mchp_config, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &reset_mchp_api)
