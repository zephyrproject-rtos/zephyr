/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2021 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include "can_mcan.h"

LOG_MODULE_REGISTER(can_sam, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT atmel_sam_can

struct can_sam_config {
	void (*config_irq)(void);
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	int divider;
};

struct can_sam_data {
	struct can_mcan_msg_sram msg_ram;
};

static int can_sam_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_sam_config *sam_cfg = mcan_cfg->custom;

	*rate = SOC_ATMEL_SAM_UPLLCK_FREQ_HZ / (sam_cfg->divider);

	return 0;
}

static void can_sam_clock_enable(const struct can_sam_config *sam_cfg)
{
	REG_PMC_PCK5 = PMC_PCK_CSS_UPLL_CLK | PMC_PCK_PRES(sam_cfg->divider - 1);
	PMC->PMC_SCER |= PMC_SCER_PCK5;

	/* Enable CAN clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&sam_cfg->clock_cfg);
}

static int can_sam_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_sam_config *sam_cfg = mcan_cfg->custom;
	int ret;

	can_sam_clock_enable(sam_cfg);

	ret = pinctrl_apply_state(sam_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret != 0) {
		return ret;
	}

	sam_cfg->config_irq();

	return ret;
}

static const struct can_driver_api can_sam_driver_api = {
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
	.get_core_clock = can_sam_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.get_max_bitrate = can_mcan_get_max_bitrate,
	.set_state_change_callback =  can_mcan_set_state_change_callback,
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

#define CAN_SAM_IRQ_CFG_FUNCTION(inst)                                                         \
static void config_can_##inst##_irq(void)                                                      \
{                                                                                              \
	LOG_DBG("Enable CAN##inst## IRQ");                                                     \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, line_0, irq),                                    \
		    DT_INST_IRQ_BY_NAME(inst, line_0, priority), can_mcan_line_0_isr,          \
					DEVICE_DT_INST_GET(inst), 0);                          \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, line_0, irq));                                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, line_1, irq),                                    \
		    DT_INST_IRQ_BY_NAME(inst, line_1, priority), can_mcan_line_1_isr,          \
					DEVICE_DT_INST_GET(inst), 0);                          \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, line_1, irq));                                    \
}

#define CAN_SAM_CFG_INST(inst)						\
	static const struct can_sam_config can_sam_cfg_##inst = {	\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(inst),		\
		.divider = DT_INST_PROP(inst, divider),			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		\
		.config_irq = config_can_##inst##_irq,			\
	};								\
									\
	static const struct can_mcan_config can_mcan_cfg_##inst =	\
		CAN_MCAN_DT_CONFIG_INST_GET(inst, &can_sam_cfg_##inst);

#define CAN_SAM_DATA_INST(inst)						\
	static struct can_sam_data can_sam_data_##inst;			\
									\
	static struct can_mcan_data can_mcan_data_##inst =		\
		CAN_MCAN_DATA_INITIALIZER(&can_sam_data_##inst.msg_ram, \
					  &can_sam_data_##inst);	\

#define CAN_SAM_DEVICE_INST(inst)					\
	DEVICE_DT_INST_DEFINE(inst, &can_sam_init, NULL,		\
			      &can_mcan_data_##inst,			\
			      &can_mcan_cfg_##inst,			\
			      POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,	\
			      &can_sam_driver_api);

#define CAN_SAM_INST(inst)                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                          \
	CAN_SAM_IRQ_CFG_FUNCTION(inst)                                         \
	CAN_SAM_CFG_INST(inst)                                                 \
	CAN_SAM_DATA_INST(inst)                                                \
	CAN_SAM_DEVICE_INST(inst)

DT_INST_FOREACH_STATUS_OKAY(CAN_SAM_INST)
