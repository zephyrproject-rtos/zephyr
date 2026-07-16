/*
 * Copyright (c) 2026 sensryio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT heimann_htpa_flash

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(flash_htpa, CONFIG_FLASH_LOG_LEVEL);

#define HTPA_FLASH_READ_OPCODE       0x03
#define HTPA_FLASH_STATUS_OPCODE     0x05
#define HTPA_FLASH_SPI_OPERATION     (SPI_OP_MODE_MASTER | SPI_WORD_SET(8))
#define HTPA_FLASH_STATUS_BUSY_MASK  0x81
#define HTPA_FLASH_INIT_MAX_ATTEMPTS 10U
#define HTPA_FLASH_INIT_RETRY_DELAY  K_MSEC(10)

struct flash_htpa_config {
	struct spi_dt_spec spi;
	size_t size;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

struct flash_htpa_data {
	int unused;
};

static const struct flash_parameters flash_htpa_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff,
};

static int flash_read_status(const struct device *dev, uint8_t *status)
{
	const struct flash_htpa_config *config = dev->config;

	uint8_t rx_buffer[1];
	uint32_t rx_len = 1;

	uint8_t tx_buffer[1] = {HTPA_FLASH_STATUS_OPCODE};
	uint32_t tx_len = 1;

	/* Transmit the status register command. */
	struct spi_buf tx_buf[1];

	tx_buf[0].buf = tx_buffer;
	tx_buf[0].len = tx_len;

	const struct spi_buf_set tx = {.buffers = (struct spi_buf *)&tx_buf, .count = 1};

	/* Receive the status register value after the command phase. */
	struct spi_buf rx_buf[2];

	rx_buf[0].buf = NULL;
	rx_buf[0].len = tx_len;

	rx_buf[1].buf = rx_buffer;
	rx_buf[1].len = rx_len;

	const struct spi_buf_set rx = {
		.buffers = (struct spi_buf *)&rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	int ret = spi_transceive_dt(&config->spi, &tx, &rx);

	if (ret) {
		LOG_ERR("SPI read failed: %d", ret);
		return -EIO;
	}

	*status = rx_buffer[0];

	return 0;
}

static bool flash_htpa_valid_range(const struct flash_htpa_config *config, off_t offset, size_t len)
{
	size_t uoffset;

	if (offset < 0) {
		return false;
	}

	uoffset = (size_t)offset;
	if ((uoffset > config->size) || (len > (config->size - uoffset))) {
		return false;
	}

	return true;
}

static int flash_htpa_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct flash_htpa_config *config = dev->config;
	/* Read command: opcode followed by a 24-bit big-endian address. */
	uint8_t cmd[4] = {HTPA_FLASH_READ_OPCODE};

	const struct spi_buf tx_bufs[] = {
		{
			.buf = cmd,
			.len = sizeof(cmd),
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = NULL,
			.len = sizeof(cmd),
		},
		{
			.buf = data,
			.len = len,
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};
	int ret;

	if (len == 0) {
		return 0;
	}

	if (!flash_htpa_valid_range(config, offset, len)) {
		LOG_ERR("Read outside flash range");
		return -EINVAL;
	}

	sys_put_be24(offset, &cmd[1]);

	ret = spi_transceive_dt(&config->spi, &tx, &rx);
	if (ret < 0) {
		LOG_ERR("Flash read failed: %d", ret);
		return ret;
	}

	return 0;
}

static int flash_htpa_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(offset);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	/*
	 * The flash API requires a write callback. Calibration flash updates will be
	 * added once the sensor calibration update routines are available.
	 */
	return -EOPNOTSUPP;
}

static const struct flash_parameters *flash_htpa_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_htpa_parameters;
}

static int flash_htpa_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_htpa_config *config = dev->config;

	*size = config->size;
	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_htpa_page_layout(const struct device *dev,
				   const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct flash_htpa_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int flash_htpa_init(const struct device *dev)
{
	const struct flash_htpa_config *config = dev->config;
	uint32_t attempts = HTPA_FLASH_INIT_MAX_ATTEMPTS;
	uint8_t status = 0;
	uint32_t id = 0;
	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	do {
		ret = flash_read_status(dev, &status);
		if (ret != 0) {
			LOG_ERR("Failed to read status");
			return ret;
		}

		if ((status & HTPA_FLASH_STATUS_BUSY_MASK) == 0U) {
			break;
		}

		k_sleep(HTPA_FLASH_INIT_RETRY_DELAY);
	} while (--attempts > 0U);

	if ((status & HTPA_FLASH_STATUS_BUSY_MASK) != 0U) {
		LOG_ERR("Flash remained busy after %u attempts", HTPA_FLASH_INIT_MAX_ATTEMPTS);
		return -ETIMEDOUT;
	}

	/* Read the flash ID. */
	attempts = HTPA_FLASH_INIT_MAX_ATTEMPTS;

	do {
		ret = flash_htpa_read(dev, 0x74, &id, sizeof(id));
		if (ret != 0) {
			LOG_ERR("Failed to read flash id");
			return ret;
		}

		if ((id != 0U) && (id != UINT32_MAX)) {
			break;
		}

		k_sleep(HTPA_FLASH_INIT_RETRY_DELAY);
	} while (--attempts > 0U);

	if ((id == 0U) || (id == UINT32_MAX)) {
		LOG_ERR("Invalid flash ID after %u attempts", HTPA_FLASH_INIT_MAX_ATTEMPTS);
		return -ETIMEDOUT;
	}

	LOG_INF("Flash id: 0x%x", id);

	return 0;
}

static DEVICE_API(flash, flash_htpa_api) = {
	.read = flash_htpa_read,
	.write = flash_htpa_write,
	.get_parameters = flash_htpa_get_parameters,
	.get_size = flash_htpa_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_htpa_page_layout,
#endif
};

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
#define FLASH_HTPA_LAYOUT_INIT(inst)                                                               \
	.layout = {                                                                                \
		.pages_count = 1,                                                                  \
		.pages_size = DT_INST_PROP(inst, size),                                            \
	},
#else
#define FLASH_HTPA_LAYOUT_INIT(inst)
#endif

#define FLASH_HTPA_INIT(inst)                                                                      \
	static struct flash_htpa_data flash_htpa_data_##inst;                                      \
	static const struct flash_htpa_config flash_htpa_config_##inst = {                         \
		.spi = SPI_DT_SPEC_INST_GET(inst, HTPA_FLASH_SPI_OPERATION),                       \
		.size = DT_INST_PROP(inst, size),                                                  \
		FLASH_HTPA_LAYOUT_INIT(inst)};                                                     \
	DEVICE_DT_INST_DEFINE(inst, flash_htpa_init, NULL, &flash_htpa_data_##inst,                \
			      &flash_htpa_config_##inst, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,  \
			      &flash_htpa_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_HTPA_INIT)
