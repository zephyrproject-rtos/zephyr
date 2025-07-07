/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for AT25XV021A SPI EEPROMs, a variant of Atmel's AT25 family
 */

#define DT_DRV_COMPAT atmel_at25xv021a

#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eeprom_at25xv021a);

/* AT25XV021A opcodes */
#define EEPROM_AT25_READ                (0x0b)
#define EEPROM_AT25_PAGE_ERASE          (0x81)
#define EEPROM_AT25_CHIP_ERASE          (0x60)
#define EEPROM_AT25_WRITE               (0x02)
#define EEPROM_AT25_WRITE_ENABLE        (0x06)
#define EEPROM_AT25_PROTECT_SECTOR      (0x36)
#define EEPROM_AT25_UNPROTECT_SECTOR    (0x39)
#define EEPROM_AT25_READ_SR             (0x05)
#define EEPROM_AT25_WRITE_SR            (0x01)
#define EEPROM_AT25_READ_DEVICE_INFO    (0x9f)
#define EEPROM_AT25_SLEEP               (0xb9)
#define EEPROM_AT25_WAKEUP              (0xab)
#define EEPROM_AT25_DEEP_SLEEP          (0x79)

/* AT25XV021A driver instruction set */
#define EEPROM_AT25_DUMMY_BYTE          (0x00)
#define EEPROM_AT25_HW_LOCK             (0xf8)
#define EEPROM_AT25_HW_UNLOCK           (0x00)
#define EEPROM_AT25_GLOBAL_PROTECT      (0x7f)
#define EEPROM_AT25_GLOBAL_UNPROTECT    (0x00)

/* AT25XV021A status register masks */
#define EEPROM_AT25_SR_BUSY             (1U << 0)
#define EEPROM_AT25_SR_WEL              (1U << 1)
#define EEPROM_AT25_SR_SWP              (3U << 2)
#define EEPROM_AT25_SR_WPP              (1U << 4)
#define EEPROM_AT25_SR_EPE              (1U << 5)
#define EEPROM_AT25_SR_SPRL             (1U << 7)

/* AT25XV021A device information */
#define EEPROM_AT25_MANUFACTURER_ID     (0x1f)
#define EEPROM_AT25_DEVICE_ID_1         (0x43)
#define EEPROM_AT25_DEVICE_ID_2         (0x01)

/* AT25XV021A parameters */
#define EEPROM_AT25_MAX_WAIT_TIME_MS    (4000)
#define EEPROM_AT25_PAGE_SIZE           (256)

#define WP_GPIOS(inst)                  DT_NODE_HAS_PROP(inst, wp_gpios) ||
#define HAS_WP_GPIOS                    (DT_FOREACH_STATUS_OKAY(atmel_at25xv021a, WP_GPIOS) 0)

struct eeprom_at25xv021a_config {
	struct spi_dt_spec spi;
#if HAS_WP_GPIOS
	struct gpio_dt_spec wp_gpio;
#endif /* HAS_WP_GPIOS */
	size_t size;
	size_t page_size;
	uint8_t addr_width;
	bool read_only;
	int64_t timeout;
};

struct eeprom_at25xv021a_data {
	struct k_mutex lock;
};

static int eeprom_at25xv021a_read_status(const struct device *dev, uint8_t *status)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[2] = { EEPROM_AT25_READ_SR, EEPROM_AT25_DUMMY_BYTE };
	uint8_t sr[2];
	const struct spi_buf tx_buf = { .buf = cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf rx_buf = { .buf = sr, .len = ARRAY_SIZE(sr) };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	err = spi_transceive_dt(&config->spi, &tx, &rx);
	if (err) {
		LOG_ERR("unable to read status register from %s", dev->name);
		return err;
	}

	*status = sr[1];

	return err;
}

static int eeprom_at25xv021a_wait_for_idle(const struct device *dev, bool forever)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	int64_t now, timeout;
	uint8_t status;

	timeout = forever ? EEPROM_AT25_MAX_WAIT_TIME_MS : config->timeout;
	timeout += k_uptime_get();

	for (;;) {
		now = k_uptime_get();

		err = eeprom_at25xv021a_read_status(dev, &status);
		if (err) {
			return err;
		}

		if (!(status & EEPROM_AT25_SR_BUSY)) {
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

static int eeprom_at25xv021a_check_status(const struct device *dev, uint8_t mask)
{
	int err;
	uint8_t status;

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_read_status(dev, &status);
	if (err) {
		return err;
	}

	return status & mask;
}

static int eeprom_at25xv021a_write_enable(const struct device *dev)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = { EEPROM_AT25_WRITE_ENABLE };
	const struct spi_buf tx_buf = { .buf = &cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	err = eeprom_at25xv021a_check_status(dev, EEPROM_AT25_SR_WEL);
	if (!err) {
		LOG_ERR("unable to enable writes on %s", dev->name);
		return err;
	}

	return !err;
}

static int eeprom_at25xv021a_global_protect(const struct device *dev)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[2] = { EEPROM_AT25_WRITE_SR, EEPROM_AT25_GLOBAL_PROTECT };
	const struct spi_buf tx_buf = { .buf = &cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	err = eeprom_at25xv021a_write_enable(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	err = !(eeprom_at25xv021a_check_status(dev, EEPROM_AT25_SR_SWP) == EEPROM_AT25_SR_SWP);
	if (err) {
		LOG_ERR("unable to perform global protect");
	}

	return err;
}

static int eeprom_at25xv021a_global_unprotect(const struct device *dev)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[2] = { EEPROM_AT25_WRITE_SR, EEPROM_AT25_GLOBAL_UNPROTECT };
	const struct spi_buf tx_buf = { .buf = &cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	err = eeprom_at25xv021a_write_enable(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	err = !(eeprom_at25xv021a_check_status(dev, EEPROM_AT25_SR_SWP) == 0);
	if (err) {
		LOG_ERR("unable to perform global unprotect");
	}

	return err;
}

static int eeprom_at25xv021a_hardware_lock(const struct device *dev)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[2] = { EEPROM_AT25_WRITE_SR, EEPROM_AT25_HW_LOCK };
	const struct spi_buf tx_buf = { .buf = &cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	err = eeprom_at25xv021a_write_enable(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	err = !eeprom_at25xv021a_check_status(dev, EEPROM_AT25_SR_SPRL);
	if (err) {
		LOG_ERR("unable to lock hardware");
		return err;
	}

#if HAS_WP_GPIOS
	err = gpio_pin_set_dt(&config->wp_gpio, GPIO_OUTPUT_LOW);
	if (err) {
		LOG_ERR("unable to set WP GPIO");
	}
#endif /* HAS_WP_GPIOS */

	return err;
}

static int eeprom_at25xv021a_hardware_unlock(const struct device *dev)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[2] = { EEPROM_AT25_WRITE_SR, EEPROM_AT25_HW_UNLOCK };
	const struct spi_buf tx_buf = { .buf = &cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

#if HAS_WP_GPIOS
	err = gpio_pin_set_dt(&config->wp_gpio, GPIO_OUTPUT_HIGH);
	if (err) {
		LOG_ERR("unable to set WP GPIO");
		return err;
	}
#endif /* HAS_WP_GPIOS */

	err = eeprom_at25xv021a_write_enable(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	err = eeprom_at25xv021a_check_status(dev, EEPROM_AT25_SR_SPRL);
	if (err) {
		LOG_ERR("unable to unlock hardware");
	}

	return err;
}

static int eeprom_at25xv021a_hardware_init(const struct device *dev)
{
	int err;

	err = eeprom_at25xv021a_hardware_unlock(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_global_protect(dev);
	if (err) {
		return err;
	}

	err = !(eeprom_at25xv021a_check_status(dev, EEPROM_AT25_SR_SWP) == EEPROM_AT25_SR_SWP);
	if (err) {
		LOG_ERR("unable to initialize hardware");
		return err;
	}

	return eeprom_at25xv021a_hardware_lock(dev);
}

static int eeprom_at25xv021a_software_protection(const struct device *dev, off_t page,
	bool protect)
{
	int err = 0;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[4] = {
		protect ? EEPROM_AT25_PROTECT_SECTOR : EEPROM_AT25_UNPROTECT_SECTOR,
		(page & GENMASK(23, 16)) >> 16,
		(page & GENMASK(15, 8)) >> 8,
		(page & GENMASK(7, 0)) >> 0,
	};
	const struct spi_buf tx_buf = { .buf = &cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	err = eeprom_at25xv021a_hardware_unlock(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_write_enable(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	err = eeprom_at25xv021a_hardware_lock(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_check_status(dev, EEPROM_AT25_SR_SWP);
	err = protect ? !err : err == EEPROM_AT25_SR_SWP;
	if (err) {
		LOG_ERR("failed to update software protection for %s", dev->name);
	}

	return err;
}

static int eeprom_at25xv021a_device_info(const struct device *dev)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = { EEPROM_AT25_READ_DEVICE_INFO };
	uint8_t info[3];
	const struct spi_buf tx_buf = { .buf = cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1, };
	const struct spi_buf rx_bufs[2] = {
		{ .buf = NULL, .len = ARRAY_SIZE(cmd) },
		{ .buf = info, .len = ARRAY_SIZE(info) },
	};
	const struct spi_buf_set rx = { .buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs) };

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = spi_transceive_dt(&config->spi, &tx, &rx);
	if (err) {
		LOG_ERR("unable to read device information from %s", dev->name);
		return err;
	}

	if ((info[0] != EEPROM_AT25_MANUFACTURER_ID) ||
		(info[1] != EEPROM_AT25_DEVICE_ID_1) ||
		(info[2] != EEPROM_AT25_DEVICE_ID_2)) {
		return -ENODEV;
	}

	return err;
}

static int eeprom_at25xv021a_read_internal(const struct device *dev, off_t offset, void *buf,
	size_t len)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[5] = {
		EEPROM_AT25_READ,
		(offset & GENMASK(23, 16)) >> 16,
		(offset & GENMASK(15, 8)) >> 8,
		(offset & GENMASK(7, 0)) >> 0,
		EEPROM_AT25_DUMMY_BYTE,
	};
	const struct spi_buf tx_buf = { .buf = cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1, };
	const struct spi_buf rx_bufs[2] = {
		{ .buf = NULL, .len = ARRAY_SIZE(cmd) },
		{ .buf = buf,  .len = len },
	};
	const struct spi_buf_set rx = { .buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs) };

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = spi_transceive_dt(&config->spi, &tx, &rx);
	if (err) {
		LOG_ERR("unable to read from %s", dev->name);
	}

	return err;
}

static int eeprom_at25xv021a_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	int err;
	struct eeprom_at25xv021a_data *data = dev->data;

	if (!len) {
		LOG_WRN("attempted to read 0 bytes from %s", dev->name);
		return 0;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	err = eeprom_at25xv021a_read_internal(dev, offset, buf, len);
	if (err) {
		return err;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

static int eeprom_at25xv021a_page_erase(const struct device *dev, off_t page_addr)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[4] = {
		EEPROM_AT25_PAGE_ERASE,
		(page_addr & GENMASK(9, 8)) >> 8,
		(page_addr & GENMASK(7, 0)) >> 0,
		EEPROM_AT25_DUMMY_BYTE,
	};
	const struct spi_buf tx_buf = { .buf = &cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	err = eeprom_at25xv021a_write_enable(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	err = eeprom_at25xv021a_check_status(dev, EEPROM_AT25_SR_EPE);
	if (err) {
		LOG_ERR("unable to erase from %s", dev->name);
	}

	return err;
}

static int eeprom_at25xv021a_write_internal(const struct device *dev, off_t offset,
	const void *buf, size_t len)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[4] = {
		EEPROM_AT25_WRITE,
		(offset & GENMASK(23, 16)) >> 16,
		(offset & GENMASK(15, 8)) >> 8,
		(offset & GENMASK(7, 0)) >> 0,
	};
	const struct spi_buf tx_bufs[2] = {
		{ .buf = cmd, .len = ARRAY_SIZE(cmd) },
		{ .buf = (void *)buf, .len = len },
	};
	const struct spi_buf_set tx = { .buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs) };

	err = eeprom_at25xv021a_write_enable(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	err = eeprom_at25xv021a_check_status(dev, EEPROM_AT25_SR_EPE);
	if (err) {
		LOG_ERR("failed to program %s", dev->name);
	}

	return err;
}

static int eeprom_at25xv021a_write(const struct device *dev, off_t offset, const void *buf,
	size_t len)
{
	int err, head = 0;
	off_t page_addr, page_start;
	size_t write_len;
	uint8_t copy[EEPROM_AT25_PAGE_SIZE];
	const struct eeprom_at25xv021a_config *config = dev->config;
	struct eeprom_at25xv021a_data *data = dev->data;

	if (config->read_only) {
		LOG_ERR("attempted to write to read-only device %s", dev->name);
		return -EINVAL;
	}

	if (!len) {
		LOG_WRN("attempted to write 0 bytes to %s", dev->name);
		return 0;
	}

	page_addr = offset / (off_t)config->page_size;
	page_start = ROUND_DOWN(offset, config->page_size);
	offset -= page_start;

	k_mutex_lock(&data->lock, K_FOREVER);

	while (len) {
		err = eeprom_at25xv021a_read_internal(dev, page_start, (void *)copy,
			config->page_size);
		if (err) {
			break;
		}

		err = eeprom_at25xv021a_software_protection(dev, page_start, false);
		if (err) {
			break;
		}

		err = eeprom_at25xv021a_page_erase(dev, page_addr);
		if (err) {
			break;
		}

		write_len = MIN(len, config->page_size - offset);
		strncpy(&copy[offset], (uint8_t *)buf + head, write_len);

		err = eeprom_at25xv021a_write_internal(dev, page_start, (void *)copy,
			config->page_size);
		if (err) {
			break;
		}

		err = eeprom_at25xv021a_software_protection(dev, page_start, true);
		if (err) {
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

static size_t eeprom_at25xv021a_size(const struct device *dev)
{
	const struct eeprom_at25xv021a_config *config = dev->config;

	return config->size;
}

static int eeprom_at25xv021a_init(const struct device *dev)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	struct eeprom_at25xv021a_data *data = dev->data;

	if (!device_is_ready(config->spi.bus)) {
		LOG_ERR("spi bus device is not ready");
		return -ENODEV;
	}

	err = eeprom_at25xv021a_device_info(dev);
	if (err) {
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

	err = eeprom_at25xv021a_hardware_init(dev);
	if (err) {
		return err;
	}

	err = k_mutex_init(&data->lock);
	if (err) {
		LOG_ERR("unable to initialize mutex");
	}

	return err;
}

#ifdef CONFIG_PM_DEVICE
static int eeprom_at25xv021a_resume(const struct device *dev)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = { EEPROM_AT25_WAKEUP };
	const struct spi_buf tx_bufs = { .buf = cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_bufs, .count = 1 };

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	err = eeprom_at25xv021a_device_info(dev);
	if (err) {
		LOG_ERR("failed to resume %s", dev->name);
	}

	return err;
}

static int eeprom_at25xv021a_suspend(const struct device *dev)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = { EEPROM_AT25_SLEEP };
	const struct spi_buf tx_bufs = { .buf = cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_bufs, .count = 1 };

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	return err;
}

static int eeprom_at25xv021a_turn_off(const struct device *dev)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = { EEPROM_AT25_DEEP_SLEEP };
	const struct spi_buf tx_bufs = { .buf = cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_bufs, .count = 1 };

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	return err;
}

static int eeprom_at25xv021a_turn_on(const struct device *dev)
{
	int err;
	uint8_t dummy_status;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = { EEPROM_AT25_DEEP_SLEEP };
	const struct spi_buf tx_bufs = { .buf = cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_bufs, .count = 1 };

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s", dev->name);
		return err;
	}

	/* Dummy function needed to write an opcode to the device */
	eeprom_at25xv021a_read_status(dev, &dummy_status);

	err = eeprom_at25xv021a_device_info(dev);
	if (err) {
		LOG_ERR("failed to wake up from deep sleep");
	}

	return err;
}

static int eeprom_at25xv021a_pm_action(const struct device *dev, enum pm_device_action action)
{
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		err = eeprom_at25xv021a_resume(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		err = eeprom_at25xv021a_suspend(dev);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		err = eeprom_at25xv021a_turn_off(dev);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		err = eeprom_at25xv021a_turn_on(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return err;
}
#endif /* CONFIG_PM_DEVICE */

/* Expose chip erase function */
int eeprom_at25xv021a_chip_erase(const struct device *dev)
{
	int err;
	const struct eeprom_at25xv021a_config *config = dev->config;
	uint8_t cmd[1] = { EEPROM_AT25_CHIP_ERASE };
	const struct spi_buf tx_buf = { .buf = &cmd, .len = ARRAY_SIZE(cmd) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	err = eeprom_at25xv021a_hardware_unlock(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_global_unprotect(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_write_enable(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_wait_for_idle(dev, false);
	if (err) {
		return err;
	}

	err = spi_write_dt(&config->spi, &tx);
	if (err) {
		LOG_ERR("unable to write to %s\n", dev->name);
		return err;
	}

	/* Chip erase can take up to 4 seconds so wait longer */
	err = eeprom_at25xv021a_wait_for_idle(dev, true);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_global_protect(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_hardware_lock(dev);
	if (err) {
		return err;
	}

	err = eeprom_at25xv021a_check_status(dev, EEPROM_AT25_SR_EPE);
	if (err) {
		LOG_ERR("failed to erase %s", dev->name);
	}

	return err;
}

static DEVICE_API(eeprom, eeprom_at25xv021a_api) = {
	.read = eeprom_at25xv021a_read,
	.write = eeprom_at25xv021a_write,
	.size = eeprom_at25xv021a_size,
};

#define ASSERT_SIZE(sz) BUILD_ASSERT(sz > 0, "Size must be non-negative")

#define ASSERT_PAGE_SIZE(pg) BUILD_ASSERT((pg != 0U) && ((pg & (pg - 1)) == 0U),	\
	"Page size must be a power of 2")

#define ASSERT_ADDRESS_WIDTH(width) BUILD_ASSERT((width == 8) || (width == 16) ||	\
	(width == 24), "Address width must be 8, 16, or 24 bits")

#define ASSERT_TIMEOUT(timeout) BUILD_ASSERT(timeout > 0, "Timeout must be non-negative")

#define SPI_OP (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))

#define WP_GPIOS_IF_PROVIDED(inst)	\
	IF_ENABLED(DT_NODE_HAS_PROP(inst, wp_gpios),	\
		(.wp_gpio = GPIO_DT_SPEC_GET(inst, wp_gpios),))

#define THIS_INST(inst) DT_INST(inst, atmel_at25xv021a)
#define EEPROM_AT25XV021A_DEFINE(inst)	\
	ASSERT_SIZE(DT_PROP(THIS_INST(inst), size));	\
	ASSERT_PAGE_SIZE(DT_PROP(THIS_INST(inst), pagesize));	\
	ASSERT_ADDRESS_WIDTH(DT_PROP(THIS_INST(inst), address_width));	\
	ASSERT_TIMEOUT(DT_PROP(THIS_INST(inst), timeout));	\
	static const struct eeprom_at25xv021a_config eeprom_at25xv021a_config_##inst = {	\
		.spi = SPI_DT_SPEC_GET(THIS_INST(inst), SPI_OP, 0),	\
		WP_GPIOS_IF_PROVIDED(THIS_INST(inst))	\
		.size = DT_PROP(THIS_INST(inst), size),	\
		.page_size = DT_PROP(THIS_INST(inst), pagesize),	\
		.addr_width = DT_PROP(THIS_INST(inst), address_width),	\
		.read_only = DT_PROP(THIS_INST(inst), read_only),	\
		.timeout = DT_PROP(THIS_INST(inst), timeout),	\
	};  \
	static struct eeprom_at25xv021a_data eeprom_at25xv021a_data_##inst;	\
	PM_DEVICE_DT_INST_DEFINE(inst, eeprom_at25xv021a_pm_action);	\
	DEVICE_DT_DEFINE(THIS_INST(inst), eeprom_at25xv021a_init, PM_DEVICE_DT_INST_GET(inst),	\
		&eeprom_at25xv021a_data_##inst, &eeprom_at25xv021a_config_##inst, POST_KERNEL,	\
		CONFIG_EEPROM_INIT_PRIORITY, &eeprom_at25xv021a_api);

DT_INST_FOREACH_STATUS_OKAY(EEPROM_AT25XV021A_DEFINE)
