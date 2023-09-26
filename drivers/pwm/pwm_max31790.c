/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max31790

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pwm/max31790.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_max31790, CONFIG_PWM_LOG_LEVEL);

#define MAX31790_OSCILLATOR_FREQUENCY_IN_HZ 32768
#define MAX31790_PWMTARGETDUTYCYCLE_MAXIMUM ((1 << 9) - 1)
#define MAX31790_CHANNEL_COUNT              6

struct max31790_config {
	struct i2c_dt_spec i2c;
};

struct max31790_data {
	struct k_mutex lock;
};

#define MAX37190_REGISTER_GLOBALCONFIGURATION               0x00
#define MAX37190_REGISTER_PWMFREQUENCY                      0x01
#define MAX37190_REGISTER_FANCONFIGURATION(channel)         (0x02 + channel)
#define MAX31790_REGISTER_PWMOUTTARGETDUTYCYCLEMSB(channel) (0x40 + 2 * channel)
#define MAX31790_REGISTER_FANDYNAMICS(channel)              (0x08 + channel)
#define MAX31790_REGISTER_TACHTARGETCOUNTMSB(channel)       (0x50 + 2 * channel)

#define MAX37190_GLOBALCONFIGURATION_STANDBY_BIT           BIT(7)
#define MAX37190_FANXCONFIGURATION_MONITOR_BIT             BIT(4)
#define MAX37190_FANXCONFIGURATION_TACHINPUTENABLED_BIT    BIT(3)
#define MAX37190_FANXCONFIGURATION_LOCKEDROTOR_BIT         BIT(2)
#define MAX37190_FANXCONFIGURATION_LOCKEDROTORPOLARITY_BIT BIT(1)
#define MAX37190_FANXCONFIGURATION_TACH_BIT                BIT(0)
#define MAX37190_FANXCONFIGURATION_MODE_BIT                BIT(7)
#define MAX37190_FANXDYNAMICS_ASYMMETRICRATEOFCHANGE_BIT   BIT(1)

#define MAX37190_FANXDYNAMICS_SPEEDRANGE_LENGTH      3
#define MAX37190_FANXDYNAMICS_SPEEDRANGE_POS         5
#define MAX37190_FANXDYNAMICS_PWMRATEOFCHANGE_LENGTH 3
#define MAX37190_FANXDYNAMICS_PWMRATEOFCHANGE_POS    2
#define MAX37190_PWMFREQUENCY_PWM_LENGTH             4
#define MAX37190_PWMFREQUENCY_PWM4TO6_POS            4
#define MAX37190_PWMFREQUENCY_PWM1TO3_LENGTH         4
#define MAX37190_PWMFREQUENCY_PWM1TO3_POS            0
#define MAX37190_FANXCONFIGURATION_SPINUP_LENGTH     2
#define MAX37190_FANXCONFIGURATION_SPINUP_POS        5

#define PWM_MAX31790_FLAG_SPEED_RANGE_GET(flags)                                                   \
	FIELD_GET(GENMASK(MAX37190_FANXDYNAMICS_SPEEDRANGE_LENGTH +                                \
				  PWM_MAX31790_FLAG_SPEED_RANGE_POS - 1,                           \
			  PWM_MAX31790_FLAG_SPEED_RANGE_POS),                                      \
		  flags)
#define PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_GET(flags)                                            \
	FIELD_GET(GENMASK(MAX37190_FANXDYNAMICS_PWMRATEOFCHANGE_LENGTH +                           \
				  PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS - 1,                    \
			  PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS),                               \
		  flags)

static void max31790_set_fandynamics_speedrange(uint8_t *destination, uint8_t value)
{
	uint8_t length = MAX37190_FANXDYNAMICS_SPEEDRANGE_LENGTH;
	uint8_t pos = MAX37190_FANXDYNAMICS_SPEEDRANGE_POS;

	*destination &= ~GENMASK(pos + length - 1, pos);
	*destination |= FIELD_PREP(GENMASK(pos + length - 1, pos), value);
}

static void max31790_set_fandynamics_pwmrateofchange(uint8_t *destination, uint8_t value)
{
	uint8_t length = MAX37190_FANXDYNAMICS_PWMRATEOFCHANGE_LENGTH;
	uint8_t pos = MAX37190_FANXDYNAMICS_PWMRATEOFCHANGE_POS;

	*destination &= ~GENMASK(pos + length - 1, pos);
	*destination |= FIELD_PREP(GENMASK(pos + length - 1, pos), value);
}

static void max31790_set_pwmfrequency(uint8_t *destination, uint8_t channel, uint8_t value)
{
	uint8_t length = MAX37190_PWMFREQUENCY_PWM_LENGTH;
	uint8_t pos = (channel / 3) * 4;

	*destination &= ~GENMASK(pos + length - 1, pos);
	*destination |= FIELD_PREP(GENMASK(pos + length - 1, pos), value);
}

static uint8_t max31790_get_pwmfrequency(uint8_t value, uint8_t channel)
{
	uint8_t length = MAX37190_PWMFREQUENCY_PWM_LENGTH;
	uint8_t pos = (channel / 3) * 4;

	return FIELD_GET(GENMASK(pos + length - 1, pos), value);
}

static void max31790_set_fanconfiguration_spinup(uint8_t *destination, uint8_t value)
{
	uint8_t length = MAX37190_FANXCONFIGURATION_SPINUP_LENGTH;
	uint8_t pos = MAX37190_FANXCONFIGURATION_SPINUP_POS;

	*destination &= ~GENMASK(pos + length - 1, pos);
	*destination |= FIELD_PREP(GENMASK(pos + length - 1, pos), value);
}

static int max31790_read_register(const struct device *dev, uint8_t address, uint8_t *value)
{
	const struct max31790_config *config = dev->config;
	int result;

	result = i2c_reg_read_byte_dt(&config->i2c, address, value);
	if (result != 0) {
		LOG_ERR("unable to read register 0x%02X, error %i", address, result);
	}

	LOG_DBG("read value 0x%02X from register 0x%02X", *value, address);

	return result;
}

static int max31790_write_register_uint8(const struct device *dev, uint8_t address, uint8_t value)
{
	const struct max31790_config *config = dev->config;
	int result;

	LOG_DBG("writing value 0x%02X to register 0x%02X", value, address);
	result = i2c_reg_write_byte_dt(&config->i2c, address, value);
	if (result != 0) {
		LOG_ERR("unable to write register 0x%02X, error %i", address, result);
	}

	return result;
}

static int max31790_write_register_uint16(const struct device *dev, uint8_t address, uint16_t value)
{
	const struct max31790_config *config = dev->config;
	int result;
	uint8_t buffer[] = {
		address,
		value >> 8,
		value,
	};

	LOG_DBG("writing value 0x%04X to address 0x%02X", value, address);
	result = i2c_write_dt(&config->i2c, buffer, sizeof(buffer));
	if (result != 0) {
		LOG_ERR("unable to write to address 0x%02X, error %i", address, result);
	}

	return result;
}

static bool max31790_convert_pwm_frequency_into_hz(uint16_t *result, uint8_t pwm_frequency)
{
	switch (pwm_frequency) {
	case 0:
		*result = 25;
		return true;
	case 1:
		*result = 30;
		return true;
	case 2:
		*result = 35;
		return true;
	case 3:
		*result = 100;
		return true;
	case 4:
		*result = 125;
		return true;
	case 5:
		*result = 150; /* actually 149.7, according to the datasheet */
		return true;
	case 6:
		*result = 1250;
		return true;
	case 7:
		*result = 1470;
		return true;
	case 8:
		*result = 3570;
		return true;
	case 9:
		*result = 5000;
		return true;
	case 10:
		*result = 12500;
		return true;
	case 11:
		*result = 25000;
		return true;
	default:
		LOG_ERR("invalid value %i for PWM frequency register", pwm_frequency);
		return false;
	}
}

static bool max31790_convert_pwm_frequency_into_register(uint8_t *result, uint32_t pwm_frequency)
{
	switch (pwm_frequency) {
	case 25:
		*result = 0;
		return true;
	case 30:
		*result = 1;
		return true;
	case 35:
		*result = 2;
		return true;
	case 100:
		*result = 3;
		return true;
	case 125:
		*result = 4;
		return true;
	case 150: /* actually 149.7, according to the datasheet */
		*result = 5;
		return true;
	case 1250:
		*result = 6;
		return true;
	case 1470:
		*result = 7;
		return true;
	case 3570:
		*result = 8;
		return true;
	case 5000:
		*result = 9;
		return true;
	case 12500:
		*result = 10;
		return true;
	case 25000:
		*result = 11;
		return true;
	default:
		LOG_ERR("invalid value %i for PWM frequency in Hz", pwm_frequency);
		return false;
	}
}

static int max31790_set_cycles_internal(const struct device *dev, uint32_t channel,
					uint32_t period_count, uint32_t pulse_count,
					pwm_flags_t flags)
{
	int result;
	uint8_t pwm_frequency_channel_value;
	uint8_t value_pwm_frequency;
	uint8_t value_fan_configuration;
	uint8_t value_fan_dynamics;
	uint8_t value_speed_range = PWM_MAX31790_FLAG_SPEED_RANGE_GET(flags);
	uint8_t value_pwm_rate_of_change = PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_GET(flags);

	if (!max31790_convert_pwm_frequency_into_register(&pwm_frequency_channel_value,
							  period_count)) {
		return -EINVAL;
	}

	result = max31790_read_register(dev, MAX37190_REGISTER_PWMFREQUENCY, &value_pwm_frequency);
	if (result != 0) {
		return result;
	}

	max31790_set_pwmfrequency(&value_pwm_frequency, channel, pwm_frequency_channel_value);

	result = max31790_write_register_uint8(dev, MAX37190_REGISTER_PWMFREQUENCY,
					       value_pwm_frequency);
	if (result != 0) {
		return result;
	}

	value_fan_configuration = 0;
	value_fan_dynamics = 0;

	if (flags & PWM_MAX31790_FLAG_SPIN_UP) {
		max31790_set_fanconfiguration_spinup(&value_fan_configuration, 2);
	} else {
		max31790_set_fanconfiguration_spinup(&value_fan_configuration, 0);
	}

	value_fan_configuration &= ~MAX37190_FANXCONFIGURATION_MONITOR_BIT;
	value_fan_configuration &= ~MAX37190_FANXCONFIGURATION_LOCKEDROTOR_BIT;
	value_fan_configuration &= ~MAX37190_FANXCONFIGURATION_LOCKEDROTORPOLARITY_BIT;
	value_fan_configuration &= ~MAX37190_FANXCONFIGURATION_TACH_BIT;
	value_fan_configuration |= MAX37190_FANXCONFIGURATION_TACHINPUTENABLED_BIT;

	max31790_set_fandynamics_speedrange(&value_fan_dynamics, value_speed_range);
	max31790_set_fandynamics_pwmrateofchange(&value_fan_dynamics, value_pwm_rate_of_change);
	value_fan_dynamics |= MAX37190_FANXDYNAMICS_ASYMMETRICRATEOFCHANGE_BIT;

	if ((flags & PWM_MAX31790_FLAG_RPM_MODE) == 0) {
		LOG_DBG("PWM mode");
		uint16_t pwm_target_duty_cycle =
			pulse_count * MAX31790_PWMTARGETDUTYCYCLE_MAXIMUM / period_count;
		value_fan_configuration &= ~MAX37190_FANXCONFIGURATION_MODE_BIT;

		result = max31790_write_register_uint16(
			dev, MAX31790_REGISTER_PWMOUTTARGETDUTYCYCLEMSB(channel),
			pwm_target_duty_cycle);
		if (result != 0) {
			return result;
		}
	} else {
		LOG_DBG("RPM mode");
		value_fan_configuration |= MAX37190_FANXCONFIGURATION_MODE_BIT;

		result = max31790_write_register_uint16(
			dev, MAX31790_REGISTER_TACHTARGETCOUNTMSB(channel), pulse_count);
		if (result != 0) {
			return result;
		}
	}

	result = max31790_write_register_uint8(dev, MAX37190_REGISTER_FANCONFIGURATION(channel),
					       value_fan_configuration);
	if (result != 0) {
		return result;
	}

	result = max31790_write_register_uint8(dev, MAX31790_REGISTER_FANDYNAMICS(channel),
					       value_fan_dynamics);
	if (result != 0) {
		return result;
	}

	return 0;
}

static int max31790_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_count,
			       uint32_t pulse_count, pwm_flags_t flags)
{
	struct max31790_data *data = dev->data;
	int result;

	LOG_DBG("set period %i with pulse %i for channel %i and flags 0x%04X", period_count,
		pulse_count, channel, flags);

	if (channel > MAX31790_CHANNEL_COUNT) {
		LOG_ERR("invalid channel number %i", channel);
		return -EINVAL;
	}

	if (period_count == 0) {
		LOG_ERR("period count must be > 0");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	result = max31790_set_cycles_internal(dev, channel, period_count, pulse_count, flags);
	k_mutex_unlock(&data->lock);
	return result;
}

static int max31790_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	struct max31790_data *data = dev->data;
	int result;
	bool success;
	uint8_t pwm_frequency_register;
	uint8_t pwm_frequency = 1;
	uint16_t pwm_frequency_in_hz;

	if (channel > MAX31790_CHANNEL_COUNT) {
		LOG_ERR("invalid channel number %i", channel);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	result = max31790_read_register(dev, MAX37190_REGISTER_GLOBALCONFIGURATION,
					&pwm_frequency_register);

	if (result != 0) {
		k_mutex_unlock(&data->lock);
		return result;
	}

	pwm_frequency = max31790_get_pwmfrequency(pwm_frequency_register, channel);
	success = max31790_convert_pwm_frequency_into_hz(&pwm_frequency_in_hz, pwm_frequency);

	if (!success) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	*cycles = pwm_frequency_in_hz;

	k_mutex_unlock(&data->lock);
	return 0;
}

static const struct pwm_driver_api max31790_api = {
	.set_cycles = max31790_set_cycles,
	.get_cycles_per_sec = max31790_get_cycles_per_sec,
};

static int max31790_init(const struct device *dev)
{
	const struct max31790_config *config = dev->config;
	struct max31790_data *data = dev->data;
	int result;
	uint8_t reg_value;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	result = max31790_read_register(dev, MAX37190_REGISTER_GLOBALCONFIGURATION, &reg_value);
	if (result != 0) {
		return result;
	}

	if ((reg_value & MAX37190_GLOBALCONFIGURATION_STANDBY_BIT) != 0) {
		LOG_DBG("taking PWM controller out of standby");

		reg_value &= ~MAX37190_GLOBALCONFIGURATION_STANDBY_BIT;
		result = max31790_write_register_uint8(dev, MAX37190_REGISTER_GLOBALCONFIGURATION,
						       reg_value);
		if (result != 0) {
			return result;
		}

		result = max31790_read_register(dev, MAX37190_REGISTER_GLOBALCONFIGURATION,
						&reg_value);
		if (result != 0) {
			return result;
		}

		if ((reg_value & MAX37190_GLOBALCONFIGURATION_STANDBY_BIT) != 0) {
			LOG_ERR("unable to take PWM controller out of standby");
			return -ENODEV;
		}
	}

	return 0;
}

#define MAX31790_INIT(inst)                                                                        \
	static const struct max31790_config max31790_##inst##_config = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	static struct max31790_data max31790_##inst##_data;                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, max31790_init, NULL, &max31790_##inst##_data,                  \
			      &max31790_##inst##_config, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,    \
			      &max31790_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31790_INIT);
