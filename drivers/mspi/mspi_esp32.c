/*
 * Copyright (c) 2026 Pavel Maloletkov.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_mspi

/*
 * esp-idf headers first — avoids BIT() redefinition warning.
 * Pattern taken directly from spi_esp32_spim.c.
 */
#include <hal/spi_hal.h>
#include <hal/spi_ll.h>
#include <esp_attr.h>
#include <esp_clk_tree.h>

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <esp_memory_utils.h>

#ifdef SOC_GDMA_SUPPORTED
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#endif

#if defined(CONFIG_MSPI_ESP32_PSRAM_DMA) && defined(SOC_GDMA_SUPPORTED) &&                         \
	defined(SOC_PSRAM_DMA_CAPABLE)
#include <esp_cache.h>
#define MSPI_ESP32_PSRAM_DMA 1
#endif

#include <soc/spi_periph.h>

#include "mspi_esp32.h"

LOG_MODULE_REGISTER(mspi_esp32, CONFIG_MSPI_LOG_LEVEL);

#define SPI_DMA_RX 0
#define SPI_DMA_TX 1

/* --------------------------------------------------------------------------
 * Helpers
 * --------------------------------------------------------------------------
 */

static int mspi_mode_to_esp32(enum mspi_cpp_mode mode)
{
	switch (mode) {
	case MSPI_CPP_MODE_0:
		return 0; /* CPOL=0, CPHA=0 */
	case MSPI_CPP_MODE_1:
		return 1; /* CPOL=0, CPHA=1 */
	case MSPI_CPP_MODE_2:
		return 2; /* CPOL=1, CPHA=0 */
	case MSPI_CPP_MODE_3:
		return 3; /* CPOL=1, CPHA=1 */
	default:
		LOG_ERR("Unsupported SPI mode %d", mode);
		return -ENOTSUP;
	}
}

static spi_line_mode_t get_spi_line_mode(enum mspi_io_mode io_mode)
{
	spi_line_mode_t line_mode = {0};

	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE:
		line_mode.cmd_lines = 1;
		line_mode.addr_lines = 1;
		line_mode.data_lines = 1;
		break;
	case MSPI_IO_MODE_DUAL:
	case MSPI_IO_MODE_DUAL_1_1_2:
		line_mode.cmd_lines = 1;
		line_mode.addr_lines = 1;
		line_mode.data_lines = 2;
		break;
	case MSPI_IO_MODE_DUAL_1_2_2:
		line_mode.cmd_lines = 1;
		line_mode.addr_lines = 2;
		line_mode.data_lines = 2;
		break;
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_1_4:
		line_mode.cmd_lines = 1;
		line_mode.addr_lines = 1;
		line_mode.data_lines = 4;
		break;
	case MSPI_IO_MODE_QUAD_1_4_4:
		line_mode.cmd_lines = 1;
		line_mode.addr_lines = 4;
		line_mode.data_lines = 4;
		break;
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_OCTAL_1_1_8:
		line_mode.cmd_lines = 1;
		line_mode.addr_lines = 1;
		line_mode.data_lines = 8;
		break;
	case MSPI_IO_MODE_OCTAL_1_8_8:
		line_mode.cmd_lines = 1;
		line_mode.addr_lines = 8;
		line_mode.data_lines = 8;
		break;
	default:
		return get_spi_line_mode(MSPI_IO_MODE_SINGLE);
	}

	return line_mode;
}

static int cs_gpio_set(const struct mspi_esp32_data *data, const struct mspi_esp32_config *config,
		       bool active)
{
	if (!config->mspi_config.ce_group || config->mspi_config.num_ce_gpios == 0) {
		LOG_DBG("CS: no GPIO CE group, skipping (HW CS in use)");
		return 0;
	}

	const struct gpio_dt_spec *cs = config->mspi_config.ce_group + data->mspi_dev_config.ce_num;

	bool pin_high =
		(data->mspi_dev_config.ce_polarity == MSPI_CE_ACTIVE_HIGH) ? active : !active;

	return gpio_pin_set_dt(cs, pin_high ? 1 : 0);
}

/* --------------------------------------------------------------------------
 * Timing configuration
 * --------------------------------------------------------------------------
 */

static int update_timing_config(const struct mspi_esp32_config *config,
				struct mspi_esp32_data *data, spi_hal_timing_conf_t *timing_conf,
				uint32_t clock_frequency)
{
	if (clock_frequency > config->mspi_config.max_freq) {
		LOG_ERR("Requested freq %u exceeds max %u", clock_frequency,
			config->mspi_config.max_freq);
		return -EINVAL;
	}

	data->clock_frequency = clock_frequency;

	spi_hal_timing_param_t tp = {
		.clk_src_hz = data->clock_source_hz,
		.half_duplex = data->dev_config.half_duplex,
		.no_compensate = data->dev_config.no_compensate,
		.expected_freq = clock_frequency,
		.duty_cycle = config->duty_cycle == 0 ? 128 : config->duty_cycle,
		.input_delay_ns = config->input_delay_ns,
		.use_gpio = !config->use_iomux,
	};

	LOG_DBG("timing: clk_src_hz=%u expected=%u duty=%u input_delay_ns=%u use_gpio=%d",
		tp.clk_src_hz, tp.expected_freq, tp.duty_cycle, tp.input_delay_ns, tp.use_gpio);

	/* New HAL: two-argument form, returns esp_err_t */
	esp_err_t err = spi_hal_cal_clock_conf(&tp, timing_conf);

	if (err != ESP_OK) {
		LOG_ERR("spi_hal_cal_clock_conf failed: %d", err);
		return -EINVAL;
	}

	LOG_DBG("timing: dummy_bits=%d", timing_conf->timing_dummy);

	return 0;
}

/* --------------------------------------------------------------------------
 * DMA helpers — GDMA (ESP32-S3, C3, C6, …)
 * --------------------------------------------------------------------------
 */

#ifdef SOC_GDMA_SUPPORTED

static int mspi_gdma_config(const struct device *dev, uint8_t dir, uint8_t *buf, size_t len)
{
	const struct mspi_esp32_config *cfg = dev->config;
	struct mspi_esp32_data *data = dev->data;
	uint8_t dma_channel = (dir == SPI_DMA_RX) ? cfg->dma_rx_ch : cfg->dma_tx_ch;
	bool *configured =
		(dir == SPI_DMA_RX) ? &data->rx_dma_configured : &data->tx_dma_configured;
	struct dma_status dma_status = {};
	int err = 0;

	if (dma_channel == 0xFF) {
		LOG_ERR("DMA channel is not configured in device tree");
		return -EINVAL;
	}

	err = dma_get_status(cfg->dma_dev, dma_channel, &dma_status);
	if (err) {
		return -EINVAL;
	}

	if (dma_status.busy) {
		LOG_ERR("DMA channel %d is busy", dma_channel);
		return -EBUSY;
	}

	if (*configured) {
		/*
		 * A fixed TX or RX channel's direction/slot/burst length never
		 * change once configured, so just rebind the descriptor chain to
		 * the new buffer/length instead of paying for a full dma_config()
		 * (channel/periph_id lookup + burst-length validation) on every
		 * chunk. Measured overhead made this worth it: per-chunk driver
		 * overhead was a significant fraction of each flush's SPI time.
		 */
		uint32_t addr = (uint32_t)buf;

		err = dma_reload(cfg->dma_dev, dma_channel, (dir == SPI_DMA_RX) ? 0 : addr,
				 (dir == SPI_DMA_RX) ? addr : 0, len);
		if (err) {
			LOG_ERR("Error reloading DMA (%d)", err);
		}

		return err;
	}

	struct dma_config dma_cfg = {};
	struct dma_block_config dma_blk = {};

	if (dir == SPI_DMA_RX) {
		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_blk.dest_address = (uint32_t)buf;
	} else {
		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_blk.source_address = (uint32_t)buf;
	}
	dma_cfg.dma_slot = cfg->dma_host;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_blk.block_size = len;

	dma_cfg.source_data_size = 4; /* 32-bit words */
	dma_cfg.dest_data_size = 4;
	dma_cfg.source_burst_length = 4;
	dma_cfg.dest_burst_length = 4;

	LOG_DBG("gdma_config: dir=%d ch=%u buf=%p len=%u slot=%d", dir, dma_channel, buf, len,
		cfg->dma_host);
	err = dma_config(cfg->dma_dev, dma_channel, &dma_cfg);
	if (err) {
		LOG_ERR("Error configuring DMA (%d)", err);
	} else {
		*configured = true;
	}

	return err;
}

static int mspi_gdma_start(const struct device *dev, uint8_t dir)
{
	const struct mspi_esp32_config *cfg = dev->config;
	uint8_t ch = (dir == SPI_DMA_RX) ? cfg->dma_rx_ch : cfg->dma_tx_ch;

	LOG_DBG("GDMA start: dir=%s ch=%u", dir == SPI_DMA_RX ? "RX" : "TX", ch);
	int ret = dma_start(cfg->dma_dev, ch);

	if (ret != 0) {
		LOG_ERR("GDMA start failed: ch=%u ret=%d", ch, ret);
	}
	return ret;
}

static void mspi_gdma_stop(const struct device *dev, uint8_t dir)
{
	const struct mspi_esp32_config *cfg = dev->config;
	uint8_t ch = (dir == SPI_DMA_RX) ? cfg->dma_rx_ch : cfg->dma_tx_ch;

	LOG_DBG("GDMA stop: dir=%s ch=%u", dir == SPI_DMA_RX ? "RX" : "TX", ch);
	dma_stop(cfg->dma_dev, ch);
}

#else /* !SOC_GDMA_SUPPORTED — integrated DMA on ESP32 / ESP32-S2 */

/* Setup DMA descriptor for ESP32/ESP32-S2 integrated DMA */
static void mspi_dma_desc_setup(lldesc_t *desc, uint8_t *buf, size_t len, bool is_rx)
{
	memset(desc, 0, sizeof(lldesc_t));
	desc->size = len;
	desc->length = is_rx ? 0 : len;
	desc->buf = buf;
	desc->owner = 1;
	desc->eof = 1;
	desc->sosf = 0;
	desc->offset = 0;
	desc->qe.stqe_next = NULL;
}

static void mspi_dma_rx_start(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct mspi_esp32_config *cfg = dev->config;
	struct mspi_esp32_data *data = dev->data;

	mspi_dma_desc_setup(&data->dma_desc_rx, buf, len, true);
	spi_dma_ll_rx_reset((spi_dma_dev_t *)cfg->spi, 0);
	spi_hal_hw_prepare_rx(cfg->spi);
	spi_dma_ll_rx_start((spi_dma_dev_t *)cfg->spi, 0, &data->dma_desc_rx);
}

static void mspi_dma_tx_start(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct mspi_esp32_config *cfg = dev->config;
	struct mspi_esp32_data *data = dev->data;

	mspi_dma_desc_setup(&data->dma_desc_tx, buf, len, false);
	spi_dma_ll_tx_reset((spi_dma_dev_t *)cfg->spi, 0);
	spi_hal_hw_prepare_tx(cfg->spi);
	spi_dma_ll_tx_start((spi_dma_dev_t *)cfg->spi, 0, &data->dma_desc_tx);
}

#endif /* SOC_GDMA_SUPPORTED */

/* --------------------------------------------------------------------------
 * transfer() sub-functions
 * --------------------------------------------------------------------------
 */

static void transfer_clear_fifo(spi_hal_context_t *hal)
{
	for (size_t i = 0; i < ARRAY_SIZE(hal->hw->data_buf); ++i) {
#ifdef CONFIG_SOC_SERIES_ESP32C6
		hal->hw->data_buf[i].val = 0;
#else
		hal->hw->data_buf[i] = 0;
#endif
	}
}

static void transfer_setup_cmd_addr(const struct mspi_xfer *xfer,
				    const struct mspi_xfer_packet *packet,
				    spi_hal_trans_config_t *tc)
{
	if (xfer->cmd_length > 0) {
		tc->cmd = (uint16_t)packet->cmd;
		tc->cmd_bits = xfer->cmd_length << 3;
	}

	if (xfer->addr_length > 0) {
		tc->addr = packet->address;
		tc->addr_bits = xfer->addr_length << 3;
	}
}

/*
 * Can the DMA engine be pointed straight at this buffer?
 *
 * Internal DRAM always works. GDMA also reaches PSRAM on SoCs advertising
 * SOC_PSRAM_DMA_CAPABLE, but only for a cache-line aligned address and length:
 * the cache maintenance around the transfer rounds outwards, so a partial line
 * at either end would write back or discard data the CPU still owns.
 */
static bool dma_reachable(const void *buf, size_t len)
{
	if (esp_ptr_dma_capable(buf)) {
		return true;
	}

#ifdef MSPI_ESP32_PSRAM_DMA
	if (esp_ptr_dma_ext_capable(buf)) {
		size_t line = esp_cache_get_line_size_by_addr(buf);

		return line != 0 && IS_ALIGNED(buf, line) && IS_ALIGNED(len, line);
	}
#else
	ARG_UNUSED(len);
#endif

	return false;
}

/*
 * Allocate a single bounce buffer for the data phase and wire it into tc.
 *
 * Since a packet is either TX or RX, only one buffer is ever needed.
 * Returns the allocated buffer (caller must k_free it), or NULL on error
 * (ret is set to a negative errno in that case).
 *
 * When dma_enabled and dir == MSPI_RX the same buffer doubles as the TX
 * zero-fill required by the hardware.
 */
static uint8_t *transfer_prepare_data(const struct device *dev,
				      const struct mspi_xfer_packet *packet,
				      spi_hal_trans_config_t *tc, size_t dma_len,
				      size_t *dma_buf_len, int *ret)
{
	const struct mspi_esp32_config *config = dev->config;
	uint8_t *buffer = NULL;
	*dma_buf_len = ROUND_UP(dma_len, 4);

	*ret = 0;

	if (packet->num_bytes == 0) {
		LOG_DBG("transfer: no data phase (num_bytes=0)");
		return NULL;
	}

	int bit_len = (int)(dma_len << 3);

	if (packet->dir == MSPI_TX) {
		uint8_t *src = packet->data_buf;
		bool need_bounce = config->dma_enabled && !dma_reachable(src, dma_len);

		if (need_bounce) {
			buffer = k_aligned_alloc(4, *dma_buf_len);
			if (!buffer) {
				*ret = -ENOMEM;
				return NULL;
			}
			memcpy(buffer, src, dma_len);
			src = buffer;
		} else {
			LOG_DBG("TX buf %p ok (non-DMA path)", src);
		}

		tc->send_buffer = src;
		tc->tx_bitlen = bit_len;
		tc->rx_bitlen = bit_len;

	} else { /* MSPI_RX */
		uint8_t *dst = packet->data_buf;
		bool need_bounce = config->dma_enabled || !esp_ptr_dma_capable((uint32_t *)dst) ||
				   ((uintptr_t)dst % 4 != 0) || (dma_len % 4 != 0);

		if (need_bounce) {
			*dma_buf_len = ((dma_len << 3) + 31) / 8;
			buffer = k_calloc(*dma_buf_len, sizeof(uint8_t));
			if (!buffer) {
				*ret = -ENOMEM;
				return NULL;
			}
			dst = buffer;
		} else {
			LOG_DBG("RX buf %p ok (non-DMA path)", dst);
		}

		tc->rcv_buffer = dst;
		tc->rx_bitlen = bit_len;

		/*
		 * DMA RX-only: hardware also needs a TX buffer (zero-filled).
		 * Reuse the same bounce buffer by pointing send_buffer at it;
		 * the buffer is already zeroed by k_calloc so no extra fill is needed.
		 */
		if (config->dma_enabled && !tc->send_buffer) {
			tc->send_buffer = dst;
			tc->tx_bitlen = bit_len;
		}
	}

	return buffer;
}

#ifdef SOC_GDMA_SUPPORTED

static int transfer_start_gdma(const struct device *dev, spi_hal_context_t *hal,
			       spi_hal_trans_config_t *tc, size_t dma_buf_len)
{
	int res = 0;

	/*
	 * GDMA reads/writes physical RAM directly and bypasses the CPU data
	 * cache. Buffers can live in cached PSRAM, so without explicit cache
	 * maintenance the peripheral may read stale TX data, or the CPU may
	 * read stale RX data after the transfer — an intermittent-corruption
	 * bug that only shows up under cache pressure.
	 */
	if (tc->rcv_buffer) {
		sys_cache_data_flush_and_invd_range(tc->rcv_buffer, dma_buf_len);
		res = mspi_gdma_config(dev, SPI_DMA_RX, tc->rcv_buffer, dma_buf_len);
		if (res) {
			return res;
		}
	}
	if (tc->send_buffer) {
		sys_cache_data_flush_range(tc->send_buffer, dma_buf_len);
		res = mspi_gdma_config(dev, SPI_DMA_TX, tc->send_buffer, dma_buf_len);
		if (res) {
			return res;
		}
	}

	/* Enable RX DMA path first (same order as SPI driver) */
	if (tc->rcv_buffer) {
		spi_ll_dma_rx_fifo_reset(hal->hw);
		spi_ll_infifo_full_clr(hal->hw);
		spi_ll_dma_rx_enable(hal->hw, 1);

		res = mspi_gdma_start(dev, SPI_DMA_RX);
		if (res) {
			return res;
		}
	}
	if (tc->send_buffer) {
		spi_ll_dma_tx_fifo_reset(hal->hw);
		spi_ll_outfifo_empty_clr(hal->hw);
		spi_ll_dma_tx_enable(hal->hw, 1);

		res = mspi_gdma_start(dev, SPI_DMA_TX);
		if (res) {
			return res;
		}
	}

	/*
	 * Enable data lines AFTER DMA is running — required ordering
	 * documented in the SPI driver comment.
	 */
	spi_hal_enable_data_line(hal->hw, tc->send_buffer != NULL, tc->rcv_buffer != NULL);

	return 0;
}

#else /* !SOC_GDMA_SUPPORTED */

static int transfer_start_intdma(const struct device *dev, spi_hal_context_t *hal,
				 const spi_hal_dev_config_t *hal_dev, spi_hal_trans_config_t *tc,
				 size_t dma_buf_len)
{
	LOG_DBG("transfer: integrated DMA path");

	if (tc->rcv_buffer) {
		LOG_DBG("transfer: intDMA RX start buf=%p len=%zu", tc->rcv_buffer, dma_buf_len);
		mspi_dma_rx_start(dev, tc->rcv_buffer, dma_buf_len);
	}
#ifdef CONFIG_SOC_SERIES_ESP32
	else if (!hal_dev->half_duplex) {
		/*
		 * ESP32 errata: RX DMA channel must be running for
		 * TX-only full-duplex transfers.
		 */
		LOG_DBG("transfer: ESP32 errata — dummy RX DMA start");
		spi_ll_dma_rx_enable(hal->hw, 1);
		spi_dma_ll_rx_start((spi_dma_dev_t *)hal->hw, 0, NULL);
	}
#endif
	if (tc->send_buffer) {
		LOG_DBG("transfer: intDMA TX start buf=%p len=%zu", tc->send_buffer, dma_buf_len);
		mspi_dma_tx_start(dev, (uint8_t *)tc->send_buffer, dma_buf_len);
	}

	LOG_DBG("transfer: intDMA enable_data_line tx=%d rx=%d", tc->send_buffer != NULL,
		tc->rcv_buffer != NULL);
	spi_hal_enable_data_line(hal->hw, tc->send_buffer != NULL, tc->rcv_buffer != NULL);

	return 0;
}

#endif /* SOC_GDMA_SUPPORTED */

#ifdef CONFIG_MSPI_ESP32_INTERRUPT

static void IRAM_ATTR mspi_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct mspi_esp32_config *config = dev->config;
	struct mspi_esp32_data *data = dev->data;

	spi_ll_disable_int(config->spi);
	spi_ll_clear_int_stat(config->spi);

	k_sem_give(&data->xfer_sem);
}

#endif /* CONFIG_MSPI_ESP32_INTERRUPT */

/*
 * Start the HW transaction configured by spi_hal_setup_trans() and block
 * until it completes or timeout_ms elapses.
 *
 * With CONFIG_MSPI_ESP32_INTERRUPT, the caller blocks on a semaphore signalled
 * by the SPI "transaction done" interrupt, freeing this thread's CPU core for
 * other work while the transfer is in flight. Without it, fall back to a
 * time-bounded poll so a stuck peripheral can never hang the caller (and thus
 * the rest of the system) forever.
 */
static int IRAM_ATTR transfer_start_and_wait(const struct device *dev, uint32_t timeout_ms)
{
	struct mspi_esp32_data *data = dev->data;
	spi_hal_context_t *hal = &data->hal;

#ifdef CONFIG_MSPI_ESP32_INTERRUPT
	const struct mspi_esp32_config *config = dev->config;

	k_sem_reset(&data->xfer_sem);
	spi_ll_clear_int_stat(config->spi);
	spi_ll_enable_int(config->spi);

	spi_hal_user_start(hal);

	if (k_sem_take(&data->xfer_sem, K_MSEC(timeout_ms)) != 0) {
		spi_ll_disable_int(config->spi);
		spi_ll_clear_int_stat(config->spi);
		LOG_ERR("Transfer timed out after %u ms", timeout_ms);
		return -ETIMEDOUT;
	}
#else
	int64_t deadline = k_uptime_get() + timeout_ms;

	spi_hal_user_start(hal);

	while (!spi_hal_usr_is_done(hal)) {
		if (k_uptime_get() > deadline) {
			LOG_ERR("Transfer timed out after %u ms", timeout_ms);
			return -ETIMEDOUT;
		}
	}
#endif

	return 0;
}

static void transfer_post_gdma(const struct device *dev, spi_hal_trans_config_t *tc,
			       size_t dma_buf_len)
{
#ifdef SOC_GDMA_SUPPORTED
	if (tc->rcv_buffer) {
		mspi_gdma_stop(dev, SPI_DMA_RX);
		LOG_DBG("transfer: cache invalidate RX buf=%p len=%zu", tc->rcv_buffer,
			dma_buf_len);
		/* Force the CPU to re-read the data the DMA just wrote. */
		sys_cache_data_invd_range(tc->rcv_buffer, dma_buf_len);
	}
	if (tc->send_buffer) {
		mspi_gdma_stop(dev, SPI_DMA_TX);
	}
#endif
}

/* --------------------------------------------------------------------------
 * Core per-packet transfer
 * --------------------------------------------------------------------------
 */

static int IRAM_ATTR transfer(const struct device *dev, const struct mspi_xfer *xfer,
			      size_t packet_index)
{
	struct mspi_esp32_data *data = dev->data;
	const struct mspi_esp32_config *config = dev->config;
	spi_hal_context_t *hal = &data->hal;
	const spi_hal_dev_config_t *hal_dev = &data->dev_config;

	const struct mspi_xfer_packet *packet = &xfer->packets[packet_index];

	if (packet->num_bytes > CONFIG_MSPI_DMA_MAX_BUFFER_SIZE) {
		LOG_ERR("Packet %zu: %u bytes exceeds MSPI_DMA_MAX_BUFFER_SIZE (%u); "
			"caller must chunk large transfers",
			packet_index, packet->num_bytes,
			(unsigned int)CONFIG_MSPI_DMA_MAX_BUFFER_SIZE);
		return -EMSGSIZE;
	}

	const uint32_t timeout_ms = xfer->timeout ? xfer->timeout : config->transfer_timeout;

	spi_hal_trans_config_t tc = {0};
	/*
	 * These dummy cycles exist purely to give the master extra settling
	 * time before it samples MISO on a read — ESP-IDF's own
	 * spi_hal_setup_trans() only ever applies its equivalent compensation
	 * when trans->rcv_buffer is set. Applying them unconditionally (as the
	 * upstream spi_esp32_spim.c reference driver also does) wastes bus
	 * time on every TX-only transaction.
	 */
	tc.dummy_bits = (packet->dir == MSPI_RX) ? data->trans_config.dummy_bits : 0;
	tc.line_mode = data->trans_config.line_mode;
	tc.cs_keep_active = xfer->hold_ce || (packet_index < (xfer->num_packet - 1));

	uint8_t *tmp = NULL;
	int res = 0;

	transfer_clear_fifo(hal);
	transfer_setup_cmd_addr(xfer, packet, &tc);

	size_t dma_len = MIN(packet->num_bytes, CONFIG_MSPI_DMA_MAX_BUFFER_SIZE);
	size_t dma_buf_len = 0;

	tmp = transfer_prepare_data(dev, packet, &tc, dma_len, &dma_buf_len, &res);
	if (res != 0) {
		goto cleanup;
	}

	if (config->dma_enabled) {
#ifdef SOC_GDMA_SUPPORTED
		res = transfer_start_gdma(dev, hal, &tc, dma_buf_len);
#endif
	}

	spi_hal_setup_trans(hal, hal_dev, &tc);

	res = transfer_start_and_wait(dev, timeout_ms);
	if (res != 0) {
		goto cleanup;
	}

	if (config->dma_enabled) {
		transfer_post_gdma(dev, &tc, dma_buf_len);
	} else {
		spi_hal_fetch_result(hal);
	}

	/* Copy bounce buffer back to caller's RX buffer */
	if (tmp && packet->dir == MSPI_RX && packet->data_buf) {
		memcpy(packet->data_buf, tmp, MIN(packet->num_bytes, dma_len));
	}

	LOG_DBG("%s[%zu]: success", __func__, packet_index);

cleanup:
	k_free(tmp);
	return res;
}

/* --------------------------------------------------------------------------
 * Device-level configuration — applied once per transceive() call
 * --------------------------------------------------------------------------
 */

static int IRAM_ATTR update_transfer_config(const struct device *dev)
{
	const struct mspi_esp32_config *config = dev->config;
	struct mspi_esp32_data *data = dev->data;
	spi_hal_context_t *hal = &data->hal;
	spi_hal_dev_config_t *hal_dev = &data->dev_config;

	/*
	 * Snapshot the device-level fields that every packet will inherit.
	 * Packet-level fields (cmd, addr, send_buffer, …) are set in transfer().
	 */
	data->trans_config.dummy_bits = hal_dev->timing_conf.timing_dummy;
	data->trans_config.line_mode = get_spi_line_mode(data->mspi_dev_config.io_mode);

	hal_dev->tx_lsbfirst = false;
	hal_dev->rx_lsbfirst = false;

	LOG_DBG("%s: mode=%d cs_pin_id=%d positive_cs=%d "
		"half_duplex=%d dummy_bits=%u io_mode=%d "
		"lines(cmd=%u addr=%u data=%u)",
		__func__, hal_dev->mode, hal_dev->cs_pin_id, hal_dev->positive_cs,
		hal_dev->half_duplex, data->trans_config.dummy_bits, data->mspi_dev_config.io_mode,
		data->trans_config.line_mode.cmd_lines, data->trans_config.line_mode.addr_lines,
		data->trans_config.line_mode.data_lines);

	/*
	 * The rest of this function reprograms device-level SPI registers and,
	 * on some SoCs, runs an extra dummy transaction below. Neither depends
	 * on io_mode, so skip both unless mspi_esp32_dev_config() actually
	 * changed CPP/CE/frequency. Without this, every single transceive()
	 * call — i.e. every command byte and every data chunk — pays for
	 * a full device reconfiguration plus an extra SPI transaction.
	 */
	if (!data->dev_hw_dirty) {
		return 0;
	}

	/* Program device-level SPI registers */
	spi_hal_setup_device(hal, hal_dev);

	/* ------------------------------------------------------------------
	 * Idle-polarity workaround — not available on original ESP32
	 * ------------------------------------------------------------------
	 */
#ifndef CONFIG_SOC_SERIES_ESP32
	{
		spi_dev_t *hw = hal->hw;
		int pol = config->line_idle_low ? 0 : 1;

		hw->ctrl.d_pol = pol;
		hw->ctrl.q_pol = pol;
		hw->ctrl.hold_pol = pol;
		hw->ctrl.wp_pol = pol;
	}
#endif

	/* ------------------------------------------------------------------
	 * CLK/CS phase-alignment dummy transaction.
	 *
	 * On ESP32-S3 and ESP32-Cx, software-driven CS can leave CLK out of
	 * phase after a mode-3 reconfiguration.  Send one zero byte to
	 * re-align before the real transaction.
	 *
	 * Not needed on ESP32 or ESP32-S2 (hardware manages it).
	 * ------------------------------------------------------------------
	 */
#if !defined(CONFIG_SOC_SERIES_ESP32) && !defined(CONFIG_SOC_SERIES_ESP32S2)
	if (config->mspi_config.num_ce_gpios > 0 && hal_dev->mode == 3) {
		uint8_t dummy_byte = 0x00;
		struct mspi_xfer_packet dummy_pkt = {
			.dir = MSPI_TX,
			.data_buf = &dummy_byte,
			.num_bytes = sizeof(dummy_byte),
			.cmd = 0,
			.address = 0,
		};
		struct mspi_xfer dummy_xfer = {
			.packets = &dummy_pkt,
			.num_packet = 1,
			.async = false,
			.timeout = 100,
			.hold_ce = false,
			.cmd_length = 0,
			.addr_length = 0,
		};

		transfer(dev, &dummy_xfer, 0);
	}
#endif

	data->dev_hw_dirty = false;

	return 0;
}

/* --------------------------------------------------------------------------
 * MSPI API — dev_config
 * --------------------------------------------------------------------------
 */

static int mspi_esp32_dev_config(const struct device *dev,
				 [[maybe_unused]] const struct mspi_dev_id *dev_id,
				 const enum mspi_dev_cfg_mask cfg_mask,
				 const struct mspi_dev_cfg *mspi_dev_config)
{
	struct mspi_esp32_data *data = dev->data;
	const struct mspi_esp32_config *config = dev->config;

	const enum mspi_dev_cfg_mask supported =
		MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_CPP | MSPI_DEVICE_CONFIG_CE_NUM |
		MSPI_DEVICE_CONFIG_CE_POL | MSPI_DEVICE_CONFIG_FREQUENCY;

	if (cfg_mask & ~supported) {
		LOG_ERR("Unsupported config mask bits 0x%x", cfg_mask & ~supported);
		return -ENOTSUP;
	}

	/*
	 * Only these fields feed spi_hal_setup_device() / the mode-3 dummy
	 * workaround in update_transfer_config(). io_mode alone must not force
	 * a device-register reprogram on every single transfer.
	 */
	if (cfg_mask & (MSPI_DEVICE_CONFIG_CPP | MSPI_DEVICE_CONFIG_CE_NUM |
			MSPI_DEVICE_CONFIG_CE_POL | MSPI_DEVICE_CONFIG_FREQUENCY)) {
		data->dev_hw_dirty = true;
	}

	if (cfg_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		data->mspi_dev_config.io_mode = mspi_dev_config->io_mode;
	}

	if (cfg_mask & MSPI_DEVICE_CONFIG_CPP) {
		int mode = mspi_mode_to_esp32(mspi_dev_config->cpp);

		if (mode < 0) {
			return mode;
		}
		data->mspi_dev_config.cpp = mspi_dev_config->cpp;
		data->dev_config.mode = mode;
	}

	if (cfg_mask & MSPI_DEVICE_CONFIG_CE_NUM) {
		data->mspi_dev_config.ce_num = mspi_dev_config->ce_num;
		/*
		 * Hardware CS pin id: use GPIO-driven CS when ce_group is
		 * populated (set cs_pin_id to -1 to disable HW CS), otherwise
		 * map directly to the HW CS line.
		 */
		data->dev_config.cs_pin_id =
			(config->mspi_config.num_ce_gpios > 0) ? -1 : (int)mspi_dev_config->ce_num;
		/* Switching target: any previously-held CE line no longer applies. */
		data->cs_active = false;
	}

	if (cfg_mask & MSPI_DEVICE_CONFIG_CE_POL) {
		data->mspi_dev_config.ce_polarity = mspi_dev_config->ce_polarity;
		data->dev_config.positive_cs =
			(mspi_dev_config->ce_polarity == MSPI_CE_ACTIVE_HIGH);
		data->cs_active = false;
	}

	if (cfg_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
		int ret = update_timing_config(config, data, &data->dev_config.timing_conf,
					       mspi_dev_config->freq);
		if (ret != 0) {
			LOG_ERR("Failed to recalculate timing: %d", ret);
			return ret;
		}
	}

	return 0;
}

/* --------------------------------------------------------------------------
 * MSPI API — transceive
 * --------------------------------------------------------------------------
 */

static int IRAM_ATTR mspi_esp32_transceive(const struct device *dev,
					   [[maybe_unused]] const struct mspi_dev_id *dev_id,
					   const struct mspi_xfer *xfer)
{
	struct mspi_esp32_data *data = dev->data;
	const struct mspi_esp32_config *config = dev->config;
	int ret = 0;

	if (!xfer || xfer->num_packet == 0 || !xfer->packets) {
		LOG_ERR("Invalid transfer parameters");
		return -EINVAL;
	}

	if (xfer->async) {
		LOG_ERR("Async mode not supported");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = update_transfer_config(dev);
	if (ret != 0) {
		LOG_ERR("Failed to configure SPI device: %d", ret);
		goto unlock;
	}

	if (!data->cs_active) {
		ret = cs_gpio_set(data, config, true);
		if (ret != 0) {
			LOG_ERR("Failed to assert CS: %d", ret);
			goto unlock;
		}
		data->cs_active = true;
	}

	for (size_t i = 0; i < xfer->num_packet; i++) {
		ret = transfer(dev, xfer, i);
		if (ret != 0) {
			LOG_ERR("Packet %zu failed: %d", i, ret);
			break;
		}
	}

	if (!xfer->hold_ce || ret != 0) {
		int cs_ret = cs_gpio_set(data, config, false);

		data->cs_active = false;

		if (cs_ret != 0) {
			LOG_ERR("Failed to de-assert CS: %d", cs_ret);
			if (ret == 0) {
				ret = cs_ret;
			}
		}
	}

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/* --------------------------------------------------------------------------
 * DMA initialisation
 * --------------------------------------------------------------------------
 */

static int init_dma(const struct device *dev)
{
	const struct mspi_esp32_config *config = dev->config;
	struct mspi_esp32_data *data = dev->data;

#ifdef SOC_GDMA_SUPPORTED
	/*
	 * GDMA: DMA is driven entirely by the Zephyr DMA driver.
	 * Do NOT set hal.dma_enabled — that selects the legacy integrated-DMA
	 * path inside the ESP-IDF HAL and must remain false for GDMA targets.
	 */

	LOG_DBG("%s: GDMA path", __func__);

	if (!device_is_ready(config->dma_dev)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}

	LOG_DBG("%s: GDMA dma_dev ready, tx_ch=%u rx_ch=%u slot=%d", __func__, config->dma_tx_ch,
		config->dma_rx_ch, config->dma_host);

	data->hal.dma_enabled = false;
	/* Force the next transfer on each channel to run a full dma_config(). */
	data->tx_dma_configured = false;
	data->rx_dma_configured = false;

#else /* !SOC_GDMA_SUPPORTED — integrated DMA (ESP32, ESP32-S2) */

	LOG_DBG("%s: integrated DMA path, enabling DMA clock", __func__);
	if (clock_control_on(config->clock_dev, (clock_control_subsys_t)config->dma_clk_src)) {
		LOG_ERR("Could not enable DMA clock");
		return -EIO;
	}
	data->hal.dma_enabled = true;
	LOG_DBG("%s: DMA clock enabled, dma_host=%d", __func__, config->dma_host);

#ifdef CONFIG_SOC_SERIES_ESP32
	/* Wire the SPI peripheral to its DMA channel */
	DPORT_SET_PERI_REG_BITS(DPORT_SPI_DMA_CHAN_SEL_REG, 3, config->dma_host + 1,
				(config->dma_host + 1) * 2);
	LOG_DBG("%s: ESP32 DMA channel wired", __func__);
#endif

#endif /* SOC_GDMA_SUPPORTED */

	LOG_DBG("%s: done", __func__);
	return 0;
}

/* --------------------------------------------------------------------------
 * CS GPIO setup
 * --------------------------------------------------------------------------
 */

static int cs_configure(const struct device *dev)
{
	const struct mspi_esp32_data *data = dev->data;
	const struct mspi_esp32_config *config = dev->config;
	const struct mspi_cfg *mspi_config = &config->mspi_config;

	for (int i = 0; i < mspi_config->num_ce_gpios; ++i) {
		const struct gpio_dt_spec *cs = mspi_config->ce_group + i;

		if (!device_is_ready(cs->port)) {
			LOG_ERR("CS GPIO %s pin %d not ready", cs->port->name, cs->pin);
			return -ENODEV;
		}

		bool pin_high = (data->mspi_dev_config.ce_polarity == MSPI_CE_ACTIVE_LOW);
		int ret = gpio_pin_configure_dt(cs, pin_high ? GPIO_OUTPUT_ACTIVE
							     : GPIO_OUTPUT_INACTIVE);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

/* --------------------------------------------------------------------------
 * Controller-level configuration  (mspi_config API entry point)
 * --------------------------------------------------------------------------
 */

static int mspi_esp32_config(const struct mspi_dt_spec *spec)
{
	const struct device *dev = spec->bus;
	struct mspi_esp32_data *data = dev->data;
	const struct mspi_esp32_config *config = dev->config;
	int ret;

	/* Force the next transceive() to reprogram device-level registers. */
	data->dev_hw_dirty = true;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to configure pins: %d", ret);
		return ret;
	}

	if (!config->clock_dev || !device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}
	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable SPI clock: %d", ret);
		return ret;
	}

	spi_hal_init(&data->hal, config->peripheral_id + 1);
	spi_ll_enable_clock(config->peripheral_id + 1, true);

	ret = esp_clk_tree_src_get_freq_hz(config->clock_source,
					   ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX,
					   &data->clock_source_hz);
	if (ret != ESP_OK) {
		LOG_ERR("Failed to get clock source frequency: %d", ret);
		return -EINVAL;
	}
	LOG_DBG("mspi_config: clock_source_hz=%u", data->clock_source_hz);

	data->mspi_dev_config.freq = config->mspi_config.max_freq;
	ret = update_timing_config(config, data, &data->dev_config.timing_conf,
				   data->mspi_dev_config.freq);
	if (ret != 0) {
		LOG_ERR("Failed to compute timing config: %d", ret);
		return ret;
	}

	if (config->dma_enabled) {
		ret = init_dma(dev);
		if (ret != 0) {
			LOG_ERR("DMA init failed: %d", ret);
			return ret;
		}
	}

	ret = cs_configure(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure CS GPIOs: %d", ret);
		return ret;
	}

	LOG_INF("ESP32 MSPI (peripheral %u) configured at %u Hz", config->peripheral_id,
		config->mspi_config.max_freq);

	return 0;
}

/* --------------------------------------------------------------------------
 * Driver init
 * --------------------------------------------------------------------------
 */

static int mspi_esp32_init(const struct device *dev)
{
	struct mspi_esp32_data *data = dev->data;
	const struct mspi_esp32_config *config = dev->config;

	k_mutex_init(&data->lock);
	k_sem_init(&data->xfer_sem, 0, 1);

	memset(&data->dev_config, 0, sizeof(data->dev_config));
	memset(&data->trans_config, 0, sizeof(data->trans_config));

	data->dev_config.cs_setup = 0;
	data->dev_config.cs_hold = 0;
	data->dev_config.half_duplex = (config->mspi_config.duplex == MSPI_HALF_DUPLEX);
	data->dev_config.tx_lsbfirst = false;
	data->dev_config.rx_lsbfirst = false;
	data->dev_config.no_compensate = false;

	/* Force the first transceive() to program the device-level registers. */
	data->dev_hw_dirty = true;

	/*
	 * Default CS pin: GPIO-driven CS disables hardware CS (cs_pin_id = -1).
	 * Hardware CS maps directly: 0 → CS0, 1 → CS1, 2 → CS2.
	 */
	data->dev_config.cs_pin_id = (config->mspi_config.num_ce_gpios > 0) ? -1 : 0;

	data->clock_frequency = config->mspi_config.max_freq;

	const struct mspi_dt_spec spec = {
		.bus = dev,
		.config = config->mspi_config,
	};

	int ret = mspi_esp32_config(&spec);

	if (ret != 0) {
		LOG_ERR("MSPI config failed: %d", ret);
		return ret;
	}

#ifdef CONFIG_MSPI_ESP32_INTERRUPT
	spi_ll_disable_int(config->spi);
	spi_ll_clear_int_stat(config->spi);

	ret = esp_intr_alloc(config->irq_source,
			     ESP_PRIO_TO_FLAGS(config->irq_priority) |
				     ESP_INT_FLAGS_CHECK(config->irq_flags) | ESP_INTR_FLAG_IRAM,
			     (intr_handler_t)mspi_esp32_isr, (void *)dev, NULL);
	if (ret != 0) {
		LOG_ERR("Could not allocate MSPI interrupt: %d", ret);
		return ret;
	}
#endif

	LOG_INF("ESP32 MSPI driver initialised (peripheral %u)", config->peripheral_id);

	return 0;
}

/* --------------------------------------------------------------------------
 * API table & instance macros
 * --------------------------------------------------------------------------
 */

static DEVICE_API(mspi, mspi_esp32_api) = {
	.config = mspi_esp32_config,
	.dev_config = mspi_esp32_dev_config,
	.transceive = mspi_esp32_transceive,
};

#ifdef SOC_GDMA_SUPPORTED
#define MSPI_DMA_CFG(inst)                                                                         \
	.dma_dev = ESP32_DT_INST_DMA_CTLR(inst, tx),                                               \
	.dma_tx_ch = ESP32_DT_INST_DMA_CELL(inst, tx, channel),                                    \
	.dma_rx_ch = ESP32_DT_INST_DMA_CELL(inst, rx, channel),
#else
#define MSPI_DMA_CFG(inst) .dma_clk_src = DT_INST_PROP(inst, dma_clk),
#endif

#define MSPI_CONTROLLER_CONFIG(inst)                                                               \
	{                                                                                          \
		.channel_num = DT_INST_PROP(inst, peripheral_id),                                  \
		.op_mode = MSPI_OP_MODE_CONTROLLER,                                                \
		.duplex = DT_ENUM_IDX_OR(DT_DRV_INST(inst), duplex, MSPI_HALF_DUPLEX),             \
		.max_freq = DT_INST_PROP(inst, clock_frequency),                                   \
		.dqs_support = false,                                                              \
		.num_periph = DT_INST_CHILD_NUM(inst),                                             \
		.sw_multi_periph = DT_INST_PROP_OR(inst, software_multiperipheral, false),         \
		.re_init = false,                                                                  \
		.ce_group = (struct gpio_dt_spec *)ce_gpios##inst,                                 \
		.num_ce_gpios = ARRAY_SIZE(ce_gpios##inst),                                        \
	}

#define ESP32_MSPI_INIT(inst)                                                                      \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct gpio_dt_spec ce_gpios##inst[] = MSPI_CE_GPIOS_DT_SPEC_INST_GET(inst);        \
                                                                                                   \
	static struct mspi_esp32_data mspi_esp32_data_##inst = {                                   \
		.hal =                                                                             \
			{                                                                          \
				.hw = (spi_dev_t *)DT_INST_REG_ADDR(inst),                         \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static const struct mspi_esp32_config mspi_esp32_cfg_##inst = {                            \
		.spi = (spi_dev_t *)DT_INST_REG_ADDR(inst),                                        \
		.peripheral_id = DT_INST_PROP(inst, peripheral_id),                                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.mspi_config = MSPI_CONTROLLER_CONFIG(inst),                                       \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                   \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, offset),         \
		.clock_source = DT_ENUM_IDX_OR(DT_DRV_INST(inst), clk_src, SPI_CLK_SRC_DEFAULT),   \
		.dma_enabled = DT_INST_PROP_OR(inst, dma_enabled, false),                          \
		.dma_host = DT_INST_PROP(inst, dma_host),                                          \
		MSPI_DMA_CFG(inst).line_idle_low = DT_INST_PROP_OR(inst, line_idle_low, false),    \
		.use_iomux = DT_INST_PROP_OR(inst, use_iomux, false),                              \
		.duty_cycle = DT_INST_PROP(inst, duty_cycle),                                      \
		.transfer_timeout = DT_INST_PROP(inst, transfer_timeout),                          \
		.input_delay_ns = 0,                                                               \
		.irq_source = DT_INST_IRQ_BY_IDX(inst, 0, irq),                                    \
		.irq_priority = DT_INST_IRQ_BY_IDX(inst, 0, priority),                             \
		.irq_flags = DT_INST_IRQ_BY_IDX(inst, 0, flags),                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mspi_esp32_init, NULL, &mspi_esp32_data_##inst,                \
			      &mspi_esp32_cfg_##inst, POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY,      \
			      &mspi_esp32_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_MSPI_INIT)
