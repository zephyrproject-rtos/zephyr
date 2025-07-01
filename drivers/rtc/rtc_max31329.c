/*
 * Copyright (c) 2024-2025 Wimansha Wijekoon <Wimanshahb@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Following is the driver for max31329 RTC from Analog Devices
 * rtc_pcf8563.c has been used as a reference to develop the base code
 *
 * with this driver following features been supported
 *
 * - RTC Alarm 1 interrupt
 * - Trickle charging
 * - Event input (uses same GPIO as alarm interrupt)
 *
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/timeutil.h>
LOG_MODULE_REGISTER(max31329, CONFIG_RTC_LOG_LEVEL);

/* Register Addresses */
#define MAX31329_REG_STATUS        0x00
#define MAX31329_REG_INT_EN        0x01
#define MAX31329_REG_RTC_RESET     0x02
#define MAX31329_REG_RTC_CONFIG1   0x03
#define MAX31329_REG_RTC_CONFIG2   0x04
#define MAX31329_REG_TIMER_CONFIG  0x05
#define MAX31329_REG_SECONDS       0x06
#define MAX31329_REG_ALM1_SEC      0x0D
#define MAX31329_REG_ALM1_MIN      0x0E
#define MAX31329_REG_ALM1_HRS      0x0F
#define MAX31329_REG_ALM1_DAY_DATE 0x10
#define MAX31329_REG_ALM1_MON      0x11
#define MAX31329_REG_ALM1_YEAR     0x12
#define MAX31329_REG_ALM2_MIN      0x13
#define MAX31329_REG_ALM2_HRS      0x14
#define MAX31329_REG_ALM2_DAY_DATE 0x15
#define MAX31329_REG_TIMER_COUNT   0x16
#define MAX31329_REG_TIMER_INIT    0x17
#define MAX31329_REG_PWR_MGMT      0x18
#define MAX31329_REG_TRICKLE_REG   0x19

#define MAX31329_HOURS_MASK    GENMASK(5, 0)
#define MAX31329_DAYS_MASK     GENMASK(5, 0)
#define MAX31329_WEEKDAYS_MASK GENMASK(2, 0)
#define MAX31329_MONTHS_MASK   GENMASK(4, 0)

#define DT_DRV_COMPAT adi_max31329

#if (DT_ANY_INST_HAS_PROP_STATUS_OKAY(inta_gpios)) &&                                              \
	(defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE))
/* The user may need only alarms but not interrupts so we will only
 * include all the interrupt code if the user configured it in the dts
 */
#define MAX31329_INT_GPIOS_IN_USE 1
#endif

#define MAX31329_RTC_ALARM_1_TIME_MASK                                                             \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_YEAR)

#define MAX31329_RTC_ALARM_2_TIME_MASK                                                             \
	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY)

struct max31329_config {
	const struct i2c_dt_spec i2c;
	bool digital_in_enable;
	uint8_t pwer_fail_threshold;
	uint8_t trickle_charging_settings;
#ifdef MAX31329_INT_GPIOS_IN_USE
	const struct gpio_dt_spec int_rtc;
#endif
};

#ifdef MAX31329_INT_GPIOS_IN_USE
/* This work will run the user callback function */
void callback_work_handler(struct k_work *work);
K_WORK_DEFINE(callback_work, callback_work_handler);
#endif

struct max31329_data {
#ifdef MAX31329_INT_GPIOS_IN_USE
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	const struct device *dev;
	struct gpio_callback intb_callback;
	struct k_work callback_work;
#endif
};

int max31329_set_time(const struct device *dev, const struct rtc_time *new_time)
{
	const struct max31329_config *config = dev->config;
	int ret = 0;
	uint8_t raw_time[7] = {0};

	/* RTC supports only for 21st and 22nd centuries */
	if (new_time->tm_year < 100) {
		LOG_WRN("supported only 21st and 22nd centuries");
		return -EINVAL;
	}

	/* Set seconds */
	raw_time[0] = bin2bcd(new_time->tm_sec);
	/* Set minutes */
	raw_time[1] = bin2bcd(new_time->tm_min);
	/* Set hours in 24hrs */
	raw_time[2] = bin2bcd(new_time->tm_hour);
	/* Set weekdays */
	raw_time[3] = new_time->tm_wday;
	/* Set days */
	raw_time[4] = bin2bcd(new_time->tm_mday);
	/*Set month */
	raw_time[5] = bin2bcd((new_time->tm_mon) + 1);
	/* Set year */
	/* if we make in to 22nd century */
	if (new_time->tm_year > 199) {
		raw_time[5] |= BIT(7);
	}
	raw_time[6] = bin2bcd((new_time->tm_year) - 100);
	/* Write to device */
	ret = i2c_burst_write_dt(&config->i2c, MAX31329_REG_SECONDS, raw_time, sizeof(raw_time));
	if (ret) {
		LOG_ERR("Error when setting time: %i", ret);
		return ret;
	}
	return 0;
}

int max31329_get_time(const struct device *dev, struct rtc_time *dest_time)
{
	const struct max31329_config *config = dev->config;
	int ret = 0;
	uint8_t raw_time[7] = {0};

	/* Starting from seconds register read till year */
	ret = i2c_burst_read_dt(&config->i2c, MAX31329_REG_SECONDS, raw_time, sizeof(raw_time));
	if (ret) {
		LOG_ERR("Fail to get time (error %d)", ret);
		return ret;
	}

	/* Rtc does not have nanosec */
	dest_time->tm_nsec = 0;
	/* Get seconds */
	dest_time->tm_sec = bcd2bin(raw_time[0]);
	/* Get minutes */
	dest_time->tm_min = bcd2bin(raw_time[1]);
	/* Get hours in 24hr format */
	dest_time->tm_hour = bcd2bin(raw_time[2] & MAX31329_HOURS_MASK);
	/* Get days */
	dest_time->tm_wday = bcd2bin(raw_time[3] & MAX31329_WEEKDAYS_MASK);
	/* Get weekdays */
	dest_time->tm_mday = bcd2bin(raw_time[4] & MAX31329_DAYS_MASK);
	/* Get month */
	dest_time->tm_mon = bcd2bin(raw_time[5] & MAX31329_MONTHS_MASK);
	/* We need 0 based(0-11) */
	dest_time->tm_mon -= 1; /* Get year */
	/* Century Bit is ignored */
	dest_time->tm_year = bcd2bin(raw_time[6]);
	dest_time->tm_year += 100;
	if (IS_BIT_SET(raw_time[5], 7)) {
		dest_time->tm_year += 100;
	}
	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int max31329_alarm_get_supported_fields(const struct device *dev, uint16_t id,
					       uint16_t *mask)
{
	ARG_UNUSED(dev);

	/* RTC supports two alarms, with two different supporting fields */
	if (id > 1) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}
	if (!id) {
		*mask = MAX31329_RTC_ALARM_1_TIME_MASK;
	} else {
		*mask = MAX31329_RTC_ALARM_2_TIME_MASK;
	}
	return 0;
}

static int max31329_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				   const struct rtc_time *timeptr)
{
	const struct max31329_config *config = dev->config;
	uint8_t regs[6];
	int ret;

	if (id > 1) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	if (id != 0) {
		LOG_ERR(" ID %d not supporting", id);
		return -ENOTSUP;
	}

	/* id =0 mapped to Alarm 1 */
	/* id =1 mapped to Alarm 2 */
	const uint16_t valid_mask =
		id ? MAX31329_RTC_ALARM_2_TIME_MASK : MAX31329_RTC_ALARM_1_TIME_MASK;

	if ((mask & ~valid_mask) != 0) {
		LOG_ERR("invalid alarm field mask 0x%04x", mask);
		return -EINVAL;
	}

	/*
	 * The 7/6 bit is used as enabled/disabled flag.
	 * The mask will clean it and also the unused bits
	 */
	/* Alarm 1 settings */
	/*  SEC */
	if ((mask & RTC_ALARM_TIME_MASK_SECOND) != 0) {
		regs[0] = bin2bcd(timeptr->tm_sec) & ~BIT(7);
	} else {
		LOG_DBG("SEC FLAG NOT SET");
		regs[0] = BIT(7);
	}

	/*  MIN */
	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) != 0) {
		regs[1] = bin2bcd(timeptr->tm_min) & ~BIT(7);
	} else {
		LOG_DBG("MIN FLAG NOT SET");
		regs[1] = BIT(7);
	}

	/*  HR */
	if ((mask & RTC_ALARM_TIME_MASK_HOUR) != 0) {
		regs[2] = bin2bcd(timeptr->tm_hour) & (MAX31329_HOURS_MASK & ~BIT(7));
	} else {
		LOG_DBG("HR FLAG NOT SET");
		regs[2] = BIT(7);
	}

	/*  MONTH DAY */
	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) != 0) {
		regs[3] = bin2bcd(timeptr->tm_mday) & (MAX31329_DAYS_MASK & ~(BIT(7) | BIT(6)));
	} else {
		LOG_DBG("MONTH DAY FLAG NOT SET");
		regs[3] = BIT(7);
	}

	/*  MONTH */
	if ((mask & RTC_ALARM_TIME_MASK_MONTH) != 0) {
		regs[4] = bin2bcd((timeptr->tm_mon) + 1) & (MAX31329_MONTHS_MASK & ~BIT(7));
	} else {
		LOG_DBG("MONTH FLAG NOT SET");
		regs[4] = BIT(7);
	}

	/*  YEAR */
	if ((mask & RTC_ALARM_TIME_MASK_YEAR) != 0) {
		regs[5] = bin2bcd((timeptr->tm_year) - 100) & ~BIT(7);
	} else {
		LOG_DBG("YEAR FLAG NOT SET");
		regs[4] |= BIT(6);
		regs[5] = 0x00;
	}

	if (!id) {
		ret = i2c_burst_write_dt(&config->i2c, MAX31329_REG_ALM1_SEC, regs, sizeof(regs));
		if (ret) {
			LOG_ERR("Error when setting alarm 1: %i", ret);
			return ret;
		}
	} else {
		ret = i2c_burst_write_dt(&config->i2c, MAX31329_REG_ALM2_MIN, &regs[1], 3);
		if (ret) {
			LOG_ERR("Error when setting alarm 2: %i", ret);
			return ret;
		}
	}

	/* Enable RTC side interrupts */
	i2c_reg_read_byte_dt(&config->i2c, MAX31329_REG_INT_EN, &regs[0]);
	regs[0] |= BIT(id ? 1 : 0);

	ret = i2c_reg_write_byte_dt(&config->i2c, MAX31329_REG_INT_EN, regs[0]);
	if (ret) {
		LOG_ERR("Error when enabling interrupts: %i", ret);
		return ret;
	}

	/* Set intA as Alarm1 interrupts: Note Table 2 in data sheet */
	regs[0] = 0x00;
	ret = i2c_reg_write_byte_dt(&config->i2c, MAX31329_REG_RTC_CONFIG2, regs[0]);
	if (ret) {
		LOG_ERR("Error when setting intB: %i", ret);
		return ret;
	}

	return 0;
}

static int max31329_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				   struct rtc_time *timeptr)
{
	const struct max31329_config *config = dev->config;
	int ret = 0;
	uint8_t raw_time[6] = {0};

	if (id != 0) {
		LOG_ERR(" ID %d not supporting", id);
		return -ENOTSUP;
	}

	/* Starting from seconds register read till year */
	ret = i2c_burst_read_dt(&config->i2c, MAX31329_REG_ALM1_SEC, raw_time, sizeof(raw_time));
	if (ret) {
		LOG_ERR("Fail to get time (error %d)", ret);
		return ret;
	}

	timeptr->tm_nsec = 0;
	/* Get seconds */
	timeptr->tm_sec = bcd2bin(raw_time[0]);
	/* Get minutes */
	timeptr->tm_min = bcd2bin(raw_time[1]);
	/* Get hours in 24hr format */
	timeptr->tm_hour = bcd2bin(raw_time[2] & MAX31329_HOURS_MASK);
	/* Get days */
	timeptr->tm_mday = bcd2bin(raw_time[3] & MAX31329_DAYS_MASK);
	/* Get month */
	timeptr->tm_mon = bcd2bin(raw_time[4] & MAX31329_MONTHS_MASK);
	/* we need 0 based(0-11)*/
	timeptr->tm_mon -= 1;
	/* Get year */
	/* Century Bit is ignored */
	timeptr->tm_year = bcd2bin(raw_time[5]);
	timeptr->tm_year += 100;
	if (IS_BIT_SET(raw_time[4], 7)) {
		timeptr->tm_year += 100;
	}
	return 0;
}

static int max31329_alarm_is_pending(const struct device *dev, uint16_t id)
{
	const struct max31329_config *config = dev->config;
	uint8_t status;
	int ret;

	/* Read the status register from the RTC */
	ret = i2c_reg_read_byte_dt(&config->i2c, MAX31329_REG_STATUS, &status);
	if (ret) {
		LOG_ERR("Error when reading status: %i", ret);
		return ret;
	}

	/* Check the alarm interrupt flag */
	return (status & BIT(id)) != 0;
}
#endif

#ifdef MAX31329_INT_GPIOS_IN_USE
void callback_work_handler(struct k_work *work)
{
	/* This function is run as a work so the user can spend here all the necessary time */
	struct max31329_data *data = CONTAINER_OF(work, struct max31329_data, callback_work);

	if (data->alarm_callback == NULL) {
		LOG_WRN("No MAX31329 alarm callback function provided");
	} else {
		data->alarm_callback(data->dev, 0, data->alarm_user_data);
	}
}

/* The function called when the clock alarm activates the interrupt */
void gpio_callback_function(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct max31329_data *data = CONTAINER_OF(cb, struct max31329_data, intb_callback);

	LOG_DBG("MAX31329 interrupt detected");
	/* By using a work we are able to run "heavier" code */
	k_work_submit(&(data->callback_work));
}
#endif

static int max31329_alarm_set_callback(const struct device *dev, uint16_t id,
				       rtc_alarm_callback callback, void *user_data)
{
#ifndef MAX31329_INT_GPIOS_IN_USE
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
#else
	const struct max31329_config *config = dev->config;
	struct max31329_data *data = dev->data;
	int ret;

	if (config->int_rtc.port == NULL) {
		return -ENOTSUP;
	}

	if (id != 0) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	/* Disable interrupt if callback is NULL */
	if (callback == NULL) {
		/* Disable RTC side interrupts */
		LOG_DBG("Alarm %d interrupt Disabled ", id + 1);
		return 0;
	}

	data->alarm_callback = callback;
	data->alarm_user_data = user_data;
	data->dev = dev;

	ret = gpio_pin_configure_dt(&config->int_rtc, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d", ret, config->int_rtc.port->name,
			config->int_rtc.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_rtc, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d", ret,
			config->int_rtc.port->name, config->int_rtc.pin);
		return ret;
	}

	gpio_init_callback(&data->intb_callback, gpio_callback_function, BIT(config->int_rtc.pin));
	gpio_add_callback(config->int_rtc.port, &data->intb_callback);
	LOG_DBG("Alarm %d interrupt Enable ", id + 1);

	return 0;
#endif
}

static const struct rtc_driver_api max31329_driver_api = {
	.set_time = max31329_set_time,
	.get_time = max31329_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = max31329_alarm_get_supported_fields,
	.alarm_set_time = max31329_alarm_set_time,
	.alarm_get_time = max31329_alarm_get_time,
	.alarm_is_pending = max31329_alarm_is_pending,
	.alarm_set_callback = max31329_alarm_set_callback,
#endif
};

int max31329_init(const struct device *dev)
{
	const struct max31329_config *config = dev->config;
	int ret;
	uint8_t reg_value;

	/* Check if the I2C bus is ready */
	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Failed to get pointer to %s device!", config->i2c.bus->name);
		return -EINVAL;
	}

	/* Check if the device is alive by reading the status register */
	ret = i2c_reg_read_byte_dt(&config->i2c, MAX31329_REG_STATUS, &reg_value);
	if (ret) {
		LOG_ERR("Failed to read from MAX31329! (err %i)", ret);
		return -EIO;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, MAX31329_REG_INT_EN, 0x00);
	if (ret) {
		LOG_ERR("Failed to disable interrupts! (err %i)", ret);
		return ret;
	}

	/* Enable power fail threshold based on DT properties */
	reg_value = config->pwer_fail_threshold ? config->pwer_fail_threshold << 2 : 0;
	ret = i2c_reg_write_byte_dt(&config->i2c, MAX31329_REG_PWR_MGMT, reg_value);
	if (ret) {
		LOG_ERR("Failed to configure power fail! (err %i)", ret);
		return ret;
	}

	/* Enable trickle charging based on DT properties */
	reg_value = config->trickle_charging_settings;
	reg_value |= config->trickle_charging_settings ? BIT(7) : 0;
	ret = i2c_reg_write_byte_dt(&config->i2c, MAX31329_REG_TRICKLE_REG, reg_value);
	if (ret) {
		LOG_ERR("Failed to configure trickle charging! (err %i)", ret);
		return ret;
	}

	/* Enable Digital-In (Event) if specified in DT */
	if (config->digital_in_enable) {
		ret = i2c_reg_read_byte_dt(&config->i2c, MAX31329_REG_INT_EN, &reg_value);
		if (ret) {
			LOG_ERR("Failed to read INT_EN register! (err %i)", ret);
			return ret;
		}
		/* Enable DIE (Digital-In Event) */
		reg_value |= BIT(3);
		ret = i2c_reg_write_byte_dt(&config->i2c, MAX31329_REG_INT_EN, reg_value);
		if (ret) {
			LOG_ERR("Error when enabling Digital-In (DIE): %i", ret);
			return ret;
		}
	}

#ifdef MAX31329_INT_GPIOS_IN_USE
	struct max31329_data *data = dev->data;

	data->callback_work = callback_work;

#endif

	LOG_INF("%s is initialized!", dev->name);
	return 0;
}

#define MAX31329_INIT(inst)                                                                        \
	static const struct max31329_config max31329_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.digital_in_enable = DT_INST_PROP(inst, digital_in_enable),                        \
		.pwer_fail_threshold = DT_INST_ENUM_IDX_OR(inst, pvft, 0),                         \
		.trickle_charging_settings = DT_INST_ENUM_IDX_OR(inst, trickle, 0),                \
		IF_ENABLED(MAX31329_INT_GPIOS_IN_USE,						   \
	(.int_rtc = GPIO_DT_SPEC_INST_GET_OR(inst, inta_gpios, {0})))};             \
	static struct max31329_data max31329_data_##inst;                                          \
	DEVICE_DT_INST_DEFINE(inst, &max31329_init, NULL, &max31329_data_##inst,                   \
			      &max31329_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,      \
			      &max31329_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31329_INIT)
