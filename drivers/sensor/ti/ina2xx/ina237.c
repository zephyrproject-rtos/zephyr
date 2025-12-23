/*
 * Copyright 2021 Grinn
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina237.h"

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/ina237.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_DECLARE(INA2XX, CONFIG_SENSOR_LOG_LEVEL);

/** @brief INA237 calibration scaling value (scaled by 10^-5) */
#define INA237_CAL_SCALING 8192ULL

/** @brief INA228 calibration scaling value (scaled by 10^-5) */
#define INA228_CAL_SCALING (INA237_CAL_SCALING << 4)

INA2XX_REG_DEFINE(ina237_config, INA237_REG_CONFIG, 16);
INA2XX_REG_DEFINE(ina237_adc_config, INA237_REG_ADC_CONFIG, 16);
INA2XX_REG_DEFINE(ina237_shunt_cal, INA237_REG_CALIB, 16);
INA2XX_REG_DEFINE(ina237_mfr_id, INA237_REG_MANUFACTURER_ID, 16);

#if DT_HAS_COMPAT_STATUS_OKAY(ti_ina237)
INA2XX_CHANNEL_DEFINE(ina237_vshunt_standard, INA237_REG_SHUNT_VOLT, 16, 0, 5000, 1);
INA2XX_CHANNEL_DEFINE(ina237_vshunt_precise, INA237_REG_SHUNT_VOLT, 16, 0, 1250, 1);
INA2XX_CHANNEL_DEFINE(ina237_voltage, INA237_REG_BUS_VOLT, 16, 0, 3125, 1);
INA2XX_CHANNEL_DEFINE(ina237_die_temp, INA237_REG_DIETEMP, 16, 4, 125000, 1);
INA2XX_CHANNEL_DEFINE(ina237_current, INA237_REG_CURRENT, 16, 0, 1, 1);
INA2XX_CHANNEL_DEFINE(ina237_power, INA237_REG_POWER, 24, 0, 1, 5);

static struct ina2xx_channels ina237_channels = {
	.voltage = &ina237_voltage,
	.vshunt = &ina237_vshunt_standard,
	.current = &ina237_current,
	.power = &ina237_power,
	.die_temp = &ina237_die_temp,
};

static int ina237_get_vshunt(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	struct ina2xx_data *data = dev->data;
	int32_t value = (int16_t)sys_get_be16(data->vshunt);

	/* Use the high precision scaling for VSHUNT */
	if ((config->config & INA237_CFG_HIGH_PRECISION)) {
		value = (value * ina237_vshunt_precise.mult) / ina237_vshunt_precise.div;
	} else {
		value = (value * ina237_vshunt_standard.mult) / ina237_vshunt_standard.div;
	}

	return sensor_value_from_micro(val, value);
}

static int ina237_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	int ret;

	if (chan == SENSOR_CHAN_VSHUNT) {
		return ina237_get_vshunt(dev, val);
	}

	ret = ina2xx_channel_get(dev, chan, val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
#endif /* ti_ina237 */

#if DT_HAS_COMPAT_STATUS_OKAY(ti_ina228)
INA2XX_CHANNEL_DEFINE(ina228_vshunt_standard, INA237_REG_SHUNT_VOLT, 20, 4, 5, 16);
INA2XX_CHANNEL_DEFINE(ina228_vshunt_precise, INA237_REG_SHUNT_VOLT, 20, 4, 125, 1600);
INA2XX_CHANNEL_DEFINE(ina228_voltage, INA237_REG_BUS_VOLT, 20, 4, 3125, 16);
INA2XX_CHANNEL_DEFINE(ina228_die_temp, INA237_REG_DIETEMP, 16, 0, 125000, 16);
INA2XX_CHANNEL_DEFINE(ina228_current, INA237_REG_CURRENT, 20, 4, 1, 1);
INA2XX_CHANNEL_DEFINE(ina228_power, INA237_REG_POWER, 24, 0, 16, 5);
INA2XX_CHANNEL_DEFINE(ina228_energy, INA228_REG_ENERGY, 40, 0, 256, 5);
INA2XX_CHANNEL_DEFINE(ina228_charge, INA228_REG_CHARGE, 40, 0, 1, 1);

static struct ina2xx_channels ina228_channels = {
	.voltage = &ina228_voltage,
	.vshunt = &ina228_vshunt_standard,
	.current = &ina228_current,
	.power = &ina228_power,
	.die_temp = &ina228_die_temp,
	.energy = &ina228_energy,
	.charge = &ina228_charge,
};

static int ina228_get_vshunt(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->vshunt;
	struct ina2xx_data *data = dev->data;
	int32_t mult;
	int32_t divisor;
	union {
		uint32_t u32;
		int32_t s32;
	} value;

	value.u32 = sys_get_be24(data->vshunt) >> ch->shift;
	value.s32 = sign_extend(value.u32, (23 - ch->shift));

	/* Use the high precision scaling for VSHUNT */
	if ((config->config & INA237_CFG_HIGH_PRECISION)) {
		mult = ina228_vshunt_precise.mult;
		divisor = ina228_vshunt_precise.div;
	} else {
		mult = ina228_vshunt_standard.mult;
		divisor = ina228_vshunt_standard.div;
	}

	value.s32 = mult * value.s32 / divisor;

	return sensor_value_from_micro(val, value.s32);
}

static int ina228_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	int ret;

	if (chan == SENSOR_CHAN_VSHUNT) {
		return ina228_get_vshunt(dev, val);
	}

	ret = ina2xx_channel_get(dev, chan, val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/**
 * @brief Reset the INA228 accumulators.
 */
static int ina228_reset_accumulators(const struct device *dev)
{
	const struct ina2xx_config *config = dev->config;

	if ((IS_ENABLED(CONFIG_INA2XX_HAS_CHANNEL_ENERGY)) ||
	    (IS_ENABLED(CONFIG_INA2XX_HAS_CHANNEL_CHARGE))) {
		return ina2xx_reg_write(&config->bus, config->config_reg->addr, BIT(14));
	}

	return 0;
}
#endif /* ti_ina228 */

/**
 * @brief sensor operation mode check
 *
 * @retval true if set or false if not set
 */
static bool ina237_is_triggered_mode_set(const struct device *dev)
{
	const struct ina237_config *config = dev->config;
	const struct ina2xx_config *common = &config->common;

	uint8_t mode = (common->adc_config & GENMASK(15, 12)) >> 12;

	switch (mode) {
	case INA237_OPER_MODE_BUS_VOLTAGE_TRIG:
	case INA237_OPER_MODE_SHUNT_VOLTAGE_TRIG:
	case INA237_OPER_MODE_SHUNT_BUS_VOLTAGE_TRIG:
	case INA237_OPER_MODE_TEMP_TRIG:
	case INA237_OPER_MODE_TEMP_BUS_VOLTAGE_TRIG:
	case INA237_OPER_MODE_TEMP_SHUNT_VOLTAGE_TRIG:
	case INA237_OPER_MODE_BUS_SHUNT_VOLTAGE_TEMP_TRIG:
		return true;
	default:
		return false;
	}
}

/**
 * @brief request for one shot measurement
 *
 * @retval 0 for success
 * @retval negative errno code on fail
 */
static int ina237_trigg_one_shot_request(const struct device *dev, enum sensor_channel chan)
{
	const struct ina237_config *config = dev->config;
	const struct ina2xx_config *common = &config->common;
	struct ina237_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_CURRENT &&
		chan != SENSOR_CHAN_POWER &&
		chan != SENSOR_CHAN_VSHUNT &&
#if DT_HAS_COMPAT_STATUS_OKAY(ti_ina228)
		chan != (enum sensor_channel)SENSOR_CHAN_INA2XX_ENERGY &&
		chan != (enum sensor_channel)SENSOR_CHAN_INA2XX_CHARGE &&
#endif /* ti_ina228 */
		chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	data->chan = chan;

	ret = ina2xx_reg_write(&common->bus, INA237_REG_ADC_CONFIG, common->adc_config);
	if (ret < 0) {
		LOG_ERR("Failed to write ADC configuration register!");
		return ret;
	}

	return 0;
}

/**
 * @brief sensor sample fetch
 *
 * @retval 0 for success
 * @retval negative errno code on fail
 */
static int ina237_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret;

	if (ina237_is_triggered_mode_set(dev)) {
		ret = ina237_trigg_one_shot_request(dev, chan);
	} else {
		ret = ina2xx_sample_fetch(dev, chan);
	}

	if (ret < 0) {
		return ret;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(ti_ina228)
	if ((chan == SENSOR_CHAN_ALL) ||
		(chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_ENERGY) ||
		(chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_CHARGE)) {
		return ina228_reset_accumulators(dev);
	}
#endif /* ti_ina228 */

	return 0;
}

static void ina237_trigger_work_handler(struct k_work *work)
{
	struct ina2xx_trigger *trigg = CONTAINER_OF(work, struct ina2xx_trigger, conversion_work);
	struct ina237_data *data = CONTAINER_OF(trigg, struct ina237_data, trigger);
	const struct ina237_config *config = data->dev->config;
	const struct ina2xx_config *common = &config->common;
	int ret;
	uint16_t reg_alert;

	/* Read reg alert to clear alerts */
	ret = ina2xx_reg_read_16(&common->bus, INA237_REG_ALERT, &reg_alert);
	if (ret < 0) {
		LOG_ERR("Failed to read alert register!");
		return;
	}

	ret = ina2xx_sample_fetch(data->dev, data->chan);
	if (ret < 0) {
		LOG_WRN("Unable to read data, ret %d", ret);
	}

	if (data->trigger.handler_alert) {
		data->trigger.handler_alert(data->dev, data->trigger.trig_alert);
	}
}

static int ina237_init(const struct device *dev)
{
	struct ina237_data *data = dev->data;
	const struct ina237_config *config = dev->config;
	const struct ina2xx_config *common = &config->common;
	int ret;

	ret = ina2xx_init(dev);
	if (ret < 0) {
		return ret;
	}

	data->dev = dev;

	if (ina237_is_triggered_mode_set(dev)) {
		if ((config->alert_config & GENMASK(15, 14)) != GENMASK(15, 14)) {
			LOG_ERR("ALATCH and CNVR bits must be enabled in triggered mode!");
			return -ENODEV;
		}

		k_work_init(&data->trigger.conversion_work, ina237_trigger_work_handler);

		ret = ina2xx_trigger_mode_init(&data->trigger, &config->alert_gpio);
		if (ret < 0) {
			LOG_ERR("Failed to init trigger mode");
			return ret;
		}

		ret = ina2xx_reg_write(&common->bus, INA237_REG_ALERT, config->alert_config);
		if (ret < 0) {
			LOG_ERR("Failed to write alert configuration register!");
			return ret;
		}
	}

	return 0;
}

static int ina237_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			      sensor_trigger_handler_t handler)
{
	ARG_UNUSED(trig);
	struct ina237_data *ina237 = dev->data;

	if (!ina237_is_triggered_mode_set(dev)) {
		return -ENOTSUP;
	}

	ina237->trigger.handler_alert = handler;
	ina237->trigger.trig_alert = trig;

	return 0;
}

#if DT_HAS_COMPAT_STATUS_OKAY(ti_ina237)
static DEVICE_API(sensor, ina237_driver_api) = {
	.attr_set = ina2xx_attr_set,
	.attr_get = ina2xx_attr_get,
	.trigger_set = ina237_trigger_set,
	.sample_fetch = ina237_sample_fetch,
	.channel_get = ina237_channel_get,
};
#endif /* ti_ina237 */

#if DT_HAS_COMPAT_STATUS_OKAY(ti_ina228)
static DEVICE_API(sensor, ina228_driver_api) = {
	.attr_set = ina2xx_attr_set,
	.attr_get = ina2xx_attr_get,
	.trigger_set = ina237_trigger_set,
	.sample_fetch = ina237_sample_fetch,
	.channel_get = ina228_channel_get,
};
#endif /* ti_ina228 */

/* Shunt calibration must be muliplied by 4 if high-prevision mode is selected */
#define CAL_PRECISION_MULTIPLIER(inst) \
	((DT_INST_PROP_OR(inst, high_precision, 0)) * 3 + 1)

#define INA237_DT_ADC_CONFIG(inst)                             \
	(DT_INST_ENUM_IDX(inst, adc_mode) << 12) |                 \
	(DT_INST_ENUM_IDX(inst, vbus_conversion_time_us) << 9) |   \
	(DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us) << 6) | \
	(DT_INST_ENUM_IDX(inst, temp_conversion_time_us) << 3) |   \
	(DT_INST_ENUM_IDX(inst, avg_count))

#define INA237_DT_CAL(inst)                               \
	CAL_PRECISION_MULTIPLIER(inst) * INA237_CAL_SCALING * \
	DT_INST_PROP(inst, current_lsb_microamps) *           \
	DT_INST_PROP(inst, rshunt_micro_ohms) / 10000000ULL

#define INA237_DRIVER_INIT(inst)                                               \
	static struct ina237_data ina237_data_##inst;                              \
	static const struct ina237_config ina237_config_##inst = {                 \
		.common = {                                                            \
			.bus = I2C_DT_SPEC_INST_GET(inst),	                               \
			.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),          \
			.config = INA237_DT_CONFIG(inst),                                  \
			.adc_config = INA237_DT_ADC_CONFIG(inst),                          \
			.cal = INA237_DT_CAL(inst),                                        \
			.id_reg = &ina237_mfr_id,                                          \
			.config_reg = &ina237_config,                                      \
			.adc_config_reg = &ina237_adc_config,                              \
			.cal_reg = &ina237_shunt_cal,                                      \
			.channels = &ina237_channels,                                      \
		},                                                                     \
		.alert_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, alert_gpios, {0}),        \
		.alert_config = DT_INST_PROP_OR(inst, alert_config, 0x01),             \
	};                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina237_init, NULL,                     \
					&ina237_data_##inst, &ina237_config_##inst, POST_KERNEL,   \
				    CONFIG_SENSOR_INIT_PRIORITY, &ina237_driver_api);

#define INA228_DRIVER_INIT(inst)                                               \
	static struct ina237_data ina228_data_##inst;                              \
	static const struct ina237_config ina228_config_##inst = {                 \
		.common = {                                                            \
			.bus = I2C_DT_SPEC_INST_GET(inst),                                 \
			.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),          \
			.adc_config = INA237_DT_ADC_CONFIG(inst),                          \
			.cal = (INA237_DT_CAL(inst) * 16),                                 \
			.id_reg = &ina237_mfr_id,                                          \
			.config_reg = &ina237_config,                                      \
			.adc_config_reg = &ina237_adc_config,                              \
			.cal_reg = &ina237_shunt_cal,                                      \
			.channels = &ina228_channels,                                      \
		},                                                                     \
	};                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina237_init, NULL,                     \
				&ina228_data_##inst, &ina228_config_##inst, POST_KERNEL,       \
				CONFIG_SENSOR_INIT_PRIORITY, &ina228_driver_api);


#define DT_DRV_COMPAT ti_ina237
DT_INST_FOREACH_STATUS_OKAY(INA237_DRIVER_INIT)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT ti_ina228
DT_INST_FOREACH_STATUS_OKAY(INA228_DRIVER_INIT)
#undef DT_DRV_COMPAT
