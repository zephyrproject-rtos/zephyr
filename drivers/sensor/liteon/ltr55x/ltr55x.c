/*
 * Copyright (c) 2025 Konrad Sikora
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ltr55x.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(LTR55X, CONFIG_SENSOR_LOG_LEVEL);

static int ltr55x_check_device_id(const struct ltr55x_config *cfg)
{
	const struct i2c_dt_spec *bus = &cfg->bus;
	uint8_t id;
	int rc;

	rc = i2c_reg_read_byte_dt(bus, LTR55X_PART_ID, &id);
	if (rc < 0) {
		LOG_ERR("Failed to read PART_ID");
		return rc;
	}

	if (id != cfg->part_id) {
		LOG_ERR("PART_ID mismatch: expected 0x%02X, got 0x%02X", cfg->part_id, id);
		return -ENODEV;
	}

	rc = i2c_reg_read_byte_dt(bus, LTR55X_MANUFAC_ID, &id);
	if (rc < 0) {
		LOG_ERR("Failed to read MANUFAC_ID");
		return rc;
	}
	if (id != LTR55X_MANUFACTURER_ID_VALUE) {
		LOG_ERR("MANUFAC_ID mismatch: expected 0x%02X, got 0x%02X",
			LTR55X_MANUFACTURER_ID_VALUE, id);
		return -ENODEV;
	}

	return 0;
}

static int ltr55x_read_data(const struct ltr55x_config *cfg, enum sensor_channel chan,
			    struct ltr55x_data *data)
{
	const struct i2c_dt_spec *bus = &cfg->bus;
	const bool need_als = (chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_LIGHT);
	const bool need_ps = (cfg->part_id == LTR55X_PART_ID_VALUE) &&
			     ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_PROX));
	const size_t read_als_ps = (LTR55X_PS_DATA1 + 1) - LTR55X_ALS_DATA_CH1_0;
	const size_t read_als_only = (LTR55X_ALS_DATA_CH0_1 + 1) - LTR55X_ALS_DATA_CH1_0;
	const size_t read_size =
		(cfg->part_id == LTR55X_PART_ID_VALUE) ? read_als_ps : read_als_only;
	uint8_t reg = LTR55X_ALS_DATA_CH1_0;
	uint8_t buff[read_als_ps];
	int rc;

	rc = i2c_write_read_dt(bus, &reg, sizeof(reg), buff, read_size);
	if (rc < 0) {
		LOG_ERR("Failed to read ALS data registers");
		return rc;
	}

	if (need_als) {
		data->als_ch1 = sys_get_le16(buff);
		data->als_ch0 = sys_get_le16(buff + 2);
	}

	if (need_ps) {
		data->ps_ch0 = sys_get_le16(buff + 5) & LTR55X_PS_DATA_MASK;
	}

	return 0;
}

static int ltr55x_init_ps_interrupt_registers(const struct device *dev)
{
	const struct ltr55x_config *cfg = dev->config;
	const struct i2c_dt_spec *bus = &cfg->bus;
	struct ltr55x_data *data = dev->data;
	uint8_t buf[6];
	int rc;

	sys_put_le16(data->ps_upper_threshold, &buf[0]);
	sys_put_le16(data->ps_lower_threshold, &buf[2]);
	sys_put_be16(data->ps_offset, &buf[4]);

	rc = i2c_burst_write_dt(bus, LTR55X_PS_THRES_UP_0, buf, 6);
	if (rc < 0) {
		LOG_ERR("Failed to set PS threshold/offset: %d", rc);
		return rc;
	}

	return 0;
}

static int ltr55x_init_ps_registers(const struct device *dev)
{
	const struct ltr55x_config *cfg = dev->config;
	const struct i2c_dt_spec *bus = &cfg->bus;
	const uint8_t ps_contr = LTR55X_REG_SET(PS_CONTR, MODE, LTR55X_PS_CONTR_MODE_ACTIVE) |
				 LTR55X_REG_SET(PS_CONTR, SAT_IND, cfg->ps_saturation_indicator);
	const uint8_t ps_led = LTR55X_REG_SET(PS_LED, PULSE_FREQ, cfg->ps_led_pulse_freq) |
			       LTR55X_REG_SET(PS_LED, DUTY_CYCLE, cfg->ps_led_duty_cycle) |
			       LTR55X_REG_SET(PS_LED, CURRENT, cfg->ps_led_current);
	const uint8_t ps_n_pulses = LTR55X_REG_SET(PS_N_PULSES, COUNT, cfg->ps_n_pulses);
	const uint8_t ps_meas_rate = LTR55X_REG_SET(PS_MEAS_RATE, RATE, cfg->ps_measurement_rate);
	const uint8_t buf[] = {ps_contr, ps_led, ps_n_pulses, ps_meas_rate};
	int rc;

	rc = i2c_burst_write_dt(bus, LTR55X_PS_CONTR, buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Failed to set PS registers");
		return rc;
	}

	return 0;
}

static int ltr55x_init_als_registers(const struct device *dev)
{
	const struct ltr55x_config *cfg = dev->config;
	const struct i2c_dt_spec *bus = &cfg->bus;
	const uint8_t control_reg = LTR55X_REG_SET(ALS_CONTR, MODE, LTR55X_ALS_CONTR_MODE_ACTIVE) |
				    LTR55X_REG_SET(ALS_CONTR, GAIN, cfg->als_gain);
	const uint8_t meas_reg = LTR55X_REG_SET(MEAS_RATE, REPEAT, cfg->als_measurement_rate) |
				 LTR55X_REG_SET(MEAS_RATE, INT_TIME, cfg->als_integration_time);
	uint8_t buffer;
	int rc;

	rc = i2c_reg_write_byte_dt(bus, LTR55X_ALS_CONTR, control_reg);
	if (rc < 0) {
		LOG_ERR("Failed to set ALS_CONTR register");
		return rc;
	}

	rc = i2c_reg_write_byte_dt(bus, LTR55X_MEAS_RATE, meas_reg);
	if (rc < 0) {
		LOG_ERR("Failed to set MEAS_RATE register");
		return rc;
	}

	/* Read back the MEAS_RATE register to verify the settings */
	rc = i2c_reg_read_byte_dt(bus, LTR55X_MEAS_RATE, &buffer);
	if (rc < 0) {
		LOG_ERR("Failed to read back MEAS_RATE register");
		return rc;
	}
	if (LTR55X_REG_GET(MEAS_RATE, REPEAT, buffer) != cfg->als_measurement_rate) {
		LOG_ERR("Measurement rate mismatch: expected %u, got %u", cfg->als_measurement_rate,
			(uint8_t)LTR55X_REG_GET(MEAS_RATE, REPEAT, buffer));
		return -ENODEV;
	}
	if (LTR55X_REG_GET(MEAS_RATE, INT_TIME, buffer) != cfg->als_integration_time) {
		LOG_ERR("Integration time mismatch: expected %u, got %u", cfg->als_integration_time,
			(uint8_t)LTR55X_REG_GET(MEAS_RATE, INT_TIME, buffer));
		return -ENODEV;
	}

	return 0;
}

static int ltr55x_update_als_threshold_registers(const struct device *dev)
{
	const struct ltr55x_config *cfg = dev->config;
	const struct i2c_dt_spec *bus = &cfg->bus;
	struct ltr55x_data *data = dev->data;
	uint8_t buf[4];
	int rc;

	sys_put_le16(data->als_upper_threshold, &buf[0]);
	sys_put_le16(data->als_lower_threshold, &buf[2]);

	rc = i2c_burst_write_dt(bus, LTR55X_ALS_THRES_UP_0, buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Failed to set ALS threshold: %d", rc);
		return rc;
	}

	return 0;
}

static int ltr55x_init_als_interrupt_registers(const struct device *dev)
{
	const struct ltr55x_config *cfg = dev->config;
	const struct i2c_dt_spec *bus = &cfg->bus;
	int rc;

	rc = ltr55x_update_als_threshold_registers(dev);
	if (rc < 0) {
		return rc;
	}

	rc = i2c_reg_update_byte_dt(
		bus, LTR55X_INTERRUPT_PERSIST, LTR55X_INTERRUPT_PERSIST_ALS_MASK,
		LTR55X_REG_SET(INTERRUPT_PERSIST, ALS, cfg->als_interrupt_persist - 1));
	if (rc < 0) {
		LOG_ERR("Failed to set ALS interrupt persist: %d", rc);
		return rc;
	}

	return 0;
}

static int ltr55x_check_data_ready(const struct ltr55x_config *cfg, enum sensor_channel chan)
{
	const struct i2c_dt_spec *bus = &cfg->bus;
	const bool need_als = (chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_LIGHT);
	const bool need_ps = (cfg->part_id == LTR55X_PART_ID_VALUE) &&
			     ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_PROX));
	uint8_t status;
	int rc;

	rc = i2c_reg_read_byte_dt(bus, LTR55X_ALS_PS_STATUS, &status);
	if (rc < 0) {
		LOG_ERR("Failed to read ALS_STATUS register");
		return rc;
	}

	if (need_als && !LTR55X_REG_GET(ALS_PS_STATUS, ALS_DATA_STATUS, status)) {
		LOG_WRN("ALS data not ready");
		return -EBUSY;
	}

	if (need_ps && !LTR55X_REG_GET(ALS_PS_STATUS, PS_DATA_STATUS, status)) {
		LOG_WRN("PS data not ready");
		return -EBUSY;
	}

	return 0;
}

#ifdef CONFIG_LITEON_LTR_TRIGGER_ALS

static int ltr55x_interrupt_set_register(const struct device *dev, bool als_enable)
{
	const struct ltr55x_config *cfg = dev->config;
	const struct i2c_dt_spec *bus = &cfg->bus;
	uint8_t als_ctrl;
	uint8_t interrupt_cfg;
	bool restore_active;
	int rc;

	rc = i2c_reg_read_byte_dt(bus, LTR55X_ALS_CONTR, &als_ctrl);
	if (rc < 0) {
		return rc;
	}

	restore_active = LTR55X_REG_GET(ALS_CONTR, MODE, als_ctrl) != 0;
	if (restore_active) {
		rc = i2c_reg_update_byte_dt(bus, LTR55X_ALS_CONTR, LTR55X_ALS_CONTR_MODE_MASK,
					    LTR55X_REG_SET(ALS_CONTR, MODE, LTR55X_ALS_CONTR_MODE_STAND_BY));
		if (rc < 0) {
			return rc;
		}
	}

	interrupt_cfg = LTR55X_REG_SET(INTERRUPT, ALS, als_enable) |
			LTR55X_REG_SET(INTERRUPT, POLARITY,
				       (cfg->int_gpio.dt_flags & GPIO_ACTIVE_LOW) == 0U);
	rc = i2c_reg_update_byte_dt(
		bus, LTR55X_INTERRUPT, LTR55X_INTERRUPT_ALS_MASK | LTR55X_INTERRUPT_POLARITY_MASK,
		interrupt_cfg);

	if (restore_active) {
		int restore_rc = i2c_reg_update_byte_dt(bus, LTR55X_ALS_CONTR, LTR55X_ALS_CONTR_MODE_MASK,
						       LTR55X_REG_SET(ALS_CONTR, MODE, LTR55X_ALS_CONTR_MODE_ACTIVE));

		if (restore_rc < 0 && rc == 0) {
			rc = restore_rc;
		}
	}

	return rc;
}

static void ltr55x_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct ltr55x_data *data = CONTAINER_OF(cb, struct ltr55x_data, gpio_cb);
	const struct ltr55x_config *cfg = data->dev->config;

	ARG_UNUSED(port);

	if ((pins & BIT(cfg->int_gpio.pin)) == 0U) {
		return;
	}

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
	k_work_submit(&data->work);
}

static void ltr55x_handle_interrupt(struct k_work *work)
{
	struct ltr55x_data *data = CONTAINER_OF(work, struct ltr55x_data, work);
	const struct device *dev = data->dev;
	const struct ltr55x_config *cfg = dev->config;
	sensor_trigger_handler_t handler;
	const struct sensor_trigger *trigger;
	uint8_t status;
	int rc;

	rc = i2c_reg_read_byte_dt(&cfg->bus, LTR55X_ALS_PS_STATUS, &status);
	if (rc < 0) {
		LOG_ERR("Failed to read interrupt status: %d", rc);
		goto out;
	}

	if (!LTR55X_REG_GET(ALS_PS_STATUS, ALS_INTR_STATUS, status)) {
		goto out;
	}

	/* Read ALS data after interrupt to refresh cached sample and clear latched status. */
	rc = ltr55x_read_data(cfg, SENSOR_CHAN_LIGHT, data);
	if (rc < 0) {
		LOG_ERR("Failed to update ALS data after interrupt: %d", rc);
		goto out;
	}

	k_mutex_lock(&data->trigger_mutex, K_FOREVER);
	handler = data->als_handler;
	trigger = data->als_trigger;
	k_mutex_unlock(&data->trigger_mutex);

	if (handler != NULL) {
		handler(dev, trigger);
	}

out:
	k_mutex_lock(&data->trigger_mutex, K_FOREVER);
	handler = data->als_handler;
	k_mutex_unlock(&data->trigger_mutex);

	if (handler != NULL) {
		gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}
}

static int ltr55x_trigger_init(const struct device *dev)
{
	const struct ltr55x_config *cfg = dev->config;
	struct ltr55x_data *data = dev->data;
	int rc;

	if (cfg->int_gpio.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("Interrupt GPIO not ready");
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Failed to configure interrupt GPIO: %d", rc);
		return rc;
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
	if (rc < 0) {
		LOG_ERR("Failed to disable interrupt GPIO: %d", rc);
		return rc;
	}

	gpio_init_callback(&data->gpio_cb, ltr55x_gpio_callback, BIT(cfg->int_gpio.pin));

	rc = gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
	if (rc < 0) {
		LOG_ERR("Failed to add interrupt callback: %d", rc);
		return rc;
	}

	k_work_init(&data->work, ltr55x_handle_interrupt);
	k_mutex_init(&data->trigger_mutex);

	return 0;
}

static int ltr55x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			      sensor_trigger_handler_t handler)
{
	const struct ltr55x_config *cfg = dev->config;
	struct ltr55x_data *data = dev->data;
	int rc;



	if (cfg->int_gpio.port == NULL) {
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_THRESHOLD || trig->chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->trigger_mutex, K_FOREVER);
	data->als_handler = handler;
	data->als_trigger = handler ? trig : NULL;
	k_mutex_unlock(&data->trigger_mutex);

	rc = ltr55x_interrupt_set_register(dev, handler != NULL);
	if (rc < 0) {
		LOG_ERR("Failed to configure ALS interrupt source: %d", rc);
		return rc;
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					     handler ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
	if (rc < 0) {
		LOG_ERR("Failed to configure GPIO interrupt: %d", rc);
		return rc;
	}

	return 0;
}
#endif /* CONFIG_LITEON_LTR_TRIGGER_ALS */


static bool ltr55x_is_channel_supported(const struct ltr55x_config *cfg, enum sensor_channel chan)
{
	if (cfg->part_id == LTR55X_PART_ID_VALUE) {
		return (chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_LIGHT) ||
		       (chan == SENSOR_CHAN_PROX);
	}

	return (chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_LIGHT);
}

static int ltr55x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ltr55x_config *cfg = dev->config;
	struct ltr55x_data *data = dev->data;
	int rc;

	if (!ltr55x_is_channel_supported(cfg, chan)) {
		return -ENOTSUP;
	}

	rc = ltr55x_check_data_ready(cfg, chan);
	if (rc < 0) {
		return rc;
	}

	rc = ltr55x_read_data(cfg, chan, data);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int ltr55x_get_mapped_gain(const uint8_t reg_val, uint8_t *const output)
{
	/* Map the register value to the gain used in the lux calculation
	 * Indices 4 and 5 are invalid.
	 */
	static const uint8_t gain_lux_calc[] = {1, 2, 4, 8, 0, 0, 48, 96};

	if (reg_val < ARRAY_SIZE(gain_lux_calc)) {
		*output = gain_lux_calc[reg_val];
		/* 0 is not a valid value */
		return (*output == 0) ? -EINVAL : 0;
	}

	return -EINVAL;
}

static int ltr55x_get_mapped_int_time(const uint8_t reg_val, uint8_t *const output)
{
	/* Map the register value to the integration time used in the lux calculation */
	static const uint8_t int_time_lux_calc[] = {10, 5, 20, 40, 15, 25, 30, 35};

	if (reg_val < ARRAY_SIZE(int_time_lux_calc)) {
		*output = int_time_lux_calc[reg_val];
		return 0;
	}

	*output = 0;
	return -EINVAL;
}

static int ltr55x_channel_light_get(const struct device *dev, struct sensor_value *val)
{
	const struct ltr55x_config *cfg = dev->config;
	struct ltr55x_data *data = dev->data;
	uint8_t gain_value;
	uint8_t integration_time_value;

	if (ltr55x_get_mapped_gain(cfg->als_gain, &gain_value) != 0) {
		LOG_ERR("Invalid gain configuration");
		return -EINVAL;
	}

	if (ltr55x_get_mapped_int_time(cfg->als_integration_time, &integration_time_value) != 0) {
		LOG_ERR("Invalid integration time configuration");
		return -EINVAL;
	}

	if ((data->als_ch0 == 0) && (data->als_ch1 == 0)) {
		LOG_WRN("Both channels are zero; cannot compute ratio");
		return -EINVAL;
	}

	/* Calculate lux value according to the appendix A of the datasheet. */
	uint64_t lux;
	/* The calculation is scaled by 1000000 to avoid floating point. */
	uint64_t scaled_ratio =
		(data->als_ch1 * UINT64_C(1000000)) / (uint64_t)(data->als_ch0 + data->als_ch1);

	if (scaled_ratio < UINT64_C(450000)) {
		lux = (UINT64_C(1774300) * data->als_ch0 + UINT64_C(1105900) * data->als_ch1);
	} else if (scaled_ratio < UINT64_C(640000)) {
		lux = (UINT64_C(4278500) * data->als_ch0 - UINT64_C(1954800) * data->als_ch1);
	} else if (scaled_ratio < UINT64_C(850000)) {
		lux = (UINT64_C(592600) * data->als_ch0 + UINT64_C(118500) * data->als_ch1);
	} else {
		LOG_WRN("Invalid ratio: %llu", scaled_ratio);
		return -EINVAL;
	}

	/* Adjust lux value for gain and integration time.
	 * Multiply by 10 before to compensate for the integration time scaling.
	 */
	lux = (lux * 10) / (gain_value * integration_time_value);

	/* Get the integer and fractional parts from fixed point value */
	val->val1 = (int32_t)(lux / UINT64_C(1000000));
	val->val2 = (int32_t)(lux % UINT64_C(1000000));

	return 0;
}

static int ltr55x_channel_proximity_get(const struct device *dev, struct sensor_value *val)
{
	const struct ltr55x_config *cfg = dev->config;
	struct ltr55x_data *data = dev->data;

	if (cfg->part_id != LTR55X_PART_ID_VALUE) {
		return -ENOTSUP;
	}

	LOG_DBG("proximity: state=%d data: %d L-H: %d - %d", data->proximity_state, data->ps_ch0,
		data->ps_lower_threshold, data->ps_upper_threshold);

	if (data->proximity_state) {
		if (data->ps_ch0 <= data->ps_lower_threshold) {
			data->proximity_state = false;
		}
	} else {
		if (data->ps_ch0 >= data->ps_upper_threshold) {
			data->proximity_state = true;
		}
	}

	val->val1 = data->proximity_state ? 1 : 0;
	val->val2 = 0;

	return 0;
}

static int ltr55x_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ltr55x_config *cfg = dev->config;
	int ret = -ENOTSUP;

	if (!ltr55x_is_channel_supported(cfg, chan)) {
		return -ENOTSUP;
	}

	if (chan == SENSOR_CHAN_LIGHT || chan == SENSOR_CHAN_ALL) {
		ret = ltr55x_channel_light_get(dev, val);

		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_PROX || chan == SENSOR_CHAN_ALL) {
		ret = ltr55x_channel_proximity_get(dev, val);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

static int ltr55x_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	struct ltr55x_data *data = dev->data;
	int32_t threshold;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	/* val2 is not used for threshold attributes, so it must be zero. */
	if (val->val2 != 0) {
		return -EINVAL;
	}

	threshold = val->val1;
	if (threshold < 0 || threshold > UINT16_MAX) {
		return -EINVAL;
	}

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		data->als_upper_threshold = (uint16_t)threshold;
	} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
		data->als_lower_threshold = (uint16_t)threshold;
	} else {
		return -ENOTSUP;
	}

	if (data->als_lower_threshold > data->als_upper_threshold) {
		return -EINVAL;
	}

	return ltr55x_update_als_threshold_registers(dev);
}

static int ltr55x_init(const struct device *dev)
{
	const struct ltr55x_config *cfg = dev->config;
	struct ltr55x_data *data = dev->data;
	int rc;

	data->dev = dev;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Wait for sensor startup */
	k_sleep(K_MSEC(LTR55X_INIT_STARTUP_MS));

	rc = ltr55x_check_device_id(cfg);
	if (rc < 0) {
		return rc;
	}

	if (cfg->part_id == LTR55X_PART_ID_VALUE) {
		rc = ltr55x_init_ps_interrupt_registers(dev);
		if (rc < 0) {
			return rc;
		}

		rc = ltr55x_init_ps_registers(dev);
		if (rc < 0) {
			return rc;
		}
	}

	rc = ltr55x_init_als_interrupt_registers(dev);
	if (rc < 0) {
		return rc;
	}

	/* Init register to enable sensor to active mode */
	rc = ltr55x_init_als_registers(dev);
	if (rc < 0) {
		return rc;
	}

#ifdef CONFIG_LITEON_LTR_TRIGGER_ALS
	rc = ltr55x_trigger_init(dev);
	if (rc < 0) {
		return rc;
	}
#endif

	return 0;
}

static DEVICE_API(sensor, ltr55x_driver_api) = {
	.attr_set = ltr55x_attr_set,
#ifdef CONFIG_LITEON_LTR_TRIGGER_ALS
	.trigger_set = ltr55x_trigger_set,
#endif
	.sample_fetch = ltr55x_sample_fetch,
	.channel_get = ltr55x_channel_get,
};

#define LTR55X_ALS_GAIN_REG(n)                                                                     \
	COND_CODE_1(DT_NODE_HAS_PROP(n, gain), (DT_PROP(n, gain)),                                 \
		    (UTIL_CAT(LTR55X_ALS_GAIN_VALUE_, DT_PROP_OR(n, als_gain, 1))))
#define LTR55X_ALS_INT_TIME_REG(n)                                                                 \
	COND_CODE_1(DT_NODE_HAS_PROP(n, integration_time), (DT_PROP(n, integration_time)),         \
		    (DT_ENUM_IDX_OR(n, als_integration_time, 0)))
#define LTR55X_ALS_MEAS_RATE_REG(n)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(n, measurement_rate), (DT_PROP(n, measurement_rate)),         \
		    (DT_ENUM_IDX_OR(n, als_measurement_rate, 3)))

#define DEFINE_LTRXXX(node_id, partid)                                                             \
	BUILD_ASSERT(DT_PROP_OR(node_id, ps_offset, 0) <= LTR55X_PS_DATA_MAX);                     \
	BUILD_ASSERT(DT_PROP_OR(node_id, ps_upper_threshold, LTR55X_PS_DATA_MASK) <=               \
		     LTR55X_PS_DATA_MASK);                                                         \
	BUILD_ASSERT(DT_PROP_OR(node_id, ps_lower_threshold, 0) <= LTR55X_PS_DATA_MAX);            \
	BUILD_ASSERT(DT_PROP_OR(node_id, ps_lower_threshold, 0) <=                                 \
		     DT_PROP_OR(node_id, ps_upper_threshold, LTR55X_PS_DATA_MAX));                 \
	BUILD_ASSERT(DT_PROP_OR(node_id, als_lower_threshold, 0) <= UINT16_MAX);                   \
	BUILD_ASSERT(DT_PROP_OR(node_id, als_upper_threshold, UINT16_MAX) <= UINT16_MAX);          \
	BUILD_ASSERT(DT_PROP_OR(node_id, als_lower_threshold, 0) <=                                \
		     DT_PROP_OR(node_id, als_upper_threshold, UINT16_MAX));                         \
	BUILD_ASSERT(DT_PROP_OR(node_id, als_interrupt_persist, 1) >= 1);                          \
	BUILD_ASSERT(DT_PROP_OR(node_id, als_interrupt_persist, 1) <=                              \
		     LTR55X_INTERRUPT_PERSIST_ALS_MASK + 1);                                        \
	static struct ltr55x_data ltr55x_data_##node_id = {                                        \
		.als_upper_threshold = DT_PROP_OR(node_id, als_upper_threshold, UINT16_MAX),        \
		.als_lower_threshold = DT_PROP_OR(node_id, als_lower_threshold, 0),                 \
		.ps_offset = DT_PROP_OR(node_id, ps_offset, 0),                                    \
		.ps_upper_threshold = DT_PROP_OR(node_id, ps_upper_threshold, LTR55X_PS_DATA_MAX), \
		.ps_lower_threshold = DT_PROP_OR(node_id, ps_lower_threshold, 0),                  \
	};                                                                                         \
	static const struct ltr55x_config ltr55x_config_##node_id = {                              \
		.bus = I2C_DT_SPEC_GET(node_id),                                                   \
		.int_gpio = GPIO_DT_SPEC_GET_OR(node_id, int_gpios, {0}),                          \
		.als_interrupt_persist = DT_PROP_OR(node_id, als_interrupt_persist, 1),            \
		.part_id = partid,                                                                 \
		.als_gain = LTR55X_ALS_GAIN_REG(node_id),                                          \
		.als_integration_time = LTR55X_ALS_INT_TIME_REG(node_id),                          \
		.als_measurement_rate = LTR55X_ALS_MEAS_RATE_REG(node_id),                         \
		.ps_led_pulse_freq = DT_ENUM_IDX_OR(node_id, ps_led_pulse_frequency, 3),           \
		.ps_led_duty_cycle = DT_ENUM_IDX_OR(node_id, ps_led_duty_cycle, 3),                \
		.ps_led_current = DT_ENUM_IDX_OR(node_id, ps_led_current, 4),                      \
		.ps_n_pulses = DT_PROP_OR(node_id, ps_n_pulses, 1),                                \
		.ps_measurement_rate = UTIL_CAT(LTR55X_PS_MEASUREMENT_RATE_VALUE_,                 \
						DT_PROP_OR(node_id, ps_measurement_rate, 100)),    \
		.ps_saturation_indicator = DT_PROP_OR(node_id, ps_saturation_indicator, false),    \
	};                                                                                         \
	SENSOR_DEVICE_DT_DEFINE(node_id, ltr55x_init, NULL, &ltr55x_data_##node_id,                \
				&ltr55x_config_##node_id, POST_KERNEL,                             \
				CONFIG_SENSOR_INIT_PRIORITY, &ltr55x_driver_api);

#define DEFINE_LTR303(node_id) DEFINE_LTRXXX(node_id, LTR303_PART_ID_VALUE)
#define DEFINE_LTR329(node_id) DEFINE_LTRXXX(node_id, LTR329_PART_ID_VALUE)
#define DEFINE_LTR55X(node_id) DEFINE_LTRXXX(node_id, LTR55X_PART_ID_VALUE)

DT_FOREACH_STATUS_OKAY(liteon_ltr303, DEFINE_LTR303)
DT_FOREACH_STATUS_OKAY(liteon_ltr329, DEFINE_LTR329)
DT_FOREACH_STATUS_OKAY(liteon_ltr553, DEFINE_LTR55X)
