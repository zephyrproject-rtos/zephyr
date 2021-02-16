/*
 * Copyright (c) 2021 Caspar Friedrich <c.s.w.friedrich@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <devicetree.h>
#include <devicetree/gpio.h>
#include <drivers/eeprom.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <logging/log.h>

#define DT_DRV_COMPAT cypress_fm25w256

/*
 * 256-Kbit ferroelectric random access memory logically organized as 32K × 8 bit
 */
#define SIZE KB(32)
#define WORD_SIZE 8

/*
 * Get the upper seven address bits and mask them properly.
 */
#define OFFSET_HI(offset) ((offset >> 8) & BIT_MASK(7))

/*
 * Get the lower eight address bits and mask them properly.
 * Redundant shift is kept for consistency.
 */
#define OFFSET_LO(offset) ((offset >> 0) & BIT_MASK(8))

LOG_MODULE_REGISTER(fm25w256, CONFIG_EEPROM_LOG_LEVEL);

enum {
	OPCODE_WREN = 0b00000110,
	OPCODE_WRDI = 0b00000100,
	OPCODE_RDSR = 0b00000101,
	OPCODE_WRSR = 0b00000001,
	OPCODE_READ = 0b00000011,
	OPCODE_WRITE = 0b00000010,
};

enum {
	STATUS_WEL = 1,
	STATUS_BP0 = 2,
	STATUS_BP1 = 3,
	STATUS_WPEN = 7,
};

struct fm25w256_config {
	struct spi_dt_spec spi_spec;
	struct gpio_dt_spec wp_spec;
	bool read_only;
};

struct fm25w256_data {
	struct k_mutex lock;
};

static inline const struct fm25w256_config *get_config(const struct device *dev)
{
	return dev->config;
}

static inline struct fm25w256_data *get_data(const struct device *dev)
{
	return dev->data;
}

static inline int enable_write_protect(const struct device *dev)
{
	if (!get_config(dev)->wp_spec.port) {
		return 0;
	}

	return gpio_pin_set_dt(&get_config(dev)->wp_spec, 1);
}

static inline int disable_write_protect(const struct device *dev)
{
	if (!get_config(dev)->wp_spec.port) {
		return 0;
	}

	return gpio_pin_set_dt(&get_config(dev)->wp_spec, 0);
}

static int fm25w256_write_enable_latch(const struct device *dev)
{
	uint8_t opcode[] = { OPCODE_WREN };
	const struct spi_buf buf[] = {
		{
			.buf = opcode,
			.len = sizeof(opcode),
		},
	};

	struct spi_buf_set tx = {
		.buffers = buf,
		.count = 1,
	};

	return spi_write_dt(&get_config(dev)->spi_spec, &tx);
}

static int fm25w256_memory_operation(const struct device *dev, off_t offset, void *buf,
				     size_t buf_len, bool is_write)
{
	int ret;

	uint8_t opcode[] = {
		is_write ? OPCODE_WRITE : OPCODE_READ,
	};

	uint8_t addr_buf[] = {
		OFFSET_HI(offset),
		OFFSET_LO(offset),
	};

	const struct spi_buf spi_buf[] = {
		{
			.buf = opcode,
			.len = sizeof(opcode),
		},
		{
			.buf = addr_buf,
			.len = sizeof(addr_buf),
		},
		{
			.buf = buf,
			.len = buf_len,
		},
	};

	struct spi_buf_set tx_buf_set = {
		.buffers = spi_buf,
		.count = is_write ? ARRAY_SIZE(spi_buf) : ARRAY_SIZE(spi_buf) - 1,
	};

	struct spi_buf_set rx_buf_set = {
		.buffers = spi_buf,
		.count = ARRAY_SIZE(spi_buf),
	};

	if (is_write && get_config(dev)->read_only) {
		LOG_ERR("Device is configured read only");
		return -EACCES;
	}

	if (offset < 0 || offset > SIZE) {
		LOG_ERR("Offset out of range: %d", offset);
		return -EINVAL;
	}

	if (offset + buf_len > SIZE) {
		LOG_ERR("Memory roll over");
		return -EINVAL;
	}

	k_mutex_lock(&get_data(dev)->lock, K_FOREVER);

	if (is_write) {
		ret = fm25w256_write_enable_latch(dev);
		if (ret) {
			LOG_ERR("Unable to set write enable latch: %d", ret);
			k_mutex_unlock(&get_data(dev)->lock);
			return ret;
		}
	}

	ret = spi_transceive_dt(&get_config(dev)->spi_spec, &tx_buf_set,
				is_write ? NULL : &rx_buf_set);
	if (ret) {
		LOG_ERR("SPI transceive failed: %d", ret);
	}

	k_mutex_unlock(&get_data(dev)->lock);

	return ret;
}

static int fm25w256_init(const struct device *dev)
{
	int ret;
	uint8_t opcode[] = { OPCODE_WRSR };
	uint8_t status[] = { 0 };

	const struct spi_buf spi_buf[] = {
		{ .buf = opcode, .len = sizeof(opcode) },
		{ .buf = status, .len = sizeof(status) },
	};

	struct spi_buf_set tx_buf_set = {
		.buffers = spi_buf,
		.count = ARRAY_SIZE(spi_buf),
	};

	ret = k_mutex_init(&get_data(dev)->lock);
	if (ret) {
		LOG_ERR("Unable initialize mutex: %d", ret);
		return ret;
	}

	/*
	 * Write protect via GPIO is optional. Only configure if set.
	 */
	if (get_config(dev)->wp_spec.port) {
		if (!device_is_ready(get_config(dev)->wp_spec.port)) {
			LOG_ERR("write protect port not ready");
			return -EINVAL;
		}

		ret = gpio_pin_configure_dt(&get_config(dev)->wp_spec, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("Unable to configure write protect pin: %d", ret);
			return ret;
		}

		/*
		 * Enable Write Protect Pin fuctionality in status register as well
		 */
		WRITE_BIT(status[0], STATUS_WPEN, 1);
	}

	ret = fm25w256_write_enable_latch(dev);
	if (ret) {
		LOG_ERR("Unable to set write enable latch: %d", ret);
		return ret;
	}

	/*
	 * Write status register defaults since BP0, BP1 and WPEN are
	 * non-volatile. This disables memory write protection since it's not
	 * supported by the eeprom API. This also enables the Write Protect Pin
	 * functionality if one is configured.
	 */
	ret = spi_transceive_dt(&get_config(dev)->spi_spec, &tx_buf_set, NULL);
	if (ret) {
		LOG_ERR("SPI transceive failed: %d", ret);
		return ret;
	}

	ret = enable_write_protect(dev);
	if (ret) {
		LOG_ERR("Unable to set write protect pin: %d", ret);
		return ret;
	}

	return 0;
}

static int fm25w256_read(const struct device *dev, off_t offset, void *buf, size_t buf_len)
{
	LOG_DBG("About to read %u bytes from 0x%04x", buf_len, offset);
	return fm25w256_memory_operation(dev, offset, buf, buf_len, false);
}

static int fm25w256_write(const struct device *dev, off_t offset, const void *buf, size_t buf_len)
{
	LOG_DBG("About to write %u bytes to 0x%04x", buf_len, offset);
	return fm25w256_memory_operation(dev, offset, (void *)buf, buf_len, true);
}

static size_t fm25w256_size(const struct device *dev)
{
	return SIZE;
}

static const struct eeprom_driver_api api = {
	.read = fm25w256_read,
	.write = fm25w256_write,
	.size = fm25w256_size,
};

#define _OPERATION(n)                                                                              \
	COND_CODE_1(DT_INST_PROP(n, spi_mode_3),                                                   \
		    SPI_WORD_SET(WORD_SIZE) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA,    \
		    SPI_WORD_SET(WORD_SIZE) | SPI_TRANSFER_MSB)

#define INIT(n)                                                                                    \
	static const struct fm25w256_config inst_##n##_config = {                                  \
		.spi_spec = SPI_DT_SPEC_INST_GET(n, _OPERATION(n), 0),                             \
		.wp_spec = GPIO_DT_SPEC_INST_GET_OR(n, wp_gpios, { 0 }),                           \
		.read_only = DT_INST_PROP(n, read_only),                                           \
	};                                                                                         \
	static struct fm25w256_data inst_##n##_data = { 0 };                                       \
	DEVICE_DT_INST_DEFINE(n, &fm25w256_init, NULL, &inst_##n##_data, &inst_##n##_config,       \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(INIT)
