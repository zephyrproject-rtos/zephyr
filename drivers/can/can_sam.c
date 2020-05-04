/*
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/can.h>
#include <soc.h>
#include <kernel.h>
#include "can_sam.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(can_driver, CONFIG_CAN_LOG_LEVEL);
#define DT_DRV_COMPAT atmel_mcan

u32_t can_sam_get_core_clock(struct device *dev)
{

	return SOC_ATMEL_SAM_MCK_FREQ_HZ / (CONFIG_CAN_CKDIV + 1);
}

void can_sam_register_state_change_isr(struct device *dev,
				       can_state_change_isr_t isr)
{
	struct can_sam_data *data = DEV_DATA(dev);

	data->mcan_data.state_change_isr = isr;
}

void can_sam_clock_enable(const struct can_sam_config *cfg)
{
	REG_PMC_PCK5 = PMC_PCK_CSS_PLLA_CLK | PMC_PCK_PRES(CONFIG_CAN_CKDIV);
	PMC->PMC_SCER |= PMC_SCER_PCK5;
	soc_pmc_peripheral_enable(cfg->pmc_id);
}


static int can_sam_init(struct device *dev)
{
	const struct can_sam_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_sam_data *data = DEV_DATA(dev);
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;
	int ret;

	can_sam_clock_enable(cfg);
	soc_gpio_list_configure(cfg->pin_list, ARRAY_SIZE(cfg->pin_list));
	ret = can_mcan_init(dev, mcan_cfg, msg_ram, mcan_data);
	if (ret) {
		return ret;
	}

	cfg->config_irq();

	return ret;
}

enum can_state can_sam_get_state(struct device *dev,
				  struct can_bus_err_cnt *err_cnt)
{
	const struct can_sam_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_get_state(mcan_cfg, err_cnt);
}

int can_sam_send(struct device *dev, const struct zcan_frame *frame,
		  k_timeout_t timeout, can_tx_callback_t callback,
		  void *callback_arg)
{
	const struct can_sam_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_sam_data *data = DEV_DATA(dev);
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;

	return can_mcan_send(mcan_cfg, mcan_data, msg_ram, frame, timeout,
			     callback, callback_arg);
}

int can_sam_attach_isr(struct device *dev, can_rx_callback_t isr, void *cb_arg,
			const struct zcan_filter *filter)
{
	struct can_sam_data *data = DEV_DATA(dev);
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;

	return can_mcan_attach_isr(mcan_data, msg_ram, isr, cb_arg, filter);
}

void can_sam_detach(struct device *dev, int filter_nr)
{
	struct can_sam_data *data = DEV_DATA(dev);
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;

	can_mcan_detach(mcan_data, msg_ram, filter_nr);
}

int can_sam_runtime_configure(struct device *dev, enum can_mode mode,
			       struct can_timing *timing,
			       struct can_timing *timing_data)
{
	const struct can_sam_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_runtime_configure(mcan_cfg, mode, timing, timing_data);
}

void can_sam_line_0_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct can_sam_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_sam_data *data = DEV_DATA(dev);
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;

	can_mcan_line_0_isr(mcan_cfg, msg_ram, mcan_data);
}

void can_sam_line_1_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct can_sam_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_sam_data *data = DEV_DATA(dev);
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;

	can_mcan_line_1_isr(mcan_cfg, msg_ram, mcan_data);
}

static const struct can_driver_api can_api_funcs = {
	.configure = can_sam_runtime_configure,
	.send = can_sam_send,
	.attach_isr = can_sam_attach_isr,
	.detach = can_sam_detach,
	.get_state = can_sam_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_mcan_recover,
#endif
	.get_core_clock = can_sam_get_core_clock,
	.register_state_change_isr = can_sam_register_state_change_isr,
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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(can0), okay)

static void config_can_0_irq();

static const struct can_sam_config can_sam_cfg_0 = {
	.pmc_id = DT_PROP(DT_NODELABEL(can0), peripheral_id),
	.pin_list = {
		ATMEL_SAM_DT_PIN(0, 0),
		ATMEL_SAM_DT_PIN(0, 1)
	},
	.config_irq = config_can_0_irq,
	.mcan_cfg = {
		.can = (struct can_mcan_reg  *)
			DT_REG_ADDR_BY_NAME(DT_NODELABEL(can0), m_can),
		.bus_speed = DT_PROP(DT_NODELABEL(can0), bus_speed),
		.sjw = DT_PROP(DT_NODELABEL(can0), sjw),
#if DT_PROP(DT_NODELABEL(can0), sample_point) > 0
		.sample_point = DT_PROP(DT_NODELABEL(can0), sample_point),
#endif
#if DT_PROP(DT_NODELABEL(can0), phase_seg1) > 0 && \
    DT_PROP(DT_NODELABEL(can0), phase_seg2) > 0
		.prop_ts1 = DT_PROP(DT_NODELABEL(can0), prop_seg) +
			DT_PROP(DT_NODELABEL(can0), phase_seg1),
		.ts2 = DT_PROP(DT_NODELABEL(can0), phase_seg2),
#endif
#ifdef CONFIG_CAN_FD_MODE
		.bus_speed_data = DT_PROP(DT_NODELABEL(can0), bus_speed_data),
		.sjw_data = DT_PROP(DT_NODELABEL(can0), sjw_data),
#if DT_PROP(DT_NODELABEL(can0), sample_point_data) > 0
		.sample_point_data =
				DT_PROP(DT_NODELABEL(can0), sample_point_data),
#endif
#if DT_PROP(DT_NODELABEL(can0), phase_seg1_data) > 0 && \
    DT_PROP(DT_NODELABEL(can0), phase_seg2_data) > 0
		.prop_ts1_data = DT_PROP(DT_NODELABEL(can0), prop_seg_data) +
				DT_PROP(DT_NODELABEL(can0), phase_seg1_data),
		.ts2_data = DT_PROP(DT_NODELABEL(can0), phase_seg2_data),
#endif
#endif /* CONFIG_CAN_FD_MODE */
	}
};

static struct can_sam_data can_sam_dev_data_0;

DEVICE_AND_API_INIT(can_mcan_0, DT_LABEL(DT_NODELABEL(can0)), &can_sam_init,
		    &can_sam_dev_data_0, &can_sam_cfg_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &can_api_funcs);

static void config_can_0_irq(void)
{
	LOG_DBG("Enable CAN0 IRQ");

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can0), line_0, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can0), line_0, priority),
		    can_sam_line_0_isr, DEVICE_GET(can_mcan_0), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can0), line_0, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can0), line_1, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can0), line_1, priority),
		    can_sam_line_1_isr, DEVICE_GET(can_mcan_0), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can0), line_1, irq));
}
#endif /*DT_HAS_NODE(DT_NODELABEL(can0))*/
