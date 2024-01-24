/*
 * Copyright (c) 2023 Grinn
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ad559x

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/mfd/ad559x.h>

#include "mfd_ad559x.h"

bool mfd_ad559x_has_pointer_byte_map(const struct device *dev)
{
	const struct mfd_ad559x_config *config = dev->config;

	return config->has_pointer_byte_map;
}

int mfd_ad559x_read_raw(const struct device *dev, uint8_t *val, size_t len)
{
	struct mfd_ad559x_data *data = dev->data;

	return data->transfer_function->read_raw(dev, val, len);
}

int mfd_ad559x_write_raw(const struct device *dev, uint8_t *val, size_t len)
{
	struct mfd_ad559x_data *data = dev->data;

	return data->transfer_function->write_raw(dev, val, len);
}

int mfd_ad559x_read_reg(const struct device *dev, uint8_t reg, uint8_t reg_data, uint16_t *val)
{
	struct mfd_ad559x_data *data = dev->data;

	return data->transfer_function->read_reg(dev, reg, reg_data, val);
}

int mfd_ad559x_write_reg(const struct device *dev, uint8_t reg, uint16_t val)
{
	struct mfd_ad559x_data *data = dev->data;

	return data->transfer_function->write_reg(dev, reg, val);
}

static int mfd_add559x_software_reset(const struct device *dev)
{
	return mfd_ad559x_write_reg(dev, AD559X_REG_SOFTWARE_RESET,
				    AD559X_SOFTWARE_RESET_MAGIC_VAL);
}

static int mfd_ad559x_init(const struct device *dev)
{
	const struct mfd_ad559x_config *config = dev->config;
	int ret;

	ret = config->bus_init(dev);
	if (ret < 0) {
		return ret;
	}

	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = mfd_add559x_software_reset(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#define MDF_AD559X_DEFINE_I2C_BUS(inst)                                                            \
	.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_init = mfd_ad559x_i2c_init,                        \
	.has_pointer_byte_map = true

#define MDF_AD559X_DEFINE_SPI_BUS_FLAGS                                                            \
	(SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER | SPI_MODE_CPOL)

#define MDF_AD559X_DEFINE_SPI_BUS(inst)                                                            \
	.spi = SPI_DT_SPEC_INST_GET(inst, MDF_AD559X_DEFINE_SPI_BUS_FLAGS, 0),                     \
	.bus_init = mfd_ad559x_spi_init, .has_pointer_byte_map = false

#define MFD_AD559X_DEFINE_BUS(inst)                                                                \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c), (MDF_AD559X_DEFINE_I2C_BUS(inst)),                  \
		    (MDF_AD559X_DEFINE_SPI_BUS(inst)))

#define MFD_AD559X_DEFINE(inst)                                                                    \
	static struct mfd_ad559x_data mfd_ad559x_data_##inst;                                      \
	static const struct mfd_ad559x_config mfd_ad559x_config_##inst = {                         \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                            \
		MFD_AD559X_DEFINE_BUS(inst),                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_ad559x_init, NULL, &mfd_ad559x_data_##inst,                \
			      &mfd_ad559x_config_##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,    \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_AD559X_DEFINE);
