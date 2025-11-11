/*
 * Copyright (c) 2025 Psicontrol N.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_hdc302x

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/crc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ti_hdc302x.h>

LOG_MODULE_REGISTER(TI_HDC302X, CONFIG_SENSOR_LOG_LEVEL);

/* Register commands (2-byte arrays) */
static const uint8_t REG_MEAS_AUTO_READ[] = {0xE0, 0x00};
static const uint8_t REG_MEAS_AUTO_EXIT[] = {0x30, 0x93};
static const uint8_t REG_MANUFACTURER_ID[] = {0x37, 0x81};
static const uint8_t REG_SOFT_RESET[] = {0x30, 0xA2};
static const uint8_t REG_READ_STATUS[] = {0xF3, 0x2D};
static const uint8_t REG_RESET_STATUS[] = {0x30, 0x41};
static const uint8_t REG_OFFSET[] = {0xA0, 0x04};
static const uint8_t REG_HEATER_ON[] = {0x30, 0x6D};
static const uint8_t REG_HEATER_OFF[] = {0x30, 0x66};
static const uint8_t REG_HEATER_LEVEL[] = {0x30, 0x6E};

struct ti_hdc302x_config {
	const struct i2c_dt_spec bus;
	const struct gpio_dt_spec int_gpio;
};

struct ti_hdc302x_data {
	struct gpio_callback cb_int;
	sensor_trigger_handler_t th_handler;
	const struct sensor_trigger *th_trigger;
	uint16_t t_sample;
	uint16_t rh_sample;
	uint16_t t_alert;
	uint16_t rh_alert;
	uint8_t t_offset;
	uint8_t rh_offset;
	enum sensor_power_mode_hdc302x power_mode;
	enum sensor_measurement_interval_hdc302x interval;
	uint8_t selected_mode[2];
};

/* Alert status registers */
static const uint8_t alert_set_commands[][2] = {
	{0x61, 0x00},
	{0x61, 0x1D},
	{0x61, 0x0B},
	{0x61, 0x16},
};
static const uint8_t alert_read_commands[][2] = {
	{0xE1, 0x02},
	{0xE1, 0x1F},
	{0xE1, 0x09},
	{0xE1, 0x14},
};

/* Register values */
#define HDC_302X_MANUFACTURER_ID 0x3000

/* CRC parameters */
#define HDC_302X_CRC8_POLYNOMIAL    0x31
#define HDC_302X_CRC8_INITIAL_VALUE 0xFF

/* Reset timing */
#define HDC_302X_RESET_TIME K_MSEC(1)

/* Conversion constants from datasheet */
#define HDC_302X_RH_SCALE              100U
#define HDC_302X_TEMP_OFFSET           -45
#define HDC_302X_TEMP_SCALE            175U
/* Temperature offset: 7-bit value, max ±21.704101°C, 0.1708984375°C per bit */
#define HDC_302X_TEMP_OFFSET_SCALE     170.8984375
/* Humidity offset: 7-bit value, max ±24.8046875%, 0.1953125% per bit */
#define HDC_302X_HUMIDITY_OFFSET_SCALE 19.53125
/* EEPROM write timeout  in milliseconds (53–77 ms, use 80 ms to be safe) */
#define HDC_302X_EEPROM_WRITE_TIME_OUT 80

/* Conversion direction enum */
typedef enum {
	RAW_TO_SENSOR,
	SENSOR_TO_RAW
} conversion_direction_t;

/* Conversion parameters structure */
struct conversion_params {
	int32_t scale;  /* Scale factor in 16.16 fixed point */
	int32_t offset; /* Offset in 16.16 fixed point */
};

/* Predefined conversion parameters */
static const struct conversion_params temp_params = {.scale = HDC_302X_TEMP_SCALE,
						     .offset = HDC_302X_TEMP_OFFSET};

static const struct conversion_params humidity_params = {
	.scale = HDC_302X_RH_SCALE, .offset = 0 /* No offset for humidity */
};

/* Lookup table for power modes and measurement intervals */
static const uint8_t
	mode_commands[HDC302X_SENSOR_POWER_MODE_MAX][HDC302X_SENSOR_MEAS_INTERVAL_MAX][2] = {
		/* HDC302X_SENSOR_POWER_MODE_0 (LPM0) */
		{
			[HDC302X_SENSOR_MEAS_INTERVAL_MANUAL] = {0x24, 0x00},
			[HDC302X_SENSOR_MEAS_INTERVAL_0_5] = {0x20, 0x32},
			[HDC302X_SENSOR_MEAS_INTERVAL_1] = {0x21, 0x30},
			[HDC302X_SENSOR_MEAS_INTERVAL_2] = {0x22, 0x36},
			[HDC302X_SENSOR_MEAS_INTERVAL_4] = {0x23, 0x34},
			[HDC302X_SENSOR_MEAS_INTERVAL_10] = {0x27, 0x37},
		},
		/* HDC302X_SENSOR_POWER_MODE_1 (LPM1) */
		{
			[HDC302X_SENSOR_MEAS_INTERVAL_MANUAL] = {0x24, 0x0B},
			[HDC302X_SENSOR_MEAS_INTERVAL_0_5] = {0x20, 0x24},
			[HDC302X_SENSOR_MEAS_INTERVAL_1] = {0x21, 0x26},
			[HDC302X_SENSOR_MEAS_INTERVAL_2] = {0x22, 0x20},
			[HDC302X_SENSOR_MEAS_INTERVAL_4] = {0x23, 0x22},
			[HDC302X_SENSOR_MEAS_INTERVAL_10] = {0x27, 0x21},
		},
		/* HDC302X_SENSOR_POWER_MODE_2 (LPM2) */
		{
			[HDC302X_SENSOR_MEAS_INTERVAL_MANUAL] = {0x24, 0x16},
			[HDC302X_SENSOR_MEAS_INTERVAL_0_5] = {0x20, 0x2F},
			[HDC302X_SENSOR_MEAS_INTERVAL_1] = {0x21, 0x2D},
			[HDC302X_SENSOR_MEAS_INTERVAL_2] = {0x22, 0x2B},
			[HDC302X_SENSOR_MEAS_INTERVAL_4] = {0x23, 0x29},
			[HDC302X_SENSOR_MEAS_INTERVAL_10] = {0x27, 0x2A},
		},
		/* HDC302X_SENSOR_POWER_MODE_3 (LPM3) */
		{
			[HDC302X_SENSOR_MEAS_INTERVAL_MANUAL] = {0x24, 0xFF},
			[HDC302X_SENSOR_MEAS_INTERVAL_0_5] = {0x20, 0xFF},
			[HDC302X_SENSOR_MEAS_INTERVAL_1] = {0x21, 0xFF},
			[HDC302X_SENSOR_MEAS_INTERVAL_2] = {0x22, 0xFF},
			[HDC302X_SENSOR_MEAS_INTERVAL_4] = {0x23, 0xFF},
			[HDC302X_SENSOR_MEAS_INTERVAL_10] = {0x27, 0xFF},
		},
};
/**
 * @brief Verify CRC for a given data buffer.
 */
static bool verify_crc(const uint8_t *data, size_t len, uint8_t expected_crc)
{
	uint8_t calculated_crc =
		crc8(data, len, HDC_302X_CRC8_POLYNOMIAL, HDC_302X_CRC8_INITIAL_VALUE, false);
	return calculated_crc == expected_crc;
}

/**
 * @brief Calculate CRC for a given data buffer.
 * @param data Pointer to the data buffer.
 * @param len Length of the data buffer.
 * @return Calculated CRC value.
 */
static uint8_t calculate_crc(const uint8_t *data, size_t len)
{
	return crc8(data, len, HDC_302X_CRC8_POLYNOMIAL, HDC_302X_CRC8_INITIAL_VALUE, false);
}

/**
 * Generic sensor conversion function
 * Formula: sensor_value = offset + scale * (raw_val / 65535)
 *
 * Temperature: T(°C) = -45 + [175 * (RAW_VAL/65535)] -> scale=175, offset=-45
 * Humidity: RH(%) = 0 + [100 * (RAW_VAL/65535)] -> scale=100, offset=0
 *
 * @param raw_value Pointer to raw value (0-65535), input for forward, output for reverse
 * @param sensor_val Pointer to sensor value struct, output for forward, input for reverse
 * @param params Pointer to conversion parameters (scale and offset)
 * @param direction Conversion direction (RAW_TO_SENSOR or SENSOR_TO_RAW)
 */
static void convert_sensor_value(uint16_t *raw_value, struct sensor_value *sensor_val,
				 const struct conversion_params *params,
				 conversion_direction_t direction)
{
	int64_t numerator;
	int32_t remainder;
	int64_t sensor_micro;
	int64_t offset_micro;
	int64_t denominator;
	int32_t raw_calc;

	if (direction == RAW_TO_SENSOR) {
		/* Forward conversion: sensor_value = offset + scale * (raw_val / 65535) */
		/* Calculate: (offset * 65535 + scale * raw_val) / 65535 */
		/* Use 64-bit arithmetic to prevent overflow */
		numerator = (int64_t)params->offset * UINT16_MAX +
			    (int64_t)params->scale * (*raw_value);

		/* Integer part */
		sensor_val->val1 = (int32_t)(numerator / UINT16_MAX);

		/* Fractional part in microseconds */
		remainder = (int32_t)(numerator % UINT16_MAX);
		if (remainder < 0) {
			/* Handle negative remainders properly */
			sensor_val->val1 -= 1;
			remainder += UINT16_MAX;
		}

		/* Convert remainder to microseconds: remainder * 1000000 / 65535 */
		sensor_val->val2 = ((int64_t)remainder * 1000000LL) / UINT16_MAX;
	} else {
		sensor_micro = (int64_t)sensor_val->val1 * 1000000LL + (int64_t)sensor_val->val2;
		offset_micro = (int64_t)params->offset * 1000000LL;
		numerator = (sensor_micro - offset_micro) * UINT16_MAX;
		denominator = (int64_t)params->scale * 1000000LL;
		raw_calc = (int32_t)(numerator / denominator);

		/* Clamp to valid 16-bit unsigned range */
		if (raw_calc < 0) {
			*raw_value = 0;
		} else if (raw_calc > UINT16_MAX) {
			*raw_value = UINT16_MAX;
		} else {
			*raw_value = (uint16_t)raw_calc;
		}
		LOG_DBG("Converted sensor value: %d.%06d to raw value: %x", sensor_val->val1,
			sensor_val->val2, *raw_value);
	}
}

/**
 * @brief Convert raw temperature value to sensor_value or vice versa.
 * @param raw_temp Pointer to raw temperature value (16-bit).
 * @param temp_val Pointer to sensor_value.
 * @param direction Conversion direction (RAW_TO_SENSOR or SENSOR_TO_RAW).
 */
static void convert_temperature(uint16_t *raw_temp, struct sensor_value *temp_val,
				conversion_direction_t direction)
{
	convert_sensor_value(raw_temp, temp_val, &temp_params, direction);
}

/**
 * @brief Convert raw humidity value to sensor_value or vice versa.
 * @param raw_humidity Pointer to raw humidity value (16-bit).
 * @param humidity_val Pointer to sensor_value.
 * @param direction Conversion direction (RAW_TO_SENSOR or SENSOR_TO_RAW).
 */
static void convert_humidity(uint16_t *raw_humidity, struct sensor_value *humidity_val,
			     conversion_direction_t direction)
{
	convert_sensor_value(raw_humidity, humidity_val, &humidity_params, direction);
}

/**
 * @brief Write a command to the sensor.
 * @param dev Pointer to the device structure.
 * @param cmd Command to write (byte array).
 * @param len Length of the command.
 * @return 0 on success, negative error code on failure.
 */
static int write_command(const struct device *dev, const uint8_t *cmd, size_t len)
{
	const struct ti_hdc302x_config *config = dev->config;

	return i2c_write_dt(&config->bus, cmd, len);
}

/**
 * @brief Read sensor data from the device.
 * @param dev Pointer to the device structure.
 * @param buf Pointer to the buffer to store the read data.
 * @param len Length of the data to read.
 * @return 0 on success, negative error code on failure.
 */
static int read_sensor_data(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct ti_hdc302x_config *config = dev->config;

	return i2c_read_dt(&config->bus, buf, len);
}

static void interrupt_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(pins);
	struct ti_hdc302x_data *data = CONTAINER_OF(cb, struct ti_hdc302x_data, cb_int);

	if (data->th_handler != NULL) {
		data->th_handler(dev, data->th_trigger);
	}
}

/**
 * @brief Fetch sensor sample data from sensor. Store raw bytes in data structure.
 * @param dev Pointer to the device structure.
 * @param chan Sensor channel to fetch data for.
 * @return 0 on success, negative error code on failure.
 */
static int ti_hdc302x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ti_hdc302x_data *data = dev->data;
	uint8_t buf[6];
	int rc;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* Trigger measurement based on mode */
	if (data->interval == HDC302X_SENSOR_MEAS_INTERVAL_MANUAL) {
		rc = write_command(dev, data->selected_mode, 2);
		if (rc < 0) {
			LOG_ERR("Failed to trigger manual measurement: %d", rc);
			return rc;
		}
	} else {
		rc = write_command(dev, REG_MEAS_AUTO_READ, 2);
		if (rc < 0) {
			LOG_ERR("Failed to read auto measurement: %d", rc);
			return rc;
		}
	}

	/* Read temperature and humidity data (6 bytes: T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB,
	 * RH_CRC)
	 */
	rc = read_sensor_data(dev, buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Failed to read sensor data: %d", rc);
		return rc;
	}

	/* Verify CRC for temperature */
	if (!verify_crc(&buf[0], 2, buf[2])) {
		LOG_ERR("Temperature CRC verification failed");
		return -EIO;
	}

	/* Verify CRC for humidity */
	if (!verify_crc(&buf[3], 2, buf[5])) {
		LOG_ERR("Humidity CRC verification failed");
		return -EIO;
	}

	/* Store raw values */
	data->t_sample = sys_get_be16(&buf[0]);
	data->rh_sample = sys_get_be16(&buf[3]);

	return 0;
}

/**
 * @brief Get sensor channel data previously read by calling ti_hdc302x_sample_fetch().
 * @param dev Pointer to the device structure.
 * @param chan Sensor channel to get data for.
 * @param val Pointer to sensor_value structure to store the result.
 * @return 0 on success, negative error code on failure.
 */
static int ti_hdc302x_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct ti_hdc302x_data *data = dev->data;
	struct sensor_value temp, humidity;

	convert_temperature(&(data->t_sample), &temp, RAW_TO_SENSOR);
	convert_humidity(&(data->rh_sample), &humidity, RAW_TO_SENSOR);

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		*val = temp;
		break;
	case SENSOR_CHAN_HUMIDITY:
		*val = humidity;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Log the status bits of the sensor.
 * @param status The status bits to log.
 */
static void log_status_bits(uint16_t status)
{
	if ((status & TI_HDC302X_STATUS_REG_BIT_ALERT) != 0) {
		LOG_DBG("Alert: At least one active alert");
	}
	if ((status & TI_HDC302X_STATUS_REG_BIT_HEATER_ON) != 0) {
		LOG_DBG("Alert: Heater is ON");
	}
	if ((status & TI_HDC302X_STATUS_REG_BIT_RH_ALERT) != 0) {
		LOG_DBG("Alert: RH alert active");
	}
	if ((status & TI_HDC302X_STATUS_REG_BIT_TEMP_ALERT) != 0) {
		LOG_DBG("Alert: Temperature alert active");
	}
	if ((status & TI_HDC302X_STATUS_REG_BIT_RH_HIGH_ALERT) != 0) {
		LOG_DBG("Alert: RH high threshold exceeded");
	}
	if ((status & TI_HDC302X_STATUS_REG_BIT_RH_LOW_ALERT) != 0) {
		LOG_DBG("Alert: RH low threshold exceeded");
	}
	if ((status & TI_HDC302X_STATUS_REG_BIT_TEMP_HIGH_ALERT) != 0) {
		LOG_DBG("Alert: Temperature high threshold exceeded");
	}
	if ((status & TI_HDC302X_STATUS_REG_BIT_TEMP_LOW_ALERT) != 0) {
		LOG_DBG("Alert: Temperature low threshold exceeded");
	}
	if ((status & TI_HDC302X_STATUS_REG_BIT_RESET_DETECTED) != 0) {
		LOG_DBG("Alert: Reset detected");
	}
	if ((status & TI_HDC302X_STATUS_REG_BIT_CRC_FAILED) != 0) {
		LOG_DBG("Alert: CRC failure detected");
	}
}

/**
 * @brief Read the status register of the sensor.
 * @param dev Pointer to the device structure.
 * @param status Pointer to store the status register value.
 * @return 0 on success, negative error code on failure.
 */
static int read_status_register(const struct device *dev, uint16_t *status)
{
	uint8_t buf[3];
	int rc;

	rc = write_command(dev, REG_READ_STATUS, 2);
	if (rc < 0) {
		LOG_ERR("Failed to request status register: %d", rc);
		return rc;
	}

	rc = read_sensor_data(dev, buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Failed to read status register: %d", rc);
		return rc;
	}

	if (!verify_crc(&buf[0], 2, buf[2])) {
		LOG_ERR("Status register CRC verification failed");
		return -EIO;
	}

	*status = sys_get_be16(&buf[0]);
	return 0;
}

static int set_power_mode_and_interval(const struct device *dev)
{
	struct ti_hdc302x_data *data = dev->data;
	int rc;

	/* Update selected mode command */
	memcpy(data->selected_mode, mode_commands[data->power_mode][data->interval],
	       sizeof(data->selected_mode));

	if (data->interval != HDC302X_SENSOR_MEAS_INTERVAL_MANUAL) {
		/* Enable automatic mode */
		rc = write_command(dev, data->selected_mode, 2);
		if (rc < 0) {
			LOG_ERR("Failed to enable automatic mode: %d", rc);
			return rc;
		}
	} else {
		/* Exit automatic mode */
		rc = write_command(dev, REG_MEAS_AUTO_EXIT, 2);
		if (rc < 0) {
			LOG_ERR("Failed to exit automatic mode: %d", rc);
			return rc;
		}
	}

	return 0;
}

static int convert_alert_threshold(struct ti_hdc302x_data *data, const uint8_t *buffer)
{
	/* Check CRC */
	if (verify_crc(buffer, 2, buffer[2]) != true) {
		LOG_ERR("CRC check failed for Alert data");
		return -EIO;
	}
	uint16_t tmp = sys_get_be16(buffer);

	data->t_alert = (tmp & 0x01FF) << 7; /* Extract temperature alert bits*/
	data->rh_alert = tmp & 0xFE00;
	return 0;
}

static void generate_alert_threshold(struct ti_hdc302x_data *data, uint8_t *buf, int offset)
{
	uint16_t tmp;

	tmp = ((data->t_alert & 0xFF10) >> 7) + offset;
	tmp += (data->rh_alert & 0xFE00) + (offset << 9);

	sys_put_be16(tmp, buf);
	buf[2] = calculate_crc(buf, 2); /* Calculate CRC for the data */
}

static int read_threshold(const struct device *dev, enum sensor_channel chan,
			  const struct sensor_value *val, bool upper, bool clear)
{
	struct ti_hdc302x_data *data = dev->data;
	int rc = 0;
	uint8_t buf[3];
	struct sensor_value temp, humidity;

	int alert_type = (upper ? 0x01 : 0x00) |
			 (clear ? 0x02 : 0x00); /* 0x01 for upper, 0x00 for lower, 0x02 for clear */

	rc = write_command(dev, alert_read_commands[alert_type], 2);
	if (rc < 0) {
		LOG_ERR("Failed to request Manual Mode readout");
		return rc;
	}
	rc = read_sensor_data(dev, (uint8_t *)buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Failed to read Alert data");
		return rc;
	}

	convert_alert_threshold(data, buf);
	convert_temperature(&data->t_alert, &temp, RAW_TO_SENSOR);
	convert_humidity(&data->rh_alert, &humidity, RAW_TO_SENSOR);
	LOG_DBG("Alert data: T Alert: %d.%06d(%d), RH Alert: %d.%06d (%d)", temp.val1, temp.val2,
		data->t_alert, humidity.val1, humidity.val2, data->rh_alert);

	return 0;
}

static int set_threshold(const struct device *dev, enum sensor_channel chan,
			 const struct sensor_value *val, bool upper)
{
	struct ti_hdc302x_data *data = dev->data;
	int rc = 0;
	uint8_t buf[5];
	uint8_t buf_clear[5];
	int alert_type;

	rc = read_threshold(dev, chan, val, upper, false);
	if (rc < 0) {
		LOG_ERR("Failed to read current threshold");
		return rc;
	}

	alert_type = (upper ? 0x01 : 0x00);
	memcpy(&buf[0], alert_set_commands[alert_type], 2);

	alert_type = (upper ? 0x01 : 0x00) | 0x02;
	memcpy(&buf_clear[0], alert_set_commands[alert_type], 2);

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		convert_temperature(&data->t_alert, (struct sensor_value *)val, SENSOR_TO_RAW);
		break;
	case SENSOR_CHAN_HUMIDITY:
		convert_humidity(&data->rh_alert, (struct sensor_value *)val, SENSOR_TO_RAW);
		break;
	default:
		return -ENOTSUP;
	}

	/* Generate alert threshold */
	generate_alert_threshold(data, &buf[2], 0);

	rc = write_command(dev, buf, 5);
	if (rc < 0) {
		LOG_ERR("Failed to set current threshold");
	}
	/* Generate clear alert threshold */
	generate_alert_threshold(data, &buf_clear[2], (upper ? -1 : 1));

	rc = write_command(dev, buf_clear, 5);
	if (rc < 0) {
		LOG_ERR("Failed to set current clear threshold");
	}

	rc = read_threshold(dev, chan, val, upper, false);
	if (rc < 0) {
		LOG_ERR("Failed to read current threshold2");
		return rc;
	}
	rc = read_threshold(dev, chan, val, upper, true);
	if (rc < 0) {
		LOG_ERR("Failed to read current threshold3");
		return rc;
	}

	return 0;
}

static void convert_offset_to_value(uint8_t offset, int16_t *raw_offset, double scale)
{
	bool add = true;

	if ((offset & 0x80) == 0) {
		add = false;
	}
	int16_t offset_bits = offset & 0x7F;
	*raw_offset = (add ? 1 : -1) * offset_bits * scale;
}

static bool convert_offset_to_temperature(uint8_t offset, struct sensor_value *val)
{
	int16_t temp_offset_mdeg;

	convert_offset_to_value(offset, &temp_offset_mdeg, HDC_302X_TEMP_OFFSET_SCALE);

	val->val1 = temp_offset_mdeg / 1000;          /* Convert to degrees Celsius */
	val->val2 = (temp_offset_mdeg % 1000) * 1000; /* Convert to microdegrees Celsius */

	LOG_DBG("Converted temperature offset: %d.%06d from raw value: %x", val->val1, val->val2,
		offset);
	return true;
}

static bool convert_offset_to_humidity(uint8_t offset, struct sensor_value *val)
{
	int16_t rh_offset_mdeg;

	convert_offset_to_value(offset, &rh_offset_mdeg, HDC_302X_HUMIDITY_OFFSET_SCALE);

	val->val1 = rh_offset_mdeg / 100;           /* Convert to percent relative humidity */
	val->val2 = (rh_offset_mdeg % 100) * 10000; /* Convert to micropercent relative humidity */

	LOG_DBG("Converted humidity offset: %d.%06d from raw value: %x", val->val1, val->val2,
		offset);
	return true;
}

static int get_offset(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	struct ti_hdc302x_data *data = dev->data;
	int rc = 0;
	uint8_t buf[3];

	rc = write_command(dev, REG_OFFSET, sizeof(REG_OFFSET));
	if (rc < 0) {
		LOG_ERR("Failed to request offset readout");
	}
	rc = read_sensor_data(dev, (uint8_t *)buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Failed to read offset data");
		return rc;
	}
	if (!verify_crc(&buf[0], 2, buf[2])) {
		LOG_ERR("Offset CRC verification failed");
		return -EIO;
	}
	data->rh_offset = buf[0];
	data->t_offset = buf[1];

	switch (chan) {
	case SENSOR_CHAN_HUMIDITY:
		return convert_offset_to_humidity(data->rh_offset, val);

	case SENSOR_CHAN_AMBIENT_TEMP:
		return convert_offset_to_temperature(data->t_offset, val);

	default:
		break;
	}

	return 0;
}

static int convert_offset_to_sensor(uint8_t *offset, int16_t raw_offset, double scale)
{
	int16_t offset_bits;
	bool add = true;

	if (raw_offset < 0) {
		add = false;              /* Negative offset */
		raw_offset = -raw_offset; /* Make value positive for conversion */
	}

	offset_bits = raw_offset / scale; /* m°C to bits */
	if (offset_bits < 0 || offset_bits > 127) {
		LOG_ERR("offset out of range!");
		return -EINVAL;
	}
	*offset = (offset_bits & 0x7F) | (add ? 0x80 : 0);
	return 0;
}

static bool convert_temperature_to_offset(uint8_t *offset, const struct sensor_value *val)
{
	int16_t temp_offset_mdeg = val->val1 * 1000 + val->val2 / 1000;

	if (convert_offset_to_sensor(offset, temp_offset_mdeg, HDC_302X_TEMP_OFFSET_SCALE) != 0) {
		return false;
	}
	LOG_DBG("Converted temperature offset: %d.%06d to raw value: %x", val->val1, val->val2,
		*offset);
	return true;
}

static bool convert_humidity_to_offset(uint8_t *offset, const struct sensor_value *val)
{
	int16_t rh_offset_crh = val->val1 * 100 + val->val2 / 10000;

	if (convert_offset_to_sensor(offset, rh_offset_crh, HDC_302X_HUMIDITY_OFFSET_SCALE) != 0) {
		return false;
	}
	LOG_DBG("Converted humidity offset: %d.%06d to raw value: %x", val->val1, val->val2,
		*offset);
	return true;
}

static int set_offset(const struct device *dev, enum sensor_channel chan,
		      const struct sensor_value *val)
{
	struct ti_hdc302x_data *data = dev->data;
	int rc = 0;
	struct sensor_value tmp_val = {0};
	uint8_t buf[5];

	if (data->interval != HDC302X_SENSOR_MEAS_INTERVAL_MANUAL) {
		LOG_ERR("Cannot set offset in automatic mode");
		return -EINVAL;
	}
	/* Get current offset values so we do not overwrite the one that is not set here. */
	get_offset(dev, chan, &tmp_val);

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (convert_temperature_to_offset(&data->t_offset, val) == false) {
			LOG_ERR("Invalid temperature offset value: %d.%06d", val->val1, val->val2);
			return -EINVAL;
		}
		break;
	case SENSOR_CHAN_HUMIDITY:
		if (convert_humidity_to_offset(&data->rh_offset, val) == false) {
			LOG_ERR("Invalid humidity offset value: %d.%06d", val->val1, val->val2);
			return -EINVAL;
		}
		break;
	default:
		LOG_ERR("Unsupported channel for offset setting: %d", chan);
		return -ENOTSUP;
	}

	/* Prepare command to write offset */
	memcpy(buf, REG_OFFSET, sizeof(REG_OFFSET));
	buf[2] = data->rh_offset;
	buf[3] = data->t_offset;
	buf[4] = calculate_crc(&buf[2], 2); /* Calculate CRC for the data */

	/* Write the offset command */
	rc = write_command(dev, buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Failed to set offset: %d", rc);
		return rc;
	}
	k_msleep(HDC_302X_EEPROM_WRITE_TIME_OUT);
	return 0;
}

static int set_heater_level(const struct device *dev, enum sensor_channel chan,
			    const struct sensor_value *val)
{
	uint8_t buf[5];
	uint16_t heater_level = 0x3FFF; /* Default to maximum heater level */
	int rc;

	if (val->val1 > 14 || val->val1 < 0) {
		LOG_ERR("Heater level out of range: %d", val->val1);
		return -EINVAL;
	}

	if (val->val1 == 0) {
		/* If heater level is set to 0, we need to clear the heater status */
		rc = write_command(dev, REG_HEATER_OFF, 2);
		if (rc < 0) {
			LOG_ERR("Failed to disable heater: %d", rc);
			return rc;
		}
		LOG_DBG("Heater disabled");
	} else {
		/* shift the heater level bits to match the expected level */
		heater_level = heater_level >> (14 - val->val1);

		/* Prepare command to write heater level */
		memcpy(buf, REG_HEATER_LEVEL, sizeof(REG_HEATER_LEVEL));
		buf[2] = (heater_level >> 8) & 0xFF;
		buf[3] = heater_level & 0xFF;
		buf[4] = calculate_crc(&buf[2], 2); /* Calculate CRC */

		rc = write_command(dev, buf, sizeof(buf));
		if (rc < 0) {
			LOG_ERR("Failed to set heater level: %d", rc);
			return rc;
		}

		/* If heater level is set to non-zero, we need to enable the heater */
		rc = write_command(dev, REG_HEATER_ON, 2);
		if (rc < 0) {
			LOG_ERR("Failed to enable heater: %d", rc);
			return rc;
		}
		LOG_DBG("Heater enabled at level %d", val->val1);
	}

	return 0;
}

static int ti_hdc302x_attr_get(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, struct sensor_value *val)
{
	int rc = 0;
	uint16_t status;

	if (attr == (enum sensor_attribute)SENSOR_ATTR_STATUS_REGISTER) {
		rc = read_status_register(dev, &status);
		if (rc < 0) {
			return rc;
		}
		log_status_bits(status);
		val->val1 = status;
		val->val2 = 0;
		return 0;
	} else if (attr == SENSOR_ATTR_OFFSET) {
		get_offset(dev, chan, val);
		return 0;
	}
	return -ENOTSUP;
}

static int ti_hdc302x_attr_set(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, const struct sensor_value *val)
{
	struct ti_hdc302x_data *data = dev->data;
	int rc;

	if (attr >= SENSOR_ATTR_PRIV_START) {
		switch ((enum sensor_attribute_hdc302x)attr) {
		case SENSOR_ATTR_POWER_MODE:
			if (val->val1 < 0 || val->val1 >= HDC302X_SENSOR_POWER_MODE_MAX) {
				LOG_ERR("Invalid power mode: %d", val->val1);
				return -EINVAL;
			}
			data->power_mode = (enum sensor_power_mode_hdc302x)val->val1;
			return set_power_mode_and_interval(dev);

		case SENSOR_ATTR_INTEGRATION_TIME:
			if (val->val1 < 0 || val->val1 >= HDC302X_SENSOR_MEAS_INTERVAL_MAX) {
				LOG_ERR("Invalid integration time: %d", val->val1);
				return -EINVAL;
			}
			data->interval = (enum sensor_measurement_interval_hdc302x)val->val1;
			return set_power_mode_and_interval(dev);
		case SENSOR_ATTR_HEATER_LEVEL:
			set_heater_level(dev, chan, val);
			break;
		default:
			LOG_ERR("Unsupported SET attribute: %d", attr);
			return -ENOTSUP;
		}
	} else {
		switch (attr) {
		case SENSOR_ATTR_ALERT:
			rc = write_command(dev, REG_RESET_STATUS, 2);
			if (rc < 0) {
				LOG_ERR("Failed to clear alert status: %d", rc);
			}
			return rc;
		case SENSOR_ATTR_UPPER_THRESH:
			set_threshold(dev, chan, val, true);
			break;
		case SENSOR_ATTR_LOWER_THRESH:
			set_threshold(dev, chan, val, false);
			break;
		case SENSOR_ATTR_OFFSET:
			set_offset(dev, chan, val);
			break;
		default:
			LOG_ERR("Unsupported attribute: %d", attr);
			return -ENOTSUP;
		}
	}
	return 0;
}

static int ti_hdc302x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				  sensor_trigger_handler_t handler)
{
	struct ti_hdc302x_data *data = dev->data;

	if (trig->type == SENSOR_TRIG_DELTA) {
		data->th_handler = handler;
		data->th_trigger = trig;
	}
	return 0;
}

static DEVICE_API(sensor, ti_hdc302x_api_funcs) = {
	.sample_fetch = ti_hdc302x_sample_fetch,
	.channel_get = ti_hdc302x_channel_get,
	.attr_set = ti_hdc302x_attr_set,
	.attr_get = ti_hdc302x_attr_get,
	.trigger_set = ti_hdc302x_trigger_set,
};

static int ti_hdc302x_reset(const struct device *dev)
{
	int rc;

	rc = write_command(dev, REG_SOFT_RESET, 2);
	if (rc < 0) {
		LOG_ERR("Failed to soft-reset device: %d", rc);
		return rc;
	}
	k_sleep(HDC_302X_RESET_TIME);
	return 0;
}

static int ti_hdc302x_init(const struct device *dev)
{
	const struct ti_hdc302x_config *config = dev->config;
	struct ti_hdc302x_data *data = dev->data;
	uint8_t manufacturer_id_buf[3];
	int rc;

	/* Initialize default settings */
	data->power_mode = HDC302X_SENSOR_POWER_MODE_0;
	data->interval = HDC302X_SENSOR_MEAS_INTERVAL_MANUAL;
	data->t_offset = 0;
	data->rh_offset = 0;
	memcpy(data->selected_mode, mode_commands[data->power_mode][data->interval],
	       sizeof(data->selected_mode));

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	/* Read and verify manufacturer ID */
	rc = i2c_write_read_dt(&config->bus, REG_MANUFACTURER_ID, 2, manufacturer_id_buf,
			       sizeof(manufacturer_id_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read manufacturer ID: %d", rc);
		return rc;
	}

	if (verify_crc(manufacturer_id_buf, 2, manufacturer_id_buf[2]) != true &&
	    sys_get_be16(manufacturer_id_buf) != HDC_302X_MANUFACTURER_ID) {
		LOG_ERR("Invalid manufacturer ID: 0x%04X (expected 0x%04X)",
			sys_get_be16(manufacturer_id_buf), HDC_302X_MANUFACTURER_ID);
		return -EINVAL;
	}

	/* Soft-reset the device */
	rc = ti_hdc302x_reset(dev);
	if (rc < 0) {
		return rc;
	}

	/* Configure interrupt GPIO if available */
	if (config->int_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->int_gpio)) {
			LOG_ERR("GPIO interrupt device not ready");
			return -ENODEV;
		}

		rc = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
		if (rc < 0) {
			LOG_ERR("Failed to configure interrupt pin: %d", rc);
			return rc;
		}

		rc = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (rc < 0) {
			LOG_ERR("Failed to configure interrupt: %d", rc);
			/* Continue without interrupt - it's optional */
		}

		gpio_init_callback(&data->cb_int, interrupt_callback, BIT(config->int_gpio.pin));

		rc = gpio_add_callback(config->int_gpio.port, &data->cb_int);
		if (rc < 0) {
			LOG_ERR("Failed to add interrupt callback: %d", rc);
			return rc;
		}
	}

	LOG_DBG("HDC302x sensor initialized successfully");
	return 0;
}

/* Device instantiation macro */
#define TI_HDC302X_DEFINE(inst)                                                                    \
	static struct ti_hdc302x_data ti_hdc302x_data_##inst;                                      \
	static const struct ti_hdc302x_config ti_hdc302x_config_##inst = {                         \
		.bus = I2C_DT_SPEC_GET(DT_INST(inst, DT_DRV_COMPAT)),                              \
		.int_gpio = GPIO_DT_SPEC_GET_OR(DT_INST(inst, DT_DRV_COMPAT), int_gpios, {0}),     \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ti_hdc302x_init, NULL, &ti_hdc302x_data_##inst,         \
				     &ti_hdc302x_config_##inst, POST_KERNEL,                       \
				     CONFIG_SENSOR_INIT_PRIORITY, &ti_hdc302x_api_funcs);

#define TI_HDC302X_FOREACH_STATUS_OKAY(fn)                                                         \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT),                                      \
				(UTIL_CAT(DT_FOREACH_OKAY_INST_, DT_DRV_COMPAT)(fn)),              \
				())

/* HDC302X sensor instance */
TI_HDC302X_FOREACH_STATUS_OKAY(TI_HDC302X_DEFINE)
