/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#include "zephyr/kernel.h"
#include "zephyr/sys/byteorder.h"
#include "zephyr/sys/util.h"
#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT netsol_s3axx04

LOG_MODULE_REGISTER(s3axx04, CONFIG_EEPROM_LOG_LEVEL);

/* Commands */
#define S3AXX04_WREN	0x06
#define S3AXX04_WRDI	0x04
#define S3AXX04_SPIE	0xFF
#define S3AXX04_QPIE	0x38
#define S3AXX04_DPIE	0x37
#define S3AXX04_DPDE	0xB9
#define S3AXX04_DPDX	0xAB
#define S3AXX04_SRTE	0x66
#define S3AXX04_SRST	0x99
#define S3AXX04_RDSR	0x05
#define S3AXX04_WRSR	0x01
#define S3AXX04_RDCX	0x46
#define S3AXX04_WRCX	0x87
#define S3AXX04_RDID	0x9F
#define S3AXX04_RUID	0x4C
#define S3AXX04_READ	0x03
#define S3AXX04_WRITE	0x02

#define S3AXX04_RW_LEN	4

#define S3AXX04_ID_MFG		0xd9
#define S3AXX04_ID_MFG_POS	0

#define S3AXX04_REG4_WRENS	GENMASK(1, 0)
#define S3AXX04_REG4_WRENS_EN	0x0
#define S3AXX04_REG4_WRENS_DIS	0x1
#define S3AXX04_REG4_WRENS_BTB	0x2

#define S3AXX04_RESET_MS	4

struct s3axx04_config {
	struct spi_dt_spec spi;
	size_t size;
	bool read_only;
	bool disable_wren;
};

struct s3axx04_data {
	struct k_sem lock;
};

static int eeprom_s3axx04_set_enable_write(const struct device *dev, bool enable_writes)
{
	const struct s3axx04_config *config = dev->config;
	uint8_t op = enable_writes ? S3AXX04_WREN : S3AXX04_WRDI;
	int ret;

	const struct spi_buf tx_bufs[] = {{
		.buf = &op,
		.len = sizeof(op),
	}};
	const struct spi_buf_set tx_buf_set = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	ret = spi_write_dt(&config->spi, &tx_buf_set);
	if (ret < 0) {
		LOG_ERR("Failed to %s writes", enable_writes ? "enable" : "disable");
		return ret;
	}

	return 0;
}

static int eeprom_s3axx04_soft_reset(const struct device *dev)
{
	const struct s3axx04_config *config = dev->config;
	uint8_t op[2] = { S3AXX04_SRTE, S3AXX04_SRST };
	int ret;

	const struct spi_buf tx_bufs[] = {{
		.buf = op,
		.len = sizeof(op),
	}};
	const struct spi_buf_set tx_buf_set = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	ret = spi_write_dt(&config->spi, &tx_buf_set);
	if (ret < 0) {
		LOG_ERR("Software reset failed");
		return ret;
	}

	return 0;
}

static int eeprom_s3axx04_check(const struct device *dev)
{
	const struct s3axx04_config *config = dev->config;
	uint8_t op[4] = { S3AXX04_RDID, 0, 0, 0 };
	uint8_t id[4];
	int ret;

	const struct spi_buf tx_bufs[] = {{
		.buf = op,
		.len = sizeof(op),
	}};
	const struct spi_buf_set tx_buf_set = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	const struct spi_buf rx_bufs[] = {{
		.buf = id,
		.len = sizeof(id),
	}};
	const struct spi_buf_set rx_buf_set = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};

	ret = spi_transceive_dt(&config->spi, &tx_buf_set, &rx_buf_set);
	if (ret < 0) {
		LOG_ERR("Couldn't read device ID");
		return ret;
	}

	if (id[1 + S3AXX04_ID_MFG_POS] != S3AXX04_ID_MFG) {
		LOG_ERR("Manufacturer isn't Netsol");
		return -ENOTSUP;
	}

	return 0;
}

static int eeprom_s3axx04_disable_wren(const struct device *dev)
{
	const struct s3axx04_config *config = dev->config;
	uint8_t op[5] = { S3AXX04_RDCX, 0, 0, 0, 0 };
	uint8_t regs[5];
	int ret;

	const struct spi_buf tx_bufs[] = {{
		.buf = op,
		.len = sizeof(op),
	}};
	const struct spi_buf_set tx_buf_set = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	const struct spi_buf rx_bufs[] = {{
		.buf = regs,
		.len = sizeof(regs),
	}};
	const struct spi_buf_set rx_buf_set = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};

	ret = spi_transceive_dt(&config->spi, &tx_buf_set, &rx_buf_set);
	if (ret < 0) {
		LOG_ERR("Couldn't read regs");
		return ret;
	}

	if ((regs[4] & S3AXX04_REG4_WRENS) == S3AXX04_REG4_WRENS_DIS) {
		return 0;
	}

	regs[4] &= ~S3AXX04_REG4_WRENS;
	regs[4] |= S3AXX04_REG4_WRENS_DIS;

	ret = eeprom_s3axx04_set_enable_write(dev, true);
	if (ret < 0) {
		LOG_ERR("Could not enable writes while disabling WREN");
		return ret;
	}

	regs[0] = S3AXX04_WRCX;

	ret = spi_write_dt(&config->spi, &rx_buf_set);
	if (ret < 0) {
		LOG_ERR("Could not disable WREN");
		return ret;
	}

	return 0;
}

int eeprom_s3axx04_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	int ret;
	const struct s3axx04_config *config = dev->config;
	uint8_t read_op[S3AXX04_RW_LEN] = { S3AXX04_READ };

	if (offset + len > config->size) {
		LOG_ERR("Can not read more data than the device size");
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	sys_put_be24(offset, &read_op[1]);

	const struct spi_buf tx_bufs[] = {{
		.buf = &read_op,
		.len = S3AXX04_RW_LEN,
	}};
	const struct spi_buf_set tx_buf_set = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	const struct spi_buf rx_bufs[2] = {
		{
			.buf = NULL,
			.len = S3AXX04_RW_LEN,
		},
		{
			.buf = data,
			.len = len,
		},
	};
	const struct spi_buf_set rx_buf_set = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};

	ret = spi_transceive_dt(&config->spi, &tx_buf_set, &rx_buf_set);
	if (ret < 0) {
		LOG_ERR("Failed to read from MRAM");
		return ret;
	}

	return 0;
}

int eeprom_s3axx04_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	int ret;
	const struct s3axx04_config *config = dev->config;
	struct s3axx04_data *dev_data = dev->data;
	uint8_t write_op[S3AXX04_RW_LEN] = { S3AXX04_WRITE };

	if (config->read_only) {
		LOG_ERR("Can not write to a readonly device");
		return -EACCES;
	}

	if (offset + len > config->size) {
		LOG_ERR("Can not write more data than the device size");
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	sys_put_be24(offset, &write_op[1]);

	const struct spi_buf tx_bufs[] = {
		{
			.buf = &write_op,
			.len = S3AXX04_RW_LEN,
		},
		{
			.buf = (void *)data,
			.len = len,
		},
	};
	const struct spi_buf_set tx_buf_set = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	k_sem_take(&dev_data->lock, K_FOREVER);

	if (!config->disable_wren) {
		ret = eeprom_s3axx04_set_enable_write(dev, true);
		if (ret < 0) {
			k_sem_give(&dev_data->lock);
			LOG_ERR("Could not enable writes");
			return ret;
		}
	}

	ret = spi_write_dt(&config->spi, &tx_buf_set);
	if (ret < 0) {
		k_sem_give(&dev_data->lock);
		LOG_ERR("Failed to write to MRAM");
		return ret;
	}

	if (!config->disable_wren) {
		ret = eeprom_s3axx04_set_enable_write(dev, false);
		if (ret < 0) {
			k_sem_give(&dev_data->lock);
			LOG_ERR("Could not disable writes");
			return ret;
		}
	}

	k_sem_give(&dev_data->lock);

	return 0;
}

size_t eeprom_s3axx04_get_size(const struct device *dev)
{
	const struct s3axx04_config *config = dev->config;

	return config->size;
}

static int eeprom_s3axx04_init(const struct device *dev)
{
	const struct s3axx04_config *config = dev->config;
	struct s3axx04_data *data = dev->data;
	int ret;

	k_sem_init(&data->lock, 1, 1);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	ret = eeprom_s3axx04_soft_reset(dev);
	if (ret < 0) {
		return ret;
	}

	k_msleep(S3AXX04_RESET_MS);

	ret = eeprom_s3axx04_check(dev);
	if (ret < 0) {
		return ret;
	}

	if (config->disable_wren) {
		ret = eeprom_s3axx04_disable_wren(dev);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(eeprom, eeprom_s3axx04_api) = {
	.read = &eeprom_s3axx04_read,
	.write = &eeprom_s3axx04_write,
	.size = &eeprom_s3axx04_get_size,
};

#define S3AXX04_INIT(inst)                                                                         \
	static struct s3axx04_data s3axx04_data_##inst;                                            \
	static const struct s3axx04_config s3axx04_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8)),           \
		.size = DT_INST_PROP(inst, size),                                                  \
		.read_only = DT_INST_PROP(inst, read_only),                                        \
		.disable_wren = DT_INST_PROP(inst, disable_wren),                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, eeprom_s3axx04_init, NULL, &s3axx04_data_##inst,               \
			      &s3axx04_config_##inst, POST_KERNEL, CONFIG_EEPROM_INIT_PRIORITY,    \
			      &eeprom_s3axx04_api);

DT_INST_FOREACH_STATUS_OKAY(S3AXX04_INIT)
