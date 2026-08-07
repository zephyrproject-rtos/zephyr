/*
 * Copyright (c) 2026 Open Device Partnership and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp451

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/sensor/tmp451.h>

#include "tmp451.h"

LOG_MODULE_REGISTER(TMP451, CONFIG_SENSOR_LOG_LEVEL);

int tmp451_reg_update(const struct tmp451_config *cfg, uint8_t read_reg, uint8_t write_reg,
		      uint8_t mask, uint8_t val)
{
	uint8_t old_val;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, read_reg, &old_val);
	if (ret < 0) {
		LOG_ERR("Failed to read old value");
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, write_reg, (old_val & ~mask) | (val & mask));
	if (ret < 0) {
		LOG_ERR("Failed to write new value");
	}

	return ret;
}

static int tmp451_write_precise_temp(const struct tmp451_config *cfg, uint8_t high_reg,
				     uint8_t low_reg, uint16_t raw)
{
	int ret;

	ret = i2c_reg_write_byte_dt(&cfg->i2c, high_reg, raw >> 4);
	if (ret < 0) {
		LOG_ERR("Failed to write temperature high byte");
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, low_reg, (raw & 0x0F) << 4);
	if (ret < 0) {
		LOG_ERR("Failed to write temperature low byte");
	}

	return ret;
}

static int tmp451_read_precise_temp(const struct tmp451_config *cfg, uint8_t high_reg,
				    uint8_t low_reg, uint16_t *raw)
{
	uint8_t high_byte, low_byte;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, high_reg, &high_byte);
	if (ret < 0) {
		LOG_ERR("Failed to read temperature high byte");
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, low_reg, &low_byte);
	if (ret < 0) {
		LOG_ERR("Failed to read temperature low byte");
		return ret;
	}

	*raw = (high_byte << 4) | (low_byte >> 4);
	return 0;
}

static int tmp451_one_shot(const struct tmp451_config *cfg)
{
	int ret;

	/* Writing any arbitrary value will start a one-shot conversion */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, TMP451_REG_ONE_SHOT_START, 0);
	if (ret < 0) {
		LOG_ERR("Failed to write one-shot conversion register");
		return ret;
	}

	/* One-shot conversion time is fixed at ~34ms max regardless of
	 * the conversion rate register (which only affects idle time
	 * between conversions in continuous mode).
	 */
	k_msleep(TMP451_CONVERSION_TIME_MS);
	return 0;
}

static int tmp451_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tmp451_config *cfg = dev->config;
	struct tmp451_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP &&
	    chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/* One shot mode guarantees a fresh sample, otherwise we return the most recent sample */
	if (data->one_shot_mode) {
		ret = tmp451_one_shot(cfg);
		if (ret < 0) {
			LOG_ERR("Failed to perform one-shot conversion");
			return ret;
		}
	}

	/*
	 * We map "local" temp to "ambient" temp because local is typically
	 * used to measure the temperature around the sensor IC.
	 */
	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP) {
		ret = tmp451_read_precise_temp(cfg, TMP451_REG_LT_H, TMP451_REG_LT_L,
					       &data->local_raw);
		if (ret < 0) {
			LOG_ERR("Failed to read local temperature");
			return ret;
		}
	}

	/*
	 * We map "remote" temp to "die" temp because remote is typically
	 * used to measure the CPU/SoC die temperature.
	 */
	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_DIE_TEMP) {
		ret = tmp451_read_precise_temp(cfg, TMP451_REG_RT_H, TMP451_REG_RT_L,
					       &data->remote_raw);
		if (ret < 0) {
			LOG_ERR("Failed to read remote temperature");
			return ret;
		}
	}

	return 0;
}

static int tmp451_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct tmp451_data *data = dev->data;
	int64_t temp;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		temp = data->local_raw;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		temp = data->remote_raw;
		break;
	default:
		return -ENOTSUP;
	}

	/* We always operate in extended mode, so apply the offset */
	temp = (temp - TMP451_EXTENDED_RANGE_OFFSET) * TMP451_TEMP_SCALE;
	return sensor_value_from_micro(val, temp);
}

static int tmp451_set_conversion_rate(const struct tmp451_config *cfg,
				      const struct sensor_value *val)
{
	uint8_t rate;

	switch (sensor_value_to_micro(val)) {
	/* 0.0625 Hz */
	case 62500:
		rate = 0;
		break;
	/* 0.125 Hz */
	case 125000:
		rate = 1;
		break;
	/* 0.25 Hz */
	case 250000:
		rate = 2;
		break;
	/* 0.5 Hz */
	case 500000:
		rate = 3;
		break;
	/* 1 Hz */
	case 1000000:
		rate = 4;
		break;
	/* 2 Hz */
	case 2000000:
		rate = 5;
		break;
	/* 4 Hz */
	case 4000000:
		rate = 6;
		break;
	/* 8 Hz */
	case 8000000:
		rate = 7;
		break;
	/* 16 Hz */
	case 16000000:
		rate = 8;
		break;
	/* 32 Hz */
	case 32000000:
		rate = 9;
		break;
	default:
		LOG_ERR("Invalid conversion rate");
		return -EINVAL;
	}

	return i2c_reg_write_byte_dt(&cfg->i2c, TMP451_REG_CR_WRITE, rate);
}

static int tmp451_set_mode(const struct tmp451_config *cfg, struct tmp451_data *data, bool shutdown,
			   bool one_shot)
{
	int ret;

	ret = tmp451_reg_update(cfg, TMP451_REG_CONFIG_READ, TMP451_REG_CONFIG_WRITE,
				TMP451_CONFIG_SD, shutdown ? TMP451_CONFIG_SD : 0);
	if (ret < 0) {
		LOG_ERR("Failed to set conversion mode");
		return ret;
	}

	data->one_shot_mode = one_shot;
	return 0;
}

static int tmp451_set_remote_offset(const struct tmp451_config *cfg, const struct sensor_value *val)
{
	/* Note: We don't apply the extended range offset because this reg represents a relative
	 * value
	 */
	int64_t raw = sensor_value_to_micro(val) / TMP451_TEMP_SCALE;

	/* 12-bit signed range in 1/16 deg C counts: -2048 (-128 deg C) to 2047 (+127.9375 deg C) */
	if (!IN_RANGE(raw, -2048, 2047)) {
		LOG_ERR("Remote offset out of range");
		return -EINVAL;
	}

	return tmp451_write_precise_temp(cfg, TMP451_REG_RTO_H, TMP451_REG_RTO_L, raw);
}

static int tmp451_set_alert_limit(const struct tmp451_config *cfg, enum sensor_channel chan,
				  const struct sensor_value *val, uint8_t local_reg,
				  uint8_t remote_high_reg, uint8_t remote_low_reg)
{
	/* Note: We apply the extended range offset because this reg represents an absolute value */
	int64_t raw =
		(sensor_value_to_micro(val) / TMP451_TEMP_SCALE) + TMP451_EXTENDED_RANGE_OFFSET;

	/* Both channels share the same range, just differ in resolution
	 * 12-bit unsigned (due to offset) range in 1/16 deg C counts: 0 (-64 deg C) to 4095
	 * (+191.9375 deg C)
	 */
	if (!IN_RANGE(raw, 0, 4095)) {
		LOG_ERR("Alert limit out of range");
		return -EINVAL;
	}

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Local limit: 8-bit, 1 deg C resolution (high byte only)
		 * We silently truncate the fractional parts if provided by user
		 * (returning error seems too strict)
		 */
		return i2c_reg_write_byte_dt(&cfg->i2c, local_reg, raw >> 4);
	case SENSOR_CHAN_DIE_TEMP:
		return tmp451_write_precise_temp(cfg, remote_high_reg, remote_low_reg, raw);
	default:
		return -ENOTSUP;
	}
}

static int tmp451_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct tmp451_config *cfg = dev->config;
	struct tmp451_data *data = dev->data;

	switch ((int)attr) {
	/* Standard attributes */
	case SENSOR_ATTR_HYSTERESIS:
		/* Hysteresis must be a non-negative 8-bit value */
		if (!IN_RANGE(val->val1, 0, UINT8_MAX)) {
			return -EINVAL;
		}

		return i2c_reg_write_byte_dt(&cfg->i2c, TMP451_REG_HYS, val->val1);
	case SENSOR_ATTR_OFFSET:
		return tmp451_set_remote_offset(cfg, val);
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return tmp451_set_conversion_rate(cfg, val);

	/* Custom attributes */
	case SENSOR_ATTR_TMP451_ONE_SHOT_MODE:
		return tmp451_set_mode(cfg, data, true, true);
	case SENSOR_ATTR_TMP451_CONTINUOUS_MODE:
		return tmp451_set_mode(cfg, data, false, false);
	case SENSOR_ATTR_TMP451_SHUTDOWN_MODE:
		return tmp451_set_mode(cfg, data, true, false);

	/* Threshold attributes
	 *
	 * We don't gate these behind triggers enabled because pin 6 might be in THERM2 mode,
	 * which relies on these registers for its limits but won't typically be used
	 * as a trigger to the MCU.
	 */
	case SENSOR_ATTR_UPPER_THRESH:
		return tmp451_set_alert_limit(cfg, chan, val, TMP451_REG_LTHL_WRITE,
					      TMP451_REG_RTHL_H_WRITE, TMP451_REG_RTHL_L);
	case SENSOR_ATTR_LOWER_THRESH:
		return tmp451_set_alert_limit(cfg, chan, val, TMP451_REG_LTLL_WRITE,
					      TMP451_REG_RTLL_H_WRITE, TMP451_REG_RTLL_L);
	default:
		return -ENOTSUP;
	}
}

static int tmp451_check_id(const struct tmp451_config *cfg)
{
	uint8_t manufacturer_id;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, TMP451_REG_ID, &manufacturer_id);
	if (ret < 0) {
		LOG_ERR("Failed to read manufacturer ID");
		return ret;
	}

	if (manufacturer_id != TMP451_ID) {
		LOG_ERR("Device with manufacturer ID 0x%02x is not supported", manufacturer_id);
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, tmp451_api) = {
	.attr_set = tmp451_attr_set,
	.sample_fetch = tmp451_sample_fetch,
	.channel_get = tmp451_channel_get,
#ifdef CONFIG_TMP451_TRIGGER
	.trigger_set = tmp451_trigger_set,
#endif
};

static int tmp451_init(const struct device *dev)
{
	const struct tmp451_config *cfg = dev->config;
	struct sensor_value init_sensor_val;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	ret = tmp451_check_id(cfg);
	if (ret < 0) {
		return ret;
	}

	/* Extended range, continuous mode, ALERT masked.
	 * ALERT is unmasked later once the application has configured
	 * proper thresholds and registered a handler.
	 */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, TMP451_REG_CONFIG_WRITE,
				    TMP451_CONFIG_RANGE | TMP451_CONFIG_MASK1);
	if (ret < 0) {
		LOG_ERR("Failed to write initial config");
		return ret;
	}

	/* These registers have sane POR values if in standard mode, but we are defaulting to
	 * extended mode so we need to rewrite these values to prevent spurious alerts (which would
	 * likely happen since the POR values match roughly room temperature when treated
	 * as extended range).
	 *
	 * So, we just set them to the max/min limits respectively.
	 */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, TMP451_REG_LTHL_WRITE, 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write local high limit");
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TMP451_REG_LTLL_WRITE, 0x00);
	if (ret < 0) {
		LOG_ERR("Failed to write local low limit");
		return ret;
	}

	ret = tmp451_write_precise_temp(cfg, TMP451_REG_RTHL_H_WRITE, TMP451_REG_RTHL_L, 0x0FFF);
	if (ret < 0) {
		LOG_ERR("Failed to write remote high limit");
		return ret;
	}

	ret = tmp451_write_precise_temp(cfg, TMP451_REG_RTLL_H_WRITE, TMP451_REG_RTLL_L, 0x0000);
	if (ret < 0) {
		LOG_ERR("Failed to write remote low limit");
		return ret;
	}

	/* Set conversion rate for continuous mode */
	sensor_value_from_micro(&init_sensor_val, cfg->conversion_rate);
	ret = tmp451_set_conversion_rate(cfg, &init_sensor_val);
	if (ret < 0) {
		LOG_ERR("Failed to set conversion rate");
		return ret;
	}

	/* Set ideality factor correction for remote channel */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, TMP451_REG_NC, cfg->ideality_factor);
	if (ret < 0) {
		LOG_ERR("Failed to write ideality factor register");
		return ret;
	}

	/* Set digital filter for remote channel */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, TMP451_REG_DF, cfg->digital_filter);
	if (ret < 0) {
		LOG_ERR("Failed to write digital filter register");
		return ret;
	}

	/* Set temperature offset for remote channel */
	sensor_value_from_milli(&init_sensor_val, cfg->remote_offset);
	ret = tmp451_set_remote_offset(cfg, &init_sensor_val);
	if (ret < 0) {
		LOG_ERR("Failed to write remote offset registers");
		return ret;
	}

	/* Note: We always apply THERM configs (regardless if triggers enabled),
	 * because the THERM pin (pin 4) and pin 6 in THERM2 mode are not intended to be used as
	 * interrupts to the MCU, but as a signal to external hardware (such as emergency shutoff).
	 */

	/* Apply local THERM limit (with extended range offset)
	 * Note: This does not apply to THERM2 (which uses the same limit registers as ALERT)
	 */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, TMP451_REG_LTH,
				    cfg->local_therm_limit + TMP451_EXTENDED_RANGE_DEGREES);
	if (ret < 0) {
		LOG_ERR("Failed to write local THERM limit");
		return ret;
	}

	/* Apply remote THERM limit (with extended range offset)
	 * Note: This does not apply to THERM2 (which uses the same limit registers as ALERT)
	 */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, TMP451_REG_RTH,
				    cfg->remote_therm_limit + TMP451_EXTENDED_RANGE_DEGREES);
	if (ret < 0) {
		LOG_ERR("Failed to write remote THERM limit");
		return ret;
	}

	/* Set THERM hysteresis (shared by THERM2 as well) */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, TMP451_REG_HYS, cfg->hysteresis);
	if (ret < 0) {
		LOG_ERR("Failed to write THERM hysteresis register");
		return ret;
	}

	if (cfg->therm2_mode) {
		/* Set pin 6 to THERM2 mode */
		ret = tmp451_reg_update(cfg, TMP451_REG_CONFIG_READ, TMP451_REG_CONFIG_WRITE,
					TMP451_CONFIG_THERM2, TMP451_CONFIG_THERM2);
		if (ret < 0) {
			LOG_ERR("Failed to set THERM2 mode");
			return ret;
		}
	}

#ifdef CONFIG_TMP451_TRIGGER
	if (!cfg->therm2_mode) {
		uint8_t conal;

		/* Consecutive alert threshold only applies if pin 6 is in ALERT mode */
		switch (cfg->consecutive_alerts) {
		case 1:
			conal = TMP451_CONAL_1;
			break;
		case 2:
			conal = TMP451_CONAL_2;
			break;
		case 3:
			conal = TMP451_CONAL_3;
			break;
		case 4:
			conal = TMP451_CONAL_4;
			break;
		default:
			return -EINVAL;
		}

		ret = tmp451_reg_update(cfg, TMP451_REG_CONAL, TMP451_REG_CONAL, TMP451_CONAL_MASK,
					conal);
		if (ret < 0) {
			LOG_ERR("Failed to write consecutive alert register");
			return ret;
		}

		/* We make the assumption that pin 6 will not be used for triggers if in THERM2 mode
		 * because the interrupt model would be different.
		 */
		ret = tmp451_init_interrupt(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize interrupt");
			return ret;
		}
	}
#endif /* CONFIG_TMP451_TRIGGER */

	return 0;
}

#define TMP451_INST(inst)                                                                          \
	BUILD_ASSERT(DT_INST_PROP(inst, therm_hysteresis) >= 0 &&                                  \
			     DT_INST_PROP(inst, therm_hysteresis) <= 255,                          \
		     "therm-hysteresis must be 0..255");                                           \
	BUILD_ASSERT(DT_INST_PROP(inst, ideality_factor) >= -128 &&                                \
			     DT_INST_PROP(inst, ideality_factor) <= 127,                           \
		     "ideality-factor must be -128..127");                                         \
	BUILD_ASSERT(DT_INST_PROP(inst, local_therm_limit) >= -64 &&                               \
			     DT_INST_PROP(inst, local_therm_limit) <= 191,                         \
		     "local-therm-limit must be -64..191");                                        \
	BUILD_ASSERT(DT_INST_PROP(inst, remote_therm_limit) >= -64 &&                              \
			     DT_INST_PROP(inst, remote_therm_limit) <= 191,                        \
		     "remote-therm-limit must be -64..191");                                       \
	static struct tmp451_data tmp451_data_##inst;                                              \
	static const struct tmp451_config tmp451_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.ideality_factor = DT_INST_PROP(inst, ideality_factor),                            \
		.hysteresis = DT_INST_PROP(inst, therm_hysteresis),                                \
		.remote_offset = DT_INST_PROP(inst, remote_offset_millicelsius),                   \
		.conversion_rate = DT_INST_PROP(inst, conversion_rate_microhz),                    \
		.digital_filter = DT_INST_PROP(inst, digital_filter),                              \
		.local_therm_limit = DT_INST_PROP(inst, local_therm_limit),                        \
		.remote_therm_limit = DT_INST_PROP(inst, remote_therm_limit),                      \
		.therm2_mode = DT_INST_ENUM_HAS_VALUE(inst, pin6_mode, therm2),                    \
		IF_ENABLED(CONFIG_TMP451_TRIGGER,                               \
			(.alert_gpio = GPIO_DT_SPEC_INST_GET_OR(inst,           \
				alert_gpios, {}),                                  \
			 .consecutive_alerts =                                  \
				DT_INST_PROP(inst, consecutive_alerts),)) };   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tmp451_init, NULL, &tmp451_data_##inst,                 \
				     &tmp451_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &tmp451_api);

DT_INST_FOREACH_STATUS_OKAY(TMP451_INST)
