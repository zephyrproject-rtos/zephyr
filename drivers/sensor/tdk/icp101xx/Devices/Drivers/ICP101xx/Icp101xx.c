/*
 * Copyright (c) 2017 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Icp101xx.h"
#include "Message.h"
#include "math.h"

#define ICP101XX_ODR_MIN_DELAY_LOW_NOISE 25000 /* us => 40 Hz */
#define ICP101XX_CRC8_INIT               0xFF
#define ICP101XX_RESP_DWORD_LEN          2
#define ICP101XX_RESP_CRC_LEN            1
#define ICP101XX_RESP_FRAME_LEN          (ICP101XX_RESP_DWORD_LEN + ICP101XX_RESP_CRC_LEN)
#define ICP101XX_CRC8_POLYNOM            0x31

static int send_measurement_command(struct inv_icp101xx *s);
static unsigned char compute_crc(uint8_t *frame);
static unsigned char check_crc(uint8_t *frame);
static void init_base(struct inv_icp101xx *s, short *otp);
static void calculate_conversion_constants(struct inv_icp101xx *s, float *p_Pa, float *p_LUT,
					   float *out);
static int read_id_from_i2c(struct inv_icp101xx *s, uint8_t *whoami);
static int read_otp_from_i2c(struct inv_icp101xx *s, short *out);
static int read_raw_pressure_temp_from_i2c(struct inv_icp101xx *s, int *pressure, int *temp);
static int process_data(struct inv_icp101xx *s, int p_LSB, int T_LSB, float *pressure,
			float *temperature);

/**
 * @brief      Compute CRC
 * @param      frame  The 2 data bytes of measure, plus 1 byte CRC
 * @return     8-bits CRC computed
 */
static unsigned char compute_crc(uint8_t *frame)
{

	uint8_t crc = ICP101XX_CRC8_INIT;
	uint8_t current_byte;
	uint8_t bit;

	/* Calculates 8-bit checksum with given polynomial. */
	for (current_byte = 0; current_byte < ICP101XX_RESP_DWORD_LEN; ++current_byte) {

		crc ^= (frame[current_byte]);
		for (bit = 8; bit > 0; --bit) {

			if (crc & 0x80) {
				crc = (crc << 1) ^ ICP101XX_CRC8_POLYNOM;
			} else {
				crc = (crc << 1);
			}
		}
	}

	return crc;
}

/**
 * @brief      Check measures CRC
 * @param      frame  The 2 bytes data bytes of measure, plus 1 byte CRC
 * @return     8-bits CRC computed
 */
static unsigned char check_crc(uint8_t *frame)
{
	uint8_t crc = compute_crc(frame);

	if (crc != frame[ICP101XX_RESP_FRAME_LEN - 1]) {
		INV_MSG(INV_ERROR, "CRC computed 0x%x doesn't match 0x%x\n", crc,
			frame[ICP101XX_RESP_FRAME_LEN - 1]);
	}
	return (crc == frame[ICP101XX_RESP_FRAME_LEN - 1]);
}

static void init_base(struct inv_icp101xx *s, short *otp)
{

	int i;

	for (i = 0; i < 4; i++) {
		s->sensor_constants[i] = (float)otp[i];
	}

	s->p_Pa_calib[0] = 45000.0;
	s->p_Pa_calib[1] = 80000.0;
	s->p_Pa_calib[2] = 105000.0;
	s->LUT_lower = 3.5 * (1 << 20);
	s->LUT_upper = 11.5 * (1 << 20);
	s->quadr_factor = 1 / 16777216.0;
	s->offst_factor = 2048.0;
}

/* p_Pa -- List of 3 values corresponding to applied pressure in Pa */
/* p_LUT -- List of 3 values corresponding to the measured p_LUT values at the applied pressures. */
static void calculate_conversion_constants(struct inv_icp101xx *s, float *p_Pa, float *p_LUT,
					   float *out)
{

	float A, B, C;

	C = (p_LUT[0] * p_LUT[1] * (p_Pa[0] - p_Pa[1]) + p_LUT[1] * p_LUT[2] * (p_Pa[1] - p_Pa[2]) +
	     p_LUT[2] * p_LUT[0] * (p_Pa[2] - p_Pa[0])) /
	    (p_LUT[2] * (p_Pa[0] - p_Pa[1]) + p_LUT[0] * (p_Pa[1] - p_Pa[2]) +
	     p_LUT[1] * (p_Pa[2] - p_Pa[0]));
	A = (p_Pa[0] * p_LUT[0] - p_Pa[1] * p_LUT[1] - (p_Pa[1] - p_Pa[0]) * C) /
	    (p_LUT[0] - p_LUT[1]);
	B = (p_Pa[0] - A) * (p_LUT[0] + C);

	out[0] = A;
	out[1] = B;
	out[2] = C;
}

/** @brief Compute pressure and temperature values based on raw data previously read in registers
 *  @param[in] p_LSB Raw pressure data from sensor
 *  @param[in] T_LSB Raw temperature data from sensor
 *  @param[out] pressure pressure data in Pascal
 *  @param[out] temperature temperature data in Degree Celsius
 */
static int process_data(struct inv_icp101xx *s, int p_LSB, int T_LSB, float *pressure,
			float *temperature)
{

	float t;
	float s1, s2, s3;
	float in[3];
	float out[3];
	float A, B, C;

	t = (float)(T_LSB - 32768);
	s1 = s->LUT_lower + (float)(s->sensor_constants[0] * t * t) * s->quadr_factor;
	s2 = s->offst_factor * s->sensor_constants[3] +
	     (float)(s->sensor_constants[1] * t * t) * s->quadr_factor;
	s3 = s->LUT_upper + (float)(s->sensor_constants[2] * t * t) * s->quadr_factor;
	in[0] = s1;
	in[1] = s2;
	in[2] = s3;

	calculate_conversion_constants(s, s->p_Pa_calib, in, out);
	A = out[0];
	B = out[1];
	C = out[2];

	*pressure = A + B / (C + p_LSB);
	*temperature = -45.f + 175.f / 65536.f * T_LSB;

	return 0;
}

static int read_id_from_i2c(struct inv_icp101xx *s, uint8_t *whoami)
{

	unsigned char data_write[10];
	unsigned char data_read[10] = {0};
	int status;
	int out;

	/* Read ID of pressure sensor */
	data_write[0] = (ICP101XX_CMD_READ_ID & 0xFF00) >> 8;
	data_write[1] = ICP101XX_CMD_READ_ID & 0x00FF;
	status = inv_icp101xx_serif_write_reg(&s->serif, ICP101XX_I2C_ADDR, data_write, 2);
	if (status) {
		return status;
	}

	status = inv_icp101xx_serif_read_reg(&s->serif, ICP101XX_I2C_ADDR, data_read, 3);
	if (status) {
		return status;
	}

	out = data_read[0] << 8 | data_read[1];
	out &= ICP101XX_PRODUCT_SPECIFIC_BITMASK; /* take bit5 to 0 */
	/* Check CRC */
	if (!check_crc(&data_read[0])) {
		return INV_ERROR;
	}

	*whoami = (uint8_t)out;

	return 0;
}

static int read_otp_from_i2c(struct inv_icp101xx *s, short *out)
{

	unsigned char data_write[10];
	unsigned char data_read[10] = {0};
	int status;
	int i;

	/* OTP Read mode */
	data_write[0] = (ICP101XX_CMD_SET_CAL_PTR & 0xFF00) >> 8;
	data_write[1] = (ICP101XX_CMD_SET_CAL_PTR & 0x00FF);
	data_write[2] = (ICP101XX_OTP_READ_ADDR & 0xFF00) >> 8;
	data_write[3] = ICP101XX_OTP_READ_ADDR & 0x00FF;
	data_write[4] = compute_crc(&data_write[2]);
	status = inv_icp101xx_serif_write_reg(&s->serif, ICP101XX_I2C_ADDR, data_write, 5);
	if (status) {
		return status;
	}

	/* Read OTP values */
	for (i = 0; i < 4; i++) {
		data_write[0] = (ICP101XX_CMD_INC_CAL_PTR & 0xFF00) >> 8;
		data_write[1] = ICP101XX_CMD_INC_CAL_PTR & 0x00FF;
		status = inv_icp101xx_serif_write_reg(&s->serif, ICP101XX_I2C_ADDR, data_write, 2);
		if (status) {
			return status;
		}

		status = inv_icp101xx_serif_read_reg(&s->serif, ICP101XX_I2C_ADDR, data_read, 3);
		if (status) {
			return status;
		}
		/* Check CRC */
		if (!check_crc(&data_read[0])) {
			return status;
		}

		out[i] = data_read[0] << 8 | data_read[1];
	}

	return 0;
}

static int send_measurement_command(struct inv_icp101xx *s)
{

	int status = 0;
	unsigned char data_write[10];

	/* Send Measurement Command */
	switch (s->measurement_mode) {
	case ICP101XX_MEAS_LOW_POWER_P_FIRST:
		data_write[0] = (ICP101XX_CMD_MEAS_LOW_POWER_P_FIRST & 0xFF00) >> 8;
		data_write[1] = ICP101XX_CMD_MEAS_LOW_POWER_P_FIRST & 0x00FF;
		break;
	case ICP101XX_MEAS_LOW_POWER_T_FIRST:
		data_write[0] = (ICP101XX_CMD_MEAS_LOW_POWER_T_FIRST & 0xFF00) >> 8;
		data_write[1] = ICP101XX_CMD_MEAS_LOW_POWER_T_FIRST & 0x00FF;
		break;
	case ICP101XX_MEAS_NORMAL_P_FIRST:
		data_write[0] = (ICP101XX_CMD_MEAS_NORMAL_P_FIRST & 0xFF00) >> 8;
		data_write[1] = ICP101XX_CMD_MEAS_NORMAL_P_FIRST & 0x00FF;
		break;
	case ICP101XX_MEAS_NORMAL_T_FIRST:
		data_write[0] = (ICP101XX_CMD_MEAS_NORMAL_T_FIRST & 0xFF00) >> 8;
		data_write[1] = ICP101XX_CMD_MEAS_NORMAL_T_FIRST & 0x00FF;
		break;
	case ICP101XX_MEAS_LOW_NOISE_P_FIRST:
		data_write[0] = (ICP101XX_CMD_MEAS_LOW_NOISE_P_FIRST & 0xFF00) >> 8;
		data_write[1] = ICP101XX_CMD_MEAS_LOW_NOISE_P_FIRST & 0x00FF;
		break;
	case ICP101XX_MEAS_LOW_NOISE_T_FIRST:
		data_write[0] = (ICP101XX_CMD_MEAS_LOW_NOISE_T_FIRST & 0xFF00) >> 8;
		data_write[1] = ICP101XX_CMD_MEAS_LOW_NOISE_T_FIRST & 0x00FF;
		break;
	case ICP101XX_MEAS_ULTRA_LOW_NOISE_P_FIRST:
		data_write[0] = (ICP101XX_CMD_MEAS_ULTRA_LOW_NOISE_P_FIRST & 0xFF00) >> 8;
		data_write[1] = ICP101XX_CMD_MEAS_ULTRA_LOW_NOISE_P_FIRST & 0x00FF;
		break;
	case ICP101XX_MEAS_ULTRA_LOW_NOISE_T_FIRST:
		data_write[0] = (ICP101XX_CMD_MEAS_ULTRA_LOW_NOISE_T_FIRST & 0xFF00) >> 8;
		data_write[1] = ICP101XX_CMD_MEAS_ULTRA_LOW_NOISE_T_FIRST & 0x00FF;
		break;
	default:
		return INV_ERROR;
	}

	status = inv_icp101xx_serif_write_reg(&s->serif, ICP101XX_I2C_ADDR, data_write, 2);
	return status;
}

static int read_raw_pressure_temp_from_i2c(struct inv_icp101xx *s, int *pressure, int *temp)
{

	unsigned char data_read[10] = {0};
	int status;

	status = inv_icp101xx_serif_read_reg(&s->serif, ICP101XX_I2C_ADDR, data_read, 9);
	if (status) {
		return status;
	}

	/* Check CRC */
	if (!check_crc(&data_read[0]) || !check_crc(&data_read[3]) || !check_crc(&data_read[6])) {
		status = INV_ERROR;
	}

	switch (s->measurement_mode) {
	case ICP101XX_MEAS_LOW_POWER_P_FIRST:
	case ICP101XX_MEAS_NORMAL_P_FIRST:
	case ICP101XX_MEAS_LOW_NOISE_P_FIRST:
	case ICP101XX_MEAS_ULTRA_LOW_NOISE_P_FIRST:
		/* read P first */
		/* Temperature */
		*temp = data_read[6] << 8 | data_read[7];

		/* Pressure */
		*pressure =
			data_read[0] << (8 * 2) | data_read[1] << (8 * 1) | data_read[3] << (8 * 0);
		break;

	case ICP101XX_MEAS_LOW_POWER_T_FIRST:
	case ICP101XX_MEAS_NORMAL_T_FIRST:
	case ICP101XX_MEAS_LOW_NOISE_T_FIRST:
	case ICP101XX_MEAS_ULTRA_LOW_NOISE_T_FIRST:
		/* Read T first */
		/* Temperature */
		*temp = data_read[0] << 8 | data_read[1];

		/* Pressure */
		*pressure = (data_read[3] << (8 * 2) | data_read[4] << (8 * 1) |
			     data_read[6] << (8 * 0));
		break;
	default:
		status = INV_ERROR;
		break;
	}

	status = send_measurement_command(s);

	return status;
}

int inv_icp101xx_init(struct inv_icp101xx *s)
{

	uint8_t whoami = 0;
	short otp[4];

	inv_icp101xx_get_whoami(s, &whoami);

	if (whoami != ICP101XX_ID) {
		return INV_ERROR;
	}

	s->min_delay_us = ICP101XX_ODR_MIN_DELAY_LOW_NOISE;
	s->measurement_mode = ICP101XX_MEAS_LOW_NOISE_P_FIRST;

	read_otp_from_i2c(s, otp);

	init_base(s, otp);

	return 0;
}

int inv_icp101xx_get_data(struct inv_icp101xx *s, int *raw_pressure, int *raw_temperature,
			  float *pressure, float *temperature)
{

	int rc = 0;
	int rawP, rawT;
	float pressure_Pa, temperature_C;

	if ((s->pressure_en) || (s->temperature_en)) {

		rc = read_raw_pressure_temp_from_i2c(s, &rawP, &rawT);
		if (rc != 0) {
			return rc;
		}

		rc = process_data(s, rawP, rawT, &pressure_Pa, &temperature_C);

		if (s->pressure_en) {
			if (pressure) {
				*raw_pressure = rawP;
				*pressure = pressure_Pa;
			}
		}

		if (s->temperature_en) {
			if (temperature) {
				*raw_temperature = rawT;
				*temperature = temperature_C;
			}
		}
	}

	return rc;
}

int inv_icp101xx_enable_sensor(struct inv_icp101xx *s, inv_bool_t en)
{

	int rc = 0;

	if (en) {
		s->pressure_en = 1;
		s->temperature_en = 1;

		rc = send_measurement_command(s);
	} else {
		s->pressure_en = 0;
		s->temperature_en = 0;
	}

	return rc;
}

int inv_icp101xx_pressure_enable_sensor(struct inv_icp101xx *s, inv_bool_t en)
{

	int rc = 0;

	if (en) {
		s->pressure_en = 1;
	} else {
		s->pressure_en = 0;
	}

	if (!s->temperature_en) {
		rc = inv_icp101xx_enable_sensor(s, en);
	}

	return rc;
}

int inv_icp101xx_temperature_enable_sensor(struct inv_icp101xx *s, inv_bool_t en)
{

	int rc = 0;

	if (en) {
		s->temperature_en = 1;
	} else {
		s->temperature_en = 0;
	}

	if (!s->pressure_en) {
		rc = inv_icp101xx_enable_sensor(s, en);
	}

	return rc;
}

int inv_icp101xx_get_whoami(struct inv_icp101xx *s, uint8_t *whoami)
{

	return read_id_from_i2c(s, whoami);
}

int inv_icp101xx_soft_reset(struct inv_icp101xx *s)
{

	int status = 0;
	unsigned char data_write[10];

	/* Send Soft Reset Command */
	data_write[0] = (ICP101XX_CMD_SOFT_RESET & 0xFF00) >> 8;
	data_write[1] = ICP101XX_CMD_SOFT_RESET & 0x00FF;

	status = inv_icp101xx_serif_write_reg(&s->serif, ICP101XX_I2C_ADDR, data_write, 2);
	inv_icp101xx_sleep_us(170);

	return status;
}
