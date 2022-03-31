/*
 * Copyright (c) 2021 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "can_mcan.h"

#include <drivers/can.h>
#include <drivers/can/transceiver.h>
#include <drivers/pinctrl.h>
#include <soc.h>
#include <kernel.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(can_sam, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT atmel_sam_can

struct can_sam_config {
	struct can_mcan_config mcan_cfg;
	void (*config_irq)(void);
	const struct pinctrl_dev_config *pcfg;
	uint8_t pmc_id;
};

struct can_sam_data {
	struct can_mcan_data mcan_data;
	struct can_mcan_msg_sram msg_ram;
};


static int can_sam_get_core_clock(const struct device *dev, uint32_t *rate)
{
	*rate = SOC_ATMEL_SAM_MCK_FREQ_HZ / (CONFIG_CAN_SAM_CKDIV + 1);
	return 0;
}

static void can_sam_set_state_change_callback(const struct device *dev,
					      can_state_change_callback_t cb,
					      void *user_data)
{
	struct can_sam_data *data = dev->data;

	data->mcan_data.state_change_cb_data = user_data;
	data->mcan_data.state_change_cb = cb;
}

static void can_sam_clock_enable(const struct can_sam_config *cfg)
{
	REG_PMC_PCK5 = PMC_PCK_CSS_PLLA_CLK | PMC_PCK_PRES(CONFIG_CAN_SAM_CKDIV);
	PMC->PMC_SCER |= PMC_SCER_PCK5;
	soc_pmc_peripheral_enable(cfg->pmc_id);
}

static int can_sam_init(const struct device *dev)
{
	const struct can_sam_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_sam_data *data = dev->data;
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;
	int ret;

	can_sam_clock_enable(cfg);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = can_mcan_init(dev, mcan_cfg, msg_ram, mcan_data);
	if (ret) {
		return ret;
	}

	cfg->config_irq();

	return ret;
}

static int can_sam_get_state(const struct device *dev, enum can_state *state,
			     struct can_bus_err_cnt *err_cnt)
{
	const struct can_sam_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_get_state(mcan_cfg, state, err_cnt);
}

static int can_sam_send(const struct device *dev, const struct zcan_frame *frame,
			k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	const struct can_sam_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_sam_data *data = dev->data;
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;

	return can_mcan_send(mcan_cfg, mcan_data, msg_ram, frame, timeout, callback, user_data);
}

static int can_sam_add_rx_filter(const struct device *dev, can_rx_callback_t cb, void *cb_arg,
				 const struct zcan_filter *filter)
{
	struct can_sam_data *data = dev->data;
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;

	return can_mcan_add_rx_filter(mcan_data, msg_ram, cb, cb_arg, filter);
}

static void can_sam_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_sam_data *data = dev->data;
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;

	can_mcan_remove_rx_filter(mcan_data, msg_ram, filter_id);
}

static int can_sam_set_mode(const struct device *dev, enum can_mode mode)
{
	const struct can_sam_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_set_mode(mcan_cfg, mode);
}

static int can_sam_set_timing(const struct device *dev, const struct can_timing *timing,
			      const struct can_timing *timing_data)
{
	const struct can_sam_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_set_timing(mcan_cfg, timing, timing_data);
}

static int can_sam_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate)
{
	const struct can_sam_config *cfg = dev->config;

	*max_bitrate = cfg->mcan_cfg.max_bitrate;

	return 0;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static int can_sam_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_sam_config *cfg = dev->config;

	return can_mcan_recover(&cfg->mcan_cfg, timeout);
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

static void can_sam_line_0_isr(const struct device *dev)
{
	const struct can_sam_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_sam_data *data = dev->data;
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;

	can_mcan_line_0_isr(mcan_cfg, msg_ram, mcan_data);
}

static void can_sam_line_1_isr(const struct device *dev)
{
	const struct can_sam_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_sam_data *data = dev->data;
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = &data->msg_ram;

	can_mcan_line_1_isr(mcan_cfg, msg_ram, mcan_data);
}

static const struct can_driver_api can_api_funcs = {
	.set_mode = can_sam_set_mode,
	.set_timing = can_sam_set_timing,
	.send = can_sam_send,
	.add_rx_filter = can_sam_add_rx_filter,
	.remove_rx_filter = can_sam_remove_rx_filter,
	.get_state = can_sam_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_sam_recover,
#endif
	.get_core_clock = can_sam_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.get_max_bitrate = can_sam_get_max_bitrate,
	.set_state_change_callback =  can_sam_set_state_change_callback,
	.timing_min = {
		.sjw = 0x1,
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
		.prop_seg = 0x00,
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

#define CAN_SAM_IRQ_CFG_FUNCTION(inst)                                                         \
static void config_can_##inst##_irq(void)                                                      \
{                                                                                              \
	LOG_DBG("Enable CAN##inst## IRQ");                                                     \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, line_0, irq),                                    \
		    DT_INST_IRQ_BY_NAME(inst, line_0, priority), can_sam_line_0_isr,           \
					DEVICE_DT_INST_GET(inst), 0);                          \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, line_0, irq));                                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, line_1, irq),                                    \
		    DT_INST_IRQ_BY_NAME(inst, line_1, priority), can_sam_line_1_isr,           \
					DEVICE_DT_INST_GET(inst), 0);                          \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, line_1, irq));                                    \
}

#ifdef CONFIG_CAN_FD_MODE
#define CAN_SAM_MCAN_CFG(inst)                                                                 \
{                                                                                              \
	.can = (struct can_mcan_reg *)DT_INST_REG_ADDR(inst),                                  \
	.bus_speed = DT_INST_PROP(inst, bus_speed), .sjw = DT_INST_PROP(inst, sjw),            \
	.sample_point = DT_INST_PROP_OR(inst, sample_point, 0),                                \
	.prop_ts1 = DT_INST_PROP_OR(inst, prop_seg, 0) + DT_INST_PROP_OR(inst, phase_seg1, 0), \
	.ts2 = DT_INST_PROP_OR(inst, phase_seg2, 0),                                           \
	.bus_speed_data = DT_INST_PROP(inst, bus_speed_data),                                  \
	.sjw_data = DT_INST_PROP(inst, sjw_data),                                              \
	.sample_point_data = DT_INST_PROP_OR(inst, sample_point_data, 0),                      \
	.prop_ts1_data = DT_INST_PROP_OR(inst, prop_seg_data, 0) +                             \
			 DT_INST_PROP_OR(inst, phase_seg1_data, 0),                            \
	.ts2_data = DT_INST_PROP_OR(inst, phase_seg2_data, 0),                                 \
	.tx_delay_comp_offset = DT_INST_PROP(inst, tx_delay_comp_offset),                      \
	.phy = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, phys)),                             \
	.max_bitrate = DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(inst, 5000000),                     \
}
#else /* CONFIG_CAN_FD_MODE */
#define CAN_SAM_MCAN_CFG(inst)                                                                 \
{                                                                                              \
	.can = (struct can_mcan_reg *)DT_INST_REG_ADDR(inst),                                  \
	.bus_speed = DT_INST_PROP(inst, bus_speed), .sjw = DT_INST_PROP(inst, sjw),            \
	.sample_point = DT_INST_PROP_OR(inst, sample_point, 0),                                \
	.prop_ts1 = DT_INST_PROP_OR(inst, prop_seg, 0) + DT_INST_PROP_OR(inst, phase_seg1, 0), \
	.ts2 = DT_INST_PROP_OR(inst, phase_seg2, 0),                                           \
	.phy = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, phys)),                             \
	.max_bitrate = DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(inst, 1000000),                     \
}
#endif /* CONFIG_CAN_FD_MODE */

#define CAN_SAM_CFG_INST(inst)                                                                 \
static const struct can_sam_config can_sam_cfg_##inst = {                                      \
	.pmc_id = DT_INST_PROP(inst, peripheral_id),                                           \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                          \
	.config_irq = config_can_##inst##_irq,                                                 \
	.mcan_cfg = CAN_SAM_MCAN_CFG(inst)                                                     \
};

#define CAN_SAM_DATA_INST(inst) static struct can_sam_data can_sam_dev_data_##inst;

#define CAN_SAM_DEVICE_INST(inst)                                                              \
DEVICE_DT_INST_DEFINE(inst, &can_sam_init, NULL, &can_sam_dev_data_##inst,                     \
		      &can_sam_cfg_##inst, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,              \
		      &can_api_funcs);

#define CAN_SAM_INST(inst)                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                          \
	CAN_SAM_IRQ_CFG_FUNCTION(inst)                                         \
	CAN_SAM_CFG_INST(inst)                                                 \
	CAN_SAM_DATA_INST(inst)                                                \
	CAN_SAM_DEVICE_INST(inst)

DT_INST_FOREACH_STATUS_OKAY(CAN_SAM_INST)
