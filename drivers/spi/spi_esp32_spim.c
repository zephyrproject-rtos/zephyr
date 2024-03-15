/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_spi

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <hal/spi_hal.h>
#include <esp_attr.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_spi, CONFIG_SPI_LOG_LEVEL);

#include <soc.h>
#include <soc/soc_memory_types.h>
#include <zephyr/drivers/spi.h>
#ifndef CONFIG_SOC_SERIES_ESP32C3
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#else
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#endif
#ifdef SOC_GDMA_SUPPORTED
#include <hal/gdma_hal.h>
#include <hal/gdma_ll.h>
#endif
#include <zephyr/drivers/clock_control.h>
#include "spi_context.h"
#include "spi_esp32_spim.h"

#ifdef CONFIG_SOC_SERIES_ESP32C3
#define ISR_HANDLER isr_handler_t
#else
#define ISR_HANDLER intr_handler_t
#endif

#define SPI_DMA_MAX_BUFFER_SIZE 4092

static bool spi_esp32_transfer_ongoing(struct spi_esp32_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static inline void spi_esp32_complete(const struct device *dev,
				      struct spi_esp32_data *data,
				      spi_dev_t *spi, int status)
{
#ifdef CONFIG_SPI_ESP32_INTERRUPT
	spi_ll_disable_int(spi);
	spi_ll_clear_int_stat(spi);
#endif

	spi_context_cs_control(&data->ctx, false);

#ifdef CONFIG_SPI_ESP32_INTERRUPT
	spi_context_complete(&data->ctx, dev, status);
#endif

}

static int IRAM_ATTR spi_esp32_transfer(const struct device *dev)
{
	struct spi_esp32_data *data = dev->data;
	const struct spi_esp32_config *cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	spi_hal_context_t *hal = &data->hal;
	spi_hal_dev_config_t *hal_dev = &data->dev_config;
	spi_hal_trans_config_t *hal_trans = &data->trans_config;
	size_t chunk_len_bytes = spi_context_max_continuous_chunk(&data->ctx) * data->dfs;
	size_t max_buf_sz =
		cfg->dma_enabled ? SPI_DMA_MAX_BUFFER_SIZE : SOC_SPI_MAXIMUM_BUFFER_SIZE;
	size_t transfer_len_bytes = MIN(chunk_len_bytes, max_buf_sz);
	size_t transfer_len_frames = transfer_len_bytes / data->dfs;
	size_t bit_len = transfer_len_bytes << 3;
	uint8_t *rx_temp = NULL;
	uint8_t *tx_temp = NULL;
	uint8_t dma_len_tx = MIN(ctx->tx_len * data->dfs, SPI_DMA_MAX_BUFFER_SIZE);
	uint8_t dma_len_rx = MIN(ctx->rx_len * data->dfs, SPI_DMA_MAX_BUFFER_SIZE);

	if (cfg->dma_enabled) {
		/* bit_len needs to be at least one byte long when using DMA */
		bit_len = !bit_len ? 8 : bit_len;
		if (ctx->tx_buf && !esp_ptr_dma_capable((uint32_t *)&ctx->tx_buf[0])) {
			LOG_DBG("Tx buffer not DMA capable");
			tx_temp = k_malloc(dma_len_tx);
			if (!tx_temp) {
				LOG_ERR("Error allocating temp buffer Tx");
				return -ENOMEM;
			}
			memcpy(tx_temp, &ctx->tx_buf[0], dma_len_tx);
		}
		if (ctx->rx_buf && (!esp_ptr_dma_capable((uint32_t *)&ctx->rx_buf[0]) ||
				    ((int)&ctx->rx_buf[0] % 4 != 0) || (dma_len_rx % 4 != 0))) {
			/* The rx buffer need to be length of
			 * multiples of 32 bits to avoid heap
			 * corruption.
			 */
			LOG_DBG("Rx buffer not DMA capable");
			rx_temp = k_calloc(((dma_len_rx << 3) + 31) / 8, sizeof(uint8_t));
			if (!rx_temp) {
				LOG_ERR("Error allocating temp buffer Rx");
				k_free(tx_temp);
				return -ENOMEM;
			}
		}
	}

	/* clean up and prepare SPI hal */
	memset((uint32_t *)hal->hw->data_buf, 0, sizeof(hal->hw->data_buf));
	hal_trans->send_buffer = tx_temp ? tx_temp : (uint8_t *)ctx->tx_buf;
	hal_trans->rcv_buffer = rx_temp ? rx_temp : ctx->rx_buf;
	hal_trans->tx_bitlen = bit_len;
	hal_trans->rx_bitlen = bit_len;

	/* keep cs line active until last transmission */
	hal_trans->cs_keep_active =
		(!ctx->num_cs_gpios &&
		 (ctx->rx_count > 1 || ctx->tx_count > 1 || ctx->rx_len > transfer_len_frames ||
		  ctx->tx_len > transfer_len_frames));

	/* configure SPI */
	spi_hal_setup_trans(hal, hal_dev, hal_trans);
	spi_hal_prepare_data(hal, hal_dev, hal_trans);

	/* send data */
	spi_hal_user_start(hal);
	spi_context_update_tx(&data->ctx, data->dfs, transfer_len_frames);

	while (!spi_hal_usr_is_done(hal)) {
		/* nop */
	}

	/* read data */
	spi_hal_fetch_result(hal);

	if (rx_temp) {
		memcpy(&ctx->rx_buf[0], rx_temp, transfer_len_bytes);
	}

	spi_context_update_rx(&data->ctx, data->dfs, transfer_len_frames);

	k_free(tx_temp);
	k_free(rx_temp);

	return 0;
}

#ifdef CONFIG_SPI_ESP32_INTERRUPT
static void IRAM_ATTR spi_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct spi_esp32_config *cfg = dev->config;
	struct spi_esp32_data *data = dev->data;

	do {
		spi_esp32_transfer(dev);
	} while (spi_esp32_transfer_ongoing(data));

	spi_esp32_complete(dev, data, cfg->spi, 0);
}
#endif

static int spi_esp32_init_dma(const struct device *dev)
{
	const struct spi_esp32_config *cfg = dev->config;
	struct spi_esp32_data *data = dev->data;
	uint8_t channel_offset;

	if (clock_control_on(cfg->clock_dev, (clock_control_subsys_t)cfg->dma_clk_src)) {
		LOG_ERR("Could not enable DMA clock");
		return -EIO;
	}

#ifdef SOC_GDMA_SUPPORTED
	gdma_hal_init(&data->hal_gdma, 0);
	gdma_ll_enable_clock(data->hal_gdma.dev, true);
	gdma_ll_tx_reset_channel(data->hal_gdma.dev, cfg->dma_host);
	gdma_ll_rx_reset_channel(data->hal_gdma.dev, cfg->dma_host);
	gdma_ll_tx_connect_to_periph(data->hal_gdma.dev, cfg->dma_host, cfg->dma_host);
	gdma_ll_rx_connect_to_periph(data->hal_gdma.dev, cfg->dma_host, cfg->dma_host);
	channel_offset = 0;
#else
	channel_offset = 1;
#endif /* SOC_GDMA_SUPPORTED */
#ifdef CONFIG_SOC_SERIES_ESP32
	/*Connect SPI and DMA*/
	DPORT_SET_PERI_REG_BITS(DPORT_SPI_DMA_CHAN_SEL_REG, 3, cfg->dma_host + 1,
				((cfg->dma_host + 1) * 2));
#endif /* CONFIG_SOC_SERIES_ESP32 */

	data->hal_config.dma_in = (spi_dma_dev_t *)cfg->spi;
	data->hal_config.dma_out = (spi_dma_dev_t *)cfg->spi;
	data->hal_config.dma_enabled = true;
	data->hal_config.tx_dma_chan = cfg->dma_host + channel_offset;
	data->hal_config.rx_dma_chan = cfg->dma_host + channel_offset;
	data->hal_config.dmadesc_n = 1;
	data->hal_config.dmadesc_rx = &data->dma_desc_rx;
	data->hal_config.dmadesc_tx = &data->dma_desc_tx;

	if (data->hal_config.dmadesc_tx == NULL || data->hal_config.dmadesc_rx == NULL) {
		k_free(data->hal_config.dmadesc_tx);
		k_free(data->hal_config.dmadesc_rx);
		return -ENOMEM;
	}

	spi_hal_init(&data->hal, cfg->dma_host + 1, &data->hal_config);
	return 0;
}

static int spi_esp32_init(const struct device *dev)
{
	int err;
	const struct spi_esp32_config *cfg = dev->config;
	struct spi_esp32_data *data = dev->data;

	if (!cfg->clock_dev) {
		return -EINVAL;
	}

	if (cfg->dma_enabled) {
		spi_esp32_init_dma(dev);
	}

#ifdef CONFIG_SPI_ESP32_INTERRUPT
	data->irq_line = esp_intr_alloc(cfg->irq_source,
			0,
			(ISR_HANDLER)spi_esp32_isr,
			(void *)dev,
			NULL);
#endif

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static inline uint8_t spi_esp32_get_line_mode(uint16_t operation)
{
	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES)) {
		switch (operation & SPI_LINES_MASK) {
		case SPI_LINES_SINGLE:
			return 1;
		case SPI_LINES_DUAL:
			return 2;
		case SPI_LINES_OCTAL:
			return 8;
		case SPI_LINES_QUAD:
			return 4;
		default:
			break;
		}
	}

	return 1;
}

static int IRAM_ATTR spi_esp32_configure(const struct device *dev,
					 const struct spi_config *spi_cfg)
{
	const struct spi_esp32_config *cfg = dev->config;
	struct spi_esp32_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	spi_hal_context_t *hal = &data->hal;
	spi_hal_dev_config_t *hal_dev = &data->dev_config;
	spi_dev_t *hw = hal->hw;
	int freq;

	if (spi_context_configured(ctx, spi_cfg)) {
		return 0;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* enables SPI peripheral */
	if (clock_control_on(cfg->clock_dev, cfg->clock_subsys)) {
		LOG_ERR("Could not enable SPI clock");
		return -EIO;
	}

	spi_ll_master_init(hal->hw);

	ctx->config = spi_cfg;

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -ENOTSUP;
	}

	hal_dev->cs_pin_id = ctx->config->slave;
	int ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	/* input parameters to calculate timing configuration */
	spi_hal_timing_param_t timing_param = {
		.half_duplex = hal_dev->half_duplex,
		.no_compensate = hal_dev->no_compensate,
		.clock_speed_hz = spi_cfg->frequency,
		.duty_cycle = cfg->duty_cycle == 0 ? 128 : cfg->duty_cycle,
		.input_delay_ns = cfg->input_delay_ns,
		.use_gpio = !cfg->use_iomux,

	};

	spi_hal_cal_clock_conf(&timing_param, &freq, &hal_dev->timing_conf);

	data->trans_config.dummy_bits = hal_dev->timing_conf.timing_dummy;

	hal_dev->tx_lsbfirst = spi_cfg->operation & SPI_TRANSFER_LSB ? 1 : 0;
	hal_dev->rx_lsbfirst = spi_cfg->operation & SPI_TRANSFER_LSB ? 1 : 0;

	data->trans_config.line_mode.data_lines = spi_esp32_get_line_mode(spi_cfg->operation);

	/* multiline for command and address not supported */
	data->trans_config.line_mode.addr_lines = 1;
	data->trans_config.line_mode.cmd_lines = 1;

	/* SPI mode */
	hal_dev->mode = 0;
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		hal_dev->mode = BIT(0);
	}
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
		hal_dev->mode |= BIT(1);
	}

	/* Chip select setup and hold times */
	/* GPIO CS have their own delay parameter*/
	if (!spi_cs_is_gpio(spi_cfg)) {
		hal_dev->cs_hold = cfg->cs_hold;
		hal_dev->cs_setup = cfg->cs_setup;
	}

	spi_hal_setup_device(hal, hal_dev);

	/* Workaround to handle default state of MISO and MOSI lines */
#ifndef CONFIG_SOC_SERIES_ESP32
	if (cfg->line_idle_low) {
		hw->ctrl.d_pol = 0;
		hw->ctrl.q_pol = 0;
	} else {
		hw->ctrl.d_pol = 1;
		hw->ctrl.q_pol = 1;
	}
#endif

	/*
	 * Workaround for ESP32S3 and ESP32C3 SoC. This dummy transaction is needed to sync CLK and
	 * software controlled CS when SPI is in mode 3
	 */
#if defined(CONFIG_SOC_SERIES_ESP32S3) || defined(CONFIG_SOC_SERIES_ESP32C3)
	if (ctx->num_cs_gpios && (hal_dev->mode & (SPI_MODE_CPOL | SPI_MODE_CPHA))) {
		spi_esp32_transfer(dev);
	}
#endif

	return 0;
}

static inline uint8_t spi_esp32_get_frame_size(const struct spi_config *spi_cfg)
{
	uint8_t dfs = SPI_WORD_SIZE_GET(spi_cfg->operation);

	dfs /= 8;
	if ((dfs == 0) || (dfs > 4)) {
		LOG_WRN("Unsupported dfs, 1-byte size will be used");
		dfs = 1;
	}
	return dfs;
}

static int transceive(const struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs, bool asynchronous,
		      spi_callback_t cb,
		      void *userdata)
{
	const struct spi_esp32_config *cfg = dev->config;
	struct spi_esp32_data *data = dev->data;
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_ESP32_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	ret = spi_esp32_configure(dev, spi_cfg);
	if (ret) {
		goto done;
	}

	data->dfs = spi_esp32_get_frame_size(spi_cfg);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);

	spi_context_cs_control(&data->ctx, true);

#ifdef CONFIG_SPI_ESP32_INTERRUPT
	spi_ll_enable_int(cfg->spi);
	spi_ll_set_int_stat(cfg->spi);
#else

	do {
		spi_esp32_transfer(dev);
	} while (spi_esp32_transfer_ongoing(data));

	spi_esp32_complete(dev, data, cfg->spi, 0);

#endif  /* CONFIG_SPI_ESP32_INTERRUPT */

done:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_esp32_transceive(const struct device *dev,
				const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_esp32_transceive_async(const struct device *dev,
				      const struct spi_config *spi_cfg,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs,
				      spi_callback_t cb,
				      void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_esp32_release(const struct device *dev,
			     const struct spi_config *config)
{
	struct spi_esp32_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_api = {
	.transceive = spi_esp32_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_esp32_transceive_async,
#endif
	.release = spi_esp32_release
};

#ifdef CONFIG_SOC_SERIES_ESP32
#define GET_AS_CS(idx) .as_cs = DT_INST_PROP(idx, clk_as_cs),
#else
#define GET_AS_CS(idx)
#endif

#define ESP32_SPI_INIT(idx)	\
				\
	PINCTRL_DT_INST_DEFINE(idx);	\
										\
	static struct spi_esp32_data spi_data_##idx = {	\
		SPI_CONTEXT_INIT_LOCK(spi_data_##idx, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_data_##idx, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(idx), ctx)	\
		.hal = {	\
			.hw = (spi_dev_t *)DT_INST_REG_ADDR(idx),	\
		},	\
		.dev_config = {	\
			.half_duplex = DT_INST_PROP(idx, half_duplex),	\
			GET_AS_CS(idx)							\
			.positive_cs = DT_INST_PROP(idx, positive_cs),	\
			.no_compensate = DT_INST_PROP(idx, dummy_comp),	\
			.sio = DT_INST_PROP(idx, sio)	\
		}	\
	};	\
		\
	static const struct spi_esp32_config spi_config_##idx = {	\
		.spi = (spi_dev_t *)DT_INST_REG_ADDR(idx),	\
			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),	\
		.duty_cycle = 0, \
		.input_delay_ns = 0, \
		.irq_source = DT_INST_IRQN(idx), \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),	\
		.clock_subsys =	\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, offset),	\
		.use_iomux = DT_INST_PROP(idx, use_iomux),	\
		.dma_enabled = DT_INST_PROP(idx, dma_enabled),	\
		.dma_clk_src = DT_INST_PROP(idx, dma_clk),	\
		.dma_host = DT_INST_PROP(idx, dma_host),	\
		.cs_setup = DT_INST_PROP_OR(idx, cs_setup_time, 0), \
		.cs_hold = DT_INST_PROP_OR(idx, cs_hold_time, 0), \
		.line_idle_low = DT_INST_PROP(idx, line_idle_low), \
	};	\
		\
	DEVICE_DT_INST_DEFINE(idx, &spi_esp32_init,	\
			      NULL, &spi_data_##idx,	\
			      &spi_config_##idx, POST_KERNEL,	\
			      CONFIG_SPI_INIT_PRIORITY, &spi_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_SPI_INIT)
