/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Regulator driver for Infineon AutAnalog PRB voltage references.
 *
 * Each instance represents one of the two Vref outputs (Vref0, Vref1) within
 * a PRB block.  The driver provides discrete voltage selection via the 16-tap
 * resistive divider.  The available voltages depend on the selected source:
 *   - VBGR (0.9 V): 56250 to 900000 uV in 56250 uV steps
 *   - VDDA (1.8 V): 112500 to 1800000 uV in 112500 uV steps
 */

#define DT_DRV_COMPAT infineon_autanalog_prb_vref

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/regulator.h>
#include <infineon_autanalog_prb.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(regulator_ifx_autanalog_prb, CONFIG_REGULATOR_LOG_LEVEL);

/**
 * Source voltage in microvolts for each PRB source selector.
 * Index 0 = VBGR (0.9V), Index 1 = VDDA (1.8V)
 */
static const int32_t prb_source_uv[] = {
	900000,  /* CY_AUTANALOG_PRB_VBGR */
	1800000, /* CY_AUTANALOG_PRB_VDDA */
};

#define PRB_TAP_COUNT 16

struct regulator_ifx_prb_config {
	struct regulator_common_config common;
	const struct device *prb_mfd;
	uint8_t vref_idx;
	uint8_t initial_source;
	uint8_t initial_tap;
};

struct regulator_ifx_prb_data {
	struct regulator_common_data common;
	uint8_t source;
	uint8_t tap;
	bool enabled;
};

/**
 * @brief Convert source + tap to microvolts
 */
static int32_t prb_tap_to_uv(uint8_t source, uint8_t tap)
{
	/* Vref = ((tap + 1) / 16) * Vsrc */
	return (prb_source_uv[source] * (tap + 1)) / PRB_TAP_COUNT;
}

/**
 * @brief Find the tap value closest to the requested voltage
 *
 * @return tap value (0-15), or -1 if no valid tap exists within [min_uv, max_uv]
 */
static int prb_uv_to_tap(uint8_t source, int32_t min_uv, int32_t max_uv)
{
	for (uint8_t tap = 0; tap < PRB_TAP_COUNT; tap++) {
		int32_t uv = prb_tap_to_uv(source, tap);

		if (uv >= min_uv && uv <= max_uv) {
			return tap;
		}
	}

	return -1;
}

static int regulator_ifx_prb_enable(const struct device *dev)
{
	const struct regulator_ifx_prb_config *cfg = dev->config;
	struct regulator_ifx_prb_data *data = dev->data;

	data->enabled = true;

	return ifx_autanalog_prb_set_vref(cfg->prb_mfd, cfg->vref_idx, true, data->source,
					  data->tap);
}

static int regulator_ifx_prb_disable(const struct device *dev)
{
	const struct regulator_ifx_prb_config *cfg = dev->config;
	struct regulator_ifx_prb_data *data = dev->data;

	data->enabled = false;

	return ifx_autanalog_prb_set_vref(cfg->prb_mfd, cfg->vref_idx, false, data->source,
					  data->tap);
}

static unsigned int regulator_ifx_prb_count_voltages(const struct device *dev)
{
	ARG_UNUSED(dev);

	return PRB_TAP_COUNT;
}

static int regulator_ifx_prb_list_voltage(const struct device *dev, unsigned int idx,
					  int32_t *volt_uv)
{
	struct regulator_ifx_prb_data *data = dev->data;

	if (idx >= PRB_TAP_COUNT) {
		return -EINVAL;
	}

	*volt_uv = prb_tap_to_uv(data->source, (uint8_t)idx);

	return 0;
}

static int regulator_ifx_prb_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_ifx_prb_config *cfg = dev->config;
	struct regulator_ifx_prb_data *data = dev->data;
	int tap;

	tap = prb_uv_to_tap(data->source, min_uv, max_uv);
	if (tap < 0) {
		LOG_ERR("No valid tap for voltage range [%d, %d] uV", min_uv, max_uv);
		return -EINVAL;
	}

	data->tap = (uint8_t)tap;

	if (data->enabled) {
		return ifx_autanalog_prb_set_vref(cfg->prb_mfd, cfg->vref_idx, true, data->source,
						  data->tap);
	}

	return 0;
}

static int regulator_ifx_prb_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	struct regulator_ifx_prb_data *data = dev->data;

	*volt_uv = prb_tap_to_uv(data->source, data->tap);

	return 0;
}

static DEVICE_API(regulator, regulator_ifx_prb_api) = {
	.enable = regulator_ifx_prb_enable,
	.disable = regulator_ifx_prb_disable,
	.count_voltages = regulator_ifx_prb_count_voltages,
	.list_voltage = regulator_ifx_prb_list_voltage,
	.set_voltage = regulator_ifx_prb_set_voltage,
	.get_voltage = regulator_ifx_prb_get_voltage,
};

static int regulator_ifx_prb_init(const struct device *dev)
{
	const struct regulator_ifx_prb_config *cfg = dev->config;
	struct regulator_ifx_prb_data *data = dev->data;

	if (!device_is_ready(cfg->prb_mfd)) {
		LOG_ERR("PRB MFD device not ready");
		return -ENODEV;
	}

	regulator_common_data_init(dev);

	data->source = cfg->initial_source;
	data->tap = cfg->initial_tap;
	data->enabled = (cfg->common.flags & REGULATOR_INIT_ENABLED) != 0;

	return regulator_common_init(dev, data->enabled);
}

#define REGULATOR_IFX_PRB_DEFINE(n)                                                                \
	static struct regulator_ifx_prb_data regulator_ifx_prb_data_##n;                           \
                                                                                                   \
	static const struct regulator_ifx_prb_config regulator_ifx_prb_config_##n = {              \
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(n),                                 \
		.prb_mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                       \
		.vref_idx = (uint8_t)DT_INST_REG_ADDR(n),                                          \
		.initial_source = (uint8_t)DT_INST_PROP(n, source),                                \
		.initial_tap = (uint8_t)DT_INST_PROP(n, tap),                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &regulator_ifx_prb_init, NULL, &regulator_ifx_prb_data_##n,       \
			      &regulator_ifx_prb_config_##n, POST_KERNEL,                          \
			      CONFIG_REGULATOR_INFINEON_AUTANALOG_PRB_INIT_PRIORITY,               \
			      &regulator_ifx_prb_api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_IFX_PRB_DEFINE)
