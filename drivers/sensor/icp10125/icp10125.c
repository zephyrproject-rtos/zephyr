/*
 * Copyright (c) 2022 Mizuki Agawa <agawa.mizuki@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icp10125

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#ifdef CONFIG_ICP10125_CHECK_CRC
#include <zephyr/sys/crc.h>
#endif /* CONFIG_ICP10125_CHECK_CRC */

LOG_MODULE_REGISTER(ICP10125, CONFIG_SENSOR_LOG_LEVEL);

#define CRC_POLY	 0x31
#define SENSOR_DATA_SIZE 2

#define AMBIENT_TEMP_DATA_NUM		1
#define PRESS_DATA_NUM			2
#define PRESS_AND_AMBIENT_TEMP_DATA_NUM (AMBIENT_TEMP_DATA_NUM + PRESS_DATA_NUM)

enum {
	LOW_POWER,
	NORMAL,
	LOW_NOISE,
	ULTRA_LOW_NOISE,
	NUM_MEASURE_MODE
};

struct icp10125_data {
	uint16_t raw_ambient_temp;
	uint32_t raw_press;
	float sensor_constants[4];
};

struct icp10125_dev_config {
	struct i2c_dt_spec i2c;
	uint8_t ambient_temp_mode;
	uint8_t press_mode;
};

struct icp10125_cmd {
	uint8_t data[2];
};

struct icp10125_sensor_data {
	uint8_t data[2];
	uint8_t crc;
};

struct icp10125_otp_read_setup {
	struct icp10125_cmd cmd;
	uint8_t data[3];
} __packed __aligned(1);

/* ambient temperature measurement command for each mode.
 * (Section 5.2 MEASUREMENT COMMANDS in the Datasheet)
 */
static const struct icp10125_cmd ambient_temp_measurement_cmds[] = {
	{{0x60, 0x9C}}, {{0x68, 0x25}}, {{0x70, 0xDF}}, {{0x78, 0x66}}
};

/* pressure measurement command for each mode.
 * (Section 5.2 MEASUREMENT COMMANDS in the Datasheet)
 */
static const struct icp10125_cmd press_measurement_cmds[] = {
	{{0x40, 0x1A}}, {{0x48, 0xA3}}, {{0x50, 0x59}}, {{0x59, 0xE0}}
};

/* Request preparation for OTP data read. It should issue before data read request.
 * (Section 5.2 MEASUREMENT COMMANDS in the Datasheet)
 */
static const struct icp10125_otp_read_setup otp_read_setup = {
		.cmd = {{0xC5, 0x95}},
		.data = {0x00, 0x66, 0x9C}
};

/* OTP data read request.
 * After issue this command 2byte x 4 sensor constant value can readable.
 */
static const struct icp10125_cmd otp_read_request_cmd = {{0xC7, 0xF7}};

/* The max conversion time for each modes.
 * (Section 2.2 OPERATION MODES in the Datasheet)
 */
static const uint32_t conv_time_max[] = {1800, 6300, 23800, 94500};

/* The typical conversion time for each modes.
 * (Section 2.2 OPERATION MODES in the Datasheet)
 */
static const uint32_t conv_time_typ[] = {1600, 5600, 20800, 83200};

/* The Datasheet has no mention of the constants and formulas.
 * Instead, it shows only how to use it in the sample code.
 * Since there is no detailed description in the ICP10125 product manual,
 * the calculation of the pressure implements is the same as shown in
 * the 5.11 SAMPLE CODE: EXAMPLE C SYNTAX
 */

static void icp10125_calculate_conversion_constants(const float *p_LUT, float *A, float *B,
						    float *C)
{
	const float p_Pa[] = {45000.0, 80000.0, 105000.0};

	*C = (p_LUT[0] * p_LUT[1] * (p_Pa[0] - p_Pa[1]) +
	      p_LUT[1] * p_LUT[2] * (p_Pa[1] - p_Pa[2]) +
	      p_LUT[2] * p_LUT[0] * (p_Pa[2] - p_Pa[0])) /
	     (p_LUT[2] * (p_Pa[0] - p_Pa[1]) + p_LUT[0] * (p_Pa[1] - p_Pa[2]) +
	      p_LUT[1] * (p_Pa[2] - p_Pa[0]));
	*A = (p_Pa[0] * p_LUT[0] - p_Pa[1] * p_LUT[1] - (p_Pa[1] - p_Pa[0]) * (*C)) /
	     (p_LUT[0] - p_LUT[1]);
	*B = (p_Pa[0] - (*A)) * (p_LUT[0] + (*C));
}

static float icp10125_calc_calibrated_ambient_temp(const struct icp10125_data *data)
{
	return -45.f + 175.f / 65536.f * data->raw_ambient_temp;
}

static float icp10125_calc_calibrated_press(const struct icp10125_data *data)
{
	const float quadr_factor = 1 / 16777216.0;
	const float offst_factor = 2048.0;
	const float LUT_lower = 3.5 * (1 << 20);
	const float LUT_upper = 11.5 * (1 << 20);
	float t;
	float in[3];
	float A, B, C;

	t = data->raw_ambient_temp - 32768.f;
	in[0] = LUT_lower + (data->sensor_constants[0] * t * t) * quadr_factor;
	in[1] = offst_factor * data->sensor_constants[3] +
		(data->sensor_constants[1] * t * t) * quadr_factor;
	in[2] = LUT_upper + (data->sensor_constants[2] * t * t) * quadr_factor;
	icp10125_calculate_conversion_constants(in, &A, &B, &C);

	return A + B / (C + data->raw_press);
}

/* End of porting the 5.11 SAMPLE CODE: EXAMPLE C SYNTAX */

static int icp10125_read_otp(const struct device *dev)
{
	struct icp10125_data *data = dev->data;
	struct icp10125_sensor_data sensor_data;

	const struct icp10125_dev_config *cfg = dev->config;
	int rc = 0;

	rc = i2c_write_dt(&cfg->i2c, (uint8_t *)&otp_read_setup, sizeof(otp_read_setup));
	if (rc < 0) {
		LOG_ERR("Failed to write otp_read_setup.\n");
		return rc;
	}

	for (size_t i = 0; i < ARRAY_SIZE(data->sensor_constants); i++) {
		rc = i2c_write_dt(&cfg->i2c, (uint8_t *)&otp_read_request_cmd,
				  sizeof(otp_read_request_cmd));
		if (rc < 0) {
			LOG_ERR("Failed to write otp_read_request.\n");
			return rc;
		}

		rc = i2c_read_dt(&cfg->i2c, (uint8_t *)&sensor_data, sizeof(sensor_data));
		if (rc < 0) {
			LOG_ERR("Failed to read otp_read_request.\n");
			return rc;
		}

		data->sensor_constants[i] = sys_get_be16(sensor_data.data);
	}

	return 0;
}

static int icp10125_check_crc(const uint8_t *data, const size_t len)
{
	/* Details of CRC are described in Chapter 5 Section 8 of the product
	 * specifications.
	 */
	return crc8(data, len, CRC_POLY, 0xFF, false);
}

static int icp10125_measure(const struct i2c_dt_spec *i2c, const struct icp10125_cmd *cmds,
			    const uint8_t mode, struct icp10125_sensor_data *sensor_data,
			    const size_t data_num)
{
	int rc = 0;

	rc = i2c_write_dt(i2c, (uint8_t *)&cmds[mode], sizeof(cmds[mode]));
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.\n");
		return rc;
	}

	/* Wait for the sensor to become readable.
	 * First wait for the typical time and then read.
	 * If that fails, wait until the time to surely became readable.
	 */
	k_sleep(K_USEC(conv_time_typ[mode]));
	if (i2c_read_dt(i2c, (uint8_t *)sensor_data, sizeof(sensor_data[0]) * data_num) < 0) {
		k_sleep(K_USEC(conv_time_max[mode] - conv_time_typ[mode]));
		rc = i2c_read_dt(i2c, (uint8_t *)sensor_data, sizeof(sensor_data[0]) * data_num);
		if (rc < 0) {
			LOG_ERR("Failed to read measurement.\n");
			return rc;
		}
	}

#ifdef CONFIG_ICP10125_CHECK_CRC
	/* Calculate CRC from Chapter 5 Section 8 of ICP10125 Product manuals. */
	for (size_t i = 0; i < data_num; i++) {
		if (!icp10125_check_crc(sensor_data[i].data, SENSOR_DATA_SIZE)) {
			LOG_ERR("Sensor data has invalid CRC.\n");
			return -EIO;
		}
	}
#endif /* CONFIG_ICP10125_CHECK_CRC */

	return 0;
}

static int icp10125_sample_fetch(const struct device *dev, const enum sensor_channel chan)
{
	struct icp10125_data *data = dev->data;
	const struct icp10125_dev_config *cfg = dev->config;
	uint8_t endian_conversion[3];
	struct icp10125_sensor_data sensor_data[PRESS_AND_AMBIENT_TEMP_DATA_NUM] = {0};
	int rc = 0;

	if (!(chan == SENSOR_CHAN_AMBIENT_TEMP || chan == SENSOR_CHAN_PRESS ||
	      chan == SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		rc = icp10125_measure(&cfg->i2c, ambient_temp_measurement_cmds,
				      cfg->ambient_temp_mode, sensor_data, AMBIENT_TEMP_DATA_NUM);
		if (rc < 0) {
			return rc;
		}

		data->raw_ambient_temp = sys_get_be16(sensor_data[0].data);
	} else {
		rc = icp10125_measure(&cfg->i2c, press_measurement_cmds, cfg->press_mode,
				      sensor_data, PRESS_AND_AMBIENT_TEMP_DATA_NUM);
		if (rc < 0) {
			return rc;
		}

		endian_conversion[0] = sensor_data[0].data[0];
		endian_conversion[1] = sensor_data[0].data[1];
		endian_conversion[2] = sensor_data[1].data[0];
		data->raw_press = sys_get_be24(endian_conversion);
		data->raw_ambient_temp = sys_get_be16(sensor_data[2].data);
	}

	return 0;
}

static void icp10125_convert_press_value(struct icp10125_data *data, struct sensor_value *val)
{
	sensor_value_from_double(val, icp10125_calc_calibrated_press(data) / 1000.f);
}

static void icp10125_convert_ambient_temp_value(struct icp10125_data *data,
						struct sensor_value *val)
{
	sensor_value_from_double(val, icp10125_calc_calibrated_ambient_temp(data));
}

static int icp10125_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icp10125_data *data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		icp10125_convert_ambient_temp_value(data, val);
	} else if (chan == SENSOR_CHAN_PRESS) {
		icp10125_convert_press_value(data, val);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int icp10125_init(const struct device *dev)
{
	int rc = icp10125_read_otp(dev);

	if (rc < 0) {
		return rc;
	}

	return 0;
}

static const struct sensor_driver_api icp10125_api_funcs = {
	.sample_fetch = icp10125_sample_fetch,
	.channel_get = icp10125_channel_get,
};

#define ICP10125_DEFINE(inst)                                                                      \
	static struct icp10125_data icp10125_drv_##inst;                                           \
	static const struct icp10125_dev_config icp10125_config_##inst = {                         \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.ambient_temp_mode = DT_INST_ENUM_IDX(inst, temperature_measurement_mode),         \
		.press_mode = DT_INST_ENUM_IDX(inst, pressure_measurement_mode)};                  \
	DEVICE_DT_INST_DEFINE(inst, icp10125_init, NULL, &icp10125_drv_##inst,                     \
			      &icp10125_config_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,   \
			      &icp10125_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(ICP10125_DEFINE)
