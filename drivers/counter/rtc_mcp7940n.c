/*
 * Copyright (c) 2019-2020 Peter Bigot Consulting, LLC
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp7940n

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc/mcp7940n.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>
#include <time.h>

LOG_MODULE_REGISTER(MCP7940N, CONFIG_COUNTER_LOG_LEVEL);

/* Alarm channels */
#define ALARM0_ID			0
#define ALARM1_ID			1

/* Size of block when writing whole struct */
#define RTC_TIME_REGISTERS_SIZE		sizeof(struct mcp7940n_time_registers)
#define RTC_ALARM_REGISTERS_SIZE	sizeof(struct mcp7940n_alarm_registers)

/* Largest block size */
#define MAX_WRITE_SIZE                  (RTC_TIME_REGISTERS_SIZE)

/* tm struct uses years since 1900 but unix time uses years since
 * 1970. MCP7940N default year is '1' so the offset is 69
 */
#define UNIX_YEAR_OFFSET		69

/* Macro used to decode BCD to UNIX time to avoid potential copy and paste
 * errors.
 */
#define RTC_BCD_DECODE(reg_prefix) (reg_prefix##_one + reg_prefix##_ten * 10)

struct mcp7940n_config {
	struct counter_config_info generic;
	const struct device *i2c_dev;
	const struct gpio_dt_spec int_gpios;
	uint8_t addr;
};

struct mcp7940n_data {
	const struct device *mcp7940n;
	struct k_sem lock;
	struct mcp7940n_time_registers registers;
	struct mcp7940n_alarm_registers alm0_registers;
	struct mcp7940n_alarm_registers alm1_registers;

	struct k_work alarm_work;
	struct gpio_callback int_callback;

	counter_alarm_callback_t counter_handler[2];
	uint32_t counter_ticks[2];
	void *alarm_user_data[2];

	bool int_active_high;
};

/** @brief Convert bcd time in device registers to UNIX time
 *
 * @param dev the MCP7940N device pointer.
 *
 * @retval returns unix time.
 */
static time_t decode_rtc(const struct device *dev)
{
	struct mcp7940n_data *data = dev->data;
	time_t time_unix = 0;
	struct tm time = { 0 };

	time.tm_sec = RTC_BCD_DECODE(data->registers.rtc_sec.sec);
	time.tm_min = RTC_BCD_DECODE(data->registers.rtc_min.min);
	time.tm_hour = RTC_BCD_DECODE(data->registers.rtc_hours.hr);
	time.tm_mday = RTC_BCD_DECODE(data->registers.rtc_date.date);
	time.tm_wday = data->registers.rtc_weekday.weekday;
	/* tm struct starts months at 0, mcp7940n starts at 1 */
	time.tm_mon = RTC_BCD_DECODE(data->registers.rtc_month.month) - 1;
	/* tm struct uses years since 1900 but unix time uses years since 1970 */
	time.tm_year = RTC_BCD_DECODE(data->registers.rtc_year.year) +
		UNIX_YEAR_OFFSET;

	time_unix = timeutil_timegm(&time);

	LOG_DBG("Unix time is %d\n", (uint32_t)time_unix);

	return time_unix;
}

/** @brief Encode time struct tm into mcp7940n rtc registers
 *
 * @param dev the MCP7940N device pointer.
 * @param time_buffer tm struct containing time to be encoded into mcp7940n
 * registers.
 *
 * @retval return 0 on success, or a negative error code from invalid
 * parameter.
 */
static int encode_rtc(const struct device *dev, struct tm *time_buffer)
{
	struct mcp7940n_data *data = dev->data;
	uint8_t month;
	uint8_t year_since_epoch;

	/* In a tm struct, months start at 0, mcp7940n starts with 1 */
	month = time_buffer->tm_mon + 1;

	if (time_buffer->tm_year < UNIX_YEAR_OFFSET) {
		return -EINVAL;
	}
	year_since_epoch = time_buffer->tm_year - UNIX_YEAR_OFFSET;

	/* Set external oscillator configuration bit */
	data->registers.rtc_sec.start_osc = 1;

	data->registers.rtc_sec.sec_one = time_buffer->tm_sec % 10;
	data->registers.rtc_sec.sec_ten = time_buffer->tm_sec / 10;
	data->registers.rtc_min.min_one = time_buffer->tm_min % 10;
	data->registers.rtc_min.min_ten = time_buffer->tm_min / 10;
	data->registers.rtc_hours.hr_one = time_buffer->tm_hour % 10;
	data->registers.rtc_hours.hr_ten = time_buffer->tm_hour / 10;
	data->registers.rtc_weekday.weekday = time_buffer->tm_wday;
	data->registers.rtc_date.date_one = time_buffer->tm_mday % 10;
	data->registers.rtc_date.date_ten = time_buffer->tm_mday / 10;
	data->registers.rtc_month.month_one = month % 10;
	data->registers.rtc_month.month_ten = month / 10;
	data->registers.rtc_year.year_one = year_since_epoch % 10;
	data->registers.rtc_year.year_ten = year_since_epoch / 10;

	return 0;
}

/** @brief Encode time struct tm into mcp7940n alarm registers
 *
 * @param dev the MCP7940N device pointer.
 * @param time_buffer tm struct containing time to be encoded into mcp7940n
 * registers.
 * @param alarm_id alarm ID, can be 0 or 1 for MCP7940N.
 *
 * @retval return 0 on success, or a negative error code from invalid
 * parameter.
 */
static int encode_alarm(const struct device *dev, struct tm *time_buffer, uint8_t alarm_id)
{
	struct mcp7940n_data *data = dev->data;
	uint8_t month;
	struct mcp7940n_alarm_registers *alm_regs;

	if (alarm_id == ALARM0_ID) {
		alm_regs = &data->alm0_registers;
	} else if (alarm_id == ALARM1_ID) {
		alm_regs = &data->alm1_registers;
	} else {
		return -EINVAL;
	}
	/* In a tm struct, months start at 0 */
	month = time_buffer->tm_mon + 1;

	alm_regs->alm_sec.sec_one = time_buffer->tm_sec % 10;
	alm_regs->alm_sec.sec_ten = time_buffer->tm_sec / 10;
	alm_regs->alm_min.min_one = time_buffer->tm_min % 10;
	alm_regs->alm_min.min_ten = time_buffer->tm_min / 10;
	alm_regs->alm_hours.hr_one = time_buffer->tm_hour % 10;
	alm_regs->alm_hours.hr_ten = time_buffer->tm_hour / 10;
	alm_regs->alm_weekday.weekday = time_buffer->tm_wday;
	alm_regs->alm_date.date_one = time_buffer->tm_mday % 10;
	alm_regs->alm_date.date_ten = time_buffer->tm_mday / 10;
	alm_regs->alm_month.month_one = month % 10;
	alm_regs->alm_month.month_ten = month / 10;

	return 0;
}

/** @brief Reads single register from MCP7940N
 *
 * @param dev the MCP7940N device pointer.
 * @param addr register address.
 * @param val pointer to uint8_t that will contain register value if
 * successful.
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction.
 */
static int read_register(const struct device *dev, uint8_t addr, uint8_t *val)
{
	const struct mcp7940n_config *cfg = dev->config;

	int rc = i2c_write_read(cfg->i2c_dev, cfg->addr,
				&addr, sizeof(addr),
				val, 1);

	return rc;
}

/** @brief Read registers from device and populate mcp7940n_registers struct
 *
 * @param dev the MCP7940N device pointer.
 * @param unix_time pointer to time_t value that will contain unix time if
 * successful.
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction.
 */
static int read_time(const struct device *dev, time_t *unix_time)
{
	struct mcp7940n_data *data = dev->data;
	const struct mcp7940n_config *cfg = dev->config;
	uint8_t addr = REG_RTC_SEC;

	int rc = i2c_write_read(cfg->i2c_dev, cfg->addr,
				&addr, sizeof(addr),
				&data->registers, RTC_TIME_REGISTERS_SIZE);

	if (rc >= 0) {
		*unix_time = decode_rtc(dev);
	}

	return rc;
}

/** @brief Write a single register to MCP7940N
 *
 * @param dev the MCP7940N device pointer.
 * @param addr register address.
 * @param value Value that will be written to the register.
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction or invalid parameter.
 */
static int write_register(const struct device *dev, enum mcp7940n_register addr, uint8_t value)
{
	const struct mcp7940n_config *cfg = dev->config;
	int rc = 0;

	uint8_t time_data[2] = {addr, value};

	rc = i2c_write(cfg->i2c_dev, time_data, sizeof(time_data), cfg->addr);

	return rc;
}

/** @brief Write a full time struct to MCP7940N registers.
 *
 * @param dev the MCP7940N device pointer.
 * @param addr first register address to write to, should be REG_RTC_SEC,
 * REG_ALM0_SEC or REG_ALM0_SEC.
 * @param size size of data struct that will be written.
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction or invalid parameter.
 */
static int write_data_block(const struct device *dev, enum mcp7940n_register addr, uint8_t size)
{
	struct mcp7940n_data *data = dev->data;
	const struct mcp7940n_config *cfg = dev->config;
	int rc = 0;
	uint8_t time_data[MAX_WRITE_SIZE + 1];
	uint8_t *write_block_start;

	if (size > MAX_WRITE_SIZE) {
		return -EINVAL;
	}

	if (addr >= REG_INVAL) {
		return -EINVAL;
	}

	if (addr == REG_RTC_SEC) {
		write_block_start = (uint8_t *)&data->registers;
	} else if (addr == REG_ALM0_SEC) {
		write_block_start = (uint8_t *)&data->alm0_registers;
	} else if (addr == REG_ALM1_SEC) {
		write_block_start = (uint8_t *)&data->alm1_registers;
	} else {
		return -EINVAL;
	}

	/* Load register address into first byte then fill in data values */
	time_data[0] = addr;
	memcpy(&time_data[1], write_block_start, size);

	rc = i2c_write(cfg->i2c_dev, time_data, size + 1, cfg->addr);

	return rc;
}

/** @brief Sets the correct weekday.
 *
 * If the time is never set then the device defaults to 1st January 1970
 * but with the wrong weekday set. This function ensures the weekday is
 * correct in this case.
 *
 * @param dev the MCP7940N device pointer.
 * @param unix_time pointer to unix time that will be used to work out the weekday
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction or invalid parameter.
 */
static int set_day_of_week(const struct device *dev, time_t *unix_time)
{
	struct mcp7940n_data *data = dev->data;
	struct tm time_buffer = { 0 };
	int rc = 0;

	gmtime_r(unix_time, &time_buffer);

	if (time_buffer.tm_wday != 0) {
		data->registers.rtc_weekday.weekday = time_buffer.tm_wday;
		rc = write_register(dev, REG_RTC_WDAY,
			*((uint8_t *)(&data->registers.rtc_weekday)));
	} else {
		rc = -EINVAL;
	}

	return rc;
}

/** @brief Checks the interrupt pending flag (IF) of a given alarm.
 *
 * A callback is fired if an IRQ is pending.
 *
 * @param dev the MCP7940N device pointer.
 * @param alarm_id ID of alarm, can be 0 or 1 for MCP7940N.
 */
static void mcp7940n_handle_interrupt(const struct device *dev, uint8_t alarm_id)
{
	struct mcp7940n_data *data = dev->data;
	uint8_t alarm_reg_address;
	struct mcp7940n_alarm_registers *alm_regs;
	counter_alarm_callback_t cb;
	uint32_t ticks = 0;
	bool fire_callback = false;

	if (alarm_id == ALARM0_ID) {
		alarm_reg_address = REG_ALM0_WDAY;
		alm_regs = &data->alm0_registers;
	} else if (alarm_id == ALARM1_ID) {
		alarm_reg_address = REG_ALM1_WDAY;
		alm_regs = &data->alm1_registers;
	} else {
		return;
	}

	k_sem_take(&data->lock, K_FOREVER);

	/* Check if this alarm has a pending interrupt */
	read_register(dev, alarm_reg_address, (uint8_t *)&alm_regs->alm_weekday);

	if (alm_regs->alm_weekday.alm_if) {
		/* Clear interrupt */
		alm_regs->alm_weekday.alm_if = 0;
		write_register(dev, alarm_reg_address,
			*((uint8_t *)(&alm_regs->alm_weekday)));

		/* Fire callback */
		if (data->counter_handler[alarm_id]) {
			cb = data->counter_handler[alarm_id];
			ticks = data->counter_ticks[alarm_id];
			fire_callback = true;
		}
	}

	k_sem_give(&data->lock);

	if (fire_callback) {
		cb(data->mcp7940n, 0, ticks, data->alarm_user_data[alarm_id]);
	}
}

static void mcp7940n_work_handler(struct k_work *work)
{
	struct mcp7940n_data *data =
		CONTAINER_OF(work, struct mcp7940n_data, alarm_work);

	/* Check interrupt flags for both alarms */
	mcp7940n_handle_interrupt(data->mcp7940n, ALARM0_ID);
	mcp7940n_handle_interrupt(data->mcp7940n, ALARM1_ID);
}

static void mcp7940n_init_cb(const struct device *dev,
				 struct gpio_callback *gpio_cb, uint32_t pins)
{
	struct mcp7940n_data *data =
		CONTAINER_OF(gpio_cb, struct mcp7940n_data, int_callback);

	ARG_UNUSED(pins);

	k_work_submit(&data->alarm_work);
}

int mcp7940n_rtc_set_time(const struct device *dev, time_t unix_time)
{
	struct mcp7940n_data *data = dev->data;
	struct tm time_buffer = { 0 };
	int rc = 0;

	if (unix_time > UINT32_MAX) {
		LOG_ERR("Unix time must be 32-bit");
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	/* Convert unix_time to civil time */
	gmtime_r(&unix_time, &time_buffer);
	LOG_DBG("Desired time is %d-%d-%d %d:%d:%d\n", (time_buffer.tm_year + 1900),
		(time_buffer.tm_mon + 1), time_buffer.tm_mday, time_buffer.tm_hour,
		time_buffer.tm_min, time_buffer.tm_sec);

	/* Encode time */
	rc = encode_rtc(dev, &time_buffer);
	if (rc < 0) {
		goto out;
	}

	/* Write to device */
	rc = write_data_block(dev, REG_RTC_SEC, RTC_TIME_REGISTERS_SIZE);

out:
	k_sem_give(&data->lock);

	return rc;
}

static int mcp7940n_counter_start(const struct device *dev)
{
	struct mcp7940n_data *data = dev->data;
	int rc = 0;

	k_sem_take(&data->lock, K_FOREVER);

	/* Set start oscillator configuration bit */
	data->registers.rtc_sec.start_osc = 1;
	rc = write_register(dev, REG_RTC_SEC,
		*((uint8_t *)(&data->registers.rtc_sec)));

	k_sem_give(&data->lock);

	return rc;
}

static int mcp7940n_counter_stop(const struct device *dev)
{
	struct mcp7940n_data *data = dev->data;
	int rc = 0;

	k_sem_take(&data->lock, K_FOREVER);

	/* Clear start oscillator configuration bit */
	data->registers.rtc_sec.start_osc = 0;
	rc = write_register(dev, REG_RTC_SEC,
		*((uint8_t *)(&data->registers.rtc_sec)));

	k_sem_give(&data->lock);

	return rc;
}

static int mcp7940n_counter_get_value(const struct device *dev,
				      uint32_t *ticks)
{
	struct mcp7940n_data *data = dev->data;
	time_t unix_time;
	int rc;

	k_sem_take(&data->lock, K_FOREVER);

	/* Get time */
	rc = read_time(dev, &unix_time);

	/* Convert time to ticks */
	if (rc >= 0) {
		*ticks = unix_time;
	}

	k_sem_give(&data->lock);

	return rc;
}

static int mcp7940n_counter_set_alarm(const struct device *dev, uint8_t alarm_id,
				      const struct counter_alarm_cfg *alarm_cfg)
{
	struct mcp7940n_data *data = dev->data;
	uint32_t seconds_until_alarm;
	time_t current_time;
	time_t alarm_time;
	struct tm time_buffer = { 0 };
	uint8_t alarm_base_address;
	struct mcp7940n_alarm_registers *alm_regs;
	int rc = 0;

	k_sem_take(&data->lock, K_FOREVER);

	if (alarm_id == ALARM0_ID) {
		alarm_base_address = REG_ALM0_SEC;
		alm_regs = &data->alm0_registers;
	} else if (alarm_id == ALARM1_ID) {
		alarm_base_address = REG_ALM1_SEC;
		alm_regs = &data->alm1_registers;
	} else {
		rc = -EINVAL;
		goto out;
	}

	/* Convert ticks to time */
	seconds_until_alarm = alarm_cfg->ticks;

	/* Get current time and add alarm offset */
	rc = read_time(dev, &current_time);
	if (rc < 0) {
		goto out;
	}

	alarm_time = current_time + seconds_until_alarm;
	gmtime_r(&alarm_time, &time_buffer);

	/* Set alarm trigger mask and alarm enable flag */
	if (alarm_id == ALARM0_ID) {
		data->registers.rtc_control.alm0_en = 1;
	} else if (alarm_id == ALARM1_ID) {
		data->registers.rtc_control.alm1_en = 1;
	}

	/* Set alarm to match with second, minute, hour, day of week, day of
	 * month and month
	 */
	alm_regs->alm_weekday.alm_msk = MCP7940N_ALARM_TRIGGER_ALL;

	/* Write time to alarm registers */
	encode_alarm(dev, &time_buffer, alarm_id);
	rc = write_data_block(dev, alarm_base_address, RTC_ALARM_REGISTERS_SIZE);
	if (rc < 0) {
		goto out;
	}

	/* Enable alarm */
	rc = write_register(dev, REG_RTC_CONTROL,
		*((uint8_t *)(&data->registers.rtc_control)));
	if (rc < 0) {
		goto out;
	}

	/* Config user data and callback */
	data->counter_handler[alarm_id] = alarm_cfg->callback;
	data->counter_ticks[alarm_id] = current_time;
	data->alarm_user_data[alarm_id] = alarm_cfg->user_data;

out:
	k_sem_give(&data->lock);

	return rc;
}

static int mcp7940n_counter_cancel_alarm(const struct device *dev, uint8_t alarm_id)
{
	struct mcp7940n_data *data = dev->data;
	int rc = 0;

	k_sem_take(&data->lock, K_FOREVER);

	/* Clear alarm enable bit */
	if (alarm_id == ALARM0_ID) {
		data->registers.rtc_control.alm0_en = 0;
	} else if (alarm_id == ALARM1_ID) {
		data->registers.rtc_control.alm1_en = 0;
	} else {
		rc = -EINVAL;
		goto out;
	}

	rc = write_register(dev, REG_RTC_CONTROL,
		*((uint8_t *)(&data->registers.rtc_control)));

out:
	k_sem_give(&data->lock);

	return rc;
}

static int mcp7940n_counter_set_top_value(const struct device *dev,
					  const struct counter_top_cfg *cfg)
{
	return -ENOTSUP;
}

/* This function can be used to poll the alarm interrupt flags if the MCU is
 * not connected to the MC7940N MFP pin. It can also be used to check if an
 * alarm was triggered while the MCU was in reset. This function will clear
 * the interrupt flag
 *
 * Return bitmask of alarm interrupt flag (IF) where each IF is shifted by
 * the alarm ID.
 */
static uint32_t mcp7940n_counter_get_pending_int(const struct device *dev)
{
	struct mcp7940n_data *data = dev->data;
	uint32_t interrupt_pending = 0;
	int rc;

	k_sem_take(&data->lock, K_FOREVER);

	/* Check interrupt flag for alarm 0 */
	rc = read_register(dev, REG_ALM0_WDAY,
		(uint8_t *)&data->alm0_registers.alm_weekday);
	if (rc < 0) {
		goto out;
	}

	if (data->alm0_registers.alm_weekday.alm_if) {
		/* Clear interrupt */
		data->alm0_registers.alm_weekday.alm_if = 0;
		rc = write_register(dev, REG_ALM0_WDAY,
			*((uint8_t *)(&data->alm0_registers.alm_weekday)));
		if (rc < 0) {
			goto out;
		}
		interrupt_pending |= (1 << ALARM0_ID);
	}

	/* Check interrupt flag for alarm 1 */
	rc = read_register(dev, REG_ALM1_WDAY,
		(uint8_t *)&data->alm1_registers.alm_weekday);
	if (rc < 0) {
		goto out;
	}

	if (data->alm1_registers.alm_weekday.alm_if) {
		/* Clear interrupt */
		data->alm1_registers.alm_weekday.alm_if = 0;
		rc = write_register(dev, REG_ALM1_WDAY,
			*((uint8_t *)(&data->alm1_registers.alm_weekday)));
		if (rc < 0) {
			goto out;
		}
		interrupt_pending |= (1 << ALARM1_ID);
	}

out:
	k_sem_give(&data->lock);

	if (rc) {
		interrupt_pending = 0;
	}
	return (interrupt_pending);
}

static uint32_t mcp7940n_counter_get_top_value(const struct device *dev)
{
	return UINT32_MAX;
}

static int mcp7940n_init(const struct device *dev)
{
	struct mcp7940n_data *data = dev->data;
	const struct mcp7940n_config *cfg = dev->config;
	int rc = 0;
	time_t unix_time = 0;

	/* Initialize and take the lock */
	k_sem_init(&data->lock, 0, 1);

	if (!device_is_ready(cfg->i2c_dev)) {
		LOG_ERR("I2C device %s is not ready", cfg->i2c_dev->name);
		rc = -ENODEV;
		goto out;
	}

	rc = read_time(dev, &unix_time);
	if (rc < 0) {
		goto out;
	}

	rc = set_day_of_week(dev, &unix_time);
	if (rc < 0) {
		goto out;
	}

	/* Set 24-hour time */
	data->registers.rtc_hours.twelve_hr = false;
	rc = write_register(dev, REG_RTC_HOUR,
		*((uint8_t *)(&data->registers.rtc_hours)));
	if (rc < 0) {
		goto out;
	}

	/* Configure alarm interrupt gpio */
	if (cfg->int_gpios.port != NULL) {

		if (!device_is_ready(cfg->int_gpios.port)) {
			LOG_ERR("Port device %s is not ready",
				cfg->int_gpios.port->name);
			rc = -ENODEV;
			goto out;
		}

		data->mcp7940n = dev;
		k_work_init(&data->alarm_work, mcp7940n_work_handler);

		gpio_pin_configure_dt(&cfg->int_gpios, GPIO_INPUT);

		gpio_pin_interrupt_configure_dt(&cfg->int_gpios,
						GPIO_INT_EDGE_TO_ACTIVE);

		gpio_init_callback(&data->int_callback, mcp7940n_init_cb,
				   BIT(cfg->int_gpios.pin));

		gpio_add_callback(cfg->int_gpios.port, &data->int_callback);

		/* Configure interrupt polarity */
		if ((cfg->int_gpios.dt_flags & GPIO_ACTIVE_LOW) == GPIO_ACTIVE_LOW) {
			data->int_active_high = false;
		} else {
			data->int_active_high = true;
		}
		data->alm0_registers.alm_weekday.alm_pol = data->int_active_high;
		data->alm1_registers.alm_weekday.alm_pol = data->int_active_high;
		rc = write_register(dev, REG_ALM0_WDAY,
				    *((uint8_t *)(&data->alm0_registers.alm_weekday)));
		rc = write_register(dev, REG_ALM1_WDAY,
				    *((uint8_t *)(&data->alm1_registers.alm_weekday)));
	}
out:
	k_sem_give(&data->lock);

	return rc;
}

static const struct counter_driver_api mcp7940n_api = {
	.start = mcp7940n_counter_start,
	.stop = mcp7940n_counter_stop,
	.get_value = mcp7940n_counter_get_value,
	.set_alarm = mcp7940n_counter_set_alarm,
	.cancel_alarm = mcp7940n_counter_cancel_alarm,
	.set_top_value = mcp7940n_counter_set_top_value,
	.get_pending_int = mcp7940n_counter_get_pending_int,
	.get_top_value = mcp7940n_counter_get_top_value,
};

#define INST_DT_MCP7904N(index)                                                         \
											\
	static struct mcp7940n_data mcp7940n_data_##index;				\
											\
	static const struct mcp7940n_config mcp7940n_config_##index = {			\
		.generic = {								\
			.max_top_value = UINT32_MAX,					\
			.freq = 1,							\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,				\
			.channels = 2,							\
		},									\
		.i2c_dev = DEVICE_DT_GET(DT_INST_BUS(index)),				\
		.addr = DT_INST_REG_ADDR(index),					\
		.int_gpios = GPIO_DT_SPEC_INST_GET_OR(index, int_gpios, {0}),		\
	};										\
											\
	DEVICE_DT_INST_DEFINE(index, mcp7940n_init, NULL,				\
		    &mcp7940n_data_##index,						\
		    &mcp7940n_config_##index,						\
		    POST_KERNEL,							\
		    CONFIG_COUNTER_INIT_PRIORITY,					\
		    &mcp7940n_api);

DT_INST_FOREACH_STATUS_OKAY(INST_DT_MCP7904N);
