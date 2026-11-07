/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/atomic.h>

#include "esp_hosted_mcu.h"

#define DT_DRV_COMPAT espressif_esp_hosted_mcu

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp_hosted_mcu_spi, CONFIG_ESP_HOSTED_MCU_LOG_LEVEL);

/*
 * SPI mode 3 (CPOL=1, CPHA=1), MSB first, 8-bit words, host drives the clock.
 * The coprocessor runs its slave in mode 3 on every target except the original
 * ESP32, and both ends must agree on CPOL and CPHA. Full duplex is stated
 * rather than left to the default because the receive stash below depends on
 * it.
 */
#define ESP_HOSTED_MCU_SPI_CONFIG                                                                  \
	(SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_MODE_CPOL |             \
	 SPI_MODE_CPHA | SPI_FULL_DUPLEX)

/* Bound on how long the coprocessor may take to raise the handshake line (ms). */
#define ESP_HOSTED_MCU_SPI_HANDSHAKE_TIMEOUT (100)
#define ESP_HOSTED_MCU_SPI_RX_SPACE_TIMEOUT (100)

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
	/* Reserve the stash before clocking a transmit, even if no RX is expected. */
	struct k_sem rx_space;
	uint8_t stash[ESP_HOSTED_MCU_FRAME_SIZE] __aligned(ESP_HOSTED_MCU_DMA_ALIGN);
	/* Atomic snapshot for data_ready(); stash contents use lock. */
	atomic_t stash_len;
	/* Covers GPIO readiness, the entire SPI transaction and stash publication. */
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

static void ESP_HOSTED_MCU_ISR_ATTR esp_hosted_mcu_spi_dataready_isr(const struct device *port,
								     struct gpio_callback *cb,
								     uint32_t pins)
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
	gpio_pin_set_dt(&spi->reset_gpio, 0);
	k_msleep(CONFIG_ESP_HOSTED_MCU_RESET_PULSE_MS);
	gpio_pin_set_dt(&spi->reset_gpio, 1);
	k_msleep(CONFIG_ESP_HOSTED_MCU_SPI_RESET_SETTLE_MS);

	/*
	 * Bias both lines to their inactive level. The coprocessor tristates
	 * them while it is in reset, and a floating input picks up enough
	 * coupling from the adjacent clock and data lines to raise edges the
	 * coprocessor never drove.
	 */
	if (gpio_pin_configure_dt(&spi->handshake_gpio, GPIO_INPUT | GPIO_PULL_DOWN) ||
	    gpio_pin_configure_dt(&spi->dataready_gpio, GPIO_INPUT | GPIO_PULL_DOWN)) {
		return -EIO;
	}

	k_sem_init(&data->rx_sem, 0, 1);
	k_sem_init(&data->rx_space, 1, 1);
	k_mutex_init(&data->lock);
	atomic_clear(&data->stash_len);

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

static void esp_hosted_mcu_spi_wait_for_rx(const struct device *dev, k_timeout_t timeout)
{
	ARG_UNUSED(dev);

	k_sem_take(&esp_hosted_mcu_spi_data_0.rx_sem, timeout);
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
	if (atomic_get(&esp_hosted_mcu_spi_data_0.stash_len) != 0) {
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
	int ret;

	ARG_UNUSED(tx_len);

	/*
	 * The coprocessor exchanges a whole frame per transaction and drops one
	 * that reaches it in pieces, so the controller has to clock the frame
	 * without splitting it. A controller whose transmit FIFO is smaller than
	 * a frame needs its DMA path enabled to manage that; on Espressif hosts
	 * that means dma-enabled on the SPI bus with the channels wired through
	 * dmas.
	 *
	 * The transaction is a fixed full frame in each direction. A
	 * receive-only poll passes no transmit buffer and leaves the controller
	 * to clock out a frame worth of zeros, which the SPI API defines a null
	 * buffer to mean.
	 */

	/*
	 * Every transmit can clock in a real frame, including HCI and RPC sends.
	 * Wait without the bus lock so the event thread can drain the stash. This
	 * also works if a cooperative Bluetooth thread is sending a whole burst.
	 * On timeout nothing has been clocked, so the pending RX frame remains intact.
	 */
	if (!want_rx && k_sem_take(&data->rx_space,
				  K_MSEC(ESP_HOSTED_MCU_SPI_RX_SPACE_TIMEOUT)) != 0) {
		LOG_WRN("receive stash busy, transmit not sent");
		return -ENOBUFS;
	}

	k_timepoint_t end = sys_timepoint_calc(K_MSEC(ESP_HOSTED_MCU_SPI_HANDSHAKE_TIMEOUT));

	for (;;) {
		k_mutex_lock(&data->lock, K_FOREVER);

		/* Recheck the stash on every poll, even if data-ready has gone low. */
		if (want_rx && atomic_get(&data->stash_len) != 0) {
			ret = atomic_get(&data->stash_len);
			memcpy(rx, data->stash, ret);
			atomic_clear(&data->stash_len);
			k_sem_give(&data->rx_space);
			k_mutex_unlock(&data->lock);
			return ret;
		}

		/*
		 * Check readiness under the same lock as the transaction. Checking
		 * outside lets another caller consume the ready transaction first.
		 * Never retain this mutex while sleeping for handshake/data-ready.
		 */
		int hs = gpio_pin_get_dt(&spi->handshake_gpio);
		int dr = gpio_pin_get_dt(&spi->dataready_gpio);

		if (hs < 0 || dr < 0) {
			ret = hs < 0 ? hs : dr;
			k_mutex_unlock(&data->lock);
			break;
		}
		if (hs > 0 && (!want_rx || dr > 0)) {
			const struct spi_buf txb = {
				.buf = (void *)tx,
				.len = ESP_HOSTED_MCU_FRAME_SIZE,
			};
			const struct spi_buf_set txs = {.buffers = &txb, .count = 1};
			const struct spi_buf rxb = {
				.buf = want_rx ? rx : data->stash,
				.len = ESP_HOSTED_MCU_FRAME_SIZE,
			};
			const struct spi_buf_set rxs = {.buffers = &rxb, .count = 1};

			/* Fixed 1600-byte full-duplex exchange; NULL TX clocks zeros. */
			ret = spi_transceive_dt(&spi->bus, &txs, &rxs);
			if (ret == 0) {
				uint16_t len = esp_hosted_mcu_spi_rx_len(rxb.buf);

				if (want_rx) {
					ret = len;
				} else if (len != 0) {
					atomic_set(&data->stash_len, len);
					/* Publish before waking RX; its lock sees complete data. */
					k_sem_give(&data->rx_sem);
					k_mutex_unlock(&data->lock);
					return 0; /* RX now owns the reserved slot. */
				}
			} else {
				LOG_ERR("spi_transceive failed (%d)", ret);
			}
			k_mutex_unlock(&data->lock);
			break;
		}
		k_mutex_unlock(&data->lock);

		if (sys_timepoint_expired(end)) {
			ret = want_rx ? 0 : -ETIMEDOUT;
			break;
		}
		k_msleep(1);
	}

	/* Dummy frames, failed transfers and handshake timeouts keep no RX slot. */
	if (!want_rx) {
		k_sem_give(&data->rx_space);
	}
	return ret;
}

const struct esp_hosted_mcu_transport_api esp_hosted_mcu_spi_api = {
	.init = esp_hosted_mcu_spi_init,
	.transfer = esp_hosted_mcu_spi_transfer,
	.data_ready = esp_hosted_mcu_spi_data_ready,
	.wait_for_rx = esp_hosted_mcu_spi_wait_for_rx,
};
