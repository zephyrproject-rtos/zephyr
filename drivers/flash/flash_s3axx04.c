/*
 * Copyright (c) 2025-2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#include "zephyr/kernel.h"
#include "zephyr/sys/byteorder.h"
#include "zephyr/sys/util.h"
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT netsol_s3axx04

LOG_MODULE_REGISTER(s3axx04, CONFIG_FLASH_LOG_LEVEL);

/* Commands */
#define S3AXX04_WREN	0x06
#define S3AXX04_WRDI	0x04
#define S3AXX04_SPIE	0xff
#define S3AXX04_QPIE	0x38
#define S3AXX04_DPIE	0x37
#define S3AXX04_DPDE	0xb9
#define S3AXX04_DPDX	0xab
#define S3AXX04_SRTE	0x66
#define S3AXX04_SRST	0x99
#define S3AXX04_RDSR	0x05
#define S3AXX04_WRSR	0x01
#define S3AXX04_RDCX	0x46
#define S3AXX04_WRCX	0x87
#define S3AXX04_RDID	0x9f
#define S3AXX04_RUID	0x4c
#define S3AXX04_READ	0x03
#define S3AXX04_WRITE	0x02

#define S3AXX04_RW_LEN	4

#define S3AXX04_ID_MFG		0xd9
#define S3AXX04_ID_MFG_POS	0

#define S3AXX04_CONFIGREGS_CNT	4
#define S3AXX04_REG4_WRENS	GENMASK(1, 0)
#define S3AXX04_REG4_WRENS_EN	0x0
#define S3AXX04_REG4_WRENS_DIS	0x1
#define S3AXX04_REG4_WRENS_BTB	0x2

#define S3AXX04_RESET_MS	4

#define S3AXX04_ERASE_VALUE	0xff

struct s3axx04_config {
	struct spi_dt_spec spi;
	uint8_t config_regs[S3AXX04_CONFIGREGS_CNT];
	bool disable_wren;
	struct flash_pages_layout layout;
};

struct s3axx04_data {
	struct k_sem lock;
};

struct flash_parameters flash_s3axx04_params = {
	.write_block_size = 1,
	.erase_value = S3AXX04_ERASE_VALUE,
};

static int flash_s3axx04_set_enable_write(const struct device *dev, bool enable_writes)
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

static int flash_s3axx04_soft_reset(const struct device *dev)
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

static int flash_s3axx04_check(const struct device *dev)
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

static int flash_s3axx04_set_configregs(const struct device *dev)
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
		LOG_ERR("Couldn't read configuration registers");
		return ret;
	}

	regs[1] = config->config_regs[0];
	regs[2] = config->config_regs[1];
	regs[3] = config->config_regs[2];
	regs[4] = config->config_regs[3];

	if (config->disable_wren) {
		regs[4] &= ~S3AXX04_REG4_WRENS;
		regs[4] |= S3AXX04_REG4_WRENS_DIS;
	}

	ret = flash_s3axx04_set_enable_write(dev, true);
	if (ret < 0) {
		LOG_ERR("Could not enable writes for configuration registers");
		return ret;
	}

	regs[0] = S3AXX04_WRCX;

	ret = spi_write_dt(&config->spi, &rx_buf_set);
	if (ret < 0) {
		LOG_ERR("Could not set configuration registers");
		return ret;
	}

	return 0;
}

static int flash_s3axx04_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct s3axx04_config *config = dev->config;
	uint8_t read_op[S3AXX04_RW_LEN] = { S3AXX04_READ };
	int ret;

	if (len == 0) {
		return 0;
	}

	if (offset < 0) {
		LOG_ERR("Offset is negative");
		return -EINVAL;
	}

	if (offset > config->layout.pages_count || len > (config->layout.pages_count - offset)) {
		LOG_ERR("Can not read more data than the device size");
		return -EINVAL;
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

static int flash_s3axx04_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	const struct s3axx04_config *config = dev->config;
	struct s3axx04_data *dev_data = dev->data;
	uint8_t write_op[S3AXX04_RW_LEN] = { S3AXX04_WRITE };
	int ret;

	if (len == 0) {
		return 0;
	}

	if (offset < 0) {
		LOG_ERR("Offset is negative");
		return -EINVAL;
	}

	if (offset > config->layout.pages_count || len > (config->layout.pages_count - offset)) {
		LOG_ERR("Can not write more data than the device size");
		return -EINVAL;
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
		ret = flash_s3axx04_set_enable_write(dev, true);
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
		ret = flash_s3axx04_set_enable_write(dev, false);
		if (ret < 0) {
			k_sem_give(&dev_data->lock);
			LOG_ERR("Could not disable writes");
			return ret;
		}
	}

	k_sem_give(&dev_data->lock);

	return 0;
}

/* Provided for compatibility, slow */
static int flash_s3axx04_erase(const struct device *dev, off_t start, size_t len)
{
	const struct s3axx04_config *config = dev->config;
	uint8_t buf = S3AXX04_ERASE_VALUE;
	int ret = 0;

	if (len == 0) {
		return 0;
	}

	if (start > config->layout.pages_count || len > (config->layout.pages_count - start)) {
		LOG_ERR("Can not erase more data than the device size");
		return -EINVAL;
	}

	for (size_t i = 0; i < len; i++) {
		ret = flash_s3axx04_write(dev, start + i, &buf, 1);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

static const struct flash_parameters *flash_s3axx04_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);
	return &flash_s3axx04_params;
}

static int flash_s3axx04_init(const struct device *dev)
{
	const struct s3axx04_config *config = dev->config;
	struct s3axx04_data *data = dev->data;
	int ret;

	k_sem_init(&data->lock, 1, 1);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	ret = flash_s3axx04_soft_reset(dev);
	if (ret < 0) {
		return ret;
	}

	k_msleep(S3AXX04_RESET_MS);

	ret = flash_s3axx04_check(dev);
	if (ret < 0) {
		return ret;
	}

	ret = flash_s3axx04_set_configregs(dev);

	return ret;
}

#if CONFIG_FLASH_PAGE_LAYOUT
void flash_s3axx04_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	const struct s3axx04_config *cfg = dev->config;

	*layout = &cfg->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_s3axx04_api) = {
	.read = flash_s3axx04_read,
	.write = flash_s3axx04_write,
	.erase = flash_s3axx04_erase,
	.get_parameters = flash_s3axx04_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_s3axx04_page_layout,
#endif
};

#define S3AXX04_INIT(inst)                                                                         \
	static struct s3axx04_data s3axx04_data_##inst;                                            \
	static const struct s3axx04_config s3axx04_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8)),           \
		.disable_wren = DT_INST_PROP(inst, disable_wren),                                  \
		.config_regs = DT_INST_PROP(inst, config_regs),                                    \
		.layout = {                                                                        \
			.pages_count = DT_INST_PROP(inst, size),                                   \
			.pages_size = 1,                                                           \
		}                                                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, flash_s3axx04_init, NULL, &s3axx04_data_##inst,                \
			      &s3axx04_config_##inst, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,     \
			      &flash_s3axx04_api);

DT_INST_FOREACH_STATUS_OKAY(S3AXX04_INIT)
