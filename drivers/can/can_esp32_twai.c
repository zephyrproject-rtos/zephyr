/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_twai

#include "can_sja1000.h"

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(can_esp32_twai, CONFIG_CAN_LOG_LEVEL);

struct can_esp32_twai_config {
	mm_reg_t base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	int irq_source;
};

static uint8_t can_esp32_twai_read_reg(const struct device *dev, uint8_t reg)
{
	const struct can_sja1000_config *sja1000_config = dev->config;
	const struct can_esp32_twai_config *twai_config = sja1000_config->custom;
	mm_reg_t addr = twai_config->base + reg * sizeof(uint32_t);

	return sys_read32(addr) & 0xFF;
}

static void can_esp32_twai_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct can_sja1000_config *sja1000_config = dev->config;
	const struct can_esp32_twai_config *twai_config = sja1000_config->custom;
	mm_reg_t addr = twai_config->base + reg * sizeof(uint32_t);

	sys_write32(val & 0xFF, addr);
}

static int can_esp32_twai_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	/* The internal clock operates at half of the oscillator frequency */
	*rate = APB_CLK_FREQ / 2;

	return 0;
}

static void IRAM_ATTR can_esp32_twai_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;

	can_sja1000_isr(dev);
}

static int can_esp32_twai_init(const struct device *dev)
{
	const struct can_sja1000_config *sja1000_config = dev->config;
	const struct can_esp32_twai_config *twai_config = sja1000_config->custom;
	int err;

	if (!device_is_ready(twai_config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = pinctrl_apply_state(twai_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		LOG_ERR("failed to configure TWAI pins (err %d)", err);
		return err;
	}

	err = clock_control_on(twai_config->clock_dev, twai_config->clock_subsys);
	if (err != 0) {
		LOG_ERR("failed to enable CAN clock (err %d)", err);
		return err;
	}

	err = can_sja1000_init(dev);
	if (err != 0) {
		LOG_ERR("failed to initialize controller (err %d)", err);
		return err;
	}

	esp_intr_alloc(twai_config->irq_source, 0, can_esp32_twai_isr, (void *)dev, NULL);

	return 0;
}

const struct can_driver_api can_esp32_twai_driver_api = {
	.get_capabilities = can_sja1000_get_capabilities,
	.start = can_sja1000_start,
	.stop = can_sja1000_stop,
	.set_mode = can_sja1000_set_mode,
	.set_timing = can_sja1000_set_timing,
	.send = can_sja1000_send,
	.add_rx_filter = can_sja1000_add_rx_filter,
	.remove_rx_filter = can_sja1000_remove_rx_filter,
	.get_state = can_sja1000_get_state,
	.set_state_change_callback = can_sja1000_set_state_change_callback,
	.get_core_clock = can_esp32_twai_get_core_clock,
	.get_max_filters = can_sja1000_get_max_filters,
	.get_max_bitrate = can_sja1000_get_max_bitrate,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_sja1000_recover,
#endif /* !CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
	.timing_min = CAN_SJA1000_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_SJA1000_TIMING_MAX_INITIALIZER,
};

#define CAN_ESP32_TWAI_DT_CDR_INST_GET(inst)                                                       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clkout_divider),                                   \
		    (UTIL_CAT(CAN_SJA1000_CDR_CD_DIV, DT_INST_PROP(inst, clkout_divider))),        \
		    (CAN_SJA1000_CDR_CLOCK_OFF))

#define CAN_ESP32_TWAI_INIT(inst)                                                                  \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct can_esp32_twai_config can_esp32_twai_config_##inst = {                 \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, offset),         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.irq_source = DT_INST_IRQN(inst),                                                  \
	};                                                                                         \
	static const struct can_sja1000_config can_sja1000_config_##inst =                         \
		CAN_SJA1000_DT_CONFIG_INST_GET(inst, &can_esp32_twai_config_##inst,                \
					       can_esp32_twai_read_reg, can_esp32_twai_write_reg,  \
					       CAN_SJA1000_OCR_OCMODE_BIPHASE,                     \
					       CAN_ESP32_TWAI_DT_CDR_INST_GET(inst));              \
                                                                                                   \
	static struct can_sja1000_data can_sja1000_data_##inst =                                   \
		CAN_SJA1000_DATA_INITIALIZER(NULL);                                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, can_esp32_twai_init, NULL, &can_sja1000_data_##inst,           \
			      &can_sja1000_config_##inst, POST_KERNEL,                             \
			      CONFIG_CAN_INIT_PRIORITY, &can_esp32_twai_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_ESP32_TWAI_INIT)
