/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rx8130ce, CONFIG_RTC_LOG_LEVEL);

#define DT_DRV_COMPAT epson_rx8130ce_rtc

enum registers {
	TIME		= 0x10,
	ALARM		= 0x17,
	/* control registers */
	EXTENSION	= 0x1C,
	FLAG		= 0x1D,
	CTRL0		= 0x1E,
	CTRL1		= 0x1F,
	OFFSET		= 0x30,
};

#define RX8130CE_SECONDS_MASK	GENMASK(6, 0)
#define RX8130CE_MINUTES_MASK	GENMASK(6, 0)
#define RX8130CE_HOURS_MASK	GENMASK(5, 0)
#define RX8130CE_DAYS_MASK	GENMASK(5, 0)
#define RX8130CE_WEEKDAYS_MASK	GENMASK(6, 0)
#define RX8130CE_MONTHS_MASK	GENMASK(4, 0)
#define RX8130CE_YEARS_MASK	GENMASK(7, 0)

#define RX8130CE_MONTHS_OFFSET (1)
#define RX8130CE_YEARS_OFFSET (100)

/* Alarm AE bit */
#define ALARM_DISABLE	BIT(7)

/* Extension reg(0x1C) bit field */
#define EXT_TSEL0	BIT(0)
#define EXT_TSEL1	BIT(1)
#define EXT_TSEL2	BIT(2)
#define EXT_WADA	BIT(3)
#define EXT_TE		BIT(4)
#define EXT_USEL	BIT(5)
#define EXT_FSEL0	BIT(6)
#define EXT_FSEL1	BIT(7)

/* Flag reg(0x1D) bit field */
#define FLAG_VBFF	BIT(0)
#define FLAG_VLF	BIT(1)
#define FLAG_RSF	BIT(2)
#define FLAG_AF		BIT(3)
#define FLAG_TF		BIT(4)
#define FLAG_UF		BIT(5)
#define FLAG_VBLF	BIT(7)

/* Control0 reg(0x1E) bit field */
#define CTRL0_TBKE	BIT(0)
#define CTRL0_TBKON	BIT(1)
#define CTRL0_TSTP	BIT(2)
#define CTRL0_AIE	BIT(3)
#define CTRL0_TIE	BIT(4)
#define CTRL0_UIE	BIT(5)
#define CTRL0_STOP	BIT(6)
#define CTRL0_TEST	BIT(7)

/* ctrl1 reg(0x1F) bit field */
#define CTRL1_BFVSEL0	BIT(0)
#define CTRL1_BFVSEL1	BIT(1)
#define CTRL1_RSVSEL	BIT(2)
#define CTRL1_INIEN	BIT(4)
#define CTRL1_CHGEN	BIT(5)
#define CTRL1_SMPTSEL0	BIT(6)
#define CTRL1_SMPTSEL1	BIT(7)


/* Digital Offest reg(0x30) bit field */
#define DIGITAL_OFFSET_NEG	BIT(6)
#define DIGITAL_OFFSET_DTE	BIT(7)

/* Digital Offset register values */
#define DIGITAL_OFFSET_MAX	192260
#define DIGITAL_OFFSET_MIN	-195310
#define DIGITAL_OFFSET_STEP_PPB	3050

/**
 * @brief rx8130ce control registers
 * 0x1C extension register
 * 0x1D Flag register
 * 0x1E control0
 * 0x1F ctrl1
 */
struct __packed rx8130ce_registers {
	uint8_t extension;
	uint8_t flag;
	uint8_t ctrl0;
	uint8_t ctrl1;
};

struct __packed rx8130ce_time {
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t weekday;
	uint8_t day;
	uint8_t month;
	uint8_t year;
};

struct __packed rx8130ce_alarm {
	uint8_t minute;
	uint8_t hour;
	union {
		uint8_t wday;
		uint8_t day;
	};
};

struct rx8130ce_config {
	const struct i2c_dt_spec i2c;
	struct gpio_dt_spec irq;
	uint16_t clockout_frequency;
	uint8_t battery_switchover;
};

struct rx8130ce_data {
	struct k_sem lock;
	const struct device *dev;
	struct rx8130ce_registers reg;

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
	struct gpio_callback irq_cb;
	struct k_work irq_work;
#endif
#ifdef CONFIG_RTC_ALARM
	void *alarm_user_data;
	rtc_alarm_callback alarm_callback;
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	void *update_user_data;
	rtc_update_callback update_callback;
#endif /* CONFIG_RTC_UPDATE */
};

static inline uint8_t wday2rtc(uint8_t wday)
{
	return 1 << wday;
}

static inline uint8_t rtc2wday(uint8_t rtc_wday)
{
	for (size_t bit = 0 ; bit < 7; bit++) {
		if (rtc_wday & (1 << bit)) {
			return bit;
		}
	}
	return 0;
}

static int rx8130ce_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	int rc = 0;
	struct rx8130ce_time rtc_time;
	const struct rx8130ce_config *cfg = dev->config;
	struct rx8130ce_data *data = dev->data;

	memset(timeptr, 0U, sizeof(*timeptr));

	k_sem_take(&data->lock, K_FOREVER);
	rc = i2c_burst_read_dt(&cfg->i2c, TIME, (uint8_t *)&rtc_time, sizeof(rtc_time));
	if (rc != 0) {
		LOG_ERR("Failed to read time");
		goto error;
	}
	timeptr->tm_sec = bcd2bin(rtc_time.second & RX8130CE_SECONDS_MASK);
	timeptr->tm_min = bcd2bin(rtc_time.minute & RX8130CE_MINUTES_MASK);
	timeptr->tm_hour = bcd2bin(rtc_time.hour & RX8130CE_HOURS_MASK);
	timeptr->tm_mday = bcd2bin(rtc_time.day & RX8130CE_DAYS_MASK);
	timeptr->tm_wday = rtc2wday(rtc_time.weekday & RX8130CE_WEEKDAYS_MASK);
	timeptr->tm_mon = bcd2bin(rtc_time.month & RX8130CE_MONTHS_MASK) - RX8130CE_MONTHS_OFFSET;
	timeptr->tm_year = bcd2bin(rtc_time.year & RX8130CE_YEARS_MASK) + RX8130CE_YEARS_OFFSET;
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;

error:
	k_sem_give(&data->lock);
	return rc;
}

static int rx8130ce_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	int rc = 0;
	struct rx8130ce_time rtc_time;
	const struct rx8130ce_config *cfg = dev->config;
	struct rx8130ce_data *data = dev->data;

	rtc_time.second = bin2bcd(timeptr->tm_sec);
	rtc_time.minute  = bin2bcd(timeptr->tm_min);
	rtc_time.hour   = bin2bcd(timeptr->tm_hour);
	rtc_time.weekday = wday2rtc(timeptr->tm_wday);
	rtc_time.day =   bin2bcd(timeptr->tm_mday);
	rtc_time.month = bin2bcd(timeptr->tm_mon + RX8130CE_MONTHS_OFFSET);
	rtc_time.year  = bin2bcd(timeptr->tm_year -
			  (timeptr->tm_year >= RX8130CE_YEARS_OFFSET ? RX8130CE_YEARS_OFFSET : 0));

	k_sem_take(&data->lock, K_FOREVER);

	rc = i2c_burst_write_dt(&cfg->i2c, TIME, (uint8_t *)&rtc_time, sizeof(rtc_time));
	if (rc != 0) {
		LOG_ERR("Failed to write time");
		goto error;
	}
	LOG_DBG("set time: year = %d, mon = %d, mday = %d, hour = %d, min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
error:
	k_sem_give(&data->lock);
	return rc;
}

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
static void rx8130ce_irq_work_handler(struct k_work *work)
{
	int rc;
	const struct device *dev = CONTAINER_OF(work, struct rx8130ce_data, irq_work)->dev;
	struct rx8130ce_data *data = CONTAINER_OF(work, struct rx8130ce_data, irq_work);
	const struct rx8130ce_config *cfg = data->dev->config;
	#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback = NULL;
	void *alarm_user_data = NULL;
	#endif /* CONFIG_RTC_ALARM */
	#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_callback = NULL;
	void *update_user_data = NULL;
	#endif /* CONFIG_RTC_UPDATE */

	k_sem_take(&data->lock, K_FOREVER);
	rc = i2c_burst_read_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to read flag register");
		goto exit;
	}
#ifdef CONFIG_RTC_ALARM
	if ((data->reg.flag & FLAG_AF) != 0) {
		LOG_INF("Alarm triggered");
		alarm_callback = data->alarm_callback;
		alarm_user_data = data->alarm_user_data;
	}
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	if ((data->reg.flag & FLAG_UF) != 0) {
		LOG_INF("Update triggered");
		update_callback = data->update_callback;
		update_user_data = data->update_user_data;
	}
#endif /* CONFIG_RTC_UPDATE */
	/* Clear alarm flags */
	data->reg.flag &= ~(FLAG_AF | FLAG_UF);
	rc = i2c_burst_write_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to clear alarm flag");
		goto exit;
	}
exit:
	k_sem_give(&data->lock);

#ifdef CONFIG_RTC_ALARM
	if (alarm_callback) {
		alarm_callback(dev, 0, alarm_user_data);
	}
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	if (update_callback) {
		update_callback(dev, update_user_data);
	}
#endif /* CONFIG_RTC_UPDATE */
}

static void rx8130ce_irq(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct rx8130ce_data *data = CONTAINER_OF(cb, struct rx8130ce_data, irq_cb);

	LOG_DBG("IRQ-recv");
	k_work_submit(&data->irq_work);
}
#endif /* CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_ALARM
#define RX8130CE_ALARM_MASK (RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | \
				RTC_ALARM_TIME_MASK_MONTHDAY)
static int rx8130ce_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						uint16_t *mask)
{
	const struct rx8130ce_config *cfg = dev->config;

	if (cfg->irq.port == NULL) {
		LOG_ERR("IRQ not configured");
		return -ENOTSUP;
	}

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	*mask = RX8130CE_ALARM_MASK;
	return 0;
}

static int rx8130ce_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
					const struct rtc_time *timeptr)
{
	int rc = 0;
	bool alarm_enabled = false;
	struct rx8130ce_alarm alarm_time;
	struct rx8130ce_data *data = dev->data;
	const struct rx8130ce_config *cfg = dev->config;

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	if ((mask & ~(RX8130CE_ALARM_MASK)) != 0U) {
		LOG_ERR("unsupported alarm field mask 0x%04x", mask);
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	rc = i2c_burst_read_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to read control registers");
		goto error;
	}
	/* Prevent alarm interrupts inadvertently while entering settings/time */
	if ((data->reg.ctrl0 & CTRL0_AIE) != 0) {
		alarm_enabled = true;
		data->reg.ctrl0 &= ~CTRL0_AIE;
		rc = i2c_burst_write_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg,
			  sizeof(data->reg));
		if (rc != 0) {
			LOG_ERR("Failed to write time");
			goto error;
		}
	}

	/* Set alarm */
	if (alarm_enabled) {
		data->reg.ctrl0 |= CTRL0_AIE;
	}

	alarm_time.minute = bin2bcd(timeptr->tm_min);
	alarm_time.hour = bin2bcd(timeptr->tm_hour);
	alarm_time.day = bin2bcd(timeptr->tm_mday);
	data->reg.extension |= EXT_WADA;

	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) == 0U) {
		alarm_time.minute |= ALARM_DISABLE;
	}
	if ((mask & RTC_ALARM_TIME_MASK_HOUR) == 0U) {
		alarm_time.hour |= ALARM_DISABLE;
	}
	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) == 0U) {
		alarm_time.day |= ALARM_DISABLE;
	}

	/* Write alarm time */
	rc = i2c_burst_write_dt(&cfg->i2c, ALARM, (uint8_t *)&alarm_time, sizeof(alarm_time));
	if (rc != 0) {
		LOG_ERR("Failed to write alarm time");
		goto error;
	}

	/* Enable alarm */
	rc = i2c_burst_write_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to write control registers");
		goto error;
	}
error:
	k_sem_give(&data->lock);
	return rc;

}

static int rx8130ce_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
					struct rtc_time *timeptr)
{
	int rc = 0;
	struct rx8130ce_alarm alarm_time;
	struct rx8130ce_data *data = dev->data;
	const struct rx8130ce_config *cfg = dev->config;

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	*mask = 0U;
	memset(timeptr, 0x00, sizeof(*timeptr));
	rc = i2c_burst_read_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to read control registers");
		goto error;
	}

	rc = i2c_burst_read_dt(&cfg->i2c, ALARM, (uint8_t *)&alarm_time, sizeof(alarm_time));
	if (rc != 0) {
		LOG_ERR("Failed to read alarm time");
		goto error;
	}

	timeptr->tm_min = bcd2bin(alarm_time.minute & RX8130CE_MINUTES_MASK);
	timeptr->tm_hour = bcd2bin(alarm_time.hour & RX8130CE_HOURS_MASK);
	if (!(alarm_time.minute & ALARM_DISABLE)) {
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}
	if (!(alarm_time.hour & ALARM_DISABLE)) {
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}
	if (data->reg.extension & EXT_WADA) {
		timeptr->tm_mday = bcd2bin(alarm_time.day);
		if (!(alarm_time.day & ALARM_DISABLE)) {
			*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		}
	} else {
		timeptr->tm_wday = rtc2wday(alarm_time.wday);
		if (!(alarm_time.wday & ALARM_DISABLE)) {
			*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
		}
	}
error:
	k_sem_give(&data->lock);
	return rc;
}

static int rx8130ce_alarm_is_pending(const struct device *dev, uint16_t id)
{
	int rc = 0;
	struct rx8130ce_data *data = dev->data;
	const struct rx8130ce_config *cfg = dev->config;

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	rc = i2c_burst_read_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to read control registers");
		goto error;
	}

	rc = (data->reg.ctrl0 & CTRL0_AIE) != 0;
error:
	k_sem_give(&data->lock);
	return rc;
}

static int rx8130ce_alarm_set_callback(const struct device *dev, uint16_t id,
				       rtc_alarm_callback callback, void *user_data)
{
	int rc = 0;
	struct rx8130ce_data *data = dev->data;
	const struct rx8130ce_config *cfg = dev->config;

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}
	if (cfg->irq.port == NULL) {
		LOG_ERR("IRQ not configured");
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);
	rc = i2c_burst_read_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to read control registers");
		goto exit;
	}
	if (callback == NULL) {
		data->alarm_user_data = NULL;
		data->alarm_callback = NULL;
		data->reg.ctrl0 &= ~CTRL0_AIE;
#ifdef CONFIG_RTC_UPDATE
		if (data->update_callback == NULL) {
#endif
			rc = gpio_pin_interrupt_configure_dt(&cfg->irq, GPIO_INT_DISABLE);
			if (rc != 0) {
				LOG_ERR("Failed to disable interrupt");
				goto exit;
			}
#ifdef CONFIG_RTC_UPDATE
		}
#endif
	} else {
		/* Enable alarm interrupt  & clear Alarm flag */
		data->reg.ctrl0 |= CTRL0_AIE;
		data->reg.flag &= ~FLAG_AF;
		data->alarm_callback = callback;
		data->alarm_user_data = user_data;
		rc = gpio_pin_interrupt_configure_dt(&cfg->irq, GPIO_INT_EDGE_TO_ACTIVE);
		if (rc != 0) {
			LOG_ERR("Failed to configure interrupt");
			goto exit;
		}
	}
	rc = i2c_burst_write_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to write control registers");
		goto exit;
	}
exit:
	k_sem_give(&data->lock);
	return rc;
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
static int rx8130ce_update_set_callback(const struct device *dev, rtc_update_callback callback,
					void *user_data)
{
	int rc = 0;
	const struct rx8130ce_config *cfg = dev->config;
	struct rx8130ce_data *data = dev->data;

	if (cfg->irq.port == NULL) {
		LOG_ERR("IRQ not configured");
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);
	rc = i2c_burst_read_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to read control registers");
		goto exit;
	}
	if (callback == NULL) {
		data->reg.ctrl0 &= ~CTRL0_UIE;
		data->update_user_data = NULL;
		data->update_callback = NULL;

		#ifdef CONFIG_RTC_ALARM
		if (data->alarm_callback == NULL) {
		#endif
			rc = gpio_pin_interrupt_configure_dt(&cfg->irq, GPIO_INT_DISABLE);
			if (rc != 0) {
				LOG_ERR("Failed to disable interrupt");
				goto exit;
			}
		#ifdef CONFIG_RTC_ALARM
		}
		#endif
	} else {
		data->reg.ctrl0 |= CTRL0_UIE;
		data->update_callback = callback;
		data->update_user_data = user_data;
		rc = gpio_pin_interrupt_configure_dt(&cfg->irq, GPIO_INT_EDGE_TO_ACTIVE);
		if (rc != 0) {
			LOG_ERR("Failed to configure interrupt");
			goto exit;
		}
	}
	rc = i2c_burst_write_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to write control registers");
		goto exit;
	}
exit:
	k_sem_give(&data->lock);
	return rc;

}
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
static int rx8130ce_set_calibration(const struct device *dev, int32_t freq_ppb)
{
	int rc;
	const struct rx8130ce_config *cfg = dev->config;
	struct rx8130ce_data *data = dev->data;
	uint8_t offset = 0;

	if (freq_ppb < DIGITAL_OFFSET_MIN || freq_ppb > DIGITAL_OFFSET_MAX) {
		LOG_ERR("Invalid calibration value: %d", freq_ppb);
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	/* Explanation see section 17 of the datasheet */
	if (freq_ppb < 0) {
		offset |= DIGITAL_OFFSET_DTE;
		offset |= DIGITAL_OFFSET_NEG;
		offset |= 128 - (-freq_ppb / DIGITAL_OFFSET_STEP_PPB);
	} else if (freq_ppb > 0) {
		offset |= DIGITAL_OFFSET_DTE;
		offset |= freq_ppb / DIGITAL_OFFSET_STEP_PPB;
	}
	LOG_DBG("set calibration: offset = 0x%02x, from %d", offset, freq_ppb);

	rc = i2c_burst_write_dt(&cfg->i2c, OFFSET, &offset, sizeof(offset));
	if (rc != 0) {
		LOG_ERR("Failed to write calibration value");
		goto exit;
	}
exit:
	k_sem_give(&data->lock);
	return rc;
}

static int rx8130ce_get_calibration(const struct device *dev, int32_t *freq_ppb)
{
	int rc;
	const struct rx8130ce_config *cfg = dev->config;
	struct rx8130ce_data *data = dev->data;
	uint8_t offset;
	*freq_ppb = 0;

	k_sem_take(&data->lock, K_FOREVER);
	rc = i2c_burst_read_dt(&cfg->i2c, OFFSET, &offset, sizeof(offset));
	if (rc != 0) {
		LOG_ERR("Failed to read calibration value");
		goto exit;
	}
	/* Explanation see section 17 of the datasheet */
	if (offset & DIGITAL_OFFSET_DTE) {
		offset &= ~DIGITAL_OFFSET_DTE;
		if (offset & DIGITAL_OFFSET_NEG) {
			*freq_ppb = -((128 - offset) * DIGITAL_OFFSET_STEP_PPB);
		} else {
			*freq_ppb = offset * DIGITAL_OFFSET_STEP_PPB;
		}
	}
	LOG_DBG("get calibration: offset = 0x%02x, freq_ppb = %d", offset, *freq_ppb);

exit:
	k_sem_give(&data->lock);
	return rc;
}
#endif /* CONFIG_RTC_CALIBRATION */


static int rx8130ce_init(const struct device *dev)
{
	int rc;
	const struct rx8130ce_config *cfg = dev->config;
	struct rx8130ce_data *data = dev->data;

	data->dev = dev;
	k_sem_init(&data->lock, 1, 1);
	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* read all control registers */
	rc = i2c_burst_read_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to read control registers");
		return rc;
	}
	data->reg.flag = 0x00;
	data->reg.extension &= ~EXT_TE;


	switch (cfg->clockout_frequency) {
	case 0: /* OFF */
		data->reg.extension |= EXT_FSEL1 | EXT_FSEL0;
		break;
	case 1: /* 1 Hz */
		data->reg.extension &= ~EXT_FSEL0;
		data->reg.extension |= EXT_FSEL1;
		break;
	case 1024: /* 1.024 kHz */
		data->reg.extension |= EXT_FSEL0;
		data->reg.extension &= ~EXT_FSEL1;
		break;
	case 32768: /* 32.768 kHz */
		data->reg.extension &= ~(EXT_FSEL1 | EXT_FSEL0);
		break;
	default:
		LOG_ERR("Invalid clockout frequency option: %d", cfg->clockout_frequency);
		return -EINVAL;
	}

	if (cfg->battery_switchover != 0) {
		/* Enable initial voltage detection, following settings depend
		 * on if the CTRL1_INIEN has been set prior (lifetime)
		 */
		data->reg.ctrl1 |= CTRL1_INIEN;
		rc = i2c_burst_write_dt(&cfg->i2c, CTRL1,
			  (uint8_t *)&data->reg.ctrl1, sizeof(data->reg.ctrl1));
		if (rc != 0) {
			LOG_ERR("Failed to write ctrl1 register");
			return rc;
		}
	}

	switch (cfg->battery_switchover) {
	case 1: /* Power switch on, non rechargeable battery */
		data->reg.ctrl1 |= CTRL1_INIEN;
		break;
	case 2: /* Power switch on, rechargeable battery */
		data->reg.ctrl1 &= ~(CTRL1_INIEN | CTRL1_CHGEN);
		break;
	case 3: /* Power switch on, rechargeable battery, i2c & Fout disabled if VDD < Vdet1 */
		data->reg.ctrl1 |= CTRL1_CHGEN | CTRL1_INIEN;
		break;
	case 4: /* Power switch on, rechargeable battery, i2c & Fout always enabled */
		data->reg.ctrl1 |= CTRL1_CHGEN;
		data->reg.ctrl1 &= ~CTRL1_INIEN;
		break;
	}

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
	k_work_init(&data->irq_work, rx8130ce_irq_work_handler);
	if (cfg->irq.port != NULL) {
		gpio_pin_configure_dt(&cfg->irq, GPIO_INPUT);
		gpio_init_callback(&data->irq_cb, rx8130ce_irq, BIT(cfg->irq.pin));
		rc = gpio_add_callback_dt(&cfg->irq, &data->irq_cb);
		if (rc != 0) {
			LOG_ERR("Failed to add callback");
			return rc;
		}
	}
#endif /* CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE */
	rc = i2c_burst_write_dt(&cfg->i2c, EXTENSION, (uint8_t *)&data->reg, sizeof(data->reg));
	if (rc != 0) {
		LOG_ERR("Failed to write control registers");
		return rc;
	}
	return 0;
}
static DEVICE_API(rtc, rx8130ce_driver_api) = {
	.set_time = rx8130ce_set_time,
	.get_time = rx8130ce_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rx8130ce_alarm_get_supported_fields,
	.alarm_set_time = rx8130ce_alarm_set_time,
	.alarm_get_time = rx8130ce_alarm_get_time,
	.alarm_is_pending = rx8130ce_alarm_is_pending,
	.alarm_set_callback = rx8130ce_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = rx8130ce_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = rx8130ce_set_calibration,
	.get_calibration = rx8130ce_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

#define RX8130CE_INIT(inst)									\
	static const struct rx8130ce_config rx8130ce_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.clockout_frequency = DT_INST_PROP_OR(inst, clockout_frequency, 0),		\
		.battery_switchover = DT_INST_PROP_OR(inst, battery_switchover, 0),		\
		.irq = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}),				\
	};											\
												\
	static struct rx8130ce_data rx8130ce_data_##inst;					\
												\
	DEVICE_DT_INST_DEFINE(inst, &rx8130ce_init, NULL,					\
			      &rx8130ce_data_##inst, &rx8130ce_config_##inst, POST_KERNEL,	\
			      CONFIG_RTC_INIT_PRIORITY, &rx8130ce_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RX8130CE_INIT)
