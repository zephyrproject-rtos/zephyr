/*
 * Copyright (c) 2025 Nordic Semiconductors ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/logging/log.h>
#include "bme680.h"

LOG_MODULE_DECLARE(BME680, CONFIG_SENSOR_LOG_LEVEL);

static int bme680_bus_check_rtio(const struct bme680_bus *bus)
{
#if CONFIG_I2C_RTIO
	if ((bus->rtio.type == BME680_BUS_TYPE_I2C) && !i2c_is_ready_iodev(bus->rtio.iodev)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}
#endif
#if CONFIG_SPI_RTIO
	if ((bus->rtio.type == BME680_BUS_TYPE_SPI) && !spi_is_ready_iodev(bus->rtio.iodev)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}
#endif
	return 0;
}

static void bme680_set_mem_page_cb(struct rtio *ctx, const struct rtio_sqe *sqe, void *arg0)
{
	const struct device *dev = (const struct device *)arg0;
	struct bme680_data *data = dev->data;
	uint8_t reg_addr_wr;

	struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *write_val_sqe = rtio_sqe_acquire(ctx);

	if (!write_reg_sqe || !write_val_sqe) {
		rtio_sqe_drop_all(ctx);
		return;
	}

	if (data->mem_page == 1U) {
		data->status &= ~BME680_SPI_MEM_PAGE_MSK;
	} else {
		data->status |= BME680_SPI_MEM_PAGE_MSK;
	}

	reg_addr_wr = BME680_REG_STATUS & BME680_SPI_WRITE_MSK;
	rtio_sqe_prep_tiny_write(write_reg_sqe, sqe->iodev, RTIO_PRIO_NORM, &reg_addr_wr, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_CHAINED;

	/* Prepare write value SQE */
	rtio_sqe_prep_tiny_write(write_val_sqe, sqe->iodev, RTIO_PRIO_NORM, &data->status, 1, NULL);
	write_val_sqe->flags |= RTIO_SQE_CHAINED;

	/* Submit the two SQEs */
	rtio_submit(ctx, 2);

	/* Update mem_page */
	data->mem_page = (data->status & BME680_SPI_MEM_PAGE_MSK) ? 1U : 0U;
}

static int bme680_set_mem_page(const struct device *dev,
								uint8_t addr)
{
	const struct bme680_bus *bus = &((const struct bme680_config *)dev->config)->bus;
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_iodev *iodev = bus->rtio.iodev;
	struct bme680_data *data = dev->data;
	uint8_t page = (addr > 0x7f) ? 0U : 1U;
	uint8_t reg_addr;

	if (data->mem_page == page) {
		LOG_DBG("No page switch needed");
		return 0;
	}

	struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *read_status_sqe = rtio_sqe_acquire(ctx);

	if (!write_reg_sqe || !read_status_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	reg_addr = BME680_REG_STATUS | BME680_SPI_READ_BIT;
	rtio_sqe_prep_tiny_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, &reg_addr, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_status_sqe, iodev, RTIO_PRIO_NORM, &data->status, 1, NULL);
	read_status_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(read_status_sqe, bme680_set_mem_page_cb, (void *)dev, NULL);

	rtio_submit(ctx, 2);

	return 0;
}

static int bme680_prep_reg_read_rtio_async(const struct device *dev,
					   uint8_t reg, uint8_t *buf, size_t size,
					   struct rtio_sqe **out)
{
	const struct bme680_bus *bus = &((const struct bme680_config *)dev->config)->bus;
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_iodev *iodev = bus->rtio.iodev;

	struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *read_buf_sqe = rtio_sqe_acquire(ctx);

	if (!write_reg_sqe || !read_buf_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	if ((bus->rtio.type == BME680_BUS_TYPE_SPI) && (bme680_set_mem_page(dev, reg) < 0)) {
		return -EIO;
	}

	rtio_sqe_prep_tiny_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_buf_sqe, iodev, RTIO_PRIO_NORM, buf, size, NULL);
	if (bus->rtio.type == BME680_BUS_TYPE_I2C) {
		read_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}

	/* Send back last SQE so it can be concatenated later. */
	if (out) {
		*out = read_buf_sqe;
	}

	return 2;
}

static int bme680_prep_reg_write_rtio_async(const struct device *dev,
						uint8_t reg, uint8_t val,
						struct rtio_sqe **out)
{
	const struct bme680_bus *bus = &((const struct bme680_config *)dev->config)->bus;
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_iodev *iodev = bus->rtio.iodev;
	uint8_t buf[2];

	struct rtio_sqe *write_sqe = rtio_sqe_acquire(ctx);

	if (!write_sqe) {
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	if ((bus->rtio.type == BME680_BUS_TYPE_SPI) && (bme680_set_mem_page(dev, reg) < 0)) {
		return -EIO;
	}

	buf[0] = reg;
	buf[1] = val;
	rtio_sqe_prep_tiny_write(write_sqe, iodev, RTIO_PRIO_NORM, buf, sizeof(buf), NULL);
	if (bus->rtio.type == BME680_BUS_TYPE_I2C) {
		write_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
	}

	/* Send back last SQE so it can be concatenated later. */
	if (out) {
		*out = write_sqe;
	}

	return 1;
}

static int bme680_reg_read_rtio(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	struct rtio *ctx = ((const struct bme680_config *)dev->config)->bus.rtio.ctx;
	struct rtio_cqe *cqe;
	int ret;

	ret = bme680_prep_reg_read_rtio_async(dev, start, buf, size, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = rtio_submit(ctx, ret);
	if (ret) {
		return ret;
	}

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			ret = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	return ret;
}

static int bme680_reg_write_rtio(const struct device *dev, uint8_t reg, uint8_t val)
{
	struct rtio *ctx = ((const struct bme680_config *)dev->config)->bus.rtio.ctx;
	struct rtio_cqe *cqe;
	int ret;

	ret = bme680_prep_reg_write_rtio_async(dev, reg, val, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = rtio_submit(ctx, ret);
	if (ret) {
		return ret;
	}

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			ret = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	return ret;
}

const struct bme680_bus_io bme680_bus_rtio = {
	.check = bme680_bus_check_rtio,
	.read = bme680_reg_read_rtio,
	.write = bme680_reg_write_rtio,
	.read_async_prep = bme680_prep_reg_read_rtio_async,
	.write_async_prep = bme680_prep_reg_write_rtio_async,
};
