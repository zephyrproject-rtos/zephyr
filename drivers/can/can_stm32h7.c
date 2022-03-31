/*
 * Copyright (c) 2022 Blue Clover
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/can.h>
#include <drivers/can/transceiver.h>
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
	struct can_mcan_msg_sram *msg_sram;
	void (*config_irq)(void);
	struct can_mcan_config mcan_cfg;
	const struct pinctrl_dev_config *pcfg;
	struct stm32_pclken pclken;
};

struct can_stm32h7_data {
	struct can_mcan_data mcan_data;
};

static int can_stm32h7_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	const uint32_t rate_tmp = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE);

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
	int ret;
	const struct can_stm32h7_config *cfg = dev->config;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	LL_RCC_SetFDCANClockSource(LL_RCC_FDCAN_CLKSOURCE_PLL1Q);

	ret = clock_control_on(clk, (clock_control_subsys_t *)&cfg->pclken);
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

static void can_stm32h7_set_state_change_cb(const struct device *dev,
					    can_state_change_callback_t cb,
					    void *user_data)
{
	struct can_stm32h7_data *data = dev->data;

	data->mcan_data.state_change_cb = cb;
	data->mcan_data.state_change_cb_data = user_data;
}

static int can_stm32h7_init(const struct device *dev)
{
	const struct can_stm32h7_config *cfg = dev->config;
	struct can_stm32h7_data *data = dev->data;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("CAN pinctrl setup failed (%d)", ret);
		return ret;
	}

	ret = can_stm32h7_clock_enable(dev);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_init(dev, &cfg->mcan_cfg, cfg->msg_sram,
			    &data->mcan_data);
	if (ret != 0) {
		return ret;
	}

	cfg->config_irq();

	return 0;
}

static int can_stm32h7_get_state(const struct device *dev, enum can_state *state,
				 struct can_bus_err_cnt *err_cnt)
{
	const struct can_stm32h7_config *cfg = dev->config;

	return can_mcan_get_state(&cfg->mcan_cfg, state, err_cnt);
}

static int can_stm32h7_send(const struct device *dev,
			    const struct zcan_frame *frame,
			    k_timeout_t timeout, can_tx_callback_t callback,
			    void *user_data)
{
	const struct can_stm32h7_config *cfg = dev->config;
	struct can_stm32h7_data *data = dev->data;

	return can_mcan_send(&cfg->mcan_cfg, &data->mcan_data, cfg->msg_sram,
			     frame, timeout, callback, user_data);
}

static int can_stm32h7_add_rx_filter(const struct device *dev,
				     can_rx_callback_t callback,
				     void *user_data,
				     const struct zcan_filter *filter)
{
	const struct can_stm32h7_config *cfg = dev->config;
	struct can_stm32h7_data *data = dev->data;

	return can_mcan_add_rx_filter(&data->mcan_data, cfg->msg_sram,
				      callback, user_data, filter);
}

static void can_stm32h7_remove_rx_filter(const struct device *dev,
					 int filter_id)
{
	const struct can_stm32h7_config *cfg = dev->config;
	struct can_stm32h7_data *data = dev->data;

	can_mcan_remove_rx_filter(&data->mcan_data, cfg->msg_sram, filter_id);
}

static int can_stm32h7_set_mode(const struct device *dev, enum can_mode mode)
{
	const struct can_stm32h7_config *cfg = dev->config;

	return can_mcan_set_mode(&cfg->mcan_cfg, mode);
}

static int can_stm32h7_set_timing(const struct device *dev,
				  const struct can_timing *timing,
				  const struct can_timing *timing_data)
{
	const struct can_stm32h7_config *cfg = dev->config;

	return can_mcan_set_timing(&cfg->mcan_cfg, timing, timing_data);
}

static int can_stm32h7_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate)
{
	const struct can_stm32h7_config *cfg = dev->config;

	*max_bitrate = cfg->mcan_cfg.max_bitrate;

	return 0;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static int can_stm32h7_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_stm32h7_config *cfg = dev->config;

	return can_mcan_recover(&cfg->mcan_cfg, timeout);
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

static void can_stm32h7_line_0_isr(const struct device *dev)
{
	const struct can_stm32h7_config *cfg = dev->config;
	struct can_stm32h7_data *data = dev->data;

	can_mcan_line_0_isr(&cfg->mcan_cfg, cfg->msg_sram, &data->mcan_data);
}

static void can_stm32h7_line_1_isr(const struct device *dev)
{
	const struct can_stm32h7_config *cfg = dev->config;
	struct can_stm32h7_data *data = dev->data;

	can_mcan_line_1_isr(&cfg->mcan_cfg, cfg->msg_sram, &data->mcan_data);
}

static const struct can_driver_api can_api_funcs = {
	.set_mode = can_stm32h7_set_mode,
	.set_timing = can_stm32h7_set_timing,
	.send = can_stm32h7_send,
	.add_rx_filter = can_stm32h7_add_rx_filter,
	.remove_rx_filter = can_stm32h7_remove_rx_filter,
	.get_state = can_stm32h7_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_stm32h7_recover,
#endif
	.get_core_clock = can_stm32h7_get_core_clock,
	.get_max_bitrate = can_stm32h7_get_max_bitrate,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback = can_stm32h7_set_state_change_cb,
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
	}
#endif /* !CONFIG_CAN_FD_MODE */

#define CAN_STM32H7_MCAN_INIT(n)					    \
	static void stm32h7_mcan_irq_config_##n(void);			    \
									    \
	PINCTRL_DT_INST_DEFINE(n);					    \
									    \
	static const struct can_stm32h7_config can_stm32h7_cfg_##n = {	    \
		.msg_sram = (struct can_mcan_msg_sram *)		    \
			     DT_INST_REG_ADDR_BY_NAME(n, message_ram),	    \
		.config_irq = stm32h7_mcan_irq_config_##n,		    \
		.mcan_cfg = CAN_STM32H7_MCAN_MCAN_INIT(n),		    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		    \
		.pclken = {						    \
			.enr = DT_INST_CLOCKS_CELL(n, bits),		    \
			.bus = DT_INST_CLOCKS_CELL(n, bus),		    \
		},							    \
	};								    \
									    \
	static struct can_stm32h7_data can_stm32h7_dev_data_##n;	    \
									    \
	DEVICE_DT_INST_DEFINE(n, &can_stm32h7_init, NULL,		    \
			      &can_stm32h7_dev_data_##n,		    \
			      &can_stm32h7_cfg_##n,			    \
			      POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,	    \
			      &can_api_funcs);				    \
									    \
	static void stm32h7_mcan_irq_config_##n(void)			    \
	{								    \
		LOG_DBG("Enable CAN inst" #n " IRQ");			    \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, line_0, irq),	    \
			DT_INST_IRQ_BY_NAME(n, line_0, priority),	    \
			can_stm32h7_line_0_isr, DEVICE_DT_INST_GET(n), 0);  \
		irq_enable(DT_INST_IRQ_BY_NAME(n, line_0, irq));	    \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, line_1, irq),	    \
			DT_INST_IRQ_BY_NAME(n, line_1, priority),	    \
			can_stm32h7_line_1_isr, DEVICE_DT_INST_GET(n), 0);  \
		irq_enable(DT_INST_IRQ_BY_NAME(n, line_1, irq));	    \
	}

DT_INST_FOREACH_STATUS_OKAY(CAN_STM32H7_MCAN_INIT)
