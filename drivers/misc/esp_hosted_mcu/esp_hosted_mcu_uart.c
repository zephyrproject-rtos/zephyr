/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/byteorder.h>

#include "esp_hosted_mcu.h"

#define DT_DRV_COMPAT espressif_esp_hosted_mcu

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp_hosted_mcu_uart, CONFIG_ESP_HOSTED_MCU_LOG_LEVEL);

/*
 * Per-byte timeout when draining the ring buffer for a frame. A single
 * dropped byte desynchronises the stream permanently, so the timeout
 * must be generous enough that a slow coprocessor or flow-controlled
 * link does not look like a lost byte.
 */
#define ESP_HOSTED_MCU_UART_RX_BYTE_TIMEOUT_MS 500

struct esp_hosted_mcu_uart_config {
	const struct device *uart_dev;
	struct gpio_dt_spec reset_gpio;
};

struct esp_hosted_mcu_uart_data {
	struct k_sem rx_sem;
	struct ring_buf rx_ringbuf;
	uint8_t rx_buf[2 * ESP_HOSTED_MCU_FRAME_SIZE];
};

static struct esp_hosted_mcu_uart_data esp_hosted_mcu_uart_data_0;

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "only one esp-hosted-mcu uart instance supported");

const struct esp_hosted_mcu_uart_config esp_hosted_mcu_uart_config_0 = {
	.uart_dev = DEVICE_DT_GET(DT_INST_BUS(0)),
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
};

static void ESP_HOSTED_MCU_ISR_ATTR esp_hosted_mcu_uart_isr(const struct device *uart_dev,
							void *user_data)
{
	struct esp_hosted_mcu_uart_data *data = user_data;
	uint8_t buf[64];
	int len;

	uart_irq_update(uart_dev);

	len = uart_fifo_read(uart_dev, buf, sizeof(buf));
	if (len < 0) {
		LOG_ERR("UART FIFO read error");
		return;
	}
	if (ring_buf_space_get(&data->rx_ringbuf) < len) {
		LOG_WRN("Ring buffer full, dropping bytes");
		return;
	}
	if (len > 0) {
		ring_buf_put(&data->rx_ringbuf, buf, len);
		k_sem_give(&data->rx_sem);
	}
}

static int esp_hosted_mcu_uart_init(const struct device *dev)
{
	const struct esp_hosted_mcu_config *cfg = dev->config;
	const struct esp_hosted_mcu_uart_config *uart_cfg = cfg->transport_config;
	struct esp_hosted_mcu_uart_data *data = &esp_hosted_mcu_uart_data_0;

	if (!device_is_ready(uart_cfg->uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&uart_cfg->reset_gpio)) {
		LOG_ERR("reset GPIO not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&uart_cfg->reset_gpio, GPIO_OUTPUT_ACTIVE) != 0) {
		return -EIO;
	}

	/* Hold the reset line active for the required pulse duration */
	k_msleep(CONFIG_ESP_HOSTED_MCU_RESET_PULSE_MS);
	gpio_pin_set_dt(&uart_cfg->reset_gpio, 0);
	k_msleep(CONFIG_ESP_HOSTED_MCU_UART_RESET_SETTLE_MS);

	k_sem_init(&data->rx_sem, 0, 1);
	ring_buf_init(&data->rx_ringbuf, sizeof(data->rx_buf), data->rx_buf);

	uart_irq_callback_user_data_set(uart_cfg->uart_dev, esp_hosted_mcu_uart_isr, data);
	uart_irq_rx_enable(uart_cfg->uart_dev);

	return 0;
}

/*
 * Drain bytes from the UART ring buffer until we find a valid header.
 * Return the number of bytes of valid payload available (including the header).
 */
static int esp_hosted_mcu_uart_drain(struct esp_hosted_mcu_uart_data *data)
{
	uint8_t hdr_buf[ESP_HOSTED_MCU_HEADER_SIZE];
	struct esp_hosted_mcu_header *hdr;

	while (ring_buf_size_get(&data->rx_ringbuf) >= ESP_HOSTED_MCU_HEADER_SIZE) {
		ring_buf_peek(&data->rx_ringbuf, hdr_buf, ESP_HOSTED_MCU_HEADER_SIZE);
		hdr = (struct esp_hosted_mcu_header *)hdr_buf;
		if (hdr->len != 0 && hdr->len <= ESP_HOSTED_MCU_MAX_PAYLOAD &&
		    hdr->offset == ESP_HOSTED_MCU_HEADER_SIZE) {
			/* Valid header found, return the number of bytes available */
			return ring_buf_size_get(&data->rx_ringbuf);
		}
		/* Invalid header, discard one byte and try again */
		ring_buf_consume(&data->rx_ringbuf, 1);
	}
	return 0;
}

/*
 * Drain exactly @a needed bytes from the ring buffer into @a dst,
 * blocking on the receive semaphore when the ring buffer is short.
 * Returns 0 on success or a negative errno on timeout.
 */
static int esp_hosted_mcu_uart_read_exact(struct esp_hosted_mcu_uart_data *data, uint8_t *dst,
					  uint16_t needed)
{
	uint16_t got = 0;

	while (got < needed) {
		uint32_t n = ring_buf_get(&data->rx_ringbuf, dst + got, needed - got);

		got += n;
		if (got < needed) {
			if (k_sem_take(&data->rx_sem,
				       K_MSEC(ESP_HOSTED_MCU_UART_RX_BYTE_TIMEOUT_MS)) != 0) {
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

static int esp_hosted_mcu_uart_transfer(const struct device *dev, const uint8_t *tx,
					uint16_t tx_len, uint8_t *rx)
{
	const struct esp_hosted_mcu_config *cfg = dev->config;
	const struct esp_hosted_mcu_uart_config *uart_cfg = cfg->transport_config;
	struct esp_hosted_mcu_uart_data *data = &esp_hosted_mcu_uart_data_0;
	int size_avail;

	if (tx != NULL && tx_len > 0) {
		for (uint16_t i = 0; i < tx_len; i++) {
			uart_poll_out(uart_cfg->uart_dev, tx[i]);
		}
	}

	if (rx == NULL) {
		return 0;
	}

	/*
	 * Receive path: drain the ring buffer until we have a valid header,
	 * then read exactly one frame from the coprocessor.
	 * We use this approach because the UART stream can desynchronize
	 * very easily. Reading a complete frame every time lets us drain
	 * missed bytes and resynchronize the stream.
	 */
	size_avail = esp_hosted_mcu_uart_drain(data);
	if (size_avail < ESP_HOSTED_MCU_HEADER_SIZE) {
		return 0;
	}

	if (esp_hosted_mcu_uart_read_exact(data, rx, ESP_HOSTED_MCU_HEADER_SIZE) != 0) {
		LOG_WRN("UART header read timed out");
		return 0;
	}

	const struct esp_hosted_mcu_header *hdr = (const struct esp_hosted_mcu_header *)rx;

	if (hdr->len == 0 || hdr->len > ESP_HOSTED_MCU_MAX_PAYLOAD ||
	    hdr->offset != ESP_HOSTED_MCU_HEADER_SIZE) {
		LOG_WRN("invalid UART frame header (len %u offset %u)", hdr->len, hdr->offset);
		return 0;
	}

	uint16_t payload_len = hdr->len;

	if (esp_hosted_mcu_uart_read_exact(data, rx + ESP_HOSTED_MCU_HEADER_SIZE, payload_len) !=
	    0) {
		LOG_WRN("UART payload read timed out (expected %u bytes)", payload_len);
		return 0;
	}

	return (int)(ESP_HOSTED_MCU_HEADER_SIZE + payload_len);
}

static bool esp_hosted_mcu_uart_data_ready(const struct device *dev)
{
	ARG_UNUSED(dev);

	return esp_hosted_mcu_uart_drain(&esp_hosted_mcu_uart_data_0) > 0;
}

static void esp_hosted_mcu_uart_wait_for_rx(const struct device *dev, k_timeout_t timeout)
{
	ARG_UNUSED(dev);

	k_sem_take(&esp_hosted_mcu_uart_data_0.rx_sem, timeout);
}

const struct esp_hosted_mcu_transport_api esp_hosted_mcu_uart_api = {
	.init = esp_hosted_mcu_uart_init,
	.transfer = esp_hosted_mcu_uart_transfer,
	.data_ready = esp_hosted_mcu_uart_data_ready,
	.wait_for_rx = esp_hosted_mcu_uart_wait_for_rx,
};
