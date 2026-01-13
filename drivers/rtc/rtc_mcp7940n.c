/*
 * Copyright (c) 2026 Plentify (Pty) Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp7940n

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/rtc/mcp7940n.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>
#include <time.h>

LOG_MODULE_REGISTER(MCP7940N, CONFIG_RTC_LOG_LEVEL);

/* Size of block when writing whole struct */
#define RTC_TIME_REGISTERS_SIZE  sizeof(struct mcp7940n_time_registers)
#define RTC_ALARM_REGISTERS_SIZE sizeof(struct mcp7940n_alarm_registers)

/* tm struct uses years since 1900 but unix time uses years since
 * 1970. MCP7940N default year is '1' so the offset is 69
 */
#define UNIX_YEAR_OFFSET 69

/* Largest block size */
#define MAX_WRITE_SIZE (RTC_TIME_REGISTERS_SIZE)

/* Macro used to decode BCD to UNIX time to avoid potential copy and paste
 * errors.
 */
#define RTC_BCD_DECODE(reg_prefix) (reg_prefix##_one + reg_prefix##_ten * 10)

struct mcp7940n_config {
	struct i2c_dt_spec i2c;
};

struct mcp7940n_data {
	const struct device *mcp7940n;
	struct k_mutex lock;
	struct mcp7940n_time_registers registers;
};

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

	rc = i2c_write_dt(&cfg->i2c, time_data, sizeof(time_data));

	return rc;
}

/** @brief Convert bcd time in device registers to UNIX time
 *
 * @param dev the MCP7940N device pointer.
 *
 * @retval returns unix time.
 */
static struct tm decode_rtc(const struct device *dev)
{
	struct mcp7940n_data *data = dev->data;
	struct tm unix_time = {0};

	unix_time.tm_sec = RTC_BCD_DECODE(data->registers.rtc_sec.sec);
	unix_time.tm_min = RTC_BCD_DECODE(data->registers.rtc_min.min);
	unix_time.tm_hour = RTC_BCD_DECODE(data->registers.rtc_hours.hr);
	unix_time.tm_mday = RTC_BCD_DECODE(data->registers.rtc_date.date);
	unix_time.tm_wday = data->registers.rtc_weekday.weekday;
	/* tm struct starts months at 0, mcp7940n starts at 1 */
	unix_time.tm_mon = RTC_BCD_DECODE(data->registers.rtc_month.month) - 1;
	/* tm struct uses years since 1900 but unix time uses years since 1970 */
	unix_time.tm_year = RTC_BCD_DECODE(data->registers.rtc_year.year) + UNIX_YEAR_OFFSET;

	return unix_time;
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
static int read_time(const struct device *dev, struct tm *unix_time)
{
	struct mcp7940n_data *data = dev->data;
	const struct mcp7940n_config *cfg = dev->config;
	uint8_t addr = REG_RTC_SEC;

	int rc = i2c_write_read_dt(&cfg->i2c, &addr, sizeof(addr), &data->registers,
				   RTC_TIME_REGISTERS_SIZE);

	if (rc >= 0) {
		*unix_time = decode_rtc(dev);
	}

	return rc;
}

/** @brief Encode time struct tm into mcp7940n rtc registers
 *
 * @param dev the MCP7940N device pointer.
 * @param unix_time tm struct containing time to be encoded into mcp7940n
 * registers.
 *
 * @retval return 0 on success, or a negative error code from invalid
 * parameter.
 */
static int encode_rtc(const struct device *dev, const struct tm *unix_time)
{
	struct mcp7940n_data *data = dev->data;
	uint8_t month;
	uint8_t year_since_epoch;

	/* In a tm struct, months start at 0, mcp7940n starts with 1 */
	month = unix_time->tm_mon + 1;

	if (unix_time->tm_year < UNIX_YEAR_OFFSET) {
		return -EINVAL;
	}
	year_since_epoch = unix_time->tm_year - UNIX_YEAR_OFFSET;

	/* Set external oscillator configuration bit */
	data->registers.rtc_sec.start_osc = 1;

	data->registers.rtc_sec.sec_one = unix_time->tm_sec % 10;
	data->registers.rtc_sec.sec_ten = unix_time->tm_sec / 10;
	data->registers.rtc_min.min_one = unix_time->tm_min % 10;
	data->registers.rtc_min.min_ten = unix_time->tm_min / 10;
	data->registers.rtc_hours.hr_one = unix_time->tm_hour % 10;
	data->registers.rtc_hours.hr_ten = unix_time->tm_hour / 10;
	data->registers.rtc_weekday.weekday = unix_time->tm_wday;
	data->registers.rtc_date.date_one = unix_time->tm_mday % 10;
	data->registers.rtc_date.date_ten = unix_time->tm_mday / 10;
	data->registers.rtc_month.month_one = month % 10;
	data->registers.rtc_month.month_ten = month / 10;
	data->registers.rtc_year.year_one = year_since_epoch % 10;
	data->registers.rtc_year.year_ten = year_since_epoch / 10;

	return 0;
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
	} else {
		return -EINVAL;
	}

	/* Load register address into first byte then fill in data values */
	time_data[0] = addr;
	memcpy(&time_data[1], write_block_start, size);

	rc = i2c_write_dt(&cfg->i2c, time_data, size + 1);

	return rc;
}

/**
 * @brief Fetch time from the RTC
 *
 * @param dev the MCP7940N device pointer.
 * @param timeptr pointer to time struct which will receive the result
 *
 * @return int return 0 on success, or a negative error code
 */
static int mcp7940n_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct mcp7940n_data *data = dev->data;
	int rc;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Get time */
	rc = read_time(dev, rtc_time_to_tm(timeptr));

	k_mutex_unlock(&data->lock);

	return rc;
}

/**
 * @brief Sets the time in the RTC
 *
 * @param dev the MCP7940N device pointer.
 * @param timeptr a pointer to the time to set
 *
 * @return int return 0 on success, or a negative error code
 */
static int mcp7940n_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct mcp7940n_data *data = dev->data;
	struct rtc_time timeptr_cpy = *timeptr;
	const struct tm *unix_time = rtc_time_to_tm(&timeptr_cpy);

	int rc = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Encode time */
	rc = encode_rtc(dev, unix_time);
	if (rc < 0) {
		goto out;
	}

	/* Write to device */
	rc = write_data_block(dev, REG_RTC_SEC, RTC_TIME_REGISTERS_SIZE);

out:
	k_mutex_unlock(&data->lock);

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
static int set_day_of_week(const struct device *dev, struct tm *unix_time)
{
	struct mcp7940n_data *data = dev->data;
	int rc;

	data->registers.rtc_weekday.weekday = unix_time->tm_wday;
	rc = write_register(dev, REG_RTC_WDAY, *((uint8_t *)(&data->registers.rtc_weekday)));

	return rc;
}

/**
 * @brief Initialisation function
 *
 * @param dev the MCP7940N device pointer.
 *
 * @return int return 0 on success, or a negative error code
 */
static int mcp7940n_init(const struct device *dev)
{
	struct mcp7940n_data *data = dev->data;
	const struct mcp7940n_config *cfg = dev->config;
	int rc = 0;
	struct tm unix_time = {0};

	/* Initialize and take the lock */
	k_mutex_init(&data->lock);

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C device %s is not ready", cfg->i2c.bus->name);
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
	rc = write_register(dev, REG_RTC_HOUR, *((uint8_t *)(&data->registers.rtc_hours)));
	if (rc < 0) {
		goto out;
	}

	/* Enable battery backup*/
	data->registers.rtc_weekday.vbaten = true;
	rc = write_register(dev, REG_RTC_WDAY, *((uint8_t *)(&data->registers.rtc_weekday)));
	if (rc < 0) {
		goto out;
	}

out:
	k_mutex_unlock(&data->lock);

	return rc;
}

static const struct rtc_driver_api mcp7940n_driver_api = {
	.set_time = mcp7940n_set_time,
	.get_time = mcp7940n_get_time,
};

#define MCP7940N_INIT(inst)                                                                        \
	static const struct mcp7940n_config mcp7940n_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst)};                                                \
                                                                                                   \
	static struct mcp7940n_data mcp7940n_data_##inst;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &mcp7940n_init, NULL, &mcp7940n_data_##inst,                   \
			      &mcp7940n_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,      \
			      &mcp7940n_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCP7940N_INIT)
