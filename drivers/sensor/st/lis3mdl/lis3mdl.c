/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis3mdl_magn

#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include "lis3mdl.h"

#define LIS3MDL_SPI_READ_BIT (1 << 7)
#define LIS3MDL_SPI_MS_BIT   (1 << 6)

LOG_MODULE_REGISTER(LIS3MDL, CONFIG_SENSOR_LOG_LEVEL);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
static int lis3dml_write_spi(const struct device *dev, const uint8_t address, uint8_t *buff,
			     uint8_t len)
{
	const struct lis3mdl_config *config = dev->config;

	// Make sure address is only 6 bits and set MSB of address to write (0)
	uint8_t address_byte = (address & (LIS3MDL_SPI_MS_BIT - 1));

	// Set MSB-1 to multi-byte write if more than 1 byte is to be written
	if (len > 1) {
		address_byte |= LIS3MDL_SPI_MS_BIT;
	}

	// Create spi buffers (2 tx buffers: 1 for address, 1 for data)
	struct spi_buf spi_tx[2] = {{
					    .buf = &address_byte,
					    .len = sizeof(address_byte),
				    },
				    {
					    .buf = buff,
					    .len = len,
				    }};

	// Create spi buffer sets
	struct spi_buf_set tx_set = {
		.buffers = spi_tx,
		.count = 2,
	};
	// No rx buffer needed for write operation
	struct spi_buf_set rx_set = {
		.buffers = NULL,
		.count = 0,
	};

	return spi_transceive_dt(&config->spi, &tx_set, &rx_set);
}

static int lis3dml_read_spi(const struct device *dev, const uint8_t address, uint8_t *buff,
			    uint8_t len)
{
	const struct lis3mdl_config *config = dev->config;
	uint8_t dummy_byte = 0x00;

	// Make sure address is only 6 bits and set MSB of address to read (1)
	uint8_t address_byte = (address & (LIS3MDL_SPI_MS_BIT - 1)) | LIS3MDL_SPI_READ_BIT;

	// Set MSB-1 to multi-byte read if more than 1 byte is to be read
	if (len > 1) {
		address_byte |= LIS3MDL_SPI_MS_BIT;
	}

	// Create spi buffers
	struct spi_buf spi_tx = {
		.buf = &address_byte,
		.len = sizeof(address_byte),
	};

	// 2 rx buffers: 1 for reading the garbage byte during address write, 1 for the actual data
	struct spi_buf spi_rx[2] = {{
					    .buf = &dummy_byte,
					    .len = sizeof(dummy_byte),
				    },
				    {
					    .buf = buff,
					    .len = len,
				    }};

	// Create spi buffer sets
	struct spi_buf_set tx_set = {
		.buffers = &spi_tx,
		.count = 1,
	};
	struct spi_buf_set rx_set = {
		.buffers = spi_rx,
		.count = 2,
	};

	return spi_transceive_dt(&config->spi, &tx_set, &rx_set);
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
static int lis3dml_write_i2c(const struct device *dev, const uint8_t address, uint8_t *buff,
			     uint8_t len)
{
	const struct lis3mdl_config *config = dev->config;
	return i2c_burst_write_dt(&config->i2c, address, buff, len);
}

static int lis3dml_read_i2c(const struct device *dev, const uint8_t address, uint8_t *buff,
			    uint8_t len)
{
	const struct lis3mdl_config *config = dev->config;
	return i2c_burst_read_dt(&config->i2c, address, buff, len);
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

static void lis3mdl_convert(struct sensor_value *val, int16_t raw_val, uint16_t divider)
{
	/* val = raw_val / divider */
	val->val1 = raw_val / divider;
	val->val2 = (((int64_t)raw_val % divider) * 1000000L) / divider;
}

static int lis3mdl_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lis3mdl_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_MAGN_XYZ) {
		/* magn_val = sample / magn_gain */
		lis3mdl_convert(val, drv_data->x_sample, lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
		lis3mdl_convert(val + 1, drv_data->y_sample, lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
		lis3mdl_convert(val + 2, drv_data->z_sample, lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	} else if (chan == SENSOR_CHAN_MAGN_X) {
		lis3mdl_convert(val, drv_data->x_sample, lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	} else if (chan == SENSOR_CHAN_MAGN_Y) {
		lis3mdl_convert(val, drv_data->y_sample, lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	} else if (chan == SENSOR_CHAN_MAGN_Z) {
		lis3mdl_convert(val, drv_data->z_sample, lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	} else if (chan == SENSOR_CHAN_DIE_TEMP) {
		/* temp_val = 25 + sample / 8 */
		lis3mdl_convert(val, drv_data->temp_sample, 8);
		val->val1 += 25;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int lis3mdl_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct lis3mdl_data *drv_data = dev->data;
	const struct lis3mdl_config *config = dev->config;
	int16_t buf[4];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_MAGN_XYZ);

	/* fetch magnetometer sample */
	if (config->read(dev, LIS3MDL_REG_SAMPLE_START, (uint8_t *)buf, 8) < 0) {
		LOG_DBG("Failed to fetch magnetometer sample.");
		return -EIO;
	}

	/*
	 * the chip doesn't allow fetching temperature data in
	 * the same read as magnetometer data, so do another
	 * burst read to fetch the temperature sample
	 */
	if (config->read(dev, LIS3MDL_REG_SAMPLE_START + 6, (uint8_t *)(buf + 3), 2) < 0) {
		LOG_DBG("Failed to fetch temperature sample.");
		return -EIO;
	}

	drv_data->x_sample = sys_le16_to_cpu(buf[0]);
	drv_data->y_sample = sys_le16_to_cpu(buf[1]);
	drv_data->z_sample = sys_le16_to_cpu(buf[2]);
	drv_data->temp_sample = sys_le16_to_cpu(buf[3]);

	return 0;
}

static DEVICE_API(sensor, lis3mdl_driver_api) = {
#if CONFIG_LIS3MDL_TRIGGER
	.trigger_set = lis3mdl_trigger_set,
#endif
	.sample_fetch = lis3mdl_sample_fetch,
	.channel_get = lis3mdl_channel_get,
};

int lis3mdl_init(const struct device *dev)
{
	const struct lis3mdl_config *config = dev->config;
	uint8_t chip_cfg[5];
	uint8_t id, idx;

	/* check chip ID */
	if (config->read(dev, LIS3MDL_REG_WHO_AM_I, &id, 1) < 0) {
		LOG_ERR("Failed to read chip ID.");
		return -EIO;
	}

	if (id != LIS3MDL_CHIP_ID) {
		LOG_ERR("Invalid chip ID.");
		return -EINVAL;
	}

	/* check if CONFIG_LIS3MDL_ODR is valid */
	for (idx = 0U; idx < ARRAY_SIZE(lis3mdl_odr_strings); idx++) {
		if (!strcmp(lis3mdl_odr_strings[idx], CONFIG_LIS3MDL_ODR)) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(lis3mdl_odr_strings)) {
		LOG_ERR("Invalid ODR value.");
		return -EINVAL;
	}

	/* Configure sensor */
	chip_cfg[0] = LIS3MDL_TEMP_EN_MASK | lis3mdl_odr_bits[idx];
	chip_cfg[1] = LIS3MDL_FS_IDX << LIS3MDL_FS_SHIFT;
	chip_cfg[2] = LIS3MDL_MD_CONTINUOUS;
	chip_cfg[3] = ((lis3mdl_odr_bits[idx] & LIS3MDL_OM_MASK) >> LIS3MDL_OM_SHIFT)
		      << LIS3MDL_OMZ_SHIFT;
	chip_cfg[4] = LIS3MDL_BDU_EN;

	if (config->write(dev, LIS3MDL_REG_CTRL1, chip_cfg, 5) < 0) {
		LOG_DBG("Failed to configure chip.");
		return -EIO;
	}

#ifdef CONFIG_LIS3MDL_TRIGGER
	if (config->irq_gpio.port) {
		if (lis3mdl_init_interrupt(dev) < 0) {
			LOG_DBG("Failed to initialize interrupts.");
			return -EIO;
		}
	}
#endif

	return 0;
}

/*
 * Instantiation macros used when a device is on an SPI bus.
 */

#define LIS3MDL_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define LIS3MDL_CONFIG_SPI(inst)                                                                   \
	static struct lis3mdl_data lis3mdl_data_##inst;                                            \
                                                                                                   \
	static struct lis3mdl_config lis3mdl_config_##inst = {                                     \
		.spi = SPI_DT_SPEC_INST_GET(inst, LIS3MDL_SPI_OPERATION, 0),                       \
		.write = lis3dml_write_spi,                                                        \
		.read = lis3dml_read_spi,                                                          \
		IF_ENABLED(CONFIG_LIS3MDL_TRIGGER,                                                 \
			   (.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}), ))};            \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lis3mdl_init, NULL, &lis3mdl_data_##inst,               \
				     &lis3mdl_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &lis3mdl_driver_api);

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LIS3MDL_CONFIG_I2C(inst)                                                                   \
	static struct lis3mdl_data lis3mdl_data_##inst;                                            \
                                                                                                   \
	static struct lis3mdl_config lis3mdl_config_##inst = {                                     \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.write = lis3dml_write_i2c,                                                        \
		.read = lis3dml_read_i2c,                                                          \
		IF_ENABLED(CONFIG_LIS3MDL_TRIGGER,                                                 \
			   (.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}), ))};            \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lis3mdl_init, NULL, &lis3mdl_data_##inst,               \
				     &lis3mdl_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &lis3mdl_driver_api);

#define LIS3MDL_DEFINE(inst)                                                                       \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), (LIS3MDL_CONFIG_SPI(inst)),                         \
		    (LIS3MDL_CONFIG_I2C(inst)));

DT_INST_FOREACH_STATUS_OKAY(LIS3MDL_DEFINE)
