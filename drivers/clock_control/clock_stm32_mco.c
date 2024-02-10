/*
 * Copyright (c) 2024 Benjamin Björnsson <benjamin.bjornsson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_mco_clock

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <stdint.h>

struct clock_control_stm32_mco_config {
	uint32_t base;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	const struct pinctrl_dev_config *pcfg;
};

static int clock_control_stm32_mco_init(const struct device *dev)
{
	const struct clock_control_stm32_mco_config *config = dev->config;

	for (int i = 0; i < config->pclk_len; i++) {
		sys_clear_bits(config->base + STM32_CLOCK_REG_GET(config->pclken->enr),
			       STM32_CLOCK_MASK_GET(config->pclken[i].enr)
				       << STM32_CLOCK_SHIFT_GET(config->pclken[i].enr));
		sys_set_bits(config->base + STM32_CLOCK_REG_GET(config->pclken->enr),
			     STM32_CLOCK_VAL_GET(config->pclken[i].enr)
				     << STM32_CLOCK_SHIFT_GET(config->pclken[i].enr));
	}

	return 0;
}

#define STM32_MCO_INIT(inst)								\
	PINCTRL_DT_INST_DEFINE(inst);							\
											\
	static const struct stm32_pclken pclken_##inst[] = STM32_DT_INST_CLOCKS(inst);	\
											\
	static struct clock_control_stm32_mco_config clk_cfg_##inst = {			\
		.base = DT_REG_ADDR(DT_INST_CLOCKS_CTLR(inst)),				\
		.pclken = pclken_##inst,						\
		.pclk_len = DT_INST_NUM_CLOCKS(inst),					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, &clock_control_stm32_mco_init,			\
			NULL, NULL, &clk_cfg_##inst,					\
			PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,		\
			NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_MCO_INIT)

#define GET_DEV(node_id) DEVICE_DT_GET(node_id),

static int stm32_mco_pinctrl_init(void)
{
	const struct device *dev_list[] = {DT_FOREACH_STATUS_OKAY(st_stm32_mco_clock, GET_DEV)};
	int list_len = ARRAY_SIZE(dev_list);

	for (int i = 0; i < list_len; i++) {
		const struct clock_control_stm32_mco_config *config = dev_list[i]->config;
		int res = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (res < 0) {
			return res;
		}
	}

	return 0;
}

/* Need to be initialised after GPIO driver */
SYS_INIT(stm32_mco_pinctrl_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
