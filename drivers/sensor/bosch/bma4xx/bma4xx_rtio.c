/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma4xx

#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/work.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor_clock.h>

#include "bma4xx.h"
#include "bma4xx_decoder.h"
#include "bma4xx_defs.h"
#include "bma4xx_rtio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bma4xx, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_BMA4XX_TEMPERATURE
/**
 * @brief Read temperature register on BMA4xx
 */
static int bma4xx_get_temp_data(const struct device *dev, int8_t *temp)
{
	struct bma4xx_data *bma4xx = dev->data;
	int status;

	status = bma4xx->hw_ops->read_reg(dev, BMA4XX_REG_TEMPERATURE, temp);
	if (status) {
		LOG_ERR("could not read temp reg: %d", status);
		return status;
	}

	LOG_DBG("temp reg val is %d", *temp);
	return 0;
}
#endif

static int bma4xx_get_accel_data(const struct device *dev, struct bma4xx_xyz_accel_data *accel_xyz)
{
	struct bma4xx_data *bma4xx = dev->data;
	uint8_t read_data[6];
	int status;
	uint8_t value = 0;

	if (!IS_ENABLED(CONFIG_BMA4XX_STREAM)) {
		do {
			status = bma4xx->hw_ops->read_reg(dev, BMA4XX_REG_INT_STAT_1, &value);
			if (status) {
				LOG_ERR("could not read int status reg: %d", status);
				return status;
			}
		} while (FIELD_GET(BMA4XX_BIT_INT_STAT_1_ACC_DRDY_INT, value) != 1);
	}

	/* Burst read regs DATA_8 through DATA_13, which holds the accel readings */
	status = bma4xx->hw_ops->read_data(dev, BMA4XX_REG_DATA_8, (uint8_t *)&read_data,
					   BMA4XX_REG_DATA_13 - BMA4XX_REG_DATA_8 + 1);
	if (status < 0) {
		LOG_ERR("Cannot read accel data: %d", status);
		return status;
	}

	LOG_DBG("Raw values [0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x]", read_data[0],
		read_data[1], read_data[2], read_data[3], read_data[4], read_data[5]);

	/* Values in accel_data[N] are left-aligned and will read 16x actual */
	accel_xyz->x = (((int16_t)read_data[1]) << 4) | FIELD_GET(GENMASK(7, 4), read_data[0]);
	accel_xyz->y = (((int16_t)read_data[3]) << 4) | FIELD_GET(GENMASK(7, 4), read_data[2]);
	accel_xyz->z = (((int16_t)read_data[5]) << 4) | FIELD_GET(GENMASK(7, 4), read_data[4]);

	LOG_DBG("XYZ reg vals are %d, %d, %d", accel_xyz->x, accel_xyz->y, accel_xyz->z);

	return 0;
}

/*
 * RTIO submit and encoding
 */

static void bma4xx_submit_fetch(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	const struct bma4xx_data *bma4xx = dev->data;
	uint32_t min_buf_len = sizeof(struct bma4xx_encoded_data);
	struct bma4xx_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	uint64_t cycles;
	int rc;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* Prepare response */
	edata = (struct bma4xx_encoded_data *)buf;
	if (IS_ENABLED(CONFIG_BMA4XX_STREAM)) {
		edata->header.is_fifo = false;
	}
	edata->header.accel_fs = bma4xx->cfg.accel_fs_range;

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	rc = bma4xx_get_accel_data(dev, &edata->accel_xyz);
	if (rc != 0) {
		LOG_ERR("Failed to fetch accel samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

#ifdef CONFIG_BMA4XX_TEMPERATURE
	rc = bma4xx_get_temp_data(dev, &edata->temp);
	if (rc != 0) {
		LOG_ERR("Failed to fetch temp sample");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}
#endif /* CONFIG_BMA4XX_TEMPERATURE */

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void bma4xx_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		struct rtio_work_req *req = rtio_work_req_alloc();

		__ASSERT_NO_MSG(req);

		rtio_work_req_submit(req, iodev_sqe, bma4xx_submit_fetch);
	} else if (IS_ENABLED(CONFIG_BMA4XX_STREAM)) {
		bma4xx_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
