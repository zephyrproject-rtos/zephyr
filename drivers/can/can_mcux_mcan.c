/*
 * Copyright (c) 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include "can_mcan.h"

LOG_MODULE_REGISTER(can_mcux_mcan, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_lpc_mcan

struct mcux_mcan_config {
	mm_reg_t base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
};

struct mcux_mcan_data {
	struct can_mcan_msg_sram msg_ram __nocache;
};

static int mcux_mcan_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcux_mcan_config *mcux_config = mcan_config->custom;

	return can_mcan_sys_read_reg(mcux_config->base, reg, val);
}

static int mcux_mcan_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcux_mcan_config *mcux_config = mcan_config->custom;

	return can_mcan_sys_write_reg(mcux_config->base, reg, val);
}

static int mcux_mcan_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcux_mcan_config *mcux_config = mcan_config->custom;

	return clock_control_get_rate(mcux_config->clock_dev, mcux_config->clock_subsys,
				      rate);
}

static int mcux_mcan_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcux_mcan_config *mcux_config = mcan_config->custom;
	int err;

	if (!device_is_ready(mcux_config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = pinctrl_apply_state(mcux_config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	err = clock_control_on(mcux_config->clock_dev, mcux_config->clock_subsys);
	if (err) {
		LOG_ERR("failed to enable clock (err %d)", err);
		return -EINVAL;
	}

	err = can_mcan_init(dev);
	if (err) {
		LOG_ERR("failed to initialize mcan (err %d)", err);
		return err;
	}

	mcux_config->irq_config_func(dev);

	return 0;
}

static const struct can_driver_api mcux_mcan_driver_api = {
	.get_capabilities = can_mcan_get_capabilities,
	.start = can_mcan_start,
	.stop = can_mcan_stop,
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_mcan_recover,
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
	.get_state = can_mcan_get_state,
	.set_state_change_callback = can_mcan_set_state_change_callback,
	.get_core_clock = mcux_mcan_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.get_max_bitrate = can_mcan_get_max_bitrate,
	/*
	 * MCUX MCAN timing limits are specified in the "Nominal bit timing and
	 * prescaler register (NBTP)" table in the SoC reference manual.
	 *
	 * Note that the values here are the "physical" timing limits, whereas
	 * the register field limits are physical values minus 1 (which is
	 * handled by the register assignments in the common MCAN driver code).
	 */
	.timing_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = 1
	},
	.timing_max = {
		.sjw = 128,
		.prop_seg = 0,
		.phase_seg1 = 256,
		.phase_seg2 = 128,
		.prescaler = 512,
	},
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	/*
	 * MCUX MCAN data timing limits are specified in the "Data bit timing
	 * and prescaler register (DBTP)" table in the SoC reference manual.
	 *
	 * Note that the values here are the "physical" timing limits, whereas
	 * the register field limits are physical values minus 1 (which is
	 * handled by the register assignments in the common MCAN driver code).
	 */
	.timing_data_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = 1,
	},
	.timing_data_max = {
		.sjw = 16,
		.prop_seg = 0,
		.phase_seg1 = 16,
		.phase_seg2 = 16,
		.prescaler = 32,
	}
#endif /* CONFIG_CAN_FD_MODE */
};

#define MCUX_MCAN_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static void mcux_mcan_irq_config_##n(const struct device *dev); \
									\
	static const struct mcux_mcan_config mcux_mcan_config_##n = {	\
		.base = (mm_reg_t)DT_INST_REG_ADDR(n),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = (clock_control_subsys_t)		\
			DT_INST_CLOCKS_CELL(n, name),			\
		.irq_config_func = mcux_mcan_irq_config_##n,		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
									\
	static const struct can_mcan_config can_mcan_config_##n =	\
		CAN_MCAN_DT_CONFIG_INST_GET(n, &mcux_mcan_config_##n,	\
					    mcux_mcan_read_reg,		\
					    mcux_mcan_write_reg);	\
									\
	static struct mcux_mcan_data mcux_mcan_data_##n;		\
									\
	static struct can_mcan_data can_mcan_data_##n =			\
		CAN_MCAN_DATA_INITIALIZER(&mcux_mcan_data_##n.msg_ram,	\
					  &mcux_mcan_data_##n);		\
									\
	DEVICE_DT_INST_DEFINE(n, &mcux_mcan_init, NULL,			\
			      &can_mcan_data_##n,			\
			      &can_mcan_config_##n,			\
			      POST_KERNEL,				\
			      CONFIG_CAN_INIT_PRIORITY,			\
			      &mcux_mcan_driver_api);			\
									\
	static void mcux_mcan_irq_config_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq),		\
			    DT_INST_IRQ_BY_IDX(n, 0, priority),		\
			    can_mcan_line_0_isr,			\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));		\
									\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 1, irq),		\
			    DT_INST_IRQ_BY_IDX(n, 1, priority),		\
			    can_mcan_line_1_isr,			\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQ_BY_IDX(n, 1, irq));		\
	}

DT_INST_FOREACH_STATUS_OKAY(MCUX_MCAN_INIT)
