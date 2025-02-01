/*
 * Copyright (c) 2025 Marcin Lyda <elektromarcin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/gpio.h>
#include "rtc_utils.h"

LOG_MODULE_REGISTER(ds1337, CONFIG_RTC_LOG_LEVEL);

#define DT_DRV_COMPAT maxim_ds1337

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "Maxim DS1337 RTC driver enabled without any devices"
#endif

/* Registers */
#define DS1337_SECONDS_REG          0x00
#define DS1337_ALARM_1_SECONDS_REG  0x07
#define DS1337_ALARM_2_MINUTES_REG  0x0B
#define DS1337_CONTROL_REG          0x0E
#define DS1337_STATUS_REG           0x0F

/* Bitmasks */
#define DS1337_SECONDS_MASK       GENMASK(6, 0)
#define DS1337_MINUTES_MASK       GENMASK(6, 0)
#define DS1337_HOURS_MASK         GENMASK(5, 0)
#define DS1337_DAY_MASK           GENMASK(2, 0)
#define DS1337_DATE_MASK          GENMASK(5, 0)
#define DS1337_MONTH_MASK         GENMASK(4, 0)
#define DS1337_YEAR_MASK          GENMASK(7, 0)
#define DS1337_ALARM_SECONDS_MASK GENMASK(6, 0)
#define DS1337_ALARM_MINUTES_MASK GENMASK(6, 0)
#define DS1337_ALARM_HOURS_MASK   GENMASK(5, 0)
#define DS1337_ALARM_DAY_MASK     GENMASK(3, 0)
#define DS1337_ALARM_DATE_MASK    GENMASK(5, 0)

#define DS1337_12_24_MODE_MASK BIT(6)
#define DS1337_CENTURY_MASK    BIT(7)
#define DS1337_DY_DT_MASK      BIT(6)

#define DS1337_ALARM_DISABLE_MASK BIT(7)

#define DS1337_EOSC_MASK  BIT(7)
#define DS1337_RS_MASK    GENMASK(4, 3)
#define DS1337_INTCN_MASK BIT(2)
#define DS1337_A2IE_MASK  BIT(1)
#define DS1337_A1IE_MASK  BIT(0)

#define DS1337_OSF_MASK BIT(7)
#define DS1337_A2F_MASK BIT(1)
#define DS1337_A1F_MASK BIT(0)

#define DS1337_SQW_FREQ_1Hz     FIELD_PREP(DS1337_RS_MASK, 0x00)
#define DS1337_SQW_FREQ_4096Hz  FIELD_PREP(DS1337_RS_MASK, 0x01)
#define DS1337_SQW_FREQ_8192Hz  FIELD_PREP(DS1337_RS_MASK, 0x02)
#define DS1337_SQW_FREQ_32768Hz FIELD_PREP(DS1337_RS_MASK, 0x03)

/* DS1337 features two independent alarms */
#define DS1337_ALARM_1_ID   0
#define DS1337_ALARM_2_ID   1
#define DS1337_ALARMS_COUNT 2

/* SQW frequency property enum values */
#define DS1337_SQW_PROP_ENUM_1HZ     0
#define DS1337_SQW_PROP_ENUM_4096HZ  1
#define DS1337_SQW_PROP_ENUM_8192HZ  2
#define DS1337_SQW_PROP_ENUM_32768HZ 3

/* DS1337 counts weekdays from 1 to 7 */
#define DS1337_DAY_OFFSET -1

/* DS1337 counts months 1 to 12 */
#define DS1337_MONTH_OFFSET -1

/* Year 2000 value represented as tm_year value */
#define DS1337_TM_YEAR_2000 (2000 - 1900)

/* RTC time fields supported by DS1337 */
#define DS1337_RTC_TIME_MASK                                                                       \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

/* RTC alarm 1 fields supported by DS1337 */
#define DS1337_RTC_ALARM_TIME_1_MASK                                                               \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_WEEKDAY)

/* RTC alarm 2 fields supported by DS1337 */
#define DS1337_RTC_ALARM_TIME_2_MASK                                                               \
	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY |    \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

/* Helper macro to guard GPIO interrupt related stuff */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) && defined(CONFIG_RTC_ALARM)
#define DS1337_INT_GPIOS_IN_USE 1
#endif

struct ds1337_config {
	const struct i2c_dt_spec i2c;
#ifdef DS1337_INT_GPIOS_IN_USE
	struct gpio_dt_spec gpio_int;
#endif
	uint8_t sqw_freq;
};

struct ds1337_data {
	struct k_sem lock;
#ifdef DS1337_INT_GPIOS_IN_USE
	const struct device *dev;
	struct gpio_callback irq_callback;
	struct k_work work;
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callbacks[DS1337_ALARMS_COUNT];
	void *alarm_user_data[DS1337_ALARMS_COUNT];
#endif
#endif
};

static void ds1337_lock_sem(const struct device *dev)
{
	struct ds1337_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
}

static void ds1337_unlock_sem(const struct device *dev)
{
	struct ds1337_data *data = dev->data;

	k_sem_give(&data->lock);
}

static bool ds1337_validate_alarm_mask(uint16_t alarm_mask, uint16_t alarm_id)
{
	const uint16_t allowed_configs[6] = {
		0,
		RTC_ALARM_TIME_MASK_SECOND,
		RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE,
		RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR,
		RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |
			RTC_ALARM_TIME_MASK_WEEKDAY,
		RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |
			RTC_ALARM_TIME_MASK_MONTHDAY
	};

	const uint16_t available_fields =
		(alarm_id == 0) ? DS1337_RTC_ALARM_TIME_1_MASK : DS1337_RTC_ALARM_TIME_2_MASK;
	if (alarm_mask & ~available_fields) {
		return false;
	}

	ARRAY_FOR_EACH_PTR(allowed_configs, config) {
		if (alarm_mask == (*config & available_fields)) {
			return true;
		}
	}
	return false;
}

#ifdef DS1337_INT_GPIOS_IN_USE

static void ds1337_work_callback(struct k_work *work)
{
	struct ds1337_data *data = CONTAINER_OF(work, struct ds1337_data, work);
	const struct device *dev = data->dev;
	const struct ds1337_config *config = dev->config;
	rtc_alarm_callback alarm_callbacks[DS1337_ALARMS_COUNT] = {NULL};
	void *alarm_user_data[DS1337_ALARMS_COUNT] = {NULL};
	uint8_t status_reg;
	int err;

	ds1337_lock_sem(dev);

	/* Read status register */
	err = i2c_reg_read_byte_dt(&config->i2c, DS1337_STATUS_REG, &status_reg);
	if (err) {
		goto out_unlock;
	}

#ifdef CONFIG_RTC_ALARM
	/* Handle alarm 1 event */
	if ((status_reg & DS1337_A1F_MASK) && (data->alarm_callbacks[DS1337_ALARM_1_ID] != NULL)) {
		status_reg &= ~DS1337_A1F_MASK;
		alarm_callbacks[DS1337_ALARM_1_ID] = data->alarm_callbacks[DS1337_ALARM_1_ID];
		alarm_user_data[DS1337_ALARM_1_ID] = data->alarm_user_data[DS1337_ALARM_1_ID];
	}

	/* Handle alarm 2 event */
	if ((status_reg & DS1337_A2F_MASK) && (data->alarm_callbacks[DS1337_ALARM_2_ID] != NULL)) {
		status_reg &= ~DS1337_A2F_MASK;
		alarm_callbacks[DS1337_ALARM_2_ID] = data->alarm_callbacks[DS1337_ALARM_2_ID];
		alarm_user_data[DS1337_ALARM_2_ID] = data->alarm_user_data[DS1337_ALARM_2_ID];
	}
#endif

	/* Clear alarm flag(s) */
	err = i2c_reg_write_byte_dt(&config->i2c, DS1337_STATUS_REG, status_reg);
	if (err) {
		goto out_unlock;
	}

	/* Check if any interrupt occurred between flags register read/write */
	err = i2c_reg_read_byte_dt(&config->i2c, DS1337_STATUS_REG, &status_reg);
	if (err) {
		goto out_unlock;
	}

	if (((status_reg & DS1337_A1F_MASK) && (alarm_callbacks[0] != NULL)) ||
	    ((status_reg & DS1337_A2F_MASK) && (alarm_callbacks[1] != NULL))) {
		/* Another interrupt occurred while servicing this one */
		(void)k_work_submit(&data->work);
	}

out_unlock:
	ds1337_unlock_sem(dev);

	/* Execute alarm callback(s) */
	for (uint8_t alarm_id = DS1337_ALARM_1_ID; alarm_id < DS1337_ALARMS_COUNT; ++alarm_id) {
		if (alarm_callbacks[alarm_id] != NULL) {
			alarm_callbacks[alarm_id](dev, alarm_id, alarm_user_data[alarm_id]);
			alarm_callbacks[alarm_id] = NULL;
		}
	}
}

static void ds1337_irq_handler(const struct device *port, struct gpio_callback *callback,
			       gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct ds1337_data *data = CONTAINER_OF(callback, struct ds1337_data, irq_callback);

	(void)k_work_submit(&data->work);
}

#endif

static int ds1337_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct ds1337_config *config = dev->config;
	uint8_t regs[7];
	int err;

	if ((timeptr == NULL) || !rtc_utils_validate_rtc_time(timeptr, DS1337_RTC_TIME_MASK)) {
		return -EINVAL;
	}

	ds1337_lock_sem(dev);

	regs[0] = bin2bcd(timeptr->tm_sec) & DS1337_SECONDS_MASK;
	regs[1] = bin2bcd(timeptr->tm_min) & DS1337_MINUTES_MASK;
	regs[2] = bin2bcd(timeptr->tm_hour) & DS1337_HOURS_MASK;
	regs[3] = bin2bcd(timeptr->tm_wday - DS1337_DAY_OFFSET) & DS1337_DAY_MASK;
	regs[4] = bin2bcd(timeptr->tm_mday) & DS1337_DATE_MASK;
	regs[5] = bin2bcd(timeptr->tm_mon - DS1337_MONTH_OFFSET) & DS1337_MONTH_MASK;

	/* Determine which century we're in */
	if (timeptr->tm_year >= DS1337_TM_YEAR_2000) {
		regs[5] |= DS1337_CENTURY_MASK;
		regs[6] = bin2bcd(timeptr->tm_year - DS1337_TM_YEAR_2000) & DS1337_YEAR_MASK;
	} else {
		regs[6] = bin2bcd(timeptr->tm_year) & DS1337_YEAR_MASK;
	}

	/* Set new time */
	err = i2c_burst_write_dt(&config->i2c, DS1337_SECONDS_REG, regs, sizeof(regs));
	if (err) {
		goto out_unlock;
	}

	/* Clear Oscillator Stop Flag, indicating data validity */
	err = i2c_reg_update_byte_dt(&config->i2c, DS1337_STATUS_REG, DS1337_OSF_MASK, 0);

out_unlock:
	ds1337_unlock_sem(dev);

	if (!err) {
		LOG_DBG("Set time: year: %d, month: %d, month day: %d, week day: %d, hour: %d, "
			"minute: %d, second: %d",
			timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
			timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
	}

	return err;
}

static int ds1337_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct ds1337_config *config = dev->config;
	uint8_t regs[7];
	uint8_t status_reg;
	int err;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	ds1337_lock_sem(dev);

	/* Check data validity */
	err = i2c_reg_read_byte_dt(&config->i2c, DS1337_STATUS_REG, &status_reg);
	if (err) {
		goto out_unlock;
	}
	if (status_reg & DS1337_OSF_MASK) {
		err = -ENODATA;
		goto out_unlock;
	}

	/* Read time data */
	err = i2c_burst_read_dt(&config->i2c, DS1337_SECONDS_REG, regs, sizeof(regs));
	if (err) {
		goto out_unlock;
	}

	timeptr->tm_sec = bcd2bin(regs[0] & DS1337_SECONDS_MASK);
	timeptr->tm_min = bcd2bin(regs[1] & DS1337_MINUTES_MASK);
	timeptr->tm_hour = bcd2bin(regs[2] & DS1337_HOURS_MASK);
	timeptr->tm_wday = bcd2bin(regs[3] & DS1337_DAY_MASK) + DS1337_DAY_OFFSET;
	timeptr->tm_mday = bcd2bin(regs[4] & DS1337_DATE_MASK);
	timeptr->tm_mon = bcd2bin(regs[5] & DS1337_MONTH_MASK) + DS1337_MONTH_OFFSET;
	timeptr->tm_year = bcd2bin(regs[6] & DS1337_YEAR_MASK);
	timeptr->tm_yday = -1;  /* Unsupported */
	timeptr->tm_isdst = -1; /* Unsupported */
	timeptr->tm_nsec = 0;   /* Unsupported */

	/* Apply century offset */
	if (regs[5] & DS1337_CENTURY_MASK) {
		timeptr->tm_year += DS1337_TM_YEAR_2000;
	}

out_unlock:
	ds1337_unlock_sem(dev);

	if (!err) {
		LOG_DBG("Read time: year: %d, month: %d, month day: %d, week day: %d, hour: %d, "
			"minute: %d, second: %d",
			timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
			timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
	}

	return err;
}

#ifdef CONFIG_RTC_ALARM

static int ds1337_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id == DS1337_ALARM_1_ID) {
		*mask = DS1337_RTC_ALARM_TIME_1_MASK;
		return 0;
	} else if (id == DS1337_ALARM_2_ID) {
		*mask = DS1337_RTC_ALARM_TIME_2_MASK;
		return 0;
	}

	LOG_ERR("Invalid alarm ID: %d", id);
	return -EINVAL;
}

static int ds1337_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				 const struct rtc_time *timeptr)
{
	const struct ds1337_config *config = dev->config;
	uint8_t regs[4];
	uint8_t reg_addr;
	uint8_t reg_offset;
	int err;

	if (id >= DS1337_ALARMS_COUNT) {
		LOG_ERR("Invalid alarm ID: %d", id);
		return -EINVAL;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) && (mask & RTC_ALARM_TIME_MASK_WEEKDAY)) {
		LOG_ERR("Month day and week day alarms cannot be set simultaneously");
		return -EINVAL;
	}

	if (!ds1337_validate_alarm_mask(mask, id)) {
		LOG_ERR("Unsupported mask 0x%04X for alarm %d", mask, id);
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("Invalid alarm time");
		return -EINVAL;
	}

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		regs[0] = bin2bcd(timeptr->tm_sec) & DS1337_ALARM_SECONDS_MASK;
	} else {
		regs[0] = DS1337_ALARM_DISABLE_MASK;
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		regs[1] = bin2bcd(timeptr->tm_min) & DS1337_ALARM_MINUTES_MASK;
	} else {
		regs[1] = DS1337_ALARM_DISABLE_MASK;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		regs[2] = bin2bcd(timeptr->tm_hour) & DS1337_ALARM_HOURS_MASK;
	} else {
		regs[2] = DS1337_ALARM_DISABLE_MASK;
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		regs[3] = bin2bcd(timeptr->tm_mday) & DS1337_ALARM_DATE_MASK;
	} else if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		regs[3] = bin2bcd(timeptr->tm_wday - DS1337_DAY_OFFSET) & DS1337_ALARM_DAY_MASK;
		regs[3] |= DS1337_DY_DT_MASK;
	} else {
		regs[3] = DS1337_ALARM_DISABLE_MASK;
	}

	/* Update alarm registers */
	if (id == DS1337_ALARM_1_ID) {
		reg_addr = DS1337_ALARM_1_SECONDS_REG;
		reg_offset = 0;
	} else {
		reg_addr = DS1337_ALARM_2_MINUTES_REG;
		reg_offset = 1;
	}

	err = i2c_burst_write_dt(&config->i2c, reg_addr, &regs[reg_offset],
				 sizeof(regs) - reg_offset);
	if (err) {
		return err;
	}

	LOG_DBG("Set alarm: month day: %d, week day: %d, hour: %d, minute: %d, second: %d mask: "
		"0x%04X",
		timeptr->tm_mday, timeptr->tm_wday, timeptr->tm_hour, timeptr->tm_min,
		timeptr->tm_sec, mask);

	return 0;
}

static int ds1337_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				 struct rtc_time *timeptr)
{
	const struct ds1337_config *config = dev->config;
	uint8_t regs[4];
	uint8_t reg_addr;
	uint8_t reg_offset;
	int err;

	if (id >= DS1337_ALARMS_COUNT) {
		LOG_ERR("Invalid alarm ID: %d", id);
		return -EINVAL;
	}

	/* Read alarm registers */
	if (id == DS1337_ALARM_1_ID) {
		reg_addr = DS1337_ALARM_1_SECONDS_REG;
		reg_offset = 0;
	} else {
		reg_addr = DS1337_ALARM_2_MINUTES_REG;
		reg_offset = 1;
	}
	err = i2c_burst_read_dt(&config->i2c, reg_addr, &regs[reg_offset],
				sizeof(regs) - reg_offset);
	if (err) {
		return err;
	}

	(void)memset(timeptr, 0, sizeof(*timeptr));
	*mask = 0;

	if ((regs[0] & DS1337_ALARM_DISABLE_MASK) == 0) {
		timeptr->tm_sec = bcd2bin(regs[0] & DS1337_ALARM_SECONDS_MASK);
		*mask |= RTC_ALARM_TIME_MASK_SECOND;
	}

	if ((regs[1] & DS1337_ALARM_DISABLE_MASK) == 0) {
		timeptr->tm_min = bcd2bin(regs[1] & DS1337_ALARM_MINUTES_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if ((regs[2] & DS1337_ALARM_DISABLE_MASK) == 0) {
		timeptr->tm_hour = bcd2bin(regs[2] & DS1337_ALARM_HOURS_MASK);
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if ((regs[3] & DS1337_ALARM_DISABLE_MASK) == 0) {
		if ((regs[3] & DS1337_DY_DT_MASK) == 0) {
			timeptr->tm_mday = bcd2bin(regs[3] & DS1337_ALARM_DATE_MASK);
			*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		} else {
			timeptr->tm_wday =
				bcd2bin(regs[3] & DS1337_ALARM_DAY_MASK) + DS1337_DAY_OFFSET;
			*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
		}
	}

	LOG_DBG("Get alarm: month day: %d, week day: %d, hour: %d, minute: %d, second: %d mask: "
		"0x%04X",
		timeptr->tm_mday, timeptr->tm_wday, timeptr->tm_hour, timeptr->tm_min,
		timeptr->tm_sec, *mask);

	return 0;
}

static int ds1337_alarm_is_pending(const struct device *dev, uint16_t id)
{
	const struct ds1337_config *config = dev->config;
	uint8_t pending = 0;
	uint8_t status_reg;
	int err;

	if (id >= DS1337_ALARMS_COUNT) {
		LOG_ERR("Invalid alarm ID: %d", id);
		return -EINVAL;
	}

	ds1337_lock_sem(dev);

	err = i2c_reg_read_byte_dt(&config->i2c, DS1337_STATUS_REG, &status_reg);
	if (err) {
		goto out_unlock;
	}

	if (id == DS1337_ALARM_1_ID) {
		if (status_reg & DS1337_A1F_MASK) {
			status_reg &= ~DS1337_A1F_MASK;
			pending = 1;
		}
	} else {
		if (status_reg & DS1337_A2F_MASK) {
			status_reg &= ~DS1337_A2F_MASK;
			pending = 1;
		}
	}

	err = i2c_reg_write_byte_dt(&config->i2c, DS1337_STATUS_REG, status_reg);
	if (err) {
		goto out_unlock;
	}

out_unlock:
	ds1337_unlock_sem(dev);

	if (err) {
		return err;
	}
	return pending;
}

#ifdef DS1337_INT_GPIOS_IN_USE

static int ds1337_alarm_set_callback(const struct device *dev, uint16_t id,
				     rtc_alarm_callback callback, void *user_data)
{
	const struct ds1337_config *config = dev->config;
	struct ds1337_data *data = dev->data;
	uint8_t reg_val = 0;
	uint8_t mask = 0;
	int err;

	if (config->gpio_int.port == NULL) {
		return -ENOTSUP;
	}

	if (id >= DS1337_ALARMS_COUNT) {
		LOG_ERR("Invalid alarm ID: %d", id);
		return -EINVAL;
	}

	ds1337_lock_sem(dev);

	data->alarm_callbacks[id] = callback;
	data->alarm_user_data[id] = user_data;

	/* Enable alarm interrupt if callback provided */
	if (id == DS1337_ALARM_1_ID) {
		mask = DS1337_A1IE_MASK;
		reg_val = (callback != NULL) ? DS1337_A1IE_MASK : 0;
	} else {
		mask = DS1337_A2IE_MASK;
		reg_val = (callback != NULL) ? DS1337_A2IE_MASK : 0;
	}
	err = i2c_reg_update_byte_dt(&config->i2c, DS1337_CONTROL_REG, mask, reg_val);

	ds1337_unlock_sem(dev);

	/* Alarm IRQ might have already been triggered */
	(void)k_work_submit(&data->work);

	return err;
}

#endif

#endif

static int ds1337_init(const struct device *dev)
{
	const struct ds1337_config *config = dev->config;
	struct ds1337_data *data = dev->data;
	uint8_t reg_val;
	int err;

	(void)k_sem_init(&data->lock, 1, 1);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

#ifdef DS1337_INT_GPIOS_IN_USE
	if (config->gpio_int.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_int)) {
			LOG_ERR("GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure interrupt GPIO, error: %d", err);
			return err;
		}

		err = gpio_pin_interrupt_configure_dt(&config->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable GPIO interrupt, error: %d", err);
			return err;
		}

		gpio_init_callback(&data->irq_callback, ds1337_irq_handler,
				   BIT(config->gpio_int.pin));

		err = gpio_add_callback_dt(&config->gpio_int, &data->irq_callback);
		if (err) {
			LOG_ERR("Failed to add GPIO callback, error: %d", err);
			return err;
		}

		data->dev = dev;
		data->work.handler = ds1337_work_callback;
	}
#endif

	/* Display warning if alarm flags are set */
	err = i2c_reg_read_byte_dt(&config->i2c, DS1337_STATUS_REG, &reg_val);
	if (err) {
		return err;
	}
	if (reg_val & DS1337_A1F_MASK) {
		LOG_WRN("Alarm 1 might have been missed!");
	}
	if (reg_val & DS1337_A2F_MASK) {
		LOG_WRN("Alarm 2 might have been missed!");
	}

	/* Enable oscillator */
	err = i2c_reg_update_byte_dt(&config->i2c, DS1337_CONTROL_REG, DS1337_EOSC_MASK, 0);
	if (err) {
		return err;
	}

	/* Configure SQW output frequency */
	switch (config->sqw_freq) {
	case DS1337_SQW_PROP_ENUM_1HZ:
		reg_val = DS1337_SQW_FREQ_1Hz;
		break;
	case DS1337_SQW_PROP_ENUM_4096HZ:
		reg_val = DS1337_SQW_FREQ_4096Hz;
		break;
	case DS1337_SQW_PROP_ENUM_8192HZ:
		reg_val = DS1337_SQW_FREQ_8192Hz;
		break;
	case DS1337_SQW_PROP_ENUM_32768HZ:
	default:
		reg_val = DS1337_SQW_FREQ_32768Hz;
		break;
	}

	/*
	 * Set SQW frequency, enable oscillator, clear INTCN (both alarms will trigger INTA),
	 * disable IRQs
	 */
	err = i2c_reg_write_byte_dt(&config->i2c, DS1337_CONTROL_REG, reg_val);
	if (err) {
		return err;
	}

	/* Clear alarm flags */
	err = i2c_reg_update_byte_dt(&config->i2c, DS1337_STATUS_REG,
				     DS1337_A1F_MASK | DS1337_A2F_MASK, 0);
	if (err) {
		return err;
	}

	return 0;
}

static DEVICE_API(rtc, ds1337_driver_api) = {
	.get_time = ds1337_get_time,
	.set_time = ds1337_set_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = ds1337_alarm_get_supported_fields,
	.alarm_set_time = ds1337_alarm_set_time,
	.alarm_get_time = ds1337_alarm_get_time,
	.alarm_is_pending = ds1337_alarm_is_pending,
#ifdef DS1337_INT_GPIOS_IN_USE
	.alarm_set_callback = ds1337_alarm_set_callback,
#endif
#endif
};

#define DS1337_INIT(inst)                                                                          \
	static struct ds1337_data ds1337_data_##inst;                                              \
	static const struct ds1337_config ds1337_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.sqw_freq = DT_INST_ENUM_IDX_OR(inst, sqw_frequency, DS1337_SQW_PROP_ENUM_1HZ),    \
		IF_ENABLED(                                                                        \
			DS1337_INT_GPIOS_IN_USE,                                                   \
			(.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}))               \
		)										   \
		};										   \
	DEVICE_DT_INST_DEFINE(inst, &ds1337_init, NULL, &ds1337_data_##inst,                       \
			      &ds1337_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,        \
			      &ds1337_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DS1337_INIT)
