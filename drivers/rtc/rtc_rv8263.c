/* Copyright (c) 2024 Daniel Kampert
 * Author: Daniel Kampert <DanielKampert@kampis-Elektroecke.de>
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include "rtc_utils.h"

#define RV8263C8_REGISTER_CONTROL_1     0x00
#define RV8263C8_REGISTER_CONTROL_2     0x01
#define RV8263C8_REGISTER_OFFSET        0x02
#define RV8263C8_REGISTER_RAM           0x03
#define RV8263C8_REGISTER_SECONDS       0x04
#define RV8263C8_REGISTER_MINUTES       0x05
#define RV8263C8_REGISTER_HOURS         0x06
#define RV8263C8_REGISTER_DATE          0x07
#define RV8263C8_REGISTER_WEEKDAY       0x08
#define RV8263C8_REGISTER_MONTH         0x09
#define RV8263C8_REGISTER_YEAR          0x0A
#define RV8263C8_REGISTER_SECONDS_ALARM 0x0B
#define RV8263C8_REGISTER_MINUTES_ALARM 0x0C
#define RV8263C8_REGISTER_HOURS_ALARM   0x0D
#define RV8263C8_REGISTER_DATE_ALARM    0x0E
#define RV8263C8_REGISTER_WEEKDAY_ALARM 0x0F
#define RV8263C8_REGISTER_TIMER_VALUE   0x10
#define RV8263C8_REGISTER_TIMER_MODE    0x11

#define RV8263_BM_FAST_MODE                 (0x01 << 7)
#define RV8263_BM_NORMAL_MODE               (0x00 << 7)
#define RV8263C8_BM_24H_MODE_ENABLE         (0x00 << 1)
#define RV8263C8_BM_24H_MODE_DISABLE        (0x00 << 1)
#define RV8263C8_BM_CLOCK_ENABLE            (0x00 << 5)
#define RV8263C8_BM_CLOCK_DISABLE           (0x01 << 5)
#define RV8263C8_BM_ALARM_INT_ENABLE        (0x01 << 7)
#define RV8263C8_BM_ALARM_INT_DISABLE       (0x00 << 7)
#define RV8263C8_BM_MINUTE_INT_ENABLE       (0x01 << 5)
#define RV8263C8_BM_MINUTE_INT_DISABLE      (0x00 << 5)
#define RV8263C8_BM_HALF_MINUTE_INT_ENABLE  (0x01 << 4)
#define RV8263C8_BM_HALF_MINUTE_INT_DISABLE (0x00 << 4)
#define RV8263C8_BM_ALARM_ENABLE            (0x00 << 7)
#define RV8263C8_BM_ALARM_DISABLE           (0x01 << 7)
#define RV8263C8_BM_AF                      (0x01 << 6)
#define RV8263C8_BM_TF                      (0x01 << 3)
#define RV8263_BM_MODE                      (0x01 << 7)
#define RV8263_BM_TD_1HZ                    (0x02 << 3)
#define RV8263_BM_TE_ENABLE                 (0x01 << 2)
#define RV8263_BM_TIE_ENABLE                (0x01 << 1)
#define RV8263_BM_TI_TP_PULSE               (0x01 << 0)
#define RV8263_BM_OS                        (0x01 << 7)
#define RV8263C8_BM_SOFTWARE_RESET          (0x58)
#define RV8263C8_BM_REGISTER_OFFSET         0x7F
#define RV8263_YEAR_OFFSET                  (2000 - 1900)

#define SECONDS_BITS  GENMASK(6, 0)
#define MINUTES_BITS  GENMASK(7, 0)
#define HOURS_BITS    GENMASK(5, 0)
#define DATE_BITS     GENMASK(5, 0)
#define MONTHS_BITS   GENMASK(4, 0)
#define WEEKDAY_BITS  GENMASK(2, 0)
#define YEAR_BITS     GENMASK(7, 0)
#define VALIDATE_24HR BIT(6)

#define MIN_SEC       0
#define MAX_SEC       59
#define MIN_MIN       0
#define MAX_MIN       59
#define MIN_HOUR      0
#define MAX_HOUR      23
#define MAX_WDAY      7
#define MIN_WDAY      1
#define MAX_MDAY      31
#define MIN_MDAY      1
#define MAX_MON       12
#define MIN_MON       1
#define MIN_YEAR_DIFF 0
#define MAX_YEAR_DIFF 99

#define DT_DRV_COMPAT microcrystal_rv_8263_c8

LOG_MODULE_REGISTER(microcrystal_rv8263c8, CONFIG_RTC_LOG_LEVEL);

struct rv8263c8_config {
	struct i2c_dt_spec i2c_bus;
	uint32_t clkout;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	struct gpio_dt_spec int_gpio;
#endif
};

struct rv8263c8_data {
	struct k_sem lock;

#if (CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE) && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct device *dev;
	struct gpio_callback gpio_cb;
#endif

#if CONFIG_RTC_ALARM && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	rtc_alarm_callback alarm_cb;
	void *alarm_cb_data;
	struct k_work alarm_work;
#endif

#if CONFIG_RTC_UPDATE && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	rtc_update_callback update_cb;
	void *update_cb_data;
	struct k_work update_work;
#endif
};

static int rv8263c8_update_disable_timer(const struct device *dev)
{
	int err;
	uint8_t buf[2];
	const struct rv8263c8_config *config = dev->config;

	/* Value 0 disables the timer. */
	buf[0] = RV8263C8_REGISTER_TIMER_VALUE;
	buf[1] = 0;
	err = i2c_write_dt(&config->i2c_bus, buf, 2);
	if (err < 0) {
		return err;
	}

	buf[0] = RV8263C8_REGISTER_TIMER_MODE;
	return i2c_write_dt(&config->i2c_bus, buf, 2);
}

#if (CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE) && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static void rv8263c8_gpio_callback_handler(const struct device *p_port, struct gpio_callback *p_cb,
					   gpio_port_pins_t pins)
{
	ARG_UNUSED(pins);
	ARG_UNUSED(p_port);

	struct rv8263c8_data *data = CONTAINER_OF(p_cb, struct rv8263c8_data, gpio_cb);

#if CONFIG_RTC_ALARM
	k_work_submit(&data->alarm_work);
#endif

#if CONFIG_RTC_UPDATE
	k_work_submit(&data->update_work);
#endif
}
#endif

#if CONFIG_RTC_ALARM && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static void rv8263c8_alarm_worker(struct k_work *p_work)
{
	struct rv8263c8_data *data = CONTAINER_OF(p_work, struct rv8263c8_data, alarm_work);
	const struct rv8263c8_config *config = data->dev->config;

	LOG_DBG("Process alarm worker from interrupt");

	if (data->alarm_cb != NULL) {
		uint8_t reg;

		i2c_reg_read_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_CONTROL_2, &reg);

		if (reg & RV8263C8_BM_AF) {
			reg &= ~RV8263C8_BM_AF;

			LOG_DBG("Calling alarm callback");
			data->alarm_cb(data->dev, 0, data->alarm_cb_data);

			i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_CONTROL_2, reg);
		}
	}
}
#endif

#if CONFIG_RTC_UPDATE && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static int rv8263c8_update_enable_timer(const struct device *dev)
{
	int err;
	const struct rv8263c8_config *config = dev->config;
	uint8_t buf[2];

	/* Set the timer preload value for 1 second. */
	buf[0] = RV8263C8_REGISTER_TIMER_VALUE;
	buf[1] = 1;
	err = i2c_write_dt(&config->i2c_bus, buf, 2);
	if (err < 0) {
		return err;
	}

	buf[0] = RV8263C8_REGISTER_TIMER_MODE;
	buf[1] = RV8263_BM_TD_1HZ | RV8263_BM_TE_ENABLE | RV8263_BM_TIE_ENABLE |
		 RV8263_BM_TI_TP_PULSE;
	return i2c_write_dt(&config->i2c_bus, buf, 2);
}

static void rv8263c8_update_worker(struct k_work *p_work)
{
	uint8_t reg;
	struct rv8263c8_data *data = CONTAINER_OF(p_work, struct rv8263c8_data, update_work);
	const struct rv8263c8_config *config = data->dev->config;

	LOG_DBG("Process update worker from interrupt");

	if (data->update_cb != NULL) {
		i2c_reg_read_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_CONTROL_2, &reg);

		if (reg & RV8263C8_BM_TF) {
			LOG_DBG("Calling update callback");
			data->update_cb(data->dev, data->update_cb_data);
		}
	}

	rv8263c8_update_enable_timer(data->dev);
	i2c_reg_update_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_CONTROL_2, RV8263C8_BM_TF,
			       RV8263C8_BM_TF);
}
#endif

static int rv8263c8_time_set(const struct device *dev, const struct rtc_time *timeptr)
{
	uint8_t regs[8];
	const struct rv8263c8_config *config = dev->config;

	if (timeptr == NULL || (timeptr->tm_year < RV8263_YEAR_OFFSET)) {
		LOG_ERR("invalid time");
		return -EINVAL;
	}

	LOG_DBG("Set time: year = %u, mon = %u, mday = %u, wday = %u, hour = %u, min = %u, sec = "
		"%u",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	regs[0] = RV8263C8_REGISTER_SECONDS;
	regs[1] = bin2bcd(timeptr->tm_sec) & SECONDS_BITS;
	regs[2] = bin2bcd(timeptr->tm_min) & MINUTES_BITS;
	regs[3] = bin2bcd(timeptr->tm_hour) & HOURS_BITS;
	regs[4] = bin2bcd(timeptr->tm_mday) & DATE_BITS;
	regs[5] = bin2bcd(timeptr->tm_wday) & WEEKDAY_BITS;
	regs[6] = bin2bcd(timeptr->tm_mon) & MONTHS_BITS;
	regs[7] = bin2bcd(timeptr->tm_year - RV8263_YEAR_OFFSET) & YEAR_BITS;

	return i2c_write_dt(&config->i2c_bus, regs, 8);
}

static int rv8263c8_time_get(const struct device *dev, struct rtc_time *timeptr)
{
	int err;
	uint8_t regs[7];
	const struct rv8263c8_config *config = dev->config;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	err = i2c_burst_read_dt(&config->i2c_bus, RV8263C8_REGISTER_SECONDS, regs, sizeof(regs));
	if (err < 0) {
		return err;
	}

	/* Return an error when the oscillator is stopped. */
	if (regs[0] & RV8263_BM_OS) {
		return -ENODATA;
	}

	timeptr->tm_sec = bcd2bin(regs[0] & SECONDS_BITS);
	timeptr->tm_min = bcd2bin(regs[1] & MINUTES_BITS);
	timeptr->tm_hour = bcd2bin(regs[2] & HOURS_BITS);
	timeptr->tm_mday = bcd2bin(regs[3] & DATE_BITS);
	timeptr->tm_wday = bcd2bin(regs[4] & WEEKDAY_BITS);
	timeptr->tm_mon = bcd2bin(regs[5] & MONTHS_BITS);
	timeptr->tm_year = bcd2bin(regs[6] & YEAR_BITS) + RV8263_YEAR_OFFSET;

	/* Unused. */
	timeptr->tm_nsec = 0;
	timeptr->tm_isdst = -1;
	timeptr->tm_yday = -1;

	/* Validate the chip in 24hr mode. */
	if (regs[2] & VALIDATE_24HR) {
		return -ENODATA;
	}

	LOG_DBG("Get time: year = %u, mon = %u, mday = %u, wday = %u, hour = %u, min = %u, sec = "
		"%u",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	return 0;
}

static int rv8263c8_init(const struct device *dev)
{
	int err;
	int temp;
	struct rv8263c8_data *data = dev->data;
	const struct rv8263c8_config *config = dev->config;

#if (CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE) && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (config->int_gpio.port == NULL) {
		return -EINVAL;
	}
#endif

	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready!");
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	err = rv8263c8_update_disable_timer(dev);
	if (err < 0) {
		LOG_ERR("Error while disabling the timer! Error: %i", err);
		return err;
	}

	err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_CONTROL_1,
				    RV8263C8_BM_24H_MODE_DISABLE | RV8263C8_BM_CLOCK_ENABLE);
	if (err < 0) {
		LOG_ERR("Error while writing CONTROL_1! Error: %i", err);
		return err;
	}

	temp = config->clkout;
	LOG_DBG("Configure ClkOut: %u", temp);

	err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_CONTROL_2,
				    RV8263C8_BM_AF | temp);
	if (err < 0) {
		LOG_ERR("Error while writing CONTROL_2! Error: %i", err);
		return err;
	}

	LOG_DBG("Configure ClkOut: %u", temp);

#if CONFIG_RTC_UPDATE && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	uint8_t buf[2];

	buf[0] = RV8263C8_REGISTER_TIMER_MODE;
	buf[1] = 0;
	err = i2c_write_dt(&config->i2c_bus, buf, 2);
	if (err < 0) {
		LOG_ERR("Error while writing CONTROL2! Error: %i", err);
		return err;
	}
#endif

#if (CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE) && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO not ready!");
		return err;
	}

	err = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure GPIO! Error: %u", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_FALLING);
	if (err < 0) {
		LOG_ERR("Failed to configure interrupt! Error: %u", err);
		return err;
	}

	gpio_init_callback(&data->gpio_cb, rv8263c8_gpio_callback_handler,
			   BIT(config->int_gpio.pin));

	err = gpio_add_callback_dt(&config->int_gpio, &data->gpio_cb);
	if (err < 0) {
		LOG_ERR("Failed to add GPIO callback! Error: %u", err);
		return err;
	}
#endif

	(void)k_sem_take(&data->lock, K_FOREVER);
#if CONFIG_RTC_ALARM && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	data->alarm_work.handler = rv8263c8_alarm_worker;
#endif

#if CONFIG_RTC_UPDATE && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	data->update_work.handler = rv8263c8_update_worker;
#endif

#if (CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE) && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	data->dev = dev;
#endif
	k_sem_give(&data->lock);

	return 0;
}

#if CONFIG_RTC_ALARM
static int rv8263c8_alarm_get_supported_fields(const struct device *dev, uint16_t id,
					       uint16_t *p_mask)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	(*p_mask) = (RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE |
		     RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY |
		     RTC_ALARM_TIME_MASK_WEEKDAY);

	return 0;
}

static int rv8263c8_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				   const struct rtc_time *timeptr)
{
	int err;
	uint8_t regs[5];
	const struct rv8263c8_config *config = dev->config;

	ARG_UNUSED(id);

	if ((mask > 0) && (timeptr == NULL)) {
		return -EINVAL;
	}

	/* Disable the alarm when mask is zero. */
	if (mask == 0) {
		return i2c_reg_update_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_CONTROL_2,
					      RV8263C8_BM_ALARM_INT_ENABLE,
					      RV8263C8_BM_ALARM_INT_DISABLE);
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("Invalid mask!");
		return -EINVAL;
	}

	regs[0] = bin2bcd(timeptr->tm_sec) & SECONDS_BITS;
	regs[1] = bin2bcd(timeptr->tm_min) & MINUTES_BITS;
	regs[2] = bin2bcd(timeptr->tm_hour) & HOURS_BITS;
	regs[3] = bin2bcd(timeptr->tm_mday) & DATE_BITS;
	regs[4] = bin2bcd(timeptr->tm_wday) & WEEKDAY_BITS;

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_SECONDS_ALARM,
					    RV8263C8_BM_ALARM_ENABLE | regs[0]);
	} else {
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_SECONDS_ALARM,
					    RV8263C8_BM_ALARM_DISABLE);
	}

	if (err < 0) {
		LOG_ERR("Error while writing SECONDS alarm! Error: %i", err);
		return err;
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_MINUTES_ALARM,
					    RV8263C8_BM_ALARM_ENABLE | regs[1]);
	} else {
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_MINUTES_ALARM,
					    RV8263C8_BM_ALARM_DISABLE);
	}

	if (err < 0) {
		LOG_ERR("Error while writing MINUTE alarm! Error: %i", err);
		return err;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_HOURS_ALARM,
					    RV8263C8_BM_ALARM_ENABLE | regs[2]);
	} else {
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_HOURS_ALARM,
					    RV8263C8_BM_ALARM_DISABLE);
	}

	if (err < 0) {
		LOG_ERR("Error while writing HOUR alarm! Error: %i", err);
		return err;
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_DATE_ALARM,
					    RV8263C8_BM_ALARM_ENABLE | regs[3]);
	} else {
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_DATE_ALARM,
					    RV8263C8_BM_ALARM_DISABLE);
	}

	if (err < 0) {
		LOG_ERR("Error while writing MONTHDAY alarm! Error: %i", err);
		return err;
	}

	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_WEEKDAY_ALARM,
					    RV8263C8_BM_ALARM_ENABLE | regs[4]);
	} else {
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_WEEKDAY_ALARM,
					    RV8263C8_BM_ALARM_DISABLE);
	}

	if (err < 0) {
		LOG_ERR("Error while writing WEEKDAY alarm! Error: %i", err);
	}

	return err;
}

static int rv8263c8_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *p_mask,
				   struct rtc_time *timeptr)
{
	int err;
	const struct rv8263c8_config *config = dev->config;
	uint8_t value[5];

	ARG_UNUSED(id);

	if (timeptr == NULL) {
		return -EINVAL;
	}

	(*p_mask) = 0;

	err = i2c_burst_read_dt(&config->i2c_bus, RV8263C8_REGISTER_SECONDS_ALARM, value,
				sizeof(value));
	if (err < 0) {
		LOG_ERR("Error while reading alarm! Error: %i", err);
		return err;
	}

	if (value[0] <= MAX_SEC) {
		timeptr->tm_sec = bcd2bin(value[0]) & SECONDS_BITS;
		(*p_mask) |= RTC_ALARM_TIME_MASK_SECOND;
	}

	if (value[1] <= MAX_MIN) {
		timeptr->tm_min = bcd2bin(value[1]) & MINUTES_BITS;
		(*p_mask) |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if (value[2] <= MAX_HOUR) {
		timeptr->tm_hour = bcd2bin(value[2]) & HOURS_BITS;
		(*p_mask) |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if (value[3] <= MAX_MDAY) {
		timeptr->tm_mday = bcd2bin(value[3]) & DATE_BITS;
		(*p_mask) |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	if (value[4] <= MAX_WDAY) {
		timeptr->tm_wday = bcd2bin(value[4]) & WEEKDAY_BITS;
		(*p_mask) |= RTC_ALARM_TIME_MASK_WEEKDAY;
	}

	return 0;
}

static int rv8263c8_alarm_set_callback(const struct device *dev, uint16_t id,
				       rtc_alarm_callback callback, void *user_data)
{
	int err;
	const struct rv8263c8_config *config = dev->config;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	struct rv8263c8_data *data = dev->data;
#endif

	ARG_UNUSED(id);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (config->int_gpio.port == NULL) {
		return -ENOTSUP;
	}

	(void)k_sem_take(&data->lock, K_FOREVER);
	data->alarm_cb = callback;
	data->alarm_cb_data = user_data;
	k_sem_give(&data->lock);
#else
	return -ENOTSUP;
#endif

	if ((callback == NULL) && (user_data == NULL)) {
		LOG_DBG("Disable alarm function");

		err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_CONTROL_2,
					     RV8263C8_BM_ALARM_INT_ENABLE,
					     RV8263C8_BM_ALARM_INT_DISABLE);
	} else {
		LOG_DBG("Enable alarm function");

		err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_CONTROL_2,
					     RV8263C8_BM_ALARM_INT_ENABLE | RV8263C8_BM_AF,
					     RV8263C8_BM_ALARM_INT_ENABLE);
	}

	if (err < 0) {
		LOG_ERR("Error while writing CONTROL2! Error: %i", err);
	}

	return err;
}

static int rv8263c8_alarm_is_pending(const struct device *dev, uint16_t id)
{
	int err;
	uint8_t reg;
	const struct rv8263c8_config *config = dev->config;

	ARG_UNUSED(id);

	err = i2c_reg_read_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_CONTROL_2, &reg);
	if (err) {
		return err;
	}

	if (reg & RV8263C8_BM_AF) {
		reg &= ~RV8263C8_BM_AF;
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_CONTROL_2, reg);
		if (err) {
			return err;
		}

		return 1;
	}

	return 0;
}
#endif

#if CONFIG_RTC_UPDATE && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
int rv8263_update_callback(const struct device *dev, rtc_update_callback callback, void *user_data)
{
	struct rv8263c8_data *const data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
	data->update_cb = callback;
	data->update_cb_data = user_data;
	k_sem_give(&data->lock);

	/* Disable the update callback. */
	if ((callback == NULL) && (user_data == NULL)) {
		return rv8263c8_update_disable_timer(dev);
	}

	return rv8263c8_update_enable_timer(dev);
}
#endif

#ifdef CONFIG_RTC_CALIBRATION
int rv8263c8_calibration_set(const struct device *dev, int32_t calibration)
{
	int8_t offset;
	int32_t test_mode0;
	int32_t test_mode1;
	int32_t offset_ppm_mode0;
	int32_t offset_ppm_mode1;
	const struct rv8263c8_config *config = dev->config;

	/* NOTE: The RTC API is using a PPB (Parts Per Billion) value. The RTC is using PPM.
	 * Here we calculate the offset when using MODE = 0.
	 *  Formula from the application manual:
	 *  Offset [ppm] = (calibration [ppb] / (4.34 [ppm] x 1000))
	 */
	offset_ppm_mode0 = calibration / 4340;

	/* Here we calculate the offset when using MODE = 1.
	 *  Formula from the application manual:
	 *  Offset [ppm] = (calibration [ppb] / (4.069 [ppm] x 1000))
	 */
	offset_ppm_mode1 = calibration / 4069;

	LOG_DBG("Offset Mode = 0: %i", offset_ppm_mode0);
	LOG_DBG("Offset Mode = 1: %i", offset_ppm_mode1);

	test_mode0 = offset_ppm_mode0 * 4340;
	test_mode0 = calibration - test_mode0;
	test_mode1 = offset_ppm_mode1 * 4069;
	test_mode1 = calibration - test_mode1;

	/* Compare the values and select the value with the smallest error. */
	test_mode0 = test_mode0 < 0 ? -test_mode0 : test_mode0;
	test_mode1 = test_mode1 < 0 ? -test_mode1 : test_mode1;
	if (test_mode0 > test_mode1) {
		LOG_DBG("Use fast mode (Mode = 1)");

		/* Error with MODE = 1 is smaller -> Use MODE = 1. */
		offset = RV8263_BM_FAST_MODE | (offset_ppm_mode1 & GENMASK(7, 0));
	} else {
		LOG_DBG("Use normal mode (Mode = 0)");

		/* Error with MODE = 0 is smaller -> Use MODE = 0. */
		offset = RV8263_BM_NORMAL_MODE | (offset_ppm_mode0 & GENMASK(7, 0));
	}

	LOG_DBG("Set offset value: %i", (offset & RV8263C8_BM_REGISTER_OFFSET));

	return i2c_reg_write_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_OFFSET, offset);
}

int rv8263c8_calibration_get(const struct device *dev, int32_t *calibration)
{
	int err;
	int32_t temp;
	int8_t offset;
	const struct rv8263c8_config *config = dev->config;

	if (calibration == NULL) {
		return -EINVAL;
	}

	err = i2c_reg_read_byte_dt(&config->i2c_bus, RV8263C8_REGISTER_OFFSET, &offset);
	if (err) {
		return err;
	}

	/* Convert the signed 7 bit into a signed 8 bit value. */
	if (offset & (0x01 << 6)) {
		temp = offset | (0x01 << 7);
	} else {
		temp = offset & (0x3F);
		temp &= ~(0x01 << 7);
	}

	LOG_DBG("Read offset: %i", temp);

	if (offset & RV8263_BM_FAST_MODE) {
		temp = temp * 4340L;
	} else {
		temp = temp * 4069L;
	}

	*calibration = temp;

	return 0;
}
#endif

static const struct rtc_driver_api rv8263c8_driver_api = {
	.set_time = rv8263c8_time_set,
	.get_time = rv8263c8_time_get,
#if CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rv8263c8_alarm_get_supported_fields,
	.alarm_set_time = rv8263c8_alarm_set_time,
	.alarm_get_time = rv8263c8_alarm_get_time,
	.alarm_is_pending = rv8263c8_alarm_is_pending,
	.alarm_set_callback = rv8263c8_alarm_set_callback,
#endif
#if CONFIG_RTC_UPDATE && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	.update_set_callback = rv8263_update_callback,
#endif
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = rv8263c8_calibration_set,
	.get_calibration = rv8263c8_calibration_get,
#endif
};

#define RV8263_DEFINE(inst)                                                                        \
	static struct rv8263c8_data rv8263c8_data_##inst;                                          \
	static const struct rv8263c8_config rv8263c8_config_##inst = {                             \
		.i2c_bus = I2C_DT_SPEC_INST_GET(inst),                                             \
		.clkout = DT_INST_ENUM_IDX(inst, clkout),                                          \
		IF_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios),                            \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0})))};         \
	DEVICE_DT_INST_DEFINE(inst, &rv8263c8_init, NULL, &rv8263c8_data_##inst,                   \
			      &rv8263c8_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,      \
			      &rv8263c8_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV8263_DEFINE)
