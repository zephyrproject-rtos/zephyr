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
#include <zephyr/logging/log.h>

#include "can_mcan.h"

LOG_MODULE_REGISTER(can_stm32fd, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_fdcan

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32_CANFD_DOMAIN_CLOCK_SUPPORT 1
#else
#define STM32_CANFD_DOMAIN_CLOCK_SUPPORT 0
#endif

struct can_stm32fd_config {
	size_t pclk_len;
	const struct stm32_pclken *pclken;
	void (*config_irq)(void);
	const struct pinctrl_dev_config *pcfg;
	uint8_t clock_divider;
};

static int can_stm32fd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const uint32_t rate_tmp = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE);

	ARG_UNUSED(dev);

	if (rate_tmp == LL_RCC_PERIPH_FREQUENCY_NO) {
		LOG_ERR("Can't read core clock");
		return -EIO;
	}

	if (FDCAN_CONFIG->CKDIV == 0) {
		*rate = rate_tmp;
	} else {
		*rate = rate_tmp / (FDCAN_CONFIG->CKDIV << 1);
	}

	return 0;
}

static int can_stm32fd_clock_enable(const struct device *dev)
{
	int ret;
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32fd_config *stm32fd_cfg = mcan_cfg->custom;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	if (IS_ENABLED(STM32_CANFD_DOMAIN_CLOCK_SUPPORT) && (stm32fd_cfg->pclk_len > 1)) {
		ret = clock_control_configure(clk,
				(clock_control_subsys_t)&stm32fd_cfg->pclken[1],
				NULL);
		if (ret < 0) {
			LOG_ERR("Could not select can_stm32fd domain clock");
			return ret;
		}
	}

	ret = clock_control_on(clk, (clock_control_subsys_t)&stm32fd_cfg->pclken[0]);
	if (ret < 0) {
		return ret;
	}

	if (stm32fd_cfg->clock_divider != 0) {
		can_mcan_enable_configuration_change(dev);
		FDCAN_CONFIG->CKDIV = stm32fd_cfg->clock_divider >> 1;
	}

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
	.start = can_mcan_start,
	.stop = can_mcan_stop,
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
	static const struct stm32_pclken can_stm32fd_pclken_##inst[] =	\
					STM32_DT_INST_CLOCKS(inst);	\
									\
	static const struct can_stm32fd_config can_stm32fd_cfg_##inst = { \
		.pclken = can_stm32fd_pclken_##inst,			\
		.pclk_len = DT_INST_NUM_CLOCKS(inst),			\
		.config_irq = config_can_##inst##_irq,			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		\
		.clock_divider = DT_INST_PROP_OR(inst, clk_divider, 0)  \
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
