/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2022 Blue Clover
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/can.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/clock_control.h>
#include <drivers/pinctrl.h>
#include <kernel.h>
#include <stm32_ll_rcc.h>
#include <logging/log.h>

#include "can_mcan.h"

LOG_MODULE_REGISTER(can_stm32h7, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32h7_fdcan

struct can_stm32h7_config {
	void (*config_irq)(void);
	const struct pinctrl_dev_config *pcfg;
	struct stm32_pclken pclken;
};

static int can_stm32h7_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const uint32_t rate_tmp = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE);

	ARG_UNUSED(dev);

	if (rate_tmp == LL_RCC_PERIPH_FREQUENCY_NO) {
		LOG_ERR("Can't read core clock");
		return -EIO;
	}

	*rate = rate_tmp;

	LOG_DBG("rate=%d", *rate);

	return 0;
}

static int can_stm32h7_clock_enable(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32h7_config *stm32h7_cfg = mcan_cfg->custom;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int ret;

	LL_RCC_SetFDCANClockSource(LL_RCC_FDCAN_CLKSOURCE_PLL1Q);

	ret = clock_control_on(clk, (clock_control_subsys_t *)&stm32h7_cfg->pclken);
	if (ret != 0) {
		LOG_ERR("failure enabling clock");
		return ret;
	}

	if (!LL_RCC_PLL1Q_IsEnabled()) {
		LOG_ERR("PLL1Q clock must be enabled!");
		return -EIO;
	}

	return 0;
}

static int can_stm32h7_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32h7_config *stm32h7_cfg = mcan_cfg->custom;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(stm32h7_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("CAN pinctrl setup failed (%d)", ret);
		return ret;
	}

	ret = can_stm32h7_clock_enable(dev);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret != 0) {
		return ret;
	}

	stm32h7_cfg->config_irq();

	return 0;
}

static const struct can_driver_api can_stm32h7_driver_api = {
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
	.get_state = can_mcan_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_mcan_recover,
#endif
	.get_core_clock = can_stm32h7_get_core_clock,
	.get_max_bitrate = can_mcan_get_max_bitrate,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback = can_mcan_set_state_change_callback,
	/* Timing limits are per the STM32H7 Reference Manual (RM0433 Rev 7),
	 * section 56.5.7, FDCAN nominal bit timing and prescaler register
	 * (FDCAN_NBTP).
	 */
	.timing_min = {
		.sjw = 0x00,
		.prop_seg = 0x00,
		.phase_seg1 = 0x00,
		.phase_seg2 = 0x00,
		.prescaler = 0x00
	},
	.timing_max = {
		.sjw = 0x7f,
		.prop_seg = 0x00,
		.phase_seg1 = 0x100,
		.phase_seg2 = 0x80,
		.prescaler = 0x200
	},
#ifdef CONFIG_CAN_FD_MODE
	/* Data timing limits are per the STM32H7 Reference Manual
	 * (RM0433 Rev 7), section 56.5.3, FDCAN data bit timing and prescaler
	 * register (FDCAN_DBTP).
	 */
	.timing_min_data = {
		.sjw = 0x00,
		.prop_seg = 0x00,
		.phase_seg1 = 0x00,
		.phase_seg2 = 0x00,
		.prescaler = 0x00
	},
	.timing_max_data = {
		.sjw = 0x10,
		.prop_seg = 0x00,
		.phase_seg1 = 0x20,
		.phase_seg2 = 0x10,
		.prescaler = 0x20
	}
#endif
};

#ifdef CONFIG_CAN_FD_MODE
#define CAN_STM32H7_MCAN_MCAN_INIT(n)					\
	{								\
		.can = (struct can_mcan_reg *)DT_INST_REG_ADDR(n),	\
		.bus_speed = DT_INST_PROP(n, bus_speed),		\
		.sjw = DT_INST_PROP(n, sjw),				\
		.sample_point = DT_INST_PROP_OR(n, sample_point, 0),	\
		.prop_ts1 = DT_INST_PROP_OR(n, prop_seg, 0) +		\
			DT_INST_PROP_OR(n, phase_seg1, 0),		\
		.ts2 = DT_INST_PROP_OR(n, phase_seg2, 0),		\
		.bus_speed_data = DT_INST_PROP(n, bus_speed_data),	\
		.sjw_data = DT_INST_PROP(n, sjw_data),			\
		.sample_point_data =					\
			DT_INST_PROP_OR(n, sample_point_data, 0),	\
		.prop_ts1_data = DT_INST_PROP_OR(n, prop_seg_data, 0) + \
			DT_INST_PROP_OR(n, phase_seg1_data, 0),		\
		.ts2_data = DT_INST_PROP_OR(n, phase_seg2_data, 0),	\
		.tx_delay_comp_offset =					\
			DT_INST_PROP(n, tx_delay_comp_offset),		\
		.phy = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(n, phys)),	\
		.max_bitrate = DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(n, 5000000), \
		.custom = &can_stm32h7_cfg_##n,				\
	}
#else /* CONFIG_CAN_FD_MODE */
#define CAN_STM32H7_MCAN_MCAN_INIT(n)					\
	{								\
		.can = (struct can_mcan_reg *)DT_INST_REG_ADDR(n),	\
		.bus_speed = DT_INST_PROP(n, bus_speed),		\
		.sjw = DT_INST_PROP(n, sjw),				\
		.sample_point = DT_INST_PROP_OR(n, sample_point, 0),	\
		.prop_ts1 = DT_INST_PROP_OR(n, prop_seg, 0) +		\
			DT_INST_PROP_OR(n, phase_seg1, 0),		\
		.ts2 = DT_INST_PROP_OR(n, phase_seg2, 0),		\
		.phy = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(n, phys)),	\
		.max_bitrate = DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(n, 1000000), \
		.custom = &can_stm32h7_cfg_##n,				\
	}
#endif /* !CONFIG_CAN_FD_MODE */

#define CAN_STM32H7_MCAN_INIT(n)					    \
	static void stm32h7_mcan_irq_config_##n(void);			    \
									    \
	PINCTRL_DT_INST_DEFINE(n);					    \
									    \
	static const struct can_stm32h7_config can_stm32h7_cfg_##n = {	    \
		.config_irq = stm32h7_mcan_irq_config_##n,		    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		    \
		.pclken = {						    \
			.enr = DT_INST_CLOCKS_CELL(n, bits),		    \
			.bus = DT_INST_CLOCKS_CELL(n, bus),		    \
		},							    \
	};								    \
									    \
	static const struct can_mcan_config can_mcan_cfg_##n =		    \
		CAN_STM32H7_MCAN_MCAN_INIT(n);				    \
									    \
	static struct can_mcan_data can_mcan_data_##n = {		    \
		.msg_ram = (struct can_mcan_msg_sram *)			    \
			DT_INST_REG_ADDR_BY_NAME(n, message_ram),	    \
	};								    \
									    \
	DEVICE_DT_INST_DEFINE(n, &can_stm32h7_init, NULL,		    \
			      &can_mcan_data_##n,			    \
			      &can_mcan_cfg_##n,			    \
			      POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,	    \
			      &can_stm32h7_driver_api);			    \
									    \
	static void stm32h7_mcan_irq_config_##n(void)			    \
	{								    \
		LOG_DBG("Enable CAN inst" #n " IRQ");			    \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, line_0, irq),	    \
			DT_INST_IRQ_BY_NAME(n, line_0, priority),	    \
			can_mcan_line_0_isr, DEVICE_DT_INST_GET(n), 0);	    \
		irq_enable(DT_INST_IRQ_BY_NAME(n, line_0, irq));	    \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, line_1, irq),	    \
			DT_INST_IRQ_BY_NAME(n, line_1, priority),	    \
			can_mcan_line_1_isr, DEVICE_DT_INST_GET(n), 0);	    \
		irq_enable(DT_INST_IRQ_BY_NAME(n, line_1, irq));	    \
	}

DT_INST_FOREACH_STATUS_OKAY(CAN_STM32H7_MCAN_INIT)
