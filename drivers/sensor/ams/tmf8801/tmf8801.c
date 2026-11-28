/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file tmf8801.c
 * @brief TMF8801 Time-of-Flight distance sensor driver.
 *
 * Implements the Zephyr sensor driver API for the TMF8801 single-zone
 * ToF sensor. Starts the on-chip measurement application (APP0),
 * restores factory calibration and performs clock-ratio corrected
 * distance measurement over I2C.
 */

#include "tmf8801.h"
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT ams_tmf8801

LOG_MODULE_REGISTER(tmf8801, CONFIG_SENSOR_LOG_LEVEL);

/* Default command set for single-zone ranging. */
static const uint8_t TMF8801_DEFAULT_CMD_SET[9] = {0x01, 0xA3, 0x00, 0x00, 0x00,
						   0x64, 0x03, 0x84, 0x02};

/* Default factory calibration data (board/module-specific values). */
static const uint8_t TMF8801_DEFAULT_CALIB[14] = {0x41, 0x57, 0x01, 0xFD, 0x04, 0x00, 0x00,
						  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};

/* Default algorithm state data, used to restore the algorithm state at startup. */
static const uint8_t TMF8801_DEFAULT_ALGO_STATE[11] = {0xB1, 0xA9, 0x02, 0x00, 0x00, 0x00,
						       0x00, 0x00, 0x00, 0x00, 0x00};

/* Set or clear a bit in the measurement command set. */
static void tmf8801_modify_cmd_set(struct tmf8801_data *data, uint8_t index, uint8_t bit, bool val)
{
	if (index >= sizeof(data->measure_cmd_set) || bit > 7) {
		return;
	}

	if (val) {
		data->measure_cmd_set[index] |= BIT(bit);
	} else {
		data->measure_cmd_set[index] &= ~BIT(bit);
	}
}

/* Return true when the CPU is ready (PON and CPU_READY set). */
static bool tmf8801_is_cpu_ready(const struct device *dev)
{
	const struct tmf8801_config *cfg = dev->config;
	uint8_t val = 0;

	if (i2c_reg_read_byte_dt(&cfg->i2c, REG_TMF8801_ENABLE, &val) < 0) {
		return false;
	}

	return val == (TMF8801_ENABLE_CPU_READY | TMF8801_ENABLE_PON);
}

/* Return true when the given application ID is currently running. */
static bool tmf8801_is_appid(const struct device *dev, uint8_t appid)
{
	const struct tmf8801_config *cfg = dev->config;
	uint8_t val = 0;

	if (i2c_reg_read_byte_dt(&cfg->i2c, REG_TMF8801_APPID, &val) < 0) {
		return false;
	}

	return val == appid;
}

/* Poll the ENABLE register every 5 ms, up to 100 ms, until the CPU is ready. */
static bool tmf8801_wait_for_cpu_ready(const struct device *dev)
{
	for (int t = 0; t < 100; t += 5) {
		k_msleep(5);
		if (tmf8801_is_cpu_ready(dev)) {
			return true;
		}
	}

	LOG_ERR("Timeout waiting for CPU ready");
	return false;
}

/* Poll the APPID register every 5 ms, up to 100 ms, until APP0 is running. */
static bool tmf8801_wait_for_app0(const struct device *dev)
{
	for (int t = 0; t < 100; t += 5) {
		k_msleep(5);
		if (tmf8801_is_appid(dev, TMF8801_APPID_APP0)) {
			return true;
		}
	}

	LOG_ERR("Timeout waiting for APP0");
	return false;
}

/* Poll the CONTENTS register every 5 ms, up to 1000 ms, until it equals @p status. */
static bool tmf8801_check_status_register(const struct device *dev, uint8_t status)
{
	const struct tmf8801_config *cfg = dev->config;
	uint8_t val = 0;

	for (int t = 0; t < 1000; t += 5) {
		k_msleep(5);
		if (i2c_reg_read_byte_dt(&cfg->i2c, REG_TMF8801_CONTENTS, &val) == 0 &&
		    val == status) {
			return true;
		}
	}

	LOG_ERR("Timeout waiting for CONTENTS 0x%02X", status);
	return false;
}

/*
 * Read the result block. When a new transaction ID is seen, store the result and
 * update the host/module clock correction factor.
 *
 * Returns true if a new measurement result was stored.
 */
static bool tmf8801_is_data_ready(const struct device *dev)
{
	const struct tmf8801_config *cfg = dev->config;
	struct tmf8801_data *data = dev->data;
	struct tmf8801_result result = {0};
	uint32_t t = k_uptime_get_32();
	uint32_t sys_t;

	if (i2c_burst_read_dt(&cfg->i2c, REG_TMF8801_STATUS, (uint8_t *)&result, sizeof(result)) <
	    0) {
		return false;
	}

	if (result.reg_contents != 0x55 || result.tid == data->result.tid) {
		return false;
	}

	data->result = result;
	sys_t = ((uint32_t)result.sysclock3 << 24) | ((uint32_t)result.sysclock2 << 16) |
		((uint32_t)result.sysclock1 << 8) | (uint32_t)result.sysclock0;

	if (data->count < 4) {
		data->host_time[data->count] = t;
		data->module_time[data->count] = sys_t;
		data->count++;
	} else if (data->count == 4) {
		data->host_time[4] = t;
		data->module_time[4] = sys_t;

		if (data->module_time[4] > data->module_time[0] &&
		    data->host_time[4] >= data->host_time[0]) {
			/*
			 * ratio = delta_host(ms) / delta_module(0.2 us ticks)
			 *       = (delta_host * 5000) / delta_module
			 * Stored in Q16 fixed point (1 << 16 == 1.0).
			 */
			uint32_t delta_host = data->host_time[4] - data->host_time[0];
			uint32_t delta_module = data->module_time[4] - data->module_time[0];

			if (delta_module > 0) {
				data->timestamp =
					(uint32_t)((((uint64_t)delta_host * 5000ULL) << 16) /
						   delta_module);
			}
		}

		memmove(data->host_time, data->host_time + 1, 4 * sizeof(uint32_t));
		memmove(data->module_time, data->module_time + 1, 4 * sizeof(uint32_t));
	} else {
		data->count = 0;
	}

	return true;
}

/* Return the clock-corrected distance in millimetres and acknowledge the interrupt. */
static uint16_t tmf8801_get_distance_raw(const struct device *dev)
{
	const struct tmf8801_config *cfg = dev->config;
	struct tmf8801_data *data = dev->data;
	uint16_t rslt = ((uint16_t)data->result.dis_h << 8) | data->result.dis_l;

	rslt = (uint16_t)(((uint64_t)rslt * data->timestamp) >> 16);

	if (data->measure_cmd_set[CMDSET_INDEX_CMD6] & BIT(CMDSET_BIT_INT)) {
		i2c_reg_update_byte_dt(&cfg->i2c, REG_TMF8801_INT_STATUS, BIT(0), BIT(0));
	}

	return rslt;
}

/*
 * Apply the selected calibration mode, start ranging, and collect warm-up
 * samples used to build the timestamp correction factor.
 */
static bool tmf8801_set_calibration_mode(const struct device *dev, enum tmf8801_calib_mode mode)
{
	const struct tmf8801_config *cfg = dev->config;
	struct tmf8801_data *data = dev->data;
	uint8_t calib_cmd = 0x0B;

	if (!data->initialized || data->measure_cmd_flag) {
		return false;
	}

	switch (mode) {
	case TMF8801_MODE_CALIB:
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_CALIB, true);
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_ALGO, false);
		i2c_burst_write_dt(&cfg->i2c, REG_TMF8801_COMMAND, &calib_cmd, 1);
		i2c_burst_write_dt(&cfg->i2c, REG_TMF8801_RESULT_NUMBER, data->calib_data,
				   sizeof(data->calib_data));
		break;

	case TMF8801_MODE_CALIB_AND_ALGO_STATE:
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_CALIB, true);
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_ALGO, true);
		i2c_burst_write_dt(&cfg->i2c, REG_TMF8801_COMMAND, &calib_cmd, 1);
		i2c_burst_write_dt(&cfg->i2c, REG_TMF8801_RESULT_NUMBER, data->calib_data,
				   sizeof(data->calib_data));
		i2c_burst_write_dt(&cfg->i2c, REG_TMF8801_STATEDATAWR, data->algo_state_data,
				   sizeof(data->algo_state_data));
		break;

	default: /* TMF8801_MODE_NO_CALIB */
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_CALIB, false);
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_ALGO, false);
		break;
	}

	/* Write the full 9-byte command set starting at REG_TMF8801_CMD_DATA7 */
	i2c_burst_write_dt(&cfg->i2c, REG_TMF8801_CMD_DATA7, data->measure_cmd_set,
			   sizeof(data->measure_cmd_set));

	k_msleep(600);

	if (!tmf8801_check_status_register(dev, 0x55)) {
		LOG_ERR("Status 0x55 not reached after calib mode set");
		return false;
	}

	/* Warm-up: collect 4 samples to build the timestamp correction factor */
	while (data->count < 4) {
		if (tmf8801_is_data_ready(dev)) {
			tmf8801_get_distance_raw(dev);
		}
		k_msleep(2);
	}

	data->measure_cmd_flag = true;
	LOG_INF("Measurement started (calib mode %d)", (int)mode);
	return true;
}

/* Put the TMF8801 into low-power sleep by asserting CPU reset. */
static void tmf8801_sleep(const struct device *dev)
{
	const struct tmf8801_config *cfg = dev->config;
	struct tmf8801_data *data = dev->data;

	i2c_reg_update_byte_dt(&cfg->i2c, REG_TMF8801_ENABLE, TMF8801_ENABLE_CPU_RESET,
			       TMF8801_ENABLE_CPU_RESET);

	data->measure_cmd_flag = false;
	data->count = 0;
}

static int tmf8801_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct tmf8801_data *data = dev->data;

	if (chan != SENSOR_CHAN_DISTANCE && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if (!tmf8801_is_data_ready(dev)) {
		return -EBUSY;
	}

	if (data->result.dis_l == 0 && data->result.dis_h == 0) {
		return -ENODATA;
	}

	data->distance_mm = tmf8801_get_distance_raw(dev);
	return 0;
}

static int tmf8801_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct tmf8801_data *data = dev->data;

	if (chan != SENSOR_CHAN_DISTANCE) {
		return -ENOTSUP;
	}

	val->val1 = data->distance_mm / 1000;
	val->val2 = (data->distance_mm % 1000) * 1000;
	return 0;
}

static DEVICE_API(sensor, tmf8801_driver_api) = {
	.sample_fetch = tmf8801_sample_fetch,
	.channel_get = tmf8801_channel_get,
};

static int tmf8801_init(const struct device *dev)
{
	const struct tmf8801_config *cfg = dev->config;
	struct tmf8801_data *data = dev->data;
	uint8_t val;
	int ret;

	/* Driver data lives in bss and is already zeroed; only set non-zero defaults. */
	data->timestamp = (1u << 16);

	memcpy(data->measure_cmd_set, TMF8801_DEFAULT_CMD_SET, sizeof(data->measure_cmd_set));
	memcpy(data->calib_data, TMF8801_DEFAULT_CALIB, sizeof(data->calib_data));
	memcpy(data->algo_state_data, TMF8801_DEFAULT_ALGO_STATE, sizeof(data->algo_state_data));

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	/* Verify the device responds on the bus */
	ret = i2c_reg_read_byte_dt(&cfg->i2c, REG_TMF8801_APPID, &val);
	if (ret < 0) {
		LOG_ERR("TMF8801 not found on I2C (addr 0x%02X)", cfg->i2c.addr);
		return -ENODEV;
	}

	/* Reset the CPU, then power on the oscillator (PON = 1) */
	tmf8801_sleep(dev);

	ret = i2c_reg_write_byte_dt(&cfg->i2c, REG_TMF8801_ENABLE, TMF8801_ENABLE_PON);
	if (ret < 0) {
		LOG_ERR("Failed to power on (%d)", ret);
		return -EIO;
	}

	if (!tmf8801_wait_for_cpu_ready(dev)) {
		return -EIO;
	}

	/* After power-on the chip starts in the bootloader; request APP0 if needed */
	ret = i2c_reg_read_byte_dt(&cfg->i2c, REG_TMF8801_APPID, &val);
	if (ret < 0) {
		return -EIO;
	}
	LOG_INF("APPID after enable = 0x%02X", val);

	if (val == TMF8801_APPID_BOOTLOADER) {
		ret = i2c_reg_write_byte_dt(&cfg->i2c, REG_TMF8801_APPREQID, TMF8801_APPID_APP0);
		if (ret < 0) {
			LOG_ERR("Failed to request APP0 (%d)", ret);
			return -EIO;
		}

		if (!tmf8801_wait_for_app0(dev)) {
			return -EIO;
		}
	}

	data->initialized = true;

	if (!tmf8801_set_calibration_mode(dev, TMF8801_MODE_CALIB)) {
		LOG_ERR("Failed to start measurement");
		return -EIO;
	}

	LOG_INF("TMF8801 ready");
	return 0;
}

#define TMF8801_DEFINE(inst)                                                                       \
	static struct tmf8801_data tmf8801_data_##inst;                                            \
	static const struct tmf8801_config tmf8801_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tmf8801_init, NULL, &tmf8801_data_##inst,               \
				     &tmf8801_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &tmf8801_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMF8801_DEFINE)
