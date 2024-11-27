/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Fujitsu MB85RSXX FRAM over SPI.
 */

#define DT_DRV_COMPAT fujitsu_mb85rsxx

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mb85rsxx, CONFIG_EEPROM_LOG_LEVEL);

/* MB85RSXX instruction set */
#define EEPROM_MB85RSXX_WREN  0x06U /* Set Write Enable Latch	 */
#define EEPROM_MB85RSXX_WRDI  0x04U /* Reset Write Enable Latch */
#define EEPROM_MB85RSXX_RDSR  0x05U /* Read Status Register     */
#define EEPROM_MB85RSXX_WRSR  0x01U /* Write Status Register    */
#define EEPROM_MB85RSXX_READ  0x03U /* Read Memory Code		 */
#define EEPROM_MB85RSXX_WRITE 0x02U /* Write Memory Code		 */
#define EEPROM_MB85RSXX_RDID  0x9FU /* Read Device ID			 */
#define EEPROM_MB85RSXX_FSTRD 0x0BU /* Fast Read Memory Code	 */
#define EEPROM_MB85RSXX_SLEEP 0xB9U /* Sleep Mode				 */

/* MB85RSXX status register bits */
#define EEPROM_MB85RSXX_STATUS_WPEN BIT(7) /* Status Register Write Protect  (RW) */
#define EEPROM_MB85RSXX_STATUS_BP1  BIT(3) /* Block protection 1			  (RW) */
#define EEPROM_MB85RSXX_STATUS_BP0  BIT(2) /* Block protection 2			  (RW) */
#define EEPROM_MB85RSXX_STATUS_WEL  BIT(1) /* Write Enable Latch			  (RO) */

/* Fujitsu manufacturer ID (2 bytes) */
#define EEPROM_MB85RSXX_MAN_ID		0x04U
#define EEPROM_MB85RSXX_CON_CODE	0x7FU

/*
 * MB85RSXX product ID (2 bytes); first byte provides memory size, so let's use a mask later when
 * checking it
 */
#define EEPROM_MB85RSXX_PROD_ID		0x20U
#define EEPROM_MB85RSXX_PROD_ID2	0x03U
#define EEPROM_MB85RSXX_PROD_MASK	GENMASK(7, 5)

struct eeprom_mb85rsxx_config {
	struct spi_dt_spec spi;
	size_t size;
	bool readonly;
};

struct eeprom_mb85rsxx_data {
	struct k_mutex lock;
};

static int eeprom_mb85rsxx_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct eeprom_mb85rsxx_config *config = dev->config;
	struct eeprom_mb85rsxx_data *data = dev->data;
	uint8_t cmd[4] = {EEPROM_MB85RSXX_READ, 0, 0, 0};
	uint8_t *paddr = &cmd[1];
	int err;

	if (offset + len > config->size) {
		LOG_ERR("attempt to read past device boundary");
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	/* Populate address in command */
	*paddr++ = (offset >> 16);
	*paddr++ = (offset >> 8);
	*paddr++ = offset;

	const struct spi_buf tx_buf = {
		.buf = cmd,
		.len = sizeof(cmd),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_bufs[2] = {
		{
			.buf = NULL,
			.len = sizeof(cmd),
		},
		{
			.buf = buf,
			.len = len,
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};

	k_mutex_lock(&data->lock, K_FOREVER);

	err = spi_transceive_dt(&config->spi, &tx, &rx);

	k_mutex_unlock(&data->lock);

	if (err < 0) {
		LOG_ERR("failed to read FRAM (err %d)", err);
	}

	return err;
}

static int eeprom_mb85rsxx_wren(const struct device *dev)
{
	const struct eeprom_mb85rsxx_config *config = dev->config;
	uint8_t cmd = EEPROM_MB85RSXX_WREN;
	const struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = sizeof(cmd),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	return spi_write_dt(&config->spi, &tx);
}

static int eeprom_mb85rsxx_wrdi(const struct device *dev)
{
	const struct eeprom_mb85rsxx_config *config = dev->config;
	uint8_t cmd = EEPROM_MB85RSXX_WRDI;
	const struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = sizeof(cmd),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	return spi_write_dt(&config->spi, &tx);
}

static int eeprom_mb85rsxx_write(const struct device *dev, off_t offset, const void *buf,
				  size_t len)
{
	const struct eeprom_mb85rsxx_config *config = dev->config;
	struct eeprom_mb85rsxx_data *data = dev->data;
	uint8_t cmd[4] = {EEPROM_MB85RSXX_WRITE, 0, 0, 0};
	uint8_t *paddr = &cmd[1];
	int err;

	if (config->readonly) {
		LOG_ERR("attempt to write to read-only device");
		return -EACCES;
	}

	if (offset + len > config->size) {
		LOG_ERR("attempt to write past device boundary");
		return -EINVAL;
	}

	/* Populate address in command */
	*paddr++ = (offset >> 16) & 0xFF;
	*paddr++ = (offset >> 8) & 0xFF;
	*paddr++ = offset & 0xFF;

	const struct spi_buf tx_bufs[2] = {
		{
			.buf = cmd,
			.len = sizeof(cmd),
		},
		{
			.buf = (void *)buf,
			.len = len,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	k_mutex_lock(&data->lock, K_FOREVER);

	err = eeprom_mb85rsxx_wren(dev);
	if (err < 0) {
		LOG_ERR("failed to disable write protection (err %d)", err);
		k_mutex_unlock(&data->lock);
		return err;
	}

	err = spi_write_dt(&config->spi, &tx);
	if (err < 0) {
		LOG_ERR("failed to write to FRAM (err %d)", err);
		k_mutex_unlock(&data->lock);
		return err;
	}

	err = eeprom_mb85rsxx_wrdi(dev);
	if (err < 0) {
		LOG_ERR("failed to disable write (err %d)", err);
	}

	k_mutex_unlock(&data->lock);

	return err;
}

static size_t eeprom_mb85rsxx_size(const struct device *dev)
{
	const struct eeprom_mb85rsxx_config *config = dev->config;

	return config->size;
}

static int eeprom_mb85rsxx_rdid(const struct device *dev)
{
	const struct eeprom_mb85rsxx_config *config = dev->config;
	struct eeprom_mb85rsxx_data *data = dev->data;
	uint8_t id[4];
	uint8_t cmd = EEPROM_MB85RSXX_RDID;
	int err;

	const struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = sizeof(cmd),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_bufs[2] = {
		{
			.buf = NULL,
			.len = sizeof(cmd),
		},
		{
			.buf = id,
			.len = sizeof(id),
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};
	k_mutex_lock(&data->lock, K_FOREVER);
	err = spi_transceive_dt(&config->spi, &tx, &rx);
	k_mutex_unlock(&data->lock);

	if (err < 0) {
		LOG_ERR("failed to read RDID (err %d)", err);
		return err;
	}

	/* Validate Manufacturer ID and Product ID */
	if (id[0] != EEPROM_MB85RSXX_MAN_ID
		|| id[1] != EEPROM_MB85RSXX_CON_CODE
		|| (id[2] & EEPROM_MB85RSXX_PROD_MASK) != EEPROM_MB85RSXX_PROD_ID
		|| id[3] != EEPROM_MB85RSXX_PROD_ID2) {
		LOG_ERR("invalid device ID: %02X %02X %02X %02X", id[0], id[1], id[2], id[3]);
		return -EIO;
	}

	LOG_INF("device ID read successfully: %02X %02X %02X %02X", id[0], id[1], id[2], id[3]);
	return 0;
}

static int eeprom_mb85rsxx_init(const struct device *dev)
{
	const struct eeprom_mb85rsxx_config *config = dev->config;
	struct eeprom_mb85rsxx_data *data = dev->data;
	int err;

	k_mutex_init(&data->lock);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -EINVAL;
	}

	err = eeprom_mb85rsxx_rdid(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize device, RDID check failed (err %d)", err);
		return err;
	}

	return 0;
}

static DEVICE_API(eeprom, mb85rsxx_driver_api) = {
	.read = &eeprom_mb85rsxx_read,
	.write = &eeprom_mb85rsxx_write,
	.size = &eeprom_mb85rsxx_size,
};

#define MB85RSXX_INIT(inst)                                                                       \
	static struct eeprom_mb85rsxx_data eeprom_mb85rsxx_data_##inst;                          \
                                                                                                   \
	static const struct eeprom_mb85rsxx_config eeprom_mb85rsxx_config_##inst = {             \
		.spi = SPI_DT_SPEC_INST_GET(                                                       \
			inst, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),         \
		.size = DT_INST_PROP(inst, size),                                                  \
		.readonly = DT_INST_PROP(inst, read_only),                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, eeprom_mb85rsxx_init, NULL, &eeprom_mb85rsxx_data_##inst,    \
			      &eeprom_mb85rsxx_config_##inst, POST_KERNEL,                        \
			      CONFIG_EEPROM_INIT_PRIORITY, &mb85rsxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MB85RSXX_INIT)
