/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (C) 2021  Ivona Loncar <ivona.loncar@greyp.com>
 *
 * Datasheet:
 * https://www.st.com/en/clocks-and-timers/m41t62.html
 */

#define DT_DRV_COMPAT st_m41t62

#include <device.h>
#include <drivers/rtc/m41t62.h>
#include <drivers/i2c.h>

#include <assert.h>
#include <logging/log.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

LOG_MODULE_REGISTER(m41t62, CONFIG_COUNTER_LOG_LEVEL);

const int month_adjust = 1;     /* Compensate for month since January [0-11] */
const int year_adjust = 100;    /* Compensate for years since 1900 */

static int m41t62_reg_read(const struct device *dev, uint8_t reg_addr)
{
	struct m41t62_device *m41t62 = dev->data;
	const struct m41t62_config *const bus_config = dev->config;
	uint8_t value;
	int rc;

	rc = i2c_reg_read_byte(m41t62->i2c, bus_config->i2c_addr,
			       reg_addr, &value);
	if (rc < 0) {
		LOG_ERR("Unable to read register. Error occurred: %s\n",
			strerror(-rc));
		return rc;
	}

	return value;
}

static int m41t62_read_mask(const struct device *dev, uint8_t reg_addr,
				uint8_t mask, uint8_t shift)
{
	assert(shift <= M41T62_REGISTER_SIZE);

	int rc;

	rc = m41t62_reg_read(dev, reg_addr);
	if (rc < 0) {
		return rc;
	}

	return (rc & mask) >> shift;
}

static int m41t62_read_bit(const struct device *dev, uint8_t reg_addr,
			       uint8_t bit)
{
	assert(bit <= M41T62_REGISTER_SIZE);

	return m41t62_read_mask(dev, reg_addr, BIT(bit), bit);
}

static int m41t62_write(const struct device *dev, uint8_t reg_addr,
			    uint8_t value)
{
	struct m41t62_device *m41t62 = dev->data;
	const struct m41t62_config *const bus_config = dev->config;

	return i2c_reg_write_byte(m41t62->i2c, bus_config->i2c_addr,
				reg_addr, value);
}

static int m41t62_write_mask(const struct device *dev, uint8_t reg_addr,
				 uint8_t value, uint8_t mask, uint8_t shift)
{
	int rc;

	assert(shift <= M41T62_REGISTER_SIZE);

	rc = m41t62_reg_read(dev, reg_addr);
	if (rc < 0) {
		return rc;
	}

	rc &= ~mask;
	rc |= value << shift;

	return m41t62_write(dev, reg_addr, rc);
}

static int m41t62_write_bit(const struct device *dev, uint8_t reg_addr,
				bool value, uint8_t bit)
{
	assert(bit <= M41T62_REGISTER_SIZE);

	return m41t62_write_mask(dev, reg_addr, value, BIT(bit), bit);
}

int m41t62_restart_oscillator(const struct device *dev)
{
	int rc;

	rc = m41t62_write_bit(dev, M41T62_REG_SEC, 1,
				  M41T62_STOP_BIT);
	if (rc < 0) {
		return rc;
	}

	rc = m41t62_write_bit(dev, M41T62_REG_SEC, 0,
				  M41T62_STOP_BIT);
	if (rc < 0) {
		return rc;
	}

	return rc;
}

int m41t62_set_sqw_freq(const struct device *dev,
			uint8_t frequency)
{
	return m41t62_write_mask(dev, M41T62_REG_WDAY, frequency,
				     M41T62_SQW_FREQUENCY_MASK, 0);
}

int m41t62_get_sqw_freq(const struct device *dev)
{
	int rc;

	rc = m41t62_reg_read(dev, M41T62_REG_WDAY);
	if (rc < 0) {
		LOG_ERR("Failed to read frequency of the square wave. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	return (rc & M41T62_SQW_FREQUENCY_MASK);
}


int m41t62_setting_default_bits(const struct device *dev)
{
	int rc;

	rc = m41t62_write_mask(dev, M41T62_REG_HOUR, 0,
				   M41T62_HOURS_MASK, 0);
	if (rc < 0) {
		LOG_ERR("Unable to write to register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	rc = m41t62_write_mask(dev, M41T62_REG_WDAY, 0,
				   M41T62_WDAY_MASK, 0);
	if (rc < 0) {
		LOG_ERR("Unable to write to register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	rc = m41t62_write_mask(dev, M41T62_REG_DAY, 0,
				   M41T62_HOURS_MASK, 0);
	if (rc < 0) {
		LOG_ERR("Unable to write to register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	rc = m41t62_write_mask(dev, M41T62_REG_ALARM_MON, 0,
				   M41T62_REG_ZERO_BIT_ALARM_MON_MASK, 0);
	if (rc < 0) {
		LOG_ERR("Unable to write to register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	rc = m41t62_write_mask(dev, M41T62_REG_ALARM_HOUR, 0,
				   M41T62_REG_ZERO_BIT_ALARM_HOUR_MASK, 0);
	if (rc < 0) {
		LOG_ERR("Unable to write to register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	rc = m41t62_write_mask(dev, M41T62_REG_FLAGS, 0,
				   M41T62_REG_ZERO_BITS_FLAGS_MASK, 0);
	if (rc < 0) {
		LOG_ERR("Unable to write to register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	return rc;
}

static uint8_t bcd2dec(const uint8_t bcd)
{
	return bcd - 6 * (bcd >> 4);
}

static uint8_t dec2bcd(const uint8_t decimal)
{
	return decimal + 6 * (decimal / 10);
}

static uint32_t decode_rtc(uint8_t *buffer)
{
	int32_t epoch;

	buffer[1] = bcd2dec(buffer[1] & ~M41T62_SECONDS_MASK);
	buffer[2] = bcd2dec(buffer[2] & ~M41T62_MINUTES_MASK);
	buffer[3] = bcd2dec(buffer[3] & ~M41T62_HOURS_MASK);
	buffer[4] = bcd2dec(buffer[4] & ~M41T62_WDAY_MASK);
	buffer[5] = bcd2dec(buffer[5] & ~M41T62_DAYMONTH_MASK);
	buffer[6] = bcd2dec(buffer[6] & ~M41T62_MONTH_MASK);
	buffer[7] = bcd2dec(buffer[7]);

	/* fill ISO C `broken-down time' structure */
	struct tm epoch_tm = { buffer[1],
			       buffer[2],
			       buffer[3],
			       buffer[5],
			       buffer[6] - month_adjust,
			       buffer[7] + year_adjust };
	epoch = (int32_t)mktime(&epoch_tm);

	return epoch;
}

static int read_time(const struct device *dev,
		     uint32_t *time)
{
	struct m41t62_device *m41t62 = dev->data;
	const struct m41t62_config *const bus_config = dev->config;
	uint8_t read_buf[M41T62_DATETIME_REG_SIZE] = { 0 };
	int rc;

	rc = i2c_burst_read(m41t62->i2c, bus_config->i2c_addr,
			    M41T62_FRACTION_SECONDS, read_buf,
			    M41T62_DATETIME_REG_SIZE);
	if (rc == 0) {
		*time = decode_rtc(read_buf);
	}

	return rc;
}

int m41t62_set_time(const struct device *dev, int32_t epoch)
{

	struct m41t62_device *m41t62 = dev->data;
	const struct m41t62_config *const bus_config = dev->config;
	time_t time_value = epoch;
	struct tm *time_buffer = gmtime(&time_value);
	uint8_t read_buf[M41T62_DATETIME_REG_SIZE] = { 0 };

	int rc = i2c_burst_read(m41t62->i2c, bus_config->i2c_addr,
				M41T62_FRACTION_SECONDS, read_buf,
				M41T62_DATETIME_REG_SIZE);

	if (rc < 0) {
		LOG_ERR("Failed to read from time register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	read_buf[0] = 0;

	read_buf[1] &= M41T62_SECONDS_MASK;
	read_buf[1] |= dec2bcd(time_buffer->tm_sec);

	read_buf[2] &= M41T62_MINUTES_MASK;
	read_buf[2] |= dec2bcd(time_buffer->tm_min);

	read_buf[3] &= M41T62_HOURS_MASK;
	read_buf[3] |= dec2bcd(time_buffer->tm_hour);

	read_buf[4] = 0;

	read_buf[5] &= M41T62_DAYMONTH_MASK;
	read_buf[5] |= dec2bcd(time_buffer->tm_mday);

	read_buf[6] &= M41T62_MONTH_MASK;
	read_buf[6] |= dec2bcd(time_buffer->tm_mon + month_adjust);

	read_buf[7] &= M41T62_YEAR_MASK;
	read_buf[7] |= dec2bcd(time_buffer->tm_year - year_adjust);

	rc = i2c_burst_write(m41t62->i2c, bus_config->i2c_addr,
			     M41T62_FRACTION_SECONDS, read_buf,
			     M41T62_DATETIME_REG_SIZE);
	if (rc < 0) {
		LOG_ERR("Failed to write into adjust register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	return rc;
}

static int m41t62_counter_get_value(const struct device *dev,
				    uint32_t *ticks)
{
	return read_time(dev, ticks);

}

int m41t62_set_alarm(const struct device *dev, uint32_t time_in_epoch)
{
	struct m41t62_device *m41t62 = dev->data;
	const struct m41t62_config *const bus_config = dev->config;
	time_t time_value = time_in_epoch;
	struct tm *time_buffer = gmtime(&time_value);
	int rc;
	uint8_t read_buf[5];

	rc = i2c_burst_read(m41t62->i2c, bus_config->i2c_addr,
			    M41T62_REG_ALARM_MON, read_buf,
			    ARRAY_SIZE(read_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read from alarm register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	read_buf[4] &= M41T62_AL_SEC_MASK;
	read_buf[4] |= dec2bcd(time_buffer->tm_sec);

	read_buf[3] &= M41T62_AL_MIN_MASK;
	read_buf[3] |= dec2bcd(time_buffer->tm_min);

	read_buf[2] &= M41T62_AL_HOUR_MASK;
	read_buf[2] |=  dec2bcd(time_buffer->tm_hour);

	read_buf[1] &= M41T62_AL_DATE_MASK;
	read_buf[1] |= dec2bcd(time_buffer->tm_mday);

	read_buf[0] &= M41T62_AL_MONTH_MASK;
	read_buf[0] |= dec2bcd(time_buffer->tm_mon + month_adjust);

	/* If the last address written is the Alarm Seconds, the address
	 * pointer will increment to the address of the Flags register.
	 * Therefore an alarm condition will not cause the interrupt/flag to occur.
	 * That is why the write addresses are decreasing in this case and therefore
	 * the i2c_burst_write function can't be used.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(read_buf); ++i) {
		rc =
			i2c_reg_write_byte(m41t62->i2c, bus_config->i2c_addr,
					   M41T62_REG_ALARM_SEC - i, read_buf[4 - i]);
		if (rc < 0) {
			LOG_ERR("Failed to write into alarm register. Error: %s\n",
				strerror(-rc));
			return rc;
		}
	}

	return rc;
}

static int m41t62_counter_cancel_alarm(const struct device *dev, uint8_t id)
{

	struct m41t62_device *m41t62 = dev->data;
	const struct m41t62_config *const bus_config = dev->config;
	uint8_t alarm_data[4];
	int rc;

	rc = i2c_burst_read(m41t62->i2c, bus_config->i2c_addr,
			    M41T62_REG_ALARM_DAY, alarm_data,
			    ARRAY_SIZE(alarm_data));
	if (rc < 0) {
		LOG_ERR("Failed to read from register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	for (size_t i = 0; i < ARRAY_SIZE(alarm_data); ++i) {
		alarm_data[i] = 0;
	}
	/* Write '0' to the alarm date registers in order to disable the alarm,*/
	rc = i2c_burst_write(m41t62->i2c, bus_config->i2c_addr,
			     M41T62_REG_ALARM_DAY, alarm_data,
			     ARRAY_SIZE(alarm_data));
	if (rc < 0) {
		LOG_ERR("Failed to write into alarm register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	/* The address point has to be moved from the flags registers*/
	rc = i2c_reg_read_byte(m41t62->i2c, bus_config->i2c_addr,
			       M41T62_REG_ALARM_DAY, alarm_data);
	if (rc < 0) {
		LOG_ERR("Failed to read from enable alarm register. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	return rc;
}

int m41t62_ctrl_update(const struct device *dev,
		       enum control_bits bit_name, uint8_t value)
{
	switch (bit_name) {
	case SQWE_BIT:
		return m41t62_write_bit(dev, M41T62_REG_ALARM_MON,
					    value, M41T62_SQWE_BIT);
	case STOP_BIT:
		return m41t62_write_bit(dev, M41T62_REG_SEC, value,
					    M41T62_STOP_BIT);
	case OSCILLATOR_FAIL_BIT:
		return m41t62_write_bit(dev, M41T62_REG_FLAGS, value,
					    M41T62_OSCILLATOR_FAIL_BIT);
	case ALARM_FLAG_ENABLE:
		return m41t62_write_bit(dev, M41T62_REG_ALARM_MON,
					    value, M41T62_AFE_BIT);
	default:
		LOG_ERR("Given bit not supported.");
		return -ENOTSUP;
	}

	return -EINVAL;
}

int m41t62_ctrl_read(const struct device *dev,
		     enum control_bits bit_name, uint8_t *value)
{
	int rc;

	switch (bit_name) {
	case SQWE_BIT:
		rc = m41t62_read_bit(dev, M41T62_REG_ALARM_MON,
					 M41T62_SQWE_BIT);
		break;
	case ALARM_FLAG:
		rc = m41t62_read_bit(dev, M41T62_REG_FLAGS,
					 M41T62_AF_BIT);
		break;
	case ALARM_FLAG_ENABLE:
		rc = m41t62_read_bit(dev, M41T62_REG_ALARM_MON,
					 M41T62_AFE_BIT);
		break;
	case OSCILLATOR_FAIL_BIT:
		rc = m41t62_read_bit(dev, M41T62_REG_FLAGS,
					 M41T62_OSCILLATOR_FAIL_BIT);
		break;
	case STOP_BIT:
		rc = m41t62_read_bit(dev, M41T62_REG_SEC,
					 M41T62_STOP_BIT);
		break;
	default:
		LOG_ERR("Given bit not supported.");
		return -ENOTSUP;
	}

	if (rc < 0) {
		return rc;
	}

	*value = rc;

	return 0;
}

static int m41t62_counter_start(const struct device *dev)
{
	return -EALREADY;
}

static int m41t62_counter_stop(const struct device *dev)
{
	return -ENOTSUP;
}


static uint32_t m41t62_counter_get_top_value(const struct device *dev)
{
	return UINT32_MAX;
}

static uint32_t m41t62_counter_get_pending_int(const struct device *dev)
{
	return 0;
}

static int m41t62_counter_set_top_value(const struct device *dev,
					const struct counter_top_cfg *cfg)
{
	return -ENOTSUP;
}

static int m41t62_counter_set_alarm(const struct device *dev,
				    uint8_t id,
				    const struct counter_alarm_cfg *alarm_cfg)
{
	return -ENOTSUP;
}

static int m41t62_init(const struct device *dev)
{
	int rc;
	struct m41t62_device *m41t62 = dev->data;
	const struct m41t62_config *const bus_config = dev->config;

	if (!device_is_ready(bus_config->bus)) {
		LOG_ERR("Device not found");
		return -ENODEV;
	}

	m41t62->i2c = device_get_binding(bus_config->bus_name);
	if (!m41t62->i2c) {
		LOG_ERR("I2C master controller not found: %s\n",
			bus_config->bus_name);
		return -EINVAL;
	}

	rc = m41t62_setting_default_bits(dev);
	if (rc < 0) {
		LOG_ERR("Unable to write to control registers. Error: %s\n",
			strerror(-rc));
		return rc;
	}

	return rc;

}

static struct m41t62_device m41t62_device;

static const struct counter_driver_api m41t62_api = {
	.start = m41t62_counter_start,
	.stop = m41t62_counter_stop,
	.get_value = m41t62_counter_get_value,
	.set_alarm = m41t62_counter_set_alarm,
	.cancel_alarm = m41t62_counter_cancel_alarm,
	.set_top_value = m41t62_counter_set_top_value,
	.get_pending_int = m41t62_counter_get_pending_int,
	.get_top_value = m41t62_counter_get_top_value,
};

static const struct m41t62_config m41t62_config = {
	.bus =  DEVICE_DT_GET(DT_INST_BUS(0)),
	.bus_name = DT_INST_BUS_LABEL(0),
	.i2c_addr = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, m41t62_init, device_pm_control_nop, &m41t62_device,
		      &m41t62_config, POST_KERNEL,
		      CONFIG_COUNTER_M41T62_INIT_PRIORITY,
		      &m41t62_api);
