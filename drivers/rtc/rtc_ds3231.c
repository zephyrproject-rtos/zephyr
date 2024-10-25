/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Gergo Vari <work@varigergo.hu>
 */

/* TODO: implement get_temp */
/* TODO: implement user mode? */
/* TODO: implement aging offset with calibration */
/* TODO: handle century bit, external storage? */

#include <zephyr/drivers/rtc/rtc_ds3231.h>

#include <zephyr/drivers/rtc.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT maxim_ds3231

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ds3231, CONFIG_RTC_LOG_LEVEL);

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#ifdef CONFIG_RTC_ALARM
#define ALARM_COUNT 2
struct alarm {
	rtc_alarm_callback cb;
	void *user_data;
};
#endif

#ifdef CONFIG_RTC_UPDATE
struct update {
	rtc_update_callback cb;
	void *user_data;
};
#endif

struct drv_data {
#ifdef CONFIG_RTC_ALARM
	struct alarm alarms[ALARM_COUNT];
#endif
#ifdef CONFIG_RTC_UPDATE
	struct update update;
#endif
	struct k_sem lock;
	struct gpio_callback isw_cb_data;
	struct k_work work;
	const struct device *dev;
};

struct drv_conf {
	struct i2c_dt_spec i2c_bus;
	struct gpio_dt_spec freq_32k_gpios;
	struct gpio_dt_spec isw_gpios;
};

static int i2c_set_registers(const struct device *dev, uint8_t start_reg, const uint8_t *buf, const size_t buf_size)
{
	struct drv_data *data = dev->data;
	const struct drv_conf *config = dev->config;

	(void)k_sem_take(&data->lock, K_FOREVER);
	int err = i2c_burst_write_dt(&config->i2c_bus, start_reg, buf, buf_size);
	k_sem_give(&data->lock);

	return err;
}
static int i2c_get_registers(const struct device *dev, uint8_t start_reg, uint8_t *buf, const size_t buf_size)
{
	struct drv_data *data = dev->data;
	const struct drv_conf *config = dev->config;
	
	/* FIXME: bad start_reg/buf_size values break i2c for that run */

	(void)k_sem_take(&data->lock, K_FOREVER);
	int err = i2c_burst_read_dt(&config->i2c_bus, start_reg, buf, buf_size);
	k_sem_give(&data->lock);

	return err;
}
static int i2c_modify_register(const struct device *dev, uint8_t reg, uint8_t *buf, const uint8_t bitmask)
{
	int err;
	if (bitmask != 255) {
		uint8_t og_buf = 0;
		err = i2c_get_registers(dev, reg, &og_buf, 1);
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
	err = i2c_set_registers(dev, reg, buf, 1);
	return err;
}

enum freq {FREQ_1000, FREQ_1024, FREQ_4096, FREQ_8192};
struct ctrl {
	bool en_osc;

	bool conv;

	enum freq sqw_freq;

	bool intctrl;
	bool en_alarm_1;
	bool en_alarm_2;
};
static int ctrl_to_buf(const struct ctrl *ctrl, uint8_t *buf)
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
static int modify_ctrl(const struct device *dev, const struct ctrl *ctrl, const uint8_t bitmask)
{
	uint8_t reg = DS3231_REG_CTRL;
	uint8_t buf = 0;

	int err = ctrl_to_buf(ctrl, &buf);
	if (err != 0) {
		return err;
	}

	return i2c_modify_register(dev, reg, &buf, bitmask);
}
static int set_ctrl(const struct device *dev, const struct ctrl *ctrl)
{
	return modify_ctrl(dev, ctrl, 255);
}

struct ctrl_sts {
	bool osf;
	bool en_32khz;
	bool bsy;
	bool a1f;
	bool a2f;
};
static int ctrl_sts_to_buf(const struct ctrl_sts *ctrl, uint8_t *buf)
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
static int modify_ctrl_sts(const struct device *dev, const struct ctrl_sts *ctrl, const uint8_t bitmask)
{
	const uint8_t reg = DS3231_REG_CTRL_STS;
	uint8_t buf = 0;

	int err = ctrl_sts_to_buf(ctrl, &buf);
	if (err != 0) {
		return err;
	}

	return i2c_modify_register(dev, reg, &buf, bitmask);
}
static int get_ctrl_sts(const struct device *dev, uint8_t *buf)
{
	return i2c_get_registers(dev, DS3231_REG_CTRL_STS, buf, 1);
}
static int set_ctrl_sts(const struct device *dev, const struct ctrl_sts *ctrl)
{
	return modify_ctrl_sts(dev, ctrl, 255);
}

struct settings {
	bool osc; /* bit 0 */
	bool intctrl_or_sqw; /* bit 1 */
	enum freq freq_sqw; /* bit 2 */
	bool freq_32khz; /* bit 3 */
	bool alarm_1; /* bit 4 */
	bool alarm_2; /* bit 5 */
};
static int modify_settings(const struct device *dev, struct settings *conf, uint8_t mask)
{
	struct ctrl ctrl;
	uint8_t ctrl_mask = 0;

	struct ctrl_sts ctrl_sts;
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

	int err = modify_ctrl(dev, &ctrl, ctrl_mask);
	if (err != 0) {
		LOG_ERR("Couldn't set control register.");
		return -EIO;
	}
	err = modify_ctrl_sts(dev, &ctrl_sts, ctrl_sts_mask);
	if (err != 0) {
		LOG_ERR("Couldn't set status register.");
		return -EIO;
	}
	return 0;
}
static int set_settings(const struct device *dev, struct settings *conf)
{
	return modify_settings(dev, conf, 255);
}

static int rtc_time_to_buf(const struct rtc_time *tm, uint8_t *buf)
{
	buf[0] = bin2bcd(tm->tm_sec)  & DS3231_BITS_TIME_SECONDS;
	buf[1] = bin2bcd(tm->tm_min)  & DS3231_BITS_TIME_MINUTES;
	buf[2] = bin2bcd(tm->tm_hour) & DS3231_BITS_TIME_HOURS;
	buf[3] = bin2bcd(tm->tm_wday) & DS3231_BITS_TIME_DAY_OF_WEEK;
	buf[4] = bin2bcd(tm->tm_mday) & DS3231_BITS_TIME_DATE;
	buf[5] = bin2bcd(tm->tm_mon)  & DS3231_BITS_TIME_MONTH;

	/* here modulo 100 returns the last two digits of the year,
	   as the DS3231 chip can only store year data for 0-99,
	   hitting that ceiling can be detected with the century bit. */
	/* TODO: figure out a way to store the WHOLE year, not just the last 2 digits */
	buf[6] = bin2bcd((tm->tm_year % 100)) & DS3231_BITS_TIME_YEAR;
	return 0;
}
static int set_time(const struct device *dev, const struct rtc_time *tm)
{
	int buf_size = 7;
	uint8_t buf[buf_size];
	int err = rtc_time_to_buf(tm, buf);
	if (err != 0) {
		return err;
	}

	return i2c_set_registers(dev, DS3231_REG_TIME_SECONDS, buf, buf_size);
}

static int reset_rtc_time(struct rtc_time *tm)
{
	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour= 0;
	tm->tm_wday = 0;
	tm->tm_mday = 0;
	tm->tm_mon = 0;
	tm->tm_year = 0;
	tm->tm_nsec = 0;
	tm->tm_isdst = -1;
	tm->tm_yday = -1;
	return 0;
}
static int buf_to_rtc_time(const uint8_t *buf, struct rtc_time *timeptr)
{
	int err = reset_rtc_time(timeptr);
	if (err != 0) {
		return -EINVAL;
	}

	timeptr->tm_sec = bcd2bin(buf[0] & DS3231_BITS_TIME_SECONDS);
	timeptr->tm_min = bcd2bin(buf[1] & DS3231_BITS_TIME_MINUTES);
	
	int hour = buf[2] & DS3231_BITS_TIME_HOURS;
	if (hour & DS3231_BITS_TIME_12HR) {
		bool pm = hour & DS3231_BITS_TIME_PM;
		hour &= ~DS3231_BITS_TIME_12HR;
		hour &= ~DS3231_BITS_TIME_PM;
		timeptr->tm_hour = bcd2bin(hour + 12*pm);
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
static int get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const size_t buf_size = 7;
	uint8_t buf[buf_size];
	int err = i2c_get_registers(dev, DS3231_REG_TIME_SECONDS, buf, buf_size);
	if (err != 0) {
		return err;
	}

	return buf_to_rtc_time(buf, timeptr);
}

#ifdef CONFIG_RTC_ALARM
struct alarm_details {
	uint8_t start_reg;
	size_t buf_size;
};
static struct alarm_details alarms[] = {
    {DS3231_REG_ALARM_1_SECONDS, 4},
    {DS3231_REG_ALARM_2_MINUTES, 3}
};
static int alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	*mask = RTC_ALARM_TIME_MASK_MONTHDAY
		| RTC_ALARM_TIME_MASK_WEEKDAY
		| RTC_ALARM_TIME_MASK_HOUR
		| RTC_ALARM_TIME_MASK_MINUTE;

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

static int rtc_time_to_alarm_buf(const struct rtc_time *tm, int id, const uint16_t mask, uint8_t *buf)
{
	if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) && (mask & RTC_ALARM_TIME_MASK_MONTHDAY)) {
		LOG_ERR("rtc_time_to_alarm_buf: Mask is invalid (%d)!\n", mask);
		return -EINVAL;
	}
	if (id < 0 || id >= 2) {
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

static int modify_alarm_time(const struct device *dev, int id, const struct rtc_time *tm, const uint8_t mask)
{
	if (id >= ALARM_COUNT) {
		return -EINVAL;
	}
	struct alarm_details details = alarms[id];
	uint8_t start_reg = details.start_reg;
	size_t buf_size = details.buf_size;

	uint8_t buf[buf_size];
	int err = rtc_time_to_alarm_buf(tm, id, mask, buf);
	if (err != 0) {
		return err;
	}

	return i2c_set_registers(dev, start_reg, buf, buf_size);
}

static int modify_alarm_state(const struct device *dev, uint16_t id, bool state)
{
	struct settings conf;
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

	return modify_settings(dev, &conf, mask);
}
static int alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask, const struct rtc_time *timeptr)
{
	if (mask == 0) {
		return modify_alarm_state(dev, id, false);
	}

	int err = modify_alarm_state(dev, id, true);
	if (err != 0) {
		return err;
	}

	return modify_alarm_time(dev, id, timeptr, mask);
}

static int alarm_buf_to_rtc_time(uint8_t *buf, int id, struct rtc_time *tm, uint16_t *mask)
{
	int err = reset_rtc_time(tm);
	if (err != 0) {
		return -EINVAL;
	}

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
static int alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask, struct rtc_time *timeptr)
{
	if (id >= ALARM_COUNT) {
		return -EINVAL;
	}
	struct alarm_details details = alarms[id];
	uint8_t start_reg = details.start_reg;
	size_t buf_size = details.buf_size;

	uint8_t buf[4];
	int err = i2c_get_registers(dev, start_reg, buf, buf_size);
	if (err != 0) {
		return err;
	}

	return alarm_buf_to_rtc_time(buf, id, timeptr, mask);
}

static int alarm_is_pending(const struct device *dev, uint16_t id)
{
	uint8_t buf;
	int err = get_ctrl_sts(dev, &buf);
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
		const struct ctrl_sts ctrl = {
			.a1f = false,
			.a2f = false
		};
		err = modify_ctrl_sts(dev, &ctrl, mask);
		if (err != 0) {
			return err;
		}
	}
	return state;
}

static int get_alarm_states(const struct device *dev, bool *states)
{
	int err = 0;
	for (int i = 0; i < ALARM_COUNT; i++) {
		states[i] = alarm_is_pending(dev, i);
		if (!(states[i] == 0 || states[i] == 1)) {
			states[i] = -EINVAL;
			err = -EINVAL;
		}
	}
	return err;
}

static int alarm_set_callback(const struct device *dev, uint16_t id, rtc_alarm_callback cb, void *user_data)
{
	if (id < 0 || id >= ALARM_COUNT) {
		return -EINVAL;
	}

	struct drv_data *data = dev->data;
	data->alarms[id] = (struct alarm){
		cb,
		user_data
	};

	return 0;
}

static void check_alarms(const struct device *dev) {
	struct drv_data *data = dev->data;

	bool states[2];
	get_alarm_states(dev, states);

	for (int i = 0; i < ALARM_COUNT; i++) {
		if (states[i]) {
			if (data->alarms[i].cb) {
				data->alarms[i].cb(dev, i, data->alarms[i].user_data);
			}
		}
	}
}
static int init_alarms(struct drv_data *data) {
	data->alarms[0] = (struct alarm){NULL, NULL};
	data->alarms[1] = (struct alarm){NULL, NULL};
	return 0;
}
#endif

#ifdef CONFIG_RTC_UPDATE
static int init_update(struct drv_data *data) {
	data->update = (struct update){NULL, NULL};
	return 0;
}
static int update_set_callback(const struct device *dev, rtc_update_callback cb, void *user_data)
{
	struct drv_data *data = dev->data;
	data->update = (struct update){
		cb,
		user_data
	};
	return 0;
}
static void update_callback(const struct device *dev) {
	struct drv_data *data = dev->data;
	if (data->update.cb) {
		data->update.cb(dev, data->update.user_data);
	}
}
#endif /* CONFIG_RTC_UPDATE */

#if defined(CONFIG_RTC_UPDATE) || defined(CONFIG_RTC_ALARM)
static void isw_h(struct k_work *work)
{
	struct drv_data *data = CONTAINER_OF(work, struct drv_data, work);
	const struct device *dev = data->dev;
	
#ifdef CONFIG_RTC_UPDATE
	update_callback(dev);
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_ALARM
	check_alarms(dev);
#endif /* CONFIG_RTC_ALARM */

}
static void isw_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct drv_data *data = CONTAINER_OF(cb, struct drv_data, isw_cb_data);

	k_work_submit(&data->work);
}
static int init_isw(const struct drv_conf *config, struct drv_data *data) {
	if (!gpio_is_ready_dt(&config->isw_gpios)) {
		LOG_ERR("ISW GPIO pin is not ready.");
		return -ENODEV;
	}

	k_work_init(&data->work, isw_h);

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

	gpio_init_callback(&data->isw_cb_data, isw_isr, BIT((config->isw_gpios).pin));
	err = gpio_add_callback((config->isw_gpios).port, &data->isw_cb_data);
	if (err != 0) {
		LOG_ERR("Couldn't add ISW interrupt callback.");
		return err;
	}

	return 0;
}
#endif /* defined(CONFIG_RTC_UPDATE) || defined(CONFIG_RTC_ALARM) */

static const struct rtc_driver_api driver_api = {
	.set_time = set_time,
	.get_time = get_time,

#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = alarm_get_supported_fields,
	.alarm_set_time = alarm_set_time,
	.alarm_get_time = alarm_get_time,
	.alarm_is_pending = alarm_is_pending,
	.alarm_set_callback = alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = update_set_callback,
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
	/*.set_calibration = set_calibration,
	.get_calibration = get_calibration,*/
#endif /* CONFIG_RTC_CALIBRATION */
};

static int init_settings(const struct device *dev, const struct drv_conf *config) {
	struct settings conf = {
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
	int err = modify_settings(dev, &conf, mask);
	if (err != 0) {
		return err;
	}
	return 0;
}

static int init_i2c(const struct drv_conf *config, struct drv_data *data) {
	k_sem_init(&data->lock, 1, 1);
	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready.");
		return -ENODEV;
	}
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int pm_action(const struct device *dev, enum pm_device_action action)
{
	int err = 0;
	switch (action) {
		case PM_DEVICE_ACTION_SUSPEND: {
			struct settings conf = {
				.osc = true,
				.intctrl_or_sqw = false,
				.freq_sqw = FREQ_1000,
				.freq_32khz = false
			};
			uint8_t mask = 255 & ~DS3231_BITS_STS_ALARM_1 & ~DS3231_BITS_STS_ALARM_2;
			err = modify_settings(dev, &conf, mask);
			if (err != 0) {
				return err;
			}
			break;
		}
		case PM_DEVICE_ACTION_RESUME: {
			/* TODO: trigger a temp CONV */
			const struct drv_conf *config = dev->config;
			err = init_settings(dev, config);
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

static int init(const struct device *dev)
{
	int err = 0;

	const struct drv_conf *config = dev->config;
	struct drv_data *data = dev->data;

#ifdef CONFIG_RTC_ALARM
	err = init_alarms(data);
	if (err != 0) {
		LOG_ERR("Failed to init alarms.");
		return err;
	}
#endif

#ifdef CONFIG_RTC_UPDATE
	err = init_update(data);
	if (err != 0) {
		LOG_ERR("Failed to init update callback.");
		return err;
	}
#endif
	
	err = init_i2c(config, data);
	if (err != 0) {
		LOG_ERR("Failed to init I2C.");
		return err;
	}

	err = init_settings(dev, config);
	if (err != 0) {
		LOG_ERR("Failed to init settings.");
		return err;
	}

#if defined(CONFIG_RTC_UPDATE) || defined(CONFIG_RTC_ALARM)
	data->dev = dev;
	err = init_isw(config, data);
	if (err != 0) {
		LOG_ERR("Initing ISW interrupt failed!");
		return err;
	}
#endif /* defined(CONFIG_RTC_UPDATE) || defined(CONFIG_RTC_ALARM) */

	return 0;
}

#define DS3231_DEFINE(inst)                                                                        \
        static struct drv_data drv_data_##inst;                                              \
        static const struct drv_conf drv_conf_##inst = {                             \
                .i2c_bus = I2C_DT_SPEC_INST_GET(inst),                                             \
                .isw_gpios = GPIO_DT_SPEC_INST_GET(inst, isw_gpios),                               \
                .freq_32k_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, freq_32khz_gpios, {NULL})             \
        };											\
	PM_DEVICE_DT_INST_DEFINE(inst, pm_action);  \
        DEVICE_DT_INST_DEFINE(inst, &init, NULL, &drv_data_##inst,                   \
                              &drv_conf_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,      \
                              &driver_api);


DT_INST_FOREACH_STATUS_OKAY(DS3231_DEFINE)
