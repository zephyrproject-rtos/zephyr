/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT asahi_kasei_ak09940a

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>

#include "ak09940a.h"

LOG_MODULE_DECLARE(AK09940A, CONFIG_SENSOR_LOG_LEVEL);

static void ak09940a_complete_cb(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg0)
{
	const struct device *dev = arg0;
	struct ak09940a_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;
	const struct ak09940a_encoded_data *edata =
		(const struct ak09940a_encoded_data *)iodev_sqe->sqe.rx.buf;
	struct rtio_cqe *cqe;
	int ret = result;

	/* A chain keeps running after a failed entry, so check every completion */
	while ((cqe = rtio_cqe_consume(r)) != NULL) {
		if ((ret == 0) && (cqe->result < 0)) {
			ret = cqe->result;
		}
		rtio_cqe_release(r, cqe);
	}

	if ((ret == 0) && data->async_single &&
	    ((edata->frame[AK09940A_FRAME_ST1] & AK099XX_ST1_DRDY) == 0U)) {
		LOG_DBG("Data not ready, st1=0x%02x", edata->frame[AK09940A_FRAME_ST1]);
		ret = -EBUSY;
	}

	atomic_clear(&data->busy);

	if (ret != 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static int ak09940a_check_channels(const struct sensor_read_config *read_cfg)
{
	for (size_t i = 0; i < read_cfg->count; i++) {
		if (read_cfg->channels[i].chan_idx != 0) {
			return -ENOTSUP;
		}

		switch (read_cfg->channels[i].chan_type) {
		case SENSOR_CHAN_ALL:
		case SENSOR_CHAN_MAGN_X:
		case SENSOR_CHAN_MAGN_Y:
		case SENSOR_CHAN_MAGN_Z:
		case SENSOR_CHAN_MAGN_XYZ:
		case SENSOR_CHAN_DIE_TEMP:
			break;
		default:
			return -ENOTSUP;
		}
	}

	return 0;
}

static int ak09940a_prep_read(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe,
			      struct ak09940a_encoded_data *edata)
{
	const struct ak09940a_config *cfg = dev->config;
	struct ak09940a_data *data = dev->data;
	struct rtio *r = data->rtio_ctx;
	const uint8_t start[] = {AK09940A_REG_CNTL3, cfg->drive->cntl3 | AK099XX_MODE_SINGLE};
	const uint8_t addr = AK09940A_REG_ST1 | (cfg->is_spi ? AK09940A_SPI_READ : 0U);
	struct rtio_sqe *start_sqe = NULL;
	struct rtio_sqe *delay_sqe = NULL;
	struct rtio_sqe *addr_sqe;
	struct rtio_sqe *read_sqe;
	struct rtio_sqe *cb_sqe;

	/* In continuous mode the data registers already hold the latest measurement */
	if (data->async_single) {
		start_sqe = rtio_sqe_acquire(r);
		delay_sqe = rtio_sqe_acquire(r);
		if ((start_sqe == NULL) || (delay_sqe == NULL)) {
			rtio_sqe_drop_all(r);
			return -ENOMEM;
		}

		rtio_sqe_prep_tiny_write(start_sqe, data->iodev, RTIO_PRIO_NORM, start,
					 sizeof(start), NULL);
		if (!cfg->is_spi) {
			start_sqe->iodev_flags = RTIO_IODEV_I2C_STOP;
		}
		start_sqe->flags = RTIO_SQE_CHAINED;

		rtio_sqe_prep_delay(delay_sqe, K_USEC(cfg->drive->measure_time_us), NULL);
		delay_sqe->flags = RTIO_SQE_CHAINED;
	}

	addr_sqe = rtio_sqe_acquire(r);
	read_sqe = rtio_sqe_acquire(r);
	cb_sqe = rtio_sqe_acquire(r);
	if ((addr_sqe == NULL) || (read_sqe == NULL) || (cb_sqe == NULL)) {
		rtio_sqe_drop_all(r);
		return -ENOMEM;
	}

	rtio_sqe_prep_tiny_write(addr_sqe, data->iodev, RTIO_PRIO_NORM, &addr, 1, NULL);
	addr_sqe->flags = RTIO_SQE_TRANSACTION;

	/* Reading up to ST2 releases the data protection */
	rtio_sqe_prep_read(read_sqe, data->iodev, RTIO_PRIO_NORM, edata->frame,
			   sizeof(edata->frame), NULL);
	if (!cfg->is_spi) {
		read_sqe->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}
	read_sqe->flags = RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(cb_sqe, ak09940a_complete_cb, (void *)dev, iodev_sqe);

	return 0;
}

void ak09940a_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	struct ak09940a_data *data = dev->data;
	struct ak09940a_encoded_data *edata;
	uint32_t buf_len;
	uint64_t cycles;
	uint8_t *buf;
	int ret;

	if (read_cfg->is_streaming) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	ret = ak09940a_check_channels(read_cfg);
	if (ret != 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	ret = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata), &buf, &buf_len);
	if (ret != 0) {
		LOG_ERR("Failed to get a read buffer of size %zu bytes", sizeof(*edata));
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}
	edata = (struct ak09940a_encoded_data *)buf;

	ret = sensor_clock_get_cycles(&cycles);
	if (ret != 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}
	edata->timestamp = sensor_clock_cycles_to_ns(cycles);

	if (!atomic_cas(&data->busy, 0, 1)) {
		rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
		return;
	}

	data->async_single = data->mode == AK099XX_MODE_POWER_DOWN;

	ret = ak09940a_prep_read(dev, iodev_sqe, edata);
	if (ret != 0) {
		atomic_clear(&data->busy);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	rtio_submit(data->rtio_ctx, 0);
}
