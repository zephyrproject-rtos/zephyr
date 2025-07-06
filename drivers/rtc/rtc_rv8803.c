/*
 * Copyright (c) 2024 Marcin Lyda <elektromarcin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/arch/common/ffs.h>
#include "rtc_utils.h"

/* Registers */
#define RV8803_SECONDS_REG               0x00
#define RV8803_MINUTES_REG               0x01
#define RV8803_HOURS_REG                 0x02
#define RV8803_WEEKDAY_REG               0x03
#define RV8803_DATE_REG                  0x04
#define RV8803_MONTH_REG                 0x05
#define RV8803_YEAR_REG                  0x06
#define RV8803_RAM_REG                   0x07
#define RV8803_MINUTES_ALARM_REG         0x08
#define RV8803_HOURS_ALARM_REG           0x09
#define RV8803_WEEKDAY_OR_DATE_ALARM_REG 0x0A
#define RV8803_EXTENSION_REG             0x0D
#define RV8803_FLAG_REG                  0x0E
#define RV8803_CONTROL_REG               0x0F
#define RV8803_OFFSET_REG                0x2C

/* Bitmasks */
#define RV8803_SECONDS_MASK GENMASK(6, 0)
#define RV8803_MINUTES_MASK GENMASK(6, 0)
#define RV8803_HOURS_MASK   GENMASK(5, 0)
#define RV8803_WEEKDAY_MASK GENMASK(6, 0)
#define RV8803_DATE_MASK    GENMASK(5, 0)
#define RV8803_MONTH_MASK   GENMASK(4, 0)
#define RV8803_YEAR_MASK    GENMASK(7, 0)

#define RV8803_MINUTES_ALARM_AE_M_BIT          BIT(7)
#define RV8803_MINUTES_ALARM_MASK              GENMASK(6, 0)
#define RV8803_HOURS_ALARM_AE_H_BIT            BIT(7)
#define RV8803_HOURS_ALARM_MASK                GENMASK(5, 0)
#define RV8803_WEEKDAY_OR_DATE_ALARM_AE_WD_BIT BIT(7)
#define RV8803_WEEKDAY_ALARM_MASK              GENMASK(6, 0)
#define RV8803_DATE_ALARM_MASK                 GENMASK(5, 0)

#define RV8803_EXTENSION_TEST_BIT BIT(7)
#define RV8803_EXTENSION_WADA_BIT BIT(6)
#define RV8803_EXTENSION_USEL_BIT BIT(5)
#define RV8803_EXTENSION_TE_BIT   BIT(4)
#define RV8803_EXTENSION_FD_MASK  GENMASK(3, 2)
#define RV8803_EXTENSION_TD_MASK  GENMASK(1, 0)

#define RV8803_EXTENSION_FD_32768Hz FIELD_PREP(RV8803_EXTENSION_FD_MASK, 0x00)
#define RV8803_EXTENSION_FD_1024Hz  FIELD_PREP(RV8803_EXTENSION_FD_MASK, 0x01)
#define RV8803_EXTENSION_FD_1Hz     FIELD_PREP(RV8803_EXTENSION_FD_MASK, 0x02)

#define RV8803_FLAG_UF_BIT  BIT(5)
#define RV8803_FLAG_TF_BIT  BIT(4)
#define RV8803_FLAG_AF_BIT  BIT(3)
#define RV8803_FLAG_EVF_BIT BIT(2)
#define RV8803_FLAG_V2F_BIT BIT(1)
#define RV8803_FLAG_V1F_BIT BIT(0)

#define RV8803_CONTROL_UIE_BIT   BIT(5)
#define RV8803_CONTROL_TIE_BIT   BIT(4)
#define RV8803_CONTROL_AIE_BIT   BIT(3)
#define RV8803_CONTROL_EIE_BIT   BIT(2)
#define RV8803_CONTROL_RESET_BIT BIT(0)

#define RV8803_MONDAY_MASK    BIT(0)
#define RV8803_TUESDAY_MASK   BIT(1)
#define RV8803_WEDNESDAY_MASK BIT(2)
#define RV8803_THURSDAY_MASK  BIT(3)
#define RV8803_FRIDAY_MASK    BIT(4)
#define RV8803_SATURDAY_MASK  BIT(5)
#define RV8803_SUNDAY_MASK    BIT(6)

#define RV8803_OFFSET_MASK GENMASK(5, 0)

/* Offset between first tm_year and first RV8803 year */
#define RV8803_YEAR_OFFSET (2000 - 1900)

/* RV8803 enumerates months 1 to 12 */
#define RV8803_MONTH_OFFSET -1

/* Max value of seconds, needed for readout procedure workaround */
#define RV8803_SECONDS_MAX_VALUE 59

/* See RV-8803-C7 Application Manual p. 22, 3.9. */
#define RV8803_OFFSET_PPB_PER_LSB    238
#define RV8803_OFFSET_PPB_MIN        (-32 * RV8803_OFFSET_PPB_PER_LSB)
#define RV8803_OFFSET_PPB_MAX        (31 * RV8803_OFFSET_PPB_PER_LSB)
#define RV8803_OFFSET_SIGN_BIT_INDEX 5 /* Required for aging offset sign extension */

/* CLKOUT property enum values */
#define RV8803_PROP_ENUM_1HZ     0
#define RV8803_PROP_ENUM_1024HZ  1
#define RV8803_PROP_ENUM_32768HZ 2

#define DT_DRV_COMPAT microcrystal_rv8803

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "Micro Crystal RV8803 driver enabled without any devices"
#endif

/* RTC time fields supported by RV8803 */
#define RV8803_RTC_TIME_MASK                                                                       \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

/* RTC alarm time fields supported by RV8803 */
#define RV8803_RTC_ALARM_TIME_MASK                                                                 \
	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY |    \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

/* Helper macro to guard GPIO interrupt related stuff */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) &&                                                 \
	(defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE))
#define RV8803_INT_GPIOS_IN_USE 1
#endif

LOG_MODULE_REGISTER(rv8803, CONFIG_RTC_LOG_LEVEL);

struct rv8803_config {
	const struct i2c_dt_spec i2c;
#ifdef RV8803_INT_GPIOS_IN_USE
	struct gpio_dt_spec gpio_int;
#endif
	uint16_t clkout_freq;
};

struct rv8803_data {
	struct k_sem lock;
#ifdef RV8803_INT_GPIOS_IN_USE
	const struct device *dev;
	struct gpio_callback irq_callback;
	struct k_work work;

#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
#endif

#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_callback;
	void *update_user_data;
#endif
#endif
};

static void rv8803_lock_sem(const struct device *dev)
{
	struct rv8803_data *data = dev->data;

	k_sem_take(&data->lock, K_FOREVER);
}

static void rv8803_unlock_sem(const struct device *dev)
{
	struct rv8803_data *data = dev->data;

	k_sem_give(&data->lock);
}

static int rv8803_read_regs(const struct device *dev, uint8_t addr, void *buffer, size_t size)
{
	const struct rv8803_config *config = dev->config;
	int err;

	err = i2c_write_read_dt(&config->i2c, &addr, sizeof(addr), buffer, size);
	if (err) {
		LOG_ERR("Failed to read %zuB from register 0x%02X, error: %d", size, addr, err);
	}
	return err;
}

static int rv8803_read_reg8(const struct device *dev, uint8_t addr, uint8_t *val)
{
	return rv8803_read_regs(dev, addr, val, sizeof(*val));
}

static int rv8803_write_regs(const struct device *dev, uint8_t addr, const void *buffer,
			     size_t size)
{
	const struct rv8803_config *config = dev->config;
	const size_t i2c_data_size = sizeof(addr) + size;
	uint8_t i2c_data[i2c_data_size];
	int err;

	/* Prepend data with I2C device address */
	i2c_data[0] = addr;
	memcpy(&i2c_data[1], buffer, size);

	err = i2c_write_dt(&config->i2c, i2c_data, i2c_data_size);
	if (err) {
		LOG_ERR("Failed to write %zuB to register 0x%02X, error: %d", i2c_data_size, addr,
			err);
	}

	return err;
}

#if defined(RV8803_INT_GPIOS_IN_USE) || defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_CALIBRATION)
static int rv8803_write_reg8(const struct device *dev, uint8_t addr, uint8_t val)
{
	return rv8803_write_regs(dev, addr, &val, sizeof(val));
}
#endif

static int rv8803_update_reg8(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	const struct rv8803_config *config = dev->config;
	int err;

	err = i2c_reg_update_byte_dt(&config->i2c, addr, mask, val);
	if (err) {
		LOG_ERR("Failed to update register 0x%02X with value 0x%02X and mask 0x%02X, "
			"error: %d",
			addr, val, mask, err);
	}
	return err;
}

static uint8_t rv8803_weekday2mask(int weekday)
{
	return (1 << weekday);
}

static int rv8803_mask2weekday(uint8_t mask)
{
	return find_lsb_set(mask) - 1;
}

#ifdef RV8803_INT_GPIOS_IN_USE

static void rv8803_work_callback(struct k_work *work)
{
	struct rv8803_data *data = CONTAINER_OF(work, struct rv8803_data, work);
	const struct device *dev = data->dev;
	rtc_alarm_callback alarm_callback = NULL;
	void *alarm_user_data = NULL;
	rtc_update_callback update_callback = NULL;
	void *update_user_data = NULL;
	int err;
	uint8_t flags;

	rv8803_lock_sem(dev);

	do {
		/* Read flags register */
		err = rv8803_read_reg8(dev, RV8803_FLAG_REG, &flags);
		if (err) {
			break;
		}

#ifdef CONFIG_RTC_ALARM
		/* Handle alarm event */
		if ((flags & RV8803_FLAG_AF_BIT) && (data->alarm_callback != NULL)) {
			flags &= ~RV8803_FLAG_AF_BIT;
			alarm_callback = data->alarm_callback;
			alarm_user_data = data->alarm_user_data;
		}
#endif

#ifdef CONFIG_RTC_UPDATE
		/* Handle update event */
		if ((flags & RV8803_FLAG_UF_BIT) && (data->update_callback != NULL)) {
			flags &= ~RV8803_FLAG_UF_BIT;
			update_callback = data->update_callback;
			update_user_data = data->update_user_data;
		}
#endif

		/* Clear flags */
		err = rv8803_write_reg8(dev, RV8803_FLAG_REG, flags);
		if (err) {
			break;
		}

		/* Check if any interrupt occurred between flags register read/write */
		err = rv8803_read_reg8(dev, RV8803_FLAG_REG, &flags);
		if (err) {
			break;
		}

		if (((flags & RV8803_FLAG_AF_BIT) && (alarm_callback != NULL)) ||
		    ((flags & RV8803_FLAG_UF_BIT) && (update_callback != NULL))) {
			/* Another interrupt occurred while servicing this one */
			k_work_submit(&data->work);
		}
	} while (0);

	rv8803_unlock_sem(dev);

	if (alarm_callback != NULL) {
		/* ID is always zero, there's only one set of alarm regs on chip */
		alarm_callback(dev, 0, alarm_user_data);
		alarm_callback = NULL;
	}

	if (update_callback != NULL) {
		update_callback(dev, update_user_data);
		update_callback = NULL;
	}
}

static void rv8803_irq_handler(const struct device *port, struct gpio_callback *callback,
			       gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct rv8803_data *data = CONTAINER_OF(callback, struct rv8803_data, irq_callback);

	k_work_submit(&data->work);
}

#endif

static int rv8803_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	uint8_t date[7];
	int err;

	if ((timeptr == NULL) || !rtc_utils_validate_rtc_time(timeptr, RV8803_RTC_TIME_MASK) ||
	    (timeptr->tm_year < RV8803_YEAR_OFFSET)) {
		return -EINVAL;
	}

	rv8803_lock_sem(dev);

	date[0] = bin2bcd(timeptr->tm_sec) & RV8803_SECONDS_MASK;
	date[1] = bin2bcd(timeptr->tm_min) & RV8803_MINUTES_MASK;
	date[2] = bin2bcd(timeptr->tm_hour) & RV8803_HOURS_MASK;
	date[3] = rv8803_weekday2mask(timeptr->tm_wday);
	date[4] = bin2bcd(timeptr->tm_mday) & RV8803_DATE_MASK;
	date[5] = bin2bcd(timeptr->tm_mon - RV8803_MONTH_OFFSET) & RV8803_MONTH_MASK;
	date[6] = bin2bcd(timeptr->tm_year - RV8803_YEAR_OFFSET) & RV8803_YEAR_MASK;

	do {
		/* Reset and freeze countdown chain */
		err = rv8803_update_reg8(dev, RV8803_CONTROL_REG, RV8803_CONTROL_RESET_BIT,
					 RV8803_CONTROL_RESET_BIT);
		if (err) {
			break;
		}

		/* Write new time value */
		err = rv8803_write_regs(dev, RV8803_SECONDS_REG, date, sizeof(date));
		if (err) {
			break;
		}

		/* Clear Voltage Low flags */
		err = rv8803_update_reg8(dev, RV8803_FLAG_REG,
					 RV8803_FLAG_V1F_BIT | RV8803_FLAG_V2F_BIT, 0);
		if (err) {
			break;
		}

		/* Release countdown chain lock */
		err = rv8803_update_reg8(dev, RV8803_CONTROL_REG, RV8803_CONTROL_RESET_BIT, 0);
		if (err) {
			break;
		}
	} while (0);

	rv8803_unlock_sem(dev);

	if (!err) {
		LOG_DBG("Set time: year: %d, month: %d, month day: %d, week day: %d, hour: %d, "
			"minute: %d, second: %d",
			timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
			timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
	}

	return err;
}

static int rv8803_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	uint8_t flags;
	uint8_t date_1[7];
	uint8_t date_2[7];
	uint8_t *date = date_1;
	uint8_t seconds_1;
	uint8_t seconds_2;
	int err;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	err = rv8803_read_reg8(dev, RV8803_FLAG_REG, &flags);
	if (err) {
		return err;
	}

	/* Voltage Flag 2 indicates data loss */
	if (flags & RV8803_FLAG_V2F_BIT) {
		return -ENODATA;
	}

	/* Time readout procedure to bypass the inability to freeze registers. */
	/* See RV-8803-C7 Application Manual p. 42, 4.12.2. */
	err = rv8803_read_regs(dev, RV8803_SECONDS_REG, date_1, sizeof(date_1));
	if (err) {
		return err;
	}
	seconds_1 = bcd2bin(date_1[0] & RV8803_SECONDS_MASK);
	if (seconds_1 == RV8803_SECONDS_MAX_VALUE) {
		err = rv8803_read_regs(dev, RV8803_SECONDS_REG, date_2, sizeof(date_2));
		if (err) {
			return err;
		}

		seconds_2 = bcd2bin(date_2[0] & RV8803_SECONDS_MASK);
		if (seconds_2 != RV8803_SECONDS_MAX_VALUE) {
			date = date_2;
		}
	}

	memset(timeptr, 0, sizeof(*timeptr));
	timeptr->tm_sec = bcd2bin(date[0] & RV8803_SECONDS_MASK);
	timeptr->tm_min = bcd2bin(date[1] & RV8803_MINUTES_MASK);
	timeptr->tm_hour = bcd2bin(date[2] & RV8803_HOURS_MASK);
	timeptr->tm_wday = rv8803_mask2weekday(date[3] & RV8803_WEEKDAY_MASK);
	timeptr->tm_mday = bcd2bin(date[4] & RV8803_DATE_MASK);
	timeptr->tm_mon = bcd2bin(date[5] & RV8803_MONTH_MASK) + RV8803_MONTH_OFFSET;
	timeptr->tm_year = bcd2bin(date[6] & RV8803_YEAR_MASK) + RV8803_YEAR_OFFSET;
	timeptr->tm_yday = -1;  /* Unsupported */
	timeptr->tm_isdst = -1; /* Unsupported */
	timeptr->tm_nsec = 0;   /* Unsupported */

	LOG_DBG("Read time: year: %d, month: %d, month day: %d, week day: %d, hour: %d, minute: "
		"%d, second: %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	return 0;
}

#ifdef CONFIG_RTC_ALARM

static int rv8803_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0) {
		LOG_ERR("Invalid alarm ID: %d", id);
		return -EINVAL;
	}

	*mask = RV8803_RTC_ALARM_TIME_MASK;

	return 0;
}

static int rv8803_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				 const struct rtc_time *timeptr)
{
	uint8_t regs[3];
	uint8_t reg_val;
	int err;

	if (id != 0) {
		LOG_ERR("Invalid alarm ID: %d", id);
		return -EINVAL;
	}

	if (mask & ~RV8803_RTC_ALARM_TIME_MASK) {
		LOG_ERR("Unsupported alarm mask 0x%04X, excess field(s): 0x%04X", mask,
			mask & ~(int16_t)RV8803_RTC_ALARM_TIME_MASK);
		return -EINVAL;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) && (mask & RTC_ALARM_TIME_MASK_WEEKDAY)) {
		LOG_ERR("Month day and week day alarms cannot be set simultaneously");
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("Invalid alarm time");
		return -EINVAL;
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		regs[0] = bin2bcd(timeptr->tm_min) & RV8803_MINUTES_ALARM_MASK;
	} else {
		regs[0] = RV8803_MINUTES_ALARM_AE_M_BIT;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		regs[1] = bin2bcd(timeptr->tm_hour) & RV8803_HOURS_ALARM_MASK;
	} else {
		regs[1] = RV8803_HOURS_ALARM_AE_H_BIT;
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		regs[2] = bin2bcd(timeptr->tm_mday) & RV8803_DATE_ALARM_MASK;
	} else if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		regs[2] = rv8803_weekday2mask(timeptr->tm_wday) & RV8803_WEEKDAY_ALARM_MASK;
	} else {
		regs[2] = RV8803_WEEKDAY_OR_DATE_ALARM_AE_WD_BIT;
	}

	/* Update WADA bit */
	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) || (mask & RTC_ALARM_TIME_MASK_WEEKDAY)) {
		reg_val = (mask & RTC_ALARM_TIME_MASK_MONTHDAY) ? RV8803_EXTENSION_WADA_BIT : 0;
		err = rv8803_update_reg8(dev, RV8803_EXTENSION_REG, RV8803_EXTENSION_WADA_BIT,
					 reg_val);
		if (err) {
			return err;
		}
	}

	/* Update alarm registers */
	err = rv8803_write_regs(dev, RV8803_MINUTES_ALARM_REG, regs, sizeof(regs));
	if (err) {
		return err;
	}

	LOG_DBG("Set alarm: month day: %d, week day: %d, hour: %d, minute: %d, mask: 0x%04X",
		timeptr->tm_mday, timeptr->tm_wday, timeptr->tm_hour, timeptr->tm_min, mask);

	return 0;
}

static int rv8803_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				 struct rtc_time *timeptr)
{
	uint8_t regs[3];
	uint8_t reg_val;
	int err;

	if (id != 0) {
		LOG_ERR("Invalid alarm ID: %d", id);
		return -EINVAL;
	}

	err = rv8803_read_regs(dev, RV8803_MINUTES_ALARM_REG, regs, sizeof(regs));
	if (err) {
		return err;
	}

	/* Read extension register to get WADA bit */
	err = rv8803_read_reg8(dev, RV8803_EXTENSION_REG, &reg_val);
	if (err) {
		return err;
	}

	memset(timeptr, 0, sizeof(*timeptr));
	*mask = 0;

	if ((regs[0] & RV8803_MINUTES_ALARM_AE_M_BIT) == 0) {
		timeptr->tm_min = bcd2bin(regs[0] & RV8803_MINUTES_ALARM_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if ((regs[1] & RV8803_HOURS_ALARM_AE_H_BIT) == 0) {
		timeptr->tm_hour = bcd2bin(regs[1] & RV8803_HOURS_ALARM_MASK);
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if ((regs[2] & RV8803_WEEKDAY_OR_DATE_ALARM_AE_WD_BIT) == 0) {
		if (reg_val & RV8803_EXTENSION_WADA_BIT) {
			timeptr->tm_mday = bcd2bin(regs[2] & RV8803_DATE_ALARM_MASK);
			*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		} else {
			timeptr->tm_wday = find_lsb_set(regs[2] & RV8803_WEEKDAY_ALARM_MASK);
			*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
		}
	}

	LOG_DBG("Get alarm: month day: %d, week day: %d, hour: %d, minute: %d, mask: 0x%04X",
		timeptr->tm_mday, timeptr->tm_wday, timeptr->tm_hour, timeptr->tm_min, *mask);

	return 0;
}

static int rv8803_alarm_is_pending(const struct device *dev, uint16_t id)
{
	uint8_t flags;
	int err;

	if (id != 0) {
		LOG_ERR("Invalid alarm ID: %d", id);
		return -EINVAL;
	}

	rv8803_lock_sem(dev);

	do {
		err = rv8803_read_reg8(dev, RV8803_FLAG_REG, &flags);
		if (err) {
			break;
		}

		if (flags & RV8803_FLAG_AF_BIT) {
			flags &= ~RV8803_FLAG_AF_BIT;

			err = rv8803_write_reg8(dev, RV8803_FLAG_REG, flags);
			if (err) {
				break;
			}

			/* Indicate that alarm is pending */
			err = 1;
		}
	} while (0);

	rv8803_unlock_sem(dev);

	return err;
}

#ifdef RV8803_INT_GPIOS_IN_USE

static int rv8803_alarm_set_callback(const struct device *dev, uint16_t id,
				     rtc_alarm_callback callback, void *user_data)
{
	const struct rv8803_config *config = dev->config;
	struct rv8803_data *data = dev->data;
	uint8_t reg_val;
	int err;

	if (config->gpio_int.port == NULL) {
		return -ENOTSUP;
	}

	if (id != 0) {
		LOG_ERR("Invalid alarm ID: %d", id);
		return -EINVAL;
	}

	rv8803_lock_sem(dev);

	data->alarm_callback = callback;
	data->alarm_user_data = user_data;

	/* Enable alarm interrupt if callback provided */
	reg_val = (callback != NULL) ? RV8803_CONTROL_AIE_BIT : 0;
	err = rv8803_update_reg8(dev, RV8803_CONTROL_REG, RV8803_CONTROL_AIE_BIT, reg_val);

	rv8803_unlock_sem(dev);

	/* Alarm IRQ might have already been triggered */
	k_work_submit(&data->work);

	return err;
}

#endif

#endif

#if defined(RV8803_INT_GPIOS_IN_USE) && defined(CONFIG_RTC_UPDATE)

static int rv8803_update_set_callback(const struct device *dev, rtc_update_callback callback,
				      void *user_data)
{
	const struct rv8803_config *config = dev->config;
	struct rv8803_data *data = dev->data;
	uint8_t reg_val;
	int err;

	if (config->gpio_int.port == NULL) {
		return -ENOTSUP;
	}

	rv8803_lock_sem(dev);

	data->update_callback = callback;
	data->update_user_data = user_data;

	/* Enable update interrupt if callback provided */
	reg_val = (callback != NULL) ? RV8803_CONTROL_UIE_BIT : 0;
	err = rv8803_update_reg8(dev, RV8803_CONTROL_REG, RV8803_CONTROL_UIE_BIT, reg_val);

	rv8803_unlock_sem(dev);

	/* Update IRQ might have already been triggered */
	k_work_submit(&data->work);

	return err;
}

#endif

#ifdef CONFIG_RTC_CALIBRATION

static int rv8803_set_calibration(const struct device *dev, int32_t freq_ppb)
{
	int8_t offset;

	if ((freq_ppb < RV8803_OFFSET_PPB_MIN) || (freq_ppb > RV8803_OFFSET_PPB_MAX)) {
		LOG_ERR("Calibration value %d ppb out of range", freq_ppb);
		return -EINVAL;
	}

	offset = (freq_ppb / RV8803_OFFSET_PPB_PER_LSB) & RV8803_OFFSET_MASK;

	LOG_DBG("Set calibration: frequency ppb: %d, offset value: %d", freq_ppb, offset);

	return rv8803_write_reg8(dev, RV8803_OFFSET_REG, offset);
}

static int rv8803_get_calibration(const struct device *dev, int32_t *freq_ppb)
{
	int8_t offset;
	int err;

	err = rv8803_read_reg8(dev, RV8803_OFFSET_REG, &offset);
	if (err) {
		return err;
	}

	*freq_ppb = sign_extend(offset, RV8803_OFFSET_SIGN_BIT_INDEX) * RV8803_OFFSET_PPB_PER_LSB;

	LOG_DBG("Get calibration: frequency ppb: %d, offset value: %d", *freq_ppb, offset);

	return 0;
}
#endif

static int rv8803_init(const struct device *dev)
{
	const struct rv8803_config *config = dev->config;
	struct rv8803_data *data = dev->data;
	uint8_t freq;
	uint8_t regs[3];
	int err;

	k_sem_init(&data->lock, 1, 1);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

#ifdef RV8803_INT_GPIOS_IN_USE
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

		gpio_init_callback(&data->irq_callback, rv8803_irq_handler,
				   BIT(config->gpio_int.pin));

		err = gpio_add_callback_dt(&config->gpio_int, &data->irq_callback);
		if (err) {
			LOG_ERR("Failed to add GPIO callback, error: %d", err);
			return err;
		}

		data->dev = dev;
		data->work.handler = rv8803_work_callback;
	}
#endif

	/* Configure CLKOUT frequency */
	switch (config->clkout_freq) {
	case RV8803_PROP_ENUM_1HZ:
		freq = RV8803_EXTENSION_FD_1Hz;
		break;
	case RV8803_PROP_ENUM_1024HZ:
		freq = RV8803_EXTENSION_FD_1024Hz;
		break;
	case RV8803_PROP_ENUM_32768HZ:
	default:
		freq = RV8803_EXTENSION_FD_32768Hz;
		break;
	}
	err = rv8803_update_reg8(dev, RV8803_EXTENSION_REG, RV8803_EXTENSION_FD_MASK, freq);
	if (err) {
		return -ENODEV;
	}

	/* Clear alarm and update flag */
	err = rv8803_update_reg8(dev, RV8803_CONTROL_REG, RV8803_FLAG_AF_BIT | RV8803_FLAG_UF_BIT,
				 RV8803_FLAG_AF_BIT | RV8803_FLAG_UF_BIT);
	if (err) {
		return -ENODEV;
	}

	/* Disable IRQs */
	err = rv8803_update_reg8(dev, RV8803_CONTROL_REG,
				 RV8803_CONTROL_AIE_BIT | RV8803_CONTROL_UIE_BIT, 0);
	if (err) {
		return -ENODEV;
	}

	/* Disable alarms */
	err = rv8803_read_regs(dev, RV8803_MINUTES_ALARM_REG, regs, sizeof(regs));
	if (err) {
		return -ENODEV;
	}

	regs[0] |= RV8803_MINUTES_ALARM_AE_M_BIT;
	regs[1] |= RV8803_HOURS_ALARM_AE_H_BIT;
	regs[2] |= RV8803_WEEKDAY_OR_DATE_ALARM_AE_WD_BIT;

	err = rv8803_write_regs(dev, RV8803_MINUTES_ALARM_REG, regs, sizeof(regs));
	if (err) {
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(rtc, rv8803_driver_api) = {
	.set_time = rv8803_set_time,
	.get_time = rv8803_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rv8803_alarm_get_supported_fields,
	.alarm_set_time = rv8803_alarm_set_time,
	.alarm_get_time = rv8803_alarm_get_time,
	.alarm_is_pending = rv8803_alarm_is_pending,
#ifdef RV8803_INT_GPIOS_IN_USE
	.alarm_set_callback = rv8803_alarm_set_callback,
#endif
#endif
#if defined(RV8803_INT_GPIOS_IN_USE) && defined(CONFIG_RTC_UPDATE)
	.update_set_callback = rv8803_update_set_callback,
#endif
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = rv8803_set_calibration,
	.get_calibration = rv8803_get_calibration
#endif
};

#define RV8803_INIT(inst)                                                                          \
	static const struct rv8803_config rv8803_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.clkout_freq = DT_INST_ENUM_IDX_OR(inst, clkout_frequency, 0),                     \
		IF_ENABLED(                                                                        \
			RV8803_INT_GPIOS_IN_USE,                                                   \
			(.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}))               \
		)                                                                                  \
	};                                                                                         \
                                                                                                   \
	static struct rv8803_data rv8803_data_##inst;                                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, rv8803_init, NULL, &rv8803_data_##inst, &rv8803_config_##inst, \
			      POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, &rv8803_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV8803_INIT);
