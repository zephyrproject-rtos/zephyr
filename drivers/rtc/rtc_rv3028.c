/*
 * Copyright (c) 2024 ANITRA system s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv3028

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include "rtc_utils.h"

LOG_MODULE_REGISTER(rv3028, CONFIG_RTC_LOG_LEVEL);

/* RV3028 RAM register addresses */
#define RV3028_REG_SECONDS              0x00
#define RV3028_REG_MINUTES              0x01
#define RV3028_REG_HOURS                0x02
#define RV3028_REG_WEEKDAY              0x03
#define RV3028_REG_DATE                 0x04
#define RV3028_REG_MONTH                0x05
#define RV3028_REG_YEAR                 0x06
#define RV3028_REG_ALARM_MINUTES        0x07
#define RV3028_REG_ALARM_HOURS          0x08
#define RV3028_REG_ALARM_WEEKDAY        0x09
#define RV3028_REG_STATUS               0x0E
#define RV3028_REG_CONTROL1             0x0F
#define RV3028_REG_CONTROL2             0x10
#define RV3028_REG_EVENT_CONTROL        0x13
#define RV3028_REG_TS_COUNT             0x14
#define RV3028_REG_TS_SECONDS           0x15
#define RV3028_REG_TS_MINUTES           0x16
#define RV3028_REG_TS_HOURS             0x17
#define RV3028_REG_TS_DATE              0x18
#define RV3028_REG_TS_MONTH             0x19
#define RV3028_REG_TS_YEAR              0x1A
#define RV3028_REG_UNIXTIME0            0x1B
#define RV3028_REG_UNIXTIME1            0x1C
#define RV3028_REG_UNIXTIME2            0x1D
#define RV3028_REG_UNIXTIME3            0x1E
#define RV3028_REG_USER_RAM1            0x1F
#define RV3028_REG_USER_RAM2            0x20
#define RV3028_REG_EEPROM_ADDRESS       0x25
#define RV3028_REG_EEPROM_DATA          0x26
#define RV3028_REG_EEPROM_COMMAND       0x27
#define RV3028_REG_ID                   0x28
#define RV3028_REG_CLKOUT               0x35
#define RV3028_REG_OFFSET               0x36
#define RV3028_REG_BACKUP               0x37

#define RV3028_CONTROL1_TD              BIT(0)
#define RV3028_CONTROL1_TE              GENMASK(2, 1)
#define RV3028_CONTROL1_EERD            BIT(3)
#define RV3028_CONTROL1_USEL            BIT(4)
#define RV3028_CONTROL1_WADA            BIT(5)
#define RV3028_CONTROL1_TRPT            BIT(7)

#define RV3028_CONTROL2_RESET           BIT(0)
#define RV3028_CONTROL2_12_24           BIT(1)
#define RV3028_CONTROL2_EIE             BIT(2)
#define RV3028_CONTROL2_AIE             BIT(3)
#define RV3028_CONTROL2_TIE             BIT(4)
#define RV3028_CONTROL2_UIE             BIT(5)
#define RV3028_CONTROL2_TSE             BIT(7)

#define RV3028_STATUS_PORF              BIT(0)
#define RV3028_STATUS_EVF               BIT(1)
#define RV3028_STATUS_AF                BIT(2)
#define RV3028_STATUS_TF                BIT(3)
#define RV3028_STATUS_UF                BIT(4)
#define RV3028_STATUS_BSF               BIT(5)
#define RV3028_STATUS_CLKF              BIT(6)
#define RV3028_STATUS_EEBUSY            BIT(7)

#define RV3028_CLKOUT_FD                GENMASK(2, 0)
#define RV3028_CLKOUT_PORIE             BIT(3)
#define RV3028_CLKOUT_CLKSY             BIT(6)
#define RV3028_CLKOUT_CLKOE             BIT(7)

#define RV3028_CLKOUT_FD_LOW            0x7

#define RV3028_BACKUP_TCE               BIT(5)
#define RV3028_BACKUP_TCR               GENMASK(1, 0)
#define RV3028_BACKUP_BSM               GENMASK(3, 2)

#define RV3028_BSM_LEVEL                0x3
#define RV3028_BSM_DIRECT               0x1
#define RV3028_BSM_DISABLED             0x0

/* RV3028 EE command register values */
#define RV3028_EEPROM_CMD_INIT          0x00
#define RV3028_EEPROM_CMD_UPDATE        0x11
#define RV3028_EEPROM_CMD_REFRESH       0x12
#define RV3028_EEPROM_CMD_WRITE         0x21
#define RV3028_EEPROM_CMD_READ          0x22

#define RV3028_SECONDS_MASK             GENMASK(6, 0)
#define RV3028_MINUTES_MASK             GENMASK(6, 0)
#define RV3028_HOURS_AMPM               BIT(5)
#define RV3028_HOURS_12H_MASK           GENMASK(4, 0)
#define RV3028_HOURS_24H_MASK           GENMASK(5, 0)
#define RV3028_DATE_MASK                GENMASK(5, 0)
#define RV3028_WEEKDAY_MASK             GENMASK(2, 0)
#define RV3028_MONTH_MASK               GENMASK(4, 0)
#define RV3028_YEAR_MASK                GENMASK(7, 0)

#define RV3028_ALARM_MINUTES_AE_M       BIT(7)
#define RV3028_ALARM_MINUTES_MASK       GENMASK(6, 0)
#define RV3028_ALARM_HOURS_AE_H         BIT(7)
#define RV3028_ALARM_HOURS_AMPM         BIT(5)
#define RV3028_ALARM_HOURS_12H_MASK     GENMASK(4, 0)
#define RV3028_ALARM_HOURS_24H_MASK     GENMASK(5, 0)
#define RV3028_ALARM_DATE_AE_WD         BIT(7)
#define RV3028_ALARM_DATE_MASK          GENMASK(5, 0)

/* The RV3028 only supports two-digit years. Leap years are correctly handled from 2000 to 2099 */
#define RV3028_YEAR_OFFSET              (2000 - 1900)

/* The RV3028 enumerates months 1 to 12 */
#define RV3028_MONTH_OFFSET 1

#define RV3028_EEBUSY_POLL_US           10000
#define RV3028_EEBUSY_TIMEOUT_MS        100

/* RTC alarm time fields supported by the RV3028 */
#define RV3028_RTC_ALARM_TIME_MASK                                                                 \
	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY)

/* RTC time fields supported by the RV3028 */
#define RV3028_RTC_TIME_MASK                                                                       \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

/* Helper macro to guard int-gpios related code */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) &&                                                 \
	(defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE))
#define RV3028_INT_GPIOS_IN_USE 1
#endif

struct rv3028_config {
	const struct i2c_dt_spec i2c;
#ifdef RV3028_INT_GPIOS_IN_USE
	struct gpio_dt_spec gpio_int;
#endif /* RV3028_INT_GPIOS_IN_USE */
	uint8_t cof;
	uint8_t backup;
};

struct rv3028_data {
	struct k_sem lock;
#if RV3028_INT_GPIOS_IN_USE
	const struct device *dev;
	struct gpio_callback int_callback;
	struct k_work work;

#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_callback;
	void *update_user_data;
#endif /* CONFIG_RTC_UPDATE */
#endif /* RV3028_INT_GPIOS_IN_USE */
};

static void rv3028_lock_sem(const struct device *dev)
{
	struct rv3028_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
}

static void rv3028_unlock_sem(const struct device *dev)
{
	struct rv3028_data *data = dev->data;

	k_sem_give(&data->lock);
}

static int rv3028_read_regs(const struct device *dev, uint8_t addr, void *buf, size_t len)
{
	const struct rv3028_config *config = dev->config;
	int err;

	err = i2c_write_read_dt(&config->i2c, &addr, sizeof(addr), buf, len);
	if (err) {
		LOG_ERR("failed to read reg addr 0x%02x, len %d (err %d)", addr, len, err);
		return err;
	}

	return 0;
}

static int rv3028_read_reg8(const struct device *dev, uint8_t addr, uint8_t *val)
{
	return rv3028_read_regs(dev, addr, val, sizeof(*val));
}

static int rv3028_write_regs(const struct device *dev, uint8_t addr, void *buf, size_t len)
{
	const struct rv3028_config *config = dev->config;
	uint8_t block[sizeof(addr) + len];
	int err;

	block[0] = addr;
	memcpy(&block[1], buf, len);

	err = i2c_write_dt(&config->i2c, block, sizeof(block));
	if (err) {
		LOG_ERR("failed to write reg addr 0x%02x, len %d (err %d)", addr, len, err);
		return err;
	}

	return 0;
}

static int rv3028_write_reg8(const struct device *dev, uint8_t addr, uint8_t val)
{
	return rv3028_write_regs(dev, addr, &val, sizeof(val));
}

static int rv3028_update_reg8(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	const struct rv3028_config *config = dev->config;
	int err;

	err = i2c_reg_update_byte_dt(&config->i2c, addr, mask, val);
	if (err) {
		LOG_ERR("failed to update reg addr 0x%02x, mask 0x%02x, val 0x%02x (err %d)", addr,
			mask, val, err);
		return err;
	}

	return 0;
}

static int rv3028_eeprom_wait_busy(const struct device *dev)
{
	uint8_t status = 0;
	int err;
	int64_t timeout_time = k_uptime_get() + RV3028_EEBUSY_TIMEOUT_MS;

	/* Wait while the EEPROM is busy */
	for (;;) {
		err = rv3028_read_reg8(dev, RV3028_REG_STATUS, &status);
		if (err) {
			return err;
		}

		if (!(status & RV3028_STATUS_EEBUSY)) {
			break;
		}

		if (k_uptime_get() > timeout_time) {
			return -ETIME;
		}

		k_busy_wait(RV3028_EEBUSY_POLL_US);
	}

	return 0;
}

static int rv3028_exit_eerd(const struct device *dev)
{
	return rv3028_update_reg8(dev, RV3028_REG_CONTROL1, RV3028_CONTROL1_EERD, 0);
}

static int rv3028_enter_eerd(const struct device *dev)
{
	uint8_t ctrl1;
	bool eerd;
	int ret;

	ret = rv3028_read_reg8(dev, RV3028_REG_CONTROL1, &ctrl1);
	if (ret) {
		return ret;
	}

	eerd = ctrl1 & RV3028_CONTROL1_EERD;
	if (eerd) {
		return 0;
	}

	ret = rv3028_update_reg8(dev, RV3028_REG_CONTROL1, RV3028_CONTROL1_EERD,
				 RV3028_CONTROL1_EERD);

	ret = rv3028_eeprom_wait_busy(dev);
	if (ret) {
		rv3028_exit_eerd(dev);
		return ret;
	}

	return ret;
}

static int rv3028_eeprom_command(const struct device *dev, uint8_t command)
{
	int err;

	err = rv3028_write_reg8(dev, RV3028_REG_EEPROM_COMMAND, RV3028_EEPROM_CMD_INIT);
	if (err) {
		return err;
	}

	return rv3028_write_reg8(dev, RV3028_REG_EEPROM_COMMAND, command);
}

static int rv3028_update(const struct device *dev)
{
	int err;

	err = rv3028_eeprom_command(dev, RV3028_EEPROM_CMD_UPDATE);
	if (err) {
		goto exit_eerd;
	}

	err = rv3028_eeprom_wait_busy(dev);

exit_eerd:
	rv3028_exit_eerd(dev);

	return err;
}

static int rv3028_refresh(const struct device *dev)
{
	int err;

	err = rv3028_eeprom_command(dev, RV3028_EEPROM_CMD_REFRESH);
	if (err) {
		goto exit_eerd;
	}

	err = rv3028_eeprom_wait_busy(dev);

exit_eerd:
	rv3028_exit_eerd(dev);

	return err;
}

static int rv3028_update_cfg(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	uint8_t val_old, val_new;
	int err;

	err = rv3028_read_reg8(dev, addr, &val_old);
	if (err) {
		return err;
	}

	val_new = (val_old & ~mask) | (val & mask);
	if (val_new == val_old) {
		return 0;
	}

	err = rv3028_enter_eerd(dev);
	if (err) {
		return err;
	}

	err = rv3028_write_reg8(dev, addr, val_new);
	if (err) {
		rv3028_exit_eerd(dev);
		return err;
	}

	return rv3028_update(dev);
}

#if RV3028_INT_GPIOS_IN_USE

static int rv3028_int_enable_unlocked(const struct device *dev, bool enable)
{
	const struct rv3028_config *config = dev->config;
	uint8_t clkout = 0;
	int err;

	if (enable || config->cof == RV3028_CLKOUT_FD_LOW) {
		/* Disable CLKOUT */
		clkout |= FIELD_PREP(RV3028_CLKOUT_FD, RV3028_CLKOUT_FD_LOW);
	} else {
		/* Configure CLKOUT frequency */
		clkout |= RV3028_CLKOUT_CLKOE |
			  FIELD_PREP(RV3028_CLKOUT_FD, config->cof);
	}

	/* Configure the CLKOUT register */
	err = rv3028_update_cfg(dev,
				RV3028_REG_CLKOUT,
				RV3028_CLKOUT_FD | RV3028_CLKOUT_CLKOE,
				clkout);
	if (err) {
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->gpio_int,
					      enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
	if (err) {
		LOG_ERR("failed to %s GPIO interrupt (err %d)", enable ? "enable" : "disable", err);
		return err;
	}

	return 0;
}

static void rv3028_work_cb(struct k_work *work)
{
	struct rv3028_data *data = CONTAINER_OF(work, struct rv3028_data, work);
	const struct device *dev = data->dev;
	rtc_alarm_callback alarm_callback = NULL;
	void *alarm_user_data = NULL;
	rtc_update_callback update_callback = NULL;
	void *update_user_data = NULL;
	uint8_t status;
	int err;

	rv3028_lock_sem(dev);

	err = rv3028_read_reg8(data->dev, RV3028_REG_STATUS, &status);
	if (err) {
		goto unlock;
	}

#ifdef CONFIG_RTC_ALARM
	if ((status & RV3028_STATUS_AF) && data->alarm_callback != NULL) {
		status &= ~(RV3028_STATUS_AF);
		alarm_callback = data->alarm_callback;
		alarm_user_data = data->alarm_user_data;
	}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	if ((status & RV3028_STATUS_UF) && data->update_callback != NULL) {
		status &= ~(RV3028_STATUS_UF);
		update_callback = data->update_callback;
		update_user_data = data->update_user_data;
	}
#endif /* CONFIG_RTC_UPDATE */

	err = rv3028_write_reg8(dev, RV3028_REG_STATUS, status);
	if (err) {
		goto unlock;
	}

	/* Check if interrupt occurred between STATUS read/write */
	err = rv3028_read_reg8(dev, RV3028_REG_STATUS, &status);
	if (err) {
		goto unlock;
	}

	if (((status & RV3028_STATUS_AF) && alarm_callback != NULL) ||
	    ((status & RV3028_STATUS_UF) && update_callback != NULL)) {
		/* Another interrupt occurred while servicing this one */
		k_work_submit(&data->work);
	}

unlock:
	rv3028_unlock_sem(dev);

	if (alarm_callback != NULL) {
		alarm_callback(dev, 0U, alarm_user_data);
		alarm_callback = NULL;
	}

	if (update_callback != NULL) {
		update_callback(dev, update_user_data);
		update_callback = NULL;
	}
}

static void rv3028_int_handler(const struct device *port, struct gpio_callback *cb,
			       gpio_port_pins_t pins)
{
	struct rv3028_data *data = CONTAINER_OF(cb, struct rv3028_data, int_callback);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}

#endif /* RV3028_INT_GPIOS_IN_USE */

static int rv3028_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	uint8_t date[7];
	int err;

	if (timeptr == NULL ||
	    !rtc_utils_validate_rtc_time(timeptr, RV3028_RTC_TIME_MASK) ||
	    (timeptr->tm_year < RV3028_YEAR_OFFSET)) {
		LOG_ERR("invalid time");
		return -EINVAL;
	}

	rv3028_lock_sem(dev);

	LOG_DBG("set time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	date[0] = bin2bcd(timeptr->tm_sec) & RV3028_SECONDS_MASK;
	date[1] = bin2bcd(timeptr->tm_min) & RV3028_MINUTES_MASK;
	date[2] = bin2bcd(timeptr->tm_hour) & RV3028_HOURS_24H_MASK;
	date[3] = bin2bcd(timeptr->tm_wday) & RV3028_WEEKDAY_MASK;
	date[4] = bin2bcd(timeptr->tm_mday) & RV3028_DATE_MASK;
	date[5] = bin2bcd(timeptr->tm_mon + RV3028_MONTH_OFFSET) & RV3028_MONTH_MASK;
	date[6] = bin2bcd(timeptr->tm_year - RV3028_YEAR_OFFSET) & RV3028_YEAR_MASK;

	err = rv3028_write_regs(dev, RV3028_REG_SECONDS, &date, sizeof(date));
	if (err) {
		goto unlock;
	}

	/* Clear Power On Reset Flag */
	err = rv3028_update_reg8(dev, RV3028_REG_STATUS, RV3028_STATUS_PORF, 0);

unlock:
	rv3028_unlock_sem(dev);

	return err;
}

static int rv3028_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	uint8_t status;
	uint8_t date[7];
	int err;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	err = rv3028_read_reg8(dev, RV3028_REG_STATUS, &status);
	if (err) {
		return err;
	}

	if (status & RV3028_STATUS_PORF) {
		/* Power On Reset Flag indicates invalid data */
		return -ENODATA;
	}

	err = rv3028_read_regs(dev, RV3028_REG_SECONDS, date, sizeof(date));
	if (err) {
		return err;
	}

	memset(timeptr, 0U, sizeof(*timeptr));
	timeptr->tm_sec = bcd2bin(date[0] & RV3028_SECONDS_MASK);
	timeptr->tm_min = bcd2bin(date[1] & RV3028_MINUTES_MASK);
	timeptr->tm_hour = bcd2bin(date[2] & RV3028_HOURS_24H_MASK);
	timeptr->tm_wday = bcd2bin(date[3] & RV3028_WEEKDAY_MASK);
	timeptr->tm_mday = bcd2bin(date[4] & RV3028_DATE_MASK);
	timeptr->tm_mon = bcd2bin(date[5] & RV3028_MONTH_MASK) - RV3028_MONTH_OFFSET;
	timeptr->tm_year = bcd2bin(date[6] & RV3028_YEAR_MASK) + RV3028_YEAR_OFFSET;
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;

	LOG_DBG("get time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	return 0;
}

#ifdef CONFIG_RTC_ALARM

static int rv3028_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	*mask = RV3028_RTC_ALARM_TIME_MASK;

	return 0;
}

static int rv3028_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				 const struct rtc_time *timeptr)
{
	uint8_t regs[3];

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	if (mask & ~(RV3028_RTC_ALARM_TIME_MASK)) {
		LOG_ERR("unsupported alarm field mask 0x%04x", mask);
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("invalid alarm time");
		return -EINVAL;
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		regs[0] = bin2bcd(timeptr->tm_min) & RV3028_ALARM_MINUTES_MASK;
	} else {
		regs[0] = RTC_ALARM_TIME_MASK_MINUTE;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		regs[1] = bin2bcd(timeptr->tm_hour) & RV3028_ALARM_HOURS_24H_MASK;
	} else {
		regs[1] = RV3028_ALARM_HOURS_AE_H;
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		regs[2] = bin2bcd(timeptr->tm_mday) & RV3028_ALARM_DATE_MASK;
	} else {
		regs[2] = RV3028_ALARM_DATE_AE_WD;
	}

	LOG_DBG("set alarm: mday = %d, hour = %d, min = %d, mask = 0x%04x",
		timeptr->tm_mday, timeptr->tm_hour, timeptr->tm_min, mask);

	/* Write registers RV3028_REG_ALARM_MINUTES through RV3028_REG_ALARM_WEEKDAY */
	return rv3028_write_regs(dev, RV3028_REG_ALARM_MINUTES, &regs, sizeof(regs));
}

static int rv3028_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				 struct rtc_time *timeptr)
{
	uint8_t regs[3];
	int err;

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	/* Read registers RV3028_REG_ALARM_MINUTES through RV3028_REG_ALARM_WEEKDAY */
	err = rv3028_read_regs(dev, RV3028_REG_ALARM_MINUTES, &regs, sizeof(regs));
	if (err) {
		return err;
	}

	memset(timeptr, 0U, sizeof(*timeptr));
	*mask = 0U;

	if ((regs[0] & RV3028_ALARM_MINUTES_AE_M) == 0) {
		timeptr->tm_min = bcd2bin(regs[0] & RV3028_ALARM_MINUTES_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if ((regs[1] & RV3028_ALARM_HOURS_AE_H) == 0) {
		timeptr->tm_hour = bcd2bin(regs[1] & RV3028_ALARM_HOURS_24H_MASK);
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if ((regs[2] & RV3028_ALARM_DATE_AE_WD) == 0) {
		timeptr->tm_mday = bcd2bin(regs[2] & RV3028_ALARM_DATE_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	LOG_DBG("get alarm: mday = %d, hour = %d, min = %d, mask = 0x%04x",
		timeptr->tm_mday, timeptr->tm_hour, timeptr->tm_min, *mask);

	return 0;
}

static int rv3028_alarm_is_pending(const struct device *dev, uint16_t id)
{
	uint8_t status;
	int err;

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	rv3028_lock_sem(dev);

	err = rv3028_read_reg8(dev, RV3028_REG_STATUS, &status);
	if (err) {
		goto unlock;
	}

	if (status & RV3028_STATUS_AF) {
		/* Clear alarm flag */
		status &= ~(RV3028_STATUS_AF);

		err = rv3028_write_reg8(dev, RV3028_REG_STATUS, status);
		if (err) {
			goto unlock;
		}

		/* Alarm pending */
		err = 1;
	}

unlock:
	rv3028_unlock_sem(dev);

	return err;
}

static int rv3028_alarm_set_callback(const struct device *dev, uint16_t id,
				     rtc_alarm_callback callback, void *user_data)
{
#ifndef RV3028_INT_GPIOS_IN_USE
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
#else
	const struct rv3028_config *config = dev->config;
	struct rv3028_data *data = dev->data;
	uint8_t control_2;
	int err = 0;

	if (config->gpio_int.port == NULL) {
		return -ENOTSUP;
	}

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	rv3028_lock_sem(dev);

	data->alarm_callback = callback;
	data->alarm_user_data = user_data;

	err = rv3028_read_reg8(dev, RV3028_REG_CONTROL2, &control_2);
	if (err) {
		goto unlock;
	}

	if (callback != NULL) {
		control_2 |= RV3028_CONTROL2_AIE;
	} else {
		control_2 &= ~(RV3028_CONTROL2_AIE);
	}

	if ((control_2 & RV3028_CONTROL2_UIE) == 0U) {
		/* Only change INT GPIO if periodic time update interrupt not enabled */
		err = rv3028_int_enable_unlocked(dev, callback != NULL);
		if (err) {
			goto unlock;
		}
	}

	err = rv3028_write_reg8(dev, RV3028_REG_CONTROL2, control_2);
	if (err) {
		goto unlock;
	}

unlock:
	rv3028_unlock_sem(dev);

	/* Alarm flag may already be set */
	k_work_submit(&data->work);

	return err;
#endif /* RV3028_INT_GPIOS_IN_USE */
}

#endif /* CONFIG_RTC_ALARM */

#if RV3028_INT_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE)

static int rv3028_update_set_callback(const struct device *dev, rtc_update_callback callback,
				      void *user_data)
{
	const struct rv3028_config *config = dev->config;
	struct rv3028_data *data = dev->data;
	uint8_t control_2;
	int err;

	if (config->gpio_int.port == NULL) {
		return -ENOTSUP;
	}

	rv3028_lock_sem(dev);

	data->update_callback = callback;
	data->update_user_data = user_data;

	err = rv3028_read_reg8(dev, RV3028_REG_CONTROL2, &control_2);
	if (err) {
		goto unlock;
	}

	if (callback != NULL) {
		control_2 |= RV3028_CONTROL2_UIE;
	} else {
		control_2 &= ~(RV3028_CONTROL2_UIE);
	}

	if ((control_2 & RV3028_CONTROL2_AIE) == 0U) {
		/* Only change INT GPIO if alarm interrupt not enabled */
		err = rv3028_int_enable_unlocked(dev, callback != NULL);
		if (err) {
			goto unlock;
		}
	}

	err = rv3028_write_reg8(dev, RV3028_REG_CONTROL2, control_2);
	if (err) {
		goto unlock;
	}

unlock:
	rv3028_unlock_sem(dev);

	/* Seconds flag may already be set */
	k_work_submit(&data->work);

	return err;
}

#endif /* RV3028_INT_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE) */

static int rv3028_init(const struct device *dev)
{
	const struct rv3028_config *config = dev->config;
	struct rv3028_data *data = dev->data;
	uint8_t regs[3];
	uint8_t val;
	int err;

	k_sem_init(&data->lock, 1, 1);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	err = rv3028_read_reg8(dev, RV3028_REG_ID, &val);
	if (err) {
		return -ENODEV;
	}

	LOG_DBG("HID: 0x%02x, VID: 0x%02x", (val & 0xF0) >> 0x04, val & 0x0F);

#if RV3028_INT_GPIOS_IN_USE
	if (config->gpio_int.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_int)) {
			LOG_ERR("GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);
		if (err) {
			LOG_ERR("failed to configure GPIO (err %d)", err);
			return -ENODEV;
		}

		gpio_init_callback(&data->int_callback, rv3028_int_handler,
				   BIT(config->gpio_int.pin));

		err = gpio_add_callback_dt(&config->gpio_int, &data->int_callback);
		if (err) {
			LOG_ERR("failed to add GPIO callback (err %d)", err);
			return -ENODEV;
		}

		data->dev = dev;
		data->work.handler = rv3028_work_cb;
	}
#endif /* RV3028_INT_GPIOS_IN_USE */

	err = rv3028_read_reg8(dev, RV3028_REG_STATUS, &val);
	if (err) {
		return -ENODEV;
	}

	if (val & RV3028_STATUS_AF) {
		LOG_WRN("an alarm may have been missed");
	}

	/* Refresh the settings in the RAM with the settings from the EEPROM */
	err = rv3028_enter_eerd(dev);
	if (err) {
		return -ENODEV;
	}
	err = rv3028_refresh(dev);
	if (err) {
		return -ENODEV;
	}

	/* Configure the CLKOUT register */
	val = FIELD_PREP(RV3028_CLKOUT_FD, config->cof) |
	      (config->cof != RV3028_CLKOUT_FD_LOW ? RV3028_CLKOUT_CLKOE : 0);
	err = rv3028_update_cfg(dev,
				RV3028_REG_CLKOUT,
				RV3028_CLKOUT_FD | RV3028_CLKOUT_CLKOE,
				val);
	if (err) {
		return -ENODEV;
	}

	err = rv3028_update_cfg(dev,
				RV3028_REG_BACKUP,
				RV3028_BACKUP_TCE | RV3028_BACKUP_TCR | RV3028_BACKUP_BSM,
				config->backup);
	if (err) {
		return -ENODEV;
	}

	err = rv3028_update_reg8(dev, RV3028_REG_CONTROL1, RV3028_CONTROL1_WADA,
				 RV3028_CONTROL1_WADA);
	if (err) {
		return -ENODEV;
	}

	/* Disable the alarms */
	err = rv3028_update_reg8(dev,
				 RV3028_REG_CONTROL2,
				 RV3028_CONTROL2_AIE | RV3028_CONTROL2_UIE,
				 0);
	if (err) {
		return -ENODEV;
	}

	err = rv3028_read_regs(dev, RV3028_REG_ALARM_MINUTES, regs, sizeof(regs));
	if (err) {
		return -ENODEV;
	}

	regs[0] |= RV3028_ALARM_MINUTES_AE_M;
	regs[1] |= RV3028_ALARM_HOURS_AE_H;
	regs[2] |= RV3028_ALARM_DATE_AE_WD;

	err = rv3028_write_regs(dev, RV3028_REG_ALARM_MINUTES, regs, sizeof(regs));
	if (err) {
		return -ENODEV;
	}

	return 0;
}

static const struct rtc_driver_api rv3028_driver_api = {
	.set_time = rv3028_set_time,
	.get_time = rv3028_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rv3028_alarm_get_supported_fields,
	.alarm_set_time = rv3028_alarm_set_time,
	.alarm_get_time = rv3028_alarm_get_time,
	.alarm_is_pending = rv3028_alarm_is_pending,
	.alarm_set_callback = rv3028_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#if RV3028_INT_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE)
	.update_set_callback = rv3028_update_set_callback,
#endif /* RV3028_INT_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE) */
};

#define RV3028_BSM_FROM_DT_INST(inst)                                                              \
	UTIL_CAT(RV3028_BSM_, DT_INST_STRING_UPPER_TOKEN(inst, backup_switch_mode))

#define RV3028_BACKUP_FROM_DT_INST(inst)                                                           \
	((FIELD_PREP(RV3028_BACKUP_BSM, RV3028_BSM_FROM_DT_INST(inst))) |                          \
	 (FIELD_PREP(RV3028_BACKUP_TCR, DT_INST_ENUM_IDX_OR(inst, trickle_resistor_ohms, 0))) |    \
	 (DT_INST_NODE_HAS_PROP(inst, trickle_resistor_ohms) ? RV3028_BACKUP_TCE : 0))

#define RV3028_INIT(inst)                                                                          \
	static const struct rv3028_config rv3028_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.cof = DT_INST_ENUM_IDX_OR(inst, clkout_frequency, RV3028_CLKOUT_FD_LOW),          \
		.backup = RV3028_BACKUP_FROM_DT_INST(inst),                                        \
		IF_ENABLED(RV3028_INT_GPIOS_IN_USE,                                                \
			   (.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0})))};         \
                                                                                                   \
	static struct rv3028_data rv3028_data_##inst;                                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &rv3028_init, NULL, &rv3028_data_##inst,                       \
			      &rv3028_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,        \
			      &rv3028_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV3028_INIT)
