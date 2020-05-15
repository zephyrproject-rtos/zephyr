/*
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/can.h>
#include <kernel.h>
#include <soc.h>
#include "can_stm32fd.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(can_driver, CONFIG_CAN_LOG_LEVEL);

u32_t can_stm32fd_get_core_clock(struct device *dev)
{
	ARG_UNUSED(dev);
	u32_t core_clock = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE);
#if CONFIG_CAN_CKDIV != 0
	core_clock /= CONFIG_CAN_CKDIV * 2;
#endif
	return core_clock;
}

void can_stm32fd_clock_enable(void)
{
	LL_RCC_SetFDCANClockSource(LL_RCC_FDCAN_CLKSOURCE_PCLK1);
	__HAL_RCC_FDCAN_CLK_ENABLE();

	FDCAN_CONFIG->CKDIV = CONFIG_CAN_CKDIV;
}

void can_stm32fd_register_state_change_isr(struct device *dev,
					   can_state_change_isr_t isr)
{
	struct can_stm32fd_data *data = DEV_DATA(dev);

	data->mcan_data.state_change_isr = isr;
}

static int can_stm32fd_init(struct device *dev)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_mcan_data *mcan_data = &DEV_DATA(dev)->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;
	int ret;

	can_stm32fd_clock_enable();
	ret = can_mcan_init(dev, mcan_cfg, msg_ram, mcan_data);
	if (ret) {
		return ret;
	}

	cfg->config_irq();

	return ret;
}

enum can_state can_stm32fd_get_state(struct device *dev,
				     struct can_bus_err_cnt *err_cnt)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_get_state(mcan_cfg, err_cnt);
}

int can_stm32fd_send(struct device *dev, const struct zcan_frame *frame,
		     k_timeout_t timeout, can_tx_callback_t callback,
		     void *callback_arg)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_mcan_data *mcan_data = &DEV_DATA(dev)->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	return can_mcan_send(mcan_cfg, mcan_data, msg_ram, frame, timeout,
			     callback, callback_arg);
}

int can_stm32fd_attach_isr(struct device *dev, can_rx_callback_t isr,
			   void *cb_arg, const struct zcan_filter *filter)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	struct can_mcan_data *mcan_data = &DEV_DATA(dev)->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	return can_mcan_attach_isr(mcan_data, msg_ram, isr, cb_arg, filter);
}

void can_stm32fd_detach(struct device *dev, int filter_nr)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	struct can_mcan_data *mcan_data = &DEV_DATA(dev)->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	can_mcan_detach(mcan_data, msg_ram, filter_nr);
}

int can_stm32fd_runtime_configure(struct device *dev, enum can_mode mode,
				  struct can_timing *timing,
				  struct can_timing *timing_data)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_runtime_configure(mcan_cfg, mode, timing, timing_data);
}

void can_stm32fd_line_0_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_stm32fd_data *data = DEV_DATA(dev);
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	can_mcan_line_0_isr(mcan_cfg, msg_ram, mcan_data);
}

void can_stm32fd_line_1_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_mcan_data *mcan_data = &DEV_DATA(dev)->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	can_mcan_line_1_isr(mcan_cfg, msg_ram, mcan_data);
}

static const struct can_driver_api can_api_funcs = {
	.configure = can_stm32fd_runtime_configure,
	.send = can_stm32fd_send,
	.attach_isr = can_stm32fd_attach_isr,
	.detach = can_stm32fd_detach,
	.get_state = can_stm32fd_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_mcan_recover,
#endif
	.get_core_clock = can_stm32fd_get_core_clock,
	.register_state_change_isr = can_stm32fd_register_state_change_isr,
	.timing_min = {
		.sjw = 0x7f,
		.prop_seg = 0x00,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x7f,
		.prop_seg = 0x00,
		.phase_seg1 = 0x100,
		.phase_seg2 = 0x80,
		.prescaler = 0x200
	},
#ifdef CONFIG_CAN_FD_MODE
	.timing_min_data = {
		.sjw = 0x01,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(can1), okay)

static void config_can_1_irq(void);

static const struct can_stm32fd_config can_stm32fd_cfg_1 = {
	.msg_sram = (struct can_mcan_msg_sram *)
			DT_REG_ADDR_BY_NAME(DT_NODELABEL(can1), message_ram),
	.config_irq = config_can_1_irq,
	.mcan_cfg = {
		.can = (struct can_mcan_reg *)
			DT_REG_ADDR_BY_NAME(DT_NODELABEL(can1), m_can),
		.bus_speed = DT_PROP(DT_NODELABEL(can1), bus_speed),
		.sjw = DT_PROP(DT_NODELABEL(can1), sjw),
#if DT_PROP(DT_NODELABEL(can1), sample_point) > 0
		.sample_point = DT_PROP(DT_NODELABEL(can1), sample_point),
#endif
#if DT_PROP(DT_NODELABEL(can1), phase_seg1) > 0 && \
    DT_PROP(DT_NODELABEL(can1), phase_seg2) > 0
		.prop_ts1 = DT_PROP(DT_NODELABEL(can1), prop_seg) +
			DT_PROP(DT_NODELABEL(can1), phase_seg1),
		.ts2 = DT_PROP(DT_NODELABEL(can1), phase_seg2),
#endif
#ifdef CONFIG_CAN_FD_MODE
		.bus_speed_data = DT_PROP(DT_NODELABEL(can1), bus_speed_data),
		.sjw_data = DT_PROP(DT_NODELABEL(can1), sjw_data),
#if DT_PROP(DT_NODELABEL(can1), sample_point_data) > 0
		.sample_point_data = DT_PROP(DT_NODELABEL(can1),
					     sample_point_data),
#endif
#if DT_PROP(DT_NODELABEL(can1), phase_seg1_data) > 0 && \
    DT_PROP(DT_NODELABEL(can1), phase_seg2_data) > 0
		.prop_ts1_data = DT_PROP(DT_NODELABEL(can1), prop_seg_data) +
				DT_PROP(DT_NODELABEL(can1), phase_seg1_data),
		.ts2_data = DT_PROP(DT_NODELABEL(can1), phase_seg2_data),
#endif
#endif /* CONFIG_CAN_FD_MODE */
	}
};

static struct can_stm32fd_data can_stm32fd_dev_data_1;

DEVICE_AND_API_INIT(can_stm32fd_1, DT_LABEL(DT_NODELABEL(can1)),
		    &can_stm32fd_init,
		    &can_stm32fd_dev_data_1, &can_stm32fd_cfg_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &can_api_funcs);

static void config_can_1_irq(void)
{
	LOG_DBG("Enable CAN1 IRQ");

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can1), line_0, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can1), line_0, priority),
		    can_stm32fd_line_0_isr, DEVICE_GET(can_stm32fd_1), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can1), line_0, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can1), line_1, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can1), line_1, priority),
		    can_stm32fd_line_1_isr, DEVICE_GET(can_stm32fd_1), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can1), line_1, irq));
}
#endif /*DT_HAS_NODE(DT_NODELABEL(can1))*/
