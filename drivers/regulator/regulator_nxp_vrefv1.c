/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_vrefv1

#include <zephyr/device.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/regulator/nxp_vref.h>
#include <fsl_device_registers.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <errno.h>

LOG_MODULE_REGISTER(nxp_vrefv1, CONFIG_REGULATOR_LOG_LEVEL);

/*
 * Expose 1 mV software steps while hardware TRIM uses 0.5 mV per LSB.
 * Map SW index [0..31] to HW TRIM value [0..62] by TRIM = idx * 2.
 */
static const struct linear_range utrim_range =
		LINEAR_RANGE_INIT(1175500, 1000, 0x0, 0x1F);

struct nxp_vref_data {
	struct regulator_common_data common;
};

struct nxp_vref_config {
	struct regulator_common_config common;
	VREF_Type *base;
	uint16_t bandgap_startup_time_us;
	uint16_t buffer_startup_delay_us;
	bool chop_oscillator_en;
	bool current_compensation_en;
	bool regulator_en;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

static int nxp_vref_enable(const struct device *dev)
{
	const struct nxp_vref_config *config = dev->config;

	/* Enable VREF module */
	config->base->SC |= (uint8_t)VREF_SC_VREFEN_MASK;

	/* Wait bandgap startup time */
	if (config->bandgap_startup_time_us) {
		k_sleep(K_USEC(config->bandgap_startup_time_us));
	}

	/* Wait until internal voltage stable, this condition
	 * is valid only when the chop oscillator is not being used.
	 */
	if (!config->chop_oscillator_en) {
		const int64_t deadline = k_uptime_get() +
			CONFIG_REGULATOR_NXP_VREFV1_READY_TIMEOUT_MS;
		while ((config->base->SC & VREF_SC_VREFST_MASK) == 0U) {
			if (k_uptime_get() >= deadline) {
				LOG_ERR("VREF ready timeout");
				return -ETIMEDOUT;
			}

			/* Yield for a short period to avoid spinning */
			k_msleep(1);
		}
	}

	return 0;
}

static int nxp_vref_disable(const struct device *dev)
{
	const struct nxp_vref_config *config = dev->config;
	VREF_Type *const base = config->base;

	/* Disable VREF module */
	base->SC &= (uint8_t)~VREF_SC_VREFEN_MASK;

	return 0;
}

static int nxp_vref_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct nxp_vref_config *config = dev->config;

	if (!((mode == NXP_VREF_MODE_STANDBY) || (mode == NXP_VREF_MODE_LOW_POWER) ||
		(mode == NXP_VREF_MODE_HIGH_POWER))) {
		return -EINVAL;
	}

	/* Set VREF mode */
	config->base->SC = ((config->base->SC & (uint8_t)~VREF_SC_MODE_LV_MASK) |
			VREF_SC_MODE_LV(mode));

	/* Wait buffer startup delay */
	if (mode != NXP_VREF_MODE_STANDBY && config->buffer_startup_delay_us) {
		k_sleep(K_USEC(config->buffer_startup_delay_us));
	}

	return 0;
}

static int nxp_vref_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct nxp_vref_config *config = dev->config;

	*mode = (regulator_mode_t)((config->base->SC & VREF_SC_MODE_LV_MASK) >>
				VREF_SC_MODE_LV_SHIFT);

	return 0;
}

static unsigned int nxp_vref_count_voltages(const struct device *dev)
{
	return linear_range_values_count(&utrim_range);
}

static int nxp_vref_list_voltage(const struct device *dev,
					unsigned int idx, int32_t *volt_uv)
{
	return linear_range_get_value(&utrim_range, idx, volt_uv);
}

static int nxp_vref_set_voltage(const struct device *dev,
					int32_t min_uv, int32_t max_uv)
{
	const struct nxp_vref_config *config = dev->config;
	uint16_t sw_idx;
	int ret;

	ret = linear_range_get_win_index(&utrim_range, min_uv, max_uv, &sw_idx);
	if ((ret < 0) && (ret != -ERANGE)) {
		return ret;
	}

	/* Map 1 mV SW step to 0.5 mV HW step */
	uint16_t hw_idx = (uint16_t)(sw_idx * 2U);
	uint16_t hw_max = (VREF_TRM_TRIM_MASK >> VREF_TRM_TRIM_SHIFT);

	if (hw_idx > hw_max) {
		hw_idx = hw_max;
	}

	uint8_t trm = config->base->TRM;

	trm &= (uint8_t)~VREF_TRM_TRIM_MASK;
	trm |= (uint8_t)(((uint16_t)hw_idx << VREF_TRM_TRIM_SHIFT) & VREF_TRM_TRIM_MASK);
	config->base->TRM = trm;

	return 0;
}

static int nxp_vref_get_voltage(const struct device *dev,
					int32_t *volt_uv)
{
	const struct nxp_vref_config *config = dev->config;
	uint16_t hw_idx;
	int ret;

	hw_idx = (uint16_t)((config->base->TRM & VREF_TRM_TRIM_MASK) >> VREF_TRM_TRIM_SHIFT);

	/* Convert to SW index (1 mV step). Round to nearest even TRIM (idx * 2). */
	uint16_t sw_idx = DIV_ROUND_UP(hw_idx, 2U);

	/* Clamp to SW range */
	if (sw_idx < utrim_range.min_idx) {
		sw_idx = utrim_range.min_idx;
	} else if (sw_idx > utrim_range.max_idx) {
		sw_idx = utrim_range.max_idx;
	}

	ret = linear_range_get_value(&utrim_range, sw_idx, volt_uv);

	return ret;
}

static DEVICE_API(regulator, api) = {
	.enable = nxp_vref_enable,
	.disable = nxp_vref_disable,
	.set_mode = nxp_vref_set_mode,
	.get_mode = nxp_vref_get_mode,
	.set_voltage = nxp_vref_set_voltage,
	.get_voltage = nxp_vref_get_voltage,
	.list_voltage = nxp_vref_list_voltage,
	.count_voltages = nxp_vref_count_voltages,
};

static int nxp_vref_init(const struct device *dev)
{
	const struct nxp_vref_config *config = dev->config;
	int ret;

	regulator_common_data_init(dev);

	ret = clock_control_on(config->clock_dev, config->clock_subsys);

	if (ret) {
		LOG_ERR("Device clock turn on failed");
		return ret;
	}

	/* Disable VREF module */
	config->base->SC &= (uint8_t)~VREF_SC_VREFEN_MASK;

	if (config->chop_oscillator_en) {
		config->base->TRM |= (uint8_t)VREF_TRM_CHOPEN_MASK;
	}

	if (config->current_compensation_en) {
		config->base->SC |= (uint8_t)VREF_SC_ICOMPEN_MASK;
	}

	if (config->regulator_en) {
		config->base->SC |= (uint8_t)VREF_SC_REGEN_MASK;
	}

	config->base->TRM &= (uint8_t)~VREF_TRM_TRIM_MASK;

	return regulator_common_init(dev, false);
}

#define NXP_VREF_DEFINE(inst)								\
	static struct nxp_vref_data data_##inst;					\
											\
	static const struct nxp_vref_config config_##inst = {				\
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),			\
		.base = (VREF_Type *)DT_INST_REG_ADDR(inst),				\
		.bandgap_startup_time_us = DT_INST_PROP(inst, bandgap_startup_time_us),	\
		.buffer_startup_delay_us = DT_INST_PROP(inst, buffer_startup_delay_us),	\
		.chop_oscillator_en = DT_INST_PROP(inst, chop_oscillator_en),		\
		.current_compensation_en = DT_INST_PROP(inst, current_compensation_en),	\
		.regulator_en = DT_INST_PROP(inst, internal_voltage_regulator_en),	\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, nxp_vref_init, NULL, &data_##inst, &config_##inst,	\
			POST_KERNEL, CONFIG_REGULATOR_NXP_VREFV1_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(NXP_VREF_DEFINE)
