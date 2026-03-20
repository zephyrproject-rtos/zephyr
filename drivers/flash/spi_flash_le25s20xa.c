/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Onsemi LE25S20XA SPI flash chips
 */

#define DT_DRV_COMPAT onnn_le25s20xa

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

LOG_MODULE_REGISTER(spi_flash_le25s20xa, CONFIG_FLASH_LOG_LEVEL);

#define FLASH_LE25S20XA_STATUS_REGISTER_WRITE 0x01U
#define FLASH_LE25S20XA_PAGE_PROGRAM          0x02U
#define FLASH_LE25S20XA_READ                  0x03U
#define FLASH_LE25S20XA_WRITE_DISABLE         0x04U
#define FLASH_LE25S20XA_STATUS_REGISTER_READ  0x05U
#define FLASH_LE25S20XA_WRITE_ENABLE          0x06U
#define FLASH_LE25S20XA_CHIP_ERASE            0x60U
#define FLASH_LE25S20XA_JEDEC_ID_READ         0x9FU
#define FLASH_LE25S20XA_EXIT_POWER_DOWN_MODE  0xABU
#define FLASH_LE25S20XA_POWER_DOWN            0xB9U
#define FLASH_LE25S20XA_SMALL_SECTOR_ERASE    0xD7U
#define FLASH_LE25S20XA_SECTOR_ERASE          0xD8U

#define FLASH_LE25S20XA_RDY BIT(0)
#define FLASH_LE25S20XA_WEN BIT(1)
#define FLASH_LE25S20XA_BP  GENMASK(3, 2)

#define FLASH_LE25S20XA_SMALL_SECTOR_SIZE KB(4)
#define FLASH_LE25S20XA_SECTOR_SIZE       KB(64)

#define FLASH_LE25S20XA_T_WAIT K_MSEC(500)
#define FLASH_LE25S20XA_T_DP   K_USEC(5)
#define FLASH_LE25S20XA_T_PRB  K_USEC(5)

struct flash_le25s20xa_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec wp_gpio;
	const uint8_t jedec_id[3];
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	const struct flash_pages_layout pages_layout;
#endif /* defined(CONFIG_FLASH_PAGE_LAYOUT) */
	const struct flash_parameters parameters;
	const uint64_t size;
	const bool read_only;
	const bool small_sector_erase;
};

struct flash_le25s20xa_data {
	struct k_mutex lock;
};

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_le25s20xa_pages_layout(const struct device *dev,
					 const struct flash_pages_layout **layout,
					 size_t *layout_size)
{
	const struct flash_le25s20xa_config *config = dev->config;

	*layout = &config->pages_layout;
	*layout_size = 1;
}
#endif /* defined(CONFIG_FLASH_PAGE_LAYOUT) */

static int flash_le25s20xa_read_status(const struct device *dev, uint8_t *const status)
{
	const struct flash_le25s20xa_config *config = dev->config;
	uint8_t cmd = FLASH_LE25S20XA_STATUS_REGISTER_READ;
	const struct spi_buf tx_buf = {.buf = &cmd, .len = 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf rx_bufs[2] = {
		{.buf = NULL, .len = 1},
		{.buf = status, .len = 1},
	};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

	return spi_transceive_dt(&config->spi, &tx, &rx);
}

static int flash_le25s20xa_wait_for_idle(const struct device *dev, k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	uint8_t status;
	int ret;

	while (!sys_timepoint_expired(end)) {
		ret = flash_le25s20xa_read_status(dev, &status);
		if (ret < 0) {
			return ret;
		}

		if ((status & FLASH_LE25S20XA_RDY) == 0) {
			return 0;
		}

		(void)k_msleep(1);
	}

	LOG_DBG("timed out waiting for %s to idle", dev->name);

	return -EBUSY;
}

static int flash_le25s20xa_write_enable(const struct device *dev, const bool enable)
{
	uint8_t cmd = enable ? FLASH_LE25S20XA_WRITE_ENABLE : FLASH_LE25S20XA_WRITE_DISABLE;
	const struct flash_le25s20xa_config *config = dev->config;
	const struct spi_buf tx_buf = {.buf = &cmd, .len = 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	uint8_t status;
	int ret;

	ret = flash_le25s20xa_wait_for_idle(dev, FLASH_LE25S20XA_T_WAIT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		LOG_DBG("failed to send write enable update");
		return ret;
	}

	ret = flash_le25s20xa_wait_for_idle(dev, FLASH_LE25S20XA_T_WAIT);
	if (ret < 0) {
		return ret;
	}

	ret = flash_le25s20xa_read_status(dev, &status);
	if (ret < 0) {
		return ret;
	}

	if (FIELD_GET(FLASH_LE25S20XA_WEN, status) != (int)enable) {
		LOG_DBG("failed to update write enable");
		return -EIO;
	}

	return 0;
}

static int flash_le25s20xa_block_protect(const struct device *dev, const bool enable)
{
	uint8_t cmd[2] = {FLASH_LE25S20XA_STATUS_REGISTER_WRITE, enable ? GENMASK(7, 0) : BIT(7)};
	const struct spi_buf tx_buf = {.buf = cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct flash_le25s20xa_config *config = dev->config;
	uint8_t status;
	int ret;

	ret = flash_le25s20xa_write_enable(dev, true);
	if (ret < 0) {
		return ret;
	}

	ret = flash_le25s20xa_wait_for_idle(dev, FLASH_LE25S20XA_T_WAIT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		LOG_DBG("failed to send block protection update");
		return ret;
	}

	ret = flash_le25s20xa_wait_for_idle(dev, FLASH_LE25S20XA_T_WAIT);
	if (ret < 0) {
		return ret;
	}

	ret = flash_le25s20xa_read_status(dev, &status);
	if (ret < 0) {
		return ret;
	}

	if (FIELD_GET(FLASH_LE25S20XA_BP, status) != (enable ? GENMASK(1, 0) : 0)) {
		LOG_DBG("failed to update block protection");
		return -EIO;
	}

	return 0;
}

static int flash_le25s20xa_allow_program(const struct device *dev)
{
	const struct flash_le25s20xa_config *config = dev->config;
	int ret;

	if (IS_ENABLED(CONFIG_SPI_FLASH_LE25S20XA_WRITE_PROTECT) && config->wp_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->wp_gpio, 0);
		if (ret < 0) {
			return ret;
		}
	}

	return flash_le25s20xa_block_protect(dev, false);
}

static int flash_le25s20xa_prevent_program(const struct device *dev)
{
	const struct flash_le25s20xa_config *config = dev->config;
	int ret;

	ret = flash_le25s20xa_block_protect(dev, true);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_SPI_FLASH_LE25S20XA_WRITE_PROTECT) && config->wp_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->wp_gpio, 1);
	}

	return 0;
}

static int flash_le25s20xa_read_helper(const struct device *dev, off_t offset, void *buf,
				       size_t len)
{
	const struct flash_le25s20xa_config *config = dev->config;
	uint8_t cmd[4] = {
		FLASH_LE25S20XA_READ,
		FIELD_GET(GENMASK(23, 16), offset),
		FIELD_GET(GENMASK(15, 8), offset),
		FIELD_GET(GENMASK(7, 0), offset),
	};
	const struct spi_buf tx_buf = {.buf = cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf rx_bufs[2] = {
		{.buf = NULL, .len = ARRAY_SIZE(cmd)},
		{.buf = buf, .len = len},
	};
	struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};
	int ret;

	/* Not strictly necessary but prevents corrupted readings during program or erase. */
	ret = flash_le25s20xa_wait_for_idle(dev, FLASH_LE25S20XA_T_WAIT);
	if (ret < 0) {
		return ret;
	}

	return spi_transceive_dt(&config->spi, &tx, &rx);
}

static int flash_le25s20xa_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct flash_le25s20xa_config *config = dev->config;
	struct flash_le25s20xa_data *data = dev->data;
	int ret;

	if (len == 0) {
		return 0;
	}

	if (len > config->size || !IN_RANGE(offset, 0, config->size - len)) {
		LOG_DBG("invalid parameters, require 0 < offset + len <= %llu", config->size);
		return -EINVAL;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, FLASH_LE25S20XA_T_WAIT);
	if (ret < 0) {
		LOG_DBG("failed to take mutex (%d)", ret);
		goto error_pm;
	}

	ret = flash_le25s20xa_read_helper(dev, offset, buf, len);
	if (ret < 0) {
		LOG_DBG("failed to read from %s (%d)", dev->name, ret);
	}

	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

static inline int flash_le25s20xa_write_helper(const struct device *dev, off_t offset,
					       const void *buf)
{
	const struct flash_le25s20xa_config *config = dev->config;
	uint8_t cmd[4] = {
		FLASH_LE25S20XA_PAGE_PROGRAM,
		FIELD_GET(GENMASK(17, 16), offset),
		FIELD_GET(GENMASK(15, 8), offset),
		FIELD_GET(GENMASK(7, 0), offset),
	};
	struct spi_buf tx_bufs[2] = {
		{.buf = cmd, .len = ARRAY_SIZE(cmd)},
		{.buf = (void *)buf, .len = config->parameters.write_block_size},
	};
	struct spi_buf_set tx = {.buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs)};
	int ret;

	ret = flash_le25s20xa_write_enable(dev, true);
	if (ret < 0) {
		return ret;
	}

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		LOG_DBG("failed to write program command");
		return ret;
	}

	return 0;
}

static int flash_le25s20xa_write(const struct device *dev, off_t offset, const void *buf,
				 size_t len)
{
	const struct flash_le25s20xa_config *config = dev->config;
	struct flash_le25s20xa_data *data = dev->data;
	int ret;

	if (len == 0) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_SPI_FLASH_LE25S20XA_READ_ONLY) || config->read_only) {
		LOG_ERR("cannot write to read-only device %s", dev->name);
		return -EPERM;
	}

	if ((offset & (config->parameters.write_block_size - 1)) != 0 ||
	    (len & (config->parameters.write_block_size - 1)) != 0) {
		LOG_DBG("offset and/or len is not aligned to page size %u",
			config->parameters.write_block_size);
		return -EINVAL;
	}

	if (len > config->size || !IN_RANGE(offset, 0, config->size - len)) {
		LOG_DBG("invalid parameters, require 0 < offset + len <= %llu", config->size);
		return -EINVAL;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, FLASH_LE25S20XA_T_WAIT);
	if (ret < 0) {
		LOG_DBG("failed to take mutex (%d)", ret);
		goto error_pm;
	}

	ret = flash_le25s20xa_allow_program(dev);
	if (ret < 0) {
		goto error_pm;
	}

	do {
		ret = flash_le25s20xa_write_helper(dev, offset, buf);
		if (ret < 0) {
			LOG_DBG("failed to program %s (%ld, %u)", dev->name, offset, len);
			break;
		}

		buf = (uint8_t *)buf + config->parameters.write_block_size;
		len -= config->parameters.write_block_size;
		offset += config->parameters.write_block_size;
	} while (len > 0);

	if (flash_le25s20xa_prevent_program(dev) < 0) {
		LOG_WRN("failed to re-enable block protection");
	}

	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

static int flash_le25s20xa_chip_erase(const struct device *dev)
{
	const struct flash_le25s20xa_config *config = dev->config;
	uint8_t cmd = FLASH_LE25S20XA_CHIP_ERASE;
	const struct spi_buf tx_buf = {.buf = &cmd, .len = 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	int ret;

	ret = flash_le25s20xa_write_enable(dev, true);
	if (ret < 0) {
		return ret;
	}

	return spi_write_dt(&config->spi, &tx);
}

static int flash_le25s20xa_sector_erase(const struct device *dev, off_t offset, size_t size,
					const uint32_t sector_size)
{
	const struct flash_le25s20xa_config *config = dev->config;
	uint8_t addr, cmd[4] = {0};
	const struct spi_buf tx_buf = {.buf = &cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	int ret;

	if (config->small_sector_erase) {
		cmd[0] = FLASH_LE25S20XA_SMALL_SECTOR_ERASE;
	} else {
		cmd[0] = FLASH_LE25S20XA_SECTOR_ERASE;
	}

	addr = offset / sector_size;

	do {
		ret = flash_le25s20xa_write_enable(dev, true);
		if (ret < 0) {
			return ret;
		}

		if (config->small_sector_erase) {
			cmd[1] = FIELD_GET(GENMASK(7, 6), addr);
			cmd[2] = FIELD_GET(GENMASK(5, 0), addr);
		} else {
			cmd[1] = addr;
			cmd[2] = 0;
		}

		ret = spi_write_dt(&config->spi, &tx);
		if (ret < 0) {
			LOG_DBG("failed to write sector erase command");
			return ret;
		}

		addr += 1;
		size -= sector_size;
	} while (size > 0);

	return 0;
}

static inline int flash_le25s20xa_erase_helper(const struct device *dev, off_t offset, size_t size,
					       const uint32_t sector_size)
{
	const struct flash_le25s20xa_config *config = dev->config;
	int ret;

	ret = flash_le25s20xa_allow_program(dev);
	if (ret < 0) {
		return ret;
	}

	if (size == config->size) {
		ret = flash_le25s20xa_chip_erase(dev);
	} else {
		ret = flash_le25s20xa_sector_erase(dev, offset, size, sector_size);
	}

	if (flash_le25s20xa_prevent_program(dev) < 0) {
		LOG_WRN("failed to re-enable block protection");
	}

	return ret;
}

static int flash_le25s20xa_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct flash_le25s20xa_config *config = dev->config;
	struct flash_le25s20xa_data *data = dev->data;
	uint32_t sector_size;
	int ret;

	if (size == 0) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_SPI_FLASH_LE25S20XA_READ_ONLY) || config->read_only) {
		LOG_ERR("cannot erase from read-only device %s", dev->name);
		return -EPERM;
	}

	sector_size = config->small_sector_erase ? (uint32_t)FLASH_LE25S20XA_SMALL_SECTOR_SIZE
						 : (uint32_t)FLASH_LE25S20XA_SECTOR_SIZE;

	if ((offset & (sector_size - 1)) != 0 || (size & (sector_size - 1)) != 0) {
		LOG_DBG("offset and/or size is not aligned to sector size %u", sector_size);
		return -EINVAL;
	}

	if (size > config->size || !IN_RANGE(offset, 0, config->size - size)) {
		LOG_DBG("invalid parameters, require 0 < offset + size <= %llu", config->size);
		return -EINVAL;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, FLASH_LE25S20XA_T_WAIT);
	if (ret < 0) {
		LOG_DBG("failed to take mutex (%d)", ret);
		goto error_pm;
	}

	ret = flash_le25s20xa_erase_helper(dev, offset, size, sector_size);
	if (ret < 0) {
		LOG_DBG("failed to erase %s", dev->name);
	}

	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

static int flash_le25s20xa_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_le25s20xa_config *config = dev->config;

	*size = config->size;

	return 0;
}

static const struct flash_parameters *flash_le25s20xa_get_parameters(const struct device *dev)
{
	const struct flash_le25s20xa_config *config = dev->config;

	return &config->parameters;
}

static int flash_le25s20xa_hardware_init(const struct device *dev)
{
	const struct flash_le25s20xa_config *config = dev->config;
	int ret;

	if (IS_ENABLED(CONFIG_SPI_FLASH_LE25S20XA_WRITE_PROTECT) && config->wp_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->wp_gpio, 0);
		if (ret < 0) {
			return ret;
		}
	}

	ret = flash_le25s20xa_write_enable(dev, true);
	if (ret < 0) {
		return ret;
	}

	/* Block protect implicitly sets SRWP. */
	ret = flash_le25s20xa_block_protect(dev, true);
	if (ret < 0) {
		return ret;
	}

	ret = flash_le25s20xa_write_enable(dev, false);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_SPI_FLASH_LE25S20XA_WRITE_PROTECT) && config->wp_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->wp_gpio, 1);
	}

	return 0;
}

static int flash_le25s20xa_verify_device(const struct device *dev)
{
	const struct flash_le25s20xa_config *config = dev->config;
	uint8_t cmd = FLASH_LE25S20XA_JEDEC_ID_READ, jedec_id[3];
	const struct spi_buf tx_buf = {.buf = &cmd, .len = 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf rx_bufs[2] = {
		{.buf = NULL, .len = 1},
		{.buf = jedec_id, .len = ARRAY_SIZE(jedec_id)},
	};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};
	int ret;

	ret = spi_transceive_dt(&config->spi, &tx, &rx);
	if (ret < 0) {
		return ret;
	}

	if (jedec_id[0] != config->jedec_id[0] || jedec_id[1] != config->jedec_id[1] ||
	    jedec_id[2] != config->jedec_id[2]) {
		LOG_ERR("invalid JEDEC ID: %02X %02X %02X", jedec_id[0], jedec_id[1], jedec_id[2]);
		return -ENODEV;
	}

	return 0;
}

static int flash_le25s20xa_bringup(const struct device *dev)
{
	const struct flash_le25s20xa_config *config = dev->config;
	int ret;

	ret = flash_le25s20xa_verify_device(dev);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_SPI_FLASH_LE25S20XA_WRITE_PROTECT) && config->wp_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->wp_gpio, GPIO_OUTPUT);
		if (ret < 0) {
			return ret;
		}
	}

	return flash_le25s20xa_hardware_init(dev);
}

static DEVICE_API(flash, spi_flash_le25s20xa_api) = {
	.read = flash_le25s20xa_read,
	.write = flash_le25s20xa_write,
	.erase = flash_le25s20xa_erase,
	.get_size = flash_le25s20xa_get_size,
	.get_parameters = flash_le25s20xa_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_le25s20xa_pages_layout,
#endif /* defined(CONFIG_FLASH_PAGE_LAYOUT) */
};

static int flash_le25s20xa_resume(const struct device *dev)
{
	const struct flash_le25s20xa_config *config = dev->config;
	uint8_t cmd = FLASH_LE25S20XA_EXIT_POWER_DOWN_MODE;
	const struct spi_buf tx_buf = {.buf = &cmd, .len = 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	int ret;

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		LOG_DBG("failed to write power up command (%d)", ret);
		return ret;
	}

	return k_sleep(FLASH_LE25S20XA_T_PRB);
}

#ifdef CONFIG_PM_DEVICE
static int flash_le25s20xa_suspend(const struct device *dev)
{
	const struct flash_le25s20xa_config *config = dev->config;
	uint8_t cmd = FLASH_LE25S20XA_POWER_DOWN;
	const struct spi_buf tx_buf = {.buf = &cmd, .len = 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	int ret;

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		LOG_DBG("failed to write power down command (%d)", ret);
		return ret;
	}

	return k_sleep(FLASH_LE25S20XA_T_DP);
}
#endif /* CONFIG_PM_DEVICE */

static int flash_le25s20xa_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return flash_le25s20xa_resume(dev);
#ifdef CONFIG_PM_DEVICE
	case PM_DEVICE_ACTION_SUSPEND:
		return flash_le25s20xa_suspend(dev);
#endif /* CONFIG_PM_DEVICE */
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	case PM_DEVICE_ACTION_TURN_ON:
		return flash_le25s20xa_bringup(dev);
	default:
		return -ENOTSUP;
	}
}

static int flash_le25s20xa_init(const struct device *dev)
{
	const struct flash_le25s20xa_config *config = dev->config;
	struct flash_le25s20xa_data *data = dev->data;
	int ret;

	ret = k_mutex_init(&data->lock);
	if (ret < 0) {
		return ret;
	}

	if (!device_is_ready(config->spi.bus)) {
		LOG_DBG("spi bus is not ready");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_SPI_FLASH_LE25S20XA_WRITE_PROTECT) && config->wp_gpio.port != NULL &&
	    !gpio_is_ready_dt(&config->wp_gpio)) {
		LOG_DBG("write protect GPIO is not ready");
		return -ENODEV;
	}

	return pm_device_driver_init(dev, flash_le25s20xa_pm_action);
}

#if CONFIG_PM_DEVICE
__maybe_unused static int flash_le25s20xa_deinit(const struct device *dev)
{
	return pm_device_driver_deinit(dev, flash_le25s20xa_pm_action);
}
#endif /* CONFIG_PM_DEVICE */

#define SPI_OP (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))

#define SPI_FLASH_LE25S20XA_PAGES_LAYOUT(inst)                                                     \
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (.pages_layout = {					   \
		.pages_count = DT_INST_PROP(inst, size) / DT_INST_PROP(inst, page_size),	   \
		.pages_size = DT_INST_PROP(inst, page_size),					   \
	}))

#define SPI_FLASH_LE25S20XA_INIT(inst)                                                             \
	COND_CODE_1(CONFIG_PM_DEVICE,								   \
		(DEVICE_DT_INST_DEINIT_DEFINE(inst, flash_le25s20xa_init, flash_le25s20xa_deinit,  \
			PM_DEVICE_DT_INST_GET(inst), &flash_le25s20xa_data_##inst,		   \
			&flash_le25s20xa_config_##inst, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,   \
			&spi_flash_le25s20xa_api)),						   \
		(DEVICE_DT_INST_DEFINE(inst, flash_le25s20xa_init, PM_DEVICE_DT_INST_GET(inst),	   \
			&flash_le25s20xa_data_##inst, &flash_le25s20xa_config_##inst, POST_KERNEL, \
			CONFIG_FLASH_INIT_PRIORITY, &spi_flash_le25s20xa_api)))

#define SPI_FLASH_LE25S20XA_DEFINE(inst)                                                           \
	static const struct flash_le25s20xa_config flash_le25s20xa_config_##inst = {               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP),                                         \
		.wp_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, wp_gpios, {0}),                          \
		.jedec_id = DT_INST_PROP(inst, jedec_id),                                          \
		.size = DT_INST_PROP(inst, size),                                                  \
		.read_only = DT_INST_PROP(inst, read_only),                                        \
		.small_sector_erase = DT_INST_PROP(inst, small_sector_erase),                      \
		SPI_FLASH_LE25S20XA_PAGES_LAYOUT(inst),                                            \
		.parameters =                                                                      \
			{                                                                          \
				.write_block_size = DT_INST_PROP(inst, page_size),                 \
				.erase_value = 0xff,                                               \
			},                                                                         \
	};                                                                                         \
	static struct flash_le25s20xa_data flash_le25s20xa_data_##inst;                            \
	PM_DEVICE_DT_INST_DEFINE(inst, flash_le25s20xa_pm_action);                                 \
	SPI_FLASH_LE25S20XA_INIT(inst);

DT_INST_FOREACH_STATUS_OKAY(SPI_FLASH_LE25S20XA_DEFINE)
