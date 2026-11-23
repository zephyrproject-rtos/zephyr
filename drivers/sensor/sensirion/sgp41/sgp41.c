/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sensirion SGP41 VOC and NOx gas sensor driver.
 *
 * Provides raw VOC and NOx signal ticks through the sensor
 * Read and Decode API using RTIO.
 */

#define DT_DRV_COMPAT sensirion_sgp41

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include "sgp41.h"

LOG_MODULE_REGISTER(SGP41, CONFIG_SENSOR_LOG_LEVEL);

#define SGP41_CONDITIONING_SAFE_MS 9000 /* Safe conditioning window (ms), below 10 s limit */
#define SGP41_COND_NOT_STARTED     (-1) /* Sentinel: conditioning has not been started yet */

/**
 * @brief Compute the SGP41 CRC-8 checksum for a 16-bit word.
 *
 * The SGP41 uses CRC-8 with polynomial 0x31 and initial value 0xFF.
 * The word is serialised big-endian before the CRC is calculated.
 *
 * @param v 16-bit value to checksum.
 * @return Computed CRC-8 byte.
 */
static uint8_t sgp41_crc(uint16_t v)
{
	uint8_t b[2];

	sys_put_be16(v, b);
	return crc8(b, 2, SGP41_CRC_POLY, SGP41_CRC_INIT, false);
}

/**
 * @brief Send a command to the SGP41 over I2C.
 *
 * @param dev         Pointer to the sensor device.
 * @param cmd         16-bit command code.
 * @param with_params If true, append default RH/T compensation ticks and CRCs.
 * @return 0 on success, negative errno on I2C error.
 */
static int sgp41_send(const struct device *dev, uint16_t cmd, bool with_params)
{
	const struct sgp41_config *cfg = dev->config;
	uint8_t tx[8];

	sys_put_be16(cmd, tx);
	if (!with_params) {
		return i2c_write_dt(&cfg->bus, tx, 2);
	}
	sys_put_be16(SGP41_DEFAULT_RH_TICKS, &tx[2]);
	tx[4] = sgp41_crc(SGP41_DEFAULT_RH_TICKS);
	sys_put_be16(SGP41_DEFAULT_T_TICKS, &tx[5]);
	tx[7] = sgp41_crc(SGP41_DEFAULT_T_TICKS);
	return i2c_write_dt(&cfg->bus, tx, sizeof(tx));
}

/**
 * @brief Read @p n 16-bit words from the SGP41, verifying each CRC byte.
 *
 * @param dev Pointer to the sensor device.
 * @param[out] w   Output array of at least @p n uint16_t values.
 * @param n   Number of words to read; must be 1 or 2.
 * @retval 0       Success.
 * @retval -EINVAL @p n is 0 or greater than 2.
 * @retval -EIO    CRC mismatch on one or more received words.
 * @return         Negative errno on I2C bus error.
 */
static int sgp41_read_words(const struct device *dev, uint16_t *w, size_t n)
{
	const struct sgp41_config *cfg = dev->config;
	uint8_t rx[6];
	int rc;

	if (n == 0 || n > 2) {
		return -EINVAL;
	}
	rc = i2c_read_dt(&cfg->bus, rx, n * 3);
	if (rc < 0) {
		return rc;
	}
	for (size_t i = 0; i < n; i++) {
		w[i] = sys_get_be16(&rx[i * 3]);
		if (sgp41_crc(w[i]) != rx[i * 3 + 2]) {
			return -EIO;
		}
	}
	return 0;
}

/**
 * @brief Run the SGP41 on-chip self-test and return the result.
 *
 * Blocks the calling thread for SGP41_SELFTEST_WAIT_MS milliseconds.
 *
 * @param dev Pointer to the sensor device.
 * @retval 0    Self-test passed.
 * @retval -EIO Self-test failed; one or both pixels reported an error.
 * @return      Negative errno on I2C bus error.
 */
static int sgp41_selftest(const struct device *dev)
{
	uint16_t res;
	int rc = sgp41_send(dev, SGP41_CMD_SELF_TEST, false);

	if (rc < 0) {
		return rc;
	}
	k_sleep(K_MSEC(SGP41_SELFTEST_WAIT_MS));
	rc = sgp41_read_words(dev, &res, 1);
	if (rc < 0) {
		return rc;
	}
	/* Bit 0 = VOC pixel fail, bit 1 = NOx pixel fail; 0x00 = all pass */
	return (res & 0x03) ? -EIO : 0;
}

/**
 * @brief Delayed work handler: read measurement results from the SGP41.
 *
 * Scheduled to run SGP41_MEASURE_WAIT_MS milliseconds after a command is
 * sent. Runs in the system work queue. Reads either 1 word (conditioning
 * phase, VOC only) or 2 words (normal measurement, VOC + NOx), fills the
 * RTIO output buffer with an sgp41_encoded_data frame, and signals
 * completion or error back to the waiting RTIO SQE.
 *
 * @param work Pointer to the k_work item embedded in sgp41_data.
 */
static void sgp41_work_handler(struct k_work *work)
{
	struct k_work_delayable *dw = k_work_delayable_from_work(work);
	struct sgp41_data *data = CONTAINER_OF(dw, struct sgp41_data, work);
	struct rtio_iodev_sqe *iodev_sqe = data->pending;
	struct sgp41_encoded_data *edata;
	uint16_t w[2] = {0};
	uint8_t *buf;
	uint32_t len;
	int rc;

	data->pending = NULL;

	rc = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata), &buf, &len);
	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}
	edata = (struct sgp41_encoded_data *)buf;
	memset(edata, 0, sizeof(*edata));

	/*
	 * Read result BEFORE setting has_voc / has_nox so that stale w[]
	 * values are never marked valid if the I2C read fails.
	 */
	if (data->conditioning_end > 0) {
		/* Conditioning returns only SRAW_VOC */
		rc = sgp41_read_words(data->dev, w, 1);
		if (rc < 0) {
			LOG_ERR("Conditioning read failed: %d", rc);
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return;
		}
		edata->sraw_voc = w[0];
		edata->has_voc = 1;
	} else {
		/* Normal measurement returns SRAW_VOC and SRAW_NOX */
		rc = sgp41_read_words(data->dev, w, 2);
		if (rc < 0) {
			LOG_ERR("Measurement read failed: %d", rc);
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return;
		}
		edata->sraw_voc = w[0];
		edata->sraw_nox = w[1];
		edata->has_voc = 1;
		edata->has_nox = 1;
	}

	edata->timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/**
 * @brief RTIO submit callback: validate and dispatch a sensor read request.
 *
 * Validates the incoming RTIO SQE (operation type, streaming flag, channel
 * types), manages the conditioning state machine, sends the appropriate
 * SGP41 command (SGP41_CMD_CONDITIONING or SGP41_CMD_MEASURE_RAW), and
 * schedules sgp41_work_handler to collect the result after the mandatory
 * measurement wait time.
 *
 * Only one request may be in flight at a time; a second concurrent call
 * returns -EBUSY.
 *
 * @param dev       Pointer to the sensor device.
 * @param iodev_sqe RTIO submission queue entry for the read request.
 */
static void sgp41_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct sgp41_data *data = dev->data;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	uint16_t cmd;
	int64_t now;
	int ret;

	if (iodev_sqe->sqe.op != RTIO_OP_RX) {
		rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
		return;
	}
	if (cfg->is_streaming) {
		LOG_ERR("Streaming not supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}
	for (size_t i = 0; i < cfg->count; i++) {
		int t = cfg->channels[i].chan_type;

		if (t != SENSOR_CHAN_GAS_RES && t != SGP41_CHAN_NOX && t != SENSOR_CHAN_ALL) {
			LOG_ERR("Unsupported channel %d", t);
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}
	if (data->pending != NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
		return;
	}

	now = k_uptime_get();
	if (data->conditioning_end == SGP41_COND_NOT_STARTED) {
		data->conditioning_end = now + SGP41_CONDITIONING_SAFE_MS;
	}
	if (data->conditioning_end > 0 && now < data->conditioning_end) {
		cmd = SGP41_CMD_CONDITIONING;
	} else {
		data->conditioning_end = 0;
		cmd = SGP41_CMD_MEASURE_RAW;
	}

	ret = sgp41_send(dev, cmd, true);
	if (ret < 0) {
		LOG_ERR("Command 0x%04x failed: %d", cmd, ret);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	data->pending = iodev_sqe;
	k_work_schedule(&data->work, K_MSEC(SGP41_MEASURE_WAIT_MS));
}

static int sgp41_init(const struct device *dev)
{
	const struct sgp41_config *cfg = dev->config;
	struct sgp41_data *data = dev->data;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->bus.bus);
		return -ENODEV;
	}

	data->dev = dev;
	data->pending = NULL;
	data->conditioning_end = SGP41_COND_NOT_STARTED;
	k_work_init_delayable(&data->work, sgp41_work_handler);

	if (cfg->selftest) {
		int rc = sgp41_selftest(dev);

		if (rc < 0) {
			LOG_ERR("Selftest failed: %d", rc);
			return rc;
		}
		LOG_DBG("Selftest succeeded");
	}

	return 0;
}

static DEVICE_API(sensor, sgp41_api) = {
	.submit = sgp41_submit,
	.get_decoder = sgp41_get_decoder,
};

#define SGP41_INIT(n)                                                                              \
	static struct sgp41_data sgp41_data_##n;                                                   \
	static const struct sgp41_config sgp41_config_##n = {                                      \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.selftest = DT_INST_PROP(n, enable_selftest),                                      \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(n, sgp41_init, NULL, &sgp41_data_##n, &sgp41_config_##n,      \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &sgp41_api);

DT_INST_FOREACH_STATUS_OKAY(SGP41_INIT)
