/*
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Copyright (c) 2024 Gergo Vari <work@varigergo.hu>
 */

/* TODO: implement alarm */
/* TODO: implement modifiable settings */
/* TODO: implement configurable settings */
/* TODO: implement get_temp */
/* TODO: implement 24h/ampm modes */
/* TODO: handle century bit */
/* TODO: decide if we need to deal with aging offset */
/* TODO: decide if we need to deal with CONV */
/* TODO: implement device power management */

#include <zephyr/drivers/rtc/rtc_ds3231.h>

#include <zephyr/drivers/rtc.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT maxim_ds3231

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ds3231, CONFIG_RTC_LOG_LEVEL);

#include <zephyr/drivers/i2c.h>

struct ds3231_drv_conf {
	struct i2c_dt_spec i2c_bus;
};
struct ds3231_data {
	struct k_spinlock lock;
};

static int i2c_set_registers(const struct device *dev, uint8_t start_reg, const uint8_t *buf, const size_t buf_size) 
{
	struct ds3231_data *data = dev->data;
	const struct ds3231_drv_conf *config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int err = i2c_burst_write_dt(&config->i2c_bus, start_reg, buf, buf_size);

	k_spin_unlock(&data->lock, key);
	return err;
}
static int i2c_get_registers(const struct device *dev, uint8_t start_reg, uint8_t *buf, const size_t buf_size) 
{
	struct ds3231_data *data = dev->data;
	const struct ds3231_drv_conf *config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int err = i2c_burst_read_dt(&config->i2c_bus, start_reg, buf, buf_size);

	k_spin_unlock(&data->lock, key);
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
struct ds3231_ctrl {
	bool en_osc;

	bool conv;

	enum freq sqw_freq;

	bool intctrl;
	bool en_alarm_1;
	bool en_alarm_2;
};
static int ds3231_ctrl_to_buf(const struct ds3231_ctrl *ctrl, uint8_t *buf) 
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
static int ds3231_modify_ctrl(const struct device *dev, const struct ds3231_ctrl *ctrl, const uint8_t bitmask) 
{
	uint8_t reg = DS3231_REG_CTRL;
	uint8_t buf = 0;

	int err = ds3231_ctrl_to_buf(ctrl, &buf);
	if (err != 0) {
		return err;
	}

	return i2c_modify_register(dev, reg, &buf, bitmask);
}
static int ds3231_set_ctrl(const struct device *dev, const struct ds3231_ctrl *ctrl) 
{
	return ds3231_modify_ctrl(dev, ctrl, 255);
} 

struct ds3231_ctrl_sts {
	bool osf;
	bool en_32khz;
	bool bsy;
	bool a1f;
	bool a2f;
};
static int ds3231_ctrl_sts_to_buf(const struct ds3231_ctrl_sts *ctrl, uint8_t *buf) 
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
static int ds3231_modify_ctrl_sts(const struct device *dev, const struct ds3231_ctrl_sts *ctrl, const uint8_t bitmask) 
{
	const uint8_t reg = DS3231_REG_CTRL_STS;
	uint8_t buf = 0;

	int err = ds3231_ctrl_sts_to_buf(ctrl, &buf);
	if (err != 0) {
		return err;
	}

	return i2c_modify_register(dev, reg, &buf, bitmask);
}
static int ds3231_set_ctrl_sts(const struct device *dev, const struct ds3231_ctrl_sts *ctrl) 
{
	return ds3231_modify_ctrl_sts(dev, ctrl, 255);
} 

struct ds3231_settings {
	bool osc;
	bool intctrl_or_sqw;
	enum freq freq_sqw;
	bool freq_32khz;
	bool alarm_1;
	bool alarm_2;
};
static int ds3231_set_settings(const struct device *dev, const struct ds3231_settings *conf) 
{
	const struct ds3231_ctrl ctrl = {
		conf->osc,	
		false, 
		conf->freq_sqw,
		conf->intctrl_or_sqw, 
		conf->alarm_1,
		conf->alarm_2 
	};

	const struct ds3231_ctrl_sts ctrl_sts = {
		false, 
		conf->freq_32khz, 
		false, 
		false, 
		false
	};

	int err = ds3231_set_ctrl(dev, &ctrl);
	if (err != 0) {
		LOG_ERR("Couldn't set control register.");
		return -EIO;
	}
	err = ds3231_set_ctrl_sts(dev, &ctrl_sts);
	if (err != 0) {
		LOG_ERR("Couldn't set status register.");
		return -EIO;
	}
	return 0;
}

static int rtc_time_to_alarm_buf(const struct rtc_time *tm, int id, const uint8_t mask, uint8_t *buf)
{
	if (((mask & RTC_ALARM_TIME_MASK_WEEKDAY) && (mask & RTC_ALARM_TIME_MASK_MONTHDAY)) || (id < 0 || id >= 2)) {
		return -EINVAL;
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		buf[1] = bin2bcd(tm->tm_min) & DS3231_BITS_TIME_MINUTES;
		buf[1] |= DS3231_BITS_ALARM_RATE;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		buf[2] = bin2bcd(tm->tm_hour) & DS3231_BITS_TIME_HOURS;
		buf[2] |= DS3231_BITS_ALARM_RATE;
	}
	
	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		buf[3] = bin2bcd(tm->tm_wday) & DS3231_BITS_TIME_DAY_OF_WEEK;
		buf[3] |= DS3231_BITS_ALARM_DATE_W_OR_M;
		buf[3] |= DS3231_BITS_ALARM_RATE;
	} else if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		buf[3] = bin2bcd(tm->tm_mday) & DS3231_BITS_TIME_DATE;
		buf[3] |= DS3231_BITS_ALARM_RATE;
	}

	switch (id) {
		case 0:
			buf[0] = bin2bcd(tm->tm_sec) & DS3231_BITS_TIME_SECONDS;
			break;
		case 1:
			/* shift the array to the left */
			for (int i = 2; i > 0; i--) {
				buf[i - 1] = buf[i];
			}
			break;
		default:
			return -EINVAL;
	}

	return 0;
}
static int modify_alarm_time(const struct device *dev, int id, const struct rtc_time *tm, const uint8_t mask)
{
	uint8_t start_reg;
	size_t buf_size;

	switch (id) {
		case 0:
			start_reg = DS3231_REG_ALARM_1_SECONDS;
			buf_size = 4;
			break;
		case 1:
			start_reg = DS3231_REG_ALARM_2_MINUTES;
			buf_size = 3;
			break;
		default:
			return -EINVAL;
	}
	
	uint8_t buf[buf_size];
	int err = rtc_time_to_alarm_buf(tm, id, mask, buf);
	if (err != 0) {
		return err;
	}

	return i2c_set_registers(dev, start_reg, buf, buf_size);
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

static int get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const size_t buf_size = 7;
	uint8_t buf[buf_size];
	int err = i2c_get_registers(dev, DS3231_REG_TIME_SECONDS, buf, buf_size);
	if (err != 0) {
		return err;
	}

	timeptr->tm_sec = bcd2bin(buf[0] & DS3231_BITS_TIME_SECONDS);
	timeptr->tm_min = bcd2bin(buf[1] & DS3231_BITS_TIME_MINUTES);
	timeptr->tm_hour = bcd2bin(buf[2] & DS3231_BITS_TIME_HOURS);
	timeptr->tm_wday = bcd2bin(buf[3] & DS3231_BITS_TIME_DAY_OF_WEEK);
	timeptr->tm_mday = bcd2bin(buf[4] & DS3231_BITS_TIME_DATE);
	timeptr->tm_mon = bcd2bin(buf[5] & DS3231_BITS_TIME_MONTH);
	timeptr->tm_year = bcd2bin(buf[6] & DS3231_BITS_TIME_YEAR);
	timeptr->tm_year = timeptr->tm_year + 100; /* FIXME: we will always just set us to 20xx for year */
	
	timeptr->tm_nsec = 0;
	timeptr->tm_isdst = -1;
	timeptr->tm_yday = -1;

	return 0;
}

static int alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	*mask |= RTC_ALARM_TIME_MASK_YEAR;
	*mask |= RTC_ALARM_TIME_MASK_MONTH;
	*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
	*mask |= RTC_ALARM_TIME_MASK_HOUR;
	*mask |= RTC_ALARM_TIME_MASK_MINUTE;
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

static int modify_alarm_state(const struct device *dev, uint16_t id, bool state) 
{
	struct ds3231_ctrl ctrl;
	uint8_t ctrl_mask = 0;
	
	switch (id) {
		case 0:
			ctrl.en_alarm_1 = state;
			ctrl_mask |= DS3231_BITS_CTRL_ALARM_1_EN;
			break;
		case 1:
			ctrl.en_alarm_2 = state;
			ctrl_mask |= DS3231_BITS_CTRL_ALARM_2_EN;
			break;
		default:
			return -EINVAL;
	}
	
	/* TODO: move to modifiable settings when implemented */
	return ds3231_modify_ctrl(dev, &ctrl, ctrl_mask);
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

static const struct rtc_driver_api ds3231_driver_api = {
	.set_time = set_time,
	.get_time = get_time,

#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = alarm_get_supported_fields,
	.alarm_set_time = alarm_set_time,
	/*.alarm_get_time = alarm_get_time,*/
	/*.alarm_is_pending = alarm_is_pending,*/
	/*.alarm_set_callback = alarm_set_callback,*/
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	/*.update_set_callback = update_set_callback,*/
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
	/*.set_calibration = set_calibration,
	.get_calibration = get_calibration,*/
#endif /* CONFIG_RTC_CALIBRATION */
};

static int ds3231_init(const struct device *dev)
{
	const struct ds3231_drv_conf *config = dev->config;
	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready.");
		return -ENODEV;
	}
	
	/* the comments below specify the settings we default to */
	struct ds3231_settings conf;
	conf.osc = true;
	conf.intctrl_or_sqw = true;
	conf.freq_sqw = FREQ_1000;
	conf.freq_32khz = false;

	/* TODO: don't turn off alarms, only modify settings and leave these alone */
	conf.alarm_1 = false;
	conf.alarm_2 = false;
	
	int err = ds3231_set_settings(dev, &conf);
	
	return err;
}

#define DS3231_DEFINE(inst)                                                                        \
	static struct ds3231_data ds3231_data_##inst;                                              \
	static const struct ds3231_drv_conf ds3231_drv_conf_##inst = {                                 \
		.i2c_bus = I2C_DT_SPEC_INST_GET(inst),                                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &ds3231_init, NULL, &ds3231_data_##inst,                       \
			      &ds3231_drv_conf_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,        \
			      &ds3231_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DS3231_DEFINE)
