/*
 * Copyright (c) 2025 Orgatex GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "bq35100.h"
#include <zephyr/drivers/fuel_gauge/bq35100_user.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_DECLARE(bq35100);

#define COMMAND_RETRY_TIMEOUT_MS      200
/* this sleep should not be lower than 0.5 seconds */
#define CHECK_STATUS_RETRY_TIMEOUT_MS 1000
#define CC_DELTA_MULT_FACTOR          1193046.0f

static int set_calibration_mode(const struct device *dev, uint16_t cntl_address)
{
	int err;

	if (cntl_address != BQ35100_MAC_CMD_ENTER_CAL && cntl_address != BQ35100_MAC_CMD_EXIT_CAL) {
		return -EINVAL;
	}

	while (1) {
		err = bq35100_send_cntl(dev, cntl_address);
		if (err < 0) {
			LOG_ERR("Error sending calibration mode: %d", err);
			return err;
		}

		err = bq35100_send_cntl(dev, BQ35100_MAC_CMD_CONTROL_STATUS);
		if (err < 0) {
			LOG_ERR("Error sending mac command control status %d", err);
			return err;
		}

		uint16_t answer;

		err = bq35100_get_status(dev, &answer);
		if (err < 0) {
			LOG_ERR("Failed to get status after sending calibration mode");
		}

		if (cntl_address == BQ35100_MAC_CMD_ENTER_CAL && (answer & (1 << 12))) {
			LOG_DBG("calibration mode entered");
			break;
		}

		if (cntl_address == BQ35100_MAC_CMD_EXIT_CAL && (!(answer & (1 << 12)))) {
			LOG_DBG("calibration mode exited");
			break;
		}

		k_msleep(COMMAND_RETRY_TIMEOUT_MS);
	}

	return 0;
}

static int bq35100_obtain_raw_calibration_data(const struct device *dev, uint8_t reg_addr,
					       uint16_t *avg_raw_data)
{
	int err;

	int samples_to_avg = CONFIG_BQ35100_RAW_DATA_SAMPLING;
	int loop_count = 0;
	uint8_t counter_now;
	uint8_t counter_prev;
	uint8_t cal_raw_data[2];
	uint32_t raw_data_sum = 0;

	err = set_calibration_mode(dev, BQ35100_MAC_CMD_ENTER_CAL);
	if (err < 0) {
		LOG_ERR("Failed to enter calibration mode");
		return err;
	}

	err = bq35100_get_data(dev, BQ35100_REG_CAL_COUNT, &counter_now, sizeof(counter_now));
	if (err < 0) {
		LOG_ERR("Failed to get counter");
		return -EIO;
	}

	counter_prev = counter_now;

	while (true) {
		k_msleep(COMMAND_RETRY_TIMEOUT_MS);

		err = bq35100_get_data(dev, BQ35100_REG_CAL_COUNT, &counter_now,
				       sizeof(counter_now));
		if (err < 0) {
			LOG_ERR("Failed to get counter");
			return -EIO;
		}

		if (counter_now == counter_prev) {
			continue;
		}

		err = bq35100_get_data(dev, reg_addr, cal_raw_data, sizeof(cal_raw_data));
		if (err < 0) {
			LOG_ERR("Failed to get cal raw data");
			return -EIO;
		}

		raw_data_sum += (uint16_t)sys_get_le16(cal_raw_data);

		loop_count++;
		counter_prev = counter_now;

		if (loop_count >= samples_to_avg) {
			break;
		}
	}

	err = set_calibration_mode(dev, BQ35100_MAC_CMD_EXIT_CAL);
	if (err < 0) {
		LOG_ERR("Failed to enter calibration mode");
		return err;
	}

	*avg_raw_data = raw_data_sum / samples_to_avg;
	LOG_DBG("sum: %d, avg: %d", raw_data_sum, *avg_raw_data);

	return 0;
}

static int store_voltage_gain(const struct device *dev, uint16_t known_volt, uint16_t avg_voltage)
{
	if (avg_voltage == 0) {
		LOG_ERR("Invalid average voltage");
		return -EINVAL;
	}

	uint8_t buffer[2];

	uint16_t gain = (known_volt * (UINT16_MAX + 1)) / avg_voltage;

	LOG_INF("Voltage gain: %u", gain);

	sys_put_be16(gain, buffer);
	return bq35100_write_extended_data(dev, BQ35100_FLASH_VIN_GAIN, buffer, ARRAY_SIZE(buffer));
}

int bq35100_calibrate_gauge_volt(const struct device *dev, const uint16_t known_volt)
{
	int err;
	uint16_t avg_voltage = 0;

	if (get_intern_sec_mode() == SECURITY_SEALED) {
		LOG_ERR("Invalid security mode sealed set");
		return -EINVAL;
	}

	err = bq35100_obtain_raw_calibration_data(dev, BQ35100_REG_CAL_VOLTAGE, &avg_voltage);
	if (err < 0) {
		LOG_ERR("Failed to calibrate avg raw AD voltage");
		return err;
	}

	err = store_voltage_gain(dev, known_volt, avg_voltage);
	if (err < 0) {
		LOG_ERR("Failed to store voltage gain in flash");
		return err;
	}

	return 0;
}

static int float_to_df(float val, uint8_t *result)
{
	int32_t exponent = 0;
	float mod_val = val;
	float tmp_val = 0;
	uint8_t data[4];

	if (isnan(val) || isinf(val) || result == NULL) {
		return -EINVAL;
	}

	if (val < 0.0f) {
		mod_val *= -1;
	}

	tmp_val = mod_val;

	tmp_val *= (1 + powf(2, -25));

	if (tmp_val < 0.5f) {
		while (tmp_val < 0.5f) {
			tmp_val *= 2;
			exponent--;
		}

	} else if (tmp_val >= 1.0f) {
		while (tmp_val >= 1.0f) {
			tmp_val /= 2;
			exponent++;
		}
	}

	if (exponent > 127) {
		exponent = 127;

	} else if (exponent < -128) {
		exponent = -128;
	}

	tmp_val = powf(2, 8 - exponent) * val - 128;

	data[2] = (uint8_t)tmp_val;
	tmp_val = powf(2, 8) * (tmp_val - data[2]);
	data[1] = (uint8_t)tmp_val;
	tmp_val = powf(2, 8) * (tmp_val - data[1]);
	data[0] = (uint8_t)tmp_val;

	if (val < 0.0f) {
		data[2] |= 0x80;
	}

	result[0] = exponent + 128;
	result[1] = data[2];
	result[2] = data[1];
	result[3] = data[0];

	LOG_HEXDUMP_DBG(result, 4, "df:");

	return 0;
}

int bq35100_calibrate_gauge_current(const struct device *dev, const uint16_t known_current)
{
	int err;
	uint8_t buffer[4] = {0};
	int16_t cc_offset;
	int8_t board_offset;
	uint16_t avg_current = 0;

	if (get_intern_sec_mode() == SECURITY_SEALED) {
		LOG_ERR("Invalid security mode sealed set");
		return -EINVAL;
	}

	err = bq35100_read_extended_data(dev, BQ35100_FLASH_CC_OFFSET, buffer, 2);
	if (err < 0) {
		LOG_ERR("Failed to read cc offset register");
		return err;
	}

	cc_offset = (int16_t)sys_get_be16(buffer);
	LOG_INF("cc_offset: %d", cc_offset);

	err = bq35100_read_extended_data(dev, BQ35100_FLASH_BOARD_OFFSET, buffer, 1);
	if (err < 0) {
		LOG_ERR("Failed to read board offset register");
		return err;
	}

	board_offset = (int8_t)buffer[0];
	LOG_INF("board_offset: %d", board_offset);

	err = bq35100_obtain_raw_calibration_data(dev, BQ35100_REG_CAL_CURRENT, &avg_current);
	if (err < 0) {
		LOG_ERR("Failed to calibrate avg raw AD voltage");
		return err;
	}

	float cc_gain =
		(float)known_current / ((int32_t)avg_current - (cc_offset + board_offset) / 16);

	float cc_delta = cc_gain * CC_DELTA_MULT_FACTOR;

	LOG_INF("cc_gain: %f", (double)cc_gain);
	LOG_INF("cc_delta: %f", (double)cc_delta);

	err = float_to_df(cc_gain, buffer);
	if (err < 0) {
		LOG_ERR("Failed to convert cc gain to df");
	}

	err = bq35100_write_extended_data(dev, BQ35100_FLASH_CC_GAIN, buffer, 4);
	if (err < 0) {
		LOG_ERR("Failed to write cc gain");
		return err;
	}

	err = float_to_df(cc_delta, buffer);
	if (err < 0) {
		LOG_ERR("Failed to convert cc delta to df");
	}

	err = bq35100_write_extended_data(dev, BQ35100_FLASH_CC_DELTA, buffer, 4);
	if (err < 0) {
		LOG_ERR("Failed to write cc delta");
		return err;
	}

	return 0;
}

int bq35100_perform_cc_offset(const struct device *dev)
{
	int err;
	uint16_t answer;

	if (get_intern_sec_mode() == SECURITY_SEALED) {
		LOG_ERR("Invalid security mode sealed set");
		return -EINVAL;
	}

	err = set_calibration_mode(dev, BQ35100_MAC_CMD_ENTER_CAL);
	if (err < 0) {
		LOG_ERR("Failed to enter calibration mode");
		return err;
	}

	while (1) {
		err = bq35100_send_cntl(dev, BQ35100_MAC_CMD_CC_OFFSET);
		if (err < 0) {
			LOG_ERR("Error sending cc offset mac command");
			return err;
		}

		err = bq35100_send_cntl(dev, BQ35100_MAC_CMD_CONTROL_STATUS);
		if (err < 0) {
			LOG_ERR("Error sending mac command control status %d", err);
			return err;
		}

		err = bq35100_get_status(dev, &answer);
		if (err < 0) {
			LOG_ERR("Failed to get status after sending cc offset mac command");
			return err;
		}

		if ((answer & BQ3500_CCA_BIT_MASK) == BQ3500_CCA_BIT_MASK) {
			LOG_INF("CCA bit set, device is calibrating cc offset now");
			break;
		}

		k_msleep(COMMAND_RETRY_TIMEOUT_MS);
	}

	while (1) {
		err = bq35100_get_status(dev, &answer);
		if (err < 0) {
			LOG_ERR("Failed to get status while calibrating cc offset");
			return err;
		}

		if ((answer & BQ3500_CCA_BIT_MASK) == 0) {
			LOG_INF("CCA bit cleared, device calibrated the cc offset");
			break;
		}

		k_msleep(CHECK_STATUS_RETRY_TIMEOUT_MS);
	}

	err = bq35100_send_cntl(dev, BQ35100_MAC_CMD_CC_OFFSET_SAVE);
	if (err < 0) {
		LOG_ERR("Error sending cc offset save mac command");
		return err;
	}

	err = set_calibration_mode(dev, BQ35100_MAC_CMD_EXIT_CAL);
	if (err < 0) {
		LOG_ERR("Failed to exit calibration mode");
		return err;
	}

	return 0;
}

int bq35100_perform_board_offset(const struct device *dev)
{
	int err;
	uint16_t answer;

	if (get_intern_sec_mode() == SECURITY_SEALED) {
		LOG_ERR("Invalid security mode sealed set");
		return -EINVAL;
	}

	err = set_calibration_mode(dev, BQ35100_MAC_CMD_ENTER_CAL);
	if (err < 0) {
		LOG_ERR("Failed to enter calibration mode");
		return err;
	}

	while (1) {
		err = bq35100_send_cntl(dev, BQ35100_MAC_CMD_BOARD_OFFSET);
		if (err < 0) {
			LOG_ERR("Error sending cc offset mac command");
			return err;
		}

		err = bq35100_send_cntl(dev, BQ35100_MAC_CMD_CONTROL_STATUS);
		if (err < 0) {
			LOG_ERR("Error sending mac command control status %d", err);
			return err;
		}

		err = bq35100_get_status(dev, &answer);
		if (err < 0) {
			LOG_ERR("Failed to get status after sending cc offset mac command");
			return err;
		}

		if ((answer & BQ3500_BCA_BIT_MASK) == BQ3500_BCA_BIT_MASK) {
			LOG_INF("BCA bit set, device is calibrating board offset now");
			break;
		}

		k_msleep(COMMAND_RETRY_TIMEOUT_MS);
	}

	while (1) {
		err = bq35100_get_status(dev, &answer);
		if (err < 0) {
			LOG_ERR("Failed to get status while calibrating board offset");
			return err;
		}

		if ((answer & BQ3500_BCA_BIT_MASK) == 0) {
			LOG_INF("BCA bit cleared, device calibrated the board offset");
			break;
		}

		k_msleep(CHECK_STATUS_RETRY_TIMEOUT_MS);
	}

	err = set_calibration_mode(dev, BQ35100_MAC_CMD_EXIT_CAL);
	if (err < 0) {
		LOG_ERR("Failed to exit calibration mode");
		return err;
	}

	return 0;
}

int bq35100_store_calibration_volt(const struct device *dev, uint16_t voltage_gain)
{
	uint8_t buffer[2];

	sys_put_be16(voltage_gain, buffer);
	return bq35100_write_extended_data(dev, BQ35100_FLASH_VIN_GAIN, buffer, ARRAY_SIZE(buffer));
}

int bq35100_store_calibration_cc_gain_delta(const struct device *dev, float cc_gain)
{
	int err;
	uint8_t buffer[4] = {0};
	float cc_delta = cc_gain * CC_DELTA_MULT_FACTOR;

	err = float_to_df(cc_gain, buffer);
	if (err < 0) {
		LOG_ERR("Failed to convert cc gain to df");
	}

	err = bq35100_write_extended_data(dev, BQ35100_FLASH_CC_GAIN, buffer, 4);
	if (err < 0) {
		LOG_ERR("Failed to write cc gain");
		return err;
	}

	err = float_to_df(cc_delta, buffer);
	if (err < 0) {
		LOG_ERR("Failed to convert cc delta to df");
	}

	err = bq35100_write_extended_data(dev, BQ35100_FLASH_CC_DELTA, buffer, 4);
	if (err < 0) {
		LOG_ERR("Failed to write cc delta");
		return err;
	}

	return 0;
}

int bq35100_store_calibration_cc_offset(const struct device *dev, int16_t cc_offset)
{
	uint8_t buffer[2];

	sys_put_be16(cc_offset, buffer);

	const int err = bq35100_write_extended_data(dev, BQ35100_FLASH_CC_OFFSET, buffer, 2);

	if (err < 0) {
		LOG_ERR("Failed to read cc offset register");
		return err;
	}

	return 0;
}

int bq35100_store_calibration_board_offset(const struct device *dev, int8_t board_offset)
{
	const int err =
		bq35100_write_extended_data(dev, BQ35100_FLASH_BOARD_OFFSET, &board_offset, 1);
	if (err < 0) {
		LOG_ERR("Failed to read board offset register");
		return err;
	}

	return 0;
}
