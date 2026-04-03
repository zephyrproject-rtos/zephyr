/*
 * Copyright (c) 2019-2023 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pcf8523

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(pcf8523, CONFIG_RTC_LOG_LEVEL);

/* PCF8523 register addresses */
#define PCF8523_CONTROL_1	0x00U
#define PCF8523_CONTROL_2	0x01U
#define PCF8523_CONTROL_3	0x02U
#define PCF8523_SECONDS		0x03U
#define PCF8523_MINUTES		0x04U
#define PCF8523_HOURS		0x05U
#define PCF8523_DAYS		0x06U
#define PCF8523_WEEKDAYS	0x07U
#define PCF8523_MONTHS		0x08U
#define PCF8523_YEARS		0x09U
#define PCF8523_MINUTE_ALARM	0x0aU
#define PCF8523_HOUR_ALARM	0x0bU
#define PCF8523_DAY_ALARM	0x0cU
#define PCF8523_WEEKDAY_ALARM	0x0dU
#define PCF8523_OFFSET		0x0eU
#define PCF8523_TMR_CLKOUT_CTRL 0x0fU
#define PCF8523_TMR_A_FREQ_CTRL 0x10U
#define PCF8523_TMR_A_REG	0x11U
#define PCF8523_TMR_B_FREQ_CTRL 0x12U
#define PCF8523_TMR_B_REG	0x13U

/* Control register bits */
#define PCF8523_CONTROL_1_CAP_SEL BIT(7)
#define PCF8523_CONTROL_1_T	  BIT(6)
#define PCF8523_CONTROL_1_STOP	  BIT(5)
#define PCF8523_CONTROL_1_SR	  BIT(4)
#define PCF8523_CONTROL_1_12_24	  BIT(3)
#define PCF8523_CONTROL_1_SIE	  BIT(2)
#define PCF8523_CONTROL_1_AIE	  BIT(1)
#define PCF8523_CONTROL_1_CIE	  BIT(0)
#define PCF8523_CONTROL_2_WTAF	  BIT(7)
#define PCF8523_CONTROL_2_CTAF	  BIT(6)
#define PCF8523_CONTROL_2_CTBF	  BIT(5)
#define PCF8523_CONTROL_2_SF	  BIT(4)
#define PCF8523_CONTROL_2_AF	  BIT(3)
#define PCF8523_CONTROL_2_WTAIE	  BIT(2)
#define PCF8523_CONTROL_2_CTAIE	  BIT(1)
#define PCF8523_CONTROL_2_CTBIE	  BIT(0)
#define PCF8523_CONTROL_3_PM_MASK GENMASK(7, 5)
#define PCF8523_CONTROL_3_BSF	  BIT(3)
#define PCF8523_CONTROL_3_BLF	  BIT(2)
#define PCF8523_CONTROL_3_BSIE	  BIT(1)
#define PCF8523_CONTROL_3_BLIE	  BIT(0)

/* Time and date register bits */
#define PCF8523_SECONDS_OS     BIT(7)
#define PCF8523_SECONDS_MASK   GENMASK(6, 0)
#define PCF8523_MINUTES_MASK   GENMASK(6, 0)
#define PCF8523_HOURS_AMPM     BIT(5)
#define PCF8523_HOURS_12H_MASK GENMASK(4, 0)
#define PCF8523_HOURS_24H_MASK GENMASK(5, 0)
#define PCF8523_DAYS_MASK      GENMASK(5, 0)
#define PCF8523_WEEKDAYS_MASK  GENMASK(2, 0)
#define PCF8523_MONTHS_MASK    GENMASK(4, 0)
#define PCF8523_YEARS_MASK     GENMASK(7, 0)

/* Alarm register bits */
#define PCF8523_MINUTE_ALARM_AEN_M  BIT(7)
#define PCF8523_MINUTE_ALARM_MASK   GENMASK(6, 0)
#define PCF8523_HOUR_ALARM_AEN_H    BIT(7)
#define PCF8523_HOUR_ALARM_AMPM	    BIT(5)
#define PCF8523_HOUR_ALARM_12H_MASK GENMASK(4, 0)
#define PCF8523_HOUR_ALARM_24H_MASK GENMASK(5, 0)
#define PCF8523_DAY_ALARM_AEN_D	    BIT(7)
#define PCF8523_DAY_ALARM_MASK	    GENMASK(5, 0)
#define PCF8523_WEEKDAY_ALARM_AEN_W BIT(7)
#define PCF8523_WEEKDAY_ALARM_MASK  GENMASK(5, 0)

/* Timer register bits */
#define PCF8523_TMR_CLKOUT_CTRL_TAM	 BIT(7)
#define PCF8523_TMR_CLKOUT_CTRL_TBM	 BIT(6)
#define PCF8523_TMR_CLKOUT_CTRL_COF_MASK GENMASK(5, 3)
#define PCF8523_TMR_CLKOUT_CTRL_TAC_MASK GENMASK(2, 1)
#define PCF8523_TMR_CLKOUT_CTRL_TBC	 BIT(0)
#define PCF8523_TMR_A_FREQ_CTRL_TAQ_MASK GENMASK(2, 0)
#define PCF8523_TMR_A_REG_T_A_MASK	 GENMASK(7, 0)
#define PCF8523_TMR_B_FREQ_CTRL_TBW_MASK GENMASK(6, 4)
#define PCF8523_TMR_B_FREQ_CTRL_TBQ_MASK GENMASK(2, 0)
#define PCF8523_TMR_B_REG_T_B_MASK	 GENMASK(7, 0)

/* Offset register bits */
#define PCF8523_OFFSET_MODE BIT(7)
#define PCF8523_OFFSET_MASK GENMASK(6, 0)

/* RTC alarm time fields supported by the PCF8523 */
#define PCF8523_RTC_ALARM_TIME_MASK                                                                \
	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY |    \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

/* The PCF8523 only supports two-digit years, calculate offset to use */
#define PCF8523_YEARS_OFFSET (2000 - 1900)

/* The PCF8523 enumerates months 1 to 12, RTC API uses 0 to 11 */
#define PCF8523_MONTHS_OFFSET 1

/* Helper macro to guard int1-gpios related code */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int1_gpios) &&                                                \
	(defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE))
#define PCF8523_INT1_GPIOS_IN_USE 1
#endif

struct pcf8523_config {
	const struct i2c_dt_spec i2c;
#ifdef PCF8523_INT1_GPIOS_IN_USE
	struct gpio_dt_spec int1;
#endif /* PCF8523_INT1_GPIOS_IN_USE */
	uint8_t cof;
	uint8_t pm;
	bool cap_sel;
	bool wakeup_source;
};

struct pcf8523_data {
	struct k_mutex lock;
#if PCF8523_INT1_GPIOS_IN_USE
	struct gpio_callback int1_callback;
	struct k_thread int1_thread;
	struct k_sem int1_sem;

	K_KERNEL_STACK_MEMBER(int1_stack, CONFIG_RTC_PCF8523_THREAD_STACK_SIZE);
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_callback;
	void *update_user_data;
#endif /* CONFIG_RTC_UPDATE */
#endif /* PCF8523_INT1_GPIOS_IN_USE */
};

static int pcf8523_read_regs(const struct device *dev, uint8_t addr, void *buf, size_t len)
{
	const struct pcf8523_config *config = dev->config;
	int err;

	err = i2c_write_read_dt(&config->i2c, &addr, sizeof(addr), buf, len);
	if (err != 0) {
		LOG_ERR("failed to read reg addr 0x%02x, len %d (err %d)", addr, len, err);
		return err;
	}

	return 0;
}

static int pcf8523_read_reg8(const struct device *dev, uint8_t addr, uint8_t *val)
{
	return pcf8523_read_regs(dev, addr, val, sizeof(*val));
}

static int pcf8523_write_regs(const struct device *dev, uint8_t addr, void *buf, size_t len)
{
	const struct pcf8523_config *config = dev->config;
	uint8_t block[sizeof(addr) + len];
	int err;

	block[0] = addr;
	memcpy(&block[1], buf, len);

	err = i2c_write_dt(&config->i2c, block, sizeof(block));
	if (err != 0) {
		LOG_ERR("failed to write reg addr 0x%02x, len %d (err %d)", addr, len, err);
		return err;
	}

	return 0;
}

static int pcf8523_write_reg8(const struct device *dev, uint8_t addr, uint8_t val)
{
	return pcf8523_write_regs(dev, addr, &val, sizeof(val));
}

static int pcf8523_write_stop_bit_unlocked(const struct device *dev, bool value)
{
	uint8_t control_1;
	int err;

	err = pcf8523_read_reg8(dev, PCF8523_CONTROL_1, &control_1);
	if (err != 0) {
		return err;
	}

	if (value) {
		control_1 |= PCF8523_CONTROL_1_STOP;
	} else {
		control_1 &= ~(PCF8523_CONTROL_1_STOP);
	}

	err = pcf8523_write_reg8(dev, PCF8523_CONTROL_1, control_1);
	if (err != 0) {
		return err;
	}

	return 0;
}

#if PCF8523_INT1_GPIOS_IN_USE
static int pcf8523_int1_enable_unlocked(const struct device *dev, bool enable)
{
	const struct pcf8523_config *config = dev->config;
	uint8_t tmr_clkout_ctrl;
	int err;

	if (!config->wakeup_source) {
		/* Only change COF if not configured as wakeup-source */
		err = pcf8523_read_reg8(dev, PCF8523_TMR_CLKOUT_CTRL, &tmr_clkout_ctrl);
		if (err != 0) {
			return err;
		}

		if (enable) {
			/* Disable CLKOUT */
			tmr_clkout_ctrl |= PCF8523_TMR_CLKOUT_CTRL_COF_MASK;
		} else if (!config->wakeup_source) {
			/* Enable CLKOUT */
			tmr_clkout_ctrl &= ~(PCF8523_TMR_CLKOUT_CTRL_COF_MASK);
			tmr_clkout_ctrl |=
				FIELD_PREP(PCF8523_TMR_CLKOUT_CTRL_COF_MASK, config->cof);
		}

		err = pcf8523_write_reg8(dev, PCF8523_TMR_CLKOUT_CTRL, tmr_clkout_ctrl);
		if (err != 0) {
			return err;
		}
	}

	/* Use edge interrupts to avoid multiple GPIO IRQs while servicing the IRQ in the thread */
	err = gpio_pin_interrupt_configure_dt(&config->int1,
					      enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
	if (err != 0) {
		LOG_ERR("failed to %s GPIO IRQ (err %d)", enable ? "enable" : "disable", err);
		return err;
	}

	return 0;
}

static void pcf8523_int1_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct pcf8523_data *data = dev->data;
	rtc_alarm_callback alarm_callback = NULL;
	void *alarm_user_data = NULL;
	rtc_update_callback update_callback = NULL;
	void *update_user_data = NULL;
	uint8_t control_2;
	int err;

	while (true) {
		k_sem_take(&data->int1_sem, K_FOREVER);
		k_mutex_lock(&data->lock, K_FOREVER);

		err = pcf8523_read_reg8(dev, PCF8523_CONTROL_2, &control_2);
		if (err != 0) {
			goto unlock;
		}

#ifdef CONFIG_RTC_ALARM
		if ((control_2 & PCF8523_CONTROL_2_AF) != 0 && data->alarm_callback != NULL) {
			control_2 &= ~(PCF8523_CONTROL_2_AF);
			alarm_callback = data->alarm_callback;
			alarm_user_data = data->alarm_user_data;
		}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
		if ((control_2 & PCF8523_CONTROL_2_SF) != 0) {
			control_2 &= ~(PCF8523_CONTROL_2_SF);
			update_callback = data->update_callback;
			update_user_data = data->update_user_data;
		}
#endif /* CONFIG_RTC_UPDATE */

		control_2 |= PCF8523_CONTROL_2_CTAF | PCF8523_CONTROL_2_CTBF;

		err = pcf8523_write_reg8(dev, PCF8523_CONTROL_2, control_2);
		if (err != 0) {
			goto unlock;
		}

		/* Check if interrupt occurred between CONTROL_2 read/write */
		err = pcf8523_read_reg8(dev, PCF8523_CONTROL_2, &control_2);
		if (err != 0) {
			goto unlock;
		}

		if (((control_2 & PCF8523_CONTROL_2_AF) != 0U && alarm_callback != NULL) ||
		    ((control_2 & PCF8523_CONTROL_2_SF) != 0U)) {
			/*
			 * Another interrupt occurred while servicing this one, process current
			 * callback(s) and yield.
			 */
			k_sem_give(&data->int1_sem);
		}

unlock:
		k_mutex_unlock(&data->lock);

		if (alarm_callback != NULL) {
			alarm_callback(dev, 0U, alarm_user_data);
			alarm_callback = NULL;
		}

		if (update_callback != NULL) {
			update_callback(dev, update_user_data);
			update_callback = NULL;
		}
	}
}

static void pcf8523_int1_callback_handler(const struct device *port, struct gpio_callback *cb,
					  gpio_port_pins_t pins)
{
	struct pcf8523_data *data = CONTAINER_OF(cb, struct pcf8523_data, int1_callback);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_sem_give(&data->int1_sem);
}
#endif /* PCF8523_INT1_GPIOS_IN_USE */

static int pcf8523_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct pcf8523_data *data = dev->data;
	uint8_t regs[7];
	int err;

	if (timeptr->tm_year < PCF8523_YEARS_OFFSET ||
	    timeptr->tm_year > PCF8523_YEARS_OFFSET + 99) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Freeze the time circuits */
	err = pcf8523_write_stop_bit_unlocked(dev, true);
	if (err != 0) {
		goto unlock;
	}

	LOG_DBG("set time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	regs[0] = bin2bcd(timeptr->tm_sec) & PCF8523_SECONDS_MASK;
	regs[1] = bin2bcd(timeptr->tm_min) & PCF8523_MINUTES_MASK;
	regs[2] = bin2bcd(timeptr->tm_hour) & PCF8523_HOURS_24H_MASK;
	regs[3] = bin2bcd(timeptr->tm_mday) & PCF8523_DAYS_MASK;
	regs[4] = bin2bcd(timeptr->tm_wday) & PCF8523_WEEKDAYS_MASK;
	regs[5] = bin2bcd(timeptr->tm_mon + PCF8523_MONTHS_OFFSET) & PCF8523_MONTHS_MASK;
	regs[6] = bin2bcd(timeptr->tm_year - PCF8523_YEARS_OFFSET) & PCF8523_YEARS_MASK;

	/* Write registers PCF8523_SECONDS through PCF8523_YEARS */
	err = pcf8523_write_regs(dev, PCF8523_SECONDS, &regs, sizeof(regs));
	if (err != 0) {
		goto unlock;
	}

	/* Unfreeze time circuits */
	err = pcf8523_write_stop_bit_unlocked(dev, false);
	if (err != 0) {
		goto unlock;
	}

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static int pcf8523_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	uint8_t regs[10];
	int err;

	/* Read registers PCF8523_CONTROL_1 through PCF8523_YEARS */
	err = pcf8523_read_regs(dev, PCF8523_CONTROL_1, &regs, sizeof(regs));
	if (err != 0) {
		return err;
	}

	if ((regs[0] & PCF8523_CONTROL_1_STOP) != 0) {
		LOG_WRN("time circuits frozen");
		return -ENODATA;
	}

	if ((regs[3] & PCF8523_SECONDS_OS) != 0) {
		LOG_WRN("oscillator stopped or interrupted");
		return -ENODATA;
	}

	memset(timeptr, 0U, sizeof(*timeptr));
	timeptr->tm_sec = bcd2bin(regs[3] & PCF8523_SECONDS_MASK);
	timeptr->tm_min = bcd2bin(regs[4] & PCF8523_MINUTES_MASK);
	timeptr->tm_hour = bcd2bin(regs[5] & PCF8523_HOURS_24H_MASK);
	timeptr->tm_mday = bcd2bin(regs[6] & PCF8523_DAYS_MASK);
	timeptr->tm_wday = bcd2bin(regs[7] & PCF8523_WEEKDAYS_MASK);
	timeptr->tm_mon = bcd2bin(regs[8] & PCF8523_MONTHS_MASK) - PCF8523_MONTHS_OFFSET;
	timeptr->tm_year = bcd2bin(regs[9] & PCF8523_YEARS_MASK) + PCF8523_YEARS_OFFSET;
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;

	LOG_DBG("get time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int pcf8523_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	*mask = PCF8523_RTC_ALARM_TIME_MASK;

	return 0;
}

static int pcf8523_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				  const struct rtc_time *timeptr)
{
	uint8_t regs[4];

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	if ((mask & ~(PCF8523_RTC_ALARM_TIME_MASK)) != 0U) {
		LOG_ERR("unsupported alarm field mask 0x%04x", mask);
		return -EINVAL;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) != 0U) {
		regs[0] = bin2bcd(timeptr->tm_min) & PCF8523_MINUTE_ALARM_MASK;
	} else {
		regs[0] = PCF8523_MINUTE_ALARM_AEN_M;
	}

	if ((mask & RTC_ALARM_TIME_MASK_HOUR) != 0U) {
		regs[1] = bin2bcd(timeptr->tm_hour) & PCF8523_HOUR_ALARM_24H_MASK;
	} else {
		regs[1] = PCF8523_HOUR_ALARM_AEN_H;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) != 0U) {
		regs[2] = bin2bcd(timeptr->tm_mday) & PCF8523_DAY_ALARM_MASK;
	} else {
		regs[2] = PCF8523_DAY_ALARM_AEN_D;
	}

	if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) != 0U) {
		regs[3] = bin2bcd(timeptr->tm_wday) & PCF8523_WEEKDAY_ALARM_MASK;
	} else {
		regs[3] = PCF8523_WEEKDAY_ALARM_AEN_W;
	}

	LOG_DBG("set alarm: year = %d, mon = %d, mday = %d, hour = %d, min = %d, mask = 0x%04x",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_hour,
		timeptr->tm_min, mask);

	/* Write registers PCF8523_MINUTE_ALARM through PCF8523_WEEKDAY_ALARM */
	return pcf8523_write_regs(dev, PCF8523_MINUTE_ALARM, &regs, sizeof(regs));
}

static int pcf8523_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				  struct rtc_time *timeptr)
{
	uint8_t regs[4];
	int err;

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	/* Read registers PCF8523_MINUTE_ALARM through PCF8523_WEEKDAY_ALARM */
	err = pcf8523_read_regs(dev, PCF8523_MINUTE_ALARM, &regs, sizeof(regs));
	if (err != 0) {
		return err;
	}

	memset(timeptr, 0U, sizeof(*timeptr));
	*mask = 0U;

	if ((regs[0] & PCF8523_MINUTE_ALARM_AEN_M) == 0) {
		timeptr->tm_min = bcd2bin(regs[0] & PCF8523_MINUTE_ALARM_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if ((regs[1] & PCF8523_HOUR_ALARM_AEN_H) == 0) {
		timeptr->tm_hour = bcd2bin(regs[1] & PCF8523_HOUR_ALARM_24H_MASK);
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if ((regs[2] & PCF8523_DAY_ALARM_AEN_D) == 0) {
		timeptr->tm_mday = bcd2bin(regs[2] & PCF8523_DAY_ALARM_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	if ((regs[3] & PCF8523_WEEKDAY_ALARM_AEN_W) == 0) {
		timeptr->tm_wday = bcd2bin(regs[3] & PCF8523_WEEKDAY_ALARM_MASK);
		*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
	}

	LOG_DBG("get alarm: year = %d, mon = %d, mday = %d, hour = %d, min = %d, mask = 0x%04x",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_hour,
		timeptr->tm_min, *mask);

	return 0;
}

static int pcf8523_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct pcf8523_data *data = dev->data;
	uint8_t control_2;
	int err;

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	err = pcf8523_read_reg8(dev, PCF8523_CONTROL_2, &control_2);
	if (err != 0) {
		goto unlock;
	}

	if ((control_2 & PCF8523_CONTROL_2_AF) != 0) {
		/* Clear alarm flag */
		control_2 &= ~(PCF8523_CONTROL_2_AF);

		/* Ensure other flags are left unchanged (PCF8523 performs logic AND at write) */
		control_2 |= PCF8523_CONTROL_2_CTAF | PCF8523_CONTROL_2_CTBF | PCF8523_CONTROL_2_SF;

		err = pcf8523_write_reg8(dev, PCF8523_CONTROL_2, control_2);
		if (err != 0) {
			goto unlock;
		}

		/* Alarm pending */
		err = 1;
	}

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static int pcf8523_alarm_set_callback(const struct device *dev, uint16_t id,
				      rtc_alarm_callback callback, void *user_data)
{
#ifndef PCF8523_INT1_GPIOS_IN_USE
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
#else
	const struct pcf8523_config *config = dev->config;
	struct pcf8523_data *data = dev->data;
	uint8_t control_1;
	int err = 0;

	if (config->int1.port == NULL) {
		return -ENOTSUP;
	}

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->alarm_callback = callback;
	data->alarm_user_data = user_data;

	if (!config->wakeup_source) {
		/* Only change AIE if not configured as wakeup-source */
		err = pcf8523_read_reg8(dev, PCF8523_CONTROL_1, &control_1);
		if (err != 0) {
			goto unlock;
		}

		if (callback != NULL) {
			control_1 |= PCF8523_CONTROL_1_AIE;
		} else {
			control_1 &= ~(PCF8523_CONTROL_1_AIE);
		}

		if ((control_1 & PCF8523_CONTROL_1_SIE) == 0U) {
			/* Only change INT1 GPIO if seconds timer interrupt not enabled */
			err = pcf8523_int1_enable_unlocked(dev, callback != NULL);
			if (err != 0) {
				goto unlock;
			}
		}

		err = pcf8523_write_reg8(dev, PCF8523_CONTROL_1, control_1);
		if (err != 0) {
			goto unlock;
		}
	}

unlock:
	k_mutex_unlock(&data->lock);

	/* Wake up the INT1 thread since the alarm flag may already be set */
	k_sem_give(&data->int1_sem);

	return err;
#endif /* PCF8523_INT1_GPIOS_IN_USE */
}
#endif /* CONFIG_RTC_ALARM */

#if PCF8523_INT1_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE)
static int pcf8523_update_set_callback(const struct device *dev, rtc_update_callback callback,
				       void *user_data)
{
	const struct pcf8523_config *config = dev->config;
	struct pcf8523_data *data = dev->data;
	uint8_t control_1;
	int err;

	if (config->int1.port == NULL) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->update_callback = callback;
	data->update_user_data = user_data;

	err = pcf8523_read_reg8(dev, PCF8523_CONTROL_1, &control_1);
	if (err != 0) {
		goto unlock;
	}

	if (callback != NULL) {
		control_1 |= PCF8523_CONTROL_1_SIE;
	} else {
		control_1 &= ~(PCF8523_CONTROL_1_SIE);
	}

	if ((control_1 & PCF8523_CONTROL_1_AIE) == 0U) {
		/* Only change INT1 GPIO if alarm interrupt not enabled */
		err = pcf8523_int1_enable_unlocked(dev, callback != NULL);
		if (err != 0) {
			goto unlock;
		}
	}

	err = pcf8523_write_reg8(dev, PCF8523_CONTROL_1, control_1);
	if (err != 0) {
		goto unlock;
	}

unlock:
	k_mutex_unlock(&data->lock);

	/* Wake up the INT1 thread since the seconds flag may already be set */
	k_sem_give(&data->int1_sem);

	return err;
}
#endif /* PCF8523_INT1_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE) */

#ifdef CONFIG_RTC_CALIBRATION

/* See PCF8523 data sheet table 29 */
#if defined(CONFIG_RTC_PCF8523_OFFSET_MODE_SLOW)
#define PCF8523_OFFSET_PPB_PER_LSB 4340
#elif defined(CONFIG_RTC_PCF8523_OFFSET_MODE_FAST)
#define PCF8523_OFFSET_PPB_PER_LSB 4069
#else
#error Unsupported offset mode
#endif

#define PCF8523_OFFSET_PPB_MIN (-64 * PCF8523_OFFSET_PPB_PER_LSB)
#define PCF8523_OFFSET_PPB_MAX (63 * PCF8523_OFFSET_PPB_PER_LSB)

static int pcf8523_set_calibration(const struct device *dev, int32_t freq_ppb)
{
	int32_t period_ppb = freq_ppb * -1;
	int8_t offset;

	if (period_ppb < PCF8523_OFFSET_PPB_MIN || period_ppb > PCF8523_OFFSET_PPB_MAX) {
		LOG_WRN("calibration value (%d ppb) out of range", freq_ppb);
		return -EINVAL;
	}

	offset = period_ppb / PCF8523_OFFSET_PPB_PER_LSB;

	if (IS_ENABLED(CONFIG_RTC_PCF8523_OFFSET_MODE_FAST)) {
		offset |= PCF8523_OFFSET_MODE;
	}

	LOG_DBG("freq_ppb = %d, period_ppb = %d, offset = %d", freq_ppb, period_ppb, offset);

	return pcf8523_write_reg8(dev, PCF8523_OFFSET, offset);
}

static int pcf8523_get_calibration(const struct device *dev, int32_t *freq_ppb)
{
	int32_t period_ppb;
	int8_t offset;
	int err;

	err = pcf8523_read_reg8(dev, PCF8523_OFFSET, &offset);
	if (err != 0) {
		return err;
	}

	/* Clear mode bit and sign extend the offset */
	period_ppb = (offset << 1U) >> 1U;

	period_ppb = period_ppb * PCF8523_OFFSET_PPB_PER_LSB;
	*freq_ppb = period_ppb * -1;

	LOG_DBG("freq_ppb = %d, period_ppb = %d, offset = %d", *freq_ppb, period_ppb, offset);

	return 0;
}
#endif /* CONFIG_RTC_CALIBRATION */

static int pcf8523_init(const struct device *dev)
{
	const struct pcf8523_config *config = dev->config;
	struct pcf8523_data *data = dev->data;
	uint8_t tmr_clkout_ctrl;
	uint8_t regs[3];
	int err;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

#if PCF8523_INT1_GPIOS_IN_USE
	k_tid_t tid;

	if (config->int1.port != NULL) {
		k_sem_init(&data->int1_sem, 0, INT_MAX);

		if (!gpio_is_ready_dt(&config->int1)) {
			LOG_ERR("GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->int1, GPIO_INPUT);
		if (err != 0) {
			LOG_ERR("failed to configure GPIO (err %d)", err);
			return -ENODEV;
		}

		gpio_init_callback(&data->int1_callback, pcf8523_int1_callback_handler,
				   BIT(config->int1.pin));

		err = gpio_add_callback_dt(&config->int1, &data->int1_callback);
		if (err != 0) {
			LOG_ERR("failed to add GPIO callback (err %d)", err);
			return -ENODEV;
		}

		tid = k_thread_create(&data->int1_thread, data->int1_stack,
				      K_THREAD_STACK_SIZEOF(data->int1_stack),
				      pcf8523_int1_thread, (void *)dev, NULL,
				      NULL, CONFIG_RTC_PCF8523_THREAD_PRIO, 0, K_NO_WAIT);
		k_thread_name_set(tid, "pcf8523");

		/*
		 * Defer GPIO interrupt configuration due to INT1/CLKOUT pin sharing. This allows
		 * using the CLKOUT square-wave signal for RTC calibration when no alarm/update
		 * callbacks are enabled (and not configured as a wakeup-source).
		 */
	}
#endif /* PCF8523_INT1_GPIOS_IN_USE */

	/*
	 * Manually initialize the required PCF8523 registers as performing a software reset will
	 * reset the time circuits.
	 */

	/* Read registers PCF8523_CONTROL_1 through PCF8523_CONTROL_3 */
	err = pcf8523_read_regs(dev, PCF8523_CONTROL_1, &regs, sizeof(regs));
	if (err != 0) {
		return -ENODEV;
	}

	/* Set quartz crystal load capacitance */
	if (config->cap_sel) {
		regs[0] |= PCF8523_CONTROL_1_CAP_SEL;
	} else {
		regs[0] &= ~(PCF8523_CONTROL_1_CAP_SEL);
	}

	/* Use 24h time format */
	regs[0] &= ~(PCF8523_CONTROL_1_12_24);

	/* Disable second, alarm, and correction interrupts */
	regs[0] &= ~(PCF8523_CONTROL_1_SIE | PCF8523_CONTROL_1_AIE | PCF8523_CONTROL_1_CIE);

	if (config->wakeup_source) {
		/*
		 * Always set AIE if wakeup-source. This allows the RTC to wake up the system even
		 * if the INT1 interrupt output is not directly connected to a GPIO (i.e. if
		 * connected to a PMIC input).
		 */
		regs[0] |= PCF8523_CONTROL_1_AIE;
	}

	/* Clear interrupt flags (except alarm flag, as a wake-up alarm may be pending) */
	regs[1] &= ~(PCF8523_CONTROL_2_CTAF | PCF8523_CONTROL_2_CTBF | PCF8523_CONTROL_2_SF);

	/* Disable watchdog, timer A, and timer B interrupts */
	regs[1] &= ~(PCF8523_CONTROL_2_WTAIE | PCF8523_CONTROL_2_CTAIE | PCF8523_CONTROL_2_CTBIE);

	/* Configure battery switch-over function */
	regs[2] &= ~(PCF8523_CONTROL_3_PM_MASK);
	regs[2] |= FIELD_PREP(PCF8523_CONTROL_3_PM_MASK, config->pm);

	/* Clear battery status interrupt flag */
	regs[2] &= ~(PCF8523_CONTROL_3_BSF);

	/* Disable battery status interrupts */
	regs[2] &= ~(PCF8523_CONTROL_3_BSIE | PCF8523_CONTROL_3_BLIE);

	/* Write registers PCF8523_CONTROL_1 through PCF8523_CONTROL_3 */
	err = pcf8523_write_regs(dev, PCF8523_CONTROL_1, &regs, sizeof(regs));
	if (err != 0) {
		return -ENODEV;
	}

	/* Disable watchdog and countdown timers, configure IRQ level*/
	tmr_clkout_ctrl = 0U;

	if (config->wakeup_source) {
		/* Disable CLKOUT */
		tmr_clkout_ctrl |= PCF8523_TMR_CLKOUT_CTRL_COF_MASK;
	} else {
		/* Configure CLKOUT frequency */
		tmr_clkout_ctrl |= FIELD_PREP(PCF8523_TMR_CLKOUT_CTRL_COF_MASK, config->cof);
	}

	err = pcf8523_write_reg8(dev, PCF8523_TMR_CLKOUT_CTRL, tmr_clkout_ctrl);
	if (err != 0) {
		return -ENODEV;
	}

	return 0;
}

/* Mapping from DT battery-switch-over enum to CONTROL_3 PM field value */
#define PCF8523_PM_STANDARD 4U
#define PCF8523_PM_DIRECT   5U
#define PCF8523_PM_DISABLED 7U

#ifdef CONFIG_PM_DEVICE
static int pcf8523_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct pcf8523_config *config = dev->config;
	uint8_t control_3;
	int err;

	if (config->pm == PCF8523_PM_DISABLED) {
		/* Only one power supply */
		return -ENOTSUP;
	}

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* Disable battery switch-over function */
		control_3 = FIELD_PREP(PCF8523_CONTROL_3_PM_MASK, PCF8523_PM_DISABLED);
		break;
	case PM_DEVICE_ACTION_RESUME:
		/* Re-enable battery switch-over function */
		control_3 = FIELD_PREP(PCF8523_CONTROL_3_PM_MASK, config->pm);
		break;
	default:
		return -ENOTSUP;
	}

	err = pcf8523_write_reg8(dev, PCF8523_CONTROL_3, control_3);
	if (err != 0) {
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(rtc, pcf8523_driver_api) = {
	.set_time = pcf8523_set_time,
	.get_time = pcf8523_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = pcf8523_alarm_get_supported_fields,
	.alarm_set_time = pcf8523_alarm_set_time,
	.alarm_get_time = pcf8523_alarm_get_time,
	.alarm_is_pending = pcf8523_alarm_is_pending,
	.alarm_set_callback = pcf8523_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#if PCF8523_INT1_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE)
	.update_set_callback = pcf8523_update_set_callback,
#endif /* PCF8523_INT1_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE) */
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = pcf8523_set_calibration,
	.get_calibration = pcf8523_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

#define PCF8523_PM_FROM_DT_INST(inst)                                                              \
	UTIL_CAT(PCF8523_PM_, DT_INST_STRING_UPPER_TOKEN(inst, battery_switch_over))
#define PCF8523_CAP_SEL_FROM_DT_INST(inst) (DT_INST_PROP(inst, quartz_load_femtofarads) == 12500)

#define PCF8523_INIT(inst)                                                                         \
	static const struct pcf8523_config pcf8523_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.cof = DT_INST_ENUM_IDX(inst, clkout_frequency),                                   \
		.pm = PCF8523_PM_FROM_DT_INST(inst),                                               \
		.cap_sel = PCF8523_CAP_SEL_FROM_DT_INST(inst),                                     \
		.wakeup_source = DT_INST_PROP(inst, wakeup_source),                                \
		IF_ENABLED(PCF8523_INT1_GPIOS_IN_USE,                                              \
			   (.int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0})))};            \
                                                                                                   \
	static struct pcf8523_data pcf8523_data_##inst;                                            \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, pcf8523_pm_action);                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &pcf8523_init, PM_DEVICE_DT_INST_GET(inst),                    \
			      &pcf8523_data_##inst, &pcf8523_config_##inst, POST_KERNEL,           \
			      CONFIG_RTC_INIT_PRIORITY, &pcf8523_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PCF8523_INIT)
