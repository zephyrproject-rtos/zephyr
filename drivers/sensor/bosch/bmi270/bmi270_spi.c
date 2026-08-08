/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMI270s accessed via SPI.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "bmi270.h"

LOG_MODULE_DECLARE(bmi270, CONFIG_SENSOR_LOG_LEVEL);

static int bmi270_bus_check_spi(const union bmi270_bus *bus)
{
	if (!spi_is_ready_dt(&bus->spi)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}
	return 0;
}

static int bmi270_reg_read_spi(const union bmi270_bus *bus,
			       uint8_t start, uint8_t *data, uint16_t len)
{
	int ret;
	uint8_t addr;
	uint8_t tmp[2];
	const struct spi_buf tx_buf = {
		.buf = &addr,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf[2];
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf)
	};

	/* First byte we read should be discarded. */
	rx_buf[0].buf = &tmp;
	rx_buf[0].len = 2;
	rx_buf[1].len = len;
	rx_buf[1].buf = data;

	addr = start | 0x80;

	ret = spi_transceive_dt(&bus->spi, &tx, &rx);
	if (ret < 0) {
		LOG_DBG("spi_transceive failed %i", ret);
		return ret;
	}

	k_usleep(BMI270_SPI_ACC_DELAY_US);
	return 0;
}

static int bmi270_reg_write_spi(const union bmi270_bus *bus, uint8_t start,
				const uint8_t *data, uint16_t len)
{
	int ret;
	uint8_t addr;
	const struct spi_buf tx_buf[2] = {
		{.buf = &addr, .len = sizeof(addr)},
		{.buf = (uint8_t *)data, .len = len}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf)
	};

	addr = start & BMI270_REG_MASK;

	ret = spi_write_dt(&bus->spi, &tx);
	if (ret < 0) {
		LOG_ERR("spi_write_dt failed %i", ret);
		return ret;
	}

	k_usleep(BMI270_SPI_ACC_DELAY_US);
	return 0;
}

static int bmi270_bus_init_spi(const union bmi270_bus *bus)
{
	uint8_t tmp;

	/* Single read of SPI initializes the chip to SPI mode
	 */
	return bmi270_reg_read_spi(bus, BMI270_REG_CHIP_ID, &tmp, 1);
}

const struct bmi270_bus_io bmi270_bus_io_spi = {
	.check = bmi270_bus_check_spi,
	.read = bmi270_reg_read_spi,
	.write = bmi270_reg_write_spi,
	.init = bmi270_bus_init_spi,
};

#if defined(CONFIG_BMI270_STREAM)
int bmi270_spi_prep_reg_read_async(const struct device *dev, uint8_t reg, uint8_t *buf,
				   size_t len, uint8_t flags)
{
	struct bmi270_data *data = dev->data;
	struct rtio_sqe *sqes[3];
	struct rtio_sqe *write_reg_sqe;
	struct rtio_sqe *dummy_sqe;
	struct rtio_sqe *read_buf_sqe;
	uint8_t addr = reg | 0x80;

	if (rtio_sqe_acquire_array(data->rtio_ctx, ARRAY_SIZE(sqes), sqes) != 0) {
		return -ENOMEM;
	}

	write_reg_sqe = sqes[0];
	dummy_sqe = sqes[1];
	read_buf_sqe = sqes[2];

	rtio_sqe_prep_tiny_write(write_reg_sqe, data->iodev, RTIO_PRIO_HIGH, &addr, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(dummy_sqe, data->iodev, RTIO_PRIO_HIGH, &data->spi_dummy_byte, 1,
			   NULL);
	/* Keep the dummy byte in the same SPI transaction; the payload read carries chaining. */
	dummy_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_buf_sqe, data->iodev, RTIO_PRIO_HIGH, buf, len, NULL);
	read_buf_sqe->flags |= flags;

	return 3;
}

int bmi270_spi_prep_reg_write_async(const struct device *dev, uint8_t reg, const uint8_t *buf,
				    size_t len, uint8_t flags)
{
	struct bmi270_data *data = dev->data;
	struct rtio_sqe *sqes[2];
	struct rtio_sqe *write_reg_sqe;
	struct rtio_sqe *write_buf_sqe;
	uint8_t addr = reg & BMI270_REG_MASK;

	if (rtio_sqe_acquire_array(data->rtio_ctx, ARRAY_SIZE(sqes), sqes) != 0) {
		return -ENOMEM;
	}

	write_reg_sqe = sqes[0];
	write_buf_sqe = sqes[1];

	rtio_sqe_prep_tiny_write(write_reg_sqe, data->iodev, RTIO_PRIO_HIGH, &addr, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_write(write_buf_sqe, data->iodev, RTIO_PRIO_HIGH, buf, len, NULL);
	write_buf_sqe->flags |= flags;

	return 2;
}
#endif
