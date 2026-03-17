/*
 * Copyright (c) 2026 Plentify (Pty) Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp7940n

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/rtc/mcp7940n.h>
#include <zephyr/logging/log.h>

#include "rtc_utils.h"
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(MCP7940N, CONFIG_RTC_LOG_LEVEL);

#define MCP7940N_RTC_TIME_MASK                                                                     \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

/* Size of block when writing whole struct */
#define RTC_TIME_REGISTERS_SIZE  sizeof(struct mcp7940n_time_registers)
#define RTC_ALARM_REGISTERS_SIZE sizeof(struct mcp7940n_alarm_registers)

#define MCP7940N_YEAR_OFFSET 100

/* Largest block size */
#define MAX_WRITE_SIZE (RTC_TIME_REGISTERS_SIZE)

struct mcp7940n_config {
	struct i2c_dt_spec i2c;
};

struct mcp7940n_data {
	struct k_sem lock;
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

	uint8_t buf[2] = {addr, value};

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

/** @brief Convert bcd time in device registers to rtc_time
 *
 * @param dev the MCP7940N device pointer.
 * @param timeptr pointer to rtc_time struct to populate.
 */
static void decode_rtc(const struct device *dev, struct rtc_time *timeptr)
{
	struct mcp7940n_data *data = dev->data;

	timeptr->tm_sec = bcd2bin(*(uint8_t *)&data->registers.rtc_sec & 0x7F);
	timeptr->tm_min = bcd2bin(*(uint8_t *)&data->registers.rtc_min & 0x7F);
	timeptr->tm_hour = bcd2bin(*(uint8_t *)&data->registers.rtc_hours & 0x3F);
	timeptr->tm_mday = bcd2bin(*(uint8_t *)&data->registers.rtc_date & 0x3F);
	timeptr->tm_wday = data->registers.rtc_weekday.weekday;
	/* rtc_time starts months at 0, mcp7940n starts at 1 */
	timeptr->tm_mon = bcd2bin(*(uint8_t *)&data->registers.rtc_month & 0x1F) - 1;
	timeptr->tm_year = bcd2bin(*(uint8_t *)&data->registers.rtc_year) + MCP7940N_YEAR_OFFSET;
}

/** @brief Read registers from device and populate rtc_time struct
 *
 * @param dev the MCP7940N device pointer.
 * @param timeptr pointer to rtc_time struct that will be populated if
 * successful.
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction.
 */
static int read_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct mcp7940n_data *data = dev->data;
	const struct mcp7940n_config *cfg = dev->config;
	uint8_t addr = REG_RTC_SEC;

	int rc = i2c_write_read_dt(&cfg->i2c, &addr, sizeof(addr), &data->registers,
				   RTC_TIME_REGISTERS_SIZE);
	if (rc < 0) {
		return rc;
	}

	decode_rtc(dev, timeptr);

	return 0;
}

/** @brief Encode rtc_time into mcp7940n rtc registers
 *
 * @param dev the MCP7940N device pointer.
 * @param timeptr rtc_time struct containing time to be encoded into mcp7940n
 * registers.
 *
 * @retval return 0 on success, or a negative error code from invalid
 * parameter.
 */
static int encode_rtc(const struct device *dev, const struct rtc_time *timeptr)
{
	struct mcp7940n_data *data = dev->data;
	uint8_t month;
	uint8_t year_since_epoch;

	/* In rtc_time, months start at 0, mcp7940n starts with 1 */
	month = timeptr->tm_mon + 1;

	if (timeptr->tm_year < MCP7940N_YEAR_OFFSET) {
		return -EINVAL;
	}
	year_since_epoch = timeptr->tm_year - MCP7940N_YEAR_OFFSET;

	/* Set external oscillator configuration bit */
	data->registers.rtc_sec.start_osc = 1;

	*(uint8_t *)&data->registers.rtc_sec =
		(*(uint8_t *)&data->registers.rtc_sec & 0x80) | bin2bcd(timeptr->tm_sec);
	*(uint8_t *)&data->registers.rtc_min = bin2bcd(timeptr->tm_min);
	*(uint8_t *)&data->registers.rtc_hours =
		(*(uint8_t *)&data->registers.rtc_hours & 0xC0) | bin2bcd(timeptr->tm_hour);
	data->registers.rtc_weekday.weekday = timeptr->tm_wday;
	*(uint8_t *)&data->registers.rtc_date = bin2bcd(timeptr->tm_mday);
	*(uint8_t *)&data->registers.rtc_month =
		(*(uint8_t *)&data->registers.rtc_month & 0xE0) | bin2bcd(month);
	*(uint8_t *)&data->registers.rtc_year = bin2bcd(year_since_epoch);

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
	__ASSERT(dev != NULL, "device required");
	__ASSERT(timeptr != NULL, "rtc_time reference required");

	struct mcp7940n_data *data = dev->data;
	int rc;

	k_sem_take(&data->lock, K_FOREVER);

	/* Get time */
	rc = read_time(dev, timeptr);
	if (rc < 0) {
		goto out;
	}

	if (!data->registers.rtc_weekday.oscrun) {
		LOG_WRN("oscillator not running");
		rc = -ENODATA;
	}

out:
	k_sem_give(&data->lock);

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
	__ASSERT(dev != NULL, "device required");
	__ASSERT(timeptr != NULL, "rtc_time reference required");

	struct mcp7940n_data *data = dev->data;
	int rc = 0;

	if (rtc_utils_validate_rtc_time(timeptr, MCP7940N_RTC_TIME_MASK) == false) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	/* Encode time */
	rc = encode_rtc(dev, timeptr);
	if (rc < 0) {
		goto out;
	}

	/* Write to device */
	rc = write_data_block(dev, REG_RTC_SEC, RTC_TIME_REGISTERS_SIZE);

out:
	k_sem_give(&data->lock);

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
	struct rtc_time timeptr = {0};

	/* Initialize and take the lock */
	k_sem_init(&data->lock, 1, 1);

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C device %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	k_sem_take(&data->lock, K_FOREVER);

	rc = read_time(dev, &timeptr);
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
	k_sem_give(&data->lock);

	return rc;
}

static DEVICE_API(rtc, mcp7940n_driver_api) = {
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
