/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_bus.h>
#include <zephyr/logging/log.h>

#include "can_mcan.h"

LOG_MODULE_REGISTER(can_stm32fd, CONFIG_CAN_LOG_LEVEL);

#if defined(CONFIG_CAN_STM32FD_CLOCK_SOURCE_HSE)
#define CAN_STM32FD_CLOCK_SOURCE LL_RCC_FDCAN_CLKSOURCE_HSE
#elif defined(CONFIG_CAN_STM32FD_CLOCK_SOURCE_PLL)
#define CAN_STM32FD_CLOCK_SOURCE LL_RCC_FDCAN_CLKSOURCE_PLL
#elif defined(CONFIG_CAN_STM32FD_CLOCK_SOURCE_PCLK1)
#define CAN_STM32FD_CLOCK_SOURCE LL_RCC_FDCAN_CLKSOURCE_PCLK1
#elif defined(CONFIG_CAN_STM32FD_CLOCK_SOURCE_PLL1Q)
#define CAN_STM32FD_CLOCK_SOURCE LL_RCC_FDCAN_CLKSOURCE_PLL1
#elif defined(CONFIG_CAN_STM32FD_CLOCK_SOURCE_PLL2P)
#define CAN_STM32FD_CLOCK_SOURCE LL_RCC_FDCAN_CLKSOURCE_PLL2
#else
#error "Unsupported FDCAN clock source"
#endif

#ifdef CONFIG_CAN_STM32FD_CLOCK_DIVISOR
#if CONFIG_CAN_STM32FD_CLOCK_DIVISOR != 1 && CONFIG_CAN_STM32FD_CLOCK_DIVISOR & 0x01
#error CAN_STM32FD_CLOCK_DIVISOR invalid. Allowed values are 1 or 2 * n, where n <= 15.
#else
#define CAN_STM32FD_CLOCK_DIVISOR CONFIG_CAN_STM32FD_CLOCK_DIVISOR
#endif /* CONFIG_CAN_STM32FD_CLOCK_DIVISOR */
#else
#define CAN_STM32FD_CLOCK_DIVISOR 1U
#endif /* CONFIG_CAN_STM32FD_CLOCK_DIVISOR*/

#define DT_DRV_COMPAT st_stm32_fdcan

struct can_stm32fd_config {
	struct stm32_pclken pclken;
	void (*config_irq)(void);
	const struct pinctrl_dev_config *pcfg;
};

static int can_stm32fd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const uint32_t rate_tmp = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE);

	ARG_UNUSED(dev);

	if (rate_tmp == LL_RCC_PERIPH_FREQUENCY_NO) {
		LOG_ERR("Can't read core clock");
		return -EIO;
	}

	*rate = rate_tmp / CAN_STM32FD_CLOCK_DIVISOR;

	return 0;
}

static int can_stm32fd_clock_enable(const struct device *dev)
{
	int ret;
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32fd_config *stm32fd_cfg = mcan_cfg->custom;

	LL_RCC_SetFDCANClockSource(CAN_STM32FD_CLOCK_SOURCE);

	/* LL_RCC API names do not align with PLL output name but are correct */
#ifdef CONFIG_CAN_STM32FD_CLOCK_SOURCE_PLL1Q
	LL_RCC_PLL1_EnableDomain_48M();
#elif CONFIG_CAN_STM32FD_CLOCK_SOURCE_PLL2P
	LL_RCC_PLL2_EnableDomain_SAI();
#endif

	ret = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			       (clock_control_subsys_t *)&stm32fd_cfg->pclken);
	if (ret < 0) {
		return ret;
	}

	FDCAN_CONFIG->CKDIV = CAN_STM32FD_CLOCK_DIVISOR >> 1;

	return 0;
}

static int can_stm32fd_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32fd_config *stm32fd_cfg = mcan_cfg->custom;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(stm32fd_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("CAN pinctrl setup failed (%d)", ret);
		return ret;
	}

	ret = can_stm32fd_clock_enable(dev);
	if (ret < 0) {
		LOG_ERR("Could not turn on CAN clock (%d)", ret);
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret != 0) {
		return ret;
	}

	stm32fd_cfg->config_irq();

	return ret;
}

static const struct can_driver_api can_stm32fd_driver_api = {
	.get_capabilities = can_mcan_get_capabilities,
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
	.get_state = can_mcan_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_mcan_recover,
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
	.get_core_clock = can_stm32fd_get_core_clock,
	.get_max_bitrate = can_mcan_get_max_bitrate,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback = can_mcan_set_state_change_callback,
	.timing_min = {
		.sjw = 0x01,
		.prop_seg = 0x00,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x80,
		.prop_seg = 0x00,
		.phase_seg1 = 0x100,
		.phase_seg2 = 0x80,
		.prescaler = 0x200
	},
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = {
		.sjw = 0x01,
		.prop_seg = 0x00,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_data_max = {
		.sjw = 0x10,
		.prop_seg = 0x00,
		.phase_seg1 = 0x20,
		.phase_seg2 = 0x10,
		.prescaler = 0x20
	}
#endif /* CONFIG_CAN_FD_MODE */
};

#define CAN_STM32FD_IRQ_CFG_FUNCTION(inst)                                     \
static void config_can_##inst##_irq(void)                                      \
{                                                                              \
	LOG_DBG("Enable CAN" #inst " IRQ");                                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, line_0, irq),                    \
		    DT_INST_IRQ_BY_NAME(inst, line_0, priority),               \
		    can_mcan_line_0_isr, DEVICE_DT_INST_GET(inst), 0);         \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, line_0, irq));                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, line_1, irq),                    \
		    DT_INST_IRQ_BY_NAME(inst, line_1, priority),               \
		    can_mcan_line_1_isr, DEVICE_DT_INST_GET(inst), 0);         \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, line_1, irq));                    \
}

#define CAN_STM32FD_CFG_INST(inst)					\
	PINCTRL_DT_INST_DEFINE(inst);					\
									\
	static const struct can_stm32fd_config can_stm32fd_cfg_##inst = { \
		.pclken = {						\
			.enr = DT_INST_CLOCKS_CELL(inst, bits),		\
			.bus = DT_INST_CLOCKS_CELL(inst, bus),		\
		},							\
		.config_irq = config_can_##inst##_irq,			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		\
	};								\
									\
	static const struct can_mcan_config can_mcan_cfg_##inst =	\
		CAN_MCAN_DT_CONFIG_INST_GET(inst, &can_stm32fd_cfg_##inst);

#define CAN_STM32FD_DATA_INST(inst)					\
	static struct can_mcan_data can_mcan_data_##inst =		\
		CAN_MCAN_DATA_INITIALIZER((struct can_mcan_msg_sram *)	\
			DT_INST_REG_ADDR_BY_NAME(inst, message_ram),	\
			NULL);

#define CAN_STM32FD_DEVICE_INST(inst)					\
	DEVICE_DT_INST_DEFINE(inst, &can_stm32fd_init, NULL,		\
			      &can_mcan_data_##inst, &can_mcan_cfg_##inst, \
			      POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,	\
			      &can_stm32fd_driver_api);

#define CAN_STM32FD_INST(inst)     \
CAN_STM32FD_IRQ_CFG_FUNCTION(inst) \
CAN_STM32FD_CFG_INST(inst)         \
CAN_STM32FD_DATA_INST(inst)        \
CAN_STM32FD_DEVICE_INST(inst)

DT_INST_FOREACH_STATUS_OKAY(CAN_STM32FD_INST)
