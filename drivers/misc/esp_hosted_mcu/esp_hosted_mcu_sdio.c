/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdio.h>

#include <esp_attr.h>

#include "esp_hosted_mcu.h"

#define DT_DRV_COMPAT espressif_esp_hosted_mcu_sdio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp_hosted_mcu_sdio, CONFIG_ESP_HOSTED_MCU_LOG_LEVEL);

/*
 * Host-visible SLC register window on the coprocessor. The coprocessor decodes only the
 * low 10 bits of the SDIO address, so these offsets are used directly as the
 * Function 1 register address on CMD52/CMD53.
 */
#define ESP_SLC_TOKEN_RDATA_REG 0x044
#define ESP_SLC_INT_ST_REG      0x058
#define ESP_SLC_PKT_LEN_REG     0x060
#define ESP_SLC_INT_CLR_REG     0x0D4
#define ESP_SLC_FUNC1_INT_ENA   0x0DC

/*
 * Host-to-coprocessor scratch register. Writing bit0 (ESP_OPEN_DATA_PATH) raises
 * the general-purpose coprocessor interrupt that flips the coprocessor's data path to
 * active. Until this fires, the coprocessor arms its SDIO peripheral but never
 * reads frames out of the FIFO or emits a response.
 */
#define ESP_SLC_HOST_TO_CP_INT 0x08C
#define ESP_SLC_OPEN_DATA_PATH 0x01

/*
 * The data FIFO is addressed from the top: a transfer of N bytes starts at
 * (END_ADDR - N) and the length is encoded by the address offset. The coprocessor
 * discards any bytes past the requested length.
 */
#define ESP_SLC_CMD53_END_ADDR 0x1F800

/* Running counters wrap at these moduli (from the coprocessor register widths). */
#define ESP_SLC_TX_BUFFER_MAX  0x1000
#define ESP_SLC_TX_BUFFER_MASK 0xFFF
#define ESP_SLC_RX_BYTE_MAX    0x100000
#define ESP_SLC_RX_BYTE_MASK   0xFFFFF

/* SLC block size negotiated with the coprocessor. */
#define ESP_SLC_BLOCK_SIZE 512

/*
 * Coprocessor receive-buffer size. The coprocessor counts one buffer credit per this
 * many bytes, so credit accounting must divide frame lengths by it.
 */
#define ESP_SLC_RX_BUFFER_SIZE 1536

/* Bound on coprocessor boot / IO-ready after reset (ms). */
#define ESP_SLC_READY_TIMEOUT 2000

/* Bound on the per-frame wait for coprocessor TX credits (ms). */
#define ESP_SLC_TX_CREDIT_TIMEOUT 500

struct esp_hosted_mcu_sdio_config {
	const struct device *sdhc_dev;
	struct gpio_dt_spec reset_gpio;
};

struct esp_hosted_mcu_sdio_data {
	struct sd_card card;
	struct sdio_func func1;
	/* Serialises bus access between the transmit and receive paths. */
	struct k_mutex bus_lock;
	/* Running byte/buffer counters, matching the coprocessor-side counters. */
	uint32_t rx_got_bytes;
	uint32_t tx_sent_buffers;
	uint16_t buffer_size;
	bool ready;
	/* Signalled by the coprocessor DAT1 card interrupt to wake the receive path. */
	struct k_sem rx_sem;
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "only one esp-hosted-mcu sdio instance supported");

static struct esp_hosted_mcu_sdio_data esp_hosted_mcu_sdio_data_0;

const struct esp_hosted_mcu_sdio_config esp_hosted_mcu_sdio_config_0 = {
	.sdhc_dev = DEVICE_DT_GET(DT_INST_PARENT(0)),
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
};

/* Read a 32-bit coprocessor register through a 4-byte CMD53 to Function 1. */
static int esp_slc_read_u32(struct esp_hosted_mcu_sdio_data *data, uint32_t reg, uint32_t *val)
{
	uint8_t buf[4];
	int ret = sdio_read_addr(&data->func1, reg, buf, sizeof(buf));

	if (ret != 0) {
		return ret;
	}

	*val = buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16) |
	       ((uint32_t)buf[3] << 24);
	return 0;
}

/* Write a 32-bit coprocessor register through a 4-byte CMD53 to Function 1. */
static int esp_slc_write_u32(struct esp_hosted_mcu_sdio_data *data, uint32_t reg, uint32_t val)
{
	uint8_t buf[4] = {
		(uint8_t)val,
		(uint8_t)(val >> 8),
		(uint8_t)(val >> 16),
		(uint8_t)(val >> 24),
	};

	return sdio_write_addr(&data->func1, reg, buf, sizeof(buf));
}

/*
 * Acknowledge the coprocessor's pending interrupt sources.
 *
 * The coprocessor raises DAT1 by latching a bit in its interrupt status
 * register, and it only drives the line again once that bit is cleared.
 * Leaving it set keeps the status permanently asserted, so no further
 * transition ever reaches the host and the receive path stalls.
 */
static int esp_slc_clear_int(struct esp_hosted_mcu_sdio_data *data)
{
	uint32_t status;
	int ret = esp_slc_read_u32(data, ESP_SLC_INT_ST_REG, &status);

	if (ret != 0 || status == 0) {
		return ret;
	}

	return esp_slc_write_u32(data, ESP_SLC_INT_CLR_REG, status);
}

/* Number of RX bytes the coprocessor currently has queued for the host. */
static int esp_slc_rx_pending(struct esp_hosted_mcu_sdio_data *data, uint32_t *pending)
{
	uint32_t len;
	int ret = esp_slc_read_u32(data, ESP_SLC_PKT_LEN_REG, &len);

	if (ret != 0) {
		return ret;
	}

	len &= ESP_SLC_RX_BYTE_MASK;

	/* An all-ones read means the bus floated; treat it as no data. */
	if (len == ESP_SLC_RX_BYTE_MASK) {
		*pending = 0;
		return 0;
	}

	*pending = (len - data->rx_got_bytes) & ESP_SLC_RX_BYTE_MASK;
	return 0;
}

/*
 * Number of RX buffers the coprocessor currently has free for host writes. The
 * coprocessor increments this token counter (top 16 bits of TOKEN_RDATA) each
 * time it loads a receive buffer; the host spends one credit per buffer
 * it writes. A CMD53 write with no free buffer is dropped by the coprocessor.
 */
static int esp_slc_tx_free(struct esp_hosted_mcu_sdio_data *data, uint32_t *free_bufs)
{
	uint32_t token;
	int ret = esp_slc_read_u32(data, ESP_SLC_TOKEN_RDATA_REG, &token);

	if (ret != 0) {
		return ret;
	}

	uint32_t latest = (token >> 16) & ESP_SLC_TX_BUFFER_MASK;

	*free_bufs = (latest - data->tx_sent_buffers) & ESP_SLC_TX_BUFFER_MASK;
	return 0;
}

/*
 * Move len bytes over the FIFO window with CMD53, block mode when a full
 * block is available and byte mode (4-byte aligned) for the remainder.
 */
static int esp_slc_fifo_xfer(struct esp_hosted_mcu_sdio_data *data, uint8_t *buf, uint32_t len,
			     bool write)
{
	uint32_t remain = len;
	uint8_t *ptr = buf;

	while (remain > 0) {
		uint32_t addr = ESP_SLC_CMD53_END_ADDR - remain;
		uint32_t chunk;
		int ret;

		if (remain >= ESP_SLC_BLOCK_SIZE) {
			chunk = (remain / ESP_SLC_BLOCK_SIZE) * ESP_SLC_BLOCK_SIZE;
		} else {
			chunk = (remain + 3) & ~3U;
		}

		if (write) {
			ret = sdio_write_addr(&data->func1, addr, ptr, chunk);
		} else {
			ret = sdio_read_addr(&data->func1, addr, ptr, chunk);
		}

		if (ret != 0) {
			return ret;
		}

		/* Advance by the real payload, not the padded chunk. */
		uint32_t consumed = (chunk > remain) ? remain : chunk;

		ptr += consumed;
		remain -= consumed;
	}

	return 0;
}

/*
 * Runs from the SDHC interrupt handler, which is registered as IRAM safe so it
 * can fire while the flash cache is disabled. Keep this callback and anything
 * it touches in IRAM; k_sem_give is safe because the kernel is linked there.
 */
static void IRAM_ATTR esp_hosted_mcu_sdio_card_int(const struct device *sdhc_dev, int reason,
						   const void *user_data)
{
	struct esp_hosted_mcu_sdio_data *data = (struct esp_hosted_mcu_sdio_data *)user_data;

	ARG_UNUSED(sdhc_dev);
	ARG_UNUSED(reason);

	k_sem_give(&data->rx_sem);
}

static int esp_hosted_mcu_sdio_init(const struct device *dev)
{
	const struct esp_hosted_mcu_config *cfg = dev->config;
	const struct esp_hosted_mcu_sdio_config *sdio = cfg->transport_config;
	struct esp_hosted_mcu_sdio_data *data = &esp_hosted_mcu_sdio_data_0;
	int ret;

	k_mutex_init(&data->bus_lock);
	k_sem_init(&data->rx_sem, 0, 1);

	if (!device_is_ready(sdio->sdhc_dev)) {
		LOG_ERR("SDHC controller not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&sdio->reset_gpio)) {
		LOG_ERR("reset GPIO not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&sdio->reset_gpio, GPIO_OUTPUT_ACTIVE)) {
		return -EIO;
	}

	/*
	 * The reset line rests in its inactive state and a momentary pulse to
	 * the active state restarts the coprocessor, so the sequence must end back at
	 * rest before its firmware boots and arms the SDIO coprocessor. Leaving it
	 * asserted holds the coprocessor in reset, where it never answers.
	 */
	gpio_pin_set_dt(&sdio->reset_gpio, 1);
	k_msleep(50);
	gpio_pin_set_dt(&sdio->reset_gpio, 0);
	k_msleep(50);
	gpio_pin_set_dt(&sdio->reset_gpio, 1);

	/*
	 * Enumerate the coprocessor as an SDIO card. It only answers once its
	 * firmware has booted and armed the SDIO peripheral, so retry until it
	 * does rather than waiting a fixed worst-case delay: a coprocessor that
	 * comes up early lets the rest of the system boot that much sooner.
	 */
	for (int64_t start = k_uptime_get();;) {
		ret = sd_init(sdio->sdhc_dev, &data->card);
		if (ret == 0) {
			break;
		}
		if ((k_uptime_get() - start) >= ESP_SLC_READY_TIMEOUT) {
			LOG_ERR("SDIO enumeration failed (%d)", ret);
			return ret;
		}
		k_msleep(10);
	}

	/* Bring up Function 1 (the SLC data function) and set its block size. */
	ret = sdio_init_func(&data->card, &data->func1, SDIO_FUNC_NUM_1);
	if (ret != 0) {
		LOG_ERR("func1 init failed (%d)", ret);
		return ret;
	}

	ret = sdio_enable_func(&data->func1);
	if (ret != 0) {
		LOG_ERR("func1 enable failed (%d)", ret);
		return ret;
	}

	ret = sdio_set_block_size(&data->func1, ESP_SLC_BLOCK_SIZE);
	if (ret != 0) {
		LOG_ERR("func1 block size failed (%d)", ret);
		return ret;
	}

	/*
	 * Raise the open-data-path interrupt so the coprocessor activates its data
	 * path and starts serving the FIFO. Without this the coprocessor accepts
	 * writes into the FIFO but never parses them or emits a response.
	 */
	ret = sdio_write_byte(&data->func1, ESP_SLC_HOST_TO_CP_INT, ESP_SLC_OPEN_DATA_PATH);
	if (ret != 0) {
		LOG_ERR("open data path failed (%d)", ret);
		return ret;
	}

	data->rx_got_bytes = 0;
	data->tx_sent_buffers = 0;
	data->buffer_size = ESP_SLC_RX_BUFFER_SIZE;

	/*
	 * Start from a clean interrupt status. Anything the coprocessor latched
	 * while the host was still coming up would otherwise keep the status
	 * asserted and suppress the next signal.
	 */
	(void)esp_slc_clear_int(data);

	/*
	 * Arm the coprocessor DAT1 card interrupt as an accelerator for the receive
	 * path. The interrupt only wakes the poll early; the poll fallback in
	 * wait_for_rx still guarantees progress if the interrupt is missed.
	 */
	ret = sdhc_enable_interrupt(sdio->sdhc_dev, esp_hosted_mcu_sdio_card_int, SDHC_INT_SDIO,
				    data);
	if (ret != 0) {
		LOG_ERR("card interrupt enable failed (%d)", ret);
		return ret;
	}

	data->ready = true;

	return 0;
}

static bool esp_hosted_mcu_sdio_data_ready(const struct device *dev)
{
	struct esp_hosted_mcu_sdio_data *data = &esp_hosted_mcu_sdio_data_0;
	uint32_t pending;
	int ret;

	ARG_UNUSED(dev);

	if (!data->ready) {
		return false;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	ret = esp_slc_rx_pending(data, &pending);
	k_mutex_unlock(&data->bus_lock);

	if (ret != 0) {
		return false;
	}

	return pending > 0;
}

static void esp_hosted_mcu_sdio_wait_for_rx(const struct device *dev, int timeout_ms)
{
	const struct esp_hosted_mcu_config *cfg = dev->config;
	const struct esp_hosted_mcu_sdio_config *sdio = cfg->transport_config;
	struct esp_hosted_mcu_sdio_data *data = &esp_hosted_mcu_sdio_data_0;

	/*
	 * Acknowledge whatever the coprocessor has already flagged. Until this
	 * happens its status register stays latched and it will not signal
	 * again, so the wait below would never be woken.
	 */
	k_mutex_lock(&data->bus_lock, K_FOREVER);
	(void)esp_slc_clear_int(data);
	k_mutex_unlock(&data->bus_lock);

	/*
	 * The SDHC driver masks the card interrupt when it fires, so it is
	 * disarmed on entry here. Drop any token left from a previous cycle
	 * before re-arming, so the wait below reflects this cycle only.
	 *
	 * Re-arming unmasks the source. If the coprocessor is still holding
	 * DAT1 asserted, the hardware raises the interrupt again at once and
	 * the handler posts the semaphore, so the wait below returns without
	 * delay. That is the only way an assertion raised while the source
	 * was masked can be seen: it is a level the coprocessor holds, not an
	 * edge that can be caught again.
	 */
	k_sem_take(&data->rx_sem, K_NO_WAIT);

	if (sdhc_enable_interrupt(sdio->sdhc_dev, esp_hosted_mcu_sdio_card_int, SDHC_INT_SDIO,
				  data) != 0) {
		LOG_DBG("card interrupt re-arm failed");
	}

	k_sem_take(&data->rx_sem, K_MSEC(timeout_ms));
}

static int esp_hosted_mcu_sdio_transfer(const struct device *dev, const uint8_t *tx,
					uint16_t tx_len, uint8_t *rx)
{
	struct esp_hosted_mcu_sdio_data *data = &esp_hosted_mcu_sdio_data_0;
	bool want_rx = rx != NULL;
	int ret;

	if (!data->ready) {
		return -ENODEV;
	}

	if (tx != NULL && tx_len > 0) {
		uint32_t padded = (tx_len + 3) & ~3U;
		uint32_t need = (tx_len + data->buffer_size - 1) / data->buffer_size;
		uint32_t free_bufs = 0;

		/*
		 * The coprocessor silently drops a FIFO write when it has no loaded
		 * receive buffer. Spend one credit per buffer the frame needs
		 * and wait for the coprocessor to advertise enough free buffers. The
		 * receive path replenishes the credits, so the bus lock is
		 * released between polls; holding it across the wait would starve
		 * the receiver and deadlock under load.
		 */
		for (int64_t start = k_uptime_get();;) {
			k_mutex_lock(&data->bus_lock, K_FOREVER);
			ret = esp_slc_tx_free(data, &free_bufs);
			k_mutex_unlock(&data->bus_lock);
			if (ret != 0) {
				return ret;
			}
			if (free_bufs >= need) {
				break;
			}
			if ((k_uptime_get() - start) >= ESP_SLC_TX_CREDIT_TIMEOUT) {
				return -EBUSY;
			}
			k_usleep(100);
		}

		k_mutex_lock(&data->bus_lock, K_FOREVER);
		ret = esp_slc_fifo_xfer(data, (uint8_t *)tx, padded, true);
		if (ret == 0) {
			data->tx_sent_buffers =
				(data->tx_sent_buffers + need) & ESP_SLC_TX_BUFFER_MASK;
		}
		k_mutex_unlock(&data->bus_lock);
		if (ret != 0) {
			LOG_ERR("SLC TX failed (%d)", ret);
			return ret;
		}
	}

	if (!want_rx) {
		return 0;
	}

	uint32_t pending;

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	ret = esp_slc_rx_pending(data, &pending);
	if (ret == 0 && pending > 0) {
		if (pending > ESP_HOSTED_MCU_FRAME_SIZE) {
			pending = ESP_HOSTED_MCU_FRAME_SIZE;
		}
		ret = esp_slc_fifo_xfer(data, rx, pending, false);
		if (ret == 0) {
			data->rx_got_bytes = (data->rx_got_bytes + pending) & ESP_SLC_RX_BYTE_MASK;
		}
	}
	k_mutex_unlock(&data->bus_lock);

	if (ret != 0) {
		LOG_ERR("SLC RX failed (%d)", ret);
		return ret;
	}

	return (pending > 0) ? (int)pending : 0;
}

const struct esp_hosted_mcu_transport_api esp_hosted_mcu_sdio_api = {
	.init = esp_hosted_mcu_sdio_init,
	.transfer = esp_hosted_mcu_sdio_transfer,
	.data_ready = esp_hosted_mcu_sdio_data_ready,
	.wait_for_rx = esp_hosted_mcu_sdio_wait_for_rx,
};
