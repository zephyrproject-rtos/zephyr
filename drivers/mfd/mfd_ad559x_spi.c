/*
 * Copyright (c) 2023 Grinn
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>

#include <zephyr/drivers/mfd/ad559x.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>

#include "mfd_ad559x.h"

static int mfd_ad559x_spi_read_raw(const struct device *dev, uint8_t *val, size_t len)
{
	const struct mfd_ad559x_config *config = dev->config;
	uint16_t nop_msg = 0;

	struct spi_buf tx_buf[] = {{.buf = &nop_msg, .len = sizeof(nop_msg)}};

	const struct spi_buf_set tx = {.buffers = tx_buf, .count = 1};

	struct spi_buf rx_buf[] = {{.buf = val, .len = len}};

	const struct spi_buf_set rx = {.buffers = rx_buf, .count = 1};

	return spi_transceive_dt(&config->spi, &tx, &rx);
}

static int mfd_ad559x_spi_write_raw(const struct device *dev, uint8_t *val, size_t len)
{
	const struct mfd_ad559x_config *config = dev->config;

	struct spi_buf tx_buf[] = {{.buf = val, .len = len}};

	const struct spi_buf_set tx = {.buffers = tx_buf, .count = 1};

	return spi_write_dt(&config->spi, &tx);
}

static int mfd_ad559x_spi_read_reg(const struct device *dev, uint8_t reg, uint8_t reg_data,
				   uint16_t *val)
{
	uint16_t data;
	uint16_t msg;
	int ret;

	switch (reg) {
	case AD559X_REG_GPIO_INPUT_EN:
		msg = sys_cpu_to_be16(AD559X_GPIO_READBACK_EN |
				      (AD559X_REG_GPIO_INPUT_EN << AD559X_REG_SHIFT_VAL) |
				      reg_data);
		break;
	default:
		msg = sys_cpu_to_be16(AD559X_LDAC_READBACK_EN |
				      (AD559X_REG_READ_AND_LDAC << AD559X_REG_SHIFT_VAL) |
				      reg << AD559X_REG_READBACK_SHIFT_VAL);
		break;
	}

	ret = mfd_ad559x_spi_write_raw(dev, (uint8_t *)&msg, sizeof(msg));
	if (ret < 0) {
		return ret;
	}

	ret = mfd_ad559x_spi_read_raw(dev, (uint8_t *)&data, sizeof(data));
	if (ret < 0) {
		return ret;
	}

	*val = sys_be16_to_cpu(data);

	return 0;
}

static int mfd_ad559x_spi_write_reg(const struct device *dev, uint8_t reg, uint16_t val)
{
	uint16_t write_mask;
	uint16_t msg;

	switch (reg) {
	case AD559X_REG_SOFTWARE_RESET:
		write_mask = AD559X_REG_RESET_VAL_MASK;
		break;
	default:
		write_mask = AD559X_REG_VAL_MASK;
		break;
	}

	msg = sys_cpu_to_be16((reg << AD559X_REG_SHIFT_VAL) | (val & write_mask));

	return mfd_ad559x_spi_write_raw(dev, (uint8_t *)&msg, sizeof(msg));
}

static const struct mfd_ad559x_transfer_function mfd_ad559x_spi_transfer_function = {
	.read_raw = mfd_ad559x_spi_read_raw,
	.write_raw = mfd_ad559x_spi_write_raw,
	.read_reg = mfd_ad559x_spi_read_reg,
	.write_reg = mfd_ad559x_spi_write_reg,
};

int mfd_ad559x_spi_init(const struct device *dev)
{
	const struct mfd_ad559x_config *config = dev->config;
	struct mfd_ad559x_data *data = dev->data;

	data->transfer_function = &mfd_ad559x_spi_transfer_function;

	if (!spi_is_ready_dt(&config->spi)) {
		return -ENODEV;
	}

	return 0;
}
