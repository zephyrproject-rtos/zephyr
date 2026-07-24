/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include <esp_attr.h>

#include "esp_hosted_mcu.h"

#define DT_DRV_COMPAT espressif_esp_hosted_mcu

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp_hosted_mcu_spi, CONFIG_ESP_HOSTED_MCU_LOG_LEVEL);

/*
 * SPI mode 2 (CPOL=1, CPHA=0), MSB first, 8-bit words, host drives the clock.
 * Full duplex is stated rather than left to the default because the
 * receive stash below depends on it.
 */
#define ESP_HOSTED_MCU_SPI_CONFIG                                                                  \
	(SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_MODE_CPOL | SPI_FULL_DUPLEX)

/* Bound on how long the coprocessor may take to raise the handshake line (ms). */
#define ESP_HOSTED_MCU_SPI_HANDSHAKE_TIMEOUT (100)

struct esp_hosted_mcu_spi_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec handshake_gpio;
	struct gpio_dt_spec dataready_gpio;
};

struct esp_hosted_mcu_spi_data {
	/* Signalled by the data-ready GPIO interrupt to wake the receive path. */
	struct k_sem rx_sem;
	struct gpio_callback dataready_cb;
	/*
	 * A frame the coprocessor clocked in while the host was transmitting. The bus
	 * is full duplex, so every transmit also shifts in whatever the coprocessor
	 * had queued; that frame is held here until the receive path collects
	 * it. Without this the coprocessor considers the frame delivered and drops
	 * data-ready, so a response that happens to arrive during a transmit is
	 * lost and a command waiting on it never completes.
	 */
	uint8_t stash[ESP_HOSTED_MCU_FRAME_SIZE];
	/* Read by the receive path without the lock, so keep the load explicit. */
	volatile uint16_t stash_len;
	struct k_mutex lock;
};

static struct esp_hosted_mcu_spi_data esp_hosted_mcu_spi_data_0;

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "only one esp-hosted-mcu spi instance supported");

const struct esp_hosted_mcu_spi_config esp_hosted_mcu_spi_config_0 = {
	.bus = SPI_DT_SPEC_INST_GET(0, ESP_HOSTED_MCU_SPI_CONFIG),
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
	.handshake_gpio = GPIO_DT_SPEC_INST_GET(0, handshake_gpios),
	.dataready_gpio = GPIO_DT_SPEC_INST_GET(0, dataready_gpios),
};

/*
 * Runs from the GPIO interrupt handler, which is registered as IRAM safe so it
 * can fire while the flash cache is disabled. Keep this callback in IRAM;
 * k_sem_give is safe because the kernel is linked there.
 */
static void IRAM_ATTR esp_hosted_mcu_spi_dataready_isr(const struct device *port,
						       struct gpio_callback *cb, uint32_t pins)
{
	struct esp_hosted_mcu_spi_data *data =
		CONTAINER_OF(cb, struct esp_hosted_mcu_spi_data, dataready_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_sem_give(&data->rx_sem);
}

static int esp_hosted_mcu_spi_init(const struct device *dev)
{
	const struct esp_hosted_mcu_config *cfg = dev->config;
	const struct esp_hosted_mcu_spi_config *spi = cfg->transport_config;
	struct esp_hosted_mcu_spi_data *data = &esp_hosted_mcu_spi_data_0;

	if (!spi_is_ready_dt(&spi->bus)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&spi->reset_gpio) || !gpio_is_ready_dt(&spi->handshake_gpio) ||
	    !gpio_is_ready_dt(&spi->dataready_gpio)) {
		LOG_ERR("handshake GPIOs not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&spi->reset_gpio, GPIO_OUTPUT_ACTIVE)) {
		return -EIO;
	}

	/*
	 * The reset line rests in its inactive state and a momentary pulse to
	 * the active state restarts the coprocessor, so the sequence must end back at
	 * rest. Leaving it asserted holds the coprocessor in reset, where it drives
	 * the shared bus lines and never answers.
	 */
	gpio_pin_set_dt(&spi->reset_gpio, 1);
	k_msleep(50);
	gpio_pin_set_dt(&spi->reset_gpio, 0);
	k_msleep(50);
	gpio_pin_set_dt(&spi->reset_gpio, 1);
	k_msleep(500);

	if (gpio_pin_configure_dt(&spi->handshake_gpio, GPIO_INPUT) ||
	    gpio_pin_configure_dt(&spi->dataready_gpio, GPIO_INPUT)) {
		return -EIO;
	}

	k_sem_init(&data->rx_sem, 0, 1);
	k_mutex_init(&data->lock);

	/*
	 * Wake the receive path on the coprocessor asserting data-ready. The level is
	 * edge-triggered here and the core re-checks data_ready() after every
	 * wait, so a missed edge costs at most one poll interval rather than
	 * wedging the receiver.
	 */
	gpio_init_callback(&data->dataready_cb, esp_hosted_mcu_spi_dataready_isr,
			   BIT(spi->dataready_gpio.pin));

	if (gpio_add_callback_dt(&spi->dataready_gpio, &data->dataready_cb) != 0) {
		LOG_ERR("data-ready callback not available");
		return -EIO;
	}

	if (gpio_pin_interrupt_configure_dt(&spi->dataready_gpio, GPIO_INT_EDGE_TO_ACTIVE) != 0) {
		LOG_ERR("data-ready interrupt not available");
		return -EIO;
	}

	return 0;
}

static void esp_hosted_mcu_spi_wait_for_rx(const struct device *dev, int timeout_ms)
{
	ARG_UNUSED(dev);

	k_sem_take(&esp_hosted_mcu_spi_data_0.rx_sem, K_MSEC(timeout_ms));
}

static bool esp_hosted_mcu_spi_data_ready(const struct device *dev)
{
	const struct esp_hosted_mcu_config *cfg = dev->config;
	const struct esp_hosted_mcu_spi_config *spi = cfg->transport_config;

	/*
	 * A frame held from a transmit counts as data pending. The coprocessor has
	 * already released it and lowered data-ready, so the GPIO alone would
	 * leave it uncollected.
	 */
	if (esp_hosted_mcu_spi_data_0.stash_len != 0) {
		return true;
	}

	return gpio_pin_get_dt(&spi->dataready_gpio) > 0;
}

/*
 * Length of the frame the coprocessor placed in a receive buffer, or 0 if it holds
 * no frame. The SPI coprocessor has no pending-byte register like the SDIO one, so
 * the length comes from the frame header the coprocessor just clocked out. Validate
 * it here rather than hand the core a length its frame walk cannot trust.
 */
static uint16_t esp_hosted_mcu_spi_rx_len(const uint8_t *rx)
{
	const struct esp_hosted_mcu_header *hdr = (const struct esp_hosted_mcu_header *)rx;
	uint32_t frame_len;

	if (hdr->offset != ESP_HOSTED_MCU_HEADER_SIZE || hdr->len == 0 ||
	    hdr->len > ESP_HOSTED_MCU_MAX_PAYLOAD) {
		return 0;
	}

	frame_len = (uint32_t)hdr->offset + hdr->len;
	if (frame_len > ESP_HOSTED_MCU_FRAME_SIZE) {
		return 0;
	}

	return (uint16_t)frame_len;
}

static int esp_hosted_mcu_spi_transfer(const struct device *dev, const uint8_t *tx, uint16_t tx_len,
				       uint8_t *rx)
{
	const struct esp_hosted_mcu_config *cfg = dev->config;
	const struct esp_hosted_mcu_spi_config *spi = cfg->transport_config;
	struct esp_hosted_mcu_spi_data *data = &esp_hosted_mcu_spi_data_0;
	bool want_rx = rx != NULL;
	uint16_t rx_len;

	ARG_UNUSED(tx_len);

	/*
	 * The transaction is a fixed full frame in each direction, so pad the
	 * transmit side and zero-fill it on a receive-only poll.
	 */
	static uint8_t tx_pad[ESP_HOSTED_MCU_FRAME_SIZE];
	static uint8_t rx_buf[ESP_HOSTED_MCU_FRAME_SIZE];

	/*
	 * Hand back a frame stashed by an earlier transmit before using the bus.
	 * The lock covers only the stash: holding it across the handshake wait
	 * below would let an idle receive poll block a transmit for the whole
	 * timeout, and the transmit holds the core's send lock while it waits.
	 */
	k_mutex_lock(&data->lock, K_FOREVER);
	if (want_rx && data->stash_len != 0) {
		rx_len = data->stash_len;
		memcpy(rx, data->stash, rx_len);
		data->stash_len = 0;
		k_mutex_unlock(&data->lock);
		return (int)rx_len;
	}
	k_mutex_unlock(&data->lock);

	if (tx == NULL) {
		memset(tx_pad, 0, sizeof(tx_pad));
	}

	const struct spi_buf txb = {
		.buf = (void *)(tx ? tx : tx_pad),
		.len = ESP_HOSTED_MCU_FRAME_SIZE,
	};
	const struct spi_buf_set txs = {.buffers = &txb, .count = 1};
	const struct spi_buf rxb = {
		.buf = want_rx ? rx : rx_buf,
		.len = ESP_HOSTED_MCU_FRAME_SIZE,
	};
	const struct spi_buf_set rxs = {.buffers = &rxb, .count = 1};

	/*
	 * A transaction may proceed only when the coprocessor has raised handshake.
	 * A receive additionally waits for data-ready, since without it the
	 * coprocessor has nothing queued and clocking would waste a full frame time.
	 * A transmit goes ahead on handshake alone and takes whatever the coprocessor
	 * happens to return along with it.
	 */
	for (int64_t start = k_uptime_get();; k_msleep(1)) {
		bool hs = gpio_pin_get_dt(&spi->handshake_gpio) > 0;
		bool dr = gpio_pin_get_dt(&spi->dataready_gpio) > 0;

		if (hs && (!want_rx || dr)) {
			break;
		}
		if ((k_uptime_get() - start) >= ESP_HOSTED_MCU_SPI_HANDSHAKE_TIMEOUT) {
			/*
			 * A receive that times out means the coprocessor had nothing
			 * to hand over, which the core treats as an idle poll.
			 * A transmit that never sees handshake did not reach
			 * the coprocessor and must be reported as an error.
			 */
			return want_rx ? 0 : -ETIMEDOUT;
		}
	}

	if (spi_transceive_dt(&spi->bus, &txs, &rxs)) {
		LOG_ERR("spi_transceive failed");
		return -EIO;
	}

	if (want_rx) {
		return (int)esp_hosted_mcu_spi_rx_len(rx);
	}

	/* Keep whatever this transmit shifted in; the coprocessor will not resend it. */
	rx_len = esp_hosted_mcu_spi_rx_len(rx_buf);
	if (rx_len != 0) {
		k_mutex_lock(&data->lock, K_FOREVER);
		if (data->stash_len != 0) {
			LOG_WRN("receive stash overrun, frame dropped");
		} else {
			memcpy(data->stash, rx_buf, rx_len);
			data->stash_len = rx_len;
			k_sem_give(&data->rx_sem);
		}
		k_mutex_unlock(&data->lock);
	}

	return 0;
}

const struct esp_hosted_mcu_transport_api esp_hosted_mcu_spi_api = {
	.init = esp_hosted_mcu_spi_init,
	.transfer = esp_hosted_mcu_spi_transfer,
	.data_ready = esp_hosted_mcu_spi_data_ready,
	.wait_for_rx = esp_hosted_mcu_spi_wait_for_rx,
};
