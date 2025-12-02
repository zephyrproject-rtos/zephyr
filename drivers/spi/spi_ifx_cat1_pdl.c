/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_cat1_spi_pdl

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cat1_spi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_IFX_CAT1_SPI_DMA
#include <zephyr/drivers/dma.h>
#endif

#include <cy_scb_spi.h>
#include <cy_trigmux.h>

#define IFX_CAT1_SPI_DEFAULT_OVERSAMPLE (4)
#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define IFX_CAT1_SPI_MIN_DATA_WIDTH (4)
#else
#define IFX_CAT1_SPI_MIN_DATA_WIDTH (8)
#endif
#define IFX_CAT1_SPI_MAX_DATA_WIDTH (32)

#define IFX_CAT1_SPI_OVERSAMPLE_MIN 4
#define IFX_CAT1_SPI_OVERSAMPLE_MAX 16

#define IFX_CAT1_SPI_PENDING_NONE  (0)
#define IFX_CAT1_SPI_PENDING_RX    (1)
#define IFX_CAT1_SPI_PENDING_TX    (2)
#define IFX_CAT1_SPI_PENDING_TX_RX (3)

#define IFX_CAT1_SPI_DEFAULT_SPEED 100000

#define IFX_CAT1_SPI_RSLT_TRANSFER_ERROR (-2)
#define IFX_CAT1_SPI_RSLT_CLOCK_ERROR    (-3)

#if CY_SCB_DRV_VERSION_MINOR >= 20 && defined(COMPONENT_CAT1) && CY_SCB_DRV_VERSION_MAJOR == 3
#define IFX_CAT1_SPI_ASYMM_PDL_FUNC_AVAIL
#endif

#ifdef CONFIG_IFX_CAT1_SPI_DMA
/* dummy buffers to be used by driver for DMA operations when app gives a NULL buffer
 * during an asymmetric transfer
 */
static uint32_t tx_dummy_data;
static uint32_t rx_dummy_data;
#endif

typedef void (*ifx_cat1_spi_event_callback_t)(void *callback_arg, uint32_t event);

/* Device config structure */
struct ifx_cat1_spi_config {
	CySCB_Type *reg_addr;
	const struct pinctrl_dev_config *pcfg;
	cy_stc_scb_spi_config_t scb_spi_config;
	cy_cb_scb_spi_handle_events_t spi_handle_events_func;

	uint32_t irq_num;
	void (*irq_config_func)(const struct device *dev);
	cy_stc_syspm_callback_params_t spi_deep_sleep_param;

	uint8_t cs_oversample[32];
	uint8_t cs_oversample_cnt;
};

#ifdef CONFIG_IFX_CAT1_SPI_DMA
struct ifx_cat1_dma_stream {
	const struct device *dev_dma;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
};
#endif

typedef struct {
	cy_israddress callback;
	void *callback_arg;
} ifx_cat1_event_callback_data_t;

/* Data structure */
struct ifx_cat1_spi_data {
	struct spi_context ctx;
	uint8_t dfs_value;
	size_t chunk_len;
	bool dma_configured;

#ifdef CONFIG_IFX_CAT1_SPI_DMA
	struct ifx_cat1_dma_stream dma_rx;
	struct ifx_cat1_dma_stream dma_tx;
	en_peri0_trig_input_pdma0_tr_t spi_rx_trigger;
	en_peri0_trig_output_pdma0_tr_t dma_rx_trigger;
#endif

#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C) || defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	uint8_t clock_peri_group;
#endif

	struct ifx_cat1_resource_inst resource;
	struct ifx_cat1_clock clock;
	cy_en_scb_spi_sclk_mode_t clk_mode;
	uint8_t data_bits;
	bool is_slave;
	uint8_t oversample_value;
	bool msb_first;
	cy_stc_scb_spi_context_t context;
	uint32_t irq_cause;

	uint16_t volatile pending;

	uint8_t write_fill;
	bool is_async;
	void *rx_buffer;
	uint32_t rx_buffer_size;
	const void *tx_buffer;
	uint32_t tx_buffer_size;
	ifx_cat1_event_callback_data_t callback_data;
	cy_stc_syspm_callback_t spi_deep_sleep;
};

cy_rslt_t ifx_cat1_spi_init_cfg(const struct device *dev, cy_stc_scb_spi_config_t *scb_spi_config);
void ifx_cat1_spi_register_callback(const struct device *dev,
				    ifx_cat1_spi_event_callback_t callback, void *callback_arg);
void spi_free(const struct device *dev);
cy_rslt_t spi_set_frequency(const struct device *dev, uint32_t hz);
static void spi_irq_handler(const struct device *dev);
static void ifx_cat1_spi_cb_wrapper(const struct device *dev, uint32_t event);
cy_rslt_t ifx_cat1_spi_transfer_async(const struct device *dev, const uint8_t *tx, size_t tx_length,
				      uint8_t *rx, size_t rx_length);

int32_t ifx_cat1_uart_get_hw_block_num(CySCB_Type *reg_addr);

static uint8_t get_dfs_value(struct spi_context *ctx)
{
	uint8_t word_size = SPI_WORD_SIZE_GET(ctx->config->operation);

	if (word_size <= 8) {
		return 1;
	} else if (word_size <= 16) {
		return 2;
	} else if (word_size <= 24) {
		return 3;
	} else {
		return 4;
	}
}

static void transfer_chunk(const struct device *dev)
{
	struct ifx_cat1_spi_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t chunk_len = spi_context_max_continuous_chunk(ctx);
	int ret = 0;

	if (chunk_len == 0) {
		goto exit;
	}

	data->chunk_len = chunk_len;

#ifdef CONFIG_IFX_CAT1_SPI_DMA
	const struct ifx_cat1_spi_config *const config = dev->config;
	CySCB_Type *spi_reg = config->reg_addr;

	Cy_SCB_SetRxFifoLevel(spi_reg, chunk_len - 1);

	register struct ifx_cat1_dma_stream *dma_tx = &data->dma_tx;
	register struct ifx_cat1_dma_stream *dma_rx = &data->dma_rx;

	if (data->dma_configured && spi_context_rx_buf_on(ctx) && spi_context_tx_buf_on(ctx)) {
		/* Optimization to reduce config time if only buffer and size
		 * are changing from the previous DMA configuration
		 */
		dma_reload(dma_tx->dev_dma, dma_tx->dma_channel, (uint32_t)ctx->tx_buf,
			   dma_tx->blk_cfg.dest_address, chunk_len);
		dma_reload(dma_rx->dev_dma, dma_rx->dma_channel, dma_rx->blk_cfg.source_address,
			   (uint32_t)ctx->rx_buf, chunk_len);
		return;
	}

	if (spi_context_rx_buf_on(ctx)) {
		dma_rx->blk_cfg.dest_address = (uint32_t)ctx->rx_buf;
		dma_rx->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		dma_rx->blk_cfg.dest_address = (uint32_t)&rx_dummy_data;
		dma_rx->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	if (spi_context_tx_buf_on(ctx)) {
		dma_tx->blk_cfg.source_address = (uint32_t)ctx->tx_buf;
		dma_tx->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	} else {
		tx_dummy_data = 0;
		dma_tx->blk_cfg.source_address = (uint32_t)&tx_dummy_data;
		dma_tx->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	dma_rx->blk_cfg.block_size = dma_tx->blk_cfg.block_size = chunk_len;
	ret = dma_config(dma_rx->dev_dma, dma_rx->dma_channel, &dma_rx->dma_cfg);
	if (ret < 0) {
		goto exit;
	}

	ret = dma_config(dma_tx->dev_dma, dma_tx->dma_channel, &dma_tx->dma_cfg);
	if (ret < 0) {
		goto exit;
	}

#ifdef CONFIG_IFX_CAT1_SPI_DMA_TX_AUTO_TRIGGER
	ret = dma_start(dma_tx->dev_dma, dma_tx->dma_channel);
	if (ret == 0) {
		return;
	}
#else
	if (ret == 0) {
		data->dma_configured = 1;
		return;
	}
#endif
#else
	cy_rslt_t result = ifx_cat1_spi_transfer_async(
		dev, ctx->tx_buf, spi_context_tx_buf_on(ctx) ? chunk_len : 0, ctx->rx_buf,
		spi_context_rx_buf_on(ctx) ? chunk_len : 0);
	if (result == CY_RSLT_SUCCESS) {
		return;
	}
#endif
	ret = -EIO;
exit:
	spi_context_cs_control(ctx, false);
	spi_context_complete(ctx, dev, ret);
}

static void spi_interrupt_callback(void *arg, uint32_t event)
{
	const struct device *dev = (const struct device *)arg;
	struct ifx_cat1_spi_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (event & CY_SCB_SPI_TRANSFER_ERR_EVENT) {
		const struct ifx_cat1_spi_config *const config = dev->config;

		Cy_SCB_SPI_AbortTransfer(config->reg_addr, &(data->context));
		data->pending = IFX_CAT1_SPI_PENDING_NONE;
	}

	if (event & CY_SCB_SPI_TRANSFER_CMPLT_EVENT) {
		spi_context_update_tx(ctx, data->dfs_value, data->chunk_len);
		spi_context_update_rx(ctx, data->dfs_value, data->chunk_len);

		transfer_chunk(dev);
	}
}

#ifdef CONFIG_IFX_CAT1_SPI_DMA
static void dma_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	struct device *dev = arg;
	struct ifx_cat1_spi_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (channel == data->dma_rx.dma_channel) {
		spi_context_update_tx(ctx, get_dfs_value(ctx), data->chunk_len);
		spi_context_update_rx(ctx, get_dfs_value(ctx), data->chunk_len);

		transfer_chunk(dev);
	} else if (channel == data->dma_tx.dma_channel) {

	} else {
		LOG_ERR("Unknown\n");
	}
}
#endif

int spi_config(const struct device *dev, const struct spi_config *spi_cfg)
{
	cy_rslt_t result;
	struct ifx_cat1_spi_data *const data = dev->data;
	const struct ifx_cat1_spi_config *const config = dev->config;
	cy_stc_scb_spi_config_t scb_spi_config = config->scb_spi_config;
	struct spi_context *ctx = &data->ctx;
	bool spi_mode_cpol = false;
	bool spi_mode_cpha = false;

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP) {
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) > IFX_CAT1_SPI_MAX_DATA_WIDTH) {
		LOG_ERR("Word size %d is greater than %d", SPI_WORD_SIZE_GET(spi_cfg->operation),
			IFX_CAT1_SPI_MAX_DATA_WIDTH);
		return -EINVAL;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) < IFX_CAT1_SPI_MIN_DATA_WIDTH) {
		LOG_ERR("Word size %d is less than %d", SPI_WORD_SIZE_GET(spi_cfg->operation),
			IFX_CAT1_SPI_MIN_DATA_WIDTH);
		return -EINVAL;
	}

	/* check if configuration was changed from previous run, if so skip setup again */
	if (spi_context_configured(ctx, spi_cfg)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	/* Store spi config in context */
	ctx->config = spi_cfg;

	if (spi_context_is_slave(ctx)) {
		scb_spi_config.spiMode = CY_SCB_SPI_SLAVE;
		scb_spi_config.oversample = 0;
		scb_spi_config.enableMisoLateSample = false;
	} else {
		scb_spi_config.spiMode = CY_SCB_SPI_MASTER;

		/* If an oversample value for a given target is not defined in the relevant
		 * devicetree/overlay files, the default of four will be used from the
		 * default configuration
		 */
		if (config->cs_oversample_cnt > 0 && spi_cfg->slave < config->cs_oversample_cnt) {
			scb_spi_config.oversample = config->cs_oversample[spi_cfg->slave];
		}
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
		spi_mode_cpol = true;
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		spi_mode_cpha = true;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation)) {
		scb_spi_config.txDataWidth = SPI_WORD_SIZE_GET(spi_cfg->operation);
		scb_spi_config.rxDataWidth = SPI_WORD_SIZE_GET(spi_cfg->operation);
	}

	if (spi_mode_cpha) {
		scb_spi_config.sclkMode =
			spi_mode_cpol ? CY_SCB_SPI_CPHA1_CPOL1 : CY_SCB_SPI_CPHA1_CPOL0;
	} else {
		scb_spi_config.sclkMode =
			spi_mode_cpol ? CY_SCB_SPI_CPHA0_CPOL1 : CY_SCB_SPI_CPHA0_CPOL0;
	}

	scb_spi_config.enableMsbFirst = (spi_cfg->operation & SPI_TRANSFER_LSB) ? false : true;

	/* Force free resource */
	if (config->reg_addr != NULL) {
		spi_free(dev);
	}

	/* Initialize the SPI peripheral */
	result = ifx_cat1_spi_init_cfg(dev, &scb_spi_config);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}

	/* Configure Slave select polarity */
	if (spi_context_is_slave(ctx)) {
		Cy_SCB_SPI_SetActiveSlaveSelectPolarity(config->reg_addr, CY_SCB_SPI_SLAVE_SELECT0,
							scb_spi_config.ssPolarity);
	}

	/* Set the data rate */
	result = spi_set_frequency(dev, spi_cfg->frequency);
	if (result != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	/* Write 0 when NULL buffer is provided for Tx/Rx */
	data->write_fill = 0;

	/* Register common SPI callback */
	ifx_cat1_spi_register_callback(dev, spi_interrupt_callback, (void *)dev);

	/* Enable the spi event */
	data->irq_cause |= CY_SCB_SPI_TRANSFER_CMPLT_EVENT;

	/* Store spi config in context */
	ctx->config = spi_cfg;

	data->dfs_value = get_dfs_value(ctx);

	return 0;
}

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	int result;
	struct ifx_cat1_spi_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_context_lock(ctx, asynchronous, cb, userdata, spi_cfg);

	result = spi_config(dev, spi_cfg);
	if (result) {
		LOG_ERR("Error in SPI Configuration (result: 0x%x)", result);
		spi_context_release(ctx, result);
		return result;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs_value);
	spi_context_cs_control(ctx, true);

	transfer_chunk(dev);
	result = spi_context_wait_for_completion(&data->ctx);

	spi_context_release(ctx, result);

	return result;
}

static int ifx_cat1_spi_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#if defined(CONFIG_SPI_ASYNC)
static int ifx_cat1_spi_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
					 const struct spi_buf_set *tx_bufs,
					 const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					 void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int ifx_cat1_spi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	spi_free(dev);

#ifdef CONFIG_IFX_CAT1_SPI_DMA
	struct ifx_cat1_spi_data *const data = dev->data;

	dma_stop(data->dma_tx.dev_dma, data->dma_tx.dma_channel);
#endif

	return 0;
}

static const struct spi_driver_api ifx_cat1_spi_api = {
	.transceive = ifx_cat1_spi_transceive_sync,
#if defined(CONFIG_SPI_ASYNC)
	.transceive_async = ifx_cat1_spi_transceive_async,
#endif
	.release = ifx_cat1_spi_release,
};

static int ifx_cat1_spi_init(const struct device *dev)
{
	struct ifx_cat1_spi_data *const data = dev->data;
	const struct ifx_cat1_spi_config *const config = dev->config;
	int ret;

	/* Dedicate SCB HW resource */
	data->resource.type = IFX_RSC_SCB;
	data->resource.block_num = ifx_cat1_uart_get_hw_block_num(config->reg_addr);

#ifdef CONFIG_IFX_CAT1_SPI_DMA
	/* spi_rx_trigger is initialized to PERI_0_TRIG_IN_MUX_0_SCB_RX_TR_OUT0,
	 * this is incremented by the resource.block_num to get the trigger for the selected SCB
	 * from the trigmux enumeration (en_peri0_trig_input_pdma0_tr_t)
	 */
	data->spi_rx_trigger += data->resource.block_num;

	if (data->dma_rx.dev_dma != NULL) {
		if (!device_is_ready(data->dma_rx.dev_dma)) {
			return -ENODEV;
		}
		data->dma_rx.blk_cfg.source_address = (uint32_t)(&(config->reg_addr->RX_FIFO_RD));
		data->dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;
		data->dma_rx.dma_cfg.user_data = (void *)dev;
		data->dma_rx.dma_cfg.dma_callback = dma_callback;
	}

	if (data->dma_tx.dev_dma != NULL) {
		if (!device_is_ready(data->dma_tx.dev_dma)) {
			return -ENODEV;
		}
		data->dma_tx.blk_cfg.dest_address = (uint32_t)(&(config->reg_addr->TX_FIFO_WR));
		data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
		data->dma_tx.dma_cfg.user_data = (void *)dev;
		data->dma_tx.dma_cfg.dma_callback = dma_callback;
	}

	Cy_TrigMux_Connect(data->spi_rx_trigger, data->dma_rx_trigger, false, TRIGGER_TYPE_LEVEL);
#endif

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Configure slave select (master) */
	spi_context_cs_configure_all(&data->ctx);

	spi_context_unlock_unconditionally(&data->ctx);

	config->irq_config_func(dev);

#ifdef CONFIG_PM
	Cy_SysPm_RegisterCallback(&data->spi_deep_sleep);
#endif
	return 0;
}

#if defined(CONFIG_IFX_CAT1_SPI_DMA)
#define SPI_DMA_CHANNEL_INIT(index, dir, ch_dir, src_data_size, dst_data_size)                     \
	.dev_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                           \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg = {                                                                               \
		.channel_direction = ch_dir,                                                       \
		.source_data_size = src_data_size,                                                 \
		.dest_data_size = dst_data_size,                                                   \
		.source_burst_length = 0,                                                          \
		.dest_burst_length = 0,                                                            \
		.block_count = 1,                                                                  \
		.complete_callback_en = 1,                                                         \
	},

#define SPI_DMA_CHANNEL(index, dir, ch_dir, src_data_size, dst_data_size)                          \
	.dma_##dir = {COND_CODE_1(                                                                 \
		DT_INST_DMAS_HAS_NAME(index, dir),                                                 \
		(SPI_DMA_CHANNEL_INIT(index, dir, ch_dir, src_data_size, dst_data_size)),          \
		(NULL))},

#define SPI_DMA_TRIGGERS(index)                                                                    \
	.spi_rx_trigger = (en_peri0_trig_input_pdma0_tr_t)(PERI_0_TRIG_IN_MUX_0_SCB_RX_TR_OUT0),   \
	.dma_rx_trigger =                                                                          \
		(en_peri0_trig_output_pdma0_tr_t)(PERI_0_TRIG_OUT_MUX_0_PDMA0_TR_IN0 +             \
						  DT_INST_DMAS_CELL_BY_NAME(index, rx, channel)),
#else
#define SPI_DMA_CHANNEL(index, dir, ch_dir, src_data_size, dst_data_size)
#define SPI_DMA_TRIGGERS(index)
#endif

#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C) || defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define PERI_INFO(n) .clock_peri_group = DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),
#else
#define PERI_INFO(n)
#endif

/* Account for spelling error in older version of the PDL */
#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define EN_XFER_SEPARATION enableTransferSeparation
#else
#define EN_XFER_SEPARATION enableTransferSeperation
#endif

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define SPI_PERI_CLOCK_INIT(n)                                                                     \
	.clock =                                                                                   \
		{                                                                                  \
			.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                 \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 0),         \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),         \
				DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                     \
			.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                    \
	},                                                                                         \
	PERI_INFO(n)
#else
#define SPI_PERI_CLOCK_INIT(n)                                                                     \
	.clock =                                                                                   \
		{                                                                                  \
			.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                 \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),         \
				DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                     \
			.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                    \
	},                                                                                         \
	PERI_INFO(n)
#endif

#define IFX_CAT1_SPI_INIT(n)                                                                       \
                                                                                                   \
	void spi_handle_events_func_##n(uint32_t event)                                            \
	{                                                                                          \
		ifx_cat1_spi_cb_wrapper(DEVICE_DT_INST_GET(n), event);                             \
	}                                                                                          \
                                                                                                   \
	static void ifx_cat1_spi_irq_config_func_##n(const struct device *dev)                     \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_irq_handler,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
	}                                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct ifx_cat1_spi_config spi_cat1_config_##n = {                                  \
		.reg_addr = (CySCB_Type *)DT_INST_REG_ADDR(n),                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.cs_oversample_cnt = DT_INST_PROP_LEN_OR(n, oversample, 0),                        \
		.cs_oversample = DT_INST_PROP_OR(n, oversample, {0}),                              \
		.scb_spi_config =                                                                  \
			{.spiMode = CY_SCB_SPI_MASTER,       /* overwrite by cfg  */               \
			 .sclkMode = CY_SCB_SPI_CPHA0_CPOL0, /* overwrite by cfg  */               \
			 .rxDataWidth = 8,                   /* overwrite by cfg  */               \
			 .txDataWidth = 8,                   /* overwrite by cfg  */               \
			 .enableMsbFirst = true,             /* overwrite by cfg  */               \
			 .subMode = DT_INST_PROP_OR(n, sub_mode, CY_SCB_SPI_MOTOROLA),             \
			 .oversample = IFX_CAT1_SPI_DEFAULT_OVERSAMPLE,                            \
			 .enableFreeRunSclk = DT_INST_PROP_OR(n, enable_free_run_sclk, false),     \
			 .enableInputFilter = DT_INST_PROP_OR(n, enable_input_filter, false),      \
			 .enableMisoLateSample =                                                   \
				 DT_INST_PROP_OR(n, enable_miso_late_sample, true),                \
			 .EN_XFER_SEPARATION =                                                     \
				 DT_INST_PROP_OR(n, enable_transfer_separation, false),            \
			 .enableWakeFromSleep = DT_INST_PROP_OR(n, enableWakeFromSleep, false),    \
			 .ssPolarity = DT_INST_PROP_OR(n, ss_polarity, CY_SCB_SPI_ACTIVE_LOW),     \
			 .rxFifoTriggerLevel = DT_INST_PROP_OR(n, rx_fifo_trigger_level, 0),       \
			 .rxFifoIntEnableMask = DT_INST_PROP_OR(n, rx_fifo_int_enable_mask, 0),    \
			 .txFifoTriggerLevel = DT_INST_PROP_OR(n, tx_fifo_trigger_level, 0),       \
			 .txFifoIntEnableMask = DT_INST_PROP_OR(n, tx_fifo_int_enable_mask, 0),    \
			 .masterSlaveIntEnableMask =                                               \
				 DT_INST_PROP_OR(n, master_slave_int_enable_mask, 0)},             \
                                                                                                   \
		.irq_num = DT_INST_IRQN(n),                                                        \
		.irq_config_func = ifx_cat1_spi_irq_config_func_##n,                               \
                                                                                                   \
		.spi_handle_events_func = spi_handle_events_func_##n,                              \
		.spi_deep_sleep_param = {(CySCB_Type *)DT_INST_REG_ADDR(n), NULL},                 \
	};                                                                                         \
                                                                                                   \
	static struct ifx_cat1_spi_data spi_cat1_data_##n = {                                      \
		SPI_CONTEXT_INIT_LOCK(spi_cat1_data_##n, ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(spi_cat1_data_##n, ctx),                                     \
		SPI_DMA_CHANNEL(n, tx, MEMORY_TO_PERIPHERAL, 1, 1)                                 \
			SPI_DMA_CHANNEL(n, rx, PERIPHERAL_TO_MEMORY, 1, 1) SPI_DMA_TRIGGERS(n)     \
				SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)               \
					SPI_PERI_CLOCK_INIT(n)                                     \
						.spi_deep_sleep = {                                \
			&Cy_SCB_SPI_DeepSleepCallback, CY_SYSPM_DEEPSLEEP,                         \
			CY_SYSPM_SKIP_BEFORE_TRANSITION,                                           \
			&spi_cat1_config_##n.spi_deep_sleep_param, NULL, NULL, 1}};                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_cat1_spi_init, NULL, &spi_cat1_data_##n,                     \
			      &spi_cat1_config_##n, POST_KERNEL,                                   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ifx_cat1_spi_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_CAT1_SPI_INIT)

cy_rslt_t ifx_cat1_spi_transfer_async(const struct device *dev, const uint8_t *tx, size_t tx_length,
				      uint8_t *rx, size_t rx_length)
{
	struct ifx_cat1_spi_data *const data = dev->data;
	const struct ifx_cat1_spi_config *const config = dev->config;

	cy_en_scb_spi_status_t spi_status;

	data->is_async = true;

	size_t tx_words = tx_length;
	size_t rx_words = rx_length;

	/* Setup transfer */
	data->rx_buffer = NULL;
	data->tx_buffer = NULL;

#if !defined(IFX_CAT1_SPI_ASYMM_PDL_FUNC_AVAIL)
	if (tx_words > rx_words) {
		if (rx_words > 0) {
			/* I) write + read, II) write only */
			data->pending = IFX_CAT1_SPI_PENDING_TX_RX;

			data->tx_buffer = tx + (rx_words);
			data->tx_buffer_size = tx_words - rx_words;

			tx_words = rx_words; /* Use tx_words to store entire transfer length */
		} else {
			/*  I) write only */
			data->pending = IFX_CAT1_SPI_PENDING_TX;

			rx = NULL;
		}
	} else if (rx_words > tx_words) {
		if (tx_words > 0) {
			/*  I) write + read, II) read only */
			data->pending = IFX_CAT1_SPI_PENDING_TX_RX;

			data->rx_buffer = rx + (tx_words);
			data->rx_buffer_size = rx_words - tx_words;
		} else {
			/*  I) read only. */
			data->pending = IFX_CAT1_SPI_PENDING_RX;

			data->rx_buffer = rx_words > 1 ? rx + 1 : NULL;
			data->rx_buffer_size = rx_words - 1;
			tx = &data->write_fill;
			tx_words = 1;
		}
	} else {
		/* RX and TX of the same size: I) write + read. */
		data->pending = IFX_CAT1_SPI_PENDING_TX_RX;
	}
	spi_status =
		Cy_SCB_SPI_Transfer(config->reg_addr, (void *)tx, rx, tx_words, &data->context);
#else /* !defined(IFX_CAT1_SPI_ASYMM_PDL_FUNC_AVAIL) */

	if (tx_words != rx_words) {
		if (tx_words == 0) {
			data->pending = IFX_CAT1_SPI_PENDING_RX;
			tx = NULL;
		} else if (rx_words == 0) {
			data->pending = IFX_CAT1_SPI_PENDING_TX;
			rx = NULL;
		} else {
			data->pending = IFX_CAT1_SPI_PENDING_TX_RX;
		}
		spi_status = Cy_SCB_SPI_Transfer_Buffer(config->reg_addr, (void *)tx, (void *)rx,
							tx_words, rx_words, data->write_fill,
							&data->context);
	} else {
		data->pending = IFX_CAT1_SPI_PENDING_TX_RX;
		spi_status = Cy_SCB_SPI_Transfer(config->reg_addr, (void *)tx, rx, tx_words,
						 &data->context);
	}

#endif /* IFX_CAT1_SPI_ASYMM_PDL_FUNC_AVAIL */
	return spi_status == CY_SCB_SPI_SUCCESS ? CY_RSLT_SUCCESS
						: IFX_CAT1_SPI_RSLT_TRANSFER_ERROR;
}

bool ifx_cat1_spi_is_busy(const struct device *dev)
{
	struct ifx_cat1_spi_data *const data = dev->data;
	const struct ifx_cat1_spi_config *const config = dev->config;

	return Cy_SCB_SPI_IsBusBusy(config->reg_addr) ||
	       (data->pending != IFX_CAT1_SPI_PENDING_NONE);
}

cy_rslt_t ifx_cat1_spi_abort_async(const struct device *dev)
{
	struct ifx_cat1_spi_data *const data = dev->data;
	const struct ifx_cat1_spi_config *const config = dev->config;

	Cy_SCB_SPI_AbortTransfer(config->reg_addr, &(data->context));
	data->pending = IFX_CAT1_SPI_PENDING_NONE;
	return CY_RSLT_SUCCESS;
}

/* Registers a callback function, which notifies that
 * SPI events occurred in the Cy_SCB_SPI_Interrupt.
 */
void ifx_cat1_spi_register_callback(const struct device *dev,
				    ifx_cat1_spi_event_callback_t callback, void *callback_arg)
{
	/* TODO: we need rework to removecallback_data structure */

	struct ifx_cat1_spi_data *const data = dev->data;
	const struct ifx_cat1_spi_config *const config = dev->config;

	uint32_t savedIntrStatus = Cy_SysLib_EnterCriticalSection();

	data->callback_data.callback = (cy_israddress)callback;
	data->callback_data.callback_arg = callback_arg;
	Cy_SysLib_ExitCriticalSection(savedIntrStatus);
	Cy_SCB_SPI_RegisterCallback(config->reg_addr, config->spi_handle_events_func,
				    &(data->context));

	data->irq_cause = 0;
}

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define IFX_CAT1_INSTANCE_GROUP(instance, group) (((instance) << 4) | (group))
#endif

static uint8_t ifx_cat1_get_hfclk_for_peri_group(uint8_t peri_group)
{
#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	switch (peri_group) {
	case IFX_CAT1_INSTANCE_GROUP(0, 0):
	case IFX_CAT1_INSTANCE_GROUP(1, 4):
		return CLK_HF0;
	case IFX_CAT1_INSTANCE_GROUP(0, 7):
	case IFX_CAT1_INSTANCE_GROUP(1, 0):
		return CLK_HF1;
	case IFX_CAT1_INSTANCE_GROUP(0, 3):
	case IFX_CAT1_INSTANCE_GROUP(1, 2):
		return CLK_HF5;
	case IFX_CAT1_INSTANCE_GROUP(0, 4):
	case IFX_CAT1_INSTANCE_GROUP(1, 3):
		return CLK_HF6;
	case IFX_CAT1_INSTANCE_GROUP(1, 1):
		return CLK_HF7;
	case IFX_CAT1_INSTANCE_GROUP(0, 2):
		return CLK_HF9;
	case IFX_CAT1_INSTANCE_GROUP(0, 1):
	case IFX_CAT1_INSTANCE_GROUP(0, 5):
		return CLK_HF10;
	case IFX_CAT1_INSTANCE_GROUP(0, 8):
		return CLK_HF11;
	case IFX_CAT1_INSTANCE_GROUP(0, 6):
	case IFX_CAT1_INSTANCE_GROUP(0, 9):
		return CLK_HF13;
	default:
		return -EINVAL;
	}
#elif defined(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
	switch (peri_group) {
	case 0:
	case 2:
		return CLK_HF0;
	case 1:
	case 3:
		return CLK_HF1;
	case 4:
		return CLK_HF2;
	case 5:
		return CLK_HF3;
	case 6:
		return CLK_HF4;
	default:
		return -EINVAL;
	}
#endif
	return -EINVAL;
}

static cy_rslt_t ifx_cat1_spi_int_frequency(const struct device *dev, uint32_t hz,
					    uint8_t *over_sample_val)
{

	struct ifx_cat1_spi_data *const data = dev->data;

	cy_rslt_t result = CY_RSLT_SUCCESS;
	uint8_t oversample_value;
	uint32_t divider_value;
	uint32_t last_diff = 0xFFFFFFFFU;
	uint8_t last_ovrsmpl_val = 0;
	uint32_t last_dvdr_val = 0;
	uint32_t oversampled_freq = 0;
	uint32_t divided_freq = 0;
	uint32_t diff = 0;

#if defined(COMPONENT_CAT1A)
	uint32_t peri_freq = Cy_SysClk_ClkPeriGetFrequency();
#elif defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C) ||                                      \
	defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	uint8_t hfclk = ifx_cat1_get_hfclk_for_peri_group(data->clock_peri_group);

	uint32_t peri_freq = Cy_SysClk_ClkHfGetFrequency(hfclk);
#endif

	if (!data->is_slave) {
		for (oversample_value = IFX_CAT1_SPI_OVERSAMPLE_MIN;
		     oversample_value <= IFX_CAT1_SPI_OVERSAMPLE_MAX; oversample_value++) {
			oversampled_freq = hz * oversample_value;
			if ((hz * oversample_value > peri_freq) &&
			    (IFX_CAT1_SPI_OVERSAMPLE_MIN == oversample_value)) {
				return IFX_CAT1_SPI_RSLT_CLOCK_ERROR;
			} else if (hz * oversample_value > peri_freq) {
				continue;
			}

			divider_value = ((peri_freq + ((hz * oversample_value) / 2)) /
					 (hz * oversample_value));
			divided_freq = peri_freq / divider_value;
			diff = (oversampled_freq > divided_freq) ? oversampled_freq - divided_freq
								 : divided_freq - oversampled_freq;

			if (diff < last_diff) {
				last_diff = diff;
				last_ovrsmpl_val = oversample_value;
				last_dvdr_val = divider_value;
				if (diff == 0) {
					break;
				}
			}
		}
		*over_sample_val = last_ovrsmpl_val;
	} else {
		/* Slave requires such frequency: required_frequency = N / ((0.5 * desired_period)
		 * – 20 nsec - tDSI, N is 3 when "Enable Input Glitch Filter" is false and 4 when
		 * true. tDSI Is external master delay which is assumed to be 16.66 nsec
		 */

		/* Divided by 2 desired period to avoid dividing in required_frequency formula */
		cy_float32_t desired_period_us_divided = 5e5f * (1 / (cy_float32_t)hz);
		uint32_t required_frequency =
			(uint32_t)(3e6f / (desired_period_us_divided - 36.66f / 1e3f));

		if (required_frequency > peri_freq) {
			return IFX_CAT1_SPI_RSLT_CLOCK_ERROR;
		}

		last_dvdr_val = 1;
		CY_UNUSED_PARAMETER(last_ovrsmpl_val);
	}

	en_clk_dst_t clk_idx = ifx_cat1_scb_get_clock_index(data->resource.block_num);

	if ((data->clock.block & 0x02) == 0) {
		result = ifx_cat1_utils_peri_pclk_set_divider(clk_idx, &(data->clock),
							      last_dvdr_val - 1);
	} else {
		result = ifx_cat1_utils_peri_pclk_set_frac_divider(clk_idx, &(data->clock),
								   last_dvdr_val - 1, 0);
	}

	return result;
}

cy_rslt_t spi_set_frequency(const struct device *dev, uint32_t hz)
{

	struct ifx_cat1_spi_data *const data = dev->data;
	const struct ifx_cat1_spi_config *const config = dev->config;

	cy_rslt_t result = CY_RSLT_SUCCESS;
	cy_rslt_t scb_init_result = CY_RSLT_SUCCESS;
	uint8_t ovr_sample_val;

	Cy_SCB_SPI_Disable(config->reg_addr, &data->context);
	result = ifx_cat1_spi_int_frequency(dev, hz, &ovr_sample_val);

	/* No need to reconfigure slave since oversample value, that was changed in
	 * ifx_cat1_spi_int_frequency, in slave is ignored
	 */
	if ((CY_RSLT_SUCCESS == result) && !data->is_slave &&
	    (data->oversample_value != ovr_sample_val)) {
		cy_stc_scb_spi_config_t config_structure = config->scb_spi_config;

		Cy_SCB_SPI_DeInit(config->reg_addr);
		config_structure.spiMode =
			data->is_slave == false ? CY_SCB_SPI_MASTER : CY_SCB_SPI_SLAVE;
		config_structure.enableMsbFirst = data->msb_first;
		config_structure.sclkMode = data->clk_mode;
		config_structure.rxDataWidth = data->data_bits;
		config_structure.txDataWidth = data->data_bits;
		config_structure.oversample = ovr_sample_val;
		data->oversample_value = ovr_sample_val;
		scb_init_result = (cy_rslt_t)Cy_SCB_SPI_Init(config->reg_addr, &config_structure,
							     &(data->context));
	}
	if (CY_RSLT_SUCCESS == scb_init_result) {
		Cy_SCB_SPI_Enable(config->reg_addr);
	}

	return result;
}

static cy_rslt_t spi_init_hw(const struct device *dev, cy_stc_scb_spi_config_t *cfg)
{
	cy_rslt_t result = CY_RSLT_SUCCESS;
	struct ifx_cat1_spi_data *const data = dev->data;
	const struct ifx_cat1_spi_config *const config = dev->config;

	data->oversample_value = cfg->oversample;
	data->data_bits = cfg->txDataWidth;
	data->msb_first = cfg->enableMsbFirst;
	data->clk_mode = cfg->sclkMode;

	result = (cy_rslt_t)Cy_SCB_SPI_Init(config->reg_addr, cfg, &(data->context));

	if (result == CY_RSLT_SUCCESS) {
		data->callback_data.callback = NULL;
		data->callback_data.callback_arg = NULL;
		data->irq_cause = 0;

		irq_enable(config->irq_num);
		Cy_SCB_SPI_Enable(config->reg_addr);
	} else {
		spi_free(dev);
	}
	return result;
}

cy_rslt_t ifx_cat1_spi_init_cfg(const struct device *dev, cy_stc_scb_spi_config_t *scb_spi_config)
{
	struct ifx_cat1_spi_data *const data = dev->data;

	cy_stc_scb_spi_config_t cfg_local = *scb_spi_config;

	cy_rslt_t result = CY_RSLT_SUCCESS;
	bool is_slave = (cfg_local.spiMode == CY_SCB_SPI_SLAVE);

	data->is_slave = is_slave;
	data->write_fill = (uint8_t)CY_SCB_SPI_DEFAULT_TX;

	result = ifx_cat1_spi_int_frequency(dev, IFX_CAT1_SPI_DEFAULT_SPEED,
					    &data->oversample_value);

	if (result == CY_RSLT_SUCCESS) {
		result = spi_init_hw(dev, &cfg_local);
	}

	if (result != CY_RSLT_SUCCESS) {
		spi_free(dev);
	}
	return result;
}

void spi_free(const struct device *dev)
{
	const struct ifx_cat1_spi_config *const config = dev->config;

	Cy_SCB_SPI_Disable(config->reg_addr, NULL);
	Cy_SCB_SPI_DeInit(config->reg_addr);
	irq_disable(config->irq_num);
}

static void spi_irq_handler(const struct device *dev)
{
	struct ifx_cat1_spi_data *const data = dev->data;
	const struct ifx_cat1_spi_config *const config = dev->config;

	Cy_SCB_SPI_Interrupt(config->reg_addr, &(data->context));

	if (!data->is_async) {
		if (CY_SCB_MASTER_INTR_SPI_DONE &
		    Cy_SCB_GetMasterInterruptStatusMasked(config->reg_addr)) {
			Cy_SCB_SetMasterInterruptMask(
				config->reg_addr, (Cy_SCB_GetMasterInterruptMask(config->reg_addr) &
						   (uint32_t)~CY_SCB_MASTER_INTR_SPI_DONE));
			Cy_SCB_ClearMasterInterrupt(config->reg_addr, CY_SCB_MASTER_INTR_SPI_DONE);
		}
		return;
	}

	if (0 == (Cy_SCB_SPI_GetTransferStatus(config->reg_addr, &data->context) &
		  CY_SCB_SPI_TRANSFER_ACTIVE)) {

#if !defined(IFX_CAT1_SPI_ASYMM_PDL_FUNC_AVAIL)
		if (NULL != data->tx_buffer) {
			/* Start TX Transfer */
			data->pending = IFX_CAT1_SPI_PENDING_TX;
			const uint8_t *buf = data->tx_buffer;

			data->tx_buffer = NULL;

			Cy_SCB_SPI_Transfer(config->reg_addr, (uint8_t *)buf, NULL,
					    data->tx_buffer_size, &data->context);
		} else if (NULL != data->rx_buffer) {
			/* Start RX Transfer */
			data->pending = IFX_CAT1_SPI_PENDING_RX;
			uint8_t *rx_buf = data->rx_buffer;
			uint8_t *tx_buf;
			size_t trx_size = data->rx_buffer_size;

			if (data->rx_buffer_size > 1) {
				/* In this case we don't have a transmit buffer; we only have a
				 * receive buffer. While the PDL is fine with passing NULL for
				 * transmit, we don't get to control what data it is sending in that
				 * case, which we allowed the user to set. To honor the user's
				 * request, we reuse the rx buffer as the tx buffer too. We set all
				 * bytes beyond the one we will start filling in with the user
				 * provided 'write_fill'. This means the tx buffer is 1 element
				 * smaller than the rx buffer. As a result, we must therefore
				 * transfer 1 less element then we really want to in this transfer.
				 * When this transfer is complete, it will call into this again to
				 * receive the final element.
				 */
				trx_size -=
					1; /* Transfer everything left except for the last byte*/

				uint8_t **rx_buffer_p = (uint8_t **)&data->rx_buffer;

				/* Start at second byte to avoid trying
				 * to transmit and receive the same byte
				 */
				tx_buf = *rx_buffer_p + 1;

				memset(tx_buf, data->write_fill, trx_size);
				/* Move to 1 byte before end */
				*rx_buffer_p += trx_size;

				/* Transfer the last byte on the next interrupt */
				data->rx_buffer_size = 1;
			} else {
				tx_buf = &data->write_fill;

				data->rx_buffer = NULL;
			}

			Cy_SCB_SPI_Transfer(config->reg_addr, tx_buf, rx_buf, trx_size,
					    &data->context);
		} else {
#endif /* IFX_CAT1_SPI_ASYMM_PDL_FUNC_AVAIL */
			/* Finish Async Transfer */
			data->pending = IFX_CAT1_SPI_PENDING_NONE;
			data->is_async = false;
#if !defined(IFX_CAT1_SPI_ASYMM_PDL_FUNC_AVAIL)
		}
#endif /* IFX_CAT1_SPI_ASYMM_PDL_FUNC_AVAIL */
	}
}

static void ifx_cat1_spi_cb_wrapper(const struct device *dev, uint32_t event)
{
	struct ifx_cat1_spi_data *const data = dev->data;
	uint32_t anded_events = (data->irq_cause & ((uint32_t)event));

	/* Don't call the callback until the final transfer
	 * has put everything in the FIFO/completed
	 */
	if ((anded_events &
	     (CY_SCB_SPI_TRANSFER_IN_FIFO_EVENT | CY_SCB_SPI_TRANSFER_CMPLT_EVENT)) &&
	    !(data->rx_buffer == NULL && data->tx_buffer == NULL)) {
		return;
	}

	if (anded_events) {
		ifx_cat1_spi_event_callback_t callback =
			(ifx_cat1_spi_event_callback_t)data->callback_data.callback;
		callback(data->callback_data.callback_arg, anded_events);
	}
}
