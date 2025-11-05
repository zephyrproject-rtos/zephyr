/*
 * Copyright 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <soc.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#define DT_DRV_COMPAT st_stm32n6_ramcfg

/* Read-only driver configuration */
struct axisram_stm32_cfg {
	/* RAMCFG instance. */
	RAMCFG_TypeDef *base;
	/* SRAM Clock configuration. */
	struct stm32_pclken pclken_axisram;
	/* RAMCFG Clock configuration. */
	struct stm32_pclken pclken_ramcfg;
};

static int axisram_stm32_init(const struct device *dev)
{
	const struct axisram_stm32_cfg *cfg = dev->config;
	RAMCFG_HandleTypeDef ramcfg = {0};
	/* enable clock for subsystem */
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t) &cfg->pclken_ramcfg) != 0) {
		return -EIO;
	}

	if (clock_control_on(clk, (clock_control_subsys_t) &cfg->pclken_axisram) != 0) {
		return -EIO;
	}

	ramcfg.Instance = cfg->base;
	HAL_RAMCFG_EnableAXISRAM(&ramcfg);

	return 0;
}

#define STM32N6_AXISRAM_INIT(idx)							\
	static const struct axisram_stm32_cfg axisram_stm32_cfg_##idx = {		\
		.base = (RAMCFG_TypeDef *)DT_INST_REG_ADDR(idx),			\
		.pclken_axisram = STM32_CLOCK_INFO_BY_NAME(idx, axisram),		\
		.pclken_ramcfg = STM32_CLOCK_INFO_BY_NAME(idx, ramcfg),			\
	};										\
											\
	DEVICE_DT_INST_DEFINE(idx, &axisram_stm32_init, NULL,				\
			      NULL, &axisram_stm32_cfg_##idx,				\
			      PRE_KERNEL_2, 0, NULL);

/**
 * On other series which have no RAMCFG, whether RAMs are enabled
 * or not can be controlled by changing their "status" in Device Tree.
 * To match this behavior on N6, we check manually during instantation
 * of RAMCFG nodes whether they have an enabled child (= RAM node) and
 * perform our own instantiation only if so thanks to COND_CODE.
 */
#define STM32N6_AXISRAM_MAYBE_INIT(idx)							\
	IF_ENABLED(DT_INST_CHILD_NUM_STATUS_OKAY(idx), (STM32N6_AXISRAM_INIT(idx)))

DT_INST_FOREACH_STATUS_OKAY(STM32N6_AXISRAM_MAYBE_INIT)
