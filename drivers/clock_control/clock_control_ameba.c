/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Realtek Ameba Clock Control
 */

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/clock_control/ameba_clock_control.h>
#include <zephyr/drivers/clock_control.h>

#define DT_DRV_COMPAT realtek_ameba_rcc

#define AMEBA_RCC_NO_PARENT    0xFF
#define AMEBA_RCC_MUX_API_NULL NULL

/* clk div */
#define AMEBA_RCC_CKD_GRP0        0
#define AMEBA_RCC_CKD_GRP1        1
#define AMEBA_RCC_CKD_GRP_INVALID 2

typedef void (*clk_src_func)(uint32_t src);

struct ameba_clk_ctrl_reg {
	uint8_t parent;
	uint8_t div_reg: 2;   /* div group idx */
	uint8_t div_shift: 5; /* div bit offset in group */
	uint8_t div_mask;     /* div mask value */

	uint32_t cke; /* clock enable reg */
	uint32_t fen; /* function enable reg */
};

/* Rcc clock cke/fen reg array */
static const struct ameba_clk_ctrl_reg ameba_clk_ctrl_reg_array[AMEBA_CLK_MAX] = {
	AMEBA_CORE_PERIPHS};

/**
 * @brief Enable a clock controlled by the device
 *
 * On success, the clock is enabled and ready when this function
 * returns.
 *
 * @param dev Device structure whose driver controls the clock.
 * @param sub_system clock idx
 *
 * @return 0 on success, negative errno on failure.
 */
static int ameba_clock_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t clk_idx = (uint32_t)sub_system;
	const struct ameba_clk_ctrl_reg *phandle;

	ARG_UNUSED(dev);

	if (clk_idx >= AMEBA_CLK_MAX) {
		return -ENOTSUP;
	}

	/* Find the parents,and loop to enable all gating */
	do {
		if (clk_idx >= AMEBA_CLK_MAX) {
			break;
		}

		phandle = &(ameba_clk_ctrl_reg_array[clk_idx]);
		if (phandle == NULL) {
			return -EFAULT;
		}

		RCC_PeriphClockCmd(phandle->fen, phandle->cke, ENABLE);

		clk_idx = phandle->parent;
	} while (clk_idx != AMEBA_RCC_NO_PARENT);

	return 0;
}

/**
 * @brief Disable a clock controlled by the device
 *
 * @param dev Device structure whose driver controls the clock.
 * @param sub_system clock idx.
 *
 * @return 0 on success, negative errno on failure.
 */
static int ameba_clock_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t clk_idx = (uint32_t)sub_system;
	const struct ameba_clk_ctrl_reg *phandle;

	ARG_UNUSED(dev);

	if (clk_idx >= AMEBA_CLK_MAX) {
		return -ENOTSUP;
	}

	phandle = &(ameba_clk_ctrl_reg_array[clk_idx]);
	if (phandle == NULL) {
		return -EFAULT;
	}
	RCC_PeriphClockCmd(phandle->fen, phandle->cke, DISABLE);

	return 0;
}

/**
 * @brief Get clock status.
 *
 * @param dev Device.
 * @param sub_system clock idx.
 *
 * @return Status, if the clock is enabled, return on(2), else off(1).
 */
static enum clock_control_status ameba_clock_get_status(const struct device *dev,
							clock_control_subsys_t sub_system)
{
	uint32_t clk_idx = (uint32_t)sub_system;
	const struct ameba_clk_ctrl_reg *phandle = NULL;

	ARG_UNUSED(dev);

	if (clk_idx >= AMEBA_CLK_MAX) {
		return CLOCK_CONTROL_STATUS_OFF;
	}

	phandle = &(ameba_clk_ctrl_reg_array[clk_idx]);
	if (phandle == NULL) {
		return CLOCK_CONTROL_STATUS_OFF;
	}

	if (RCC_PeriphClockEnableChk(phandle->cke)) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

/**
 * @brief Initialize Ameba RCC Driver
 *
 * @param dev Device structure whose driver controls the clock
 *
 * @return 0 on success
 */
static int ameba_clock_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static const struct clock_control_driver_api ameba_clock_driver_api = {
	.on = ameba_clock_on,
	.off = ameba_clock_off,
	.get_status = ameba_clock_get_status,
};

DEVICE_DT_INST_DEFINE(0, &ameba_clock_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &ameba_clock_driver_api);
