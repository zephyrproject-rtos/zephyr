/*
 * Copyright (c) 2024 Juliane Schulze, deveritec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vishay_vcnl36825t

#include "vcnl36825t.h"

#include <stdlib.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(VCNL36825T, CONFIG_SENSOR_LOG_LEVEL);

int vcnl36825t_read(const struct i2c_dt_spec *spec, uint8_t reg_addr, uint16_t *value)
{
	uint8_t rx_buf[2];
	int rc;

	rc = i2c_write_read_dt(spec, &reg_addr, sizeof(reg_addr), rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		return rc;
	}

	*value = sys_get_le16(rx_buf);

	return 0;
}

int vcnl36825t_write(const struct i2c_dt_spec *spec, uint8_t reg_addr, uint16_t value)
{
	uint8_t tx_buf[3] = {reg_addr};

	sys_put_le16(value, &tx_buf[1]);
	return i2c_write_dt(spec, tx_buf, sizeof(tx_buf));
}

int vcnl36825t_update(const struct i2c_dt_spec *spec, uint8_t reg_addr, uint16_t mask,
		      uint16_t value)
{
	int rc;
	uint16_t old_value, new_value;

	rc = vcnl36825t_read(spec, reg_addr, &old_value);
	if (rc < 0) {
		return rc;
	}

	new_value = (old_value & ~mask) | (value & mask);

	if (new_value == old_value) {
		return 0;
	}

	return vcnl36825t_write(spec, reg_addr, new_value);
}

#if CONFIG_PM_DEVICE
static int vcnl36825t_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct vcnl36825t_config *config = dev->config;
	struct vcnl36825t_data *data = dev->data;

	int rc;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		rc = vcnl36825t_update(&config->i2c, VCNL36825T_REG_PS_CONF1, VCNL36825T_PS_ON_MSK,
				       VCNL36825T_PS_ON);
		if (rc < 0) {
			return rc;
		}

		if (config->low_power) {
			rc = vcnl36825t_update(&config->i2c, VCNL36825T_REG_PS_CONF4,
					       VCNL36825T_PS_LPEN_MSK, VCNL36825T_PS_LPEN_ENABLED);
			if (rc < 0) {
				return rc;
			}
		}

		if (config->operation_mode == VCNL36825T_OPERATION_MODE_AUTO) {
			rc = vcnl36825t_update(&config->i2c, VCNL36825T_REG_PS_CONF3,
					       VCNL36825T_PS_AF_MSK, VCNL36825T_PS_AF_AUTO);
			if (rc < 0) {
				return rc;
			}
		}

		k_usleep(VCNL36825T_POWER_UP_US);

		rc = vcnl36825t_update(&config->i2c, VCNL36825T_REG_PS_CONF2, VCNL36825T_PS_ST_MSK,
				       VCNL36825T_PS_ST_START);
		if (rc < 0) {
			return rc;
		}

		data->meas_timeout_us = data->meas_timeout_wakeup_us;

		break;
	case PM_DEVICE_ACTION_SUSPEND:
		rc = vcnl36825t_update(&config->i2c, VCNL36825T_REG_PS_CONF2, VCNL36825T_PS_ST_MSK,
				       VCNL36825T_PS_ST_STOP);
		if (rc < 0) {
			return rc;
		}

		if (config->operation_mode == VCNL36825T_OPERATION_MODE_AUTO) {
			rc = vcnl36825t_update(&config->i2c, VCNL36825T_REG_PS_CONF3,
					       VCNL36825T_PS_AF_MSK, VCNL36825T_PS_AF_FORCE);
			if (rc < 0) {
				return rc;
			}
		}

		/* unset LPEN-bit if active, otherwise high current draw can be observed */
		if (config->low_power) {
			rc = vcnl36825t_update(&config->i2c, VCNL36825T_REG_PS_CONF4,
					       VCNL36825T_PS_LPEN_MSK, VCNL36825T_PS_LPEN_DISABLED);
			if (rc < 0) {
				return rc;
			}
		}

		rc = vcnl36825t_update(&config->i2c, VCNL36825T_REG_PS_CONF1, VCNL36825T_PS_ON_MSK,
				       VCNL36825T_PS_OFF);
		if (rc < 0) {
			return rc;
		}
		break;
	default:
		LOG_ERR("action %d not supported", (int)action);
		return -ENOTSUP;
	}

	return 0;
}

#endif

static int vcnl36825t_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct vcnl36825t_config *config = dev->config;
	struct vcnl36825t_data *data = dev->data;
	int rc;

#if CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return -EBUSY;
	}
#endif

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_PROX:
		if (config->operation_mode == VCNL36825T_OPERATION_MODE_FORCE) {
			rc = vcnl36825t_update(&config->i2c, VCNL36825T_REG_PS_CONF3,
					       VCNL36825T_PS_TRIG_MSK, VCNL36825T_PS_TRIG_ONCE);
			if (rc < 0) {
				LOG_ERR("could not trigger proximity measurement %d", rc);
				return rc;
			}

			k_usleep(data->meas_timeout_us);

#ifdef CONFIG_PM_DEVICE
			data->meas_timeout_us = data->meas_timeout_running_us;
#endif
		}

		rc = vcnl36825t_read(&config->i2c, VCNL36825T_REG_PS_DATA, &data->proximity);
		if (rc < 0) {
			LOG_ERR("could not fetch proximity measurement %d", rc);
			return rc;
		}

		break;
	default:
		LOG_ERR("invalid sensor channel");
		return -EINVAL;
	}

	return 0;
}

static int vcnl36825t_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct vcnl36825t_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_PROX:
		val->val1 = data->proximity & VCNL36825T_OS_DATA_MSK;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int vcnl36825t_attr_set(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, const struct sensor_value *val)
{
	CHECKIF(dev == NULL) {
		LOG_ERR("dev: NULL");
		return -EINVAL;
	}

	CHECKIF(val == NULL) {
		LOG_ERR("val: NULL");
		return -EINVAL;
	}

	int __maybe_unused rc;

	switch (attr) {
	default:
#if CONFIG_VCNL36825T_TRIGGER
		rc = vcnl36825t_trigger_attr_set(dev, chan, attr, val);
		if (rc < 0) {
			return rc;
		}
#else
		return -ENOTSUP;
#endif
	}

	return 0;
}

/**
 * @brief calculate measurement timeout in us
 *
 * @param meas_duration base duration of a measurement in us*VCNL36825T_FORCED_FACTOR_SCALE
 * @param meas_factor factor which needs to be multiplied to cope with additional delays (multiplied
 *                    by VCNL36825T_FORCED_FACTOR_SCALE)
 *
 * @note
 *  Always add 1 to prevent corner case losses due to precision.
 */
static inline unsigned int vcn36825t_measurement_timeout_us(unsigned int meas_duration,
							    unsigned int forced_factor)
{
	return ((meas_duration * forced_factor) / VCNL36825T_FORCED_FACTOR_SCALE) + 1;
}

/**
 * @brief helper function to configure the registers
 *
 * @param dev pointer to the VCNL36825T instance
 */
static int vcnl36825t_init_registers(const struct device *dev)
{
	const struct vcnl36825t_config *config = dev->config;
	struct vcnl36825t_data *data = dev->data;

	int rc;
	uint16_t reg_value;

	unsigned int meas_duration = 1;

	/* reset registers as defined by the datasheet */
	const uint16_t resetValues[][2] = {
		{VCNL36825T_REG_PS_CONF1, VCNL36825T_CONF1_DEFAULT},
		{VCNL36825T_REG_PS_CONF2, VCNL36825T_CONF2_DEFAULT},
		{VCNL36825T_REG_PS_CONF3, VCNL36825T_CONF3_DEFAULT},
		{VCNL36825T_REG_PS_THDL, VCNL36825T_THDL_DEFAULT},
		{VCNL36825T_REG_PS_THDH, VCNL36825T_THDH_DEFAULT},
		{VCNL36825T_REG_PS_CANC, VCNL36825T_CANC_DEFAULT},
		{VCNL36825T_REG_PS_CONF4, VCNL36825T_CONF4_DEFAULT},
	};

	for (size_t i = 0; i < ARRAY_SIZE(resetValues); ++i) {
		vcnl36825t_write(&config->i2c, resetValues[i][0], resetValues[i][1]);
	}

	/* PS_CONF1 */
	reg_value = 0x01; /* must be set according to datasheet */
	reg_value |= VCNL36825T_PS_ON;

	rc = vcnl36825t_write(&config->i2c, VCNL36825T_REG_PS_CONF1, reg_value);
	if (rc < 0) {
		LOG_ERR("I2C for PS_ON returned %d", rc);
		return -EIO;
	}

	reg_value |= VCNL36825T_PS_CAL;
	reg_value |= 1 << 9; /* reserved, must be set by datasheet */

	rc = vcnl36825t_write(&config->i2c, VCNL36825T_REG_PS_CONF1, reg_value);
	if (rc < 0) {
		LOG_ERR("I2C for PS_CAL returned %d", rc);
	}

	k_usleep(VCNL36825T_POWER_UP_US);

	/* PS_CONF2 */
	reg_value = 0;

	switch (config->period) {
	case VCNL36825T_MEAS_PERIOD_10MS:
		reg_value |= VCNL36825T_PS_PERIOD_10MS;
		break;
	case VCNL36825T_MEAS_PERIOD_20MS:
		reg_value |= VCNL36825T_PS_PERIOD_20MS;
		break;
	case VCNL36825T_MEAS_PERIOD_40MS:
		reg_value |= VCNL36825T_PS_PERIOD_40MS;
		break;
	case VCNL36825T_MEAS_PERIOD_80MS:
		__fallthrough;
	default:
		reg_value |= VCNL36825T_PS_PERIOD_80MS;
		break;
	}

	reg_value |= VCNL36825T_PS_PERS_1;
	reg_value |= VCNL36825T_PS_ST_STOP;

	switch (config->proximity_it) {
	case VCNL36825T_PROXIMITY_INTEGRATION_1T:
		reg_value |= VCNL36825T_PS_IT_1T;
		meas_duration *= 1;
		break;
	case VCNL36825T_PROXIMITY_INTEGRATION_2T:
		reg_value |= VCNL36825T_PS_IT_2T;
		meas_duration *= 2;
		break;
	case VCNL36825T_PROXIMITY_INTEGRATION_4T:
		reg_value |= VCNL36825T_PS_IT_4T;
		meas_duration *= 4;
		break;
	case VCNL36825T_PROXIMITY_INTEGRATION_8T:
		__fallthrough;
	default:
		reg_value |= VCNL36825T_PS_IT_8T;
		meas_duration *= 8;
		break;
	}

	switch (config->multi_pulse) {
	case VCNL38652T_MULTI_PULSE_1:
		reg_value |= VCNL36825T_MPS_PULSES_1;
		break;
	case VCNL38652T_MULTI_PULSE_2:
		reg_value |= VCNL36825T_MPS_PULSES_2;
		break;
	case VCNL38652T_MULTI_PULSE_4:
		reg_value |= VCNL36825T_MPS_PULSES_4;
		break;
	case VCNL38652T_MULTI_PULSE_8:
		__fallthrough;
	default:
		reg_value |= VCNL36825T_MPS_PULSES_8;
		break;
	}

	switch (config->proximity_itb) {
	case VCNL36825T_PROXIMITY_INTEGRATION_DURATION_25us:
		reg_value |= VCNL36825T_PS_ITB_25us;
		meas_duration *= 25;
		break;
	case VCNL36825T_PROXIMITY_INTEGRATION_DURATION_50us:
		__fallthrough;
	default:
		reg_value |= VCNL36825T_PS_ITB_50us;
		meas_duration *= 50;
		break;
	}

	if (config->high_gain) {
		reg_value |= VCNL36825T_PS_HG_HIGH;
	}

	rc = vcnl36825t_write(&config->i2c, VCNL36825T_REG_PS_CONF2, reg_value);
	if (rc < 0) {
		LOG_ERR("I2C for setting PS_CONF2 returned %d", rc);
		return -EIO;
	}

	/* PS_CONF3 */
	reg_value = 0;

	if (config->operation_mode == VCNL36825T_OPERATION_MODE_FORCE) {
		reg_value |= VCNL36825T_PS_AF_FORCE;
	}

	switch (config->laser_current) {
	case VCNL36825T_LASER_CURRENT_10MS:
		reg_value |= VCNL36825T_PS_I_VCSEL_10MA;
		break;
	case VCNL36825T_LASER_CURRENT_12MS:
		reg_value |= VCNL36825T_PS_I_VCSEL_12MA;
		break;
	case VCNL36825T_LASER_CURRENT_14MS:
		reg_value |= VCNL36825T_PS_I_VCSEL_14MA;
		break;
	case VCNL36825T_LASER_CURRENT_16MS:
		reg_value |= VCNL36825T_PS_I_VCSEL_16MA;
		break;
	case VCNL36825T_LASER_CURRENT_18MS:
		reg_value |= VCNL36825T_PS_I_VCSEL_18MA;
		break;
	case VCNL36825T_LASER_CURRENT_20MS:
		__fallthrough;
	default:
		reg_value |= VCNL36825T_PS_I_VCSEL_20MA;
		break;
	}

	if (config->high_dynamic_output) {
		reg_value |= VCNL36825T_PS_HD_16BIT;
	}

	if (config->sunlight_cancellation) {
		reg_value |= VCNL36825T_PS_SC_ENABLED;
	}

	rc = vcnl36825t_write(&config->i2c, VCNL36825T_REG_PS_CONF3, reg_value);
	if (rc < 0) {
		LOG_ERR("I2C for setting PS_CONF3 returned %d", rc);
		return -EIO;
	}

	/* PS_CONF4 */
	reg_value = 0;

	if (config->low_power) {
		reg_value |= VCNL36825T_PS_LPEN_ENABLED;
	}

	switch (config->period) {
	case VCNL36825T_MEAS_PERIOD_40MS:
		reg_value |= VCNL36825T_PS_LPPER_40MS;
		break;
	case VCNL36825T_MEAS_PERIOD_80MS:
		reg_value |= VCNL36825T_PS_LPPER_80MS;
		break;
	case VCNL36825T_MEAS_PERIOD_160MS:
		reg_value |= VCNL36825T_PS_LPPER_160MS;
		break;
	case VCNL36825T_MEAS_PERIOD_320MS:
		__fallthrough;
	default:
		reg_value |= VCNL36825T_PS_LPPER_320MS;
		break;
	}

	rc = vcnl36825t_write(&config->i2c, VCNL36825T_REG_PS_CONF4, reg_value);
	if (rc < 0) {
		LOG_ERR("I2C for setting PS_CONF4 returned %d", rc);
		return -EIO;
	}

	data->meas_timeout_us =
		vcn36825t_measurement_timeout_us(meas_duration, VCNL36825T_FORCED_FACTOR_SUM);

#ifdef CONFIG_PM_DEVICE
	data->meas_timeout_running_us = data->meas_timeout_us;
	data->meas_timeout_wakeup_us = vcn36825t_measurement_timeout_us(
		meas_duration, VCNL36825T_FORCED_FACTOR_WAKEUP_SUM);

	/* ensure that the time is roughly around VCNL36825T_FORCED_WAKEUP_DELAY_MAX_US if the
	 * wakeup time is bigger but "normal" measurement time is less
	 */
	if (data->meas_timeout_wakeup_us > VCNL36825T_FORCED_WAKEUP_DELAY_MAX_US) {
		data->meas_timeout_wakeup_us =
			MAX(data->meas_timeout_running_us, VCNL36825T_FORCED_WAKEUP_DELAY_MAX_US);
	}
#endif

	return 0;
}

static int vcnl36825t_init(const struct device *dev)
{
	const struct vcnl36825t_config *config = dev->config;
	int rc;

	uint16_t reg_value;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("device is not ready");
		return -ENODEV;
	}

	rc = vcnl36825t_read(&config->i2c, VCNL36825T_REG_DEV_ID, &reg_value);
	if (rc < 0) {
		LOG_ERR("could not read device id");
		return rc;
	}

	if ((reg_value & VCNL36825T_ID_MSK) != VCNL36825T_DEVICE_ID) {
		LOG_ERR("incorrect device id (%d)", reg_value);
		return -EIO;
	}

	LOG_INF("version code: 0x%X",
		(unsigned int)FIELD_GET(VCNL36825T_VERSION_CODE_MSK, reg_value));

	rc = vcnl36825t_init_registers(dev);
	if (rc < 0) {
		return rc;
	}

#if CONFIG_VCNL36825T_TRIGGER
	rc = vcnl36825t_trigger_init(dev);
	if (rc < 0) {
		return rc;
	}
#endif
	rc = vcnl36825t_update(&config->i2c, VCNL36825T_REG_PS_CONF2, VCNL36825T_PS_ST_MSK,
			       VCNL36825T_PS_ST_START);
	if (rc < 0) {
		LOG_ERR("error starting measurement");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(sensor, vcnl36825t_driver_api) = {
	.sample_fetch = vcnl36825t_sample_fetch,
	.channel_get = vcnl36825t_channel_get,
	.attr_set = vcnl36825t_attr_set,
#if CONFIG_VCNL36825T_TRIGGER
	.trigger_set = vcnl36825t_trigger_set,
#endif
};

#define VCNL36825T_DEFINE(inst)                                                                    \
	BUILD_ASSERT(!DT_INST_PROP(inst, low_power) || (DT_INST_PROP(inst, measurement_period) >=  \
							VCNL36825T_PS_LPPER_VALUE_MIN_MS),         \
		     "measurement-period must be greater/equal 40 ms in low-power mode");          \
	BUILD_ASSERT(                                                                              \
		DT_INST_PROP(inst, low_power) || (DT_INST_PROP(inst, measurement_period) <=        \
						  VCNL36825T_PS_PERIOD_VALUE_MAX_MS),              \
		"measurement-period must be less/equal 80 ms with deactivated low-power mode");    \
	BUILD_ASSERT(!DT_INST_PROP(inst, low_power) || (DT_INST_ENUM_IDX(inst, operation_mode) ==  \
							VCNL36825T_OPERATION_MODE_AUTO),           \
		     "operation-mode \"force\" only available if low-power mode deactivated");     \
	static struct vcnl36825t_data vcnl36825t_data_##inst;                                      \
	static const struct vcnl36825t_config vcnl36825t_config_##inst = {                         \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.operation_mode = DT_INST_ENUM_IDX(inst, operation_mode),                          \
		.period = DT_INST_ENUM_IDX(inst, measurement_period),                              \
		.proximity_it = DT_INST_ENUM_IDX(inst, proximity_it),                              \
		.proximity_itb = DT_INST_ENUM_IDX(inst, proximity_itb),                            \
		.multi_pulse = DT_INST_ENUM_IDX(inst, multi_pulse),                                \
		.low_power = DT_INST_PROP(inst, low_power),                                        \
		.high_gain = DT_INST_PROP(inst, high_gain),                                        \
		.laser_current = DT_INST_ENUM_IDX(inst, laser_current),                            \
		.high_dynamic_output = DT_INST_PROP(inst, high_dynamic_output),                    \
		.sunlight_cancellation = DT_INST_PROP(inst, sunlight_cancellation),                \
		IF_ENABLED(CONFIG_VCNL36825T_TRIGGER,                                              \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                    \
			    .int_mode = DT_INST_ENUM_IDX(inst, int_mode),                          \
			    .int_proximity_count = DT_INST_PROP(inst, int_proximity_count),        \
			    .int_smart_persistence = DT_INST_PROP(inst, int_smart_persistence)))}; \
	IF_ENABLED(CONFIG_PM_DEVICE, (PM_DEVICE_DT_INST_DEFINE(inst, vcnl36825t_pm_action)));      \
	SENSOR_DEVICE_DT_INST_DEFINE(                                                              \
		inst, vcnl36825t_init,                                                             \
		COND_CODE_1(CONFIG_PM_DEVICE, (PM_DEVICE_DT_INST_GET(inst)), (NULL)),              \
		&vcnl36825t_data_##inst, &vcnl36825t_config_##inst, POST_KERNEL,                   \
		CONFIG_SENSOR_INIT_PRIORITY, &vcnl36825t_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VCNL36825T_DEFINE)
