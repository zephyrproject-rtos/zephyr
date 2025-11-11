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
#include "rtc_utils.h"

LOG_MODULE_REGISTER(bq32002, CONFIG_RTC_LOG_LEVEL);

#define DT_DRV_COMPAT ti_bq32002

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "Texas Instruments BQ32002 RTC driver enabled without any devices"
#endif

/* Registers */
#define BQ32002_SECONDS_REG    0x00
#define BQ32002_MINUTES_REG    0x01
#define BQ32002_CENT_HOURS_REG 0x02
#define BQ32002_DAY_REG        0x03
#define BQ32002_DATE_REG       0x04
#define BQ32002_MONTH_REG      0x05
#define BQ32002_YEARS_REG      0x06
#define BQ32002_CAL_CFG1_REG   0x07
#define BQ32002_CFG2_REG       0x09
#define BQ32002_SF_KEY_1_REG   0x20
#define BQ32002_SF_KEY_2_REG   0x21
#define BQ32002_SFR_REG        0x22

/* Bitmasks */
#define BQ32002_SECONDS_MASK GENMASK(6, 0)
#define BQ32002_MINUTES_MASK GENMASK(6, 0)
#define BQ32002_HOURS_MASK   GENMASK(5, 0)
#define BQ32002_DAY_MASK     GENMASK(2, 0)
#define BQ32002_DATE_MASK    GENMASK(5, 0)
#define BQ32002_MONTH_MASK   GENMASK(4, 0)
#define BQ32002_YEAR_MASK    GENMASK(7, 0)
#define BQ32002_CAL_MASK     GENMASK(4, 0)

#define BQ32002_OSC_STOP_MASK  BIT(7)
#define BQ32002_OSC_FAIL_MASK  BIT(7)
#define BQ32002_CENT_EN_MASK   BIT(7)
#define BQ32002_CENT_MASK      BIT(6)
#define BQ32002_OUT_MASK       BIT(7)
#define BQ32002_FREQ_TEST_MASK BIT(6)
#define BQ32002_CAL_SIGN_MASK  BIT(5)
#define BQ32002_FTF_MASK       BIT(0)

/* Keys to unlock special function register */
#define BQ32002_SF_KEY_1 0x5E
#define BQ32002_SF_KEY_2 0xC7

/* BQ32002 counts weekdays from 1 to 7 */
#define BQ32002_DAY_OFFSET -1

/* BQ32002 counts months from 1 to 12 */
#define BQ32002_MONTH_OFFSET -1

/* Year 2000 value represented as tm_year value */
#define BQ32002_TM_YEAR_2000 (2000 - 1900)

/* Calibration constants, see BQ32002 datasheet, Table 12, p.16 */
#define BQ32002_CAL_PPB_PER_LSB_POS 2034 /* 1e9 / 491520 */
#define BQ32002_CAL_PPB_PER_LSB_NEG 4069 /* 1e9 / 245760 */
#define BQ32002_CAL_PPB_MIN         (-31 * BQ32002_CAL_PPB_PER_LSB_POS)
#define BQ32002_CAL_PPB_MAX         (31 * BQ32002_CAL_PPB_PER_LSB_NEG)

/* IRQ frequency property enum values */
#define BQ32002_IRQ_FREQ_ENUM_1HZ      0
#define BQ32002_IRQ_FREQ_ENUM_512HZ    1
#define BQ32002_IRQ_FREQ_ENUM_DISABLED 2

/* RTC time fields supported by BQ32002 */
#define BQ32002_RTC_TIME_MASK                                                                      \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

struct bq32002_config {
	struct i2c_dt_spec i2c;
	uint8_t irq_freq;
};

struct bq32002_data {
	struct k_sem lock;
};

static void bq32002_lock_sem(const struct device *dev)
{
	struct bq32002_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
}

static void bq32002_unlock_sem(const struct device *dev)
{
	struct bq32002_data *data = dev->data;

	k_sem_give(&data->lock);
}

static int bq32002_set_irq_frequency(const struct device *dev)
{
	const struct bq32002_config *config = dev->config;
	uint8_t sf_regs[3];
	uint8_t cfg1_val;
	uint8_t cfg2_val;
	int err;

	switch (config->irq_freq) {
	case BQ32002_IRQ_FREQ_ENUM_1HZ:
		cfg1_val = BQ32002_FREQ_TEST_MASK;
		cfg2_val = BQ32002_FTF_MASK;
		break;
	case BQ32002_IRQ_FREQ_ENUM_512HZ:
		cfg1_val = BQ32002_FREQ_TEST_MASK;
		cfg2_val = 0;
		break;
	default:
		cfg1_val = BQ32002_OUT_MASK;
		cfg2_val = 0;
		break;
	}

	err = i2c_reg_update_byte_dt(&config->i2c, BQ32002_CAL_CFG1_REG, BQ32002_FREQ_TEST_MASK,
				     cfg1_val);
	if (err) {
		return err;
	}

	/* Update FTF value if frequency output enabled */
	if (cfg1_val & BQ32002_FREQ_TEST_MASK) {
		sf_regs[0] = BQ32002_SF_KEY_1;
		sf_regs[1] = BQ32002_SF_KEY_2;
		sf_regs[2] = cfg2_val;
		err = i2c_burst_write_dt(&config->i2c, BQ32002_SF_KEY_1_REG, sf_regs,
					 sizeof(sf_regs));
		if (err) {
			return err;
		}
	}

	return 0;
}

static int bq32002_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct bq32002_config *config = dev->config;
	int err;
	uint8_t regs[7];

	if ((timeptr == NULL) || !rtc_utils_validate_rtc_time(timeptr, BQ32002_RTC_TIME_MASK)) {
		return -EINVAL;
	}

	bq32002_lock_sem(dev);

	/* Update the registers */
	regs[0] = bin2bcd(timeptr->tm_sec) & BQ32002_SECONDS_MASK;
	regs[1] = bin2bcd(timeptr->tm_min) & BQ32002_MINUTES_MASK; /* Clear oscillator fail flag */
	regs[2] = (bin2bcd(timeptr->tm_hour) & BQ32002_HOURS_MASK) | BQ32002_CENT_EN_MASK;
	regs[3] = bin2bcd(timeptr->tm_wday - BQ32002_DAY_OFFSET) & BQ32002_DAY_MASK;
	regs[4] = bin2bcd(timeptr->tm_mday) & BQ32002_DATE_MASK;
	regs[5] = bin2bcd(timeptr->tm_mon - BQ32002_MONTH_OFFSET) & BQ32002_MONTH_MASK;

	/* Determine which century we're in */
	if (timeptr->tm_year >= BQ32002_TM_YEAR_2000) {
		regs[2] |= BQ32002_CENT_MASK;
		regs[6] = bin2bcd(timeptr->tm_year - BQ32002_TM_YEAR_2000) & BQ32002_YEAR_MASK;
	} else {
		regs[6] = bin2bcd(timeptr->tm_year) & BQ32002_YEAR_MASK;
	}

	/* Write new time to the chip */
	err = i2c_burst_write_dt(&config->i2c, BQ32002_SECONDS_REG, regs, sizeof(regs));

	bq32002_unlock_sem(dev);

	if (!err) {
		LOG_DBG("Set time: year: %d, month: %d, month day: %d, week day: %d, hour: %d, "
			"minute: %d, second: %d",
			timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
			timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
	}

	return err;
}

static int bq32002_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct bq32002_config *config = dev->config;
	int err;
	uint8_t reg_val;
	uint8_t regs[7];

	if (timeptr == NULL) {
		return -EINVAL;
	}

	bq32002_lock_sem(dev);

	err = i2c_reg_read_byte_dt(&config->i2c, BQ32002_MINUTES_REG, &reg_val);
	if (err) {
		goto out_unlock;
	}

	/* Oscillator failure detected, data might be invalid */
	if (reg_val & BQ32002_OSC_FAIL_MASK) {
		err = -ENODATA;
		goto out_unlock;
	}

	err = i2c_burst_read_dt(&config->i2c, BQ32002_SECONDS_REG, regs, sizeof(regs));
	if (err) {
		goto out_unlock;
	}

	timeptr->tm_sec = bcd2bin(regs[0] & BQ32002_SECONDS_MASK);
	timeptr->tm_min = bcd2bin(regs[1] & BQ32002_MINUTES_MASK);
	timeptr->tm_hour = bcd2bin(regs[2] & BQ32002_HOURS_MASK);
	timeptr->tm_wday = bcd2bin(regs[3] & BQ32002_DAY_MASK) + BQ32002_DAY_OFFSET;
	timeptr->tm_mday = bcd2bin(regs[4] & BQ32002_DATE_MASK);
	timeptr->tm_mon = bcd2bin(regs[5] & BQ32002_MONTH_MASK) + BQ32002_MONTH_OFFSET;
	timeptr->tm_year = bcd2bin(regs[6] & BQ32002_YEAR_MASK);
	timeptr->tm_yday = -1;  /* Unsupported */
	timeptr->tm_isdst = -1; /* Unsupported */
	timeptr->tm_nsec = 0;   /* Unsupported */

	/* Apply century offset */
	if (regs[2] & BQ32002_CENT_MASK) {
		timeptr->tm_year += BQ32002_TM_YEAR_2000;
	}

out_unlock:
	bq32002_unlock_sem(dev);

	if (!err) {
		LOG_DBG("Read time: year: %d, month: %d, month day: %d, week day: %d, hour: %d, "
			"minute: "
			"%d, second: %d",
			timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
			timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
	}

	return err;
}

#ifdef CONFIG_RTC_CALIBRATION

static int bq32002_set_calibration(const struct device *dev, int32_t freq_ppb)
{
	const struct bq32002_config *config = dev->config;
	int err;
	uint8_t offset;
	uint8_t reg_val;

	if ((freq_ppb < BQ32002_CAL_PPB_MIN) || (freq_ppb > BQ32002_CAL_PPB_MAX)) {
		LOG_ERR("Calibration value %d ppb out of range", freq_ppb);
		return -EINVAL;
	}

	err = i2c_reg_read_byte_dt(&config->i2c, BQ32002_CAL_CFG1_REG, &reg_val);
	if (err) {
		return err;
	}

	reg_val &= ~(BQ32002_CAL_SIGN_MASK | BQ32002_CAL_MASK);

	if (freq_ppb > 0) {
		reg_val |= BQ32002_CAL_SIGN_MASK; /* Negative sign speeds the oscillator up */
		offset =
			DIV_ROUND_CLOSEST(freq_ppb, BQ32002_CAL_PPB_PER_LSB_NEG) & BQ32002_CAL_MASK;
	} else {
		offset = DIV_ROUND_CLOSEST(-freq_ppb, BQ32002_CAL_PPB_PER_LSB_POS) &
			 BQ32002_CAL_MASK;
	}
	reg_val |= offset;

	err = i2c_reg_write_byte_dt(&config->i2c, BQ32002_CAL_CFG1_REG, reg_val);
	if (err) {
		return err;
	}

	LOG_DBG("Set calibration: frequency ppb: %d, offset value: %d, sign: %d", freq_ppb, offset,
		freq_ppb > 0);

	return 0;
}

static int bq32002_get_calibration(const struct device *dev, int32_t *freq_ppb)
{
	const struct bq32002_config *config = dev->config;
	uint8_t reg_val;
	uint8_t offset;
	int err;

	err = i2c_reg_read_byte_dt(&config->i2c, BQ32002_CAL_CFG1_REG, &reg_val);
	if (err) {
		return err;
	}

	offset = reg_val & BQ32002_CAL_MASK;

	if (reg_val & BQ32002_CAL_SIGN_MASK) {
		*freq_ppb = offset * BQ32002_CAL_PPB_PER_LSB_NEG;
	} else {
		*freq_ppb = -offset * BQ32002_CAL_PPB_PER_LSB_POS;
	}

	LOG_DBG("Get calibration: frequency ppb: %d, offset value: %d, sign: %d", *freq_ppb, offset,
		*freq_ppb > 0);

	return 0;
}

#endif

static DEVICE_API(rtc, bq32002_driver_api) = {
	.set_time = bq32002_set_time,
	.get_time = bq32002_get_time,
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = bq32002_set_calibration,
	.get_calibration = bq32002_get_calibration
#endif
};

static int bq32002_init(const struct device *dev)
{
	const struct bq32002_config *config = dev->config;
	struct bq32002_data *data = dev->data;
	int err;

	(void)k_sem_init(&data->lock, 1, 1);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Start the oscillator */
	err = i2c_reg_update_byte_dt(&config->i2c, BQ32002_SECONDS_REG, BQ32002_OSC_STOP_MASK, 0);
	if (err) {
		return err;
	}

	/* Configure IRQ output frequency */
	err = bq32002_set_irq_frequency(dev);
	if (err) {
		return err;
	}

	return 0;
}

#define BQ32002_INIT(inst)                                                                         \
	static struct bq32002_data bq32002_data_##inst;                                            \
	static const struct bq32002_config bq32002_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.irq_freq =                                                                        \
			DT_INST_ENUM_IDX_OR(inst, irq_frequency, BQ32002_IRQ_FREQ_ENUM_DISABLED)   \
		};										   \
	DEVICE_DT_INST_DEFINE(inst, &bq32002_init, NULL, &bq32002_data_##inst,                     \
			      &bq32002_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,       \
			      &bq32002_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ32002_INIT)
