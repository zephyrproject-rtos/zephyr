/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Gergo Vari <work@gergovari.com>
 */

/* TODO: implement user mode? */
/* TODO: implement aging offset with calibration */
/* TODO: handle century bit, external storage? */

#include <zephyr/drivers/mfd/ds3231.h>
#include <zephyr/drivers/rtc/rtc_ds3231.h>

#include <zephyr/drivers/rtc.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(RTC_DS3231, CONFIG_RTC_LOG_LEVEL);

#include <zephyr/drivers/gpio.h>

#define DT_DRV_COMPAT maxim_ds3231_rtc

#ifdef CONFIG_RTC_ALARM
#define ALARM_COUNT 2
struct rtc_ds3231_alarm {
	rtc_alarm_callback cb;
	void *user_data;
};
#endif

#ifdef CONFIG_RTC_UPDATE
struct rtc_ds3231_update {
	rtc_update_callback cb;
	void *user_data;
};
#endif

struct rtc_ds3231_data {
#ifdef CONFIG_RTC_ALARM
	struct rtc_ds3231_alarm alarms[ALARM_COUNT];
#endif
#ifdef CONFIG_RTC_UPDATE
	struct rtc_ds3231_update update;
#endif
	struct k_sem lock;
	struct gpio_callback isw_cb_data;
	struct k_work work;
	const struct device *dev;
};

struct rtc_ds3231_conf {
	const struct device *mfd;
	struct gpio_dt_spec freq_32k_gpios;
	struct gpio_dt_spec isw_gpios;
};

static int rtc_ds3231_modify_register(const struct device *dev, uint8_t reg, uint8_t *buf,
				      const uint8_t bitmask)
{
	int err;
	const struct rtc_ds3231_conf *config = dev->config;

	if (bitmask != 255) {
		uint8_t og_buf = 0;

		err = mfd_ds3231_i2c_get_registers(config->mfd, reg, &og_buf, 1);
		if (err != 0) {
			return err;
		}
		og_buf &= ~bitmask;
		*buf &= bitmask;
		og_buf |= *buf;
		*buf = og_buf;
	}
	if (err != 0) {
		return err;
	}
	err = mfd_ds3231_i2c_set_registers(config->mfd, reg, buf, 1);
	return err;
}

enum rtc_ds3231_freq {
	FREQ_1000,
	FREQ_1024,
	FREQ_4096,
	FREQ_8192
};
struct rtc_ds3231_ctrl {
	bool en_osc;

	bool conv;

	enum rtc_ds3231_freq sqw_freq;

	bool intctrl;
	bool en_alarm_1;
	bool en_alarm_2;
};
static int rtc_ds3231_ctrl_to_buf(const struct rtc_ds3231_ctrl *ctrl, uint8_t *buf)
{
	if (ctrl->en_alarm_1) {
		*buf |= DS3231_BITS_CTRL_ALARM_1_EN;
	}

	if (ctrl->en_alarm_2) {
		*buf |= DS3231_BITS_CTRL_ALARM_2_EN;
	}

	switch (ctrl->sqw_freq) {
	case FREQ_1000:
		break;
	case FREQ_1024:
		*buf |= DS3231_BITS_CTRL_RS1;
		break;
	case FREQ_4096:
		*buf |= DS3231_BITS_CTRL_RS2;
		break;
	case FREQ_8192:
		*buf |= DS3231_BITS_CTRL_RS1;
		*buf |= DS3231_BITS_CTRL_RS2;
		break;
	}
	if (ctrl->intctrl) {
		*buf |= DS3231_BITS_CTRL_INTCTRL;
	} else { /* enable sqw */
		*buf |= DS3231_BITS_CTRL_BBSQW;
	}

	if (ctrl->conv) {
		*buf |= DS3231_BITS_CTRL_CONV;
	}

	if (!ctrl->en_osc) { /* active low */
		*buf |= DS3231_BITS_CTRL_EOSC;
	}
	return 0;
}
static int rtc_ds3231_modify_ctrl(const struct device *dev, const struct rtc_ds3231_ctrl *ctrl,
				  const uint8_t bitmask)
{
	uint8_t reg = DS3231_REG_CTRL;
	uint8_t buf = 0;

	int err = rtc_ds3231_ctrl_to_buf(ctrl, &buf);

	if (err != 0) {
		return err;
	}

	return rtc_ds3231_modify_register(dev, reg, &buf, bitmask);
}

struct rtc_ds3231_ctrl_sts {
	bool osf;
	bool en_32khz;
	bool bsy;
	bool a1f;
	bool a2f;
};
static int rtc_ds3231_ctrl_sts_to_buf(const struct rtc_ds3231_ctrl_sts *ctrl, uint8_t *buf)
{
	if (ctrl->a1f) {
		*buf |= DS3231_BITS_CTRL_STS_ALARM_1_FLAG;
	}
	if (ctrl->a2f) {
		*buf |= DS3231_BITS_CTRL_STS_ALARM_2_FLAG;
	}
	if (ctrl->osf) {
		*buf |= DS3231_BITS_CTRL_STS_OSF;
	}
	if (ctrl->en_32khz) {
		*buf |= DS3231_BITS_CTRL_STS_32_EN;
	}
	if (ctrl->bsy) {
		*buf |= DS3231_BITS_CTRL_STS_BSY;
	}
	return 0;
}
static int rtc_ds3231_modify_ctrl_sts(const struct device *dev,
				      const struct rtc_ds3231_ctrl_sts *ctrl, const uint8_t bitmask)
{
	const uint8_t reg = DS3231_REG_CTRL_STS;
	uint8_t buf = 0;

	int err = rtc_ds3231_ctrl_sts_to_buf(ctrl, &buf);

	if (err != 0) {
		return err;
	}

	return rtc_ds3231_modify_register(dev, reg, &buf, bitmask);
}

#ifdef CONFIG_RTC_ALARM
static int rtc_ds3231_get_ctrl_sts(const struct device *dev, uint8_t *buf)
{
	const struct rtc_ds3231_conf *config = dev->config;

	return mfd_ds3231_i2c_get_registers(config->mfd, DS3231_REG_CTRL_STS, buf, 1);
}
#endif /* CONFIG_RTC_ALARM */

struct rtc_ds3231_settings {
	bool osc;                      /* bit 0 */
	bool intctrl_or_sqw;           /* bit 1 */
	enum rtc_ds3231_freq freq_sqw; /* bit 2 */
	bool freq_32khz;               /* bit 3 */
	bool alarm_1;                  /* bit 4 */
	bool alarm_2;                  /* bit 5 */
};
static int rtc_ds3231_modify_settings(const struct device *dev, struct rtc_ds3231_settings *conf,
				      uint8_t mask)
{
	struct rtc_ds3231_ctrl ctrl = {};
	uint8_t ctrl_mask = 0;

	struct rtc_ds3231_ctrl_sts ctrl_sts = {};
	uint8_t ctrl_sts_mask = 0;

	if (mask & DS3231_BITS_STS_OSC) {
		ctrl.en_osc = conf->osc;
		ctrl_mask |= DS3231_BITS_CTRL_EOSC;
	}
	if (mask & DS3231_BITS_STS_INTCTRL) {
		ctrl.intctrl = !conf->intctrl_or_sqw;
		ctrl_mask |= DS3231_BITS_CTRL_BBSQW;
	}
	if (mask & DS3231_BITS_STS_SQW) {
		ctrl.sqw_freq = conf->freq_sqw;
		ctrl_mask |= DS3231_BITS_CTRL_RS1;
		ctrl_mask |= DS3231_BITS_CTRL_RS2;
	}
	if (mask & DS3231_BITS_STS_32KHZ) {
		ctrl_sts.en_32khz = conf->freq_32khz;
		ctrl_sts_mask |= DS3231_BITS_CTRL_STS_32_EN;
	}
	if (mask & DS3231_BITS_STS_ALARM_1) {
		ctrl.en_alarm_1 = conf->alarm_1;
		ctrl_mask |= DS3231_BITS_CTRL_ALARM_1_EN;
	}
	if (mask & DS3231_BITS_STS_ALARM_2) {
		ctrl.en_alarm_2 = conf->alarm_2;
		ctrl_mask |= DS3231_BITS_CTRL_ALARM_2_EN;
	}

	ctrl.conv = false;

	int err = rtc_ds3231_modify_ctrl(dev, &ctrl, ctrl_mask);

	if (err != 0) {
		LOG_ERR("Couldn't set control register.");
		return -EIO;
	}
	err = rtc_ds3231_modify_ctrl_sts(dev, &ctrl_sts, ctrl_sts_mask);
	if (err != 0) {
		LOG_ERR("Couldn't set status register.");
		return -EIO;
	}
	return 0;
}

static int rtc_ds3231_rtc_time_to_buf(const struct rtc_time *tm, uint8_t *buf)
{
	buf[0] = bin2bcd(tm->tm_sec) & DS3231_BITS_TIME_SECONDS;
	buf[1] = bin2bcd(tm->tm_min) & DS3231_BITS_TIME_MINUTES;
	buf[2] = bin2bcd(tm->tm_hour) & DS3231_BITS_TIME_HOURS;
	buf[3] = bin2bcd(tm->tm_wday) & DS3231_BITS_TIME_DAY_OF_WEEK;
	buf[4] = bin2bcd(tm->tm_mday) & DS3231_BITS_TIME_DATE;
	buf[5] = bin2bcd(tm->tm_mon) & DS3231_BITS_TIME_MONTH;

	/* here modulo 100 returns the last two digits of the year,
	 * as the DS3231 chip can only store year data for 0-99,
	 * hitting that ceiling can be detected with the century bit.
	 */

	/* TODO: figure out a way to store the WHOLE year, not just the last 2 digits. */
	buf[6] = bin2bcd((tm->tm_year % 100)) & DS3231_BITS_TIME_YEAR;
	return 0;
}
static int rtc_ds3231_set_time(const struct device *dev, const struct rtc_time *tm)
{
	const struct rtc_ds3231_conf *config = dev->config;

	int buf_size = 7;
	uint8_t buf[buf_size];
	int err = rtc_ds3231_rtc_time_to_buf(tm, buf);

	if (err != 0) {
		return err;
	}

	return mfd_ds3231_i2c_set_registers(config->mfd, DS3231_REG_TIME_SECONDS, buf, buf_size);
}

static void rtc_ds3231_reset_rtc_time(struct rtc_time *tm)
{
	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;
	tm->tm_wday = 0;
	tm->tm_mday = 0;
	tm->tm_mon = 0;
	tm->tm_year = 0;
	tm->tm_nsec = 0;
	tm->tm_isdst = -1;
	tm->tm_yday = -1;
}
static int rtc_ds3231_buf_to_rtc_time(const uint8_t *buf, struct rtc_time *timeptr)
{
	rtc_ds3231_reset_rtc_time(timeptr);

	timeptr->tm_sec = bcd2bin(buf[0] & DS3231_BITS_TIME_SECONDS);
	timeptr->tm_min = bcd2bin(buf[1] & DS3231_BITS_TIME_MINUTES);

	int hour = buf[2] & DS3231_BITS_TIME_HOURS;

	if (hour & DS3231_BITS_TIME_12HR) {
		bool pm = hour & DS3231_BITS_TIME_PM;

		hour &= ~DS3231_BITS_TIME_12HR;
		hour &= ~DS3231_BITS_TIME_PM;
		timeptr->tm_hour = bcd2bin(hour + 12 * pm);
	} else {
		timeptr->tm_hour = bcd2bin(hour);
	}

	timeptr->tm_wday = bcd2bin(buf[3] & DS3231_BITS_TIME_DAY_OF_WEEK);
	timeptr->tm_mday = bcd2bin(buf[4] & DS3231_BITS_TIME_DATE);
	timeptr->tm_mon = bcd2bin(buf[5] & DS3231_BITS_TIME_MONTH);
	timeptr->tm_year = bcd2bin(buf[6] & DS3231_BITS_TIME_YEAR);

	/* FIXME: we will always just set us to 20xx for year */
	timeptr->tm_year = timeptr->tm_year + 100;

	return 0;
}
static int rtc_ds3231_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_ds3231_conf *config = dev->config;

	const size_t buf_size = 7;
	uint8_t buf[buf_size];
	int err = mfd_ds3231_i2c_get_registers(config->mfd, DS3231_REG_TIME_SECONDS, buf, buf_size);

	if (err != 0) {
		return err;
	}

	return rtc_ds3231_buf_to_rtc_time(buf, timeptr);
}

#ifdef CONFIG_RTC_ALARM
struct rtc_ds3231_alarm_details {
	uint8_t start_reg;
	size_t buf_size;
};
static struct rtc_ds3231_alarm_details alarms[] = {{DS3231_REG_ALARM_1_SECONDS, 4},
						   {DS3231_REG_ALARM_2_MINUTES, 3}};
static int rtc_ds3231_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						 uint16_t *mask)
{
	*mask = RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_WEEKDAY |
		RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MINUTE;

	switch (id) {
	case 0:
		*mask |= RTC_ALARM_TIME_MASK_SECOND;
		break;
	case 1:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rtc_ds3231_rtc_time_to_alarm_buf(const struct rtc_time *tm, int id, const uint16_t mask,
					    uint8_t *buf)
{
	if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) && (mask & RTC_ALARM_TIME_MASK_MONTHDAY)) {
		LOG_ERR("rtc_time_to_alarm_buf: Mask is invalid (%d)!\n", mask);
		return -EINVAL;
	}
	if (id < 0 || id >= ALARM_COUNT) {
		LOG_ERR("rtc_time_to_alarm_buf: Alarm ID is out of range (%d)!\n", id);
		return -EINVAL;
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		buf[1] = bin2bcd(tm->tm_min) & DS3231_BITS_TIME_MINUTES;
	} else {
		buf[1] |= DS3231_BITS_ALARM_RATE;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		buf[2] = bin2bcd(tm->tm_hour) & DS3231_BITS_TIME_HOURS;
	} else {
		buf[2] |= DS3231_BITS_ALARM_RATE;
	}

	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		buf[3] = bin2bcd(tm->tm_wday) & DS3231_BITS_TIME_DAY_OF_WEEK;
		buf[3] |= DS3231_BITS_ALARM_DATE_W_OR_M;
	} else if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		buf[3] = bin2bcd(tm->tm_mday) & DS3231_BITS_TIME_DATE;
	} else {
		buf[3] |= DS3231_BITS_ALARM_RATE;
	}

	switch (id) {
	case 0:
		if (mask & RTC_ALARM_TIME_MASK_SECOND) {
			buf[0] = bin2bcd(tm->tm_sec) & DS3231_BITS_TIME_SECONDS;
		} else {
			buf[0] |= DS3231_BITS_ALARM_RATE;
		}
		break;
	case 1:
		if (mask & RTC_ALARM_TIME_MASK_SECOND) {
			return -EINVAL;
		}

		for (int i = 0; i < 3; i++) {
			buf[i] = buf[i + 1];
		}

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rtc_ds3231_modify_alarm_time(const struct device *dev, int id, const struct rtc_time *tm,
					const uint8_t mask)
{
	const struct rtc_ds3231_conf *config = dev->config;

	if (id >= ALARM_COUNT) {
		return -EINVAL;
	}
	struct rtc_ds3231_alarm_details details = alarms[id];
	uint8_t start_reg = details.start_reg;
	size_t buf_size = details.buf_size;

	uint8_t buf[buf_size];
	int err = rtc_ds3231_rtc_time_to_alarm_buf(tm, id, mask, buf);

	if (err != 0) {
		return err;
	}

	return mfd_ds3231_i2c_set_registers(config->mfd, start_reg, buf, buf_size);
}

static int rtc_ds3231_modify_alarm_state(const struct device *dev, uint16_t id, bool state)
{
	struct rtc_ds3231_settings conf;
	uint8_t mask = 0;

	switch (id) {
	case 0:
		conf.alarm_1 = state;
		mask = DS3231_BITS_STS_ALARM_1;
		break;
	case 1:
		conf.alarm_2 = state;
		mask = DS3231_BITS_STS_ALARM_2;
		break;
	default:
		return -EINVAL;
	}

	return rtc_ds3231_modify_settings(dev, &conf, mask);
}
static int rtc_ds3231_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				     const struct rtc_time *timeptr)
{
	if (mask == 0) {
		return rtc_ds3231_modify_alarm_state(dev, id, false);
	}

	int err = rtc_ds3231_modify_alarm_state(dev, id, true);

	if (err != 0) {
		return err;
	}

	return rtc_ds3231_modify_alarm_time(dev, id, timeptr, mask);
}

static int rtc_ds3231_alarm_buf_to_rtc_time(uint8_t *buf, int id, struct rtc_time *tm,
					    uint16_t *mask)
{
	rtc_ds3231_reset_rtc_time(tm);

	if (id < 0 || id > 1) {
		return -EINVAL;
	} else if (id == 1) {
		/* shift to the right to match original func */
		for (int i = 3; i > 0; i--) {
			buf[i] = buf[i - 1];
		}
		buf[0] = 0;
	}

	*mask = 0;
	if (!(buf[1] & DS3231_BITS_ALARM_RATE)) {
		tm->tm_min = bcd2bin(buf[1] & DS3231_BITS_TIME_MINUTES);
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}
	if (!(buf[2] & DS3231_BITS_ALARM_RATE)) {
		tm->tm_hour = bcd2bin(buf[2] & DS3231_BITS_TIME_HOURS);
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}
	if (!(buf[3] & DS3231_BITS_ALARM_RATE)) {
		if (buf[3] & DS3231_BITS_ALARM_DATE_W_OR_M) {
			tm->tm_wday = bcd2bin(buf[3] & DS3231_BITS_TIME_DAY_OF_WEEK);
			*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
		} else {
			tm->tm_mday = bcd2bin(buf[3] & DS3231_BITS_TIME_DATE);
			*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		}
	}
	if (!(buf[0] & DS3231_BITS_ALARM_RATE)) {
		tm->tm_sec = bcd2bin(buf[0] & DS3231_BITS_TIME_SECONDS);
		*mask |= RTC_ALARM_TIME_MASK_SECOND;
	}

	if ((*mask & RTC_ALARM_TIME_MASK_WEEKDAY) && (*mask & RTC_ALARM_TIME_MASK_MONTHDAY)) {
		return -EINVAL;
	}

	return 0;
}
static int rtc_ds3231_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				     struct rtc_time *timeptr)
{
	const struct rtc_ds3231_conf *config = dev->config;

	if (id >= ALARM_COUNT) {
		return -EINVAL;
	}
	struct rtc_ds3231_alarm_details details = alarms[id];
	uint8_t start_reg = details.start_reg;
	size_t buf_size = details.buf_size;

	uint8_t buf[4];
	int err = mfd_ds3231_i2c_get_registers(config->mfd, start_reg, buf, buf_size);

	if (err != 0) {
		return err;
	}

	return rtc_ds3231_alarm_buf_to_rtc_time(buf, id, timeptr, mask);
}

static int rtc_ds3231_alarm_is_pending(const struct device *dev, uint16_t id)
{
	uint8_t buf;
	int err = rtc_ds3231_get_ctrl_sts(dev, &buf);

	if (err != 0) {
		return err;
	}

	uint8_t mask = 0;

	switch (id) {
	case 0:
		mask |= DS3231_BITS_CTRL_STS_ALARM_1_FLAG;
		break;
	case 1:
		mask |= DS3231_BITS_CTRL_STS_ALARM_2_FLAG;
		break;
	default:
		return -EINVAL;
	}

	bool state = buf & mask;

	if (state) {
		const struct rtc_ds3231_ctrl_sts ctrl = {.a1f = false, .a2f = false};

		err = rtc_ds3231_modify_ctrl_sts(dev, &ctrl, mask);
		if (err != 0) {
			return err;
		}
	}
	return state;
}

static int rtc_ds3231_get_alarm_states(const struct device *dev, bool *states)
{
	int err = 0;

	for (int i = 0; i < ALARM_COUNT; i++) {
		states[i] = rtc_ds3231_alarm_is_pending(dev, i);
		if (!(states[i] == 0 || states[i] == 1)) {
			states[i] = -EINVAL;
			err = -EINVAL;
		}
	}
	return err;
}

static int rtc_ds3231_alarm_set_callback(const struct device *dev, uint16_t id,
					 rtc_alarm_callback cb, void *user_data)
{
	if (id < 0 || id >= ALARM_COUNT) {
		return -EINVAL;
	}

	struct rtc_ds3231_data *data = dev->data;

	data->alarms[id] = (struct rtc_ds3231_alarm){cb, user_data};

	return 0;
}

static void rtc_ds3231_check_alarms(const struct device *dev)
{
	struct rtc_ds3231_data *data = dev->data;

	bool states[2];

	rtc_ds3231_get_alarm_states(dev, states);

	for (int i = 0; i < ALARM_COUNT; i++) {
		if (states[i]) {
			if (data->alarms[i].cb) {
				data->alarms[i].cb(dev, i, data->alarms[i].user_data);
			}
		}
	}
}
static int rtc_ds3231_init_alarms(struct rtc_ds3231_data *data)
{
	data->alarms[0] = (struct rtc_ds3231_alarm){NULL, NULL};
	data->alarms[1] = (struct rtc_ds3231_alarm){NULL, NULL};
	return 0;
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
static int rtc_ds3231_init_update(struct rtc_ds3231_data *data)
{
	data->update = (struct rtc_ds3231_update){NULL, NULL};
	return 0;
}
static int rtc_ds3231_update_set_callback(const struct device *dev, rtc_update_callback cb,
					  void *user_data)
{
	struct rtc_ds3231_data *data = dev->data;

	data->update = (struct rtc_ds3231_update){cb, user_data};
	return 0;
}
static void rtc_ds3231_update_callback(const struct device *dev)
{
	struct rtc_ds3231_data *data = dev->data;

	if (data->update.cb) {
		data->update.cb(dev, data->update.user_data);
	}
}
#endif /* CONFIG_RTC_UPDATE */

#if defined(CONFIG_RTC_UPDATE) || defined(CONFIG_RTC_ALARM)
static void rtc_ds3231_isw_h(struct k_work *work)
{
	struct rtc_ds3231_data *data = CONTAINER_OF(work, struct rtc_ds3231_data, work);
	const struct device *dev = data->dev;

#ifdef CONFIG_RTC_UPDATE
	rtc_ds3231_update_callback(dev);
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_ALARM
	rtc_ds3231_check_alarms(dev);
#endif /* CONFIG_RTC_ALARM */
}
static void rtc_ds3231_isw_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct rtc_ds3231_data *data = CONTAINER_OF(cb, struct rtc_ds3231_data, isw_cb_data);

	k_work_submit(&data->work);
}
static int rtc_ds3231_init_isw(const struct rtc_ds3231_conf *config, struct rtc_ds3231_data *data)
{
	if (!gpio_is_ready_dt(&config->isw_gpios)) {
		LOG_ERR("ISW GPIO pin is not ready.");
		return -ENODEV;
	}

	k_work_init(&data->work, rtc_ds3231_isw_h);

	int err = gpio_pin_configure_dt(&(config->isw_gpios), GPIO_INPUT);

	if (err != 0) {
		LOG_ERR("Couldn't configure ISW GPIO pin.");
		return err;
	}
	err = gpio_pin_interrupt_configure_dt(&(config->isw_gpios), GPIO_INT_EDGE_TO_ACTIVE);
	if (err != 0) {
		LOG_ERR("Couldn't configure ISW interrupt.");
		return err;
	}

	gpio_init_callback(&data->isw_cb_data, rtc_ds3231_isw_isr, BIT((config->isw_gpios).pin));
	err = gpio_add_callback((config->isw_gpios).port, &data->isw_cb_data);
	if (err != 0) {
		LOG_ERR("Couldn't add ISW interrupt callback.");
		return err;
	}

	return 0;
}
#endif /* defined(CONFIG_RTC_UPDATE) || defined(CONFIG_RTC_ALARM) */

static DEVICE_API(rtc, driver_api) = {
	.set_time = rtc_ds3231_set_time,
	.get_time = rtc_ds3231_get_time,

#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_ds3231_alarm_get_supported_fields,
	.alarm_set_time = rtc_ds3231_alarm_set_time,
	.alarm_get_time = rtc_ds3231_alarm_get_time,
	.alarm_is_pending = rtc_ds3231_alarm_is_pending,
	.alarm_set_callback = rtc_ds3231_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = rtc_ds3231_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
/*.set_calibration = set_calibration,
 * .get_calibration = get_calibration,
 */
#endif /* CONFIG_RTC_CALIBRATION */
};

static int rtc_ds3231_init_settings(const struct device *dev, const struct rtc_ds3231_conf *config)
{
	struct rtc_ds3231_settings conf = {
		.osc = true,
#ifdef CONFIG_RTC_UPDATE
		.intctrl_or_sqw = false,
		.freq_sqw = FREQ_1000,
#else
		.intctrl_or_sqw = true,
#endif
		.freq_32khz = config->freq_32k_gpios.port,
	};
	uint8_t mask = 255 & ~DS3231_BITS_STS_ALARM_1 & ~DS3231_BITS_STS_ALARM_2;
	int err = rtc_ds3231_modify_settings(dev, &conf, mask);

	if (err != 0) {
		return err;
	}
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int rtc_ds3231_pm_action(const struct device *dev, enum pm_device_action action)
{
	int err = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND: {
		struct rtc_ds3231_settings conf = {.osc = true,
						   .intctrl_or_sqw = false,
						   .freq_sqw = FREQ_1000,
						   .freq_32khz = false};
		uint8_t mask = 255 & ~DS3231_BITS_STS_ALARM_1 & ~DS3231_BITS_STS_ALARM_2;

		err = rtc_ds3231_modify_settings(dev, &conf, mask);
		if (err != 0) {
			return err;
		}
		break;
	}
	case PM_DEVICE_ACTION_RESUME: {
		/* TODO: trigger a temp CONV */
		const struct rtc_ds3231_conf *config = dev->config;

		err = rtc_ds3231_init_settings(dev, config);
		if (err != 0) {
			return err;
		}
		break;
	}
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static int rtc_ds3231_init(const struct device *dev)
{
	int err = 0;

	const struct rtc_ds3231_conf *config = dev->config;
	struct rtc_ds3231_data __maybe_unused *data = dev->data;

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

#ifdef CONFIG_RTC_ALARM
	err = rtc_ds3231_init_alarms(data);
	if (err != 0) {
		LOG_ERR("Failed to init alarms.");
		return err;
	}
#endif

#ifdef CONFIG_RTC_UPDATE
	err = rtc_ds3231_init_update(data);
	if (err != 0) {
		LOG_ERR("Failed to init update callback.");
		return err;
	}
#endif

	err = rtc_ds3231_init_settings(dev, config);
	if (err != 0) {
		LOG_ERR("Failed to init settings.");
		return err;
	}

#if defined(CONFIG_RTC_UPDATE) || defined(CONFIG_RTC_ALARM)
	data->dev = dev;
	err = rtc_ds3231_init_isw(config, data);
	if (err != 0) {
		LOG_ERR("Initing ISW interrupt failed!");
		return err;
	}
#endif /* defined(CONFIG_RTC_UPDATE) || defined(CONFIG_RTC_ALARM) */

	return 0;
}

#define RTC_DS3231_DEFINE(inst)                                                                    \
	static struct rtc_ds3231_data rtc_ds3231_data_##inst;                                      \
	static const struct rtc_ds3231_conf rtc_ds3231_conf_##inst = {                             \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.isw_gpios = GPIO_DT_SPEC_INST_GET(inst, isw_gpios),                               \
		.freq_32k_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, freq_32khz_gpios, {NULL})};       \
	PM_DEVICE_DT_INST_DEFINE(inst, rtc_ds3231_pm_action);                                      \
	DEVICE_DT_INST_DEFINE(inst, &rtc_ds3231_init, PM_DEVICE_DT_INST_GET(inst),                 \
			      &rtc_ds3231_data_##inst, &rtc_ds3231_conf_##inst, POST_KERNEL,       \
			      CONFIG_RTC_DS3231_INIT_PRIORITY, &driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_DS3231_DEFINE)
