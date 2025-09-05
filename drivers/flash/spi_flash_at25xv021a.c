/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for AT25XV021A SPI flash devices, a variant of Atmel's AT25 family
 */

#define DT_DRV_COMPAT atmel_at25xv021a

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_flash_at25xv021a, CONFIG_FLASH_LOG_LEVEL);

/* AT25XV021A opcodes */
#define DEV_READ             (0x0b)
#define DEV_PAGE_ERASE       (0x81)
#define DEV_CHIP_ERASE       (0x60)
#define DEV_WRITE            (0x02)
#define DEV_WRITE_ENABLE     (0x06)
#define DEV_PROTECT          (0x36)
#define DEV_UNPROTECT        (0x39)
#define DEV_READ_SR          (0x05)
#define DEV_WRITE_SR         (0x01)
#define DEV_READ_DEVICE_INFO (0x9f)
#define DEV_DEEP_SLEEP       (0xb9)
#define DEV_ULTRA_DEEP_SLEEP (0x79)
#define DEV_RESUME           (0xab)

/* AT25XV021A driver instruction set */
#define DEV_DUMMY_BYTE       (0x00)
#define DEV_HW_LOCK          (0xf8)
#define DEV_HW_UNLOCK        (0x00)
#define DEV_GLOBAL_PROTECT   (0x7f)
#define DEV_GLOBAL_UNPROTECT (0x00)

/* AT25XV021A status register masks */
#define DEV_SR_BUSY (1U << 0)
#define DEV_SR_WEL  (1U << 1)
#define DEV_SR_SWP  (3U << 2)
#define DEV_SR_WPP  (1U << 4)
#define DEV_SR_EPE  (1U << 5)
#define DEV_SR_SPRL (1U << 7)

#define ANY_DEV_WRITEABLE   !DT_ALL_INST_HAS_BOOL_STATUS_OKAY(read_only)
#define ANY_DEV_HAS_WP_GPIO DT_ANY_INST_HAS_PROP_STATUS_OKAY(wp_gpios)

struct flash_at25xv021a_config {
	struct spi_dt_spec spi;
#if ANY_DEV_HAS_WP_GPIO
	struct gpio_dt_spec wp_gpio;
#endif /* ANY_DEV_HAS_WP_GPIO */
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout pages_layout;
#endif /* defined(CONFIG_FLASH_PAGE_LAYOUT) */
	struct flash_parameters parameters;
	uint8_t jedec_id[3];
	size_t size;
	k_timeout_t timeout;
	bool read_only;
	bool ultra_deep_sleep;
#if ANY_DEV_WRITEABLE
	size_t page_size;
	k_timeout_t timeout_erase;
#endif /* ANY_DEV_WRITEABLE */
};

struct flash_at25xv021a_data {
	struct k_mutex lock;
};

static int flash_at25xv021a_read_status(const struct device *dev, uint8_t *status)
{
	int err;
	uint8_t sr[2];
	uint8_t cmd[2] = {DEV_READ_SR, DEV_DUMMY_BYTE};
	const struct flash_at25xv021a_config *config = dev->config;
	const struct spi_buf tx_buf = {.buf = cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf rx_buf = {.buf = sr, .len = ARRAY_SIZE(sr)};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	err = spi_transceive_dt(&config->spi, &tx, &rx);
	if (err < 0) {
		LOG_ERR("unable to read status register from %s", dev->name);
		return err;
	}

	*status = sr[1];

	return err;
}

static int flash_at25xv021a_wait_for_idle(const struct device *dev, k_timeout_t timeout)
{
	int err;
	uint8_t status;
	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (!sys_timepoint_expired(end)) {
		err = flash_at25xv021a_read_status(dev, &status);
		if (err != 0) {
			return err;
		}

		if ((status & DEV_SR_BUSY) == 0) {
			return 0;
		}

		k_msleep(1);
	}

	LOG_ERR("timed out waiting for %s to idle", dev->name);
	return -EBUSY;
}

static int flash_at25xv021a_spi_transceive(const struct device *dev, const struct spi_dt_spec *spi,
					   const struct spi_buf_set *tx,
					   const struct spi_buf_set *rx)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;

	err = flash_at25xv021a_wait_for_idle(dev, config->timeout);
	if (err != 0) {
		return err;
	}

	err = spi_transceive_dt(spi, tx, rx);
	if (err < 0) {
		LOG_ERR("unable to read from %s", dev->name);
	}

	return err;
}

static int flash_at25xv021a_verify_device(const struct device *dev)
{
	int err;
	uint8_t info[3];
	uint8_t cmd[1] = {DEV_READ_DEVICE_INFO};
	const struct flash_at25xv021a_config *config = dev->config;
	const struct spi_buf tx_buf = {.buf = cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf rx_bufs[2] = {
		{.buf = NULL, .len = ARRAY_SIZE(cmd)},
		{.buf = info, .len = ARRAY_SIZE(info)},
	};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

	err = flash_at25xv021a_spi_transceive(dev, &config->spi, &tx, &rx);
	if (err != 0) {
		return err;
	}

	if ((info[0] != config->jedec_id[0]) || (info[1] != config->jedec_id[1]) ||
	    (info[2] != config->jedec_id[2])) {
		return -ENODEV;
	}

	return err;
}

static int flash_at25xv021a_read_internal(const struct device *dev, off_t offset, void *buf,
					  size_t len)
{
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[5] = {
		DEV_READ,
		FIELD_GET(GENMASK(23, 16), offset),
		FIELD_GET(GENMASK(15, 8), offset),
		FIELD_GET(GENMASK(7, 0), offset),
		DEV_DUMMY_BYTE,
	};
	const struct spi_buf tx_buf = {.buf = cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_bufs[2] = {
		{.buf = NULL, .len = ARRAY_SIZE(cmd)},
		{.buf = buf, .len = len},
	};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

	return flash_at25xv021a_spi_transceive(dev, &config->spi, &tx, &rx);
}

static int flash_at25xv021a_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	int err;
	struct flash_at25xv021a_data *data = dev->data;
	const struct flash_at25xv021a_config *config = dev->config;

	if (len == 0) {
		LOG_DBG("attempted to read 0 bytes from %s", dev->name);
		return 0;
	}

	if (len > config->size) {
		LOG_ERR("attempted to read more than device %s size: %u", dev->name, config->size);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	err = flash_at25xv021a_read_internal(dev, offset, buf, len);

	k_mutex_unlock(&data->lock);

	return err;
}

#if ANY_DEV_WRITEABLE
static int flash_at25xv021a_check_status(const struct device *dev, uint8_t mask, uint8_t *status)
{
	int err;
	uint8_t temp_status;
	const struct flash_at25xv021a_config *config = dev->config;

	err = flash_at25xv021a_wait_for_idle(dev, config->timeout);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_read_status(dev, &temp_status);
	if (err != 0) {
		return err;
	}

	*status = (temp_status & mask);

	return err;
}

static int flash_at25xv021a_spi_write(const struct device *dev, const struct spi_dt_spec *spi,
				      const struct spi_buf_set *tx)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;

	err = flash_at25xv021a_wait_for_idle(dev, config->timeout);
	if (err != 0) {
		return err;
	}

	err = spi_write_dt(spi, tx);
	if (err < 0) {
		LOG_ERR("unable to write to %s", dev->name);
	}

	return err;
}

static int flash_at25xv021a_write_enable(const struct device *dev)
{
	int err;
	uint8_t status;
	uint8_t cmd[1] = {DEV_WRITE_ENABLE};
	const struct flash_at25xv021a_config *config = dev->config;
	const struct spi_buf tx_buf = {.buf = &cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	err = flash_at25xv021a_spi_write(dev, &config->spi, &tx);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_check_status(dev, DEV_SR_WEL, &status);
	if (err != 0) {
		return err;
	}

	if (status != DEV_SR_WEL) {
		LOG_ERR("unable to enable writes on %s", dev->name);
		return -EIO;
	}

	return err;
}

static int flash_at25xv021a_hardware_lock(const struct device *dev)
{
	int err;
	uint8_t status;
	uint8_t cmd[2] = {DEV_WRITE_SR, DEV_HW_LOCK};
	const struct flash_at25xv021a_config *config = dev->config;
	const struct spi_buf tx_buf = {.buf = &cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	err = flash_at25xv021a_write_enable(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_spi_write(dev, &config->spi, &tx);
	if (err != 0) {
		return err;
	}

	/* Ensure device is idle before configuring WP pin. */
	err = flash_at25xv021a_wait_for_idle(dev, config->timeout);
	if (err != 0) {
		return err;
	}

#if ANY_DEV_HAS_WP_GPIO
	err = gpio_pin_configure_dt(&config->wp_gpio, GPIO_OUTPUT_ACTIVE);
	if (err < 0) {
		LOG_ERR("unable to set WP GPIO");
		return err;
	}
#endif /* ANY_DEV_HAS_WP_GPIOS */

	err = flash_at25xv021a_check_status(dev, DEV_SR_SPRL, &status);
	if (err != 0) {
		return err;
	}

	if (status != DEV_SR_SPRL) {
		LOG_ERR("unable to lock hardware");
		return -EIO;
	}

	return err;
}

static int flash_at25xv021a_hardware_unlock(const struct device *dev)
{
	int err;
	uint8_t status;
	uint8_t cmd[2] = {DEV_WRITE_SR, DEV_HW_UNLOCK};
	const struct flash_at25xv021a_config *config = dev->config;
	const struct spi_buf tx_buf = {.buf = &cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	/* Ensure device is idle before configuring WP pin. */
	err = flash_at25xv021a_wait_for_idle(dev, config->timeout);
	if (err != 0) {
		return err;
	}

#if ANY_DEV_HAS_WP_GPIO
	err = gpio_pin_configure_dt(&config->wp_gpio, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("unable to set WP GPIO");
		return err;
	}
#endif /* ANY_DEV_HAS_WP_GPIO */

	err = flash_at25xv021a_write_enable(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_spi_write(dev, &config->spi, &tx);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_check_status(dev, DEV_SR_SPRL, &status);
	if (err != 0) {
		return err;
	}

	if (status == DEV_SR_SPRL) {
		LOG_ERR("unable to unlock hardware");
		return -EIO;
	}

	return err;
}

static int flash_at25xv021a_global_protection(const struct device *dev, uint8_t protection_cmd)
{
	int err;
	uint8_t expected_status, status;
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[2] = {DEV_WRITE_SR, protection_cmd};
	const struct spi_buf tx_buf = {.buf = &cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	err = flash_at25xv021a_hardware_unlock(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_write_enable(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_spi_write(dev, &config->spi, &tx);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_hardware_lock(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_check_status(dev, DEV_SR_SWP, &status);
	if (err != 0) {
		return err;
	}

	expected_status = (protection_cmd == DEV_GLOBAL_PROTECT) ? DEV_SR_SWP : 0;
	if (status != expected_status) {
		LOG_ERR("unable to update global protection");
		return -EIO;
	}

	return err;
}

static int flash_at25xv021a_software_protection(const struct device *dev, off_t page,
						uint8_t protection_cmd)
{
	int err;
	uint8_t status;
	uint8_t unexpected_status;
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[4] = {
		protection_cmd,
		FIELD_GET(GENMASK(23, 16), page),
		FIELD_GET(GENMASK(15, 8), page),
		FIELD_GET(GENMASK(7, 0), page),
	};
	const struct spi_buf tx_buf = {.buf = &cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	err = flash_at25xv021a_hardware_unlock(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_write_enable(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_spi_write(dev, &config->spi, &tx);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_hardware_lock(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_check_status(dev, DEV_SR_SWP, &status);
	if (err != 0) {
		return err;
	}

	unexpected_status = (protection_cmd == DEV_PROTECT) ? 0 : DEV_SR_SWP;
	if (status == unexpected_status) {
		LOG_ERR("failed to update software protection for %s", dev->name);
		return -EIO;
	}

	return err;
}

static int flash_at25xv021a_hardware_init(const struct device *dev)
{
	int err;
	uint8_t status;

	err = flash_at25xv021a_hardware_unlock(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_global_protection(dev, DEV_GLOBAL_PROTECT);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_hardware_lock(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_check_status(dev, (DEV_SR_SPRL | DEV_SR_SWP), &status);
	if (err != 0) {
		return err;
	}

	if (status != (DEV_SR_SPRL | DEV_SR_SWP)) {
		LOG_ERR("unable to initialize hardware");
		return -EIO;
	}

	return err;
}

static int flash_at25xv021a_write_internal(const struct device *dev, off_t offset, const void *buf,
					   size_t len)
{
	int err;
	uint8_t status;
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[4] = {
		DEV_WRITE,
		FIELD_GET(GENMASK(23, 16), offset),
		FIELD_GET(GENMASK(15, 8), offset),
		FIELD_GET(GENMASK(7, 0), offset),
	};
	const struct spi_buf tx_bufs[2] = {
		{.buf = cmd, .len = ARRAY_SIZE(cmd)},
		{.buf = (void *)buf, .len = len},
	};
	const struct spi_buf_set tx = {.buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs)};

	err = flash_at25xv021a_write_enable(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_spi_write(dev, &config->spi, &tx);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_check_status(dev, DEV_SR_EPE, &status);
	if (err != 0) {
		return err;
	}

	if (status != 0) {
		LOG_ERR("failed to program %s", dev->name);
		return -EIO;
	}

	return err;
}

static int flash_at25xv021a_process_write(const struct device *dev, off_t offset, const void *buf,
					  size_t len)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;
	off_t page_start = ROUND_DOWN(offset, config->page_size);

	err = flash_at25xv021a_software_protection(dev, page_start, DEV_UNPROTECT);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_write_internal(dev, offset, buf, len);
	if (err != 0) {
		return err;
	}

	return flash_at25xv021a_software_protection(dev, page_start, DEV_PROTECT);
}

static int flash_at25xv021a_chip_erase(const struct device *dev)
{
	int err;
	uint8_t status;
	uint8_t cmd[1] = {DEV_CHIP_ERASE};
	const struct flash_at25xv021a_config *config = dev->config;
	const struct spi_buf tx_buf = {.buf = &cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	err = flash_at25xv021a_global_protection(dev, DEV_GLOBAL_UNPROTECT);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_write_enable(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_spi_write(dev, &config->spi, &tx);
	if (err != 0) {
		return err;
	}

	/* Need to wait extra time for chip erase. */
	err = flash_at25xv021a_wait_for_idle(dev, config->timeout_erase);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_global_protection(dev, DEV_GLOBAL_PROTECT);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_check_status(dev, DEV_SR_EPE, &status);
	if (err != 0) {
		return err;
	}

	if (status != 0) {
		LOG_ERR("failed to erase %s", dev->name);
		return -EIO;
	}

	return err;
}

static int flash_at25xv021a_erase_internal(const struct device *dev, uint32_t addr)
{
	int err;
	uint8_t status;
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[4] = {
		DEV_PAGE_ERASE,
		FIELD_GET(GENMASK(9, 8), addr),
		FIELD_GET(GENMASK(7, 0), addr),
		DEV_DUMMY_BYTE,
	};
	const struct spi_buf tx_buf = {.buf = &cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	err = flash_at25xv021a_write_enable(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_spi_write(dev, &config->spi, &tx);
	if (err != 0) {
		return err;
	}

	/* Page erase operations can take up to 20 milliseconds. */
	err = flash_at25xv021a_wait_for_idle(dev, config->timeout_erase);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_check_status(dev, DEV_SR_EPE, &status);
	if (err != 0) {
		return err;
	}

	if (status != 0) {
		LOG_ERR("unable to erase from %s", dev->name);
		return -EIO;
	}

	return err;
}

static int flash_at25xv021a_process_erase(const struct device *dev, off_t *offset)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;
	uint32_t page_addr = *offset / config->page_size;

	err = flash_at25xv021a_software_protection(dev, *offset, DEV_UNPROTECT);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_erase_internal(dev, page_addr);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_software_protection(dev, *offset, DEV_PROTECT);
	if (err != 0) {
		return err;
	}

	*offset += config->page_size;

	return err;
}

static int flash_at25xv021a_write(const struct device *dev, off_t offset, const void *buf,
				  size_t len)
{
	int err;
	struct flash_at25xv021a_data *data = dev->data;
	const struct flash_at25xv021a_config *config = dev->config;

	if (config->read_only) {
		LOG_ERR("attempted to write to read-only device %s", dev->name);
		return -EINVAL;
	}

	if (len == 0) {
		LOG_DBG("attempted to write 0 bytes to %s", dev->name);
		return 0;
	}

	if (len > config->page_size) {
		LOG_ERR("attempted to write more than page size in one write operation");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	err = flash_at25xv021a_process_write(dev, offset, buf, len);
	if (err != 0) {
		LOG_ERR("unable to complete write operation for %s", dev->name);
	}

	k_mutex_unlock(&data->lock);

	return err;
}

static int flash_at25xv021a_erase(const struct device *dev, off_t offset, size_t size)
{
	int err = 0;
	struct flash_at25xv021a_data *data = dev->data;
	const struct flash_at25xv021a_config *config = dev->config;

	if (config->read_only) {
		LOG_ERR("attempted to erase from read-only device %s", dev->name);
		return -EINVAL;
	}

	if (size == 0) {
		LOG_DBG("attempted to erase 0 bytes from %s", dev->name);
		return 0;
	}

	if (offset % config->page_size != 0 || size % config->page_size != 0) {
		LOG_ERR("offset and/or size is not aligned to page size in %s erase", dev->name);
		return -EINVAL;
	}

	if (offset + size > config->size) {
		LOG_ERR("attempted to erase beyond %s size boundary: %u", dev->name, config->size);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (offset == 0 && size == config->size) {
		err = flash_at25xv021a_chip_erase(dev);
		k_mutex_unlock(&data->lock);
		return err;
	}

	while (size != 0) {

		err = flash_at25xv021a_process_erase(dev, &offset);
		if (err != 0) {
			LOG_ERR("unable to complete erase operation for %s", dev->name);
			break;
		}

		size -= config->page_size;
	}

	k_mutex_unlock(&data->lock);

	return err;
}
#else
static int flash_at25xv021a_write(const struct device *dev, off_t offset, const void *buf,
				  size_t len)
{
	LOG_ERR("attempted to write to read-only device %s", dev->name);

	return -EINVAL;
}

static int flash_at25xv021a_erase(const struct device *dev, off_t offset, size_t size)
{
	LOG_ERR("attempted to erase from read-only device %s", dev->name);

	return -EINVAL;
}
#endif /* ANY_DEV_WRITEABLE */

static int flash_at25xv021a_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_at25xv021a_config *config = dev->config;

	*size = (uint64_t)config->size;

	return 0;
}

static const struct flash_parameters *flash_at25xv021a_get_parameters(const struct device *dev)
{
	const struct flash_at25xv021a_config *config = dev->config;

	return &config->parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_at25xv021a_pages_layout(const struct device *dev,
					  const struct flash_pages_layout **layout,
					  size_t *layout_size)
{
	const struct flash_at25xv021a_config *config = dev->config;

	*layout = &config->pages_layout;
	*layout_size = 1;
}
#endif /* defined(CONFIG_FLASH_PAGE_LAYOUT) */

#ifdef CONFIG_PM_DEVICE
static int flash_at25xv021a_resume(const struct device *dev)
{
	int err;
	uint8_t cmd[1] = {DEV_RESUME};
	const struct flash_at25xv021a_config *config = dev->config;
	const struct spi_buf tx_bufs = {.buf = cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_bufs, .count = 1};

	/* If in ultra deep sleep mode, we can send any command. Don't check return status. */
	(void)spi_write_dt(&config->spi, &tx);

	/* Device takes a minimum of 70 microseconds to exit ultra deep sleep mode. */
	k_msleep(1);

	err = flash_at25xv021a_verify_device(dev);
	if (err != 0) {
		LOG_ERR("failed to resume %s", dev->name);
	}

	return err;
}

static int flash_at25xv021a_suspend(const struct device *dev)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = {config->ultra_deep_sleep ? DEV_ULTRA_DEEP_SLEEP : DEV_DEEP_SLEEP};
	const struct spi_buf tx_bufs = {.buf = cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_bufs, .count = 1};

	/* Longer timeout in case suspend is called during a chip erase operation. */
	err = flash_at25xv021a_wait_for_idle(dev, config->timeout_erase);
	if (err != 0) {
		return err;
	}

	return flash_at25xv021a_spi_write(dev, &config->spi, &tx);
}

static int flash_at25xv021a_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return flash_at25xv021a_resume(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return flash_at25xv021a_suspend(dev);
	case PM_DEVICE_ACTION_TURN_OFF:
		__fallthrough;
	case PM_DEVICE_ACTION_TURN_ON:
		__fallthrough;
	default:
		break;
	}

	return -ENOTSUP;
}
#endif /* CONFIG_PM_DEVICE */

static int flash_at25xv021a_init(const struct device *dev)
{
	int err;
	struct flash_at25xv021a_data *data = dev->data;
	const struct flash_at25xv021a_config *config = dev->config;

	err = k_mutex_init(&data->lock);
	if (err != 0) {
		LOG_ERR("unable to initialize mutex");
		return err;
	}

	if (!device_is_ready(config->spi.bus)) {
		LOG_ERR("spi bus is not ready");
		return -ENODEV;
	}

#ifdef CONFIG_PM_DEVICE
	/* Resume if device was previously suspended. */
	err = flash_at25xv021a_resume(dev);
	if (err != 0) {
		return err;
	}
#endif /* CONFIG_PM_DEVICE */

	err = flash_at25xv021a_verify_device(dev);
	if (err != 0) {
		LOG_ERR("unable to verify device information");
		return err;
	}

#if ANY_DEV_WRITEABLE && ANY_DEV_HAS_WP_GPIO
	if (!device_is_ready(config->wp_gpio.port)) {
		LOG_ERR("device controlling WP GPIO is not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->wp_gpio)) {
		LOG_ERR("WP GPIO is not ready");
		return -ENODEV;
	}
#endif /* ANY_DEV_WRITEABLE && ANY_DEV_HAS_WP_GPIO */

#if ANY_DEV_WRITEABLE
	return flash_at25xv021a_hardware_init(dev);
#else
	return 0;
#endif /* ANY_DEV_WRITEABLE */
}

static DEVICE_API(flash, spi_flash_at25xv021a_api) = {
	.read = flash_at25xv021a_read,
	.write = flash_at25xv021a_write,
	.erase = flash_at25xv021a_erase,
	.get_size = flash_at25xv021a_get_size,
	.get_parameters = flash_at25xv021a_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_at25xv021a_pages_layout,
#endif
};

#define ASSERT_SIZE(sz) BUILD_ASSERT(sz > 0, "Size must be positive")

#define ASSERT_PAGE_SIZE(pg)                                                                       \
	BUILD_ASSERT(IS_POWER_OF_TWO(pg), "Page size must be positive and a power of 2")

#define ASSERT_TIMEOUTS(timeout, timeout_erase)                                                    \
	BUILD_ASSERT((timeout > 0) && (timeout_erase > 0), "Timeouts must be positive")

#define SPI_OP (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))

#define PAGE_SIZE_IF_WRITEABLE(inst)                                                               \
	COND_CODE_0(DT_INST_NODE_PROP_OR(inst, read_only, false),	\
		(.page_size = DT_INST_PROP(inst, page_size),),	\
		(EMPTY))

#define TIMEOUT_ERASE_IF_WRITEABLE(inst)                                                           \
	COND_CODE_0(DT_INST_NODE_PROP_OR(inst, read_only, false),	\
		(.timeout_erase = K_MSEC(DT_INST_PROP(inst, timeout_erase)),),	\
		(EMPTY))

#define WP_GPIO_IF_PROVIDED(inst)                                                                  \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, wp_gpios),	\
		(.wp_gpio = GPIO_DT_SPEC_INST_GET(inst, wp_gpios),))

#define PAGES_LAYOUT_IF_ENABLED(inst)                                                              \
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (.pages_layout = {	\
		.pages_count = DT_INST_PROP(inst, size) / DT_INST_PROP(inst, page_size),\
		.pages_size = DT_INST_PROP(inst, page_size),	\
	},))

#define SPI_FLASH_AT25XV021A_DEFINE(inst)                                                          \
                                                                                                   \
	ASSERT_SIZE(DT_INST_PROP(inst, size));                                                     \
	ASSERT_PAGE_SIZE(DT_INST_PROP(inst, page_size));                                           \
	ASSERT_TIMEOUTS(DT_INST_PROP(inst, timeout), DT_INST_PROP(inst, timeout_erase));           \
                                                                                                   \
	static const struct flash_at25xv021a_config flash_at25xv021a_config_##inst = {             \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP, 0),                                      \
		.jedec_id = DT_INST_PROP(inst, jedec_id),                                          \
		.size = DT_INST_PROP(inst, size),                                                  \
		.timeout = K_MSEC(DT_INST_PROP(inst, timeout)),                                    \
		.read_only = DT_INST_PROP(inst, read_only),                                        \
		.ultra_deep_sleep = DT_INST_PROP(inst, ultra_deep_sleep),                          \
		.parameters =                                                                      \
			{                                                                          \
				.write_block_size = DT_INST_PROP(inst, page_size),                 \
				.erase_value = 0xff,                                               \
			},                                                                         \
		WP_GPIO_IF_PROVIDED(inst) PAGES_LAYOUT_IF_ENABLED(inst)                            \
			PAGE_SIZE_IF_WRITEABLE(inst) TIMEOUT_ERASE_IF_WRITEABLE(inst)};            \
                                                                                                   \
	static struct flash_at25xv021a_data flash_at25xv021a_data_##inst;                          \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, flash_at25xv021a_pm_action);                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, flash_at25xv021a_init, PM_DEVICE_DT_INST_GET(inst),            \
			      &flash_at25xv021a_data_##inst, &flash_at25xv021a_config_##inst,      \
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &spi_flash_at25xv021a_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_FLASH_AT25XV021A_DEFINE)
