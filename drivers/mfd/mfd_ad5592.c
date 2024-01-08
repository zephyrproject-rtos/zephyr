/*
 * Copyright (c) 2023 Grinn
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ad5592

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/mfd/ad5592.h>

#define AD5592_GPIO_READBACK_EN BIT(10)
#define AD5592_LDAC_READBACK_EN BIT(6)
#define AD5592_REG_SOFTWARE_RESET 0x0FU
#define AD5592_SOFTWARE_RESET_MAGIC_VAL 0x5AC
#define AD5592_REG_VAL_MASK 0x3FF
#define AD5592_REG_RESET_VAL_MASK 0x7FF
#define AD5592_REG_SHIFT_VAL 11
#define AD5592_REG_READBACK_SHIFT_VAL 2

#define AD5592_SPI_SPEC_CONF (SPI_WORD_SET(8) | SPI_TRANSFER_MSB | \
			      SPI_OP_MODE_MASTER | SPI_MODE_CPOL)

struct mfd_ad5592_config {
	struct gpio_dt_spec reset_gpio;
	struct spi_dt_spec bus;
};

int mfd_ad5592_read_raw(const struct device *dev, uint16_t *val)
{
	const struct mfd_ad5592_config *config = dev->config;
	uint16_t nop_msg = 0;

	struct spi_buf tx_buf[] = {
		{
			.buf = &nop_msg,
			.len = sizeof(nop_msg)
		}
	};

	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 1
	};

	struct spi_buf rx_buf[] = {
		{
			.buf = val,
			.len = sizeof(uint16_t)
		}
	};

	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 1
	};

	return spi_transceive_dt(&config->bus, &tx, &rx);
}

int mfd_ad5592_write_raw(const struct device *dev, uint16_t val)
{
	const struct mfd_ad5592_config *config = dev->config;

	struct spi_buf tx_buf[] = {
		{
			.buf = &val,
			.len = sizeof(val)
		}
	};

	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 1
	};

	return spi_write_dt(&config->bus, &tx);
}

int mfd_ad5592_read_reg(const struct device *dev, uint8_t reg, uint8_t reg_data, uint16_t *val)
{
	uint16_t data;
	uint16_t msg;
	int ret;

	switch (reg) {
	case AD5592_REG_GPIO_INPUT_EN:
		msg = sys_cpu_to_be16(AD5592_GPIO_READBACK_EN |
				      (AD5592_REG_GPIO_INPUT_EN << AD5592_REG_SHIFT_VAL) |
				      reg_data);
		break;
	default:
		msg = sys_cpu_to_be16(AD5592_LDAC_READBACK_EN |
				      (AD5592_REG_READ_AND_LDAC << AD5592_REG_SHIFT_VAL) |
				      reg << AD5592_REG_READBACK_SHIFT_VAL);
		break;
	}

	ret = mfd_ad5592_write_raw(dev, msg);
	if (ret < 0) {
		return ret;
	}

	ret = mfd_ad5592_read_raw(dev, &data);
	if (ret < 0) {
		return ret;
	}

	*val = sys_be16_to_cpu(data);

	return 0;
}

int mfd_ad5592_write_reg(const struct device *dev, uint8_t reg, uint16_t val)
{
	uint16_t write_mask;
	uint16_t msg;

	switch (reg) {
	case AD5592_REG_SOFTWARE_RESET:
		write_mask = AD5592_REG_RESET_VAL_MASK;
		break;
	default:
		write_mask = AD5592_REG_VAL_MASK;
		break;
	}

	msg = sys_cpu_to_be16((reg << AD5592_REG_SHIFT_VAL) | (val & write_mask));

	return mfd_ad5592_write_raw(dev, msg);
}

static int mfd_add592_software_reset(const struct device *dev)
{
	return mfd_ad5592_write_reg(dev,
				    AD5592_REG_SOFTWARE_RESET,
				    AD5592_SOFTWARE_RESET_MAGIC_VAL);
}

static int mfd_ad5592_init(const struct device *dev)
{
	const struct mfd_ad5592_config *config = dev->config;
	int ret;

	if (!spi_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = mfd_add592_software_reset(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#define MFD_AD5592_DEFINE(inst)							\
	static const struct mfd_ad5592_config mfd_ad5592_config_##inst = {	\
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),		\
		.bus = SPI_DT_SPEC_INST_GET(inst, AD5592_SPI_SPEC_CONF, 0),	\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, mfd_ad5592_init, NULL,			\
			      NULL,						\
			      &mfd_ad5592_config_##inst,			\
			      POST_KERNEL,					\
			      CONFIG_MFD_INIT_PRIORITY,				\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_AD5592_DEFINE);
