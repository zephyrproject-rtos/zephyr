/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_tsen

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <register.h>

#define TSEN_CTRL_REG offsetof(TSEN_TypeDef, TSEN_CTRL_REG)
#define TSEN_RDATA    offsetof(TSEN_TypeDef, TSEN_RDATA)
#define TSEN_IRQ      offsetof(TSEN_TypeDef, TSEN_IRQ)

#define SYS_CFG_ANAU_CR offsetof(HPSYS_CFG_TypeDef, ANAU_CR)

LOG_MODULE_REGISTER(sf32lb_tsen, CONFIG_SENSOR_LOG_LEVEL);

struct sf32lb_tsen_config {
	uintptr_t base;
	uintptr_t cfg_base;
	struct sf32lb_clock_dt_spec clock;
};

struct sf32lb_tsen_data {
	struct k_mutex mutex;
	uint32_t last_temp;
};

static int sf32lb_tsen_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct sf32lb_tsen_config *config = dev->config;
	struct sf32lb_tsen_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);

	while (!sys_test_bit(config->base + TSEN_IRQ, TSEN_TSEN_IRQ_TSEN_IRSR_Pos)) {
		k_msleep(1);
	}

	data->last_temp = sys_read32(config->base + TSEN_RDATA);

	sys_set_bit(config->base + TSEN_IRQ, TSEN_TSEN_IRQ_TSEN_ICR_Pos);

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int sf32lb_tsen_channel_get(const struct device *dev, enum sensor_channel chan,
				   struct sensor_value *val)
{
	struct sf32lb_tsen_data *data = dev->data;
	float temp;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	temp = ((int32_t)data->last_temp + 3000) * 749.2916 / 10100 - 277; /* see manual 8.2.3.2 */

	return sensor_value_from_float(val, temp);
}

static DEVICE_API(sensor, sf32lb_tsen_driver_api) = {
	.sample_fetch = sf32lb_tsen_sample_fetch,
	.channel_get = sf32lb_tsen_channel_get,
};

static int sf32lb_tsen_init(const struct device *dev)
{
	const struct sf32lb_tsen_config *config = dev->config;
	struct sf32lb_tsen_data *data = dev->data;
	int ret;

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	ret = sf32lb_clock_control_on_dt(&config->clock);
	if (ret < 0) {
		return ret;
	}

	if (!sys_test_bit(config->cfg_base + SYS_CFG_ANAU_CR, HPSYS_CFG_ANAU_CR_EN_BG_Pos)) {
		sys_set_bit(config->cfg_base + SYS_CFG_ANAU_CR, HPSYS_CFG_ANAU_CR_EN_BG_Pos);
	}

	sys_clear_bit(config->base + TSEN_CTRL_REG, TSEN_TSEN_CTRL_REG_ANAU_TSEN_RSTB_Pos);
	sys_set_bit(config->base + TSEN_CTRL_REG, TSEN_TSEN_CTRL_REG_ANAU_TSEN_EN_Pos);
	sys_set_bit(config->base + TSEN_CTRL_REG, TSEN_TSEN_CTRL_REG_ANAU_TSEN_PU_Pos);
	sys_set_bit(config->base + TSEN_CTRL_REG, TSEN_TSEN_CTRL_REG_ANAU_TSEN_RSTB_Pos);
	k_busy_wait(20);
	sys_set_bit(config->base + TSEN_CTRL_REG, TSEN_TSEN_CTRL_REG_ANAU_TSEN_RUN_Pos);

	k_mutex_init(&data->mutex);

	return ret;
}

#define SF32LB_TSEN_DEFINE(inst)                                                                   \
	static struct sf32lb_tsen_data sf32lb_tsen_data_##inst;                                    \
	static const struct sf32lb_tsen_config sf32lb_tsen_config_##inst = {                       \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.cfg_base = DT_REG_ADDR(DT_INST_PHANDLE(inst, sifli_cfg)),                         \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(inst),                                      \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sf32lb_tsen_init, NULL, &sf32lb_tsen_data_##inst,       \
				     &sf32lb_tsen_config_##inst, POST_KERNEL,                      \
				     CONFIG_SENSOR_INIT_PRIORITY, &sf32lb_tsen_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_TSEN_DEFINE)
