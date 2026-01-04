/**
 * Copyright (c) 2025 Gabriele Zampieri <opensource@arsenaling.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>

#include "rtc_utils.h"

#define DT_DRV_COMPAT adi_max31331

LOG_MODULE_REGISTER(max31331, CONFIG_RTC_LOG_LEVEL);

/* MAX31331 Register Map */
#define MAX31331_STATUS      0x00
#define MAX31331_INT_EN      0x01
#define MAX31331_RTC_CONFIG2 0x04
#define MAX31331_SECONDS     0x08
#define MAX31331_ALM1_SEC    0x0F
#define MAX31331_TRICKLE_REG 0x1B
#define MAX31331_OFFSET_HIGH 0x1D
#define MAX31331_OFFSET_LOW  0x1E

/* MAX31331_STATUS Bit Definitions */
#define MAX31331_STATUS_A1F BIT(0)

/* MAX31331_INT_EN Bit Definitions */
#define MAX31331_INT_EN_A1IE BIT(0)

/* MAX31331_RTC_CONFIG2 Bit Definitions */
#define MAX31331_RTC_CONFIG2_ENCLKO  BIT(2)
#define MAX31331_RTC_CONFIG2_CLKO_HZ GENMASK(1, 0)

/* Century bit in MONTH register */
#define MAX31335_MONTH_CENTURY BIT(7)

/* MAX31331_TRICKLE_REG Bit Definitions */
#define MAX31331_TRICKLE_REG_DIODE      BIT(3)
#define MAX31331_TRICKLE_REG_ROHMS      GENMASK(2, 1)
#define MAX31331_TRICKLE_REG_EN_TRICKLE BIT(0)

/* Compensation resolution */
#define MAX31331_COMP_PPB 477
#define MAX31331_MAX_COMP (INT16_MAX * MAX31331_COMP_PPB)
#define MAX31331_MIN_COMP (INT16_MIN * MAX31331_COMP_PPB)

/* Supported alarms on alarm 1 (referenced as 0 by the driver) */
#define MAX31331_ALARM1_MASK                                                                       \
	RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |       \
		RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH |                         \
		RTC_ALARM_TIME_MASK_YEAR

/* Trickle charger diode configuration (DT enum mapping) */
static const uint8_t tc_diode_value[2] = {
	0, /* 0b0xx schottky diode */
	1, /* 0b1xx standard+schottky diode */
};
#define MAX31331_TC_DIODE_INVALID (sizeof(tc_diode_value) + 1)

/* Trickle charger resistor configuration (DT enum mapping) */
static const uint8_t tc_rohms_value[3] = {
	1, /* 0bx01 3 kOhms */
	2, /* 0bx10 6 kOhms */
	3, /* 0bx11 11 kOhms*/
};
#define MAX31331_TC_ROHMS_INVALID (sizeof(tc_rohms_value) + 1)

struct max31331_config {
	const struct i2c_dt_spec i2c;

	struct gpio_dt_spec int_gpio;

	/* CLKO output*/
	int en_clko;

	/* Trickle charger */
	bool tc_enable;
	uint8_t tc_diode_idx;
	uint8_t tc_rohms_idx;

	/* Quartz compensation */
	int32_t comp_ppb;
};

struct max31331_data {
	struct k_work work;
	const struct device *dev;
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	struct gpio_callback int_callback;
};

static int max31331_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct max31331_config *config = dev->config;

	LOG_DBG("Set time: year=%d mon=%d mday=%d wday=%d hour=%d min=%d sec=%d", timeptr->tm_year,
		timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec);

	uint8_t date[7];

	date[0] = bin2bcd(timeptr->tm_sec);
	date[1] = bin2bcd(timeptr->tm_min);
	date[2] = bin2bcd(timeptr->tm_hour);
	date[3] = bin2bcd(timeptr->tm_wday + 1);
	date[4] = bin2bcd(timeptr->tm_mday);
	date[5] = bin2bcd(timeptr->tm_mon + 1);
	date[6] = bin2bcd(timeptr->tm_year % 100);

	if (timeptr->tm_year >= 200) {
		date[5] |= FIELD_PREP(MAX31335_MONTH_CENTURY, 1);
	}

	return i2c_burst_write_dt(&config->i2c, MAX31331_SECONDS, date, sizeof(date));
}

static int max31331_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct max31331_config *config = dev->config;
	uint8_t date[7];
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, MAX31331_SECONDS, date, sizeof(date));

	if (ret) {
		return ret;
	}

	timeptr->tm_sec = bcd2bin(date[0] & 0x7f);
	timeptr->tm_min = bcd2bin(date[1] & 0x7f);
	timeptr->tm_hour = bcd2bin(date[2] & 0x3f);
	timeptr->tm_wday = bcd2bin(date[3] & 0x7) - 1;
	timeptr->tm_mday = bcd2bin(date[4] & 0x3f);
	timeptr->tm_mon = bcd2bin(date[5] & 0x1f) - 1;
	timeptr->tm_year = bcd2bin(date[6]) + 100;

	if (FIELD_GET(MAX31335_MONTH_CENTURY, date[5])) {
		timeptr->tm_year += 100;
	}

	LOG_DBG("Get time: year=%d mon=%d mday=%d wday=%d hour=%d min=%d sec=%d", timeptr->tm_year,
		timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec);

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int max31331_alarm_get_supported_fields(const struct device *dev, uint16_t id,
					       uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0) {
		LOG_ERR("Invalid ID: %d", id);
		return -EINVAL;
	}

	*mask = MAX31331_ALARM1_MASK;

	return 0;
}

static int max31331_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				   const struct rtc_time *timeptr)
{
	const struct max31331_config *config = dev->config;
	uint8_t regs[6];
	int ret;

	if (id != 0) {
		LOG_ERR("Invalid ID: %d", id);
		return -EINVAL;
	}

	if ((mask & ~(MAX31331_ALARM1_MASK)) != 0) {
		LOG_ERR("Invalid alarm mask: 0x%04X", mask);
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("Failed to validate the RTC time");
		return -EINVAL;
	}

	regs[0] = bin2bcd((mask & RTC_ALARM_TIME_MASK_SECOND) ? timeptr->tm_sec : 0);
	regs[1] = bin2bcd((mask & RTC_ALARM_TIME_MASK_MINUTE) ? timeptr->tm_min : 0);
	regs[2] = bin2bcd((mask & RTC_ALARM_TIME_MASK_HOUR) ? timeptr->tm_hour : 0);
	regs[3] = bin2bcd((mask & RTC_ALARM_TIME_MASK_MONTHDAY) ? timeptr->tm_mday : 0);
	regs[4] = bin2bcd((mask & RTC_ALARM_TIME_MASK_MONTH) ? timeptr->tm_mon + 1 : 0);
	regs[5] = bin2bcd((mask & RTC_ALARM_TIME_MASK_YEAR) ? timeptr->tm_year % 100 : 0);

	ret = i2c_burst_write_dt(&config->i2c, MAX31331_ALM1_SEC, regs, sizeof(regs));

	if (ret) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, MAX31331_INT_EN, MAX31331_INT_EN_A1IE,
				     FIELD_PREP(MAX31331_INT_EN_A1IE, !!mask));

	if (ret) {
		return ret;
	}

	return i2c_reg_update_byte_dt(&config->i2c, MAX31331_STATUS, MAX31331_STATUS_A1F, 0);
}

static int max31331_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				   struct rtc_time *timeptr)
{
	const struct max31331_config *config = dev->config;
	int ret;
	uint8_t regs[6];
	struct rtc_time curr_time;

	if (id != 0) {
		LOG_ERR("Invalid ID: %d", id);
		return -EINVAL;
	}

	ret = i2c_burst_read_dt(&config->i2c, MAX31331_ALM1_SEC, regs, sizeof(regs));
	if (ret) {
		return ret;
	}

	timeptr->tm_sec = bcd2bin(regs[0] & 0x7f);
	timeptr->tm_min = bcd2bin(regs[1] & 0x7f);
	timeptr->tm_hour = bcd2bin(regs[2] & 0x3f);
	timeptr->tm_mday = bcd2bin(regs[3] & 0x3f);
	timeptr->tm_mon = bcd2bin(regs[4] & 0x7f) - 1;
	timeptr->tm_year = bcd2bin(regs[5]) + 100;

	ret = max31331_get_time(dev, &curr_time);
	if (ret) {
		return ret;
	}

	if (curr_time.tm_year >= 200) {
		timeptr->tm_year += 100;
	}

	*mask |= (regs[0] ? RTC_ALARM_TIME_MASK_SECOND : 0);
	*mask |= (regs[1] ? RTC_ALARM_TIME_MASK_MINUTE : 0);
	*mask |= (regs[2] ? RTC_ALARM_TIME_MASK_HOUR : 0);
	*mask |= (regs[3] ? RTC_ALARM_TIME_MASK_MONTHDAY : 0);
	*mask |= (regs[4] ? RTC_ALARM_TIME_MASK_MONTH : 0);
	*mask |= (regs[5] ? RTC_ALARM_TIME_MASK_YEAR : 0);

	return 0;
}

static int max31331_alarm_is_pending(const struct device *dev, uint16_t id)
{
	const struct max31331_config *config = dev->config;
	int ret;
	uint8_t ctrl, status;

	if (id != 0) {
		LOG_ERR("Invalid ID: %d", id);
		return -EINVAL;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, MAX31331_INT_EN, &ctrl);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, MAX31331_STATUS, &status);
	if (ret) {
		return ret;
	}

	return (ctrl & MAX31331_INT_EN_A1IE) && (status & MAX31331_STATUS_A1F);
}

static void gpio_int_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct max31331_data *data = CONTAINER_OF(cb, struct max31331_data, int_callback);

	k_work_submit(&data->work);
}

void callback_work_handler(struct k_work *work)
{
	struct max31331_data *data = CONTAINER_OF(work, struct max31331_data, work);
	const struct max31331_config *config = data->dev->config;
	int ret;
	uint8_t status;

	/* Automatically clears the active flags */
	ret = i2c_reg_read_byte_dt(&config->i2c, MAX31331_STATUS, &status);
	if (ret) {
		return;
	}

	/* Fire the callback only for Alarm 1 interrupts */
	if (data->alarm_callback && status & MAX31331_STATUS_A1F) {
		data->alarm_callback(data->dev, 0, data->alarm_user_data);
	} else {
		LOG_WRN("Missing MAX31331 alarm callback");
	}
}

static int max31331_alarm_set_callback(const struct device *dev, uint16_t id,
				       rtc_alarm_callback callback, void *user_data)
{
	const struct max31331_config *config = dev->config;
	struct max31331_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("int-gpios not provided");
		return -ENOTSUP;
	}

	if (id != 0) {
		LOG_ERR("Invalid ID %d", id);
		return -EINVAL;
	}

	data->dev = dev;
	data->alarm_callback = callback;
	data->alarm_user_data = user_data;

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT | GPIO_PULL_UP);
	if (ret < 0) {
		LOG_ERR("Failed to configure int gpio (err %d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Failed to configure edge on int gpio (err %d)", ret);
		return ret;
	}

	gpio_init_callback(&data->int_callback, gpio_int_callback, BIT(config->int_gpio.pin));
	ret = gpio_add_callback_dt(&config->int_gpio, &data->int_callback);
	if (ret < 0) {
		LOG_ERR("Failed to add callback to int gpio (err %d)", ret);
		return ret;
	}

	return 0;
}
#endif

#if CONFIG_RTC_CALIBRATION
static int max31331_set_calibration(const struct device *dev, int32_t calibration)
{
	const struct max31331_config *config = dev->config;
	uint8_t regs[2];
	int16_t comp;

	if (calibration < MAX31331_MIN_COMP || calibration > MAX31331_MAX_COMP) {
		LOG_ERR("Calibration value %d ppb out of range! [min: %d ppb max: %d ppb]",
			calibration, MAX31331_MIN_COMP, MAX31331_MAX_COMP);
		return -EINVAL;
	}

	comp = DIV_ROUND_CLOSEST(calibration, MAX31331_COMP_PPB);

	LOG_DBG("Actual compensation value: %d ppb", comp * MAX31331_COMP_PPB);

	sys_put_be16(comp, regs);

	return i2c_burst_write_dt(&config->i2c, MAX31331_OFFSET_HIGH, regs, sizeof(regs));
}

static int max31331_get_calibration(const struct device *dev, int32_t *calibration)
{
	const struct max31331_config *config = dev->config;
	uint8_t regs[2];
	int16_t comp;
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, MAX31331_OFFSET_HIGH, regs, sizeof(regs));
	if (ret) {
		return ret;
	}

	comp = sys_get_be16(regs);

	*calibration = comp * MAX31331_COMP_PPB;

	return 0;
}
#endif

static DEVICE_API(rtc, max31331_driver_api) = {
	.set_time = max31331_set_time,
	.get_time = max31331_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = max31331_alarm_get_supported_fields,
	.alarm_set_time = max31331_alarm_set_time,
	.alarm_get_time = max31331_alarm_get_time,
	.alarm_is_pending = max31331_alarm_is_pending,
	.alarm_set_callback = max31331_alarm_set_callback,
#endif
#if CONFIG_RTC_CALIBRATION
	.set_calibration = max31331_set_calibration,
	.get_calibration = max31331_get_calibration,
#endif
};

static int max31331_setup_tc(const struct device *dev)
{
	const struct max31331_config *config = dev->config;
	uint8_t tc_reg;

	if (config->tc_enable && config->tc_rohms_idx == MAX31331_TC_ROHMS_INVALID) {
		LOG_ERR("Trickle charger resistor value not defined");
		return -EINVAL;
	}

	if (config->tc_enable && config->tc_diode_idx == MAX31331_TC_DIODE_INVALID) {
		LOG_ERR("Trickle charger diode typen not defined");
		return -EINVAL;
	}

	tc_reg = FIELD_PREP(MAX31331_TRICKLE_REG_DIODE, tc_diode_value[config->tc_diode_idx]) |
		 FIELD_PREP(MAX31331_TRICKLE_REG_ROHMS, tc_rohms_value[config->tc_rohms_idx]) |
		 FIELD_PREP(MAX31331_TRICKLE_REG_EN_TRICKLE, config->tc_enable);

	LOG_DBG("Trickle charger register value: 0x%02X", tc_reg);

	return i2c_reg_write_byte_dt(&config->i2c, MAX31331_TRICKLE_REG, tc_reg);
}

static int max31331_setup_clko(const struct device *dev)
{
	const struct max31331_config *config = dev->config;

	return i2c_reg_update_byte_dt(
		&config->i2c, MAX31331_RTC_CONFIG2,
		MAX31331_RTC_CONFIG2_ENCLKO | MAX31331_RTC_CONFIG2_CLKO_HZ,
		FIELD_PREP(MAX31331_RTC_CONFIG2_ENCLKO, 1) |
			FIELD_PREP(MAX31331_RTC_CONFIG2_CLKO_HZ, config->en_clko));
}

int max31331_init(const struct device *dev)
{
	const struct max31331_config *config = dev->config;
	__maybe_unused struct max31331_data *data = dev->data;
	int ret;
	uint8_t dummy_check;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C device not ready: %s", config->i2c.bus->name);
		return -ENODEV;
	}

	/*
	 * Determine if the I2C device is alive by attempting to read from status register.
	 * Unfortunately the device does not have an ID register, so we cannot know if the device is
	 * actually a MAX31331.
	 */
	ret = i2c_reg_read_byte_dt(&config->i2c, MAX31331_STATUS, &dummy_check);
	if (ret < 0) {
		LOG_ERR("Failed to communicate with MAX31331 (err %d)", ret);
		return -ENODEV;
	}

#ifdef CONFIG_RTC_ALARM
	/* Interrupt work handler */
	k_work_init(&data->work, callback_work_handler);
#endif

#if CONFIG_RTC_CALIBRATION
	ret = max31331_set_calibration(dev, config->comp_ppb);
	if (ret != 0) {
		LOG_ERR("Failed to apply frequency compensation (err %d)", ret);
		return -ENODEV;
	}
#endif

	ret = max31331_setup_tc(dev);
	if (ret) {
		LOG_ERR("Failed to setup trickle charger (err %d)", ret);
		return -ENODEV;
	}

	ret = max31331_setup_clko(dev);
	if (ret) {
		LOG_ERR("Failed to setup CLKO output (err %d)", ret);
		return -ENODEV;
	}

	LOG_DBG("%s initialized", dev->name);

	return 0;
}

#define MAX31331_INIT(inst)                                                                        \
	static const struct max31331_config max31331_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
		.en_clko = DT_INST_ENUM_IDX(inst, clko_frequency),                                 \
		.tc_enable = DT_INST_PROP(inst, enable_trickle_charger),                           \
		.tc_rohms_idx = DT_INST_ENUM_IDX_OR(inst, trickle_resistor_ohms,                   \
						    MAX31331_TC_ROHMS_INVALID),                    \
		.tc_diode_idx = DT_INST_ENUM_IDX_OR(inst, tc_diode, MAX31331_TC_DIODE_INVALID),    \
		.comp_ppb = DT_INST_PROP(inst, comp_ppb),                                          \
	};                                                                                         \
                                                                                                   \
	static struct max31331_data max31331_data_##inst;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &max31331_init, NULL, &max31331_data_##inst,                   \
			      &max31331_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,      \
			      &max31331_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31331_INIT)
