/*
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPI/GPIO transaction layer for the SX128X. The command protocol is
 * opcode + address/offset + a dummy byte on reads, with busy-pin polling
 * around each transaction and optional IRQ pin interrupt support - see
 * sx128x_regs.h for the opcode values.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>

#include "sx128x.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx128x_hal, CONFIG_LORA_LOG_LEVEL);

#define SX128X_RESET_PULSE_MS       10
#define SX128X_RESET_WAIT_MS        10
#define SX128X_BUSY_DEFAULT_TIMEOUT 1000

/*
 * How many times to read BUSY back-to-back before falling back to delays.
 * Every command pays two of these waits (before, to know the chip can take
 * the command, and after, to know it finished), so at GFSK 2 Mb/s - where a
 * 127-byte packet is only ~550 us long - even a 5 us sleep per wait is a
 * measurable slice of the inter-packet gap. The longest BUSY a normal
 * command produces is the 80 us STDBY_RC->Tx transition (datasheet Table
 * 10-2), so spin through that, and only leave the delays for the slow paths
 * (reset, oscillator startup) where they do not cost throughput.
 */
#define SX128X_BUSY_SPIN_POLLS 512

static inline struct sx128x_hal_data *get_hal_data(const struct device *dev)
{
	struct sx128x_data *data = dev->data;

	return &data->hal;
}

static int spi_transfer(const struct spi_dt_spec *spi, const uint8_t *hdr, size_t hdr_len,
			uint8_t *data, size_t data_len, bool read)
{
	uint8_t rx_hdr[4];

	__ASSERT_NO_MSG(hdr_len <= sizeof(rx_hdr));

	struct spi_buf tx_bufs[] = {
		{.buf = (uint8_t *)hdr, .len = hdr_len},
		{.buf = read ? NULL : data, .len = data_len},
	};
	struct spi_buf rx_bufs[] = {
		{.buf = rx_hdr, .len = hdr_len},
		{.buf = data, .len = data_len},
	};
	int count = (data_len == 0) ? 1 : 2;
	struct spi_buf_set tx_set = {.buffers = tx_bufs, .count = count};
	struct spi_buf_set rx_set = {.buffers = rx_bufs, .count = count};

	return spi_transceive_dt(spi, &tx_set, read ? &rx_set : NULL);
}

int sx128x_hal_wait_busy(const struct device *dev, uint32_t timeout_ms)
{
	const struct sx128x_hal_config *config = dev->config;
	int64_t deadline;
	unsigned int polls = 0;
	int level;

	for (polls = 0; polls < SX128X_BUSY_SPIN_POLLS; polls++) {
		level = gpio_pin_get_dt(&config->busy);
		if (level < 0) {
			LOG_ERR("BUSY GPIO%u read failed: %d", (unsigned int)config->busy.pin,
				level);
			return level;
		}
		if (level == 0) {
			return 0;
		}
	}

	deadline = k_uptime_get() + timeout_ms;
	do {
		level = gpio_pin_get_dt(&config->busy);
		if (level < 0) {
			LOG_ERR("BUSY GPIO%u read failed: %d", (unsigned int)config->busy.pin,
				level);
			return level;
		}
		if (level == 0) {
			return 0;
		}
		if (polls++ < SX128X_BUSY_SPIN_POLLS + 200) {
			k_busy_wait(5);
		} else {
			k_msleep(1);
		}
	} while (k_uptime_get() < deadline);

	LOG_WRN("BUSY GPIO%u stayed high for %u ms", (unsigned int)config->busy.pin, timeout_ms);
	return -ETIMEDOUT;
}

int sx128x_hal_reset(const struct device *dev)
{
	const struct sx128x_hal_config *config = dev->config;
	int ret;

	if (config->reset.port == NULL) {
		/* NRESET has an internal pull-up; rely on power-on reset. */
		k_msleep(SX128X_RESET_WAIT_MS);
		return sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
	}

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		LOG_ERR("Failed to assert reset: %d", ret);
		return ret;
	}

	k_msleep(SX128X_RESET_PULSE_MS);

	ret = gpio_pin_set_dt(&config->reset, 0);
	if (ret < 0) {
		LOG_ERR("Failed to release reset: %d", ret);
		return ret;
	}

	k_msleep(SX128X_RESET_WAIT_MS);

	ret = sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Reset complete");
	return 0;
}

int sx128x_hal_wakeup(const struct device *dev)
{
	const struct sx128x_hal_config *config = dev->config;
	uint8_t buf[2] = {SX128X_CMD_GET_STATUS, 0x00};
	struct spi_buf tx_buf = {.buf = buf, .len = sizeof(buf)};
	struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	int ret;

	/*
	 * A write-only GET_STATUS command wakes the chip via the NSS falling
	 * edge. Use spi_write_dt() (TX-only): MISO is undefined while asleep.
	 */
	ret = spi_write_dt(&config->spi, &tx_set);
	if (ret < 0) {
		LOG_ERR("Wakeup SPI failed: %d", ret);
		return ret;
	}

	return sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
}

int sx128x_hal_write_cmd(const struct device *dev, uint8_t opcode, const uint8_t *data, size_t len)
{
	const struct sx128x_hal_config *config = dev->config;
	uint8_t hdr[1] = {opcode};
	int ret;

	ret = sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), (uint8_t *)data, len, false);
	if (ret < 0) {
		LOG_ERR("SPI write failed: %d", ret);
		return ret;
	}

	if (opcode != SX128X_CMD_SET_SLEEP) {
		ret = sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
	} else {
		k_busy_wait(500);
	}

	return ret;
}

int sx128x_hal_read_cmd(const struct device *dev, uint8_t opcode, uint8_t *data, size_t len)
{
	const struct sx128x_hal_config *config = dev->config;
	uint8_t hdr[2] = {opcode, 0x00};
	int ret;

	ret = sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	if (opcode == SX128X_CMD_GET_STATUS) {
		/* Unlike other read commands, status arrives during the opcode byte. */
		uint8_t rx = 0;
		struct spi_buf tx_buf = {.buf = &opcode, .len = 1};
		struct spi_buf rx_buf = {.buf = &rx, .len = 1};
		struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
		struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};

		if (len != 1) {
			return -EINVAL;
		}
		ret = spi_transceive_dt(&config->spi, &tx_set, &rx_set);
		if (ret == 0) {
			data[0] = rx;
		}
		return ret;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), data, len, true);
	if (ret < 0) {
		LOG_ERR("SPI transceive failed: %d", ret);
		return ret;
	}

	return 0;
}

int sx128x_hal_write_regs(const struct device *dev, uint16_t address, const uint8_t *data,
			  size_t len)
{
	const struct sx128x_hal_config *config = dev->config;
	uint8_t hdr[3];
	int ret;

	ret = sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	hdr[0] = SX128X_CMD_WRITE_REGISTER;
	sys_put_be16(address, &hdr[1]);

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), (uint8_t *)data, len, false);
	if (ret < 0) {
		LOG_ERR("SPI write regs failed: %d", ret);
		return ret;
	}

	return sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
}

int sx128x_hal_read_regs(const struct device *dev, uint16_t address, uint8_t *data, size_t len)
{
	const struct sx128x_hal_config *config = dev->config;
	uint8_t hdr[4];
	int ret;

	ret = sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	hdr[0] = SX128X_CMD_READ_REGISTER;
	sys_put_be16(address, &hdr[1]);
	hdr[3] = 0x00;

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), data, len, true);
	if (ret < 0) {
		LOG_ERR("SPI read regs failed: %d", ret);
		return ret;
	}

	return 0;
}

int sx128x_hal_write_buffer(const struct device *dev, uint8_t offset, const uint8_t *data,
			    size_t len)
{
	const struct sx128x_hal_config *config = dev->config;
	uint8_t hdr[2] = {SX128X_CMD_WRITE_BUFFER, offset};
	int ret;

	ret = sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), (uint8_t *)data, len, false);
	if (ret < 0) {
		LOG_ERR("SPI write buffer failed: %d", ret);
		return ret;
	}

	return sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
}

int sx128x_hal_read_buffer(const struct device *dev, uint8_t offset, uint8_t *data, size_t len)
{
	const struct sx128x_hal_config *config = dev->config;
	uint8_t hdr[3] = {SX128X_CMD_READ_BUFFER, offset, 0x00};
	int ret;

	ret = sx128x_hal_wait_busy(dev, SX128X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), data, len, true);
	if (ret < 0) {
		LOG_ERR("SPI read buffer failed: %d", ret);
		return ret;
	}

	return 0;
}

void sx128x_hal_set_rf_switch(const struct device *dev, bool enable, bool tx)
{
	const struct sx128x_hal_config *config = dev->config;

	if (config->tx_enable.port != NULL) {
		gpio_pin_set_dt(&config->tx_enable, enable && tx);
	}

	if (config->rx_enable.port != NULL) {
		gpio_pin_set_dt(&config->rx_enable, enable && !tx);
	}
}

static int configure_optional_gpio(const struct gpio_dt_spec *gpio, gpio_flags_t flags,
				   const char *name)
{
	int ret;

	if (gpio->port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(gpio)) {
		LOG_ERR("%s GPIO not ready", name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(gpio, flags);
	if (ret < 0) {
		LOG_ERR("Failed to configure %s: %d", name, ret);
		return ret;
	}

	return 0;
}

static void irq_isr(const struct device *gpio, struct gpio_callback *cb, uint32_t pins)
{
	struct sx128x_hal_data *data = CONTAINER_OF(cb, struct sx128x_hal_data, irq_cb);

	if (data->irq_callback != NULL) {
		data->irq_callback(data->dev);
	}
}

int sx128x_hal_set_irq_callback(const struct device *dev,
				void (*callback)(const struct device *dev))
{
	const struct sx128x_hal_config *config = dev->config;
	struct sx128x_hal_data *data = get_hal_data(dev);
	int ret;

	data->irq_callback = callback;
	if (config->irq.port == NULL) {
		return 0;
	}

	if (callback != NULL) {
		ret = gpio_pin_interrupt_configure_dt(&config->irq, GPIO_INT_EDGE_TO_ACTIVE);
	} else {
		ret = gpio_pin_interrupt_configure_dt(&config->irq, GPIO_INT_DISABLE);
	}

	return ret;
}

int sx128x_hal_init(const struct device *dev)
{
	const struct sx128x_hal_config *config = dev->config;
	struct sx128x_hal_data *data = get_hal_data(dev);
	int ret;

	data->dev = dev;
	data->irq_callback = NULL;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	ret = configure_optional_gpio(&config->reset, GPIO_OUTPUT_INACTIVE, "reset");
	if (ret < 0) {
		return ret;
	}

	if (!gpio_is_ready_dt(&config->busy)) {
		LOG_ERR("Busy GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->busy, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure busy GPIO: %d", ret);
		return ret;
	}

	ret = configure_optional_gpio(&config->irq, GPIO_INPUT, "IRQ pin");
	if (ret < 0) {
		return ret;
	}

	if (config->irq.port != NULL) {
		gpio_init_callback(&data->irq_cb, irq_isr, BIT(config->irq.pin));
		ret = gpio_add_callback(config->irq.port, &data->irq_cb);
		if (ret < 0) {
			LOG_ERR("Failed to add IRQ pin callback: %d", ret);
			return ret;
		}
	}

	ret = configure_optional_gpio(&config->tx_enable, GPIO_OUTPUT_INACTIVE, "TX enable");
	if (ret < 0) {
		return ret;
	}

	ret = configure_optional_gpio(&config->rx_enable, GPIO_OUTPUT_INACTIVE, "RX enable");
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("HAL initialized");
	return 0;
}
