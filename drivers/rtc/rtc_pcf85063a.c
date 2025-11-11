/*
 * Copyright (c) 2025 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "rtc_utils.h"

#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pcf85063a);

#define DT_DRV_COMPAT nxp_pcf85063a

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int1_gpios) &&                                                \
	(defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE))
/* The user may need only alarms but not interrupts so we will only
 * include all the interrupt code if the user configured it in the dts
 */
#define PCF85063A_INT1_GPIOS_IN_USE 1
#endif

#define PCF85063A_CONTROL1_REGISTER  0x00
#define PCF85063A_CONTROL2_REGISTER  0x01
#define PCF85063A_TIME_DATE_REGISTER 0x04
#define PCF85063A_ALARM_REGISTER     0x0B

#define PCF85063A_CONTROL2_REGISTER_AIE_EN BIT(7)
#define PCF85063A_CONTROL1_REGISTER_12_24  BIT(1)

#define PCF85063A_SECONDS_MASK  GENMASK(6, 0)
#define PCF85063A_MINUTES_MASK  GENMASK(6, 0)
#define PCF85063A_HOURS_MASK    GENMASK(5, 0)
#define PCF85063A_DAYS_MASK     GENMASK(5, 0)
#define PCF85063A_WEEKDAYS_MASK GENMASK(2, 0)
#define PCF85063A_MONTHS_MASK   GENMASK(4, 0)

#define PCF85063A_YEARS_OFFSET  100
#define PCF85063A_MONTHS_OFFSET 1

#define PCF85063A_RTC_ALARM_TIME_MASK                                                              \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_WEEKDAY)

#define PCF85063A_RTC_TIME_MASK                                                                    \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

struct pcf85063a_config {
	const struct i2c_dt_spec i2c;
#ifdef PCF85063A_INT1_GPIOS_IN_USE
	const struct gpio_dt_spec int1;
#endif
};

#ifdef PCF85063A_INT1_GPIOS_IN_USE
static void callback_work_handler(struct k_work *work);
K_WORK_DEFINE(callback_work, callback_work_handler);
#endif

struct pcf85063a_data {
#ifdef PCF85063A_INT1_GPIOS_IN_USE
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	const struct device *dev;
	struct gpio_callback int1_callback;
	struct k_work callback_work;
#endif
};

static int pcf85063a_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct pcf85063a_config *config = dev->config;
	int ret;
	uint8_t raw_time[7];

	if (timeptr->tm_year < PCF85063A_YEARS_OFFSET ||
	    timeptr->tm_year > PCF85063A_YEARS_OFFSET + 99) {
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, PCF85063A_RTC_TIME_MASK)) {
		LOG_ERR("invalid time");
		return -EINVAL;
	}

	raw_time[0] = bin2bcd(timeptr->tm_sec);
	raw_time[1] = bin2bcd(timeptr->tm_min);
	raw_time[2] = bin2bcd(timeptr->tm_hour);
	raw_time[3] = bin2bcd(timeptr->tm_mday);
	raw_time[4] = timeptr->tm_wday;
	raw_time[5] = bin2bcd((timeptr->tm_mon + PCF85063A_MONTHS_OFFSET));
	raw_time[6] = bin2bcd(timeptr->tm_year - PCF85063A_YEARS_OFFSET);

	ret = i2c_burst_write_dt(&config->i2c, PCF85063A_TIME_DATE_REGISTER, raw_time,
				 sizeof(raw_time));
	if (ret) {
		LOG_ERR("Error when setting time: %i", ret);
		return ret;
	}

	return 0;
}

static int pcf85063a_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct pcf85063a_config *config = dev->config;
	int ret;
	uint8_t raw_time[7];

	ret = i2c_burst_read_dt(&config->i2c, PCF85063A_TIME_DATE_REGISTER, raw_time,
				sizeof(raw_time));
	if (ret) {
		LOG_ERR("Unable to get time. Err: %i", ret);
		return ret;
	}

	/* Seconds register bit7 = OS (oscillator stopped). */
	if (raw_time[0] & BIT(7)) {
		LOG_DBG("Oscillator stop flag set (OS=1)");
		return -ENODATA;
	}

	memset(timeptr, 0U, sizeof(*timeptr));

	timeptr->tm_sec = bcd2bin(raw_time[0] & PCF85063A_SECONDS_MASK);
	timeptr->tm_min = bcd2bin(raw_time[1] & PCF85063A_MINUTES_MASK);
	timeptr->tm_hour = bcd2bin(raw_time[2] & PCF85063A_HOURS_MASK);
	timeptr->tm_mday = bcd2bin(raw_time[3] & PCF85063A_DAYS_MASK);
	timeptr->tm_wday = raw_time[4] & PCF85063A_WEEKDAYS_MASK;
	timeptr->tm_mon = bcd2bin(raw_time[5] & PCF85063A_MONTHS_MASK) - PCF85063A_MONTHS_OFFSET;
	timeptr->tm_year = bcd2bin(raw_time[6]) + PCF85063A_YEARS_OFFSET;

	timeptr->tm_isdst = -1;

	return 0;
}

#ifdef PCF85063A_INT1_GPIOS_IN_USE

static void callback_work_handler(struct k_work *work)
{
	struct pcf85063a_data *data = CONTAINER_OF(work, struct pcf85063a_data, callback_work);

	if (data->alarm_callback == NULL) {
		LOG_WRN("No PCF85063A alarm callback function provided");
	} else {
		data->alarm_callback(data->dev, 0, data->alarm_user_data);
	}
}

static void gpio_callback_function(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct pcf85063a_data *data = CONTAINER_OF(cb, struct pcf85063a_data, int1_callback);

	LOG_DBG("PCF85063A interrupt detected");
	k_work_submit(&(data->callback_work));
}

#endif /* PCF85063A_INT1_GPIOS_IN_USE */

#ifdef CONFIG_RTC_ALARM

static int pcf85063a_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	*mask = PCF85063A_RTC_ALARM_TIME_MASK;

	return 0;
}

static int pcf85063a_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				    const struct rtc_time *timeptr)
{
	const struct pcf85063a_config *config = dev->config;
	uint8_t regs[5]; /* seconds, minutes, hours, day, weekday (0Bh..0Fh) */
	int ret;

	if (id != 0) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("invalid alarm time");
		return -EINVAL;
	}

	/* Seconds alarm (AEN_S bit7 == 1 => disabled, 0 => enabled) */
	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		regs[0] = bin2bcd(timeptr->tm_sec) & PCF85063A_SECONDS_MASK;
	} else {
		regs[0] = BIT(7);
	}

	/* Minute alarm */
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		regs[1] = bin2bcd(timeptr->tm_min) & PCF85063A_MINUTES_MASK;
	} else {
		regs[1] = BIT(7);
	}

	/* Hour alarm (assume 24-hour mode) */
	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		regs[2] = bin2bcd(timeptr->tm_hour) & PCF85063A_HOURS_MASK;
	} else {
		regs[2] = BIT(7);
	}

	/* Day (month day) alarm */
	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		regs[3] = bin2bcd(timeptr->tm_mday) & PCF85063A_DAYS_MASK;
	} else {
		regs[3] = BIT(7);
	}

	/* Weekday alarm */
	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		regs[4] = bin2bcd(timeptr->tm_wday) & PCF85063A_WEEKDAYS_MASK;
	} else {
		regs[4] = BIT(7);
	}

	ret = i2c_burst_write_dt(&config->i2c, PCF85063A_ALARM_REGISTER, regs, sizeof(regs));
	if (ret) {
		LOG_ERR("Error when setting alarm: %i", ret);
		return ret;
	}

	i2c_reg_write_byte_dt(&config->i2c, PCF85063A_CONTROL2_REGISTER,
			      PCF85063A_CONTROL2_REGISTER_AIE_EN);

	return 0;
}

static int pcf85063a_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				    struct rtc_time *timeptr)
{
	const struct pcf85063a_config *config = dev->config;
	uint8_t regs[5];
	int err;

	if (id != 0) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	err = i2c_burst_read_dt(&config->i2c, PCF85063A_ALARM_REGISTER, regs, sizeof(regs));
	if (err) {
		LOG_ERR("Error when getting alarm time: %i", err);
		return err;
	}

	/* Initialize data structure and mask */
	memset(timeptr, 0U, sizeof(*timeptr));
	*mask = 0U;

	if (!(regs[0] & BIT(7))) {
		timeptr->tm_sec = bcd2bin(regs[0] & PCF85063A_SECONDS_MASK);
		*mask |= RTC_ALARM_TIME_MASK_SECOND;
	}

	if (!(regs[1] & BIT(7))) {
		timeptr->tm_min = bcd2bin(regs[1] & PCF85063A_MINUTES_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if (!(regs[2] & BIT(7))) {
		timeptr->tm_hour = bcd2bin(regs[2] & PCF85063A_HOURS_MASK);
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if (!(regs[3] & BIT(7))) {
		timeptr->tm_mday = bcd2bin(regs[3] & PCF85063A_DAYS_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	if (!(regs[4] & BIT(7))) {
		timeptr->tm_wday = bcd2bin(regs[4] & PCF85063A_WEEKDAYS_MASK);
		*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
	}

	return 0;
}

static int pcf85063a_alarm_is_pending(const struct device *dev, uint16_t id)
{
	const struct pcf85063a_config *config = dev->config;
	uint8_t reg;
	int err;

	if (id != 0) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	err = i2c_reg_read_byte_dt(&config->i2c, PCF85063A_CONTROL2_REGISTER, &reg);
	if (err) {
		LOG_ERR("Error when getting the control register 2: %i", err);
		return err;
	}

	if (reg & BIT(6)) {
		/* Clear AF by writing back Control_2 with AF bit cleared. */
		uint8_t new = reg & ~BIT(6);

		err = i2c_reg_write_byte_dt(&config->i2c, PCF85063A_CONTROL2_REGISTER, new);
		if (err) {
			LOG_ERR("Error clearing AF flag: %i", err);
			return err;
		}
		return 1;
	}

	return 0;
}

static int pcf85063a_alarm_set_callback(const struct device *dev, uint16_t id,
					rtc_alarm_callback callback, void *user_data)
{
#ifndef PCF85063A_INT1_GPIOS_IN_USE
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
#else  /* PCF85063A_INT1_GPIOS_IN_USE */
	const struct pcf85063a_config *config = dev->config;
	struct pcf85063a_data *data = dev->data;
	int ret;

	if (config->int1.port == NULL) {
		return -ENOTSUP;
	}

	if (id != 0) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	data->alarm_callback = callback;
	data->alarm_user_data = user_data;
	data->dev = dev;

	ret = gpio_pin_configure_dt(&config->int1, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d", ret, config->int1.port->name,
			config->int1.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int1, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d", ret,
			config->int1.port->name, config->int1.pin);
		return ret;
	}

	gpio_init_callback(&data->int1_callback, gpio_callback_function, BIT(config->int1.pin));
	gpio_add_callback(config->int1.port, &data->int1_callback);
	LOG_DBG("Alarm set");
	return 0;
#endif /* PCF85063A_INT1_GPIOS_IN_USE */
}

#endif /* CONFIG_RTC_ALARM */

static DEVICE_API(rtc, pcf85063a_driver_api) = {
	.set_time = pcf85063a_set_time,
	.get_time = pcf85063a_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = pcf85063a_alarm_get_supported_fields,
	.alarm_set_time = pcf85063a_alarm_set_time,
	.alarm_get_time = pcf85063a_alarm_get_time,
	.alarm_is_pending = pcf85063a_alarm_is_pending,
	.alarm_set_callback = pcf85063a_alarm_set_callback,
#endif
};

static int pcf85063a_init(const struct device *dev)
{
	const struct pcf85063a_config *config = dev->config;
	int ret;
#ifdef PCF85063A_INT1_GPIOS_IN_USE
	struct pcf85063a_data *data = dev->data;

	data->callback_work = callback_work;
	if (!gpio_is_ready_dt(&config->int1)) {
		LOG_ERR("Interrupt GPIO device not ready");
		return -ENODEV;
	}
#endif

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C device not ready: %s", config->i2c.bus->name);
		return -ENODEV;
	}

	/* Ensure 24 hours format is set */
	ret = i2c_reg_update_byte_dt(&config->i2c, PCF85063A_CONTROL1_REGISTER,
				     PCF85063A_CONTROL1_REGISTER_12_24, 0);
	if (ret) {
		LOG_ERR("Failed to set hour format: %i", ret);
		return ret;
	}

	return 0;
}

#define PCF85063A_INIT(inst)                                                                       \
	static const struct pcf85063a_config pcf85063a_config_##inst = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                               \
		IF_ENABLED(PCF85063A_INT1_GPIOS_IN_USE,                                            \
		(.int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0}))) };               \
                                                                                                   \
	static struct pcf85063a_data pcf85063a_data_##inst;                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &pcf85063a_init, NULL, &pcf85063a_data_##inst,                 \
			      &pcf85063a_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,     \
			      &pcf85063a_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PCF85063A_INIT)
