/*
 * Copyright (c) 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/can.h>
#include <drivers/can/transceiver.h>
#include <drivers/clock_control.h>
#include <logging/log.h>

#include "can_mcan.h"

LOG_MODULE_REGISTER(can_mcux_mcan, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_lpc_mcan

struct mcux_mcan_config {
	struct can_mcan_config mcan;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_mcan_data {
	struct can_mcan_data mcan;
	struct can_mcan_msg_sram msg_ram __nocache;
};

static int mcux_mcan_set_mode(const struct device *dev, enum can_mode mode)
{
	const struct mcux_mcan_config *config = dev->config;

	return can_mcan_set_mode(&config->mcan, mode);
}

static int mcux_mcan_set_timing(const struct device *dev,
				const struct can_timing *timing,
				const struct can_timing *timing_data)
{
	const struct mcux_mcan_config *config = dev->config;

	return can_mcan_set_timing(&config->mcan, timing, timing_data);
}

static int mcux_mcan_send(const struct device *dev, const struct zcan_frame *msg,
			  k_timeout_t timeout, can_tx_callback_t callback,
			  void *user_data)
{
	const struct mcux_mcan_config *config = dev->config;
	struct mcux_mcan_data *data = dev->data;

	return can_mcan_send(&config->mcan, &data->mcan, &data->msg_ram,
			     msg, timeout, callback, user_data);
}

static int mcux_mcan_add_rx_filter(const struct device *dev,
				   can_rx_callback_t cb,
				   void *user_data,
				   const struct zcan_filter *filter)
{
	struct mcux_mcan_data *data = dev->data;

	return can_mcan_add_rx_filter(&data->mcan, &data->msg_ram,
				      cb, user_data, filter);
}

static void mcux_mcan_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct mcux_mcan_data *data = dev->data;

	can_mcan_remove_rx_filter(&data->mcan, &data->msg_ram, filter_id);
}

static int mcux_mcan_get_state(const struct device *dev, enum can_state *state,
			       struct can_bus_err_cnt *err_cnt)
{
	const struct mcux_mcan_config *config = dev->config;

	return can_mcan_get_state(&config->mcan, state, err_cnt);
}

static void mcux_mcan_set_state_change_callback(const struct device *dev,
						can_state_change_callback_t cb,
						void *user_data)
{
	struct mcux_mcan_data *data = dev->data;

	data->mcan.state_change_cb = cb;
	data->mcan.state_change_cb_data = user_data;
}

static int mcux_mcan_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct mcux_mcan_config *config = dev->config;

	return clock_control_get_rate(config->clock_dev, config->clock_subsys,
				      rate);
}

static int mcux_mcan_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate)
{
	const struct mcux_mcan_config *config = dev->config;

	*max_bitrate = config->mcan.max_bitrate;

	return 0;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static int mcux_mcan_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct mcux_mcan_config *config = dev->config;

	return can_mcan_recover(&config->mcan, timeout);
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

static void mcux_mcan_line_0_isr(const struct device *dev)
{
	const struct mcux_mcan_config *config = dev->config;
	struct mcux_mcan_data *data = dev->data;

	can_mcan_line_0_isr(&config->mcan, &data->msg_ram, &data->mcan);
}

static void mcux_mcan_line_1_isr(const struct device *dev)
{
	const struct mcux_mcan_config *config = dev->config;
	struct mcux_mcan_data *data = dev->data;

	can_mcan_line_1_isr(&config->mcan, &data->msg_ram, &data->mcan);
}

static int mcux_mcan_init(const struct device *dev)
{
	const struct mcux_mcan_config *config = dev->config;
	struct mcux_mcan_data *data = dev->data;
	int err;

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err) {
		LOG_ERR("failed to enable clock (err %d)", err);
		return -EINVAL;
	}

	err = can_mcan_init(dev, &config->mcan, &data->msg_ram, &data->mcan);
	if (err) {
		LOG_ERR("failed to initialize mcan (err %d)", err);
		return err;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct can_driver_api mcux_mcan_driver_api = {
	.set_mode = mcux_mcan_set_mode,
	.set_timing = mcux_mcan_set_timing,
	.send = mcux_mcan_send,
	.add_rx_filter = mcux_mcan_add_rx_filter,
	.remove_rx_filter = mcux_mcan_remove_rx_filter,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = mcux_mcan_recover,
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
	.get_state = mcux_mcan_get_state,
	.set_state_change_callback = mcux_mcan_set_state_change_callback,
	.get_core_clock = mcux_mcan_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.get_max_bitrate = mcux_mcan_get_max_bitrate,
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
	/*
	 * MCUX MCAN data timing limits are specified in the "Data bit timing
	 * and prescaler register (DBTP)" table in the SoC reference manual.
	 *
	 * Note that the values here are the "physical" timing limits, whereas
	 * the register field limits are physical values minus 1 (which is
	 * handled by the register assignments in the common MCAN driver code).
	 */
	.timing_min_data = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = 1,
	},
	.timing_max_data = {
		.sjw = 16,
		.prop_seg = 0,
		.phase_seg1 = 16,
		.phase_seg2 = 16,
		.prescaler = 32,
	}
#endif /* CONFIG_CAN_FD_MODE */
};

#ifdef CONFIG_CAN_FD_MODE
#define MCUX_MCAN_MCAN_INIT(n)			\
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
#define MCUX_MCAN_MCAN_INIT(n)						\
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

#define MCUX_MCAN_INIT(n)						\
	static void mcux_mcan_irq_config_##n(const struct device *dev); \
									\
	static const struct mcux_mcan_config mcux_mcan_config_##n = {	\
		.mcan = MCUX_MCAN_MCAN_INIT(n),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = (clock_control_subsys_t)		\
			DT_INST_CLOCKS_CELL(n, name),			\
		.irq_config_func = mcux_mcan_irq_config_##n,		\
	};								\
									\
	static struct mcux_mcan_data mcux_mcan_data_##n;		\
									\
	DEVICE_DT_INST_DEFINE(n, &mcux_mcan_init, NULL,			\
			      &mcux_mcan_data_##n,			\
			      &mcux_mcan_config_##n,			\
			      POST_KERNEL,				\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &mcux_mcan_driver_api);			\
									\
	static void mcux_mcan_irq_config_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq),		\
			    DT_INST_IRQ_BY_IDX(n, 0, priority),		\
			    mcux_mcan_line_0_isr,			\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));		\
									\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 1, irq),		\
			    DT_INST_IRQ_BY_IDX(n, 1, priority),		\
			    mcux_mcan_line_1_isr,			\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQ_BY_IDX(n, 1, irq));		\
	}

DT_INST_FOREACH_STATUS_OKAY(MCUX_MCAN_INIT)
