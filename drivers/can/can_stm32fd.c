/*
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/can.h>
#include <drivers/pinctrl.h>
#include <kernel.h>
#include <soc.h>
#include <stm32_ll_rcc.h>
#include "can_stm32fd.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(can_driver, CONFIG_CAN_LOG_LEVEL);

#if CONFIG_CAN_STM32_CLOCK_DIVISOR != 1 && CONFIG_CAN_STM32_CLOCK_DIVISOR & 0x01
#error CAN_STM32_CLOCK_DIVISOR invalid.\
Allowed values are 1 or 2 * n, where n <= 15
#endif

#define DT_DRV_COMPAT st_stm32_fdcan

int can_stm32fd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);
	int rate_tmp;

	rate_tmp = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE);

	if (rate_tmp == LL_RCC_PERIPH_FREQUENCY_NO) {
		LOG_ERR("Can't read core clock");
		return -EIO;
	}

	*rate = rate_tmp / CONFIG_CAN_STM32_CLOCK_DIVISOR;

	return 0;
}

int can_stm32fd_get_max_filters(const struct device *dev, enum can_ide id_type)
{
	if (id_type == CAN_STANDARD_IDENTIFIER) {
		return NUM_STD_FILTER_DATA;
	} else {
		return NUM_EXT_FILTER_DATA;
	}
}

void can_stm32fd_clock_enable(void)
{
	LL_RCC_SetFDCANClockSource(LL_RCC_FDCAN_CLKSOURCE_PCLK1);
	__HAL_RCC_FDCAN_CLK_ENABLE();

	FDCAN_CONFIG->CKDIV = CONFIG_CAN_STM32_CLOCK_DIVISOR >> 1;
}

void can_stm32fd_register_state_change_isr(const struct device *dev,
					   can_state_change_isr_t isr)
{
	struct can_stm32fd_data *data = DEV_DATA(dev);

	data->mcan_data.state_change_isr = isr;
}

static int can_stm32fd_init(const struct device *dev)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_mcan_data *mcan_data = &DEV_DATA(dev)->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("CAN pinctrl setup failed (%d)", ret);
		return ret;
	}

	can_stm32fd_clock_enable();
	ret = can_mcan_init(dev, mcan_cfg, msg_ram, mcan_data);
	if (ret) {
		return ret;
	}

	cfg->config_irq();

	return ret;
}

enum can_state can_stm32fd_get_state(const struct device *dev,
				     struct can_bus_err_cnt *err_cnt)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_get_state(mcan_cfg, err_cnt);
}

int can_stm32fd_send(const struct device *dev, const struct zcan_frame *frame,
		     k_timeout_t timeout, can_tx_callback_t callback,
		     void *user_data)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_mcan_data *mcan_data = &DEV_DATA(dev)->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	return can_mcan_send(mcan_cfg, mcan_data, msg_ram, frame, timeout,
			     callback, user_data);
}

int can_stm32fd_attach_isr(const struct device *dev, can_rx_callback_t isr,
			   void *cb_arg, const struct zcan_filter *filter)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	struct can_mcan_data *mcan_data = &DEV_DATA(dev)->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	return can_mcan_attach_isr(mcan_data, msg_ram, isr, cb_arg, filter);
}

void can_stm32fd_detach(const struct device *dev, int filter_nr)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	struct can_mcan_data *mcan_data = &DEV_DATA(dev)->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	can_mcan_detach(mcan_data, msg_ram, filter_nr);
}

int can_stm32fd_set_mode(const struct device *dev, enum can_mode mode)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_set_mode(mcan_cfg, mode);
}

int can_stm32fd_set_timing(const struct device *dev,
			   const struct can_timing *timing,
			   const struct can_timing *timing_data)
{
	const struct can_stm32fd_config *cfg = DEV_CFG(dev);
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_set_timing(mcan_cfg, timing, timing_data);
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
	.set_mode = can_stm32fd_set_mode,
	.set_timing = can_stm32fd_set_timing,
	.send = can_stm32fd_send,
	.attach_isr = can_stm32fd_attach_isr,
	.detach = can_stm32fd_detach,
	.get_state = can_stm32fd_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_mcan_recover,
#endif
	.get_core_clock = can_stm32fd_get_core_clock,
	.get_max_filters = can_stm32fd_get_max_filters,
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

#define CAN_STM32FD_IRQ_CFG_FUNCTION(inst)                                     \
static void config_can_##inst##_irq(void)                                      \
{                                                                              \
	LOG_DBG("Enable CAN" #inst " IRQ");                                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, line_0, irq),                    \
		    DT_INST_IRQ_BY_NAME(inst, line_0, priority),               \
		    can_stm32fd_line_0_isr, DEVICE_DT_INST_GET(inst), 0);      \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, line_0, irq));                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, line_1, irq),                    \
		    DT_INST_IRQ_BY_NAME(inst, line_1, priority),               \
		    can_stm32fd_line_1_isr, DEVICE_DT_INST_GET(inst), 0);      \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, line_1, irq));                    \
}

#ifdef CONFIG_CAN_FD_MODE

#define CAN_STM32FD_CFG_INST(inst)                                             \
									       \
PINCTRL_DT_INST_DEFINE(inst)						       \
									       \
static const struct can_stm32fd_config can_stm32fd_cfg_##inst = {              \
	.msg_sram = (struct can_mcan_msg_sram *)                               \
			DT_INST_REG_ADDR_BY_NAME(inst, message_ram),           \
	.config_irq = config_can_##inst##_irq,                                 \
	.mcan_cfg = {                                                          \
		.can = (struct can_mcan_reg *)                                 \
			DT_INST_REG_ADDR_BY_NAME(inst, m_can),                 \
		.bus_speed = DT_INST_PROP(inst, bus_speed),                    \
		.sjw = DT_INST_PROP(inst, sjw),                                \
		.sample_point = DT_INST_PROP_OR(inst, sample_point, 0),        \
		.prop_ts1 = DT_INST_PROP_OR(inst, prop_seg, 0) +               \
			    DT_INST_PROP_OR(inst, phase_seg1, 0),              \
		.ts2 = DT_INST_PROP_OR(inst, phase_seg2, 0),                   \
		.bus_speed_data = DT_INST_PROP(inst, bus_speed_data),          \
		.sjw_data = DT_INST_PROP(inst, sjw_data),                      \
		.sample_point_data =                                           \
			DT_INST_PROP_OR(inst, sample_point_data, 0),           \
		.prop_ts1_data = DT_INST_PROP_OR(inst, prop_seg_data, 0) +     \
				DT_INST_PROP_OR(inst, phase_seg1_data, 0),     \
		.ts2_data = DT_INST_PROP_OR(inst, phase_seg2_data, 0),         \
		.tx_delay_comp_offset =                                        \
			DT_INST_PROP(inst, tx_delay_comp_offset)               \
	},                                                                     \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                          \
};

#else /* CONFIG_CAN_FD_MODE */

#define CAN_STM32FD_CFG_INST(inst)                                             \
									       \
PINCTRL_DT_INST_DEFINE(inst)						       \
									       \
static const struct can_stm32fd_config can_stm32fd_cfg_##inst = {              \
	.msg_sram = (struct can_mcan_msg_sram *)                               \
			DT_INST_REG_ADDR_BY_NAME(inst, message_ram),           \
	.config_irq = config_can_##inst##_irq,                                 \
	.mcan_cfg = {                                                          \
		.can = (struct can_mcan_reg *)                                 \
			DT_INST_REG_ADDR_BY_NAME(inst, m_can),                 \
		.bus_speed = DT_INST_PROP(inst, bus_speed),                    \
		.sjw = DT_INST_PROP(inst, sjw),                                \
		.sample_point = DT_INST_PROP_OR(inst, sample_point, 0),        \
		.prop_ts1 = DT_INST_PROP_OR(inst, prop_seg, 0) +               \
			    DT_INST_PROP_OR(inst, phase_seg1, 0),              \
		.ts2 = DT_INST_PROP_OR(inst, phase_seg2, 0),                   \
	},                                                                     \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                          \
};

#endif /* CONFIG_CAN_FD_MODE */

#define CAN_STM32FD_DATA_INST(inst) \
static struct can_stm32fd_data can_stm32fd_dev_data_##inst;

#define CAN_STM32FD_DEVICE_INST(inst)                                          \
DEVICE_DT_INST_DEFINE(inst, &can_stm32fd_init, NULL,                           \
		      &can_stm32fd_dev_data_##inst, &can_stm32fd_cfg_##inst,   \
		      POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,                   \
		      &can_api_funcs);

#define CAN_STM32FD_INST(inst)     \
CAN_STM32FD_IRQ_CFG_FUNCTION(inst) \
CAN_STM32FD_CFG_INST(inst)         \
CAN_STM32FD_DATA_INST(inst)        \
CAN_STM32FD_DEVICE_INST(inst)

DT_INST_FOREACH_STATUS_OKAY(CAN_STM32FD_INST)
