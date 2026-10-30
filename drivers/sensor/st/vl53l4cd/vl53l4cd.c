/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Darshan Maru <darshan.maru@dnkmail.in>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file vl53l4cd.c
 * @brief Driver implementation for the ST VL53L4CD Time-of-Flight sensor.
 *
 * Zephyr driver for the ST VL53L4CD Time-of-Flight ranging sensor. Samples
 * are read through the RTIO-based sensor read/decode API.
 *
 * Register semantics and ranging timing: ST VL53L4CD ULD API (STSW-IMG026).
 */

#define DT_DRV_COMPAT st_vl53l4cd

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/vl53l4cd.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/work.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>

#include "vl53l4cd.h"

LOG_MODULE_REGISTER(VL53L4CD, CONFIG_SENSOR_LOG_LEVEL);

#define VL53L4CD_BLOCK_SIZE      30   /* Max payload bytes per I2C write burst */
#define VL53L4CD_CFG_START_REG   0x30 /* First register of default configuration */
#define VL53L4CD_CFG_END_REG     0x87 /* Last register of default configuration */
#define VL53L4CD_BOOT_TIMEOUT_MS 100  /* Firmware boot timeout in ms */
#define VL53L4CD_VHV_TIMEOUT_MS  1000 /* VHV calibration timeout in ms */
#define VL53L4CD_READY_MARGIN_MS 100  /* Extra margin added to data-ready timeout in ms */

/**
 * @brief Default configuration for registers 0x30..0x87.
 *
 * Taken from the ST ULD VL53L4CD_DEFAULT_CONFIGURATION.
 * - 0x30: bit 4 = 1 -> interrupt active low (bits 3:0 must be 0x1)
 * - 0x46: 0x20 -> "new sample ready"
 * - 0x64/0x65: sigma threshold, 0x66/0x67: min count rate
 * - 0x6C..0x6F: inter-measurement (overwritten by range timing setup)
 * - 0x72..0x75: distance thresholds
 * - 0x86: clear interrupt, 0x87: start/stop
 *
 * All other registers are not user-modifiable.
 */
static const uint8_t vl53l4cd_default_config[] = {
	0x11, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, /* 0x30 - 0x37 */
	0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, /* 0x38 - 0x3f */
	0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, /* 0x40 - 0x47 */
	0x00, 0x00, 0x02, 0x14, 0x21, 0x00, 0x00, 0x05, /* 0x48 - 0x4f */
	0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, /* 0x50 - 0x57 */
	0xff, 0x01, 0x00, 0x08, 0x00, 0x00, 0x01, 0xcc, /* 0x58 - 0x5f */
	0x07, 0x01, 0xf1, 0x05, 0x00, 0xa0, 0x00, 0x80, /* 0x60 - 0x67 */
	0x08, 0x38, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x89, /* 0x68 - 0x6f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, /* 0x70 - 0x77 */
	0x07, 0x05, 0x06, 0x06, 0x00, 0x00, 0x02, 0xc7, /* 0x78 - 0x7f */
	0xff, 0x9b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, /* 0x80 - 0x87 */
};

BUILD_ASSERT(ARRAY_SIZE(vl53l4cd_default_config) ==
		     (VL53L4CD_CFG_END_REG - VL53L4CD_CFG_START_REG + 1),
	     "VL53L4CD default configuration size mismatch");

/**
 * @brief Read consecutive bytes starting at a VL53L4CD register.
 *
 * The sensor uses a 16-bit big-endian register index. A STOP condition is
 * issued between the index write and the data read.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] reg 16-bit register address of the first byte to read.
 * @param[out] buf Buffer to store the bytes read.
 * @param[in] len Number of bytes to read.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_read_regs(const struct device *dev, uint16_t reg, uint8_t *buf, size_t len)
{
	const struct vl53l4cd_config *cfg = dev->config;
	uint8_t addr[2];
	int ret;

	sys_put_be16(reg, addr);

	ret = i2c_write_dt(&cfg->i2c, addr, sizeof(addr));
	if (ret < 0) {
		return ret;
	}

	return i2c_read_dt(&cfg->i2c, buf, len);
}

/**
 * @brief Write consecutive bytes starting at a VL53L4CD register.
 *
 * The 16-bit big-endian register index and the payload are sent in a single
 * I2C write transaction.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] reg 16-bit register address of the first byte to write.
 * @param[in] data Bytes to write.
 * @param[in] len Number of bytes to write (at most VL53L4CD_BLOCK_SIZE).
 *
 * @retval 0 Success.
 * @retval -EINVAL @p len exceeds VL53L4CD_BLOCK_SIZE.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_write_regs(const struct device *dev, uint16_t reg, const uint8_t *data,
			       size_t len)
{
	const struct vl53l4cd_config *cfg = dev->config;
	uint8_t buf[2 + VL53L4CD_BLOCK_SIZE];

	if (len > VL53L4CD_BLOCK_SIZE) {
		return -EINVAL;
	}

	sys_put_be16(reg, buf);
	memcpy(&buf[2], data, len);

	return i2c_write_dt(&cfg->i2c, buf, 2 + len);
}

/**
 * @brief Read a single byte from a VL53L4CD register.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] reg 16-bit register address.
 * @param[out] val Pointer to store the byte read.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_read8(const struct device *dev, uint16_t reg, uint8_t *val)
{
	return vl53l4cd_read_regs(dev, reg, val, 1);
}

/**
 * @brief Read a 16-bit big-endian value from a VL53L4CD register.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] reg 16-bit register address of the most significant byte.
 * @param[out] val Pointer to store the assembled 16-bit value.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_read16(const struct device *dev, uint16_t reg, uint16_t *val)
{
	uint8_t buf[2];
	int ret = vl53l4cd_read_regs(dev, reg, buf, sizeof(buf));

	if (ret == 0) {
		*val = sys_get_be16(buf);
	}
	return ret;
}

/**
 * @brief Read a 32-bit big-endian value from a VL53L4CD register.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] reg 16-bit register address of the most significant byte.
 * @param[out] val Pointer to store the assembled 32-bit value.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_read32(const struct device *dev, uint16_t reg, uint32_t *val)
{
	uint8_t buf[4];
	int ret = vl53l4cd_read_regs(dev, reg, buf, sizeof(buf));

	if (ret == 0) {
		*val = sys_get_be32(buf);
	}
	return ret;
}

/**
 * @brief Write a single byte to a VL53L4CD register.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] reg 16-bit register address.
 * @param[in] val Value to write.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_write8(const struct device *dev, uint16_t reg, uint8_t val)
{
	return vl53l4cd_write_regs(dev, reg, &val, 1);
}

/**
 * @brief Write a 16-bit big-endian value to a VL53L4CD register.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] reg 16-bit register address of the most significant byte.
 * @param[in] val Value to write.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_write16(const struct device *dev, uint16_t reg, uint16_t val)
{
	uint8_t buf[2];

	sys_put_be16(val, buf);
	return vl53l4cd_write_regs(dev, reg, buf, sizeof(buf));
}

/**
 * @brief Write a 32-bit big-endian value to a VL53L4CD register.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] reg 16-bit register address of the most significant byte.
 * @param[in] val Value to write.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_write32(const struct device *dev, uint16_t reg, uint32_t val)
{
	uint8_t buf[4];

	sys_put_be32(val, buf);
	return vl53l4cd_write_regs(dev, reg, buf, sizeof(buf));
}

/**
 * @brief Block until a new ranging sample is available or a timeout expires.
 *
 * The interrupt polarity is active low (register 0x30 bit 4 = 1 in the
 * default configuration), so new data is available when
 * GPIO__TIO_HV_STATUS bit 0 is 0. This only reads a register; no interrupt
 * pin is used.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] timeout_ms Maximum time to wait, in milliseconds.
 *
 * @retval 0 Success, new data is ready.
 * @retval -ETIMEDOUT No new data within @p timeout_ms.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_wait_data_ready(const struct device *dev, uint32_t timeout_ms)
{
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(timeout_ms));
	uint8_t status;
	int ret;

	do {
		ret = vl53l4cd_read8(dev, VL53L4CD_GPIO__TIO_HV_STATUS, &status);
		if (ret < 0) {
			return ret;
		}
		if ((status & 0x01) == 0) {
			return 0;
		}
		k_sleep(K_MSEC(1));
	} while (!sys_timepoint_expired(end));

	return -ETIMEDOUT;
}

/**
 * @brief Clear the sensor data-ready interrupt flag.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_clear_interrupt(const struct device *dev)
{
	return vl53l4cd_write8(dev, VL53L4CD_SYSTEM__INTERRUPT_CLEAR, 0x01);
}

/**
 * @brief Stop ranging.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_stop_ranging(const struct device *dev)
{
	return vl53l4cd_write8(dev, VL53L4CD_SYSTEM_START, VL53L4CD_STOP);
}

/**
 * @brief Start ranging in continuous or autonomous mode.
 *
 * Reads the programmed inter-measurement period: continuous mode is used if
 * it is 0, otherwise autonomous (low power) mode is used.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_start_ranging(const struct device *dev)
{
	uint32_t inter_measurement = 0;
	int ret;

	ret = vl53l4cd_read32(dev, VL53L4CD_INTERMEASUREMENT_MS, &inter_measurement);
	if (ret < 0) {
		return ret;
	}

	return vl53l4cd_write8(dev, VL53L4CD_SYSTEM_START,
			       inter_measurement == 0 ? VL53L4CD_START_CONTINUOUS
						      : VL53L4CD_START_AUTONOMOUS);
}

/**
 * @brief Encode a timing budget into the RANGE_CONFIG_x register format.
 *
 * Based on VL53L4CD_SetRangeTiming(). The result is a 16-bit value whose
 * high byte is an exponent and whose low byte is a mantissa.
 *
 * @param budget_us_shifted Timing budget in microseconds, pre-shifted left by 12.
 * @param macro_period_us Macro period in microseconds.
 * @param factor Scaling factor (16 for RANGE_CONFIG_A, 12 for RANGE_CONFIG_B).
 * @param[out] out Encoded 16-bit register value.
 *
 * @retval 0 Success.
 * @retval -EINVAL Parameters cannot be encoded (zero divisor or zero result).
 */
static int vl53l4cd_encode_range_cfg(uint32_t budget_us_shifted, uint32_t macro_period_us,
				     uint32_t factor, uint16_t *out)
{
	uint32_t tmp = (macro_period_us * factor) >> 6;
	uint32_t ls_byte;
	uint8_t ms_byte = 0;

	if (tmp == 0) {
		return -EINVAL;
	}

	ls_byte = ((budget_us_shifted + (tmp >> 1)) / tmp);
	if (ls_byte == 0) {
		return -EINVAL;
	}
	ls_byte -= 1;

	while (ls_byte > 0xFF) {
		ls_byte >>= 1;
		ms_byte++;
	}

	*out = ((uint16_t)ms_byte << 8) | (uint16_t)ls_byte;
	return 0;
}

/**
 * @brief Program the ranging timing budget and inter-measurement period.
 *
 * Based on VL53L4CD_SetRangeTiming(). An inter-measurement period of 0
 * selects continuous mode; a non-zero value selects autonomous low power
 * mode. Ranging must be stopped by the caller.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] budget_ms Timing budget in milliseconds (10..200).
 * @param[in] inter_ms Inter-measurement period in milliseconds, or 0 for
 *                     continuous mode. Must be greater than @p budget_ms
 *                     when non-zero.
 *
 * @retval 0 Success.
 * @retval -EINVAL Timing budget out of range, or inter-measurement period
 *                 not greater than the timing budget.
 * @retval -EIO Oscillator frequency register read back as 0.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_set_range_timing(const struct device *dev, uint16_t budget_ms,
				     uint32_t inter_ms)
{
	uint16_t osc_frequency;
	uint16_t cfg_a;
	uint16_t cfg_b;
	uint32_t budget_us;
	uint32_t macro_period_us;
	int ret;

	if (budget_ms < VL53L4CD_TIMING_BUDGET_MIN_MS ||
	    budget_ms > VL53L4CD_TIMING_BUDGET_MAX_MS) {
		return -EINVAL;
	}
	if (inter_ms != 0 && inter_ms <= budget_ms) {
		return -EINVAL;
	}

	ret = vl53l4cd_read16(dev, VL53L4CD_OSC_FREQUENCY, &osc_frequency);
	if (ret < 0) {
		return ret;
	}
	if (osc_frequency == 0) {
		return -EIO;
	}

	budget_us = (uint32_t)budget_ms * 1000U;
	macro_period_us = ((uint32_t)2304 * (0x40000000U / osc_frequency)) >> 6;

	if (inter_ms == 0) {
		/* Continuous mode */
		ret = vl53l4cd_write32(dev, VL53L4CD_INTERMEASUREMENT_MS, 0);
		if (ret < 0) {
			return ret;
		}
		budget_us -= 2500U;
	} else {
		/* Autonomous low power mode */
		uint16_t clock_pll;
		uint32_t inter_measurement;

		ret = vl53l4cd_read16(dev, VL53L4CD_RESULT__OSC_CALIBRATE_VAL, &clock_pll);
		if (ret < 0) {
			return ret;
		}
		clock_pll &= 0x3FF;

		/* 1.055 * inter_ms * clock_pll in integer maths */
		inter_measurement = (uint32_t)(((uint64_t)inter_ms * clock_pll * 1055U) / 1000U);
		ret = vl53l4cd_write32(dev, VL53L4CD_INTERMEASUREMENT_MS, inter_measurement);
		if (ret < 0) {
			return ret;
		}

		budget_us -= 4300U;
		budget_us /= 2U;
	}

	budget_us <<= 12;

	ret = vl53l4cd_encode_range_cfg(budget_us, macro_period_us, 16, &cfg_a);
	if (ret < 0) {
		return ret;
	}
	ret = vl53l4cd_write16(dev, VL53L4CD_RANGE_CONFIG_A, cfg_a);
	if (ret < 0) {
		return ret;
	}

	ret = vl53l4cd_encode_range_cfg(budget_us, macro_period_us, 12, &cfg_b);
	if (ret < 0) {
		return ret;
	}

	return vl53l4cd_write16(dev, VL53L4CD_RANGE_CONFIG_B, cfg_b);
}

/**
 * @brief Read the latest ranging result from the sensor.
 *
 * Based on VL53L4CD_GetResult(). Reads registers 0x89..0x97 in one burst,
 * maps the raw range status to the ST ULD status code, and derives the
 * per-SPAD signal and ambient rates.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[out] out Encoded sample to fill in (timestamp is not modified).
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_read_results(const struct device *dev, struct vl53l4cd_encoded_data *out)
{
	/* Maps the raw hardware range status (5 bits) to the ST ULD status code */
	static const uint8_t status_rtn[24] = {255, 255, 255, 5,   2,   4,   1,   7,
					       3,   0,   255, 255, 9,   13,  255, 255,
					       255, 255, 10,  6,   255, 255, 11,  12};
	uint8_t buf[15]; /* 0x89 .. 0x97 */
	uint8_t status;
	int ret;

	ret = vl53l4cd_read_regs(dev, VL53L4CD_RESULT__RANGE_STATUS, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	status = buf[0] & 0x1F; /* 0x89 */
	if (status < ARRAY_SIZE(status_rtn)) {
		status = status_rtn[status];
	}

	out->range_status = status;
	out->spad_count = buf[3];                                      /* 0x8C */
	out->signal_rate_kcps = (uint32_t)sys_get_be16(&buf[5]) * 8U;  /* 0x8E */
	out->ambient_rate_kcps = (uint32_t)sys_get_be16(&buf[7]) * 8U; /* 0x90 */
	out->sigma_mm = sys_get_be16(&buf[9]) / 4U;                    /* 0x92 */
	out->range_mm = sys_get_be16(&buf[13]);                        /* 0x96 */

	/* Per-SPAD rates; avoid division by zero when no SPADs are reported */
	if (out->spad_count != 0) {
		out->signal_per_spad_kcps = out->signal_rate_kcps / out->spad_count;
		out->ambient_per_spad_kcps = out->ambient_rate_kcps / out->spad_count;
	} else {
		out->signal_per_spad_kcps = 0;
		out->ambient_per_spad_kcps = 0;
	}

	return 0;
}

/**
 * @brief Wait for and fetch one ranging sample.
 *
 * Waits for the data-ready flag (timeout is the timing budget plus the
 * inter-measurement period plus a fixed margin), timestamps the sample,
 * clears the interrupt and reads the result registers.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[out] edata Encoded sample to fill in.
 *
 * @retval 0 Success.
 * @retval -ETIMEDOUT No new data within the computed timeout.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_get_sample(const struct device *dev, struct vl53l4cd_encoded_data *edata)
{
	struct vl53l4cd_data *data = dev->data;
	uint32_t timeout_ms =
		data->timing_budget_ms + data->inter_measurement_ms + VL53L4CD_READY_MARGIN_MS;
	int ret;

	ret = vl53l4cd_wait_data_ready(dev, timeout_ms);
	if (ret < 0) {
		return ret;
	}

	edata->timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());

	ret = vl53l4cd_clear_interrupt(dev);
	if (ret < 0) {
		return ret;
	}

	return vl53l4cd_read_results(dev, edata);
}

/**
 * @brief Process an RTIO read request in the RTIO work queue context.
 *
 * Allocates the receive buffer from the SQE, fetches a sample under the
 * device lock and completes the SQE with success or the error code.
 *
 * @param[in] iodev_sqe RTIO submission queue entry to process.
 */
static void vl53l4cd_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = read_cfg->sensor;
	struct vl53l4cd_data *data = dev->data;
	struct vl53l4cd_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	int ret;

	ret = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata), &buf, &buf_len);
	if (ret < 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	edata = (struct vl53l4cd_encoded_data *)buf;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = vl53l4cd_get_sample(dev, edata);
	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		LOG_ERR("Failed to get sample (%d)", ret);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/**
 * @brief Submit an RTIO read request to the driver.
 *
 * The blocking sample fetch is deferred to the RTIO work queue, so the
 * submit call itself does not block on I2C or on the sensor.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] iodev_sqe RTIO submission queue entry to process.
 */
void vl53l4cd_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	ARG_UNUSED(dev);

	if (req == NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, vl53l4cd_submit_sync);
}

static int vl53l4cd_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct vl53l4cd_data *data = dev->data;
	uint16_t budget_ms = data->timing_budget_ms;
	uint32_t inter_ms = data->inter_measurement_ms;
	int ret;
	int ret_start;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DISTANCE) {
		return -ENOTSUP;
	}

	if (val->val1 < 0 || val->val2 != 0) {
		return -EINVAL;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_VL53L4CD_TIMING_BUDGET:
		if (val->val1 < VL53L4CD_TIMING_BUDGET_MIN_MS ||
		    val->val1 > VL53L4CD_TIMING_BUDGET_MAX_MS) {
			return -EINVAL;
		}
		budget_ms = (uint16_t)val->val1;
		break;
	case SENSOR_ATTR_VL53L4CD_INTER_MEASUREMENT:
		inter_ms = (uint32_t)val->val1;
		break;
	default:
		return -ENOTSUP;
	}

	if (inter_ms != 0 && inter_ms <= budget_ms) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = vl53l4cd_stop_ranging(dev);
	if (ret == 0) {
		ret = vl53l4cd_set_range_timing(dev, budget_ms, inter_ms);
	}
	/* Always try to resume ranging */
	ret_start = vl53l4cd_start_ranging(dev);
	if (ret == 0) {
		ret = ret_start;
	}
	if (ret == 0) {
		data->timing_budget_ms = budget_ms;
		data->inter_measurement_ms = inter_ms;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int vl53l4cd_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	struct vl53l4cd_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DISTANCE) {
		return -ENOTSUP;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_VL53L4CD_TIMING_BUDGET:
		val->val1 = data->timing_budget_ms;
		break;
	case SENSOR_ATTR_VL53L4CD_INTER_MEASUREMENT:
		val->val1 = data->inter_measurement_ms;
		break;
	default:
		return -ENOTSUP;
	}

	val->val2 = 0;
	return 0;
}

/**
 * @brief Wait for the sensor firmware to finish booting.
 *
 * Polls FIRMWARE__SYSTEM_STATUS until it reports the booted value, sleeping
 * briefly between reads. I2C read errors are tolerated while polling.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 Success, firmware booted.
 * @retval -ETIMEDOUT Firmware not booted within VL53L4CD_BOOT_TIMEOUT_MS.
 */
static int vl53l4cd_wait_boot(const struct device *dev)
{
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(VL53L4CD_BOOT_TIMEOUT_MS));
	uint8_t status;
	int ret;

	do {
		ret = vl53l4cd_read8(dev, VL53L4CD_FIRMWARE__SYSTEM_STATUS, &status);
		if (ret == 0 && status == VL53L4CD_FIRMWARE_BOOTED) {
			return 0;
		}
		k_sleep(K_MSEC(1));
	} while (!sys_timepoint_expired(end));

	return -ETIMEDOUT;
}

/**
 * @brief Load the default sensor configuration.
 *
 * Applies the I2C Fast-mode Plus and I/O voltage settings from devicetree,
 * then writes the ST default configuration (registers 0x30..0x87) in blocks
 * of at most VL53L4CD_BLOCK_SIZE bytes.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int vl53l4cd_load_default_config(const struct device *dev)
{
	const struct vl53l4cd_config *cfg = dev->config;
	size_t offset = 0;
	int ret;

	/* 0x2D: set bit 1 and 4 for fast mode plus (1 MHz I2C) */
	ret = vl53l4cd_write8(dev, VL53L4CD_FAST_MODE_PLUS_CTRL, cfg->fast_mode_plus ? 0x12 : 0x00);
	if (ret < 0) {
		return ret;
	}

	/* 0x2E / 0x2F: bit 0 = 0 for 1.8 V pull-up, 1 for pull-up at AVDD */
	ret = vl53l4cd_write8(dev, VL53L4CD_I2C_PULLUP_CTRL, cfg->io_1v8 ? 0x00 : 0x01);
	if (ret < 0) {
		return ret;
	}
	ret = vl53l4cd_write8(dev, VL53L4CD_GPIO_PULLUP_CTRL, cfg->io_1v8 ? 0x00 : 0x01);
	if (ret < 0) {
		return ret;
	}

	/* Write the default configuration table in I2C-sized chunks */
	while (offset < ARRAY_SIZE(vl53l4cd_default_config)) {
		size_t chunk =
			MIN(VL53L4CD_BLOCK_SIZE, ARRAY_SIZE(vl53l4cd_default_config) - offset);

		ret = vl53l4cd_write_regs(dev, VL53L4CD_CFG_START_REG + offset,
					  &vl53l4cd_default_config[offset], chunk);
		if (ret < 0) {
			return ret;
		}
		offset += chunk;
	}

	return 0;
}

static int vl53l4cd_init(const struct device *dev)
{
	const struct vl53l4cd_config *cfg = dev->config;
	struct vl53l4cd_data *data = dev->data;
	uint16_t model;
	int ret;

	k_mutex_init(&data->lock);

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	if (cfg->xshut.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->xshut)) {
			LOG_ERR("XSHUT GPIO not ready");
			return -ENODEV;
		}
		/* Hardware standby, then release; tBOOT is 1.2 ms max */
		ret = gpio_pin_configure_dt(&cfg->xshut, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
		k_msleep(1);
		ret = gpio_pin_set_dt(&cfg->xshut, 1);
		if (ret < 0) {
			return ret;
		}
		k_msleep(2);
	}

	/* Verify the model ID / module type before touching anything else */
	ret = vl53l4cd_read16(dev, VL53L4CD_IDENTIFICATION__MODEL_ID, &model);
	if (ret < 0) {
		LOG_ERR("Failed to read model ID (%d)", ret);
		return ret;
	}
	if (model != VL53L4CD_MODEL_ID_MODULE_TYPE) {
		LOG_ERR("Unexpected model ID/module type 0x%04x", model);
		return -ENODEV;
	}

	ret = vl53l4cd_wait_boot(dev);
	if (ret < 0) {
		LOG_ERR("Sensor boot timeout");
		return ret;
	}

	ret = vl53l4cd_load_default_config(dev);
	if (ret < 0) {
		return ret;
	}

	/* Start VHV calibration and wait for it to complete */
	ret = vl53l4cd_write8(dev, VL53L4CD_SYSTEM_START, VL53L4CD_START_VHV);
	if (ret < 0) {
		return ret;
	}
	ret = vl53l4cd_wait_data_ready(dev, VL53L4CD_VHV_TIMEOUT_MS);
	if (ret < 0) {
		LOG_ERR("VHV timeout");
		return ret;
	}

	/* Finish VHV: clear the interrupt, stop ranging and restore VHV settings */
	ret = vl53l4cd_clear_interrupt(dev);
	if (ret < 0) {
		return ret;
	}
	ret = vl53l4cd_stop_ranging(dev);
	if (ret < 0) {
		return ret;
	}
	ret = vl53l4cd_write8(dev, VL53L4CD_VHV_CONFIG__TIMEOUT_MACROP, 0x09);
	if (ret < 0) {
		return ret;
	}
	ret = vl53l4cd_write8(dev, VL53L4CD_VHV_CONFIG_0B, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = vl53l4cd_write16(dev, VL53L4CD_VHV_CONFIG_24, 0x500);
	if (ret < 0) {
		return ret;
	}

	/* Apply the timing configuration from devicetree */
	data->timing_budget_ms = cfg->timing_budget_ms;
	data->inter_measurement_ms = cfg->inter_measurement_ms;

	ret = vl53l4cd_set_range_timing(dev, data->timing_budget_ms, data->inter_measurement_ms);
	if (ret < 0) {
		LOG_ERR("Failed to set range timing (%d)", ret);
		return ret;
	}

	/* Ranging runs continuously; reads poll the data-ready register bit */
	return vl53l4cd_start_ranging(dev);
}

static DEVICE_API(sensor, vl53l4cd_api) = {
	.attr_set = vl53l4cd_attr_set,
	.attr_get = vl53l4cd_attr_get,
	.submit = vl53l4cd_submit,
	.get_decoder = vl53l4cd_get_decoder,
};

#define VL53L4CD_INIT(inst)                                                                        \
	BUILD_ASSERT(DT_INST_PROP(inst, timing_budget_ms) >= VL53L4CD_TIMING_BUDGET_MIN_MS &&      \
			     DT_INST_PROP(inst, timing_budget_ms) <=                               \
				     VL53L4CD_TIMING_BUDGET_MAX_MS,                                \
		     "timing-budget-ms must be 10..200");                                          \
	BUILD_ASSERT(DT_INST_PROP(inst, inter_measurement_ms) == 0 ||                              \
			     DT_INST_PROP(inst, inter_measurement_ms) >                            \
				     DT_INST_PROP(inst, timing_budget_ms),                         \
		     "inter-measurement-ms must be 0 or greater than timing-budget-ms");           \
                                                                                                   \
	static struct vl53l4cd_data vl53l4cd_data_##inst;                                          \
                                                                                                   \
	static const struct vl53l4cd_config vl53l4cd_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.xshut = GPIO_DT_SPEC_INST_GET_OR(inst, xshut_gpios, {0}),                         \
		.timing_budget_ms = DT_INST_PROP(inst, timing_budget_ms),                          \
		.inter_measurement_ms = DT_INST_PROP(inst, inter_measurement_ms),                  \
		.io_1v8 = DT_INST_PROP(inst, io_1v8),                                              \
		.fast_mode_plus = DT_INST_PROP(inst, fast_mode_plus),                              \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, vl53l4cd_init, NULL, &vl53l4cd_data_##inst,             \
				     &vl53l4cd_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &vl53l4cd_api);

DT_INST_FOREACH_STATUS_OKAY(VL53L4CD_INIT)
