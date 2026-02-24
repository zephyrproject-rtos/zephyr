/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "rtc_utils.h"

#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pcf85263a);

#define DT_DRV_COMPAT nxp_pcf85263a

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int1_gpios) &&                                                \
	(defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE))
/* Only include IRQ/callback handling if the board routes INTA to a GPIO */
#define PCF85263A_INT1_GPIOS_IN_USE 1
#endif

/* Time/date (RTC mode) */
#define PCF85263A_TIME_DATE_REGISTER 0x01 /* Seconds .. Years */

/* Alarm registers (RTC mode) */
#define PCF85263A_ALARM1_REGISTER        0x08 /* Sec, Min, Hour, Day, Month */
#define PCF85263A_ALARM2_REGISTER        0x0D /* Min, Hour, Weekday */
#define PCF85263A_ALARM_ENABLES_REGISTER 0x10

/* Control/status */
#define PCF85263A_OSCILLATOR_REGISTER 0x25
#define PCF85263A_PIN_IO_REGISTER     0x27
#define PCF85263A_FUNCTION_REGISTER   0x28
#define PCF85263A_INTA_ENABLE_REG     0x29
#define PCF85263A_FLAGS_REGISTER      0x2B

/* Oscillator register bits */
#define PCF85263A_OSC_12_24 BIT(5) /* 0=24h, 1=12h */

/* Function register bits */
#define PCF85263A_FUNC_RTCM BIT(4) /* 0=RTC mode, 1=stop-watch mode */

/* Pin_IO register bits */
#define PCF85263A_PIN_IO_INTAPM_MASK  GENMASK(1, 0)
#define PCF85263A_PIN_IO_INTAPM_INTA  0x2 /* 10b = INTA output */

/* INTA_enable register bits */
#define PCF85263A_INTA_ILPA BIT(7)  /* 0=pulse (default), 1=level */
#define PCF85263A_INTA_A1IE BIT(4)  /* Alarm1 interrupt enable */
#define PCF85263A_INTA_A2IE BIT(3)  /* Alarm2 interrupt enable */

/* Alarm_enables register bits */
#define PCF85263A_AE_SEC_A1E  BIT(0)
#define PCF85263A_AE_MIN_A1E  BIT(1)
#define PCF85263A_AE_HR_A1E   BIT(2)
#define PCF85263A_AE_DAY_A1E  BIT(3)
#define PCF85263A_AE_MON_A1E  BIT(4)
#define PCF85263A_AE_MIN_A2E  BIT(5)
#define PCF85263A_AE_HR_A2E   BIT(6)
#define PCF85263A_AE_WDAY_A2E BIT(7)

#define PCF85263A_AE_MASK_A1 (PCF85263A_AE_SEC_A1E | PCF85263A_AE_MIN_A1E | PCF85263A_AE_HR_A1E | \
			 PCF85263A_AE_DAY_A1E | PCF85263A_AE_MON_A1E)
#define PCF85263A_AE_MASK_A2 (PCF85263A_AE_MIN_A2E | PCF85263A_AE_HR_A2E | PCF85263A_AE_WDAY_A2E)

/* Flags register bits */
#define PCF85263A_FLG_A1F BIT(5)
#define PCF85263A_FLG_A2F BIT(6)

/* Time register masks */
#define PCF85263A_SECONDS_MASK  GENMASK(6, 0) /* Seconds register bit7 = OS */
#define PCF85263A_MINUTES_MASK  GENMASK(6, 0) /* Minutes register bit7 = EMON */
#define PCF85263A_HOURS_MASK    GENMASK(5, 0) /* 24h mode */
#define PCF85263A_DAYS_MASK     GENMASK(5, 0)
#define PCF85263A_WEEKDAYS_MASK GENMASK(2, 0)
#define PCF85263A_MONTHS_MASK   GENMASK(4, 0)

#define PCF85263A_YEARS_OFFSET  100
#define PCF85263A_MONTHS_OFFSET 1

#define PCF85263A_RTC_TIME_MASK                                                                    \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

#define PCF85263A_ALARM1_TIME_MASK                                                                 \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH)

#define PCF85263A_ALARM2_TIME_MASK                                                                 \
	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_WEEKDAY)

struct pcf85263a_config {
	const struct i2c_dt_spec i2c;
#ifdef PCF85263A_INT1_GPIOS_IN_USE
	const struct gpio_dt_spec int1;
#endif
};

#ifdef PCF85263A_INT1_GPIOS_IN_USE
static void callback_work_handler(struct k_work *work);
K_WORK_DEFINE(callback_work, callback_work_handler);
#endif

struct pcf85263a_data {
#ifdef PCF85263A_INT1_GPIOS_IN_USE
    rtc_alarm_callback alarm_callback[2];
    void *alarm_user_data[2];
    const struct device *dev;
    struct gpio_callback int1_callback;
    struct k_work callback_work;
    bool irq_configured;
#else
    uint8_t _unused;
#endif
};

#if defined(CONFIG_RTC_ALARM) || defined(PCF85263A_INT1_GPIOS_IN_USE)
static int pcf85263a_flags_clear(const struct device *dev, uint8_t clear_mask)
{
	const struct pcf85263a_config *config = dev->config;

	/*
	 * Flags register uses an internal AND during write:
	 * writing 0 clears the flag, writing 1 leaves it unchanged.
	 */
	return i2c_reg_write_byte_dt(&config->i2c, PCF85263A_FLAGS_REGISTER, (uint8_t)~clear_mask);
}
#endif

static int pcf85263a_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct pcf85263a_config *config = dev->config;
	uint8_t raw_time[7];
	int ret;

	if (timeptr->tm_year < PCF85263A_YEARS_OFFSET ||
	    timeptr->tm_year > PCF85263A_YEARS_OFFSET + 99) {
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, PCF85263A_RTC_TIME_MASK)) {
		LOG_ERR("invalid time");
		return -EINVAL;
	}

	raw_time[0] = bin2bcd(timeptr->tm_sec);
	raw_time[1] = bin2bcd(timeptr->tm_min);
	raw_time[2] = bin2bcd(timeptr->tm_hour);
	raw_time[3] = bin2bcd(timeptr->tm_mday);
	raw_time[4] = timeptr->tm_wday;
	raw_time[5] = bin2bcd(timeptr->tm_mon + PCF85263A_MONTHS_OFFSET);
	raw_time[6] = bin2bcd(timeptr->tm_year - PCF85263A_YEARS_OFFSET);

	ret = i2c_burst_write_dt(&config->i2c, PCF85263A_TIME_DATE_REGISTER, raw_time, 
						sizeof(raw_time));
	if (ret) {
		LOG_ERR("Error when setting time: %i", ret);
		return ret;
	}

	return 0;
}

static int pcf85263a_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct pcf85263a_config *config = dev->config;
	uint8_t raw_time[7];
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, PCF85263A_TIME_DATE_REGISTER, raw_time, 
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

	timeptr->tm_sec = bcd2bin(raw_time[0] & PCF85263A_SECONDS_MASK);
	timeptr->tm_min = bcd2bin(raw_time[1] & PCF85263A_MINUTES_MASK);
	timeptr->tm_hour = bcd2bin(raw_time[2] & PCF85263A_HOURS_MASK);
	timeptr->tm_mday = bcd2bin(raw_time[3] & PCF85263A_DAYS_MASK);
	timeptr->tm_wday = raw_time[4] & PCF85263A_WEEKDAYS_MASK;
	timeptr->tm_mon = bcd2bin(raw_time[5] & PCF85263A_MONTHS_MASK) - PCF85263A_MONTHS_OFFSET;
	timeptr->tm_year = bcd2bin(raw_time[6]) + PCF85263A_YEARS_OFFSET;
	/* Unknown DST */
	timeptr->tm_isdst = -1;

	return 0;
}

#ifdef PCF85263A_INT1_GPIOS_IN_USE

static void callback_work_handler(struct k_work *work)
{
	struct pcf85263a_data *data = CONTAINER_OF(work, struct pcf85263a_data, callback_work);
	const struct device *rtc = data->dev;
	const struct pcf85263a_config *config = rtc->config;
	uint8_t flags;
	uint8_t clear = 0;
	int err;

	err = i2c_reg_read_byte_dt(&config->i2c, PCF85263A_FLAGS_REGISTER, &flags);
	if (err) {
		LOG_ERR("Error reading flags: %d", err);
		return;
	}

	if (flags & PCF85263A_FLG_A1F) {
		clear |= PCF85263A_FLG_A1F;
		if (data->alarm_callback[0]) {
			data->alarm_callback[0](rtc, 0, data->alarm_user_data[0]);
		} else {
			LOG_WRN("Alarm1 fired but no callback registered");
		}
	}

	if (flags & PCF85263A_FLG_A2F) {
		clear |= PCF85263A_FLG_A2F;
		if (data->alarm_callback[1]) {
			data->alarm_callback[1](rtc, 1, data->alarm_user_data[1]);
		} else {
			LOG_WRN("Alarm2 fired but no callback registered");
		}
	}

	if (clear) {
		(void)pcf85263a_flags_clear(rtc, clear);
	}
}

static void gpio_callback_function(const struct device *dev, struct gpio_callback *cb, 
						uint32_t pins)
{
	struct pcf85263a_data *data = CONTAINER_OF(cb, struct pcf85263a_data, int1_callback);

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	LOG_DBG("PCF85263A interrupt detected");
	k_work_submit(&(data->callback_work));
}

#endif /* PCF85263A_INT1_GPIOS_IN_USE */

#ifdef CONFIG_RTC_ALARM

static int pcf85263a_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						uint16_t *mask)
{
	ARG_UNUSED(dev);

	switch (id) {
	case 0:
		*mask = PCF85263A_ALARM1_TIME_MASK;
		return 0;
	case 1:
		*mask = PCF85263A_ALARM2_TIME_MASK;
		return 0;
	default:
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}
}

static int pcf85263a_alarm1_set_time(const struct device *dev,
                     const struct pcf85263a_config *config,
                     uint16_t mask,
                     const struct rtc_time *timeptr)
{
    uint8_t ae;
    uint8_t regs[5];
    int ret;

    if ((mask & ~PCF85263A_ALARM1_TIME_MASK) != 0U) {
        return -EINVAL;
    }

    if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
        LOG_ERR("invalid alarm1 time");
        return -EINVAL;
    }

    regs[0] = bin2bcd(timeptr->tm_sec) & PCF85263A_SECONDS_MASK;
    regs[1] = bin2bcd(timeptr->tm_min) & PCF85263A_MINUTES_MASK;
    regs[2] = bin2bcd(timeptr->tm_hour) & PCF85263A_HOURS_MASK;
    regs[3] = bin2bcd(timeptr->tm_mday) & PCF85263A_DAYS_MASK;
    regs[4] = bin2bcd(timeptr->tm_mon + PCF85263A_MONTHS_OFFSET) 
							& PCF85263A_MONTHS_MASK;

    ret = i2c_burst_write_dt(&config->i2c, PCF85263A_ALARM1_REGISTER, regs,
								sizeof(regs));
    if (ret) {
        LOG_ERR("Error when setting alarm1 regs: %i", ret);
        return ret;
    }

    ret = i2c_reg_read_byte_dt(&config->i2c, PCF85263A_ALARM_ENABLES_REGISTER, &ae);
    if (ret) {
        LOG_ERR("Error reading alarm enables: %i", ret);
        return ret;
    }

    ae &= ~PCF85263A_AE_MASK_A1;
    if (mask & RTC_ALARM_TIME_MASK_SECOND) {
        ae |= PCF85263A_AE_SEC_A1E;
    }
    if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
        ae |= PCF85263A_AE_MIN_A1E;
    }
    if (mask & RTC_ALARM_TIME_MASK_HOUR) {
        ae |= PCF85263A_AE_HR_A1E;
    }
    if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
        ae |= PCF85263A_AE_DAY_A1E;
    }
    if (mask & RTC_ALARM_TIME_MASK_MONTH) {
        ae |= PCF85263A_AE_MON_A1E;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, PCF85263A_ALARM_ENABLES_REGISTER, ae);
    if (ret) {
        LOG_ERR("Error writing alarm enables: %i", ret);
        return ret;
    }

    /* Clear any previously latched flag */
    (void)pcf85263a_flags_clear(dev, PCF85263A_FLG_A1F);

    return 0;
}

static int pcf85263a_alarm2_set_time(const struct device *dev,
                     const struct pcf85263a_config *config,
                     uint16_t mask,
                     const struct rtc_time *timeptr)
{
    uint8_t ae;
    uint8_t regs[3];
    int ret;

    if ((mask & ~PCF85263A_ALARM2_TIME_MASK) != 0U) {
        return -EINVAL;
    }

    if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
        LOG_ERR("invalid alarm2 time");
        return -EINVAL;
    }

    regs[0] = bin2bcd(timeptr->tm_min) & PCF85263A_MINUTES_MASK;
    regs[1] = bin2bcd(timeptr->tm_hour) & PCF85263A_HOURS_MASK;
    regs[2] = timeptr->tm_wday & PCF85263A_WEEKDAYS_MASK;

    ret = i2c_burst_write_dt(&config->i2c, PCF85263A_ALARM2_REGISTER, regs,
		sizeof(regs));
    if (ret) {
        LOG_ERR("Error when setting alarm2 regs: %i", ret);
        return ret;
    }

    ret = i2c_reg_read_byte_dt(&config->i2c, PCF85263A_ALARM_ENABLES_REGISTER, &ae);
    if (ret) {
        LOG_ERR("Error reading alarm enables: %i", ret);
        return ret;
    }

    ae &= ~PCF85263A_AE_MASK_A2;
    if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
        ae |= PCF85263A_AE_MIN_A2E;
    }
    if (mask & RTC_ALARM_TIME_MASK_HOUR) {
        ae |= PCF85263A_AE_HR_A2E;
    }
    if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
        ae |= PCF85263A_AE_WDAY_A2E;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, PCF85263A_ALARM_ENABLES_REGISTER, ae);
    if (ret) {
        LOG_ERR("Error writing alarm enables: %i", ret);
        return ret;
    }

    /* Clear any previously latched flag */
    (void)pcf85263a_flags_clear(dev, PCF85263A_FLG_A2F);

    return 0;
}

static int pcf85263a_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
                    const struct rtc_time *timeptr)
{
    const struct pcf85263a_config *config = dev->config;

    switch (id) {
    case 0:
        return pcf85263a_alarm1_set_time(dev, config, mask, timeptr);
    case 1:
        return pcf85263a_alarm2_set_time(dev, config, mask, timeptr);
    default:
        LOG_ERR("invalid ID %d", id);
        return -EINVAL;
    }
}

static int pcf85263a_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				    struct rtc_time *timeptr)
{
	const struct pcf85263a_config *config = dev->config;
	uint8_t ae;
	int ret;

	memset(timeptr, 0U, sizeof(*timeptr));
	*mask = 0U;

	ret = i2c_reg_read_byte_dt(&config->i2c, PCF85263A_ALARM_ENABLES_REGISTER, &ae);
	if (ret) {
		LOG_ERR("Error reading alarm enables: %i", ret);
		return ret;
	}

	if (id == 0) {
		uint8_t regs[5];
		ret = i2c_burst_read_dt(&config->i2c, PCF85263A_ALARM1_REGISTER, regs, 
			sizeof(regs));
		if (ret) {
			LOG_ERR("Error reading alarm1 regs: %i", ret);
			return ret;
		}

		if (ae & PCF85263A_AE_SEC_A1E) {
			timeptr->tm_sec = bcd2bin(regs[0] & PCF85263A_SECONDS_MASK);
			*mask |= RTC_ALARM_TIME_MASK_SECOND;
		}
		if (ae & PCF85263A_AE_MIN_A1E) {
			timeptr->tm_min = bcd2bin(regs[1] & PCF85263A_MINUTES_MASK);
			*mask |= RTC_ALARM_TIME_MASK_MINUTE;
		}
		if (ae & PCF85263A_AE_HR_A1E) {
			timeptr->tm_hour = bcd2bin(regs[2] & PCF85263A_HOURS_MASK);
			*mask |= RTC_ALARM_TIME_MASK_HOUR;
		}
		if (ae & PCF85263A_AE_DAY_A1E) {
			timeptr->tm_mday = bcd2bin(regs[3] & PCF85263A_DAYS_MASK);
			*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		}
		if (ae & PCF85263A_AE_MON_A1E) {
			timeptr->tm_mon = bcd2bin(regs[4] & PCF85263A_MONTHS_MASK) - PCF85263A_MONTHS_OFFSET;
			*mask |= RTC_ALARM_TIME_MASK_MONTH;
		}

		return 0;
	}

	if (id == 1) {
		uint8_t regs[3];
		ret = i2c_burst_read_dt(&config->i2c, PCF85263A_ALARM2_REGISTER, regs, sizeof(regs));
		if (ret) {
			LOG_ERR("Error reading alarm2 regs: %i", ret);
			return ret;
		}

		if (ae & PCF85263A_AE_MIN_A2E) {
			timeptr->tm_min = bcd2bin(regs[0] & PCF85263A_MINUTES_MASK);
			*mask |= RTC_ALARM_TIME_MASK_MINUTE;
		}
		if (ae & PCF85263A_AE_HR_A2E) {
			timeptr->tm_hour = bcd2bin(regs[1] & PCF85263A_HOURS_MASK);
			*mask |= RTC_ALARM_TIME_MASK_HOUR;
		}
		if (ae & PCF85263A_AE_WDAY_A2E) {
			timeptr->tm_wday = regs[2] & PCF85263A_WEEKDAYS_MASK;
			*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
		}

		return 0;
	}

	LOG_ERR("invalid ID %d", id);
	return -EINVAL;
}

static int pcf85263a_alarm_is_pending(const struct device *dev, uint16_t id)
{
	const struct pcf85263a_config *config = dev->config;
	uint8_t flags;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, PCF85263A_FLAGS_REGISTER, &flags);
	if (ret) {
		LOG_ERR("Error reading flags: %i", ret);
		return ret;
	}

	if (id == 0) {
		if (flags & PCF85263A_FLG_A1F) {
			ret = pcf85263a_flags_clear(dev, PCF85263A_FLG_A1F);
			if (ret) {
				LOG_ERR("Error clearing A1F: %i", ret);
				return ret;
			}
			return 1;
		}
		return 0;
	}

	if (id == 1) {
		if (flags & PCF85263A_FLG_A2F) {
			ret = pcf85263a_flags_clear(dev, PCF85263A_FLG_A2F);
			if (ret) {
				LOG_ERR("Error clearing A2F: %i", ret);
				return ret;
			}
			return 1;
		}
		return 0;
	}

	LOG_ERR("invalid ID %d", id);
	return -EINVAL;
}

static int pcf85263a_alarm_set_callback(const struct device *dev, uint16_t id,
				       rtc_alarm_callback callback, void *user_data)
{
#ifndef PCF85263A_INT1_GPIOS_IN_USE
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
#else
	const struct pcf85263a_config *config = dev->config;
	struct pcf85263a_data *data = dev->data;
	uint8_t inta_en;
	int ret;

	if (config->int1.port == NULL) {
		return -ENOTSUP;
	}

	if (id > 1) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	data->alarm_callback[id] = callback;
	data->alarm_user_data[id] = user_data;
	data->dev = dev;

	/* Configure the RTC INTA pin as interrupt output (only affects INTA pin). */
	ret = i2c_reg_update_byte_dt(&config->i2c, PCF85263A_PIN_IO_REGISTER,
				     PCF85263A_PIN_IO_INTAPM_MASK, PCF85263A_PIN_IO_INTAPM_INTA);
	if (ret) {
		LOG_ERR("Failed to set INTA pin mode: %d", ret);
		return ret;
	}

	/* Enable/disable alarm interrupt routing to INTA. Keep pulse mode (ILPA=0). */
	ret = i2c_reg_read_byte_dt(&config->i2c, PCF85263A_INTA_ENABLE_REG, &inta_en);
	if (ret) {
		LOG_ERR("Failed to read INTA_enable: %d", ret);
		return ret;
	}

	inta_en &= ~PCF85263A_INTA_ILPA; /* force pulse mode */
	if (id == 0) {
		if (callback) {
			inta_en |= PCF85263A_INTA_A1IE;
		} else {
			inta_en &= ~PCF85263A_INTA_A1IE;
		}
	} else {
		if (callback) {
			inta_en |= PCF85263A_INTA_A2IE;
		} else {
			inta_en &= ~PCF85263A_INTA_A2IE;
		}
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, PCF85263A_INTA_ENABLE_REG, inta_en);
	if (ret) {
		LOG_ERR("Failed to write INTA_enable: %d", ret);
		return ret;
	}

	/* Configure GPIO interrupt once */
	if (!data->irq_configured) {
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
		data->irq_configured = true;
	}

	LOG_DBG("Alarm callback set for id=%u", id);
	return 0;
#endif
}

#endif /* CONFIG_RTC_ALARM */

static DEVICE_API(rtc, pcf85263a_driver_api) = {
	.set_time = pcf85263a_set_time,
	.get_time = pcf85263a_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = pcf85263a_alarm_get_supported_fields,
	.alarm_set_time = pcf85263a_alarm_set_time,
	.alarm_get_time = pcf85263a_alarm_get_time,
	.alarm_is_pending = pcf85263a_alarm_is_pending,
	.alarm_set_callback = pcf85263a_alarm_set_callback,
#endif
};

static int pcf85263a_init(const struct device *dev)
{
	const struct pcf85263a_config *config = dev->config;
	int ret;

#ifdef PCF85263A_INT1_GPIOS_IN_USE
	struct pcf85263a_data *data = dev->data;

	data->callback_work = callback_work;
	data->irq_configured = false;
	memset(data->alarm_callback, 0, sizeof(data->alarm_callback));
	memset(data->alarm_user_data, 0, sizeof(data->alarm_user_data));

	if (!gpio_is_ready_dt(&config->int1)) {
		LOG_ERR("Interrupt GPIO device not ready");
		return -ENODEV;
	}
#endif

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C device not ready: %s", config->i2c.bus->name);
		return -ENODEV;
	}

	/* Ensure RTC mode (not stop-watch mode). */
	ret = i2c_reg_update_byte_dt(&config->i2c, PCF85263A_FUNCTION_REGISTER,
				     PCF85263A_FUNC_RTCM, 0);
	if (ret) {
		LOG_ERR("Failed to select RTC mode: %i", ret);
		return ret;
	}

	/* Ensure 24-hour mode. */
	ret = i2c_reg_update_byte_dt(&config->i2c, PCF85263A_OSCILLATOR_REGISTER,
				     PCF85263A_OSC_12_24, 0);
	if (ret) {
		LOG_ERR("Failed to set 24h mode: %i", ret);
		return ret;
	}

	return 0;
}

#define PCF85263A_INIT(inst)                                                                       \
	static const struct pcf85263a_config pcf85263a_config_##inst = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                               \
		IF_ENABLED(PCF85263A_INT1_GPIOS_IN_USE,                                            \
			(.int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0})))               \
	};                                                                                           \
	static struct pcf85263a_data pcf85263a_data_##inst;                                        \
	DEVICE_DT_INST_DEFINE(inst, &pcf85263a_init, NULL, &pcf85263a_data_##inst,                 \
			      &pcf85263a_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,     \
			      &pcf85263a_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PCF85263A_INIT)
