/*
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/can.h>
#include <drivers/can/transceiver.h>
#include <drivers/pinctrl.h>
#include <kernel.h>
#include <soc.h>
#include <stm32_ll_rcc.h>
#include <logging/log.h>

#include "can_stm32fd.h"

LOG_MODULE_REGISTER(can_stm32fd, CONFIG_CAN_LOG_LEVEL);

#if defined(CONFIG_CAN_STM32FD_CLOCK_SOURCE_HSE)
#define CAN_STM32FD_CLOCK_SOURCE LL_RCC_FDCAN_CLKSOURCE_HSE
#elif defined(CONFIG_CAN_STM32FD_CLOCK_SOURCE_PLL)
#define CAN_STM32FD_CLOCK_SOURCE LL_RCC_FDCAN_CLKSOURCE_PLL
#elif defined(CONFIG_CAN_STM32FD_CLOCK_SOURCE_PCLK1)
#define CAN_STM32FD_CLOCK_SOURCE LL_RCC_FDCAN_CLKSOURCE_PCLK1
#else
#error "Unsupported FDCAN clock source"
#endif

#ifdef CONFIG_CAN_STM32FD_CLOCK_DIVISOR
#if CONFIG_CAN_STM32FD_CLOCK_DIVISOR != 1 && CONFIG_CAN_STM32FD_CLOCK_DIVISOR & 0x01
#error CAN_STM32FD_CLOCK_DIVISOR invalid. Allowed values are 1 or 2 * n, where n <= 15.
#else
#define CAN_STM32FD_CLOCK_DIVISOR CONFIG_CAN_STM32FD_CLOCK_DIVISOR
#endif /* CONFIG_CAN_STM32FD_CLOCK_DIVISOR */
#else
#define CAN_STM32FD_CLOCK_DIVISOR 1U
#endif /* CONFIG_CAN_STM32FD_CLOCK_DIVISOR*/

#define DT_DRV_COMPAT st_stm32_fdcan

static int can_stm32fd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);
	const uint32_t rate_tmp = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE);

	if (rate_tmp == LL_RCC_PERIPH_FREQUENCY_NO) {
		LOG_ERR("Can't read core clock");
		return -EIO;
	}

	*rate = rate_tmp / CAN_STM32FD_CLOCK_DIVISOR;

	return 0;
}

static void can_stm32fd_clock_enable(void)
{
	LL_RCC_SetFDCANClockSource(CAN_STM32FD_CLOCK_SOURCE);
	__HAL_RCC_FDCAN_CLK_ENABLE();

	FDCAN_CONFIG->CKDIV = CAN_STM32FD_CLOCK_DIVISOR >> 1;
}

static void can_stm32fd_set_state_change_callback(const struct device *dev,
						  can_state_change_callback_t cb,
						  void *user_data)
{
	struct can_stm32fd_data *data = dev->data;

	data->mcan_data.state_change_cb = cb;
	data->mcan_data.state_change_cb_data = user_data;
}

static int can_stm32fd_init(const struct device *dev)
{
	const struct can_stm32fd_config *cfg = dev->config;
	struct can_stm32fd_data *data = dev->data;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_mcan_data *mcan_data = &data->mcan_data;
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

static int can_stm32fd_get_state(const struct device *dev, enum can_state *state,
				 struct can_bus_err_cnt *err_cnt)
{
	const struct can_stm32fd_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_get_state(mcan_cfg, state, err_cnt);
}

static int can_stm32fd_send(const struct device *dev, const struct zcan_frame *frame,
			    k_timeout_t timeout, can_tx_callback_t callback,
			    void *user_data)
{
	const struct can_stm32fd_config *cfg = dev->config;
	struct can_stm32fd_data *data = dev->data;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	return can_mcan_send(mcan_cfg, mcan_data, msg_ram, frame, timeout,
			     callback, user_data);
}

static int can_stm32fd_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
				     void *user_data, const struct zcan_filter *filter)
{
	const struct can_stm32fd_config *cfg = dev->config;
	struct can_stm32fd_data *data = dev->data;
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	return can_mcan_add_rx_filter(mcan_data, msg_ram, callback, user_data, filter);
}

static void can_stm32fd_remove_rx_filter(const struct device *dev, int filter_id)
{
	const struct can_stm32fd_config *cfg = dev->config;
	struct can_stm32fd_data *data = dev->data;
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	can_mcan_remove_rx_filter(mcan_data, msg_ram, filter_id);
}

static int can_stm32fd_set_mode(const struct device *dev, enum can_mode mode)
{
	const struct can_stm32fd_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_set_mode(mcan_cfg, mode);
}

static int can_stm32fd_set_timing(const struct device *dev,
				  const struct can_timing *timing,
				  const struct can_timing *timing_data)
{
	const struct can_stm32fd_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;

	return can_mcan_set_timing(mcan_cfg, timing, timing_data);
}

static int can_stm32fd_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate)
{
	const struct can_stm32fd_config *cfg = dev->config;

	*max_bitrate = cfg->mcan_cfg.max_bitrate;

	return 0;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static int can_stm32fd_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_stm32fd_config *cfg = dev->config;

	return can_mcan_recover(&cfg->mcan_cfg, timeout);
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

static void can_stm32fd_line_0_isr(const struct device *dev)
{
	const struct can_stm32fd_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_stm32fd_data *data = dev->data;
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	can_mcan_line_0_isr(mcan_cfg, msg_ram, mcan_data);
}

static void can_stm32fd_line_1_isr(const struct device *dev)
{
	const struct can_stm32fd_config *cfg = dev->config;
	const struct can_mcan_config *mcan_cfg = &cfg->mcan_cfg;
	struct can_stm32fd_data *data = dev->data;
	struct can_mcan_data *mcan_data = &data->mcan_data;
	struct can_mcan_msg_sram *msg_ram = cfg->msg_sram;

	can_mcan_line_1_isr(mcan_cfg, msg_ram, mcan_data);
}

static const struct can_driver_api can_api_funcs = {
	.set_mode = can_stm32fd_set_mode,
	.set_timing = can_stm32fd_set_timing,
	.send = can_stm32fd_send,
	.add_rx_filter = can_stm32fd_add_rx_filter,
	.remove_rx_filter = can_stm32fd_remove_rx_filter,
	.get_state = can_stm32fd_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_stm32fd_recover,
#endif
	.get_core_clock = can_stm32fd_get_core_clock,
	.get_max_bitrate = can_stm32fd_get_max_bitrate,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback = can_stm32fd_set_state_change_callback,
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
PINCTRL_DT_INST_DEFINE(inst);						       \
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
			DT_INST_PROP(inst, tx_delay_comp_offset),              \
		.phy = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, phys)),     \
		.max_bitrate =                                                 \
			DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(inst, 5000000),    \
	},                                                                     \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                          \
};

#else /* CONFIG_CAN_FD_MODE */

#define CAN_STM32FD_CFG_INST(inst)                                             \
									       \
PINCTRL_DT_INST_DEFINE(inst);						       \
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
		.phy = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, phys)),     \
		.max_bitrate =                                                 \
			DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(inst, 1000000),    \
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
