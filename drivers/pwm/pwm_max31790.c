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
#define MAX31790_TACHTARGETCOUNT_MAXIMUM    ((1 << 11) - 1)

struct max31790_config {
	struct i2c_dt_spec i2c;
	size_t channel_count;
};

struct max31790_data {
	struct k_mutex lock;
};

enum max31790_register {
	MAX37190REGISTER_GLOBALCONFIGURATION = 0x00,
	MAX37190REGISTER_PWMFREQUENCY = 0x01,
	MAX37190REGISTER_FAN1CONFIGURATION = 0x02,
	MAX37190REGISTER_FAN2CONFIGURATION = 0x03,
	MAX37190REGISTER_FAN3CONFIGURATION = 0x04,
	MAX37190REGISTER_FAN4CONFIGURATION = 0x05,
	MAX37190REGISTER_FAN5CONFIGURATION = 0x06,
	MAX37190REGISTER_FAN6CONFIGURATION = 0x07,
	MAX37190REGISTER_FAN1DYNAMICS = 0x08,
	MAX37190REGISTER_FAN2DYNAMICS = 0x09,
	MAX37190REGISTER_FAN3DYNAMICS = 0x0A,
	MAX37190REGISTER_FAN4DYNAMICS = 0x0B,
	MAX37190REGISTER_FAN5DYNAMICS = 0x0C,
	MAX37190REGISTER_FAN6DYNAMICS = 0x0D,
	MAX37190REGISTER_USERBYTE1 = 0x0E,
	MAX37190REGISTER_USERBYTE2 = 0x0F,
	MAX37190REGISTER_FANFAULTSTATUS2 = 0x10,
	MAX37190REGISTER_FANFAULTSTATUS1 = 0x11,
	MAX37190REGISTER_FANFAULTMASK2 = 0x12,
	MAX37190REGISTER_FANFAULTMASK1 = 0x13,
	MAX37190REGISTER_FAILEDFANOPTIONS = 0x14,
	MAX37190REGISTER_USERBYTE3 = 0x15,
	MAX37190REGISTER_USERBYTE4 = 0x16,
	MAX37190REGISTER_USERBYTE5 = 0x17,
	MAX37190REGISTER_TACH1COUNTMSB = 0x18,
	MAX37190REGISTER_TACH1COUNTLSB = 0x19,
	MAX37190REGISTER_TACH2COUNTMSB = 0x1A,
	MAX37190REGISTER_TACH2COUNTLSB = 0x1B,
	MAX37190REGISTER_TACH3COUNTMSB = 0x1C,
	MAX37190REGISTER_TACH3COUNTLSB = 0x1D,
	MAX37190REGISTER_TACH4COUNTMSB = 0x1E,
	MAX37190REGISTER_TACH4COUNTLSB = 0x1F,
	MAX37190REGISTER_TACH5COUNTMSB = 0x20,
	MAX37190REGISTER_TACH5COUNTLSB = 0x21,
	MAX37190REGISTER_TACH6COUNTMSB = 0x22,
	MAX37190REGISTER_TACH6COUNTLSB = 0x23,
	MAX37190REGISTER_TACH7COUNTMSB = 0x24,
	MAX37190REGISTER_TACH7COUNTLSB = 0x25,
	MAX37190REGISTER_TACH8COUNTMSB = 0x26,
	MAX37190REGISTER_TACH8COUNTLSB = 0x27,
	MAX37190REGISTER_TACH9COUNTMSB = 0x28,
	MAX37190REGISTER_TACH9COUNTLSB = 0x29,
	MAX37190REGISTER_TACH10COUNTMSB = 0x2A,
	MAX37190REGISTER_TACH10COUNTLSB = 0x2B,
	MAX37190REGISTER_TACH11COUNTMSB = 0x2C,
	MAX37190REGISTER_TACH11COUNTLSB = 0x2D,
	MAX37190REGISTER_TACH12COUNTMSB = 0x2E,
	MAX37190REGISTER_TACH12COUNTLSB = 0x2F,
	MAX37190REGISTER_PWMOUT1DUTYCYCLEMSB = 0x30,
	MAX37190REGISTER_PWMOUT1DUTYCYCLELSB = 0x31,
	MAX37190REGISTER_PWMOUT2DUTYCYCLEMSB = 0x32,
	MAX37190REGISTER_PWMOUT2DUTYCYCLELSB = 0x33,
	MAX37190REGISTER_PWMOUT3DUTYCYCLEMSB = 0x34,
	MAX37190REGISTER_PWMOUT3DUTYCYCLELSB = 0x35,
	MAX37190REGISTER_PWMOUT4DUTYCYCLEMSB = 0x36,
	MAX37190REGISTER_PWMOUT4DUTYCYCLELSB = 0x37,
	MAX37190REGISTER_PWMOUT5DUTYCYCLEMSB = 0x38,
	MAX37190REGISTER_PWMOUT5DUTYCYCLELSB = 0x39,
	MAX37190REGISTER_PWMOUT6DUTYCYCLEMSB = 0x3A,
	MAX37190REGISTER_PWMOUT6DUTYCYCLELSB = 0x3B,
	MAX37190REGISTER_PWMOUT1TARGETDUTYCYCLEMSB = 0x40,
	MAX37190REGISTER_PWMOUT1TARGETDUTYCYCLELSB = 0x41,
	MAX37190REGISTER_PWMOUT2TARGETDUTYCYCLEMSB = 0x42,
	MAX37190REGISTER_PWMOUT2TARGETDUTYCYCLELSB = 0x43,
	MAX37190REGISTER_PWMOUT3TARGETDUTYCYCLEMSB = 0x44,
	MAX37190REGISTER_PWMOUT3TARGETDUTYCYCLELSB = 0x45,
	MAX37190REGISTER_PWMOUT4TARGETDUTYCYCLEMSB = 0x46,
	MAX37190REGISTER_PWMOUT4TARGETDUTYCYCLELSB = 0x47,
	MAX37190REGISTER_PWMOUT5TARGETDUTYCYCLEMSB = 0x48,
	MAX37190REGISTER_PWMOUT5TARGETDUTYCYCLELSB = 0x49,
	MAX37190REGISTER_PWMOUT6TARGETDUTYCYCLEMSB = 0x4A,
	MAX37190REGISTER_PWMOUT6TARGETDUTYCYCLELSB = 0x4B,
	MAX37190REGISTER_USERBYTE6 = 0x4C,
	MAX37190REGISTER_USERBYTE7 = 0x4D,
	MAX37190REGISTER_USERBYTE8 = 0x4E,
	MAX37190REGISTER_USERBYTE9 = 0x4F,
	MAX37190REGISTER_TACH1TARGETCOUNTMSB = 0x50,
	MAX37190REGISTER_TACH1TARGETCOUNTLSB = 0x51,
	MAX37190REGISTER_TACH2TARGETCOUNTMSB = 0x52,
	MAX37190REGISTER_TACH2TARGETCOUNTLSB = 0x53,
	MAX37190REGISTER_TACH3TARGETCOUNTMSB = 0x54,
	MAX37190REGISTER_TACH3TARGETCOUNTLSB = 0x55,
	MAX37190REGISTER_TACH4TARGETCOUNTMSB = 0x56,
	MAX37190REGISTER_TACH4TARGETCOUNTLSB = 0x57,
	MAX37190REGISTER_TACH5TARGETCOUNTMSB = 0x58,
	MAX37190REGISTER_TACH5TARGETCOUNTLSB = 0x59,
	MAX37190REGISTER_TACH6TARGETCOUNTMSB = 0x5A,
	MAX37190REGISTER_TACH6TARGETCOUNTLSB = 0x5B,
	MAX37190REGISTER_USERBYTE10 = 0x5C,
	MAX37190REGISTER_USERBYTE11 = 0x5D,
	MAX37190REGISTER_USERBYTE12 = 0x5E,
	MAX37190REGISTER_USERBYTE13 = 0x5F,
	MAX37190REGISTER_WINDOW1 = 0x60,
	MAX37190REGISTER_WINDOW2 = 0x61,
	MAX37190REGISTER_WINDOW3 = 0x62,
	MAX37190REGISTER_WINDOW4 = 0x63,
	MAX37190REGISTER_WINDOW5 = 0x64,
	MAX37190REGISTER_WINDOW6 = 0x65,
	MAX37190REGISTER_USERBYTE14 = 0x66,
	MAX37190REGISTER_USERBYTE15 = 0x67,
};

#define MAX37190REGISTER_GET_VALUE(value, pos, length)                                             \
	FIELD_GET(GENMASK(pos + length - 1, pos), value)
#define MAX37190REGISTER_SET_VALUE(target, value, pos, length)                                     \
	target &= ~GENMASK(pos + length - 1, pos);                                                 \
	target |= FIELD_PREP(GENMASK(pos + length - 1, pos), value)

#define MAX37190REGISTER_GLOBALCONFIGURATION_STANDBY_LENGTH 1
#define MAX37190REGISTER_GLOBALCONFIGURATION_STANDBY_POS    7
#define MAX37190REGISTER_GLOBALCONFIGURATION_STANDBY_GET(value)                                    \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_GLOBALCONFIGURATION_STANDBY_POS,        \
				   MAX37190REGISTER_GLOBALCONFIGURATION_STANDBY_LENGTH)
#define MAX37190REGISTER_GLOBALCONFIGURATION_STANDBY_SET(target, value)                            \
	MAX37190REGISTER_SET_VALUE(target, value,                                                  \
				   MAX37190REGISTER_GLOBALCONFIGURATION_STANDBY_POS,               \
				   MAX37190REGISTER_GLOBALCONFIGURATION_STANDBY_LENGTH)
#define MAX37190REGISTER_GLOBALCONFIGURATION_RESET_LENGTH 1
#define MAX37190REGISTER_GLOBALCONFIGURATION_RESET_POS	  6
#define MAX37190REGISTER_GLOBALCONFIGURATION_RESET_GET(value)                                      \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_GLOBALCONFIGURATION_RESET_POS,          \
				   MAX37190REGISTER_GLOBALCONFIGURATION_RESET_LENGTH)
#define MAX37190REGISTER_GLOBALCONFIGURATION_RESET_SET(target, value)                              \
	MAX37190REGISTER_SET_VALUE(target, value, MAX37190REGISTER_GLOBALCONFIGURATION_RESET_POS,  \
				   MAX37190REGISTER_GLOBALCONFIGURATION_RESET_LENGTH)
#define MAX37190REGISTER_GLOBALCONFIGURATION_NOTBUSTIMEOUT_LENGTH 1
#define MAX37190REGISTER_GLOBALCONFIGURATION_NOTBUSTIMEOUT_POS	  5
#define MAX37190REGISTER_GLOBALCONFIGURATION_NOTBUSTIMEOUT_GET(value)                              \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_GLOBALCONFIGURATION_NOTBUSTIMEOUT_POS,  \
				   MAX37190REGISTER_GLOBALCONFIGURATION_NOTBUSTIMEOUT_LENGTH)
#define MAX37190REGISTER_GLOBALCONFIGURATION_NOTBUSTIMEOUT_SET(target, value)                      \
	MAX37190REGISTER_SET_VALUE(target, value,                                                  \
				   MAX37190REGISTER_GLOBALCONFIGURATION_NOTBUSTIMEOUT_POS,         \
				   MAX37190REGISTER_GLOBALCONFIGURATION_NOTBUSTIMEOUT_LENGTH)
#define MAX37190REGISTER_GLOBALCONFIGURATION_OSC_LENGTH 1
#define MAX37190REGISTER_GLOBALCONFIGURATION_OSC_POS	3
#define MAX37190REGISTER_GLOBALCONFIGURATION_OSC_GET(value)                                        \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_GLOBALCONFIGURATION_OSC_POS,            \
				   MAX37190REGISTER_GLOBALCONFIGURATION_OSC_LENGTH)
#define MAX37190REGISTER_GLOBALCONFIGURATION_OSC_SET(target, value)                                \
	MAX37190REGISTER_SET_VALUE(target, value, MAX37190REGISTER_GLOBALCONFIGURATION_OSC_POS,    \
				   MAX37190REGISTER_GLOBALCONFIGURATION_OSC_LENGTH)
#define MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOG_LENGTH 2
#define MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOG_POS	1
#define MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOG_GET(value)                                \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOG_POS,    \
				   MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOG_LENGTH)
#define MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOG_SET(target, value)                        \
	MAX37190REGISTER_SET_VALUE(target, value,                                                  \
				   MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOG_POS,           \
				   MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOG_LENGTH)
#define MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOGSTATUS_LENGTH 1
#define MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOGSTATUS_POS    0
#define MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOGSTATUS_GET(value)                          \
	MAX37190REGISTER_GET_VALUE(value,                                                          \
				   MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOGSTATUS_POS,     \
				   MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOGSTATUS_LENGTH)
#define MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOGSTATUS_SET(target, value)                  \
	MAX37190REGISTER_SET_VALUE(target, value,                                                  \
				   MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOGSTATUS_POS,     \
				   MAX37190REGISTER_GLOBALCONFIGURATION_I2CWATCHDOGSTATUS_LENGTH)
#define MAX37190REGISTER_PWMFREQUENCY_PWM4TO6_LENGTH 4
#define MAX37190REGISTER_PWMFREQUENCY_PWM4TO6_POS    4
#define MAX37190REGISTER_PWMFREQUENCY_PWM4TO6_GET(value)                                           \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_PWMFREQUENCY_PWM4TO6_POS,               \
				   MAX37190REGISTER_PWMFREQUENCY_PWM4TO6_LENGTH)
#define MAX37190REGISTER_PWMFREQUENCY_PWM4TO6_SET(target, value)                                   \
	MAX37190REGISTER_SET_VALUE(target, value, MAX37190REGISTER_PWMFREQUENCY_PWM4TO6_POS,       \
				   MAX37190REGISTER_PWMFREQUENCY_PWM4TO6_LENGTH)
#define MAX37190REGISTER_PWMFREQUENCY_PWM1TO3_LENGTH 4
#define MAX37190REGISTER_PWMFREQUENCY_PWM1TO3_POS    0
#define MAX37190REGISTER_PWMFREQUENCY_PWM1TO3_GET(value)                                           \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_PWMFREQUENCY_PWM1TO3_POS,               \
				   MAX37190REGISTER_PWMFREQUENCY_PWM1TO3_LENGTH)
#define MAX37190REGISTER_PWMFREQUENCY_PWM1TO3_SET(target, value)                                   \
	MAX37190REGISTER_SET_VALUE(target, value, MAX37190REGISTER_PWMFREQUENCY_PWM1TO3_POS,       \
				   MAX37190REGISTER_PWMFREQUENCY_PWM1TO3_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_MODE_LENGTH 1
#define MAX37190REGISTER_FANXCONFIGURATION_MODE_POS    7
#define MAX37190REGISTER_FANXCONFIGURATION_MODE_GET(value)                                         \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_FANXCONFIGURATION_MODE_POS,             \
				   MAX37190REGISTER_FANXCONFIGURATION_MODE_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_MODE_SET(target, value)                                 \
	MAX37190REGISTER_SET_VALUE(target, value, MAX37190REGISTER_FANXCONFIGURATION_MODE_POS,     \
				   MAX37190REGISTER_FANXCONFIGURATION_MODE_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_SPINUP_LENGTH 2
#define MAX37190REGISTER_FANXCONFIGURATION_SPINUP_POS	 5
#define MAX37190REGISTER_FANXCONFIGURATION_SPINUP_GET(value)                                       \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_FANXCONFIGURATION_SPINUP_POS,           \
				   MAX37190REGISTER_FANXCONFIGURATION_SPINUP_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_SPINUP_SET(target, value)                               \
	MAX37190REGISTER_SET_VALUE(target, value, MAX37190REGISTER_FANXCONFIGURATION_SPINUP_POS,   \
				   MAX37190REGISTER_FANXCONFIGURATION_SPINUP_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_MONITOR_LENGTH 1
#define MAX37190REGISTER_FANXCONFIGURATION_MONITOR_POS	  4
#define MAX37190REGISTER_FANXCONFIGURATION_MONITOR_GET(value)                                      \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_FANXCONFIGURATION_MONITOR_POS,          \
				   MAX37190REGISTER_FANXCONFIGURATION_MODE_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_MONITOR_SET(target, value)                              \
	MAX37190REGISTER_SET_VALUE(target, value, MAX37190REGISTER_FANXCONFIGURATION_MONITOR_POS,  \
				   MAX37190REGISTER_FANXCONFIGURATION_MODE_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_TACHINPUTENABLED_LENGTH 1
#define MAX37190REGISTER_FANXCONFIGURATION_TACHINPUTENABLED_POS	   3
#define MAX37190REGISTER_FANXCONFIGURATION_TACHINPUTENABLED_GET(value)                             \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_FANXCONFIGURATION_TACHINPUTENABLED_POS, \
				   MAX37190REGISTER_FANXCONFIGURATION_TACHINPUTENABLED_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_TACHINPUTENABLED_SET(target, value)                     \
	MAX37190REGISTER_SET_VALUE(target, value,                                                  \
				   MAX37190REGISTER_FANXCONFIGURATION_TACHINPUTENABLED_POS,        \
				   MAX37190REGISTER_FANXCONFIGURATION_TACHINPUTENABLED_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTOR_LENGTH 1
#define MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTOR_POS    2
#define MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTOR_GET(value)                                  \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTOR_POS,      \
				   MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTOR_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTOR_SET(target, value)                          \
	MAX37190REGISTER_SET_VALUE(target, value,                                                  \
				   MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTOR_POS,             \
				   MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTOR_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTORPOLARITY_LENGTH 1
#define MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTORPOLARITY_POS    1
#define MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTORPOLARITY_GET(value)                          \
	MAX37190REGISTER_GET_VALUE(value,                                                          \
				   MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTORPOLARITY_POS,     \
				   MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTORPOLARITY_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTORPOLARITY_SET(target, value)                  \
	MAX37190REGISTER_SET_VALUE(target, value,                                                  \
				   MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTORPOLARITY_POS,     \
				   MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTORPOLARITY_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_TACH_LENGTH 1
#define MAX37190REGISTER_FANXCONFIGURATION_TACH_POS    0
#define MAX37190REGISTER_FANXCONFIGURATION_TACH_GET(value)                                         \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_FANXCONFIGURATION_TACH_POS,             \
				   MAX37190REGISTER_FANXCONFIGURATION_TACH_LENGTH)
#define MAX37190REGISTER_FANXCONFIGURATION_TACH_SET(target, value)                                 \
	MAX37190REGISTER_SET_VALUE(target, value, MAX37190REGISTER_FANXCONFIGURATION_TACH_POS,     \
				   MAX37190REGISTER_FANXCONFIGURATION_TACH_LENGTH)
#define MAX37190REGISTER_FANXDYNAMICS_SPEEDRANGE_LENGTH 3
#define MAX37190REGISTER_FANXDYNAMICS_SPEEDRANGE_POS	5
#define MAX37190REGISTER_FANXDYNAMICS_SPEEDRANGE_GET(value)                                        \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_FANXDYNAMICS_SPEEDRANGE_POS,            \
				   MAX37190REGISTER_FANXDYNAMICS_SPEEDRANGE_LENGTH)
#define MAX37190REGISTER_FANXDYNAMICS_SPEEDRANGE_SET(target, value)                                \
	MAX37190REGISTER_SET_VALUE(target, value, MAX37190REGISTER_FANXDYNAMICS_SPEEDRANGE_POS,    \
				   MAX37190REGISTER_FANXDYNAMICS_SPEEDRANGE_LENGTH)
#define MAX37190REGISTER_FANXDYNAMICS_PWMRATEOFCHANGE_LENGTH 3
#define MAX37190REGISTER_FANXDYNAMICS_PWMRATEOFCHANGE_POS    2
#define MAX37190REGISTER_FANXDYNAMICS_PWMRATEOFCHANGE_GET(value)                                   \
	MAX37190REGISTER_GET_VALUE(value, MAX37190REGISTER_FANXDYNAMICS_PWMRATEOFCHANGE_POS,       \
				   MAX37190REGISTER_FANXDYNAMICS_PWMRATEOFCHANGE_LENGTH)
#define MAX37190REGISTER_FANXDYNAMICS_PWMRATEOFCHANGE_SET(target, value)                           \
	MAX37190REGISTER_SET_VALUE(target, value,                                                  \
				   MAX37190REGISTER_FANXDYNAMICS_PWMRATEOFCHANGE_POS,              \
				   MAX37190REGISTER_FANXDYNAMICS_PWMRATEOFCHANGE_LENGTH)
#define MAX37190REGISTER_FANXDYNAMICS_ASYMMETRICRATEOFCHANGE_LENGTH 1
#define MAX37190REGISTER_FANXDYNAMICS_ASYMMETRICRATEOFCHANGE_POS    1
#define MAX37190REGISTER_FANXDYNAMICS_ASYMMETRICRATEOFCHANGE_GET(value)                            \
	MAX37190REGISTER_GET_VALUE(value,                                                          \
				   MAX37190REGISTER_FANXDYNAMICS_ASYMMETRICRATEOFCHANGE_POS,       \
				   MAX37190REGISTER_FANXDYNAMICS_ASYMMETRICRATEOFCHANGE_LENGTH)
#define MAX37190REGISTER_FANXDYNAMICS_ASYMMETRICRATEOFCHANGE_SET(target, value)                    \
	MAX37190REGISTER_SET_VALUE(target, value,                                                  \
				   MAX37190REGISTER_FANXDYNAMICS_ASYMMETRICRATEOFCHANGE_POS,       \
				   MAX37190REGISTER_FANXDYNAMICS_ASYMMETRICRATEOFCHANGE_LENGTH)

static int max31790_read_register(const struct device *dev, enum max31790_register address,
				  uint8_t *value)
{
	const struct max31790_config *config = dev->config;
	int result = i2c_reg_read_byte_dt(&config->i2c, address, value);

	LOG_DBG("read value 0x%02X from register 0x%02X", *value, address);

	if (result != 0) {
		LOG_ERR("unable to read register 0x%02X, error %i", address, result);
	}

	return result;
}

static int max31790_write_register(const struct device *dev, enum max31790_register address,
				   uint8_t value)
{
	const struct max31790_config *config = dev->config;

	LOG_DBG("writing value 0x%02X to register 0x%02X", value, address);
	int result = i2c_reg_write_byte_dt(&config->i2c, address, value);

	if (result != 0) {
		LOG_ERR("unable to write register 0x%02X, error %i", address, result);
	}

	return result;
}

static bool max31790_convert_pwm_frequency_into_hz(uint16_t *result, uint8_t pwm_frequency)
{
	switch (pwm_frequency) {
	case 0b0000:
		*result = 25;
		return true;
	case 0b0001:
		*result = 30;
		return true;
	case 0b0010:
		*result = 35;
		return true;
	case 0b0011:
		*result = 100;
		return true;
	case 0b0100:
		*result = 125;
		return true;
	case 0b0101:
		*result = 150; /* actually 149.7, according to the datasheet */
		return true;
	case 0b0110:
		*result = 1250;
		return true;
	case 0b0111:
		*result = 1470;
		return true;
	case 0b1000:
		*result = 3570;
		return true;
	case 0b1001:
		*result = 5000;
		return true;
	case 0b1010:
		*result = 12500;
		return true;
	case 0b1011:
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
		*result = 0b0000;
		return true;
	case 30:
		*result = 0b0001;
		return true;
	case 35:
		*result = 0b0010;
		return true;
	case 100:
		*result = 0b0011;
		return true;
	case 125:
		*result = 0b0100;
		return true;
	case 150: /* actually 149.7, according to the datasheet */
		*result = 0b0101;
		return true;
	case 1250:
		*result = 0b0110;
		return true;
	case 1470:
		*result = 0b0111;
		return true;
	case 3570:
		*result = 0b1000;
		return true;
	case 5000:
		*result = 0b1001;
		return true;
	case 12500:
		*result = 0b1010;
		return true;
	case 25000:
		*result = 0b1011;
		return true;
	default:
		LOG_ERR("invalid value %i for PWM frequency in Hz", pwm_frequency);
		return false;
	}
}

static bool max31790_convert_speed_range_into_value(uint8_t *result, uint8_t speed_range)
{
	switch (speed_range) {
	case 0b000:
		*result = 1;
		return true;
	case 0b001:
		*result = 2;
		return true;
	case 0b010:
		*result = 4;
		return true;
	case 0b011:
		*result = 8;
		return true;
	case 0b100:
		*result = 16;
		return true;
	case 0b101:
	case 0b110:
	case 0b111:
		*result = 32;
		return true;
	default:
		LOG_ERR("invalid value %i for speed range register", speed_range);
		return false;
	}
}

static int max31790_set_cycles_internal(const struct device *dev, uint32_t channel,
					uint32_t period_count, uint32_t pulse_count,
					pwm_flags_t flags)
{
	int result;
	uint8_t pwm_frequency_channel;
	enum max31790_register register_fan_configuration;
	enum max31790_register register_pwm_target_duty_cycle_msb;
	enum max31790_register register_pwm_target_duty_cycle_lsb;
	enum max31790_register register_fan_dynamics;
	enum max31790_register register_tach_target_count_msb;
	enum max31790_register register_tach_target_count_lsb;
	uint8_t value_pwm_frequency;
	uint8_t value_fan_configuration;
	uint8_t value_fan_dynamics;
	uint8_t value_speed_range = (flags >> PWM_MAX31790_FLAG_SPEED_RANGE_POS) &
				    GENMASK(MAX37190REGISTER_FANXDYNAMICS_SPEEDRANGE_LENGTH - 1, 0);
	uint8_t value_pwm_rate_of_change =
		(flags >> PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS) &
		GENMASK(MAX37190REGISTER_FANXDYNAMICS_PWMRATEOFCHANGE_LENGTH - 1, 0);
	uint16_t tach_target_count = 0;

	if (!max31790_convert_pwm_frequency_into_register(&pwm_frequency_channel, period_count)) {
		return -EINVAL;
	}

	result = max31790_read_register(dev, MAX37190REGISTER_PWMFREQUENCY, &value_pwm_frequency);

	if (result != 0) {
		return result;
	}

	switch (channel) {
	case 0:
	case 1:
	case 2:
		MAX37190REGISTER_PWMFREQUENCY_PWM1TO3_SET(value_pwm_frequency,
							  pwm_frequency_channel);
		break;
	case 3:
	case 4:
	case 5:
		MAX37190REGISTER_PWMFREQUENCY_PWM4TO6_SET(value_pwm_frequency,
							  pwm_frequency_channel);
		break;
	default:
		LOG_ERR("invalid channel %i", channel);
		return -EINVAL;
	}

	result = max31790_write_register(dev, MAX37190REGISTER_PWMFREQUENCY, value_pwm_frequency);

	if (result != 0) {
		return result;
	}

	value_fan_configuration = 0;
	value_fan_dynamics = 0;

	if (flags & PWM_MAX31790_FLAG_SPIN_UP) {
		MAX37190REGISTER_FANXCONFIGURATION_SPINUP_SET(value_fan_configuration, 0b10);
	} else {
		MAX37190REGISTER_FANXCONFIGURATION_SPINUP_SET(value_fan_configuration, 0b00);
	}

	MAX37190REGISTER_FANXCONFIGURATION_MONITOR_SET(value_fan_configuration, 0b0);
	MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTOR_SET(value_fan_configuration, 0b0);
	MAX37190REGISTER_FANXCONFIGURATION_LOCKEDROTORPOLARITY_SET(value_fan_configuration, 0b0);
	MAX37190REGISTER_FANXCONFIGURATION_TACH_SET(value_fan_configuration, 0b0);
	MAX37190REGISTER_FANXCONFIGURATION_TACHINPUTENABLED_SET(value_fan_configuration, 0b1);

	MAX37190REGISTER_FANXDYNAMICS_SPEEDRANGE_SET(value_fan_dynamics, value_speed_range);
	MAX37190REGISTER_FANXDYNAMICS_PWMRATEOFCHANGE_SET(value_fan_dynamics,
							  value_pwm_rate_of_change);
	MAX37190REGISTER_FANXDYNAMICS_ASYMMETRICRATEOFCHANGE_SET(value_fan_dynamics, 0b1);

	switch (channel) {
	case 0:
		register_fan_configuration = MAX37190REGISTER_FAN1CONFIGURATION;
		register_pwm_target_duty_cycle_msb = MAX37190REGISTER_PWMOUT1TARGETDUTYCYCLEMSB;
		register_pwm_target_duty_cycle_lsb = MAX37190REGISTER_PWMOUT1TARGETDUTYCYCLELSB;
		register_fan_dynamics = MAX37190REGISTER_FAN1DYNAMICS;
		register_tach_target_count_msb = MAX37190REGISTER_TACH1TARGETCOUNTMSB;
		register_tach_target_count_lsb = MAX37190REGISTER_TACH1TARGETCOUNTLSB;
		break;
	case 1:
		register_fan_configuration = MAX37190REGISTER_FAN2CONFIGURATION;
		register_pwm_target_duty_cycle_msb = MAX37190REGISTER_PWMOUT2TARGETDUTYCYCLEMSB;
		register_pwm_target_duty_cycle_lsb = MAX37190REGISTER_PWMOUT2TARGETDUTYCYCLELSB;
		register_fan_dynamics = MAX37190REGISTER_FAN2DYNAMICS;
		register_tach_target_count_msb = MAX37190REGISTER_TACH2TARGETCOUNTMSB;
		register_tach_target_count_lsb = MAX37190REGISTER_TACH2TARGETCOUNTLSB;
		break;
	case 2:
		register_fan_configuration = MAX37190REGISTER_FAN3CONFIGURATION;
		register_pwm_target_duty_cycle_msb = MAX37190REGISTER_PWMOUT3TARGETDUTYCYCLEMSB;
		register_pwm_target_duty_cycle_lsb = MAX37190REGISTER_PWMOUT3TARGETDUTYCYCLELSB;
		register_fan_dynamics = MAX37190REGISTER_FAN3DYNAMICS;
		register_tach_target_count_msb = MAX37190REGISTER_TACH3TARGETCOUNTMSB;
		register_tach_target_count_lsb = MAX37190REGISTER_TACH3TARGETCOUNTLSB;
		break;
	case 3:
		register_fan_configuration = MAX37190REGISTER_FAN4CONFIGURATION;
		register_pwm_target_duty_cycle_msb = MAX37190REGISTER_PWMOUT4TARGETDUTYCYCLEMSB;
		register_pwm_target_duty_cycle_lsb = MAX37190REGISTER_PWMOUT4TARGETDUTYCYCLELSB;
		register_fan_dynamics = MAX37190REGISTER_FAN4DYNAMICS;
		register_tach_target_count_msb = MAX37190REGISTER_TACH4TARGETCOUNTMSB;
		register_tach_target_count_lsb = MAX37190REGISTER_TACH4TARGETCOUNTLSB;
		break;
	case 4:
		register_fan_configuration = MAX37190REGISTER_FAN5CONFIGURATION;
		register_pwm_target_duty_cycle_msb = MAX37190REGISTER_PWMOUT5TARGETDUTYCYCLEMSB;
		register_pwm_target_duty_cycle_lsb = MAX37190REGISTER_PWMOUT5TARGETDUTYCYCLELSB;
		register_fan_dynamics = MAX37190REGISTER_FAN5DYNAMICS;
		register_tach_target_count_msb = MAX37190REGISTER_TACH5TARGETCOUNTMSB;
		register_tach_target_count_lsb = MAX37190REGISTER_TACH5TARGETCOUNTLSB;
		break;
	case 5:
		register_fan_configuration = MAX37190REGISTER_FAN6CONFIGURATION;
		register_pwm_target_duty_cycle_msb = MAX37190REGISTER_PWMOUT6TARGETDUTYCYCLEMSB;
		register_pwm_target_duty_cycle_lsb = MAX37190REGISTER_PWMOUT6TARGETDUTYCYCLELSB;
		register_fan_dynamics = MAX37190REGISTER_FAN6DYNAMICS;
		register_tach_target_count_msb = MAX37190REGISTER_TACH6TARGETCOUNTMSB;
		register_tach_target_count_lsb = MAX37190REGISTER_TACH6TARGETCOUNTLSB;
		break;
	default:
		LOG_ERR("invalid channel %i", channel);
		return -EINVAL;
	}

	if ((flags & PWM_MAX31790_FLAG_RPM_MODE) == 0) {
		LOG_DBG("PWM mode");
		uint16_t pwm_target_duty_cycle =
			pulse_count * MAX31790_PWMTARGETDUTYCYCLE_MAXIMUM / period_count;
		uint8_t value_pwm_target_duty_cycle_msb = pwm_target_duty_cycle >> 1;
		uint8_t value_pwm_target_duty_cycle_lsb = (pwm_target_duty_cycle & 0x01) << 7;

		MAX37190REGISTER_FANXCONFIGURATION_MODE_SET(value_fan_configuration, 0b0);

		result = max31790_write_register(dev, register_pwm_target_duty_cycle_msb,
						 value_pwm_target_duty_cycle_msb);

		if (result != 0) {
			return result;
		}

		result = max31790_write_register(dev, register_pwm_target_duty_cycle_lsb,
						 value_pwm_target_duty_cycle_lsb);

		if (result != 0) {
			return result;
		}
	} else {
		LOG_DBG("RPM mode");
		uint8_t speed_range_factor;

		if (!max31790_convert_speed_range_into_value(&speed_range_factor,
							     value_speed_range)) {
			return -EINVAL;
		}

		if (pulse_count > 0) {
			/* This formula was taken from the datasheet and adapted to the PWM API*/
			tach_target_count = speed_range_factor *
					    (MAX31790_OSCILLATOR_FREQUENCY_IN_HZ / 256) *
					    period_count / pulse_count;
		} else {
			tach_target_count = MAX31790_TACHTARGETCOUNT_MAXIMUM;
		}

		if (tach_target_count > MAX31790_TACHTARGETCOUNT_MAXIMUM) {
			tach_target_count = MAX31790_TACHTARGETCOUNT_MAXIMUM;
		}

		uint8_t value_tach_target_count_msb = tach_target_count >> 3;
		uint8_t value_tach_target_count_lsb = (tach_target_count & 0x05) << 5;

		MAX37190REGISTER_FANXCONFIGURATION_MODE_SET(value_fan_configuration, 0b1);

		result = max31790_write_register(dev, register_tach_target_count_msb,
						 value_tach_target_count_msb);

		if (result != 0) {
			return result;
		}

		result = max31790_write_register(dev, register_tach_target_count_lsb,
						 value_tach_target_count_lsb);

		if (result != 0) {
			return result;
		}
	}

	result = max31790_write_register(dev, register_fan_configuration, value_fan_configuration);

	if (result != 0) {
		return result;
	}

	result = max31790_write_register(dev, register_fan_dynamics, value_fan_dynamics);

	if (result != 0) {
		return result;
	}

	return 0;
}

static int max31790_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_count,
			       uint32_t pulse_count, pwm_flags_t flags)
{
	const struct max31790_config *config = dev->config;
	struct max31790_data *data = dev->data;
	int result;

	LOG_DBG("set period %i with pulse %i for channel %i and flags 0x%04X", period_count,
		pulse_count, channel, flags);

	if (channel > config->channel_count) {
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
	const struct max31790_config *config = dev->config;
	struct max31790_data *data = dev->data;
	int result;
	uint8_t pwm_frequency_register;
	uint8_t pwm_frequency = 1;
	uint16_t pwm_frequency_in_hz;

	if (channel > config->channel_count) {
		LOG_ERR("invalid channel number %i", channel);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	result = max31790_read_register(dev, MAX37190REGISTER_GLOBALCONFIGURATION,
					&pwm_frequency_register);

	if (result == 0) {
		switch (channel) {
		case 0:
		case 1:
		case 2:
			pwm_frequency =
				MAX37190REGISTER_PWMFREQUENCY_PWM1TO3_GET(pwm_frequency_register);
			break;
		case 3:
		case 4:
		case 5:
			pwm_frequency =
				MAX37190REGISTER_PWMFREQUENCY_PWM4TO6_GET(pwm_frequency_register);
			break;
		}

		if (max31790_convert_pwm_frequency_into_hz(&pwm_frequency_in_hz, pwm_frequency)) {
			*cycles = pwm_frequency_in_hz;
		} else {
			result = -EINVAL;
		}
	}
	k_mutex_unlock(&data->lock);

	return result;
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
	uint8_t register_value;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	result = max31790_read_register(dev, MAX37190REGISTER_GLOBALCONFIGURATION, &register_value);

	if (result != 0) {
		return result;
	}

	if (MAX37190REGISTER_GLOBALCONFIGURATION_STANDBY_GET(register_value) == 0b1) {
		LOG_ERR("PWM controller is in standby");
		return -ENODEV;
	}

	return 0;
}

#define MAX31790_INIT(inst)                                                                        \
	static const struct max31790_config max31790_##inst##_config = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.channel_count = DT_INST_PROP(inst, channel_count),                                \
	};                                                                                         \
                                                                                                   \
	static struct max31790_data max31790_##inst##_data;                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, max31790_init, NULL, &max31790_##inst##_data,                  \
			      &max31790_##inst##_config, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,    \
			      &max31790_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31790_INIT);
