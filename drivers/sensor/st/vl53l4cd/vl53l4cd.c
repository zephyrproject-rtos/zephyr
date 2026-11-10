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
 * are read through the RTIO-based sensor read/decode API using asynchronous
 * I2C transfers on the bus RTIO iodev.
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
#include <zephyr/rtio/rtio.h>
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
#define VL53L4CD_RTIO_SQ_SIZE    12   /* 6 SQEs per read, two reads in flight */
#define VL53L4CD_RTIO_CQ_SIZE    12   /* Completion queue size */

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
 * pin is used. Only used during initialization (VHV calibration); normal
 * sample reads are asynchronous, see vl53l4cd_submit().
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
		if ((status & VL53L4CD_HV_STATUS_NOT_READY) == 0) {
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
 * @brief Queue a chained "write register index, then read" on the I2C iodev.
 *
 * Each step is a separate chained transfer with a STOP in between, matching
 * vl53l4cd_read_regs().
 *
 * @param[in] data Driver runtime data holding the RTIO context and iodev.
 * @param[in] reg 16-bit register address of the first byte to read.
 * @param[out] buf Buffer that receives the bytes read.
 * @param[in] len Number of bytes to read.
 *
 * @retval 0 Success.
 * @retval -ENOMEM No free SQEs. The caller must call rtio_sqe_drop_all().
 */
static int vl53l4cd_rtio_queue_read(struct vl53l4cd_data *data, uint16_t reg, uint8_t *buf,
				    uint32_t len)
{
	struct rtio_sqe *wr_sqe = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *rd_sqe = rtio_sqe_acquire(data->rtio_ctx);
	uint8_t addr[2];

	if (wr_sqe == NULL || rd_sqe == NULL) {
		return -ENOMEM;
	}

	sys_put_be16(reg, addr);

	rtio_sqe_prep_tiny_write(wr_sqe, data->iodev, RTIO_PRIO_NORM, addr, sizeof(addr), NULL);
	wr_sqe->flags |= RTIO_SQE_CHAINED;
	wr_sqe->iodev_flags = RTIO_IODEV_I2C_STOP;

	rtio_sqe_prep_read(rd_sqe, data->iodev, RTIO_PRIO_NORM, buf, len, NULL);
	rd_sqe->flags |= RTIO_SQE_CHAINED;
	rd_sqe->iodev_flags = RTIO_IODEV_I2C_STOP;

	return 0;
}

/**
 * @brief Queue a chained single-byte register write on the I2C iodev.
 *
 * @param[in] data Driver runtime data holding the RTIO context and iodev.
 * @param[in] reg 16-bit register address.
 * @param[in] val Value to write.
 *
 * @retval 0 Success.
 * @retval -ENOMEM No free SQE. The caller must call rtio_sqe_drop_all().
 */
static int vl53l4cd_rtio_queue_write8(struct vl53l4cd_data *data, uint16_t reg, uint8_t val)
{
	struct rtio_sqe *sqe = rtio_sqe_acquire(data->rtio_ctx);
	uint8_t msg[3];

	if (sqe == NULL) {
		return -ENOMEM;
	}

	sys_put_be16(reg, msg);
	msg[2] = val;

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, msg, sizeof(msg), NULL);
	sqe->flags |= RTIO_SQE_CHAINED;
	sqe->iodev_flags = RTIO_IODEV_I2C_STOP;

	return 0;
}

/**
 * @brief Drain the completion queue and return the first bus error, if any.
 *
 * @param[in] rtio_ctx RTIO context to drain.
 *
 * @retval 0 All completions succeeded.
 * @retval -errno First negative result found in the completion queue.
 */
static int vl53l4cd_flush_cqes(struct rtio *rtio_ctx)
{
	struct rtio_cqe *cqe;
	int res = 0;

	do {
		cqe = rtio_cqe_consume(rtio_ctx);
		if (cqe != NULL) {
			if (cqe->result < 0 && res == 0) {
				LOG_ERR("Bus error: %d", cqe->result);
				res = cqe->result;
			}
			rtio_cqe_release(rtio_ctx, cqe);
		}
	} while (cqe != NULL);

	return res;
}

/**
 * @brief Completion callback of the read chain.
 *
 * Runs after the status read, result read and interrupt clear have finished.
 * Only checks for bus errors and data-ready; the raw registers are converted
 * by the decoder.
 *
 * @param[in] ctx RTIO context.
 * @param[in] sqe Callback SQE (unused).
 * @param[in] result Result of the previous operation (unused, CQEs are checked).
 * @param[in] arg0 Parent RTIO iodev SQE of the sensor read request.
 */
static void vl53l4cd_complete_cb(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
				 void *arg0)
{
	struct rtio_iodev_sqe *iodev_sqe = arg0;
	struct vl53l4cd_encoded_data *edata;
	uint32_t buf_len;
	uint8_t *buf;
	int ret;

	ARG_UNUSED(sqe);
	ARG_UNUSED(result);

	ret = vl53l4cd_flush_cqes(ctx);
	if (ret < 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	ret = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata), &buf, &buf_len);
	if (ret < 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}
	edata = (struct vl53l4cd_encoded_data *)buf;

	if ((edata->hv_status & VL53L4CD_HV_STATUS_NOT_READY) != 0) {
		rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/**
 * @brief Submit an RTIO read request to the driver.
 *
 * Queues one asynchronous I2C chain (status read, result read, interrupt
 * clear) on the bus iodev and returns immediately. No work queue is used and
 * nothing blocks; the request is completed from vl53l4cd_complete_cb().
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] iodev_sqe RTIO submission queue entry to process.
 */
void vl53l4cd_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct vl53l4cd_data *data = dev->data;
	struct vl53l4cd_encoded_data *edata;
	struct rtio_sqe *cb_sqe;
	uint32_t buf_len;
	uint8_t *buf;
	int ret;

	ret = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata), &buf, &buf_len);
	if (ret < 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}
	edata = (struct vl53l4cd_encoded_data *)buf;
	edata->timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());

	ret = vl53l4cd_rtio_queue_read(data, VL53L4CD_GPIO__TIO_HV_STATUS, &edata->hv_status, 1);
	if (ret == 0) {
		ret = vl53l4cd_rtio_queue_read(data, VL53L4CD_RESULT__RANGE_STATUS, edata->result,
					       sizeof(edata->result));
	}
	if (ret == 0) {
		ret = vl53l4cd_rtio_queue_write8(data, VL53L4CD_SYSTEM__INTERRUPT_CLEAR, 0x01);
	}
	if (ret == 0) {
		cb_sqe = rtio_sqe_acquire(data->rtio_ctx);
		if (cb_sqe == NULL) {
			ret = -ENOMEM;
		} else {
			rtio_sqe_prep_callback_no_cqe(cb_sqe, vl53l4cd_complete_cb, iodev_sqe,
						      NULL);
		}
	}
	if (ret < 0) {
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	rtio_submit(data->rtio_ctx, 0);
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
	ret = vl53l4cd_write8(dev, VL53L4CD_SYSTEM_START, VL53L4CD_START_AUTONOMOUS);
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

	/* Ranging runs continuously; reads fetch the latest result asynchronously */
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
	I2C_DT_IODEV_DEFINE(vl53l4cd_iodev_##inst, DT_DRV_INST(inst));                             \
	RTIO_DEFINE(vl53l4cd_rtio_ctx_##inst, VL53L4CD_RTIO_SQ_SIZE, VL53L4CD_RTIO_CQ_SIZE);       \
                                                                                                   \
	static struct vl53l4cd_data vl53l4cd_data_##inst = {                                       \
		.rtio_ctx = &vl53l4cd_rtio_ctx_##inst,                                             \
		.iodev = &vl53l4cd_iodev_##inst,                                                   \
	};                                                                                         \
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
