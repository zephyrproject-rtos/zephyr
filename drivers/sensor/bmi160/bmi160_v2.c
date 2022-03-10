/* Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bmi160.h"

#include <string.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/math/util.h>
#include <zephyr/sys_clock.h>

static int bmi160_read_data(const struct device *sensor, uint32_t *sensor_types,
			    size_t type_list_count)
{
	struct bmi160_data *data = sensor->data;
	uint16_t expected_rading_size = 0;
	int rc;
	uint16_t *sample;

	/* Check that we have a buffer */
	if (data->raw_data_buffer == NULL) {
		return -ENOSYS;
	}
	/* Check that the buffer is big enough */
	for (size_t i = 0; i < type_list_count; ++i) {
		switch (sensor_types[i]) {
		case SENSOR_TYPE_ACCELEROMETER:
		case SENSOR_TYPE_GYROSCOPE:
			expected_rading_size += 3;
			break;
		default:
			/* Unsupported type */
			break;
		}
	}
	if (data->raw_data_buffer->header.reading_size < 1 + expected_rading_size) {
		return -ENOSYS;
	}
	/* Check that we have a callback */
	if (data->process_data_callback == NULL) {
		return -ENOSYS;
	}
	data->raw_data_buffer->header.base_timestamp = k_uptime_get() * USEC_PER_MSEC;

	rc = bmi160_sample_fetch(sensor);
	if (rc != 0) {
		return rc;
	}

	switch (sensor_type) {
	case SENSOR_TYPE_ACCELEROMETER:
		sample = data->sample.acc;
		break;
	case SENSOR_TYPE_GYROSCOPE:
		sample = data->sample.gyr;
		break;
	default:
		return -ENOSYS;
	}

	out_data->header.reading_count = 1;
	out_data->readings[0].x = sample[0];
	out_data->readings[0].y = sample[1];
	out_data->readings[0].z = sample[2];

	return 0;
}

static int bmi160_set_range(const struct device *sensor, uint32_t sensor_type, fp_t range,
			    bool round_up)
{
	switch (sensor_type) {
	case SENSOR_TYPE_ACCELEROMETER:
		return bmi160_acc_range_set(sensor, FP_TO_INT(fp_div(range, SENSOR_G)));
	case SENSOR_TYPE_GYROSCOPE:
		return bmi160_gyr_range_set(sensor, FP_TO_INT(sensor_rad_to_degrees(range)));
	default:
		return -ENOSYS;
	}
}

static int bmi160_set_bias(const struct device *sensor, uint32_t sensor_type, int16_t temperature,
			   fp_t bias_x, fp_t bias_y, fp_t bias_z, bool round_up)
{
	const struct bmi160_cfg *cfg = sensor->config;
	int8_t bias[3];

	switch (sensor_type) {
	case SENSOR_TYPE_ACCELEROMETER:
		bias_x = fp_div(bias_x, FLOAT_TO_FP(3.9f));
		bias_y = fp_div(bias_y, FLOAT_TO_FP(3.9f));
		bias_z = fp_div(bias_z, FLOAT_TO_FP(3.9f));
		if (round_up) {
			if (bias_x > FLOAT_TO_FP(0.0f)) {
				bias_x += FLOAT_TO_FP(0.5f);
			} else {
				bias_x -= FLOAT_TO_FP(0.5f);
			}
			if (bias_y > FLOAT_TO_FP(0.0f)) {
				bias_y += FLOAT_TO_FP(0.5f);
				bias_y -= FLOAT_TO_FP(0.5f);
			}
			if (bias_z > FLOAT_TO_FP(0.0f)) {
				bias_z += FLOAT_TO_FP(0.5f);
			} else {
				bias_z -= FLOAT_TO_FP(0.5f);
			}
		}
		bias[0] = (uint8_t)CLAMP(FP_TO_INT(bias_x), -128, 127);
		bias[1] = (uint8_t)CLAMP(FP_TO_INT(bias_y), -128, 127);
		bias[2] = (uint8_t)CLAMP(FP_TO_INT(bias_z), -128, 127);
		if (cfg->bus_io->write(sensor, BMI160_REG_OFFSET_ACC_X, &bias[0], 1) != 0) {
			return -EIO;
		}
		if (cfg->bus_io->write(sensor, BMI160_REG_OFFSET_ACC_Y, &bias[1], 1) != 0) {
			return -EIO;
		}
		if (cfg->bus_io->write(sensor, BMI160_REG_OFFSET_ACC_Z, &bias[2], 1) != 0) {
			return -EIO;
		}
		return 0;
	}
	return -ENOSYS;
}

static int bmi160_get_bias(const struct device *sensor, uint32_t sensor_type, int16_t *temperature,
			   fp_t *bias_x, fp_t *bias_y, fp_t *bias_z)
{
	const struct bmi160_cfg *cfg = sensor->config;
	int8_t bias[3];

	switch (sensor_type) {
	case SENSOR_TYPE_ACCELEROMETER:
		if (cfg->bus_io->read(sensor, BMI160_REG_OFFSET_ACC_X, &bias[0], 1) != 0) {
			return -EIO;
		}
		if (cfg->bus_io->read(sensor, BMI160_REG_OFFSET_ACC_Y, &bias[1], 1) != 0) {
			return -EIO;
		}
		if (cfg->bus_io->read(sensor, BMI160_REG_OFFSET_ACC_Z, &bias[2], 1) != 0) {
			return -EIO;
		}
		*temperature = INT16_MIN;
		*bias_x = fp_mul(INT_TO_FP(bias[0]), FLOAT_TO_FP(3.9f));
		*bias_y = fp_mul(INT_TO_FP(bias[1]), FLOAT_TO_FP(3.9f));
		*bias_z = fp_mul(INT_TO_FP(bias[2]), FLOAT_TO_FP(3.9f));
		return 0;
	}
	return -ENOSYS;
}

static int bmi160_get_range(const struct device *sensor, uint32_t sensor_type, fp_t *range)
{
	int rc;
	uint8_t range_value;

	switch (sensor_type) {
	case SENSOR_TYPE_ACCELEROMETER:
		rc = bmi160_byte_read(sensor, BMI160_REG_ACC_RANGE, &range_value);
		if (rc != 0) {
			return rc;
		}
		*range = INT_TO_FP(bmi160_acc_reg_val_to_range(range_value));
		return 0;
	case SENSOR_TYPE_GYROSCOPE:
		rc = bmi160_byte_read(sensor, BMI160_REG_GYR_RANGE, &range_value);
		if (rc != 0) {
			return rc;
		}
		*range = INT_TO_FP(bmi160_gyr_reg_val_to_range(range_value));
		return 0;
	default:
		return -ENOSYS;
	}
}

static int bmi160_get_scale(const struct device *sensor, uint32_t sensor_type,
			    struct sensor_scale_metadata *scale)
{
	int rc;

	scale->resolution = 16;
	rc = bmi160_get_range(sensor, sensor_type, &scale->range);
	if (rc != 0) {
		return rc;
	}

	switch (sensor_type) {
	case SENSOR_TYPE_ACCELEROMETER:
		scale->range_units = SENSOR_RANGE_UNITS_ACCEL_G;
		break;
	case SENSOR_TYPE_GYROSCOPE:
		scale->range_units = SENSOR_RANGE_UNITS_ANGLE_DEGREES;
		break;
	default:
		return -ENOSYS;
	}

	return 0;
}

static int bmi160_set_data_buffer(const struct device *sensor, struct sensor_raw_data *buffer)
{
	struct bmi160_data *data = sensor->data;

	data->raw_data_buffer = buffer;
	return 0;
}

static int bmi160_set_data_callback(const struct device *sensor,
				    sensor_process_data_callback_t callback)
{
	struct bmi160_data *data = sensor->data;

	data->process_data_callback = callback;
	return 0;
}

#ifdef CONFIG_SENSOR_STREAMING_MODE

static int bmi160_get_sample_rate_available(const struct device *sensor,
					    const struct sensor_sample_rate_info **sample_rates,
					    uint8_t *count)
{
	static const struct sensor_sample_rate_info bmi160_sample_rates[] = {
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(781)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(1563)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(3125)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(6250)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(12500)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(25000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(50000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(100000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(200000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(400000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(800000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_ACCELEROMETER, UINT32_C(1600000)),

		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(781)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(1563)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(3125)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(6250)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(12500)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(25000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(50000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(100000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(200000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(400000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(800000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(1600000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GYROSCOPE, UINT32_C(3200000)),

		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(781)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(1563)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(3125)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(6250)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(12500)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(25000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(50000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(100000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(200000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(400000)),
		BUILD_SAMPLE_RATE_INFO(SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(800000)),
	};

	*sample_rates = bmi160_sample_rates;
	*count = ARRAY_SIZE(bmi160_sample_rates);
	return 0;
}

static int bmi160_set_watermark(const struct device *sensor, uint8_t watermark_percent,
				bool round_up)
{
	const struct bmi160_cfg *cfg = sensor->config;
	uint8_t watermark_reg_value = 0;
	uint8_t int_status_1;

	watermark_percent = CLAMP(watermark_percent, 0, 100);

	if (watermark_percent != 0) {
		/* BMI160 watermark is 4 bytes per 1 value in the register.
		 * First convert the watermark_percent to an actual value relative to the FIFO size
		 * (1024):
		 *     1024 * watermark_percent / 100
		 * Then divide by 4 to get:
		 *     (1024 * watermark_percent) / 400
		 */
		watermark_reg_value = (1024 * watermark_percent) / 400;
		if (round_up && ((1024 * watermark_percent) / 400) != 0) {
			watermark_reg_value++;
		}
		if (cfg->bus_io->write(sensor, BMI160_REG_FIFO_CONFIG0, &watermark_reg_value, 1) !=
		    0) {
			return -EIO;
		}
	} else {
		if (cfg->bus_io->write(sensor, BMI160_REG_FIFO_CONFIG0, &watermark_reg_value, 1) !=
		    0) {
			return -EIO;
		}
	}

	/* Read the current watermark config */
	if (cfg->bus_io->read(sensor, BMI160_REG_INT_STATUS1, &int_status_1, 1) != 0) {
		return -EIO;
	}
	/* Update the status register */
	if (watermark_percent == 0) {
		if ((int_status_1 & (BMI160_INT_STATUS1_FFULL | BMI160_INT_STATUS1_FWM)) == 0) {
			/* Interrupts are already disabled, do nothing */
			return 0;
		}
		int_status_1 &= ~(BMI160_INT_STATUS1_FFULL | BMI160_INT_STATUS1_FWM);
	} else {
		if ((int_status_1 & (BMI160_INT_STATUS1_FFULL | BMI160_INT_STATUS1_FWM)) ==
		    (BMI160_INT_STATUS1_FFULL | BMI160_INT_STATUS1_FWM)) {
			/* Interrupts are already enabled, do thing */
			return 0;
		}
		int_status_1 |= (BMI160_INT_STATUS1_FFULL | BMI160_INT_STATUS1_FWM);
	}
	if (cfg->bus_io->write(sensor, BMI160_REG_INT_STATUS1, &int_status_1, 1) != 0) {
		return -EIO;
	}
	return 0;
}

static int bmi160_get_watermark(const struct device *sensor, uint8_t *watermark_percent)
{
	return -ENOSYS;
}
#endif

static const struct sensor_driver_api_v2 bmi160_api = {
	.set_data_buffer = bmi160_set_data_buffer,
	.set_data_callback = bmi160_set_data_callback,
	.read_data = bmi160_read_data,
	.get_scale = bmi160_get_scale,
	.set_range = bmi160_set_range,
	.set_bias = bmi160_set_bias,
	.get_bias = bmi160_get_bias,
#ifdef CONFIG_SENSOR_STREAMING_MODE
	.get_sample_rate_available = bmi160_get_sample_rate_available,
	.set_watermark = bmi160_set_watermark,
	.get_watermark = bmi160_get_watermark,
#endif
};

#if defined(CONFIG_BMI160_TRIGGER)
#define BMI160_TRIGGER_CFG(inst) .interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios),
#else
#define BMI160_TRIGGER_CFG(inst)
#endif

static int bmi160_v2_init(const struct device *dev)
{
	struct bmi160_data *data = dev->data;
	const struct bmi160_cfg *cfg = dev->config;
	uint8_t config_data[5] int rc;

	rc = cfg->bus_io->read(sensor, BMI160_REG_ACC_CONF, config_data, ARRAY_SIZE(config_data));
	if (rc != 0) {
		return -EIO;
	}
	data->current_accel_range = bmi160_acc_reg_val_to_range(config_data[1]);
	data->current_gyro_range = bmi160_gyr_reg_val_to_range(config_data[3]);

	return bmi160_init(dev);
}

#define BMI160_DEVICE_INIT(inst)                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmi160_v2_init, NULL, &bmi160_data_##inst,              \
				     &bmi160_cfg_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
				     &bmi160_api);

/* Instantiation macros used when a device is on a SPI bus */
#define BMI160_DEFINE_SPI(inst)                                                                    \
	static struct bmi160_data bmi160_data_##inst;                                              \
	static const struct bmi160_cfg bmi160_cfg_##inst = {                                       \
		.bus.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0),                         \
		.bus_io = &bmi160_bus_spi_io,                                                      \
		BMI160_TRIGGER_CFG(inst)};                                                         \
	BMI160_DEVICE_INIT(inst)

/* Instantiation macros used when a device is on an I2C bus */
#define BMI160_CONFIG_I2C(inst)                                                                    \
	{                                                                                          \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_io = &bmi160_bus_i2c_io,               \
	}

#define BMI160_DEFINE_I2C(inst)                                                                    \
	static struct bmi160_data bmi160_data_##inst;                                              \
	static const struct bmi160_cfg bmi160_cfg_##inst = BMI160_CONFIG_I2C(inst);                \
	BMI160_DEVICE_INIT(inst)

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */
#define BMI160_DEFINE(inst)                                                                        \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), (BMI160_DEFINE_SPI(inst)), (BMI160_DEFINE_I2C(inst)))

DT_INST_FOREACH_STATUS_OKAY(BMI160_DEFINE)
