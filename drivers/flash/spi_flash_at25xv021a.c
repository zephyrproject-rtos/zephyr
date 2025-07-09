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

#define GET_PAGE_SIZE(inst) DT_PROP(inst, page_size)
#define DEV_PAGE_SIZE       (DT_FOREACH_STATUS_OKAY(atmel_at25xv021a, GET_PAGE_SIZE))

#define WP_GPIOS(inst) DT_NODE_HAS_PROP(inst, wp_gpios) ||
#define HAS_WP_GPIOS   (DT_FOREACH_STATUS_OKAY(atmel_at25xv021a, WP_GPIOS) 0)

enum flash_at25xv021a_timeout {
	DEV_TIMEOUT,
	DEV_TIMEOUT_CHIP_ERASE,
};

struct flash_at25xv021a_config {
	struct spi_dt_spec spi;
#if HAS_WP_GPIOS
	struct gpio_dt_spec wp_gpio;
#endif /* HAS_WP_GPIOS */
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout pages_layout;
#endif /* defined(CONFIG_FLASH_PAGE_LAYOUT) */
	uint8_t jedec_id[3];
	size_t size;
	size_t page_size;
	size_t addr_width;
	int64_t timeout[2];
	bool read_only;
	bool ultra_deep_sleep;
};

struct flash_at25xv021a_data {
	struct k_mutex lock;
};

static const struct flash_parameters flash_at25xv021a_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff,
};

static int flash_at25xv021a_read_status(const struct device *dev, uint8_t *status)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[2] = {DEV_READ_SR, DEV_DUMMY_BYTE};
	uint8_t sr[2];
	const struct spi_buf tx_buf = {.buf = cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf rx_buf = {.buf = sr, .len = ARRAY_SIZE(sr)};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	err = spi_transceive_dt(&config->spi, &tx, &rx);
	if (err != 0) {
		LOG_ERR("unable to read status register from %s", dev->name);
		return err;
	}

	*status = sr[1];

	return err;
}

static int flash_at25xv021a_wait_for_idle(const struct device *dev,
					  enum flash_at25xv021a_timeout timeout_option)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;
	int64_t now, timeout;
	uint8_t status;

	timeout = config->timeout[timeout_option] + k_uptime_get();

	for (;;) {
		now = k_uptime_get();

		err = flash_at25xv021a_read_status(dev, &status);
		if (err != 0) {
			return err;
		}

		if (!(status & DEV_SR_BUSY)) {
			return 0;
		}

		if (now > timeout) {
			break;
		}

		k_msleep(1);
	}

	LOG_ERR("timed out waiting for %s to idle", dev->name);
	return -EBUSY;
}

static int flash_at25xv021a_spi_write(const struct device *dev, const struct spi_dt_spec *spi,
				      const struct spi_buf_set *tx)
{
	int err;

	err = flash_at25xv021a_wait_for_idle(dev, DEV_TIMEOUT);
	if (err != 0) {
		return err;
	}

	err = spi_write_dt(spi, tx);
	if (err != 0) {
		LOG_ERR("unable to write to %s\n", dev->name);
	}

	return err;
}

static int flash_at25xv021a_spi_transceive(const struct device *dev, const struct spi_dt_spec *spi,
					   const struct spi_buf_set *tx,
					   const struct spi_buf_set *rx)
{
	int err;

	err = flash_at25xv021a_wait_for_idle(dev, DEV_TIMEOUT);
	if (err != 0) {
		return err;
	}

	err = spi_transceive_dt(spi, tx, rx);
	if (err != 0) {
		LOG_ERR("unable to read from %s\n", dev->name);
	}

	return err;
}

static int flash_at25xv021a_check_status(const struct device *dev, uint8_t mask)
{
	int err;
	uint8_t status;

	err = flash_at25xv021a_wait_for_idle(dev, DEV_TIMEOUT);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_read_status(dev, &status);
	if (err != 0) {
		return err;
	}

	return status & mask;
}

static int flash_at25xv021a_write_enable(const struct device *dev)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = {DEV_WRITE_ENABLE};
	const struct spi_buf tx_buf = {.buf = &cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	err = flash_at25xv021a_spi_write(dev, &config->spi, &tx);
	if (err != 0) {
		return err;
	}

	err = !flash_at25xv021a_check_status(dev, DEV_SR_WEL);
	if (err != 0) {
		LOG_ERR("unable to enable writes on %s", dev->name);
	}

	return err;
}

static int flash_at25xv021a_hardware_lock(const struct device *dev)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[2] = {DEV_WRITE_SR, DEV_HW_LOCK};
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
	err = flash_at25xv021a_wait_for_idle(dev, DEV_TIMEOUT);
	if (err != 0) {
		return err;
	}

#if HAS_WP_GPIOS
	err = gpio_pin_configure_dt(&config->wp_gpio, GPIO_OUTPUT_ACTIVE);
	if (err != 0) {
		LOG_ERR("unable to set WP GPIO");
		return err;
	}
#endif /* HAS_WP_GPIOS */

	err = !flash_at25xv021a_check_status(dev, DEV_SR_SPRL);
	if (err != 0) {
		LOG_ERR("unable to lock hardware");
	}

	return err;
}

static int flash_at25xv021a_hardware_unlock(const struct device *dev)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[2] = {DEV_WRITE_SR, DEV_HW_UNLOCK};
	const struct spi_buf tx_buf = {.buf = &cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	/* Ensure device is idle before configuring WP pin. */
	err = flash_at25xv021a_wait_for_idle(dev, DEV_TIMEOUT);
	if (err != 0) {
		return err;
	}

#if HAS_WP_GPIOS
	err = gpio_pin_configure_dt(&config->wp_gpio, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		LOG_ERR("unable to set WP GPIO");
		return err;
	}
#endif /* HAS_WP_GPIOS */

	err = flash_at25xv021a_write_enable(dev);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_spi_write(dev, &config->spi, &tx);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_check_status(dev, DEV_SR_SPRL);
	if (err != 0) {
		LOG_ERR("unable to unlock hardware");
	}

	return err;
}

static int flash_at25xv021a_global_protection(const struct device *dev, uint8_t protection_cmd)
{
	int err;
	uint8_t expected;
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

	expected = (protection_cmd == DEV_GLOBAL_PROTECT) ? DEV_SR_SWP : 0;
	err = !(flash_at25xv021a_check_status(dev, DEV_SR_SWP) == expected);
	if (err != 0) {
		LOG_ERR("unable to update global protection");
	}

	return err;
}

static int flash_at25xv021a_software_protection(const struct device *dev, off_t page,
						uint8_t protection_cmd)
{
	int err = 0;
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

	if (protection_cmd == DEV_PROTECT) {
		err = !flash_at25xv021a_check_status(dev, DEV_SR_SWP);
	} else {
		err = (flash_at25xv021a_check_status(dev, DEV_SR_SWP) == DEV_SR_SWP);
	}

	if (err != 0) {
		LOG_ERR("failed to update software protection for %s", dev->name);
	}

	return err;
}

static int flash_at25xv021a_device_info(const struct device *dev)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = {DEV_READ_DEVICE_INFO};
	uint8_t info[3];
	const struct spi_buf tx_buf = {.buf = cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
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

	if (len == 0) {
		LOG_WRN("attempted to read 0 bytes from %s", dev->name);
		return 0;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	err = flash_at25xv021a_read_internal(dev, offset, buf, len);
	if (err != 0) {
		return err;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

static int flash_at25xv021a_erase_internal(const struct device *dev, off_t addr)
{
	int err;
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

	err = flash_at25xv021a_check_status(dev, DEV_SR_EPE);
	if (err != 0) {
		LOG_ERR("unable to erase from %s", dev->name);
	}

	return err;
}

static int flash_at25xv021a_write_internal(const struct device *dev, off_t offset, const void *buf,
					   size_t len)
{
	int err;
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

	err = flash_at25xv021a_check_status(dev, DEV_SR_EPE);
	if (err != 0) {
		LOG_ERR("failed to program %s", dev->name);
	}

	return err;
}

static int flash_at25xv021a_write(const struct device *dev, off_t offset, const void *buf,
				  size_t len)
{
	int err, head = 0;
	off_t page_addr, page_start;
	size_t write_len;
	uint8_t copy[DEV_PAGE_SIZE];
	const struct flash_at25xv021a_config *config = dev->config;
	struct flash_at25xv021a_data *data = dev->data;

	if (config->read_only) {
		LOG_ERR("attempted to write to read-only device %s", dev->name);
		return -EINVAL;
	}

	if (len == 0) {
		LOG_WRN("attempted to write 0 bytes to %s", dev->name);
		return 0;
	}

	page_addr = offset / (off_t)config->page_size;
	page_start = ROUND_DOWN(offset, config->page_size);
	offset -= page_start;

	k_mutex_lock(&data->lock, K_FOREVER);

	while (len != 0) {
		write_len = MIN(len, config->page_size - offset);

		if (write_len < config->page_size) {
			err = flash_at25xv021a_read_internal(dev, page_start, (void *)copy,
							     config->page_size);
			if (err != 0) {
				break;
			}
		}

		err = flash_at25xv021a_software_protection(dev, page_start, DEV_UNPROTECT);
		if (err != 0) {
			break;
		}

		err = flash_at25xv021a_erase_internal(dev, page_addr);
		if (err != 0) {
			break;
		}

		if (write_len < config->page_size) {
			strncpy(&copy[offset], (const void *)((const uint8_t *)buf + head),
				write_len);

			err = flash_at25xv021a_write_internal(dev, page_start, (void *)copy,
							      config->page_size);
		} else {
			err = flash_at25xv021a_write_internal(
				dev, page_start, (const void *)((const uint8_t *)buf + head),
				config->page_size);
		}

		if (err != 0) {
			break;
		}

		err = flash_at25xv021a_software_protection(dev, page_start, DEV_PROTECT);
		if (err != 0) {
			break;
		}

		head += write_len;
		len -= write_len;
		page_addr += 1;
		page_start += config->page_size;
		offset = 0;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

static int flash_at25xv021a_chip_erase(const struct device *dev)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = {DEV_CHIP_ERASE};
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
	err = flash_at25xv021a_wait_for_idle(dev, DEV_TIMEOUT_CHIP_ERASE);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_global_protection(dev, DEV_GLOBAL_PROTECT);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_check_status(dev, DEV_SR_EPE);
	if (err != 0) {
		LOG_ERR("failed to erase %s", dev->name);
	}

	return err;
}

static int flash_at25xv021a_process_erase(const struct device *dev, off_t page_addr,
					  off_t page_start, off_t offset, size_t size,
					  size_t write_len, uint8_t *copy)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;

	err = flash_at25xv021a_software_protection(dev, page_start, DEV_UNPROTECT);
	if (err != 0) {
		return err;
	}

	if (write_len < config->page_size) {
		err = flash_at25xv021a_read_internal(dev, page_start, (void *)copy,
						     config->page_size);
		if (err != 0) {
			return err;
		}
	}

	err = flash_at25xv021a_erase_internal(dev, page_addr);
	if (err != 0) {
		return err;
	}

	if (write_len < config->page_size) {
		if (offset != 0) {
			err = flash_at25xv021a_write_internal(dev, page_start, (void *)copy,
							      offset);
		} else {
			err = flash_at25xv021a_write_internal(dev, page_start + size,
							      (void *)&copy[size],
							      config->page_size - size);
		}

		if (err != 0) {
			return err;
		}
	}

	err = flash_at25xv021a_software_protection(dev, page_start, DEV_PROTECT);

	return err;
}

static int flash_at25xv021a_erase(const struct device *dev, off_t offset, size_t size)
{
	int err = 0;
	off_t page_addr, page_start;
	size_t write_len;
	uint8_t copy[DEV_PAGE_SIZE];
	const struct flash_at25xv021a_config *config = dev->config;
	struct flash_at25xv021a_data *data = dev->data;

	if (config->read_only) {
		LOG_ERR("attempted to erase from read-only device %s", dev->name);
		return -EINVAL;
	}

	if (size == 0) {
		LOG_WRN("attempted to erase 0 bytes from %s", dev->name);
		return 0;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (size >= config->size) {
		err = flash_at25xv021a_chip_erase(dev);
		k_mutex_unlock(&data->lock);
		return err;
	}

	page_addr = offset / (off_t)config->page_size;
	page_start = ROUND_DOWN(offset, config->page_size);
	offset -= page_start;

	while (size != 0) {
		write_len = MIN(size, config->page_size - offset);

		err = flash_at25xv021a_process_erase(dev, page_addr, page_start, offset, size,
						     write_len, copy);
		if (err != 0) {
			break;
		}

		page_addr += 1;
		page_start += config->page_size;
		size -= write_len;
		offset = 0;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

static int flash_at25xv021a_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_at25xv021a_config *config = dev->config;

	*size = (uint64_t)config->size;

	return 0;
}

static const struct flash_parameters *flash_at25xv021a_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_at25xv021a_parameters;
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
	const struct flash_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = {DEV_RESUME};
	const struct spi_buf tx_bufs = {.buf = cmd, .len = ARRAY_SIZE(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_bufs, .count = 1};

	/* If in ultra deep sleep mode, we can send any command. Don't check return status. */
	(void)spi_write_dt(&config->spi, &tx);

	err = flash_at25xv021a_device_info(dev);
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
	err = flash_at25xv021a_wait_for_idle(dev, DEV_TIMEOUT_CHIP_ERASE);
	if (err != 0) {
		return err;
	}

	err = flash_at25xv021a_spi_write(dev, &config->spi, &tx);

	return err;
}

static int flash_at25xv021a_pm_action(const struct device *dev, enum pm_device_action action)
{
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		err = flash_at25xv021a_resume(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		err = flash_at25xv021a_suspend(dev);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		__fallthrough;
	case PM_DEVICE_ACTION_TURN_ON:
		__fallthrough;
	default:
		return -ENOTSUP;
	}

	return err;
}
#endif /* CONFIG_PM_DEVICE */

static int flash_at25xv021a_hardware_init(const struct device *dev)
{
	int err;

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

	err = !(flash_at25xv021a_check_status(dev, DEV_SR_SWP) == DEV_SR_SWP);
	if (err != 0) {
		LOG_ERR("unable to initialize hardware");
	}

	return err;
}

static int flash_at25xv021a_init(const struct device *dev)
{
	int err;
	const struct flash_at25xv021a_config *config = dev->config;
	struct flash_at25xv021a_data *data = dev->data;

	if (!device_is_ready(config->spi.bus)) {
		LOG_ERR("spi bus device is not ready");
		return -ENODEV;
	}

#ifdef CONFIG_PM_DEVICE
	/* Resume if device was previously suspended. */
	err = flash_at25xv021a_resume(dev);
	if (err != 0) {
		return err;
	}
#endif /* CONFIG_PM_DEVICE */

	err = flash_at25xv021a_device_info(dev);
	if (err != 0) {
		LOG_ERR("unable to verify device information");
		return err;
	}

#if HAS_WP_GPIOS
	if (!device_is_ready(config->wp_gpio.port)) {
		LOG_ERR("device controlling WP GPIO is not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->wp_gpio)) {
		LOG_ERR("WP GPIO is not ready");
		return -ENODEV;
	}
#endif /* HAS_WP_GPIOS */

	err = flash_at25xv021a_hardware_init(dev);
	if (err != 0) {
		return err;
	}

	err = k_mutex_init(&data->lock);
	if (err != 0) {
		LOG_ERR("unable to initialize mutex");
	}

	return err;
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

#define ASSERT_SIZE(sz) BUILD_ASSERT(sz > 0, "Size must be non-negative")

#define ASSERT_PAGE_SIZE(pg)                                                                       \
	BUILD_ASSERT((pg != 0U) && ((pg & (pg - 1)) == 0U), "Page size must be a power of 2")

#define SPI_OP (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))

#define WP_GPIOS_IF_PROVIDED(inst)                                                                 \
	IF_ENABLED(DT_NODE_HAS_PROP(inst, wp_gpios),	\
		(.wp_gpio = GPIO_DT_SPEC_GET(inst, wp_gpios),))

#define PAGES_LAYOUT_IF_PROVIDED(inst)                                                             \
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (.pages_layout = {	\
		.pages_count = DT_INST_PROP(inst, size) / DT_INST_PROP(inst, page_size),\
		.pages_size = DT_INST_PROP(inst, page_size),	\
	},))

#define SPI_FLASH_AT25XV021A_DEFINE(inst)                                                          \
                                                                                                   \
	ASSERT_SIZE(DT_INST_PROP(inst, size));                                                     \
	ASSERT_PAGE_SIZE(DT_INST_PROP(inst, page_size));                                           \
                                                                                                   \
	static const struct flash_at25xv021a_config flash_at25xv021a_config_##inst = {             \
		.spi = SPI_DT_SPEC_GET(DT_INST(inst, atmel_at25xv021a), SPI_OP, 0),                \
		WP_GPIOS_IF_PROVIDED(DT_INST(inst, atmel_at25xv021a))                              \
			PAGES_LAYOUT_IF_PROVIDED(inst)                                             \
				.jedec_id = DT_INST_PROP(inst, jedec_id),                          \
		.size = DT_INST_PROP(inst, size),                                                  \
		.page_size = DT_INST_PROP(inst, page_size),                                        \
		.timeout = DT_INST_PROP(inst, timeout),                                            \
		.ultra_deep_sleep = DT_INST_PROP(inst, ultra_deep_sleep),                          \
		.read_only = DT_INST_PROP(inst, read_only),                                        \
	};                                                                                         \
                                                                                                   \
	static struct flash_at25xv021a_data flash_at25xv021a_data_##inst;                          \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, flash_at25xv021a_pm_action);                                \
                                                                                                   \
	DEVICE_DT_DEFINE(DT_INST(inst, atmel_at25xv021a), flash_at25xv021a_init,                   \
			 PM_DEVICE_DT_INST_GET(inst), &flash_at25xv021a_data_##inst,               \
			 &flash_at25xv021a_config_##inst, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, \
			 &spi_flash_at25xv021a_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_FLASH_AT25XV021A_DEFINE)
