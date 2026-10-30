/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 * Author: Mahendra Sondagar <mahendra@dotcom.co.in>
 * Author: Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 */
/**
 * @file tmf8801.c
 * @brief TMF8801 Time-of-Flight distance sensor driver.
 *
 * Implements the Zephyr sensor driver API for the TMF8801 single-zone
 * ToF sensor. Supports RAM patch download via the bootloader protocol,
 * factory calibration restore, and clock-ratio corrected distance
 * measurement over I2C.
 */

#include "tmf8801.h"
#include "tmf8801_firmware.h"
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT ams_tmf8801

LOG_MODULE_REGISTER(tmf8801, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Default command set for single-zone ranging.
 */
static const uint8_t TMF8801_DEFAULT_CMD_SET[9] = {0x01, 0xA3, 0x00, 0x00, 0x00,
						   0x64, 0x03, 0x84, 0x02};

/**
 * @brief Default factory calibration data.
 *
 * Board/module-specific calibration values.
 */
static const uint8_t TMF8801_DEFAULT_CALIB[14] = {0x41, 0x57, 0x01, 0xFD, 0x04, 0x00, 0x00,
						  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};

/**
 * @brief Default algorithm state data.
 *
 * Used to restore the sensor algorithm state at startup.
 */
static const uint8_t TMF8801_DEFAULT_ALGO_STATE[11] = {0xB1, 0xA9, 0x02, 0x00, 0x00, 0x00,
						       0x00, 0x00, 0x00, 0x00, 0x00};

/**
 * @brief Write multiple bytes to a TMF8801 register.
 *
 * @param dev  TMF8801 device.
 * @param reg  Starting register address.
 * @param buf  Data buffer.
 * @param size Number of bytes to write.
 *
 * @return 0 on success, negative errno on failure.
 */
static int tmf8801_write_reg(const struct device *dev, uint8_t reg, const uint8_t *buf, size_t size)
{
	const struct tmf8801_config *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->i2c, reg, buf, size);
}

/**
 * @brief Write one byte to a TMF8801 register.
 *
 * @param dev TMF8801 device.
 * @param reg Register address.
 * @param val Value to write.
 *
 * @return 0 on success, negative errno on failure.
 */
static int tmf8801_write_byte(const struct device *dev, uint8_t reg, uint8_t val)
{
	return tmf8801_write_reg(dev, reg, &val, 1);
}

/**
 * @brief Read multiple bytes from a TMF8801 register.
 *
 * @param dev  TMF8801 device.
 * @param reg  Starting register address.
 * @param buf  Buffer for received data.
 * @param size Number of bytes to read.
 *
 * @return 0 on success, negative errno on failure.
 */
static int tmf8801_read_reg(const struct device *dev, uint8_t reg, uint8_t *buf, size_t size)
{
	const struct tmf8801_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->i2c, reg, buf, size);
}

/**
 * @brief Read one byte from a TMF8801 register.
 *
 * @param dev TMF8801 device.
 * @param reg Register address.
 * @param val Buffer for the received value.
 *
 * @return 0 on success, negative errno on failure.
 */
static int tmf8801_read_byte(const struct device *dev, uint8_t reg, uint8_t *val)
{
	return tmf8801_read_reg(dev, reg, val, 1);
}

/**
 * @brief Set or clear a bit in the measurement command set.
 *
 * @param data  TMF8801 driver data.
 * @param index Byte index.
 * @param bit   Bit position.
 * @param val   true to set, false to clear.
 */
static void tmf8801_modify_cmd_set(struct tmf8801_data *data, uint8_t index, uint8_t bit, bool val)
{
	if (index >= sizeof(data->measure_cmd_set) || bit > 7) {
		return;
	}
	if (val) {
		data->measure_cmd_set[index] |= (1u << bit);
	} else {
		data->measure_cmd_set[index] = ~((~data->measure_cmd_set[index]) | (1u << bit));
	}
}

/**
 * @brief Calculate the checksum of a calibration data buffer.
 *
 * @param data Data buffer to checksum.
 * @param len  Number of bytes in the buffer.
 *
 * @return One's-complement 8-bit checksum.
 */
static uint8_t tmf8801_cal_checksum(const uint8_t *data, uint8_t len)
{
	uint8_t sum = 0;

	for (uint8_t i = 0; i < len; i++) {
		sum += data[i];
	}
	return sum ^ 0xFF;
}

/**
 * @brief Check if the TMF8801 CPU is ready.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @retval true  CPU is ready.
 * @retval false CPU is not ready or the read failed.
 */
static bool tmf8801_is_cpu_ready(const struct device *dev)
{
	uint8_t val = 0;

	tmf8801_read_byte(dev, REG_TMF8801_ENABLE, &val);
	return (val == 0x41);
}

/**
 * @brief Check if the TMF8801 is running APP0.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @retval true  APP0 is active.
 * @retval false APP0 is not active or the read failed.
 */
static bool tmf8801_is_app0(const struct device *dev)
{
	uint8_t val = 0;

	tmf8801_read_byte(dev, REG_TMF8801_APPID, &val);
	return (val == TMF8801_APPID_APP0);
}

/**
 * @brief Check if the TMF8801 is in bootloader mode.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @retval true  Bootloader is active.
 * @retval false Bootloader is not active or the read failed.
 */
static bool tmf8801_is_bootloader(const struct device *dev)
{
	uint8_t val = 0;

	tmf8801_read_byte(dev, REG_TMF8801_APPID, &val);
	return (val == TMF8801_APPID_BOOTLOADER);
}

/**
 * @brief Read the TMF8801 CONTENTS register.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @return CONTENTS register value.
 */
static uint8_t tmf8801_get_reg_contents(const struct device *dev)
{
	uint8_t val = 0;

	tmf8801_read_byte(dev, REG_TMF8801_CONTENTS, &val);
	return val;
}

/* Wait functions */

/**
 * @brief Wait for the TMF8801 CPU to become ready.
 *
 * Polls the ENABLE register every 5 ms for up to 100 ms.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @retval true  CPU is ready.
 * @retval false Timeout waiting for CPU readiness.
 */
static bool tmf8801_wait_for_cpu_ready(const struct device *dev)
{
	for (int t = 0; t < 100; t += 5) {
		k_msleep(5);
		if (tmf8801_is_cpu_ready(dev)) {
			return true;
		}
	}
	LOG_ERR("waitForCpuReady timed out");
	return false;
}

/**
 * @brief Wait for the TMF8801 to enter bootloader mode.
 *
 * Polls the APPID register every 5 ms for up to 100 ms.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @retval true  Bootloader mode is active.
 * @retval false Timeout waiting for bootloader mode.
 */
static bool tmf8801_wait_for_bootloader(const struct device *dev)
{
	for (int t = 0; t < 100; t += 5) {
		k_msleep(5);
		if (tmf8801_is_bootloader(dev)) {
			return true;
		}
	}
	LOG_ERR("waitForBootloader timed out");
	return false;
}

/**
 * @brief Wait for the CONTENTS register to match a status value.
 *
 * Polls the register every 5 ms for up to 1000 ms.
 *
 * @param dev    Pointer to the TMF8801 device.
 * @param status Expected CONTENTS register value.
 *
 * @retval true  Expected status was received.
 * @retval false Timeout waiting for the expected status.
 */
static bool tmf8801_check_status_register(const struct device *dev, uint8_t status)
{
	for (int t = 0; t < 1000; t += 5) {
		k_msleep(5);
		if (tmf8801_get_reg_contents(dev) == status) {
			return true;
		}
	}
	LOG_ERR("checkStatusRegister: timeout waiting for 0x%02X", status);
	return false;
}

/**
 * @brief Read and validate a bootloader ACK response.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @retval true  ACK is valid.
 * @retval false I2C read failed or ACK is invalid.
 */
static bool tmf8801_read_status_ack(const struct device *dev)
{
	uint8_t buf[3] = {0, 0, 0};
	int ret = tmf8801_read_reg(dev, 0x08, buf, 3);

	if (ret < 0) {
		LOG_ERR("readStatusACK I2C error %d", ret);
		return false;
	}
	/* Only log failures */
	if (buf[2] != 0xFF) {
		LOG_ERR("BL ACK bad: [0x%02X][0x%02X][0x%02X] (expected [0x00][0x00][0xFF])",
			buf[0], buf[1], buf[2]);
		return false;
	}

	LOG_DBG("BL ACK OK");
	return true;
}

/**
 * @brief Request bootloader mode and wait for the transition.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @retval true  Device entered bootloader mode.
 * @retval false Bootloader transition timed out.
 */
static bool tmf8801_load_bootloader(const struct device *dev)
{
	uint8_t val = 0x80;

	tmf8801_write_reg(dev, REG_TMF8801_APPREQID, &val, 1);
	return tmf8801_wait_for_bootloader(dev);
}

/**
 * @brief Download the TMF8801 RAM patch.
 *
 * Performs the bootloader download sequence and waits for the CPU
 * to become ready after the device reset.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @retval true  RAM patch downloaded successfully.
 * @retval false RAM patch download or device readiness failed.
 */
static bool tmf8801_download_ram_patch(const struct device *dev)
{
	LOG_INF("Downloading RAM patch...");

	uint8_t buf[20];
	int flag;

	/* Step 1: ensure bootloader is running */
	if (!tmf8801_is_bootloader(dev)) {
		LOG_INF("Not in bootloader, switching...");
		if (!tmf8801_load_bootloader(dev)) {
			LOG_ERR("load bootloader failed");
			return false;
		}
	}

	/* Step 2: DOWNLOAD_INIT */
	{
		uint8_t init[4] = {0x14, 0x01, 0x29, 0xC1};

		tmf8801_write_reg(dev, 0x08, init, 4);
		k_msleep(10);
		if (!tmf8801_read_status_ack(dev)) {
			LOG_ERR("BL DOWNLOAD_INIT ACK failed");
			return false;
		}
		LOG_DBG("BL DOWNLOAD_INIT OK");
	}

	/* Step 3: SET_RAM_ADDR */
	{
		uint8_t addr_cmd[5] = {0x43, 0x02, 0x00, 0x00, 0xBA};

		tmf8801_write_reg(dev, 0x08, addr_cmd, 5);
		k_msleep(5);
	}

	/* Step 4: WRITE_RAM chunks */
	{
		const uint8_t *p = tmf8801_ram_patch;

		int chunk_count = 0;

		while ((flag = (int)(*p++)) > 0) {
			buf[0] = 0x41;
			buf[1] = (uint8_t)flag;
			memcpy(&buf[2], p, flag);
			p += flag;
			buf[2 + flag] = tmf8801_cal_checksum(buf, (uint8_t)(flag + 2));
			tmf8801_write_reg(dev, 0x08, buf, (size_t)(flag + 3));

			if (!tmf8801_read_status_ack(dev)) {
				LOG_ERR("BL WRITE_RAM ACK failed at chunk %d", chunk_count);
				return false;
			}
			chunk_count++;
		}
		LOG_INF("BL: %d firmware chunks written", chunk_count);
	}

	/* Step 5: RAMREMAP_RESET */
	{
		uint8_t reset_cmd[3] = {0x11, 0x00, 0xEE};

		tmf8801_write_reg(dev, 0x08, reset_cmd, 3);
		/* No ACK check here — device resets immediately */
		k_msleep(10);
	}

	/* Step 6: wait for CPU ready after reset */
	if (!tmf8801_wait_for_cpu_ready(dev)) {
		LOG_ERR("CPU not ready after RAM patch");
		return false;
	}
	LOG_INF("RAM patch downloaded successfully");
	return true;
}

/**
 * @brief Check for and store a new TMF8801 measurement result.
 *
 * Updates the measurement result and clock correction when a new
 * transaction ID is received.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @retval true  A new measurement result was stored.
 * @retval false No new measurement is available.
 */
static bool tmf8801_is_data_ready(const struct device *dev)
{
	struct tmf8801_data *data = dev->data;
	struct tmf8801_result result;
	uint32_t sys_t;
	uint32_t t;

	memset(&result, 0, sizeof(result));
	t = k_uptime_get_32();

	tmf8801_read_reg(dev, REG_TMF8801_STATUS, (uint8_t *)&result, sizeof(result));

	if (result.reg_contents == 0x55) {
		if (result.tid != data->result.tid) {
			data->result = result;
			sys_t = ((uint32_t)result.sysclock3 << 24) |
				((uint32_t)result.sysclock2 << 16) |
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
					 * timestamp calculation (integer):
					 *   ratio = t1/t2
					 *         = (delta_host*10) / (delta_module*0.002)
					 *         = (delta_host*5000) / delta_module
					 * host unit = ms, module unit = 0.2 µs. Reproduces the
					 * original floating-point ratio exactly using only
					 * integers (1<<16 == 1.0 in).
					 */
					uint32_t delta_host =
						data->host_time[4] - data->host_time[0];
					uint32_t delta_module =
						data->module_time[4] - data->module_time[0];

					if (delta_module > 0) {
						data->timestamp =
							(uint32_t)(((uint64_t)delta_host * 5000ULL
								    << 16) /
								   delta_module);
					}
				}
				memmove(data->host_time, data->host_time + 1, 4 * sizeof(uint32_t));
				memmove(data->module_time, data->module_time + 1,
					4 * sizeof(uint32_t));
			} else {
				data->count = 0;
			}
			return true;
		}
	}

	return false;
}

/**
 * @brief Get the clock-corrected raw distance.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @return Distance in millimetres.
 */
static uint16_t tmf8801_get_distance_raw(const struct device *dev)
{
	struct tmf8801_data *data = dev->data;

	uint16_t rslt = ((uint16_t)data->result.dis_h << 8) | data->result.dis_l;

	rslt = (uint16_t)(((uint64_t)rslt * data->timestamp) >> 16);

	if (data->measure_cmd_set[CMDSET_INDEX_CMD6] & (1u << CMDSET_BIT_INT)) {
		uint8_t val = 0;

		tmf8801_read_byte(dev, REG_TMF8801_INT_STATUS, &val);
		val |= 0x01;
		tmf8801_write_byte(dev, REG_TMF8801_INT_STATUS, val);
	}
	return rslt;
}

/**
 * @brief Set the TMF8801 calibration mode and start measurement.
 *
 * Applies the selected calibration mode, starts ranging, and
 * collects warm-up samples for timestamp correction.
 *
 * @param dev  Pointer to the TMF8801 device.
 * @param mode Calibration mode to apply.
 *
 * @retval true  Calibration mode set and measurement started.
 * @retval false Driver is busy, not initialized, or setup failed.
 */
static bool tmf8801_set_calibration_mode(const struct device *dev, enum tmf8801_calib_mode mode)
{
	struct tmf8801_data *data = dev->data;

	if (!data->initialized || data->measure_cmd_flag) {
		return false;
	}

	uint8_t calib_cmd = 0x0B;

	switch (mode) {
	case TMF8801_MODE_CALIB:
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_CALIB, true);
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_ALGO, false);
		tmf8801_write_reg(dev, REG_TMF8801_COMMAND, &calib_cmd, 1);
		tmf8801_write_reg(dev, REG_TMF8801_RESULT_NUMBER, data->calib_data,
				  sizeof(data->calib_data));
		break;

	case TMF8801_MODE_CALIB_AND_ALGO_STATE:
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_CALIB, true);
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_ALGO, true);
		tmf8801_write_reg(dev, REG_TMF8801_COMMAND, &calib_cmd, 1);
		tmf8801_write_reg(dev, REG_TMF8801_RESULT_NUMBER, data->calib_data,
				  sizeof(data->calib_data));
		tmf8801_write_reg(dev, REG_TMF8801_STATEDATAWR, data->algo_state_data,
				  sizeof(data->algo_state_data));
		break;

	default: /* TMF8801_MODE_NO_CALIB */
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_CALIB, false);
		tmf8801_modify_cmd_set(data, CMDSET_INDEX_CMD7, CMDSET_BIT_ALGO, false);
		break;
	}

	/* Write full 9-byte command set starting at REG_TMF8801_CMD_DATA7 (0x08) */
	tmf8801_write_reg(dev, REG_TMF8801_CMD_DATA7, data->measure_cmd_set,
			  sizeof(data->measure_cmd_set));

	k_msleep(600);

	if (!tmf8801_check_status_register(dev, 0x55)) {
		LOG_ERR("Status 0x55 not reached after calib mode set");
		return false;
	}

	/* Warm-up: collect 4 samples to build timestamp correction factor */
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

/**
 * @brief Put the TMF8801 into low-power sleep.
 *
 * Asserts CPU reset and clears the measurement state.
 *
 * @param dev Pointer to the TMF8801 device.
 */
static void tmf8801_sleep(const struct device *dev)
{
	struct tmf8801_data *data = dev->data;
	uint8_t val = 0;

	tmf8801_read_byte(dev, REG_TMF8801_ENABLE, &val);
	val |= TMF8801_ENABLE_CPU_RESET;
	tmf8801_write_byte(dev, REG_TMF8801_ENABLE, val);

	data->measure_cmd_flag = false;
	data->count = 0;
}

/**
 * @brief Fetch a new distance sample from the TMF8801.
 *
 * @param dev  Pointer to the TMF8801 device.
 * @param chan Sensor channel to fetch.
 *
 * @retval 0         Sample fetched successfully.
 * @retval -ENOTSUP  Unsupported sensor channel.
 * @retval -EBUSY    No new measurement is available.
 */
static int tmf8801_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct tmf8801_data *data = dev->data;

	if (chan != SENSOR_CHAN_DISTANCE && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if (!tmf8801_is_data_ready(dev)) {
		return -EBUSY;
	}

	data->distance_mm = tmf8801_get_distance_raw(dev);
	return 0;
}

/**
 * @brief Get the last fetched distance value.
 *
 * Converts the distance from millimetres to the Zephyr sensor_value format.
 *
 * @param dev  Pointer to the TMF8801 device.
 * @param chan Sensor channel to read.
 * @param val  Pointer to the sensor_value structure.
 *
 * @retval 0         Distance value retrieved successfully.
 * @retval -ENOTSUP  Unsupported sensor channel.
 */
static int tmf8801_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_DISTANCE) {
		return -ENOTSUP;
	}
	struct tmf8801_data *data = dev->data;

	val->val1 = data->distance_mm / 1000;
	val->val2 = (data->distance_mm % 1000) * 1000;
	return 0;
}

/** @brief Sensor API callbacks for the TMF8801 driver. */
static DEVICE_API(sensor, tmf8801_driver_api) = {
	.sample_fetch = tmf8801_sample_fetch,
	.channel_get = tmf8801_channel_get,
};

/**
 * @brief Initialize the TMF8801 sensor.
 *
 * Initializes GPIOs, verifies the I2C device, loads the RAM patch if
 * required, and starts ranging with calibration enabled.
 *
 * @param dev Pointer to the TMF8801 device.
 *
 * @retval 0       Sensor initialized successfully.
 * @retval -ENODEV I2C bus is not ready or device is not detected.
 * @retval -EIO    Sensor initialization or firmware setup failed.
 */
static int tmf8801_init(const struct device *dev)
{
	struct tmf8801_data *data = dev->data;
	const struct tmf8801_config *cfg = dev->config;
	uint8_t val;
	int ret;

	/* Zero all runtime state */
	data->initialized = false;
	data->measure_cmd_flag = false;
	data->count = 0;
	data->timestamp = (1u << 16);
	memset(data->host_time, 0, sizeof(data->host_time));
	memset(data->module_time, 0, sizeof(data->module_time));
	memset(&data->result, 0, sizeof(data->result));

	/* Load defaults */
	memcpy(data->measure_cmd_set, TMF8801_DEFAULT_CMD_SET, sizeof(data->measure_cmd_set));
	memcpy(data->calib_data, TMF8801_DEFAULT_CALIB, sizeof(data->calib_data));
	memcpy(data->algo_state_data, TMF8801_DEFAULT_ALGO_STATE, sizeof(data->algo_state_data));

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	/* Verify device on bus */
	ret = tmf8801_read_byte(dev, REG_TMF8801_APPID, &val);
	if (ret < 0) {
		LOG_ERR("TMF8801 not found on I2C (addr 0x%02X)", cfg->i2c.addr);
		return -ENODEV;
	}

	/* sleep() → CPU reset */
	tmf8801_sleep(dev);

	/* PON = 1 → enable oscillator */
	tmf8801_write_byte(dev, REG_TMF8801_ENABLE, 1);

	if (!tmf8801_wait_for_cpu_ready(dev)) {
		LOG_ERR("CPU not ready after enable");
		return -EIO;
	}

	tmf8801_read_byte(dev, REG_TMF8801_APPID, &val);
	LOG_INF("APPID after enable = 0x%02X", val);

	if (val == TMF8801_APPID_BOOTLOADER) {
		LOG_INF("In bootloader - downloading RAM patch");
		if (!tmf8801_download_ram_patch(dev)) {
			LOG_ERR("RAM patch download failed");
			return -EIO;
		}
		if (!tmf8801_is_app0(dev)) {
			LOG_ERR("APP0 not running after patch");
			return -EIO;
		}
	} else {
		LOG_INF("APP0 already running");
	}

	data->initialized = true;

	/* startMeasurement(TMF8801_MODE_CALIB) */
	if (!tmf8801_set_calibration_mode(dev, TMF8801_MODE_CALIB)) {
		LOG_ERR("startMeasurement failed");
		return -EIO;
	}

	LOG_INF("TMF8801 ready");
	return 0;
}

/**
 * @brief Define a TMF8801 device instance from devicetree.
 *
 * Creates the instance data and configuration and registers the
 * device with the Zephyr sensor subsystem.
 *
 * @param inst Devicetree instance index.
 */
#define TMF8801_DEFINE(inst)                                                                       \
	static struct tmf8801_data tmf8801_data_##inst;                                            \
	static const struct tmf8801_config tmf8801_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tmf8801_init, NULL, &tmf8801_data_##inst,               \
				     &tmf8801_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &tmf8801_driver_api);

/** @brief Instantiate one TMF8801 driver for every enabled DT node. */
DT_INST_FOREACH_STATUS_OKAY(TMF8801_DEFINE)
