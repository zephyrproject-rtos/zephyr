/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>

#include "lr11xx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lr11xx_hal, CONFIG_LORA_LOG_LEVEL);

/* Timing constants
 * Reset pulse = 100 uS from DS
 */
#define LR11XX_RESET_PULSE_MS		1
#define LR11XX_RESET_WAIT_MS		5
#define LR11XX_BUSY_DEFAULT_TIMEOUT	1000

static inline struct lr11xx_hal_data *get_hal_data(const struct device *dev)
{
	struct lr11xx_data *data = dev->data;

	return &data->hal;
}

int lr11xx_hal_wait_busy(const struct device *dev, uint32_t timeout_ms)
{
	if (!WAIT_FOR(!lr11xx_hal_is_busy(dev),
		      timeout_ms * 1000,
		      k_msleep(1))) {
		LOG_WRN("Busy timeout after %u ms", timeout_ms);
		return -ETIMEDOUT;
	}

	return 0;
}

static int spi_transfer(const struct device *dev,
			const uint8_t *hdr, size_t hdr_len,
			uint8_t *data, size_t data_len, bool read,
			uint8_t hdr_read[6])
{
	const struct lr11xx_hal_config *config = dev->config;
	int ret;
	/* Dummy buffers because some SPI drivers require a buffer
	 * and won't take NULL for 'use zeroes'
	 */
	uint8_t *hdr_buf = (uint8_t [6]) {0};
	uint8_t dum_buf;

	if (hdr_read != NULL) {
		hdr_buf = hdr_read;
	}

	__ASSERT_NO_MSG(hdr_len <= 6);

	if (read) {
		struct spi_buf tx_bufs[] = {
			{ .buf = (uint8_t *)hdr, .len = hdr_len },
		};
		struct spi_buf rx_bufs[] = {
			{ .buf = hdr_buf, .len = hdr_len },
		};
		struct spi_buf_set tx_set = { .buffers = tx_bufs, .count = 1 };
		struct spi_buf_set rx_set = { .buffers = rx_bufs, .count = 1 };

		ret = spi_transceive_dt(&config->spi, &tx_set, &rx_set);
		if (ret != 0) {
			return ret;
		}

		ret = lr11xx_hal_wait_busy(dev, LR11XX_BUSY_DEFAULT_TIMEOUT);
		if (ret != 0) {
			return ret;
		}

		if (data_len > 0) {
			struct spi_buf rx_bufs_rx[] = {
				/* Stat1 buffer always present in the response */
				{ .buf = &dum_buf, .len = 1 },
				{ .buf = data, .len = data_len },
			};
			rx_set.buffers = rx_bufs_rx;
			rx_set.count = 2;

			ret = spi_read_dt(&config->spi, &rx_set);
		}
	} else {
		int count = (data_len == 0) ? 1 : 2;

		struct spi_buf tx_bufs[] = {
			{ .buf = (uint8_t *)hdr, .len = hdr_len },
			{ .buf = data, .len = data_len },
		};
		struct spi_buf rx_bufs[] = {
			{ .buf = hdr_read, .len = hdr_len },
		};
		struct spi_buf_set tx_set = { .buffers = tx_bufs, .count = count };
		struct spi_buf_set rx_set = { .buffers = rx_bufs, .count = 1 };

		ret = spi_transceive_dt(&config->spi, &tx_set, &rx_set);
	}

	return ret;
}

int lr11xx_hal_reset(const struct device *dev)
{
	const struct lr11xx_hal_config *config = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&config->reset)) {
		LOG_ERR("Reset GPIO not ready");
		return -ENODEV;
	}

	/* Pull reset low */
	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		LOG_ERR("Failed to assert reset: %d", ret);
		return ret;
	}

	k_msleep(LR11XX_RESET_PULSE_MS);

	/* Release reset */
	ret = gpio_pin_set_dt(&config->reset, 0);
	if (ret < 0) {
		LOG_ERR("Failed to release reset: %d", ret);
		return ret;
	}

	k_msleep(LR11XX_RESET_WAIT_MS);

	/* Wait for chip to be ready */
	ret = lr11xx_hal_wait_busy(dev, LR11XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Reset complete");
	return 0;
}

bool lr11xx_hal_is_busy(const struct device *dev)
{
	const struct lr11xx_hal_config *config = dev->config;

	return gpio_pin_get_dt(&config->busy) != 0;
}

static void irq_isr(const struct device *gpio, struct gpio_callback *cb,
		    uint32_t pins)
{
	struct lr11xx_hal_data *data = CONTAINER_OF(cb, struct lr11xx_hal_data, irq_cb);

	if (data->irq_callback != NULL) {
		data->irq_callback(data->dev);
	}
}

int lr11xx_hal_set_irq_callback(const struct device *dev,
				void (*callback)(const struct device *dev))
{
	const struct lr11xx_hal_config *config = dev->config;
	struct lr11xx_hal_data *data = get_hal_data(dev);
	int ret;

	data->irq_callback = callback;

	if (callback != NULL) {
		ret = gpio_pin_interrupt_configure_dt(&config->irq,
						      GPIO_INT_EDGE_TO_ACTIVE);
	} else {
		ret = gpio_pin_interrupt_configure_dt(&config->irq,
						      GPIO_INT_DISABLE);
	}

	return ret;
}

void lr11xx_hal_irq_enable(const struct device *dev)
{
	/* GPIO interrupts auto re-arm after being serviced, no action needed */
	ARG_UNUSED(dev);
}

int lr11xx_hal_init(const struct device *dev)
{
	const struct lr11xx_hal_config *config = dev->config;
	struct lr11xx_hal_data *data = get_hal_data(dev);
	int ret;

	/* Store device reference for callbacks */
	data->dev = dev;
	data->irq_callback = NULL;

	/* Check SPI bus */
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	/* Configure reset GPIO */
	if (!gpio_is_ready_dt(&config->reset)) {
		LOG_ERR("Reset GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure reset GPIO: %d", ret);
		return ret;
	}

	/* Configure busy GPIO */
	if (!gpio_is_ready_dt(&config->busy)) {
		LOG_ERR("Busy GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->busy, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure busy GPIO: %d", ret);
		return ret;
	}

	/* Configure IRQ GPIO with interrupt */
	if (!gpio_is_ready_dt(&config->irq)) {
		LOG_ERR("IRQ GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->irq, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure IRQ GPIO: %d", ret);
		return ret;
	}

	/* Setup IRQ GPIO interrupt callback */
	gpio_init_callback(&data->irq_cb, irq_isr, BIT(config->irq.pin));
	ret = gpio_add_callback(config->irq.port, &data->irq_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add IRQ GPIO callback: %d", ret);
		return ret;
	}

	LOG_DBG("HAL initialized");
	return 0;
}

int lr11xx_hal_wakeup(const struct device *dev)
{
	const struct lr11xx_hal_config *config = dev->config;
	uint8_t buf[6] = { LR11XX_OPCODE(LR11XX_CMD_GET_STATUS), 0x00, 0x00, 0x00, 0x00 };
	struct spi_buf tx_buf = {
		.buf = buf,
		.len = sizeof(buf),
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};
	int ret;

	/*
	 * Send a write-only GET_STATUS command. The NSS falling edge wakes
	 * the chip from sleep. Use spi_write_dt() (TX-only) rather than
	 * spi_transceive_dt() because we are not interested in the status yet.
	 */
	ret = spi_write_dt(&config->spi, &tx_set);
	if (ret < 0) {
		LOG_ERR("Wakeup SPI failed: %d", ret);
		return ret;
	}

	return lr11xx_hal_wait_busy(dev, LR11XX_BUSY_DEFAULT_TIMEOUT);
}

int lr11xx_hal_write_cmd(const struct device *dev, uint16_t opcode, const uint8_t *data, size_t len)
{
	uint8_t hdr[2] = { LR11XX_OPCODE(opcode) };
	int ret;

	ret = lr11xx_hal_wait_busy(dev, LR11XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(dev, hdr, sizeof(hdr), (uint8_t *)data, len, false, NULL);
	if (ret < 0) {
		LOG_ERR("SPI write failed: %d", ret);
		return ret;
	}

	if (opcode == LR11XX_CMD_SET_SLEEP) {
		/*
		 * The chip needs time to fully enter sleep mode before the
		 * next NSS falling edge can wake it up reliably.
		 */
		k_busy_wait(500);
		return ret;
	}

	ret = lr11xx_hal_wait_busy(dev, LR11XX_BUSY_DEFAULT_TIMEOUT);

	lr11xx_hal_read_status(dev, hdr, NULL);

	if (LR11XX_STAT1_CMD_GET(hdr[0]) != LR11XX_STAT1_CMD_OK
	    && LR11XX_STAT1_CMD_GET(hdr[0]) != LR11XX_STAT1_CMD_DAT
	    && ret == 0) {
		ret = LR11XX_NOT_API_ERR(LR11XX_STAT1_CMD_GET(hdr[0]));
	}

	return ret;
}

int lr11xx_hal_read_cmd(const struct device *dev, uint16_t opcode, uint8_t *data, size_t len)
{
	uint8_t hdr[2] = { LR11XX_OPCODE(opcode) };
	int ret;

	ret = lr11xx_hal_wait_busy(dev, LR11XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(dev, hdr, sizeof(hdr), data, len, true, NULL);
	if (ret < 0) {
		LOG_ERR("SPI transceive failed: %d", ret);
		return ret;
	}

	return 0;
}

int lr11xx_hal_read_status(const struct device *dev, uint8_t stats[2], uint32_t *irq_status)
{
	uint8_t hdr[6] = { LR11XX_OPCODE(LR11XX_CMD_GET_STATUS), 0x0, 0x0, 0x0, 0x0 };
	uint8_t status[6];
	int ret;

	ret = lr11xx_hal_wait_busy(dev, LR11XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(dev, hdr, sizeof(hdr), NULL, 0, false, status);
	if (ret < 0) {
		LOG_ERR("SPI transceive failed: %d", ret);
		return ret;
	}

	if (stats != NULL) {
		stats[0] = status[0];
		stats[1] = status[1];
	}

	if (irq_status != NULL) {
		*irq_status = sys_get_be32(&status[2]);
	}

	return 0;
}

int lr11xx_hal_write_buffer(const struct device *dev, const uint8_t *data, size_t len)
{
	__ASSERT_NO_MSG(len <= LR11XX_MAX_PAYLOAD_LEN);

	uint8_t hdr[2] = { LR11XX_OPCODE(LR11XX_CMD_WRITE_BUFFER) };
	int ret;

	ret = lr11xx_hal_wait_busy(dev, LR11XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(dev, hdr, sizeof(hdr), (uint8_t *)data, len, false, NULL);
	if (ret < 0) {
		LOG_ERR("SPI write buffer failed: %d", ret);
		return ret;
	}

	return lr11xx_hal_wait_busy(dev, LR11XX_BUSY_DEFAULT_TIMEOUT);
}

int lr11xx_hal_read_buffer(const struct device *dev, uint8_t offset, uint8_t *data, size_t len)
{
	__ASSERT_NO_MSG(len <= LR11XX_MAX_PAYLOAD_LEN);

	uint8_t hdr[4] = { LR11XX_OPCODE(LR11XX_CMD_READ_BUFFER), offset, (uint8_t)len };
	int ret;

	ret = lr11xx_hal_wait_busy(dev, LR11XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(dev, hdr, sizeof(hdr), data, len, true, NULL);
	if (ret < 0) {
		LOG_ERR("SPI read buffer failed: %d", ret);
		return ret;
	}

	return 0;
}
