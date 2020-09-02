/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Atmel AT24 I2C and Atmel AT25 SPI EEPROMs.
 */

#include <drivers/eeprom.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(eeprom_at2x);

/* AT25 instruction set */
#define EEPROM_AT25_WRSR  0x01U /* Write STATUS register        */
#define EEPROM_AT25_WRITE 0x02U /* Write data to memory array   */
#define EEPROM_AT25_READ  0x03U /* Read data from memory array  */
#define EEPROM_AT25_WRDI  0x04U /* Reset the write enable latch */
#define EEPROM_AT25_RDSR  0x05U /* Read STATUS register         */
#define EEPROM_AT25_WREN  0x06U /* Set the write enable latch   */

/* AT25 status register bits */
#define EEPROM_AT25_STATUS_WIP BIT(0) /* Write-In-Process   (RO) */
#define EEPROM_AT25_STATUS_WEL BIT(1) /* Write Enable Latch (RO) */
#define EEPROM_AT25_STATUS_BP0 BIT(2) /* Block Protection 0 (RW) */
#define EEPROM_AT25_STATUS_BP1 BIT(3) /* Block Protection 1 (RW) */

struct eeprom_at2x_config {
	const char *bus_dev_name;
	uint16_t bus_addr;
	uint32_t max_freq;
	const char *spi_cs_dev_name;
	gpio_pin_t spi_cs_pin;
	gpio_dt_flags_t spi_cs_dt_flags;
	gpio_pin_t wp_gpio_pin;
	gpio_dt_flags_t wp_gpio_flags;
	const char *wp_gpio_name;
	size_t size;
	size_t pagesize;
	uint8_t addr_width;
	bool readonly;
	uint16_t timeout;
	eeprom_api_read read_fn;
	eeprom_api_write write_fn;
};

struct eeprom_at2x_data {
	const struct device *bus_dev;
#ifdef CONFIG_EEPROM_AT25
	struct spi_config spi_cfg;
	struct spi_cs_control spi_cs;
#endif /* CONFIG_EEPROM_AT25 */
	const struct device *wp_gpio_dev;
	struct k_mutex lock;
};

static inline int eeprom_at2x_write_protect(const struct device *dev)
{
	const struct eeprom_at2x_config *config = dev->config;
	struct eeprom_at2x_data *data = dev->data;

	if (!data->wp_gpio_dev) {
		return 0;
	}

	return gpio_pin_set(data->wp_gpio_dev, config->wp_gpio_pin, 1);
}

static inline int eeprom_at2x_write_enable(const struct device *dev)
{
	const struct eeprom_at2x_config *config = dev->config;
	struct eeprom_at2x_data *data = dev->data;

	if (!data->wp_gpio_dev) {
		return 0;
	}

	return gpio_pin_set(data->wp_gpio_dev, config->wp_gpio_pin, 0);
}

static int eeprom_at2x_read(const struct device *dev, off_t offset, void *buf,
			    size_t len)
{
	const struct eeprom_at2x_config *config = dev->config;
	struct eeprom_at2x_data *data = dev->data;
	int err;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	err = config->read_fn(dev, offset, buf, len);
	k_mutex_unlock(&data->lock);

	if (err) {
		LOG_ERR("failed to read EEPROM (err %d)", err);
		return err;
	}

	return 0;
}

static size_t eeprom_at2x_limit_write_count(const struct device *dev,
					    off_t offset,
					    size_t len)
{
	const struct eeprom_at2x_config *config = dev->config;
	size_t count = len;
	off_t page_boundary;

	/* We can at most write one page at a time */
	if (count > config->pagesize) {
		count = config->pagesize;
	}

	/* Writes can not cross a page boundary */
	page_boundary = ROUND_UP(offset + 1, config->pagesize);
	if (offset + count > page_boundary) {
		count = page_boundary - offset;
	}

	return count;
}

static int eeprom_at2x_write(const struct device *dev, off_t offset,
			     const void *buf,
			     size_t len)
{
	const struct eeprom_at2x_config *config = dev->config;
	struct eeprom_at2x_data *data = dev->data;
	const uint8_t *pbuf = buf;
	int ret;

	if (config->readonly) {
		LOG_WRN("attempt to write to read-only device");
		return -EACCES;
	}

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = eeprom_at2x_write_enable(dev);
	if (ret) {
		LOG_ERR("failed to write-enable EEPROM (err %d)", ret);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	while (len) {
		ret = config->write_fn(dev, offset, pbuf, len);
		if (ret < 0) {
			LOG_ERR("failed to write to EEPROM (err %d)", ret);
			eeprom_at2x_write_protect(dev);
			k_mutex_unlock(&data->lock);
			return ret;
		}

		pbuf += ret;
		offset += ret;
		len -= ret;
	}

	ret = eeprom_at2x_write_protect(dev);
	if (ret) {
		LOG_ERR("failed to write-protect EEPROM (err %d)", ret);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static size_t eeprom_at2x_size(const struct device *dev)
{
	const struct eeprom_at2x_config *config = dev->config;

	return config->size;
}

#ifdef CONFIG_EEPROM_AT24
static int eeprom_at24_read(const struct device *dev, off_t offset, void *buf,
			    size_t len)
{
	const struct eeprom_at2x_config *config = dev->config;
	struct eeprom_at2x_data *data = dev->data;
	int64_t timeout;
	uint8_t addr[2];
	int err;

	if (config->addr_width == 16) {
		sys_put_be16(offset, addr);
	} else {
		addr[0] = offset & BIT_MASK(8);
	}

	/*
	 * A write cycle may be in progress so reads must be attempted
	 * until the current write cycle should be completed.
	 */
	timeout = k_uptime_get() + config->timeout;
	while (1) {
		int64_t now = k_uptime_get();
		err = i2c_write_read(data->bus_dev, config->bus_addr,
				     addr, config->addr_width / 8,
				     buf, len);
		if (!err || now > timeout) {
			break;
		}
		k_sleep(K_MSEC(1));
	}

	return err;
}

static int eeprom_at24_write(const struct device *dev, off_t offset,
			     const void *buf, size_t len)
{
	const struct eeprom_at2x_config *config = dev->config;
	struct eeprom_at2x_data *data = dev->data;
	int count = eeprom_at2x_limit_write_count(dev, offset, len);
	uint8_t block[config->addr_width / 8 + count];
	int64_t timeout;
	int i = 0;
	int err;

	/*
	 * Not all I2C EEPROMs support repeated start so the the
	 * address (offset) and data (buf) must be provided in one
	 * write transaction (block).
	 */
	if (config->addr_width == 16) {
		block[i++] = offset >> 8;
	}
	block[i++] = offset;
	memcpy(&block[i], buf, count);

	/*
	 * A write cycle may already be in progress so writes must be
	 * attempted until the previous write cycle should be
	 * completed.
	 */
	timeout = k_uptime_get() + config->timeout;
	while (1) {
		int64_t now = k_uptime_get();
		err = i2c_write(data->bus_dev, block, sizeof(block),
				config->bus_addr);
		if (!err || now > timeout) {
			break;
		}
		k_sleep(K_MSEC(1));
	}

	if (err < 0) {
		return err;
	}

	return count;
}
#endif /* CONFIG_EEPROM_AT24 */

#ifdef CONFIG_EEPROM_AT25
static int eeprom_at25_rdsr(const struct device *dev, uint8_t *status)
{
	struct eeprom_at2x_data *data = dev->data;
	uint8_t rdsr[2] = { EEPROM_AT25_RDSR, 0 };
	uint8_t sr[2];
	int err;
	const struct spi_buf tx_buf = {
		.buf = rdsr,
		.len = sizeof(rdsr),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_buf = {
		.buf = sr,
		.len = sizeof(sr),
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};

	err = spi_transceive(data->bus_dev, &data->spi_cfg, &tx, &rx);
	if (!err) {
		*status = sr[1];
	}

	return err;
}

static int eeprom_at25_wait_for_idle(const struct device *dev)
{
	const struct eeprom_at2x_config *config = dev->config;
	int64_t timeout;
	uint8_t status;
	int err;

	timeout = k_uptime_get() + config->timeout;
	while (1) {
		int64_t now = k_uptime_get();
		err = eeprom_at25_rdsr(dev, &status);
		if (err) {
			LOG_ERR("Could not read status register (err %d)", err);
			return err;
		}

		if (!(status & EEPROM_AT25_STATUS_WIP)) {
			return 0;
		}
		if (now > timeout) {
			break;
		}
		k_sleep(K_MSEC(1));
	}

	return -EBUSY;
}

static int eeprom_at25_read(const struct device *dev, off_t offset, void *buf,
			    size_t len)
{
	const struct eeprom_at2x_config *config = dev->config;
	struct eeprom_at2x_data *data = dev->data;
	size_t cmd_len = 1 + config->addr_width / 8;
	uint8_t cmd[4] = { EEPROM_AT25_READ, 0, 0, 0 };
	uint8_t *paddr;
	int err;
	const struct spi_buf tx_buf = {
		.buf = cmd,
		.len = cmd_len,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_bufs[2] = {
		{
			.buf = NULL,
			.len = cmd_len,
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

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	paddr = &cmd[1];
	switch (config->addr_width) {
	case 24:
		*paddr++ = offset >> 16;
		__fallthrough;
	case 16:
		*paddr++ = offset >> 8;
		__fallthrough;
	case 8:
		*paddr++ = offset;
		break;
	default:
		__ASSERT(0, "invalid address width");
	}

	err = eeprom_at25_wait_for_idle(dev);
	if (err) {
		LOG_ERR("EEPROM idle wait failed (err %d)", err);
		k_mutex_unlock(&data->lock);
		return err;
	}

	return spi_transceive(data->bus_dev, &data->spi_cfg, &tx, &rx);
}

static int eeprom_at25_wren(const struct device *dev)
{
	struct eeprom_at2x_data *data = dev->data;
	uint8_t cmd = EEPROM_AT25_WREN;
	const struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = 1,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	return spi_write(data->bus_dev, &data->spi_cfg, &tx);
}

static int eeprom_at25_write(const struct device *dev, off_t offset,
			     const void *buf, size_t len)
{
	const struct eeprom_at2x_config *config = dev->config;
	struct eeprom_at2x_data *data = dev->data;
	int count = eeprom_at2x_limit_write_count(dev, offset, len);
	uint8_t cmd[4] = { EEPROM_AT25_WRITE, 0, 0, 0 };
	size_t cmd_len = 1 + config->addr_width / 8;
	uint8_t *paddr;
	int err;
	const struct spi_buf tx_bufs[2] = {
		{
			.buf = cmd,
			.len = cmd_len,
		},
		{
			.buf = (void *)buf,
			.len = count,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	paddr = &cmd[1];
	switch (config->addr_width) {
	case 24:
		*paddr++ = offset >> 16;
		__fallthrough;
	case 16:
		*paddr++ = offset >> 8;
		__fallthrough;
	case 8:
		*paddr++ = offset;
		break;
	default:
		__ASSERT(0, "invalid address width");
	}

	err = eeprom_at25_wait_for_idle(dev);
	if (err) {
		LOG_ERR("EEPROM idle wait failed (err %d)", err);
		return err;
	}

	err = eeprom_at25_wren(dev);
	if (err) {
		LOG_ERR("failed to disable write protection (err %d)", err);
		return err;
	}

	err = spi_transceive(data->bus_dev, &data->spi_cfg, &tx, NULL);
	if (err) {
		return err;
	}

	return count;
}
#endif /* CONFIG_EEPROM_AT25 */

static int eeprom_at2x_init(const struct device *dev)
{
	const struct eeprom_at2x_config *config = dev->config;
	struct eeprom_at2x_data *data = dev->data;
	int err;

	k_mutex_init(&data->lock);

	data->bus_dev = device_get_binding(config->bus_dev_name);
	if (!data->bus_dev) {
		LOG_ERR("could not get parent bus device");
		return -EINVAL;
	}

#ifdef CONFIG_EEPROM_AT25
	data->spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |
		SPI_WORD_SET(8);
	data->spi_cfg.frequency = config->max_freq;
	data->spi_cfg.slave = config->bus_addr;

	if (config->spi_cs_dev_name) {
		data->spi_cs.gpio_dev =
			device_get_binding(config->spi_cs_dev_name);
		if (!data->spi_cs.gpio_dev) {
			LOG_ERR("could not get SPI CS GPIO device");
			return -EINVAL;
		}

		data->spi_cs.gpio_pin = config->spi_cs_pin;
		data->spi_cs.gpio_dt_flags = config->spi_cs_dt_flags;
		data->spi_cfg.cs = &data->spi_cs;
	}
#endif /* CONFIG_EEPROM_AT25 */

	if (config->wp_gpio_name) {
		data->wp_gpio_dev = device_get_binding(config->wp_gpio_name);
		if (!data->wp_gpio_dev) {
			LOG_ERR("could not get WP GPIO device");
			return -EINVAL;
		}

		err = gpio_pin_configure(data->wp_gpio_dev, config->wp_gpio_pin,
					 GPIO_OUTPUT_ACTIVE | config->wp_gpio_flags);
		if (err) {
			LOG_ERR("failed to configure WP GPIO pin (err %d)",
				err);
			return err;
		}
	}

	return 0;
}

static const struct eeprom_driver_api eeprom_at2x_api = {
	.read = eeprom_at2x_read,
	.write = eeprom_at2x_write,
	.size = eeprom_at2x_size,
};

#define ASSERT_AT24_ADDR_W_VALID(w) \
	BUILD_ASSERT(w == 8U || w == 16U,		\
		     "Unsupported address width")

#define ASSERT_AT25_ADDR_W_VALID(w)			\
	BUILD_ASSERT(w == 8U || w == 16U || w == 24U,	\
		     "Unsupported address width")

#define ASSERT_PAGESIZE_IS_POWER_OF_2(page) \
	BUILD_ASSERT((page != 0U) && ((page & (page - 1)) == 0U),	\
		     "Page size is not a power of two")

#define ASSERT_SIZE_PAGESIZE_VALID(size, page)				\
	BUILD_ASSERT(size % page == 0U,					\
		     "Size is not an integer multiple of page size")

#define INST_DT_AT2X(inst, t) DT_INST(inst, atmel_at##t)

#define EEPROM_AT2X_DEVICE(n, t) \
	ASSERT_PAGESIZE_IS_POWER_OF_2(DT_PROP(INST_DT_AT2X(n, t), pagesize)); \
	ASSERT_SIZE_PAGESIZE_VALID(DT_PROP(INST_DT_AT2X(n, t), size), \
				   DT_PROP(INST_DT_AT2X(n, t), pagesize)); \
	ASSERT_AT##t##_ADDR_W_VALID(DT_PROP(INST_DT_AT2X(n, t), \
					    address_width)); \
	static const struct eeprom_at2x_config eeprom_at##t##_config_##n = { \
		.bus_dev_name = DT_BUS_LABEL(INST_DT_AT2X(n, t)), \
		.bus_addr = DT_REG_ADDR(INST_DT_AT2X(n, t)), \
		.max_freq = UTIL_AND( \
			DT_NODE_HAS_PROP(INST_DT_AT2X(n, t), \
					 spi_max_frequency), \
			DT_PROP(INST_DT_AT2X(n, t), spi_max_frequency)), \
		.spi_cs_dev_name = UTIL_AND( \
			DT_SPI_DEV_HAS_CS_GPIOS(INST_DT_AT2X(n, t)), \
			DT_SPI_DEV_CS_GPIOS_LABEL(INST_DT_AT2X(n, t))),	\
		.spi_cs_pin = UTIL_AND( \
			DT_SPI_DEV_HAS_CS_GPIOS(INST_DT_AT2X(n, t)), \
			DT_SPI_DEV_CS_GPIOS_PIN(INST_DT_AT2X(n, t))), \
		.spi_cs_dt_flags = UTIL_AND( \
			DT_SPI_DEV_HAS_CS_GPIOS(INST_DT_AT2X(n, t)), \
			DT_SPI_DEV_CS_GPIOS_FLAGS(INST_DT_AT2X(n, t))), \
		.wp_gpio_pin = UTIL_AND( \
			DT_NODE_HAS_PROP(INST_DT_AT2X(n, t), wp_gpios), \
			DT_GPIO_PIN(INST_DT_AT2X(n, t), wp_gpios)), \
		.wp_gpio_flags = UTIL_AND( \
			DT_NODE_HAS_PROP(INST_DT_AT2X(n, t), wp_gpios), \
			DT_GPIO_FLAGS(INST_DT_AT2X(n, t), wp_gpios)), \
		.wp_gpio_name = UTIL_AND( \
			DT_NODE_HAS_PROP(INST_DT_AT2X(n, t), wp_gpios), \
			DT_GPIO_LABEL(INST_DT_AT2X(n, t), wp_gpios)), \
		.size = DT_PROP(INST_DT_AT2X(n, t), size), \
		.pagesize = DT_PROP(INST_DT_AT2X(n, t), pagesize), \
		.addr_width = DT_PROP(INST_DT_AT2X(n, t), address_width), \
		.readonly = DT_PROP(INST_DT_AT2X(n, t), read_only), \
		.timeout = DT_PROP(INST_DT_AT2X(n, t), timeout), \
		.read_fn = eeprom_at##t##_read, \
		.write_fn = eeprom_at##t##_write, \
	}; \
	static struct eeprom_at2x_data eeprom_at##t##_data_##n; \
	DEVICE_AND_API_INIT(eeprom_at##t##_##n, \
			    DT_LABEL(INST_DT_AT2X(n, t)), \
			    &eeprom_at2x_init, &eeprom_at##t##_data_##n, \
			    &eeprom_at##t##_config_##n, POST_KERNEL, \
			    CONFIG_EEPROM_AT2X_INIT_PRIORITY, \
			    &eeprom_at2x_api)

#define EEPROM_AT24_DEVICE(n) EEPROM_AT2X_DEVICE(n, 24)
#define EEPROM_AT25_DEVICE(n) EEPROM_AT2X_DEVICE(n, 25)

#define CALL_WITH_ARG(arg, expr) expr(arg);

#define INST_DT_AT2X_FOREACH(t, inst_expr) \
	UTIL_LISTIFY(DT_NUM_INST_STATUS_OKAY(atmel_at##t),	\
		     CALL_WITH_ARG, inst_expr)

#ifdef CONFIG_EEPROM_AT24
INST_DT_AT2X_FOREACH(24, EEPROM_AT24_DEVICE);
#endif

#ifdef CONFIG_EEPROM_AT25
INST_DT_AT2X_FOREACH(25, EEPROM_AT25_DEVICE);
#endif
