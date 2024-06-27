/*
 * Copyright (c) 2023 Alvaro Garcia Gomez <maxpowel@gmail.com>
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
LOG_MODULE_REGISTER(pcf8563);

#define DT_DRV_COMPAT nxp_pcf8563

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int1_gpios) &&                                                \
	(defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE))
/* The user may need only alarms but not interrupts so we will only
 * include all the interrupt code if the user configured it in the dts
 */
#define PCF8563_INT1_GPIOS_IN_USE 1
#endif

/* The device registers */
#define PCF8563_TIME_DATE_REGISTER	0x02
#define PCF8563_ALARM_REGISTER		0x09
#define PCF8563_CONTROL1_REGISTER	0x00
#define PCF8563_CONTROL2_REGISTER	0x01
#define PCF8563_CONTROL2_REGISTER_TIE_EN (1 << 0)
#define PCF8563_CONTROL2_REGISTER_AIE_EN (1 << 1)

/* These masks were retrieved from the datasheet
 * https://www.nxp.com/docs/en/data-sheet/PCF8563.pdf
 * page 6, section 8.2 Register organization.
 * Basically, I clean the unused bits and the bits used
 * for other stuff
 */
#define PCF8563_SECONDS_MASK  GENMASK(6, 0)
#define PCF8563_MINUTES_MASK  GENMASK(6, 0)
#define PCF8563_HOURS_MASK    GENMASK(5, 0)
#define PCF8563_DAYS_MASK     GENMASK(5, 0)
#define PCF8563_WEEKDAYS_MASK GENMASK(2, 0)
#define PCF8563_MONTHS_MASK   GENMASK(4, 0)

/* RTC alarm time fields supported by the PCF8563, page 7 of the datasheet */
#define PCF8563_RTC_ALARM_TIME_MASK                                                                \
	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY |    \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

#define PCF8563_RTC_TIME_MASK                                                                      \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

struct pcf8563_config {
	const struct i2c_dt_spec i2c;
	#ifdef PCF8563_INT1_GPIOS_IN_USE
	const struct gpio_dt_spec int1;
	#endif
};


#ifdef PCF8563_INT1_GPIOS_IN_USE
/* This work will run the user callback function */
void callback_work_handler(struct k_work *work);
K_WORK_DEFINE(callback_work, callback_work_handler);
#endif

struct pcf8563_data {
#ifdef PCF8563_INT1_GPIOS_IN_USE
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	const struct device *dev;
	struct gpio_callback int1_callback;
	struct k_work callback_work;
#endif
};

/**
 * The format described below is described in the datasheet
 * https://www.nxp.com/docs/en/data-sheet/PCF8563.pdf page 10 starting
 * with 8.4.2 Register Minutes.
 *
 * For seconds, first bit is ignored (it is used to check the clock integrity).
 * The upper digit takes the next 3 bits for the tens place and then the rest
 * bits for the unit
 * So for example, value 43 is 40 * 10 + 3, so the tens digit is 4 and unit digit is 3.
 * Then we put the number 3 in the last 4 bits and the number 4 in next 3 bits
 * It uses BCD notation so the number 3 is 0011 and the number for is 100 so the final
 * byte is 0 (ignored bit) 100 (the 4) 0011 (the 3) -> 0100001
 * Luckily, zephyr provides a couple of functions to do exactlly this: bin2bcd and bcd2bin,
 * but we will take care about the bits marked as non used in
 * the datasheet because they may contain unexpected values. Applying a mask will help us
 * to sanitize the read values
 */
int pcf8563_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct pcf8563_config *config = dev->config;
	int ret;
	uint8_t raw_time[7];

	if (!rtc_utils_validate_rtc_time(timeptr, PCF8563_RTC_TIME_MASK)) {
		LOG_ERR("invalid time");
		return -EINVAL;
	}

	/* Set seconds */
	raw_time[0] = bin2bcd(timeptr->tm_sec);

	/* Set minutes */
	raw_time[1] = bin2bcd(timeptr->tm_min);

	/* Set hours */
	raw_time[2] = bin2bcd(timeptr->tm_hour);

	/* Set days */
	raw_time[3] = bin2bcd(timeptr->tm_mday);

	/* Set weekdays */
	raw_time[4] = timeptr->tm_wday;

	/*Set month */
	raw_time[5] = bin2bcd(timeptr->tm_mon);

	/* Set year */
	raw_time[6] = bin2bcd(timeptr->tm_year);

	/* Write to device */
	ret = i2c_burst_write_dt(&config->i2c, PCF8563_TIME_DATE_REGISTER,
				 raw_time, sizeof(raw_time));
	if (ret) {
		LOG_ERR("Error when setting time: %i", ret);
		return ret;
	}

	return 0;
}

int pcf8563_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct pcf8563_config *config = dev->config;
	int ret;
	uint8_t raw_time[7];

	ret = i2c_burst_read_dt(&config->i2c, PCF8563_TIME_DATE_REGISTER,
				raw_time, sizeof(raw_time));
	if (ret) {
		LOG_ERR("Unable to get time. Err: %i", ret);
		return ret;
	}

	/* Check integrity, if the first bit is 1 it is ok */
	if (raw_time[0] & BIT(7)) {
		LOG_WRN("Clock integrity failed");
		return -ENODATA;
	}

	/* Nanoseconds */
	timeptr->tm_nsec = 0;

	/* Get seconds */
	timeptr->tm_sec = bcd2bin(raw_time[0] & PCF8563_SECONDS_MASK);

	/* Get minutes */
	timeptr->tm_min = bcd2bin(raw_time[1] & PCF8563_MINUTES_MASK);

	/* Get hours */
	timeptr->tm_hour = bcd2bin(raw_time[2] & PCF8563_HOURS_MASK);

	/* Get days */
	timeptr->tm_mday = bcd2bin(raw_time[3] & PCF8563_DAYS_MASK);

	/* Get weekdays */
	timeptr->tm_wday = raw_time[4] & PCF8563_WEEKDAYS_MASK;

	/* Get month */
	timeptr->tm_mon = bcd2bin(raw_time[5] & PCF8563_MONTHS_MASK);

	/* Get year */
	timeptr->tm_year = bcd2bin(raw_time[6]);

	/* Day number not used */
	timeptr->tm_yday = -1;

	/* DST not used  */
	timeptr->tm_isdst = -1;

	return 0;
}



#ifdef CONFIG_RTC_ALARM

static int pcf8563_alarm_get_supported_fields(const struct device *dev, uint16_t id,
					      uint16_t *mask)
{
	ARG_UNUSED(dev);

	/* This device only has one channel*/
	if (id != 0) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	*mask = PCF8563_RTC_ALARM_TIME_MASK;

	return 0;
}

static int pcf8563_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				  const struct rtc_time *timeptr)
{
	const struct pcf8563_config *config = dev->config;
	uint8_t regs[4];
	int ret;

	if (id != 0) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	if ((mask & ~(PCF8563_RTC_ALARM_TIME_MASK)) != 0) {
		LOG_ERR("invalid alarm field mask 0x%04x", mask);
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("invalid alarm time");
		return -EINVAL;
	}

	/*
	 * The first bit is used as enabled/disabled flag.
	 * The mask will clean it and also the unused bits
	 */
	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) != 0) {
		regs[0] = bin2bcd(timeptr->tm_min) & PCF8563_MINUTES_MASK;
	} else {
		/* First bit to 1 is alarm disabled */
		regs[0] = BIT(7);
	}

	if ((mask & RTC_ALARM_TIME_MASK_HOUR) != 0) {
		regs[1] = bin2bcd(timeptr->tm_hour) & PCF8563_HOURS_MASK;
	} else {
		regs[1] = BIT(7);
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) != 0) {
		regs[2] = bin2bcd(timeptr->tm_mday) & PCF8563_DAYS_MASK;
	} else {
		regs[2] = BIT(7);
	}

	if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) != 0) {
		regs[3] = bin2bcd(timeptr->tm_wday) & PCF8563_WEEKDAYS_MASK;
	} else {
		regs[3] = BIT(7);
	}

	ret = i2c_burst_write_dt(&config->i2c, PCF8563_ALARM_REGISTER, regs, sizeof(regs));
	if (ret) {
		LOG_ERR("Error when setting alarm: %i", ret);
		return ret;
	}

	/* Dont forget to enable interrupts */
	i2c_reg_write_byte_dt(
		&config->i2c,
		PCF8563_CONTROL2_REGISTER,
		PCF8563_CONTROL2_REGISTER_TIE_EN | PCF8563_CONTROL2_REGISTER_AIE_EN
	);

	return 0;
}

static int pcf8563_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				  struct rtc_time *timeptr)
{
	const struct pcf8563_config *config = dev->config;
	uint8_t regs[4];
	int err;

	if (id != 0) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	err = i2c_burst_read_dt(&config->i2c, PCF8563_ALARM_REGISTER, regs, sizeof(regs));
	if (err) {
		LOG_ERR("Error when getting alarm time: %i", err);
		return err;
	}

	/* Initialize data structure and mask */
	memset(timeptr, 0U, sizeof(*timeptr));
	*mask = 0U;

	/* The first bit is the enabled flag */
	if (regs[0] & BIT(7)) {
		timeptr->tm_min = bcd2bin(regs[0] & GENMASK(6, 0));
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if (regs[1] & BIT(7)) {
		timeptr->tm_hour = bcd2bin(regs[1] & GENMASK(5, 0));
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if (regs[2] & BIT(7)) {
		timeptr->tm_mday = bcd2bin(regs[2] & GENMASK(5, 0));
		*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	if (regs[3] & BIT(7)) {
		timeptr->tm_wday = bcd2bin(regs[3] & GENMASK(2, 0));
		*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
	}

	return 0;
}

static int pcf8563_alarm_is_pending(const struct device *dev, uint16_t id)
{
	/* The description of this register is at page 7, section 8.3.2 Register Control_status_2
	 * There are several kinds of alarms, but here we only need to know that anything but 0
	 * means that there was some kind of alarm active
	 */
	const struct pcf8563_config *config = dev->config;
	uint8_t reg;
	int err;

	if (id != 0) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	err = i2c_reg_read_byte_dt(&config->i2c, PCF8563_CONTROL2_REGISTER, &reg);
	if (err) {
		LOG_ERR("Error when getting the control register 2: %i", err);
		return err;
	}

	/* Only the last bits use useful here */
	if (reg & GENMASK(3, 2)) {
		/* Clean the alarm */
		err = i2c_reg_write_byte_dt(&config->i2c, PCF8563_CONTROL2_REGISTER, GENMASK(1, 0));
		if (err) {
			LOG_ERR("Error when clearing alarms: %d", err);
			return err;
		}
		/* There was an alarm */
		return 1;
	}
	/* No alarms */
	return 0;
}
#endif

#ifdef PCF8563_INT1_GPIOS_IN_USE
/* The logic related to the pin interrupt logic */

void callback_work_handler(struct k_work *work)
{
	/* This function is run as a work so the user can spend here all the necessary time */
	struct pcf8563_data *data = CONTAINER_OF(work, struct pcf8563_data, callback_work);

	if (data->alarm_callback == NULL) {
		LOG_WRN("No PCF8563 alarm callback function provided");
	} else {
		data->alarm_callback(data->dev, 0, data->alarm_user_data);
	}
}


/* The function called when the clock alarm activates the interrupt*/
void gpio_callback_function(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	struct pcf8563_data *data = CONTAINER_OF(cb, struct pcf8563_data, int1_callback);

	LOG_DBG("PCF8563 interrupt detected");
	/* By using a work we are able to run "heavier" code */
	k_work_submit(&(data->callback_work));

}

#endif

static int pcf8563_alarm_set_callback(const struct device *dev, uint16_t id,
				      rtc_alarm_callback callback, void *user_data)
{
#ifndef PCF8563_INT1_GPIOS_IN_USE
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
#else
	const struct pcf8563_config *config = dev->config;
	struct pcf8563_data *data = dev->data;
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

	/* The PCF8563 int pin requires a pull up to work */
	ret = gpio_pin_configure_dt(&config->int1, GPIO_INPUT | GPIO_PULL_UP);
	if (ret < 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d",
		       ret, config->int1.port->name, config->int1.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int1, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
			ret, config->int1.port->name, config->int1.pin);
		return ret;
	}


	gpio_init_callback(&data->int1_callback, gpio_callback_function, BIT(config->int1.pin));
	gpio_add_callback(config->int1.port, &data->int1_callback);
	LOG_DBG("Alarm set");
	return 0;
#endif
}

static const struct rtc_driver_api pcf8563_driver_api = {
	.set_time = pcf8563_set_time,
	.get_time = pcf8563_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = pcf8563_alarm_get_supported_fields,
	.alarm_set_time = pcf8563_alarm_set_time,
	.alarm_get_time = pcf8563_alarm_get_time,
	.alarm_is_pending = pcf8563_alarm_is_pending,
	.alarm_set_callback = pcf8563_alarm_set_callback,
#endif
};


int pcf8563_init(const struct device *dev)
{
	const struct pcf8563_config *config = dev->config;
	int ret;
	uint8_t reg;
	#ifdef PCF8563_INT1_GPIOS_IN_USE
	struct pcf8563_data *data = dev->data;

	data->callback_work = callback_work;
	#endif

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Failed to get pointer to %s device!", config->i2c.bus->name);
		return -ENODEV;
	}

	/* Check if it's alive. */
	ret = i2c_reg_read_byte_dt(&config->i2c, PCF8563_CONTROL1_REGISTER, &reg);
	if (ret) {
		LOG_ERR("Failed to read from PCF85063! (err %i)", ret);
		return -ENODEV;
	}

	LOG_INF("%s is initialized!", dev->name);

	return 0;
}

#define PCF8563_INIT(inst)                                                                         \
	static const struct pcf8563_config pcf8563_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		IF_ENABLED(PCF8563_INT1_GPIOS_IN_USE,                                              \
		(.int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0})))                         \
		};                                                                                 \
                                                                                                   \
	static struct pcf8563_data pcf8563_data_##inst;                                            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &pcf8563_init, NULL,                                           \
			      &pcf8563_data_##inst, &pcf8563_config_##inst, POST_KERNEL,           \
			      CONFIG_RTC_INIT_PRIORITY, &pcf8563_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PCF8563_INIT)
