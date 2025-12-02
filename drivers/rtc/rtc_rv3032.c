/*
 * Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv3032

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include "rtc_utils.h"

LOG_MODULE_REGISTER(rv3032, CONFIG_RTC_LOG_LEVEL);

/* RV3032 RAM register addresses */
#define RV3032_REG_100TH_SECONDS 0x00
#define RV3032_REG_SECONDS       0x01
#define RV3032_REG_MINUTES       0x02
#define RV3032_REG_HOURS         0x03
#define RV3032_REG_WEEKDAY       0x04
#define RV3032_REG_DATE          0x05
#define RV3032_REG_MONTH         0x06
#define RV3032_REG_YEAR          0x07
#define RV3032_REG_ALARM_MINUTES 0x08
#define RV3032_REG_ALARM_HOURS   0x09
#define RV3032_REG_ALARM_DATE    0x0A
#define RV3032_REG_TIMER_VALUE_0 0x0B
#define RV3032_REG_TIMER_VALUE_1 0x0C
#define RV3032_REG_STATUS        0x0D
#define RV3032_REG_TEMPERATURE   0x0E
#define RV3032_REG_CONTROL1      0x10
#define RV3032_REG_CONTROL2      0x11

#define RV3032_REG_EEPROM_ADDRESS 0x3D
#define RV3032_REG_EEPROM_DATA    0x3E
#define RV3032_REG_EEPROM_COMMAND 0x3F
#define RV3032_REG_EEPROM_PMU     0xC0

#define RV3032_CONTROL1_TD   GENMASK(1, 0)
#define RV3032_CONTROL1_EERD BIT(2)
#define RV3032_CONTROL1_TE   BIT(3)
#define RV3032_CONTROL1_USEL BIT(4)
#define RV3032_CONTROL1_GP0  BIT(5)

#define RV3032_CONTROL2_STOP  BIT(0)
#define RV3032_CONTROL2_GP1   BIT(1)
#define RV3032_CONTROL2_EIE   BIT(2)
#define RV3032_CONTROL2_AIE   BIT(3)
#define RV3032_CONTROL2_TIE   BIT(4)
#define RV3032_CONTROL2_UIE   BIT(5)
#define RV3032_CONTROL2_CLKIE BIT(6)

#define RV3032_STATUS_VLF  BIT(0)
#define RV3032_STATUS_PORF BIT(1)
#define RV3032_STATUS_EVF  BIT(2)
#define RV3032_STATUS_AF   BIT(3)
#define RV3032_STATUS_TF   BIT(4)
#define RV3032_STATUS_UF   BIT(5)
#define RV3032_STATUS_TLF  BIT(6)
#define RV3032_STATUS_THF  BIT(7)

#define RV3032_TEMPERATURE_BSF      BIT(0)
#define RV3032_TEMPERATURE_CLKF     BIT(1)
#define RV3032_TEMPERATURE_EEBUSY   BIT(2)
#define RV3032_TEMPERATURE_EEF      BIT(3)
#define RV3032_TEMPERATURE_TEMP_LSB GENMASK(7, 4)

#define RV3032_EEPROM_PMU_NCLKE BIT(6)
#define RV3032_EEPROM_PMU_BSM   GENMASK(5, 4)
#define RV3032_EEPROM_PMU_TCR   GENMASK(3, 2)
#define RV3032_EEPROM_PMU_TCM   GENMASK(1, 0)

#define RV3032_REG_EEPROM_CLKOUT1 0xC2
#define RV3032_REG_EEPROM_CLKOUT2 0xC3

#define RV3032_EEPROM_CLKOUT1_HFD_LOW GENMASK(7, 0)

#define RV3032_EEPROM_CLKOUT2_OS       BIT(7)
#define RV3032_EEPROM_CLKOUT2_FD       GENMASK(6, 5)
#define RV3032_EEPROM_CLKOUT2_HFD_HIGH GENMASK(4, 0)

#define RV3032_EEPROM_CLKOUT2_OS_XTAL 0x0
#define RV3032_EEPROM_CLKOUT2_OS_HF   0x1

#define RV3032_EEPROM_CLKOUT2_FD_32768HZ 0x0
#define RV3032_EEPROM_CLKOUT2_FD_1024HZ  0x1
#define RV3032_EEPROM_CLKOUT2_FD_64HZ    0x2
#define RV3032_EEPROM_CLKOUT2_FD_1HZ     0x3

#define RV3032_BSM_DISABLED 0x0
#define RV3032_BSM_DIRECT   0x1
#define RV3032_BSM_LEVEL    0x2

#define RV3032_TCM_DISABLED 0x0
#define RV3032_TCM_1750MV   0x1
#define RV3032_TCM_3000MV   0x2
#define RV3032_TCM_4500MV   0x3

#define RV3032_TCR_600_OHM   0x0
#define RV3032_TCR_2000_OHM  0x1
#define RV3032_TCR_7000_OHM  0x2
#define RV3032_TCR_12000_OHM 0x3

/* CLKOUT frequency constants */
#define RV3032_CLKOUT_FREQ_1HZ     1U
#define RV3032_CLKOUT_FREQ_64HZ    64U
#define RV3032_CLKOUT_FREQ_1024HZ  1024U
#define RV3032_CLKOUT_FREQ_32768HZ 32768U
#define RV3032_CLKOUT_FREQ_HF_MIN  8192U
#define RV3032_CLKOUT_FREQ_HF_MAX  67108864U
#define RV3032_CLKOUT_FREQ_HF_STEP 8192U

#define RV3032_EEPROM_CMD_INIT    0x00
#define RV3032_EEPROM_CMD_UPDATE  0x11
#define RV3032_EEPROM_CMD_REFRESH 0x12
#define RV3032_EEPROM_CMD_WRITE   0x21
#define RV3032_EEPROM_CMD_READ    0x22

#define RV3032_100TH_SECONDS_MASK GENMASK(7, 0)
#define RV3032_SECONDS_MASK       GENMASK(6, 0)
#define RV3032_MINUTES_MASK       GENMASK(6, 0)
#define RV3032_HOURS_AMPM         BIT(5)
#define RV3032_HOURS_12H_MASK     GENMASK(4, 0)
#define RV3032_HOURS_24H_MASK     GENMASK(5, 0)
#define RV3032_DATE_MASK          GENMASK(5, 0)
#define RV3032_WEEKDAY_MASK       GENMASK(2, 0)
#define RV3032_MONTH_MASK         GENMASK(4, 0)
#define RV3032_YEAR_MASK          GENMASK(7, 0)

#define RV3032_ALARM_MINUTES_AE_M   BIT(7)
#define RV3032_ALARM_MINUTES_MASK   GENMASK(6, 0)
#define RV3032_ALARM_HOURS_AE_H     BIT(7)
#define RV3032_ALARM_HOURS_AMPM     BIT(5)
#define RV3032_ALARM_HOURS_12H_MASK GENMASK(4, 0)
#define RV3032_ALARM_HOURS_24H_MASK GENMASK(5, 0)
#define RV3032_ALARM_DATE_AE_D      BIT(7)
#define RV3032_ALARM_DATE_MASK      GENMASK(5, 0)

/* The RV3032 only supports two-digit years. Leap years are correctly handled from 2000 to 2099 */
#define RV3032_YEAR_OFFSET (2000 - 1900)

/* The RV3032 enumerates months 1 to 12 */
#define RV3032_MONTH_OFFSET 1

/* RV3032 EEPROM timing from datasheet */
#define RV3032_EEBUSY_READ_POLL_MS  2   /* tREAD = ~1.1ms, poll every 2ms */
#define RV3032_EEBUSY_WRITE_POLL_MS 5   /* tWRITE = ~4.8ms, poll every 5ms */
#define RV3032_EEBUSY_TIMEOUT_MS    100 /* Max wait for any EEPROM operation */

/* Recommended pre-refresh time before reading the time registers */
#define RV3032_POR_REFRESH_TIME_MS 66 /* tPREFR = ~66ms */

/* Number of nanoseconds per 1/100th of a second */
#define RV3032_NSEC_PER_100TH_SECOND 10000000L

/* RTC alarm time fields supported by the RV3032 */
#define RV3032_RTC_ALARM_TIME_MASK                                                                 \
	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY)

/* RTC time fields supported by the RV3032 */
#define RV3032_RTC_TIME_MASK                                                                       \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

/* Helper macro to guard int-gpios related code */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) &&                                                 \
	(defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE))
#define RV3032_INT_GPIOS_IN_USE 1
#endif

struct rv3032_config {
	const struct i2c_dt_spec i2c;
#ifdef RV3032_INT_GPIOS_IN_USE
	struct gpio_dt_spec gpio_int;
#endif /* RV3032_INT_GPIOS_IN_USE */
	uint8_t backup;
	uint32_t clkout_freq;
};

struct rv3032_data {
	struct k_sem lock;
#if RV3032_INT_GPIOS_IN_USE
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
#endif /* RV3032_INT_GPIOS_IN_USE */
};

static void rv3032_lock_sem(const struct device *dev)
{
	struct rv3032_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
}

static void rv3032_unlock_sem(const struct device *dev)
{
	struct rv3032_data *data = dev->data;

	k_sem_give(&data->lock);
}

static int rv3032_read_regs(const struct device *dev, uint8_t addr, void *buf, size_t len)
{
	const struct rv3032_config *config = dev->config;
	int err;

	err = i2c_write_read_dt(&config->i2c, &addr, sizeof(addr), buf, len);
	if (err) {
		LOG_ERR("failed to read reg addr 0x%02x, len %d (err %d)", addr, len, err);
		return err;
	}

	return 0;
}

static int rv3032_read_reg8(const struct device *dev, uint8_t addr, uint8_t *val)
{
	return rv3032_read_regs(dev, addr, val, sizeof(*val));
}

static int rv3032_write_regs(const struct device *dev, uint8_t addr, void *buf, size_t len)
{
	const struct rv3032_config *config = dev->config;
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

static int rv3032_write_reg8(const struct device *dev, uint8_t addr, uint8_t val)
{
	return rv3032_write_regs(dev, addr, &val, sizeof(val));
}

static int rv3032_update_reg8(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	const struct rv3032_config *config = dev->config;
	int err;

	err = i2c_reg_update_byte_dt(&config->i2c, addr, mask, val);
	if (err) {
		LOG_ERR("failed to update reg addr 0x%02x, mask 0x%02x, val 0x%02x (err %d)", addr,
			mask, val, err);
		return err;
	}

	return 0;
}

static int rv3032_eeprom_wait_busy(const struct device *dev, int poll_ms)
{
	uint8_t status = 0;
	int err;
	int64_t timeout_time = k_uptime_get() + RV3032_EEBUSY_TIMEOUT_MS;

	/* Wait while the EEPROM is busy */
	for (;;) {
		err = rv3032_read_reg8(dev, RV3032_REG_TEMPERATURE, &status);
		if (err) {
			return err;
		}

		if (!(status & RV3032_TEMPERATURE_EEBUSY)) {
			break;
		}

		if (k_uptime_get() > timeout_time) {
			return -ETIME;
		}

		k_msleep(poll_ms);
	}

	return 0;
}

static int rv3032_exit_eerd(const struct device *dev)
{
	return rv3032_update_reg8(dev, RV3032_REG_CONTROL1, RV3032_CONTROL1_EERD, 0);
}

static int rv3032_enter_eerd(const struct device *dev)
{
	uint8_t ctrl1;
	bool eerd;
	int ret;

	ret = rv3032_read_reg8(dev, RV3032_REG_CONTROL1, &ctrl1);
	if (ret) {
		return ret;
	}

	eerd = ctrl1 & RV3032_CONTROL1_EERD;
	if (eerd) {
		return 0;
	}

	ret = rv3032_update_reg8(dev, RV3032_REG_CONTROL1, RV3032_CONTROL1_EERD,
				 RV3032_CONTROL1_EERD);

	if (ret) {
		return ret;
	}

	ret = rv3032_eeprom_wait_busy(dev, RV3032_EEBUSY_WRITE_POLL_MS);
	if (ret) {
		rv3032_exit_eerd(dev);
		return ret;
	}

	return ret;
}

static int rv3032_eeprom_command(const struct device *dev, uint8_t command)
{
	int err;

	err = rv3032_write_reg8(dev, RV3032_REG_EEPROM_COMMAND, RV3032_EEPROM_CMD_INIT);
	if (err) {
		return err;
	}

	return rv3032_write_reg8(dev, RV3032_REG_EEPROM_COMMAND, command);
}

static int rv3032_update(const struct device *dev)
{
	int err;

	err = rv3032_eeprom_command(dev, RV3032_EEPROM_CMD_UPDATE);
	if (err) {
		goto exit_eerd;
	}

	err = rv3032_eeprom_wait_busy(dev, RV3032_EEBUSY_WRITE_POLL_MS);

exit_eerd:
	rv3032_exit_eerd(dev);

	return err;
}

static int rv3032_refresh(const struct device *dev)
{
	int err;

	err = rv3032_eeprom_command(dev, RV3032_EEPROM_CMD_REFRESH);
	if (err) {
		goto exit_eerd;
	}

	err = rv3032_eeprom_wait_busy(dev, RV3032_EEBUSY_READ_POLL_MS);

exit_eerd:
	rv3032_exit_eerd(dev);

	return err;
}

static int rv3032_update_cfg(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	uint8_t val_old;
	uint8_t val_new;
	int err;

	err = rv3032_read_reg8(dev, addr, &val_old);
	if (err) {
		return err;
	}

	val_new = (val_old & ~mask) | (val & mask);
	if (val_new == val_old) {
		return 0;
	}

	err = rv3032_enter_eerd(dev);
	if (err) {
		return err;
	}

	err = rv3032_write_reg8(dev, addr, val_new);
	if (err) {
		rv3032_exit_eerd(dev);
		return err;
	}

	return rv3032_update(dev);
}

static int rv3032_configure_clkout(const struct device *dev, uint32_t freq)
{
	uint8_t clkout1_reg = 0;
	uint8_t clkout2_reg = 0;
	uint8_t pmu_reg = 0;
	uint16_t hfd;
	int err;

	if (freq == 0) {
		/* Disable CLKOUT: NCLKE = 1 for minimum power consumption */
		pmu_reg = RV3032_EEPROM_PMU_NCLKE;
		/* Registers can be left as 0 (XTAL mode, 32768 Hz, HFD = 0) */
		clkout1_reg = 0;
		clkout2_reg = 0; /* OS = 0 (XTAL), FD = 00 (32768Hz), HFD[12:8] = 0 */
	} else {
		/* Enable CLKOUT: NCLKE = 0 */
		pmu_reg = 0;

		switch (freq) {
		case RV3032_CLKOUT_FREQ_1HZ:
			clkout1_reg = 0; /* HFD[7:0] not used in XTAL mode */
			clkout2_reg =
				FIELD_PREP(RV3032_EEPROM_CLKOUT2_OS,
					   RV3032_EEPROM_CLKOUT2_OS_XTAL) |
				FIELD_PREP(RV3032_EEPROM_CLKOUT2_FD, RV3032_EEPROM_CLKOUT2_FD_1HZ);
			break;
		case RV3032_CLKOUT_FREQ_64HZ:
			clkout1_reg = 0; /* HFD[7:0] not used in XTAL mode */
			clkout2_reg =
				FIELD_PREP(RV3032_EEPROM_CLKOUT2_OS,
					   RV3032_EEPROM_CLKOUT2_OS_XTAL) |
				FIELD_PREP(RV3032_EEPROM_CLKOUT2_FD, RV3032_EEPROM_CLKOUT2_FD_64HZ);
			break;
		case RV3032_CLKOUT_FREQ_1024HZ:
			clkout1_reg = 0; /* HFD[7:0] not used in XTAL mode */
			clkout2_reg = FIELD_PREP(RV3032_EEPROM_CLKOUT2_OS,
						 RV3032_EEPROM_CLKOUT2_OS_XTAL) |
				      FIELD_PREP(RV3032_EEPROM_CLKOUT2_FD,
						 RV3032_EEPROM_CLKOUT2_FD_1024HZ);
			break;
		case RV3032_CLKOUT_FREQ_32768HZ:
			clkout1_reg = 0; /* HFD[7:0] not used in XTAL mode */
			clkout2_reg = FIELD_PREP(RV3032_EEPROM_CLKOUT2_OS,
						 RV3032_EEPROM_CLKOUT2_OS_XTAL) |
				      FIELD_PREP(RV3032_EEPROM_CLKOUT2_FD,
						 RV3032_EEPROM_CLKOUT2_FD_32768HZ);
			break;
		default:
			hfd = (freq / RV3032_CLKOUT_FREQ_HF_STEP) - 1;

			clkout1_reg = FIELD_GET(RV3032_EEPROM_CLKOUT1_HFD_LOW, hfd); /* HFD[7:0] */
			clkout2_reg =
				FIELD_PREP(RV3032_EEPROM_CLKOUT2_OS, RV3032_EEPROM_CLKOUT2_OS_HF) |
				FIELD_PREP(RV3032_EEPROM_CLKOUT2_HFD_HIGH,
					   (hfd >> 8)); /* HFD[12:8] */
			break;
		}
	}

	/* Configure PMU register NCLKE bit */
	err = rv3032_update_cfg(dev, RV3032_REG_EEPROM_PMU, RV3032_EEPROM_PMU_NCLKE, pmu_reg);
	if (err) {
		LOG_ERR("Failed to configure PMU NCLKE: %d", err);
		return err;
	}

	/* Configure CLKOUT registers - write C2h and C3h separately */
	err = rv3032_enter_eerd(dev);
	if (err) {
		return err;
	}

	/* Write EEPROM Clkout 1 (C2h) */
	err = rv3032_write_reg8(dev, RV3032_REG_EEPROM_CLKOUT1, clkout1_reg);
	if (err) {
		rv3032_exit_eerd(dev);
		LOG_ERR("Failed to configure CLKOUT1 register: %d", err);
		return err;
	}

	/* Write EEPROM Clkout 2 (C3h) */
	err = rv3032_write_reg8(dev, RV3032_REG_EEPROM_CLKOUT2, clkout2_reg);
	if (err) {
		rv3032_exit_eerd(dev);
		LOG_ERR("Failed to configure CLKOUT2 register: %d", err);
		return err;
	}

	err = rv3032_update(dev);
	if (err) {
		LOG_ERR("Failed to update CLKOUT configuration: %d", err);
		return err;
	}

	if (freq == 0) {
		LOG_DBG("CLKOUT disabled for power saving");
	} else {
		LOG_DBG("CLKOUT configured for %u Hz (C2h=0x%02x, C3h=0x%02x)", freq, clkout1_reg,
			clkout2_reg);
	}

	return 0;
}

#if RV3032_INT_GPIOS_IN_USE

static void rv3032_work_cb(struct k_work *work)
{
	struct rv3032_data *data = CONTAINER_OF(work, struct rv3032_data, work);
	const struct device *dev = data->dev;
	rtc_alarm_callback alarm_callback = NULL;
	void *alarm_user_data = NULL;
	rtc_update_callback update_callback = NULL;
	void *update_user_data = NULL;
	uint8_t status;
	int err;

	rv3032_lock_sem(dev);

	err = rv3032_read_reg8(data->dev, RV3032_REG_STATUS, &status);
	if (err) {
		goto unlock;
	}

#ifdef CONFIG_RTC_ALARM
	if ((status & RV3032_STATUS_AF) && data->alarm_callback) {
		status &= ~(RV3032_STATUS_AF);
		alarm_callback = data->alarm_callback;
		alarm_user_data = data->alarm_user_data;
	}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	if ((status & RV3032_STATUS_UF) && data->update_callback) {
		status &= ~(RV3032_STATUS_UF);
		update_callback = data->update_callback;
		update_user_data = data->update_user_data;
	}
#endif /* CONFIG_RTC_UPDATE */

	err = rv3032_write_reg8(dev, RV3032_REG_STATUS, status);
	if (err) {
		goto unlock;
	}

	/* Check if interrupt occurred between STATUS read/write */
	err = rv3032_read_reg8(dev, RV3032_REG_STATUS, &status);
	if (err) {
		goto unlock;
	}

	if (((status & RV3032_STATUS_AF) && alarm_callback) ||
	    ((status & RV3032_STATUS_UF) && update_callback)) {
		/* Another interrupt occurred while servicing this one */
		k_work_submit(&data->work);
	}

unlock:
	rv3032_unlock_sem(dev);

	if (alarm_callback) {
		alarm_callback(dev, 0U, alarm_user_data);
	}

	if (update_callback) {
		update_callback(dev, update_user_data);
	}
}

static void rv3032_int_handler(const struct device *port, struct gpio_callback *cb,
			       gpio_port_pins_t pins)
{
	struct rv3032_data *data = CONTAINER_OF(cb, struct rv3032_data, int_callback);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}

#endif /* RV3032_INT_GPIOS_IN_USE */

static int rv3032_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	uint8_t date[7];
	int err;

	if (!timeptr || !rtc_utils_validate_rtc_time(timeptr, RV3032_RTC_TIME_MASK) ||
	    (timeptr->tm_year < RV3032_YEAR_OFFSET)) {
		LOG_ERR("invalid time");
		return -EINVAL;
	}

	rv3032_lock_sem(dev);

	LOG_DBG("set time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d, centisec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec,
		(int)(timeptr->tm_nsec / RV3032_NSEC_PER_100TH_SECOND));

	date[0] = bin2bcd(timeptr->tm_sec) & RV3032_SECONDS_MASK;
	date[1] = bin2bcd(timeptr->tm_min) & RV3032_MINUTES_MASK;
	date[2] = bin2bcd(timeptr->tm_hour) & RV3032_HOURS_24H_MASK;
	date[3] = timeptr->tm_wday & RV3032_WEEKDAY_MASK;
	date[4] = bin2bcd(timeptr->tm_mday) & RV3032_DATE_MASK;
	date[5] = bin2bcd(timeptr->tm_mon + RV3032_MONTH_OFFSET) & RV3032_MONTH_MASK;
	date[6] = bin2bcd(timeptr->tm_year - RV3032_YEAR_OFFSET) & RV3032_YEAR_MASK;

	/* Write seconds through year registers. Note: Writing to seconds register automatically
	 * clears the 100th seconds register to 00h per datasheet
	 */
	err = rv3032_write_regs(dev, RV3032_REG_SECONDS, &date, sizeof(date));
	if (err) {
		goto unlock;
	}

	/* Clear Power On Reset Flag */
	err = rv3032_update_reg8(dev, RV3032_REG_STATUS, RV3032_STATUS_PORF, 0);

unlock:
	rv3032_unlock_sem(dev);

	return err;
}

static int rv3032_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	uint8_t status;
	uint8_t date[8];
	int err;

	if (!timeptr) {
		return -EINVAL;
	}

	err = rv3032_read_reg8(dev, RV3032_REG_STATUS, &status);
	if (err) {
		return err;
	}

	if (status & RV3032_STATUS_PORF) {
		/* Power On Reset Flag indicates invalid data */
		return -ENODATA;
	}

	/* Read 100th seconds through year registers */
	err = rv3032_read_regs(dev, RV3032_REG_100TH_SECONDS, date, ARRAY_SIZE(date));
	if (err) {
		return err;
	}

	memset(timeptr, 0U, sizeof(*timeptr));
	timeptr->tm_nsec =
		bcd2bin(date[0] & RV3032_100TH_SECONDS_MASK) * RV3032_NSEC_PER_100TH_SECOND;
	timeptr->tm_sec = bcd2bin(date[1] & RV3032_SECONDS_MASK);
	timeptr->tm_min = bcd2bin(date[2] & RV3032_MINUTES_MASK);
	timeptr->tm_hour = bcd2bin(date[3] & RV3032_HOURS_24H_MASK);
	timeptr->tm_wday = date[4] & RV3032_WEEKDAY_MASK;
	timeptr->tm_mday = bcd2bin(date[5] & RV3032_DATE_MASK);
	timeptr->tm_mon = bcd2bin(date[6] & RV3032_MONTH_MASK) - RV3032_MONTH_OFFSET;
	timeptr->tm_year = bcd2bin(date[7] & RV3032_YEAR_MASK) + RV3032_YEAR_OFFSET;
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;

	LOG_DBG("get time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d, centisec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec,
		(int)(timeptr->tm_nsec / 10000000));

	return 0;
}

#ifdef CONFIG_RTC_ALARM

static int rv3032_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	*mask = RV3032_RTC_ALARM_TIME_MASK;

	return 0;
}

static int rv3032_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				 const struct rtc_time *timeptr)
{
	uint8_t regs[3];

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	if (mask & ~(RV3032_RTC_ALARM_TIME_MASK)) {
		LOG_ERR("unsupported alarm field mask 0x%04x", mask);
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("invalid alarm time");
		return -EINVAL;
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		regs[0] = bin2bcd(timeptr->tm_min) & RV3032_ALARM_MINUTES_MASK;
	} else {
		regs[0] = RV3032_ALARM_MINUTES_AE_M;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		regs[1] = bin2bcd(timeptr->tm_hour) & RV3032_ALARM_HOURS_24H_MASK;
	} else {
		regs[1] = RV3032_ALARM_HOURS_AE_H;
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		regs[2] = bin2bcd(timeptr->tm_mday) & RV3032_ALARM_DATE_MASK;
	} else {
		regs[2] = RV3032_ALARM_DATE_AE_D;
	}

	LOG_DBG("set alarm: mday = %d, hour = %d, min = %d, mask = 0x%04x", timeptr->tm_mday,
		timeptr->tm_hour, timeptr->tm_min, mask);

	/* Write registers RV3032_REG_ALARM_MINUTES through RV3032_REG_ALARM_DATE */
	return rv3032_write_regs(dev, RV3032_REG_ALARM_MINUTES, &regs, ARRAY_SIZE(regs));
}

static int rv3032_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				 struct rtc_time *timeptr)
{
	uint8_t regs[3];
	int err;

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	/* Read registers RV3032_REG_ALARM_MINUTES through RV3032_REG_ALARM_WEEKDAY */
	err = rv3032_read_regs(dev, RV3032_REG_ALARM_MINUTES, &regs, ARRAY_SIZE(regs));
	if (err) {
		return err;
	}

	memset(timeptr, 0U, sizeof(*timeptr));
	*mask = 0U;

	if ((regs[0] & RV3032_ALARM_MINUTES_AE_M) == 0) {
		timeptr->tm_min = bcd2bin(regs[0] & RV3032_ALARM_MINUTES_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if ((regs[1] & RV3032_ALARM_HOURS_AE_H) == 0) {
		timeptr->tm_hour = bcd2bin(regs[1] & RV3032_ALARM_HOURS_24H_MASK);
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if ((regs[2] & RV3032_ALARM_DATE_AE_D) == 0) {
		timeptr->tm_mday = bcd2bin(regs[2] & RV3032_ALARM_DATE_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	LOG_DBG("get alarm: mday = %d, hour = %d, min = %d, mask = 0x%04x", timeptr->tm_mday,
		timeptr->tm_hour, timeptr->tm_min, *mask);

	return 0;
}

static int rv3032_alarm_is_pending(const struct device *dev, uint16_t id)
{
	uint8_t status;
	int err;

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	rv3032_lock_sem(dev);

	err = rv3032_read_reg8(dev, RV3032_REG_STATUS, &status);
	if (err) {
		goto unlock;
	}

	if (status & RV3032_STATUS_AF) {
		/* Clear alarm flag */
		status &= ~(RV3032_STATUS_AF);

		err = rv3032_write_reg8(dev, RV3032_REG_STATUS, status);
		if (err) {
			goto unlock;
		}

		/* Alarm pending */
		err = 1;
	}

unlock:
	rv3032_unlock_sem(dev);

	return err;
}

static int rv3032_alarm_set_callback(const struct device *dev, uint16_t id,
				     rtc_alarm_callback callback, void *user_data)
{
#ifndef RV3032_INT_GPIOS_IN_USE
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
#else
	const struct rv3032_config *config = dev->config;
	struct rv3032_data *data = dev->data;
	int err;

	if (!config->gpio_int.port) {
		return -ENOTSUP;
	}

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	rv3032_lock_sem(dev);

	data->alarm_callback = callback;
	data->alarm_user_data = user_data;

	err = rv3032_update_reg8(dev, RV3032_REG_CONTROL2, RV3032_CONTROL2_AIE,
				 callback ? RV3032_CONTROL2_AIE : 0);
	if (err) {
		goto unlock;
	}

unlock:
	rv3032_unlock_sem(dev);

	/* Alarm flag may already be set */
	k_work_submit(&data->work);

	return err;
#endif /* RV3032_INT_GPIOS_IN_USE */
}

#endif /* CONFIG_RTC_ALARM */

#if RV3032_INT_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE)

static int rv3032_update_set_callback(const struct device *dev, rtc_update_callback callback,
				      void *user_data)
{
	const struct rv3032_config *config = dev->config;
	struct rv3032_data *data = dev->data;
	int err;

	if (!config->gpio_int.port) {
		return -ENOTSUP;
	}

	rv3032_lock_sem(dev);

	data->update_callback = callback;
	data->update_user_data = user_data;

	err = rv3032_update_reg8(dev, RV3032_REG_CONTROL2, RV3032_CONTROL2_UIE,
				 callback ? RV3032_CONTROL2_UIE : 0);
	if (err) {
		goto unlock;
	}

unlock:
	rv3032_unlock_sem(dev);

	/* Seconds flag may already be set */
	k_work_submit(&data->work);

	return err;
}

#endif /* RV3032_INT_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE) */

static int rv3032_init(const struct device *dev)
{
	const struct rv3032_config *config = dev->config;
	struct rv3032_data *data = dev->data;
	uint8_t regs[3];
	uint8_t val;
	int err;
	uint8_t status;

	k_sem_init(&data->lock, 1, 1);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

#if RV3032_INT_GPIOS_IN_USE
	if (config->gpio_int.port) {
		if (!gpio_is_ready_dt(&config->gpio_int)) {
			LOG_ERR("GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);
		if (err) {
			LOG_ERR("failed to configure GPIO (err %d)", err);
			return -ENODEV;
		}

		err = gpio_pin_interrupt_configure_dt(&config->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("failed to enable GPIO interrupt (err %d)", err);
			return err;
		}

		gpio_init_callback(&data->int_callback, rv3032_int_handler,
				   BIT(config->gpio_int.pin));

		err = gpio_add_callback_dt(&config->gpio_int, &data->int_callback);
		if (err) {
			LOG_ERR("failed to add GPIO callback (err %d)", err);
			return -ENODEV;
		}

		data->dev = dev;
		data->work.handler = rv3032_work_cb;
	}
#endif /* RV3032_INT_GPIOS_IN_USE */

	/* Wait for RV3032 EEPROM refresh to complete after cold boot */
	/* According to datasheet: tPREFR = ~66ms for automatic EEPROM refresh at POR */
	/* During this time, all I2C communication will fail, so we must wait */
	int64_t remaining_time_ms = RV3032_POR_REFRESH_TIME_MS - k_uptime_get();

	if (remaining_time_ms > 0) {
		k_sleep(K_MSEC(remaining_time_ms));
	}

	/* Now read status register */
	err = rv3032_read_reg8(dev, RV3032_REG_STATUS, &val);
	if (err) {
		LOG_ERR("Status register read failed after EEPROM refresh: %d", err);
		return err; /* Return actual I2C error code */
	}

	if (val & RV3032_STATUS_AF) {
		LOG_WRN("an alarm may have been missed");
	}

	/* Refresh the settings in the RAM with the settings from the EEPROM */
	err = rv3032_enter_eerd(dev);
	if (err) {
		LOG_ERR("Failed to enter EERD mode: %d", err);
		return err;
	}
	err = rv3032_refresh(dev);
	if (err) {
		LOG_ERR("Failed to refresh EEPROM settings: %d", err);
		return err;
	}

	/* Configure the EEPROM PMU register */
	err = rv3032_update_cfg(dev, RV3032_REG_EEPROM_PMU,
				RV3032_EEPROM_PMU_TCR | RV3032_EEPROM_PMU_TCM |
					RV3032_EEPROM_PMU_BSM,
				config->backup);
	if (err) {
		LOG_ERR("Failed to configure PMU register: %d", err);
		return err;
	}

	/* Configure CLKOUT frequency */
	err = rv3032_configure_clkout(dev, config->clkout_freq);
	if (err) {
		LOG_ERR("Failed to configure CLKOUT: %d", err);
		return err;
	}

	err = rv3032_read_reg8(dev, RV3032_REG_STATUS, &status);
	if (err) {
		return -ENODEV;
	}
	if (status & RV3032_STATUS_PORF) {
		/* Disable the alarms */
		err = rv3032_update_reg8(dev, RV3032_REG_CONTROL2,
					 RV3032_CONTROL2_AIE | RV3032_CONTROL2_UIE, 0);
		if (err) {
			return -ENODEV;
		}
		err = rv3032_update_reg8(dev, RV3032_REG_STATUS, RV3032_STATUS_PORF, 0);
		if (err) {
			return -ENODEV;
		}
	}

	err = rv3032_read_regs(dev, RV3032_REG_ALARM_MINUTES, regs, sizeof(regs));
	if (err) {
		return -ENODEV;
	}

	LOG_DBG("%s: RV3032 RTC driver initialized", dev->name);

	return 0;
}

static DEVICE_API(rtc, rv3032_driver_api) = {
	.set_time = rv3032_set_time,
	.get_time = rv3032_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rv3032_alarm_get_supported_fields,
	.alarm_set_time = rv3032_alarm_set_time,
	.alarm_get_time = rv3032_alarm_get_time,
	.alarm_is_pending = rv3032_alarm_is_pending,
	.alarm_set_callback = rv3032_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#if RV3032_INT_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE)
	.update_set_callback = rv3032_update_set_callback,
#endif /* RV3032_INT_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE) */
};

#define RV3032_BSM_FROM_DT_INST(inst)                                                              \
	UTIL_CAT(RV3032_BSM_, DT_INST_STRING_UPPER_TOKEN(inst, backup_switch_mode))

#define RV3032_TCM_FROM_DT_INST(inst)                                                              \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, trickle_charger_mode),                         \
		    (DT_INST_PROP(inst, trickle_charger_mode) == 1750 ?                            \
		    RV3032_TCM_1750MV :                                                            \
		     DT_INST_PROP(inst, trickle_charger_mode) == 3000 ?                            \
		     RV3032_TCM_3000MV :                                                           \
		     DT_INST_PROP(inst, trickle_charger_mode) == 4500 ?                            \
			 RV3032_TCM_4500MV :                                                       \
		     RV3032_TCM_DISABLED),                                                         \
		    (RV3032_TCM_DISABLED))

#define RV3032_TCR_FROM_DT_INST(inst)                                                              \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, trickle_resistor_ohms),                        \
		    (DT_INST_PROP(inst, trickle_resistor_ohms) == 600 ?                            \
		    RV3032_TCR_600_OHM :                                                           \
		     DT_INST_PROP(inst, trickle_resistor_ohms) == 2000 ?                           \
		     RV3032_TCR_2000_OHM :                                                         \
		     DT_INST_PROP(inst, trickle_resistor_ohms) == 7000 ?                           \
		     RV3032_TCR_7000_OHM :                                                         \
		     RV3032_TCR_12000_OHM),                                                        \
		    (RV3032_TCR_600_OHM))

#define RV3032_BACKUP_FROM_DT_INST(inst)                                                           \
	((FIELD_PREP(RV3032_EEPROM_PMU_BSM, RV3032_BSM_FROM_DT_INST(inst))) |                      \
	 (FIELD_PREP(RV3032_EEPROM_PMU_TCR, RV3032_TCR_FROM_DT_INST(inst))) |                      \
	 (FIELD_PREP(RV3032_EEPROM_PMU_TCM, RV3032_TCM_FROM_DT_INST(inst))))

#define RV3032_CLKOUT_FREQ_IS_VALID(freq)                                                          \
	((freq) == 0 || (freq) == RV3032_CLKOUT_FREQ_1HZ || (freq) == RV3032_CLKOUT_FREQ_64HZ ||   \
	 (freq) == RV3032_CLKOUT_FREQ_1024HZ || (freq) == RV3032_CLKOUT_FREQ_32768HZ ||            \
	 (((freq) >= RV3032_CLKOUT_FREQ_HF_MIN) && ((freq) <= RV3032_CLKOUT_FREQ_HF_MAX) &&        \
	  (((freq) % RV3032_CLKOUT_FREQ_HF_STEP) == 0)))

#define RV3032_INIT(inst)                                                                          \
	BUILD_ASSERT(RV3032_CLKOUT_FREQ_IS_VALID(DT_INST_PROP_OR(inst, clkout_frequency, 0)),      \
		     "Invalid CLKOUT frequency for RV3032 instance " STRINGIFY(inst));             \
                                                                                                   \
	static const struct rv3032_config rv3032_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.backup = RV3032_BACKUP_FROM_DT_INST(inst),                                        \
		.clkout_freq = DT_INST_PROP_OR(inst, clkout_frequency, 0),                         \
		IF_ENABLED(RV3032_INT_GPIOS_IN_USE, (.gpio_int =\
				GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0})))};             \
                                                                                                   \
	static struct rv3032_data rv3032_data_##inst;                                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &rv3032_init, NULL, &rv3032_data_##inst,                       \
			      &rv3032_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,        \
			      &rv3032_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV3032_INIT)
